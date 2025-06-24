#include <omp.h>
#include <cstring>
#include <boost/program_options.hpp>
#include <sys/mman.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <set>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "memory_mapper.h"
#include "ann_exception.h"
#include "index_factory.h"
#include "index.h"
#include "memory_mapper.h"
#include "utils.h"
#include "program_options_utils.hpp"
#include "index_factory.h"
#include "abstract_index.h" 
#include "utils.h"

namespace po = boost::program_options;

template <typename T, typename TagT = uint32_t>
void search(std::unique_ptr<diskann::AbstractIndex>& index, 
            const std::string& query_file,
            const std::string& truthset_file,
            uint32_t num_threads,
            uint32_t K,
            uint32_t L
            ) {
    T* queries = nullptr;
    TagT* groundtruth_ids = nullptr;
    float *groundtruth_dists = nullptr;
    size_t n_query, query_dim, query_aligned_dim, n_groundtruth, groundtruth_dim;
    diskann::load_aligned_bin<T>(query_file, queries, n_query, query_dim, query_aligned_dim);
    diskann::load_truthset(truthset_file, groundtruth_ids, groundtruth_dists, n_groundtruth, groundtruth_dim);

    std::vector<uint32_t> query_result_ids(K * n_query);
    std::vector<uint64_t> latency_stats(n_query, 0);

    auto search_start = std::chrono::high_resolution_clock::now();

    omp_set_num_threads(num_threads);
    #pragma omp parallel for schedule(dynamic, 1)
    for (int64_t i = 0; i < (int64_t)n_query; i++) {
        auto single_query_start = std::chrono::high_resolution_clock::now();
    
        index->search(
            queries + i * query_aligned_dim, 
            K, L, 
            query_result_ids.data() + i * K
        );
    
        auto single_query_end = std::chrono::high_resolution_clock::now();
        auto single_query_took = std::chrono::duration_cast<std::chrono::microseconds>(single_query_end - single_query_start).count();
        latency_stats[i] = single_query_took;
    }
    
    auto search_end = std::chrono::high_resolution_clock::now();
    auto search_took = std::chrono::duration_cast<std::chrono::duration<float>>(search_end - search_start).count();
    
    float average_qps = n_query / search_took;
    float average_qps_per_thread = average_qps / num_threads;
    float average_latency = std::accumulate(latency_stats.begin(), latency_stats.end(), 0ULL) / (n_query * 1000.0f);
    
    std::cout << "Average QPS using " << num_threads << " threads for search: " << average_qps << std::endl;
    std::cout << "Average QPS per thread: " << average_qps_per_thread << std::endl;
    std::cout << "Average latency: " << average_latency << " ms" << std::endl;

    // Calculate recalls
    double total_recall = 0.0;
    for (int32_t i = 0; i < n_query; i++) {
        std::set<uint32_t> groundtruth_closest_neighbors;
        std::set<uint32_t> calculated_closest_neighbors;
        for (int32_t j = 0; j < K; j++) {
            calculated_closest_neighbors.insert(*(query_result_ids.data() + i * K + j));
            groundtruth_closest_neighbors.insert(*(groundtruth_ids + i * groundtruth_dim + j));
        }
        uint32_t matching_neighbors = 0;
        for (uint32_t x : calculated_closest_neighbors) if (groundtruth_closest_neighbors.count(x)) matching_neighbors++;
        double recall = matching_neighbors / (double)K;
        total_recall += recall;        
    }
    double average_recall = total_recall / (n_query);

    std::cout << K << "Recall@" << K << ": " << average_recall * 100 << "%" << std::endl;
}

template <typename T, typename TagT = uint32_t>
void delete_batch(std::unique_ptr<diskann::AbstractIndex>& index, 
                float batch_size_in_percentage, 
    uint32_t n_vectors) {
    std::cout << n_vectors * batch_size_in_percentage / 100 << " vectors will be deleted" << std::endl;
}

int main(int argc, char **argv) {
    std::string data_type, data_path, index_path, query_path, groundtruth_path;
    uint32_t insert_threads, consolidate_threads, search_threads, build_threads, R, L, K, n_iterations;
    float alpha, start_point_norm, batch_size_in_percentage;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("data_type", po::value<std::string>(&data_type)->required(), "Data type")
        ("data_path", po::value<std::string>(&data_path)->required(), "Path to data")
        ("index_path", po::value<std::string>(&index_path)->required(), "Path to index")
        ("query_path", po::value<std::string>(&query_path)->required(), "Path to queries")
        ("groundtruth_path", po::value<std::string>(&groundtruth_path)->required(), "Path to groundtruth")
        ("insert_threads", po::value<uint32_t>(&insert_threads)->required(), "Insert threads")
        ("search_threads", po::value<uint32_t>(&search_threads)->required(), "Search threads")
        ("build_threads", po::value<uint32_t>(&build_threads)->required(), "Build threads")
        ("consolidate_threads", po::value<uint32_t>(&consolidate_threads)->required(), "Consolidate threads")
        ("R", po::value<uint32_t>(&R)->required(), "R parameter")
        ("L", po::value<uint32_t>(&L)->required(), "L parameter")
        ("K", po::value<uint32_t>(&K)->required(), "K parameter")
        ("batch_size", po::value<float>(&batch_size_in_percentage)->required(), "Batch size in percentage")
        ("iterations", po::value<uint32_t>(&n_iterations)->required(), "Number of iterations")
        ("alpha", po::value<float>(&alpha)->required(), "Alpha")
        ("start_point_norm", po::value<float>(&start_point_norm)->required(), "Start point norm")
    ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm); // Throws on missing required options
    } 
    catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }


    // Support only L2 distance function.
    diskann::Metric metric = diskann::Metric::INNER_PRODUCT;

    // Build the index in memory
    std::unique_ptr<diskann::AbstractIndex> index;
    uint32_t n_vectors = 0;

    try {
        diskann::cout << "Starting index build with R: " << R << "  Lbuild: " << L << "  alpha: " << alpha
                      << "  #threads: " << insert_threads << std::endl;

        size_t data_num, data_dim;
        diskann::get_bin_metadata(data_path, data_num, data_dim);
        n_vectors = data_num;

        auto index_build_params = diskann::IndexWriteParametersBuilder(L, R)
                                      .with_filter_list_size(L)
                                      .with_alpha(alpha)
                                      .with_saturate_graph(false)
                                      .with_num_threads(build_threads)
                                      .build();

        auto filter_params = diskann::IndexFilterParamsBuilder()
                                 .with_universal_label("")
                                 .with_label_file("")
                                 .with_save_path_prefix(index_path)
                                 .build();


        auto config = diskann::IndexConfigBuilder()
                          .with_metric(metric)
                          .with_dimension(data_dim)
                          .with_max_points(data_num)
                          .with_data_load_store_strategy(diskann::DataStoreStrategy::MEMORY)
                          .with_graph_load_store_strategy(diskann::GraphStoreStrategy::MEMORY)
                          .with_data_type(data_type)
                          .with_label_type("uint")
                          .is_dynamic_index(false)
                          .with_index_write_params(index_build_params)
                          .is_enable_tags(false)
                          .is_use_opq(false)
                          .is_pq_dist_build(false)
                          .with_num_pq_chunks(0)
                          .build();

        auto index_factory = diskann::IndexFactory(config);
        index = index_factory.create_instance();
        index->build(data_path, data_num, filter_params);
        index->save(index_path.c_str());
    }
    catch (const std::exception &e) {
        std::cout << std::string(e.what()) << std::endl;
        diskann::cerr << "Index build failed." << std::endl;
        return -1;
    }

    std::cout << std::endl << "=================== Initial Search ===================" << std::endl;
    try {
        if (data_type == std::string("float")) search<float>(index, query_path, groundtruth_path, search_threads, K, L);
        else if (data_type == std::string("uint8")) search<uint8_t>(index, query_path, groundtruth_path, search_threads, K, L);
        else if (data_type == std::string("int8")) search<int8_t>(index, query_path, groundtruth_path, search_threads, K, L);
        else {
            std::cout << "Unsupported type. Use float/int8/uint8" << std::endl;
            return -1;
        }   
    }
    catch (std::exception &e) {
        std::cout << std::string(e.what()) << std::endl;
        diskann::cerr << "Index search failed." << std::endl;
        return -1;
    }
    std::cout << "======================================================" << std::endl;
 


    diskann::IndexWriteParameters delete_params = diskann::IndexWriteParametersBuilder(L, R)
                                                    .with_alpha(alpha)
                                                    .with_num_threads(consolidate_threads)
                                                    .build();

    for (uint32_t i = 0; i < n_iterations; i++) {
        if (data_type == std::string("float")) delete_batch<float>(index, batch_size_in_percentage, n_vectors);
        else if (data_type == std::string("uint8")) delete_batch<uint8_t>(index, batch_size_in_percentage, n_vectors);
        else if (data_type == std::string("int8")) delete_batch<int8_t>(index, batch_size_in_percentage, n_vectors);
    }


}
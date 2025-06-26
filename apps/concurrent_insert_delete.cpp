// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <index.h>
#include <numeric>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <timer.h>
#include <boost/program_options.hpp>
#include <future>

#include "utils.h"
#include "filter_utils.h"
#include "program_options_utils.hpp"
#include "index_factory.h"

#include "memory_mapper.h"

namespace po = boost::program_options;

template <typename T, typename TagT = uint32_t>
void build(diskann::AbstractIndex &index, size_t end, int32_t thread_count, T *data, size_t aligned_dim) {
    #pragma omp parallel for num_threads(thread_count) schedule(dynamic)
    for (int64_t j = 0; j < (int64_t)end; j++) {
        index.insert_point(data + j * aligned_dim, 1 + static_cast<TagT>(j));
    }
    std::cout << "built using [" << 0 << "," << end-1 << "]" << std::endl;
}

template <typename T, typename TagT = uint32_t>
void insert(diskann::AbstractIndex &index, size_t start, size_t end, int32_t thread_count, T *data, size_t aligned_dim) {
    #pragma omp parallel for num_threads(thread_count) schedule(dynamic)
    for (int64_t j = start; j < (int64_t)end; j++) {
        index.insert_point(data + j * aligned_dim, 1 + static_cast<TagT>(j));
    }
    std::cout << "inserted [" << start << "," << end-1 << "]" << std::endl;
}

template <typename T, typename TagT = uint32_t>
void delete_and_consolidate(diskann::AbstractIndex &index, const diskann::IndexWriteParameters &delete_params, size_t start, size_t end){
    for (size_t i = start; i < end; i++)
        index.lazy_delete(static_cast<TagT>(1 + i));

    index.consolidate_deletes(delete_params);
    std::cout << "deleted and consolidated [" << start << "," << end-1 << "]" << std::endl;
}

template <typename T, typename TagT = uint32_t>
void experiment(const std::string &data_path, 
                size_t chunk_size,
                uint32_t R, uint32_t L, float alpha,
                uint32_t build_threads, uint32_t insert_threads,
                uint32_t consolidate_threads, uint32_t search_threads) {

    size_t dim, aligned_dim;
    size_t num_points;
    diskann::get_bin_metadata(data_path, num_points, dim);
    aligned_dim = ROUND_UP(dim, 8);
    size_t num_chunks = (num_points / chunk_size);

    diskann::IndexWriteParameters params = diskann::IndexWriteParametersBuilder(L, R)
                                                .with_alpha(alpha)
                                                .with_num_threads(consolidate_threads)
                                                .build();

    auto index_search_params = diskann::IndexSearchParams(L, search_threads);

    diskann::IndexConfig index_config = diskann::IndexConfigBuilder()
                                            .with_metric(diskann::L2)
                                            .with_dimension(dim)
                                            .with_max_points(num_points)
                                            .is_dynamic_index(true)
                                            .with_index_write_params(params)
                                            .with_index_search_params(index_search_params)
                                            .with_data_type(diskann_type_to_name<T>())
                                            .with_tag_type(diskann_type_to_name<TagT>())
                                            .with_data_load_store_strategy(diskann::DataStoreStrategy::MEMORY)
                                            .with_graph_load_store_strategy(diskann::GraphStoreStrategy::MEMORY)
                                            .is_enable_tags(true)
                                            .is_filtered(false)
                                            .with_num_frozen_pts(0)
                                            .is_concurrent_consolidate(false)
                                            .build();

    diskann::IndexFactory index_factory = diskann::IndexFactory(index_config);
    auto index = index_factory.create_instance();

    T *data = nullptr;
    diskann::alloc_aligned((void **)&data, num_points * aligned_dim * sizeof(T), 8 * sizeof(T));
    diskann::load_aligned_bin<T>(data_path, data, num_points, dim, aligned_dim);

    // Build the index using the first chunk
    index->set_start_points_at_random(static_cast<T>(0));
    build<T, TagT>(*index, chunk_size, build_threads, data, aligned_dim);

    std::future<void> insert_task;
    std::future<void> delete_task;

    for (size_t current_chunk = 2; current_chunk <= num_chunks; current_chunk++) {
        // Calculate batch boundaries
        size_t insert_start = (current_chunk - 1) * chunk_size;
        size_t insert_end = current_chunk * chunk_size;
    
        size_t delete_start = (current_chunk - 2) * chunk_size;
        size_t delete_end = (current_chunk - 1) * chunk_size;
    
        // Launch insert
        insert_task = std::async(std::launch::async,
            [&index, insert_start, insert_end, data, aligned_dim, insert_threads]() {
                insert<T, TagT>(*index, insert_start, insert_end, insert_threads, data, aligned_dim);
            });
    
        // Build delete parameters
        diskann::IndexWriteParameters delete_params = diskann::IndexWriteParametersBuilder(params).with_num_threads(consolidate_threads).build();

        // Launch delete
        delete_task = std::async(std::launch::async,
            [&index, delete_params, delete_start, delete_end]() {
                delete_and_consolidate<T, TagT>(*index, delete_params, delete_start, delete_end);
            });
            
        // Wait for the current batches to be done
        insert_task.wait();
        delete_task.wait();
    }
    
    diskann::aligned_free(data);
}

int main(int argc, char **argv)
{
    std::string data_type, data_path;
    uint32_t R, L, build_threads, insert_threads, consolidate_threads, search_threads;
    float alpha;
    size_t chunk_size;

    po::options_description desc;

    try
    {
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "Print information on arguments")
            ("data_type", po::value<std::string>(&data_type)->required(), "Type of data")
            ("data_path", po::value<std::string>(&data_path)->required(), "Path to data")
            ("chunk_size", po::value<size_t>(&chunk_size)->required(), "Chunk size")
            ("R", po::value<uint32_t>(&R)->required(), "Value of R")
            ("L", po::value<uint32_t>(&L)->required(), "Value of L")
            ("alpha", po::value<float>(&alpha)->required(), "Alpha parameter")
            ("build_threads", po::value<uint32_t>(&build_threads)->required(), "Threads for building")
            ("insert_threads", po::value<uint32_t>(&insert_threads)->required(), "Threads for insertion")
            ("consolidate_threads", po::value<uint32_t>(&consolidate_threads)->required(), "Threads for consolidation")
            ("search_threads", po::value<uint32_t>(&search_threads)->required(), "Threads for searching");
    
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cout << desc;
            return 0;
        }
        po::notify(vm);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }

    if (data_type == std::string("int8"))
        experiment<int8_t>(data_path, chunk_size, R, L, alpha, build_threads, insert_threads, consolidate_threads, search_threads);
    else if (data_type == std::string("uint8"))
        experiment<uint8_t>(data_path, chunk_size, R, L, alpha, build_threads, insert_threads, consolidate_threads, search_threads);
    else if (data_type == std::string("float"))
        experiment<float>(data_path, chunk_size, R, L, alpha, build_threads, insert_threads, consolidate_threads, search_threads);
    else
        std::cout << "Unsupported type. Use float/int8/uint8" << std::endl;

    return 0;
}

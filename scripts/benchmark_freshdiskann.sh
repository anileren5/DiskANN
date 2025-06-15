#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) Data type for the vectors (e.g., float32)
data_type="float"

# Dataset name — assumed to exist under ./data/ and ./index/ directories
dataset="siftsmall"

# (ii) Working, index, and data directory paths
working_dir="tmp"
index_dir="index"
data_dir="data"

# (iii) Indexing and I/O paths
index_prefix="${index_dir}/${dataset}/${dataset}"
merge_index_path="${index_dir}/${dataset}/${dataset}_merge"
mem_index_path="${index_dir}/${dataset}/${dataset}_mem"
query_path="${data_dir}/${dataset}/${dataset}_query.bin"
gt_path="${data_dir}/${dataset}/${dataset}_groundtruth.bin"
data_path="${data_dir}/${dataset}/${dataset}_base.bin"
single_file_index=0

# (iv) Search and merge parameters
n_iters=5
total_insert_count=500
total_delete_count=500
R=128
k=100
L=192
L_mem=192
alpha_mem=1.2
L_disk=192
alpha_disk=1.2

# ---------------------- Execute FreshDiskANN Benchmark ----------------------

"./build/tests/test_concurr_merge_insert" \
  "$data_type" \
  "./$working_dir/" \
  "./$index_prefix" \
  "./$merge_index_path" \
  "./$mem_index_path" \
  "$L_mem" "$alpha_mem" \
  "$L_disk" "$alpha_disk" \
  "./$data_path" \
  "$single_file_index" \
  "./$query_path" \
  "./$gt_path" \
  "$n_iters" "$total_insert_count" "$total_delete_count" \
  "$R" "$k" "$L"

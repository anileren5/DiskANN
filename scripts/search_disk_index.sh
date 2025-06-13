#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) Type of the dataset to search. Options: float
data_type="float"

# (ii) Dataset name — assumed to exist under ./data/, ./index/, and ./results/ directories
dataset="siftsmall"

# (iii) Index and query parameters
single_file_index=0
tags=0
num_nodes_to_cache=512
T=1 # Number of threads
W=4 # Beamwidth in BFS
K=100 # Recall@K
similarity_function="l2"
L1=192 # Search List Size


# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Construct required paths
index_path="${ROOT_DIR}/index/${dataset}/${dataset}"
query_path="${ROOT_DIR}/data/${dataset}/${dataset}_query.bin"
results_prefix="${ROOT_DIR}/results/${dataset}/${dataset}"

# ---------------------- Execute Search ----------------------

"${ROOT_DIR}/build/tests/search_disk_index" \
  "$data_type" \
  "$index_path" \
  "$single_file_index" \
  "$tags" \
  "$num_nodes_to_cache" \
  "$T" \
  "$W" \
  "$query_path" \
  null \
  "$K" \
  "$results_prefix" \
  "$similarity_function" \
  "$L1" 
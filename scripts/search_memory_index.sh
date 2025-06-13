#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) Type of the dataset. Options: float etc.
datatype="float"

# (ii) Dataset name — assumed to exist under ./data/, ./index/, and ./results/ directories
dataset="siftsmall"

# (iii) Number of max points to search from
maxpoints=1000000

# (iv) Index configuration
dynamic_index=0
single_file_index=0

# (v) Search parameters
K=100
T=1
distance_metric="l2"
L1=192

# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Construct required paths
index_prefix="${ROOT_DIR}/index/${dataset}/${dataset}"
query_path="${ROOT_DIR}/data/${dataset}/${dataset}_query.bin"
results_prefix="${ROOT_DIR}/results/${dataset}/${dataset}"

# ---------------------- Execute In-Memory Index Search ----------------------

"${ROOT_DIR}/build/tests/search_memory_index" \
  "$datatype" \
  "$maxpoints" \
  "$index_prefix" \
  "$dynamic_index" \
  "$single_file_index" \
  "$query_path" \
  null \
  "$K" \
  "$results_prefix" \
  "$T" \
  "$distance_metric" \
  "$L1"

#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) Type of the dataset to index. Options: float etc.
data_type="float"

# (ii) Dataset name — assumed to exist under ./data/ and ./index/ directories
dataset="siftsmall"

# (iii) Index parameters
dynamic_index=0
single_file_index=0
R=128
L=192
alpha=1.2
T=4
similarity_metric="l2"

# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Construct paths
data_path="${ROOT_DIR}/data/${dataset}/${dataset}_base.bin"
index_prefix="${ROOT_DIR}/index/${dataset}/${dataset}"

# ---------------------- Execute In-Memory Index Build ----------------------

"${ROOT_DIR}/build/tests/build_memory_index" \
  "$data_type" \
  "$data_path" \
  null \
  "$index_prefix" \
  "$dynamic_index" \
  "$single_file_index" \
  "$R" \
  "$L" \
  "$alpha" \
  "$T" \
  "$similarity_metric"
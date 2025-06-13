#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) data_type: Type of the dataset to index. Options: float etc.
# This determines how the binary file will be interpreted.
data_type="float"

# Dataset name — assumed to exist under ./data/ and ./index/ directories
dataset="siftsmall"

# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# (ii) data_file: Input data file in .bin format.
dataset_base="${dataset}_base"
data_path="${ROOT_DIR}/data/${dataset}/${dataset_base}.bin"

# (iii) index_prefix_path: Output prefix path for index files.
# Files like *_pq_pivots.bin, *_disk.index will be created here.
index_prefix="${ROOT_DIR}/index/${dataset}/${dataset}"

# (iv) R: Graph degree — controls index quality and size.
# Typical values: 60–150. Higher = better recall, larger index.
R=128

# (v) L: Search list size during index build.
# Typical values: 75–200. Must be >= R for best results.
L=192

# (vi) B: RAM limit (in GB) during search time.
# Controls in-memory vector compression. Higher = faster search.
B=2

# (vii) M: RAM limit (in GB) during build time.
# If M is small, index will be built in subgraphs. Higher = faster build.
M=8

# (viii) T: Number of threads to use.
# More threads = faster build time. 
T=4

# Similarity metric for index building.
# Typically 'l2' for Euclidean distance
similarity_metric="l2"

# Idk if this makes a difference, but it is required by the script.
single_file_index=0

# ---------------------- Execute Index Build ----------------------

"${ROOT_DIR}/build/tests/build_disk_index" \
  "$data_type" \
  "$data_path" \
  "$index_prefix" \
  "$R" "$L" "$B" "$M" "$T" \
  "$similarity_metric" \
  "$single_file_index"

#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) data_type: Type of the dataset to index. Options: float, etc.
data_type="float"

# Dataset name — assumed to exist under ./data/ and ./index/ directories
dataset="siftsmall"

# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# (ii) data_file: Input data file in .bin format.
dataset_base="${dataset}_base"
data_path="${ROOT_DIR}/data/${dataset}/${dataset_base}.bin"

# (iii) index_prefix_path: Output prefix path for index files.
index_prefix="${ROOT_DIR}/index/${dataset}/${dataset}"

# (iv) R: Graph degree
R=128

# (v) L: Search list size during index build
L=192

# (vi) B: RAM limit (in GB) during search
B=4

# (vii) M: RAM limit (in GB) during build
M=8

# (viii) T: Number of threads to use
T=4

# (ix) P: Unused here, retained if needed in future variants
P=16

# (x) Placeholder for metric — 'null' is required by the binary
similarity_metric="null"

# (xi) single_file_index flag — determines file output style
single_file_index=0

# ---------------------- Execute Index Build ----------------------

"${ROOT_DIR}/build/tests/build_stream_merger_disk_index" \
  "$data_type" \
  "$data_path" \
  "$index_prefix" \
  "$R" "$L" "$B" "$M" "$T" \
  "$similarity_metric" \
  "$single_file_index"

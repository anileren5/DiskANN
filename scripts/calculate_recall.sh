#!/bin/bash

# ---------------------- Configuration Parameters ----------------------

# (i) Dataset name — assumed to exist under ./data/ and ./results/ directories
dataset="sift"

# (ii) Recall calculation parameters
L=192
K=100

# Set project root relative to this script
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Construct paths
groundtruth_path="${ROOT_DIR}/data/${dataset}/${dataset}_groundtruth.bin"
results_path="${ROOT_DIR}/results/${dataset}/${dataset}_${L}_idx_uint32.bin"

# ---------------------- Execute Recall Calculation ----------------------

"${ROOT_DIR}/build/tests/utils/calculate_recall" \
  "$groundtruth_path" \
  "$results_path" \
  "$K"
#!/bin/bash

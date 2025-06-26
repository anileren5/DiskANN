#!/bin/bash

# Change to project root
cd "$(dirname "$0")/../.." || exit 1

# Define variables
DATASET="sift"
DATA_TYPE="float"
DATA_PATH="data/$DATASET/${DATASET}_base.bin"
R=64
L=300
ALPHA=1.2
CHUNK_SIZE=100000
BUILD_THREADS=32
INSERT_THREADS=32
CONSOLIDATE_THREADS=32
SEARCH_THREADS=32

# Step 1: Insertions and deletions
./build/apps/concurrent_insert_delete \
  --data_type "$DATA_TYPE" \
  --data_path "$DATA_PATH" \
  --chunk_size "$CHUNK_SIZE" \
  --R "$R" \
  --L "$L" \
  --alpha "$ALPHA" \
  --build_threads "$BUILD_THREADS" \
  --insert_threads "$INSERT_THREADS" \
  --consolidate_threads "$CONSOLIDATE_THREADS" \
  --search_threads "$SEARCH_THREADS" \

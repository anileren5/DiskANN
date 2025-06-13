#!/bin/bash

# Navigate to the project root (one directory up from scripts)
cd "$(dirname "$0")/.." || exit 1

mkdir -p build && \
  cd build && \
  cmake .. && \
  make -j

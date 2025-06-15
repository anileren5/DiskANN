#!/bin/bash

# ---------------------- Cleanup ----------------------

# Remove all files under ../tmp/ but keep the directory
rm -f ./tmp/*

# Remove all files under ../index/sift/ and ../index/siftsmall/ but keep the directories
rm -f ./index/sift/*
rm -f ./index/siftsmall/*

# ---------------------- Tag Generation ----------------------

# Run the tag generation script (already in current directory)
./scripts/generate_tags.sh

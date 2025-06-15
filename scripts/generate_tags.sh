#!/bin/bash

python3 ./scripts/generate_tags.py 10000 ./index/siftsmall/siftsmall_disk.index.tags
python3 ./scripts/generate_tags.py 1000000 ./index/sift/sift_disk.index.tags

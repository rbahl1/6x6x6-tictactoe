#!/bin/bash
# logs/paper/flow_counts/run.sh -- recounts the flow statistics quoted in Appendix S4.1 of the paper.
# Run from anywhere inside the unpacked archive; needs g++.
set -euo pipefail
cd "$(dirname "$0")"
g++ -O2 -std=c++17 -w -o flowcount flowcount.cpp
./flowcount ../../../results/final/leaves.txt
rm -f flowcount

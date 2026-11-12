#!/bin/bash
# referee X (second round, mathematics): reruns all own checks.  Run from logs/paper/referee2/ in the project
# tree after  make bin/treecheck .  About 2 CPU-minutes.  Nothing outside this directory is written.
set -euo pipefail
cd "$(dirname "$0")"
N="nice -n 10"
$N python3 x_geom.py
$N python3 x_prop422.py        > x_prop422.log      # Prop. 4.22 over all 2^24 busy sets; Figure 2
$N python3 x_sec5.py           > x_sec5.log         # Example 5.3 vs strategy.tbl; Lemma 5.2; Figure 4; Table 6
$N python3 x_round5.py         > x_round5.log       # Table 3, row t = 5, from the table entries
$N python3 x_leaf.py 2000      > x_leaf.log         # Alg. 2, variant with deleted lines, Lemma 7.5, treecheck --leafcheck vs brute force
$N python3 x_vectors.py        > x_vectors.log      # Appendix A: canonical forms, 26 LEAF vectors
g++ -O2 -std=c++17 -w -o x_lost x_lost.cpp
$N ./x_lost < ../lost_vectors/vectors.txt > x_lost.log   # Table 10 (literal Definition 5.4)
$N python3 x_lost_xinF.py      > x_lost_xinF.log    # triples on which LOST depends on patterns with x in F
rm -f x_lost
$N python3 x_seven.py          > x_seven.log        # Section 9.1, Appendix B, mutations of the table
rm -rf x_seven_mut
$N python3 x_deps.py           > x_deps.log         # dependency closure of Theorem 1.1 and Corollary 9.3
grep -h "RESULT\|agree\|mismatch\|smallest busy set" x_*.log

#!/bin/bash
# logs/paper/lost_vectors/run.sh -- recomputes the LOST test vectors of Appendix A.6 with three
# implementations and compares the verdicts.  Run from the root of the archive after  make bin/treecheck .
set -euo pipefail
cd "$(dirname "$0")"
ROOT=../../..
g++ -O2 -std=c++17 -w -o lost_driver lost_driver.cpp
"$ROOT/bin/treecheck" --lostcheck < vectors.txt > treecheck.out
./lost_driver < vectors.txt > replication.out
python3 spotcheck_lost.py vectors.txt > spotcheck.out
python3 - <<'PY'
import json
tc = [json.loads(l) for l in open("treecheck.out")]
a = [l.split() for l in open("replication.out")]
b = [l.split() for l in open("spotcheck.out")]
assert len(tc) == len(a) == len(b)
for x, y, z in zip(tc, a, b):
    v = "true" if x["lost"] else "false"
    assert y[-1] == "lost=" + v and z[-1] == "lost=" + v and y[3:5] == z[3:5], (x, y, z)
    print(x["t"], x["B"], x["F"], "LOST =", v, "pattern", x["pattern"], y[3], y[4])
print("all three implementations agree on", len(tc), "instances")
PY
# the 800 instances (t = 9..13) of the audit of the main verifier: treecheck against the literal test
INST=$ROOT/logs/review/treecheck_mut/lost_inst.txt
"$ROOT/bin/treecheck" --lostcheck < "$INST" > all800_treecheck.tmp
./lost_driver < "$INST" > all800_replication.tmp
python3 - <<'PY'
import json
from collections import Counter
tc = [json.loads(l) for l in open("all800_treecheck.tmp")]
a = [l.split() for l in open("all800_replication.tmp")]
assert len(tc) == len(a)
bad = sum(1 for x, y in zip(tc, a) if y[-1] != "lost=" + ("true" if x["lost"] else "false"))
c = Counter((x["t"], x["lost"]) for x in tc)
print("audit instances:", len(tc), "; LOST true/false by t:",
      ", ".join("t=%d: %d/%d" % (t, c[(t, True)], c[(t, False)]) for t in sorted({k[0] for k in c})),
      "; mismatches between treecheck --lostcheck and the literal test:", bad)
assert bad == 0
PY
rm -f lost_driver all800_treecheck.tmp all800_replication.tmp

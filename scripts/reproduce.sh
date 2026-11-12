#!/bin/bash
# One-command reproduction of the computer-assisted proof that the second player (SP) never loses 6x6x6
# tic-tac-toe.  Everything is recomputed from source into results/repro/ and compared with results/final/.
#
#   scripts/reproduce.sh                 # proof pipeline: build, self-tests, search, 3 verifiers, leaf checks, simulator
#   scripts/reproduce.sh --with-ip       # additionally the HiGHS cross-checks (hours of CPU; needs python3 + highspy)
#   GAMES=100000 THREADS=4 scripts/reproduce.sh   # fewer simulator games / explicit thread count
#
# Requirements: g++ >= 9 with OpenMP, make, python3 (>= 3.10).  Nothing is uploaded anywhere.
# Expected time on 4 idle cores: about 55 minutes without --with-ip (search 1 min, treecheck < 1 min,
# Python verifier 6 min, enumeration of the theta = 3 leaves 13 min, simulator 10^6 games about 26 min).
# Validated on 2026-11-12 with GAMES=20000 THREADS=2: all steps passed, tree identical to results/final.
set -u
cd "$(dirname "$0")/.."
THREADS=${THREADS:-$(nproc)}
GAMES=${GAMES:-1000000}
WITH_IP=0; for a in "$@"; do [ "$a" = "--with-ip" ] && WITH_IP=1; done
OUT=results/repro; LOG=logs/repro
mkdir -p $OUT/tree $OUT/checks $OUT/leaftable $OUT/sim $LOG
FAIL=0
say()  { echo; echo "== $* =="; }
run()  { # run NAME CMD... : run a step, record PASS/FAIL
  local name=$1; shift
  if "$@" > "$LOG/$name.log" 2>&1; then echo "PASS  $name"; else echo "FAIL  $name   (see $LOG/$name.log)"; FAIL=1; fi
}

say "build"
for b in geom_selftest test_canon test_core leaftable phicheck enumcheck search certify minstones treecheck sim; do
  make -s bin/$b || { echo "FAIL  build bin/$b"; exit 2; }
done
echo "PASS  build"

say "self-tests of every program"
run geom_selftest        bin/geom_selftest
run test_canon           bin/test_canon
run test_core            bin/test_core
run treecheck_selftest   bin/treecheck --selftest --quick
run sim_selftest         bin/sim --selftest
run spotcheck_selftest   python3 verify/spotcheck.py --selftest
run search_lost_selftest bin/search --lost-selftest 2000

say "regression 1: minimum number of SP stones for a pairing, n = 5..8 (expected 9, 7, 3, 0)"
run minstones            bin/minstones --out $OUT/minstones.json

say "leaf table by exact max-flow: LEAF(B, empty, |B|) for every orbit of busy sets, |B| = 8, 9, 10"
for k in 8 9 10; do run leaftable_k$k bin/leaftable --k $k --out $OUT/leaftable; done
run leaftable_merge      bin/leaftable --k 8,9,10 --merge --out $OUT/leaftable
grep -h "summary" $LOG/leaftable_merge.log 2>/dev/null

say "AND-OR search: root of the tree of SPEC section 4, R_max = 10 (about 1 CPU-minute)"
run search               bin/search --rmax 10 --maxcand 4 --relcand 10 --nonbusy --threads $THREADS --out $OUT/tree --selfcheck
if cmp -s $OUT/tree/strategy.tbl results/final/strategy.tbl && cmp -s $OUT/tree/leaves.txt results/final/leaves.txt; then
  echo "PASS  tree identical to results/final"
else
  echo "NOTE  tree differs from results/final (allowed: it is verified below on its own)"
fi

say "independent verification of the whole tree (verify/treecheck.cpp, exact keys, validated flows)"
run treecheck            bin/treecheck --table $OUT/tree/strategy.tbl --leaves $OUT/tree/leaves.txt --rmax 10 --threads $THREADS --exact --paranoid --out $OUT/treecheck.json
say "third implementation (verify/spotcheck.py, pure Python), exhaustive mode"
run spotcheck_full       python3 verify/spotcheck.py --table $OUT/tree/strategy.tbl --leaves $OUT/tree/leaves.txt --rmax 10 --samples 0 --seed 20261112 --procs $THREADS --full --all-leaves --out $OUT/spotcheck_full.json

say "leaf cross-checks"
run leaf_instances       python3 scripts/leaf_instances.py $OUT/tree/leaves.txt $OUT/checks 20000
bin/phicheck < $OUT/checks/leaves_all.txt > $OUT/checks/phi_all.jsonl
bin/treecheck --leafcheck < $OUT/checks/leaves_all.txt > $OUT/checks/tc_all.jsonl 2> $LOG/tc_all.log
bin/phicheck < $OUT/checks/perturbed.txt > $OUT/checks/phi_pert.jsonl
bin/treecheck --leafcheck < $OUT/checks/perturbed.txt > $OUT/checks/tc_pert.jsonl 2> $LOG/tc_pert.log
run leafcheck_all_leaves python3 scripts/compare_leafcheck.py $OUT/checks/phi_all.jsonl $OUT/checks/tc_all.jsonl --expect-all-true
run leafcheck_perturbed  python3 scripts/compare_leafcheck.py $OUT/checks/phi_pert.jsonl $OUT/checks/tc_pert.jsonl
for th in 0 1 2 3; do
  if [ -s $OUT/checks/leaves_theta$th.txt ]; then
    bin/enumcheck < $OUT/checks/leaves_theta$th.txt > $OUT/checks/enum_theta$th.jsonl
    run enum_theta$th python3 -c "
import json,sys
r=[json.loads(l) for l in open('$OUT/checks/enum_theta$th.jsonl')]
print(len(r),'leaves, C holds on',sum(x['holds'] for x in r)); sys.exit(0 if r and all(x['holds'] for x in r) else 1)"
  fi
done

say "simulator (sim/sim.cpp): SP plays the strategy against $GAMES games of mixed adversaries"
run sim_smoke            bin/sim --table $OUT/tree/strategy.tbl --leaves $OUT/tree/leaves.txt --games 2000 --seed 1 --adversary mix --threads $THREADS --verify-canonical --out $OUT/sim/smoke.json --bugs-out $OUT/sim/smoke_bugs.jsonl
run sim_mix              bin/sim --table $OUT/tree/strategy.tbl --leaves $OUT/tree/leaves.txt --games $GAMES --seed 20261112 --adversary mix --threads $THREADS --out $OUT/sim/mix.json --bugs-out $OUT/sim/mix_bugs.jsonl

if [ $WITH_IP = 1 ]; then
  say "HiGHS cross-checks (floating point; not part of the proof)"
  run ip_regression      python3 py/regress.py --fresh --procs 3 --out $OUT/ip_regression.json
  run ip_crosscheck      python3 py/crosscheck.py --stage all --outdir $OUT/crosscheck --no-reuse
  python3 - <<PY > $OUT/checks/ip_leaf_sample.txt
import random
rng = random.Random(4242); quota = {0:300,1:300,2:150,3:80}
import os
for th in range(0, 9):
    p = '$OUT/checks/leaves_theta%d.txt' % th
    if not os.path.exists(p): continue
    rows = [l for l in open(p).read().split('\n') if l]
    for l in (rng.sample(rows, min(quota[th], len(rows))) if th in quota else rows): print(l)
PY
  run ip_leaves          python3 py/ip_check.py --in $OUT/checks/ip_leaf_sample.txt --out $OUT/checks/ip_leaf_sample.jsonl --mode decide --time-limit 1800 --quiet
  run ip_leaves_all_hold python3 -c "
import json,sys
r=[json.loads(l) for l in open('$OUT/checks/ip_leaf_sample.jsonl')]
print(len(r),'leaves, IP holds on',sum(x['decision']=='holds' for x in r)); sys.exit(0 if r and all(x['decision']=='holds' for x in r) else 1)"
fi

say "summary"
python3 - <<PY
import json
def J(p):
    try: return json.load(open(p))
    except Exception as e: return {"error": str(e)}
s = J('$OUT/tree/stats.json'); t = J('$OUT/treecheck.json'); p = J('$OUT/spotcheck_full.json'); m = J('$OUT/sim/mix.json')
print('search   : proved =', s.get('proved'), ' P-states', s.get('p_states'), ' Q-states', s.get('q_states'), ' leaves', s.get('leaves'))
print('treecheck: ok =', t.get('ok'), ' states', t.get('states_visited'), ' leaves checked', t.get('leaves_checked'), ' errors', t.get('error_count'))
print('spotcheck: ok =', p.get('ok'), ' P expanded', p.get('p_expanded'), ' Q checked', p.get('q_checked'), ' leaves recomputed', p.get('leaves_recomputed'))
print('simulator: games', m.get('games'), ' FP lines', m.get('fp_lines'), ' holes', m.get('holes'), ' pairing failures', m.get('pairing_failures'), ' admissibility failures', m.get('admissibility_failures'))
PY
if [ $FAIL = 0 ]; then echo "ALL STEPS PASSED"; else echo "SOME STEPS FAILED"; fi
exit $FAIL

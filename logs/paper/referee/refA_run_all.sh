#!/bin/bash
# referee 1a: re-runs all checks (about 7 CPU-minutes in total, three at a time); outputs refA_*.out
cd "$(dirname "$0")"
run() { ( nice -n 10 python3 refA_$1.py > refA_$1.out 2>&1; echo "exit $?" >> refA_$1.out ) ; }
run sec4 & run sec4b & run leaves & wait
run monoC & run ip & run vectors & wait
run sec2 & run sec3 & run aut & wait
run delete & run orbits & wait
rm -rf __pycache__
grep -H '^exit' refA_*.out

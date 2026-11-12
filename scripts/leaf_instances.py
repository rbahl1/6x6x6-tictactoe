#!/usr/bin/env python3
"""Build instance files from a leaves.txt (lines "t B F"):
   <out>/leaves_all.txt        every leaf as an instance "k B F" with k = t
   <out>/leaves_theta<j>.txt   the leaves with t-|F| = j (j = 0..8)
   <out>/perturbed.txt         leaf-derived instances that are often NOT leaves: k+1, k+2, one SP stone removed,
                               one revealed stone removed (seeded sample), used to compare LEAF implementations."""
import sys, random, os
src, out = sys.argv[1], sys.argv[2]
nper = int(sys.argv[3]) if len(sys.argv) > 3 else 20000
os.makedirs(out, exist_ok=True)
rows = []
for line in open(src):
    p = line.split()
    if len(p) != 3: continue
    t = int(p[0]); B = [] if p[1] == '-' else p[1].split(','); F = [] if p[2] == '-' else p[2].split(',')
    rows.append((t, B, F))
fmt = lambda s: ','.join(s) if s else '-'
with open(f'{out}/leaves_all.txt', 'w') as f:
    for t, B, F in rows: f.write(f'{t} {fmt(B)} {fmt(F)}\n')
byth = {}
for t, B, F in rows: byth.setdefault(t - len(F), []).append((t, B, F))
for th, rs in sorted(byth.items()):
    with open(f'{out}/leaves_theta{th}.txt', 'w') as f:
        for t, B, F in rs: f.write(f'{t} {fmt(B)} {fmt(F)}\n')
rng = random.Random(20261112)
with open(f'{out}/perturbed.txt', 'w') as f:
    for t, B, F in rng.sample(rows, min(nper, len(rows))):
        kind = rng.randrange(4)
        if kind == 0: f.write(f'{t+1} {fmt(B)} {fmt(F)}\n')
        elif kind == 1: f.write(f'{t+2} {fmt(B)} {fmt(F)}\n')
        elif kind == 2 and B:
            B2 = list(B); B2.pop(rng.randrange(len(B2))); f.write(f'{t} {fmt(B2)} {fmt(F)}\n')
        elif F:
            F2 = list(F); F2.pop(rng.randrange(len(F2))); f.write(f'{t} {fmt(B)} {fmt(F2)}\n')
        else: f.write(f'{t+1} {fmt(B)} {fmt(F)}\n')
print({th: len(rs) for th, rs in sorted(byth.items())}, 'total', len(rows))

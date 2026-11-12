# refA_leaves.py -- referee 1a: LEAF(B,F,t) for a stratified sample of the leaves of the certificate
# (results/final/leaves.txt), with my own literal implementation of Algorithm 2; and LEAF(B,F,t+1)
# for the same leaves (to see how often the leaves are tight).  Read-only use of the certificate.
import sys, time, random
from refA_geom import *
from refA_lib import *
t0 = time.time()
rows = {}
with open('/home/user/ttt6/results/final/leaves.txt') as f:
    for line in f:
        t, B, F = line.split()
        t = int(t)
        B = [] if B == '-' else [int(x) for x in B.split(',')]
        F = [] if F == '-' else [int(x) for x in F.split(',')]
        rows.setdefault(t - len(F), []).append((t, B, F))
print('leaves by theta:', {k: len(v) for k, v in sorted(rows.items())}); sys.stdout.flush()
rng = random.Random(61)
bad = 0; n = 0; tight = 0
for theta in sorted(rows, reverse=True):
    sample = rows[theta] if theta >= 5 else rng.sample(rows[theta], 60 if theta >= 1 else 150)
    ok = 0; tg = 0
    for t, B, F in sample:
        if time.time() - t0 > 240:
            break
        assert len(B) == t and len(F) <= t and not (set(B) & set(F))
        r = leaf_alg2(mask(B), mask(F), t)
        ok += r; bad += (not r); n += 1
        if theta >= 5:
            r2 = leaf_alg2(mask(B), mask(F), t + 1)
            tg += (not r2)
    print('theta', theta, 'sampled', len(sample), 'LEAF true', ok, ('LEAF(B,F,t+1) false: %d' % tg) if theta >= 5 else ''); sys.stdout.flush()
print('total tested', n, 'failures', bad, ' (%.1f s)' % (time.time() - t0))

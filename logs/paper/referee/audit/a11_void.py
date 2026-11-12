#!/usr/bin/env python3
# Referee audit A11: classify the void children of type (ii) (Section 6.3): cap (five on a live line) or danger pair?
import itertools, collections
ROOT = '/home/user/ttt6/'
def cid(x, y, z): return 36 * x + 6 * y + z
LN = set()
for f in itertools.product(range(8), repeat=3):
    if all(v < 6 for v in f): continue
    LN.add(frozenset(cid(*[(v if v < 6 else (t if v == 6 else 5 - t)) for v in f]) for t in range(6)))
LN = list(LN)
def pset(s): return set() if s == '-' else set(int(v) for v in s.split(','))
cnt = collections.Counter(); byt = collections.Counter(); n_ii = 0
for l in open(ROOT + 'results/final/strategy.tbl'):
    t, b, f, m = l.split(); t = int(t); B = pset(b); F = pset(f); m = int(m)
    if len(F) >= t: continue
    n_ii += 1
    X = F | {m}
    live = [x for x in LN if not (x & B)]
    k = [len(x & X) for x in live]
    cap = any(v >= 5 for v in k)
    four = [x for x, v in zip(live, k) if v == 4]
    dp = any((a & b) and not ((a & b) & X) for a, b in itertools.combinations(four, 2))
    if cap or dp:
        cnt[('cap' if cap else '') + ('+' if cap and dp else '') + ('danger' if dp else '')] += 1; byt[t] += 1
        # is it a line with four cells of F through m?
        if cap: assert any(m in x and len(x & F) == 4 for x in live)
print('Q-states with |F| < t:', n_ii, ' void children (ii):', sum(cnt.values()), dict(cnt), ' by round:', dict(sorted(byt.items())))

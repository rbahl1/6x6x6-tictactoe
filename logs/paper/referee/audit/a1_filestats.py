#!/usr/bin/env python3
# Referee audit A1: statistics recounted from the two certificate files only.
import sys, hashlib, collections, itertools
ROOT = '/home/user/ttt6/'
TBL = ROOT + 'results/final/strategy.tbl'
LVS = ROOT + 'results/final/leaves.txt'

def co(c): return (c // 36, (c // 6) % 6, c % 6)
def lam(i): return min(i, 5 - i)
def busy(c):
    x, y, z = co(c); return lam(x) == lam(y) == lam(z)
def pset(s): return [] if s == '-' else [int(v) for v in s.split(',')]

for f in (TBL, LVS):
    d = open(f, 'rb').read()
    print(f, 'bytes', len(d), 'lines', d.count(b'\n'), 'sha256', hashlib.sha256(d).hexdigest())

tbl = [l.split() for l in open(TBL).read().splitlines()]
lvs = [l.split() for l in open(LVS).read().splitlines()]

# well-formedness
bad = 0; keys = set()
qbyt = collections.Counter(); nb = collections.Counter(); q_ii = 0
for t, b, f, m in tbl:
    t = int(t); B = pset(b); F = pset(f); m = int(m)
    if len(B) != t - 1 or len(F) > t or set(B) & set(F) or m in B or m in F or B != sorted(B) or F != sorted(F) or not 0 <= m < 216: bad += 1
    if (t, b, f) in keys: bad += 1
    keys.add((t, b, f))
    qbyt[t] += 1
    if not busy(m): nb[t] += 1
    if len(F) < t: q_ii += 1
print('table: bad lines', bad, 'distinct keys', len(keys))
print('Q-states by t', [qbyt[t] for t in range(11)], sum(qbyt.values()))
print('non-busy moves by t', dict(sorted(nb.items())), sum(nb.values()))
print('Q-states with |F| < t', q_ii)

bad = 0; lk = set()
lbyt = collections.Counter(); tt = collections.Counter(); nbl = collections.Counter(); th = collections.Counter()
for t, b, f in lvs:
    t = int(t); B = pset(b); F = pset(f)
    if len(B) != t or len(F) > t or set(B) & set(F) or B != sorted(B) or F != sorted(F): bad += 1
    if (t, b, f) in lk: bad += 1
    lk.add((t, b, f))
    lbyt[t] += 1; tt[(t, t - len(F))] += 1; th[t - len(F)] += 1
    nbl[sum(1 for c in B if not busy(c))] += 1
print('leaves: bad lines', bad, 'distinct', len(lk))
print('leaves by t', dict(sorted(lbyt.items())))
for t in (8, 9, 10):
    print(' t=%d by theta' % t, [tt[(t, k)] for k in range(9)], sum(tt[(t, k)] for k in range(9)))
print(' total by theta', [th[k] for k in range(9)], sum(th.values()))
print(' theta=0 share %.4f%%' % (100.0 * th[0] / len(lvs)))
print(' theta<=3: %d = %.4f%%; theta>=4: %d' % (sum(th[k] for k in range(4)), 100.0 * sum(th[k] for k in range(4)) / len(lvs), sum(th[k] for k in range(4, 9))))
print(' theta>=3:', sum(th[k] for k in range(3, 9)), ' theta>=2:', sum(th[k] for k in range(2, 9)))
print('leaves by number of non-busy Breaker stones', dict(sorted(nbl.items())))
print('the theta=8 leaf:', [l for l in lvs if l[2] == '-' and l[0] == '8'])

# cited lines
print('table line 1:', ' '.join(tbl[0])); print('table line 2354:', ' '.join(tbl[2353])); print('table line 9791:', ' '.join(tbl[9790]))
print('leaf line 1:', ' '.join(lvs[0])); print('leaf line 8185:', ' '.join(lvs[8184])); print('leaf last:', ' '.join(lvs[-1]))
def cs(s): return ', '.join(str(co(c)) for c in pset(s)) if s != '-' else '{}'
for l in (tbl[0], tbl[2353], tbl[9790]): print('  ', l[0], cs(l[1]), '|', cs(l[2]), '->', co(int(l[3])), 'busy' if busy(int(l[3])) else 'NON-BUSY')
for l in (lvs[0], lvs[8184], lvs[-1]): print('  ', l[0], cs(l[1]), '|', cs(l[2]))

# Table 6: all entries of rounds 1..3 (and 4)
print('--- Table 6: entries of rounds 1-3 in coordinates')
for t, b, f, m in sorted(tbl, key=lambda r: (int(r[0]), pset(r[1]), len(pset(r[2])), pset(r[2]))):
    if int(t) <= 3: print(t, cs(b), '|', cs(f), '->', co(int(m)), 'busy' if busy(int(m)) else 'NON-BUSY')
print('--- round 4 entries with B = (0,0,0),(1,1,4),(4,4,1) i.e. ids 0,46,169')
for t, b, f, m in tbl:
    if t == '4' and b == '0,46,169': print(t, cs(b), '|', cs(f), '->', co(int(m)))
print('round-4 entries:', sum(1 for r in tbl if r[0] == '4'))
print('rounds 1..6 non-busy moves:', sum(nb[t] for t in range(1, 7)))

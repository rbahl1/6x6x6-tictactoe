#!/usr/bin/env python3
# Referee audit A10: (1) order of the full automorphism group of (Q, L) by backtracking (Computed Fact 2.9);
# (2) Lemma 9.1 counts and Table 13 (pairing for the 45 mid-plane lines of the 7^3 board), parsed from paper.txt
import itertools, re, sys
sys.setrecursionlimit(10000)
def lines(n):
    S = set()
    for f in itertools.product(range(n + 2), repeat=3):
        if all(v < n for v in f): continue
        S.add(frozenset(tuple((v if v < n else (t if v == n else n - 1 - t)) for v in f) for t in range(n)))
    return S
L6 = lines(6); assert len(L6) == 148
cells = sorted(itertools.product(range(6), repeat=3)); idx = {c: i for i, c in enumerate(cells)}
coll = [0] * 216; deg = [0] * 216
for l in L6:
    for a in l:
        deg[idx[a]] += 1
        for b in l:
            if a != b: coll[idx[a]] |= 1 << idx[b]
lineset = set(frozenset(idx[c] for c in l) for l in L6)
# order: BFS by collinearity from cell 0, so that constraints bite early
order = [0]; seen = {0}
for c in order:
    for d in range(216):
        if d not in seen and coll[c] >> d & 1: seen.add(d); order.append(d)
assert len(order) == 216
img = [-1] * 216; used = [False] * 216; count = 0; nodes = 0
def rec(k):
    global count, nodes
    if k == 216:
        if all(frozenset(img[c] for c in l) in lineset for l in lineset): count += 1
        return
    c = order[k]
    for d in range(216):
        if used[d] or deg[d] != deg[c]: continue
        ok = True
        for j in range(k):
            q = order[j]
            if (coll[c] >> q & 1) != (coll[d] >> img[q] & 1): ok = False; break
        if not ok: continue
        nodes += 1
        img[c] = d; used[d] = True; rec(k + 1); used[d] = False; img[c] = -1
rec(0)
print('automorphisms of (Q, L) found by backtracking over collinearity-preserving bijections:', count, '(search nodes %d)' % nodes)

# ---- 7^3
L7 = lines(7); print('lines of 7^3:', len(L7))
M = set(c for c in itertools.product(range(7), repeat=3) if 3 in c)
inM = [l for l in L7 if l <= M]; rest = [l for l in L7 if not l <= M]
print('|M| =', len(M), ' lines inside M:', len(inM), ' other lines:', len(rest), ' each other line meets M in one cell:', all(len(l & M) == 1 for l in rest))
phi = lambda i: i if i <= 2 else i - 1
im = set(frozenset(tuple(phi(v) for v in c) for c in l - M) for l in rest)
print('images of the other lines are exactly the 148 lines of 6^3:', im == L6)
txt = open('/home/user/ttt6/paper/paper.txt').read()
tab = txt[txt.index('plane z = 3                            plane y = 3'):txt.index('Table 13. A pairing for the 45 lines')]
ent = re.findall(r'\(([0-9t−\-, ]+)\)\s+\((\d), (\d), (\d)\), \((\d), (\d), (\d)\)', tab)
print('table entries parsed:', len(ent))
allc = []; ok = True; got = set()
for e in ent:
    f = [s.strip().replace('−', '-') for s in e[0].split(',')]
    line = frozenset(tuple((int(v) if v.isdigit() else (t if v == 't' else 6 - t)) for v in f) for t in range(7))
    c1 = tuple(map(int, e[1:4])); c2 = tuple(map(int, e[4:7]))
    if line not in L7 or not line <= M or c1 not in line or c2 not in line or c1 == c2: ok = False; print('BAD', e)
    got.add(line); allc += [c1, c2]
print('all entries are lines inside M with two of their own cells:', ok, ' distinct lines:', len(got), ' = all lines inside M:', got == set(inM))
print('cells assigned:', len(allc), 'distinct:', len(set(allc)), ' unassigned cells of M:', len(M) - len(set(allc)), ' centre unassigned:', (3, 3, 3) not in allc)
# ES sums
from fractions import Fraction
for n, nl in ((5, 109), (6, 148), (7, 193)): print('Erdos-Selfridge sum n=%d:' % n, Fraction(nl, 2 ** n), float(Fraction(nl, 2 ** n)))
print('lines by formula ((n+2)^3-n^3)/2 and 3n^2+6n+4:', [(n, ((n + 2) ** 3 - n ** 3) // 2, 3 * n * n + 6 * n + 4, n ** 3, 2 * (3 * n * n + 6 * n + 4)) for n in range(3, 10)])
print('HJ bounds d=3: odd n >= 3^3-1 =', 3 ** 3 - 1, '; even n >= 2^4-2 =', 2 ** 4 - 2)

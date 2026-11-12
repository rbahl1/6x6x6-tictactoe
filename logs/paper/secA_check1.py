#!/usr/bin/env python3
"""Independent re-check of every concrete number and example used in Sections 2-4 and Appendix C.1
(own geometry, brute force; nothing from src/, verify/, py/ is imported).
Usage: python3 logs/paper/secA_check1.py            (about 10 seconds)
"""
import itertools, sys
from collections import deque

# ---------------------------------------------------------------- geometry
cells = [(x, y, z) for x in range(6) for y in range(6) for z in range(6)]
cid = {c: 36 * c[0] + 6 * c[1] + c[2] for c in cells}
choices = [('c', v) for v in range(6)] + [('t', 0), ('r', 0)]
lines = set()
param_count = {}
for f in itertools.product(choices, repeat=3):
    if all(k == 'c' for k, _ in f):
        continue
    pts = []
    for t in range(6):
        pts.append(tuple(v if k == 'c' else (t if k == 't' else 5 - t) for k, v in f))
    L = frozenset(pts)
    assert len(L) == 6
    lines.add(L)
    param_count[L] = param_count.get(L, 0) + 1
lines = sorted(lines, key=lambda L: sorted(cid[c] for c in L))
assert len(lines) == 148 and all(v == 2 for v in param_count.values())
def ltype(L):
    return sum(1 for i in range(3) if len({c[i] for c in L}) > 1)
print("lines:", len(lines), "by number of active coordinates:", [sum(1 for L in lines if ltype(L) == k) for k in (1, 2, 3)])
through = {c: [i for i, L in enumerate(lines) if c in L] for c in cells}
# two cells on at most one line
pairline = {}
for i, L in enumerate(lines):
    for p, q in itertools.combinations(sorted(L), 2):
        assert (p, q) not in pairline
        pairline[(p, q)] = i
deg = {c: len(through[c]) for c in cells}
print("degrees:", {d: sum(1 for c in cells if deg[c] == d) for d in sorted(set(deg.values()))},
      "incidences", sum(deg.values()), "= 148*6 =", 148 * 6)
lam = lambda i: min(i, 5 - i)
busy = [c for c in cells if lam(c[0]) == lam(c[1]) == lam(c[2])]
assert len(busy) == 24 and all(deg[c] == 7 for c in busy) and all(deg[c] in (3, 4) for c in cells if c not in busy)
for c in cells:
    lv = (lam(c[0]), lam(c[1]), lam(c[2]))
    assert deg[c] == {1: 7, 2: 4, 3: 3}[len(set(lv))]
    nfd = sum(1 for i in through[c] if ltype(lines[i]) == 2)
    nsd = sum(1 for i in through[c] if ltype(lines[i]) == 3)
    assert nfd == {1: 3, 2: 1, 3: 0}[len(set(lv))] and nsd == (1 if len(set(lv)) == 1 else 0)
space = [L for L in lines if ltype(L) == 3]
for L in space:
    assert all(c in busy for c in L) and all(tuple(5 - v for v in c) in L for c in L)
assert all(sum(1 for c in L if c in busy) < 6 for L in lines if ltype(L) < 3)
print("busy cells: 24, each on 3 axis lines, 3 face diagonals, 1 space diagonal; space diagonals closed under antipodes: ok")

# ---------------------------------------------------------------- group
H = [s for s in itertools.permutations(range(6)) if all(s[5 - i] == 5 - s[i] for i in range(6))]
assert len(H) == 48
maps = set()
ntriples = 0
for pi in itertools.permutations(range(3)):
    for r in itertools.product((0, 1), repeat=3):
        for s in H:
            ntriples += 1
            img = []
            for c in cells:
                d = [None] * 3
                for i in range(3):
                    d[pi[i]] = c[i]
                d = [5 - d[i] if r[i] else d[i] for i in range(3)]
                d = tuple(s[v] for v in d)
                img.append(cid[d])
            maps.add(tuple(img))
maps = sorted(maps)
print("triples (pi,r,s):", ntriples, "distinct maps:", len(maps))
assert len(maps) == 1152
lineids = {frozenset(cid[c] for c in L): i for i, L in enumerate(lines)}
for g in maps:
    assert sorted(g) == list(range(216))
    for L in lineids:
        assert frozenset(g[c] for c in L) in lineids
mapset = set(maps)
# closure: G is generated-closed  (g o h in G for all g and for h in a generating set, plus inverses)
import random
random.seed(1)
for g in maps:
    inv = [0] * 216
    for a, b in enumerate(g):
        inv[b] = a
    assert tuple(inv) in mapset
gens = random.sample(maps, 40)
for g in maps:
    for h in gens:
        assert tuple(g[h[c]] for c in range(216)) in mapset
        assert tuple(h[g[c]] for c in range(216)) in mapset
# full closure test via generated subgroup: closure of 6 natural generators equals the set of 1152 maps
def mk(fn):
    return tuple(cid[fn(c)] for c in cells)
s_swap01 = [1, 0, 2, 3, 5, 4]      # swaps the layer pairs {0,5} and {1,4}
s_swap12 = [0, 2, 1, 4, 3, 5]      # swaps {1,4} and {2,3}
s_flip0 = [5, 1, 2, 3, 4, 0]       # reverses the pair {0,5} only
for s in (s_swap01, s_swap12, s_flip0):
    assert all(s[5 - i] == 5 - s[i] for i in range(6))
gen6 = [mk(lambda c: (c[1], c[0], c[2])), mk(lambda c: (c[0], c[2], c[1])), mk(lambda c: (5 - c[0], c[1], c[2])),
        mk(lambda c: tuple(s_swap01[v] for v in c)), mk(lambda c: tuple(s_swap12[v] for v in c)),
        mk(lambda c: tuple(s_flip0[v] for v in c))]
seen = {tuple(range(216))}
frontier = list(seen)
while frontier:
    nxt = []
    for g in frontier:
        for h in gen6:
            k = tuple(h[g[c]] for c in range(216))
            if k not in seen:
                seen.add(k); nxt.append(k)
    frontier = nxt
assert seen == mapset
print("group: 1152 line-preserving bijections; inverses in G; generated group of 6 generators equals the set: ok")

# ---------------------------------------------------------------- pairings / flows
LINES = [sorted(cid[c] for c in L) for L in lines]
LSET = [frozenset(L) for L in LINES]
THR = [[i for i, L in enumerate(LSET) if c in L] for c in range(216)]

def live(B):
    B = set(B)
    return [i for i in range(148) if not (LSET[i] & B)]

def slack(B):
    return 216 - len(B) - 2 * len(live(B))

def admissible(B, X):
    B = set(B); X = set(X)
    if B & X:
        return False
    lv = live(B)
    cnt = {i: len(LSET[i] & X) for i in lv}
    if any(v > 4 for v in cnt.values()):
        return False
    four = [i for i in lv if cnt[i] == 4]
    for a, b in itertools.combinations(four, 2):
        m = LSET[a] & LSET[b]
        if m and not (m & X):
            return False
    return True

class Flow:
    """assignment of cells to live lines; holder[c] = line or None; cap[l] = max cells of line l"""
    def __init__(self, B, F):
        self.F = set(F)
        self.lv = live(B)
        self.holder = {}
        self.held = {l: set() for l in self.lv}
        self.cap = {l: 2 for l in self.lv}
    def augment(self, l0):
        # BFS from line l0 for an alternating path to an unassigned cell
        prev = {}
        seenl = {l0}
        dq = deque([l0])
        while dq:
            l = dq.popleft()
            for c in LINES[l]:
                if c in self.F or c in self.held[l] or c in prev:
                    continue
                prev[c] = l
                h = self.holder.get(c)
                if h is None:
                    # re-assign along the path
                    while True:
                        l = prev[c]
                        old = None
                        self.holder[c] = l; self.held[l].add(c)
                        if l == l0:
                            return True
                        # line l must give up the cell through which it was entered
                        c2 = self.entry[l]
                        self.held[l].discard(c2)
                        c = c2
                if h not in seenl:
                    seenl.add(h)
                    self.entry = getattr(self, 'entry', {})
                    self.entry[h] = c
                    dq.append(h)
        self.reach = seenl
        return False
    def pairing(self):
        for l in self.lv:
            while len(self.held[l]) < 2:
                self.entry = {}
                if not self.augment(l):
                    return False
        return True

def has_pairing(B, X):
    return Flow(B, X).pairing()

def copyflow(fl):
    g = Flow.__new__(Flow)
    g.F = fl.F; g.lv = fl.lv; g.holder = dict(fl.holder); g.held = {l: set(s) for l, s in fl.held.items()}; g.cap = dict(fl.cap)
    return g

def mu(fl0, U, limit):
    """number of additional cells the lines of U can collect starting from the pairing fl0 (at most limit)"""
    fl = copyflow(fl0)
    n = 0
    while n < limit:
        ok = False
        for l in U:
            fl.entry = {}
            if fl.augment(l):
                ok = True; break
        if not ok:
            break
        n += 1
    return n

def adjacency(B, F):
    lv = live(B); F = set(F)
    adj = {l: set() for l in lv}
    for a, b in itertools.combinations(lv, 2):
        m = LSET[a] & LSET[b]
        if m and not (m & F):
            adj[a].add(b); adj[b].add(a)
    return adj

def conn_triples(adj):
    out = set()
    for m in adj:
        for a, b in itertools.combinations(sorted(adj[m]), 2):
            out.add(tuple(sorted((a, m, b))))
    return out

def leaf(B, F, k):
    if not admissible(B, F):
        return (False, 1)
    fl = Flow(B, F)
    if not fl.pairing():
        return (False, 2)
    theta = k - len(set(F))
    adj = adjacency(B, F)
    for T in sorted(conn_triples(adj)):
        if mu(fl, T, theta) < theta:
            return (False, 3)
    return (True, 0)

def sur(S, F=()):
    N = set()
    for l in S:
        N |= LSET[l]
    return len(N - set(F)) - 2 * len(S)

def connected(S, adj):
    S = set(S)
    if not S:
        return True
    st = [next(iter(S))]; seen = set(st)
    while st:
        a = st.pop()
        for b in adj[a]:
            if b in S and b not in seen:
                seen.add(b); st.append(b)
    return seen == S

C = lambda x, y, z: 36 * x + 6 * y + z
T7 = [C(0, 0, 0), C(5, 5, 5), C(1, 1, 4), C(4, 4, 1), C(2, 3, 2), C(3, 2, 3), C(5, 0, 0)]
T8 = T7 + [C(0, 5, 5)]
for name, T in (("T7", T7), ("T8", T8)):
    lv = live(T)
    pairs = [(a, b) for a, b in itertools.combinations(sorted(T), 2) if any(a in LSET[i] and b in LSET[i] for i in range(148))]
    adj = adjacency(T, ())
    N = set().union(*[LSET[l] for l in lv])
    print(name, "cells", sorted(T), "killed", 148 - len(lv), "live", len(lv), "slack", slack(T),
          "collinear pairs", len(pairs), "pairing", has_pairing(T, ()),
          "| all live lines: connected", connected(lv, adj), "cover", len(N), "cells, sur", sur(lv))
# fewer than 7 stones: counting bound
print("counting bound: 6 stones leave >=", 148 - 42, "live lines, need", 2 * (148 - 42), "cells, have", 216 - 6)
print("LEAF(T8,-,8) =", leaf(T8, (), 8), " LEAF(T8,-,9) =", leaf(T8, (), 9), " LEAF(T7,-,7) =", leaf(T7, (), 7))
X9 = [C(1, 0, 0), C(2, 0, 0), C(3, 0, 0), C(0, 1, 0), C(0, 2, 0), C(0, 3, 0), C(0, 0, 1), C(0, 0, 2), C(0, 0, 3)]
print("X9 admissible for T8:", admissible(T8, X9), " pairing for (T8,X9):", has_pairing(T8, X9),
      "; any 2 cells admissible for T7, e.g.", admissible(T7, X9[:2]), "pairing:", has_pairing(T7, X9[:2]))

# ---------------------------------------------------------------- connected triples of the empty board (Lemma on 4+3+3 stones)
adj0 = adjacency((), ())
tri = conn_triples(adj0)
kinds = {"triangle": 0, "star": 0, "path": 0}
bad = 0
for T in tri:
    a, b, c = T
    N = LSET[a] | LSET[b] | LSET[c]
    m = [LSET[a] & LSET[b], LSET[a] & LSET[c], LSET[b] & LSET[c]]
    nm = sum(1 for s in m if s)
    if nm == 3 and len(m[0] | m[1] | m[2]) == 3:
        kinds["triangle"] += 1; assert len(N) == 15
    elif nm == 3:
        kinds["star"] += 1; assert len(N) == 16 and len(m[0] | m[1] | m[2]) == 1
    else:
        kinds["path"] += 1; assert nm == 2 and len(N) == 16
    for first in range(3):
        order = [T[first]] + [T[j] for j in range(3) if j != first]
        X = []
        for idx, l in enumerate(order):
            priv = sorted(LSET[l] - set().union(*[LSET[o] for o in order if o != l]))
            assert len(priv) >= 4
            X += priv[:4 if idx == 0 else 3]
        if len(set(X)) != 10 or not admissible((), X):
            bad += 1
print("connected triples of the empty board:", len(tri), kinds, "inadmissible 4+3+3 placements:", bad)

# ---------------------------------------------------------------- busy 10-sets: the triangle argument
BUSY = sorted(cid[c] for c in busy)
facediag = [i for i, L in enumerate(lines) if ltype(L) == 2]
assert len(facediag) == 36
assert all(sum(1 for i in facediag if b in LSET[i]) == 3 for b in BUSY)
nobusy = [i for i in range(148) if not (LSET[i] & set(BUSY))]
print("lines without busy cell:", len(nobusy), "(all axis lines:", all(ltype(lines[i]) == 1 for i in nobusy), ")")
okd = 0
for d in facediag:
    found = False
    for r, s in itertools.combinations(nobusy, 2):
        a, b, c2 = LSET[d] & LSET[r], LSET[d] & LSET[s], LSET[r] & LSET[s]
        if a and b and c2 and len(a | b | c2) == 3:
            found = True; break
    okd += found
print("face diagonals that form a triangle with two busy-free lines:", okd, "of 36")
# explicit construction of the paper: plane z = c (and permutations), row r, column s
cnt = 0
for d in facediag:
    L = lines[d]
    const = [i for i in range(3) if len({p[i] for p in L}) == 1][0]
    cval = next(iter(L))[const]
    act = [i for i in range(3) if i != const]
    main = all(p[act[0]] == p[act[1]] for p in L)
    for r in range(6):
        for s in range(6):
            if r in (cval, 5 - cval) or s in (cval, 5 - cval):
                continue
            if (main and r == s) or (not main and s == 5 - r):
                continue
            row = frozenset(tuple(cval if i == const else (t if i == act[0] else r) for i in range(3)) for t in range(6))
            col = frozenset(tuple(cval if i == const else (s if i == act[0] else t) for i in range(3)) for t in range(6))
            assert row in set(lines) and col in set(lines)
            assert not any(p in busy for p in row) and not any(p in busy for p in col)
            m1, m2, m3 = L & row, L & col, row & col
            assert len(m1) == len(m2) == len(m3) == 1 and len(m1 | m2 | m3) == 3 and len(L | row | col) == 15
            cnt += 1
print("explicit (diagonal,row,column) triangles checked:", cnt, "= 36*12 =", 36 * 12)

# ---------------------------------------------------------------- C true, LEAF false (three lines through (0,0,0) in the plane x = 0)
Tl = [i for i, L in enumerate(lines) if L in (frozenset((0, 0, t) for t in range(6)), frozenset((0, t, 0) for t in range(6)), frozenset((0, t, t) for t in range(6)))]
assert len(Tl) == 3
NT = sorted(set().union(*[LSET[l] for l in Tl]))
Bx = [c for c in range(216) if c not in NT]
assert len(NT) == 16 and len(Bx) == 200 and live(Bx) == sorted(Tl)
print("example: |N(T)| = 16, L(B) = T:", live(Bx) == sorted(Tl), " sur(T) =", sur(Tl), " LEAF(B,-,11) =", leaf(Bx, (), 11), " LEAF(B,-,10) =", leaf(Bx, (), 10))
nadm = 0; maxsz = 0; hallfail = 0
subsetsT = [s for k in range(1, 4) for s in itertools.combinations(Tl, k)]
for mask in range(1 << 16):
    X = [NT[i] for i in range(16) if mask >> i & 1]
    Xs = set(X)
    cnts = [len(LSET[l] & Xs) for l in Tl]
    if max(cnts) > 4:
        continue
    if 0 not in Xs and sum(1 for v in cnts if v == 4) >= 2:
        continue
    nadm += 1; maxsz = max(maxsz, len(X))
    for S in subsetsT:
        N = set().union(*[LSET[l] for l in S]) - Xs
        if len(N) < 2 * len(S):
            hallfail += 1
print("example: admissible subsets of N(T):", nadm, " largest:", maxsz, " Hall violations:", hallfail)
# sanity: the shortcut admissibility test used above equals the general one on a sample
random.seed(2)
for _ in range(300):
    mask = random.randrange(1 << 16)
    X = [NT[i] for i in range(16) if mask >> i & 1]
    Xs = set(X); cnts = [len(LSET[l] & Xs) for l in Tl]
    quick = max(cnts) <= 4 and not (0 not in Xs and sum(1 for v in cnts if v == 4) >= 2)
    assert quick == admissible(Bx, X)

# ---------------------------------------------------------------- appendix: counterexample for inadmissible F
l = LINES[0]; b = l[0]; F = l[1:]
print("appendix counterexample: line", l, "b =", b, " F admissible for {}:", admissible((), F), " for {b}:", admissible([b], F),
      " live lines for {b}:", len(live([b])), " free cells:", 216 - 1 - 5, " pairing for ({b},F):", has_pairing([b], F))
print("ALL CHECKS PASSED")

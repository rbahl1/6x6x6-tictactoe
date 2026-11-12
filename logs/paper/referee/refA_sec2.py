# refA_sec2.py -- referee 1a: brute-force tests of the statements of Section 2.
import itertools, random, sys
from refA_geom import *

out = []
def say(*a):
    s = ' '.join(str(x) for x in a)
    out.append(s); print(s); sys.stdout.flush()

# --- Lemma 2.1 -----------------------------------------------------------------
assert len(LINES) == 148
assert all(len(l) == 6 for l in LINES)
kinds = [kind(l) for l in LINES]
say('lines', len(LINES), 'axis/face/space =', kinds.count(1), kinds.count(2), kinds.count(3))
assert (kinds.count(1), kinds.count(2), kinds.count(3)) == (108, 36, 4)
mx = max(len(a & b) for a, b in itertools.combinations(LINE_IDS, 2))
say('max |a cap b| over distinct lines =', mx)
assert mx == 1
# the paper's definition of a line (parametrizations) gives the same 148 sets, each twice
par = {}
maps = [('c', v) for v in range(6)] + [('id', None), ('rho', None)]
def ev(m, t):
    return m[1] if m[0] == 'c' else (t if m[0] == 'id' else 5 - t)
for f in itertools.product(maps, repeat=3):
    if all(m[0] == 'c' for m in f):
        continue
    l = frozenset(CID[tuple(ev(m, t) for m in f)] for t in range(6))
    par.setdefault(l, []).append(f)
assert set(par) == set(LINE_IDS) and all(len(v) == 2 for v in par.values())
say('parametrizations: 296, every line has exactly two; line set equals the walk-generated set')

# --- Corollary 2.2 (collinearity test) -----------------------------------------
pairline = {}
for li, l in enumerate(LINE_IDS):
    for a, b in itertools.combinations(sorted(l), 2):
        pairline[(a, b)] = li
bad = 0
for p, q in itertools.combinations(CELLS, 2):
    col = (min(CID[p], CID[q]), max(CID[p], CID[q])) in pairline
    test = False
    for t1 in range(6):
        for t2 in range(6):
            if t1 != t2 and all((p[i], q[i]) in ((t1, t2), (5 - t1, 5 - t2)) for i in range(3) if p[i] != q[i]):
                test = True
    if col != test:
        bad += 1
    if col:
        nd = sum(1 for i in range(3) if p[i] != q[i])
        assert kind(LINES[pairline[(min(CID[p], CID[q]), max(CID[p], CID[q]))]]) == nd
say('Cor 2.2: mismatches over all', 216 * 215 // 2, 'pairs =', bad, '; collinear pairs =', len(pairline))
assert bad == 0 and len(pairline) == 148 * 15

# --- Lemma 2.3 (lines through a cell) ------------------------------------------
deg = {c: len(THROUGH[CID[c]]) for c in CELLS}
hist = {}
for c in CELLS:
    nl = len({lam(v) for v in c})
    hist.setdefault((nl, deg[c]), 0)
    hist[(nl, deg[c])] += 1
    # exactly one line for every admissible active set, none otherwise
    for A in [(0,), (1,), (2,), (0, 1), (0, 2), (1, 2), (0, 1, 2)]:
        cnt = sum(1 for li in THROUGH[CID[c]]
                  if tuple(j for j in range(3) if len({p[j] for p in LINES[li]}) > 1) == A)
        want = 1 if len({lam(c[i]) for i in A}) == 1 else 0
        assert cnt == want, (c, A, cnt)
say('Lemma 2.3: (number of distinct levels, degree) -> cells:', sorted(hist.items()))
assert hist == {(1, 7): 24, (2, 4): 144, (3, 3): 48}
assert sum(deg.values()) == 888

# --- Lemma 2.5 (busy cells) ----------------------------------------------------
assert len(BUSY) == 24 and all(deg[c] == 7 for c in BUSY)
space = [l for l in LINES if kind(l) == 3]
assert all(is_busy(c) for l in space for c in l)
assert all(sorted(lam(c[0]) for c in l) == [0, 0, 1, 1, 2, 2] for l in space)
assert all(antip(c) in l for l in space for c in l)
assert sum(len(l) for l in space) == 24 and len(set().union(*space)) == 24
nb = lambda l: sum(1 for c in l if is_busy(c))
assert all(nb(l) == 2 for l in LINES if kind(l) == 2)
ax = [l for l in LINES if kind(l) == 1]
assert set(nb(l) for l in ax) == {0, 2}
ax0 = [l for l in ax if nb(l) == 0]
say('Lemma 2.5(b): axis lines without busy cell =', len(ax0))
assert len(ax0) == 72
for l in ax:
    pts = list(l)
    const = [pts[0][j] for j in range(3) if len({p[j] for p in pts}) == 1]
    assert (nb(l) == 0) == (lam(const[0]) != lam(const[1]))
bad = 0
for p, q in itertools.combinations(BUSY, 2):
    col = (min(CID[p], CID[q]), max(CID[p], CID[q])) in pairline
    same_level = lam(p[0]) == lam(q[0])
    same_diag = any(p in l and q in l for l in space)
    if col != (same_level or same_diag):
        bad += 1
say('Lemma 2.5(c): mismatches over busy pairs =', bad)
assert bad == 0

# --- Lemma 2.6 (the group) -----------------------------------------------------
G = group_G()
say('|H| =', len(H_perms()), ' triples (s,r,pi) =', 48 * 8 * 6, ' distinct maps |G| =', len(G))
assert len(H_perms()) == 48 and len(G) == 1152
Gl = list(G)
Gset = set(Gl)
lineset = set(LINE_IDS)
kind_of = {l: kind(LINES[i]) for i, l in enumerate(LINE_IDS)}
for g in Gl:
    assert sorted(g) == list(range(216))
    for l in LINE_IDS:
        gl = frozenset(g[i] for i in l)
        assert gl in lineset and kind_of[gl] == kind_of[l]
    assert sorted(g[i] for i in BUSY_IDS) == BUSY_IDS
    for c in CELLS:
        assert g[CID[antip(c)]] == CID[antip(IDC[g[CID[c]]])]
say('every g in G: bijection, maps lines onto lines of the same kind, preserves busy cells, commutes with antipode')
random.seed(1)
for _ in range(20000):
    a, b = random.choice(Gl), random.choice(Gl)
    assert tuple(a[b[i]] for i in range(216)) in Gset
for a in Gl:
    inv = [0] * 216
    for i in range(216):
        inv[a[i]] = i
    assert tuple(inv) in Gset
say('closure: 20000 random products in G, all 1152 inverses in G')
# each map described exactly twice
# (count of triples is 2304 and |G| = 1152; check no map has 3 descriptions)
cntdesc = {}
Hs = H_perms()
# orbits of G on cells (Appendix A.1 claims three orbits = the three degree classes)
orb = {}
for i in range(216):
    orb[i] = min(g[i] for g in Gl)
classes = {}
for i in range(216):
    classes.setdefault(orb[i], []).append(i)
say('orbits of G on cells: sizes', sorted(len(v) for v in classes.values()), 'representatives', sorted(classes))
assert sorted(len(v) for v in classes.values()) == [24, 48, 144]

# --- order on sets / canonical form (Section 2.5) -------------------------------
def less(A, Ap):          # the paper's order: smallest cell of the symmetric difference belongs to A
    d = set(A) ^ set(Ap)
    return min(d) in A
random.seed(2)
for _ in range(20000):
    n = random.randint(0, 6)
    A = frozenset(random.sample(range(30), n)); Ap = frozenset(random.sample(range(30), n))
    if A == Ap:
        continue
    assert less(A, Ap) == (sorted(A) < sorted(Ap))
    assert less(A, Ap) != less(Ap, A)
# transitivity on random triples of arbitrary sizes
for _ in range(20000):
    S = [frozenset(random.sample(range(12), random.randint(0, 6))) for _ in range(3)]
    if len(set(S)) < 3:
        continue
    a, b, c = S
    if less(a, b) and less(b, c):
        assert less(a, c)
say('order of Section 2.5: total, transitive (random test), equals lexicographic order of sorted lists for equal sizes')

def canon(B, F):
    best = None
    for g in Gl:
        key = (sorted(g[i] for i in B), sorted(g[i] for i in F))
        if best is None or key < best:
            best = key
    return best
T7 = [cid(0,0,0), cid(5,5,5), cid(5,0,0), cid(1,1,4), cid(4,4,1), cid(2,3,2), cid(3,2,3)]
T8 = T7 + [cid(0,5,5)]
say('canon(T7,-) =', canon(T7, [])[0], ' (paper, App. A.2: 0,5,61,93,122,154,210)')
say('canon(T8,-) =', canon(T8, [])[0], ' (paper: 0,5,61,93,122,154,210,215)')
say('canon({43},{0}) =', canon([43], [0]), ' (paper: 0 | 43)')
assert canon(T7, [])[0] == [0, 5, 61, 93, 122, 154, 210]
assert canon(T8, [])[0] == [0, 5, 61, 93, 122, 154, 210, 215]
assert canon([43], [0]) == ([0], [43])
c4 = canon([5,30,43,64,86,93,128,169,180], [0,35,46,87,122,151,172,185,210])
say('canon(4th vector) =', c4)
assert c4 == ([0,35,43,64,86,93,123,154,210], [5,30,61,122,129,151,172,180,215])
# Lemma 2.8 on random pairs
random.seed(3)
for _ in range(60):
    nB, nF = random.randint(0, 5), random.randint(0, 4)
    cells = random.sample(range(216), nB + nF + 1)
    B, F, m = cells[:nB], cells[nB:nB + nF], cells[-1]
    h = random.choice(Gl)
    cB = canon(B, F)
    assert canon([h[i] for i in B], [h[i] for i in F]) == cB                  # (a)
    gs = [g for g in Gl if (sorted(g[i] for i in B), sorted(g[i] for i in F)) == cB]
    for g in gs[:3]:
        mc = g[m]
        assert canon(B + [m], F) == canon(cB[0] + [mc], cB[1])               # (b)
        assert canon(B, F + [m]) == canon(cB[0], cB[1] + [mc])
say('Lemma 2.8 (a),(b): 60 random instances ok')
say('SEC2 ALL OK')

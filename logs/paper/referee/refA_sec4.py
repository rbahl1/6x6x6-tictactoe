# refA_sec4.py -- referee 1a: brute-force tests of the statements of Section 4 and of
# Proposition 7.2 (Algorithm 2) on the real 6x6x6 geometry.
import itertools, random, sys, time
from refA_geom import *
from refA_lib import *
from refA_small import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

t0 = time.time()
T7 = [cid(0,0,0), cid(5,5,5), cid(5,0,0), cid(1,1,4), cid(4,4,1), cid(2,3,2), cid(3,2,3)]
T8 = T7 + [cid(0,5,5)]

# ---------- Lemma 4.2 / 4.3 on random positions of the full board -----------------
rng = random.Random(11)
cnt = {True: 0, False: 0}
for it in range(4000):
    B = rng.sample(range(216), rng.randint(0, 12))
    Bm = mask(B)
    L = live(Bm)
    # X: stones concentrated on a few live lines so that 4s, 5s and danger pairs occur
    X = set()
    for _ in range(rng.randint(1, 4)):
        l = rng.choice(L)
        X |= set(rng.sample(sorted(LINE_IDS[l]), rng.randint(2, 5)))
    if rng.random() < 0.5:                       # a second line through a cell of the first ones
        c = rng.choice(sorted(X))
        ls = [l for l in THROUGH[c] if l in L]
        if ls:
            l = rng.choice(ls)
            X |= set(rng.sample(sorted(LINE_IDS[l]), rng.randint(3, 4)))
    X -= set(B)
    Xm = mask(X)
    adm = admissible(Bm, Xm, L)
    condA = all(6 - popcount(LMASK[l] & Xm) >= 2 for l in L)
    condB = all(popcount((LMASK[a] | LMASK[b]) & ~Xm) >= 4 for a, b in itertools.combinations(L, 2))
    assert adm == (condA and condB), (B, X)
    cnt[adm] += 1
    if adm:                                       # Lemma 4.2 (a), (b)
        Xs = mask(rng.sample(sorted(X), rng.randint(0, len(X))))
        assert admissible(Bm, Xs, L)
        free = [c for c in range(216) if not ((Bm | Xm) >> c & 1)]
        Bp = Bm | mask(rng.sample(free, 3))
        assert admissible(Bp, Xm)
say('Lemma 4.3 (equivalence) and 4.2 on 4000 random (B,X): admissible', cnt[True], 'not admissible', cnt[False])

# ---------- Example 4.11 -----------------------------------------------------------
o = cid(0, 0, 0)
a1 = frozenset(cid(0, 0, t) for t in range(6)); a2 = frozenset(cid(0, t, 0) for t in range(6))
a3 = frozenset(cid(0, t, t) for t in range(6))
Tl = [LINE_IDS.index(a) for a in (a1, a2, a3)]
NT = mask(a1 | a2 | a3)
Bm = FULL & ~NT
inst = Small(Bm)
assert popcount(NT) == 16 and popcount(Bm) == 200 and sorted(inst.L) == sorted(Tl)
ph = inst.phi(0)
mx = 0; n_adm = 0; allpair = True
for sub in range(1 << 16):
    Xm = 0
    for i, c in enumerate(inst.cells):
        if sub >> i & 1:
            Xm |= 1 << c
    if inst.adm(Xm):
        n_adm += 1
        mx = max(mx, popcount(Xm))
        if not inst.has_pairing(Xm):
            allpair = False
say('Example 4.11: live lines = T:', sorted(inst.L) == sorted(Tl), ' Phi(B,0) =', ph, ' LEAF(B,0,11) =', inst.leaf(0, 11),
    ' LEAF(B,0,10) =', inst.leaf(0, 10), '; admissible X (all 2^16 subsets of N(T) tried):', n_adm,
    ' largest', mx, ' all have a pairing:', allpair, ' => C(B,0,11) true')
assert ph == 10 and not inst.leaf(0, 11) and mx == 10 and allpair

# ---------- Section 4.5: connected triples on the empty board ----------------------
kinds = {}
for a, b, c in itertools.combinations(range(148), 3):
    m = [bool(LMASK[x] & LMASK[y]) for x, y in ((a, b), (a, c), (b, c))]
    if sum(m) < 2:
        continue
    u = popcount(LMASK[a] | LMASK[b] | LMASK[c])
    if sum(m) == 3:
        k = 'star' if LMASK[a] & LMASK[b] & LMASK[c] else 'triangle'
    else:
        k = 'path'
    kinds.setdefault((k, u), 0); kinds[(k, u)] += 1
say('connected triples of lines (kind, |N(T)|) -> count:', sorted(kinds.items()))
assert set(kinds) == {('triangle', 15), ('star', 16), ('path', 16)}

# ---------- Lemma 4.19 ---------------------------------------------------------------
rng = random.Random(12)
for it in range(3000):
    B = rng.sample(range(216), rng.randint(0, 10)); Bm = mask(B); L = live(Bm)
    a = rng.sample(L, 3)
    priv = [[c for c in LINE_IDS[x] if all(c not in LINE_IDS[y] for y in a if y != x)] for x in a]
    assert all(len(p) >= 4 for p in priv)
    XT = rng.sample(priv[0], 4) + rng.sample(priv[1], 3) + rng.sample(priv[2], 3)
    assert len(set(XT)) == 10 and admissible(Bm, mask(XT), L)
    sub = rng.sample(XT, rng.randint(0, 10))
    assert admissible(Bm, mask(sub), L)
say('Lemma 4.19: 3000 random triples of live lines, X_T and random subsets admissible')

# ---------- Prop 4.22: the triangle of the proof, for every face diagonal -----------
face = [i for i in range(148) if kind(LINES[i]) == 2]
nb = lambda l: sum(1 for c in LINES[l] if is_busy(c))
ntri = 0
for d in face:
    pts = sorted(LINES[d])
    j = [j for j in range(3) if len({p[j] for p in pts}) == 1][0]     # constant coordinate
    v = pts[0][j]
    others = [i for i in range(3) if i != j]
    vals = [u for u in range(6) if lam(u) != lam(v)]
    for r in vals:
        for s in vals:
            if s in (r, 5 - r):
                continue
            # a: coordinate others[0] runs, others[1] = r ; b: others[0] = s, others[1] runs
            def cell(u0, u1):
                c = [0, 0, 0]; c[j] = v; c[others[0]] = u0; c[others[1]] = u1
                return cid(*c)
            a = frozenset(cell(t, r) for t in range(6)); b = frozenset(cell(s, t) for t in range(6))
            assert a in LINE_IDS and b in LINE_IDS
            ai, bi = LINE_IDS.index(a), LINE_IDS.index(b)
            assert nb(ai) == 0 and nb(bi) == 0
            m = [LINE_IDS[d] & a, LINE_IDS[d] & b, a & b]
            assert all(len(x) == 1 for x in m) and len(set().union(*m)) == 3
            ntri += 1
say('Prop 4.22: for each of the 36 face diagonals and each admissible choice of (r,s): a, b busy-free, d,a,b a triangle;',
    ntri, 'triangles checked')
# the caption of Figure 2 uses v=1, r=2, s=3:
say('   Figure 2 (v=1, r=2, s=3): s in {r, 5-r}?', 3 in (2, 5 - 2), ' -> violates the choice made in the proof (r, s from different pairs)')
# independent of the proof: every set of <= 11 busy cells leaves a live triangle; what about 12?
import numpy as np
busy = BUSY_IDS; pos = {c: i for i, c in enumerate(busy)}
bm = [sum(1 << pos[c] for c in l if c in pos) for l in LINE_IDS]
tri_masks = set()
for a, b, c in itertools.combinations(range(148), 3):
    if LMASK[a] & LMASK[b] and LMASK[a] & LMASK[c] and LMASK[b] & LMASK[c] and not (LMASK[a] & LMASK[b] & LMASK[c]):
        tri_masks.add(bm[a] | bm[b] | bm[c])
minimal = [m for m in tri_masks if not any((o & m) == o and o != m for o in tri_masks)]
say('   triangles: distinct busy-footprints', len(tri_masks), ' minimal ones', len(minimal),
    ' sizes', sorted(set(bin(m).count('1') for m in minimal)))
def subsets_of_size(n, k):
    # all k-subsets of range(n) as uint32 masks (Gosper)
    out = np.empty(len(list(itertools.combinations(range(n), k))) if False else __import__('math').comb(n, k), dtype=np.uint32)
    x = (1 << k) - 1; i = 0; lim = 1 << n
    while x < lim:
        out[i] = x; i += 1
        c = x & -x; r = x + c
        x = (((r ^ x) >> 2) // c) | r
    return out
mins = np.array(minimal, dtype=np.uint32)
deadk = {}
for k in (11, 12, 17, 18):
    S = subsets_of_size(24, k)
    alive = np.zeros(len(S), dtype=bool)
    for m in mins:
        alive |= (S & m) == 0
    say('   busy %d-sets: %d ; without a live triangle: %d' % (k, len(S), int((~alive).sum())))
    if k <= 17:
        assert alive.all()
    deadk[k] = S[~alive]
if len(deadk[18]):
    ex = int(deadk[18][0])
    say('   example of a busy 18-set without a live triangle:', [IDC[busy[i]] for i in range(24) if ex >> i & 1])

# ---------- small instances: Thm 4.9, Rem 4.10, Prop 4.13, 4.15, Cor 4.17, Prop 4.18, 4.20, Cor 4.21, Prop 7.2 ----
rng = random.Random(13)
stat = dict(inst=0, leaf=0, C=0, C_not_leaf=0, alg_mismatch=0, mu_checks=0, phi_inf=0, eq_checks=0, comp_checks=0,
            mono_a=0, mono_c=0, slack_checks=0, order_checks=0, closed_checks=0)
for it in range(700):
    inst = random_small(rng, (3, 6), (0, 2), 8)
    Fm = random_F(rng, inst, 5, admissible_only=(rng.random() < 0.9))
    nF = popcount(Fm)
    L = inst.L
    stat['inst'] += 1
    kmax = nF + rng.randint(0, 6)
    ph = inst.phi(Fm)
    if ph is None:
        stat['phi_inf'] += 1
    for k in range(nF, kmax + 1):
        lf = inst.leaf(Fm, k)
        la = leaf_alg2(inst.Bm, Fm, k, L)
        if lf != la:
            stat['alg_mismatch'] += 1
            say('MISMATCH Alg 2 vs definition', bits(FULL & ~inst.Bm), bits(Fm), k, lf, la)
        if it < 260 and k - nF <= 4:
            c, bad = inst.C(Fm, k)
            assert (not lf) or c, 'LEAF without C'            # Theorem 4.9
            if c:
                stat['C'] += 1
                if not lf and inst.adm(Fm):
                    stat['C_not_leaf'] += 1
            if k == nF and inst.adm(Fm):
                assert c == lf                                  # Remark 4.10 (iii)
        stat['leaf'] += lf
    if not inst.adm(Fm):
        continue
    a = pairing(L, Fm)
    assert (a is not None) == inst.has_pairing(Fm)
    if a is None:
        continue
    # Cor 4.17 (b)
    for S in components(L, Fm):
        NS = 0
        for l in S:
            NS |= LMASK[l]
        un = sum(1 for c in bits(NS & ~Fm) if c not in a.own)
        assert un == sur(S, Fm)
        stat['comp_checks'] += 1
    # Prop 4.13 (b), 4.15 (c),(d),(e) for all F-connected triples and some other U
    triples = [T for T in itertools.combinations(L, 3) if is_connected(T, Fm)]
    mus = []
    Us = [list(T) for T in triples]
    for _ in range(4):
        Us.append(rng.sample(L, rng.randint(1, min(len(L), 4))))
    for U in Us:
        mb = mu_brute(L, Fm, U)
        n1, b1 = gain_full(a, U)
        n2, b2 = gain_full(a, U, rng)
        n3, b3 = gain_one_at_a_time(a, U)
        assert mb == n1 == n2 == n3, (mb, n1, n2, n3)
        stat['mu_checks'] += 1; stat['order_checks'] += 1
        for b in (b1, b2, b3):
            R = b.reachable(U)
            NR = 0
            for l in R:
                NR |= LMASK[l]
            assert all(b.own.get(c) in R for c in bits(NR & ~Fm))          # closed
            assert sur(sorted(R), Fm) == mb                                 # witness
            if is_connected(U, Fm):
                assert is_connected(sorted(R), Fm)
            # U-assignment: lines outside U hold exactly 2, lines of U at least 2
            assert all(len(b.held(l)) == 2 for l in L if l not in U) and all(len(b.held(l)) >= 2 for l in U)
        # (a): pruning rule mu(U) <= mu(U') for U subset U'
        if len(U) >= 2:
            assert mu_brute(L, Fm, U[:1]) <= mb and mu_brute(L, Fm, U[:2]) <= mb
        if len(U) == 3 and is_connected(U, Fm):
            mus.append(mb)
    assert ph == (min(mus) if mus else None)                               # Prop 4.13 (b)
    # (b) closed sets persist: exhaust u1, then augment from others
    if len(L) >= 2:
        U = rng.sample(L, min(len(L), 3))
        b = a.copy()
        while b.augment([U[0]]):
            pass
        R1 = b.reachable([U[0]])
        NR = 0
        for l in R1:
            NR |= LMASK[l]
        snap = {c: b.own.get(c) for c in bits(NR & ~Fm)}
        assert all(v in R1 for v in snap.values())
        while b.augment(U[1:], rng):
            assert {c: b.own.get(c) for c in snap} == snap
            assert b.find_path([U[0]]) is None
        stat['closed_checks'] += 1
    # Prop 4.18
    k = nF + rng.randint(0, 5)
    if inst.leaf(Fm, k):
        cand = [c for c in inst.cells if not (Fm >> c & 1)]
        if cand and nF < k:
            Fp = Fm | (1 << rng.choice(cand))
            if inst.adm(Fp):
                assert inst.leaf(Fp, k); stat['mono_a'] += 1
        free = [c for c in inst.cells if not (Fm >> c & 1)]
        if free:
            bcell = rng.choice(free)
            i2 = Small(inst.Bm | (1 << bcell))
            assert i2.leaf(Fm, k); stat['mono_c'] += 1
say('small instances:', stat)
assert stat['alg_mismatch'] == 0

# Prop 4.20 / Cor 4.21 : F empty, k <= 10, exhaustive C at the threshold k = Phi and k = Phi+1
rng = random.Random(14)
st = dict(inst=0, C_true=0, C_false=0, slack_viol=0, visited=0, both_impl=0)
tt = time.time()
while st['inst'] < 40 and time.time() - tt < 150:
    inst = random_small(rng, (3, 4), (0, 1), 5)
    if len(inst.cells) > 22 or not inst.has_pairing(0):
        continue
    ph = inst.phi(0)
    ks = sorted(set(k for k in ((ph, ph + 1) if ph is not None else (10,)) if 0 <= k <= 10) | {rng.randint(0, 10)})
    for k in ks:
        lf = inst.leaf(0, k)
        c, bad, vis = inst.C_fast(0, k)
        st['visited'] += vis
        assert c == lf, ('Prop 4.20 fails', bits(FULL & ~inst.Bm), k)
        if not c:
            assert inst.adm(bad) and not inst.has_pairing(bad) and popcount(bad) <= k
        if vis < 3000:
            assert inst.C(0, k)[0] == c; st['both_impl'] += 1
        st['C_true' if c else 'C_false'] += 1
        if lf and len(inst.L) >= 3:
            slack = 216 - popcount(inst.Bm) - 2 * len(inst.L)
            if slack < k:
                st['slack_viol'] += 1
    st['inst'] += 1
say('Prop 4.20 (C <=> LEAF for F empty, k<=10) and Cor 4.21 by exhaustive enumeration:', st)
assert st['slack_viol'] == 0

# ---------- T8: Phi(T8,0) = min over triples of mu(T), every witness recomputed -----
Bm = mask(T8); L = live(Bm)
a = pairing(L, 0)
adj = {l: [x for x in L if x != l and LMASK[x] & LMASK[l]] for l in L}
say('T8: max number of lines adjacent to a live line:', max(len(v) for v in adj.values()),
    '; on the empty board:', max(sum(1 for x in range(148) if x != l and LMASK[x] & LMASK[l]) for l in range(148)))
single = {l: gain_full(a, [l])[0] for l in L}
say('T8: mu({l}) histogram:', sorted((v, list(single.values()).count(v)) for v in set(single.values())))
seen = set(); mmin = None; ntr = 0
rng = random.Random(15)
trs = []
for b in L:
    for x, y in itertools.combinations(adj[b], 2):
        T = frozenset((x, b, y))
        if T not in seen:
            seen.add(T); trs.append(T)
say('T8: connected triples:', len(trs))
sample = trs if len(trs) <= 4000 else rng.sample(trs, 4000)
for T in sample:
    n, bfin = gain_full(a, list(T))
    R = bfin.reachable(T)
    assert sur(sorted(R), 0) == n
    mmin = n if mmin is None else min(mmin, n)
    ntr += 1
say('T8: min mu(T) over', ntr, 'triples =', mmin, '(Fact 4.23: Phi(T8,0) = 8)')
say('SEC4 ALL OK  (%.1f s)' % (time.time() - t0))

# refA_sec3.py -- referee 1a: brute-force tests of the statements of Section 3
# (Lemma 3.2, 3.3, 3.5, Prop 3.6, Lemma 3.7, Examples 3.8/3.9, Fact 3.10) and of Fact 4.23.
import itertools, random, sys
import numpy as np
from refA_geom import *
from refA_lib import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

T7 = [cid(0,0,0), cid(5,5,5), cid(5,0,0), cid(1,1,4), cid(4,4,1), cid(2,3,2), cid(3,2,3)]
T8 = T7 + [cid(0,5,5)]

# ---- Examples 3.8, 3.9 ---------------------------------------------------------
for name, T in (('T7', T7), ('T8', T8)):
    Bm = mask(T)
    L = live(Bm)
    slack = 216 - len(T) - 2 * len(L)
    p = sum(1 for a, b in itertools.combinations(T, 2) if any(a in l and b in l for l in LINE_IDS))
    say(name, ': live lines', len(L), ' dead', 148 - len(L), ' slack', slack, ' collinear pairs p =', p,
        ' formula 148-7|B|+p =', 148 - 7 * len(T) + p, ' 13|B|-80-2p =', 13 * len(T) - 80 - 2 * p)
    a = pairing(L, 0)
    assert a is not None and check_pairing(L, Bm, 0, a)
    assert pairing_kuhn(L, 0) is not None
    NL = 0
    for l in L:
        NL |= LMASK[l]
    say('   pairing for (%s, empty) found and checked against Def 3.1 (two methods); live lines cover' % name,
        popcount(NL), 'cells; connected:', is_connected(L, 0), '; sur(all live lines) =', sur(L, 0))
    # per-space-diagonal counts
    for l in [x for x in range(148) if kind(LINES[x]) == 3]:
        assert popcount(LMASK[l] & Bm) <= 2
assert len(live(mask(T7))) == 104 and len(live(mask(T8))) == 100
# eighth stone: lines through (0,5,5) already dead
dead_before = [l for l in THROUGH[cid(0,5,5)] if LMASK[l] & mask(T7)]
say('lines through (0,5,5) already dead w.r.t. T7:', len(dead_before),
    [sorted(IDC[c] for c in LINE_IDS[l] if c in T7) for l in dead_before])

# ---- Fact 4.23 by Algorithm 2 ---------------------------------------------------
for name, T, k in (('T8', T8, 8), ('T8', T8, 9), ('T7', T7, 1), ('T7', T7, 2)):
    say('LEAF(%s, empty, %d) by Algorithm 2 (literal):' % (name, k), leaf_alg2(mask(T), 0, k, want_reason=True))
# Phi(T8, empty) = 8 and Phi(T7, empty) = 1 :  LEAF true at 8 / 1 gives >=, the set of all live lines gives <=.

# ---- Lemma 3.7 exhaustively over ALL 2^24 sets of busy cells --------------------
busy = BUSY_IDS
pos = {c: i for i, c in enumerate(busy)}
lb = np.array([sum(1 << pos[c] for c in l if c in pos) for l in LINE_IDS], dtype=np.uint32)
space = [i for i in range(148) if kind(LINES[i]) == 3]
pairs = []
for i, j in itertools.combinations(range(24), 2):
    if any(busy[i] in l and busy[j] in l for l in LINE_IDS):
        pairs.append((1 << i) | (1 << j))
pairs = np.array(pairs, dtype=np.uint32)
say('collinear pairs of busy cells:', len(pairs))
def popc(a):
    a = a.copy()
    c = np.zeros(a.shape, dtype=np.uint8)
    while a.any():
        c += (a & 1).astype(np.uint8)
        a >>= 1
    return c
chunk = 1 << 20
viol = 0; tested = 0; excluded = 0
slack_hist8 = {}
maxdead7 = 0
min_dead_fail = None
for start in range(0, 1 << 24, chunk):
    s = np.arange(start, start + chunk, dtype=np.uint32)
    size = popc(s).astype(np.int32)
    livecnt = np.zeros(chunk, dtype=np.int32)
    for m in lb:
        livecnt += ((s & m) == 0)
    p = np.zeros(chunk, dtype=np.int32)
    for m in pairs:
        p += ((s & m) == m)
    ok = np.ones(chunk, dtype=bool)
    for l in space:
        ok &= popc(s & lb[l]) <= 2
    f = 148 - 7 * size + p
    viol += int(np.count_nonzero(ok & (f != livecnt)))
    tested += int(np.count_nonzero(ok)); excluded += int(np.count_nonzero(~ok))
    # also: is the hypothesis needed?  (sets with 3 cells on a space diagonal)
    nf = (~ok) & (f != livecnt)
    if np.count_nonzero(nf) and min_dead_fail is None:
        min_dead_fail = int(s[nf][0])
    sl = 216 - size - 2 * livecnt
    m8 = size == 8
    for v, c in zip(*np.unique(sl[m8], return_counts=True)):
        slack_hist8[int(v)] = slack_hist8.get(int(v), 0) + int(c)
    m7 = size == 7
    if m7.any():
        maxdead7 = max(maxdead7, int((148 - livecnt[m7]).max()))
say('Lemma 3.7: sets satisfying the hypothesis:', tested, ' violations of |L(B)| = 148-7|B|+p(B):', viol,
    ' (sets excluded by the hypothesis:', excluded, ')')
if min_dead_fail is not None:
    ex = [IDC[busy[i]] for i in range(24) if min_dead_fail >> i & 1]
    say('   the hypothesis is needed: the formula fails e.g. for', ex)
say('busy 8-sets by slack:', sorted(slack_hist8.items()))
say('max number of lines met by 7 busy cells:', maxdead7)
assert viol == 0

# ---- Prop 3.6: slack <= 13|B|-80 ; sanity on random sets -------------------------
rng = random.Random(5)
worst = -10**9
for _ in range(20000):
    n = rng.randint(0, 6)
    B = rng.sample(range(216), n) if rng.random() < 0.5 else rng.sample(busy, n)
    sl = 216 - n - 2 * len(live(mask(B)))
    assert sl <= 13 * n - 80 <= -2
    worst = max(worst, sl)
say('Prop 3.6: 20000 random sets with |B|<=6, largest slack seen', worst, '(bound -2)')

# ---- Lemma 3.3 (a),(b) and Lemma 3.5 on small instances --------------------------
rng = random.Random(6)
n_inst = 0; n_pair = 0
for _ in range(1500):
    Bm, L = small_instance(rng, (2, 9), (0, 4))
    if len(L) > 13:
        continue
    free = bits(FULL & ~Bm)
    X = rng.sample(free, rng.randint(0, min(len(free), rng.choice((3, 6, 10, 14)))))
    Xm = mask(X)
    delta = max_deficiency_brute(L, Xm)
    a = pairing(L, Xm); b = pairing_kuhn(L, Xm)
    assert (a is not None) == (delta == 0) == (b is not None)
    # (a): maximum assignment = 2|L| - delta : compute a maximum assignment greedily by augmenting
    asg = Assign(L, Xm)
    tot = 0
    progress = True
    while progress:
        progress = False
        for l in L:
            if len(asg.held(l)) < 2 and asg.augment([l]):
                tot += 1; progress = True
    assert tot == 2 * len(L) - delta, (tot, len(L), delta)
    if a is not None:
        check_pairing(L, Bm, Xm, a)
        slack = 216 - popcount(Bm) - 2 * len(L)
        assert popcount(Xm) <= slack
        assigned = len(a.own)
        assert 216 - popcount(Bm) - popcount(Xm) - assigned == slack - popcount(Xm)
        n_pair += 1
    n_inst += 1
say('Lemma 3.3(a),(b), 3.5: instances', n_inst, 'with pairing', n_pair, ': max assignment = 2|L|-delta, pairing iff delta=0 (brute force over all S)')

# ---- Lemma 3.2: play the pairing strategy from (T8, X), X admissible of size 8 ---
rng = random.Random(7)
games = 0
B0 = mask(T8); L0 = live(B0)
while games < 300:
    free = [c for c in range(216) if not (B0 >> c & 1)]
    X = rng.sample(free, 8)
    Xm = mask(X)
    if not admissible(B0, Xm, L0):
        continue
    a = pairing(L0, Xm)
    assert a is not None            # this is C(T8, empty, 8) on a sample
    partner = {}
    for l in L0:
        h = a.held(l); partner[h[0]] = h[1]; partner[h[1]] = h[0]
    Bm, Xc = B0, Xm
    empty = [c for c in range(216) if not ((Bm | Xc) >> c & 1)]
    rng.shuffle(empty)
    emp = set(empty)
    while emp:
        # Maker: adversarial-ish: prefer a cell on a line with most own stones and no Breaker stone
        best = None; bs = -2
        cand = rng.sample(sorted(emp), min(len(emp), 12))
        for c in cand:
            sc = max([popcount(LMASK[l] & Xc) for l in THROUGH[c] if not (LMASK[l] & Bm)] + [-1])
            if sc > bs:
                bs, best = sc, c
        x = best
        emp.discard(x); Xc |= 1 << x
        assert all((LMASK[l] & Xc) != LMASK[l] for l in THROUGH[x])
        assert emp, 'Breaker to move on a full board'
        if x in partner and partner[x] in emp:
            z = partner[x]
        else:
            z = min(emp)
        emp.discard(z); Bm |= 1 << z
    assert all((LMASK[l] & Xc) != LMASK[l] for l in range(148))
    games += 1
say('Lemma 3.2: 300 games from (T8, X), X random admissible 8-sets: a pairing always existed, Breaker always had a move, Maker never completed a line')
say('SEC3 ALL OK')

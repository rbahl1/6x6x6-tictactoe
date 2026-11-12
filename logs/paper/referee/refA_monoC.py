# refA_monoC.py -- referee 1a: Proposition C.1 (monotonicity of C in Breaker's stones).
#  (b) the counterexample, for every line l and every b in l;
#  (a) the statement, by exhaustive enumeration on small instances of the real geometry, and
#      the construction of the proof (X^o, S', Claims 1 and 2), step by step, for every
#      (B+b)-admissible X and every S (also when C(B,F,k) is false: the claims do not need it).
import itertools, random, sys, time
from refA_geom import *
from refA_lib import *
from refA_small import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

t0 = time.time()
# ---------------- (b) ----------------------------------------------------------------
n = 0; npair = 0
rng = random.Random(21)
for li in range(148):
    for b in sorted(LINE_IDS[li]):
        Fm = LMASK[li] & ~(1 << b)
        assert not admissible(0, Fm)                         # C(empty,F,5) vacuous
        Bm = 1 << b
        Lb = live(Bm)
        assert admissible(Bm, Fm, Lb)                        # F is {b}-admissible
        assert len(Lb) >= 141 and 2 * len(Lb) > 216 - 6      # no pairing by counting
        if rng.random() < 0.03:
            assert pairing(Lb, Fm) is None; npair += 1       # ... and by actual matching
        n += 1
say('Prop C.1 (b): all', n, 'pairs (l, b): F not 0-admissible, F {b}-admissible, 2|L({b})| > 210;',
    'matching confirms "no pairing" on', npair, 'sampled cases')

# ---------------- (a) ----------------------------------------------------------------
def deficiency(S, Xm):
    u = 0
    for l in S:
        u |= LMASK[l]
    return 2 * len(S) - popcount(u & ~Xm)

def construct(inst, b, Fm, Xm, S, rng):
    """the construction of the proof of (a); inst: Small instance for B; returns (X^o, S', info)"""
    Bm = inst.Bm
    LB = inst.L
    D = [l for l in LB if LMASK[l] >> b & 1]
    LBb = [l for l in LB if not (LMASK[l] >> b & 1)]
    cnt = lambda l, M: popcount(LMASK[l] & M)
    D5 = [l for l in D if cnt(l, Xm) == 5]
    D4 = [l for l in D if cnt(l, Xm) == 4]
    # fact (alpha)
    assert all(cnt(l, Fm) <= 4 for l in D) and sum(1 for l in D if cnt(l, Fm) == 4) <= 1
    if not D4 and not D5:
        return Xm, list(S), 'trivial'
    c_of = {}
    a_of = {}
    for l in D4:
        rest = bits(LMASK[l] & ~Xm)
        assert b in rest and len(rest) == 2
        c_of[l] = [c for c in rest if c != b][0]
        al = [a for a in LBb if LMASK[a] >> c_of[l] & 1 and cnt(a, Xm) == 4]
        assert len(al) <= 1                                   # fact (beta)
        if al:
            a_of[l] = al[0]
    four = [l for l in D if cnt(l, Fm) == 4]
    if four:
        l0 = four[0]
        assert l0 in D4 or l0 in D5
    elif D5:
        l0 = rng.choice(D5)
    else:
        l0 = rng.choice(D4)
    R = set()
    for l in D5:
        if l != l0:
            pool = bits(LMASK[l] & Xm & ~Fm); assert len(pool) >= 2
            R |= set(rng.sample(pool, 2))
    for l in D4:
        if l != l0:
            pool = bits(LMASK[l] & Xm & ~Fm); assert len(pool) >= 1
            R.add(rng.choice(pool))
    eps = 0; a0 = None
    if l0 in D5:
        pool = bits(LMASK[l0] & Xm & ~Fm); assert len(pool) >= 1
        R.add(rng.choice(pool))
    elif l0 in a_of:
        a0 = a_of[l0]
        pool = bits((LMASK[l0] | LMASK[a0]) & Xm & ~Fm); assert len(pool) >= 1      # (R3) exists
        R.add(rng.choice(pool)); eps = 1
    assert len(R) <= 2 * len(D5) + len(D4) - 1 + eps                                  # (17)
    Xo = Xm & ~mask(R)
    Sp = list(S) + D4 + D5
    if eps and a0 not in Sp:
        Sp.append(a0)
    return Xo, Sp, 'case l0 in %s, eps=%d, a0 in S: %s' % ('D5' if l0 in D5 else 'D4', eps, a0 in S if eps else '-')

rng = random.Random(22)
st = dict(instances=0, C_true=0, C_false=0, b_tested=0, X_new_adm=0, X_not_B_adm=0, constructions=0,
          claim_fail=0, statement_fail=0, stmt_checked_on_new_X=0)
cases = {}

def run_instance(inst, Fm, k, bs):
    nF = popcount(Fm)
    cB, _ = inst.C(Fm, k)
    st['instances'] += 1
    st['C_true' if cB else 'C_false'] += 1
    for b in bs:
        inst2 = Small(inst.Bm | (1 << b))
        st['b_tested'] += 1
        assert sorted(inst2.L) == sorted(l for l in inst.L if not (LMASK[l] >> b & 1))
        cand = [c for c in inst.cells if c != b and not (Fm >> c & 1)]
        assert inst2.adm(Fm), 'F must be (B+b)-admissible (Lemma 4.2 (b))'
        stack = [(Fm, 0, nF)]
        while stack:
            Xm, start, size = stack.pop()
            st['X_new_adm'] += 1
            notB = not inst.adm(Xm)
            st['X_not_B_adm'] += notB
            if cB:
                st['stmt_checked_on_new_X'] += notB
                if not inst2.has_pairing(Xm):
                    st['statement_fail'] += 1
                    say('STATEMENT FAILS', bits(FULL & ~inst.Bm), b, bits(Fm), k, bits(Xm))
            Ss = []
            n2 = inst2.n
            if n2:
                best = max(range(1 << n2), key=lambda s: 2 * inst2.size[s] - popcount(inst2.union[s] & ~Xm))
                Ss.append([inst2.L[i] for i in range(n2) if best >> i & 1])
                for _ in range(2 if notB else 0):
                    s = rng.randrange(1 << n2)
                    Ss.append([inst2.L[i] for i in range(n2) if s >> i & 1])
            else:
                Ss.append([])
            for S in Ss:
                Xo, Sp, info = construct(inst, b, Fm, Xm, S, rng)
                cases[info] = cases.get(info, 0) + 1
                st['constructions'] += 1
                ok = (Xo & Fm) == Fm and (Xo & ~Xm) == 0 and inst.adm(Xo) and all(l in inst.L for l in Sp) \
                     and len(set(Sp)) == len(Sp) and deficiency(Sp, Xo) >= deficiency(S, Xm)
                if not ok:
                    st['claim_fail'] += 1
                    say('CLAIM FAILS', info, bits(FULL & ~inst.Bm), b, bits(Fm), bits(Xm), S)
            if size < k:
                for i in range(start, len(cand)):
                    Y = Xm | (1 << cand[i])
                    if inst2.adm(Y):
                        stack.append((Y, i + 1, size + 1))

# phase 1: sparse random instances
while st['instances'] < 120 and time.time() - t0 < 60:
    inst = random_small(rng, (3, 6), (0, 2), 8)
    if len(inst.cells) > 30:
        continue
    Fm = random_F(rng, inst, rng.choice((0, 2, 4, 6, 8)), admissible_only=True)
    theta = rng.randint(0, 3 if len(inst.cells) > 20 else 4)
    cand_b = [c for c in inst.cells if not (Fm >> c & 1)]
    if cand_b:
        run_instance(inst, Fm, popcount(Fm) + theta, rng.sample(cand_b, min(3, len(cand_b))))
say('   after phase 1 (sparse):', st)
# phase 2: F concentrated on the lines through b (so that D4, D5 and the lines a_l occur), sparse and dense boards
while st['instances'] < 700 and time.time() - t0 < 240:
    inst = dense_small(rng, (3, 7), 9) if rng.random() < 0.6 else random_small(rng, (3, 6), (0, 1), 8)
    if len(inst.cells) > 30 or not inst.cells:
        continue
    b = rng.choice(inst.cells)
    D = [l for l in inst.L if LMASK[l] >> b & 1]
    Fm = 0
    for l in D:
        cs = [c for c in LINE_IDS[l] if c != b]
        for c in rng.sample(cs, rng.choice((0, 2, 3, 3, 4))):
            if inst.adm(Fm | (1 << c)):
                Fm |= 1 << c
    # stones on lines crossing the lines of D (candidates for a_l)
    for _ in range(rng.randint(0, 6)):
        c = rng.choice(inst.cells)
        if c != b and inst.adm(Fm | (1 << c)):
            Fm |= 1 << c
    theta = rng.randint(1, 4)
    run_instance(inst, Fm, popcount(Fm) + theta, [b])
say('Prop C.1 (a):', st)
say('   cases of the construction met:', sorted(cases.items()))
assert st['claim_fail'] == 0 and st['statement_fail'] == 0
say('MONOC ALL OK  (%.1f s)' % (time.time() - t0))

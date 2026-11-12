# refA_sec4b.py -- referee 1a: the battery of refA_sec4.py on DENSE small instances (several lines
# of one plane, large admissible F), where pairings fail, the surplus is small and C / LEAF differ.
# Tests: Algorithm 2 == Definition 4.7 (Prop 7.2); LEAF => C (Thm 4.9); theta = 0 exactness (Rem 4.10 (iii));
# Prop 4.13 (b); Prop 4.15 (a),(c),(d),(e), (b); Cor 4.17; Prop 4.18; the five-step test incl. quick rejection.
import itertools, random, sys, time
from refA_geom import *
from refA_lib import *
from refA_small import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

t0 = time.time()
rng = random.Random(41)
stat = dict(inst=0, leaf_true=0, leaf_false=0, fail_L1=0, fail_L2=0, fail_L3=0, C_true=0, C_false=0, C_not_leaf=0,
            alg_mismatch=0, mu_checks=0, triples=0, quick_reject=0, quick_reject_wrong=0, mono_a=0, mono_c=0,
            pair_phase_fail=0, hall_violator_ok=0, theta0=0)
examples = []
while stat['inst'] < 900 and time.time() - t0 < 200:
    inst = dense_small(rng, (4, 8), 10)
    L = inst.L
    if rng.random() < 0.15:
        Fm = random_F(rng, inst, 8, admissible_only=False)
    else:
        Fm = random_adm_F(rng, inst, rng.randint(0, 16))
    nF = popcount(Fm)
    stat['inst'] += 1
    ph = inst.phi(Fm)
    adm = inst.adm(Fm)
    hp = inst.has_pairing(Fm)
    for k in range(nF, nF + 5):
        lf = inst.leaf(Fm, k)
        la, why = leaf_alg2(inst.Bm, Fm, k, L, want_reason=True)
        if lf != la:
            stat['alg_mismatch'] += 1
            say('MISMATCH', bits(FULL & ~inst.Bm), bits(Fm), k, lf, la)
        if not la:
            stat['fail_' + why] += 1
            assert why == ('L1' if not adm else 'L2' if not hp else 'L3')
        stat['leaf_true' if lf else 'leaf_false'] += 1
        if k - nF <= 3:
            c, bad = inst.C(Fm, k)
            assert (not lf) or c, 'LEAF without C'
            stat['C_true' if c else 'C_false'] += 1
            if c and adm and not lf:
                stat['C_not_leaf'] += 1
                if len(examples) < 3 and len(L) <= 5:
                    examples.append((sorted(L), bits(Fm), k, ph))
            if k == nF and adm:
                assert c == lf; stat['theta0'] += 1
    if not adm:
        continue
    a = pairing(L, Fm)
    assert (a is not None) == hp
    if a is None:
        # the pairing phase of Algorithm 2 failed at some line: the set of lines reachable from that line
        # must violate Hall's condition (argument missing in the proof of Prop 7.2)
        asg = Assign(L, Fm)
        for l in L:
            failed = False
            for _ in range(2):
                if not asg.augment([l]):
                    failed = True; break
            if failed:
                R = sorted(asg.reachable([l]))
                assert sur(R, Fm) < 0
                stat['hall_violator_ok'] += 1
                break
        stat['pair_phase_fail'] += 1
        continue
    # quick rejection (Cor 4.17 (b)) on the F-components of L
    for S in components(L, Fm):
        NS = 0
        for l in S:
            NS |= LMASK[l]
        un = sum(1 for c in bits(NS & ~Fm) if c not in a.own)
        assert un == sur(S, Fm)
        if len(S) >= 3:
            for theta in range(0, 5):
                if un < theta:
                    stat['quick_reject'] += 1
                    if inst.leaf(Fm, nF + theta):
                        stat['quick_reject_wrong'] += 1
    triples = [T for T in itertools.combinations(L, 3) if is_connected(T, Fm)]
    mus = []
    for T in triples[:40]:
        U = list(T)
        mb = mu_brute(L, Fm, U)
        n1, b1 = gain_full(a, U); n2, b2 = gain_full(a, U, rng); n3, b3 = gain_one_at_a_time(a, U)
        assert mb == n1 == n2 == n3
        for b in (b1, b2, b3):
            R = b.reachable(U)
            NR = 0
            for l in R:
                NR |= LMASK[l]
            assert all(b.own.get(c) in R for c in bits(NR & ~Fm)) and sur(sorted(R), Fm) == mb and is_connected(sorted(R), Fm)
            # (a): excess <= sur_F(S) for random S >= U
            for _ in range(3):
                S = set(U) | set(rng.sample(L, rng.randint(0, len(L))))
                assert n1 <= sur(sorted(S), Fm)
        # pruning rule (a)
        assert max(mu_brute(L, Fm, [U[0]]), mu_brute(L, Fm, U[:2]), mu_brute(L, Fm, U[1:])) <= mb
        stat['mu_checks'] += 1
    if len(triples) <= 40:
        allmu = [mu_brute(L, Fm, list(T)) for T in triples]
        assert ph == (min(allmu) if allmu else None)
        stat['triples'] += len(triples)
    # Prop 4.18
    for k in range(nF, nF + 4):
        if inst.leaf(Fm, k):
            cand = [c for c in inst.cells if not (Fm >> c & 1)]
            if cand:
                c = rng.choice(cand)
                if nF < k and inst.adm(Fm | (1 << c)):
                    assert inst.leaf(Fm | (1 << c), k); stat['mono_a'] += 1
                assert Small(inst.Bm | (1 << c)).leaf(Fm, k); stat['mono_c'] += 1
say('dense small instances:', stat)
say('examples with F admissible, C true, LEAF false (lines, F, k, Phi):')
for e in examples:
    say('   ', e)
assert stat['alg_mismatch'] == 0 and stat['quick_reject_wrong'] == 0
say('SEC4B ALL OK  (%.1f s)' % (time.time() - t0))

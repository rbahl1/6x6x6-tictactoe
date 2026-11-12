# refA_delete.py -- referee 1a.  The main verifier (verify/treecheck.cpp, class LeafChecker) does NOT run
# Algorithm 2 as printed: a line found strong "gives up its two cells and is deleted", and all later flows
# (single lines, pairs, triples) are computed in the network without the deleted lines.  The paper does not
# describe or justify this.  Here the variant is re-implemented from the description in the source comment
# and compared (a) with Definition 4.7 by brute force and (b) with Algorithm 2 as printed, and the lemma
# proposed in the report (finding on Section 7.3) is tested:
#     min(theta, mu_D(T)) = min(theta, mu(T))   for every set D of strong lines and every T disjoint from D,
# where mu_D(T) = min{ sur_F(S) : T <= S <= L(B) \ D }.
import itertools, random, sys, time
from refA_geom import *
from refA_lib import *
from refA_small import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

def leaf_deleting(Bm, Fm, k, L, stats):
    if not admissible(Bm, Fm, L):
        return False
    a = pairing(L, Fm)
    if a is None:
        return False
    theta = k - popcount(Fm)
    if theta <= 0:
        return True
    deleted = set()
    weak = set(L)
    for l in L:
        avail = len(a.cells[l]) - 2
        if avail < theta:
            continue
        got, fin = gain(a, [l], theta)
        if got < theta:
            continue
        # literal validation of "flow for {l} with gain theta relative to D"
        assert all(len(fin.held(d)) == 0 for d in deleted)
        assert len(fin.held(l)) == 2 + theta
        assert all(len(fin.held(x)) == 2 for x in L if x != l and x not in deleted)
        weak.discard(l)
        for c in a.held(l):
            del a.own[c]
        deleted.add(l)
        # the deleted line must never be reached again: remove it from the line set of the assignment
        a.L = [x for x in a.L if x != l]
    stats['deleted'] += len(deleted)
    # NB: Assign.find_path only walks over owners, and deleted lines own nothing, so they are never reached.
    wl = sorted(weak)
    if len(wl) < 3:
        return True
    strong_pair = set()
    for x, y in itertools.combinations(wl, 2):
        if LMASK[x] & LMASK[y] & ~Fm:
            got, fin = gain(a, [x, y], theta)
            if got >= theta:
                strong_pair.add((x, y))
    done = set()
    for b in wl:
        nb = [x for x in wl if x != b and (LMASK[x] & LMASK[b] & ~Fm) and (min(x, b), max(x, b)) not in strong_pair]
        for x, y in itertools.combinations(nb, 2):
            if (LMASK[x] & LMASK[y] & ~Fm) and (min(x, y), max(x, y)) in strong_pair:
                continue
            T = frozenset((x, b, y))
            if T in done:
                continue
            done.add(T)
            got, fin = gain(a, [x, b, y], theta)
            stats['triple_flows'] += 1
            if got < theta:
                R = sorted(fin.reachable([x, b, y]))
                assert not (set(R) & deleted) and sur(R, Fm) == got and is_connected(R, Fm) and len(R) >= 3
                return False
    return True

t0 = time.time()
rng = random.Random(51)
st = dict(inst=0, evals=0, true=0, false=0, mismatch_def=0, mismatch_alg2=0, deleted=0, triple_flows=0, lemma_checks=0)
while st['inst'] < 700 and time.time() - t0 < 150:
    inst = dense_small(rng, (4, 8), 10) if rng.random() < 0.7 else random_small(rng, (3, 6), (0, 2), 9)
    Fm = random_adm_F(rng, inst, rng.randint(0, 14))
    nF = popcount(Fm)
    st['inst'] += 1
    for k in range(nF, nF + 6):
        want = inst.leaf(Fm, k)
        got = leaf_deleting(inst.Bm, Fm, k, inst.L, st)
        alg2 = leaf_alg2(inst.Bm, Fm, k, inst.L)
        st['evals'] += 1
        st['true' if want else 'false'] += 1
        st['mismatch_def'] += got != want
        st['mismatch_alg2'] += alg2 != want
    # the lemma: D = any set of strong lines
    if inst.adm(Fm) and inst.has_pairing(Fm) and len(inst.L) <= 9:
        L = inst.L
        for theta in (1, 2, 3):
            strong = [l for l in L if mu_brute(L, Fm, [l]) >= theta]
            if not strong:
                continue
            D = rng.sample(strong, rng.randint(1, len(strong)))
            rest = [l for l in L if l not in D]
            for _ in range(6):
                if not rest:
                    break
                T = rng.sample(rest, rng.randint(1, min(3, len(rest))))
                assert min(theta, mu_brute(rest, Fm, T)) == min(theta, mu_brute(L, Fm, T))
                st['lemma_checks'] += 1
say('deleting variant (as in treecheck.cpp) vs Definition 4.7 vs Algorithm 2 as printed:', st)
assert st['mismatch_def'] == 0 and st['mismatch_alg2'] == 0
say('DELETE OK  (%.1f s)' % (time.time() - t0))

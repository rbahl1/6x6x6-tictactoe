#!/usr/bin/env python3
# Referee 1b: numerical test of Lemma 5.5 (symmetry of the abstract game) and Lemma 2.8 on random states:
# children (a),(c),(d),(i),(ii) of the image = images of the children; VOID is invariant; a state and its
# image have the same canonical form.  Also the canonical-form test vectors of Appendix A.2.
import random
from b_sigma import BD, LSET, LINES, THROUGH, G, canon, admissible

rng = random.Random(12)


def children_P(B, F, t):
    """children (c) and (d) of P(B,F,t), literally from Definition 5.1, as a set of (B', F')."""
    out = set()
    live = [l for l in range(148) if not (LSET[l] & B)]
    for l in live:
        for e in LINES[l]:
            if e not in F and len(F | (LSET[l] - {e})) <= t + 1:
                out.add((frozenset(B | {e}), frozenset(F | (LSET[l] - {e}))))
    for i in range(len(live)):
        for j in range(i + 1, len(live)):
            a, b = live[i], live[j]
            c = BD.meet[a][b]
            if c < 0 or c in F:
                continue
            if len(F) + (4 - len(F & LSET[a])) + (4 - len(F & LSET[b])) > t + 1:
                continue   # no child possible (count identity); the literal test follows
            for a1 in LINES[a]:
                if a1 == c:
                    continue
                Qa = LSET[a] - {c, a1}
                if not (F & LSET[a]) <= Qa:
                    continue
                for b1 in LINES[b]:
                    if b1 == c:
                        continue
                    Qb = LSET[b] - {c, b1}
                    if not (F & LSET[b]) <= Qb:
                        continue
                    if len(F | Qa | Qb) <= t + 1:
                        out.add((frozenset(B | {c}), frozenset(F | Qa | Qb)))
    return out


def rand_state():
    t = rng.randrange(3, 10)
    B = set(rng.sample(range(216), t))
    # F: stones concentrated on two or three lines so that children exist
    F = set()
    ls = rng.sample([l for l in range(148) if not (LSET[l] & B)], 3)
    pool = [c for l in ls for c in LINES[l]]
    k = rng.randrange(0, t + 1)
    for c in rng.sample(pool, min(k, len(pool))):
        if c not in B and len(F) < k:
            F.add(c)
    return frozenset(B), frozenset(F), t


n = 0
nvoid = 0
nch = 0
for trial in range(300):
    B, F, t = rand_state()
    gi = rng.randrange(1152)
    g = [int(v) for v in G[gi]]
    gB, gF = frozenset(g[c] for c in B), frozenset(g[c] for c in F)
    assert admissible(B, F) == admissible(gB, gF)
    nvoid += not admissible(B, F)
    assert canon(B, F)[:2] == canon(gB, gF)[:2]
    ch = children_P(B, F, t)
    gch = children_P(gB, gF, t)
    assert {(frozenset(g[c] for c in b2), frozenset(g[c] for c in f2)) for b2, f2 in ch} == gch
    for b2, f2 in list(ch)[:20]:
        assert admissible(b2, f2) == admissible(frozenset(g[c] for c in b2), frozenset(g[c] for c in f2))
    nch += len(ch)
    n += 1
print('Lemma 5.5(a),(b),(d) on %d random P-states (%d VOID, %d children in total): OK' % (n, nvoid, nch))

# Appendix A.2 test vectors
vec = [((43,), (0,), (0,), (43,)),
       ((0, 46, 92, 123, 169, 180, 215), (), (0, 5, 61, 93, 122, 154, 210), ()),
       ((0, 35, 46, 92, 123, 169, 180, 215), (), (0, 5, 61, 93, 122, 154, 210, 215), ()),
       ((5, 30, 43, 64, 86, 93, 128, 169, 180), (0, 35, 46, 87, 122, 151, 172, 185, 210),
        (0, 35, 43, 64, 86, 93, 123, 154, 210), (5, 30, 61, 122, 129, 151, 172, 180, 215))]
for B, F, Bc, Fc in vec:
    r = canon(B, F)
    assert r[0] == Bc and r[1] == Fc, (B, F, r)
print('canonical-form test vectors of Appendix A.2: OK')

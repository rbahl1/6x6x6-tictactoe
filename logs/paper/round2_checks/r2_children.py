# r2_children.py -- X1: the three examples in the remark after the lemma "Different data define different
# children" (Section 5.2): for a VOID P-state each of the statements (a), (b), (c) of the lemma can fail.
# The children are computed literally from the definition of the children (c) and (d) of a P-state.
import itertools
from r2_geom import *
def children(B, F, t):
    cc, dd = [], []
    Lv = live(B)
    for l in Lv:
        for e in l - F:
            F2 = F | (l - {e})
            if len(F2) <= t + 1: cc.append(((l, e), (B | {e}, F2)))
    for a, b in itertools.combinations(Lv, 2):
        m = a & b
        if not m: continue
        (c,) = m
        if c in F: continue
        for Qa in itertools.combinations(sorted(a - {c}), 4):
            Qa = frozenset(Qa)
            if not (F & a) <= Qa: continue
            for Qb in itertools.combinations(sorted(b - {c}), 4):
                Qb = frozenset(Qb)
                if not (F & b) <= Qb: continue
                F2 = F | Qa | Qb
                if len(F2) <= t + 1: dd.append(((frozenset((a, b)), Qa, Qb), (B | {c}, F2)))
    return cc, dd
X = lambda j: frozenset(cid(i, j[0], j[1]) for i in range(6))      # axis line in direction x
Y = lambda j: frozenset(cid(j[0], i, j[1]) for i in range(6))
Z = lambda j: frozenset(cid(j[0], j[1], i) for i in range(6))
B = frozenset()
c = cid(0, 0, 0)
lx, ly, lz = X((0, 0)), Y((0, 0)), Z((0, 0))                        # three live lines through c = (0,0,0)
assert lx & ly == {c} and lx & lz == {c} and ly & lz == {c}
ok = True
def report(what, cond):
    global ok
    print("%-108s %s" % (what, "ok" if cond else "FAILED")); ok = ok and cond

# (a): F contains l-e and l'-e
F = (lx - {c}) | (ly - {c}); t = len(F)
assert not admissible(B, F)
cc, dd = children(B, F, t)
s1 = [s for d, s in cc if d == (lx, c)]; s2 = [s for d, s in cc if d == (ly, c)]
report("(a) fails: F = (l-e) u (l'-e), t = |F| = %d: the pairs (l,e), (l',e) define the same child, revealed set F" % t,
       len(s1) == 1 and s1 == s2 and s1[0] == (B | {c}, F))

# (b): F has exactly four cells on a and on a1; b a third live line through c
for extra in (frozenset(), frozenset({cid(0, 0, 3)})):              # F cap b empty, or one cell
    Fa = frozenset(sorted(lx - {c})[:4]); Fa1 = frozenset(sorted(ly - {c})[:4])
    F = Fa | Fa1 | extra
    for t in (len(F), len(F) + 2, len(F) + 3, len(F) + 4):
        assert not admissible(B, F)
        cc, dd = children(B, F, t)
        n = 0; good = True
        for (pair, Qa, Qb), s in dd:
            if pair == frozenset((lx, lz)):
                qa, qb = (Qa, Qb) if Qa <= lx else (Qb, Qa)
                if qa != Fa: continue
                n += 1
                twins = [s2 for (p2, A2, B2), s2 in dd if p2 == frozenset((ly, lz)) and {A2, B2} == {Fa1, qb}]
                good = good and twins == [s] and s == (B | {c}, F | qb)
        report("(b) fails: |F cap a| = |F cap a1| = 4, |F cap b| = %d, t = %d: %d children ({a,b},F cap a,Q_b) = ({a1,b},F cap a1,Q_b)"
               % (len(extra), t, n), good and (n == (5 if not extra else 4)) == (t >= len(F) + 3 - len(extra)) and (n == 0 or t >= len(F) + 3 - len(extra)))

# (c): F contains l-c and has exactly four cells on a and on b
d1 = frozenset(cid(i, i, 0) for i in range(6)); d2 = frozenset(cid(i, 0, i) for i in range(6))   # face diagonals through c
for l, a, b in ((lx, ly, lz), (lz, d1, d2)):
    assert l & a == {c} and l & b == {c} and a & b == {c}
    F = (l - {c}) | frozenset(sorted(a - {c})[:4]) | frozenset(sorted(b - {c})[1:])
    t = len(F)
    assert not admissible(B, F)
    cc, dd = children(B, F, t)
    s1 = [s for d, s in cc if d == (l, c)]
    s2 = [s for (pair, Qa, Qb), s in dd if pair == frozenset((a, b)) and {Qa, Qb} == {F & a, F & b}]
    report("(c) fails: F = (l-c) u 4 cells of a u 4 cells of b, t = |F| = %d: child (c) of (l,c) = child (d) of ({a,b},F cap a,F cap b)" % t,
           len(s1) == 1 and s1 == s2 and s1[0] == (B | {c}, F))
print("ALL OK" if ok else "SOME CHECK FAILED")
raise SystemExit(0 if ok else 1)

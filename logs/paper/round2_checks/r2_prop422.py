# r2_prop422.py -- X6: the number 17 in the proposition on busy sets with at most seventeen cells
# (Section 4.5) is best possible.  Checks, on the real board:
#  (1) the 36 face diagonals carry two busy cells each, and their busy pairs are exactly the pairs inside
#      the six classes (level j = 0,1,2 and parity of the number of coordinates equal to j);
#  (2) every triangle (three lines that meet pairwise in three distinct cells) contains a face diagonal;
#      the triangles are counted by type;
#  (3) for each of the 4^6 = 4096 sets B that consist of three cells from each class: |B| = 18, B meets
#      every face diagonal, and the live lines contain no triangle (checked directly, not via (2));
#  (4) the three small facts used in the printed justification.
import itertools
from r2_geom import *
ok = True
def report(what, cond):
    global ok
    print("%-104s %s" % (what, "ok" if cond else "FAILED")); ok = ok and cond
busy = set(BUSY)
fd = [l for l in LINES if kind(l) == 2]; ax = [l for l in LINES if kind(l) == 1]; sd = [l for l in LINES if kind(l) == 3]
classes = {}
for c in BUSY:
    x = cell(c); j = lam(x[0]); par = sum(1 for v in x if v == j) % 2
    classes.setdefault((j, par), []).append(c)
report("six classes of four busy cells", len(classes) == 6 and all(len(v) == 4 for v in classes.values()))
pairs = {frozenset(p) for v in classes.values() for p in itertools.combinations(v, 2)}
report("36 face diagonals, two busy cells each, busy pairs = the 36 pairs inside the classes",
       len(fd) == 36 and all(len(l & busy) == 2 for l in fd) and {frozenset(l & busy) for l in fd} == pairs and len(pairs) == 36)
# all triangles
meet = {}
for a, b in itertools.combinations(range(148), 2):
    m = LINES[a] & LINES[b]
    if m: meet[(a, b)] = next(iter(m))
adj = [set() for _ in range(148)]
for (a, b) in meet: adj[a].add(b); adj[b].add(a)
tri = []
for (a, b), c1 in meet.items():
    for c in adj[a] & adj[b]:
        if c > b:
            c2 = meet[(a, c)]; c3 = meet[(b, c)]
            if len({c1, c2, c3}) == 3: tri.append((a, b, c))
types = {}
for t in tri:
    k = tuple(sorted(kind(LINES[i]) for i in t)); types[k] = types.get(k, 0) + 1
print("triangles:", len(tri), "by kinds of lines (1 axis, 2 face diagonal, 3 space diagonal):", sorted(types.items()))
report("every triangle contains a face diagonal", all(any(kind(LINES[i]) == 2 for i in t) for t in tri))
# (3) all 4^6 sets
n = 0; good = True
cl = list(classes.values())
for omit in itertools.product(range(4), repeat=6):
    B = frozenset(c for v, o in zip(cl, omit) for i, c in enumerate(v) if i != o)
    n += 1
    if len(B) != 18 or any(not (l & B) for l in fd): good = False; break
    dead = [bool(LINES[i] & B) for i in range(148)]
    if any(not (dead[a] or dead[b] or dead[c]) for a, b, c in tri): good = False; break
report("%d sets of three cells from each class: 18 busy cells, every face diagonal is met, no live triangle" % n, good and n == 4096)
# (4) facts of the printed justification
report("the four space diagonals are pairwise disjoint", all(not (a & b) for a, b in itertools.combinations(sd, 2)))
def direction(l):
    a, b = sorted(l)[:2]; return tuple(i for i, (u, v) in enumerate(zip(cell(a), cell(b))) if u != v)
g = True
for a, b, c in itertools.combinations(ax, 3):
    if a & b and a & c and b & c:
        g = g and len({direction(a), direction(b), direction(c)}) == 3 and len(a & b & c) == 1
report("three axis lines that meet pairwise have three different directions and pass through one cell", g)
g = True
for a, b in itertools.combinations(ax, 2):
    if not (a & b): continue
    for d in sd:
        p, q = a & d, b & d
        if p and q and p != q: g = False
report("no two axis lines that meet each other meet a space diagonal in two different cells", g)
report("two different cells of a space diagonal differ in all three coordinates",
       all(all(u != v for u, v in zip(cell(p), cell(q))) for d in sd for p, q in itertools.combinations(d, 2)))
print("ALL OK" if ok else "SOME CHECK FAILED")
raise SystemExit(0 if ok else 1)

# r2_geom.py -- geometry of the 6x6x6 board for the checks of the second round (final editor's own code).
# Cells have ids 36x+6y+z.  Lines: the 148 sets {p + i*d : i = 0..5} with d in {-1,0,1}^3, d != 0.
import itertools
N = 6
def cid(x, y, z): return 36 * x + 6 * y + z
def cell(i): return (i // 36, (i // 6) % 6, i % 6)
_lines = set()
for p in itertools.product(range(N), repeat=3):
    for d in itertools.product((-1, 0, 1), repeat=3):
        if d == (0, 0, 0): continue
        pts = [tuple(p[k] + i * d[k] for k in range(3)) for i in range(N)]
        if all(0 <= v < N for q in pts for v in q):
            _lines.add(frozenset(cid(*q) for q in pts))
LINES = sorted(_lines, key=sorted)
assert len(LINES) == 148
def kind(l):                      # 1 axis line, 2 face diagonal, 3 space diagonal
    a, b = sorted(l)[:2]
    return sum(1 for u, v in zip(cell(a), cell(b)) if u != v)
assert sorted(kind(l) for l in LINES).count(1) == 108 and sorted(kind(l) for l in LINES).count(2) == 36
lam = lambda v: min(v, 5 - v)
BUSY = [cid(*c) for c in itertools.product(range(N), repeat=3) if lam(c[0]) == lam(c[1]) == lam(c[2])]
assert len(BUSY) == 24
def live(B): return [l for l in LINES if not (l & B)]
def admissible(B, X):
    """Definition of B-admissible: X and B disjoint, every live line has at most 4 cells in X, no danger pair."""
    if B & X: return False
    four = []
    for l in live(B):
        n = len(l & X)
        if n >= 5: return False
        if n == 4: four.append(l)
    for a, b in itertools.combinations(four, 2):
        m = a & b
        if m and not (m & X): return False
    return True
def parse(s): return frozenset() if s == '-' else frozenset(int(v) for v in s.split(','))

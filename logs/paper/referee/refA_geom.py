# refA_geom.py -- referee 1a: independent geometry of the 6x6x6 board.
# Lines are generated from (start cell, direction) walks, NOT from the paper's
# parametrizations, so that Lemma 2.1 is tested rather than assumed.
import itertools

N = 6
CELLS = [(x, y, z) for x in range(N) for y in range(N) for z in range(N)]
CID = {c: 36 * c[0] + 6 * c[1] + c[2] for c in CELLS}
IDC = {v: k for k, v in CID.items()}


def gen_lines():
    lines = set()
    for c in CELLS:
        for d in itertools.product((-1, 0, 1), repeat=3):
            if d == (0, 0, 0):
                continue
            pts = []
            ok = True
            for i in range(N):
                p = tuple(c[j] + i * d[j] for j in range(3))
                if not all(0 <= v < N for v in p):
                    ok = False
                    break
                pts.append(p)
            if ok:
                lines.add(frozenset(pts))
    return sorted(lines, key=lambda l: sorted(CID[c] for c in l))


LINES = gen_lines()
LINE_IDS = [frozenset(CID[c] for c in l) for l in LINES]       # as sets of ids
LMASK = [sum(1 << i for i in l) for l in LINE_IDS]              # 216-bit masks
THROUGH = {i: [] for i in range(216)}                           # cell id -> line indices
for li, l in enumerate(LINE_IDS):
    for i in l:
        THROUGH[i].append(li)


def kind(l):
    """number of coordinates that vary along the line (1 axis, 2 face diag, 3 space diag)"""
    pts = list(l)
    return sum(1 for j in range(3) if len({p[j] for p in pts}) > 1)


def lam(i):
    return min(i, 5 - i)


def is_busy(c):
    return lam(c[0]) == lam(c[1]) == lam(c[2])


BUSY = [c for c in CELLS if is_busy(c)]
BUSY_IDS = sorted(CID[c] for c in BUSY)


def antip(c):
    return (5 - c[0], 5 - c[1], 5 - c[2])


# ---- the group G of Section 2.3, as permutations of cell ids -----------------
def H_perms():
    out = []
    for s in itertools.permutations(range(6)):
        if all(s[5 - i] == 5 - s[i] for i in range(6)):
            out.append(s)
    return out


def group_G():
    """all maps Sigma_s o P_r o Pi_pi as tuples perm[id] = image id (with multiplicity removed)"""
    maps = {}
    Hs = H_perms()
    for pi in itertools.permutations(range(3)):
        for r in itertools.product((0, 1), repeat=3):
            for s in Hs:
                img = [0] * 216
                for c in CELLS:
                    d = [0, 0, 0]
                    for i in range(3):
                        d[pi[i]] = c[i]                 # (Pi_pi c)_{pi(i)} = c_i
                    d = [5 - d[i] if r[i] else d[i] for i in range(3)]
                    d = tuple(s[v] for v in d)
                    img[CID[c]] = CID[d]
                maps[tuple(img)] = (s, r, pi)
    return maps


def live_lines(B):
    """indices of lines with no cell in B (B: iterable of ids)"""
    bm = 0
    for b in B:
        bm |= 1 << b
    return [li for li in range(len(LINES)) if LMASK[li] & bm == 0]


def cid(x, y, z):
    return 36 * x + 6 * y + z

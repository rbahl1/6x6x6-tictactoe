# refA_lib.py -- referee 1a: predicates of Sections 3 and 4, written from the definitions.
# Cells are ids 0..215, sets of cells are Python ints used as bit masks, lines are indices
# into refA_geom.LINES.
import itertools, random
from collections import deque
from refA_geom import LINES, LINE_IDS, LMASK, THROUGH, CID, IDC, cid

FULL = (1 << 216) - 1


def mask(cells):
    m = 0
    for c in cells:
        m |= 1 << c
    return m


def bits(m):
    out = []
    while m:
        b = m & -m
        out.append(b.bit_length() - 1)
        m ^= b
    return out


def live(Bm):
    return [li for li in range(148) if LMASK[li] & Bm == 0]


def popcount(m):
    return m.bit_count()


# ---------------- Definition 4.1 -------------------------------------------------
def threats(Bm, Xm, L=None):
    L = live(Bm) if L is None else L
    return [l for l in L if popcount(LMASK[l] & Xm) == 5]


def danger_pairs(Bm, Xm, L=None):
    L = live(Bm) if L is None else L
    four = [l for l in L if popcount(LMASK[l] & Xm) == 4]
    out = []
    for a, b in itertools.combinations(four, 2):
        c = LMASK[a] & LMASK[b]
        if c and not (c & Xm):
            out.append((a, b))
    return out


def admissible(Bm, Xm, L=None):
    if Bm & Xm:
        return False
    L = live(Bm) if L is None else L
    if any(popcount(LMASK[l] & Xm) > 4 for l in L):
        return False
    return not danger_pairs(Bm, Xm, L)


# ---------------- Hall, by brute force over all sets of live lines ----------------
def max_deficiency_brute(L, Xm):
    """max over S subset of L of 2|S| - |N(S) \\ X|   (L small)"""
    n = len(L)
    best = 0
    # Gray-code free: simple DP over subsets by lowest bit
    union = [0] * (1 << n)
    for s in range(1, 1 << n):
        low = (s & -s).bit_length() - 1
        union[s] = union[s & (s - 1)] | LMASK[L[low]]
        d = 2 * popcount(s) - popcount(union[s] & ~Xm)
        if d > best:
            best = d
    return best


def components(S, Ym):
    """Y-components of a list S of line indices"""
    S = list(S)
    comp = []
    seen = set()
    for s in S:
        if s in seen:
            continue
        cur = [s]; seen.add(s)
        q = [s]
        while q:
            a = q.pop()
            for b in S:
                if b not in seen and (LMASK[a] & LMASK[b] & ~Ym):
                    seen.add(b); cur.append(b); q.append(b)
        comp.append(cur)
    return comp


def is_connected(S, Ym):
    return len(S) > 0 and len(components(S, Ym)) == 1


def sur(S, Ym):
    u = 0
    for l in S:
        u |= LMASK[l]
    return popcount(u & ~Ym) - 2 * len(S)


def phi_brute(L, Fm):
    """Phi(B,F) straight from Definition 4.7 (L small); returns (value, witness) ; value None = infinity"""
    n = len(L)
    best, wit = None, None
    adj = [[bool(LMASK[L[i]] & LMASK[L[j]] & ~Fm) and i != j for j in range(n)] for i in range(n)]
    nb = [sum(1 << j for j in range(n) if adj[i][j]) for i in range(n)]
    union = [0] * (1 << n)
    for s in range(1, 1 << n):
        low = (s & -s).bit_length() - 1
        union[s] = union[s & (s - 1)] | LMASK[L[low]]
        k = popcount(s)
        if k < 3:
            continue
        # connectivity
        reach = 1 << low
        frontier = reach
        while frontier:
            b = (frontier & -frontier).bit_length() - 1
            frontier &= frontier - 1
            new = nb[b] & s & ~reach
            reach |= new
            frontier |= new
        if reach != s:
            continue
        v = popcount(union[s] & ~Fm) - 2 * k
        if best is None or v < best:
            best, wit = v, [L[i] for i in range(n) if s >> i & 1]
    return best, wit


def mu_brute(L, Fm, U):
    """mu(U) = min sur_F(S), U subset S subset L (L small)"""
    rest = [l for l in L if l not in U]
    base = 0
    for l in U:
        base |= LMASK[l]
    best = None
    n = len(rest)
    union = [base] * (1 << n)
    for s in range(1 << n):
        if s:
            low = (s & -s).bit_length() - 1
            union[s] = union[s & (s - 1)] | LMASK[rest[low]]
        v = popcount(union[s] & ~Fm) - 2 * (len(U) + popcount(s))
        if best is None or v < best:
            best = v
    return best


# ---------------- assignments and augmenting paths (Definition 4.14 / Section 7.3) ----
class Assign:
    def __init__(self, L, Fm):
        self.L = list(L)
        self.Fm = Fm
        self.own = {}                      # cell -> line
        self.cells = {l: [c for c in sorted(LINE_IDS[l]) if not (Fm >> c & 1)] for l in self.L}

    def copy(self):
        a = Assign.__new__(Assign)
        a.L, a.Fm, a.cells = self.L, self.Fm, self.cells
        a.own = dict(self.own)
        return a

    def held(self, l):
        return [c for c in self.cells[l] if self.own.get(c) == l]

    def find_path(self, starts, rng=None):
        """BFS for an augmenting path from one of the lines `starts`; returns list of
        (line, cell) steps  l0,c1,l1,c2,...,lj,c_{j+1}  or None.  If rng is given the
        order of exploration is randomised (to test independence of the choice)."""
        starts = list(starts)
        if rng:
            rng.shuffle(starts)
        prev = {l: None for l in starts}          # line -> (previous line, cell)
        q = deque(starts)
        while q:
            l = q.popleft() if not rng or rng.random() < 0.5 else q.pop()
            cs = list(self.cells[l])
            if rng:
                rng.shuffle(cs)
            for c in cs:
                o = self.own.get(c)
                if o is None:
                    path = [(l, c)]
                    while prev[l] is not None:
                        pl, pc = prev[l]
                        path.append((pl, pc))
                        l = pl
                    path.reverse()
                    return path
                if o != l and o not in prev:
                    prev[o] = (l, c)
                    q.append(o)
        return None

    def reachable(self, starts):
        seen = set(starts)
        q = list(starts)
        while q:
            l = q.pop()
            for c in self.cells[l]:
                o = self.own.get(c)
                if o is not None and o not in seen:
                    seen.add(o); q.append(o)
        return seen

    def augment(self, starts, rng=None):
        p = self.find_path(starts, rng)
        if p is None:
            return False
        for l, c in p:
            self.own[c] = l
        return True


def pairing(L, Xm):
    """Algorithm 2, lines 3-5: returns an Assign in which every line of L holds two cells, or None"""
    a = Assign(L, Xm)
    for l in L:
        for _ in range(2):
            if not a.augment([l]):
                return None
    return a


def pairing_kuhn(L, Xm):
    """independent second method: Kuhn's DFS matching on two copies of every line"""
    match = {}                                # cell -> (line, copy)
    cells = {l: [c for c in LINE_IDS[l] if not (Xm >> c & 1)] for l in L}

    def try_(v, seen):
        l = v[0]
        for c in cells[l]:
            if c in seen:
                continue
            seen.add(c)
            if c not in match or try_(match[c], seen):
                match[c] = v
                return True
        return False
    import sys
    sys.setrecursionlimit(10000)
    for l in L:
        for k in (0, 1):
            if not try_((l, k), set()):
                return None
    return match


def check_pairing(L, Bm, Xm, a):
    """Definition 3.1 literally"""
    used = set()
    for l in L:
        h = a.held(l)
        assert len(h) == 2, (l, h)
        for c in h:
            assert c in LINE_IDS[l] and not (Xm >> c & 1) and not (Bm >> c & 1) and c not in used
            used.add(c)
    return True


def gain(a, U, theta, rng=None):
    b = a.copy()
    n = 0
    while n < theta and b.augment(U, rng):
        n += 1
    return n, b


def gain_full(a, U, rng=None):
    """augment from U until no path; returns (count, final assignment)"""
    b = a.copy()
    n = 0
    while b.augment(U, rng):
        n += 1
    return n, b


def gain_one_at_a_time(a, U):
    b = a.copy()
    n = 0
    for u in U:
        while b.augment([u]):
            n += 1
    return n, b


def leaf_alg2(Bm, Fm, k, L=None, want_reason=False):
    """Algorithm 2 of Section 7.3, literally (strong-line pruning only)."""
    L = live(Bm) if L is None else L
    if not admissible(Bm, Fm, L):
        return (False, 'L1') if want_reason else False
    a = pairing(L, Fm)
    if a is None:
        return (False, 'L2') if want_reason else False
    theta = k - popcount(Fm)
    if theta <= 0:
        return (True, '') if want_reason else True
    Lw = [l for l in L if gain(a, [l], theta)[0] < theta]
    Lws = set(Lw)
    done = set()
    for b in Lw:
        nbrs = [x for x in Lw if x != b and (LMASK[x] & LMASK[b] & ~Fm)]
        for x, y in itertools.combinations(nbrs, 2):
            T = frozenset((x, b, y))
            if T in done:
                continue
            done.add(T)
            if gain(a, list(T), theta)[0] < theta:
                return (False, 'L3') if want_reason else False
    return (True, '') if want_reason else True


def leaf_def(Bm, Fm, k, L=None):
    """LEAF straight from Definition 4.7 with brute force (L small)"""
    L = live(Bm) if L is None else L
    if not admissible(Bm, Fm, L):
        return False
    if max_deficiency_brute(L, Fm) > 0:
        return False
    ph, _ = phi_brute(L, Fm)
    return ph is None or ph >= k - popcount(Fm)


def C_enum(Bm, Fm, k, L=None, free=None):
    """C(B,F,k) by enumeration of all B-admissible X >= F with |X| <= k (Remark 4.5 (iv)).
    Only cells on live lines matter for admissibility and for pairings, but X may contain other
    free cells as well; those only use up the budget k, so it suffices to enumerate X inside
    N(L(B)) (a pairing for (B,X) depends only on X cap N(L))."""
    L = live(Bm) if L is None else L
    if not admissible(Bm, Fm, L):
        return True, None
    NL = 0
    for l in L:
        NL |= LMASK[l]
    cand = [c for c in bits(NL & ~Fm & ~Bm)]
    nF = popcount(Fm)

    def rec(Xm, start, size):
        if max_deficiency_brute(L, Xm) > 0:
            return Xm
        if size == k:
            return None
        for i in range(start, len(cand)):
            Y = Xm | (1 << cand[i])
            if admissible(Bm, Y, L):
                r = rec(Y, i + 1, size + 1)
                if r is not None:
                    return r
        return None
    bad = rec(Fm, 0, nF)
    return bad is None, bad


def small_instance(rng, nlines=(3, 7), extra=(0, 3)):
    """a set B whose complement is the union of a few (mostly intersecting) lines plus a few cells;
    returns (Bm, L)"""
    k = rng.randint(*nlines)
    S = [rng.randrange(148)]
    while len(S) < k:
        if rng.random() < 0.8:
            base = rng.choice(S)
            c = rng.choice(sorted(LINE_IDS[base]))
            l = rng.choice(THROUGH[c])
        else:
            l = rng.randrange(148)
        if l not in S:
            S.append(l)
    W = 0
    for l in S:
        W |= LMASK[l]
    for _ in range(rng.randint(*extra)):
        W |= 1 << rng.randrange(216)
    Bm = FULL & ~W
    return Bm, live(Bm)

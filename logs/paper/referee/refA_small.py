# refA_small.py -- referee 1a: exhaustive evaluation of C(B,F,k), worst(B,F,k), Phi, LEAF on
# instances of the real 6x6x6 geometry with few live lines (B = complement of a small cell set).
import itertools, random
from refA_geom import LINES, LINE_IDS, LMASK, THROUGH
from refA_lib import *


class Small:
    def __init__(self, Bm):
        self.Bm = Bm
        self.L = live(Bm)
        n = self.n = len(self.L)
        assert n <= 12
        self.union = [0] * (1 << n)
        self.size = [0] * (1 << n)
        for s in range(1, 1 << n):
            low = (s & -s).bit_length() - 1
            self.union[s] = self.union[s & (s - 1)] | LMASK[self.L[low]]
            self.size[s] = self.size[s & (s - 1)] + 1
        self.NL = self.union[(1 << n) - 1] if n else 0
        self.cells = bits(self.NL)

    def adm(self, Xm):
        return admissible(self.Bm, Xm, self.L)

    def delta(self, Xm):
        nX = ~Xm
        best = 0
        for s in range(1, 1 << self.n):
            d = 2 * self.size[s] - popcount(self.union[s] & nX)
            if d > best:
                best = d
        return best

    def has_pairing(self, Xm):
        nX = ~Xm
        for s in range(1, 1 << self.n):
            if 2 * self.size[s] > popcount(self.union[s] & nX):
                return False
        return True

    def enum_adm(self, Fm, k):
        """all B-admissible X with F <= X, |X| <= k, X inside N(L) u F  (generator of masks)"""
        if not self.adm(Fm):
            return
        cand = [c for c in self.cells if not (Fm >> c & 1)]
        stack = [(Fm, 0, popcount(Fm))]
        while stack:
            Xm, start, size = stack.pop()
            yield Xm
            if size == k:
                continue
            for i in range(start, len(cand)):
                Y = Xm | (1 << cand[i])
                if self.adm(Y):
                    stack.append((Y, i + 1, size + 1))

    def C(self, Fm, k):
        """returns (truth value, violating X or None)"""
        for Xm in self.enum_adm(Fm, k):
            if not self.has_pairing(Xm):
                return False, Xm
        return True, None

    def C_fast(self, Fm, k):
        """same as C, with an incremental admissibility test (independent second implementation):
        returns (truth value, violating X or None, number of admissible sets visited)"""
        if not self.adm(Fm):
            return True, None, 0
        L = self.L; n = self.n
        lm = [LMASK[l] for l in L]
        cand = [c for c in self.cells if not (Fm >> c & 1)]
        through = {c: [i for i in range(n) if lm[i] >> c & 1] for c in cand}
        meet = [[lm[i] & lm[j] if i != j else 0 for j in range(n)] for i in range(n)]
        cnt = [(lm[i] & Fm).bit_count() for i in range(n)]
        union = self.union; size = self.size; full = 1 << n
        visited = 0
        def ok_pair(Xm):
            nX = ~Xm
            for s in range(1, full):
                if 2 * size[s] > (union[s] & nX).bit_count():
                    return False
            return True
        def rec(Xm, start, sz):
            nonlocal visited
            visited += 1
            if not ok_pair(Xm):
                return Xm
            if sz == k:
                return None
            for idx in range(start, len(cand)):
                c = cand[idx]
                Y = Xm | (1 << c)
                good = True
                for i in through[c]:
                    if cnt[i] >= 4:
                        good = False; break
                if good:
                    for i in through[c]:
                        cnt[i] += 1
                    # danger pairs: two lines with exactly four stones meeting in a cell outside Y
                    four = [i for i in range(n) if cnt[i] == 4]
                    for x in range(len(four)):
                        for y in range(x + 1, len(four)):
                            m = meet[four[x]][four[y]]
                            if m and not (m & Y):
                                good = False
                    if good:
                        r = rec(Y, idx + 1, sz + 1)
                        if r is not None:
                            for i in through[c]:
                                cnt[i] -= 1
                            return r
                    for i in through[c]:
                        cnt[i] -= 1
            return None
        bad = rec(Fm, 0, Fm.bit_count())
        return bad is None, bad, visited

    def worst(self, Fm, k):
        best = None
        for Xm in self.enum_adm(Fm, k):
            d = self.delta(Xm)
            if best is None or d > best:
                best = d
        return best

    def phi(self, Fm):
        return phi_brute(self.L, Fm)[0]

    def leaf(self, Fm, k):
        if not self.adm(Fm) or not self.has_pairing(Fm):
            return False
        ph = self.phi(Fm)
        return ph is None or ph >= k - popcount(Fm)


def random_small(rng, nlines=(3, 6), extra=(0, 2), maxlive=9):
    while True:
        Bm, L = small_instance(rng, nlines, extra)
        if len(L) <= maxlive:
            return Small(Bm)


def random_F(rng, inst, maxF=6, admissible_only=True):
    for _ in range(50):
        m = rng.randint(0, maxF)
        pool = inst.cells if rng.random() < 0.85 else bits(FULL & ~inst.Bm)
        if m > len(pool):
            continue
        Fm = mask(rng.sample(pool, m))
        if not admissible_only or inst.adm(Fm):
            return Fm
    return 0


def dense_small(rng, nl=(4, 8), maxlive=11):
    """B = complement of the union of several lines of one coordinate plane (many intersections,
    small surplus), optionally with one further line through a cell of the union."""
    from refA_geom import LINES
    while True:
        j = rng.randrange(3); v = rng.randrange(6)
        plane = [i for i in range(148) if all(c[j] == v for c in LINES[i])]
        S = rng.sample(plane, rng.randint(*nl))
        W = 0
        for l in S:
            W |= LMASK[l]
        if rng.random() < 0.3:
            c = rng.choice(bits(W))
            W |= LMASK[rng.choice(THROUGH[c])]
        Bm = FULL & ~W
        if len(live(Bm)) <= maxlive:
            return Small(Bm)


def random_adm_F(rng, inst, size):
    """a random B-admissible set of about `size` cells inside N(L), grown cell by cell"""
    Fm = 0
    cells = list(inst.cells)
    rng.shuffle(cells)
    for c in cells:
        if popcount(Fm) >= size:
            break
        if inst.adm(Fm | (1 << c)):
            Fm |= 1 << c
    return Fm

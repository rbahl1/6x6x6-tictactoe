# Proof-writer scratch: generic n^d geometry and brute-force reference implementations of the
# definitions of SPEC section 3, generalised to line length n (cap n-2, 2 cells per line).
# Used only to fuzz-test the lemmas of PROOF.md on small boards. Not part of the proof.
import itertools

def make_geom(n, d):
    cells = list(itertools.product(range(n), repeat=d))
    idx = {c: i for i, c in enumerate(cells)}
    lines = set()
    for f in itertools.product(range(n + 2), repeat=d):
        if all(v < n for v in f):
            continue
        l = []
        for t in range(n):
            c = tuple(v if v < n else (t if v == n else n - 1 - t) for v in f)
            l.append(idx[c])
        lines.add(tuple(sorted(l)))
    return cells, sorted(lines)

class Geo:
    def __init__(s, n, d):
        s.n, s.d = n, d
        s.cells, s.lines = make_geom(n, d)
        s.NC, s.NL = len(s.cells), len(s.lines)
        s.lmask = [sum(1 << c for c in l) for l in s.lines]
        s.through = [[] for _ in range(s.NC)]
        for i, l in enumerate(s.lines):
            for c in l:
                s.through[c].append(i)
        for i in range(s.NL):
            for j in range(i + 1, s.NL):
                assert (s.lmask[i] & s.lmask[j]).bit_count() <= 1
        s.cap = n - 2

    def live(s, B):
        return [i for i in range(s.NL) if not (s.lmask[i] & B)]

    def admissible(s, B, X, live=None):
        if X & B:
            return False
        if live is None:
            live = s.live(B)
        full = []
        for i in live:
            c = (s.lmask[i] & X).bit_count()
            if c > s.cap:
                return False
            if c == s.cap:
                full.append(i)
        for a in range(len(full)):
            for b in range(a + 1, len(full)):
                sh = s.lmask[full[a]] & s.lmask[full[b]]
                if sh and not (sh & X):
                    return False
        return True

    def pairing(s, B, X, live=None):
        """Kuhn matching, 2 cells per live line. Returns owner dict (cell -> line) or None."""
        if live is None:
            live = s.live(B)
        owner = {}
        def aug(l, seen):
            for c in s.lines[l]:
                if (X >> c) & 1 or c in seen:
                    continue
                seen.add(c)
                if c not in owner or aug(owner[c], seen):
                    owner[c] = l
                    return True
            return False
        for l in live:
            for _ in range(2):
                if not aug(l, set()):
                    return None
        return owner

    def nmask(s, S):
        m = 0
        for l in S:
            m |= s.lmask[l]
        return m

    def deficiency_bf(s, B, X, live):
        best = 0
        for r in range(1, len(live) + 1):
            for S in itertools.combinations(live, r):
                v = 2 * r - (s.nmask(S) & ~X).bit_count()
                best = max(best, v)
        return best

    def relevant_cells(s, B, live):
        m = s.nmask(live)
        return [c for c in range(s.NC) if (m >> c) & 1]

    def C(s, B, F, k, live=None):
        """Exact C(B,F,k): returns (holds, violator X or None). Checks the pairing at every node."""
        if live is None:
            live = s.live(B)
        if not s.admissible(B, F, live):
            return True, None
        free = [c for c in s.relevant_cells(B, live) if not (F >> c) & 1]
        def rec(X, start, left):
            if s.pairing(B, X, live) is None:
                return X
            if left == 0:
                return None
            for i in range(start, len(free)):
                X2 = X | (1 << free[i])
                if s.admissible(B, X2, live):
                    r = rec(X2, i + 1, left - 1)
                    if r is not None:
                        return r
            return None
        v = rec(F, 0, k - F.bit_count())
        return v is None, v

    def fadj(s, live, F):
        """F-adjacency as dict line -> set of lines."""
        adj = {l: set() for l in live}
        for a in range(len(live)):
            for b in range(a + 1, len(live)):
                sh = s.lmask[live[a]] & s.lmask[live[b]]
                if sh and not (sh & F):
                    adj[live[a]].add(live[b]); adj[live[b]].add(live[a])
        return adj

    @staticmethod
    def connected(S, adj):
        S = set(S)
        if not S:
            return True
        st = [next(iter(S))]
        seen = {st[0]}
        while st:
            u = st.pop()
            for v in adj[u]:
                if v in S and v not in seen:
                    seen.add(v); st.append(v)
        return len(seen) == len(S)

    def sur(s, S, F):
        return (s.nmask(S) & ~F).bit_count() - 2 * len(S)

    def phi_bf(s, B, F, live=None):
        """min sur_F over F-connected S, |S|>=3 (None if no such S); brute force."""
        if live is None:
            live = s.live(B)
        adj = s.fadj(live, F)
        best = None
        for r in range(3, len(live) + 1):
            for S in itertools.combinations(live, r):
                if s.connected(S, adj):
                    v = s.sur(S, F)
                    if best is None or v < best:
                        best = v
        return best

    def LEAF_bf(s, B, F, k, live=None):
        if live is None:
            live = s.live(B)
        if not s.admissible(B, F, live):
            return False, 1
        if s.pairing(B, F, live) is None:
            return False, 2
        p = s.phi_bf(B, F, live)
        if p is not None and p < k - F.bit_count():
            return False, 3
        return True, 0

    def mu_bf(s, T, F, live):
        rest = [l for l in live if l not in T]
        best = None
        for r in range(0, len(rest) + 1):
            for S in itertools.combinations(rest, r):
                v = s.sur(list(T) + list(S), F)
                if best is None or v < best:
                    best = v
        return best

    def mu_flow(s, T, F, live, owner):
        """Number of augmentations from a pairing when the lines of T are uncapacitated."""
        owner = dict(owner)
        T = list(T)
        def aug(l, seen):
            for c in s.lines[l]:
                if (F >> c) & 1 or c in seen:
                    continue
                seen.add(c)
                if c not in owner or (owner[c] not in T and aug(owner[c], seen)):
                    owner[c] = l
                    return True
            return False
        # NOTE: a cell owned by a T line is never re-routed (moving it between T lines or away
        # cannot help: T lines are sources of unbounded supply).  Verified against brute force.
        cnt = 0
        progress = True
        while progress:
            progress = False
            for l in T:
                if aug(l, set()):
                    cnt += 1; progress = True
        return cnt

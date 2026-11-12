#!/usr/bin/env python3
"""Independent Python geometry for 6x6x6 tic-tac-toe (SPEC.md section 1 and 3).

Written from SPEC section 1 only; it does not read or translate src/*.hpp.
Everything here is exact integer arithmetic.

Cell (x,y,z) has id 36x+6y+z.  Line ids and symmetry indices are internal to
this module (lines sorted lexicographically by their ascending cell tuples).
"""
import itertools
import sys

N = 6
NC = N * N * N


def cid(x, y, z):
    return 36 * x + 6 * y + z


def xyz(c):
    return (c // 36, (c // 6) % 6, c % 6)


# ----------------------------------------------------------------------------
# lines
# ----------------------------------------------------------------------------

def _build_lines():
    # each coordinate function is a constant 0..5, t, or 5-t; not all constant
    funcs = [("c", v) for v in range(N)] + [("t", 0), ("r", 0)]

    def ev(f, t):
        if f[0] == "c":
            return f[1]
        return t if f[0] == "t" else N - 1 - t

    seen = {}
    for f1 in funcs:
        for f2 in funcs:
            for f3 in funcs:
                nvar = sum(1 for f in (f1, f2, f3) if f[0] != "c")
                if nvar == 0:
                    continue
                cells = tuple(sorted(cid(ev(f1, t), ev(f2, t), ev(f3, t)) for t in range(N)))
                assert len(set(cells)) == N
                if cells in seen:
                    assert seen[cells] == nvar
                seen[cells] = nvar
    lines = sorted(seen)
    kinds = [seen[l] for l in lines]
    return lines, kinds


LINES, LINE_KIND = _build_lines()          # LINE_KIND: 1 axis, 2 face diagonal, 3 space diagonal
NL = len(LINES)
assert NL == 148, NL
assert LINE_KIND.count(1) == 108 and LINE_KIND.count(2) == 36 and LINE_KIND.count(3) == 4
LINE_SET = [frozenset(l) for l in LINES]
LINE_ID = {l: i for i, l in enumerate(LINES)}

CELL_LINES = [[] for _ in range(NC)]
for _i, _l in enumerate(LINES):
    for _c in _l:
        CELL_LINES[_c].append(_i)

# two cells share at most one line
LINE_OF = {}
for _i, _l in enumerate(LINES):
    for _a, _b in itertools.combinations(_l, 2):
        assert (_a, _b) not in LINE_OF, "two cells on two lines"
        LINE_OF[(_a, _b)] = _i
        LINE_OF[(_b, _a)] = _i


def antipode(c):
    x, y, z = xyz(c)
    return cid(5 - x, 5 - y, 5 - z)


ANTIP = [antipode(c) for c in range(NC)]


def _is_busy(c):
    x, y, z = xyz(c)
    return min(x, 5 - x) == min(y, 5 - y) == min(z, 5 - z)


BUSY = [c for c in range(NC) if _is_busy(c)]
BUSY_SET = frozenset(BUSY)
BUSY_IDX = {c: i for i, c in enumerate(BUSY)}
assert len(BUSY) == 24
for _c in range(NC):
    if _c in BUSY_SET:
        assert len(CELL_LINES[_c]) == 7
    else:
        assert len(CELL_LINES[_c]) in (3, 4)
assert all(ANTIP[c] in BUSY_SET for c in BUSY)

# ----------------------------------------------------------------------------
# symmetries
# ----------------------------------------------------------------------------

_SYM = None
_SYM_LINES = None


def _relabelings():
    # permutations s of {0..5} with s(5-i) = 5 - s(i)
    out = []
    for pi in itertools.permutations(range(3)):
        for flips in itertools.product((0, 1), repeat=3):
            s = [0] * 6
            for j in range(3):
                v = pi[j] if not flips[j] else 5 - pi[j]
                s[j] = v
                s[5 - j] = 5 - v
            assert sorted(s) == list(range(6))
            assert all(s[5 - i] == 5 - s[i] for i in range(6))
            out.append(tuple(s))
    assert len(set(out)) == 48
    return out


def symmetries():
    """The 1152 symmetries as tuples g with g[c] = image of cell c."""
    global _SYM, _SYM_LINES
    if _SYM is not None:
        return _SYM
    maps = set()
    for perm in itertools.permutations(range(3)):
        for rev in itertools.product((0, 1), repeat=3):
            for s in _relabelings():
                g = []
                for c in range(NC):
                    p = xyz(c)
                    q = [p[perm[0]], p[perm[1]], p[perm[2]]]
                    q = [5 - q[i] if rev[i] else q[i] for i in range(3)]
                    q = [s[v] for v in q]
                    g.append(cid(*q))
                assert sorted(g) == list(range(NC))
                maps.add(tuple(g))
    syms = sorted(maps)
    assert len(syms) == 1152, len(syms)
    # every map sends lines to lines
    sym_lines = []
    for g in syms:
        row = []
        for l in LINES:
            img = tuple(sorted(g[c] for c in l))
            assert img in LINE_ID, "symmetry does not map a line to a line"
            row.append(LINE_ID[img])
        assert sorted(row) == list(range(NL))
        sym_lines.append(tuple(row))
    # busy cells and antipodes are preserved
    for g in syms:
        assert all(g[c] in BUSY_SET for c in BUSY)
    _SYM, _SYM_LINES = syms, sym_lines
    return _SYM


def symmetry_line_maps():
    symmetries()
    return _SYM_LINES


def busy_perms():
    """Action of the group on the 24 busy cells: list of 24-tuples (index -> index)."""
    return [tuple(BUSY_IDX[g[c]] for c in BUSY) for g in symmetries()]


# ----------------------------------------------------------------------------
# sets, instances
# ----------------------------------------------------------------------------

def fmt_set(s):
    s = sorted(s)
    return ",".join(str(c) for c in s) if s else "-"


def parse_set(txt):
    txt = txt.strip()
    if txt == "-" or txt == "":
        return ()
    cells = tuple(sorted(int(t) for t in txt.split(",")))
    assert len(set(cells)) == len(cells), "duplicate cell in set"
    assert all(0 <= c < NC for c in cells), "cell id out of range"
    return cells


def parse_instance(line):
    """'k B F' -> (k, B, F) with B, F ascending tuples of cell ids."""
    parts = line.split()
    if len(parts) != 3:
        raise ValueError("instance line must have three fields 'k B F': %r" % line)
    k = int(parts[0])
    return k, parse_set(parts[1]), parse_set(parts[2])


def fmt_instance(k, B, F):
    return "%d %s %s" % (k, fmt_set(B), fmt_set(F))


def cells_xyz(triples):
    return tuple(sorted(cid(*t) for t in triples))


# ----------------------------------------------------------------------------
# section 3: live lines, slack, admissibility, pairing
# ----------------------------------------------------------------------------

def live_lines(B):
    Bs = B if isinstance(B, (set, frozenset)) else set(B)
    return [i for i in range(NL) if not (LINE_SET[i] & Bs)]


def slack(B):
    Bs = set(B)
    return (NC - len(Bs)) - 2 * len(live_lines(Bs))


def neighbourhood(S):
    out = set()
    for l in S:
        out.update(LINES[l])
    return out


def deficiency(S, X):
    """2|S| - |N(S) \\ X| for a collection S of line ids (exact integer)."""
    S = set(S)
    return 2 * len(S) - len(neighbourhood(S) - set(X))


def admissible(B, X):
    """X is B-admissible (SPEC section 3)."""
    Bs, Xs = set(B), set(X)
    if Bs & Xs:
        return False
    live = live_lines(Bs)
    cnt = {}
    for l in live:
        n = len(LINE_SET[l] & Xs)
        if n > 4:
            return False
        cnt[l] = n
    four = [l for l in live if cnt[l] == 4]
    fourset = set(four)
    for a in four:
        for c in LINES[a]:
            if c in Xs:
                continue
            for b in CELL_LINES[c]:
                if b != a and b in fourset:
                    return False
    return True


def danger_pairs(B, X):
    """All danger pairs (a, b, c): live lines a<b with 4 X-cells each sharing c not in X."""
    Bs, Xs = set(B), set(X)
    live = live_lines(Bs)
    four = set(l for l in live if len(LINE_SET[l] & Xs) == 4)
    out = []
    for a in sorted(four):
        for c in LINES[a]:
            if c in Xs:
                continue
            for b in CELL_LINES[c]:
                if b > a and b in four:
                    out.append((a, b, c))
    return out


def max_deficiency(B, X):
    """max over S <= L(B) of 2|S| - |N(S) \\ X|, exactly, as 2|L(B)| - (maximum matching).

    (Deficiency form of Hall's theorem on the graph 'two slots per live line' x 'cells
    outside X'; an optimal slot set can be taken closed under twins, i.e. a set of lines.)
    """
    Bs, Xs = set(B), set(X)
    live = live_lines(Bs)
    avail = {l: [c for c in LINES[l] if c not in Xs] for l in live}
    match = {}
    if sys.getrecursionlimit() < 5000:
        sys.setrecursionlimit(5000)

    def try_slot(slot, visited):
        for c in avail[slot >> 1]:
            if c in visited:
                continue
            visited.add(c)
            m = match.get(c)
            if m is None or try_slot(m, visited):
                match[c] = slot
                return True
        return False

    for l in live:
        for j in (0, 1):
            try_slot(2 * l + j, set())
    return 2 * len(live) - len(match)


def brute_worst(B, F, k):
    """Exact worst(B,F,k) by enumeration, only for k - |F| <= 1.  None if no admissible X."""
    Bs, Fs = set(B), set(F)
    free = k - len(Fs)
    assert free <= 1, "brute_worst is only meant for k - |F| <= 1"
    if free < 0 or not admissible(Bs, Fs):
        return None
    best = max_deficiency(Bs, Fs)
    if free == 1:
        for c in range(NC):
            if c in Bs or c in Fs:
                continue
            X = Fs | {c}
            if admissible(Bs, X):
                best = max(best, max_deficiency(Bs, X))
    return best


def find_pairing(B, X):
    """Exact pairing by augmenting paths.

    Returns (True, {line: (c1, c2)}) or (False, S) where S is a sorted list of live
    line ids with |N(S) \\ X| < 2|S| (a Hall violator, verified before returning).
    """
    Bs, Xs = set(B), set(X)
    live = live_lines(Bs)
    avail = {l: [c for c in LINES[l] if c not in Xs] for l in live}
    match = {}                      # cell -> slot (2*line + j)
    old = sys.getrecursionlimit()
    if old < 5000:
        sys.setrecursionlimit(5000)

    def try_slot(slot, visited):
        for c in avail[slot >> 1]:
            if c in visited:
                continue
            visited.add(c)
            m = match.get(c)
            if m is None or try_slot(m, visited):
                match[c] = slot
                return True
        return False

    for l in live:
        for j in (0, 1):
            visited = set()
            if not try_slot(2 * l + j, visited):
                S = {l}
                for c in visited:
                    S.add(match[c] >> 1)
                S = sorted(S)
                assert deficiency(S, Xs) > 0, "internal error: bad Hall violator"
                return False, S
    assign = {l: [] for l in live}
    for c, slot in match.items():
        assign[slot >> 1].append(c)
    for l in live:
        assert len(assign[l]) == 2 and all(c in LINE_SET[l] and c not in Xs for c in assign[l])
    assert len(match) == 2 * len(live)
    return True, {l: tuple(sorted(v)) for l, v in assign.items()}


# ----------------------------------------------------------------------------
# LEAF predicate (SPEC section 3), simple exact reference implementation
# ----------------------------------------------------------------------------

def sur(S, F):
    """sur_F(S) = |N(S) \\ F| - 2|S| (exact integer)."""
    S = set(S)
    return len(neighbourhood(S) - set(F)) - 2 * len(S)


def f_connected(S, F):
    """S (line ids) is connected in the F-adjacency graph (shared cell not in F)."""
    S = list(set(S))
    if not S:
        return True
    Fs = set(F)
    Sset = set(S)
    seen = {S[0]}
    stack = [S[0]]
    while stack:
        l = stack.pop()
        for c in LINES[l]:
            if c in Fs:
                continue
            for m in CELL_LINES[c]:
                if m in Sset and m not in seen:
                    seen.add(m)
                    stack.append(m)
    return len(seen) == len(S)


def leaf_check(B, F, k):
    """Exact LEAF(B,F,k).  Returns (holds, reason, info).

    reason 0: holds (info = number of F-connected triples examined);
    reason 1: F not B-admissible; reason 2: no pairing for (B,F) (info = Hall violator);
    reason 3: info = (S, sur) with S an F-connected set of >= 3 live lines and
              sur_F(S) = sur < k-|F|  (re-verified exactly before returning).
    mu(T) is computed as in SPEC section 3: start from a pairing for (B,F), let the lines
    of T take arbitrarily many cells and count augmenting paths (capped at theta = k-|F|).
    """
    Bs, Fs = set(B), set(F)
    if not admissible(Bs, Fs):
        return False, 1, None
    ok, pr = find_pairing(Bs, Fs)
    if not ok:
        return False, 2, pr
    theta = k - len(Fs)
    if theta <= 0:
        return True, 0, 0
    live = live_lines(Bs)
    liveset = set(live)
    avail = {l: [c for c in LINES[l] if c not in Fs] for l in live}
    owner0 = {}
    for l, cs in pr.items():
        for c in cs:
            owner0[c] = l

    def mu(T, cap):
        """(number of extra cells the lines of T can take, capped at cap; reached lines)."""
        owner = dict(owner0)
        Tset = set(T)
        got = 0
        seen_l = set(Tset)
        while got < cap:
            entry = {}                # non-T line -> its own cell through which it was reached
            parent = {}               # cell -> line that reached it
            queue = list(T)
            seen_l = set(Tset)
            end = None
            qi = 0
            while qi < len(queue) and end is None:
                l = queue[qi]
                qi += 1
                for c in avail[l]:
                    if c in parent:
                        continue
                    o = owner.get(c)
                    if o == l or (o is not None and o in Tset):
                        continue
                    parent[c] = l
                    if o is None:
                        end = c
                        break
                    if o not in seen_l:
                        seen_l.add(o)
                        entry[o] = c
                        queue.append(o)
            if end is None:
                break
            c = end
            while True:
                l = parent[c]
                owner[c] = l
                if l in Tset:
                    break
                c = entry[l]          # l hands this cell to the line that reached it
            got += 1
        return got, seen_l

    mu1 = {l: mu((l,), theta)[0] for l in live}
    adj = {l: set() for l in live}
    for l in live:
        for c in avail[l]:
            for m in CELL_LINES[c]:
                if m != l and m in liveset:
                    adj[l].add(m)
    seen = set()
    ntri = 0
    for a in live:
        if mu1[a] >= theta:
            continue
        for b in adj[a]:
            if mu1[b] >= theta:
                continue
            if mu((a, b), theta)[0] >= theta:
                continue
            for c in adj[a] | adj[b]:
                if c == a or c == b or mu1[c] >= theta:
                    continue
                T = tuple(sorted((a, b, c)))
                if T in seen:
                    continue
                seen.add(T)
                ntri += 1
                m, S = mu(T, theta)
                if m < theta:
                    S = sorted(S)
                    assert len(S) >= 3 and f_connected(S, Fs) and sur(S, Fs) == m
                    return False, 3, (S, m)
    return True, 0, ntri


# ----------------------------------------------------------------------------
# regression constants
# ----------------------------------------------------------------------------

T7 = cells_xyz([(0, 0, 0), (5, 5, 5), (1, 1, 4), (4, 4, 1), (2, 3, 2), (3, 2, 3), (5, 0, 0)])
T8 = cells_xyz([(0, 0, 0), (5, 5, 5), (1, 1, 4), (4, 4, 1), (2, 3, 2), (3, 2, 3), (5, 0, 0), (0, 5, 5)])


def selftest():
    syms = symmetries()
    assert len(syms) == 1152
    assert len(set(busy_perms())) == 1152      # the action on the busy cells is faithful
    assert slack(()) == 216 - 2 * 148
    assert slack(T7) == 1, slack(T7)
    assert slack(T8) == 8, slack(T8)
    ok, _ = find_pairing(T7, ())
    assert ok
    ok, S = find_pairing((), ())
    assert not ok and deficiency(S, ()) > 0
    assert admissible(T8, ())
    k, B, F = parse_instance("8 0,215,34 -")
    assert (k, B, F) == (8, (0, 34, 215), ())
    assert fmt_instance(k, B, F) == "8 0,34,215 -"
    # antipode map is a symmetry
    assert tuple(ANTIP) in set(syms)
    print("geom selftest ok: %d lines, %d busy cells, %d symmetries, slack(T7)=%d, slack(T8)=%d"
          % (NL, len(BUSY), len(syms), slack(T7), slack(T8)))


if __name__ == "__main__":
    selftest()

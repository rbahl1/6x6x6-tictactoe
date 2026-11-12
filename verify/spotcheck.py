#!/usr/bin/env python3
"""verify/spotcheck.py -- third, independent re-check of the 6x6x6 proof tree (pure Python).

Written ONLY from SPEC.md sections 1, 3, 4, 6.  It shares no code with src/, sim/, verify/treecheck.cpp or
py/geom.py: own geometry, own symmetry group, own canonical form, own matching / max-flow, own LOST.
All arithmetic is exact (Python integers; cell sets are 216-bit integers).

  python3 verify/spotcheck.py --table FILE --leaves FILE --rmax 10 --samples N --seed S --procs P --out FILE.json

  --samples N        number of random descents from the root (part C).  Descents 0,1,2 are deterministic
                     (main line; reveal whenever possible; forced child whenever possible).
  --exhaustive-t T   every reachable state with round <= T is checked exhaustively (default 5)
  --full             exhaustive traversal of the WHOLE tree from the root (structure only; LEAF is still
                     recomputed on the part-B sample and on the leaves hit by the descents)
  --leaf-samples M   part B: uniform seeded sample of M leaves with t-|F| <= 3, in addition to ALL leaves
                     with t-|F| >= 4 (default 5000)
  --all-leaves       part B: recompute LEAF for every line of the leaf file
  --canon-leaves M   part A: canonical-form check on M sampled leaf lines; 0 = all leaf lines (default 0)
  --selftest         unit tests of this file (geometry, group, canonical form, LEAF flow code against a
                     literal brute force, LOST against a literal unpruned enumeration); no files needed

What is checked
  A. every line of both files: syntax, ascending lists, sizes (|B| = t-1 table / t leaves, |F| <= t),
     B and F disjoint, table move outside B and F, no duplicate keys, key is in canonical form (SPEC 6).
  B. LEAF(B,F,t) (SPEC 3) recomputed for the stratified leaf sample.
  C. node expansion (SPEC 4): exhaustive for t <= T, then random descents (or everything with --full).
     non-leaf P-state: t < R_max, LOST false, every child (a),(c),(d) is VOID / has a table entry (Q) /
     is a listed leaf or has t+1 < R_max (P).  Listed leaf: LEAF recomputed (descents).  Q-state: table
     entry, (i) child is a listed leaf or has t < R_max, (ii) child is VOID or has a table entry.
Every error is re-computed a second time by the master before it is reported ("reproduced").

Remark on LOST: every E2 pattern has >= 11 distinct stones and every E3 pattern >= 12 (two lines share at
most one cell), while an expanded P-state has t+1 <= R_max stones available.  For R_max = 10 LOST is
therefore false at every expanded state for a purely arithmetic reason; this script still runs its pruned
enumeration whenever its F-relative screen does not reject (reported as p_states_where_LOST_enumeration_ran).
"""
import sys, os, json, time, random, argparse, itertools
import multiprocessing as mp

# ----------------------------------------------------------------------------------------------------
# SPEC 1: board, lines, busy cells, symmetry group
# ----------------------------------------------------------------------------------------------------
NC = 216


def _build_lines():
    fs = [('c', v) for v in range(6)] + [('t', 0), ('r', 0)]
    out = set()
    for f1 in fs:
        for f2 in fs:
            for f3 in fs:
                if f1[0] == 'c' and f2[0] == 'c' and f3[0] == 'c':
                    continue
                cells = []
                for t in range(6):
                    co = []
                    for f in (f1, f2, f3):
                        co.append(f[1] if f[0] == 'c' else (t if f[0] == 't' else 5 - t))
                    cells.append(36 * co[0] + 6 * co[1] + co[2])
                out.add(tuple(sorted(cells)))
    return sorted(out)


LINES = _build_lines()
NL = len(LINES)
LM = [sum(1 << c for c in ln) for ln in LINES]            # line masks
CL = [[] for _ in range(NC)]                              # lines through a cell
for _l, _ln in enumerate(LINES):
    for _c in _ln:
        CL[_c].append(_l)


def _coords(c):
    return (c // 36, (c // 6) % 6, c % 6)


BUSY = [c for c in range(NC)
        if len({min(v, 5 - v) for v in _coords(c)}) == 1]
BUSYSET = frozenset(BUSY)


def _build_group():
    rel = [s for s in itertools.permutations(range(6)) if all(s[5 - i] == 5 - s[i] for i in range(6))]
    maps = set()
    for p in itertools.permutations(range(3)):
        for rv in itertools.product((0, 1), repeat=3):
            for s in rel:
                img = []
                for c in range(NC):
                    v = _coords(c)
                    w = [v[p[0]], v[p[1]], v[p[2]]]
                    w = [5 - w[i] if rv[i] else w[i] for i in range(3)]
                    w = [s[w[i]] for i in range(3)]
                    img.append(36 * w[0] + 6 * w[1] + w[2])
                maps.add(tuple(img))
    return sorted(maps), len(rel)


GM, _NREL = _build_group()
NG = len(GM)
ALLG = (1 << NG) - 1
# MT[v][c] = bitmask over g of "g(c) == v"
MT = [[0] * NC for _ in range(NC)]
for _g, _gm in enumerate(GM):
    _bit = 1 << _g
    for _c in range(NC):
        MT[_gm[_c]][_c] |= _bit
IDENT = GM.index(tuple(range(NC)))


def mask(cells):
    m = 0
    for c in cells:
        m |= 1 << c
    return m


def cells_of(m):
    out = []
    while m:
        b = m & -m
        out.append(b.bit_length() - 1)
        m ^= b
    return out


# ----------------------------------------------------------------------------------------------------
# SPEC 6: canonical form.  Greedy over cell ids: among the still-possible maps, the lexicographically
# smallest ascending image list contains the smallest id that any possible map can produce.
# ----------------------------------------------------------------------------------------------------
def _scan(cells, cand):
    n = len(cells)
    if n == 0:
        return (), cand
    if cand & (cand - 1) == 0:
        gm = GM[cand.bit_length() - 1]
        return tuple(sorted([gm[c] for c in cells])), cand
    img = []
    v = 0
    while True:
        mv = MT[v]
        h = 0
        for c in cells:
            h |= mv[c]
        x = cand & h
        if x:
            cand = x
            img.append(v)
            if len(img) == n:
                return tuple(img), cand
            if x & (x - 1) == 0:
                gm = GM[x.bit_length() - 1]
                return tuple(sorted([gm[c] for c in cells])), cand
        v += 1


def canon(B, F):
    """canonical (B,F): smallest image pair, B-images first (order 2.4), F-images second."""
    bi, cand = _scan(B, ALLG)
    fi, cand = _scan(F, cand)
    return bi, fi


def canon_brute(B, F):
    best = None
    for gm in GM:
        k = (tuple(sorted(gm[c] for c in B)), tuple(sorted(gm[c] for c in F)))
        if best is None or k < best:
            best = k
    return best


# ----------------------------------------------------------------------------------------------------
# SPEC 3: admissibility, pairing, LEAF
# ----------------------------------------------------------------------------------------------------
def live_lines(Bm):
    return [l for l in range(NL) if not (LM[l] & Bm)]


def admissible(Bm, Xm):
    if Bm & Xm:
        return False
    seen = 0
    for l in range(NL):
        lm = LM[l]
        if lm & Bm:
            continue
        n = (lm & Xm).bit_count()
        if n > 4:
            return False
        if n == 4:
            e = lm & ~Xm                # the 2 cells of l outside X
            if e & seen:                # shared with another live 4-line, and not in X: danger pair
                return False
            seen |= e
    return True


def find_pairing(live, LC):
    """2 cells of l\\F for every live line, no cell twice.  Returns (owner list | None, violator lines)."""
    owner = [-1] * NC

    def grab(l, seen):
        for c in LC[l]:
            o = owner[c]
            if o == l or c in seen:
                continue
            seen.add(c)
            if o < 0 or grab(o, seen):
                owner[c] = l
                return True
        return False

    for l in live:
        for _ in range(2):
            if not grab(l, set()):
                # Hall violator: lines reachable from l by alternating paths
                R = {l}
                stack = [l]
                while stack:
                    a = stack.pop()
                    for c in LC[a]:
                        o = owner[c]
                        if o >= 0 and o not in R:
                            R.add(o)
                            stack.append(o)
                return None, sorted(R)
    return owner, None


def _augment(owner, LC, src, apply):
    """one augmenting path from line src to an unowned cell (BFS over lines)."""
    par = {src: None}
    queue = [src]
    i = 0
    while i < len(queue):
        l = queue[i]
        i += 1
        for c in LC[l]:
            o = owner[c]
            if o < 0:
                if apply:
                    cur, cell = l, c
                    while True:
                        owner[cell] = cur
                        p = par[cur]
                        if p is None:
                            break
                        cur, cell = p
                return True
            if o not in par:
                par[o] = (l, c)
                queue.append(o)
    return False


def _reach(owner, LC, srcs):
    R = set(srcs)
    stack = list(srcs)
    while stack:
        a = stack.pop()
        for c in LC[a]:
            o = owner[c]
            if o >= 0 and o not in R:
                R.add(o)
                stack.append(o)
    return R


def sur(S, Fm):
    n = 0
    for l in S:
        n |= LM[l]
    return (n & ~Fm).bit_count() - 2 * len(S)


def f_connected(S, Fm):
    S = list(S)
    if not S:
        return True
    comp = {S[0]}
    stack = [S[0]]
    while stack:
        a = stack.pop()
        for b in S:
            if b not in comp and (LM[a] & LM[b] & ~Fm):
                comp.add(b)
                stack.append(b)
    return len(comp) == len(S)


def leaf_check(Bm, Fm, k):
    """LEAF(B,F,k) of SPEC 3.  Returns (holds, reason, witness).
    reason 0 holds; 1 F not admissible; 2 no pairing; 3 C_Phi fails (witness = F-connected S, |S|>=3,
    sur_F(S) < k-|F|, verified by direct counting); 4 k < |F|."""
    nF = Fm.bit_count()
    if k < nF:
        return False, 4, None
    if not admissible(Bm, Fm):
        return False, 1, None
    live = live_lines(Bm)
    LC = [None] * NL
    for l in live:
        LC[l] = tuple(c for c in LINES[l] if not (Fm >> c) & 1)
    owner, viol = find_pairing(live, LC)
    if owner is None:
        if sur(viol, Fm) >= 0:
            raise RuntimeError("internal: pairing failed without a Hall violator")
        return False, 2, viol
    theta = k - nF
    if theta <= 0:
        return True, 0, None
    # single lines: mu({l}) capped at theta
    single = {}
    for l in live:
        if len(LC[l]) - 2 >= theta:          # otherwise mu({l}) <= sur({l}) < theta for sure
            st = owner[:]
            n = 0
            while n < theta and _augment(st, LC, l, True):
                n += 1
            if n >= theta:
                continue
        else:
            st = owner[:]
            n = 0
            while n < theta and _augment(st, LC, l, True):
                n += 1
        single[l] = (st, n)
    weak = sorted(single)
    if len(weak) < 3:
        return True, 0, None
    wset = set(weak)
    adj = {l: set() for l in weak}
    notF = ~Fm
    for i, a in enumerate(weak):
        for b in weak[i + 1:]:
            if LM[a] & LM[b] & notF:
                adj[a].add(b)
                adj[b].add(a)
    # weak adjacent pairs: mu({a,b}) < theta
    pairs = {}
    for a in weak:
        st_a, n_a = single[a]
        for b in adj[a]:
            if b < a:
                continue
            st = st_a[:]
            n = n_a
            while n < theta and _augment(st, LC, b, True):
                n += 1
            if n < theta:
                pairs[(a, b)] = (st, n)
    # F-connected triples, each handled from its smallest adjacent pair
    for (a, b), (st, n) in pairs.items():
        need = theta - n
        for c in (adj[a] | adj[b]):
            if c == a or c == b:
                continue
            skip = False
            for p, q in ((a, c), (b, c)):
                if q in adj[p]:
                    pq = (p, q) if p < q else (q, p)
                    if pq not in pairs:      # mu(pair) >= theta  =>  mu(T) >= theta
                        skip = True
                        break
                    if pq < (a, b):          # this triple is handled from the smaller pair
                        skip = True
                        break
            if skip:
                continue
            if need == 1:
                ok = _augment(st, LC, c, False)
                fin = st
            else:
                fin = st[:]
                m = 0
                while m < need and _augment(fin, LC, c, True):
                    m += 1
                ok = (m == need)
            if not ok:
                R = _reach(fin, LC, (a, b, c))
                s = sur(R, Fm)
                if not (len(R) >= 3 and s < theta and f_connected(R, Fm)):
                    raise RuntimeError("internal: flow says C_Phi fails but the cut is not a witness")
                return False, 3, sorted(R)
    return True, 0, None


def leaf_brute(Bm, Fm, k, maxlines=16):
    """literal LEAF: admissibility, Hall over all subsets, C_Phi over all F-connected subsets."""
    nF = Fm.bit_count()
    if k < nF:
        return False, 4
    if not admissible(Bm, Fm):
        return False, 1
    live = live_lines(Bm)
    if len(live) > maxlines:
        raise ValueError("too many live lines for brute force")
    theta = k - nF
    n = len(live)
    res3 = True
    for bits in range(1, 1 << n):
        S = [live[i] for i in range(n) if (bits >> i) & 1]
        s = sur(S, Fm)
        if s < 0:
            return False, 2
        if len(S) >= 3 and s < theta and f_connected(S, Fm):
            res3 = False
    return (True, 0) if res3 else (False, 3)


# ----------------------------------------------------------------------------------------------------
# SPEC 4: LOST (E2, E3).  Screens are lower bounds on the number of new stones (sound, see comments).
# ----------------------------------------------------------------------------------------------------
LOST_ENUM = 0


def lost(Bm, Fm, t, e2=True, e3=True):
    """returns None if LOST is false, else a description of a fitting pattern."""
    lim = t + 1
    nF = Fm.bit_count()
    budget = lim - nF
    live = live_lines(Bm)
    if not live:
        return None
    f = [0] * NL
    for l in live:
        f[l] = (LM[l] & Fm).bit_count()
    # |S_l \ F| = 5-f_l (e not in F), |S_a \ F| = 4-f_a (F∩a ⊆ S_a); two distinct lines share <= 1 cell and
    # S_a∩S_b = ∅ (c is in neither), so E2 needs >= (5-f_l)+(4-f_a)+(4-f_b)-2 new stones and E3 needs
    # >= sum of four (4-f) - 4 (the pairs (a,b) and (a',b') are disjoint as stone sets).
    need4 = sorted(4 - f[l] for l in live)
    need5 = min(5 - f[l] for l in live)
    FB = Fm
    global LOST_ENUM
    if (e2 and len(live) >= 3 and need5 + need4[0] + need4[1] - 2 <= budget) or \
            (e3 and len(live) >= 4 and sum(need4[:4]) - 4 <= budget):
        LOST_ENUM += 1                       # the screen did not reject: the enumeration below really runs
    if e2 and len(live) >= 3 and need5 + need4[0] + need4[1] - 2 <= budget:
        for l in live:
            if 5 - f[l] > budget:
                continue
            Ll = LM[l]
            for e in LINES[l]:
                if (FB >> e) & 1:
                    continue
                U1 = FB | (Ll & ~(1 << e))
                for x in LINES[l]:
                    if x == e:
                        continue
                    for a in CL[x]:
                        if a == l or (LM[a] & Bm):
                            continue
                        La = LM[a]
                        if (U1 | La).bit_count() - 2 > lim:
                            continue
                        for c in LINES[a]:
                            if c == x or c == e or (FB >> c) & 1:
                                continue
                            Lac = La & ~(1 << c)
                            for d in LINES[a]:
                                if d == c or d == x or (FB >> d) & 1:
                                    continue
                                U2 = U1 | (Lac & ~(1 << d))
                                if U2.bit_count() > lim:
                                    continue
                                for b in CL[c]:
                                    if b == a or b == l or (LM[b] & Bm):
                                        continue
                                    Lb = LM[b]
                                    if (Lb >> x) & 1 or (Lb >> e) & 1:
                                        continue
                                    Lbc = Lb & ~(1 << c)
                                    if (U2 | Lbc).bit_count() - 1 > lim:
                                        continue
                                    for d2 in LINES[b]:
                                        if d2 == c or (FB >> d2) & 1:
                                            continue
                                        U3 = U2 | (Lbc & ~(1 << d2))
                                        if U3.bit_count() <= lim:
                                            return {"pattern": "E2", "l": LINES[l], "a": LINES[a],
                                                    "b": LINES[b], "x": x, "e": e, "c": c,
                                                    "stones": cells_of(U3)}
    if e3 and len(live) >= 4 and sum(need4[:4]) - 4 <= budget:
        for a in live:
            La = LM[a]
            for x in LINES[a]:
                for a2 in CL[x]:
                    if a2 <= a or (LM[a2] & Bm):       # the pattern is symmetric in (a,b,c) <-> (a',b',c')
                        continue
                    La2 = LM[a2]
                    if (FB | La | La2).bit_count() - 4 > lim:
                        continue
                    for c in LINES[a]:
                        if c == x or (FB >> c) & 1:
                            continue
                        for d in LINES[a]:
                            if d == c or d == x or (FB >> d) & 1:
                                continue
                            U1 = FB | (La & ~(1 << c) & ~(1 << d))
                            if U1.bit_count() > lim:
                                continue
                            for c2 in LINES[a2]:
                                if c2 == x or c2 == c or (FB >> c2) & 1:
                                    continue
                                for d2 in LINES[a2]:
                                    if d2 == c2 or d2 == x or (FB >> d2) & 1:
                                        continue
                                    U2 = U1 | (La2 & ~(1 << c2) & ~(1 << d2))
                                    if U2.bit_count() > lim:
                                        continue
                                    for b in CL[c]:
                                        if b == a or b == a2 or (LM[b] & Bm):
                                            continue
                                        Lb = LM[b]
                                        if (Lb >> x) & 1:
                                            continue
                                        Lbc = Lb & ~(1 << c)
                                        if (Lb >> c2) & 1:
                                            d3s = (c2,)                 # c' must not be a stone
                                        else:
                                            d3s = [q for q in LINES[b] if q != c and not (FB >> q) & 1]
                                        for d3 in d3s:
                                            U3 = U2 | (Lbc & ~(1 << d3))
                                            if U3.bit_count() > lim:
                                                continue
                                            for b2 in CL[c2]:
                                                if b2 == a2 or b2 == a or b2 == b or (LM[b2] & Bm):
                                                    continue
                                                Lb2 = LM[b2]
                                                if (Lb2 >> x) & 1:
                                                    continue
                                                Lb2c = Lb2 & ~(1 << c2)
                                                if (Lb2 >> c) & 1:
                                                    d4s = (c,)          # c must not be a stone
                                                else:
                                                    d4s = [q for q in LINES[b2]
                                                           if q != c2 and not (FB >> q) & 1]
                                                for d4 in d4s:
                                                    U4 = U3 | (Lb2c & ~(1 << d4))
                                                    if U4.bit_count() <= lim:
                                                        return {"pattern": "E3", "a": LINES[a],
                                                                "a'": LINES[a2], "b": LINES[b],
                                                                "b'": LINES[b2], "x": x, "c": c,
                                                                "c'": c2, "stones": cells_of(U4)}
    return None


def lost_literal(Bm, Fm, t, e2=True, e3=True):
    """E2 / E3 transcribed from SPEC 4 with sets and itertools; no pruning.  Unit tests only."""
    lim = t + 1
    F = set(cells_of(Fm))
    live = [set(LINES[l]) for l in live_lines(Bm)]
    n = len(live)
    for il in range(n if e2 else 0):
        for ia in range(n):
            for ib in range(n):
                if len({il, ia, ib}) < 3:
                    continue
                l, a, b = live[il], live[ia], live[ib]
                if len(l & a) != 1 or len(a & b) != 1:
                    continue
                (x,) = l & a
                (c,) = a & b
                if x in b:
                    continue
                for e in l:
                    if len({x, e, c}) < 3 or e in b or e in F or c in F:
                        continue
                    Sl = l - {e}
                    for Sa in itertools.combinations(sorted(a - {c}), 4):
                        Sa = set(Sa)
                        if x not in Sa or not (F & a) <= Sa:
                            continue
                        for Sb in itertools.combinations(sorted(b - {c}), 4):
                            Sb = set(Sb)
                            if not (F & b) <= Sb:
                                continue
                            if len(F | Sl | Sa | Sb) <= lim:
                                return "E2"
    for ia in range(n if e3 else 0):
        for ia2 in range(n):
            if ia == ia2:
                continue
            a, a2 = live[ia], live[ia2]
            if len(a & a2) != 1:
                continue
            (x,) = a & a2
            for ib in range(n):
                if ib in (ia, ia2):
                    continue
                b = live[ib]
                if len(a & b) != 1 or x in b:
                    continue
                (c,) = a & b
                for ib2 in range(n):
                    if ib2 in (ia, ia2, ib):
                        continue
                    b2 = live[ib2]
                    if len(a2 & b2) != 1 or x in b2:
                        continue
                    (c2,) = a2 & b2
                    if len({x, c, c2}) < 3 or c in F or c2 in F:
                        continue
                    for Sa in itertools.combinations(sorted(a - {c}), 4):
                        Sa = set(Sa)
                        if x not in Sa or not (F & a) <= Sa:
                            continue
                        for Sa2 in itertools.combinations(sorted(a2 - {c2}), 4):
                            Sa2 = set(Sa2)
                            if x not in Sa2 or not (F & a2) <= Sa2:
                                continue
                            for Sb in itertools.combinations(sorted(b - {c}), 4):
                                Sb = set(Sb)
                                if not (F & b) <= Sb:
                                    continue
                                for Sb2 in itertools.combinations(sorted(b2 - {c2}), 4):
                                    Sb2 = set(Sb2)
                                    if not (F & b2) <= Sb2:
                                        continue
                                    allst = Sa | Sa2 | Sb | Sb2
                                    if c in allst or c2 in allst:
                                        continue
                                    if len(F | allst) <= lim:
                                        return "E3"
    return None


# ----------------------------------------------------------------------------------------------------
# files
# ----------------------------------------------------------------------------------------------------
def fmt_set(cells):
    return ",".join(str(c) for c in cells) if cells else "-"


def fmt_state(key):
    return "%d %s %s" % (key[0], fmt_set(key[1]), fmt_set(key[2]))


def parse_set(s):
    """returns (tuple, problem or None)"""
    if s == "-":
        return (), None
    try:
        v = tuple(int(p) for p in s.split(","))
    except ValueError:
        return None, "not a cell list"
    for c in v:
        if not (0 <= c < NC):
            return None, "cell id out of range"
    for i in range(1, len(v)):
        if v[i] <= v[i - 1]:
            return None, "cell list not strictly ascending"
    return v, None


def parse_file(path, is_table):
    """returns (entries, errors).  entries: list of (lineno, key, m)"""
    entries, errors = [], []
    nf = 4 if is_table else 3
    with open(path) as fh:
        for no, raw in enumerate(fh, 1):
            ln = raw.rstrip("\n")
            parts = ln.split(" ")
            where = "%s:%d" % (os.path.basename(path), no)
            if len(parts) != nf:
                errors.append({"kind": "bad_line", "where": where, "line": ln[:200], "detail": "field count"})
                continue
            try:
                t = int(parts[0])
            except ValueError:
                errors.append({"kind": "bad_line", "where": where, "line": ln[:200], "detail": "t not an integer"})
                continue
            B, pb = parse_set(parts[1])
            F, pf = parse_set(parts[2])
            if pb or pf:
                errors.append({"kind": "bad_line", "where": where, "line": ln[:200], "detail": pb or pf})
                continue
            m = None
            if is_table:
                try:
                    m = int(parts[3])
                except ValueError:
                    errors.append({"kind": "bad_line", "where": where, "line": ln[:200], "detail": "move not an integer"})
                    continue
            bad = None
            if len(B) != (t - 1 if is_table else t):
                bad = "|B| != %s" % ("t-1" if is_table else "t")
            elif len(F) > t:
                bad = "|F| > t"
            elif set(B) & set(F):
                bad = "B and F not disjoint"
            elif is_table and not (0 <= m < NC):
                bad = "move out of range"
            elif is_table and (m in B or m in F):
                bad = "table move inside B or F"
            if bad:
                errors.append({"kind": "bad_line", "where": where, "state": fmt_state((t, B, F)), "detail": bad})
                continue
            entries.append((no, (t, B, F), m))
    return entries, errors


# ----------------------------------------------------------------------------------------------------
# globals shared with forked workers
# ----------------------------------------------------------------------------------------------------
TABLE = {}        # Q key -> move
LEAVES = set()    # P keys
TAB_ENTRIES = []
LEAF_ENTRIES = []
RMAX = 10
SEED = 0
RECS = {}         # (kind, key) -> record with children (filled by the exhaustive phase, inherited by fork)
P_FORCED = 0.4    # probability of following a forced child (c)/(d) at a P-state when one exists
P_REVEAL = 0.4    # probability of following child (ii) at a Q-state when it exists


def _canon_chunk(args):
    which, lo, hi = args
    ent = TAB_ENTRIES if which == "table" else LEAF_ENTRIES
    bad = []
    void = 0
    for i in range(lo, hi):
        no, key, m = ent[i]
        cb, cf = canon(key[1], key[2])
        if cb != key[1] or cf != key[2]:
            bad.append((which, no, key, (cb, cf)))
        if which == "table" and not admissible(mask(key[1]), mask(key[2])):
            void += 1
    return which, hi - lo, bad, void


def _canon_list(args):
    which, idxs = args
    ent = TAB_ENTRIES if which == "table" else LEAF_ENTRIES
    bad = []
    for i in idxs:
        no, key, m = ent[i]
        cb, cf = canon(key[1], key[2])
        if cb != key[1] or cf != key[2]:
            bad.append((which, no, key, (cb, cf)))
    return which, len(idxs), bad, 0


def _leaf_one(key):
    t, B, F = key
    try:
        ok, reason, wit = leaf_check(mask(B), mask(F), t)
    except Exception as ex:                                   # internal inconsistency = report, do not hide
        return key, False, "exception: %r" % (ex,), None
    return key, ok, reason, wit


def _leaf_chunk(keys):
    t0 = time.process_time()
    out = [_leaf_one(k) for k in keys]
    return out, time.process_time() - t0


# ----------------------------------------------------------------------------------------------------
# node expansion (SPEC 4)
# ----------------------------------------------------------------------------------------------------
def check_P(key):
    """expand a non-VOID P-state given in canonical form.  Leaves are not expanded (rec['leaf'])."""
    t, B, F = key
    rec = {"kind": "P", "key": key, "leaf": False, "errors": [], "n": {"a": 0, "c": 0, "d": 0}, "void": 0,
           "a": None, "forced": [], "leafkids": 0, "void_ex": []}
    st = "P " + fmt_state(key)
    Bm, Fm = mask(B), mask(F)
    if len(B) != t or len(F) > t or (Bm & Fm):
        rec["errors"].append({"kind": "malformed_P_state", "state": st})
        return rec
    if not admissible(Bm, Fm):
        rec["errors"].append({"kind": "visited_VOID_P_state", "state": st})
        return rec
    if key in LEAVES:
        rec["leaf"] = True
        return rec
    if t >= RMAX:
        rec["errors"].append({"kind": "P_not_leaf_at_rmax", "state": st,
                              "detail": "non-leaf P-state with t >= R_max cannot be WON"})
        return rec
    n0 = LOST_ENUM
    pat = lost(Bm, Fm, t)
    rec["lost_enum"] = LOST_ENUM - n0
    if pat is not None:
        rec["errors"].append({"kind": "LOST_true", "state": st, "detail": pat})
    nF = len(F)
    u1 = t + 1 - nF
    forced = set()
    # (a)
    rec["n"]["a"] += 1
    qk = (t + 1, B, F)                       # already canonical: same (B,F)
    if qk in TABLE:
        rec["a"] = qk
    else:
        rec["errors"].append({"kind": "missing_table_entry", "state": st, "child": "Q " + fmt_state(qk),
                              "detail": "child (a)"})
    live = live_lines(Bm)
    fcnt = {l: (LM[l] & Fm).bit_count() for l in live}

    def p_child(B2m, F2m, typ, desc):
        if not admissible(B2m, F2m):
            rec["void"] += 1
            if len(rec["void_ex"]) < 3:
                rec["void_ex"].append("child (%s) %s of %s is VOID: P %d %s %s" % (
                    typ, desc, st, t + 1, fmt_set(cells_of(B2m)), fmt_set(cells_of(F2m))))
            return
        cb, cf = canon(cells_of(B2m), cells_of(F2m))
        ck = (t + 1, cb, cf)
        if ck in LEAVES:
            rec["leafkids"] += 1
            forced.add(ck)
        elif t + 1 < RMAX:
            forced.add(ck)
        else:
            rec["errors"].append({"kind": "child_P_not_leaf_at_rmax", "state": st,
                                  "child": "P " + fmt_state(ck), "detail": "child (%s) %s" % (typ, desc)})

    # (c) threats
    for l in live:
        lm = LM[l]
        if 5 - fcnt[l] > u1:                 # |F ∪ (l\{e})| = |F| + 5 - |F∩l|  (e not in F)
            continue
        for e in LINES[l]:
            if (Fm >> e) & 1:
                continue
            F2 = Fm | (lm & ~(1 << e))
            if F2.bit_count() > t + 1:
                continue
            rec["n"]["c"] += 1
            p_child(Bm | (1 << e), F2, "c", "line %s e=%d" % (fmt_set(LINES[l]), e))
    # (d) dangers
    liveset = set(live)
    for c in range(NC):
        if (Fm >> c) & 1 or (Bm >> c) & 1:
            continue
        ls = [l for l in CL[c] if l in liveset]
        for i in range(len(ls)):
            a = ls[i]
            for j in range(i + 1, len(ls)):
                b = ls[j]
                if 8 - fcnt[a] - fcnt[b] > u1:      # Qa, Qb disjoint, F∩a ⊆ Qa, F∩b ⊆ Qb
                    continue
                Lac = LM[a] & ~(1 << c)
                Lbc = LM[b] & ~(1 << c)
                for da in LINES[a]:
                    if da == c or (Fm >> da) & 1:
                        continue
                    Qa = Lac & ~(1 << da)
                    for db in LINES[b]:
                        if db == c or (Fm >> db) & 1:
                            continue
                        F2 = Fm | Qa | (Lbc & ~(1 << db))
                        if F2.bit_count() > t + 1:
                            continue
                        rec["n"]["d"] += 1
                        p_child(Bm | (1 << c), F2, "d", "c=%d a=%s b=%s" % (c, fmt_set(LINES[a]), fmt_set(LINES[b])))
    rec["forced"] = sorted(forced)
    return rec


def check_Q(key):
    t, B, F = key
    rec = {"kind": "Q", "key": key, "errors": [], "n": {"i": 0, "ii": 0}, "void": 0, "i": None, "ii": None,
           "i_leaf": False, "void_ex": []}
    st = "Q " + fmt_state(key)
    Bm, Fm = mask(B), mask(F)
    if len(B) != t - 1 or len(F) > t or (Bm & Fm):
        rec["errors"].append({"kind": "malformed_Q_state", "state": st})
        return rec
    if not admissible(Bm, Fm):
        rec["errors"].append({"kind": "visited_VOID_Q_state", "state": st})
        return rec
    m = TABLE.get(key)
    if m is None:
        rec["errors"].append({"kind": "missing_table_entry", "state": st})
        return rec
    if (Bm >> m) & 1 or (Fm >> m) & 1 or not (0 <= m < NC):
        rec["errors"].append({"kind": "illegal_table_move", "state": st, "detail": "m=%d" % m})
        return rec
    rec["m_busy"] = m in BUSYSET
    # (i)
    rec["n"]["i"] += 1
    B2 = Bm | (1 << m)
    if not admissible(B2, Fm):
        rec["void"] += 1                     # cannot happen (admissibility is monotone in B); counted if it does
        rec["errors"].append({"kind": "child_i_VOID", "state": st, "detail": "m=%d" % m})
    else:
        cb, cf = canon(cells_of(B2), F)
        pk = (t, cb, cf)
        if pk in LEAVES:
            rec["i"] = pk
            rec["i_leaf"] = True
        elif t < RMAX:
            rec["i"] = pk
        else:
            rec["errors"].append({"kind": "child_P_not_leaf_at_rmax", "state": st, "child": "P " + fmt_state(pk),
                                  "detail": "child (i) m=%d" % m})
    # (ii)
    if len(F) < t:
        rec["n"]["ii"] += 1
        F2 = Fm | (1 << m)
        if not admissible(Bm, F2):
            rec["void"] += 1
            rec["void_ex"].append("child (ii) m=%d of %s is VOID" % (m, st))
        else:
            cb, cf = canon(B, cells_of(F2))
            qk = (t, cb, cf)
            if qk in TABLE:
                rec["ii"] = qk
            else:
                rec["errors"].append({"kind": "missing_table_entry", "state": st, "child": "Q " + fmt_state(qk),
                                      "detail": "child (ii) m=%d" % m})
    return rec


def _expand_chunk(items):
    t0 = time.process_time()
    out = []
    for kind, key in items:
        out.append(check_P(key) if kind == "P" else check_Q(key))
    return out, time.process_time() - t0


def _slim(rec):
    s = {"kind": rec["kind"], "key": rec["key"], "errors": rec["errors"], "n": rec["n"], "void": rec["void"],
         "void_ex": rec.get("void_ex", [])}
    if rec["kind"] == "P":
        s["leaf"] = rec["leaf"]
        s["leafkids"] = rec.get("leafkids", 0)
        s["nforced"] = len(rec.get("forced", ()))
        s["lost_enum"] = rec.get("lost_enum", 0)
    else:
        s["m_busy"] = rec.get("m_busy")
    return s


def _descents(idxs):
    """random descents from the root.  Deterministic per descent index (independent of --procs)."""
    t0 = time.process_time()
    cache = {}
    visited = {}
    leafres = {}
    lens = []

    def get(kind, key):
        k = (kind, key)
        r = RECS.get(k)
        if r is None:
            r = cache.get(k)
            if r is None:
                r = check_P(key) if kind == "P" else check_Q(key)
                cache[k] = r
        return r

    for i in idxs:
        rng = random.Random("%d:descent:%d" % (SEED, i))
        kind, key = "P", (0, (), ())
        steps = 0
        while True:
            rec = get(kind, key)
            steps += 1
            if (kind, key) not in visited:
                visited[(kind, key)] = _slim(rec)
            if kind == "P":
                if rec["leaf"]:
                    if key not in leafres:
                        leafres[key] = _leaf_one(key)
                    break
                forced = rec["forced"]
                a = rec["a"]
                if i == 0:
                    go_forced = False
                elif i == 2:
                    go_forced = bool(forced)
                else:
                    go_forced = bool(forced) and (a is None or rng.random() < P_FORCED)
                if go_forced:
                    kind, key = "P", forced[rng.randrange(len(forced))]
                elif a is not None:
                    kind, key = "Q", a
                else:
                    break
            else:
                ci, cii = rec["i"], rec["ii"]
                if i == 1:
                    go_ii = cii is not None
                elif i == 0 or i == 2:
                    go_ii = False
                else:
                    go_ii = cii is not None and (ci is None or rng.random() < P_REVEAL)
                if go_ii:
                    kind, key = "Q", cii
                elif ci is not None:
                    kind, key = "P", ci
                else:
                    break
        lens.append(steps)
    return visited, leafres, lens, time.process_time() - t0


# ----------------------------------------------------------------------------------------------------
# self test
# ----------------------------------------------------------------------------------------------------
def selftest(seed):
    rng = random.Random("selftest:%d" % seed)
    res = {}

    def cid(x, y, z):
        return 36 * x + 6 * y + z

    # geometry
    assert NL == 148, NL
    kinds = [0, 0, 0]
    for ln in LINES:
        co = [_coords(c) for c in ln]
        nvar = sum(1 for i in range(3) if len({p[i] for p in co}) > 1)
        kinds[nvar - 1] += 1
    assert kinds == [108, 36, 4], kinds
    assert len(BUSY) == 24 and all(len(CL[c]) == 7 for c in BUSY)
    assert all(len(CL[c]) in (3, 4) for c in range(NC) if c not in BUSYSET)
    for a in range(NL):
        for b in range(a + 1, NL):
            assert (LM[a] & LM[b]).bit_count() <= 1
    assert _NREL == 48 and NG == 1152, (_NREL, NG)
    lineset = set(LINES)
    for gm in GM:
        assert sorted(gm) == list(range(NC))
        for ln in LINES:
            assert tuple(sorted(gm[c] for c in ln)) in lineset
    gset = set(GM)
    for _ in range(300):                                  # closure under composition (sample) and inverse
        g, h = GM[rng.randrange(NG)], GM[rng.randrange(NG)]
        assert tuple(g[h[c]] for c in range(NC)) in gset
        inv = [0] * NC
        for c in range(NC):
            inv[g[c]] = c
        assert tuple(inv) in gset
    res["geometry"] = "148 lines (108/36/4), 24 busy cells on 7 lines, |G| = 1152, lines -> lines"

    # canonical form against brute force over the 1152 maps
    n = 0
    for _ in range(250):
        nb = rng.randrange(0, 11)
        nf = rng.randrange(0, 11)
        pool = BUSY if rng.random() < 0.5 else list(range(NC))
        cells = rng.sample(pool, min(len(pool), nb + nf))
        B, F = cells[:nb], cells[nb:]
        if rng.random() < 0.3 and B:                     # symmetric B: orbit of one cell under a few maps
            B = list({GM[rng.randrange(NG)][B[0]] for _ in range(nb)} | {B[0]})
            F = [c for c in F if c not in B]
        k1 = canon(B, F)
        k2 = canon_brute(B, F)
        assert k1 == k2, (B, F, k1, k2)
        g = GM[rng.randrange(NG)]
        assert canon([g[c] for c in B], [g[c] for c in F]) == k1
        n += 1
    res["canon"] = "%d random states: greedy canon == brute force over 1152 maps, invariant under G" % n

    # SPEC 8 values that LEAF can reproduce (F = empty: LEAF <=> C)
    T7 = [cid(0, 0, 0), cid(5, 5, 5), cid(1, 1, 4), cid(4, 4, 1), cid(2, 3, 2), cid(3, 2, 3), cid(5, 0, 0)]
    T8 = T7 + [cid(0, 5, 5)]
    slack = lambda Bc: (216 - len(Bc)) - 2 * len(live_lines(mask(Bc)))
    assert slack(T7) == 1 and slack(T8) == 8
    assert leaf_check(mask(T7), 0, 7)[1] == 3              # pairing exists, C_Phi fails (worst = 6)
    assert leaf_check(mask(T8), 0, 8)[0] is True           # worst = 0
    assert leaf_check(mask(T8), 0, 9)[0] is False          # worst = 1
    assert leaf_check(0, 0, 0)[1] == 2                     # empty board: no pairing
    B9 = T8 + [cid(0, 1, 2)]
    F9 = [cid(x, 1, 2) for x in range(1, 6)]
    res["spec8"] = "slack(T7)=1, slack(T8)=8, LEAF(T7,-,7)=F, LEAF(T8,-,8)=T, LEAF(T8,-,9)=F; item 7 LEAF=%s" % (
        leaf_check(mask(B9), mask(F9), 9)[0],)

    # LEAF flow code against the literal brute force on artificial instances with <= 14 live lines
    cnt = {0: 0, 1: 0, 2: 0, 3: 0}
    tries = 0
    while sum(cnt.values()) < 6000 and tries < 200000:
        tries += 1
        nk = rng.randrange(3, 13)
        keep = set()
        seedc = rng.randrange(NC)
        keep.add(rng.choice(CL[seedc]))
        while len(keep) < nk:
            if rng.random() < 0.8:
                c = rng.choice(LINES[rng.choice(sorted(keep))])
                keep.add(rng.choice(CL[c]))
            else:
                keep.add(rng.randrange(NL))
        N = 0
        for l in keep:
            N |= LM[l]
        Bm = ((1 << NC) - 1) & ~N
        if rng.random() < 0.5:                           # a few extra SP stones inside
            for c in rng.sample(cells_of(N), rng.randrange(0, 4)):
                Bm |= 1 << c
        live = live_lines(Bm)
        if not (3 <= len(live) <= 14):
            continue
        free = cells_of(((1 << NC) - 1) & ~Bm)
        mode = rng.random()
        if mode < 0.3:
            nf = rng.randrange(0, min(9, len(free)))
            Fm = mask(rng.sample(free, nf))
        elif mode < 0.6:                                 # F on the private cells of a few live lines
            grp = rng.sample(live, rng.randrange(2, min(5, len(live)) + 1))
            Fm = 0
            for l in grp:
                priv = [c for c in LINES[l] if all(not (LM[o] >> c) & 1 for o in grp if o != l)]
                for c in rng.sample(priv, min(len(priv), rng.randrange(2, 5))):
                    Fm |= 1 << c
            nf = Fm.bit_count()
        else:                                            # F concentrated on a few live lines
            Fm = 0
            for l in rng.sample(live, rng.randrange(1, min(5, len(live)) + 1)):
                for c in rng.sample(LINES[l], rng.randrange(1, 5)):
                    Fm |= 1 << c
            nf = Fm.bit_count()
        if rng.random() < 0.9 and not admissible(Bm, Fm):
            continue
        for theta in range(0, 8):
            k = nf + theta
            r1 = leaf_check(Bm, Fm, k)
            r2 = leaf_brute(Bm, Fm, k)
            assert (r1[0], r1[1]) == r2, ("LEAF mismatch", cells_of(Bm), cells_of(Fm), k, r1, r2)
            cnt[r2[1]] += 1
    res["leaf_vs_brute"] = "%d instances (<=14 live lines): holds %d, not admissible %d, no pairing %d, C_Phi fails %d" % (
        sum(cnt.values()), cnt[0], cnt[1], cnt[2], cnt[3])
    assert cnt[0] > 50 and cnt[3] > 50 and cnt[2] > 20, cnt

    # LOST against the literal unpruned enumeration
    lc = {"E2": 0, "E3": 0, None: 0}
    tries = 0
    while sum(lc.values()) < 1200 and tries < 100000:
        tries += 1
        nk = rng.randrange(3, 8)
        c0 = rng.randrange(NC)
        keep = {rng.choice(CL[c0])}
        while len(keep) < nk:
            c = rng.choice(LINES[rng.choice(sorted(keep))])
            keep.add(rng.choice(CL[c]))
        N = 0
        for l in keep:
            N |= LM[l]
        Bm = ((1 << NC) - 1) & ~N
        live = live_lines(Bm)
        if not (3 <= len(live) <= 9):
            continue
        free = cells_of(N)
        nf = rng.randrange(0, 13)
        Fm = 0
        for l in rng.sample(sorted(keep), min(len(keep), 3)):       # F concentrated on a few lines
            for c in rng.sample(LINES[l], rng.randrange(0, 5)):
                if Fm.bit_count() < nf:
                    Fm |= 1 << c
        if not admissible(Bm, Fm) and rng.random() < 0.8:
            continue
        t = Fm.bit_count() + rng.randrange(0, 9)
        for which in ("E2", "E3"):
            r1 = lost(Bm, Fm, t, e2=(which == "E2"), e3=(which == "E3"))
            r2 = lost_literal(Bm, Fm, t, e2=(which == "E2"), e3=(which == "E3"))
            assert (r1 is None) == (r2 is None), ("LOST mismatch", which, cells_of(Bm), cells_of(Fm), t, r1, r2)
            if r1 is not None:
                assert r1["pattern"] == which == r2 and len(r1["stones"]) <= t + 1 and Fm & ~mask(r1["stones"]) == 0
            lc[which if r1 is not None else None] += 1
    res["lost_vs_literal"] = "%d instances: E2 %d, E3 %d, not lost %d" % (sum(lc.values()), lc["E2"], lc["E3"], lc[None])
    assert lc["E2"] > 20 and lc["E3"] > 5 and lc[None] > 50, lc
    # empty board sanity: 11 stones = E2 fits, 10 do not; 12 needed for E3-only shape is covered by E2 at 11
    assert lost(0, 0, 9) is None and lost(0, 0, 10) is not None
    return res


# ----------------------------------------------------------------------------------------------------
# main
# ----------------------------------------------------------------------------------------------------
def chunks(seq, n):
    for i in range(0, len(seq), n):
        yield seq[i:i + n]


def main():
    global TABLE, LEAVES, TAB_ENTRIES, LEAF_ENTRIES, RMAX, SEED, RECS
    ap = argparse.ArgumentParser()
    ap.add_argument("--table")
    ap.add_argument("--leaves")
    ap.add_argument("--rmax", type=int, default=10)
    ap.add_argument("--samples", type=int, default=300)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--procs", type=int, default=2)
    ap.add_argument("--out")
    ap.add_argument("--exhaustive-t", type=int, default=5)
    ap.add_argument("--full", action="store_true")
    ap.add_argument("--leaf-samples", type=int, default=5000)
    ap.add_argument("--canon-leaves", type=int, default=0)
    ap.add_argument("--all-leaves", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    wall0 = time.time()

    if args.selftest:
        r = selftest(args.seed)
        for k, v in r.items():
            print("selftest %-16s %s" % (k, v))
        print("selftest OK")
        if args.out:
            with open(args.out, "w") as fh:
                json.dump({"ok": True, "selftest": r}, fh, indent=1)
        return 0
    if not (args.table and args.leaves and args.out):
        ap.error("--table, --leaves and --out are required")

    RMAX, SEED = args.rmax, args.seed
    errors = []
    notes = []

    def log(msg):
        print("[%7.1fs] %s" % (time.time() - wall0, msg), flush=True)

    # ---------------- part A: file-level checks ----------------
    TAB_ENTRIES, e1 = parse_file(args.table, True)
    LEAF_ENTRIES, e2 = parse_file(args.leaves, False)
    errors += e1 + e2
    table_lines = len(TAB_ENTRIES) + len(e1)
    leaf_lines = len(LEAF_ENTRIES) + len(e2)
    nonbusy = 0
    for no, key, m in TAB_ENTRIES:
        if key in TABLE:
            errors.append({"kind": "duplicate_table_key", "where": "table:%d" % no, "state": "Q " + fmt_state(key),
                           "detail": "moves %d and %d" % (TABLE[key], m)})
        else:
            TABLE[key] = m
        if m not in BUSYSET:
            nonbusy += 1
        if key[0] > RMAX or key[0] < 1:
            notes.append("table line %d has t=%d outside 1..R_max" % (no, key[0]))
    for no, key, m in LEAF_ENTRIES:
        if key in LEAVES:
            errors.append({"kind": "duplicate_leaf_key", "where": "leaves:%d" % no, "state": "P " + fmt_state(key)})
        else:
            LEAVES.add(key)
        if key[0] > RMAX:
            notes.append("leaf line %d has t=%d > R_max" % (no, key[0]))
    log("parsed %d table lines, %d leaf lines (%d syntax/size errors)" % (table_lines, leaf_lines, len(e1) + len(e2)))

    ctx = mp.get_context("fork")
    procs = max(1, args.procs)
    cpu_leaf = 0.0
    cpu_expand = 0.0

    # canonical form of all table lines and all (or sampled) leaf lines
    tasks = [("table", lo, min(lo + 2000, len(TAB_ENTRIES))) for lo in range(0, len(TAB_ENTRIES), 2000)]
    list_tasks = []
    if args.canon_leaves and args.canon_leaves < len(LEAF_ENTRIES):
        idx = sorted(random.Random("%d:canonleaves" % SEED).sample(range(len(LEAF_ENTRIES)), args.canon_leaves))
        list_tasks = [("leaves", ch) for ch in chunks(idx, 2000)]
        canon_all_leaves = False
    else:
        tasks += [("leaves", lo, min(lo + 2000, len(LEAF_ENTRIES))) for lo in range(0, len(LEAF_ENTRIES), 2000)]
        canon_all_leaves = True
    canon_checked = {"table": 0, "leaves": 0}
    table_void_keys = 0
    with ctx.Pool(procs) as pool:
        for which, n, bad, void in itertools.chain(pool.imap_unordered(_canon_chunk, tasks),
                                                   pool.imap_unordered(_canon_list, list_tasks)):
            canon_checked[which] += n
            table_void_keys += void
            for (w, no, key, ck) in bad:
                cb, cf = canon_brute(key[1], key[2])                 # reproduce with the brute-force canon
                errors.append({"kind": "noncanonical_key", "where": "%s:%d" % (w, no),
                               "state": ("Q " if w == "table" else "P ") + fmt_state(key),
                               "detail": "canonical form is %s %s" % (fmt_set(ck[0]), fmt_set(ck[1])),
                               "reproduced": (cb, cf) == ck and (cb, cf) != (key[1], key[2])})
    if table_void_keys:
        notes.append("%d table keys are VOID states (F not B-admissible); never looked up" % table_void_keys)
    log("part A: canonical form checked on %d table + %d leaf lines; %d errors so far" % (
        canon_checked["table"], canon_checked["leaves"], len(errors)))

    # ---------------- part B: LEAF on a stratified sample ----------------
    leaf_keys = [k for _, k, _ in LEAF_ENTRIES]
    big = [k for k in leaf_keys if k[0] - len(k[2]) >= 4]
    rest = [k for k in leaf_keys if k[0] - len(k[2]) < 4]
    nsmp = len(rest) if (args.all_leaves or args.leaf_samples < 0) else min(len(rest), args.leaf_samples)
    smp = random.Random("%d:leafsample" % SEED).sample(rest, nsmp)
    todo = sorted(set(big) | set(smp))
    random.Random("%d:leaforder" % SEED).shuffle(todo)
    leaf_results = {}
    with ctx.Pool(procs) as pool:
        for out, cpu in pool.imap_unordered(_leaf_chunk, list(chunks(todo, 40))):
            cpu_leaf += cpu
            for key, ok, reason, wit in out:
                leaf_results[key] = (ok, reason, wit)
    theta_hist = {}
    for key, (ok, reason, wit) in leaf_results.items():
        th = key[0] - len(key[2])
        theta_hist[th] = theta_hist.get(th, 0) + 1
    log("part B: LEAF recomputed on %d leaves (%d with t-|F| >= 4, %d sampled of %d others), %.0f CPU-s" % (
        len(leaf_results), len(big), nsmp, len(rest), cpu_leaf))

    # ---------------- part C: node expansion ----------------
    root = ("P", (0, (), ()))
    limit = RMAX if args.full else args.exhaustive_t
    seen = {root}
    frontier = [root]
    counted = set()              # (kind, key) of every state accounted for
    reached_leaves = set()
    children = {"a": 0, "c": 0, "d": 0, "i": 0, "ii": 0}
    tot = {"void": 0, "p_expanded": 0, "q_checked": 0, "p_leaf": 0, "nonbusy": 0, "lost_enum": 0,
           "leafkids": 0, "forced_distinct": 0}
    by_t = {}
    node_errors = []             # (kind, key, errors)
    void_by = {}
    void_examples = []

    def account(k, r):
        if k in counted:
            return
        counted.add(k)
        d = by_t.setdefault(k[1][0], {"P_expanded": 0, "P_leaf": 0, "Q": 0})
        if r["kind"] == "P":
            if r.get("leaf"):
                tot["p_leaf"] += 1
                d["P_leaf"] += 1
                reached_leaves.add(k[1])
            else:
                tot["p_expanded"] += 1
                d["P_expanded"] += 1
                tot["lost_enum"] += r.get("lost_enum", 0)
                tot["leafkids"] += r.get("leafkids", 0)
                tot["forced_distinct"] += r.get("nforced", 0)
        else:
            tot["q_checked"] += 1
            d["Q"] += 1
            if r.get("m_busy") is False:
                tot["nonbusy"] += 1
        for typ, n in r["n"].items():
            children[typ] += n
        tot["void"] += r["void"]
        if r["void"]:
            vt = void_by.setdefault(r["kind"] + str(k[1][0]), 0)
            void_by[r["kind"] + str(k[1][0])] = vt + r["void"]
            for ex in r.get("void_ex", []):
                if len(void_examples) < 40:
                    void_examples.append(ex)
        if r["errors"]:
            node_errors.append((k[0], k[1], r["errors"]))

    LEAFREC = {"kind": "P", "errors": [], "n": {}, "void": 0, "leaf": True}
    rounds = 0
    with ctx.Pool(procs) as pool:
        while frontier:
            rounds += 1
            nxt = []
            csize = max(1, min(200, len(frontier) // (4 * procs) + 1))
            for out, cpu in pool.imap_unordered(_expand_chunk, list(chunks(frontier, csize))):
                cpu_expand += cpu
                for rec in out:
                    k = (rec["kind"], rec["key"])
                    account(k, _slim(rec))
                    if not args.full:
                        RECS[k] = rec
                    if rec["kind"] == "P":
                        if rec["leaf"]:
                            continue
                        kids = [("P", ck) for ck in rec["forced"]]
                        if rec["a"] is not None:
                            kids.append(("Q", rec["a"]))
                    else:
                        kids = []
                        if rec["i"] is not None:
                            kids.append(("P", rec["i"]))
                        if rec["ii"] is not None:
                            kids.append(("Q", rec["ii"]))
                    for ck in kids:
                        if ck in seen or ck[1][0] > limit:
                            continue
                        seen.add(ck)
                        if ck[0] == "P" and ck[1] in LEAVES:      # leaves are not expanded
                            account(ck, LEAFREC)
                            continue
                        nxt.append(ck)
            frontier = sorted(nxt)
            if args.full:
                log("  traversal round %d: %d states so far, next frontier %d" % (rounds, len(counted), len(frontier)))
    n_exh = len(counted)
    log("part C: exhaustive phase (t <= %d): %d states in %d rounds, %.0f CPU-s" % (limit, n_exh, rounds, cpu_expand))

    descent_lens = []
    descent_leaf = {}
    cpu_desc = 0.0
    if args.samples > 0:
        idxs = list(range(args.samples))
        with ctx.Pool(procs) as pool:
            for visited, leafres, lens, cpu in pool.imap_unordered(_descents, list(chunks(idxs, 5))):
                cpu_desc += cpu
                descent_lens += lens
                for k in sorted(visited):
                    account(k, visited[k])
                for key, res in leafres.items():
                    descent_leaf[key] = res[1:]
        log("part C: %d descents, %d states visited in total, %.0f CPU-s" % (args.samples, len(counted), cpu_desc))

    # ---------------- collect ----------------
    for key, res in descent_leaf.items():
        leaf_results.setdefault(key, res)
    for key, (ok, reason, wit) in sorted(leaf_results.items()):
        if not ok:
            again = _leaf_one(key)                                   # second computation
            errors.append({"kind": "LEAF_false", "state": "P " + fmt_state(key),
                           "detail": {"reason": reason, "theta": key[0] - len(key[2]),
                                      "reason_text": {1: "F not B-admissible", 2: "no pairing for (B,F)",
                                                      3: "C_Phi fails", 4: "k < |F|"}.get(reason, str(reason)),
                                      "witness_size": len(wit) if wit else None,
                                      "witness_sur_F": sur(wit, mask(key[2])) if wit else None,
                                      "witness_lines_first12": [list(LINES[l]) for l in wit[:12]] if wit else None},
                           "reproduced": (again[1], again[2]) == (ok, reason)})
    for kind, key, errs in sorted(node_errors, key=lambda z: (z[0], z[1])):
        again = check_P(key) if kind == "P" else check_Q(key)        # second computation
        same = json.dumps(again["errors"], sort_keys=True, default=str) == json.dumps(errs, sort_keys=True, default=str)
        for e in errs:
            e = dict(e)
            e["reproduced"] = same
            errors.append(e)
    unreached = None
    if args.full:
        qv = sum(1 for k in counted if k[0] == "Q")
        unreached = {"table_entries_not_reached": len(TABLE) - qv,
                     "leaf_entries_not_reached": len(LEAVES) - len(reached_leaves)}
        if unreached["table_entries_not_reached"] or unreached["leaf_entries_not_reached"]:
            notes.append("full traversal: %d table entries and %d leaf entries are not reachable from the root "
                         "(allowed by SPEC 7.4: extra proven states)" % (
                             unreached["table_entries_not_reached"], unreached["leaf_entries_not_reached"]))

    for e in errors:
        print("ERROR %s" % json.dumps(e, default=str), flush=True)
    tms = os.times()
    cpu_total = tms.user + tms.system + tms.children_user + tms.children_system
    result = {
        "ok": len(errors) == 0,
        "table_lines": table_lines,
        "leaf_lines": leaf_lines,
        "canonical_checked": canon_checked["table"] + canon_checked["leaves"],
        "leaves_recomputed": len(leaf_results),
        "p_expanded": tot["p_expanded"],
        "q_checked": tot["q_checked"],
        "children_checked": children,
        "void_children": tot["void"],
        "errors": errors[:200],
        "error_count": len(errors),
        # ---- extra detail ----
        "rmax": RMAX, "seed": SEED, "procs": procs, "samples": args.samples,
        "mode": "full traversal from the root" + (" + %d descents" % args.samples if args.samples else "")
                if args.full else "exhaustive t<=%d + %d random descents" % (limit, args.samples),
        "children_counting": "raw children generated from the canonical representative (not deduplicated by symmetry), VOID included",
        "canonical_checked_table": canon_checked["table"],
        "canonical_checked_leaves": canon_checked["leaves"],
        "canonical_all_leaf_lines": canon_all_leaves,
        "table_nonbusy_moves": nonbusy,
        "table_void_keys": table_void_keys,
        "leaves_recomputed_partB": len(todo),
        "leaves_theta_ge4_all": len(big),
        "leaves_theta_le3_sampled": nsmp,
        "leaves_theta_le3_total": len(rest),
        "leaves_recomputed_by_theta_partB": {str(k): v for k, v in sorted(theta_hist.items())},
        "leaves_recomputed_in_descents": len(descent_leaf),
        "p_leaf_visited": tot["p_leaf"],
        "distinct_leaves_reached": len(reached_leaves),
        "states_exhaustive_phase": n_exh,
        "states_visited_total": len(counted),
        "states_visited_by_t": {str(t): by_t[t] for t in sorted(by_t)},
        "q_visited_nonbusy_move": tot["nonbusy"],
        "p_states_where_LOST_enumeration_ran": tot["lost_enum"],
        "distinct_forced_children": tot["forced_distinct"],
        "children_that_are_leaves_distinct_per_parent": tot["leafkids"],
        "unreached": unreached,
        "void_children_by_parent_kind_and_t": void_by,
        "void_examples": void_examples,
        "descent_steps": {"n": len(descent_lens), "mean": round(sum(descent_lens) / len(descent_lens), 2) if descent_lens else 0,
                          "max": max(descent_lens) if descent_lens else 0},
        "coverage": {"q_checked_fraction_of_table": round(tot["q_checked"] / max(1, len(TABLE)), 4),
                     "leaves_recomputed_fraction": round(len(leaf_results) / max(1, len(LEAVES)), 4),
                     "leaves_reached_fraction": round(len(reached_leaves) / max(1, len(LEAVES)), 4)},
        "cpu_seconds": {"total": round(cpu_total, 1), "leaf_partB": round(cpu_leaf, 1), "expand": round(cpu_expand, 1),
                        "descents": round(cpu_desc, 1)},
        "wall_seconds": round(time.time() - wall0, 1),
        "notes": notes[:50],
        "argv": sys.argv,
    }
    with open(args.out, "w") as fh:
        json.dump(result, fh, indent=1, default=str)
    log("ok=%s  errors=%d  p_expanded=%d q_checked=%d children=%s void=%d leaves_recomputed=%d  CPU %.0fs" % (
        result["ok"], len(errors), tot["p_expanded"], tot["q_checked"], children, tot["void"], len(leaf_results), cpu_total))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())

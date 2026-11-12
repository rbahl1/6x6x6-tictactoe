#!/usr/bin/env python3
"""Checks for Section 9.1 and Appendix B of the paper (the 7x7x7 board): the decomposition of the 7^3 board
by its three mid-planes (after Golomb and Hales) and the PRINTED pairing table of Appendix B.

Board {0..6}^3.  M = cells with at least one coordinate equal to 3 (127 cells); A = the other 216 cells, a
6x6x6 array with coordinates {0,1,2,4,5,6}.

(0) The board has 193 lines, and every line has exactly two parametrizations.
(1) Lemma 9.1 (b), (c): every line of 7^3 is either contained in M or meets M in exactly one cell, its
    middle cell f(3), and then its other six cells form a line of the 6^3 array A; every line of A arises
    exactly once.  Exactly 45 lines are contained in M.
(2) The 45 lines contained in M admit a pairing inside M (two private cells per line).  One such pairing is
    found by bipartite matching and printed.  This shows existence only; it is NOT the pairing of the paper.
(3) The table of Appendix B as printed: it is parsed from the LaTeX source paper/app_seven.tex and checked:
    45 entries; the 45 lines are pairwise different and are exactly the lines contained in M (Lemma 9.1 (d));
    each of the two cells of an entry lies on its line; the 90 cells are pairwise different (Lemma 9.2);
    the centre and 36 further cells of M are not assigned; and the rule stated in the proof of Lemma 9.2
    reproduces every entry.

Exact integer arithmetic only.  Exit code 0 iff all checks hold.
Usage: check_7cube_midplanes.py [path/to/app_seven.tex]     (default: ../paper/app_seven.tex)"""
import itertools, os, re, sys

n = 7


def parametrizations(n):
    """all triples of components: a constant v < n, the identity (coded n) or the reversal (coded n+1),
    not all constant"""
    for f in itertools.product(range(n + 2), repeat=3):
        if not all(v < n for v in f):
            yield f


def cells_of(f, n):
    ev = lambda v, t: v if v < n else (t if v == n else n - 1 - t)
    return [tuple(ev(v, t) for v in f) for t in range(n)]


def lines(n):
    par = {}
    for f in parametrizations(n):
        par.setdefault(frozenset(cells_of(f, n)), []).append(f)
    return par


# ---- (0) -------------------------------------------------------------------------------------------------
P7 = lines(7)
assert len(P7) == 193 and all(len(v) == 2 for v in P7.values())
assert sum(len(v) for v in P7.values()) == 9 ** 3 - 7 ** 3 == 386
L7 = sorted(sorted(l) for l in P7)
P6 = lines(6)
assert len(P6) == 148 and all(len(v) == 2 for v in P6.values())
L6 = set(P6)
print("lines of 7^3:", len(L7), "(two parametrizations each);  lines of 6^3:", len(L6))

# ---- (1) -------------------------------------------------------------------------------------------------
relabel = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}
inM = lambda c: 3 in c
Mlines, seen6 = [], set()
for l in L7:
    rest = [c for c in l if not inM(c)]
    if not rest:
        # contained in M: some parametrization has a constant component 3 (Lemma 9.1 (a) and its converse)
        assert all(any(v == 3 for v in f) for f in P7[frozenset(l)])
        Mlines.append(l)
        continue
    assert len(rest) == 6, (l, rest)
    # the only cell in M is f(3) for both parametrizations
    for f in P7[frozenset(l)]:
        assert [c for c in l if inM(c)] == [cells_of(f, 7)[3]]
    img = frozenset(tuple(relabel[v] for v in c) for c in rest)
    assert img in L6 and img not in seen6
    seen6.add(img)
assert len(seen6) == 148 and len(Mlines) == 45
Mcells = [c for c in itertools.product(range(7), repeat=3) if inM(c)]
assert len(Mcells) == 127
print("contained in M:", len(Mlines), " meeting A in a 6^3 line (each line of 6^3 exactly once):", len(seen6),
      " cells of M:", len(Mcells))

# ---- (2) -------------------------------------------------------------------------------------------------
slots = [(i, s) for i in range(len(Mlines)) for s in (0, 1)]
owner = {}


def aug(slot, vis):
    for c in Mlines[slot[0]]:
        if c in vis:
            continue
        vis.add(c)
        if c not in owner or aug(owner[c], vis):
            owner[c] = slot
            return True
    return False


ok = all(aug(s, set()) for s in slots)
print("a pairing of the", len(Mlines), "lines inside M exists (bipartite matching):", ok)
if ok:
    pairs = {}
    for c, (i, s) in owner.items():
        pairs.setdefault(i, []).append(c)
    assert all(len(v) == 2 for v in pairs.values()) and len(pairs) == len(Mlines)
    assert len(set(owner)) == 2 * len(Mlines)
    print("  (one such pairing, not the one of the paper:)")
    for i in sorted(pairs):
        print("  line", Mlines[i][0], "..", Mlines[i][-1], " cells", sorted(pairs[i]))

# ---- (3) the printed table of Appendix B --------------------------------------------------------------------
tex = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                                                         "paper", "app_seven.tex")
if not os.path.exists(tex):
    print("TABLE NOT CHECKED: cannot find", tex)
    sys.exit(2)
src = open(tex).read()
a = src.index(r"\begin{tabular}")
b = src.index(r"\end{tabular}")
body = src[a:b]
comp = r"(\d|t|6\{-\}t)"
entry = re.compile(r"\$\(" + comp + "," + comp + "," + comp + r"\)\$\s*&\s*\$\((\d),(\d),(\d)\),\\ \((\d),(\d),(\d)\)\$")
table = []
for m in entry.finditer(body):
    f = m.group(1, 2, 3)
    line = frozenset(tuple(int(v) if v.isdigit() else (t if v == "t" else 6 - t) for v in f) for t in range(7))
    c1 = tuple(int(x) for x in m.group(4, 5, 6))
    c2 = tuple(int(x) for x in m.group(7, 8, 9))
    table.append((f, line, c1, c2))
print("table of Appendix B (%s): %d entries parsed" % (os.path.relpath(tex), len(table)))
tab_ok = True


def check(cond, what):
    global tab_ok
    print("  %-78s %s" % (what, "ok" if cond else "FAILED"))
    tab_ok = tab_ok and bool(cond)


check(len(table) == 45, "45 entries")
check(all(len(l) == 7 and l in P7 for _, l, _, _ in table), "every entry is a line of the 7^3 board")
check(len({l for _, l, _, _ in table}) == len(table), "the lines are pairwise different")
check(all(all(inM(c) for c in l) for _, l, _, _ in table), "every line is contained in M")
check({l for _, l, _, _ in table} == {frozenset(l) for l in Mlines},
      "the lines are exactly the lines contained in M (Lemma 9.1 (d))")
check(all(c1 in l and c2 in l and c1 != c2 for _, l, c1, c2 in table), "both cells of every entry lie on its line")
allc = [c for _, _, c1, c2 in table for c in (c1, c2)]
check(len(allc) == 90 and len(set(allc)) == 90, "the 90 cells are pairwise different (Lemma 9.2)")
free = [c for c in Mcells if c not in set(allc)]
check(len(free) == 37 and (3, 3, 3) in free, "37 cells of M are not assigned, among them the centre")
# the rule in the proof of Lemma 9.2: psi = inverse of the relabelling, indices modulo 6
psi = [0, 1, 2, 4, 5, 6]
rule = {}
for pos in range(3):  # position of the coordinate 3: plane z = 3 (pos 2), y = 3 (pos 1), x = 3 (pos 0)
    def put(p, q, pos=pos):  # the cell with coordinates p, q in the plane, in this order
        c = [p, q]
        c.insert(pos, 3)
        return tuple(c)
    for r in range(6):
        rule[frozenset(put(psi[r], t) for t in range(7))] = {put(psi[r], psi[(r + 1) % 6]), put(psi[r], psi[(r + 2) % 6])}
        rule[frozenset(put(t, psi[r]) for t in range(7))] = {put(psi[(r + 1) % 6], psi[r]), put(psi[(r + 2) % 6], psi[r])}
    rule[frozenset(put(t, t) for t in range(7))] = {put(0, 0), put(6, 6)}
    rule[frozenset(put(t, 6 - t) for t in range(7))] = {put(1, 5), put(5, 1)}
for pos in range(3):  # the three lines through the centre: the cells with t = 0 and t = 1
    def axis(t, pos=pos):
        c = [3, 3, 3]
        c[pos] = t
        return tuple(c)
    rule[frozenset(axis(t) for t in range(7))] = {axis(0), axis(1)}
check(len(rule) == 45 and all(rule.get(l) == {c1, c2} for _, l, c1, c2 in table),
      "the rule in the proof of Lemma 9.2 reproduces every entry")
print("RESULT:", "all checks passed" if (ok and tab_ok) else "FAILED")
sys.exit(0 if (ok and tab_ok) else 1)

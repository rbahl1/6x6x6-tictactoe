#!/usr/bin/env python3
# Referee 1b: brute-force checks of
#  (1) Prop. C.3(a): any four pairwise meeting lines of the 6^3 board are concurrent;
#  (2) Prop. 9.5: seven cells meet at most 46 lines (exact maximum over busy 7-sets; bound for the rest);
#  (3) Lemma 9.1 (mid-plane decomposition of the 7^3 board) and Lemma 9.2 / Table 13 (pairing of the 45
#      lines in M), with the table parsed from paper/app_seven.tex.
import itertools, re, sys
from b_geom import Board6, lines_of, cid

b = Board6()
L = b.lines
meet = b.meet

# ---------- (1) four pairwise meeting lines ----------
adj = [set(j for j in range(148) if meet[i][j] >= 0) for i in range(148)]
n4 = 0
nonconc = 0
ntri = 0
for a in range(148):
    for c in adj[a]:
        if c <= a:
            continue
        for d in adj[a] & adj[c]:
            if d <= c:
                continue
            if len(L[a] & L[c] & L[d]) == 0:
                ntri += 1
            for e in adj[a] & adj[c] & adj[d]:
                if e <= d:
                    continue
                n4 += 1
                if len(L[a] & L[c] & L[d] & L[e]) != 1:
                    nonconc += 1
print('(1) 4-sets of pairwise meeting lines:', n4, ' non-concurrent:', nonconc,
      ' [triangles (3 pairwise meeting, not concurrent):', ntri, ']')
assert nonconc == 0
# expected number of concurrent 4-sets: sum over cells of C(deg,4)
from math import comb
assert n4 == sum(comb(len(t), 4) for t in b.through)

# ---------- (2) Prop 9.5 ----------
best = 0
busy = b.busy
for S in itertools.combinations(busy, 7):
    dead = set()
    for c in S:
        dead.update(b.through[c])
    if len(dead) > best:
        best = len(dead)
print('(2) max number of lines met by 7 busy cells:', best, '(paper bound: 46; needed for slack >= 7: 47)')
assert best <= 46
# with at least one non-busy cell the bound 6*7+4 = 46 is immediate from the degrees (7,4,3).
# For completeness: best value with exactly 6 busy cells + 1 cell of degree 4
best6 = 0
for S in itertools.combinations(busy, 6):
    dead = set()
    for c in S:
        dead.update(b.through[c])
    best6 = max(best6, len(dead))
print('    max number of lines met by 6 busy cells:', best6, '-> with a 7th non-busy cell at most', best6 + 4)
assert best6 + 4 <= 46

# ---------- (3) 7^3 board ----------
L7, P7 = lines_of(7)
assert len(L7) == 193
inM = lambda c: 3 in c
M = [c for c in itertools.product(range(7), repeat=3) if inM(c)]
A = [c for c in itertools.product(range(7), repeat=3) if not inM(c)]
assert len(M) == 127 and len(A) == 216
phi = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}
Phi = lambda c: tuple(phi[v] for v in c)
L6 = set(frozenset(l) for l in lines_of(6)[0])
linesM = [l for l in L7 if all(inM(c) for c in l)]
others = [l for l in L7 if not all(inM(c) for c in l)]
print('(3) lines of 7^3 inside M:', len(linesM), ' others:', len(others))
assert len(linesM) == 45 and len(others) == 148
img = set()
for l in others:
    mid = [c for c in l if inM(c)]
    assert len(mid) == 1                       # Lemma 9.1(b): exactly one cell in M
    im = frozenset(Phi(c) for c in l if not inM(c))
    assert im in L6                            # ... and the rest is a line of 6^3
    img.add(im)
assert img == L6 and len(img) == 148           # Lemma 9.1(c): bijection
# each line in M has a parametrization with a constant component 3 (Lemma 9.1(a),(d))
for l in linesM:
    assert any(any(k == 'c' and v == 3 for k, v in f) for f in P7[l])
for l in others:
    assert not any(any(k == 'c' and v == 3 for k, v in f) for f in P7[l])

# parse Table 13
tex = open('/home/user/ttt6/paper/app_seven.tex').read()
body = tex[tex.index('\\midrule') : tex.index('\\bottomrule')]
rows = [r for r in body.split('\\\\') if '$' in r]
entries = []
for r in rows:
    cols = [c.strip() for c in r.split('&')]
    assert len(cols) == 6, cols
    for k in range(3):
        entries.append((cols[2 * k], cols[2 * k + 1]))
assert len(entries) == 45, len(entries)


def parse_line(s):
    s = s.replace('$', '').replace('{-}', '-').replace(' ', '')
    s = s[s.index('(') + 1 : s.rindex(')')]
    comps = s.split(',')
    assert len(comps) == 3
    cells = []
    for t in range(7):
        c = []
        for comp in comps:
            if comp == 't':
                c.append(t)
            elif comp == '6-t':
                c.append(6 - t)
            else:
                c.append(int(comp))
        cells.append(tuple(c))
    return frozenset(cells)


def parse_cells(s):
    return [tuple(int(v) for v in m) for m in re.findall(r'\((\d),(\d),(\d)\)', s)]


table = {}
allcells = []
for ls, cs in entries:
    l = parse_line(ls)
    cells = parse_cells(cs)
    assert len(cells) == 2 and cells[0] != cells[1], (ls, cs)
    assert l in set(linesM), ('not a line inside M', ls)
    assert l not in table, ('line listed twice', ls)
    for c in cells:
        assert c in l, ('cell not on its line', ls, c)
        assert inM(c)
    table[l] = cells
    allcells += cells
assert set(table.keys()) == set(linesM)
assert len(allcells) == 90 and len(set(allcells)) == 90
unassigned = set(M) - set(allcells)
print('    Table 13: 45 distinct lines = all lines inside M; 90 distinct cells, each on its line; unassigned cells of M:',
      len(unassigned), ' centre unassigned:', (3, 3, 3) in unassigned)
assert len(unassigned) == 37 and (3, 3, 3) in unassigned

# the rule stated in the proof of Lemma 9.2 (plane z = 3)
psi = {v: k for k, v in phi.items()}
for r in range(6):
    l = frozenset((psi[r], t, 3) for t in range(7))
    assert set(table[l]) == {(psi[r], psi[(r + 1) % 6], 3), (psi[r], psi[(r + 2) % 6], 3)}
    l = frozenset((t, psi[r], 3) for t in range(7))
    assert set(table[l]) == {(psi[(r + 1) % 6], psi[r], 3), (psi[(r + 2) % 6], psi[r], 3)}
assert set(table[frozenset((t, t, 3) for t in range(7))]) == {(0, 0, 3), (6, 6, 3)}
assert set(table[frozenset((t, 6 - t, 3) for t in range(7))]) == {(1, 5, 3), (5, 1, 3)}
# planes y = 3 and x = 3 are obtained by moving the coordinate 3
for l, cells in list(table.items()):
    if all(c[2] == 3 for c in l) and not all(c[0] == 3 for c in l) and not all(c[1] == 3 for c in l):
        ly = frozenset((c[0], 3, c[1]) for c in l)
        lx = frozenset((3, c[0], c[1]) for c in l)
        assert set(table[ly]) == {(c[0], 3, c[1]) for c in cells}
        assert set(table[lx]) == {(3, c[0], c[1]) for c in cells}
print('    rule in the proof of Lemma 9.2 agrees with the table: OK')
import pickle
pickle.dump({'table': {tuple(sorted(l)): cells for l, cells in table.items()}},
            open('/home/user/ttt6/logs/paper/referee/b_seven_table.pkl', 'wb'))
print('ALL CHECKS PASSED')

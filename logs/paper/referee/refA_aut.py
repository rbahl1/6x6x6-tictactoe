# refA_aut.py -- referee 1a: Computed Fact 2.9.  Counts ALL automorphisms of the incidence structure
# (216 cells, 148 lines) that fix the cell (0,0,0), by exhaustive backtracking with domain propagation
# (own code).  Since automorphisms preserve the number of lines through a cell and G is transitive on
# the 24 busy cells, |Aut| = 24 * (number found).
import sys, time
from refA_geom import *

sys.setrecursionlimit(10000)
t0 = time.time()
LM = [sum(1 << c for c in l) for l in LINE_IDS]
lineof = {}
for li, l in enumerate(LINE_IDS):
    for a in l:
        for b in l:
            if a != b:
                lineof[(a, b)] = li
COL = [0] * 216
for (a, b) in lineof:
    COL[a] |= 1 << b
ALL = (1 << 216) - 1
deg = [len(THROUGH[c]) for c in range(216)]
DEGM = {d: sum(1 << c for c in range(216) if deg[c] == d) for d in (3, 4, 7)}
lineset = set(LINE_IDS)

count = 0
nodes = 0
sols = []

def search(phi, dom, lmap, used):
    global count, nodes
    nodes += 1
    # choose the unassigned cell with the smallest domain
    best = None; bs = 10**9
    for c in range(216):
        if phi[c] < 0:
            n = dom[c].bit_count()
            if n == 0:
                return
            if n < bs:
                bs, best = n, c
                if n == 1:
                    break
    if best is None:
        # complete map: verify from the definition
        assert sorted(phi) == list(range(216))
        for l in LINE_IDS:
            if frozenset(phi[c] for c in l) not in lineset:
                return
        count += 1
        if len(sols) < 2000:
            sols.append(tuple(phi))
        return
    c = best
    m = dom[c]
    while m:
        bit = m & -m
        m ^= bit
        d = bit.bit_length() - 1
        phi2 = phi[:]; dom2 = dom[:]; lmap2 = dict(lmap)
        phi2[c] = d
        ok = True
        nb = ~bit
        cd = COL[d]; ncd = ~cd
        cc = COL[c]
        for c2 in range(216):
            if phi2[c2] < 0:
                dom2[c2] &= (cd if (cc >> c2) & 1 else ncd) & nb
        # line images
        for c1 in range(216):
            if phi2[c1] >= 0 and c1 != c and (cc >> c1) & 1:
                l = lineof[(c, c1)]
                key = (d, phi2[c1])
                if key not in lineof:
                    ok = False; break
                ml = lineof[key]
                if l in lmap2:
                    if lmap2[l] != ml:
                        ok = False; break
                else:
                    if ml in lmap2.values():
                        ok = False; break
                    lmap2[l] = ml
                    mm = LM[ml]; nmm = ~mm
                    lm = LM[l]
                    for c2 in range(216):
                        if phi2[c2] < 0:
                            dom2[c2] &= mm if (lm >> c2) & 1 else nmm
                        elif ((lm >> c2) & 1) != ((mm >> phi2[c2]) & 1):
                            ok = False
                    if not ok:
                        break
        if ok:
            search(phi2, dom2, lmap2, used | bit)

phi = [-1] * 216
dom = [DEGM[deg[c]] for c in range(216)]
# fix (0,0,0)
phi0 = phi[:];
dom0 = dom[:]
c, d = 0, 0
phi0[c] = d
for c2 in range(216):
    if phi0[c2] < 0:
        dom0[c2] &= (COL[d] if (COL[c] >> c2) & 1 else ~COL[d]) & ~(1 << d)
search(phi0, dom0, {}, 1)
print('automorphisms fixing (0,0,0):', count, ' search nodes:', nodes, ' (%.1f s)' % (time.time() - t0))
G = group_G()
inG = sum(1 for s in sols if s in G)
stabG = sum(1 for g in G if g[0] == 0)
print('of these in G:', inG, ' ; elements of G fixing (0,0,0):', stabG)
print('|Aut| = 24 *', count, '=', 24 * count, ' (paper: 1,152 = |G|)')
assert count == 48 and inG == 48 and stabG == 48
print('AUT OK')

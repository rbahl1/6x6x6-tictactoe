#!/usr/bin/env python3
# Referee audit A3: follow the strategy table from the root (Section 6.6, Lines A and B),
# with an own geometry and brute-force canonical forms.  Also slack values and Table 5 orbit data.
import itertools, sys
ROOT = '/home/user/ttt6/'
def cid(x, y, z): return 36 * x + 6 * y + z
def co(c): return (c // 36, (c // 6) % 6, c % 6)
def lam(i): return min(i, 5 - i)
def busy(c):
    x, y, z = co(c); return lam(x) == lam(y) == lam(z)
# lines
LN = set()
for f in itertools.product(range(8), repeat=3):
    if all(v < 6 for v in f): continue
    LN.add(frozenset(cid(*[(v if v < 6 else (t if v == 6 else 5 - t)) for v in f]) for t in range(6)))
LN = sorted(LN, key=sorted); assert len(LN) == 148
# group
H = [s for s in itertools.permutations(range(6)) if all(s[5 - i] == 5 - s[i] for i in range(6))]
assert len(H) == 48
G = set()
for pi in itertools.permutations(range(3)):
    for r in range(8):
        for s in H:
            g = []
            for c in range(216):
                x = co(c); d = [0, 0, 0]
                for i in range(3): d[pi[i]] = x[i]
                for i in range(3):
                    if r >> i & 1: d[i] = 5 - d[i]
                g.append(cid(*[s[v] for v in d]))
            G.add(tuple(g))
G = sorted(G); assert len(G) == 1152
def canon_maps(B, F):
    best = None; maps = []
    for g in G:
        k = (sorted(g[c] for c in B), sorted(g[c] for c in F))
        if best is None or k < best: best = k; maps = [g]
        elif k == best: maps.append(g)
    return best, maps
def sstr(v): return ','.join(map(str, v)) if v else '-'
tbl = {}
for l in open(ROOT + 'results/final/strategy.tbl'):
    t, b, f, m = l.split(); tbl[(int(t), b, f)] = int(m)
lv = set(tuple(l.split()) for l in open(ROOT + 'results/final/leaves.txt'))
def moves(B, F, t):
    (cb, cf), maps = canon_maps(B, F)
    mc = tbl[(t, sstr(cb), sstr(cf))]
    return sorted(set(g.index(mc) for g in maps)), (cb, cf, mc)
def is_leaf(B, F, t):
    (cb, cf), _ = canon_maps(B, F)
    return (str(t), sstr(cb), sstr(cf)) in lv, (cb, cf)
def live(B): return [l for l in LN if not (l & set(B))]
def slack(B): return 216 - len(B) - 2 * len(live(B))

print('=== Line A (always child (i))')
claimA = [(0,0,0),(5,5,5),(1,1,4),(4,4,1),(2,3,2),(3,2,3),(0,5,5),(5,0,0)]
B = []; F = []; ok = True
for t in range(1, 11):
    ms, (cb, cf, mc) = moves(B, F, t)
    want = cid(*claimA[t - 1]) if t <= len(claimA) else None
    print(' round', t, 'possible real moves', [co(m) for m in ms], 'claimed', claimA[t - 1] if want is not None else None, 'OK' if want in ms else 'MISMATCH')
    if want not in ms: ok = False; break
    B.append(want)
    lf, (cb, cf) = is_leaf(B, F, t)
    if lf:
        print(' leaf after round', t, ': "%d %s %s"' % (t, sstr(cb), sstr(cf)), ' slack', slack(B), ' B is T8:', sorted(B) == sorted([0,35,46,92,123,169,180,215]))
        break
print(' Line A consistent:', ok)

print('=== Line B (child (ii) then child (i) in every round)')
claimB = [(1,1,1),(5,0,0),(0,0,5),(0,5,0),(1,4,4),(4,4,1),(3,3,2),(2,3,3),(2,2,2)]
anti = lambda c: tuple(5 - v for v in c)
B = []; F = []; ok = True
for t in range(1, 11):
    ms, _ = moves(B, F, t)
    tried = (0,0,0) if t == 1 else anti(claimB[t - 2])
    print(' round', t, 'first wanted cell(s)', [co(m) for m in ms], 'claimed', tried, 'OK' if cid(*tried) in ms else 'MISMATCH')
    if cid(*tried) not in ms: ok = False; break
    F.append(cid(*tried))
    assert len(F) == t
    ms, _ = moves(B, F, t)
    want = claimB[t - 1]
    print('          second cell(s)', [co(m) for m in ms], 'claimed', want, 'OK' if cid(*want) in ms else 'MISMATCH')
    if cid(*want) not in ms: ok = False; break
    B.append(cid(*want))
    lf, (cb, cf) = is_leaf(B, F, t)
    if lf:
        last = open(ROOT + 'results/final/leaves.txt').read().splitlines()[-1]
        print(' leaf after round', t, ': "%d %s %s"' % (t, sstr(cb), sstr(cf)), ' slack', slack(B), ' theta', t - len(F), ' is last line of leaf file:', last == '%d %s %s' % (t, sstr(cb), sstr(cf)))
        print(' real B ids', sorted(B), 'real F ids', sorted(F))
        break
print(' Line B consistent:', ok)

print('=== slack / live lines of T7, T8, O1..O6')
T7 = [cid(*c) for c in [(0,0,0),(5,5,5),(5,0,0),(1,1,4),(4,4,1),(2,3,2),(3,2,3)]]
T8 = T7 + [cid(0,5,5)]
print(' T7 ids', sorted(T7), 'live', len(live(T7)), 'slack', slack(T7))
print(' T8 ids', sorted(T8), 'live', len(live(T8)), 'slack', slack(T8))
O = ['0,5,43,61,93,122,154,210','0,5,43,61,93,122,154,215','0,5,43,61,93,122,172,215','0,5,43,92,93,122,172,210','0,5,43,92,93,122,172,215','0,5,61,93,122,154,210,215']
tot = 0
for i, o in enumerate(O):
    S = [int(v) for v in o.split(',')]
    (cb, _), maps = canon_maps(S, [])
    orb = 1152 // len(maps); tot += orb
    npairs = sum(1 for c in S if (215 - c) in S) // 2
    print(' O%d' % (i + 1), [co(c) for c in S], 'canonical:', cb == S, 'orbit', orb, 'live', len(live(S)), 'slack', slack(S), 'antipodal pairs', npairs)
print(' total sets in six orbits', tot)
print(' canon(T8) == O6:', canon_maps(T8, [])[0][0] == [int(v) for v in O[5].split(',')])
print(' canon(T7):', canon_maps(T7, [])[0][0], [co(c) for c in canon_maps(T7, [])[0][0]])

# IP size for B = T8 (Appendix B.1)
L8 = live(T8); cells = [c for c in range(216) if c not in T8]
n9 = sum(len(l) for l in L8); n14 = 0
for c in cells:
    k = sum(1 for l in L8 if c in l); n14 += k * (k - 1) // 2
print('=== IP for T8: variables', 2 * len(L8) + 2 * len(cells), 'constraints (9)', n9, '(10)', len(cells), '(11)', 1, '(12)', len(L8), '(13)', len(L8), '(14)', n14, 'total', n9 + len(cells) + 1 + 2 * len(L8) + n14)

# face obstruction of Section 6.5
Fc = [cid(5,0,0), cid(5,0,5), cid(5,5,0), cid(5,5,5)]
S = [l for l in LN if all(co(c)[0] == 5 for c in l)]
N = set().union(*S)
print('=== face x=5: lines in face', len(S), 'cells', len(N), 'sur_F', len(N - set(Fc)) - 2 * len(S), 'busy cells in face', [co(c) for c in N if busy(c)])
# lines meeting a face in >=2 cells
print('=== max stones: six stones kill at most', 'see minstones')

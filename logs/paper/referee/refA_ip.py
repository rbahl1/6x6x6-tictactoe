# refA_ip.py -- referee 1a: Appendix B, Proposition B.1.  The 0-1 program (8)-(14) is built here
# directly from the printed constraints (own code, own geometry) and compared with a brute-force
# evaluation of worst(B,F,k) on small instances; model sizes for T8 are recounted.
import itertools, random, sys, time
import numpy as np
import highspy
from refA_geom import *
from refA_lib import *
from refA_small import *

def say(*a):
    print(' '.join(str(x) for x in a)); sys.stdout.flush()

def build(Bm, Fm, k, danger=True):
    L = live(Bm)
    cells = [c for c in range(216) if not (Bm >> c & 1)]
    var = {}
    def v(name):
        var.setdefault(name, len(var)); return var[name]
    for l in L:
        v(('s', l)); v(('q', l))
    for c in cells:
        v(('n', c)); v(('x', c))
    rows = []   # (lb, ub, {var: coef}, type)
    INF = highspy.kHighsInf
    for l in L:
        for c in LINE_IDS[l]:
            rows.append((0, INF, {v(('n', c)): 1, v(('s', l)): -1}, 'n'))            # (9)
    for c in cells:
        rows.append((-INF, 0, {v(('x', c)): 1, v(('n', c)): -1}, 'xn'))              # (10)
    rows.append((-INF, k, {v(('x', c)): 1 for c in cells}, 'k'))                     # (11)
    for l in L:
        rows.append((-INF, 4, {v(('x', c)): 1 for c in LINE_IDS[l]}, 'cap'))         # (12)
    if danger:
        for l in L:
            d = {v(('x', c)): -1 for c in LINE_IDS[l]}; d[v(('q', l))] = 1
            rows.append((-3, INF, d, 'q'))                                           # (13)
        for c in cells:
            ls = [l for l in THROUGH[c] if l in set(L)]
            for a, b in itertools.combinations(ls, 2):
                rows.append((-INF, 1, {v(('q', a)): 1, v(('q', b)): 1, v(('x', c)): -1}, 'danger'))   # (14)
    fixed = [v(('x', c)) for c in bits(Fm)] if not (Fm & Bm) else None
    obj = {}
    for l in L:
        obj[v(('s', l))] = 2
    for c in cells:
        obj[v(('n', c))] = -1; obj[v(('x', c))] = 1
    return var, rows, fixed, obj

def solve(Bm, Fm, k, danger=True):
    var, rows, fixed, obj = build(Bm, Fm, k, danger)
    h = highspy.Highs()
    h.setOptionValue('output_flag', False)
    h.setOptionValue('mip_rel_gap', 0.0)
    h.setOptionValue('mip_abs_gap', 0.99)
    n = len(var)
    lb = np.zeros(n); ub = np.ones(n)
    for i in fixed:
        lb[i] = 1
    for i in range(n):
        h.addVar(lb[i], ub[i])
        h.changeColIntegrality(i, highspy.HighsVarType.kInteger)
        h.changeColCost(i, float(obj.get(i, 0)))
    for lo, up, d, _ in rows:
        idx = np.array(list(d.keys()), dtype=np.int32); val = np.array(list(d.values()), dtype=np.float64)
        h.addRow(lo, up, len(idx), idx, val)
    h.changeObjectiveSense(highspy.ObjSense.kMaximize)
    h.run()
    ms = h.getModelStatus()
    if ms == highspy.HighsModelStatus.kInfeasible:
        return None
    assert ms == highspy.HighsModelStatus.kOptimal, ms
    return round(h.getInfo().objective_function_value)

t0 = time.time()
T7 = [cid(0,0,0), cid(5,5,5), cid(5,0,0), cid(1,1,4), cid(4,4,1), cid(2,3,2), cid(3,2,3)]
T8 = T7 + [cid(0,5,5)]
var, rows, fixed, obj = build(mask(T8), 0, 8)
kinds = {}
for r in rows:
    kinds[r[3]] = kinds.get(r[3], 0) + 1
say('T8 model: variables', len(var), ' constraints', len(rows), kinds, ' (paper: 616 variables, 1,605 constraints, 596 of type (14))')

# small instances: IP optimum == brute-force worst ; feasibility <=> F admissible and |F| <= k
rng = random.Random(31)
st = dict(n=0, infeasible=0, equal=0, diff=0, pos=0)
while st['n'] < 150 and time.time() - t0 < 120:
    inst = random_small(rng, (3, 5), (0, 2), 7)
    if len(inst.cells) > 26:
        continue
    Fm = random_F(rng, inst, 5, admissible_only=(rng.random() < 0.8))
    nF = popcount(Fm)
    k = nF + rng.randint(-1 if nF else 0, 3)
    want = inst.worst(Fm, k) if (inst.adm(Fm) and nF <= k) else None
    got = solve(inst.Bm, Fm, k)
    st['n'] += 1
    if want is None:
        st['infeasible'] += 1
    if got == want:
        st['equal'] += 1
        if want:
            st['pos'] += 1
    else:
        st['diff'] += 1
        say('DIFF', bits(FULL & ~inst.Bm), bits(Fm), k, 'brute', want, 'ip', got)
say('Prop B.1 on small instances:', st)
assert st['diff'] == 0
# dense instances with large admissible F: worst > 0 occurs
st = dict(n=0, equal=0, diff=0, pos=0, hist={})
tt = time.time()
while st['n'] < 200 and time.time() - tt < 150:
    inst = dense_small(rng, (4, 8), 10)
    Fm = random_adm_F(rng, inst, rng.randint(0, 14))
    nF = popcount(Fm)
    k = nF + rng.randint(0, 3)
    want = inst.worst(Fm, k)
    got = solve(inst.Bm, Fm, k)
    st['n'] += 1
    st['hist'][want] = st['hist'].get(want, 0) + 1
    if got == want:
        st['equal'] += 1
        st['pos'] += want > 0
    else:
        st['diff'] += 1
        say('DIFF', bits(FULL & ~inst.Bm), bits(Fm), k, 'brute', want, 'ip', got)
    # without (13),(14) the optimum is the maximum over sets that only respect the cap of four
    if st['n'] % 10 == 0:
        best = None
        cand = [c for c in inst.cells if not (Fm >> c & 1)]
        capok = lambda M: all(popcount(LMASK[l] & M) <= 4 for l in inst.L)
        if capok(Fm):
            stack = [(Fm, 0, nF)]
            while stack:
                Xm, start, size = stack.pop()
                d = inst.delta(Xm)
                best = d if best is None or d > best else best
                if size < k:
                    for i in range(start, len(cand)):
                        Y = Xm | (1 << cand[i])
                        if capok(Y):
                            stack.append((Y, i + 1, size + 1))
        g2 = solve(inst.Bm, Fm, k, danger=False)
        assert g2 == best, (g2, best)
say('Prop B.1 on dense instances:', st)
assert st['diff'] == 0
# two values of Table 12 that are cheap: worst on the empty board for k = 0, 1 (text of B.3)
say('worst(0,0,0) =', solve(0, 0, 0), ' worst(0,0,1) =', solve(0, 0, 1), ' (paper: 80, 81)')
say('worst(T7,0,7) =', solve(mask(T7), 0, 7), '(paper: 6);  worst(T8,0,9) =', solve(mask(T8), 0, 9), '(paper: 1)')
say('IP ALL OK  (%.1f s)' % (time.time() - t0))

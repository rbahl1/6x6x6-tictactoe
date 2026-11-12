#!/usr/bin/env python3
# Referee 1b: Algorithm 1 of the paper transcribed literally from the printed pseudocode and from
# Definitions 5.1 / 5.6 (own geometry, own canonical form), run on the two certificate files.
# The leaf predicate LEAF is NOT evaluated here (it belongs to Section 4 / Algorithm 2); for listed leaves
# only (L1) "F is B-admissible" is checked, and (L2) "a pairing for (B,F) exists" on a random sample.
# Purpose: (i) check that the printed algorithm / definitions reproduce the state counts printed in
# Appendix A.5 and Table 3, (ii) check (V2) "t < Rmax" and (V3) on the whole reachable set,
# (iii) check the shape claims and "child (a) / child (i) are never VOID", (iv) LOST false (t <= 9).
import sys, time, random
from collections import defaultdict
from b_sigma import BD, LSET, LINES, THROUGH, canon, load_certificate, admissible, find_pairing

RMAX = 10
tbl, lv = load_certificate()
MEET = BD.meet
PAIRS_AT = [[] for _ in range(216)]          # unordered pairs of distinct lines through a cell
for c in range(216):
    th = THROUGH[c]
    for i in range(len(th)):
        for j in range(i + 1, len(th)):
            PAIRS_AT[c].append((th[i], th[j]))

seen = set()
work = []
cnt = defaultdict(lambda: defaultdict(int))


def push(kind, B, F, t, tag, parent_t):
    """Push(s'): discard VOID children, canonicalize, add to Seen / W if new."""
    assert len(F) <= t and not (B & F)
    assert len(B) == (t if kind == 'P' else t - 1)                 # shape
    if not admissible(B, F):
        cnt['void_' + tag][parent_t] += 1
        return False
    Bc, Fc, _ = canon(B, F)
    s = (kind, t, Bc, Fc)
    if s not in seen:
        seen.add(s)
        work.append(s)
    return True


root = ('P', 0, (), ())
seen.add(root); work.append(root)
t0 = time.time()
used_tbl, used_lv = set(), set()
while work:
    kind, t, Bc, Fc = work.pop()
    B, F = frozenset(Bc), frozenset(Fc)
    if kind == 'P':
        cnt['P'][t] += 1
        if (t, Bc, Fc) in lv:
            cnt['leaf'][t] += 1
            cnt['leaf_theta'][t - len(F)] += 1
            used_lv.add((t, Bc, Fc))
            assert admissible(B, F)                                 # (L1)
            continue
        assert t < RMAX, 'REJECT: expanded P-state with t >= Rmax'
        assert t + 1 <= 10                                          # LOST false by Prop. 5.9(c)
        cnt['expanded'][t] += 1
        ok = push('Q', B, F, t + 1, 'a', t)                        # child (a)
        assert ok, 'child (a) VOID'
        live = [l for l in range(148) if not (LSET[l] & B)]
        fc = {l: len(LSET[l] & F) for l in live}
        # children (c)
        ch = set()
        for l in live:
            for e in LINES[l]:
                if e in F:
                    continue
                newF = F | (LSET[l] - {e})
                if len(newF) <= t + 1:
                    assert len(newF) == len(F) + 5 - fc[l]          # the "identity" of Section 7.2
                    ch.add((B | {e}, newF))
        for (B2, F2) in ch:
            if push('P', B2, F2, t + 1, 'c', t):
                cnt['threat_children'][t + 1] += 1
        # children (d)
        ch = set()
        u = t + 1 - len(F)
        for c in range(216):
            if c in B or c in F:
                continue
            for (a, b) in PAIRS_AT[c]:
                if a not in fc or b not in fc:
                    continue                                        # both lines must be live
                if (4 - fc[a]) + (4 - fc[b]) > u:
                    # identity |F u Qa u Qb| = |F| + (4-|F&a|) + (4-|F&b|); verified below when it is used
                    # (here it only prunes; to be literal we still verify it on one instance)
                    continue
                for a1 in LINES[a]:
                    if a1 == c or a1 in F:
                        continue
                    Qa = LSET[a] - {c, a1}
                    for b1 in LINES[b]:
                        if b1 == c or b1 in F:
                            continue
                        Qb = LSET[b] - {c, b1}
                        newF = F | Qa | Qb
                        assert len(newF) == len(F) + (4 - fc[a]) + (4 - fc[b])
                        if len(newF) <= t + 1:
                            ch.add((B | {c}, newF))
        for (B2, F2) in ch:
            if push('P', B2, F2, t + 1, 'd', t):
                cnt['danger_children'][t + 1] += 1
    else:
        cnt['Q'][t] += 1
        key = (t, Bc, Fc)
        assert key in tbl, 'REJECT: no table entry'
        used_tbl.add(key)
        m = tbl[key]
        assert 0 <= m < 216 and m not in B and m not in F, 'REJECT: illegal table move'
        if not (len(LSET[0]) and m in BD.busy):
            cnt['nonbusy_table_moves'][t] += 1
        ok = push('P', B | {m}, F, t, 'i', t)                      # child (i)
        assert ok, 'child (i) VOID'
        if len(F) < t:
            cnt['children_ii'][t] += 1
            push('Q', B, F | {m}, t, 'ii', t)                      # child (ii)

print('expansion finished in %.1f s' % (time.time() - t0))
tot = lambda k: sum(cnt[k].values())
row = lambda k: [cnt[k].get(t, 0) for t in range(11)]
print('P-states          ', row('P'), tot('P'))
print('expanded          ', row('expanded'), tot('expanded'))
print('leaves            ', row('leaf'), tot('leaf'))
print('Q-states          ', row('Q'), tot('Q'))
print('threat children   ', row('threat_children'), tot('threat_children'))
print('danger children   ', row('danger_children'), tot('danger_children'))
print('children (ii)     ', row('children_ii'), tot('children_ii'))
print('VOID children (ii)', row('void_ii'), tot('void_ii'))
print('VOID children (c) ', tot('void_c'), ' VOID children (d)', tot('void_d'))
print('non-busy table moves', row('nonbusy_table_moves'), tot('nonbusy_table_moves'))
print('leaves by theta   ', [cnt['leaf_theta'].get(k, 0) for k in range(9)])
print('table entries used: %d of %d; leaves used: %d of %d' % (len(used_tbl), len(tbl), len(used_lv), len(lv)))

exp = {
    'P': [1, 2, 5, 14, 35, 181, 1718, 7227, 34801, 93124, 16027],
    'leaf': [0, 0, 0, 0, 0, 0, 0, 0, 3382, 87314, 16027],
    'Q': [0, 2, 5, 14, 35, 101, 446, 3487, 14362, 62381, 12331],
    'threat_children': [0, 0, 0, 0, 0, 824, 4075, 14460, 47020, 72352, 4767],
    'danger_children': [0, 0, 0, 0, 0, 0, 0, 0, 20416, 6060, 1799],
    'void_ii': [0, 0, 0, 0, 0, 0, 1, 6, 54, 88, 2],
}
for k, v in exp.items():
    assert row(k) == v, ('MISMATCH with Appendix A.5', k, row(k), v)
assert tot('expanded') == 46412 and tot('children_ii') == 54482 and tot('void_c') == 0 and tot('void_d') == 0
assert [cnt['leaf_theta'].get(k, 0) for k in range(9)] == [64841, 32703, 7267, 1434, 343, 105, 25, 4, 1]
assert len(used_tbl) == len(tbl) and len(used_lv) == len(lv)
assert tot('nonbusy_table_moves') == 434
print('all counts agree with Appendix A.5 / Table 3; (V2) t<Rmax and (V3) hold on the reachable set')
# (L2) on a random sample of leaves
rng = random.Random(4)
sample = rng.sample(sorted(lv), 1500)
for (t, Bc, Fc) in sample:
    assert find_pairing(set(Bc), set(Fc)) is not None
print('(L1) on all leaves and (L2) on a sample of 1500 leaves: OK   [LEAF (L3) not evaluated here]')
import pickle
pickle.dump(seen, open(__import__('os').path.join(__import__('tempfile').gettempdir(), 'b_reachable.pkl'), 'wb'))
print('reachable set V (%d canonical states) written for b_play6.py' % len(seen))
print('total %.1f s' % (time.time() - t0))

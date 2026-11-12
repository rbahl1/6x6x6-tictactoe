#!/usr/bin/env python3
# Referee audit A6: Table 5 (leaf table of busy sets), orbit counts by Burnside, orbit sizes
import json, itertools, collections, sys
sys.path.insert(0, '.')
ROOT = '/home/user/ttt6/'
exec(open('a3_lines_of_play.py').read().split("print('=== Line A")[0])   # geometry, group, canon_maps, live, slack
K = [c for c in range(216) if busy(c)]
# Burnside
def poly_mul(a, b):
    r = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b): r[i + j] += x * y
    return r
tot = [0] * 25
for g in G:
    seen = set(); p = [1]
    for c in K:
        if c in seen: continue
        n = 0; x = c
        while x not in seen: seen.add(x); x = g[x]; n += 1
        q = [0] * (n + 1); q[0] = 1; q[n] = 1; p = poly_mul(p, q)
    for i, v in enumerate(p): tot[i] += v
print('Burnside orbit counts of busy k-sets, k=7..10:', [tot[k] // 1152 for k in (7, 8, 9, 10)], 'exact division:', all(tot[k] % 1152 == 0 for k in range(25)))
mine = {}
for l in open('lt_leaf.out'):
    inst, res = l.rsplit(' : ', 1); mine[inst] = res.strip()
can = {}
for l in open('lt_canon.out'):
    a, b = l.strip().split(' -> '); can[a.split(' | ')[0]] = b.split(' | ')[0]
for k in (8, 9, 10):
    recs = [json.loads(l) for l in open(ROOT + 'results/leaftable/phi_k%d.jsonl' % k)]
    Bs = [r['B'] for r in recs]
    allbusy = all(all(busy(int(c)) for c in b.split(',')) and len(b.split(',')) == k for b in Bs)
    canonical = all(can[b] == b for b in Bs)
    sl_ok = all(slack([int(c) for c in r['B'].split(',')]) == r['slack'] for r in recs)
    lf_ok = all((mine['%d %s -' % (k, r['B'])] == 'true') == r['leaf'] for r in recs)
    nleaf = sum(1 for r in recs if mine['%d %s -' % (k, r['B'])] == 'true')
    nsl = sum(1 for r in recs if r['slack'] >= k)
    print('k=%d records %d distinct %d all busy k-sets %s all canonical %s slack fields correct %s LEAF fields equal mine %s | slack>=k: %d  LEAF (mine): %d  max slack %d' % (k, len(recs), len(set(Bs)), allbusy, canonical, sl_ok, lf_ok, nsl, nleaf, max(r['slack'] for r in recs)))
    if k in (8, 9):
        tot_sets = 0
        for r in recs:
            if r['leaf']:
                _, maps = canon_maps([int(c) for c in r['B'].split(',')], [])
                tot_sets += 1152 // len(maps)
        print('   sets in the LEAF orbits:', tot_sets)
        print('   LEAF orbits all have slack>=k:', all(r['slack'] >= k for r in recs if r['leaf']), ' slack values of LEAF orbits', collections.Counter(r['slack'] for r in recs if r['leaf']))

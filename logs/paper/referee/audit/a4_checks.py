#!/usr/bin/env python3
# Referee audit A4/A5: results/final/checks (exact enumeration; integer program on leaves)
import json, glob, collections, statistics
ROOT = '/home/user/ttt6/'
leaves = [tuple(l.split()) for l in open(ROOT + 'results/final/leaves.txt')]
LS = set(leaves)
def theta(k, F): return int(k) - (0 if F == '-' else len(F.split(',')))
print('=== exact enumeration')
tot = 0; allset = set()
for th in range(4):
    recs = [json.loads(l) for l in open(ROOT + 'results/final/checks/enum_theta%d.jsonl' % th)]
    keys = set((str(r['k']), r['B'], r['F']) for r in recs)
    want = set(l for l in leaves if theta(l[0], l[2]) == th)
    nodes = [r['nodes'] for r in recs]
    print(' theta', th, 'records', len(recs), 'distinct', len(keys), 'equal to the set of leaves with this theta:', keys == want,
          'all holds:', all(r['holds'] is True for r in recs), 'status values', collections.Counter(r['status'] for r in recs),
          'max nodes', max(nodes), 'sum nodes', sum(nodes), 'X all "-":', all(r['X'] == '-' for r in recs))
    tot += len(recs); allset |= keys
print(' total', tot, 'share of leaves %.4f%%' % (100.0 * tot / len(leaves)), 'distinct', len(allset))

print('=== integer program on the leaves')
recs = []
for f in ['ip_leaf_sample.jsonl', 'ip_leaf_theta_ge4_rest.shard0of2.jsonl', 'ip_leaf_theta_ge4_rest.shard1of2.jsonl']:
    r = [json.loads(l) for l in open(ROOT + 'results/final/checks/' + f)]
    print(' ', f, len(r)); recs += r
keys = [(str(r['k']), r['B'], r['F']) for r in recs]
print(' records', len(recs), 'distinct', len(set(keys)), 'all are leaves:', all(k in LS for k in keys))
byth = collections.Counter(theta(*((k[0], k[2]))) for k in set(keys))
print(' distinct instances by theta', dict(sorted(byth.items())))
print(' theta>=4 covered', sum(v for t, v in byth.items() if t >= 4), 'of', sum(1 for l in leaves if theta(l[0], l[2]) >= 4),
      '; all theta>=4 leaves covered:', set(l for l in leaves if theta(l[0], l[2]) >= 4) <= set(keys))
print(' theta<=3 sampled', sum(v for t, v in byth.items() if t <= 3))
print(' decisions', collections.Counter(r['decision'] for r in recs), 'status', collections.Counter(r['status'] for r in recs), 'modes', collections.Counter(r['mode'] for r in recs))
print(' worst values', collections.Counter(r['worst'] for r in recs))
tm = collections.defaultdict(list)
for r in recs: tm[theta(r['k'], r['F'])].append(r['seconds'])
for t in sorted(tm): print('  theta %d n=%d median %.3f max %.1f' % (t, len(tm[t]), statistics.median(tm[t]), max(tm[t])))
t45 = tm[4] + tm[5]
print('  theta 4,5 together: median %.2f max %.1f' % (statistics.median(t45), max(t45)))
print('  total seconds %.0f = %.2f h' % (sum(r['seconds'] for r in recs), sum(r['seconds'] for r in recs) / 3600))

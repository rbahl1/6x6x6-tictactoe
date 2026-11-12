#!/usr/bin/env python3
# referee2 checker Y: recount numbers of the revised paper from raw files
import json, sys, collections, hashlib
R='/home/user/ttt6/'
def ids(s): return [] if s=='-' else [int(x) for x in s.split(',')]
# geometry
def lines():
    L=set()
    import itertools
    for f in itertools.product(range(8),repeat=3):  # 0..5 const, 6 id, 7 rev
        if all(c<6 for c in f): continue
        cells=[]
        for t in range(6):
            c=[ (v if v<6 else (t if v==6 else 5-t)) for v in f]
            cells.append(36*c[0]+6*c[1]+c[2])
        L.add(frozenset(cells))
    return sorted(L,key=sorted)
LN=lines(); assert len(LN)==148
def lam(i): return min(i,5-i)
busy={36*x+6*y+z for x in range(6) for y in range(6) for z in range(6) if lam(x)==lam(y)==lam(z)}
assert len(busy)==24
def admissible(B,F):
    B=set(B);F=set(F)
    if B&F: return False
    four=[]
    for l in LN:
        if l&B: continue
        n=len(l&F)
        if n>=5: return False
        if n==4: four.append(l)
    for i in range(len(four)):
        for j in range(i+1,len(four)):
            c=four[i]&four[j]
            if c and not (c&F): return False
    return True
# 1. degenerate instances
deg=collections.Counter(); dec=collections.Counter()
for ln in open(R+'results/crosscheck/certify.jsonl'):
    r=json.loads(ln); dec[r['decision']]+=1
    if r['decision'] not in('holds','fails'):
        B=ids(r['B']);F=ids(r['F']);k=r['k']
        adm=admissible(B,F)
        deg[('notadm' if not adm else 'adm', 'F>k' if len(F)>k else 'F<=k')]+=1
print('certify decisions',dict(dec)); print('degenerate',dict(deg))
# holds with leaf false
n=0
for ln in open(R+'results/crosscheck/certify.jsonl'):
    r=json.loads(ln)
    if r['decision']=='holds' and not r['leaf']: n+=1
print('holds with LEAF false',n)
# 2. leaves by t, theta ; busy stats
T=collections.Counter(); TT=collections.Counter(); nb=collections.Counter()
nl=0
for ln in open(R+'results/final/leaves.txt'):
    t,B,F=ln.split(); t=int(t); B=ids(B); F=ids(F); nl+=1
    T[(t,t-len(F))]+=1; TT[t-len(F)]+=1
    nb[sum(1 for b in B if b not in busy)]+=1
print('leaves',nl,'by theta',sorted(TT.items()))
for t in (8,9,10): print(t,[T[(t,th)] for th in range(9)])
print('leaves by #nonbusy B stones',sorted(nb.items()))
nbm=collections.Counter(); nq=0; ii=0
for ln in open(R+'results/final/strategy.tbl'):
    t,B,F,m=ln.split(); nq+=1
    if int(m) not in busy: nbm[int(t)]+=1
    if len(ids(F))<int(t): ii+=1
print('table',nq,'nonbusy moves',sorted(nbm.items()),sum(nbm.values()),'|F|<t',ii)
for f in ('results/final/strategy.tbl','results/final/leaves.txt'):
    d=open(R+f,'rb').read(); print(f,len(d),hashlib.sha256(d).hexdigest())

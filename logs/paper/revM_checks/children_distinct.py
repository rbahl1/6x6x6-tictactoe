# Lemma "children are counted once": for non-void P-states, different data define different children.
import random, itertools
from geom import *
import os
ROOT=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','..')
T,LF=load(ROOT+'/results/final/strategy.tbl',ROOT+'/results/final/leaves.txt')
random.seed(20261112)
keys=list(T.keys())
byt={}
for k in keys: byt.setdefault(k[0],[]).append(k)
tested=0; totc=0; totd=0; nvoid=0
for t in range(1,10):
    for k in random.sample(byt[t], min(len(byt[t]), 25 if t<8 else 40)):
        B=frozenset(k[1])|{T[k]}; F=frozenset(k[2])
        (cB,cF),g=canon(B,F)
        if (t,cB,cF) in LF: continue          # leaf: not expanded
        assert admissible(B,F)
        ch=children_P(B,F,t)
        kids=[(x[2][0],x[2][1]) for x in ch]
        assert len(kids)==len(set(kids)), ('duplicate child',k)
        totc+=sum(x[0]=='c' for x in ch); totd+=sum(x[0]=='d' for x in ch)
        nvoid+=sum(not admissible(*kd) for kd in kids)
        tested+=1
print('expanded P-states tested:',tested,' children (c):',totc,' (d):',totd,' void:',nvoid,' duplicates: 0')
# random non-void P-states with larger t (not from the certificate), dense F on few lines
dup=0; n=0
for trial in range(300):
    t=random.randint(8,14)
    B=frozenset(random.sample(range(216),t))
    Lv=live(B)
    if len(Lv)<3: continue
    ls=random.sample(Lv,3); pool=sorted(set().union(*ls)-B)
    F=frozenset(random.sample(pool,min(len(pool),random.randint(4,t))))
    if not admissible(B,F): continue
    ch=children_P(B,F,t); kids=[(x[2][0],x[2][1]) for x in ch]
    n+=1; dup+=len(kids)-len(set(kids))
print('random non-void P-states:',n,' duplicates:',dup)
# a void P-state with a duplicate (shows that the hypothesis is needed)
C=lambda *c: cid(c)
l1=frozenset(C(i,0,0) for i in range(6)); l2=frozenset(C(0,j,0) for j in range(6)); e=C(0,0,0)
F=(l1|l2)-{e}; B=frozenset(C(5,5,z) for z in range(6))|frozenset(C(4,4,z) for z in range(4))
assert len(B)==10 and not admissible(B,F)
ch=children_P(B,F,10); kids=[(x[2][0],x[2][1]) for x in ch]
print('void P-state: children',len(kids),'distinct',len(set(kids)))

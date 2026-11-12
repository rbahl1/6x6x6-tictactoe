# referee X: Table 3, round 5, recomputed with own code from the table entries of rounds 4 and 5
# (threat children of the expanded P-states of round 4; P-states and Q-states of round 5)
from x_geom import *
def parse(s): return () if s=='-' else tuple(int(v) for v in s.split(','))
T={}
for ln in open("/home/user/ttt6/results/final/strategy.tbl"):
    t,B,F,m=ln.split(' '); T[(int(t),parse(B),parse(F))]=int(m)
P4=set()
for (t,B,F),m in T.items():
    if t==4:
        assert m not in B and m not in F
        (cb,cf),_=canon(B+(m,),F); P4.add((cb,cf))
print("P-states of round 4:",len(P4))
thr=0; kids=set(); q5=set(); void=0
for B,F in P4:
    Bm=mask(B); Fm=mask(F); assert admissible(Bm,Fm)
    (cb,cf),_=canon(B,F); assert (cb,cf)==(B,F)
    q5.add((B,F))                                   # child (a): Q(B,F,5), canonical because (B,F) is
    seen=set()
    for l in live(Bm):
        for e in LINES[l]:
            if Fm>>e&1: continue
            F2=Fm|(LMASK[l]&~(1<<e))
            if F2.bit_count()>5: continue
            if not admissible(Bm|1<<e,F2): void+=1; continue
            assert (Bm|1<<e,F2) not in seen; seen.add((Bm|1<<e,F2))
            thr+=1
            kids.add(canon(cells_of(Bm|1<<e),cells_of(F2))[0])
print("threat children entering round 5:",thr,"(VOID children (c):",void,"); distinct canonical forms:",len(kids))
# Q-states of round 5: child (a) of the P-states of round 4, and children (ii) inside round 5
Q=set(q5); stack=list(q5); P5=set(kids); void2=0
while stack:
    B,F=stack.pop(); m=T[(5,B,F)]
    P5.add(canon(B+(m,),F)[0])
    if len(F)<5:
        if admissible(mask(B),mask(F+(m,))):
            k=canon(B,F+(m,))[0]
            if k not in Q: Q.add(k); stack.append(k)
        else: void2+=1
print("Q-states of round 5:",len(Q),"; P-states of round 5:",len(P5),"; VOID children (ii) in round 5:",void2)
assert (len(P4),thr,len(Q),len(P5),void2)==(35,824,101,181,0)
print("= Table 3, row t=5: 181 P-states, 101 Q-states, 824 threat children, 0 VOID: ok")

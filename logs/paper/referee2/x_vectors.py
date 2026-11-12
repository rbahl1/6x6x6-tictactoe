# referee X: Appendix A -- canonical-form test vectors, the 26 LEAF test vectors (Table 9), own code
import sys, time
from x_geom import *
from x_flow import *
t0=time.time(); ok=True
def chk(cond,what):
    global ok
    print("  %-104s %s"%(what,"ok" if cond else "FAILED")); ok=ok and bool(cond)
def P(s): return [] if s.strip()=='-' else [int(v) for v in s.split(',')]
print("A.1 canonical forms")
vec=[("43","0","0","43"),
     ("0,46,92,123,169,180,215","-","0,5,61,93,122,154,210","-"),
     ("0,35,46,92,123,169,180,215","-","0,5,61,93,122,154,210,215","-"),
     ("5,30,43,64,86,93,128,169,180","0,35,46,87,122,151,172,185,210","0,35,43,64,86,93,123,154,210","5,30,61,122,129,151,172,180,215")]
for B,F,cB,cF in vec:
    (b,f),_=canon(P(B),P(F))
    chk(list(b)==P(cB) and list(f)==P(cF),"%s | %s -> %s | %s"%(B,F,cB,cF))
C=lambda *p: cid(p)
T7=sorted(C(*p) for p in [(0,0,0),(5,5,5),(1,1,4),(4,4,1),(2,3,2),(3,2,3),(5,0,0)]); T8=sorted(T7+[C(0,5,5)])
chk(T7==P("0,46,92,123,169,180,215") and T8==P("0,35,46,92,123,169,180,215"),"ids of T7 and T8")
# entries quoted in A.2, A.3: present in the files, at the stated positions, coordinates as stated
tbl=open("/home/user/ttt6/results/final/strategy.tbl").read().split("\n"); lv=open("/home/user/ttt6/results/final/leaves.txt").read().split("\n")
chk(tbl[0]=="1 - - 0" and tbl[2353]=="5 0,46,169,215 - 92" and tbl[9790]=="7 0,35,46,92,123,169 5,30,43,180,185,210,215 50","the three quoted table entries are entries 1, 2354, 9791")
chk(lv[0]=="9 0,1,5,30,43,93,122,172,210 7,13,14,19,21,25,28,31,35" and lv[8184]=="8 0,5,61,93,122,154,210,215 -" and lv[106722]=="9 0,35,43,64,86,93,123,154,210 5,30,61,122,129,151,172,180,215" and lv[106723]=="" and len(lv)==106724,"the three quoted leaf entries are entries 1, 8185 and the last")
chk([cell(c) for c in P("0,46,169,215")]==[(0,0,0),(1,1,4),(4,4,1),(5,5,5)] and cell(92)==(2,3,2) and cell(50)==(1,2,2) and 50 not in BUSY,"coordinates of the second and third table entry")
chk([cell(c) for c in P("5,30,43,180,185,210,215")]==[(0,0,5),(0,5,0),(1,1,1),(5,0,0),(5,0,5),(5,5,0),(5,5,5)] and [cell(c) for c in P("0,35,46,92,123,169")]==[(0,0,0),(0,5,5),(1,1,4),(2,3,2),(3,2,3),(4,4,1)],"coordinates of the third table entry (B and F)")
chk([cell(c) for c in P("7,13,14,19,21,25,28,31,35")]==[(0,1,1),(0,2,1),(0,2,2),(0,3,1),(0,3,3),(0,4,1),(0,4,4),(0,5,1),(0,5,5)] and [cell(c) for c in P("0,1,5,30,43,93,122,172,210")]==[(0,0,0),(0,0,1),(0,0,5),(0,5,0),(1,1,1),(2,3,3),(3,2,2),(4,4,4),(5,5,0)],"coordinates of the first leaf entry")
print("A.5 LEAF test vectors (Table 9): Algorithm 2 as printed, own code")
def leaf(k,B,F): return alg2(mask(P(B)),mask(P(F)),k)
rows=[(7,"0,46,92,123,169,180,215","-",(False,3)),
      (8,"0,35,46,92,123,169,180,215","-",(True,0)),
      (9,"0,35,46,92,123,169,180,215","-",(False,3))]
hs=[5,30,43,61,64,86,87,93,122,128,129,151,154,172,185,210]
chk(sorted(hs)==sorted(c for c in BUSY if c not in T8),"the 16 cells h are the busy cells outside T8")
rows+=[(8,"0,35,46,92,123,169,180,215",str(h),(True,0)) for h in hs]
rows+=[(8,"0,35,43,46,92,123,169,180","215",(True,0)),
       (8,"0,5,35,43,46,61,86,92","123,129,169,172,180,210,215",(False,2)),
       (9,"0,5,35,43,46,61,86,87,92","123,129,154,169,172,180,210,215",(True,0)),
       (10,"0,5,30,35,43,46,61,86,87,92","123,128,129,154,169,172,180,210,215",(True,0)),
       (9,"0,8,35,46,92,123,169,180,215","44,80,116,152,188",(True,0)),
       (8,"0,8,46,92,123,169,180,215","44,80,116,152,188",(False,3)),
       (0,"-","-",(False,2))]
chk(len(rows)==26,"26 instances")
bad=0
for k,B,F,want in rows:
    got=leaf(k,B,F)
    if got!=want: bad+=1; print("   MISMATCH",k,B,F,got,want)
chk(bad==0,"all 26 values of LEAF and the failing condition reproduced")
# descriptions
chk(sorted(P("0,35,43,46,92,123,169,180"))==sorted([c for c in T8 if c!=C(5,5,5)]+[C(1,1,1)]),"row 5: B = T8-(5,5,5)+(1,1,1)")
play=[(0,0,0),(1,1,4),(2,3,2),(0,5,5),(1,1,1),(2,2,2),(0,0,5),(1,4,1),(2,2,3),(0,5,0)]
anti=lambda p:(5-p[0],5-p[1],5-p[2])
for n,(Bs,Fs) in ((8,("0,5,35,43,46,61,86,92","123,129,169,172,180,210,215")),(9,("0,5,35,43,46,61,86,87,92","123,129,154,169,172,180,210,215")),(10,("0,5,30,35,43,46,61,86,87,92","123,128,129,154,169,172,180,210,215"))):
    chk(sorted(C(*p) for p in play[:n])==P(Bs) and sorted(C(*anti(p)) for p in play[:n-1])==P(Fs),"antipode stealing after %d rounds: B = first %d stones, F = antipodes of all but the last"%(n,n))
chk(P("44,80,116,152,188")==[C(x,1,2) for x in range(1,6)] and C(0,1,2)==8,"threat rows: F = {(x,1,2): x>=1}, forced cell (0,1,2) = id 8")
# worst(B,F,k) where it can be obtained cheaply with own code
def delta(Bm,Xm):
    """max deficiency = 2|L| - maximum assignment (own augmenting paths, lines may hold up to 2 cells)"""
    net=Net(Bm,Xm); tot=0
    for l in range(net.n):
        for _ in range(2):
            if net.augment([l]): tot+=1
    # a line whose first search fails may still gain later only through other sources; repeat until stable
    ch=True
    while ch:
        ch=False
        cntl=[0]*net.n
        for c,l in net.own.items(): cntl[l]+=1
        for l in range(net.n):
            while cntl[l]<2 and net.augment([l]): cntl[l]+=1; tot+=1; ch=True
    return 2*net.n-tot
chk(delta(0,0)==80,"empty board: delta = 80  (worst(0,0,0) = 80)")
Bm=mask(P("0,5,35,43,46,61,86,92")); Fm=mask(P("123,129,169,172,180,210,215"))
w=delta(Bm,Fm)
for c in range(216):
    if (Bm|Fm)>>c&1: continue
    if admissible(Bm,Fm|1<<c): w=max(w,delta(Bm,Fm|1<<c))
chk(w==6,"antipode stealing after 8 rounds: worst = %d (enumeration of all admissible X with |X| <= 8)"%w)
import itertools, random
rnd=random.Random(5)
def lowerbound(Bs,Fs,k,tries):
    Bm=mask(P(Bs)); Fm=mask(P(Fs)); best=delta(Bm,Fm); free=[c for c in range(216) if not (Bm|Fm)>>c&1]
    for _ in range(tries):
        X=Fm
        for c in rnd.sample(free,k-Fm.bit_count()):
            if admissible(Bm,X|1<<c): X|=1<<c
        best=max(best,delta(Bm,X))
    return best
chk(lowerbound("0,46,92,123,169,180,215","-",7,30)==6,"T7, k=7: an admissible X with deficiency 6 exists (worst >= 6; worst <= 6 since Phi = 1)")
chk(lowerbound("0,35,46,92,123,169,180,215","-",9,30)==1,"T8, k=9: an admissible X with deficiency 1 exists (worst >= 1; worst <= 1 since Phi = 8)")
print("RESULT:","all checks passed" if ok else "FAILED","  (%.1fs)"%(time.time()-t0))

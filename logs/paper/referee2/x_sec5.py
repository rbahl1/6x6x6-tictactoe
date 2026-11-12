# referee X: Section 5 -- Example 5.3 against results/final/strategy.tbl, Lemma 5.2, Figure 4 (own code)
import itertools, random, sys, time
from x_geom import *
t0=time.time()
ROOT="/home/user/ttt6/"
def parse(s): return () if s=='-' else tuple(int(v) for v in s.split(','))
TBL={}
for ln in open(ROOT+"results/final/strategy.tbl"):
    t,B,F,m=ln.split(' ')
    k=(int(t),parse(B),parse(F)); assert k not in TBL; TBL[k]=int(m)
print("table entries:",len(TBL))
C=lambda *p: cid(p)
def moves(B,F,t):
    """set of real cells that sigma may play at Q(B,F,t): g^{-1}(m^c) over all g attaining the canonical form"""
    (cb,cf),gs=canon(B,F)
    m=TBL[(t,cb,cf)]
    return {g.index(m) for g in gs}, (cb,cf,m)
def show(S): return "{"+",".join(str(cell(c)) for c in sorted(S))+"}"
ok=True
def chk(cond,what):
    global ok
    print("  %-100s %s"%(what,"ok" if cond else "FAILED")); ok=ok and bool(cond)
print("Example 5.3")
ms,_=moves([],[],1); chk(C(0,0,0) in ms,"Q(0,0,1): table names (0,0,0)  [possible cells: %d corners]"%len(ms))
ms,_=moves([],[C(0,0,0)],1); chk(C(1,1,1) in ms,"Q(0,{(0,0,0)},1): (1,1,1)   [possible: %s]"%show(ms))
ms,_=moves([C(0,0,0)],[],2); chk(ms=={C(5,5,5)},"Q({(0,0,0)},0,2): (5,5,5), unique")
ms,_=moves([C(0,0,0),C(5,5,5)],[],3); chk(C(1,1,4) in ms,"Q({(0,0,0),(5,5,5)},0,3): (1,1,4)  [possible: %d cells]"%len(ms))
ms,k=moves([C(0,0,0),C(5,5,5),C(1,1,4)],[],4); chk(ms=={C(4,4,1)},"Q(..,(1,1,4)},0,4): (4,4,1), unique; canonical entry %s"%(k,))
B4=[C(0,0,0),C(5,5,5),C(1,1,4),C(4,4,1)]
ms,_=moves(B4,[],5); chk(C(2,3,2) in ms,"round 5: (2,3,2)  [possible: %s]"%show(ms))
B5=B4+[C(2,3,2)]
ms,_=moves(B5,[],6); chk(ms=={C(3,2,3)},"round 6: (3,2,3), unique")
B6=B5+[C(3,2,3)]
ms,_=moves(B6,[],7); chk(C(0,5,5) in ms,"round 7: (0,5,5)  [possible: %s]"%show(ms))
B7=B6+[C(0,5,5)]
ms,_=moves(B7,[],8); chk(ms=={C(5,0,0)},"round 8: (5,0,0), unique")
T8=sorted(B7+[C(5,0,0)])
chk(T8==sorted(C(*p) for p in [(0,0,0),(5,5,5),(5,0,0),(1,1,4),(4,4,1),(2,3,2),(3,2,3),(0,5,5)]),"B_7+(5,0,0) = T_8")
# occupied cell
ms,_=moves([C(0,0,0)],[C(5,5,5)],2); chk(C(1,1,4) in ms,"Q({(0,0,0)},{(5,5,5)},2): (1,1,4)  [possible: %d cells]"%len(ms))
chk(admissible(mask([C(0,0,0),C(1,1,4)]),mask([C(5,5,5),C(2,2,0)])),"position of 'An occupied cell' is admissible, no forced event")
# children
def children(B,F,t):
    """literal Definition 5.1; returns lists of (data, (B',F')) for (c) and (d)"""
    Bm=mask(B); Fm=mask(F); Lv=live(Bm); cc=[]; dd=[]
    for l in Lv:
        for e in LINES[l]:
            if Fm>>e&1: continue
            F2=Fm|(LMASK[l]&~(1<<e))
            if F2.bit_count()<=t+1: cc.append(((l,e),(Bm|1<<e,F2)))
    for i,a in enumerate(Lv):
        for b in Lv[i+1:]:
            m=LMASK[a]&LMASK[b]
            if not m: continue
            c=m.bit_length()-1
            if Fm>>c&1: continue
            ra=[x for x in LINES[a] if x!=c]; rb=[x for x in LINES[b] if x!=c]
            for Qa in itertools.combinations(ra,4):
                qa=mask(Qa)
                if (Fm&LMASK[a])&~qa: continue
                for Qb in itertools.combinations(rb,4):
                    qb=mask(Qb)
                    if (Fm&LMASK[b])&~qb: continue
                    F2=Fm|qa|qb
                    if F2.bit_count()<=t+1: dd.append(((a,b,qa,qb),(Bm|1<<c,F2)))
    return cc,dd
def npairs(B):
    Lv=live(mask(B)); return sum(1 for i,a in enumerate(Lv) for b in Lv[i+1:] if LMASK[a]&LMASK[b])
cc,dd=children(B4,[],4)
chk(len(live(mask(B4)))==122,"122 live lines w.r.t. B_4")
chk(len(cc)==732 and len(dd)==0 and len({s for _,s in cc})==732,"P(B_4,0,4): 732 children (c), pairwise different, no child (d)")
chk(all(admissible(b,f) for _,(b,f) in cc),"none of them VOID")
l=LINES.index(tuple(sorted(C(0,i,1) for i in range(6))))
want=(mask(B4+[C(0,0,1)]),mask([C(0,i,1) for i in range(1,6)]))
chk(any(d==(l,C(0,0,1)) and s==want for d,s in cc),"the threat child for l={(0,i,1)}, e=(0,0,1) is as printed")
cc,dd=children(B7,[],7)
chk(len(live(mask(B7)))==104,"104 live lines w.r.t. B_7")
chk(npairs(B7)==650,"650 pairs of live lines that meet")
chk(len(cc)==624 and len(dd)==16250,"P(B_7,0,7): 624 children (c), 16,250 children (d)")
chk(len({s for _,s in cc+dd})==624+16250,"all of them pairwise different")
chk(all(admissible(b,f) for _,(b,f) in cc+dd),"none of them VOID")
a=LINES.index(tuple(sorted(C(i,2,0) for i in range(6)))); b=LINES.index(tuple(sorted(C(2,j,0) for j in range(6))))
Qa=mask([C(0,2,0),C(1,2,0),C(3,2,0),C(4,2,0)]); Qb=mask([C(2,0,0),C(2,1,0),C(2,3,0),C(2,4,0)])
want=(mask(B7+[C(2,2,0)]),Qa|Qb)
chk(a in live(mask(B7)) and b in live(mask(B7)) and any(s==want for d,s in dd),"the danger child for a={(i,2,0)}, b={(2,j,0)} is as printed")
# Table 6: all entries of rounds 1..3
tab6=[k+(m,) for k,m in TBL.items() if k[0]<=3]
chk(sorted(len([1 for e in tab6 if e[0]==t]) for t in (1,2,3))==[2,5,14],"2+5+14 table entries in rounds 1,2,3")
printed="""1||0,0,0
1|0,0,0|1,1,1
2 0,0,0||5,5,5
2 0,0,0|1,1,1|5,5,5
2 0,0,0|5,5,5|1,1,4
2 0,0,0|1,1,1;5,5,5|1,1,4
2 0,0,0|1,1,4;5,5,5|1,4,4
3 0,0,0;5,5,5||1,1,4
3 0,0,0;5,5,5|1,1,1|1,1,4
3 0,0,0;5,5,5|1,1,4|2,2,3
3 0,0,0;5,5,5|1,1,1;1,1,4|1,4,1
3 0,0,0;5,5,5|1,1,4;2,2,3|1,4,4
3 0,0,0;5,5,5|1,1,1;1,1,4;1,4,1|1,4,4
3 0,0,0;5,5,5|1,1,4;1,4,4;2,2,3|1,4,1
3 0,0,0;1,1,4|4,4,1|5,5,5
3 0,0,0;1,1,4|0,0,5;4,4,1|5,5,5
3 0,0,0;1,1,4|0,5,0;4,4,1|5,5,5
3 0,0,0;1,1,4|4,4,1;5,5,5|0,5,5
3 0,0,0;1,1,4|0,0,5;4,4,1;5,5,5|0,5,5
3 0,0,0;1,1,4|0,5,0;4,4,1;5,5,5|0,5,5
3 0,0,0;1,1,4|0,5,5;4,4,1;5,5,5|5,0,5"""
def ps(s): return tuple(sorted(cid(tuple(int(v) for v in p.split(','))) for p in s.split(';') if p))
pr=set()
for ln in printed.split('\n'):
    if ln[1]=='|': t=int(ln[0]); rest='|'+ln[2:] if ln[1]=='|' else ln[2:]; Bs,Fs,ms_=('',)+tuple(ln[2:].split('|'))
    else: t=int(ln[0]); Bs,Fs,ms_=ln[2:].split('|')
    pr.add((t,ps(Bs),ps(Fs),ps(ms_)[0]))
chk(pr==set(tab6),"Table 6 (rounds 1-3) = the 21 entries of the file")
# ---------------- Lemma 5.2 on random states
print("Lemma 5.2 (different data define different children)")
rnd=random.Random(20261112)
def rand_state():
    t=rnd.choice([7,8,9,10,11,12])
    B=set(rnd.sample(BUSY,rnd.randint(0,min(t,8))))
    while len(B)<t: B.add(rnd.randrange(216))
    Bm=mask(B); Lv=live(Bm)
    F=set()
    # stones concentrated on a few live lines through a common cell, to provoke coincidences
    c=rnd.choice([x for x in range(216) if not Bm>>x&1])
    ls=[l for l in THRU[c] if l in Lv]; rnd.shuffle(ls)
    for l in ls[:rnd.randint(1,3)]:
        cand=[x for x in LINES[l] if x!=c]
        for x in rnd.sample(cand,rnd.randint(2,4)):
            if len(F)<t and admissible(Bm,mask(F|{x})): F.add(x)
    for _ in range(rnd.randint(0,3)):
        x=rnd.randrange(216)
        if len(F)<t and not Bm>>x&1 and admissible(Bm,mask(F|{x})): F.add(x)
    return sorted(B),sorted(F),t
nst=0; nch=0; nd=0; bad=0; four=0
while nst<400:
    B,F,t=rand_state()
    if not admissible(mask(B),mask(F)): continue
    cc,dd=children(B,F,t)
    allc=[s for _,s in cc+dd]
    if len(set(allc))!=len(allc): bad+=1
    nst+=1; nch+=len(cc); nd+=len(dd)
    if any((LMASK[l]&mask(F)).bit_count()==4 for l in live(mask(B))): four+=1
chk(bad==0,"%d random non-VOID P-states (%d with a live line holding 4 cells of F): %d children (c), %d children (d), all pairwise different"%(nst,four,nch,nd))
# VOID parents: explicit failures of (a), (b), (c)
e=C(2,2,0); la=[C(i,2,0) for i in range(6)]; lb=[C(2,j,0) for j in range(6)]; ld=[C(i,i,0) for i in range(6)]
F=[x for x in la+lb if x!=e]
cc,dd=children([],F,10)
chk(len({s for _,s in cc})<len(cc),"VOID parent, F = (l-e) u (l'-e): two pairs (l,e),(l',e) give the same child (c)  [(a) fails]")
chk(len({s for _,s in dd})==len(dd),"   ... for this F statement (b) still holds (the example of the text shows the failure of (a) only)")
F=[x for x in la if x!=e and x!=C(5,2,0)]+[x for x in lb if x!=e and x!=C(2,5,0)]
cc,dd=children([],F,12)
chk(len({s for _,s in dd})<len(dd),"VOID parent, F = 4 cells of a-c and 4 cells of a1-c: different data give the same child (d)  [(b) fails]")
F=[x for x in la+lb+ld if x!=e]; F=[x for x in F if x not in (C(5,2,0),C(2,5,0))]
cc,dd=children([],F,14)
chk(bool({s for _,s in cc}&{s for _,s in dd}),"VOID parent, F > (l-c) u Q_a u Q_b: a child (c) equals a child (d)  [(c) fails]")
# ---------------- Figure 4
print("Figure 4 (patterns) against Definition 5.4")
def Ln(pts): return LINES.index(tuple(sorted(cid(p) for p in pts)))
def isE2(Bm,Fm,l,a,b,x,e,c,Sa,Sb):
    Lv=live(Bm)
    if not (l in Lv and a in Lv and b in Lv and len({l,a,b})==3 and len({x,e,c})==3): return False
    if not (LMASK[l]&LMASK[a]==1<<x and LMASK[l]>>e&1 and LMASK[a]&LMASK[b]==1<<c and not LMASK[b]>>x&1 and not LMASK[b]>>e&1): return False
    if not (Sa&~(LMASK[a]&~(1<<c))==0 and Sa.bit_count()==4 and Sa>>x&1 and Sb&~(LMASK[b]&~(1<<c))==0 and Sb.bit_count()==4): return False
    if Fm>>e&1 or Fm>>c&1 or (Fm&LMASK[a])&~Sa or (Fm&LMASK[b])&~Sb: return False
    return True
def isE3(Bm,Fm,a,a1,b,b1,x,c,c1,Sa,Sa1,Sb,Sb1):
    Lv=live(Bm)
    if not (all(z in Lv for z in (a,a1,b,b1)) and len({a,a1,b,b1})==4 and len({x,c,c1})==3): return False
    if not (LMASK[a]&LMASK[a1]==1<<x and LMASK[a]&LMASK[b]==1<<c and LMASK[a1]&LMASK[b1]==1<<c1 and not (LMASK[b]|LMASK[b1])>>x&1): return False
    for S,ln,cc_ in ((Sa,a,c),(Sa1,a1,c1),(Sb,b,c),(Sb1,b1,c1)):
        if S&~(LMASK[ln]&~(1<<cc_)) or S.bit_count()!=4: return False
        if S>>c&1 or S>>c1&1: return False
        if (Fm&LMASK[ln])&~S: return False
    if not (Sa>>x&1 and Sa1>>x&1): return False
    if Fm>>c&1 or Fm>>c1&1: return False
    return True
P=lambda x,y: C(x,y,0)
# E2 picture
l=Ln([(i,0,0) for i in range(6)]); a=Ln([(0,j,0) for j in range(6)]); b=Ln([(i,5-i,0) for i in range(6)])
stones=[P(*p) for p in [(0,0),(2,0),(3,0),(4,0),(5,0),(0,1),(0,2),(0,3),(4,1),(3,2),(2,3)]]
x=P(0,0); e=P(1,0); c=P(0,5); S=mask(stones)
Sl=LMASK[l]&~(1<<e); Sa=S&LMASK[a]; Sb=S&LMASK[b]
chk(isE2(0,0,l,a,b,x,e,c,Sa,Sb),"left picture is an E2 pattern for (0,0) with S_a, S_b = the stones on a, b")
chk((Sl|Sa|Sb)==S and S.bit_count()==11,"its stone sets are exactly the 11 black dots")
X=S&~(1<<x)
chk(admissible(0,X),"the stones without x form an admissible set")
chk((LMASK[l]&S).bit_count()==5 and not S>>e&1,"after x: l is a threat with sixth cell e")
chk(not admissible(1<<e,S) ,"after the reply e the set is not admissible (the danger pair {a,b} survives)")
# E3 picture
a=Ln([(i,0,0) for i in range(6)]); b1=Ln([(i,1,0) for i in range(6)]); a1=Ln([(0,j,0) for j in range(6)]); b=Ln([(i,5-i,0) for i in range(6)])
stones=[P(*p) for p in [(0,0),(1,0),(2,0),(3,0),(0,2),(0,3),(0,5),(1,4),(2,3),(4,1),(1,1),(2,1),(3,1)]]
x=P(0,0); c=P(5,0); c1=P(0,1); S=mask(stones)
Sa=S&LMASK[a]; Sa1=S&LMASK[a1]; Sb=S&LMASK[b]; Sb1=S&LMASK[b1]
chk(isE3(0,0,a,a1,b,b1,x,c,c1,Sa,Sa1,Sb,Sb1),"right picture is an E3 pattern for (0,0)")
chk((Sa|Sa1|Sb|Sb1)==S and S.bit_count()==13,"its stone sets are exactly the 13 black dots")
chk(admissible(0,S&~(1<<x)),"the stones without x form an admissible set")
chk(all(not admissible(1<<z,S) for z in range(216) if not S>>z&1),"no single reply of Breaker makes the 13 stones admissible")
chk(all((LMASK[i]&S).bit_count()<=4 for i in range(NL)),"no line has five of the 13 stones (no threat)")
# minimal numbers of stones on this board (Proposition S2.5), by enumeration of the lines of all patterns for B=F=0
meet={}
for i in range(NL):
    for j in range(NL):
        if i!=j and LMASK[i]&LMASK[j]: meet[(i,j)]=(LMASK[i]&LMASK[j]).bit_length()-1
best2=99; best3=99
for a in range(NL):
    for x in LINES[a]:
        for c in LINES[a]:
            if c==x: continue
            for b in THRU[c]:
                if b==a: continue
                # E2: l through x, l != a ; e in l - x, e not in b ; union = 12 - |S_l n S_b|
                for l in THRU[x]:
                    if l==a: continue
                    u=meet.get((l,b))
                    for e in LINES[l]:
                        if e==x or LMASK[b]>>e&1: continue
                        # S_l n S_b = {u} possible iff u exists, u != e (then u in S_l), u != c (u in b-c; u==c impossible since c not in l unless l=a)
                        k=12-(1 if (u is not None and u!=e and u!=c) else 0)
                        best2=min(best2,k)
                # E3
                for a1 in THRU[x]:
                    if a1==a: continue
                    for c1 in LINES[a1]:
                        if c1==x: continue
                        for b1 in THRU[c1]:
                            if b1==a1 or b1==b or b1==a or b==a1: continue
                            if LMASK[b]>>x&1 or LMASK[b1]>>x&1: continue
                            k=15
                            for (p,q) in ((a,b1),(a1,b),(b,b1)):
                                u=meet.get((p,q))
                                if u is not None and u!=c and u!=c1: k-=1
                            best3=min(best3,k)
chk(best2==11 and best3==13,"smallest number of stones over all patterns on the board: E2 %d, E3 %d"%(best2,best3))
print("RESULT:","all checks passed" if ok else "FAILED","  (%.1fs)"%(time.time()-t0))

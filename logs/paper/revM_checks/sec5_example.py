# Facts quoted in Example 5.3 ("The first rounds on the real board") of the revised paper; own geometry (geom.py).
import sys, itertools
from geom import *
import os
ROOT=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','..')
T,LF=load(ROOT+'/results/final/strategy.tbl',ROOT+'/results/final/leaves.txt')
C=lambda *c: cid(c)
def show(S): return '{'+', '.join(str(cc(c)) for c in sorted(S))+'}'
def allmoves(B,F,t):
    (cB,cF),g0=canon(B,F); mc=T[(t,cB,cF)]; res=set()
    for g in G:
        if (tuple(sorted(g[c] for c in B)),tuple(sorted(g[c] for c in F)))==(cB,cF):
            inv={g[c]:c for c in range(216)}; res.add(inv[mc])
    return (cB,cF,mc), sorted(res)
def inV(B,F,t,kind):
    (cB,cF),g=canon(B,F)
    if kind=='P':
        if (t,cB,cF) in LF: return 'leaf '+str((t,cB,cF))
        # expanded iff its child (a) has an entry
        return ('expanded (child (a) has entry %s)'%str(T[(t+1,cB,cF)])) if (t+1,cB,cF) in T else 'NOT IN V?'
    return ('entry %d'%T[(t,cB,cF)]) if (t,cB,cF) in T else 'NO ENTRY'
# --- round 1
print('Q(0,0,1):',allmoves(set(),set(),1))
print('Q(0,{000},1):',allmoves(set(),{0},1))
print('P({111},{000},1):',inV({C(1,1,1)},{0},1,'P'))
# --- rounds 2-4 quiet
B1={0}
print('Q({000},0,2):',allmoves(B1,set(),2))
print('Q({000},{555},2) [table move occupied]:',allmoves(B1,{C(5,5,5)},2), 'adm:',admissible(B1,{C(5,5,5)}))
info,ms=allmoves(B1,{C(5,5,5)},2)
for m in ms: print('   then P(B1+%s,{555},2):'%str(cc(m)), inV(B1|{m},{C(5,5,5)},2,'P'))
B2={0,C(5,5,5)}
print('Q(B2,0,3):',allmoves(B2,set(),3), [cc(m) for m in allmoves(B2,set(),3)[1]])
B3=B2|{C(1,1,4)}
info,ms=allmoves(B3,set(),4); print('Q(B3,0,4): canonical',show(info[0]),'entry',cc(info[2]),'real moves',[cc(m) for m in ms])
# the explicit g: Sigma_s o P_{3}, s: 0->1,5->4,1->0,4->5,2->2,3->3
s={0:1,5:4,1:0,4:5,2:2,3:3}
gmap=tuple(cid(tuple(s[v] for v in (c[0],c[1],5-c[2]))) for c in map(cc,range(216)))
assert gmap in set(G)
print('g(B3)=',show({gmap[c] for c in B3}), ' g^-1(555)=',cc([c for c in range(216) if gmap[c]==C(5,5,5)][0]))
B4=B3|{C(4,4,1)}
print('B4 canonical?',canon(B4,set())[0], sorted(B4))
# --- threat child at t=4
ch=children_P(B4,set(),4)
nc=[x for x in ch if x[0]=='c']; nd=[x for x in ch if x[0]=='d']
print('P(B4,0,4): live lines',len(live(frozenset(B4))),'children (c):',len(nc),'(d):',len(nd),
      'distinct children:',len({(x[2][0],x[2][1]) for x in ch}),
      'non-void:',sum(admissible(x[2][0],x[2][1]) for x in ch),
      'distinct canonical forms:',len({canon(x[2][0],x[2][1])[0] for x in ch}))
l=frozenset(C(0,i,1) for i in range(6)); assert l in LSET and not (l&B4)
e=C(0,0,1); Fc=l-{e}; Bc=B4|{e}
print('threat child: B',show(Bc),'F',show(Fc),'adm:',admissible(Bc,Fc),'canon:',canon(Bc,Fc)[0],'status:',inV(Bc,Fc,5,'P'))
print('  its child (a) Q(Bc,Fc,6):',allmoves(Bc,Fc,6)[0],[cc(m) for m in allmoves(Bc,Fc,6)[1]])
# Maker's 4 stones before: admissible?
X4={C(0,1,1),C(0,2,1),C(0,3,1),C(0,4,1)}; print('X4 B4-admissible:',admissible(B4,X4),' X5 (B4+e)-admissible:',admissible(Bc,X4|{C(0,5,1)}))
# --- quiet rounds 5-7
B5=B4|{C(2,3,2)}; B6=B5|{C(3,2,3)}; B7=B6|{C(0,5,5)}
for (Bx,t) in ((B4,5),(B5,6),(B6,7),(B7,8)):
    info,ms=allmoves(Bx,set(),t); print('Q(B%d,0,%d): entry'%(t-1,t),cc(info[2]),'canonical B',info[0],'real moves',[cc(m) for m in ms])
print('P(B7,0,7):',inV(B7,set(),7,'P'))
# --- danger child at t=7
a=frozenset(C(i,2,0) for i in range(6)); b=frozenset(C(2,j,0) for j in range(6)); c=C(2,2,0)
assert a in LSET and b in LSET and not (a&B7) and not (b&B7) and a&b=={c}
Qa=frozenset({C(0,2,0),C(1,2,0),C(3,2,0),C(4,2,0)}); Qb=frozenset({C(2,0,0),C(2,1,0),C(2,3,0),C(2,4,0)})
Bd=B7|{c}; Fd=Qa|Qb
print('danger child: |F|',len(Fd),'adm:',admissible(Bd,Fd),'status:',inV(Bd,Fd,8,'P'),'canon',canon(Bd,Fd)[0])
X7=Fd-{C(4,2,0)}; print('X7 B7-admissible:',admissible(B7,X7),'X8 B7-adm:',admissible(B7,Fd),'X8 (B7+c)-adm:',admissible(Bd,Fd))
ch7=children_P(B7,set(),7)
print('P(B7,0,7): children (c)',sum(x[0]=='c' for x in ch7),'(d)',sum(x[0]=='d' for x in ch7),'distinct',len({(x[2][0],x[2][1]) for x in ch7}),
      'void',sum(not admissible(x[2][0],x[2][1]) for x in ch7), 'live',len(live(frozenset(B7))))

#!/usr/bin/env python3
# referee2 checker Y: literal evaluation of LOST (Definition 5.4 of the revised paper) on the 8 vectors of Table 10
import itertools, sys
def lines():
    L=set()
    for f in itertools.product(range(8),repeat=3):
        if all(c<6 for c in f): continue
        L.add(frozenset(36*(f[0] if f[0]<6 else (t if f[0]==6 else 5-t))+6*(f[1] if f[1]<6 else (t if f[1]==6 else 5-t))+(f[2] if f[2]<6 else (t if f[2]==6 else 5-t)) for t in range(6)))
    return sorted(L,key=sorted)
LN=lines(); assert len(LN)==148
def ids(s): return set() if s=='-' else {int(x) for x in s.split(',')}
def lost(B,F,t):
    live=[l for l in LN if not (l&B)]
    thru={}
    for l in live:
        for c in l: thru.setdefault(c,[]).append(l)
    e2=e3=None
    # E2
    for a in live:
        for x in a:
            for c in a:
                if c==x or c in F: continue
                sas=[a-{c,p} for p in a if p not in (c,x) and p not in F]
                if not sas: continue
                for b in thru[c]:
                    if b==a: continue
                    sbs=[b-{c,q} for q in b if q!=c and q not in F]
                    if not sbs: continue
                    for l in thru[x]:
                        if l==a or l==b: continue
                        if len(F)+(5-len(F&l))+(4-len(F&a))+(4-len(F&b))-2>t+1: continue
                        for e in l:
                            if e==x or e in F or e in b: continue
                            Sl=l-{e}
                            for Sa in sas:
                                for Sb in sbs:
                                    n=len(F|Sl|Sa|Sb)
                                    if n<=t+1:
                                        s=len(Sl|Sa|Sb)
                                        if e2 is None or s<e2: e2=s
    # E3
    for a in live:
        for x in a:
            for a2 in thru[x]:
                if a2==a: continue
                for c in a:
                    if c==x or c in F: continue
                    for c2 in a2:
                        if c2==x or c2 in F or c2==c: continue
                        for b in thru[c]:
                            if b in (a,a2) or x in b: continue
                            for b2 in thru[c2]:
                                if b2 in (a,a2,b) or x in b2: continue
                                if len(F)+(4-len(F&a))+(4-len(F&a2))-1+max(0,4-len(F&b)-1)+max(0,4-len(F&b2)-2)>t+1: continue
                                def subs(ln,cc,need=None):
                                    out=[]
                                    for S in itertools.combinations(sorted(ln-{cc}),4):
                                        S=frozenset(S)
                                        if c in S or c2 in S: continue
                                        if need is not None and need not in S: continue
                                        if not ((F&ln)<=S): continue
                                        out.append(S)
                                    return out
                                A=subs(a,c,x); A2=subs(a2,c2,x); Bs=subs(b,c); B2=subs(b2,c2)
                                for Sa in A:
                                    for Sa2 in A2:
                                        u=F|Sa|Sa2
                                        if len(u)>t+1: continue
                                        for Sb in Bs:
                                            for Sb2 in B2:
                                                if len(u|Sb|Sb2)<=t+1:
                                                    s=len(Sa|Sa2|Sb|Sb2)
                                                    if e3 is None or s<e3: e3=s
    return e2,e3
for ln in open(sys.argv[1]):
    t,B,F=ln.split(); 
    e2,e3=lost(ids(B),ids(F),int(t))
    print(t,B[:30],'LOST=',(e2 is not None or e3 is not None),'E2 min stones',e2,'E3 min stones',e3)

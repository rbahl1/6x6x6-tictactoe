# Proposition 4.22 (busy sets with at most 17 cells) and the corrected Figure 2; own geometry (geom.py).
import itertools
from geom import *
busy=[c for c in range(216) if len(THRU[c])==7]; assert len(busy)==24
def kind(l):
    cs=[cc(c) for c in l]; return sum(len({c[i] for c in cs})>1 for i in range(3))
fd=[l for l in LINES if kind(l)==2]; assert len(fd)==36
edges=[tuple(sorted(c for c in l if c in busy)) for l in fd]
assert all(len(e)==2 for e in edges) and len(set(edges))==36
# components of the graph (busy cells, face diagonals)
adj={b:set() for b in busy}
for u,v in edges: adj[u].add(v); adj[v].add(u)
seen=set(); comps=[]
for b in busy:
    if b in seen: continue
    st=[b]; comp=set()
    while st:
        u=st.pop()
        if u in comp: continue
        comp.add(u); st+=list(adj[u])
    seen|=comp; comps.append(sorted(comp))
print('components:',[len(c) for c in comps], 'each complete:',all(all(v in adj[u] for u in c for v in c if u!=v) for c in comps))
lam=lambda i: min(i,5-i)
for c in comps:
    lv={lam(cc(x)[0]) for x in c}; par={sum(v==min(v,5-v) for v in cc(x))%2 for x in c}
    assert len(lv)==1 and len(par)==1
print('min vertex cover = sum over K4 of 3 =',3*len(comps), ' number of minimum covers =',4**len(comps))
# triangle of the proof for every face diagonal and every admissible (r,s), both types
cnt=0
for l in fd:
    cs=sorted(cc(c) for c in l)
    const=[i for i in range(3) if len({c[i] for c in cs})==1][0]; v=cs[0][const]
    act=[i for i in range(3) if i!=const]
    vals=[i for i in range(6) if lam(i)!=lam(v)]
    for r in vals:
        for s in vals:
            if s in (r,5-r): continue
            def cell(p,q):
                c=[None]*3; c[const]=v; c[act[0]]=p; c[act[1]]=q; return cid(tuple(c))
            a=frozenset(cell(t,r) for t in range(6)); b=frozenset(cell(s,t) for t in range(6))
            assert a in LSET and b in LSET and not any(x in busy for x in a|b)
            pts={next(iter(l&a)),next(iter(l&b)),next(iter(a&b))}
            assert len(l&a)==1 and len(l&b)==1 and len(a&b)==1 and len(pts)==3
            cnt+=1
print('triangles of the proof checked:',cnt)
# figure: v=1, d=(t,t,1), r=2, s=0, and the other diagonal (t,5-t,1)
C=lambda *c: cid(c)
for d in (frozenset(C(t,t,1) for t in range(6)), frozenset(C(t,5-t,1) for t in range(6))):
    a=frozenset(C(t,2,1) for t in range(6)); b=frozenset(C(0,t,1) for t in range(6))
    print('figure: meeting cells',[cc(next(iter(x))) for x in (d&a,d&b,a&b)])
print('busy cells in plane z=1:',[cc(c) for c in busy if cc(c)[2]==1])

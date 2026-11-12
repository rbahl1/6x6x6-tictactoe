# referee X: Proposition 4.22 (busy sets with at most 17 cells leave a live triangle) -- exhaustive over all 2^24 busy sets
import itertools, numpy as np, time
from x_geom import *
t0=time.time()
bidx={c:i for i,c in enumerate(BUSY)}
def bmask(l): return sum(1<<bidx[c] for c in LINES[l] if c in bidx)
# all triangles: three lines meeting pairwise in three distinct cells
meet=[[None]*NL for _ in range(NL)]
for a in range(NL):
    for b in range(a+1,NL):
        m=LMASK[a]&LMASK[b]
        if m:
            assert m.bit_count()==1
            meet[a][b]=meet[b][a]=m.bit_length()-1
tri=[]; star=0; path=0
for a in range(NL):
    for b in range(a+1,NL):
        if meet[a][b] is None: continue
        for c in range(b+1,NL):
            if meet[a][c] is None or meet[b][c] is None: continue
            if len({meet[a][b],meet[a][c],meet[b][c]})==3: tri.append((a,b,c))
            else:
                assert meet[a][b]==meet[a][c]==meet[b][c]; star+=1
print("triangles:",len(tri),"stars:",star)
for T in tri[:2000]:
    U=LMASK[T[0]]|LMASK[T[1]]|LMASK[T[2]]
    assert U.bit_count()==15
masks=set(bmask(a)|bmask(b)|bmask(c) for a,b,c in tri)
print("distinct busy-masks of triangles:",len(masks),"min popcount",min(bin(m).count('1') for m in masks))
kinds={}
for T in tri:
    k=tuple(sorted(kind(l) for l in T)); kinds[k]=kinds.get(k,0)+1
print("triangles by kinds of lines (1 axis,2 face diag,3 space diag):",kinds)
# g[C]=1 iff some triangle mask is contained in C  (C = set of busy cells NOT in B)
g=np.zeros(1<<24,dtype=np.uint8)
g[np.array(sorted(masks),dtype=np.int64)]=1
g=g.reshape([2]*24)
for ax in range(24):
    sl0=[slice(None)]*24; sl1=[slice(None)]*24
    sl0[ax]=0; sl1[ax]=1
    g[tuple(sl1)]|=g[tuple(sl0)]
g=g.reshape(-1)
# popcount of all 24-bit numbers
pc=np.zeros(1<<24,dtype=np.uint8)
idx=np.arange(1<<24,dtype=np.uint32)
for b in range(24): pc+=((idx>>b)&1).astype(np.uint8)
# B = complement of C ; B has no live triangle iff g[C]==0
bad=(g==0)
sizeB=24-pc[bad]
print("smallest busy set without a live triangle has",int(sizeB.min()),"cells; number of such minimal sets:",int((sizeB==sizeB.min()).sum()))
for k in range(0,25):
    tot=int((pc==24-k).sum()); nb=int((sizeB==k).sum())
    if nb or k<=18: print("  |B|=%2d: %8d sets, %7d without a live triangle"%(k,tot,nb))
assert int(sizeB.min())==18
# every B with |B|<=17 leaves a live triangle  (exhaustive)
assert not bad[pc>=7].any()
# first step of the proof: 36 face diagonals <-> pairs inside 6 classes of 4
fd=[l for l in range(NL) if kind(l)==2]; assert len(fd)==36
pairs=set()
for l in fd:
    bc=[c for c in LINES[l] if c in bidx]; assert len(bc)==2
    p,q=cell(bc[0]),cell(bc[1])
    assert lam(p[0])==lam(q[0]) and sum(1 for i in range(3) if p[i]!=q[i])==2
    pairs.add(frozenset(bc))
assert len(pairs)==36
classes={}
for c in BUSY:
    x=cell(c); j=lam(x[0]); par=sum(1 for v in x if v==j)%2
    classes.setdefault((j,par),[]).append(c)
assert len(classes)==6 and all(len(v)==4 for v in classes.values())
cp=set(frozenset(p) for v in classes.values() for p in itertools.combinations(v,2))
assert cp==pairs
# 18-set of the remark: three cells from each class meets every face diagonal
B18=[c for v in classes.values() for c in v[:3]]
Bm=mask(B18); assert all(LMASK[l]&Bm for l in fd)
Lv=live(Bm); lt=[T for T in tri if all(l in Lv for l in T)]
print("the 18-set of the remark (3 cells of each class): live lines",len(Lv),"live triangles",len(lt))
# axis lines without busy cell: 72 ; figure 2
ax0=[l for l in range(NL) if kind(l)==1 and not any(c in bidx for c in LINES[l])]; assert len(ax0)==72
def L(pts): return LINES.index(tuple(sorted(cid(p) for p in pts)))
d=L([(t,t,1) for t in range(6)]); a=L([(t,2,1) for t in range(6)]); b=L([(0,t,1) for t in range(6)]); d2=L([(t,5-t,1) for t in range(6)])
assert a in ax0 and b in ax0
assert {meet[d][a],meet[d][b],meet[a][b]}=={cid((2,2,1)),cid((0,0,1)),cid((0,2,1))}
assert len({meet[d2][a],meet[d2][b],meet[a][b]})==3 and meet[d2][a]==cid((3,2,1)) and meet[d2][b]==cid((0,5,1))
assert sorted(c for c in BUSY if cell(c)[2]==1)==sorted(cid((x,y,1)) for x in (1,4) for y in (1,4))
print("Figure 2: d,a,b and the other diagonal: triangles, meeting cells as in the caption: ok")
print("time %.1fs"%(time.time()-t0))

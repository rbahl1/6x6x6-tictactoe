# referee X: own geometry of the 6x6x6 board (written from Section 2 of the paper only)
import itertools
N=6
CELLS=[(x,y,z) for x in range(N) for y in range(N) for z in range(N)]
def cid(c): return 36*c[0]+6*c[1]+c[2]
def cell(i): return (i//36,(i//6)%6,i%6)
def params():
    comps=[('c',v) for v in range(N)]+[('i',0),('r',0)]
    for f in itertools.product(comps,repeat=3):
        if all(k=='c' for k,_ in f): continue
        yield f
def ev(f,t):
    return tuple(v if k=='c' else (t if k=='i' else N-1-t) for k,v in f)
_L={}
for f in params():
    s=frozenset(cid(ev(f,t)) for t in range(N))
    _L.setdefault(s,[]).append(f)
assert len(_L)==148 and all(len(v)==2 for v in _L.values())
LINES=sorted(tuple(sorted(s)) for s in _L)          # each line: sorted tuple of ids
LMASK=[sum(1<<c for c in l) for l in LINES]
NL=len(LINES)
THRU=[[] for _ in range(216)]
for i,l in enumerate(LINES):
    for c in l: THRU[c].append(i)
def kind(i):
    f=_L[frozenset(LINES[i])][0]
    return sum(1 for k,_ in f if k!='c')
lam=lambda v: min(v,5-v)
BUSY=[cid(c) for c in CELLS if lam(c[0])==lam(c[1])==lam(c[2])]
assert len(BUSY)==24 and all(len(THRU[c])==7 for c in BUSY)
assert sorted(len(t) for t in THRU).count(7)==24 and sum(len(t) for t in THRU)==888
def mask(S):
    m=0
    for c in S: m|=1<<c
    return m
def cells_of(m):
    out=[]; i=0
    while m:
        if m&1: out.append(i)
        m>>=1; i+=1
    return out
# group G: Sigma_s o P_r o Pi_pi
def _H():
    out=[]
    for p in itertools.permutations(range(6)):
        if all(p[5-i]==5-p[i] for i in range(6)): out.append(p)
    return out
H=_H(); assert len(H)==48
GROUP=set()
for s in H:
    for r in itertools.product((0,1),repeat=3):
        for pi in itertools.permutations(range(3)):
            img=[]
            for c in CELLS:
                d=[None]*3
                for i in range(3): d[pi[i]]=c[i]          # Pi_pi: coordinate i moves to position pi(i)
                d=[5-d[i] if r[i] else d[i] for i in range(3)]
                d=[s[v] for v in d]
                img.append(cid(tuple(d)))
            GROUP.add(tuple(img))
GROUP=sorted(GROUP); assert len(GROUP)==1152
_ls=set(LINES)
for g in GROUP[:50]+GROUP[-50:]:
    assert all(tuple(sorted(g[c] for c in l)) in _ls for l in LINES)
def canon(B,F):
    """B,F iterables of ids; returns (sorted tuple gB, sorted tuple gF) minimal, comparing B first; and list of maps attaining it"""
    best=None; arg=[]
    B=list(B); F=list(F)
    for g in GROUP:
        k=(tuple(sorted(g[c] for c in B)),tuple(sorted(g[c] for c in F)))
        if best is None or k<best: best=k; arg=[g]
        elif k==best: arg.append(g)
    return best,arg
def live(Bm): return [i for i in range(NL) if not (LMASK[i]&Bm)]
def admissible(Bm,Xm):
    if Bm&Xm: return False
    Lv=live(Bm)
    four=[]
    for i in Lv:
        n=(LMASK[i]&Xm).bit_count()
        if n>=5: return False
        if n==4: four.append(i)
    for a in range(len(four)):
        for b in range(a+1,len(four)):
            m=LMASK[four[a]]&LMASK[four[b]]
            if m and not (m&Xm): return False
    return True
if __name__=="__main__":
    print("geometry ok: 148 lines, 1152 maps, 24 busy cells")

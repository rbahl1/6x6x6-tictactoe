# Own geometry for the worked example of Section 5.2 (reviser M). Shares no code with the project.
import itertools
N=6
def cid(c): return 36*c[0]+6*c[1]+c[2]
def cc(i): return (i//36, (i//6)%6, i%6)
def gen_lines():
    L=set()
    for f in itertools.product(range(N+2), repeat=3):   # v<N: constant v; N: identity; N+1: reversal
        if all(v<N for v in f): continue
        ev=lambda v,t: v if v<N else (t if v==N else N-1-t)
        L.add(frozenset(cid(tuple(ev(v,t) for v in f)) for t in range(N)))
    return sorted(L, key=lambda l: sorted(l))
LINES=gen_lines(); assert len(LINES)==148
THRU=[[l for l in LINES if c in l] for c in range(216)]
assert sorted(len(t) for t in THRU).count(7)==24
for a,b in itertools.combinations(LINES,2): assert len(a&b)<=1
def group():
    H=[]
    for s0 in range(6):
        for s1 in range(6):
            for s2 in range(6):
                s=[None]*6; ok=True
                for i,v in ((0,s0),(1,s1),(2,s2)):
                    s[i]=v; s[5-i]=5-v
                if sorted(s)==list(range(6)): H.append(tuple(s))
    assert len(H)==48
    G=set()
    for s in H:
        for r in itertools.product((0,1),repeat=3):
            for p in itertools.permutations(range(3)):
                img=[]
                for i in range(216):
                    c=cc(i)
                    d=[None]*3
                    for k in range(3): d[p[k]]=c[k]          # Pi_pi: coordinate k moves to position p[k]
                    d=[5-d[k] if r[k] else d[k] for k in range(3)]   # P_r
                    d=[s[v] for v in d]                      # Sigma_s
                    img.append(cid(tuple(d)))
                G.add(tuple(img))
    return sorted(G)
G=group(); assert len(G)==1152
LSET=set(LINES)
for g in G[:40]:
    for l in LINES[:40]: assert frozenset(g[c] for c in l) in LSET
def canon(B,F):
    best=None; bg=None
    for g in G:
        k=(tuple(sorted(g[c] for c in B)), tuple(sorted(g[c] for c in F)))
        if best is None or k<best: best=k; bg=g
    return best,bg
def live(B): return [l for l in LINES if not (l & B)]
def admissible(B,X):
    B=frozenset(B); X=frozenset(X)
    if B & X: return False
    Lv=live(B)
    four=[]
    for l in Lv:
        n=len(l & X)
        if n>=5: return False
        if n==4: four.append(l)
    for a,b in itertools.combinations(four,2):
        c=a&b
        if c and not (c & X): return False
    return True
def children_P(B,F,t):
    """all data of children (c) and (d) of P(B,F,t), literally from Definition 5.1"""
    B=frozenset(B); F=frozenset(F); Lv=live(B); out=[]
    for l in Lv:
        for e in l-F:
            Fn=F|(l-{e})
            if len(Fn)<=t+1: out.append(('c',(l,e),(B|{e},Fn,t+1)))
    for a,b in itertools.combinations(Lv,2):
        cset=a&b
        if not cset: continue
        (c,)=cset
        if c in F: continue
        for Qa in itertools.combinations(sorted(a-{c}),4):
            Qa=frozenset(Qa)
            if not (F&a)<=Qa: continue
            for Qb in itertools.combinations(sorted(b-{c}),4):
                Qb=frozenset(Qb)
                if not (F&b)<=Qb: continue
                Fn=F|Qa|Qb
                if len(Fn)<=t+1: out.append(('d',(a,b,Qa,Qb),(B|{c},Fn,t+1)))
    return out
def load(tbl,lv):
    T={}; Lf=set()
    for line in open(tbl):
        t,B,F,m=line.split()
        key=(int(t), tuple(int(x) for x in B.split(',')) if B!='-' else (), tuple(int(x) for x in F.split(',')) if F!='-' else ())
        assert key not in T; T[key]=int(m)
    for line in open(lv):
        t,B,F=line.split()
        Lf.add((int(t), tuple(int(x) for x in B.split(',')) if B!='-' else (), tuple(int(x) for x in F.split(',')) if F!='-' else ()))
    return T,Lf

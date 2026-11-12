# referee X: own augmenting-path code and Algorithm 2 as printed (copied from x_leaf.py)
import itertools
from x_geom import *
# ---------- flows (own augmenting-path code; lines are indices into Lv)
class Net:
    def __init__(self,Bm,Fm):
        self.Lv=live(Bm); self.n=len(self.Lv); self.Fm=Fm
        self.cells=[[c for c in LINES[l] if not Fm>>c&1] for l in self.Lv]
        self.own={}
    def augment(self,src):
        par={}; seenl=set(src); q=list(src); qi=0; seenc=set()
        while qi<len(q):
            l=q[qi]; qi+=1
            for c in self.cells[l]:
                if c in seenc: continue
                o=self.own.get(c)
                if o==l: continue
                seenc.add(c); par[c]=l
                if o is None:
                    cur=c
                    while True:
                        pl=par[cur]; self.own[cur]=pl
                        pc=par.get(('L',pl))
                        if pc is None: break
                        cur=pc
                    return True
                if o not in seenl:
                    seenl.add(o); par[('L',o)]=c; q.append(o)
        self.reached=q
        return False
    def pairing(self):
        for l in range(self.n):
            for _ in range(2):
                if not self.augment([l]): return False
        return True
    def gain(self,U,theta,exhaust=False):
        orig=self.own; self.own=dict(orig); g=0          # work on a copy; the stored assignment is not changed
        while (exhaust or g<theta) and self.augment(list(U)): g+=1
        fin=self.own; self.own=orig
        return g,fin
def fadj(net):
    n=net.n; A=[[False]*n for _ in range(n)]
    for i in range(n):
        si=set(net.cells[i])
        for j in range(i+1,n):
            if si&set(net.cells[j]): A[i][j]=A[j][i]=True
    return A
def alg2(Bm,Fm,k):
    """Algorithm 2 exactly as printed (pruning by strong lines only)"""
    if not admissible(Bm,Fm): return (False,1)
    net=Net(Bm,Fm)
    if not net.pairing(): return (False,2)
    theta=k-Fm.bit_count()
    if theta<=0: return (True,0)
    Lw=[l for l in range(net.n) if net.gain([l],theta)[0]<theta]
    A=fadj(net); done=set()
    for l2 in Lw:
        nb=[l for l in Lw if A[l][l2]]
        for l1,l3 in itertools.combinations(nb,2):
            T=frozenset((l1,l2,l3))
            if T in done: continue
            done.add(T)
            if net.gain(T,theta)[0]<theta: return (False,3)
    return (True,0)

# referee X: Algorithm 2, the variant with deleted strong lines (Lemma 7.5, steps (1)-(4) of S4.1) and
# verify/treecheck.cpp --leafcheck against a brute-force evaluation of Definition 4.6 over ALL sets of
# live lines, on small instances of the real geometry.  Own code throughout.
import random, subprocess, sys, time, itertools
from x_geom import *
t0=time.time()
rnd=random.Random(7052026)
# ---------- brute force over all subsets of live lines
class Brute:
    def __init__(self,Bm,Fm):
        self.Bm=Bm; self.Fm=Fm; self.Lv=live(Bm); n=len(self.Lv); self.n=n
        self.adm=admissible(Bm,Fm)
        nf=~Fm
        lm=[LMASK[l]&nf for l in self.Lv]
        adj=[0]*n
        for i in range(n):
            for j in range(n):
                if i!=j and lm[i]&lm[j]: adj[i]|=1<<j
        self.adj=adj
        N=[0]*(1<<n); sur=[0]*(1<<n); conn=[False]*(1<<n)
        for S in range(1,1<<n):
            low=(S&-S).bit_length()-1
            N[S]=N[S&(S-1)]|lm[low]
            sur[S]=N[S].bit_count()-2*S.bit_count()
            # F-connected?
            seen=1<<low; fr=seen
            while fr:
                i=(fr&-fr).bit_length()-1; fr&=fr-1
                new=adj[i]&S&~seen; seen|=new; fr|=new
            conn[S]=(seen==S)
        self.sur=sur; self.conn=conn
        self.pairing=all(sur[S]>=0 for S in range(1,1<<n))          # Hall's condition, literally
        ph=[sur[S] for S in range(1,1<<n) if conn[S] and S.bit_count()>=3]
        self.Phi=min(ph) if ph else None                            # None = infinity
        # mu(U) = min sur over supersets: superset-min transform
        mu=sur[:]; mu[0]=0
        for b in range(n):
            bit=1<<b
            for S in range(1<<n):
                if not S&bit and mu[S|bit]<mu[S]: mu[S]=mu[S|bit]
        self.mu=mu
    def leaf(self,theta):
        if not self.adm: return (False,1)
        if not self.pairing: return (False,2)
        if self.Phi is not None and self.Phi<theta: return (False,3)
        return (True,0)
    def muD(self,T,D):
        """min sur_F(S), T <= S <= L - D   (brute force)"""
        n=self.n; free=((1<<n)-1)&~D&~T; best=None; sub=free
        while True:
            S=T|sub
            if S:
                v=self.sur[S]
                if best is None or v<best: best=v
            if sub==0: break
            sub=(sub-1)&free
        return best
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
STAT={'dele':0,'T_checked':0,'valid':0,'quick':0,'pairflows':0,'tripleflows':0,'false_at_triple':0,'true_lt3weak':0,'true_after_triples':0}
def validate(net,fin,T,D,theta):
    """the properties listed after Lemma 7.5"""
    cnt=[0]*net.n
    for c,l in fin.items():
        assert c in LINES[net.Lv[l]] and not net.Fm>>c&1
        cnt[l]+=1
    for l in range(net.n):
        if l in D: assert cnt[l]==0 and l not in T
        elif l in T: assert cnt[l]>=2
        else: assert cnt[l]==2
    assert sum(cnt[l] for l in T)==2*len(T)+theta
    STAT['valid']+=1
def main_variant(Bm,Fm,k,br=None):
    """steps (1)-(4) of Appendix S4.1 (the leaf test of the main verifier), with deletion of strong lines.
    If br (brute force) is given, Lemma 7.5 is asserted at every flow."""
    if not admissible(Bm,Fm): return (False,1)
    net=Net(Bm,Fm)
    if not net.pairing(): return (False,2)
    theta=k-Fm.bit_count()
    if theta<=0: return (True,0)
    n=net.n; A=fadj(net)
    # (1) quick rejection: F-components
    comp=[-1]*n
    for i in range(n):
        if comp[i]<0:
            st=[i]; comp[i]=i
            while st:
                u=st.pop()
                for v in range(n):
                    if A[u][v] and comp[v]<0: comp[v]=i; st.append(v)
    for r in set(comp):
        S=[l for l in range(n) if comp[l]==r]
        if len(S)>=3:
            N=0
            for l in S: N|=LMASK[net.Lv[l]]
            if (N&~Fm).bit_count()-2*len(S)<theta:
                STAT['quick']+=1; return (False,3)
    # (2) single lines: backward search from the unassigned cells
    owner=net.own
    reach=[]; inr=[False]*n
    for l in range(n):
        if any(c not in owner for c in net.cells[l]): inr[l]=True; reach.append(l)
    h=0
    while h<len(reach):
        o=reach[h]; h+=1
        for c in net.cells[o]:
            if owner.get(c)!=o: continue
            for l in range(n):
                if l!=o and not inr[l] and c in net.cells[l]: inr[l]=True; reach.append(l)
    D=set(); weak=[True]*n
    def bmaskT(T): return sum(1<<l for l in T)
    def check75(T,g):
        if br is None: return
        Dm=bmaskT(D); Tm=bmaskT(T)
        assert all(br.mu[1<<d]>=theta for d in D)                       # every deleted line is strong
        assert min(theta,g)==min(theta,br.mu[Tm])                       # verdict relative to D = true verdict
        ge,_=net.gain(T,theta,exhaust=True)
        assert ge==br.muD(Tm,Dm)                                        # Lemma 7.5 (b), second assertion
        assert min(theta,br.mu[Tm])==min(theta,br.muD(Tm,Dm))           # Lemma 7.5 (b), first assertion
        STAT['T_checked']+=1
    for l in reach:
        if len(net.cells[l])-2<theta: continue
        g,fin=net.gain([l],theta)
        check75([l],g)
        if theta==1: assert g==1
        if g<theta: continue
        validate(net,fin,{l},D,theta)
        weak[l]=False
        for c in [c for c in net.cells[l] if owner.get(c)==l]: del owner[c]
        D.add(l); STAT['dele']+=1
    for l in range(n):
        if not inr[l]:
            g,_=net.gain([l],1); assert g==0
            if br is not None: assert br.mu[1<<l]==0
    W=[l for l in range(n) if weak[l]]
    if len(W)<3:
        STAT['true_lt3weak']+=1; return (True,0)
    # (3) pairs
    strongpair=set()
    for a,b in itertools.combinations(W,2):
        if not A[a][b]: continue
        if len(set(net.cells[a])|set(net.cells[b]))-4<theta: continue
        g,fin=net.gain([a,b],theta); check75([a,b],g); STAT['pairflows']+=1
        if g>=theta: validate(net,fin,{a,b},D,theta); strongpair.add((a,b))
    # (4) triples
    done=set()
    for b in W:
        nb=[a for a in W if A[a][b] and (min(a,b),max(a,b)) not in strongpair]
        for a,c in itertools.combinations(nb,2):
            if A[a][c] and (min(a,c),max(a,c)) in strongpair: continue
            T=frozenset((a,b,c))
            if T in done: continue
            done.add(T)
            g,fin=net.gain(T,theta); check75(list(T),g); STAT['tripleflows']+=1
            if g<theta:
                # the reached set is a literal violation of (L3)
                STAT['false_at_triple']+=1
                return (False,3)
            validate(net,fin,set(T),D,theta)
    STAT['true_after_triples']+=1
    return (True,0)
# ---------- instances with few live lines on the real board
def plane_lines():
    out=[]
    for ax in range(3):
        for v in range(6):
            out.append([i for i in range(NL) if all(cell(c)[ax]==v for c in LINES[i])])
    return out
PL=plane_lines(); assert all(len(p)==14 for p in PL)
def instance(maxlive=14):
    while True:
        mode=rnd.randrange(6)
        if mode==0:   base=rnd.sample(rnd.choice(PL),rnd.randint(4,14))
        elif mode==1:
            c=rnd.randrange(216); base=list(THRU[c])
            for _ in range(rnd.randint(1,6)):
                c2=rnd.choice(LINES[rnd.choice(base)]); base.append(rnd.choice(THRU[c2]))
        elif mode==2: base=rnd.sample(rnd.choice(PL),rnd.randint(3,8))+rnd.sample(rnd.choice(PL),rnd.randint(2,6))
        elif mode==3: base=rnd.sample(range(NL),rnd.randint(4,10))
        elif mode==4: base=list(rnd.choice(PL))
        else:
            c=rnd.choice(BUSY); base=list(THRU[c])+rnd.sample(rnd.choice(PL),rnd.randint(2,6))
        U=0
        for l in set(base): U|=LMASK[l]
        Bm=((1<<216)-1)&~U
        for c in rnd.sample(cells_of(U),rnd.randint(0,3)): Bm|=1<<c
        Lv=live(Bm)
        if not (3<=len(Lv)<=maxlive): continue
        free=[c for c in cells_of(U) if not Bm>>c&1]
        F=0
        for c in rnd.sample(free,min(len(free),rnd.randint(0,14))):
            if rnd.random()<0.93 and not admissible(Bm,F|1<<c): continue
            F|=1<<c
        return Bm,F
def ids(m): return ",".join(str(c) for c in cells_of(m)) if m else "-"
NI=int(sys.argv[1]) if len(sys.argv)>1 else 700
inst=[]; res={}; cnt={}; NLV=[]
lines_out=[]
for it in range(NI):
    Bm,Fm=instance()
    br=Brute(Bm,Fm); NLV.append(br.n)
    top=(br.Phi if br.Phi is not None else 3)+1
    thetas=sorted(set([0,1,2,3]+[max(1,top-2),max(1,top-1),top])) if br.adm and br.pairing else [1]
    thetas=[th for th in thetas if 0<=th<=9]
    for th in thetas:
        k=Fm.bit_count()+th
        want=br.leaf(th)
        a2=alg2(Bm,Fm,k)
        mv=main_variant(Bm,Fm,k,br)
        assert a2==want, ("alg2",ids(Bm),ids(Fm),k,a2,want)
        assert mv==want, ("variant",ids(Bm),ids(Fm),k,mv,want)
        cnt[want]=cnt.get(want,0)+1
        lines_out.append(("%d %s %s"%(k,ids(Bm),ids(Fm)),want))
    # Lemma 7.5 (b) directly: random D inside the strong lines, random T outside D
    if br.adm and br.pairing:
        net=Net(Bm,Fm); assert net.pairing()
        for th in (1,2,3):
            strong=[l for l in range(br.n) if br.mu[1<<l]>=th]
            for _ in range(4):
                D=set(rnd.sample(strong,rnd.randint(0,len(strong))))
                rest=[l for l in range(br.n) if l not in D]
                if not rest: continue
                T=rnd.sample(rest,min(len(rest),rnd.randint(1,3)))
                net2=Net(Bm,Fm); net2.own={c:l for c,l in net.own.items() if l not in D}
                g,_=net2.gain(T,99,exhaust=True)
                Tm=sum(1<<l for l in T); Dm=sum(1<<l for l in D)
                assert g==br.muD(Tm,Dm), "Lemma 7.5 (b): augmentations in the network without D"
                assert min(th,br.mu[Tm])==min(th,br.muD(Tm,Dm)), "Lemma 7.5 (b): min(theta,mu)"
                cnt['75b']=cnt.get('75b',0)+1
                # the hypothesis is needed: with a non-strong line in D the equality can fail
                weakl=[l for l in range(br.n) if br.mu[1<<l]<th and l not in T]
                if weakl:
                    D2=D|{rnd.choice(weakl)}; D2m=sum(1<<l for l in D2)
                    if not (Tm&D2m) and min(th,br.mu[Tm])!=min(th,br.muD(Tm,D2m)): cnt['75b_hyp_needed']=cnt.get('75b_hyp_needed',0)+1
print("instances:",NI," evaluations by verdict (holds, failing condition):",{str(k):v for k,v in cnt.items() if isinstance(k,tuple)})
print("live lines per instance: min %d max %d mean %.1f"%(min(NLV),max(NLV),sum(NLV)/len(NLV)))
print("variant: ",STAT)
print("Algorithm 2 as printed == brute force; variant with deleted lines (S4.1 steps 1-4) == brute force: all agree")
print("Lemma 7.5 asserted at %d flows of the variant (%d deletions, %d validated assignments); direct tests of 7.5(b): %d; "
      "cases in which 7.5(b) fails when D contains a line that is not strong: %d"%(STAT['T_checked'],STAT['dele'],STAT['valid'],cnt.get('75b',0),cnt.get('75b_hyp_needed',0)))
# treecheck --leafcheck on the same instances
inp="\n".join(s for s,_ in lines_out)+"\n"
p=subprocess.run(["/home/user/ttt6/bin/treecheck","--leafcheck"],input=inp,capture_output=True,text=True)
import json
outs=[json.loads(x) for x in p.stdout.strip().split("\n")]
assert len(outs)==len(lines_out)
mism=0
for o,(s,want) in zip(outs,lines_out):
    if (o["leaf"],o["reason"])!=want: mism+=1; print("MISMATCH",s,o,want)
print("bin/treecheck --leafcheck (default = paranoid) on the same %d evaluations: %d mismatches in verdict or failing condition"%(len(outs),mism))
print(p.stderr.strip())
print("time %.1fs"%(time.time()-t0))

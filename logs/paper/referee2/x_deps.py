# referee X: dependency chain of Theorem 1.1 and of Corollary 9.3, extracted from the LaTeX sources.
# For every theorem-like statement: the labels cited in the statement and in its proof.  Closure from thm:main / cor:seven.
import re, sys, collections
P="/home/user/ttt6/paper/"
order=[l.split('{')[1].split('}')[0] for l in open(P+"paper.tex") if l.strip().startswith("\\input{") and 'macros' not in l]
files=[]
for f in order:
    files.append(f)
    if f=="supp_math": files+= ["app_extra_A","app_extra_B"]
# supp_math inputs app_extra_A / app_extra_B in the middle; handle by textual inclusion
def load(f):
    s=open(P+f+".tex").read()
    s=re.sub(r"(?m)(?<!\\)%.*$","",s)
    return re.sub(r"\\input\{(app_extra_[AB])\}",lambda m: load(m.group(1)),s)
SUPP={"supp_front","supp_math","supp_search","supp_checks","app_ip","supp_repro","supp_tables"}
envs="theorem|lemma|proposition|corollary|fact|definition|remark|example"
stm={}      # label -> dict(kind,file,supp,refs(set),proofrefs(set))
where={}    # every label -> (file, supp)
aux={}
for l in open(P+"paper.aux"):
    m=re.match(r"\\newlabel\{([^}@]+)\}\{\{([^}]*)\}\{(\d+)\}",l)
    if m: aux[m.group(1)]=(m.group(2),int(m.group(3)))
def refs(txt):
    out=set()
    for m in re.finditer(r"\\(?:[cC]ref|ref|eqref|[cC]refrange|pageref)\*?\{([^}]*)\}(?:\{([^}]*)\})?",txt):
        for g in m.groups():
            if g: out|={x.strip() for x in g.split(',')}
    return out
for f in [x for x in order]:
    s=load(f); supp=f in SUPP
    for m in re.finditer(r"\\label\{([^}]*)\}",s): where[m.group(1)]=(f,supp)
    toks=list(re.finditer(r"\\begin\{("+envs+r"|proof)\}(\[[^\]]*(?:\\cref\{[^}]*\}[^\]]*)?\])?|\\end\{("+envs+r"|proof)\}",s))
    last=None; i=0
    while i<len(toks):
        t=toks[i]
        if t.group(1) and t.group(1)!="proof":
            # find matching end
            j=i+1
            while not (toks[j].group(3)==t.group(1)): j+=1
            body=s[t.end():toks[j].start()]
            labs=re.findall(r"\\label\{([^}]*)\}",body[:400])
            lab=labs[0] if labs else None
            if lab:
                # labels of items/equations inside the statement belong to it
                stm[lab]=dict(kind=t.group(1),file=f,supp=supp,refs=refs(body)-set(labs),proofrefs=set(),sub=set(re.findall(r"\\label\{([^}]*)\}",body)))
                last=lab
            i=j+1; continue
        if t.group(1)=="proof":
            j=i+1; depth=1
            while True:
                if toks[j].group(1)=="proof": depth+=1
                if toks[j].group(3)=="proof":
                    depth-=1
                    if depth==0: break
                j+=1
            body=s[t.end():toks[j].start()]
            tgt=last
            if t.group(2):
                r=refs(t.group(2))
                if r: tgt=sorted(r)[0]
            if tgt in stm:
                stm[tgt]['proofrefs']|=refs(body); stm[tgt]['sub']|=set(re.findall(r"\\label\{([^}]*)\}",body))
            i=j+1; continue
        i+=1
# map sub-labels (items, equations, algorithm lines) to their statement
owner={}
for lab,d in stm.items():
    for x in d['sub']: owner.setdefault(x,lab)
def norm(x): return owner.get(x,x)
def closure(root,use_statement_refs=True):
    seen=[]; st=[root]; edges=collections.defaultdict(set)
    while st:
        u=st.pop()
        if u in seen: continue
        seen.append(u)
        if u not in stm: continue
        d=stm[u]
        rs=set(d['proofrefs'])|(set(d['refs']) if use_statement_refs else set())
        for v in rs:
            v=norm(v)
            if v==u: continue
            edges[u].add(v)
            if v in stm and stm[v]['kind']!='remark' and stm[v]['kind']!='example': st.append(v)
    return seen,edges
def num(l): return aux.get(l,("?",0))
def report(root,title):
    seen,edges=closure(root)
    print("=== "+title)
    facts=[]; supp=[]
    for u in sorted(seen,key=lambda l:(num(l)[1],l)):
        if u not in stm: continue
        d=stm[u]
        print("  %-12s %-6s p.%-3d %-18s %-22s uses: %s"%(d['kind'],num(u)[0],num(u)[1],d['file'],u,", ".join(sorted(norm(v) for v in (d['proofrefs']|d['refs']) if norm(v)!=u and (norm(v) in stm)))))
        if d['kind']=='fact': facts.append(u)
        if d['supp']: supp.append(u)
    # every label referenced anywhere in the closure that lives in the supplement
    suppref=set()
    for u in seen:
        if u in stm:
            for v in stm[u]['proofrefs']|stm[u]['refs']:
                if v in where and where[v][1]: suppref.add((u,v))
    print("  statements in the closure: %d;  Computed Facts: %s;  statements of the supplement in the closure: %s"%(len([u for u in seen if u in stm]),facts,supp))
    print("  references from statements/proofs of the closure to labels of the supplement (pointers only?):",sorted(suppref))
    print("  max page of a statement in the closure:",max(num(u)[1] for u in seen if u in stm))
    # circularity among proofs: strongly connected components of the graph restricted to proofrefs of theorem-like statements
    g={u:{norm(v) for v in stm[u]['proofrefs'] if norm(v) in stm and stm[norm(v)]['kind'] in('theorem','lemma','proposition','corollary','fact') and norm(v)!=u} for u in seen if u in stm and stm[u]['kind'] in ('theorem','lemma','proposition','corollary','fact')}
    idx={}; low={}; stack=[]; on=set(); sccs=[]; c=[0]
    sys.setrecursionlimit(10000)
    def dfs(u):
        idx[u]=low[u]=c[0]; c[0]+=1; stack.append(u); on.add(u)
        for v in g.get(u,()):
            if v not in g: continue
            if v not in idx: dfs(v); low[u]=min(low[u],low[v])
            elif v in on: low[u]=min(low[u],idx[v])
        if low[u]==idx[u]:
            comp=[]
            while True:
                w=stack.pop(); on.discard(w); comp.append(w)
                if w==u: break
            if len(comp)>1: sccs.append(comp)
    for u in g:
        if u not in idx: dfs(u)
    print("  cycles among the proofs (strongly connected components with more than one statement):",sccs if sccs else "none")
    for comp in sccs:
        for u in comp:
            print("     ",u,"-> ",sorted(v for v in g[u] if v in comp))
report("thm:main","Theorem 1.1 (thm:main)")
report("cor:seven","Corollary 9.3 (cor:seven)")
# all labels referenced in the main part that are defined in the supplement, by file (to see that they are pointers)
print("=== undefined or dangling: labels referenced but not defined:")
alltext="".join(load(f) for f in order)
und=sorted(r for r in refs(alltext) if r not in where and not r.startswith("S:") and r not in owner)
print("  ",und if und else "none")

# referee X: Section 9.1 / Appendix B -- own check of the 7x7x7 statements and of the printed table, then
# scripts/check_7cube_midplanes.py on the real table and on mutated copies of it.
import itertools, re, subprocess, sys, os, shutil
ok=True
def chk(cond,what):
    global ok
    print("  %-110s %s"%(what,"ok" if cond else "FAILED")); ok=ok and bool(cond)
n=7
comps=[('c',v) for v in range(n)]+[('i',0),('r',0)]
def ev(f,t): return tuple(v if k=='c' else (t if k=='i' else n-1-t) for k,v in f)
par={}
for f in itertools.product(comps,repeat=3):
    if all(k=='c' for k,_ in f): continue
    par.setdefault(frozenset(ev(f,t) for t in range(n)),[]).append(f)
chk(len(par)==193 and all(len(v)==2 for v in par.values()) and (9**3-7**3)//2==193,"193 lines, each with exactly two parametrizations; (9^3-7^3)/2 = 193")
def normalized(f):
    act=[k for k,_ in f if k!='c']; return act[0]=='i'
chk(all(sum(1 for f in v if normalized(f))==1 for v in par.values()),"exactly one of the two parametrizations of a line is normalized")
# a line determines its normalized parametrization as described in the text
good=True
for L,v in par.items():
    f=[g for g in v if normalized(g)][0]
    const=[i for i in range(3) if len({c[i] for c in L})==1]
    i0=min(i for i in range(3) if i not in const)
    rec=[None]*7
    for c in L: rec[c[i0]]=c
    good&= all(rec[t]==ev(f,t) for t in range(7)) and all(f[i]==('c',next(iter(L))[i]) for i in const)
chk(good,"the normalized parametrization is recovered from the line as in Section 9.1")
M=lambda c: 3 in c
phi={0:0,1:1,2:2,4:3,5:4,6:5}
chk(all(phi[6-i]==5-phi[i] for i in phi),"phi(6-i) = 5-phi(i) for i != 3")
# lines of the 6^3 board
c6=[('c',v) for v in range(6)]+[('i',0),('r',0)]
L6=set()
for f in itertools.product(c6,repeat=3):
    if all(k=='c' for k,_ in f): continue
    L6.add(frozenset(tuple(v if k=='c' else (t if k=='i' else 5-t) for k,v in f) for t in range(6)))
assert len(L6)==148
inM=[]; img={}
g=True
for L,v in par.items():
    has3=[any(k=='c' and val==3 for k,val in f) for f in v]
    if any(has3):
        g&=all(has3) and all(M(c) for c in L); inM.append(L)            # (a)
    else:
        g&=all([c for c in L if M(c)]==[ev(f,3)] for f in v)           # (b) first half
        im=frozenset(tuple(phi[x] for x in c) for c in L if not M(c))
        g&= im in L6 and im not in img; img[im]=L                         # (b) second half, (c) uniqueness
chk(g and len(img)==148 and len(inM)==45,"Lemma 9.1 (a),(b),(c): 45 lines inside M; the other 148 give every line of the 6^3 board exactly once")
chk(sum(1 for c in itertools.product(range(7),repeat=3) if M(c))==127,"|M| = 127, |A| = 216")
# the printed table, parsed with own code (row by row)
tex="/home/user/ttt6/paper/app_seven.tex"
def parse(path):
    rows=[]
    for ln in open(path):
        if '&' not in ln or '$(' not in ln: continue
        cellsrc=[x.strip() for x in ln.replace('\\\\','').split('&')]
        if len(cellsrc)!=6: return None
        for i in range(0,6,2):
            m=re.fullmatch(r"\$\(([^)]*)\)\$",cellsrc[i]); m2=re.fullmatch(r"\$\((\d),(\d),(\d)\),\\ \((\d),(\d),(\d)\)\$",cellsrc[i+1])
            if not m or not m2: return None
            f=m.group(1).split(',')
            line=frozenset(tuple(int(x) if x.isdigit() else (t if x=='t' else 6-t) for x in f) for t in range(7))
            if any(x not in ('t','6{-}t') and not x.isdigit() for x in f): return None
            d=[int(x) for x in m2.groups()]
            rows.append((tuple(f),line,tuple(d[:3]),tuple(d[3:]),i//2))
    return rows
T=parse(tex)
chk(T is not None and len(T)==45,"45 entries parsed from paper/app_seven.tex")
chk({l for _,l,_,_,_ in T}==set(inM) and len({l for _,l,_,_,_ in T})==45,"the 45 entries are 45 different lines = the lines inside M  (Lemma 9.1 (d))")
chk(all(c1 in l and c2 in l and c1!=c2 for _,l,c1,c2,_ in T),"both cells of an entry lie on its line")
allc=[c for _,_,c1,c2,_ in T for c in (c1,c2)]
chk(len(set(allc))==90,"the 90 cells are pairwise different  (Lemma 9.2)")
chk((3,3,3) not in allc and 127-90==37,"the centre and 36 further cells of M are not assigned")
# first active component of every printed triple is t (normalized), as the proof of Lemma 9.1 (d) says
chk(all([x for x in f if not x.isdigit()][0]=='t' for f,_,_,_,_ in T),"every printed triple is a normalized parametrization")
# counts 13 per direction of single-active lines, 6 with two active components
chk(sum(1 for f,_,_,_,_ in T if sum(not x.isdigit() for x in f)==1)==39 and sum(1 for f,_,_,_,_ in T if sum(not x.isdigit() for x in f)==2)==6,"39 + 6 entries")
# column headings: plane z=3 / y=3 / x=3 ; last row: lines through the centre
pos={0:2,1:1,2:0}
chk(all(all(c[pos[col]]==3 for c in l) for f,l,_,_,col in T),"every entry stands in the column of a mid-plane that contains its line")
# rule of the proof of Lemma 9.2 (own transcription)
psi=[0,1,2,4,5,6]; rule={}
for r in range(6):
    rule[frozenset((psi[r],t,3) for t in range(7))]={(psi[r],psi[(r+1)%6],3),(psi[r],psi[(r+2)%6],3)}
    rule[frozenset((t,psi[r],3) for t in range(7))]={(psi[(r+1)%6],psi[r],3),(psi[(r+2)%6],psi[r],3)}
rule[frozenset((t,t,3) for t in range(7))]={(0,0,3),(6,6,3)}
rule[frozenset((t,6-t,3) for t in range(7))]={(1,5,3),(5,1,3)}
z3=dict(rule)
for L,cs in z3.items():
    mv=lambda c:(c[0],3,c[1]); rule[frozenset(mv(c) for c in L)]={mv(c) for c in cs}     # coordinate 3 to the second position
    mv2=lambda c:(3,c[0],c[1]); rule[frozenset(mv2(c) for c in L)]={mv2(c) for c in cs}   # ... to the first position
rule[frozenset((t,3,3) for t in range(7))]={(0,3,3),(1,3,3)}
rule[frozenset((3,t,3) for t in range(7))]={(3,0,3),(3,1,3)}
rule[frozenset((3,3,t) for t in range(7))]={(3,3,0),(3,3,1)}
chk(len(rule)==45 and all(rule[l]=={c1,c2} for _,l,c1,c2,_ in T),"the rule in the proof of Lemma 9.2 reproduces every entry")
res=lambda c:(psi.index(c[1])-psi.index(c[0]))%6
kinds={}
for L,cs in z3.items():
    k='col' if len({c[0] for c in L})==1 else ('row' if len({c[1] for c in L})==1 else ('d1' if (0,0,3) in L else 'd2'))
    for c in cs: kinds.setdefault(k,set()).add(res(c))
chk(kinds=={'col':{1,2},'row':{5,4},'d1':{0},'d2':{3}},"residues of s-r mod 6: 1,2 / 5,4 / 0 / 3 for the four kinds of lines, as in the proof")
# ---- the script of the archive on the real table and on mutated copies
SCR="/home/user/ttt6/scripts/check_7cube_midplanes.py"
def run(path):
    p=subprocess.run([sys.executable,SCR,path],capture_output=True,text=True)
    fails=[l.strip() for l in p.stdout.split("\n") if l.strip().endswith("FAILED") and not l.startswith("RESULT")]
    return p.returncode,fails
rc,f=run(tex)
chk(rc==0 and not f,"scripts/check_7cube_midplanes.py on paper/app_seven.tex: exit code 0, all checks passed")
src=open(tex).read()
muts=[
 ("a cell moved off its line",              r"$(0,1,3),\ (0,2,3)$", r"$(1,1,3),\ (0,2,3)$"),
 ("a cell used twice (two lines share it)", r"$(1,2,3),\ (1,4,3)$", r"$(1,0,3),\ (1,4,3)$"),
 ("another cell of the same line, still a valid pairing, not the rule", r"$(0,0,3),\ (6,6,3)$", r"$(0,0,3),\ (3,3,3)$"),
 ("a line replaced by another line of M (duplicate line)", r"$(2,t,3)$ & $(2,4,3),\ (2,5,3)$", r"$(4,t,3)$ & $(2,4,3),\ (2,5,3)$"),
 ("a line replaced by a line outside M",    r"$(t,6{-}t,3)$ & $(1,5,3),\ (5,1,3)$", r"$(t,6{-}t,2)$ & $(1,5,3),\ (5,1,3)$"),
 ("reversal dropped: (t,3,6-t) -> (t,3,t), duplicate line", r"$(t,3,6{-}t)$", r"$(t,3,t)$"),
 ("one row deleted",                        "$(6,t,3)$ & $(6,0,3),\\ (6,1,3)$ & $(6,3,t)$ & $(6,3,0),\\ (6,3,1)$ & $(3,6,t)$ & $(3,6,0),\\ (3,6,1)$\\\\\n", ""),
 ("a digit of a cell changed to a non-digit (entry not parseable)", r"$(3,3,0),\ (3,3,1)$", r"$(3,3,0),\ (3,3,t)$"),
 ("two cells of one entry exchanged (same pairing; must still pass)", r"$(1,5,3),\ (5,1,3)$", r"$(5,1,3),\ (1,5,3)$"),
 ("central line: cell of another central line", r"$(3,0,3),\ (3,1,3)$", r"$(3,0,3),\ (1,3,3)$"),
]
os.makedirs("x_seven_mut",exist_ok=True)
for i,(what,a,b) in enumerate(muts):
    assert src.count(a)==1,(what,src.count(a))
    p="x_seven_mut/app_seven_mut%02d.tex"%i
    open(p,"w").write(src.replace(a,b))
    rc,f=run(p); mine=parse(p)
    expect_pass="must still pass" in what
    chk((rc==0)==expect_pass,"mutation %d (%s): exit code %d; failed checks: %s"%(i,what,rc,"; ".join(x[:40].strip() for x in f) if f else "none"))
print("RESULT:","all checks passed" if ok else "FAILED")

# referee X: is there a compact triple (B,F,t), |B| = t, F B-admissible, for which LOST holds ONLY through patterns with x in F?
# (the eight vectors of Table 10 do not distinguish Definition 5.4 from the variant that requires x not in F)
import random, subprocess
from x_geom import *
C=lambda x,y,z=0: cid((x,y,z))
S=[C(*p) for p in [(0,0),(2,0),(3,0),(4,0),(5,0),(0,1),(0,2),(0,3),(4,1),(3,2),(2,3)]]   # E2 of Figure 4, x=(0,0,0)
l=[C(i,0) for i in range(6)]; a=[C(0,j) for j in range(6)]; b=[C(i,5-i) for i in range(6)]
rnd=random.Random(11)
cands=[]
for y1 in (C(2,0),C(3,0),C(4,0)):
    for y2 in (C(0,1),C(0,2),C(0,3),C(4,1),C(3,2),C(2,3)):
        F=sorted(c for c in S if c not in (y1,y2))
        for _ in range(6):
            free=[c for c in range(216) if c not in l+a+b]
            B=sorted(rnd.sample(free,10))
            if admissible(mask(B),mask(F)): cands.append((10,B,F))
inp="".join("%d %s %s\n"%(t,",".join(map(str,B)),",".join(map(str,F))) for t,B,F in cands)
out=subprocess.run(["./x_lost"],input=inp,capture_output=True,text=True).stdout.strip().split("\n")
import re
hits=[]
for ln in out:
    m=re.search(r"LOST=(\w+)  E2 fits=\w+ \((\d+) patterns, (\d+) of them.*E3 fits=\w+ \((\d+), (\d+) with",ln)
    lost,n2,n2x,n3,n3x=m.group(1),int(m.group(2)),int(m.group(3)),int(m.group(4)),int(m.group(5))
    if lost=="true" and n2==n2x and n3==n3x: hits.append(ln)
print(len(cands),"candidates;",len(hits),"triples with |B|=t=10, F admissible, on which LOST is true only through patterns with x in F")
for h in hits[:3]: print("  ",h)

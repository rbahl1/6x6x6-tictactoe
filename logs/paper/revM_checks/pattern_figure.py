# literal check of the two patterns drawn in the new figure of Section 5.3 (plane z = 0), B = F = empty
import itertools
from geom import *
P=lambda i,j: cid((i,j,0))
row=lambda j: frozenset(P(i,j) for i in range(6)); col=lambda i: frozenset(P(i,j) for j in range(6))
anti=frozenset(P(i,5-i) for i in range(6))
F=frozenset(); B=frozenset()
def dangerpairs(B,X):
    four=[l for l in live(B) if len(l&X)==4]
    return [(a,b) for a,b in itertools.combinations(four,2) if (a&b) and not ((a&b)&X)]
def threats(B,X): return [l for l in live(B) if len(l&X)==5]
# ---- E2
l=row(0); a=col(0); b=anti; x=P(0,0); e=P(1,0); c=P(0,5)
Sa=frozenset({P(0,0),P(0,1),P(0,2),P(0,3)}); Sb=frozenset({P(5,0),P(4,1),P(3,2),P(2,3)}); Sl=l-{e}
assert all(z in LSET for z in (l,a,b)) and len({l,a,b})==3 and len({x,e,c})==3
assert l&a=={x} and e in l and a&b=={c} and x not in b and e not in b            # (E2.1)
assert Sa<=a-{c} and len(Sa)==4 and x in Sa and Sb<=b-{c} and len(Sb)==4        # (E2.2)
assert e not in F and c not in F and (F&a)<=Sa and (F&b)<=Sb                    # (E2.3)
X1=Sl|Sa|Sb; X=X1-{x}
print('E2: stones',len(X1),' X admissible:',admissible(B,X),' threats of X1:',len(threats(B,X1)),
      ' danger pairs of X1:',len(dangerpairs(B,X1)),' after reply e: admissible',admissible(B|{e},X1),
      ' danger pairs:',len(dangerpairs(B|{e},X1)))
print('   stones:',sorted(cc(z)[:2] for z in X1))
# ---- E3
a=row(0); a1=col(0); b=anti; b1=row(1); x=P(0,0); c=P(5,0); c1=P(0,1)
Sa=frozenset({P(0,0),P(1,0),P(2,0),P(3,0)}); Sa1=frozenset({P(0,0),P(0,2),P(0,3),P(0,5)})
Sb=frozenset({P(0,5),P(1,4),P(2,3),P(4,1)}); Sb1=frozenset({P(1,1),P(2,1),P(3,1),P(4,1)})
assert len({a,a1,b,b1})==4 and len({x,c,c1})==3
assert a&a1=={x} and a&b=={c} and a1&b1=={c1} and x not in b|b1                  # (E3.1)
assert Sa<=a-{c} and Sa1<=a1-{c1} and Sb<=b-{c} and Sb1<=b1-{c1}
assert all(len(S)==4 for S in (Sa,Sa1,Sb,Sb1)) and x in Sa&Sa1 and not ({c,c1}&(Sa|Sa1|Sb|Sb1))  # (E3.2)
X1=Sa|Sa1|Sb|Sb1; X=X1-{x}
dp=dangerpairs(B,X1)
print('E3: stones',len(X1),' X admissible:',admissible(B,X),' threats of X1:',len(threats(B,X1)),' danger pairs of X1:',len(dp),
      [cc(next(iter(p&q)))[:2] for p,q in dp])
for r in (c,c1): print('   reply',cc(r)[:2],': admissible',admissible(B|{r},X1))
print('   any single reply restores admissibility:',any(admissible(B|{r},X1) for r in range(216) if r not in X1))
print('   stones:',sorted(cc(z)[:2] for z in X1))

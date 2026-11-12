// referee X: literal evaluation of Definition 5.4 (patterns E2, E3; LOST) -- own code, no screens.
// Reads lines "t B F" (ids), prints which kinds of patterns fit and the smallest union |F u stone sets|.
// The only shortcuts are monotone: a partial union that already exceeds t+1 cells cannot fit.
#include <bits/stdc++.h>
using namespace std;
typedef array<uint64_t,4> BS;
static inline BS operator|(const BS&a,const BS&b){return {a[0]|b[0],a[1]|b[1],a[2]|b[2],a[3]|b[3]};}
static inline BS operator&(const BS&a,const BS&b){return {a[0]&b[0],a[1]&b[1],a[2]&b[2],a[3]&b[3]};}
static inline int pc(const BS&a){return __builtin_popcountll(a[0])+__builtin_popcountll(a[1])+__builtin_popcountll(a[2])+__builtin_popcountll(a[3]);}
static inline bool tst(const BS&a,int c){return a[c>>6]>>(c&63)&1;}
static inline void st(BS&a,int c){a[c>>6]|=1ULL<<(c&63);}
static inline BS without(BS a,int c){a[c>>6]&=~(1ULL<<(c&63));return a;}
static inline bool sub(const BS&a,const BS&b){for(int i=0;i<4;i++) if(a[i]&~b[i]) return false; return true;}
vector<array<int,6>> LN; vector<BS> LM; vector<int> THRU[216];
int main(){
  // lines from parametrizations (Section 2.1): components constant 0..5, identity (6), reversal (7)
  set<array<int,6>> S;
  for(int f0=0;f0<8;f0++)for(int f1=0;f1<8;f1++)for(int f2=0;f2<8;f2++){
    if(f0<6&&f1<6&&f2<6) continue;
    array<int,6> l; int f[3]={f0,f1,f2};
    for(int t=0;t<6;t++){int c[3]; for(int i=0;i<3;i++) c[i]=f[i]<6?f[i]:(f[i]==6?t:5-t); l[t]=36*c[0]+6*c[1]+c[2];}
    sort(l.begin(),l.end()); S.insert(l);
  }
  assert(S.size()==148);
  for(auto&l:S){BS m={0,0,0,0}; for(int c:l){st(m,c);} for(int c:l) THRU[c].push_back(LN.size()); LN.push_back(l); LM.push_back(m);}
  auto meet=[&](int a,int b)->int{BS m=LM[a]&LM[b]; int n=pc(m); assert(n<=1||a==b); if(n!=1) return -1; for(int c=0;c<216;c++) if(tst(m,c)) return c; return -1;};
  string line;
  while(getline(cin,line)){
    if(line.empty()) continue;
    istringstream is(line); int t; string sb,sf; is>>t>>sb>>sf;
    auto parse=[&](const string&s){BS m={0,0,0,0}; if(s!="-"){stringstream ss(s); string x; while(getline(ss,x,',')) st(m,stoi(x));} return m;};
    BS B=parse(sb),F=parse(sf);
    assert(pc(B&F)==0);
    vector<int> live; for(int l=0;l<148;l++) if(pc(LM[l]&B)==0) live.push_back(l);
    vector<char> isl(148,0); for(int l:live) isl[l]=1;
    int lim=t+1; int best2=999,best3=999; long n2=0,n3=0,n2xF=0,n3xF=0;
    // ---- E2: lines l,a,b ; cells x,e,c ; sets S_a,S_b
    for(int a:live) for(int x:LN[a]) for(int l:THRU[x]){ if(l==a||!isl[l]) continue;           // l n a = {x}
      for(int e:LN[l]){ if(e==x||tst(F,e)) continue;                                              // e in l, e != x, e not in F
        BS U1=F|without(LM[l],e);                                                                  // F u S_l
        for(int c:LN[a]){ if(c==x||c==e||tst(F,c)) continue;                                       // c: common cell of a,b ; c not in F
          for(int b:THRU[c]){ if(b==a||b==l||!isl[b]) continue;                                    // a n b = {c}
            if(tst(LM[b],x)||tst(LM[b],e)) continue;                                               // x not in b, e not in b
            // S_a subset of a-{c}, 4 cells, x in S_a, F n a subset S_a : drop one cell p of a-{c}, p != x, p not in F
            for(int p:LN[a]){ if(p==c||p==x||tst(F,p)) continue;
              BS Sa=without(without(LM[a],c),p); BS U2=U1|Sa;
              for(int q:LN[b]){ if(q==c||tst(F,q)) continue;
                BS Sb=without(without(LM[b],c),q); int u=pc(U2|Sb);
                best2=min(best2,u); if(u<=lim){ n2++; if(tst(F,x)) n2xF++; }
              }}}}}}
    // ---- E3: lines a,a',b,b' ; cells x,c,c' ; four sets.  (a,b,c) and (a',b',c') may be exchanged, so a<a' suffices.
    for(int a:live) for(int x:LN[a]) for(int a1:THRU[x]){ if(a1<=a||!isl[a1]) continue;
      for(int c:LN[a]){ if(c==x||tst(F,c)) continue;
        for(int c1:LN[a1]){ if(c1==x||c1==c||tst(F,c1)) continue;
          for(int p:LN[a]){ if(p==c||p==x||tst(F,p)) continue;                 // S_a = a-{c,p}, contains x; c,c' not in S_a: c' not on a (a n a' = {x})
            BS Sa=without(without(LM[a],c),p);
            for(int p1:LN[a1]){ if(p1==c1||p1==x||tst(F,p1)) continue;
              BS Sa1=without(without(LM[a1],c1),p1); BS U1=F|Sa|Sa1; if(pc(U1)>lim && best3<=pc(U1)) continue;
              for(int b:THRU[c]){ if(b==a||!isl[b]||b==a1||tst(LM[b],x)) continue;
                for(int b1:THRU[c1]){ if(b1==a1||!isl[b1]||b1==a||b1==b||tst(LM[b1],x)) continue;
                  for(int q:LN[b]){ if(q==c||tst(F,q)) continue;
                    BS Sb=without(without(LM[b],c),q); if(tst(Sb,c1)) continue;       // c' in no stone set
                    for(int q1:LN[b1]){ if(q1==c1||tst(F,q1)) continue;
                      BS Sb1=without(without(LM[b1],c1),q1); if(tst(Sb1,c)) continue;  // c in no stone set
                      if(tst(Sa,c1)||tst(Sa1,c)) continue;
                      int u=pc(U1|Sb|Sb1); best3=min(best3,u); if(u<=lim){ n3++; if(tst(F,x)) n3xF++; }
                    }}}}}}}}}
    printf("%s : LOST=%s  E2 fits=%s (%ld patterns, %ld of them with x in F; smallest union %d)  E3 fits=%s (%ld, %ld with x in F; smallest union %d)  t+1=%d\n",line.c_str(),
      (n2||n3)?"true":"false", n2?"yes":"no", n2, n2xF, best2, n3?"yes":"no", n3, n3xF, best3, lim);
  }
}

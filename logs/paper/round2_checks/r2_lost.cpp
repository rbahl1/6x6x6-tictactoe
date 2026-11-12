// r2_lost.cpp -- second round, findings X2 and X8: a literal evaluation of the predicate LOST(B,F,t)
// (definition of the patterns E2 and E3 in Section 5.4 of the paper), written by the final editor
// independently of the three implementations compared in logs/paper/lost_vectors/.
// Reads lines "t B F" (sets as comma-separated cell ids 36x+6y+z, "-" for the empty set) and prints, for
// every line: |B|, |F|, whether F is B-admissible, the number of fitting E2 patterns and how many of them
// have x in F, the same for E3, and the verdict.  No pruning other than the conditions of the definition.
//   g++ -O2 -std=c++17 -o r2_lost r2_lost.cpp && ./r2_lost < ../lost_vectors/vectors.txt
#include <bits/stdc++.h>
using namespace std;
typedef array<uint64_t,4> M;                       // set of cells, 216 bits
static M mk() { return M{0,0,0,0}; }
static void add(M& m,int c){ m[c>>6] |= 1ULL<<(c&63); }
static bool has(const M& m,int c){ return m[c>>6]>>(c&63)&1; }
static M uni(M a,const M& b){ for(int i=0;i<4;i++) a[i]|=b[i]; return a; }
static M inter(M a,const M& b){ for(int i=0;i<4;i++) a[i]&=b[i]; return a; }
static bool subset(const M& a,const M& b){ for(int i=0;i<4;i++) if(a[i]&~b[i]) return false; return true; }
static int cnt(const M& a){ int n=0; for(int i=0;i<4;i++) n+=__builtin_popcountll(a[i]); return n; }
static vector<vector<int>> L;                      // the 148 lines
static vector<M> LM;
static M parseSet(const string& s){ M m=mk(); if(s=="-") return m; stringstream ss(s); string x; while(getline(ss,x,',')) add(m,stoi(x)); return m; }
// all 4-subsets of line a without the cell c that contain the cells of F on a (and the cell x, if x >= 0)
static vector<M> fourSets(int a,int c,const M& F,int x){
  vector<int> r; for(int v: L[a]) if(v!=c) r.push_back(v);
  vector<M> out;
  for(int omit=0; omit<5; omit++){
    M s=mk(); for(int i=0;i<5;i++) if(i!=omit) add(s,r[i]);
    if(!subset(inter(F,LM[a]),s)) continue;
    if(x>=0 && !has(s,x)) continue;
    out.push_back(s);
  }
  return out;
}
int main(){
  set<vector<int>> S;
  for(int x=0;x<6;x++)for(int y=0;y<6;y++)for(int z=0;z<6;z++)
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)for(int dz=-1;dz<=1;dz++){
      if(!dx&&!dy&&!dz) continue; vector<int> l; bool ok=true;
      for(int i=0;i<6;i++){ int a=x+i*dx,b=y+i*dy,c=z+i*dz; if(a<0||a>5||b<0||b>5||c<0||c>5){ok=false;break;} l.push_back(36*a+6*b+c); }
      if(ok){ sort(l.begin(),l.end()); S.insert(l); } }
  for(auto& l: S){ L.push_back(l); M m=mk(); for(int c: l) add(m,c); LM.push_back(m); }
  if(L.size()!=148){ fprintf(stderr,"geometry\n"); return 2; }
  string line;
  while(getline(cin,line)){
    stringstream ss(line); int t; string sb,sf; if(!(ss>>t>>sb>>sf)) continue;
    M B=parseSet(sb), F=parseSet(sf);
    vector<int> live; for(int i=0;i<148;i++) if(cnt(inter(LM[i],B))==0) live.push_back(i);
    // admissibility of F
    bool adm = cnt(inter(B,F))==0; vector<int> four;
    for(int i: live){ int n=cnt(inter(LM[i],F)); if(n>=5) adm=false; if(n==4) four.push_back(i); }
    for(size_t i=0;i<four.size();i++) for(size_t j=i+1;j<four.size();j++){ M m=inter(LM[four[i]],LM[four[j]]); if(cnt(m)==1 && cnt(inter(m,F))==0) adm=false; }
    auto meet=[&](int p,int q)->int{ M m=inter(LM[p],LM[q]); if(cnt(m)!=1) return -1; for(int c: L[p]) if(has(m,c)) return c; return -1; };
    long long e2=0,e2x=0,e3=0,e3x=0; int min2=99,min3=99;
    // E2: lines l, a, b; cells x, e, c
    for(int l: live) for(int a: live){ if(a==l) continue; int x=meet(l,a); if(x<0) continue;
      for(int b: live){ if(b==l||b==a) continue; int c=meet(a,b); if(c<0) continue; if(has(LM[b],x)) continue; if(has(F,c)) continue;
        for(int e: L[l]){ if(e==x||e==c) continue; if(has(LM[b],e)) continue; if(has(F,e)) continue;
          M Sl=LM[l]; Sl[e>>6]&=~(1ULL<<(e&63));
          for(const M& Sa: fourSets(a,c,F,x)) for(const M& Sb: fourSets(b,c,F,-1)){
            int n=cnt(uni(uni(F,Sl),uni(Sa,Sb))); int st=cnt(uni(Sl,uni(Sa,Sb))); 
            if(n<=t+1){ e2++; if(has(F,x)) e2x++; min2=min(min2,st); } } } } }
    // E3: lines a, a', b, b'; cells x, c, c'
    for(int a: live) for(int a2: live){ if(a2==a) continue; int x=meet(a,a2); if(x<0) continue;
      for(int b: live){ if(b==a||b==a2) continue; int c=meet(a,b); if(c<0||c==x) continue; if(has(LM[b],x)) continue; if(has(F,c)) continue;
        for(int b2: live){ if(b2==a||b2==a2||b2==b) continue; int c2=meet(a2,b2); if(c2<0||c2==x||c2==c) continue; if(has(LM[b2],x)) continue; if(has(F,c2)) continue;
          for(const M& Sa: fourSets(a,c,F,x)){ if(has(Sa,c2)) continue;
           for(const M& Sa2: fourSets(a2,c2,F,x)){ if(has(Sa2,c)) continue;
            M u1=uni(F,uni(Sa,Sa2));
            for(const M& Sb: fourSets(b,c,F,-1)){ if(has(Sb,c2)) continue;
             for(const M& Sb2: fourSets(b2,c2,F,-1)){ if(has(Sb2,c)) continue;
              int n=cnt(uni(u1,uni(Sb,Sb2))); int st=cnt(uni(uni(Sa,Sa2),uni(Sb,Sb2)));
              if(n<=t+1){ e3++; if(has(F,x)) e3x++; min3=min(min3,st); } } } } } } } }
    printf("%d %s %s |B|=%d |F|=%d admissible=%d E2: %lld fitting, %lld with x in F, min stones %d; E3: %lld fitting, %lld with x in F, min stones %d; lost=%s\n",
      t,sb.c_str(),sf.c_str(),cnt(B),cnt(F),(int)adm,e2,e2x,e2?min2:0,e3,e3x,e3?min3:0,(e2||e3)?"true":"false");
    fflush(stdout);
  }
  return 0;
}

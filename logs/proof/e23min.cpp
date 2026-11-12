// e23min.cpp -- literal enumeration of ALL patterns E2 and E3 of SPEC section 4 on the empty board
// (B = F = {}: every line is live), with their stone sets, and the histogram of the number of distinct
// stones |S_l u S_a u S_b| resp. |S_a u S_a' u S_b u S_b'|.  Written for PROOF.md, Remark 8.4b.
// Own geometry; nothing from src/, verify/ or sim/ is used.
// Build / run:  g++ -O2 -std=c++17 -Wall -o <scratch>/e23min logs/proof/e23min.cpp && nice -n 10 <scratch>/e23min > logs/proof/e23min.log
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <set>
#include <vector>

struct CS {
  uint64_t w[4] = {0, 0, 0, 0};
  void set(int c) { w[c >> 6] |= 1ULL << (c & 63); }
  bool test(int c) const { return (w[c >> 6] >> (c & 63)) & 1; }
  int count() const { return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) + __builtin_popcountll(w[2]) + __builtin_popcountll(w[3]); }
};
static CS uni(const CS& a, const CS& b) { CS r; for (int i = 0; i < 4; i++) r.w[i] = a.w[i] | b.w[i]; return r; }

int main() {
  // lines: triples of (constant 0..5 | t | 5-t), not all constant
  std::set<std::array<int, 6>> ls;
  for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) for (int f3 = 0; f3 < 8; f3++) {
    if (f1 < 6 && f2 < 6 && f3 < 6) continue;
    std::array<int, 6> l;
    for (int t = 0; t < 6; t++) {
      int f[3] = {f1, f2, f3}, co[3];
      for (int i = 0; i < 3; i++) co[i] = f[i] < 6 ? f[i] : (f[i] == 6 ? t : 5 - t);
      l[t] = 36 * co[0] + 6 * co[1] + co[2];
    }
    std::sort(l.begin(), l.end());
    ls.insert(l);
  }
  std::vector<std::array<int, 6>> L(ls.begin(), ls.end());
  int NL = (int)L.size();
  if (NL != 148) { printf("ERROR: %d lines\n", NL); return 1; }
  std::vector<std::vector<int>> through(216);
  for (int i = 0; i < NL; i++) for (int c : L[i]) through[c].push_back(i);
  auto common = [&](int i, int j) {            // the common cell of two distinct lines, or -1
    int r = -1, n = 0;
    for (int c : L[i]) for (int d : L[j]) if (c == d) { r = c; n++; }
    if (n > 1) { printf("ERROR: two lines share %d cells\n", n); exit(1); }
    return r;
  };
  auto has = [&](int l, int c) { for (int d : L[l]) if (d == c) return true; return false; };
  // 4-subsets of line `l` that avoid cell `avoid` and (if must >= 0) contain cell `must`
  auto stoneSets = [&](int l, int avoid, int must) {
    std::vector<CS> r;
    for (int drop : L[l]) {
      if (drop == avoid || drop == must) continue;
      CS s;
      for (int c : L[l]) if (c != avoid && c != drop) s.set(c);
      r.push_back(s);
    }
    return r;
  };

  // ---------------------------------------------------------------- E2
  long long h2[20] = {0}, n2 = 0;
  for (int l = 0; l < NL; l++) for (int a = 0; a < NL; a++) {
    if (a == l) continue;
    int x = common(l, a);
    if (x < 0) continue;
    for (int c : L[a]) {
      if (c == x) continue;
      for (int b : through[c]) {
        if (b == a || b == l) continue;
        if (has(b, x)) continue;                     // x not in b
        for (int e : L[l]) {
          if (e == x || e == c) continue;            // x, e, c pairwise distinct
          if (has(b, e)) continue;                   // e not in b
          CS Sl;
          for (int d : L[l]) if (d != e) Sl.set(d);
          auto SA = stoneSets(a, c, x);              // S_a in a\{c}, |S_a| = 4, x in S_a
          auto SB = stoneSets(b, c, -1);             // S_b in b\{c}, |S_b| = 4
          for (auto& Sa : SA) for (auto& Sb : SB) { h2[uni(uni(Sl, Sa), Sb).count()]++; n2++; }
        }
      }
    }
  }
  printf("E2: %lld patterns (l,a,b,e,S_a,S_b); distinct stones:", n2);
  for (int k = 0; k < 20; k++) if (h2[k]) printf("  %d: %lld", k, h2[k]);
  printf("\n");

  // ---------------------------------------------------------------- E3
  long long h3[20] = {0}, n3 = 0;
  for (int a = 0; a < NL; a++) for (int a2 = 0; a2 < NL; a2++) {
    if (a == a2) continue;
    int x = common(a, a2);
    if (x < 0) continue;
    for (int c : L[a]) {
      if (c == x) continue;
      for (int b : through[c]) {
        if (b == a || b == a2) continue;
        if (has(b, x)) continue;                                   // x not in b
        auto SA = stoneSets(a, c, x);
        auto SB = stoneSets(b, c, -1);
        for (int c2 : L[a2]) {
          if (c2 == x || c2 == c) continue;                        // x, c, c' pairwise distinct
          for (int b2 : through[c2]) {
            if (b2 == a2 || b2 == a || b2 == b) continue;          // four distinct lines
            if (has(b2, x)) continue;                              // x not in b'
            auto SA2 = stoneSets(a2, c2, x);
            auto SB2 = stoneSets(b2, c2, -1);
            for (auto& Sa : SA) for (auto& Sb : SB) {
              if (Sa.test(c2) || Sb.test(c2)) continue;            // c' in no stone set
              CS u1 = uni(Sa, Sb);
              for (auto& Sa2 : SA2) {
                if (Sa2.test(c)) continue;                         // c in no stone set
                CS u2 = uni(u1, Sa2);
                for (auto& Sb2 : SB2) {
                  if (Sb2.test(c)) continue;
                  h3[uni(u2, Sb2).count()]++; n3++;
                }
              }
            }
          }
        }
      }
    }
  }
  printf("E3: %lld patterns (a,a',b,b' ordered, with stone sets); distinct stones:", n3);
  for (int k = 0; k < 20; k++) if (h3[k]) printf("  %d: %lld", k, h3[k]);
  printf("\n");
  int m2 = 0, m3 = 0;
  for (int k = 19; k >= 0; k--) { if (h2[k]) m2 = k; if (h3[k]) m3 = k; }
  printf("minimum number of distinct stones: E2 %d, E3 %d\n", m2, m3);
  return 0;
}

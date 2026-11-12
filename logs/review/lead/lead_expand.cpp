// logs/review/lead/lead_expand.cpp -- lead reviewer's own re-expansion of the proof tree of SPEC section 4.
// Shares no code with src/, verify/, sim/.  Exact keys (full B, F, t, kind), canonical form = minimum over all
// 1152 maps (brute force), literal children, literal admissibility.
//
//   g++ -O2 -std=c++17 -o lead_expand lead_expand.cpp
//   ./lead_expand results/final/strategy.tbl results/final/leaves.txt 10 > lead_expand.json
//
// Checked (Definition 8.1 of PROOF.md, (V2), (V3) and the closure of the reachable set V):
//   * every line of both files is well formed and in canonical form (my canonical form), no duplicate key;
//   * from the root P(-,-,0): a reachable non-VOID P-state is a listed leaf, or has t < R_max and gets ALL its
//     children (a), (c), (d); a reachable non-VOID Q-state has a table entry m outside B and F, child (i) (which must
//     not be VOID) and, if |F| < t, child (ii) unless VOID;
//   * every listed leaf / table entry is reached (so the LEAF recomputation of every line of leaves.txt by
//     lead_leaf.cpp covers exactly the reachable leaves).
// NOT checked here: LEAF (done by lead_leaf.cpp on every line of leaves.txt) and LOST.  LOST is not evaluated
// because the program refuses R_max > 10: an expanded state then has t <= 9, FP has at most 10 stones after his
// move, and a pattern E2/E3 needs >= 11 distinct stones (PROOF Prop. 8.4a, re-derived by me in REVIEW.md).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <iostream>

typedef uint64_t u64;
static const int NC = 216;
struct CS {
  u64 w[4];
  CS() { w[0] = w[1] = w[2] = w[3] = 0; }
  bool test(int c) const { return (w[c >> 6] >> (c & 63)) & 1; }
  void set(int c) { w[c >> 6] |= 1ULL << (c & 63); }
  void reset(int c) { w[c >> 6] &= ~(1ULL << (c & 63)); }
  int count() const { return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) + __builtin_popcountll(w[2]) + __builtin_popcountll(w[3]); }
  bool meets(const CS& o) const { return (w[0] & o.w[0]) | (w[1] & o.w[1]) | (w[2] & o.w[2]) | (w[3] & o.w[3]); }
  CS operator|(const CS& o) const { CS r; for (int i = 0; i < 4; i++) r.w[i] = w[i] | o.w[i]; return r; }
  CS operator&(const CS& o) const { CS r; for (int i = 0; i < 4; i++) r.w[i] = w[i] & o.w[i]; return r; }
  bool operator==(const CS& o) const { return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3]; }
  int list(uint8_t* out) const {
    int n = 0;
    for (int i = 0; i < 4; i++) { u64 x = w[i]; while (x) { out[n++] = (uint8_t)(i * 64 + __builtin_ctzll(x)); x &= x - 1; } }
    return n;
  }
};
static int NL = 0;
static int LINE[160][6];
static CS LSET[160];
static std::vector<int> CL[NC];
static int MEETC[160][160];
static std::vector<std::array<uint8_t, NC>> GM;

static int cid(int x, int y, int z) { return 36 * x + 6 * y + z; }
static void geom() {
  std::set<std::array<int, 6>> seen;
  for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
    if (!dx && !dy && !dz) continue;
    int first = dx ? dx : (dy ? dy : dz);
    if (first < 0) continue;
    for (int x = 0; x < 6; x++) for (int y = 0; y < 6; y++) for (int z = 0; z < 6; z++) {
      int px = x - dx, py = y - dy, pz = z - dz;
      if (px >= 0 && px < 6 && py >= 0 && py < 6 && pz >= 0 && pz < 6) continue;
      int ex = x + 5 * dx, ey = y + 5 * dy, ez = z + 5 * dz;
      if (ex < 0 || ex > 5 || ey < 0 || ey > 5 || ez < 0 || ez > 5) continue;
      std::array<int, 6> a;
      for (int i = 0; i < 6; i++) a[i] = cid(x + i * dx, y + i * dy, z + i * dz);
      std::sort(a.begin(), a.end());
      seen.insert(a);
    }
  }
  for (auto& a : seen) { for (int i = 0; i < 6; i++) { LINE[NL][i] = a[i]; LSET[NL].set(a[i]); CL[a[i]].push_back(NL); } NL++; }
  if (NL != 148) { fprintf(stderr, "lines: %d\n", NL); exit(3); }
  for (int a = 0; a < NL; a++) for (int b = 0; b < NL; b++) {
    MEETC[a][b] = -1;
    if (a == b) continue;
    CS I = LSET[a] & LSET[b];
    if (I.count() > 1) { fprintf(stderr, "two lines share 2 cells\n"); exit(3); }
    if (I.count() == 1) { uint8_t c[4]; I.list(c); MEETC[a][b] = c[0]; }
  }
  // group: closure of six generators
  auto mk = [](int (*f)(int, int, int)) { std::array<uint8_t, NC> m; for (int c = 0; c < NC; c++) m[c] = (uint8_t)f(c / 36, (c / 6) % 6, c % 6); return m; };
  static const int S1[6] = {1, 0, 2, 3, 5, 4}, S2[6] = {0, 2, 1, 4, 3, 5}, S3[6] = {5, 1, 2, 3, 4, 0};
  std::vector<std::array<uint8_t, NC>> gens;
  gens.push_back(mk([](int x, int y, int z) { return cid(y, x, z); }));
  gens.push_back(mk([](int x, int y, int z) { return cid(x, z, y); }));
  gens.push_back(mk([](int x, int y, int z) { return cid(5 - x, y, z); }));
  gens.push_back(mk([](int x, int y, int z) { return cid(S1[x], S1[y], S1[z]); }));
  gens.push_back(mk([](int x, int y, int z) { return cid(S2[x], S2[y], S2[z]); }));
  gens.push_back(mk([](int x, int y, int z) { return cid(S3[x], S3[y], S3[z]); }));
  std::set<std::array<uint8_t, NC>> all;
  std::array<uint8_t, NC> id;
  for (int c = 0; c < NC; c++) id[c] = (uint8_t)c;
  all.insert(id);
  std::vector<std::array<uint8_t, NC>> fr{id};
  while (!fr.empty()) {
    std::vector<std::array<uint8_t, NC>> nx;
    for (auto& g : fr) for (auto& s : gens) {
      std::array<uint8_t, NC> h;
      for (int c = 0; c < NC; c++) h[c] = s[g[c]];
      if (all.insert(h).second) nx.push_back(h);
    }
    fr.swap(nx);
  }
  GM.assign(all.begin(), all.end());
  if (GM.size() != 1152) { fprintf(stderr, "group order %zu\n", GM.size()); exit(3); }
  std::set<std::array<int, 6>> ls(seen.begin(), seen.end());
  for (auto& g : GM) for (int l = 0; l < NL; l++) {
    std::array<int, 6> a;
    for (int i = 0; i < 6; i++) a[i] = g[LINE[l][i]];
    std::sort(a.begin(), a.end());
    if (!ls.count(a)) { fprintf(stderr, "map not line preserving\n"); exit(3); }
  }
}
// canonical form: lexicographically smallest (sorted image of B, sorted image of F) over all 1152 maps
static void canon(const CS& B, const CS& F, CS& Bo, CS& Fo) {
  uint8_t bc[NC], fc[NC];
  int nb = B.list(bc), nf = F.list(fc);
  uint8_t best[2 * NC + 2], cur[2 * NC + 2];
  bool have = false;
  int ng = (int)GM.size();
  for (int g = 0; g < ng; g++) {
    const uint8_t* p = GM[g].data();
    for (int i = 0; i < nb; i++) cur[i] = p[bc[i]];
    std::sort(cur, cur + nb);
    if (have) {
      int c = memcmp(cur, best, nb);
      if (c > 0) continue;
      for (int i = 0; i < nf; i++) cur[nb + i] = p[fc[i]];
      std::sort(cur + nb, cur + nb + nf);
      if (c == 0 && memcmp(cur + nb, best + nb, nf) >= 0) continue;
    } else {
      for (int i = 0; i < nf; i++) cur[nb + i] = p[fc[i]];
      std::sort(cur + nb, cur + nb + nf);
    }
    memcpy(best, cur, nb + nf);
    have = true;
  }
  Bo = CS(); Fo = CS();
  for (int i = 0; i < nb; i++) Bo.set(best[i]);
  for (int i = 0; i < nf; i++) Fo.set(best[nb + i]);
}
static bool admissible(const CS& B, const CS& X) {
  if (B.meets(X)) return false;
  int four[160], n4 = 0;
  for (int l = 0; l < NL; l++) {
    if (LSET[l].meets(B)) continue;
    int n = (LSET[l] & X).count();
    if (n > 4) return false;
    if (n == 4) four[n4++] = l;
  }
  for (int i = 0; i < n4; i++) for (int j = i + 1; j < n4; j++) {
    int c = MEETC[four[i]][four[j]];
    if (c >= 0 && !X.test(c)) return false;
  }
  return true;
}
struct Key {
  u64 w[9];
  bool operator==(const Key& o) const { return memcmp(w, o.w, sizeof w) == 0; }
};
struct KeyHash {
  size_t operator()(const Key& k) const {
    u64 h = 1469598103934665603ULL;
    for (int i = 0; i < 9; i++) { h ^= k.w[i]; h *= 1099511628211ULL; h ^= h >> 29; }
    return (size_t)h;
  }
};
static Key mkKey(const CS& B, const CS& F, int t, int kind) {
  Key k;
  for (int i = 0; i < 4; i++) { k.w[i] = B.w[i]; k.w[4 + i] = F.w[i]; }
  k.w[8] = ((u64)t << 4) | (u64)kind;
  return k;
}
static bool parseSet(const std::string& s, CS& out, bool& ascending) {
  out = CS(); ascending = true;
  if (s == "-") return true;
  int prev = -1;
  size_t i = 0;
  while (i < s.size()) {
    int v = 0, nd = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); i++; nd++; }
    if (nd == 0 || nd > 3 || v >= NC || out.test(v)) return false;
    if (v <= prev) ascending = false;
    prev = v;
    out.set(v);
    if (i == s.size()) return true;
    if (s[i] != ',') return false;
    i++;
    if (i == s.size()) return false;
  }
  return false;
}
struct Item { CS B, F; int t, kind; };   // kind 0 = P, 1 = Q

int main(int argc, char** argv) {
  if (argc < 4) { fprintf(stderr, "usage: lead_expand TABLE LEAVES RMAX\n"); return 2; }
  int rmax = atoi(argv[3]);
  if (rmax < 0 || rmax > 10) { fprintf(stderr, "this program does not evaluate LOST and therefore refuses R_max > 10\n"); return 2; }
  geom();
  std::unordered_map<Key, int, KeyHash> table;
  std::unordered_set<Key, KeyHash> leaves, usedLeaves, usedTable;
  long tl = 0, ll = 0, bad = 0, noncanon = 0, dup = 0;
  {
    std::ifstream f(argv[1]);
    std::string line;
    while (std::getline(f, line)) {
      tl++;
      std::stringstream ss(line);
      std::string a, b, c, d, extra;
      ss >> a >> b >> c >> d;
      if (d.empty() || (ss >> extra)) { bad++; continue; }
      CS B, F; bool asc1, asc2;
      int t = atoi(a.c_str()), m = atoi(d.c_str());
      if (!parseSet(b, B, asc1) || !parseSet(c, F, asc2) || !asc1 || !asc2) { bad++; continue; }
      if (t < 1 || t > 60 || B.count() != t - 1 || F.count() > t || B.meets(F) || m < 0 || m >= NC || B.test(m) || F.test(m)) { bad++; continue; }
      CS Bc, Fc;
      canon(B, F, Bc, Fc);
      if (!(Bc == B) || !(Fc == F)) noncanon++;
      if (!table.emplace(mkKey(B, F, t, 1), m).second) dup++;
    }
  }
  {
    std::ifstream f(argv[2]);
    std::string line;
    while (std::getline(f, line)) {
      ll++;
      std::stringstream ss(line);
      std::string a, b, c, extra;
      ss >> a >> b >> c;
      if (c.empty() || (ss >> extra)) { bad++; continue; }
      CS B, F; bool asc1, asc2;
      int t = atoi(a.c_str());
      if (!parseSet(b, B, asc1) || !parseSet(c, F, asc2) || !asc1 || !asc2) { bad++; continue; }
      if (t < 0 || t > 60 || B.count() != t || F.count() > t || B.meets(F)) { bad++; continue; }
      CS Bc, Fc;
      canon(B, F, Bc, Fc);
      if (!(Bc == B) || !(Fc == F)) noncanon++;
      if (!leaves.insert(mkKey(B, F, t, 0)).second) dup++;
    }
  }
  fprintf(stderr, "loaded %ld table lines, %ld leaf lines, bad %ld, noncanonical %ld, duplicate keys %ld\n", tl, ll, bad, noncanon, dup);
  std::unordered_set<Key, KeyHash> visited;
  std::vector<Item> stack;
  long errors = 0;
  long P[16] = {0}, Q[16] = {0}, LF[16] = {0}, thr[16] = {0}, dng[16] = {0}, vthr[16] = {0}, vdng[16] = {0}, vrev[16] = {0}, rev[16] = {0};
  long nonbusy = 0;
  auto err = [&](const char* what, const Item& it) {
    errors++;
    if (errors <= 20) {
      uint8_t c[NC];
      fprintf(stderr, "ERROR %s: %s t=%d B=", what, it.kind ? "Q" : "P", it.t);
      int n = it.B.list(c); for (int i = 0; i < n; i++) fprintf(stderr, "%d,", c[i]);
      fprintf(stderr, " F=");
      n = it.F.list(c); for (int i = 0; i < n; i++) fprintf(stderr, "%d,", c[i]);
      fprintf(stderr, "\n");
    }
  };
  auto push = [&](const CS& B, const CS& F, int t, int kind) {
    CS Bc, Fc;
    canon(B, F, Bc, Fc);
    if (visited.insert(mkKey(Bc, Fc, t, kind)).second) { Item it; it.B = Bc; it.F = Fc; it.t = t; it.kind = kind; stack.push_back(it); }
  };
  { Item r; r.t = 0; r.kind = 0; visited.insert(mkKey(r.B, r.F, 0, 0)); stack.push_back(r); }
  while (!stack.empty()) {
    Item it = stack.back();
    stack.pop_back();
    const CS& B = it.B; const CS& F = it.F; int t = it.t;
    int nF = F.count();
    if (it.kind == 0) {
      P[t]++;
      if (B.count() != t || nF > t || B.meets(F)) { err("malformed P-state", it); continue; }
      if (!admissible(B, F)) { err("VOID P-state queued", it); continue; }
      Key k = mkKey(B, F, t, 0);
      if (leaves.count(k)) { LF[t]++; usedLeaves.insert(k); continue; }
      if (t >= rmax) { err("non-leaf P-state with t >= R_max", it); continue; }
      // (a)
      if (visited.insert(mkKey(B, F, t + 1, 1)).second) { Item q = it; q.t = t + 1; q.kind = 1; stack.push_back(q); }
      bool live[160];
      for (int l = 0; l < NL; l++) live[l] = !LSET[l].meets(B);
      // (c) every live line l, every e in l \ F with |F u (l \ {e})| <= t+1
      for (int l = 0; l < NL; l++) {
        if (!live[l]) continue;
        for (int i = 0; i < 6; i++) {
          int e = LINE[l][i];
          if (F.test(e)) continue;
          CS F2 = F | LSET[l];
          F2.reset(e);
          if (F2.count() > t + 1) continue;
          CS B2 = B; B2.set(e);
          if (!admissible(B2, F2)) { vthr[t + 1]++; continue; }
          thr[t + 1]++;
          push(B2, F2, t + 1, 0);
        }
      }
      // (d) every unordered pair of live lines a != b with a common cell c not in F, all 4-subsets Qa of a\{c},
      //     Qb of b\{c} containing F∩a resp. F∩b, |F u Qa u Qb| <= t+1
      for (int a = 0; a < NL; a++) {
        if (!live[a]) continue;
        for (int b = a + 1; b < NL; b++) {
          if (!live[b]) continue;
          int c = MEETC[a][b];
          if (c < 0 || F.test(c)) continue;
          CS Fa = F & LSET[a], Fb = F & LSET[b];
          if (nF + (4 - Fa.count()) + (4 - Fb.count()) > t + 1) continue;   // exact size of F u Qa u Qb (asserted below)
          for (int i = 0; i < 6; i++) {
            int ya = LINE[a][i];
            if (ya == c) continue;
            CS Qa = LSET[a]; Qa.reset(c); Qa.reset(ya);
            if (!((Fa & Qa) == Fa)) continue;
            for (int j = 0; j < 6; j++) {
              int yb = LINE[b][j];
              if (yb == c) continue;
              CS Qb = LSET[b]; Qb.reset(c); Qb.reset(yb);
              if (!((Fb & Qb) == Fb)) continue;
              CS F2 = F | Qa | Qb;
              if (F2.count() != nF + (4 - Fa.count()) + (4 - Fb.count())) { fprintf(stderr, "internal: size identity\n"); return 4; }
              if (F2.count() > t + 1) continue;
              CS B2 = B; B2.set(c);
              if (!admissible(B2, F2)) { vdng[t + 1]++; continue; }
              dng[t + 1]++;
              push(B2, F2, t + 1, 0);
            }
          }
        }
      }
    } else {
      Q[t]++;
      if (t < 1 || B.count() != t - 1 || nF > t || B.meets(F)) { err("malformed Q-state", it); continue; }
      if (!admissible(B, F)) { err("VOID Q-state queued", it); continue; }
      Key k = mkKey(B, F, t, 1);
      auto f = table.find(k);
      if (f == table.end()) { err("no table entry (hole)", it); continue; }
      usedTable.insert(k);
      int m = f->second;
      if (m < 0 || m >= NC || B.test(m) || F.test(m)) { err("illegal table move", it); continue; }
      int x = m / 36, y = (m / 6) % 6, z = m % 6;
      if (!(std::min(x, 5 - x) == std::min(y, 5 - y) && std::min(y, 5 - y) == std::min(z, 5 - z))) nonbusy++;
      CS B1 = B; B1.set(m);
      if (!admissible(B1, F)) { err("child (i) is VOID", it); continue; }
      push(B1, F, t, 0);
      if (nF < t) {
        CS F1 = F; F1.set(m);
        if (!admissible(B, F1)) vrev[t]++;
        else { rev[t]++; push(B, F1, t, 1); }
      }
    }
  }
  long tp = 0, tq = 0, tlf = 0, tthr = 0, tdng = 0, tv1 = 0, tv2 = 0, tv3 = 0, trev = 0;
  for (int i = 0; i < 16; i++) { tp += P[i]; tq += Q[i]; tlf += LF[i]; tthr += thr[i]; tdng += dng[i]; tv1 += vthr[i]; tv2 += vdng[i]; tv3 += vrev[i]; trev += rev[i]; }
  bool ok = errors == 0 && bad == 0 && noncanon == 0 && dup == 0;
  auto arr = [](const long* a) { std::string s = "["; for (int i = 0; i <= 10; i++) { s += std::to_string(a[i]); if (i < 10) s += ","; } return s + "]"; };
  printf("{\"ok\":%s,\"rmax\":%d,\"errors\":%ld,\"table_lines\":%ld,\"leaf_lines\":%ld,\"bad_lines\":%ld,\"noncanonical_lines\":%ld,\"duplicate_keys\":%ld,\n",
         ok ? "true" : "false", rmax, errors, tl, ll, bad, noncanon, dup);
  printf(" \"p_states\":%ld,\"q_states\":%ld,\"leaves_reached\":%ld,\"p_expanded\":%ld,\"states\":%ld,\n", tp, tq, tlf, tp - tlf, tp + tq);
  printf(" \"threat_children\":%ld,\"danger_children\":%ld,\"reveal_children\":%ld,\"void_threat\":%ld,\"void_danger\":%ld,\"void_reveal\":%ld,\"nonbusy_moves\":%ld,\n",
         tthr, tdng, trev, tv1, tv2, tv3, nonbusy);
  printf(" \"table_entries_used\":%zu,\"table_entries\":%zu,\"leaf_entries_used\":%zu,\"leaf_entries\":%zu,\n", usedTable.size(), table.size(), usedLeaves.size(), leaves.size());
  printf(" \"by_t\":{\"p_states\":%s,\"q_states\":%s,\"leaves\":%s,\"threat_children\":%s,\"danger_children\":%s,\"void_reveal\":%s}}\n",
         arr(P).c_str(), arr(Q).c_str(), arr(LF).c_str(), arr(thr).c_str(), arr(dng).c_str(), arr(vrev).c_str());
  return ok ? 0 : 1;
}

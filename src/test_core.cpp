// test_core -- tests for match.hpp / phi.hpp (SPEC 7.2, 7.3, 8).  Runs in well under a minute.
//   bin/test_core            all tests + micro-benchmark
//   bin/test_core --bench    micro-benchmark only
// Exit code 0 iff every check passed.  The helpers t_* below are written from the definitions of
// SPEC 3 and do not use the matcher of match.hpp (they are the "naive" reference).
#include "phi.hpp"
#include <chrono>
#include <omp.h>
#include <time.h>
using namespace ttt;

// ------------------------------------------------------------------ infrastructure
static uint64_t rng_state = 0x243F6A8885A308D3ull;
static uint64_t rnd() {
  uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}
static int rndi(int n) { return (int)(rnd() % (uint64_t)n); }

static long nfail = 0, nchecks = 0;
#define CHECK(cond, ...)                                                        \
  do {                                                                          \
    nchecks++;                                                                  \
    if (!(cond)) {                                                              \
      nfail++;                                                                  \
      if (nfail <= 40) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } \
    }                                                                           \
  } while (0)

static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static double cpu_now() {   // CPU time of this thread (the machine is shared, wall time is noisy)
  timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static CellSet mk(std::initializer_list<std::array<int, 3>> v) {
  CellSet s;
  for (auto& c : v) s.set(cid(c[0], c[1], c[2]));
  return s;
}
static std::string inst(const CellSet& B, const CellSet& F, int k) {
  return std::to_string(k) + " " + to_str(B) + " " + to_str(F);
}

// ------------------------------------------------------------------ naive reference (SPEC 3)
static std::vector<int> t_live(const CellSet& B) {
  std::vector<int> v;
  for (int l = 0; l < NL; l++) if (!G().lineSet[l].intersects(B)) v.push_back(l);
  return v;
}
static int t_sur(const std::vector<int>& S, const CellSet& F) {
  CellSet N;
  for (int l : S) N = N | G().lineSet[l];
  return N.minus(F).count() - 2 * (int)S.size();
}
static bool t_fadj(int a, int b, const CellSet& F) { return a != b && !(G().lineSet[a] & G().lineSet[b]).minus(F).empty(); }
static bool t_fconn(const std::vector<int>& S, const CellSet& F) {
  if (S.empty()) return true;
  std::vector<char> seen(S.size(), 0);
  std::vector<int> st = {0};
  seen[0] = 1;
  size_t cnt = 1;
  while (!st.empty()) {
    int i = st.back();
    st.pop_back();
    for (size_t j = 0; j < S.size(); j++)
      if (!seen[j] && t_fadj(S[i], S[j], F)) { seen[j] = 1; cnt++; st.push_back((int)j); }
  }
  return cnt == S.size();
}
static bool t_admissible(const CellSet& B, const CellSet& X) {
  if (X.intersects(B)) return false;
  std::vector<int> four;
  for (int l = 0; l < NL; l++) {
    if (G().lineSet[l].intersects(B)) continue;
    int n = (G().lineSet[l] & X).count();
    if (n > 4) return false;
    if (n == 4) four.push_back(l);
  }
  for (size_t i = 0; i < four.size(); i++)
    for (size_t j = i + 1; j < four.size(); j++)
      if (!(G().lineSet[four[i]] & G().lineSet[four[j]]).minus(X).empty()) return false;
  return true;
}
// Maximum number of cells that can be assigned when live line live[i] may own cap[i] cells
// (plain Kuhn on "slots", from scratch).
struct TMatch {
  std::vector<int> slotLine;
  int matchCell[NC];
  int vis[NC];
  int stamp;
  const CellSet* X;
  bool dfs(int s) {
    int l = slotLine[s];
    for (int j = 0; j < 6; j++) {
      int c = G().line[l][j];
      if (X->test(c) || vis[c] == stamp) continue;
      vis[c] = stamp;
      if (matchCell[c] < 0 || dfs(matchCell[c])) { matchCell[c] = s; return true; }
    }
    return false;
  }
  int run(const std::vector<int>& live, const std::vector<int>& cap, const CellSet& X_) {
    X = &X_;
    slotLine.clear();
    for (size_t i = 0; i < live.size(); i++) for (int j = 0; j < cap[i]; j++) slotLine.push_back(live[i]);
    for (int c = 0; c < NC; c++) { matchCell[c] = -1; vis[c] = 0; }
    stamp = 0;
    int res = 0;
    for (size_t s = 0; s < slotLine.size(); s++) { stamp++; if (dfs((int)s)) res++; }
    return res;
  }
};
static bool t_pairing(const CellSet& B, const CellSet& X) {
  std::vector<int> live = t_live(B);
  std::vector<int> cap(live.size(), 2);
  TMatch m;
  return m.run(live, cap, X) == 2 * (int)live.size();
}
// min(cap, min over F-connected triples T of mu(T)) by a from-scratch matching per triple; -1 if no pairing.
static int t_phi(const CellSet& B, const CellSet& F, int capv, long* ntriples) {
  std::vector<int> live = t_live(B);
  int n = (int)live.size();
  if (!t_pairing(B, F)) return -1;
  std::vector<std::vector<char>> adj(n, std::vector<char>(n, 0));
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) adj[i][j] = t_fadj(live[i], live[j], F);
  int best = capv;
  TMatch m;
  std::vector<int> cap(n, 2);
  long cnt = 0;
  for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) for (int k = j + 1; k < n; k++) {
    int e = adj[i][j] + adj[j][k] + adj[i][k];
    if (e < 2) continue;
    cnt++;
    cap[i] = cap[j] = cap[k] = 6;
    int mu = m.run(live, cap, F) - 2 * n;
    cap[i] = cap[j] = cap[k] = 2;
    if (mu < best) best = mu;
  }
  if (ntriples) *ntriples = cnt;
  return best;
}
// C(B,F,k) by plain enumeration of every stone set (no pruning at all).
struct TEnum {
  CellSet B, F, X, viol;
  std::vector<int> cells;
  bool found;
  long sets;
  void rec(size_t start, int left) {
    if (found) return;
    sets++;
    if (t_admissible(B, X) && !t_pairing(B, X)) { found = true; viol = X; return; }
    if (left == 0) return;
    for (size_t i = start; i < cells.size() && !found; i++) {
      X.set(cells[i]);
      rec(i + 1, left - 1);
      X.reset(cells[i]);
    }
  }
  bool holds(const CellSet& B_, const CellSet& F_, int k) {
    B = B_; F = F_; X = F_;
    found = false;
    sets = 0;
    if (F.count() > k) return true;
    cells.clear();
    for (int c = 0; c < NC; c++) if (!B.test(c) && !F.test(c)) cells.push_back(c);
    rec(0, k - F.count());
    return !found;
  }
};

// ------------------------------------------------------------------ instance generators
static CellSet T7() { return mk({{0, 0, 0}, {5, 5, 5}, {1, 1, 4}, {4, 4, 1}, {2, 3, 2}, {3, 2, 3}, {5, 0, 0}}); }
static CellSet T8() { CellSet s = T7(); s.set(cid(0, 5, 5)); return s; }

static CellSet random_B() {
  const Geom& g = G();
  CellSet B;
  int type = rndi(4);
  if (type == 0) {                       // random busy cells (7..10) + optional non-busy cells
    int n = 7 + rndi(4);
    while (B.count() < n) B.set(g.busy[rndi(NB)]);
    int extra = rndi(3);
    for (int i = 0; i < extra; i++) B.set(rndi(NC));
  } else if (type == 1) {                // image of T8 + 0..3 random stones
    B = g.image(rndi(NG), T8());
    int extra = rndi(4);
    for (int i = 0; i < extra; i++) B.set(rndi(2) ? g.busy[rndi(NB)] : rndi(NC));
  } else if (type == 2) {                // image of T7 + 1..3 random stones
    B = g.image(rndi(NG), T7());
    int extra = 1 + rndi(3);
    for (int i = 0; i < extra; i++) B.set(rndi(2) ? g.busy[rndi(NB)] : rndi(NC));
  } else {                               // image of T8 with one stone moved
    B = g.image(rndi(NG), T8());
    std::vector<int> cs = B.cells();
    B.reset(cs[rndi((int)cs.size())]);
    B.set(g.busy[rndi(NB)]);
    if (rndi(2)) B.set(rndi(NC));
  }
  return B;
}
static CellSet random_F(const CellSet& B, int size) {
  const Geom& g = G();
  CellSet F;
  std::vector<int> live = t_live(B);
  int tries = 0;
  while (F.count() < size && tries++ < 200) {
    int c;
    int mode = rndi(3);
    if (mode == 0) c = g.busy[rndi(NB)];
    else if (mode == 1 && !live.empty()) {
      // prefer cells on a line that already carries a stone of F (creates caps / near-dangers)
      int l = live[rndi((int)live.size())];
      for (int t = 0; t < 8 && !g.lineSet[l].intersects(F); t++) l = live[rndi((int)live.size())];
      c = g.line[l][rndi(6)];
    } else c = rndi(NC);
    if (B.test(c) || F.test(c)) continue;
    CellSet F2 = F;
    F2.set(c);
    if (admissible(B, F2)) F = F2;
  }
  return F;
}

// ------------------------------------------------------------------ checks on results
// Validate a PhiResult against the definitions; returns the recomputed truth of the claim.
static void check_phi_result(const CellSet& B, const CellSet& F, int k, const PhiResult& r) {
  int theta = k - F.count();
  std::string id = inst(B, F, k);
  CHECK(r.holds == (r.reason == 0), "holds/reason mismatch %s", id.c_str());
  if (r.reason == 1) CHECK(!t_admissible(B, F), "reason 1 but F admissible: %s", id.c_str());
  if (r.reason != 1) CHECK(t_admissible(B, F), "F not admissible but reason %d: %s", r.reason, id.c_str());
  if (r.reason == 2) {
    CHECK(!r.S.empty() && t_sur(r.S, F) < 0, "reason 2 without Hall violator: %s", id.c_str());
    CHECK(t_sur(r.S, F) == r.sur, "reason 2 sur mismatch: %s", id.c_str());
    for (int l : r.S) CHECK(!G().lineSet[l].intersects(B), "reason 2: dead line in S: %s", id.c_str());
    CHECK(!t_pairing(B, F), "reason 2 but naive pairing exists: %s", id.c_str());
  }
  if (r.reason == 0 || r.reason == 3) CHECK(t_pairing(B, F), "reason %d but no naive pairing: %s", r.reason, id.c_str());
  if (r.reason == 3) {
    CHECK(r.S.size() >= 3, "reason 3: |S| < 3: %s", id.c_str());
    for (int l : r.S) CHECK(!G().lineSet[l].intersects(B), "reason 3: dead line in S: %s", id.c_str());
    int s = t_sur(r.S, F);
    CHECK(s == r.sur, "reason 3: sur %d reported %d: %s", s, r.sur, id.c_str());
    CHECK(s < theta, "reason 3: sur %d >= theta %d: %s", s, theta, id.c_str());
    CHECK(t_fconn(r.S, F), "reason 3: S not F-connected: %s", id.c_str());
  }
}

// F = empty: find an admissible X inside N(S) with |X| = need (plain backtracking, ascending cells).
struct VBuild {
  CellSet B, X;
  std::vector<int> cells;
  long nodes;
  bool rec(size_t start, int left) {
    if (left == 0) return true;
    if (++nodes > 200000) return false;
    for (size_t i = start; i < cells.size(); i++) {
      X.set(cells[i]);
      if (admissible(B, X) && rec(i + 1, left - 1)) return true;
      X.reset(cells[i]);
    }
    return false;
  }
  bool build(const CellSet& B_, const std::vector<int>& S, int need) {
    B = B_;
    X = CellSet();
    CellSet N;
    for (int l : S) N = N | G().lineSet[l];
    cells = N.cells();
    nodes = 0;
    return rec(0, need);
  }
};

// ------------------------------------------------------------------ 1. regression values (SPEC 8)
static void test_regression() {
  const Geom& g = G();
  printf("== regression values (SPEC 8, slack / LEAF) ==\n");
  CellSet E, t7 = T7(), t8 = T8();
  // item 2
  Pairing p7 = find_pairing(t7, E);
  printf("item 2: slack(T7) = %d (expect 1), pairing for (T7, empty): %s\n", slack(t7), p7.ok ? "yes" : "no");
  CHECK(slack(t7) == 1, "slack(T7)");
  CHECK(p7.ok, "pairing T7");
  bool l77 = leaf_holds(t7, E, 7);
  PhiResult r77 = phi_check(t7, E, 7);
  printf("item 2: LEAF(T7,-,7) = %d (expect 0; worst = 6)  reason %d, witness |S| = %zu, sur = %d; phi_value(T7,-,12) = %d\n",
         (int)l77, r77.reason, r77.S.size(), r77.sur, phi_value(t7, E, 12));
  CHECK(!l77 && !r77.holds, "LEAF(T7,-,7) must be false");
  check_phi_result(t7, E, 7, r77);
  // item 3
  bool l88 = leaf_holds(t8, E, 8), l89 = leaf_holds(t8, E, 9);
  PhiResult r89 = phi_check(t8, E, 9);
  printf("item 3: slack(T8) = %d (expect 8); LEAF(T8,-,8) = %d (expect 1; worst = 0); LEAF(T8,-,9) = %d (expect 0; worst = 1), witness |S| = %zu sur = %d; phi_value(T8,-,12) = %d\n",
         slack(t8), (int)l88, (int)l89, r89.S.size(), r89.sur, phi_value(t8, E, 12));
  CHECK(slack(t8) == 8, "slack(T8)");
  CHECK(l88 && phi_check(t8, E, 8).holds, "LEAF(T8,-,8) must be true");
  CHECK(!l89 && !r89.holds, "LEAF(T8,-,9) must be false");
  check_phi_result(t8, E, 9, r89);
  // item 4
  int n4 = 0, ok4 = 0;
  for (int i = 0; i < NB; i++) {
    int h = g.busy[i];
    if (t8.test(h)) continue;
    CellSet F;
    F.set(h);
    n4++;
    bool v = leaf_holds(t8, F, 8);
    ok4 += v;
    CHECK(v == phi_check(t8, F, 8).holds, "item 4 leaf_holds/phi_check");
  }
  printf("item 4: LEAF(T8,{h},8) true for %d of %d busy h not in T8 (expect 16 of 16; worst = 0)\n", ok4, n4);
  CHECK(n4 == 16 && ok4 == 16, "item 4");
  // item 5
  {
    CellSet B = t8, F;
    B.reset(cid(5, 5, 5));
    B.set(cid(1, 1, 1));
    F.set(cid(5, 5, 5));
    PhiResult r = phi_check(B, F, 8);
    printf("item 5: LEAF(T8-(5,5,5)+(1,1,1), {(5,5,5)}, 8) = %d (worst = 0, so expected 1)  reason %d; phi_value(cap 12) = %d\n",
           (int)r.holds, r.reason, phi_value(B, F, 12));
    CHECK(r.holds && leaf_holds(B, F, 8), "item 5 LEAF expected true");
    check_phi_result(B, F, 8, r);
  }
  // item 6
  {
    int seq[10][3] = {{0, 0, 0}, {1, 1, 4}, {2, 3, 2}, {0, 5, 5}, {1, 1, 1}, {2, 2, 2}, {0, 0, 5}, {1, 4, 1}, {2, 2, 3}, {0, 5, 0}};
    for (int r = 8; r <= 10; r++) {
      CellSet B, F;
      for (int i = 0; i < r; i++) {
        int c = cid(seq[i][0], seq[i][1], seq[i][2]);
        B.set(c);
        if (i < r - 1) F.set(g.antip[c]);
      }
      PhiResult res = phi_check(B, F, r);
      bool lh = leaf_holds(B, F, r);
      int pv = (res.reason == 1 || res.reason == 2) ? -1 : phi_value(B, F, 12);
      printf("item 6: antipode stealing after %d rounds (|F| = %d): LEAF = %d (expect %d; worst = %d)  reason %d sur %d |S| %zu  phi_value(cap 12) = %d\n",
             r, F.count(), (int)lh, r == 8 ? 0 : 1, r == 8 ? 6 : 0, res.reason, res.sur, res.S.size(), pv);
      CHECK(lh == res.holds, "item 6 leaf_holds/phi_check");
      CHECK(lh == (r != 8), "item 6 round %d", r);
      check_phi_result(B, F, r, res);
    }
  }
  // item 7
  {
    CellSet F;
    for (int x = 1; x <= 5; x++) F.set(cid(x, 1, 2));
    CellSet B8 = t8, B7 = t7;
    B8.set(cid(0, 1, 2));
    B7.set(cid(0, 1, 2));
    PhiResult a = phi_check(B8, F, 9), b = phi_check(B7, F, 8);
    printf("item 7: threat branch LEAF(T8+(0,1,2), F, 9) = %d (expect 1; worst = 0), phi_value(cap 12) = %d;  LEAF(T7+(0,1,2), F, 8) = %d (expect 0; worst = 2) reason %d sur %d |S| %zu\n",
           (int)a.holds, phi_value(B8, F, 12), (int)b.holds, b.reason, b.sur, b.S.size());
    CHECK(a.holds && leaf_holds(B8, F, 9), "item 7 T8 variant must be true");
    CHECK(!b.holds && !leaf_holds(B7, F, 8), "item 7 T7 variant must be false");
    check_phi_result(B8, F, 9, a);
    check_phi_result(B7, F, 8, b);
    // exact cross-check of the T7 variant by enumeration (theta = 3): C must fail, since worst = 2
    EnumResult e = enum_check(B7, F, 8);
    printf("item 7: enum_check(T7+(0,1,2), F, 8): status %d (expect 0), nodes %llu\n", e.status, (unsigned long long)e.nodes);
    CHECK(e.status == 0, "item 7 enum T7 variant");
  }
  // small-k exact values around T7/T8 by enumeration (C == LEAF for F empty, k <= 10)
  for (int k = 0; k <= 3; k++) {
    EnumResult e7 = enum_check(t7, E, k), e8 = enum_check(t8, E, k);
    bool l7 = leaf_holds(t7, E, k), l8 = leaf_holds(t8, E, k);
    printf("small k: C(T7,-,%d) = %d LEAF = %d (nodes %llu);  C(T8,-,%d) = %d LEAF = %d (nodes %llu)\n", k, e7.status, (int)l7,
           (unsigned long long)e7.nodes, k, e8.status, (int)l8, (unsigned long long)e8.nodes);
    CHECK((e7.status == 1) == l7, "C vs LEAF, T7 k=%d", k);
    CHECK((e8.status == 1) == l8, "C vs LEAF, T8 k=%d", k);
  }
}

// ------------------------------------------------------------------ 2. basic functions vs naive
static void test_basic() {
  printf("== live_lines / slack / admissible / find_pairing against the naive reference ==\n");
  const Geom& g = G();
  long nadm = 0, npair = 0;
  for (int it = 0; it < 3000; it++) {
    CellSet B = random_B();
    // live lines, slack
    int out[NL];
    int n = live_lines(B, out);
    std::vector<int> live = t_live(B);
    CHECK(n == (int)live.size() && std::equal(live.begin(), live.end(), out), "live_lines");
    CHECK(slack(B) == NC - B.count() - 2 * n, "slack");
    // admissible on random X, including dense fills of lines
    CellSet X;
    int mode = rndi(3);
    if (mode == 0) { int m = rndi(12); for (int i = 0; i < m; i++) X.set(rndi(NC)); }
    else {
      int nlines = 1 + rndi(3);
      for (int i = 0; i < nlines; i++) {
        int l = live.empty() ? rndi(NL) : live[rndi((int)live.size())];
        if (i > 0 && rndi(2)) {                 // a line crossing the previous stones
          std::vector<int> xs = X.cells();
          if (!xs.empty()) { int c = xs[rndi((int)xs.size())]; l = g.lc[c][rndi(g.nlc[c])]; }
        }
        int cnt = 3 + rndi(3);
        for (int j = 0; j < cnt; j++) X.set(g.line[l][rndi(6)]);
      }
      if (mode == 2) X = X.minus(B);
    }
    bool a = admissible(B, X), ta = t_admissible(B, X);
    nadm += a;
    CHECK(a == ta, "admissible mismatch: B=%s X=%s lib=%d naive=%d", to_str(B).c_str(), to_str(X).c_str(), (int)a, (int)ta);
    if (it < 1000) {
      CellSet X2 = X.minus(B);
      Pairing p = find_pairing(B, X2);
      bool tp = t_pairing(B, X2);
      npair += p.ok;
      CHECK(p.ok == tp, "find_pairing mismatch: B=%s X=%s", to_str(B).c_str(), to_str(X2).c_str());
      if (p.ok) {
        std::vector<int> cnt(NL, 0);
        bool good = p.hall.empty();
        for (int c = 0; c < NC; c++) {
          int l = p.owner[c];
          if (l < 0) continue;
          if (l >= NL || !g.lineSet[l].test(c) || X2.test(c) || B.test(c) || g.lineSet[l].intersects(B)) { good = false; continue; }
          cnt[l]++;
        }
        for (int l = 0; l < NL; l++) if (cnt[l] != (g.lineSet[l].intersects(B) ? 0 : 2)) good = false;
        CHECK(good, "find_pairing returned an invalid pairing: B=%s X=%s", to_str(B).c_str(), to_str(X2).c_str());
      } else {
        bool good = !p.hall.empty() && t_sur(p.hall, X2) < 0;
        for (int l : p.hall) if (g.lineSet[l].intersects(B)) good = false;
        for (int c = 0; c < NC; c++) if (p.owner[c] != -1) good = false;
        CHECK(good, "find_pairing returned an invalid Hall violator: B=%s X=%s", to_str(B).c_str(), to_str(X2).c_str());
      }
    }
  }
  printf("3000 admissible comparisons (%ld admissible), 1000 find_pairing comparisons (%ld with pairing)\n", nadm, npair);
}

// ------------------------------------------------------------------ 3. consistency on random instances
static void test_random(int ninst) {
  printf("== consistency on %d random instances (a) witnesses (b) LEAF => C (c) LEAF false => C false for F empty (e) monotonicity ==\n", ninst);
  const Geom& g = G();
  long cnt_true = 0, cnt_r1 = 0, cnt_r2 = 0, cnt_r3 = 0, cnt_b = 0, cnt_b3 = 0, cnt_c = 0, cnt_c_enum = 0, cnt_mono = 0, cnt_val = 0;
  long cnt_enum_false_leaf_false = 0, cnt_enum_true_leaf_false = 0;
  double t_enum3 = 0;
  for (int it = 0; it < ninst; it++) {
    CellSet B = random_B();
    bool emptyF = (it % 3 == 0);
    CellSet F = emptyF ? CellSet() : random_F(B, rndi(7));
    int nf = F.count();
    int theta = (it % 5 == 4) ? rndi(9) : rndi(4);
    int k = nf + theta;
    std::string id = inst(B, F, k);
    PhiResult r = phi_check(B, F, k);
    bool lh = leaf_holds(B, F, k);
    CHECK(lh == r.holds, "leaf_holds != phi_check: %s", id.c_str());
    check_phi_result(B, F, k, r);                                             // (a)
    if (r.reason == 0) cnt_true++;
    if (r.reason == 1) cnt_r1++;
    if (r.reason == 2) cnt_r2++;
    if (r.reason == 3) cnt_r3++;

    // phi_value: threshold behaviour of LEAF in k
    if (r.reason == 0 || r.reason == 3) {
      int v = phi_value(B, F, 10);
      CHECK(v >= 0 && v <= 10, "phi_value range: %s", id.c_str());
      for (int th = 0; th <= 10; th++) {
        bool expect = (th <= v);
        if (v == 10 && th == 10) expect = true;
        CHECK(leaf_holds(B, F, nf + th) == expect, "phi_value %d vs LEAF at theta %d: %s", v, th, id.c_str());
      }
      CHECK(r.holds == (theta <= v), "phi_value %d vs phi_check (theta %d): %s", v, theta, id.c_str());
      if (r.reason == 3) CHECK(r.sur >= v, "witness surplus %d below phi_value %d: %s", r.sur, v, id.c_str());
      cnt_val++;
    } else {
      CHECK(phi_value(B, F, 10) == -1, "phi_value must be -1 when reasons 1/2 apply: %s", id.c_str());
    }

    // (b) LEAF true => C true (exact enumeration), theta <= 3
    if (r.holds && theta <= 3 && (theta <= 2 || cnt_b3 < 12)) {
      double t0 = now();
      EnumResult e = enum_check(B, F, k);
      if (theta == 3) { cnt_b3++; t_enum3 += now() - t0; }
      cnt_b++;
      CHECK(e.status == 1, "LEAF true but enum_check status %d: %s (X=%s)", e.status, id.c_str(), to_str(e.X).c_str());
    }
    // LEAF false with reason 3 and theta <= 2: record what C says (LEAF is only sufficient for F != empty)
    if (r.reason == 3 && theta <= 2 && !emptyF) {
      EnumResult e = enum_check(B, F, k);
      if (e.status == 0) cnt_enum_false_leaf_false++; else cnt_enum_true_leaf_false++;
    }
    // (c) F empty, k <= 10, LEAF false => C false, with an explicit violator
    if (emptyF && !r.holds && k <= 10) {
      cnt_c++;
      if (r.reason == 2) {
        CHECK(!find_pairing(B, F).ok, "reason 2 but find_pairing ok: %s", id.c_str());
      } else {
        VBuild vb;
        bool built = vb.build(B, r.S, r.sur + 1);
        CHECK(built, "could not build an admissible violator inside N(S): %s", id.c_str());
        if (built) {
          CHECK(vb.X.count() == r.sur + 1 && vb.X.count() <= k, "violator size: %s", id.c_str());
          CHECK(t_admissible(B, vb.X), "violator not admissible: %s", id.c_str());
          Pairing p = find_pairing(B, vb.X);
          CHECK(!p.ok, "violator has a pairing: %s X=%s", id.c_str(), to_str(vb.X).c_str());
          CHECK(!t_pairing(B, vb.X), "violator has a naive pairing: %s", id.c_str());
        }
      }
      if (k <= 3) {
        cnt_c_enum++;
        EnumResult e = enum_check(B, F, k);
        CHECK(e.status == 0, "F empty, LEAF false, but enum_check status %d: %s", e.status, id.c_str());
      }
    }
    // (e) monotonicity of LEAF (SPEC 3)
    if (r.holds) {
      for (int t = 0; t < 4; t++) {                       // F' = F + f, admissible, |F'| <= k
        int f = rndi(2) ? g.busy[rndi(NB)] : rndi(NC);
        CellSet F2 = F;
        F2.set(f);
        if (B.test(f) || F.test(f) || nf + 1 > k || !admissible(B, F2)) continue;
        cnt_mono++;
        CHECK(leaf_holds(B, F2, k), "monotone in F violated: %s + f=%d", id.c_str(), f);
      }
      for (int k2 = nf; k2 < k; k2++) { cnt_mono++; CHECK(leaf_holds(B, F, k2), "monotone in k violated: %s k'=%d", id.c_str(), k2); }
      for (int t = 0; t < 4; t++) {                       // B' = B + b, b not in F
        int b = rndi(2) ? g.busy[rndi(NB)] : rndi(NC);
        if (B.test(b) || F.test(b)) continue;
        CellSet B2 = B;
        B2.set(b);
        cnt_mono++;
        CHECK(leaf_holds(B2, F, k), "monotone in B violated: %s + b=%d", id.c_str(), b);
      }
    }
    // (e) monotonicity of admissibility (SPEC 3): subsets, and larger B' disjoint from X
    if (r.reason != 1 && nf > 0) {
      std::vector<int> fs = F.cells();
      CellSet Fs = F;
      Fs.reset(fs[rndi(nf)]);
      CHECK(admissible(B, Fs), "subset of admissible set not admissible: %s", id.c_str());
      int b = rndi(NC);
      if (!B.test(b) && !F.test(b)) { CellSet B2 = B; B2.set(b); CHECK(admissible(B2, F), "admissible w.r.t. B but not B+b: %s b=%d", id.c_str(), b); }
      cnt_mono += 2;
    }
  }
  printf("LEAF true %ld, reason 1: %ld, reason 2: %ld, reason 3: %ld; phi_value threshold checks on %ld instances\n", cnt_true, cnt_r1, cnt_r2, cnt_r3, cnt_val);
  printf("(b) LEAF true => enum_check status 1: %ld instances (theta = 3: %ld, %.2f s)\n", cnt_b, cnt_b3, t_enum3);
  printf("(c) F empty, LEAF false: %ld instances with explicit violator / Hall failure, %ld also confirmed by enum_check\n", cnt_c, cnt_c_enum);
  printf("    F nonempty, LEAF false by reason 3, theta <= 2: C false in %ld cases, C true in %ld cases (LEAF is only sufficient)\n",
         cnt_enum_false_leaf_false, cnt_enum_true_leaf_false);
  printf("(e) %ld monotonicity checks\n", cnt_mono);
}

// ------------------------------------------------------------------ 3b. more targeted instances
static void test_targeted() {
  printf("== targeted instances: inadmissible F (reason 1), F empty with k = 4..10 (c), random busy 8-sets (SPEC 8 item 9) ==\n");
  const Geom& g = G();
  // reason 1: five stones on a live line, or a danger pair
  long n1 = 0;
  for (int it = 0; it < 200; it++) {
    CellSet B = random_B();
    std::vector<int> live = t_live(B);
    if (live.size() < 2) continue;
    CellSet F = random_F(B, rndi(4));
    int a = live[rndi((int)live.size())];
    if (it % 2 == 0) {
      int skip = rndi(6);
      for (int j = 0; j < 6; j++) if (j != skip || rndi(4) == 0) F.set(g.line[a][j]);     // 5 or 6 stones on a
    } else {
      // danger pair: a and a line b crossing it at c, four stones on each, c empty
      int c = g.line[a][rndi(6)], b = -1;
      for (int q = 0; q < g.nlc[c]; q++) { int l = g.lc[c][q]; if (l != a && !g.lineSet[l].intersects(B)) b = l; }
      if (b < 0) continue;
      F = CellSet();
      int na = 0, nb = 0;
      for (int j = 0; j < 6 && na < 4; j++) if (g.line[a][j] != c) { F.set(g.line[a][j]); na++; }
      for (int j = 5; j >= 0 && nb < 4; j--) if (g.line[b][j] != c) { F.set(g.line[b][j]); nb++; }
    }
    if (F.intersects(B)) continue;
    if (t_admissible(B, F)) continue;
    n1++;
    int k = F.count() + rndi(4);
    PhiResult r = phi_check(B, F, k);
    CHECK(!admissible(B, F), "inadmissible F accepted: %s", inst(B, F, k).c_str());
    CHECK(!r.holds && r.reason == 1 && !leaf_holds(B, F, k), "reason 1 expected: %s", inst(B, F, k).c_str());
    CHECK(phi_value(B, F, 5) == -1, "phi_value on inadmissible F: %s", inst(B, F, k).c_str());
    CHECK(enum_check(B, F, k).status == 1, "enum_check on inadmissible F must be vacuous: %s", inst(B, F, k).c_str());
  }
  // (c) again with larger k: F empty, k = 4..10
  long nc = 0, ncf = 0;
  for (int it = 0; it < 400; it++) {
    CellSet B = random_B(), F;
    int k = 4 + rndi(7);
    PhiResult r = phi_check(B, F, k);
    CHECK(r.holds == leaf_holds(B, F, k), "leaf_holds != phi_check: %s", inst(B, F, k).c_str());
    check_phi_result(B, F, k, r);
    nc++;
    if (r.holds) continue;
    ncf++;
    if (r.reason == 2) { CHECK(!find_pairing(B, F).ok && !t_pairing(B, F), "reason 2: %s", inst(B, F, k).c_str()); continue; }
    VBuild vb;
    bool built = vb.build(B, r.S, r.sur + 1);
    CHECK(built, "could not build an admissible violator inside N(S): %s", inst(B, F, k).c_str());
    if (built) {
      CHECK(vb.X.count() == r.sur + 1 && vb.X.count() <= k && t_admissible(B, vb.X), "violator size/admissibility: %s", inst(B, F, k).c_str());
      CHECK(!find_pairing(B, vb.X).ok && !t_pairing(B, vb.X), "violator has a pairing: %s X=%s", inst(B, F, k).c_str(), to_str(vb.X).c_str());
    }
  }
  // SPEC 8 item 9: random busy 8-sets are mostly bad
  long n8 = 0, good8 = 0, hist[12] = {0};
  for (int it = 0; it < 2000; it++) {
    CellSet B;
    while (B.count() < 8) B.set(g.busy[rndi(NB)]);
    n8++;
    bool v = leaf_holds(B, CellSet(), 8);
    good8 += v;
    PhiResult r = phi_check(B, CellSet(), 8);
    CHECK(r.holds == v, "busy 8-set: leaf_holds != phi_check");
    int pv = (r.reason == 2) ? -1 : phi_value(B, CellSet(), 10);
    hist[pv + 1]++;
  }
  printf("reason 1: %ld inadmissible instances;  (c) F empty, k = 4..10: %ld instances, %ld LEAF false, all with explicit violator\n", n1, nc, ncf);
  printf("item 9: %ld of %ld random busy 8-sets have LEAF(B,-,8) true; phi_value(B,-,10) histogram (no pairing, 0, 1, .., 10):", good8, n8);
  for (int i = 0; i < 12; i++) printf(" %ld", hist[i]);
  printf("\n");
}

// ------------------------------------------------------------------ 3c. thread safety
static void test_threads() {
  printf("== thread safety: 4 OpenMP threads against the serial results ==\n");
  const int n = 400;
  std::vector<CellSet> Bs(n), Fs(n);
  std::vector<int> ks(n), ref(n), refv(n), refe(n);
  for (int i = 0; i < n; i++) {
    Bs[i] = random_B();
    Fs[i] = (i % 3) ? random_F(Bs[i], rndi(6)) : CellSet();
    ks[i] = Fs[i].count() + rndi(7);
    PhiResult r = phi_check(Bs[i], Fs[i], ks[i]);
    ref[i] = r.reason * 1000 + (r.reason >= 2 ? r.sur + 500 : 0) + 100000 * (int)r.S.size();
    refv[i] = phi_value(Bs[i], Fs[i], 6);
    refe[i] = enum_check(Bs[i], Fs[i], Fs[i].count() + 1).status;
  }
  long bad = 0;
#pragma omp parallel for num_threads(4) schedule(dynamic, 1) reduction(+ : bad)
  for (int j = 0; j < 4 * n; j++) {
    int i = j % n;
    PhiResult r = phi_check(Bs[i], Fs[i], ks[i]);
    int v = r.reason * 1000 + (r.reason >= 2 ? r.sur + 500 : 0) + 100000 * (int)r.S.size();
    if (v != ref[i]) bad++;
    if (leaf_holds(Bs[i], Fs[i], ks[i]) != (r.reason == 0)) bad++;
    if (phi_value(Bs[i], Fs[i], 6) != refv[i]) bad++;
    if (enum_check(Bs[i], Fs[i], Fs[i].count() + 1).status != refe[i]) bad++;
    if (find_pairing(Bs[i], Fs[i]).ok != (r.reason == 0 || r.reason == 3)) bad++;
  }
  CHECK(bad == 0, "%ld mismatches between threaded and serial results", bad);
  printf("%d instances x 4 passes, %ld mismatches\n", n, bad);
}

// ------------------------------------------------------------------ 4. enum_check vs naive enumeration
static void test_enum_naive() {
  printf("== (d) enum_check against the naive enumeration ==\n");
  const Geom& g = G();
  int agree = 0, nholds = 0;
  double tn = 0, te = 0;
  auto one = [&](const CellSet& B, const CellSet& F, int k) {
    std::string id = inst(B, F, k);
    double t0 = now();
    EnumResult e = enum_check(B, F, k);
    double t1 = now();
    TEnum tnv;
    bool h = !t_admissible(B, F) ? true : tnv.holds(B, F, k);
    double t2 = now();
    te += t1 - t0;
    tn += t2 - t1;
    CHECK(e.status == (h ? 1 : 0), "enum_check %d vs naive %d: %s", e.status, (int)h, id.c_str());
    if (e.status == (h ? 1 : 0)) agree++;
    nholds += h;
    if (e.status == 0) {
      bool good = F.subset_of(e.X) && e.X.count() <= k && t_admissible(B, e.X) && !t_pairing(B, e.X) && !e.S.empty() && t_sur(e.S, e.X) < 0;
      for (int l : e.S) if (g.lineSet[l].intersects(B)) good = false;
      CHECK(good, "enum_check violator invalid: %s X=%s", id.c_str(), to_str(e.X).c_str());
    }
  };
  int n = 0;
  for (int it = 0; it < 36; it++) {
    CellSet B = random_B();
    CellSet F = (it % 2) ? random_F(B, rndi(7)) : CellSet();
    int k = F.count() + (it % 6 == 5 ? 2 : rndi(2));
    one(B, F, k);
    n++;
  }
  // inadmissible F and |F| > k are vacuous
  {
    CellSet B = T8(), F;
    for (int j = 0; j < 5; j++) F.set(g.line[t_live(B)[0]][j]);
    EnumResult e = enum_check(B, F, 6);
    CHECK(e.status == 1 && !admissible(B, F), "inadmissible F must be vacuous");
    CellSet F1;
    F1.set(g.line[t_live(B)[0]][0]);
    CHECK(enum_check(B, F1, 0).status == 1, "|F| > k must be vacuous");
  }
  // depth 3: one holding instance (T8 image + many extra stones keeps the naive run short) and failing ones
  {
    CellSet B = g.image(rndi(NG), T8());
    while (B.count() < 100) B.set(rndi(NC));
    one(B, CellSet(), 3);
    n++;
    CellSet B2 = g.image(rndi(NG), T7());
    one(B2, CellSet(), 3);
    n++;
    CellSet B3 = g.image(rndi(NG), T7());
    B3.set(rndi(NC));
    one(B3, random_F(B3, 3), 0);
    n++;
  }
  // node limit
  {
    EnumResult e = enum_check(T8(), CellSet(), 3, 1000);
    CHECK(e.status == -1 && e.nodes > 1000, "node limit");
  }
  printf("%d instances, %d agree (%d hold); enum_check %.3f s, naive %.2f s\n", n, agree, nholds, te, tn);
}

// ------------------------------------------------------------------ 5. phi_value vs naive per-triple max-flow
static void test_phi_naive() {
  printf("== phi_value against a from-scratch matching per F-connected triple ==\n");
  const Geom& g = G();
  int n = 0, agree = 0;
  long triples = 0;
  double t0 = now();
  for (int it = 0; it < 16; it++) {
    CellSet B = g.image(rndi(NG), (it % 4 == 3) ? T7() : T8());
    int extra = (it < 2) ? 0 : (it % 4 == 3) ? 1 + rndi(3) : rndi(7);
    while (B.count() < (it % 4 == 3 ? 7 : 8) + extra) B.set(rndi(2) ? g.busy[rndi(NB)] : rndi(NC));
    CellSet F = (it == 0) ? CellSet() : random_F(B, 1 + rndi(8));
    long nt = 0;
    int tv = t_phi(B, F, 10, &nt);
    int v = phi_value(B, F, 10);
    triples += nt;
    n++;
    if (tv == v) agree++;
    CHECK(tv == v, "phi_value %d vs naive %d: B=%s F=%s", v, tv, to_str(B).c_str(), to_str(F).c_str());
    printf("  |B| = %d |F| = %d: phi_value = %d, naive = %d (%ld triples)\n", B.count(), F.count(), v, tv, nt);
  }
  printf("%d instances, %d agree, %ld triples, %.2f s\n", n, agree, triples, now() - t0);
  // many smaller boards (25..70 SP stones) with all kinds of F: blocked cells, statically unsafe lines, ...
  int n2 = 0, agree2 = 0, hist[12] = {0};
  long triples2 = 0;
  double t1 = now();
  for (int it = 0; it < 150; it++) {
    CellSet B = (it % 2) ? g.image(rndi(NG), T8()) : CellSet();
    int target = 25 + rndi(46);
    while (B.count() < target) B.set(rndi(NC));
    CellSet F = random_F(B, rndi(9));
    long nt = 0;
    int tv = t_phi(B, F, 10, &nt);
    PhiResult r = phi_check(B, F, F.count());
    int v = (r.reason == 2) ? -1 : phi_value(B, F, 10);
    triples2 += nt;
    n2++;
    if (tv == v) agree2++;
    hist[v + 1]++;
    CHECK(tv == v, "phi_value %d vs naive %d: B=%s F=%s", v, tv, to_str(B).c_str(), to_str(F).c_str());
  }
  printf("%d smaller boards, %d agree, %ld triples, %.2f s; phi_value histogram (no pairing, 0..10):", n2, agree2, triples2, now() - t1);
  for (int i = 0; i < 12; i++) printf(" %d", hist[i]);
  printf("\n");
}

// ------------------------------------------------------------------ 6. micro-benchmark
static void bench() {
  printf("== micro-benchmark (thread CPU time, best of 3): leaf_holds on T8-like sets (B = image of T8 + 0..2 stones, |F| = 0..4) ==\n");
  const Geom& g = G();
  std::vector<CellSet> Bs, Fs;
  for (int i = 0; i < 300; i++) {
    CellSet B = g.image(rndi(NG), T8());
    int extra = rndi(3);
    for (int j = 0; j < extra; j++) B.set(rndi(2) ? g.busy[rndi(NB)] : rndi(NC));
    Bs.push_back(B);
    Fs.push_back(random_F(B, rndi(5)));
  }
  volatile int sink = 0;
  for (int theta = 0; theta <= 8; theta += 2) {
    double tt = 0, tf = 0, tmax = 0;
    long nt = 0, nfalse = 0;
    for (size_t i = 0; i < Bs.size(); i++) {
      int k = Fs[i].count() + theta;
      int reps = theta <= 4 ? 20 : 2;
      double best = 1e9;
      bool v = false;
      for (int rep = 0; rep < 3; rep++) {
        double t0 = cpu_now();
        for (int q = 0; q < reps; q++) v = leaf_holds(Bs[i], Fs[i], k);
        double t = (cpu_now() - t0) / reps;
        if (t < best) best = t;
      }
      sink += v;
      if (v) { tt += best; nt++; } else { tf += best; nfalse++; }
      if (best > tmax) tmax = best;
    }
    printf("theta = %d: %7.2f us per call on average (true: %ld calls, %8.2f us avg;  false: %ld calls, %7.2f us avg;  max %.1f us)\n", theta,
           1e6 * (tt + tf) / Bs.size(), nt, nt ? 1e6 * tt / nt : 0.0, nfalse, nfalse ? 1e6 * tf / nfalse : 0.0, 1e6 * tmax);
  }
  // the two reference cases
  CellSet E;
  for (int k = 8; k <= 9; k++) {
    double best = 1e9;
    bool v = false;
    for (int rep = 0; rep < 5; rep++) { double t0 = cpu_now(); v = leaf_holds(T8(), E, k); best = std::min(best, cpu_now() - t0); }
    int out[NL];
    printf("LEAF(T8,-,%d) = %d: %.1f us (%d live lines)\n", k, (int)v, 1e6 * best, live_lines(T8(), out));
  }
  {
    double t0 = cpu_now();
    EnumResult e = enum_check(T8(), E, 3);
    printf("enum_check(T8,-,3): status %d, %llu nodes, %.1f ms\n", e.status, (unsigned long long)e.nodes, 1e3 * (cpu_now() - t0));
  }
}

int main(int argc, char** argv) {
  double t0 = now();
  bool only_bench = argc > 1 && std::string(argv[1]) == "--bench";
  if (!only_bench) {
    test_regression();
    test_basic();
    test_random(600);
    test_targeted();
    test_threads();
    test_enum_naive();
    test_phi_naive();
  }
  bench();
  printf("== %ld checks, %ld failures, %.1f s ==\n", nchecks, nfail, now() - t0);
  printf(nfail ? "TEST_CORE FAILED\n" : "TEST_CORE OK\n");
  return nfail ? 1 : 0;
}

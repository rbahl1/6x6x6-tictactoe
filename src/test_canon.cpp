// test_canon.cpp -- tests of src/canon.hpp (SPEC 7.1, regression value 8).  Task 1.
// Usage: bin/test_canon [--quick]     exit code 0 iff every check passed.
#include "canon.hpp"
#include <chrono>
#include <random>
#include <set>
#include <unordered_set>
using namespace ttt;

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static double now() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Reference implementation: all 1152 images, plain comparisons, smallest g.
static CanonResult canon_ref(const CellSet& B, const CellSet& F) {
  const Geom& geo = G();
  CanonResult best; best.g = -1;
  for (int g = 0; g < NG; g++) {
    CellSet b = geo.image(g, B), f = geo.image(g, F);
    if (best.g < 0) { best.B = b; best.F = f; best.g = g; continue; }
    int r = cmp(b, best.B);
    if (r == 0) r = cmp(f, best.F);
    if (r < 0) { best.B = b; best.F = f; best.g = g; }
  }
  return best;
}

// Burnside: number of orbits of k-subsets of the busy cells = (1/|G|) sum_g #{k-subsets fixed by g};
// a k-subset is fixed by g iff it is a union of cycles of g on the 24 busy cells.
static void burnside(unsigned long long out[NB + 1]) {
  const Geom& geo = G();
  unsigned long long sum[NB + 1];
  for (int k = 0; k <= NB; k++) sum[k] = 0;
  for (int g = 0; g < NG; g++) {
    bool seen[NB] = {false};
    unsigned long long poly[NB + 1] = {1};
    for (int i = 0; i < NB; i++) if (!seen[i]) {
      int len = 0, j = i;
      while (!seen[j]) { seen[j] = true; len++; j = geo.busyIdx[geo.gmap[g][geo.busy[j]]]; }
      for (int k = NB; k >= len; k--) poly[k] += poly[k - len];
    }
    for (int k = 0; k <= NB; k++) sum[k] += poly[k];
  }
  for (int k = 0; k <= NB; k++) {
    CHECK(sum[k] % NG == 0, "Burnside sum for k=%d is not divisible by %d", k, NG);
    out[k] = sum[k] / NG;
  }
}

static CellSet random_set(std::mt19937_64& rng, int nbusy, int nother, const CellSet& avoid) {
  const Geom& geo = G();
  CellSet s;
  int tries = 0;
  while (nbusy > 0 && tries++ < 10000) {
    int c = geo.busy[rng() % NB];
    if (s.test(c) || avoid.test(c)) continue;
    s.set(c); nbusy--;
  }
  tries = 0;
  while (nother > 0 && tries++ < 10000) {
    int c = (int)(rng() % NC);
    if (s.test(c) || avoid.test(c)) continue;
    s.set(c); nother--;
  }
  return s;
}

int main(int argc, char** argv) {
  bool quick = argc > 1 && std::string(argv[1]) == "--quick";
  const Geom& geo = G();
  double t0 = now();

  // ---- 1. every map is a bijection on cells, sends lines onto lines, busy cells to busy cells,
  //         commutes with the antipode, and the 1152 maps are pairwise distinct.
  {
    long bad = 0;
    std::set<std::vector<uint8_t>> distinct;
    for (int g = 0; g < NG; g++) {
      CellSet all;
      for (int c = 0; c < NC; c++) all.set(geo.gmap[g][c]);
      if (all.count() != NC) bad++;
      std::set<int> imgs;
      for (int l = 0; l < NL; l++) {
        CellSet im = geo.image(g, geo.lineSet[l]);
        int found = -1;
        for (int m = 0; m < NL; m++) if (geo.lineSet[m] == im) found = m;
        if (found < 0 || found != geo.gline[g][l]) bad++;
        imgs.insert(found);
      }
      if ((int)imgs.size() != NL) bad++;
      if (!(geo.image(g, geo.busySet) == geo.busySet)) bad++;
      for (int c = 0; c < NC; c++) {
        if (geo.gmap[geo.ginv[g]][geo.gmap[g][c]] != c) bad++;
        if (geo.antip[geo.gmap[g][c]] != geo.gmap[g][geo.antip[c]]) bad++;
      }
      distinct.insert(std::vector<uint8_t>(geo.gmap[g], geo.gmap[g] + NC));
    }
    CHECK(bad == 0, "symmetry sanity: %ld violations", bad);
    CHECK((int)distinct.size() == NG, "only %zu distinct maps", distinct.size());
    // faithful on the busy cells?
    std::set<std::vector<int>> onbusy;
    for (int g = 0; g < NG; g++) {
      std::vector<int> v;
      for (int i = 0; i < NB; i++) v.push_back(geo.gmap[g][geo.busy[i]]);
      onbusy.insert(v);
    }
    printf("[1] 1152 maps: bijections, lines -> lines (onto), busy -> busy, commute with antipode: %s; "
           "distinct actions on the 24 busy cells: %zu\n", bad ? "FAILED" : "ok", onbusy.size());
    // orbits of G on cells
    int orb[3] = {0, 0, 0};
    for (int c = 0; c < NC; c++) {
      int m = canon_detail::T().orbMin[c];
      CHECK(m == 0 || m == 1 || m == 8, "unexpected orbit minimum %d", m);
      orb[m == 0 ? 0 : m == 1 ? 1 : 2]++;
    }
    CHECK(orb[0] == 24 && orb[1] == 144 && orb[2] == 48, "cell orbits %d %d %d", orb[0], orb[1], orb[2]);
    printf("    cell orbits under G: %d (busy) + %d + %d\n", orb[0], orb[1], orb[2]);
  }

  // ---- 2. busy_mask / busy_set / busy_image against the geometry
  {
    std::mt19937_64 rng(12345);
    long bad = 0;
    for (int it = 0; it < 200000; it++) {
      uint32_t m = (uint32_t)(rng() & 0xffffff);
      int g = (int)(rng() % NG);
      CellSet s = busy_set(m);
      if (busy_mask(s) != m) bad++;
      if (s.count() != __builtin_popcount(m) || !s.subset_of(geo.busySet)) bad++;
      if (!(busy_set(busy_image(g, m)) == geo.image(g, s))) bad++;
    }
    CellSet mixed = {0, 1, 2, 5, 43, 100, 215};
    CHECK(busy_set(busy_mask(mixed)) == (mixed & geo.busySet), "busy_mask of a mixed set");
    CHECK(bad == 0, "busy_image/busy_mask: %ld mismatches", bad);
    printf("[2] busy_mask/busy_set/busy_image vs geometry (200000 random): %s\n", bad ? "FAILED" : "ok");
  }

  // ---- 3. orbit counts of busy k-subsets: enumeration (two ways) and Burnside
  {
    unsigned long long bs[NB + 1];
    burnside(bs);
    const int expect[11] = {-1, -1, -1, -1, -1, -1, -1, 527, 1057, 1669, -1};
    printf("[3] orbits of busy k-subsets   k: reps / fixed points of busy_canon / Burnside\n");
    int kmaxFix = quick ? 8 : 10;
    for (int k = 0; k <= 12; k++) {
      double t1 = now();
      std::vector<uint32_t> reps = busy_orbit_reps(k);
      double t2 = now();
      long fix = -1;
      if (k <= kmaxFix) {
        // independent count: masks that are their own busy_canon (anchor method, different code path)
        fix = 0;
        std::vector<uint32_t> fixed;
        if (k == 0) { fix = 1; fixed.push_back(0); }
        else {
          uint32_t m = (uint32_t(1) << k) - 1;
          while (m < (uint32_t(1) << NB)) {
            if (busy_canon(m) == m) { fix++; fixed.push_back(m); }
            uint32_t c = m & (~m + 1), r = m + c;
            m = (((r ^ m) >> 2) / c) | r;
          }
        }
        std::sort(fixed.begin(), fixed.end(), busy_less);
        CHECK(fixed == reps, "k=%d: busy_orbit_reps differs from the fixed points of busy_canon", k);
      }
      for (size_t i = 0; i + 1 < reps.size(); i++)
        CHECK(cmp(busy_set(reps[i]), busy_set(reps[i + 1])) < 0, "k=%d: reps not sorted by cmp", k);
      for (uint32_t m : reps) CHECK(__builtin_popcount(m) == k, "k=%d: rep of wrong size", k);
      CHECK(reps.size() == bs[k], "k=%d: enumeration %zu != Burnside %llu", k, reps.size(), bs[k]);
      if (k <= 10 && expect[k] >= 0) CHECK((int)reps.size() == expect[k], "k=%d: %zu orbits, SPEC says %d", k, reps.size(), expect[k]);
      printf("    k=%2d: %6zu / %6ld / %6llu   (reps %.3fs)\n", k, reps.size(), fix, bs[k], t2 - t1);
    }
    // orbit sizes must add up to C(24,k) (k = 8): size = 1152 / |stabiliser|
    {
      std::vector<uint32_t> reps = busy_orbit_reps(8);
      unsigned long long total = 0;
      for (uint32_t m : reps) {
        int st = 0;
        for (int g = 0; g < NG; g++) if (busy_image(g, m) == m) st++;
        CHECK(NG % st == 0, "stabiliser order %d does not divide 1152", st);
        total += NG / st;
      }
      CHECK(total == 735471ULL, "k=8 orbit sizes add up to %llu, expected C(24,8)=735471", total);
      printf("    k=8 orbit sizes add up to %llu (C(24,8) = 735471)\n", total);
    }
  }

  // ---- 4. canon_pair against the reference, invariance, g maps input to output, idempotence
  {
    std::mt19937_64 rng(777);
    long nref = 0, bad = 0;
    int N = quick ? 1500 : 6000;
    for (int it = 0; it < N; it++) {
      int mode = it % 6;
      CellSet B, F;
      if (mode == 0) { B = random_set(rng, (int)(rng() % 13), 0, CellSet()); F = random_set(rng, (int)(rng() % 4), (int)(rng() % 12), B); }
      else if (mode == 1) { B = random_set(rng, (int)(rng() % 10), (int)(rng() % 4), CellSet()); F = random_set(rng, 0, (int)(rng() % 20), B); }
      else if (mode == 2) { B = random_set(rng, 0, (int)(rng() % 6), CellSet()); F = random_set(rng, (int)(rng() % 3), (int)(rng() % 6), B); }
      else if (mode == 3) { B = CellSet(); F = random_set(rng, (int)(rng() % 3), (int)(rng() % 5), B); }
      else if (mode == 4) { B = random_set(rng, (int)(rng() % 25), 0, CellSet()); F = CellSet(); }
      else {  // symmetric sets: a busy set closed under the antipode plus lines
        B = random_set(rng, (int)(rng() % 5), 0, CellSet());
        CellSet A; B.for_each([&](int c) { A.set(geo.antip[c]); }); B = B | A;
        int l = (int)(rng() % NL);
        F = geo.lineSet[l].minus(B);
      }
      CanonResult c = canon_pair(B, F), r = canon_ref(B, F);
      nref++;
      if (!(c.B == r.B) || !(c.F == r.F) || c.g != r.g) {
        bad++;
        if (bad < 5) printf("  mismatch: B=%s F=%s  canon g=%d ref g=%d\n", to_str(B).c_str(), to_str(F).c_str(), c.g, r.g);
      }
      if (!(geo.image(c.g, B) == c.B) || !(geo.image(c.g, F) == c.F)) bad++;      // g maps input to output
      if (c.B.count() != B.count() || c.F.count() != F.count()) bad++;
      CanonResult cc = canon_pair(c.B, c.F);                                        // idempotent
      if (!(cc.B == c.B) || !(cc.F == c.F)) bad++;
      if (!(geo.image(cc.g, c.B) == c.B) || !(geo.image(cc.g, c.F) == c.F)) bad++;
      for (int j = 0; j < 4; j++) {                                                 // invariance
        int h = (int)(rng() % NG);
        CanonResult ch = canon_pair(geo.image(h, B), geo.image(h, F));
        if (!(ch.B == c.B) || !(ch.F == c.F)) bad++;
        if (!(geo.image(ch.g, geo.image(h, B)) == c.B) || !(geo.image(ch.g, geo.image(h, F)) == c.F)) bad++;
      }
      // move translation: real move = g^-1(table move)
      int tm = (int)(rng() % NC);
      if (geo.gmap[c.g][geo.gmap[geo.ginv[c.g]][tm]] != tm) bad++;
    }
    CHECK(bad == 0, "canon_pair: %ld violations", bad);
    printf("[4] canon_pair vs brute-force reference (%ld random pairs, incl. g), invariance under random maps, "
           "g maps input to output, idempotence: %s\n", nref, bad ? "FAILED" : "ok");
    // large and highly symmetric sets (exercise the candidate-pair overflow path of min_image)
    {
      long badL = 0, nL = 0;
      std::vector<std::pair<CellSet, CellSet>> big;
      CellSet all, o0, o1, o8;
      for (int c = 0; c < NC; c++) {
        all.set(c);
        int m = canon_detail::T().orbMin[c];
        (m == 0 ? o0 : m == 1 ? o1 : o8).set(c);
      }
      big.push_back({all, CellSet()}); big.push_back({o1, o0}); big.push_back({o8, o1}); big.push_back({o1 | o8, CellSet()});
      big.push_back({o0, o8}); big.push_back({CellSet(), o1}); big.push_back({o1.minus(CellSet{1}), CellSet{1}});
      for (int it = 0; it < (quick ? 40 : 150); it++) {
        CellSet B = random_set(rng, (int)(rng() % 20), 20 + (int)(rng() % 120), CellSet());
        CellSet F = random_set(rng, 0, (int)(rng() % 30), B);
        if (it % 3 == 0) B = o1.minus(F);
        if (it % 3 == 1) B = (B | o1).minus(F);
        big.push_back({B, F});
      }
      for (auto& bf : big) {
        CanonResult c = canon_pair(bf.first, bf.second), r = canon_ref(bf.first, bf.second);
        nL++;
        if (!(c.B == r.B) || !(c.F == r.F) || c.g != r.g) badL++;
        int h = (int)(rng() % NG);
        CanonResult ch = canon_pair(geo.image(h, bf.first), geo.image(h, bf.second));
        if (!(ch.B == c.B) || !(ch.F == c.F)) badL++;
      }
      CHECK(badL == 0, "canon_pair on large/symmetric sets: %ld violations", badL);
      printf("    large and symmetric sets (%ld pairs, up to 216 cells) vs reference: %s\n", nL, badL ? "FAILED" : "ok");
    }
    // trivial cases
    CanonResult e = canon_pair(CellSet(), CellSet());
    CHECK(e.B.empty() && e.F.empty() && e.g == 0, "canon of the empty pair");
    for (int c = 0; c < NC; c++) {
      CanonResult s = canon_pair(CellSet{c}, CellSet());
      CHECK(s.B.count() == 1 && s.B.test(canon_detail::T().orbMin[c]), "canon of a single cell %d", c);
    }
  }

  // ---- 5. busy_canon agrees with canon_pair on busy sets
  {
    std::mt19937_64 rng(99);
    long bad = 0;
    int N = quick ? 50000 : 300000;
    for (int it = 0; it < N; it++) {
      uint32_t m = (uint32_t)(rng() & 0xffffff);
      if (it % 3 == 0) m &= (uint32_t)(rng() & 0xffffff);
      if (it % 7 == 0) m &= (uint32_t)(rng() & 0xffffff);
      uint32_t c = busy_canon(m);
      CanonResult r = canon_pair(busy_set(m), CellSet());
      if (!(busy_set(c) == r.B)) bad++;
      if (busy_image(r.g, m) != c) bad++;
      if (busy_canon(c) != c) bad++;
      if (busy_canon_tables(m) != c) bad++;
      if (busy_canon(busy_image((int)(rng() % NG), m)) != c) bad++;
    }
    // exhaustive brute force for k = 3 (2024 masks): min over all 1152 images by cmp
    for (uint32_t m = 7; m < (1u << NB);) {
      CellSet best = busy_set(m);
      for (int g = 0; g < NG; g++) { CellSet im = geo.image(g, busy_set(m)); if (cmp(im, best) < 0) best = im; }
      if (!(busy_set(busy_canon(m)) == best)) bad++;
      uint32_t c = m & (~m + 1), r = m + c;
      m = (((r ^ m) >> 2) / c) | r;
    }
    CHECK(busy_canon(0) == 0, "busy_canon(0)");
    CHECK(bad == 0, "busy_canon: %ld violations", bad);
    printf("[5] busy_canon == canon_pair on busy sets (%d random masks; all 3-subsets by brute force): %s\n", N, bad ? "FAILED" : "ok");
  }

  // ---- 6. state_hash: no collisions on distinct states, t matters, B/F are not interchangeable
  {
    std::mt19937_64 rng(4242);
    std::unordered_set<uint64_t> hs;
    std::set<std::array<uint64_t, 9>> states;
    int N = quick ? 100000 : 400000;
    for (int it = 0; it < N; it++) {
      CellSet B = random_set(rng, (int)(rng() % 10), (int)(rng() % 3), CellSet());
      CellSet F = random_set(rng, 0, (int)(rng() % 8), B);
      int t = (int)(rng() % 16);
      std::array<uint64_t, 9> key = {B.w[0], B.w[1], B.w[2], B.w[3], F.w[0], F.w[1], F.w[2], F.w[3], (uint64_t)t};
      if (states.insert(key).second) hs.insert(state_hash(B, F, t));
    }
    CHECK(hs.size() == states.size(), "state_hash: %zu states, %zu hashes", states.size(), hs.size());
    CellSet a = {0, 5}, b = {7};
    CHECK(state_hash(a, b, 3) != state_hash(b, a, 3), "state_hash symmetric in B,F");
    CHECK(state_hash(a, b, 3) != state_hash(a, b, 4), "state_hash ignores t");
    // low bits (used as table index): bucket occupancy with 2^16 buckets
    std::vector<int> bucket(1 << 16, 0);
    int mx = 0;
    for (uint64_t h : hs) mx = std::max(mx, ++bucket[h & 0xffff]);
    printf("[6] state_hash: %zu distinct states -> %zu distinct hashes; max load of 65536 low-bit buckets: %d (mean %.1f)\n",
           states.size(), hs.size(), mx, hs.size() / 65536.0);
    CHECK(mx < 6 * (int)(hs.size() / 65536 + 1) + 20, "state_hash low bits badly distributed");
  }

  // ---- 7. timing
  {
    std::mt19937_64 rng(2026);
    struct Case { const char* name; int bb, bo, fb, fo; };
    const Case cases[] = {
      {"|B|=8 busy, F empty            ", 8, 0, 0, 0},
      {"|B|=10 busy, |F|=10            ", 10, 0, 1, 9},
      {"|B|=9 busy + 3 other, |F|=18   ", 9, 3, 2, 16},
      {"|B|=12 busy, |F|=18            ", 12, 0, 2, 16},
      {"|B|=2 busy, |F|=6              ", 2, 0, 1, 5},
      {"|B|=4 other (no busy), |F|=10  ", 0, 4, 0, 10},
      {"B empty, |F|=5                 ", 0, 0, 1, 4},
    };
    printf("[7] canon_pair timing (single thread, -O2):\n");
    double worst = 0;
    for (const Case& cs : cases) {
      const int M = 2000;
      std::vector<CellSet> Bs(M), Fs(M);
      for (int i = 0; i < M; i++) { Bs[i] = random_set(rng, cs.bb, cs.bo, CellSet()); Fs[i] = random_set(rng, cs.fb, cs.fo, Bs[i]); }
      int reps = quick ? 20 : 100;
      uint64_t sink = 0;
      double t1 = now();
      for (int r = 0; r < reps; r++) for (int i = 0; i < M; i++) { CanonResult c = canon_pair(Bs[i], Fs[i]); sink += c.g + c.B.w[0]; }
      double us = (now() - t1) * 1e6 / (double(reps) * M);
      worst = std::max(worst, us);
      printf("    %s %.3f us/call   (checksum %llu)\n", cs.name, us, (unsigned long long)sink);
    }
    {  // most symmetric large case: all 24 busy cells
      CellSet B = geo.busySet, F = geo.lineSet[0].minus(B);
      double t1 = now(); uint64_t sink = 0; int reps = 2000;
      for (int r = 0; r < reps; r++) { CanonResult c = canon_pair(B, F); sink += c.g; }
      printf("    B = all 24 busy cells (1152-fold symmetric) %.3f us/call\n", (now() - t1) * 1e6 / reps);
    }
    {  // busy_canon
      const int M = 4096; std::vector<uint32_t> ms(M);
      for (int i = 0; i < M; i++) { uint32_t m = 0; while (__builtin_popcount(m) < 9) m |= 1u << (rng() % NB); ms[i] = m; }
      double t1 = now(); uint64_t sink = 0; int reps = quick ? 20 : 100;
      for (int r = 0; r < reps; r++) for (int i = 0; i < M; i++) sink += busy_canon(ms[i]);
      printf("    busy_canon, 9 cells: %.3f us/call\n", (now() - t1) * 1e6 / (double(reps) * M));
      t1 = now();
      for (int r = 0; r < reps; r++) for (int i = 0; i < M; i++) sink += busy_canon_tables(ms[i]);
      printf("    busy_canon_tables, 9 cells: %.3f us/call   (checksum %llu)\n", (now() - t1) * 1e6 / (double(reps) * M), (unsigned long long)sink);
    }
    // timing depends on the load of the shared machine: report, do not fail
    printf("    slowest random case: %.3f us/call -> %s the SPEC target of 20 us\n", worst, worst <= 20.0 ? "within" : "WARNING: above");
  }

  printf("%s  (%d failed checks, %.1f s)\n", g_fail ? "TEST_CANON FAILED" : "TEST_CANON PASSED", g_fail, now() - t0);
  return g_fail ? 1 : 0;
}

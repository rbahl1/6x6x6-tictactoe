// canon.hpp -- canonical forms under the 1152 symmetries (SPEC 6 and 7.1).  Task 1.
// Header-only, thread-safe (all tables are built once in a function-static object; every function
// below only reads them and works on its own stack).
#pragma once
#include "geom.hpp"

namespace ttt {

struct CanonResult { CellSet B, F; int g; };            // B = image(g,Bin), F = image(g,Fin)

namespace canon_detail {

struct Tables {
  // G has three orbits on cells (24 busy cells, 144 cells with exactly two equal "depths",
  // 48 cells with three different depths).  orbMin[c] = smallest cell id in the orbit of c (0, 1 or 8).
  uint8_t orbMin[NC];
  // toMin[toMinOff[c] .. toMinOff[c+1]) = all g with gmap[g][c] == orbMin[c], ascending (48, 8 or 24 maps).
  int toMinOff[NC + 1];
  std::vector<uint16_t> toMin;
  // second[a][c] = min over { g : g(a) = orbMin[a] } of g(c)   (a != c).
  uint8_t second[NC][NC];
  // pairMaps[pairOff[a*NC+c] .. pairOff[a*NC+c+1]) = all g with g(a) = orbMin[a] and g(c) = second[a][c].
  std::vector<int> pairOff;
  std::vector<uint16_t> pairMaps;
  // Third level.  cls[ti][m] numbers the pairs (t,m) = (orbMin[a], second[a][c]) that occur (ti = 0,1,2 for
  // t = 0,1,8); orb3[cls*NC + x] = min over { h : h(t) = t, h(m) = m } of h(x).
  int16_t cls[3][NC];
  std::vector<uint8_t> orb3;
  // bimg[(g*3 + j)*256 + byte] = image mask of the busy cells whose indices are 8j + (bits of byte).
  std::vector<uint32_t> bimg;
  static int tindex(int t) { return t == 0 ? 0 : t == 1 ? 1 : 2; }

  Tables() {
    const Geom& geo = G();
    for (int c = 0; c < NC; c++) {
      int m = NC;
      for (int g = 0; g < NG; g++) m = std::min<int>(m, geo.gmap[g][c]);
      orbMin[c] = (uint8_t)m;
    }
    memset(second, 0xff, sizeof second);
    for (int c = 0; c < NC; c++) {
      toMinOff[c] = (int)toMin.size();
      for (int g = 0; g < NG; g++) if (geo.gmap[g][c] == orbMin[c]) {
        toMin.push_back((uint16_t)g);
        const uint8_t* m = geo.gmap[g];
        for (int d = 0; d < NC; d++) if (m[d] < second[c][d]) second[c][d] = m[d];
      }
    }
    toMinOff[NC] = (int)toMin.size();
    for (int c = 0; c < NC; c++)
      if (orbMin[c] != 0 && orbMin[c] != 1 && orbMin[c] != 8) { fprintf(stderr, "canon: unexpected cell orbits\n"); abort(); }
    pairOff.assign((size_t)NC * NC + 1, 0);
    for (int ti = 0; ti < 3; ti++) for (int m = 0; m < NC; m++) cls[ti][m] = -1;
    int ncls = 0;
    for (int a = 0; a < NC; a++) for (int c = 0; c < NC; c++) {
      pairOff[(size_t)a * NC + c] = (int)pairMaps.size();
      if (a == c) continue;
      int m = second[a][c];
      for (int q = toMinOff[a]; q < toMinOff[a + 1]; q++)
        if (geo.gmap[toMin[q]][c] == m) pairMaps.push_back(toMin[q]);
      int16_t& id = cls[tindex(orbMin[a])][m];
      if (id < 0) id = (int16_t)ncls++;
    }
    pairOff[(size_t)NC * NC] = (int)pairMaps.size();
    orb3.assign((size_t)ncls * NC, 0xff);
    const int tval[3] = {0, 1, 8};
    for (int ti = 0; ti < 3; ti++) for (int m = 0; m < NC; m++) if (cls[ti][m] >= 0) {
      uint8_t* row = &orb3[(size_t)cls[ti][m] * NC];
      for (int g = 0; g < NG; g++) if (geo.gmap[g][tval[ti]] == tval[ti] && geo.gmap[g][m] == m)
        for (int x = 0; x < NC; x++) if (geo.gmap[g][x] < row[x]) row[x] = geo.gmap[g][x];
    }
    bimg.assign((size_t)NG * 3 * 256, 0);
    for (int g = 0; g < NG; g++) {
      uint32_t single[NB];
      for (int i = 0; i < NB; i++) {
        int im = geo.busyIdx[geo.gmap[g][geo.busy[i]]];
        if (im < 0) { fprintf(stderr, "canon: a symmetry moves a busy cell to a non-busy cell\n"); abort(); }
        single[i] = uint32_t(1) << im;
      }
      for (int j = 0; j < 3; j++) {
        uint32_t* t = &bimg[((size_t)g * 3 + j) * 256];
        for (int b = 1; b < 256; b++) {
          int low = __builtin_ctz(b);
          t[b] = t[b & (b - 1)] | single[8 * j + low];
        }
      }
    }
  }
};

inline const Tables& T() { static Tables t; return t; }

// Image of the cells list under g.
inline CellSet image_list(const uint8_t* m, const uint8_t* cells, int n) {
  CellSet r;
  for (int i = 0; i < n; i++) { int x = m[cells[i]]; r.w[x >> 6] |= uint64_t(1) << (x & 63); }
  return r;
}

// Smallest image (order cmp) of a non-empty set S over all 1152 maps.
// Writes the maps attaining it to ties[] (not sorted), returns their number.
//
// cmp on sets of equal size is the lexicographic order of the ascending cell lists, so the minimum
//  (1) has the smallest possible first cell t = min orbMin over S; the map sends some "anchor" a in S
//      (orbMin[a] = t) to t;
//  (2) has, among those, the smallest possible second cell m = min over anchors a and c != a of
//      second[a][c]; the maps with g(a) = t, g(c) = m are the coset Stab(t,m) o g0 (g0 = any of them);
//  (3) has, among those, the smallest possible third cell m3 = min over such pairs (a,c) and d != a,c
//      of orb3[(t,m)][g0(d)].
// Only the maps with g(a) = t, g(c) = m, g(d) = m3 for such a triple (a,c,d) are candidates (each map
// arises from exactly one triple); their images are compared in full.
inline int min_image(const CellSet& S, CellSet& best, uint16_t* ties) {
  const Geom& geo = G();
  const Tables& tb = T();
  uint8_t cells[NC];
  int n = 0;
  S.for_each([&](int c) { cells[n++] = (uint8_t)c; });
  int t = NC;
  for (int i = 0; i < n; i++) t = std::min<int>(t, tb.orbMin[cells[i]]);
  uint16_t cand[NG];
  int nc = 0;
  if (n == 1) {
    int a = cells[0];
    for (int j = tb.toMinOff[a]; j < tb.toMinOff[a + 1]; j++) cand[nc++] = tb.toMin[j];
  } else {
    // level 2
    uint8_t anchors[NC], amin[NC];
    int na = 0, m = NC;
    for (int i = 0; i < n; i++) if (tb.orbMin[cells[i]] == t) {
      int a = cells[i];
      const uint8_t* row = tb.second[a];
      int ma = NC;
      for (int j = 0; j < n; j++) if (j != i) ma = std::min<int>(ma, row[cells[j]]);
      anchors[na] = (uint8_t)a; amin[na] = (uint8_t)ma; na++;
      m = std::min(m, ma);
    }
    if (n == 2) {
      for (int i = 0; i < na; i++) if (amin[i] == m) {
        int a = anchors[i], c = (cells[0] == a) ? cells[1] : cells[0];
        const int* po = &tb.pairOff[(size_t)a * NC + c];
        for (int q = po[0]; q < po[1]; q++) cand[nc++] = tb.pairMaps[q];
      }
    } else {
      // level 3
      const uint8_t* o3 = &tb.orb3[(size_t)tb.cls[Tables::tindex(t)][m] * NC];
      int m3 = NC, np = 0;
      int pa[NC], pc[NC];        // candidate pairs; the number of pairs (a,c) with second = m is < NC
      uint8_t p3[NC];
      for (int i = 0; i < na; i++) if (amin[i] == m) {
        int a = anchors[i];
        const uint8_t* row = tb.second[a];
        for (int j = 0; j < n; j++) {
          int c = cells[j];
          if (c == a || row[c] != m) continue;
          const uint8_t* g0 = geo.gmap[tb.pairMaps[tb.pairOff[(size_t)a * NC + c]]];
          int v = NC;
          for (int q = 0; q < n; q++) {
            int d = cells[q];
            if (d != a && d != c) v = std::min<int>(v, o3[g0[d]]);
          }
          if (v > m3) continue;
          if (v < m3) { m3 = v; np = 0; }
          if (np < NC) { pa[np] = a; pc[np] = c; p3[np] = (uint8_t)v; np++; }
          else {  // cannot happen for more than NC pairs in practice; fall back to all maps of the pair
            const int* po = &tb.pairOff[(size_t)a * NC + c];
            for (int q = po[0]; q < po[1]; q++) if (nc < NG) cand[nc++] = tb.pairMaps[q];
          }
        }
      }
      for (int i = 0; i < np; i++) if (p3[i] == m3) {
        int a = pa[i], c = pc[i];
        const int* po = &tb.pairOff[(size_t)a * NC + c];
        const uint8_t* g0 = geo.gmap[tb.pairMaps[po[0]]];
        for (int q = 0; q < n; q++) {
          int d = cells[q];
          if (d == a || d == c || o3[g0[d]] != m3) continue;
          for (int r = po[0]; r < po[1]; r++) {
            int g = tb.pairMaps[r];
            if (geo.gmap[g][d] == m3 && nc < NG) cand[nc++] = (uint16_t)g;
          }
        }
      }
    }
  }
  int nt = 0;
  for (int i = 0; i < nc; i++) {
    CellSet im = image_list(geo.gmap[cand[i]], cells, n);
    int r = nt ? cmp(im, best) : -1;
    if (r < 0) { best = im; nt = 0; ties[nt++] = cand[i]; }
    else if (r == 0) ties[nt++] = cand[i];
  }
  return nt;
}

inline uint64_t mix64(uint64_t x) {
  x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27; x *= 0x94d049bb133111ebULL;
  x ^= x >> 31; return x;
}

}  // namespace canon_detail

// Canonical form of the pair (B,F) (SPEC 6): the smallest image pair, B-images compared first (cmp),
// F-images second; g = the smallest symmetry index attaining it.
inline CanonResult canon_pair(const CellSet& B, const CellSet& F) {
  using namespace canon_detail;
  const Geom& geo = G();
  CanonResult res;
  uint16_t ties[NG];
  if (B.empty()) {
    if (F.empty()) { res.g = 0; return res; }
    int nt = min_image(F, res.F, ties);
    int g = ties[0];
    for (int i = 1; i < nt; i++) if (ties[i] < g) g = ties[i];
    res.g = g;
    return res;
  }
  int nt = min_image(B, res.B, ties);
  if (F.empty()) {
    int g = ties[0];
    for (int i = 1; i < nt; i++) if (ties[i] < g) g = ties[i];
    res.g = g;
    return res;
  }
  uint8_t cells[NC];
  int n = 0;
  F.for_each([&](int c) { cells[n++] = (uint8_t)c; });
  int bestg = -1;
  for (int i = 0; i < nt; i++) {
    int g = ties[i];
    CellSet im = image_list(geo.gmap[g], cells, n);
    int r = bestg < 0 ? -1 : cmp(im, res.F);
    if (r < 0 || (r == 0 && g < bestg)) { res.F = im; bestg = g; }
  }
  res.g = bestg;
  return res;
}

// bit i <-> G().busy[i]   (s ∩ busy)
inline uint32_t busy_mask(const CellSet& s) {
  const Geom& geo = G();
  uint32_t m = 0;
  (s & geo.busySet).for_each([&](int c) { m |= uint32_t(1) << geo.busyIdx[c]; });
  return m;
}

inline CellSet busy_set(uint32_t mask) {
  const Geom& geo = G();
  CellSet s;
  while (mask) { int i = __builtin_ctz(mask); s.set(geo.busy[i]); mask &= mask - 1; }
  return s;
}

inline uint32_t busy_image(int g, uint32_t mask) {
  const uint32_t* t = &canon_detail::T().bimg[(size_t)g * 768];
  return t[mask & 255] | t[256 + ((mask >> 8) & 255)] | t[512 + ((mask >> 16) & 255)];
}

// true iff busy_set(a) is smaller than busy_set(b) in the order cmp (busy[] is ascending, so the set
// that contains the lowest differing bit is the smaller one).
inline bool busy_less(uint32_t a, uint32_t b) {
  uint32_t d = a ^ b;
  return d && (a & (d & (~d + 1)));
}

// The image of mask whose cell set is smallest under cmp (same result as canon_pair(busy_set(mask), {}).B).
inline uint32_t busy_canon(uint32_t mask) {
  mask &= 0xffffffu;
  if (!mask) return 0;
  CellSet best;
  uint16_t ties[NG];
  canon_detail::min_image(busy_set(mask), best, ties);
  return busy_mask(best);
}

// Reference version of busy_canon using only the byte tables (slower; kept for the tests).
// The 24 busy cells are one orbit and busy[0] = cell 0, so the minimum contains bit 0: only the
// 48 maps sending a cell of the set to cell 0 have to be tried, for each cell of the set.
inline uint32_t busy_canon_tables(uint32_t mask) {
  const canon_detail::Tables& tb = canon_detail::T();
  const Geom& geo = G();
  mask &= 0xffffffu;
  if (!mask) return 0;
  uint32_t best = 0;
  bool have = false;
  for (uint32_t r = mask; r; r &= r - 1) {
    int a = geo.busy[__builtin_ctz(r)];
    for (int q = tb.toMinOff[a]; q < tb.toMinOff[a + 1]; q++) {
      uint32_t im = busy_image(tb.toMin[q], mask);
      if (!have || busy_less(im, best)) { best = im; have = true; }
    }
  }
  return best;
}

// Canonical masks of all k-subsets of the busy cells, sorted by cmp of their sets.
// (Visited bitmap over the 2^24 masks; every orbit is expanded once with all 1152 maps.)
inline std::vector<uint32_t> busy_orbit_reps(int k) {
  std::vector<uint32_t> reps;
  if (k < 0 || k > NB) return reps;
  if (k == 0) { reps.push_back(0); return reps; }
  std::vector<uint64_t> seen((size_t(1) << NB) / 64, 0);
  const uint32_t limit = uint32_t(1) << NB;
  uint32_t m = (uint32_t(1) << k) - 1;
  while (m < limit) {
    if (!((seen[m >> 6] >> (m & 63)) & 1)) {
      uint32_t best = m;
      for (int g = 0; g < NG; g++) {
        uint32_t im = busy_image(g, m);
        seen[im >> 6] |= uint64_t(1) << (im & 63);
        if (busy_less(im, best)) best = im;
      }
      reps.push_back(best);
    }
    uint32_t c = m & (~m + 1), r = m + c;        // Gosper: next mask with the same popcount
    m = (((r ^ m) >> 2) / c) | r;
  }
  std::sort(reps.begin(), reps.end(), busy_less);
  return reps;
}

// 64-bit hash of a (canonical) state.
inline uint64_t state_hash(const CellSet& B, const CellSet& F, int t) {
  using canon_detail::mix64;
  uint64_t h = mix64(0x9e3779b97f4a7c15ULL + (uint64_t)(uint32_t)t);
  for (int i = 0; i < 4; i++) h = mix64(h ^ (B.w[i] + 0x9e3779b97f4a7c15ULL * (uint64_t)(2 * i + 1)));
  for (int i = 0; i < 4; i++) h = mix64(h ^ (F.w[i] + 0x9e3779b97f4a7c15ULL * (uint64_t)(2 * i + 2)));
  return h;
}

}  // namespace ttt

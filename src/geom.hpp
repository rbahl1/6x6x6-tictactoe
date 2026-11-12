// geom.hpp -- FROZEN INTERFACE (written by the coordinator; see SPEC.md section 2).
// Do not edit. Geometry of the 6x6x6 board: cells, lines, busy cells, the 1152 symmetries.
#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace ttt {

constexpr int NC = 216;   // cells
constexpr int NL = 148;   // lines
constexpr int NB = 24;    // busy cells
constexpr int NG = 1152;  // symmetries

inline int cid(int x, int y, int z) { return 36 * x + 6 * y + z; }
inline int cx(int c) { return c / 36; }
inline int cy(int c) { return (c / 6) % 6; }
inline int cz(int c) { return c % 6; }

// A set of cells as a 216-bit bitset.
struct CellSet {
  uint64_t w[4];
  CellSet() : w{0, 0, 0, 0} {}
  CellSet(std::initializer_list<int> cs) : w{0, 0, 0, 0} { for (int c : cs) set(c); }
  explicit CellSet(const std::vector<int>& cs) : w{0, 0, 0, 0} { for (int c : cs) set(c); }
  bool test(int c) const { return (w[c >> 6] >> (c & 63)) & 1; }
  void set(int c) { w[c >> 6] |= uint64_t(1) << (c & 63); }
  void reset(int c) { w[c >> 6] &= ~(uint64_t(1) << (c & 63)); }
  int count() const {
    return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) +
           __builtin_popcountll(w[2]) + __builtin_popcountll(w[3]);
  }
  bool empty() const { return !(w[0] | w[1] | w[2] | w[3]); }
  bool intersects(const CellSet& o) const {
    return (w[0] & o.w[0]) | (w[1] & o.w[1]) | (w[2] & o.w[2]) | (w[3] & o.w[3]);
  }
  bool subset_of(const CellSet& o) const {
    return !((w[0] & ~o.w[0]) | (w[1] & ~o.w[1]) | (w[2] & ~o.w[2]) | (w[3] & ~o.w[3]));
  }
  CellSet operator|(const CellSet& o) const { CellSet r; for (int i = 0; i < 4; i++) r.w[i] = w[i] | o.w[i]; return r; }
  CellSet operator&(const CellSet& o) const { CellSet r; for (int i = 0; i < 4; i++) r.w[i] = w[i] & o.w[i]; return r; }
  CellSet minus(const CellSet& o) const { CellSet r; for (int i = 0; i < 4; i++) r.w[i] = w[i] & ~o.w[i]; return r; }
  bool operator==(const CellSet& o) const { return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3]; }
  bool operator!=(const CellSet& o) const { return !(*this == o); }
  template <class Fn> void for_each(Fn f) const {  // ascending cell id
    for (int i = 0; i < 4; i++) {
      uint64_t x = w[i];
      while (x) { int b = __builtin_ctzll(x); f(i * 64 + b); x &= x - 1; }
    }
  }
  std::vector<int> cells() const { std::vector<int> v; for_each([&](int c) { v.push_back(c); }); return v; }
};

// Total order on cell sets (SPEC 2.4): let p be the smallest cell id whose membership differs;
// the set that CONTAINS p is the smaller one. For two sets of equal size this is exactly the
// lexicographic order of their ascending cell-id lists. Returns -1, 0, +1.
inline int cmp(const CellSet& a, const CellSet& b) {
  for (int i = 0; i < 4; i++) {
    uint64_t d = a.w[i] ^ b.w[i];
    if (d) { uint64_t low = d & (~d + 1); return (a.w[i] & low) ? -1 : 1; }
  }
  return 0;
}

struct Geom {
  int line[NL][6];          // cells of each line, ascending cell id; lines sorted lexicographically
  CellSet lineSet[NL];      // same as bitsets
  int nlc[NC];              // number of lines through a cell (3, 4 or 7)
  int lc[NC][7];            // the lines through a cell, ascending line id
  int16_t lineOf[NC][NC];   // id of the line through two distinct cells, or -1
  int busy[NB];             // the 24 busy cells, ascending
  int busyIdx[NC];          // index in busy[] or -1
  CellSet busySet;
  int antip[NC];            // antipode (5-x,5-y,5-z)
  uint8_t gmap[NG][NC];     // gmap[g][c] = image of cell c under symmetry g
  int16_t gline[NG][NL];    // image line id
  int ginv[NG];             // index of the inverse symmetry
  // symmetry g = (perm p, reversal mask r, relabelling s):
  //   (c0,c1,c2) -> (c[p0],c[p1],c[p2]) -> reverse coordinate i iff bit i of r -> apply s to every coordinate.
  Geom() {
    // ---- lines
    std::vector<std::array<int, 6>> ls;
    for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) for (int f3 = 0; f3 < 8; f3++) {
      if (f1 < 6 && f2 < 6 && f3 < 6) continue;  // codes 0..5 const, 6 = t, 7 = 5-t
      std::array<int, 6> l;
      for (int t = 0; t < 6; t++) {
        auto ev = [&](int f) { return f < 6 ? f : (f == 6 ? t : 5 - t); };
        l[t] = cid(ev(f1), ev(f2), ev(f3));
      }
      std::sort(l.begin(), l.end());
      ls.push_back(l);
    }
    std::sort(ls.begin(), ls.end());
    ls.erase(std::unique(ls.begin(), ls.end()), ls.end());
    if ((int)ls.size() != NL) { fprintf(stderr, "geom: %zu lines\n", ls.size()); abort(); }
    memset(nlc, 0, sizeof nlc);
    for (int a = 0; a < NC; a++) for (int b = 0; b < NC; b++) lineOf[a][b] = -1;
    for (int i = 0; i < NL; i++) {
      for (int j = 0; j < 6; j++) { line[i][j] = ls[i][j]; lineSet[i].set(ls[i][j]); }
      for (int j = 0; j < 6; j++) {
        int c = line[i][j];
        lc[c][nlc[c]++] = i;
        for (int k = 0; k < 6; k++) if (k != j) {
          if (lineOf[c][line[i][k]] != -1) { fprintf(stderr, "geom: two cells on two lines\n"); abort(); }
          lineOf[c][line[i][k]] = (int16_t)i;
        }
      }
    }
    // ---- busy cells and antipodes
    int nb = 0;
    for (int c = 0; c < NC; c++) {
      int x = cx(c), y = cy(c), z = cz(c);
      antip[c] = cid(5 - x, 5 - y, 5 - z);
      int mx = std::min(x, 5 - x), my = std::min(y, 5 - y), mz = std::min(z, 5 - z);
      busyIdx[c] = -1;
      if (mx == my && my == mz) { busyIdx[c] = nb; busy[nb++] = c; busySet.set(c); }
    }
    if (nb != NB) abort();
    for (int c = 0; c < NC; c++) {
      int want = busyIdx[c] >= 0 ? 7 : -1;
      if (want == 7 ? nlc[c] != 7 : (nlc[c] != 3 && nlc[c] != 4)) { fprintf(stderr, "geom: degree\n"); abort(); }
    }
    // ---- symmetries
    std::vector<std::array<int, 3>> perms;
    { std::array<int, 3> p = {0, 1, 2}; do perms.push_back(p); while (std::next_permutation(p.begin(), p.end())); }
    std::vector<std::array<int, 6>> sig;
    { std::array<int, 6> s = {0, 1, 2, 3, 4, 5};
      do { bool ok = true; for (int i = 0; i < 6; i++) if (s[5 - i] != 5 - s[i]) ok = false; if (ok) sig.push_back(s); }
      while (std::next_permutation(s.begin(), s.end())); }
    if (sig.size() != 48) abort();
    std::map<std::vector<uint8_t>, int> seen;
    int ng = 0;
    for (auto& p : perms) for (int r = 0; r < 8; r++) for (auto& s : sig) {
      std::vector<uint8_t> m(NC);
      for (int c = 0; c < NC; c++) {
        int co[3] = {cx(c), cy(c), cz(c)}, d[3];
        for (int i = 0; i < 3; i++) { d[i] = co[p[i]]; if (r >> i & 1) d[i] = 5 - d[i]; d[i] = s[d[i]]; }
        m[c] = (uint8_t)cid(d[0], d[1], d[2]);
      }
      if (seen.count(m)) continue;
      if (ng >= NG) abort();
      seen[m] = ng;
      memcpy(gmap[ng], m.data(), NC);
      ng++;
    }
    if (ng != NG) { fprintf(stderr, "geom: %d symmetries\n", ng); abort(); }
    for (int g = 0; g < NG; g++) {
      for (int l = 0; l < NL; l++) {
        int a = gmap[g][line[l][0]], b = gmap[g][line[l][1]];
        int im = lineOf[a][b];
        if (im < 0) { fprintf(stderr, "geom: symmetry breaks a line\n"); abort(); }
        for (int j = 0; j < 6; j++) if (!lineSet[im].test(gmap[g][line[l][j]])) { fprintf(stderr, "geom: symmetry breaks a line\n"); abort(); }
        gline[g][l] = (int16_t)im;
      }
      std::vector<uint8_t> inv(NC);
      for (int c = 0; c < NC; c++) inv[gmap[g][c]] = (uint8_t)c;
      auto it = seen.find(inv);
      if (it == seen.end()) { fprintf(stderr, "geom: not closed under inverse\n"); abort(); }
      ginv[g] = it->second;
    }
  }
  CellSet image(int g, const CellSet& s) const { CellSet r; s.for_each([&](int c) { r.set(gmap[g][c]); }); return r; }
};

inline const Geom& G() { static Geom g; return g; }

// Parse / print a cell set as "c1,c2,...,cn" (ascending ids) or "-" for the empty set.
inline std::string to_str(const CellSet& s) {
  std::string r; s.for_each([&](int c) { if (!r.empty()) r += ','; r += std::to_string(c); });
  return r.empty() ? std::string("-") : r;
}
inline CellSet parse_set(const std::string& t) {
  CellSet s; if (t == "-" || t.empty()) return s;
  size_t i = 0; while (i < t.size()) { size_t j = t.find(',', i); if (j == std::string::npos) j = t.size(); s.set(atoi(t.substr(i, j - i).c_str())); i = j + 1; }
  return s;
}
}  // namespace ttt

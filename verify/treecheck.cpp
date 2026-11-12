// verify/treecheck.cpp -- independent verifier of the proof tree for "6x6x6 tic-tac-toe is a draw"
// (SPEC.md section 7.5, task 6c).
//
// INDEPENDENCE.  This file is self-contained: it includes nothing from src/ or sim/ and was written
// from SPEC.md alone (sections 1, 3, 4, 6).  It has its own geometry, its own symmetry group, its own
// canonical form, its own bipartite matching / max-flow and its own LOST and child generation.
//
// WHAT "ok":true MEANS.  Starting from the root P-state the program visits every state reachable through
// the rules of SPEC section 4 exactly once (states are identified up to the 1152 symmetries) and checks,
// for every visited state, the local condition of the definition of WON:
//   * P-state listed in the leaf file : LEAF(B,F,t) is recomputed from scratch (exact integer arithmetic).
//   * P-state not listed              : t < R_max, LOST(B,F,t) is false, and all children (a), (c), (d)
//                                       that are not VOID are visited.
//   * Q-state                         : the strategy table holds a move m for it, m is a cell outside B and F,
//                                       child (i) is visited, and child (ii) is visited unless |F| = t or it is VOID.
// The successor relation is well founded (the rank (t, P after Q, |F|) grows strictly along every edge and
// t <= R_max), so "every visited state satisfies its local condition and the work list ran empty" implies by
// induction from the bottom that every visited state, in particular the root, is WON.
//
// WHY WORKING IN THE CANONICAL FRAME IS SOUND.  Every definition of sections 3 and 4 (live lines,
// admissibility, pairings, surplus, LEAF, LOST, the children, WON) is phrased through the cell/line
// incidence only, and every map g of the group sends cells bijectively to cells and lines to lines (this is
// asserted for all 1152 maps at every start of the program).  Hence a state s is WON iff g(s) is WON, and
// children(g(s)) = g(children(s)).  The verifier therefore keeps only one representative per symmetry class:
// each state is replaced by an image under a group element before it is stored, and everything (table move
// included) is evaluated in the frame of that representative; no move is ever translated back.  Table and
// leaf-file entries are re-canonicalised with THIS program's canonical form when they are loaded (the move
// is mapped with the same g), so the result does not depend on the canonical form of the search program.
// Soundness never depends on the representative being the minimal one: the stored state is always computed
// as (image(g,B), image(g,F)) for one group element g.  Minimality only matters for finding table entries
// and for visiting each class once; a wrong choice could only produce spurious errors or extra work.
//
// HASHING.  The visited set is keyed by a 128-bit hash of (kind, t, B, F) of the representative (option
// --exact stores the full 576-bit key instead and removes the only probabilistic step).  A collision could
// make the verifier skip a state; with n states the probability is about n^2 / 2^129 (< 1e-22 for n = 1e8).
// Table and leaf-file lookups also go through hashes (120 bits), but a collision there is harmless for
// soundness: a wrongly found move is still checked for legality and its children are verified, and a state
// wrongly taken for a listed leaf still has to pass the LEAF computation.
//
// LAYERS OF SELF-CHECKING.
//   always       geometry and group assertions at start; the base pairing of every LEAF computation is
//                validated literally; every "LEAF = false" certificate is re-evaluated from the definition.
//   default      every flow that supports a "LEAF = true" verdict is validated literally (validateFlow);
//                --fast switches this off.
//   --exact      the visited set stores full keys (no hashing argument needed).
//   --crosscheck every state / expansion is re-derived with the naive reference code of section 10
//                (brute-force canonical form, literal admissibility, literal LOST, literal children) and
//                every N-th leaf with an unpruned triple enumeration on an independent matching code.
//   --selftest   compares all fast routines with the reference code on random and hand-built inputs,
//                LEAF with an exhaustive enumeration of all sets of live lines on small instances, and the
//                engine with a naive recursive re-expansion on small synthetic proof trees, including
//                deliberately broken tables / leaf files that must be rejected.
//
// Sections: 1 cell sets, 2 geometry, 3 canonical form, 4 hash keys, 5 admissibility, 6 LEAF, 7 LOST,
// 8 children, 9 table / leaf file, 10 reference implementations (tests and --crosscheck only), 11 engine,
// 12 test scaffolding (naive solver producing small proof trees), 13 selftest, 14 command line.
//
// Build: g++ -O2 -std=c++17 -march=native -fopenmp -Wall  (see Makefile).  Usage: see usage() below.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef uint64_t u64;

#define CHECK(cond)                                                                      \
  do {                                                                                   \
    if (!(cond)) {                                                                       \
      fprintf(stderr, "treecheck: CHECK failed at line %d: %s\n", __LINE__, #cond);      \
      fflush(stderr);                                                                    \
      abort();                                                                           \
    }                                                                                    \
  } while (0)

static inline double nowSec() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// =====================================================================================================
// 1. Cell sets
// =====================================================================================================

static const int NC = 216;       // cells
static const int NLMAX = 148;    // lines
static const int NGMAX = 1152;   // symmetries

struct CellSet {
  u64 w[4];
  CellSet() : w{0, 0, 0, 0} {}
  inline bool test(int c) const { return (w[c >> 6] >> (c & 63)) & 1ULL; }
  inline void set(int c) { w[c >> 6] |= 1ULL << (c & 63); }
  inline void reset(int c) { w[c >> 6] &= ~(1ULL << (c & 63)); }
  inline int count() const {
    return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) + __builtin_popcountll(w[2]) +
           __builtin_popcountll(w[3]);
  }
  inline bool empty() const { return (w[0] | w[1] | w[2] | w[3]) == 0; }
  inline bool intersects(const CellSet& o) const {
    return ((w[0] & o.w[0]) | (w[1] & o.w[1]) | (w[2] & o.w[2]) | (w[3] & o.w[3])) != 0;
  }
  inline bool subsetOf(const CellSet& o) const {
    return ((w[0] & ~o.w[0]) | (w[1] & ~o.w[1]) | (w[2] & ~o.w[2]) | (w[3] & ~o.w[3])) == 0;
  }
  inline CellSet operator|(const CellSet& o) const {
    CellSet r;
    for (int i = 0; i < 4; i++) r.w[i] = w[i] | o.w[i];
    return r;
  }
  inline CellSet operator&(const CellSet& o) const {
    CellSet r;
    for (int i = 0; i < 4; i++) r.w[i] = w[i] & o.w[i];
    return r;
  }
  inline CellSet minus(const CellSet& o) const {
    CellSet r;
    for (int i = 0; i < 4; i++) r.w[i] = w[i] & ~o.w[i];
    return r;
  }
  inline bool operator==(const CellSet& o) const {
    return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3];
  }
  inline bool operator!=(const CellSet& o) const { return !(*this == o); }
  // ascending list of the cells; returns the number of cells
  inline int list(uint8_t* out) const {
    int n = 0;
    for (int i = 0; i < 4; i++) {
      u64 x = w[i];
      while (x) {
        out[n++] = (uint8_t)(i * 64 + __builtin_ctzll(x));
        x &= x - 1;
      }
    }
    return n;
  }
};

// Set order of SPEC 2.4: let p be the smallest cell whose membership differs; the set containing p is the
// smaller one.  Returns -1 if a < b, 0 if equal, +1 if a > b.
static inline int cmpSet(const CellSet& a, const CellSet& b) {
  for (int i = 0; i < 4; i++) {
    u64 x = a.w[i] ^ b.w[i];
    if (x) return ((a.w[i] >> __builtin_ctzll(x)) & 1ULL) ? -1 : +1;
  }
  return 0;
}

static std::string setStr(const CellSet& s) {
  uint8_t c[NC];
  int n = s.list(c);
  if (n == 0) return "-";
  std::string r;
  for (int i = 0; i < n; i++) {
    if (i) r += ',';
    r += std::to_string((int)c[i]);
  }
  return r;
}

// strict parser for "-" or "a,b,c" (cells 0..215, no duplicates; any order accepted)
static bool parseSetRange(const char* p, const char* e, CellSet& out) {
  out = CellSet();
  if (p >= e) return false;
  if (e - p == 1 && *p == '-') return true;
  for (;;) {
    if (p >= e || *p < '0' || *p > '9') return false;
    int v = 0, nd = 0;
    while (p < e && *p >= '0' && *p <= '9') {
      v = v * 10 + (*p - '0');
      if (++nd > 3) return false;
      p++;
    }
    if (v >= NC || out.test(v)) return false;
    out.set(v);
    if (p == e) return true;
    if (*p != ',') return false;
    p++;
  }
}
static bool parseIntRange(const char* p, const char* e, int& out) {
  if (p >= e || e - p > 6) return false;
  int v = 0;
  for (; p < e; p++) {
    if (*p < '0' || *p > '9') return false;
    v = v * 10 + (*p - '0');
  }
  out = v;
  return true;
}
// split [p,e) into whitespace separated tokens; returns the number of tokens (at most maxTok+1)
static int splitTokens(const char* p, const char* e, const char** tb, const char** te, int maxTok) {
  int n = 0;
  while (p < e) {
    while (p < e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (p >= e) break;
    const char* q = p;
    while (q < e && !(*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
    if (n < maxTok) {
      tb[n] = p;
      te[n] = q;
    }
    n++;
    p = q;
  }
  return n;
}
// "t B F" (three tokens)
static bool parseState3(const std::string& s, int& t, CellSet& B, CellSet& F) {
  const char* tb[4];
  const char* te[4];
  int n = splitTokens(s.data(), s.data() + s.size(), tb, te, 4);
  if (n != 3) return false;
  return parseIntRange(tb[0], te[0], t) && parseSetRange(tb[1], te[1], B) && parseSetRange(tb[2], te[2], F);
}

// =====================================================================================================
// 2. Geometry (SPEC section 1), built and asserted at every program start
// =====================================================================================================

static int NL = 0;                    // number of lines (148)
static uint8_t LINE[NLMAX][6];        // cells of a line, ascending
static CellSet LSET[NLMAX];           // the same as a set
static uint8_t NLC[NC];               // number of lines through a cell
static uint8_t LC[NC][7];             // lines through a cell
static int16_t MEET[NLMAX][NLMAX];    // the common cell of two distinct lines, or -1
static uint8_t ISBUSY[NC];
static int NBUSY = 0;
static uint8_t BUSY[24];
static int NG = 0;                    // number of symmetries (1152)
static uint8_t PERM[NGMAX][NC];       // PERM[g][c] = image of cell c under g
static int G_ID = -1;                 // index of the identity
static uint8_t ORBREP[NC];            // smallest cell id in the orbit of c
static uint16_t MAPOFF[NC + 1];       // MAPG[MAPOFF[c] .. MAPOFF[c+1]) = all g with g(c) = ORBREP[c]
static uint16_t MAPG[NC * 48];
// For c != c2: PAIRMIN[c][c2] = the smallest g(c2) over all g with g(c) = ORBREP[c], and
// PAIRG[PAIROFF[c*NC+c2] .. PAIROFF[c*NC+c2+1]) = the maps g attaining it.
static uint8_t PAIRMIN[NC][NC];
static std::vector<uint32_t> PAIROFF;
static std::vector<uint16_t> PAIRG;

static inline int cid(int x, int y, int z) { return 36 * x + 6 * y + z; }

static void initGeometry() {
  // ---- lines: all {(f1(t),f2(t),f3(t))}, fi constant / t / 5-t, not all constant; duplicates removed
  std::map<std::array<uint8_t, 6>, int> found;   // sorted cells -> number of non-constant coordinates
  for (int f1 = 0; f1 < 8; f1++)
    for (int f2 = 0; f2 < 8; f2++)
      for (int f3 = 0; f3 < 8; f3++) {
        if (f1 < 6 && f2 < 6 && f3 < 6) continue;
        std::array<uint8_t, 6> cells;
        for (int t = 0; t < 6; t++) {
          int v1 = f1 < 6 ? f1 : (f1 == 6 ? t : 5 - t);
          int v2 = f2 < 6 ? f2 : (f2 == 6 ? t : 5 - t);
          int v3 = f3 < 6 ? f3 : (f3 == 6 ? t : 5 - t);
          cells[t] = (uint8_t)cid(v1, v2, v3);
        }
        std::sort(cells.begin(), cells.end());
        int nvar = (f1 >= 6) + (f2 >= 6) + (f3 >= 6);
        auto it = found.find(cells);
        if (it == found.end())
          found[cells] = nvar;
        else
          CHECK(it->second == nvar);
      }
  CHECK(found.size() == 148);
  int byKind[4] = {0, 0, 0, 0};
  NL = 0;
  for (auto& kv : found) {
    byKind[kv.second]++;
    for (int i = 0; i < 6; i++) LINE[NL][i] = kv.first[i];
    NL++;
  }
  CHECK(NL == NLMAX);
  CHECK(byKind[1] == 108 && byKind[2] == 36 && byKind[3] == 4);
  for (int l = 0; l < NL; l++) {
    CellSet s;
    for (int i = 0; i < 6; i++) s.set(LINE[l][i]);
    CHECK(s.count() == 6);
    LSET[l] = s;
  }
  memset(NLC, 0, sizeof NLC);
  for (int l = 0; l < NL; l++)
    for (int i = 0; i < 6; i++) {
      int c = LINE[l][i];
      CHECK(NLC[c] < 7);
      LC[c][NLC[c]++] = (uint8_t)l;
    }
  // two cells share at most one line
  {
    std::vector<int16_t> lineOf(NC * NC, -1);
    for (int l = 0; l < NL; l++)
      for (int i = 0; i < 6; i++)
        for (int j = i + 1; j < 6; j++) {
          int c1 = LINE[l][i], c2 = LINE[l][j];
          CHECK(lineOf[c1 * NC + c2] == -1);
          lineOf[c1 * NC + c2] = lineOf[c2 * NC + c1] = (int16_t)l;
        }
  }
  // two lines share at most one cell
  for (int a = 0; a < NL; a++)
    for (int b = 0; b < NL; b++) MEET[a][b] = -1;
  for (int c = 0; c < NC; c++)
    for (int i = 0; i < NLC[c]; i++)
      for (int j = i + 1; j < NLC[c]; j++) {
        int a = LC[c][i], b = LC[c][j];
        CHECK(MEET[a][b] == -1);
        MEET[a][b] = MEET[b][a] = (int16_t)c;
      }
  for (int a = 0; a < NL; a++)
    for (int b = a + 1; b < NL; b++) {
      CellSet I = LSET[a] & LSET[b];
      int n = I.count();
      CHECK(n <= 1);
      CHECK((n == 1) == (MEET[a][b] >= 0));
      if (n == 1) CHECK(I.test(MEET[a][b]));
    }
  // ---- busy cells: min(x,5-x) = min(y,5-y) = min(z,5-z); 24 cells on 7 lines each, all others on 3 or 4
  NBUSY = 0;
  for (int c = 0; c < NC; c++) {
    int x = c / 36, y = (c / 6) % 6, z = c % 6;
    int mx = std::min(x, 5 - x), my = std::min(y, 5 - y), mz = std::min(z, 5 - z);
    bool busy = (mx == my && my == mz);
    ISBUSY[c] = busy;
    if (busy) {
      CHECK(NLC[c] == 7);
      CHECK(NBUSY < 24);
      BUSY[NBUSY++] = (uint8_t)c;
    } else {
      CHECK(NLC[c] == 3 || NLC[c] == 4);
    }
  }
  CHECK(NBUSY == 24);
  // ---- symmetries: coordinate permutation, then reversal of a subset of coordinates, then one relabelling
  //      s of {0..5} with s(5-i) = 5-s(i) applied to every coordinate.
  std::vector<std::array<int, 6>> relab;
  {
    std::array<int, 6> p = {0, 1, 2, 3, 4, 5};
    do {
      bool ok = true;
      for (int i = 0; i < 6; i++)
        if (p[5 - i] != 5 - p[i]) ok = false;
      if (ok) relab.push_back(p);
    } while (std::next_permutation(p.begin(), p.end()));
  }
  CHECK(relab.size() == 48);
  static const int cp[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::set<std::array<uint8_t, NC>> maps;
  for (int pi = 0; pi < 6; pi++)
    for (int rev = 0; rev < 8; rev++)
      for (auto& s : relab) {
        std::array<uint8_t, NC> m;
        for (int c = 0; c < NC; c++) {
          int v[3] = {c / 36, (c / 6) % 6, c % 6};
          int wv[3];
          for (int i = 0; i < 3; i++) {
            wv[i] = v[cp[pi][i]];
            if ((rev >> i) & 1) wv[i] = 5 - wv[i];
            wv[i] = s[wv[i]];
          }
          m[c] = (uint8_t)cid(wv[0], wv[1], wv[2]);
        }
        maps.insert(m);
      }
  CHECK(maps.size() == 1152);
  NG = 0;
  for (auto& m : maps) {
    memcpy(PERM[NG], m.data(), NC);
    NG++;
  }
  // every map is a bijection of the cells and sends every line onto a line
  std::set<std::array<uint8_t, 6>> lineSet;
  for (int l = 0; l < NL; l++) {
    std::array<uint8_t, 6> a;
    for (int i = 0; i < 6; i++) a[i] = LINE[l][i];
    lineSet.insert(a);
  }
  G_ID = -1;
  for (int g = 0; g < NG; g++) {
    CellSet img;
    bool id = true;
    for (int c = 0; c < NC; c++) {
      img.set(PERM[g][c]);
      if (PERM[g][c] != c) id = false;
    }
    CHECK(img.count() == NC);
    if (id) {
      CHECK(G_ID == -1);
      G_ID = g;
    }
    for (int l = 0; l < NL; l++) {
      std::array<uint8_t, 6> a;
      for (int i = 0; i < 6; i++) a[i] = PERM[g][LINE[l][i]];
      std::sort(a.begin(), a.end());
      CHECK(lineSet.count(a) == 1);
    }
  }
  CHECK(G_ID >= 0);
  // ---- orbit representatives and, for every cell, the maps sending it to its representative
  int n = 0;
  for (int c = 0; c < NC; c++) {
    int m = NC;
    for (int g = 0; g < NG; g++) m = std::min(m, (int)PERM[g][c]);
    ORBREP[c] = (uint8_t)m;
    MAPOFF[c] = (uint16_t)n;
    for (int g = 0; g < NG; g++)
      if (PERM[g][c] == m) {
        CHECK(n < (int)(sizeof(MAPG) / sizeof(MAPG[0])));
        MAPG[n++] = (uint16_t)g;
      }
  }
  MAPOFF[NC] = (uint16_t)n;
  PAIROFF.assign((size_t)NC * NC + 1, 0);
  PAIRG.clear();
  for (int c = 0; c < NC; c++)
    for (int c2 = 0; c2 < NC; c2++) {
      PAIROFF[(size_t)c * NC + c2] = (uint32_t)PAIRG.size();
      PAIRMIN[c][c2] = 255;
      if (c2 == c) continue;
      int m = NC;
      for (int j = MAPOFF[c]; j < MAPOFF[c + 1]; j++) m = std::min(m, (int)PERM[MAPG[j]][c2]);
      PAIRMIN[c][c2] = (uint8_t)m;
      for (int j = MAPOFF[c]; j < MAPOFF[c + 1]; j++)
        if (PERM[MAPG[j]][c2] == m) PAIRG.push_back(MAPG[j]);
    }
  PAIROFF[(size_t)NC * NC] = (uint32_t)PAIRG.size();
}

// =====================================================================================================
// 3. Canonical form (SPEC section 6)
// =====================================================================================================

struct CanonOut {
  CellSet B, F;
  int g;
};

static inline CellSet imageList(int g, const uint8_t* cells, int n) {
  CellSet r;
  const uint8_t* p = PERM[g];
  for (int i = 0; i < n; i++) r.set(p[cells[i]]);
  return r;
}
static CellSet imageSet(int g, const CellSet& s) {
  uint8_t c[NC];
  int n = s.list(c);
  return imageList(g, c, n);
}

// Smallest image pair (g(B), g(F)), B-images compared first (order of 2.4), F-images second.
// Speed-up: let P be the first non-empty one of B, F.  For sets of equal size the order of 2.4 is the
// lexicographic order of the ascending cell lists.  Let m0 be the smallest orbit representative of a cell
// of P: no image of P contains a cell below m0 and some image contains m0, so the smallest image contains
// m0.  Among the images containing m0, let m1 be the smallest possible second cell (if |P| >= 2): the
// smallest image has the two smallest cells m0 < m1.  Hence only the maps g with g(c) = m0 and g(c2) = m1
// for some cells c, c2 of P have to be tried (tables PAIRMIN / PAIRG).  Whatever this search returns, the
// result is computed as the image under ONE map g, so it is always a genuine symmetric copy of the input
// (see the header comment); minimality is tested against a brute-force search in --selftest.
static CanonOut canonState(const CellSet& B, const CellSet& F) {
  uint8_t bc[NC], fc[NC];
  int nb = B.list(bc), nf = F.list(fc);
  CanonOut out;
  out.g = G_ID;
  const uint8_t* pc = nb ? bc : fc;
  int np = nb ? nb : nf;
  if (np == 0) return out;
  int m0 = NC;
  for (int i = 0; i < np; i++) m0 = std::min(m0, (int)ORBREP[pc[i]]);
  uint16_t ties[NGMAX];
  int nt = 0;
  CellSet best;
  auto consider = [&](int g) {
    CellSet img = imageList(g, pc, np);
    int cmp = nt ? cmpSet(img, best) : -1;
    if (cmp < 0) {
      best = img;
      nt = 0;
      ties[nt++] = (uint16_t)g;
    } else if (cmp == 0) {
      CHECK(nt < NGMAX);
      ties[nt++] = (uint16_t)g;
    }
  };
  if (np == 1) {
    int c = pc[0];
    for (int j = MAPOFF[c]; j < MAPOFF[c + 1]; j++) consider(MAPG[j]);
  } else {
    int m1 = NC;
    for (int i = 0; i < np; i++) {
      int c = pc[i];
      if (ORBREP[c] != m0) continue;
      for (int j = 0; j < np; j++)
        if (j != i) m1 = std::min(m1, (int)PAIRMIN[c][pc[j]]);
    }
    for (int i = 0; i < np; i++) {
      int c = pc[i];
      if (ORBREP[c] != m0) continue;
      for (int j = 0; j < np; j++) {
        if (j == i || PAIRMIN[c][pc[j]] != m1) continue;
        size_t base = (size_t)c * NC + pc[j];
        for (uint32_t q = PAIROFF[base]; q < PAIROFF[base + 1]; q++) consider(PAIRG[q]);
      }
    }
  }
  CHECK(nt > 0);
  int gb = ties[0];
  if (nb && nf && nt > 1) {
    CellSet bf = imageList(gb, fc, nf);
    for (int j = 1; j < nt; j++) {
      CellSet img = imageList(ties[j], fc, nf);
      if (cmpSet(img, bf) < 0) {
        bf = img;
        gb = ties[j];
      }
    }
  }
  out.g = gb;
  out.B = imageList(gb, bc, nb);
  out.F = imageList(gb, fc, nf);
  return out;
}

// =====================================================================================================
// 4. Hash keys
// =====================================================================================================

enum { KP = 0, KQ = 1 };

struct K128 {
  u64 a, b;
};

static inline u64 rotl64(u64 x, int r) { return (x << r) | (x >> (64 - r)); }
static inline u64 fmix64(u64 k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;
  return k;
}
// 128-bit hash (MurmurHash3 x64-128 round structure) of the 10 words B[0..3], F[0..3], meta, constant.
static inline K128 stateKey(const CellSet& B, const CellSet& F, int t, int kind) {
  const u64 c1 = 0x87c37b91114253d5ULL, c2 = 0x4cf5ad432745937fULL;
  u64 blk[10] = {B.w[0], B.w[1], B.w[2], B.w[3], F.w[0], F.w[1], F.w[2], F.w[3],
                 ((u64)t << 8) | (u64)(kind + 1), 0x9e3779b97f4a7c15ULL};
  u64 h1 = 0x243f6a8885a308d3ULL, h2 = 0x13198a2e03707344ULL;
  for (int i = 0; i < 10; i += 2) {
    u64 k1 = blk[i], k2 = blk[i + 1];
    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    h1 ^= k1;
    h1 = rotl64(h1, 27);
    h1 += h2;
    h1 = h1 * 5 + 0x52dce729;
    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    h2 ^= k2;
    h2 = rotl64(h2, 31);
    h2 += h1;
    h2 = h2 * 5 + 0x38495ab5;
  }
  h1 ^= 80;
  h2 ^= 80;
  h1 += h2;
  h2 += h1;
  h1 = fmix64(h1);
  h2 = fmix64(h2);
  h1 += h2;
  h2 += h1;
  K128 k;
  k.a = h1;
  k.b = h2;
  if (k.a == 0 && k.b == 0) k.b = 1;   // (0,0) marks an empty slot
  return k;
}

// =====================================================================================================
// 5. Admissibility (SPEC section 3)
// =====================================================================================================

// X is B-admissible iff X and B are disjoint, every live line has at most 4 cells of X, and there is no
// danger pair: two distinct live lines with exactly 4 cells of X each that share a cell outside X.
// Two distinct lines share at most one cell, so a danger pair exists iff some cell outside X lies on two
// live lines with exactly 4 cells of X; "seen" collects the cells outside X of the lines found so far.
static bool admissible(const CellSet& B, const CellSet& X) {
  if (B.intersects(X)) return false;
  CellSet seen;
  for (int l = 0; l < NL; l++) {
    const CellSet& L = LSET[l];
    if (L.intersects(B)) continue;
    int c = (L & X).count();
    if (c > 4) return false;
    if (c == 4) {
      CellSet rest = L.minus(X);
      if (rest.intersects(seen)) return false;
      seen = seen | rest;
    }
  }
  return true;
}

// =====================================================================================================
// 6. LEAF(B,F,k) (SPEC section 3), exact integer arithmetic
// =====================================================================================================
//
// LEAF(B,F,k):  (1) F is B-admissible;  (2) a pairing for (B,F) exists;  (3) every F-connected set S of at
// least 3 live lines has sur_F(S) = |N(S)\F| - 2|S| >= theta := k - |F|.
//
// How (3) is decided once (1) and (2) hold (theta >= 1; for theta <= 0 it follows from Hall's condition):
//
//  * Call a line l STRONG if every set S of live lines containing l has sur_F(S) >= theta (i.e.
//    mu({l}) >= theta), and a pair {a,b} strong if every S containing a and b has sur_F(S) >= theta.
//    Every F-connected S (|S| >= 3) containing a strong line or a strong pair is fine: these are the two
//    pruning rules of SPEC section 3.
//  * Let D be a set of lines already known to be strong ("deleted" lines).  A "flow for the source set T
//    with gain q relative to D" is an assignment owner[] of cells to live lines such that every assigned
//    cell lies on its line and outside F, no cell is assigned twice (owner[] is a function), the lines of D
//    own nothing, every other live line outside T owns exactly 2 cells and the lines of T together own
//    2|T| + q cells.  If such a flow exists, then every S containing T has sur_F(S) >= min(q, theta):
//    if S meets D this holds because the lines of D are strong; otherwise the lines of S own at least
//    2|S| + q distinct cells, all inside N(S)\F.  This is the only fact needed for "LEAF = true"; the flow
//    is checked literally by validateFlow() (unless --fast is given).
//    So: a line with a flow of gain theta relative to the current D is strong and joins D (it gives up its
//    two cells, which makes all later searches short).  Deleting strong lines never changes which of the
//    remaining lines / pairs / triples have such flows (max-flow = min-cut over the sets avoiding D).
//  * Flows are found by augmenting paths (BFS) starting from the pairing.  If no augmenting path exists,
//    the set S of lines reached by the search contains T, avoids D (deleted lines own nothing, so they are
//    never reached) and owns every cell of N(S)\F, so sur_F(S) = current gain; S is F-connected.  This
//    gives a certificate for "LEAF = false" (reason 3), which is re-evaluated directly from the definition
//    before it is reported.
//  * Every F-connected S with |S| >= 3 contains an F-connected triple.  The enumeration below visits every
//    F-connected triple of live lines that contains no strong line and no strong F-adjacent pair, and
//    demands a flow with gain theta for it.  Lines / pairs that are not known to be strong are simply
//    treated as weak, which is always safe.
//  * Quick rejection: for a whole F-connected component S (|S| >= 3) sur_F(S) is evaluated directly.

struct LeafOut {
  bool holds = false;
  int reason = 0;          // 0 holds, 1 F inadmissible, 2 no pairing, 3 low-surplus set
  std::vector<int> S;      // reason 2: Hall violator; reason 3: F-connected set with sur < theta (line ids)
  int sur = 0;             // reason 3: sur_F(S)
};

static int surplusOf(const std::vector<int>& S, const CellSet& F) {
  CellSet N;
  for (int l : S) N = N | LSET[l];
  return N.minus(F).count() - 2 * (int)S.size();
}
// direct test of F-connectedness of a set of lines (used for certificates and tests)
static bool isFConnected(const std::vector<int>& S, const CellSet& F) {
  if (S.empty()) return false;
  std::vector<char> seen(S.size(), 0);
  std::vector<int> st;
  st.push_back(0);
  seen[0] = 1;
  size_t cnt = 1;
  while (!st.empty()) {
    int i = st.back();
    st.pop_back();
    for (size_t j = 0; j < S.size(); j++) {
      if (seen[j]) continue;
      CellSet I = (LSET[S[i]] & LSET[S[j]]).minus(F);
      if (!I.empty()) {
        seen[j] = 1;
        cnt++;
        st.push_back((int)j);
      }
    }
  }
  return cnt == S.size();
}

struct LeafChecker {
  static constexpr uint8_t NONE = 255;
  static constexpr int UNDO_MAX = 8192;
  uint8_t owner[NC];              // line owning the cell, or NONE
  uint8_t inF[NC];
  uint8_t isLive[NLMAX];
  uint8_t live[NLMAX];
  int nlive = 0;
  uint32_t lineStamp[NLMAX], cellStamp[NC], stamp = 0;
  uint8_t parentCell[NLMAX], parentLine[NC], bfsQ[NLMAX];
  int bfsN = 0;                   // after a failed search: bfsQ[0..bfsN) = the lines reached
  uint8_t undoCell[UNDO_MAX], undoOld[UNDO_MAX];
  int nundo = 0;
  uint8_t weak[NLMAX];
  uint8_t deleted[NLMAX];         // strong lines that have given up their cells (the set D above)
  uint8_t wpair[NLMAX][NLMAX];
  bool paranoid = false;
  u64 nCalls = 0, nBfs = 0, nSingle = 0, nPair = 0, nTriple = 0;

  LeafChecker() {
    memset(lineStamp, 0, sizeof lineStamp);
    memset(cellStamp, 0, sizeof cellStamp);
  }

  // One augmenting-path search from the source lines.  Residual moves: a reached line l may take any cell
  // c of l outside F that it does not own; if c is free the path ends, otherwise the owner of c is reached
  // (it will give up c and has to take another cell).  On success the ownership along the path is shifted:
  // the source at the start of the path gains one cell, every intermediate line keeps its number of cells.
  bool augment(const uint8_t* src, int ns, bool logUndo) {
    nBfs++;
    if (++stamp == 0) {
      memset(lineStamp, 0, sizeof lineStamp);
      memset(cellStamp, 0, sizeof cellStamp);
      stamp = 1;
    }
    int qh = 0, qt = 0;
    for (int i = 0; i < ns; i++) {
      int s = src[i];
      lineStamp[s] = stamp;
      parentCell[s] = NONE;
      bfsQ[qt++] = (uint8_t)s;
    }
    while (qh < qt) {
      int l = bfsQ[qh++];
      const uint8_t* lc = LINE[l];
      for (int i = 0; i < 6; i++) {
        int c = lc[i];
        if (inF[c] || cellStamp[c] == stamp) continue;
        int o = owner[c];
        if (o == l) continue;
        cellStamp[c] = stamp;
        parentLine[c] = (uint8_t)l;
        if (o == NONE) {
          int cur = c;
          for (;;) {
            int pl = parentLine[cur];
            if (logUndo) {
              CHECK(nundo < UNDO_MAX);
              undoCell[nundo] = (uint8_t)cur;
              undoOld[nundo] = owner[cur];
              nundo++;
            }
            owner[cur] = (uint8_t)pl;
            int pcell = parentCell[pl];
            if (pcell == NONE) break;
            cur = pcell;
          }
          return true;
        }
        if (lineStamp[o] != stamp) {
          lineStamp[o] = stamp;
          parentCell[o] = (uint8_t)c;
          bfsQ[qt++] = (uint8_t)o;
        }
      }
    }
    bfsN = qt;
    return false;
  }
  // up to theta augmentations from the pairing; returns the gain reached (changes are logged for rollback)
  int flow(const uint8_t* src, int ns, int theta) {
    int got = 0;
    while (got < theta && augment(src, ns, true)) got++;
    return got;
  }
  void rollback() {
    while (nundo > 0) {
      nundo--;
      owner[undoCell[nundo]] = undoOld[nundo];
    }
  }
  // literal check of "flow for the source set src with gain q" (see the comment above)
  void validateFlow(const uint8_t* src, int ns, int q) const {
    int cnt[NLMAX];
    memset(cnt, 0, sizeof cnt);
    for (int c = 0; c < NC; c++) {
      int o = owner[c];
      if (o == NONE) continue;
      CHECK(o < NL && isLive[o]);
      CHECK(!inF[c]);
      CHECK(LSET[o].test(c));
      cnt[o]++;
    }
    uint8_t isSrc[NLMAX];
    memset(isSrc, 0, sizeof isSrc);
    for (int i = 0; i < ns; i++) {
      CHECK(isLive[src[i]] && !isSrc[src[i]]);
      isSrc[src[i]] = 1;
    }
    int tot = 0;
    for (int i = 0; i < nlive; i++) {
      int l = live[i];
      if (deleted[l]) {
        CHECK(cnt[l] == 0 && !isSrc[l]);
      } else if (isSrc[l]) {
        CHECK(cnt[l] >= 2);
        tot += cnt[l];
      } else {
        CHECK(cnt[l] == 2);
      }
    }
    CHECK(tot == 2 * ns + q);
  }

  // The LEAF predicate.  If out != nullptr the reason and a certificate are stored.
  bool check(const CellSet& B, const CellSet& F, int k, LeafOut* out = nullptr) {
    nCalls++;
    if (out) *out = LeafOut();
    // (1) F is B-admissible
    if (!admissible(B, F)) {
      if (out) out->reason = 1;
      return false;
    }
    nlive = 0;
    for (int l = 0; l < NL; l++) {
      isLive[l] = !LSET[l].intersects(B);
      if (isLive[l]) live[nlive++] = (uint8_t)l;
    }
    for (int c = 0; c < NC; c++) {
      inF[c] = F.test(c);
      owner[c] = NONE;
    }
    memset(deleted, 0, sizeof deleted);
    // (2) pairing for (B,F): greedy start, then augmenting paths.  A failed search from a line l that owns
    // fewer than 2 cells reaches a set S of lines owning all of N(S)\F, i.e. |N(S)\F| <= 2|S|-1.
    {
      uint8_t cnt[NLMAX];
      memset(cnt, 0, sizeof cnt);
      for (int i = 0; i < nlive; i++) {
        int l = live[i];
        for (int j = 0; j < 6 && cnt[l] < 2; j++) {
          int c = LINE[l][j];
          if (!inF[c] && owner[c] == NONE) {
            owner[c] = (uint8_t)l;
            cnt[l]++;
          }
        }
      }
      for (int i = 0; i < nlive; i++) {
        uint8_t l = live[i];
        while (cnt[l] < 2) {
          if (!augment(&l, 1, false)) {
            std::vector<int> S(bfsQ, bfsQ + bfsN);
            CHECK(surplusOf(S, F) < 0);          // literal Hall violation
            if (out) {
              out->reason = 2;
              out->S = S;
              out->sur = surplusOf(S, F);
            }
            return false;
          }
          cnt[l]++;
        }
      }
      validateFlow(nullptr, 0, 0);               // the pairing itself is always validated literally
    }
    const int theta = k - F.count();
    if (theta <= 0) {
      if (out) out->holds = true;
      return true;
    }
    // quick rejection: sur_F of whole F-connected components, evaluated directly from the definition
    {
      int par[NLMAX];
      for (int i = 0; i < nlive; i++) par[live[i]] = live[i];
      auto find = [&](int x) {
        while (par[x] != x) {
          par[x] = par[par[x]];
          x = par[x];
        }
        return x;
      };
      for (int c = 0; c < NC; c++) {
        if (inF[c]) continue;
        int first = -1;
        for (int j = 0; j < NLC[c]; j++) {
          int l = LC[c][j];
          if (!isLive[l]) continue;
          if (first < 0)
            first = find(l);
          else {
            int r = find(l);
            if (r != first) par[r] = first;
          }
        }
      }
      int size[NLMAX];
      CellSet N[NLMAX];
      memset(size, 0, sizeof size);
      for (int i = 0; i < nlive; i++) {
        int r = find(live[i]);
        size[r]++;
        N[r] = N[r] | LSET[live[i]];
      }
      for (int i = 0; i < nlive; i++) {
        int r = live[i];
        if (par[r] != r || size[r] < 3) continue;
        int sur = N[r].minus(F).count() - 2 * size[r];
        if (sur < theta) {
          if (out) {
            out->reason = 3;
            for (int j = 0; j < nlive; j++)
              if (find(live[j]) == r) out->S.push_back(live[j]);
            CHECK(isFConnected(out->S, F));
            CHECK(surplusOf(out->S, F) == sur);
            out->sur = sur;
          }
          return false;
        }
      }
    }
    // Lines from which an augmenting path exists at all, found by one backward search from the free
    // cells: a line is "reaching" if it contains a free cell outside F, or a cell outside F owned by
    // another reaching line (this is exactly the relation followed forwards by augment()).  A reaching
    // line has a flow with gain 1; a line that is not reaching has gain 0 and stays so when reaching lines
    // are deleted later (nothing it can get to changes owner).  order[] lists the reaching lines by
    // increasing distance from the free cells.
    uint8_t reach[NLMAX], order[NLMAX];
    int nreach = 0;
    memset(reach, 0, sizeof reach);
    for (int c = 0; c < NC; c++) {
      if (inF[c] || owner[c] != NONE) continue;
      for (int j = 0; j < NLC[c]; j++) {
        int l = LC[c][j];
        if (isLive[l] && !reach[l]) {
          reach[l] = 1;
          order[nreach++] = (uint8_t)l;
        }
      }
    }
    for (int h = 0; h < nreach; h++) {
      int o = order[h];
      for (int i = 0; i < 6; i++) {
        int c = LINE[o][i];
        if (owner[c] != o) continue;             // the cells owned by o (they are outside F)
        for (int j = 0; j < NLC[c]; j++) {
          int l = LC[c][j];
          if (l != o && isLive[l] && !reach[l]) {
            reach[l] = 1;
            order[nreach++] = (uint8_t)l;
          }
        }
      }
    }
    // single lines; every line found strong is deleted at once (see the comment above the class)
    for (int i = 0; i < nlive; i++) weak[live[i]] = 1;
    for (int h = 0; h < nreach; h++) {
      uint8_t l = order[h];
      // a line cannot own more than |l\F| cells, so the gain of {l} is at most |l\F| - 2
      int avail = LSET[l].minus(F).count() - 2;
      if (avail < theta) continue;               // stays weak
      if (theta == 1) {
        // reaching = gain 1 = strong; with --paranoid the flow is constructed and checked as well
        if (paranoid) {
          int got = flow(&l, 1, 1);
          CHECK(got == 1);
          validateFlow(&l, 1, got);
          rollback();
        }
      } else {
        nSingle++;
        int got = flow(&l, 1, theta);
        if (got >= theta && paranoid) validateFlow(&l, 1, got);
        rollback();
        if (got < theta) continue;               // stays weak
      }
      weak[l] = 0;
      int freed = 0;
      for (int i = 0; i < 6; i++) {
        int c = LINE[l][i];
        if (owner[c] == l) {
          owner[c] = NONE;
          freed++;
        }
      }
      CHECK(freed == 2);
      deleted[l] = 1;
    }
    int nweak = 0;
    for (int i = 0; i < nlive; i++) {
      uint8_t l = live[i];
      nweak += weak[l];
      if (paranoid && !reach[l]) {               // cross-check: a line that is not reaching has gain 0
        int got = flow(&l, 1, 1);
        CHECK(got == 0);
        rollback();
      }
    }
    if (nweak < 3) {
      if (out) out->holds = true;
      return true;
    }
    // F-adjacent pairs of weak lines
    memset(wpair, 1, sizeof wpair);
    for (int i = 0; i < nlive; i++) {
      int a = live[i];
      if (!weak[a]) continue;
      for (int ci = 0; ci < 6; ci++) {
        int c = LINE[a][ci];
        if (inF[c]) continue;
        for (int j = 0; j < NLC[c]; j++) {
          int b = LC[c][j];
          if (b <= a || !isLive[b] || !weak[b]) continue;
          int avail = (LSET[a] | LSET[b]).minus(F).count() - 4;
          if (avail < theta) continue;           // stays weak
          nPair++;
          uint8_t src[2] = {(uint8_t)a, (uint8_t)b};
          int got = flow(src, 2, theta);
          if (got >= theta) {
            if (paranoid) validateFlow(src, 2, got);
            wpair[a][b] = wpair[b][a] = 0;
          }
          rollback();
        }
      }
    }
    // F-connected triples {a,b,c} with centre b (b F-adjacent to a and to c).  A triple whose lines are
    // pairwise F-adjacent is handled at its smallest line only.
    for (int i = 0; i < nlive; i++) {
      int b = live[i];
      if (!weak[b]) continue;
      uint8_t nb[NLMAX];
      int nn = 0;
      for (int ci = 0; ci < 6; ci++) {
        int c = LINE[b][ci];
        if (inF[c]) continue;
        for (int j = 0; j < NLC[c]; j++) {
          int a = LC[c][j];
          if (a == b || !isLive[a] || !weak[a] || !wpair[a][b]) continue;
          nb[nn++] = (uint8_t)a;
        }
      }
      for (int x = 0; x < nn; x++)
        for (int y = x + 1; y < nn; y++) {
          int a = nb[x], c = nb[y];
          int m = MEET[a][c];
          if (m >= 0 && !inF[m]) {
            if (!(b < a && b < c)) continue;     // handled at the smallest line of the triangle
            if (!wpair[a][c]) continue;          // strong pair
          }
          nTriple++;
          uint8_t src[3] = {(uint8_t)a, (uint8_t)b, (uint8_t)c};
          int got = flow(src, 3, theta);
          if (got < theta) {
            // certificate: the lines reached by the failed search
            std::vector<int> S(bfsQ, bfsQ + bfsN);
            int sur = surplusOf(S, F);
            CHECK(sur == got && sur < theta);
            CHECK(S.size() >= 3 && isFConnected(S, F));
            rollback();
            if (out) {
              out->reason = 3;
              out->S = S;
              out->sur = sur;
            }
            return false;
          }
          if (paranoid) validateFlow(src, 3, got);
          rollback();
        }
    }
    if (out) out->holds = true;
    return true;
  }
};

// =====================================================================================================
// 7. LOST(B,F,t) (SPEC section 4): patterns E2 and E3, exact, with sound lower-bound screens
// =====================================================================================================
//
// "need" = t+1-|F| is the number of new stones available.  Screens (all only use |S \ F| >= |S| - |F∩line|
// and the fact that two stone sets on different lines share at most one cell):
//   E2: new stones >= (5-f_l) + (4-f_a) + (4-f_b) - 2          (Sl∩Sa = {x}, |Sl∩Sb| <= 1, Sa∩Sb = ∅)
//   E3: new stones >= (4-f_a)+(4-f_a')+(4-f_b)+(4-f_b') - 4    (Sa∩Sb = Sa'∩Sb' = ∅, the other 4 pairs <= 1)
// where f_l = |F∩l|.  Whatever passes the screens is decided by explicit enumeration of the stone sets.

struct LostOut {
  bool lost = false;
  int pattern = 0;           // 2 or 3
  int ln[4] = {-1, -1, -1, -1};   // E2: l, a, b;  E3: a, a', b, b'
  int x = -1, e = -1, c = -1, c2 = -1;
  CellSet S[4];              // stone sets in the order of ln[]
  int stones = 0;            // |F ∪ stone sets|
};

struct LostCtx {
  uint8_t isLive[NLMAX];
  int f[NLMAX];
  int top[4];
  int nlive;
  int budget, room;
};
static void lostPrepare(const CellSet& B, const CellSet& F, int t, LostCtx& x) {
  x.nlive = 0;
  x.top[0] = x.top[1] = x.top[2] = x.top[3] = 0;
  for (int l = 0; l < NL; l++) {
    x.isLive[l] = !LSET[l].intersects(B);
    x.f[l] = 0;
    if (!x.isLive[l]) continue;
    x.nlive++;
    int v = (LSET[l] & F).count();
    x.f[l] = v;
    for (int i = 0; i < 4; i++)
      if (v > x.top[i]) std::swap(v, x.top[i]);   // keeps top[] = the 4 largest values, descending
  }
  x.budget = t + 1;
  x.room = t + 1 - F.count();
}

static bool lostE2(const CellSet& F, const LostCtx& x, LostOut* out) {
  const int room = x.room, budget = x.budget;
  const int* f = x.f;
  const int* top = x.top;
  if (x.nlive < 3) return false;
  if (11 - (top[0] + top[1] + top[2]) > room) return false;
  for (int l = 0; l < NL; l++) {
    if (!x.isLive[l]) continue;
    if ((5 - f[l]) + (4 - top[0]) + (4 - top[1]) - 2 > room) continue;
    for (int ix = 0; ix < 6; ix++) {
      const int cx = LINE[l][ix];
      for (int ja = 0; ja < NLC[cx]; ja++) {
        const int a = LC[cx][ja];
        if (a == l || !x.isLive[a]) continue;                      // l ∩ a = {x}
        if ((5 - f[l]) + (4 - f[a]) + (4 - top[0]) - 2 > room) continue;
        for (int ic = 0; ic < 6; ic++) {
          const int c = LINE[a][ic];
          if (c == cx || F.test(c)) continue;                      // c != x, c ∉ F
          for (int jb = 0; jb < NLC[c]; jb++) {
            const int b = LC[c][jb];
            if (b == a || b == l || !x.isLive[b]) continue;        // a ∩ b = {c}
            if (LSET[b].test(cx)) continue;                        // x ∉ b
            if ((5 - f[l]) + (4 - f[a]) + (4 - f[b]) - 2 > room) continue;
            for (int ie = 0; ie < 6; ie++) {
              const int e = LINE[l][ie];
              if (e == cx || e == c || F.test(e) || LSET[b].test(e)) continue;
              CellSet Sl = LSET[l];
              Sl.reset(e);
              CellSet base = F | Sl;
              for (int iy = 0; iy < 6; iy++) {
                const int ya = LINE[a][iy];                        // the cell of a\{c} left out of Sa
                if (ya == c || ya == cx || F.test(ya)) continue;   // x ∈ Sa, F∩a ⊆ Sa
                CellSet Sa = LSET[a];
                Sa.reset(c);
                Sa.reset(ya);
                CellSet u1 = base | Sa;
                for (int iz = 0; iz < 6; iz++) {
                  const int yb = LINE[b][iz];                      // the cell of b\{c} left out of Sb
                  if (yb == c || F.test(yb)) continue;             // F∩b ⊆ Sb
                  CellSet Sb = LSET[b];
                  Sb.reset(c);
                  Sb.reset(yb);
                  int n = (u1 | Sb).count();
                  if (n <= budget) {
                    if (out) {
                      out->lost = true;
                      out->pattern = 2;
                      out->ln[0] = l;
                      out->ln[1] = a;
                      out->ln[2] = b;
                      out->x = cx;
                      out->e = e;
                      out->c = c;
                      out->S[0] = Sl;
                      out->S[1] = Sa;
                      out->S[2] = Sb;
                      out->stones = n;
                    }
                    return true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
}

static bool lostE3(const CellSet& F, const LostCtx& x, LostOut* out) {
  const int room = x.room, budget = x.budget;
  const int* f = x.f;
  const int* top = x.top;
  if (x.nlive < 4) return false;
  if (12 - (top[0] + top[1] + top[2] + top[3]) > room) return false;
  for (int a = 0; a < NL; a++) {
    if (!x.isLive[a]) continue;
    if ((4 - f[a]) + 12 - (top[0] + top[1] + top[2]) - 4 > room) continue;
    for (int ix = 0; ix < 6; ix++) {
      const int cx = LINE[a][ix];
      for (int ja = 0; ja < NLC[cx]; ja++) {
        const int a2 = LC[cx][ja];
        if (a2 == a || !x.isLive[a2]) continue;                    // a ∩ a' = {x}
        if ((4 - f[a]) + (4 - f[a2]) + 8 - (top[0] + top[1]) - 4 > room) continue;
        for (int ic = 0; ic < 6; ic++) {
          const int c = LINE[a][ic];
          if (c == cx || F.test(c)) continue;
          for (int jb = 0; jb < NLC[c]; jb++) {
            const int b = LC[c][jb];
            if (b == a || b == a2 || !x.isLive[b]) continue;       // a ∩ b = {c}
            if (LSET[b].test(cx)) continue;                        // x ∉ b
            if ((4 - f[a]) + (4 - f[a2]) + (4 - f[b]) + (4 - top[0]) - 4 > room) continue;
            for (int ic2 = 0; ic2 < 6; ic2++) {
              const int c2 = LINE[a2][ic2];
              if (c2 == cx || c2 == c || F.test(c2)) continue;
              for (int jb2 = 0; jb2 < NLC[c2]; jb2++) {
                const int b2 = LC[c2][jb2];
                if (b2 == a2 || b2 == a || b2 == b || !x.isLive[b2]) continue;   // a' ∩ b' = {c'}
                if (LSET[b2].test(cx)) continue;                   // x ∉ b'
                if (16 - (f[a] + f[a2] + f[b] + f[b2]) - 4 > room) continue;
                // all stones lie in V \ {c,c'}; each of the 4 left-out cells removes at most one cell
                CellSet V = LSET[a] | LSET[a2] | LSET[b] | LSET[b2];
                V.reset(c);
                V.reset(c2);
                if (V.minus(F).count() - 4 > room) continue;
                for (int i1 = 0; i1 < 6; i1++) {
                  const int ya = LINE[a][i1];
                  if (ya == c || ya == cx || F.test(ya)) continue;
                  CellSet Sa = LSET[a];
                  Sa.reset(c);
                  Sa.reset(ya);
                  if (Sa.test(c2)) continue;
                  for (int i2 = 0; i2 < 6; i2++) {
                    const int ya2 = LINE[a2][i2];
                    if (ya2 == c2 || ya2 == cx || F.test(ya2)) continue;
                    CellSet Sa2 = LSET[a2];
                    Sa2.reset(c2);
                    Sa2.reset(ya2);
                    if (Sa2.test(c)) continue;
                    CellSet u2 = F | Sa | Sa2;
                    for (int i3 = 0; i3 < 6; i3++) {
                      const int yb = LINE[b][i3];
                      if (yb == c || F.test(yb)) continue;
                      CellSet Sb = LSET[b];
                      Sb.reset(c);
                      Sb.reset(yb);
                      if (Sb.test(c2)) continue;                   // c' in no stone set
                      CellSet u3 = u2 | Sb;
                      for (int i4 = 0; i4 < 6; i4++) {
                        const int yb2 = LINE[b2][i4];
                        if (yb2 == c2 || F.test(yb2)) continue;
                        CellSet Sb2 = LSET[b2];
                        Sb2.reset(c2);
                        Sb2.reset(yb2);
                        if (Sb2.test(c)) continue;                 // c in no stone set
                        int n = (u3 | Sb2).count();
                        if (n <= budget) {
                          if (out) {
                            out->lost = true;
                            out->pattern = 3;
                            out->ln[0] = a;
                            out->ln[1] = a2;
                            out->ln[2] = b;
                            out->ln[3] = b2;
                            out->x = cx;
                            out->c = c;
                            out->c2 = c2;
                            out->S[0] = Sa;
                            out->S[1] = Sa2;
                            out->S[2] = Sb;
                            out->S[3] = Sb2;
                            out->stones = n;
                          }
                          return true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
}

static bool lostCheck(const CellSet& B, const CellSet& F, int t, LostOut* out) {
  LostCtx x;
  lostPrepare(B, F, t, x);
  if (lostE2(F, x, out)) return true;
  if (lostE3(F, x, out)) return true;
  return false;
}

// =====================================================================================================
// 8. Children of a P-state (SPEC section 4, (a), (c), (d))
// =====================================================================================================

enum { CH_A = 0, CH_C = 1, CH_D = 2, CH_I = 3, CH_II = 4, NCH = 5 };

// emit(type, B', F'): the child is Q(B',F',t+1) for type CH_A and P(B',F',t+1) for CH_C / CH_D.
// VOID children are NOT filtered here.
template <class Emit>
static void genChildrenP(const CellSet& B, const CellSet& F, int t, Emit&& emit) {
  const int nF = F.count();
  const int room = t + 1 - nF;
  uint8_t isLive[NLMAX];
  int f[NLMAX];
  for (int l = 0; l < NL; l++) {
    isLive[l] = !LSET[l].intersects(B);
    f[l] = (LSET[l] & F).count();
  }
  // (a)
  emit((int)CH_A, B, F);
  // (c) threat: live line l, e ∈ l\F, |F ∪ (l\{e})| <= t+1
  for (int l = 0; l < NL; l++) {
    if (!isLive[l]) continue;
    if (5 - f[l] > room) continue;               // |F ∪ (l\{e})| = |F| + 5 - |F∩l| for e ∉ F
    for (int i = 0; i < 6; i++) {
      int e = LINE[l][i];
      if (F.test(e)) continue;
      CellSet Fc = F | LSET[l];
      Fc.reset(e);                               // e ∉ F, so this is F ∪ (l\{e})
      if (Fc.count() > t + 1) continue;
      CellSet Bc = B;
      Bc.set(e);
      emit((int)CH_C, Bc, Fc);
    }
  }
  // (d) danger: live lines a != b with a∩b = {c}, c ∉ F; Qa = a\{c,ya}, Qb = b\{c,yb};
  //     F∩a ⊆ Qa  <=>  ya ∉ F (as c ∉ F), same for b;  |F ∪ Qa ∪ Qb| <= t+1.
  for (int c = 0; c < NC; c++) {
    if (B.test(c) || F.test(c)) continue;
    for (int i = 0; i < NLC[c]; i++) {
      int a = LC[c][i];
      if (!isLive[a]) continue;
      for (int j = i + 1; j < NLC[c]; j++) {
        int b = LC[c][j];
        if (!isLive[b]) continue;
        if (8 - f[a] - f[b] > room) continue;    // |F ∪ Qa ∪ Qb| >= |F| + (4-|F∩a|) + (4-|F∩b|)
        for (int ia = 0; ia < 6; ia++) {
          int ya = LINE[a][ia];
          if (ya == c || F.test(ya)) continue;
          CellSet Qa = LSET[a];
          Qa.reset(c);
          Qa.reset(ya);
          for (int ib = 0; ib < 6; ib++) {
            int yb = LINE[b][ib];
            if (yb == c || F.test(yb)) continue;
            CellSet Qb = LSET[b];
            Qb.reset(c);
            Qb.reset(yb);
            CellSet Fc = F | Qa | Qb;
            if (Fc.count() > t + 1) continue;
            CellSet Bc = B;
            Bc.set(c);
            emit((int)CH_D, Bc, Fc);
          }
        }
      }
    }
  }
}

// =====================================================================================================
// 9. Strategy table and leaf file
// =====================================================================================================

// sorted array of 120-bit keys (a, b>>8) with an 8-bit payload (b & 255) and a prefix index
struct KeyStore {
  std::vector<K128> v;
  std::vector<u64> idx;
  int shift = 63;
  u64 dups = 0, conflicts = 0;
  void finalize() {
    std::sort(v.begin(), v.end(), [](const K128& x, const K128& y) { return x.a != y.a ? x.a < y.a : x.b < y.b; });
    size_t j = 0;
    for (size_t i = 0; i < v.size(); i++) {
      if (j > 0 && v[j - 1].a == v[i].a && (v[j - 1].b >> 8) == (v[i].b >> 8)) {
        if ((v[j - 1].b & 255) == (v[i].b & 255))
          dups++;
        else
          conflicts++;                           // same state, different payload: the first one is kept
        continue;
      }
      v[j++] = v[i];
    }
    v.resize(j);
    int bits = 4;
    while (bits < 26 && (1ULL << (bits + 1)) <= v.size()) bits++;
    shift = 64 - bits;
    idx.assign((1ULL << bits) + 1, 0);
    for (auto& e : v) idx[(e.a >> shift) + 1]++;
    for (size_t i = 1; i < idx.size(); i++) idx[i] += idx[i - 1];
  }
  bool find(const K128& k, int* payload) const {
    if (v.empty()) return false;
    u64 p = k.a >> shift;
    for (u64 i = idx[p]; i < idx[p + 1]; i++)
      if (v[i].a == k.a && (v[i].b >> 8) == (k.b >> 8)) {
        *payload = (int)(v[i].b & 255);
        return true;
      }
    return false;
  }
};

struct LoadStats {
  u64 lines = 0, entries = 0, bad = 0, noncanon = 0;
  u64 bytes = 0, fnv = 0xcbf29ce484222325ULL;    // size and FNV-1a 64 checksum of the raw file
  std::vector<std::string> badMsgs;
};

// Parses one line of the table ("t B F m", Q-state, |B| = t-1) or of the leaf file ("t B F", P-state,
// |B| = t).  Returns 0 for an empty/comment line, 1 for a valid entry (key filled), 2 for a malformed one.
static int parseEntry(const char* p, const char* e, bool isTable, K128& key, bool& noncanon) {
  const char* tb[6];
  const char* te[6];
  int n = splitTokens(p, e, tb, te, 5);
  if (n == 0) return 0;
  if (*tb[0] == '#') return 0;
  if (n != (isTable ? 4 : 3)) return 2;
  int t, m = 0;
  CellSet B, F;
  if (!parseIntRange(tb[0], te[0], t) || !parseSetRange(tb[1], te[1], B) || !parseSetRange(tb[2], te[2], F)) return 2;
  if (t > 60 || B.intersects(F) || F.count() > t) return 2;
  if (isTable) {
    if (!parseIntRange(tb[3], te[3], m) || m >= NC) return 2;
    if (B.count() != t - 1 || B.test(m) || F.test(m)) return 2;
  } else {
    if (B.count() != t) return 2;
  }
  // re-canonicalise with this program's canonical form; the move is mapped with the same g
  CanonOut cn = canonState(B, F);
  noncanon = (cn.B != B || cn.F != F);
  key = stateKey(cn.B, cn.F, t, isTable ? KQ : KP);
  if (isTable) key.b = (key.b & ~255ULL) | (u64)PERM[cn.g][m];
  else key.b = (key.b & ~255ULL);
  return 1;
}

static void loadKeys(FILE* f, bool isTable, int nthreads, KeyStore& ks, LoadStats& st) {
  const size_t CHUNK = (size_t)1 << 24;
  std::vector<char> buf;
  size_t have = 0;
  bool eof = false;
  while (!eof) {
    buf.resize(have + CHUNK);
    size_t n = fread(buf.data() + have, 1, CHUNK, f);
    for (size_t i = 0; i < n; i++) st.fnv = (st.fnv ^ (unsigned char)buf[have + i]) * 0x100000001b3ULL;
    st.bytes += n;
    size_t total = have + n;
    if (n == 0) eof = true;
    size_t end = total;
    if (!eof) {
      while (end > 0 && buf[end - 1] != '\n') end--;
      if (end == 0) {
        have = total;
        continue;
      }
    }
    std::vector<size_t> starts;
    {
      size_t p = 0;
      while (p < end) {
        starts.push_back(p);
        const char* q = (const char*)memchr(buf.data() + p, '\n', end - p);
        p = q ? (size_t)(q - buf.data()) + 1 : end;
      }
      starts.push_back(end);
    }
    size_t nl = starts.size() - 1;
    std::vector<K128> keys(nl);
    std::vector<uint8_t> flag(nl, 0);
    auto work = [&](size_t lo, size_t hi) {
      for (size_t i = lo; i < hi; i++) {
        bool nc = false;
        int r = parseEntry(buf.data() + starts[i], buf.data() + starts[i + 1], isTable, keys[i], nc);
        flag[i] = (uint8_t)(r | (nc ? 4 : 0));
      }
    };
    int nt = std::max(1, std::min(nthreads, (int)(nl / 2000) + 1));
    if (nt == 1)
      work(0, nl);
    else {
      std::vector<std::thread> th;
      for (int i = 0; i < nt; i++) th.emplace_back(work, nl * i / nt, nl * (i + 1) / nt);
      for (auto& x : th) x.join();
    }
    for (size_t i = 0; i < nl; i++) {
      int r = flag[i] & 3;
      if (r == 0) continue;
      st.lines++;
      if (r == 2) {
        st.bad++;
        if (st.badMsgs.size() < 10) {
          std::string s(buf.data() + starts[i], buf.data() + starts[i + 1]);
          while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
          if (s.size() > 200) s.resize(200);
          st.badMsgs.push_back(s);
        }
        continue;
      }
      if (flag[i] & 4) st.noncanon++;
      ks.v.push_back(keys[i]);
    }
    memmove(buf.data(), buf.data() + end, total - end);
    have = total - end;
  }
  ks.finalize();
  st.entries = ks.v.size();
}

// =====================================================================================================
// 10. Reference implementations (used by --selftest and --crosscheck; deliberately naive and literal)
// =====================================================================================================

struct Rng {
  u64 s;
  explicit Rng(u64 seed) : s(seed) {}
  u64 next() {
    s += 0x9e3779b97f4a7c15ULL;
    u64 z = s;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }
  int below(int n) { return (int)(next() % (u64)n); }
};

static CanonOut canonBrute(const CellSet& B, const CellSet& F) {
  CanonOut best;
  bool have = false;
  for (int g = 0; g < NG; g++) {
    CellSet b = imageSet(g, B), f = imageSet(g, F);
    int c = have ? cmpSet(b, best.B) : -1;
    if (c == 0) c = cmpSet(f, best.F);
    if (c < 0) {
      best.B = b;
      best.F = f;
      best.g = g;
      have = true;
    }
  }
  return best;
}

static bool admissibleRef(const CellSet& B, const CellSet& X) {
  if (!(B & X).empty()) return false;
  std::vector<int> lv;
  for (int l = 0; l < NL; l++)
    if ((LSET[l] & B).empty()) lv.push_back(l);
  for (int l : lv)
    if ((LSET[l] & X).count() > 4) return false;
  for (size_t i = 0; i < lv.size(); i++)
    for (size_t j = i + 1; j < lv.size(); j++) {
      int a = lv[i], b = lv[j];
      if ((LSET[a] & X).count() != 4 || (LSET[b] & X).count() != 4) continue;
      CellSet I = LSET[a] & LSET[b];
      if (!I.empty() && !I.subsetOf(X)) return false;
    }
  return true;
}

static const int NO_SET = 1000;   // "no F-connected set of >= 3 live lines exists"

// LEAF by brute force over all subsets of live lines (definition of section 3, Hall's condition for the
// pairing).  Returns -1 if there are more than maxLive live lines, 1 if F is not B-admissible, 2 if no
// pairing exists, and otherwise 0 with minSur = min of sur_F(S) over all F-connected S with |S| >= 3
// (NO_SET if there is none).  Then LEAF(B,F,k) holds iff k - |F| <= minSur.
static int leafBrute(const CellSet& B, const CellSet& F, int maxLive, int& minSur) {
  minSur = NO_SET;
  if (!admissibleRef(B, F)) return 1;
  std::vector<int> lv;
  for (int l = 0; l < NL; l++)
    if ((LSET[l] & B).empty()) lv.push_back(l);
  int n = (int)lv.size();
  if (n > maxLive) return -1;
  std::vector<uint32_t> adj(n, 0);
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      if (i != j && !(LSET[lv[i]] & LSET[lv[j]]).minus(F).empty()) adj[i] |= 1u << j;
  std::vector<CellSet> N((size_t)1 << n);
  bool hall = true;
  for (uint32_t mask = 1; mask < (1u << n); mask++) {
    int low = __builtin_ctz(mask);
    N[mask] = N[mask & (mask - 1)] | LSET[lv[low]];
    int s = __builtin_popcount(mask);
    int cnt = N[mask].minus(F).count();
    if (cnt < 2 * s) hall = false;
    if (s >= 3 && cnt - 2 * s < minSur) {
      uint32_t reach = 1u << low, fr = reach;
      while (fr) {
        int i = __builtin_ctz(fr);
        fr &= fr - 1;
        uint32_t nw = adj[i] & mask & ~reach;
        reach |= nw;
        fr |= nw;
      }
      if (reach == mask) minSur = cnt - 2 * s;
    }
  }
  if (!hall) return 2;
  return 0;
}

// LEAF through the "triple" characterisation with an independent matching code (Kuhn's recursive
// augmentation, no pruning, every F-connected triple, every flow pushed to its maximum).  Returns 1 / 2 as
// above, otherwise 0 with minMu = min over all F-connected triples T of mu(T) (NO_SET if there is none).
struct KuhnRef {
  std::vector<int> lv;
  int user[NC];
  bool inF[NC];
  bool vis[NC];
  bool tryLine(int l) {
    for (int i = 0; i < 6; i++) {
      int c = LINE[l][i];
      if (inF[c] || user[c] == l || vis[c]) continue;
      vis[c] = true;
      if (user[c] < 0 || tryLine(user[c])) {
        user[c] = l;
        return true;
      }
    }
    return false;
  }
  int run(const CellSet& B, const CellSet& F, int& minMu) {
    minMu = NO_SET;
    if (!admissibleRef(B, F)) return 1;
    lv.clear();
    for (int l = 0; l < NL; l++)
      if ((LSET[l] & B).empty()) lv.push_back(l);
    for (int c = 0; c < NC; c++) {
      user[c] = -1;
      inF[c] = F.test(c);
    }
    for (int l : lv)
      for (int r = 0; r < 2; r++) {
        memset(vis, 0, sizeof vis);
        if (!tryLine(l)) return 2;
      }
    int n = (int)lv.size();
    std::vector<std::vector<char>> adj(n, std::vector<char>(n, 0));
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        if (i != j && !(LSET[lv[i]] & LSET[lv[j]]).minus(F).empty()) adj[i][j] = 1;
    int save[NC];
    memcpy(save, user, sizeof user);
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++)
        for (int h = j + 1; h < n; h++) {
          if (adj[i][j] + adj[j][h] + adj[i][h] < 2) continue;
          int T[3] = {lv[i], lv[j], lv[h]};
          int got = 0;
          bool progress = true;
          while (progress) {
            progress = false;
            for (int q = 0; q < 3; q++)
              for (;;) {
                memset(vis, 0, sizeof vis);
                if (!tryLine(T[q])) break;
                got++;
                progress = true;
              }
          }
          memcpy(user, save, sizeof user);
          minMu = std::min(minMu, got);
        }
    return 0;
  }
};

// LOST, literal: returns bit 0 for E2, bit 1 for E3; if doE3 is false E3 is skipped and bit 2 is set.
// prune = false: no pruning at all (very slow for E3 when many lines are live).
// prune = true : a triple / quadruple of lines is skipped when even all stones of F on these lines cannot
//                bring the pattern within the budget: a pattern has at least 11 (E2) resp. 12 (E3) distinct
//                stones, all on its lines, so it needs at least 11 resp. 12 minus |F ∩ (union of the lines)|
//                new stones.  (A different and cruder bound than the ones used by lostE2 / lostE3.)
static int lostRef(const CellSet& B, const CellSet& F, int t, bool doE3, bool prune) {
  std::vector<int> lv;
  for (int l = 0; l < NL; l++)
    if ((LSET[l] & B).empty()) lv.push_back(l);
  int res = 0;
  const int room = t + 1 - F.count();
  std::vector<int16_t> common(NLMAX * NLMAX, -1);   // the unique common cell of two lines, or -1
  std::vector<std::vector<int>> meet(NLMAX);        // live lines sharing exactly one cell with a live line
  for (int a : lv)
    for (int b : lv) {
      if (a == b) continue;
      CellSet I = LSET[a] & LSET[b];
      if (I.count() != 1) continue;
      uint8_t c[4];
      I.list(c);
      common[a * NLMAX + b] = c[0];
      meet[a].push_back(b);
    }
  auto single = [&](int a, int b) -> int { return common[a * NLMAX + b]; };
  auto subsets4 = [&](int line, int without) {    // all 4-subsets of line \ {without}
    std::vector<CellSet> r;
    std::vector<int> cells;
    for (int i = 0; i < 6; i++)
      if (LINE[line][i] != without) cells.push_back(LINE[line][i]);
    for (int m = 0; m < 32; m++)
      if (__builtin_popcount(m) == 4) {
        CellSet s;
        for (int i = 0; i < 5; i++)
          if ((m >> i) & 1) s.set(cells[i]);
        r.push_back(s);
      }
    return r;
  };
  // E2: lines l, a, b; l∩a = {x}; a∩b = {c}
  bool e2 = false;
  for (size_t il = 0; il < lv.size() && !e2; il++) {
    int l = lv[il];
    for (size_t ia = 0; ia < meet[l].size() && !e2; ia++) {
      int a = meet[l][ia];
      int x = single(l, a);
      for (size_t ib = 0; ib < meet[a].size() && !e2; ib++) {
        int b = meet[a][ib];
        if (b == l || b == a) continue;
        int c = single(a, b);
        if (prune && 11 - (F & (LSET[l] | LSET[a] | LSET[b])).count() > room) continue;
        if (LSET[b].test(x)) continue;
        if (F.test(c)) continue;
        for (int ie = 0; ie < 6 && !e2; ie++) {
          int e = LINE[l][ie];
          if (e == x || e == c || x == c) continue;
          if (LSET[b].test(e) || F.test(e)) continue;
          CellSet Sl = LSET[l];
          Sl.reset(e);
          for (auto& Sa : subsets4(a, c)) {
            if (!Sa.test(x) || !(F & LSET[a]).subsetOf(Sa)) continue;
            for (auto& Sb : subsets4(b, c)) {
              if (!(F & LSET[b]).subsetOf(Sb)) continue;
              if ((F | Sl | Sa | Sb).count() <= t + 1) e2 = true;
            }
          }
        }
      }
    }
  }
  if (e2) res |= 1;
  if (!doE3) return res | 4;
  // E3: lines a, a', b, b'; a∩a' = {x}; a∩b = {c}; a'∩b' = {c'}
  bool e3 = false;
  for (size_t ia = 0; ia < lv.size() && !e3; ia++) {
    int a = lv[ia];
    for (size_t ia2 = 0; ia2 < meet[a].size() && !e3; ia2++) {
      int a2 = meet[a][ia2];
      int x = single(a, a2);
      for (size_t ib = 0; ib < meet[a].size() && !e3; ib++) {
        int b = meet[a][ib];
        if (b == a || b == a2) continue;
        int c = single(a, b);
        if (c == x || F.test(c) || LSET[b].test(x)) continue;
        for (size_t ib2 = 0; ib2 < meet[a2].size() && !e3; ib2++) {
          int b2 = meet[a2][ib2];
          if (b2 == a || b2 == a2 || b2 == b) continue;
          int c2 = single(a2, b2);
          if (c2 == x || c2 == c || F.test(c2) || LSET[b2].test(x)) continue;
          if (prune && 12 - (F & (LSET[a] | LSET[a2] | LSET[b] | LSET[b2])).count() > room) continue;
          for (auto& Sa : subsets4(a, c)) {
            if (!Sa.test(x) || Sa.test(c2) || !(F & LSET[a]).subsetOf(Sa)) continue;
            for (auto& Sa2 : subsets4(a2, c2)) {
              if (!Sa2.test(x) || Sa2.test(c) || !(F & LSET[a2]).subsetOf(Sa2)) continue;
              for (auto& Sb : subsets4(b, c)) {
                if (Sb.test(c2) || !(F & LSET[b]).subsetOf(Sb)) continue;
                for (auto& Sb2 : subsets4(b2, c2)) {
                  if (Sb2.test(c) || !(F & LSET[b2]).subsetOf(Sb2)) continue;
                  if ((F | Sa | Sa2 | Sb | Sb2).count() <= t + 1) e3 = true;
                }
              }
            }
          }
        }
      }
    }
  }
  if (e3) res |= 2;
  return res;
}

// children (c) and (d) of a P-state, literal; returns "kind t B F" strings of the (uncanonicalised) children
static std::set<std::string> childrenRef(const CellSet& B, const CellSet& F, int t) {
  std::set<std::string> out;
  auto key = [&](const char* kind, const CellSet& Bc, const CellSet& Fc) {
    return std::string(kind) + " " + std::to_string(t + 1) + " " + setStr(Bc) + " " + setStr(Fc);
  };
  out.insert(key("Q", B, F));
  std::vector<int> lv;
  for (int l = 0; l < NL; l++)
    if ((LSET[l] & B).empty()) lv.push_back(l);
  for (int l : lv)
    for (int i = 0; i < 6; i++) {
      int e = LINE[l][i];
      if (F.test(e)) continue;
      CellSet S = LSET[l];
      S.reset(e);
      CellSet Fc = F | S;
      if (Fc.count() > t + 1) continue;
      CellSet Bc = B;
      Bc.set(e);
      out.insert(key("P", Bc, Fc));
    }
  for (size_t i = 0; i < lv.size(); i++)
    for (size_t j = i + 1; j < lv.size(); j++) {
      int a = lv[i], b = lv[j];
      CellSet I = LSET[a] & LSET[b];
      if (I.count() != 1) continue;
      uint8_t cc[4];
      I.list(cc);
      int c = cc[0];
      if (F.test(c)) continue;
      std::vector<int> ca, cb;
      for (int q = 0; q < 6; q++) {
        if (LINE[a][q] != c) ca.push_back(LINE[a][q]);
        if (LINE[b][q] != c) cb.push_back(LINE[b][q]);
      }
      for (int ma = 0; ma < 32; ma++) {
        if (__builtin_popcount(ma) != 4) continue;
        CellSet Qa;
        for (int q = 0; q < 5; q++)
          if ((ma >> q) & 1) Qa.set(ca[q]);
        if (!(F & LSET[a]).subsetOf(Qa)) continue;
        for (int mb = 0; mb < 32; mb++) {
          if (__builtin_popcount(mb) != 4) continue;
          CellSet Qb;
          for (int q = 0; q < 5; q++)
            if ((mb >> q) & 1) Qb.set(cb[q]);
          if (!(F & LSET[b]).subsetOf(Qb)) continue;
          CellSet Fc = F | Qa | Qb;
          if (Fc.count() > t + 1) continue;
          CellSet Bc = B;
          Bc.set(c);
          out.insert(key("P", Bc, Fc));
        }
      }
    }
  return out;
}

static std::string stateStr(int t, const CellSet& B, const CellSet& F) {
  return std::to_string(t) + " " + setStr(B) + " " + setStr(F);
}

// Reference re-expansion: recursive, single threaded, std::set of strings, brute-force canonical form,
// literal children, literal LOST, literal admissibility.  LEAF is taken from LeafChecker (it is validated
// separately).  Used to cross-check the counts of the engine on small trees.
struct RefExpand {
  std::map<std::string, int> table;      // canonical "t B F" -> canonical move
  std::set<std::string> leaves;          // canonical "t B F"
  std::set<std::string> seen;
  int rmax = 0;
  long pStates = 0, qStates = 0, leafCount = 0, errors = 0;
  LeafChecker lc;
  void addTableLine(const std::string& line) {
    const char* tb[5];
    const char* te[5];
    int n = splitTokens(line.data(), line.data() + line.size(), tb, te, 5);
    if (n != 4) return;
    int t, m;
    CellSet B, F;
    CHECK(parseIntRange(tb[0], te[0], t) && parseSetRange(tb[1], te[1], B) && parseSetRange(tb[2], te[2], F) &&
          parseIntRange(tb[3], te[3], m));
    CanonOut cn = canonBrute(B, F);
    std::string k = stateStr(t, cn.B, cn.F);
    if (!table.count(k)) table[k] = PERM[cn.g][m];
  }
  void addLeafLine(const std::string& line) {
    int t;
    CellSet B, F;
    if (!parseState3(line, t, B, F)) return;
    CanonOut cn = canonBrute(B, F);
    leaves.insert(stateStr(t, cn.B, cn.F));
  }
  void visit(int kind, const CellSet& B0, const CellSet& F0, int t) {
    CanonOut cn = canonBrute(B0, F0);
    const CellSet B = cn.B, F = cn.F;
    std::string k = stateStr(t, B, F);
    if (!seen.insert((kind == KP ? "P " : "Q ") + k).second) return;
    if (kind == KP) {
      pStates++;
      if (leaves.count(k)) {
        if (lc.check(B, F, t))
          leafCount++;
        else
          errors++;
        return;
      }
      if (t >= rmax) {
        errors++;
        return;
      }
      if (lostRef(B, F, t, true, true) & 3) {
        errors++;
        return;
      }
      for (const std::string& ch : childrenRef(B, F, t)) {
        int tc;
        CellSet Bc, Fc;
        CHECK(parseState3(ch.substr(2), tc, Bc, Fc));
        if (!admissibleRef(Bc, Fc)) continue;
        visit(ch[0] == 'P' ? KP : KQ, Bc, Fc, tc);
      }
    } else {
      qStates++;
      auto it = table.find(k);
      if (it == table.end()) {
        errors++;
        return;
      }
      int m = it->second;
      if (B.test(m) || F.test(m)) {
        errors++;
        return;
      }
      CellSet B1 = B;
      B1.set(m);
      if (!admissibleRef(B1, F)) {
        errors++;
        return;
      }
      visit(KP, B1, F, t);
      if (F.count() < t) {
        CellSet F1 = F;
        F1.set(m);
        if (admissibleRef(B, F1)) visit(KQ, B, F1, t);
      }
    }
  }
};

// =====================================================================================================
// 11. The expansion engine
// =====================================================================================================

struct Item {
  CellSet B, F;
  uint8_t t, kind;
};

struct FullKey {
  u64 w[9];
  bool operator==(const FullKey& o) const { return memcmp(w, o.w, sizeof w) == 0; }
};
struct FullKeyHash {
  size_t operator()(const FullKey& k) const {
    u64 h = 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 9; i++) h = fmix64(h ^ k.w[i]) + 0x632be59bd9b4e019ULL * (u64)(i + 1);
    return (size_t)h;
  }
};

// concurrent visited set: 1024 shards, each an open-addressing table of 128-bit keys (or, with --exact,
// a hash set of the full keys)
struct Visited {
  static const int NSH = 1024;
  struct Shard {
    std::mutex mu;
    std::vector<K128> tab;
    size_t n = 0;
    std::unordered_set<FullKey, FullKeyHash> full;
  };
  std::vector<Shard> sh;
  bool exact = false;
  Visited() : sh(NSH) {}
  // returns true iff the state was not present before
  bool insert(const CellSet& B, const CellSet& F, int t, int kind) {
    K128 k = stateKey(B, F, t, kind);
    Shard& s = sh[k.a >> 54];
    std::lock_guard<std::mutex> lk(s.mu);
    if (exact) {
      FullKey fk;
      for (int i = 0; i < 4; i++) {
        fk.w[i] = B.w[i];
        fk.w[4 + i] = F.w[i];
      }
      fk.w[8] = ((u64)t << 8) | (u64)(kind + 1);
      bool ins = s.full.insert(fk).second;
      if (ins) s.n++;
      return ins;
    }
    if ((s.n + 1) * 10 > s.tab.size() * 6) {
      size_t ncap = s.tab.empty() ? 64 : s.tab.size() * 2;
      std::vector<K128> nt(ncap, K128{0, 0});
      for (auto& e : s.tab) {
        if (e.a == 0 && e.b == 0) continue;
        size_t i = e.b & (ncap - 1);
        while (nt[i].a != 0 || nt[i].b != 0) i = (i + 1) & (ncap - 1);
        nt[i] = e;
      }
      s.tab.swap(nt);
    }
    size_t mask = s.tab.size() - 1;
    size_t i = k.b & mask;
    for (;;) {
      K128& e = s.tab[i];
      if (e.a == 0 && e.b == 0) {
        e = k;
        s.n++;
        return true;
      }
      if (e.a == k.a && e.b == k.b) return false;
      i = (i + 1) & mask;
    }
  }
  u64 size() {
    u64 n = 0;
    for (auto& s : sh) n += s.n;
    return n;
  }
};

struct ErrRec {
  std::string kind, state, msg;
};

struct Stats {
  u64 p[64], q[64], leaves[64];
  u64 edges[NCH][64];       // non-VOID children generated, by type and by the t of the child
  u64 voids[NCH][64];       // VOID children skipped
  u64 fresh[NCH][64];       // children that were new states
  u64 nonbusy = 0;
  int maxT = 0;
  Stats() {
    memset(p, 0, sizeof p);
    memset(q, 0, sizeof q);
    memset(leaves, 0, sizeof leaves);
    memset(edges, 0, sizeof edges);
    memset(voids, 0, sizeof voids);
    memset(fresh, 0, sizeof fresh);
  }
  void add(const Stats& o) {
    for (int i = 0; i < 64; i++) {
      p[i] += o.p[i];
      q[i] += o.q[i];
      leaves[i] += o.leaves[i];
      for (int k = 0; k < NCH; k++) {
        edges[k][i] += o.edges[k][i];
        voids[k][i] += o.voids[k][i];
        fresh[k][i] += o.fresh[k][i];
      }
    }
    nonbusy += o.nonbusy;
    maxT = std::max(maxT, o.maxT);
  }
};

struct EngineConfig {
  int rmax = 0;
  int threads = 1;
  bool exact = false, paranoid = false, busyOnly = false;
  bool crosscheck = false;      // re-derive everything with the naive reference code of section 10 as well
  int leafSample = 0;           // with crosscheck: re-check every leaf whose key is 0 mod leafSample with KuhnRef
  u64 maxErrors = 50;
  double timeLimit = 0, progressSec = 0;
  int rootT = 0;
  CellSet rootB, rootF;
};

struct EngineResult {
  bool ok = false, complete = false, rootVoid = false, timedOut = false;
  u64 pStates = 0, qStates = 0, leavesChecked = 0, errorCount = 0, visited = 0;
  std::vector<ErrRec> errors;
  Stats st;
  double seconds = 0;
  u64 leafBfs = 0, leafTriples = 0;
  u64 crossStates = 0, crossExpanded = 0, crossLeaves = 0;   // --crosscheck: states / expansions / leaves re-derived
  std::vector<u64> perThread;      // states expanded by each worker
};

struct Engine {
  EngineConfig cfg;
  KeyStore table, leaves;
  LoadStats tableLoad, leafLoad;

  Visited visited;
  std::mutex mu;
  std::condition_variable cv;
  std::vector<Item> global;
  int idle = 0;
  bool done = false;
  std::atomic<bool> stop{false};
  std::atomic<int> idleA{0};
  std::atomic<size_t> globalSize{0};
  std::mutex emu;
  std::vector<ErrRec> errors;
  std::atomic<u64> nerrors{0};

  struct alignas(64) Worker {
    std::unique_ptr<LeafChecker> lc;
    Stats st;
    std::vector<Item> local;
    std::atomic<u64> expanded{0};   // states expanded by this worker (read by the progress monitor)
    std::unique_ptr<KuhnRef> kr;    // --crosscheck only
    u64 crossStates = 0, crossExpanded = 0, crossLeaves = 0;
    char pad[64];
  };

  void addError(const char* kind, const CellSet& B, const CellSet& F, int t, const std::string& msg) {
    u64 n = nerrors.fetch_add(1);
    if (n < 50) {
      std::lock_guard<std::mutex> lk(emu);
      ErrRec e;
      e.kind = kind;
      e.state = std::to_string(t) + " " + setStr(B) + " " + setStr(F);
      e.msg = msg;
      errors.push_back(e);
    }
    if (n + 1 >= cfg.maxErrors) stop = true;
  }

  // child of the given type: skipped if VOID, otherwise replaced by its canonical representative and
  // queued unless that representative was queued before
  void pushChild(Worker& w, int kind, const CellSet& Bc, const CellSet& Fc, int tc, int type) {
    if (cfg.crosscheck && admissible(Bc, Fc) != admissibleRef(Bc, Fc))
      addError(kind == KP ? "P" : "Q", Bc, Fc, tc, "crosscheck: VOID test of a child differs from the literal reference");
    if (!admissible(Bc, Fc)) {
      w.st.voids[type][tc]++;
      return;
    }
    w.st.edges[type][tc]++;
    CanonOut cn = canonState(Bc, Fc);
    if (visited.insert(cn.B, cn.F, tc, kind)) {
      w.st.fresh[type][tc]++;
      Item it;
      it.B = cn.B;
      it.F = cn.F;
      it.t = (uint8_t)tc;
      it.kind = (uint8_t)kind;
      w.local.push_back(it);
    }
  }

  void process(const Item& it, Worker& w) {
    const CellSet& B = it.B;
    const CellSet& F = it.F;
    const int t = it.t;
    const int nB = B.count(), nF = F.count();
    w.st.maxT = std::max(w.st.maxT, t);
    if (cfg.crosscheck) {
      // the queued representative must be the brute-force minimum over all 1152 maps
      w.crossStates++;
      CanonOut cb = canonBrute(B, F);
      if (cb.B != B || cb.F != F) addError(it.kind == KP ? "P" : "Q", B, F, t, "crosscheck: queued state is not the brute-force canonical form");
    }
    if (it.kind == KP) {
      w.st.p[t]++;
      if (nB != t || nF > t || B.intersects(F)) {
        addError("P", B, F, t, "state violates |B| = t, |F| <= t, B and F disjoint");
        return;
      }
      if (!admissible(B, F)) {
        addError("P", B, F, t, "internal error: a VOID state was queued");
        return;
      }
      int dummy = 0;
      if (leaves.find(stateKey(B, F, t, KP), &dummy)) {
        // (P1) listed leaf: LEAF(B,F,t) recomputed from scratch
        if (w.lc->check(B, F, t, nullptr)) {
          w.st.leaves[t]++;
          if (cfg.crosscheck && cfg.leafSample > 0 && stateKey(B, F, t, KP).a % (u64)cfg.leafSample == 0) {
            // independent matching code, no pruning: min over all F-connected triples of mu(T)
            w.crossLeaves++;
            int minMu = 0;
            int code = w.kr->run(B, F, minMu);
            if (code != 0 || t - nF > minMu)
              addError("P", B, F, t, "crosscheck: LEAF holds but the unpruned reference disagrees (code " + std::to_string(code) + ", min mu " +
                                         std::to_string(minMu) + ")");
          }
        } else {
          LeafOut lo;
          w.lc->check(B, F, t, &lo);
          addError("P", B, F, t, "listed leaf is not LEAF (reason " + std::to_string(lo.reason) + ")");
        }
        return;
      }
      // (P2)
      if (t >= cfg.rmax) {
        addError("P", B, F, t, "non-leaf P-state with t >= R_max");
        return;
      }
      LostOut lo;
      if (lostCheck(B, F, t, &lo)) {
        addError("P", B, F, t, std::string("LOST: pattern E") + (lo.pattern == 2 ? "2" : "3") + " fits with " +
                                   std::to_string(lo.stones) + " stones");
        return;
      }
      if (cfg.crosscheck) {
        w.crossExpanded++;
        // LOST was found false: both patterns must also fail in the literal reference (crude pruning only)
        if (lostRef(B, F, t, true, true) & 3) addError("P", B, F, t, "crosscheck: literal LOST reference finds a fitting pattern");
        // children: the same set as the literal generator
        std::set<std::string> fast;
        genChildrenP(B, F, t, [&](int type, const CellSet& Bc, const CellSet& Fc) {
          fast.insert(std::string(type == CH_A ? "Q " : "P ") + std::to_string(t + 1) + " " + setStr(Bc) + " " + setStr(Fc));
        });
        if (fast != childrenRef(B, F, t)) addError("P", B, F, t, "crosscheck: children differ from the literal reference");
      }
      genChildrenP(B, F, t, [&](int type, const CellSet& Bc, const CellSet& Fc) {
        pushChild(w, type == CH_A ? KQ : KP, Bc, Fc, t + 1, type);
      });
    } else {
      w.st.q[t]++;
      if (t < 1 || nB != t - 1 || nF > t || B.intersects(F)) {
        addError("Q", B, F, t, "state violates |B| = t-1, |F| <= t, B and F disjoint");
        return;
      }
      if (!admissible(B, F)) {
        addError("Q", B, F, t, "internal error: a VOID state was queued");
        return;
      }
      int m = -1;
      if (!table.find(stateKey(B, F, t, KQ), &m)) {
        addError("Q", B, F, t, "no table entry (strategy hole)");
        return;
      }
      if (m < 0 || m >= NC || B.test(m) || F.test(m)) {
        addError("Q", B, F, t, "illegal table move " + std::to_string(m));
        return;
      }
      if (!ISBUSY[m]) {
        w.st.nonbusy++;
        if (cfg.busyOnly) {
          addError("Q", B, F, t, "table move " + std::to_string(m) + " is not a busy cell (--busy-only)");
          return;
        }
      }
      // (i) P(B+m, F, t).  It cannot be VOID (admissibility is inherited by supersets of B); if this
      // program finds it VOID something is wrong with it, which is reported as an error.
      CellSet B1 = B;
      B1.set(m);
      if (!admissible(B1, F)) {
        addError("Q", B, F, t, "internal error: child (i) is VOID");
        return;
      }
      pushChild(w, KP, B1, F, t, CH_I);
      // (ii) if |F| < t: Q(B, F+m, t), unless VOID
      if (nF < t) {
        CellSet F1 = F;
        F1.set(m);
        pushChild(w, KQ, B, F1, t, CH_II);
      }
    }
  }

  void workerMain(Worker* wp) {
    Worker& w = *wp;
    const size_t HIGH = 4096;
    for (;;) {
      if (w.local.empty()) {
        std::unique_lock<std::mutex> lk(mu);
        for (;;) {
          if (done) return;
          if (!global.empty()) break;
          idle++;
          idleA = idle;
          if (idle == cfg.threads) {
            done = true;
            cv.notify_all();
            return;
          }
          cv.wait(lk);
          idle--;
          idleA = idle;
        }
        size_t take = std::min<size_t>(global.size(), std::max<size_t>(1, std::min<size_t>(256, global.size() / cfg.threads + 1)));
        for (size_t i = 0; i < take; i++) {
          w.local.push_back(global.back());
          global.pop_back();
        }
        globalSize = global.size();
      }
      Item it = w.local.back();
      w.local.pop_back();
      process(it, w);
      w.expanded.store(w.expanded.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
      if (stop.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lk(mu);
        done = true;
        cv.notify_all();
        return;
      }
      if (cfg.threads > 1 && w.local.size() > 1 &&
          (w.local.size() > HIGH || (globalSize.load(std::memory_order_relaxed) == 0 && idleA.load(std::memory_order_relaxed) > 0))) {
        std::lock_guard<std::mutex> lk(mu);
        size_t give = w.local.size() / 2;
        global.insert(global.end(), w.local.begin(), w.local.begin() + give);
        w.local.erase(w.local.begin(), w.local.begin() + give);
        globalSize = global.size();
        cv.notify_all();
      }
    }
  }

  EngineResult run() {
    EngineResult res;
    double t0 = nowSec();
    visited.exact = cfg.exact;
    CHECK(cfg.threads >= 1 && cfg.threads <= 256);
    CHECK(cfg.rmax >= 0 && cfg.rmax <= 60 && cfg.rootT <= 60);
    std::vector<Worker> workers(cfg.threads);
    for (auto& w : workers) {
      w.lc.reset(new LeafChecker());
      w.lc->paranoid = cfg.paranoid;
      if (cfg.crosscheck) w.kr.reset(new KuhnRef());
    }
    // root
    bool rootBad = false;
    if (cfg.rootB.count() != cfg.rootT || cfg.rootF.count() > cfg.rootT || cfg.rootB.intersects(cfg.rootF)) {
      addError("P", cfg.rootB, cfg.rootF, cfg.rootT, "root violates |B| = t, |F| <= t, B and F disjoint");
      rootBad = true;
    } else if (!admissible(cfg.rootB, cfg.rootF)) {
      res.rootVoid = true;
    } else {
      CanonOut cn = canonState(cfg.rootB, cfg.rootF);
      visited.insert(cn.B, cn.F, cfg.rootT, KP);
      Item it;
      it.B = cn.B;
      it.F = cn.F;
      it.t = (uint8_t)cfg.rootT;
      it.kind = KP;
      global.push_back(it);
    }
    if (!rootBad && !res.rootVoid) {
      std::vector<std::thread> th;
      for (int i = 0; i < cfg.threads; i++) th.emplace_back(&Engine::workerMain, this, &workers[i]);
      {
        std::unique_lock<std::mutex> lk(mu);
        double last = nowSec();
        u64 lastN = 0;
        while (!done) {
          cv.wait_for(lk, std::chrono::milliseconds(200));
          double now = nowSec();
          if (cfg.timeLimit > 0 && now - t0 > cfg.timeLimit && !stop) {
            res.timedOut = true;
            stop = true;
          }
          if (cfg.progressSec > 0 && now - last >= cfg.progressSec) {
            u64 n = 0;
            for (auto& w : workers) n += w.expanded.load(std::memory_order_relaxed);
            fprintf(stderr, "[treecheck] %.0fs: %llu states expanded (%.0f/s), %llu known, shared stack %zu, errors %llu\n",
                    now - t0, (unsigned long long)n, (double)(n - lastN) / (now - last),
                    (unsigned long long)visited.size(), global.size(), (unsigned long long)nerrors.load());
            last = now;
            lastN = n;
          }
        }
      }
      for (auto& x : th) x.join();
    }
    bool leftover = !global.empty();
    for (auto& w : workers) {
      u64 n = 0;
      for (int i = 0; i < 64; i++) n += w.st.p[i] + w.st.q[i];
      res.perThread.push_back(n);
      res.st.add(w.st);
      if (!w.local.empty()) leftover = true;
      res.leafBfs += w.lc->nBfs;
      res.leafTriples += w.lc->nTriple;
      res.crossStates += w.crossStates;
      res.crossExpanded += w.crossExpanded;
      res.crossLeaves += w.crossLeaves;
    }
    for (int i = 0; i < 64; i++) {
      res.pStates += res.st.p[i];
      res.qStates += res.st.q[i];
      res.leavesChecked += res.st.leaves[i];
    }
    res.errorCount = nerrors.load();
    res.errors = errors;
    std::sort(res.errors.begin(), res.errors.end(), [](const ErrRec& x, const ErrRec& y) {
      if (x.state != y.state) return x.state < y.state;
      if (x.kind != y.kind) return x.kind < y.kind;
      return x.msg < y.msg;
    });
    res.complete = !leftover && !stop.load() && !rootBad;
    res.visited = visited.size();
    // every queued state must have been expanded exactly once
    if (res.complete) CHECK(res.visited == res.pStates + res.qStates);
    res.ok = res.complete && res.errorCount == 0 && tableLoad.bad == 0 && leafLoad.bad == 0;
    res.seconds = nowSec() - t0;
    return res;
  }
};

static std::string jsonEscape(const std::string& s) {
  std::string r;
  for (char ch : s) {
    if (ch == '"' || ch == '\\') {
      r += '\\';
      r += ch;
    } else if ((unsigned char)ch < 32) {
      r += ' ';
    } else
      r += ch;
  }
  return r;
}

static std::string resultJson(const Engine& en, const EngineResult& r, double loadSec) {
  std::string s;
  char buf[256];
  auto addU = [&](const char* name, u64 v) {
    snprintf(buf, sizeof buf, "\"%s\":%llu,", name, (unsigned long long)v);
    s += buf;
  };
  auto addB = [&](const char* name, bool v) {
    s += std::string("\"") + name + "\":" + (v ? "true" : "false") + ",";
  };
  s += "{";
  addB("ok", r.ok);
  addU("p_states", r.pStates);
  addU("q_states", r.qStates);
  addU("leaves_checked", r.leavesChecked);
  addU("max_t", (u64)r.st.maxT);
  s += "\"errors\":[";
  for (size_t i = 0; i < r.errors.size(); i++) {
    if (i) s += ",";
    s += "{\"kind\":\"" + r.errors[i].kind + "\",\"state\":\"" + jsonEscape(r.errors[i].state) + "\",\"msg\":\"" +
         jsonEscape(r.errors[i].msg) + "\"}";
  }
  s += "],";
  addU("error_count", r.errorCount);
  addB("complete", r.complete);
  addB("timed_out", r.timedOut);
  addB("root_void", r.rootVoid);
  addU("rmax", (u64)en.cfg.rmax);
  s += "\"root\":\"" + std::to_string(en.cfg.rootT) + " " + setStr(en.cfg.rootB) + " " + setStr(en.cfg.rootF) + "\",";
  addU("threads", (u64)en.cfg.threads);
  addB("exact_visited_set", en.cfg.exact);
  addB("paranoid_flows", en.cfg.paranoid);
  addB("busy_only", en.cfg.busyOnly);
  addB("crosscheck", en.cfg.crosscheck);
  addU("crosscheck_states_canon", r.crossStates);
  addU("crosscheck_expansions", r.crossExpanded);
  addU("crosscheck_leaves", r.crossLeaves);
  addU("states_visited", r.visited);
  snprintf(buf, sizeof buf, "\"table_bytes\":%llu,\"table_fnv1a64\":\"%016llx\",\"leaf_bytes\":%llu,\"leaf_fnv1a64\":\"%016llx\",",
           (unsigned long long)en.tableLoad.bytes, (unsigned long long)en.tableLoad.fnv, (unsigned long long)en.leafLoad.bytes,
           (unsigned long long)en.leafLoad.fnv);
  s += buf;
  addU("table_entries", en.tableLoad.entries);
  addU("table_lines", en.tableLoad.lines);
  addU("table_bad_lines", en.tableLoad.bad);
  addU("table_noncanonical", en.tableLoad.noncanon);
  addU("table_duplicates", en.table.dups);
  addU("table_conflicts", en.table.conflicts);
  addU("leaf_entries", en.leafLoad.entries);
  addU("leaf_lines", en.leafLoad.lines);
  addU("leaf_bad_lines", en.leafLoad.bad);
  addU("leaf_noncanonical", en.leafLoad.noncanon);
  addU("leaf_duplicates", en.leaves.dups);
  s += "\"bad_lines\":[";
  {
    bool first = true;
    for (auto& m : en.tableLoad.badMsgs) {
      if (!first) s += ",";
      first = false;
      s += "\"table: " + jsonEscape(m) + "\"";
    }
    for (auto& m : en.leafLoad.badMsgs) {
      if (!first) s += ",";
      first = false;
      s += "\"leaves: " + jsonEscape(m) + "\"";
    }
  }
  s += "],";
  addU("nonbusy_moves", r.st.nonbusy);
  u64 tot[NCH] = {0, 0, 0, 0, 0}, totV[NCH] = {0, 0, 0, 0, 0}, totN[NCH] = {0, 0, 0, 0, 0};
  for (int k = 0; k < NCH; k++)
    for (int i = 0; i < 64; i++) {
      tot[k] += r.st.edges[k][i];
      totV[k] += r.st.voids[k][i];
      totN[k] += r.st.fresh[k][i];
    }
  addU("threat_children", tot[CH_C]);
  addU("danger_children", tot[CH_D]);
  addU("children_new", totN[CH_A] + totN[CH_C] + totN[CH_D] + totN[CH_I] + totN[CH_II]);
  addU("void_skipped", totV[CH_A] + totV[CH_C] + totV[CH_D] + totV[CH_I] + totV[CH_II]);
  addU("void_skipped_threat", totV[CH_C]);
  addU("void_skipped_danger", totV[CH_D]);
  addU("void_skipped_reveal", totV[CH_II]);
  int mt = r.st.maxT;
  auto arr = [&](const char* name, const u64* a) {
    s += std::string("\"") + name + "\":[";
    for (int i = 0; i <= mt; i++) {
      if (i) s += ",";
      s += std::to_string((unsigned long long)a[i]);
    }
    s += "]";
  };
  s += "\"by_t\":{\"note\":\"index = t of the state itself; children are counted at the t of the child; "
       "threat/danger children are the non-VOID children generated (with repetitions)\",";
  arr("p_states", r.st.p);
  s += ",";
  arr("q_states", r.st.q);
  s += ",";
  arr("leaves", r.st.leaves);
  s += ",";
  arr("threat_children", r.st.edges[CH_C]);
  s += ",";
  arr("danger_children", r.st.edges[CH_D]);
  s += ",";
  u64 vs[64];
  for (int i = 0; i < 64; i++) vs[i] = r.st.voids[CH_A][i] + r.st.voids[CH_C][i] + r.st.voids[CH_D][i] + r.st.voids[CH_I][i] + r.st.voids[CH_II][i];
  arr("void_skipped", vs);
  s += "},";
  s += "\"states_per_thread\":[";
  for (size_t i = 0; i < r.perThread.size(); i++) s += (i ? "," : "") + std::to_string((unsigned long long)r.perThread[i]);
  s += "],";
  addU("leaf_bfs", r.leafBfs);
  addU("leaf_triple_flows", r.leafTriples);
  snprintf(buf, sizeof buf, "\"load_seconds\":%.3f,\"seconds\":%.3f,\"states_per_second\":%.1f}", loadSec, r.seconds,
           r.seconds > 0 ? (double)(r.pStates + r.qStates) / r.seconds : 0.0);
  s += buf;
  return s;
}

// =====================================================================================================
// 12. Test scaffolding: a naive AND-OR solver that produces a table and a leaf file for a small tree.
//     It is NOT part of the verification; it only manufactures inputs for the tests of the engine.
// =====================================================================================================

struct TreeGen {
  int rmax = 0;
  bool allCells = false;
  u64 nodes = 0, nodeLimit = 0;
  bool aborted = false;
  std::unordered_map<std::string, char> memo;
  std::map<std::string, int> table;      // canonical "t B F" -> move
  std::set<std::string> leaves;          // canonical "t B F"
  LeafChecker lc;

  bool solveP(const CellSet& B0, const CellSet& F0, int t, bool forceExpand) {
    CanonOut cn = canonState(B0, F0);
    const CellSet B = cn.B, F = cn.F;
    std::string key = "P " + stateStr(t, B, F);
    if (!forceExpand) {
      auto it = memo.find(key);
      if (it != memo.end()) return it->second != 0;
    }
    if (aborted) return false;
    if (nodeLimit && ++nodes > nodeLimit) {
      aborted = true;
      return false;
    }
    bool res = false;
    if (!forceExpand && lc.check(B, F, t)) {
      leaves.insert(stateStr(t, B, F));
      res = true;
    } else if (t < rmax && !lostCheck(B, F, t, nullptr)) {
      std::vector<Item> ch;
      genChildrenP(B, F, t, [&](int type, const CellSet& Bc, const CellSet& Fc) {
        if (!admissible(Bc, Fc)) return;
        Item it;
        it.B = Bc;
        it.F = Fc;
        it.t = (uint8_t)(t + 1);
        it.kind = (uint8_t)(type == CH_A ? KQ : KP);
        ch.push_back(it);
      });
      res = true;
      // cheap children first: P-children (mostly leaves), the Q-child last
      for (int pass = 0; pass < 2 && res; pass++)
        for (auto& it : ch) {
          if ((it.kind == KP) != (pass == 0)) continue;
          bool r = it.kind == KQ ? solveQ(it.B, it.F, it.t) : solveP(it.B, it.F, it.t, false);
          if (!r) {
            res = false;
            break;
          }
        }
    }
    if (!aborted && !forceExpand) memo[key] = res;
    return res;
  }
  bool solveQ(const CellSet& B0, const CellSet& F0, int t) {
    CanonOut cn = canonState(B0, F0);
    const CellSet B = cn.B, F = cn.F;
    std::string key = "Q " + stateStr(t, B, F);
    auto it = memo.find(key);
    if (it != memo.end()) return it->second != 0;
    if (aborted) return false;
    if (nodeLimit && ++nodes > nodeLimit) {
      aborted = true;
      return false;
    }
    bool res = false;
    for (int pass = 0; pass < (allCells ? 2 : 1) && !res; pass++)
      for (int m = 0; m < NC && !res; m++) {
        if ((ISBUSY[m] != 0) != (pass == 0)) continue;
        if (B.test(m) || F.test(m)) continue;
        CellSet B1 = B;
        B1.set(m);
        if (!solveP(B1, F, t, false)) continue;
        if (F.count() < t) {
          CellSet F1 = F;
          F1.set(m);
          if (admissible(B, F1) && !solveQ(B, F1, t)) continue;
        }
        if (aborted) break;
        table[stateStr(t, B, F)] = m;
        res = true;
      }
    if (!aborted) memo[key] = res;
    return res;
  }
  std::string tableText() const {
    std::string s;
    for (auto& kv : table) s += kv.first + " " + std::to_string(kv.second) + "\n";
    return s;
  }
  std::string leavesText() const {
    std::string s;
    for (auto& k : leaves) s += k + "\n";
    return s;
  }
};

// =====================================================================================================
// 13. --selftest
// =====================================================================================================

static int g_fail = 0;
// test sizes for the three levels: 0 = --quick, 1 = default, 2 = --full
static int pick(int level, int a, int b, int c) { return level <= 0 ? a : (level == 1 ? b : c); }
static void expect(bool cond, const std::string& what) {
  if (!cond) {
    g_fail++;
    printf("  [FAIL] %s\n", what.c_str());
  }
}
static double g_testStart = 0;
static void report(const char* name, int failBefore, const std::string& info) {
  double now = nowSec();
  printf("%s %-28s (%.1f s) %s\n", g_fail == failBefore ? "[ok]  " : "[FAIL]", name, now - g_testStart, info.c_str());
  g_testStart = now;
  fflush(stdout);
}

static CellSet cellsOf(std::initializer_list<int> l) {
  CellSet s;
  for (int c : l) s.set(c);
  return s;
}

static EngineResult runEngineOnText(const std::string& tableText, const std::string& leavesText, const EngineConfig& cfg,
                                    Engine* keep = nullptr) {
  std::unique_ptr<Engine> own;
  Engine* en = keep;
  if (!en) {
    own.reset(new Engine());
    en = own.get();
  }
  en->cfg = cfg;
  FILE* f = fmemopen((void*)tableText.data(), tableText.size() ? tableText.size() : 1, "r");
  CHECK(f != nullptr);
  if (tableText.empty()) fseek(f, 0, SEEK_END);
  loadKeys(f, true, cfg.threads, en->table, en->tableLoad);
  fclose(f);
  f = fmemopen((void*)leavesText.data(), leavesText.size() ? leavesText.size() : 1, "r");
  CHECK(f != nullptr);
  if (leavesText.empty()) fseek(f, 0, SEEK_END);
  loadKeys(f, false, cfg.threads, en->leaves, en->leafLoad);
  fclose(f);
  return en->run();
}

static std::vector<std::string> splitLines(const std::string& s) {
  std::vector<std::string> r;
  size_t p = 0;
  while (p < s.size()) {
    size_t q = s.find('\n', p);
    if (q == std::string::npos) q = s.size();
    if (q > p) r.push_back(s.substr(p, q - p));
    p = q + 1;
  }
  return r;
}
static std::string joinLines(const std::vector<std::string>& v) {
  std::string s;
  for (auto& l : v) s += l + "\n";
  return s;
}

// regression sets of SPEC section 8
static CellSet setT7() { return cellsOf({cid(0, 0, 0), cid(5, 5, 5), cid(1, 1, 4), cid(4, 4, 1), cid(2, 3, 2), cid(3, 2, 3), cid(5, 0, 0)}); }
static CellSet setT8() {
  CellSet s = setT7();
  s.set(cid(0, 5, 5));
  return s;
}

static void testGeometry() {
  int f0 = g_fail;
  // closure of the group under composition and inverse
  std::set<std::array<uint8_t, NC>> maps;
  for (int g = 0; g < NG; g++) {
    std::array<uint8_t, NC> a;
    memcpy(a.data(), PERM[g], NC);
    maps.insert(a);
  }
  expect(maps.size() == 1152, "1152 distinct maps");
  long bad = 0;
  for (int g = 0; g < NG; g++) {
    std::array<uint8_t, NC> inv;
    for (int c = 0; c < NC; c++) inv[PERM[g][c]] = (uint8_t)c;
    if (!maps.count(inv)) bad++;
  }
  expect(bad == 0, "group closed under inverse");
  bad = 0;
  for (int g = 0; g < NG; g++)
    for (int h = 0; h < NG; h++) {
      std::array<uint8_t, NC> comp;
      for (int c = 0; c < NC; c++) comp[c] = PERM[g][PERM[h][c]];
      if (!maps.count(comp)) bad++;
    }
  expect(bad == 0, "group closed under composition");
  // orbits of cells: 24 + 144 + 48
  std::map<int, int> orb;
  for (int c = 0; c < NC; c++) orb[ORBREP[c]]++;
  expect(orb.size() == 3 && orb[0] == 24 && orb[1] == 144 && orb[8] == 48, "cell orbits 24/144/48 with representatives 0,1,8");
  bool busyOrbit = true;
  for (int c = 0; c < NC; c++)
    if ((ORBREP[c] == 0) != (ISBUSY[c] != 0)) busyOrbit = false;
  expect(busyOrbit, "busy cells = orbit of cell 0");
  // antipode is a group element
  {
    std::array<uint8_t, NC> anti;
    for (int c = 0; c < NC; c++) anti[c] = (uint8_t)(215 - c);
    expect(maps.count(anti) == 1, "antipode map belongs to the group");
  }
  // set order of SPEC 2.4
  expect(cmpSet(cellsOf({0, 5}), cellsOf({1, 2})) < 0, "{0,5} < {1,2}");
  expect(cmpSet(cellsOf({0, 1}), cellsOf({0})) < 0, "{0,1} < {0}");
  expect(cmpSet(cellsOf({3}), CellSet()) < 0, "{3} < {}");
  expect(cmpSet(cellsOf({7, 100}), cellsOf({7, 99})) > 0, "{7,100} > {7,99}");
  expect(cmpSet(cellsOf({64, 200}), cellsOf({64, 200})) == 0, "equal sets");
  // slack values of the regression sets (SPEC 8.2, 8.3)
  auto slack = [&](const CellSet& B) {
    int nl = 0;
    for (int l = 0; l < NL; l++)
      if (!LSET[l].intersects(B)) nl++;
    return 216 - B.count() - 2 * nl;
  };
  expect(slack(setT7()) == 1, "slack(T7) = 1");
  expect(slack(setT8()) == 8, "slack(T8) = 8");
  report("geometry", f0, "148 lines = 108+36+4, 24 busy cells on 7 lines, 1152 line-preserving maps closed under composition/inverse");
}

static CellSet randomSubset(Rng& rng, const std::vector<int>& pool, int k) {
  std::vector<int> p = pool;
  CellSet s;
  for (int i = 0; i < k && !p.empty(); i++) {
    int j = rng.below((int)p.size());
    s.set(p[j]);
    p[j] = p.back();
    p.pop_back();
  }
  return s;
}
static std::vector<int> allCells() {
  std::vector<int> v;
  for (int c = 0; c < NC; c++) v.push_back(c);
  return v;
}
static std::vector<int> busyCells() { return std::vector<int>(BUSY, BUSY + 24); }
static std::vector<int> cellsNotIn(const CellSet& s) {
  std::vector<int> v;
  for (int c = 0; c < NC; c++)
    if (!s.test(c)) v.push_back(c);
  return v;
}

static void testCanon(int level) {
  int f0 = g_fail;
  Rng rng(12345);
  int n = pick(level, 600, 1500, 3000);
  long mism = 0, invar = 0, notImage = 0;
  for (int it = 0; it < n; it++) {
    int nb, nf;
    int mode = it % 6;
    CellSet B, F;
    if (mode == 0) {
      nb = rng.below(13);
      nf = rng.below(10);
      B = randomSubset(rng, busyCells(), nb);
      F = randomSubset(rng, cellsNotIn(B), nf);
    } else if (mode == 1) {
      nb = 0;
      nf = rng.below(6);
      F = randomSubset(rng, allCells(), nf);
    } else if (mode == 2) {
      nb = rng.below(4);
      B = randomSubset(rng, allCells(), nb);
    } else if (mode == 3) {
      // symmetric configurations: orbits of a random subgroup element
      int g = rng.below(NG);
      int c = rng.below(NC);
      for (int i = 0; i < 12; i++) {
        B.set(c);
        c = PERM[g][c];
      }
      int d = rng.below(NC);
      for (int i = 0; i < 12; i++) {
        if (!B.test(d)) F.set(d);
        d = PERM[g][d];
      }
    } else {
      nb = rng.below(20);
      nf = rng.below(20);
      B = randomSubset(rng, allCells(), nb);
      F = randomSubset(rng, cellsNotIn(B), nf);
    }
    CanonOut a = canonState(B, F);
    CanonOut b = canonBrute(B, F);
    if (a.B != b.B || a.F != b.F) mism++;
    if (imageSet(a.g, B) != a.B || imageSet(a.g, F) != a.F) notImage++;
    int g = rng.below(NG);
    CanonOut c = canonState(imageSet(g, B), imageSet(g, F));
    if (c.B != a.B || c.F != a.F) invar++;
  }
  expect(mism == 0, "canonState == brute-force minimum over the 1152 maps");
  expect(notImage == 0, "canonical form is the image under the returned map");
  expect(invar == 0, "canonical form invariant under random group elements");
  // hash sanity: different (kind, t) give different keys; equal states give equal keys
  {
    CellSet B = setT8(), F;
    K128 k1 = stateKey(B, F, 8, KP), k2 = stateKey(B, F, 8, KQ), k3 = stateKey(B, F, 9, KP), k4 = stateKey(B, F, 8, KP);
    expect(!(k1.a == k2.a && k1.b == k2.b) && !(k1.a == k3.a && k1.b == k3.b) && k1.a == k4.a && k1.b == k4.b, "state keys");
    std::set<std::pair<u64, u64>> keys;
    Rng r2(99);
    int nk = 20000;
    std::set<std::string> strs;
    for (int i = 0; i < nk; i++) {
      CellSet b = randomSubset(r2, allCells(), 1 + r2.below(3));
      CellSet f = randomSubset(r2, cellsNotIn(b), r2.below(3));
      strs.insert(setStr(b) + "|" + setStr(f));
      K128 k = stateKey(b, f, 5, KP);
      keys.insert({k.a, k.b});
    }
    expect(keys.size() == strs.size(), "no hash collisions among 20000 small random states");
  }
  // visited set, both modes
  for (int ex = 0; ex < 2; ex++) {
    Visited v;
    v.exact = ex;
    Rng r3(7);
    std::set<std::string> ref;
    long diff = 0;
    for (int i = 0; i < 30000; i++) {
      CellSet b = randomSubset(r3, busyCells(), r3.below(3));
      CellSet f = randomSubset(r3, cellsNotIn(b), r3.below(2));
      int t = 3 + r3.below(2), kind = r3.below(2);
      bool ins = v.insert(b, f, t, kind);
      bool insRef = ref.insert(std::to_string(kind) + stateStr(t, b, f)).second;
      if (ins != insRef) diff++;
    }
    expect(diff == 0 && v.size() == ref.size(), std::string("visited set agrees with std::set, exact=") + (ex ? "1" : "0"));
  }
  report("canonical form / hashing", f0, std::to_string(n) + " random states vs brute force, invariance, visited-set model check");
}

static void testAdmissible(int level) {
  int f0 = g_fail;
  Rng rng(777);
  int n = pick(level, 2000, 5000, 10000);
  long mism = 0, nAdm = 0;
  for (int it = 0; it < n; it++) {
    CellSet B = randomSubset(rng, allCells(), rng.below(12));
    CellSet X;
    int mode = it % 4;
    if (mode == 0) {
      X = randomSubset(rng, cellsNotIn(B), rng.below(30));
    } else {
      // stones concentrated on a few lines, to create caps and danger pairs
      int nl = 1 + rng.below(3);
      for (int i = 0; i < nl; i++) {
        int l = rng.below(NL);
        int k = 3 + rng.below(3);
        std::vector<int> cells(LINE[l], LINE[l] + 6);
        CellSet s = randomSubset(rng, cells, k);
        X = X | s;
      }
      if (mode == 1) X = X.minus(B);
    }
    bool a = admissible(B, X), b = admissibleRef(B, X);
    if (a != b) mism++;
    if (a) nAdm++;
  }
  expect(mism == 0, "admissible == literal reference");
  // hand-made: cap and danger pair
  {
    CellSet B;
    CellSet X = cellsOf({cid(0, 0, 0), cid(1, 0, 0), cid(2, 0, 0), cid(3, 0, 0)});   // 4 on a row
    expect(admissible(B, X), "4 stones on one line are admissible");
    X.set(cid(4, 0, 0));
    expect(!admissible(B, X), "5 stones on a live line are not admissible");
    expect(admissible(cellsOf({cid(5, 0, 0)}), X), "5 stones on a dead line are admissible");
    // danger pair: row y=0,z=0 without (5,0,0),(4,0,0); column x=5,z=0: (5,1,0)..(5,4,0); common cell (5,0,0)
    CellSet Y = cellsOf({cid(0, 0, 0), cid(1, 0, 0), cid(2, 0, 0), cid(3, 0, 0), cid(5, 1, 0), cid(5, 2, 0), cid(5, 3, 0), cid(5, 4, 0)});
    expect(!admissible(B, Y), "danger pair through an empty common cell");
    CellSet Y2 = Y;
    Y2.reset(cid(3, 0, 0));
    Y2.set(cid(4, 0, 0));
    expect(!admissible(B, Y2), "danger pair (other 4-subset)");
    expect(admissible(cellsOf({cid(5, 0, 0)}), Y), "no danger pair once the common cell is taken by SP");
    expect(admissible(cellsOf({cid(5, 5, 0)}), Y), "no danger pair once one of the lines is dead");
  }
  report("admissibility", f0, std::to_string(n) + " random (B,X) vs literal reference (" + std::to_string(nAdm) + " admissible), hand-made cases");
}

static void testLeafRegression() {
  int f0 = g_fail;
  LeafChecker lc;
  lc.paranoid = true;
  CellSet T7 = setT7(), T8 = setT8(), E;
  LeafOut lo;
  std::string info;
  expect(!lc.check(T7, E, 7, &lo), "LEAF(T7,{},7) is false");
  info += "LEAF(T7,-,7)=F(reason " + std::to_string(lo.reason) + ")";
  expect(lc.check(T8, E, 8, &lo), "LEAF(T8,{},8) is true");
  info += " LEAF(T8,-,8)=T";
  expect(!lc.check(T8, E, 9, &lo), "LEAF(T8,{},9) is false");
  info += " LEAF(T8,-,9)=F(reason " + std::to_string(lo.reason) + ")";
  int nh = 0, nTrue = 0;
  for (int i = 0; i < 24; i++) {
    int h = BUSY[i];
    if (T8.test(h)) continue;
    nh++;
    if (lc.check(T8, cellsOf({h}), 8)) nTrue++;
  }
  expect(nh == 16 && nTrue == 16, "LEAF(T8,{h},8) true for the 16 busy h outside T8");
  info += " LEAF(T8,{h},8)=T for " + std::to_string(nTrue) + "/" + std::to_string(nh);
  {
    CellSet B = T8;
    B.reset(cid(5, 5, 5));
    B.set(cid(1, 1, 1));
    bool v = lc.check(B, cellsOf({cid(5, 5, 5)}), 8);
    expect(v, "item 5: LEAF(T8-(5,5,5)+(1,1,1), {(5,5,5)}, 8) is true");
    info += std::string(" item5=") + (v ? "T" : "F");
  }
  {
    // item 6 (antipode stealing); only the first value is forced (worst = 6 > 0 implies LEAF false)
    int order[10][3] = {{0, 0, 0}, {1, 1, 4}, {2, 3, 2}, {0, 5, 5}, {1, 1, 1}, {2, 2, 2}, {0, 0, 5}, {1, 4, 1}, {2, 2, 3}, {0, 5, 0}};
    info += " item6:";
    for (int rounds = 8; rounds <= 10; rounds++) {
      CellSet B, F;
      for (int i = 0; i < rounds; i++) B.set(cid(order[i][0], order[i][1], order[i][2]));
      for (int i = 0; i < rounds - 1; i++) F.set(cid(5 - order[i][0], 5 - order[i][1], 5 - order[i][2]));
      bool v = lc.check(B, F, rounds, &lo);
      if (rounds == 8) expect(!v, "item 6 after 8 rounds: LEAF false (worst = 6)");
      info += std::string(" r") + std::to_string(rounds) + "=" + (v ? "T" : "F(reason " + std::to_string(lo.reason) + ")");
    }
  }
  {
    CellSet F;
    for (int x = 1; x <= 5; x++) F.set(cid(x, 1, 2));
    CellSet B8 = T8;
    B8.set(cid(0, 1, 2));
    CellSet B7 = T7;
    B7.set(cid(0, 1, 2));
    bool v1 = lc.check(B8, F, 9), v2 = lc.check(B7, F, 8, &lo);
    expect(v1, "item 7: LEAF(T8+(0,1,2), {(x,1,2)}, 9) is true");
    expect(!v2, "item 7: LEAF(T7+(0,1,2), {(x,1,2)}, 8) is false");
    info += std::string(" item7=") + (v1 ? "T" : "F") + "/" + (v2 ? "T" : "F(reason " + std::to_string(lo.reason) + ")");
  }
  // degenerate inputs
  expect(!lc.check(cellsOf({0}), cellsOf({0}), 5, &lo) && lo.reason == 1, "F meeting B is inadmissible");
  expect(!lc.check(E, E, 0, &lo) && lo.reason == 2, "empty board has no pairing");
  report("LEAF regression (SPEC 8)", f0, info);
}

// instance generator for the tiny exhaustive LEAF tests
static bool tinyInstance(Rng& rng, int mode, CellSet& B, CellSet& F, int& k, int maxLive) {
  CellSet N;
  if (mode == 0) {
    // a full plane (14 lines in 36 cells)
    int axis = rng.below(3), v = rng.below(6);
    for (int c = 0; c < NC; c++) {
      int co[3] = {c / 36, (c / 6) % 6, c % 6};
      if (co[axis] == v) N.set(c);
    }
  } else {
    // a random F-connected cluster of lines
    int target = 3 + rng.below(9);
    std::vector<int> cl;
    cl.push_back(rng.below(NL));
    N = LSET[cl[0]];
    int guard = 0;
    while ((int)cl.size() < target && guard++ < 1000) {
      int l = rng.below(NL);
      bool in = false;
      for (int q : cl)
        if (q == l) in = true;
      if (in || !LSET[l].intersects(N)) continue;
      cl.push_back(l);
      N = N | LSET[l];
    }
  }
  B = CellSet();
  for (int c = 0; c < NC; c++)
    if (!N.test(c)) B.set(c);
  // sometimes a few SP stones inside
  int extra = rng.below(3) == 0 ? 1 + rng.below(2) : 0;
  std::vector<int> inside;
  for (int c = 0; c < NC; c++)
    if (N.test(c)) inside.push_back(c);
  B = B | randomSubset(rng, inside, extra);
  int nl = 0;
  for (int l = 0; l < NL; l++)
    if (!LSET[l].intersects(B)) nl++;
  if (nl > maxLive || nl < 1) return false;
  std::vector<int> pool = cellsNotIn(B);
  int nf = rng.below(3) == 0 ? rng.below(22) : rng.below(7);
  if (rng.below(4) == 0) nf = std::max(0, (int)pool.size() - 2 * nl + rng.below(4) - 1);   // around the Hall threshold of all lines
  F = randomSubset(rng, pool, nf);
  if (rng.below(6) == 0) {
    // 4 or 5 stones on one or two live lines: caps and danger pairs
    for (int rep = 1 + rng.below(2); rep > 0; rep--) {
      int l = rng.below(NL);
      if (LSET[l].intersects(B)) continue;
      std::vector<int> cells(LINE[l], LINE[l] + 6);
      F = F | randomSubset(rng, cells, 4 + rng.below(2));
    }
  }
  if (rng.below(40) == 0 && !inside.empty()) F.set(inside[rng.below((int)inside.size())]);   // may hit B: reason 1
  k = F.count() + rng.below(9) - (rng.below(10) == 0 ? 1 : 0);
  if (k < 0) k = 0;
  return true;
}

// checks one LeafChecker verdict (and its certificate) against the expected reason code
static bool leafAgrees(LeafChecker& lc, const CellSet& B, const CellSet& F, int k, int expected, long& certBad) {
  LeafOut lo;
  bool v = lc.check(B, F, k, &lo);
  bool v2 = lc.check(B, F, k, nullptr);
  int theta = k - F.count();
  if (lo.reason == 2 || lo.reason == 3) {
    bool okc = !lo.S.empty();
    std::set<int> ds(lo.S.begin(), lo.S.end());
    if (ds.size() != lo.S.size()) okc = false;
    for (int l : lo.S)
      if (LSET[l].intersects(B)) okc = false;
    if (lo.reason == 2 && !(surplusOf(lo.S, F) < 0)) okc = false;
    if (lo.reason == 3 && !(lo.S.size() >= 3 && isFConnected(lo.S, F) && surplusOf(lo.S, F) < theta && surplusOf(lo.S, F) == lo.sur)) okc = false;
    if (!okc) certBad++;
  }
  bool ok = (v == (expected == 0)) && lo.reason == expected && v2 == v && lo.holds == v;
  if (!ok) printf("  mismatch: k=%d B=%s F=%s expected=%d mine=%d\n", k, setStr(B).c_str(), setStr(F).c_str(), expected, lo.reason);
  return ok;
}

static void testLeafBrute(int level) {
  int f0 = g_fail;
  Rng rng(2024);
  LeafChecker lc;
  lc.paranoid = true;
  int n = pick(level, 1500, 3000, 6000);
  long inst = 0, evals = 0, mism = 0, certBad = 0, byReason[4] = {0, 0, 0, 0}, surHist[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  int maxLive = 16;
  for (int it = 0; it < n; it++) {
    CellSet B, F;
    int k0;
    if (!tinyInstance(rng, it % 3 == 0 ? 0 : 1, B, F, k0, maxLive)) continue;
    int minSur;
    int code = leafBrute(B, F, maxLive, minSur);
    if (code < 0) continue;
    inst++;
    int nF = F.count();
    if (code == 0) surHist[std::min(minSur, 7)]++;
    // every k from |F|-1 up to one above the exact threshold (capped)
    int kHi = code ? nF + 3 : nF + std::min(minSur, 12) + 1;
    for (int k = std::max(0, nF - 1); k <= kHi; k++) {
      int expected = code ? code : (k - nF <= minSur ? 0 : 3);
      evals++;
      byReason[expected]++;
      if (!leafAgrees(lc, B, F, k, expected, certBad)) mism++;
    }
  }
  expect(mism == 0, "LEAF == exhaustive enumeration of all subsets of live lines");
  expect(certBad == 0, "certificates of reasons 2 and 3 verified from the definitions");
  expect(byReason[0] >= 100 && byReason[1] >= 100 && byReason[2] >= 100 && byReason[3] >= 100, "all four outcomes occur often");
  char buf[320];
  snprintf(buf, sizeof buf,
           "%ld instances with <= %d live lines, every k up to the exact threshold + 1: %ld evaluations, reasons 0/1/2/3 = %ld/%ld/%ld/%ld; "
           "min surplus 0/1/2/3/4/5/6/>=7: %ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld",
           inst, maxLive, evals, byReason[0], byReason[1], byReason[2], byReason[3], surHist[0], surHist[1], surHist[2], surHist[3], surHist[4],
           surHist[5], surHist[6], surHist[7]);
  report("LEAF vs exhaustive subsets", f0, buf);
}

static void testLeafFlowRef(int level) {
  int f0 = g_fail;
  Rng rng(4711);
  LeafChecker lc;
  lc.paranoid = true;
  KuhnRef kr;
  int n = pick(level, 40, 80, 160);
  long mism = 0, evals = 0, byReason[4] = {0, 0, 0, 0}, certBad = 0, muHist[12] = {0}, withTriples = 0, withSingles = 0;
  CellSet T8 = setT8();
  for (int it = 0; it < n; it++) {
    CellSet B, F;
    int mode = it % 4;
    if (mode == 0) {
      B = randomSubset(rng, allCells(), 22 + rng.below(30));
      F = randomSubset(rng, cellsNotIn(B), rng.below(9));
    } else if (mode == 1) {
      // tight positions around T8: total slack 8 + 2*(lines killed) - |extra| - |F|
      B = T8 | randomSubset(rng, cellsNotIn(T8), rng.below(3));
      F = randomSubset(rng, cellsNotIn(B), 3 + rng.below(6) + 4 * (B.count() - 8));
    } else if (mode == 2) {
      B = randomSubset(rng, busyCells(), 9 + rng.below(8));
      B = B | randomSubset(rng, cellsNotIn(B), rng.below(8));
      F = randomSubset(rng, cellsNotIn(B), rng.below(9));
    } else {
      B = T8;
      B.reset(BUSY[rng.below(24)]);
      B = B | randomSubset(rng, cellsNotIn(B), 2 + rng.below(3));
      F = randomSubset(rng, cellsNotIn(B), 1 + rng.below(6));
    }
    int minMu;
    int code = kr.run(B, F, minMu);
    int nF = F.count();
    if (code == 0) muHist[std::min(minMu, 11)]++;
    std::vector<int> ks;
    if (code) {
      ks.push_back(nF);
      ks.push_back(nF + 2);
    } else {
      // at the exact threshold, one above, and a few values below
      int top = std::min(minMu, 14);
      ks.push_back(nF + top);
      ks.push_back(nF + top + 1);
      for (int th = 1; th < top; th += (top > 6 ? 2 : 1)) ks.push_back(nF + th);
    }
    for (int k : ks) {
      int expected = code ? code : (k - nF <= minMu ? 0 : 3);
      u64 t0 = lc.nTriple, s0 = lc.nSingle;
      evals++;
      byReason[expected]++;
      if (!leafAgrees(lc, B, F, k, expected, certBad)) mism++;
      if (expected == 0 && lc.nTriple > t0) withTriples++;
      if (expected == 0 && lc.nSingle > s0) withSingles++;
    }
  }
  expect(mism == 0, "LEAF == unpruned triple enumeration with an independent matching (Kuhn)");
  expect(certBad == 0, "certificates verified");
  expect(byReason[0] >= 20 && byReason[3] >= 20, "true and low-surplus outcomes both occur");
  expect(withTriples >= 5 && withSingles >= 5, "true verdicts that needed single-line flows / triple flows occur");
  char buf[400];
  snprintf(buf, sizeof buf,
           "%d mid-size instances, k at the exact threshold, above and below: %ld evaluations, reasons 0/1/2/3 = %ld/%ld/%ld/%ld; "
           "threshold value 0..10,>=11: %ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld; true verdicts using single flows %ld, triple flows %ld",
           n, evals, byReason[0], byReason[1], byReason[2], byReason[3], muHist[0], muHist[1], muHist[2], muHist[3], muHist[4], muHist[5], muHist[6],
           muHist[7], muHist[8], muHist[9], muHist[10], muHist[11], withSingles, withTriples);
  report("LEAF vs unpruned flows", f0, buf);
}

static void testLost(int level) {
  int f0 = g_fail;
  CellSet E;
  LostOut lo;
  // --- hand-built E2: l = row (i,0,0), a = column (0,j,0), b = diagonal (i,5-i,0)
  expect(!lostCheck(E, E, 9, &lo), "LOST(-,-,9) false (a pattern needs >= 11 stones)");
  expect(lostCheck(E, E, 10, &lo) && lo.pattern == 2 && lo.stones == 11, "LOST(-,-,10) true by E2 with 11 stones");
  CellSet lrow, acol, bdia;
  for (int i = 0; i < 6; i++) {
    lrow.set(cid(i, 0, 0));
    acol.set(cid(0, i, 0));
    bdia.set(cid(i, 5 - i, 0));
  }
  CellSet N3 = lrow | acol | bdia, Bout;
  for (int c = 0; c < NC; c++)
    if (!N3.test(c)) Bout.set(c);
  {
    int nl = 0;
    for (int l = 0; l < NL; l++)
      if (!LSET[l].intersects(Bout)) nl++;
    expect(nl == 3, "exactly three live lines in the triangle position");
  }
  expect(lostCheck(Bout, E, 10, &lo) && lo.pattern == 2 && lo.stones == 11, "triangle: E2 fits at t = 10");
  expect(!lostCheck(Bout, E, 9, &lo), "triangle: near miss at t = 9");
  {
    // F = the three corners: the common cell c of any two of the lines is in F -> no pattern at all
    CellSet Fc = cellsOf({cid(0, 0, 0), cid(5, 0, 0), cid(0, 5, 0)});
    expect(!lostCheck(Bout, Fc, 30, &lo), "triangle with the three corners revealed: nothing fits even with t = 30");
    // F = 4 stones of the row + 3 of the column: 4 more stones needed
    CellSet F7 = cellsOf({cid(0, 0, 0), cid(2, 0, 0), cid(3, 0, 0), cid(4, 0, 0), cid(0, 1, 0), cid(0, 2, 0), cid(0, 3, 0)});
    // pattern: Sl = row \ {e=(1,0,0)} needs (5,0,0); Sa = column \ {c=(0,5,0)} \ {(0,4,0)}: complete;
    // Sb = diagonal \ {c} \ {one}: (5,0,0) + 3 more  -> 7 + 1 + 3 = 11
    expect(lostCheck(Bout, F7, 10, &lo) && lo.stones == 11, "triangle with 7 revealed stones: fits at t = 10");
    expect(!lostCheck(Bout, F7, 9, &lo), "triangle with 7 revealed stones: near miss at t = 9");
    // kill the diagonal: only two live lines
    CellSet B2 = Bout;
    B2.set(cid(2, 3, 0));
    expect(!lostCheck(B2, E, 40, &lo), "two live lines: nothing fits");
  }
  // --- hand-built E3: a = row (i,0,0), a' = column (0,j,0), x = origin, b = diagonal through c = (5,0,0),
  //     b' = row y=2 through c' = (0,2,0); 13 stones
  CellSet brow2;
  for (int i = 0; i < 6; i++) brow2.set(cid(i, 2, 0));
  CellSet N4 = lrow | acol | bdia | brow2, B4;
  for (int c = 0; c < NC; c++)
    if (!N4.test(c)) B4.set(c);
  {
    int nl = 0;
    for (int l = 0; l < NL; l++)
      if (!LSET[l].intersects(B4)) nl++;
    expect(nl == 4, "exactly four live lines in the E3 position");
    LostCtx x;
    // 12 stones of an E3 pattern revealed: Sa = row\{(5,0,0),(4,0,0)}, Sa' = column\{(0,2,0),(0,1,0)},
    // Sb = diagonal\{(5,0,0),(1,4,0)}, Sb' = row2\{(0,2,0),(1,2,0)}  (13 cells; reveal all but (2,2,0))
    CellSet Sa = lrow, Sa2 = acol, Sb = bdia, Sb2 = brow2;
    Sa.reset(cid(5, 0, 0));
    Sa.reset(cid(4, 0, 0));
    Sa2.reset(cid(0, 2, 0));
    Sa2.reset(cid(0, 1, 0));
    Sb.reset(cid(5, 0, 0));
    Sb.reset(cid(1, 4, 0));
    Sb2.reset(cid(0, 2, 0));
    Sb2.reset(cid(1, 2, 0));
    CellSet all = Sa | Sa2 | Sb | Sb2;
    expect(all.count() == 13, "the E3 pattern has 13 stones");
    // all 13 stones revealed, t = 12: E3 fits with no new stone; E2 would need a line with 5 stones,
    // i.e. a 14th stone.  (LOST is a function of (B,F,t); |F| <= t is not needed for this unit test.)
    lostPrepare(B4, all, 12, x);
    bool e2 = lostE2(all, x, nullptr), e3 = lostE3(all, x, &lo);
    expect(!e2 && e3 && lo.pattern == 3 && lo.stones == 13, "E3 position: E3 fits at t = 12, E2 does not");
    lostPrepare(B4, all, 11, x);
    expect(!lostE2(all, x, nullptr) && !lostE3(all, x, nullptr), "E3 position: near miss at t = 11");
    int r = lostRef(B4, all, 12, true, false);
    expect(r == 2, "E3 position: literal reference agrees (E3 only)");
    lostPrepare(B4, all, 13, x);
    expect(lostE2(all, x, nullptr), "E3 position: one more stone makes E2 fit as well");
    // 12 of the 13 stones revealed (not (2,2,0)), t = 12: now E2 fits (column + (0,2,0) is a threat, row and
    // diagonal are a danger pair at (5,0,0))
    CellSet F12 = all;
    F12.reset(cid(2, 2, 0));
    lostPrepare(B4, F12, 12, x);
    expect(lostE2(F12, x, &lo) && lo.stones == 13 && lostRef(B4, F12, 12, true, false) == 3, "E3 position minus one stone: E2 and E3 fit at t = 12");
    // c' = (0,2,0) revealed: no danger pair can use it as common cell any more
    CellSet Fx = all;
    Fx.set(cid(0, 2, 0));
    lostPrepare(B4, Fx, 13, x);
    bool e3x = lostE3(Fx, x, nullptr), e2x = lostE2(Fx, x, nullptr);
    int rx = lostRef(B4, Fx, 13, true, false);
    expect(e3x == ((rx & 2) != 0) && e2x == ((rx & 1) != 0), "E3 position with c' revealed: agrees with the reference");
  }
  // --- random comparison with the literal reference
  Rng rng(31337);
  int n = pick(level, 120, 180, 360);
  long mism = 0, nE2 = 0, nE3 = 0, nNone = 0, nE3only = 0, e3checked = 0;
  for (int it = 0; it < n; it++) {
    CellSet B, F;
    int t;
    int mode = it % 6;
    if (mode == 5) {
      // the stones of a random E3 configuration, exactly enough budget for it
      bool built = false;
      for (int guard = 0; guard < 200 && !built; guard++) {
        int a = rng.below(NL), cx = LINE[a][rng.below(6)];
        int a2 = LC[cx][rng.below(NLC[cx])];
        if (a2 == a) continue;
        int c = LINE[a][rng.below(6)], c2 = LINE[a2][rng.below(6)];
        if (c == cx || c2 == cx) continue;
        int b = LC[c][rng.below(NLC[c])], b2 = LC[c2][rng.below(NLC[c2])];
        if (b == a || b == a2 || b2 == a || b2 == a2 || b == b2) continue;
        auto pick = [&](int line, int without, int keep) {
          CellSet S = LSET[line];
          S.reset(without);
          for (;;) {
            int y = LINE[line][rng.below(6)];
            if (y == without || y == keep) continue;
            S.reset(y);
            return S;
          }
        };
        CellSet Sa = pick(a, c, cx), Sa2 = pick(a2, c2, cx), Sb = pick(b, c, -1), Sb2 = pick(b2, c2, -1);
        F = Sa | Sa2 | Sb | Sb2;
        if (F.test(c) || F.test(c2)) continue;
        CellSet N = LSET[a] | LSET[a2] | LSET[b] | LSET[b2];
        B = randomSubset(rng, cellsNotIn(N), 60 + rng.below(60));
        t = F.count() - 1 + (rng.below(4) == 0 ? rng.below(3) - 1 : 0);
        if (rng.below(3) == 0) {
          uint8_t fc[NC];
          int nf = F.list(fc);
          F.reset(fc[rng.below(nf)]);       // one stone still unrevealed
        }
        built = true;
      }
      if (!built) continue;
    } else if (mode == 0) {
      // few live lines: a cluster
      CellSet N;
      int target = 4 + rng.below(8);
      std::vector<int> cl;
      cl.push_back(rng.below(NL));
      N = LSET[cl[0]];
      int guard = 0;
      while ((int)cl.size() < target && guard++ < 1000) {
        int l = rng.below(NL);
        if (!LSET[l].intersects(N)) continue;
        cl.push_back(l);
        N = N | LSET[l];
      }
      for (int c = 0; c < NC; c++)
        if (!N.test(c)) B.set(c);
      F = randomSubset(rng, cellsNotIn(B), rng.below(14));
      t = F.count() + rng.below(12);
    } else if (mode == 1) {
      // a plane
      int axis = rng.below(3), v = rng.below(6);
      for (int c = 0; c < NC; c++) {
        int co[3] = {c / 36, (c / 6) % 6, c % 6};
        if (co[axis] != v) B.set(c);
      }
      F = randomSubset(rng, cellsNotIn(B), rng.below(12));
      t = F.count() + rng.below(13);
    } else if (mode == 2) {
      B = randomSubset(rng, allCells(), 30 + rng.below(40));
      // stones along live lines
      for (int i = 0; i < 3; i++) {
        int l = rng.below(NL);
        std::vector<int> cells(LINE[l], LINE[l] + 6);
        F = F | randomSubset(rng, cells, 2 + rng.below(3));
      }
      F = F.minus(B);
      t = F.count() + rng.below(10);
    } else if (mode == 3) {
      B = randomSubset(rng, busyCells(), 6 + rng.below(8));
      F = randomSubset(rng, cellsNotIn(B), rng.below(10));
      t = F.count() + 2 + rng.below(9);
    } else {
      B = randomSubset(rng, allCells(), 45 + rng.below(40));
      F = randomSubset(rng, cellsNotIn(B), rng.below(8));
      t = F.count() + 6 + rng.below(7);
    }
    int nl = 0;
    for (int l = 0; l < NL; l++)
      if (!LSET[l].intersects(B)) nl++;
    bool doE3 = nl <= 60 || (it % 25 == 3);
    LostCtx x;
    lostPrepare(B, F, t, x);
    bool e2 = lostE2(F, x, nullptr), e3 = lostE3(F, x, nullptr);
    int r = lostRef(B, F, t, doE3, false);          // no pruning at all
    int rp = lostRef(B, F, t, true, true);          // crude pruning, always both patterns
    bool bad = (e2 != ((r & 1) != 0)) || (e2 != ((rp & 1) != 0)) || (e3 != ((rp & 2) != 0));
    if (doE3) {
      e3checked++;
      if (e3 != ((r & 2) != 0)) bad = true;
    }
    if (bad) {
      mism++;
      if (mism <= 5) printf("  mismatch: t=%d B=%s F=%s fast=%d/%d ref=%d\n", t, setStr(B).c_str(), setStr(F).c_str(), e2, e3, r);
    }
    if (e2) nE2++;
    if (e3) nE3++;
    if (e3 && !e2) nE3only++;
    if (!e2 && !e3) nNone++;
    // the combined function and its witness
    LostOut w;
    bool lost = lostCheck(B, F, t, &w);
    if (lost != (e2 || e3)) mism++;
    if (lost) {
      CellSet U = F;
      int nsets = w.pattern == 2 ? 3 : 4;
      for (int i = 0; i < nsets; i++) U = U | w.S[i];
      if (U.count() != w.stones || w.stones > t + 1) mism++;
      for (int i = 0; i < nsets; i++)
        if (LSET[w.ln[i]].intersects(B) || !w.S[i].subsetOf(LSET[w.ln[i]])) mism++;
    }
  }
  expect(mism == 0, "LOST (E2, E3 separately) == literal reference (unpruned and crudely pruned)");
  expect(nE2 > 10 && nE3 > 10 && nNone > 10 && nE3only >= 5, "fitting and non-fitting positions both occur, also E3 without E2");
  char buf[240];
  snprintf(buf, sizeof buf, "hand-built E2/E3 + near misses; %d random states vs literal reference: E2 fits %ld, E3 fits %ld (E3 only %ld), none %ld; unpruned E3 reference on %ld, crudely pruned reference on all", n,
           nE2, nE3, nE3only, nNone, e3checked);
  report("LOST", f0, buf);
}

static void testChildren(int level) {
  int f0 = g_fail;
  Rng rng(5150);
  int n = pick(level, 150, 300, 600);
  long mism = 0, total = 0, maxCh = 0;
  CellSet T8 = setT8();
  for (int it = 0; it < n; it++) {
    CellSet B, F;
    int t;
    int mode = it % 4;
    if (mode == 0) {
      t = 3 + rng.below(9);
      B = randomSubset(rng, busyCells(), t);
      F = randomSubset(rng, cellsNotIn(B), rng.below(t + 1));
    } else if (mode == 1) {
      t = 6 + rng.below(6);
      B = randomSubset(rng, allCells(), t);
      for (int i = 0; i < 3; i++) {
        int l = rng.below(NL);
        std::vector<int> cells(LINE[l], LINE[l] + 6);
        F = F | randomSubset(rng, cells, 2 + rng.below(3));
      }
      F = F.minus(B);
      if (F.count() > t) continue;
    } else if (mode == 2) {
      B = T8 | randomSubset(rng, cellsNotIn(T8), rng.below(3));
      t = B.count();
      F = randomSubset(rng, cellsNotIn(B), rng.below(t + 1));
    } else {
      t = rng.below(5);
      B = randomSubset(rng, allCells(), t);
      F = randomSubset(rng, cellsNotIn(B), rng.below(t + 1));
    }
    std::set<std::string> fast;
    long cnt = 0;
    genChildrenP(B, F, t, [&](int type, const CellSet& Bc, const CellSet& Fc) {
      cnt++;
      fast.insert(std::string(type == CH_A ? "Q " : "P ") + stateStr(t + 1, Bc, Fc));
    });
    std::set<std::string> ref = childrenRef(B, F, t);
    if (fast != ref) {
      mism++;
      if (mism <= 5) printf("  mismatch: t=%d B=%s F=%s fast=%zu ref=%zu\n", t, setStr(B).c_str(), setStr(F).c_str(), fast.size(), ref.size());
    }
    total += (long)ref.size();
    maxCh = std::max(maxCh, (long)ref.size());
  }
  expect(mism == 0, "children (a),(c),(d) == literal reference");
  // hand counts on the empty board: t = 3 (room 4): no threat child (5 stones needed), no danger child (8)
  {
    CellSet E;
    long a = 0, c = 0, d = 0;
    genChildrenP(E, E, 3, [&](int type, const CellSet&, const CellSet&) { (type == CH_A ? a : type == CH_C ? c : d)++; });
    expect(a == 1 && c == 0 && d == 0, "empty board, t=3: only child (a)");
    a = c = d = 0;
    genChildrenP(E, E, 4, [&](int type, const CellSet&, const CellSet&) { (type == CH_A ? a : type == CH_C ? c : d)++; });
    expect(a == 1 && c == 148 * 6 && d == 0, "empty board, t=4: 888 threat children, no danger child");
    a = c = d = 0;
    genChildrenP(E, E, 7, [&](int type, const CellSet&, const CellSet&) { (type == CH_A ? a : type == CH_C ? c : d)++; });
    // pairs of lines through a cell: 24*C(7,2) + (#cells on 4 lines)*6 + (#cells on 3 lines)*3, times 25
    long pairs = 0;
    for (int cc = 0; cc < NC; cc++) pairs += NLC[cc] * (NLC[cc] - 1) / 2;
    expect(a == 1 && c == 888 && d == pairs * 25, "empty board, t=7: all danger children");
  }
  char buf[160];
  snprintf(buf, sizeof buf, "%d random P-states vs literal reference (%ld children in total, max %ld), hand counts", n, total, maxCh);
  report("children (a),(c),(d)", f0, buf);
}

struct TreeCase {
  const char* name;
  const char* root;      // "t B F"
  int rmax;
  bool forceRoot;
  bool allCells;
};

static void testTree(int level) {
  int f0 = g_fail;
  std::string info;
  u64 crossLeavesTotal = 0, voidTotal = 0;
  // Roots found by experiment (see logs/6c): the naive solver proves them WON within the given R_max.
  std::vector<TreeCase> cases;
  cases.push_back(TreeCase{"A", "8 0,35,46,92,123,169,180,215 -", 8, false, false});                 // the root is itself a listed leaf
  cases.push_back(TreeCase{"B", "9 0,8,35,46,92,123,169,180,215 44,80,116,152,188", 10, true, false});
  cases.push_back(TreeCase{"C", "8 0,35,46,92,123,169,180,215 1,2,3,4,36,72,108,144", 10, true, false});
  cases.push_back(TreeCase{"D", "7 0,46,92,123,169,180,215 43,172", 9, false, false});               // inner non-leaf states
  cases.push_back(TreeCase{"G", "7 0,46,92,123,169,180,215 10,15,20,25", 9, true, false});            // VOID children (threat, reveal)
  cases.push_back(TreeCase{"H", "8 0,35,46,92,123,169,180,215 44,80,116,152", 9, true, false});       // VOID threat children
  if (level >= 2) cases.push_back(TreeCase{"E", "7 0,46,92,123,169,180,215 35", 9, false, false});
  if (level >= 2) cases.push_back(TreeCase{"F", "8 0,35,46,92,123,169,180,215 -", 9, true, false});
  for (auto& tc : cases) {
    int t;
    CellSet B, F;
    CHECK(parseState3(tc.root, t, B, F));
    TreeGen gen;
    gen.rmax = tc.rmax;
    gen.allCells = tc.allCells;
    gen.nodeLimit = 400000;
    bool won = gen.solveP(B, F, t, tc.forceRoot);
    expect(won && !gen.aborted, std::string("tree ") + tc.name + ": the test generator proves the root");
    if (!won) continue;
    std::string tbl = gen.tableText(), lv = gen.leavesText();
    EngineConfig cfg;
    cfg.rmax = tc.rmax;
    cfg.rootT = t;
    cfg.rootB = B;
    cfg.rootF = F;
    cfg.threads = 1;
    cfg.paranoid = true;
    EngineResult r1 = runEngineOnText(tbl, lv, cfg);
    expect(r1.ok && r1.errorCount == 0, std::string("tree ") + tc.name + ": accepted (1 thread, paranoid)");
    cfg.threads = 4;
    cfg.paranoid = false;
    EngineResult r4 = runEngineOnText(tbl, lv, cfg);
    {
      // the engine's own slow path (--crosscheck) must agree everywhere
      EngineConfig cc = cfg;
      cc.threads = 2;
      cc.crosscheck = true;
      cc.leafSample = pick(level, 80, 40, 10);
      EngineResult rc = runEngineOnText(tbl, lv, cc);
      expect(rc.ok && rc.pStates == r1.pStates && rc.qStates == r1.qStates && rc.crossStates == r1.pStates + r1.qStates,
             std::string("tree ") + tc.name + ": --crosscheck finds no discrepancy");
      crossLeavesTotal += rc.crossLeaves;
    }
    cfg.exact = true;
    EngineResult rx = runEngineOnText(tbl, lv, cfg);
    cfg.exact = false;
    bool same = r4.ok && rx.ok && r4.pStates == r1.pStates && r4.qStates == r1.qStates && r4.leavesChecked == r1.leavesChecked &&
                rx.pStates == r1.pStates && rx.qStates == r1.qStates && rx.leavesChecked == r1.leavesChecked;
    for (int i = 0; i < 64; i++)
      for (int k = 0; k < NCH; k++)
        if (r4.st.edges[k][i] != r1.st.edges[k][i] || r4.st.voids[k][i] != r1.st.voids[k][i] || rx.st.edges[k][i] != r1.st.edges[k][i] ||
            rx.st.voids[k][i] != r1.st.voids[k][i])
          same = false;
    expect(same, std::string("tree ") + tc.name + ": identical counts with 4 threads and with --exact");
    // reference re-expansion
    RefExpand ref;
    ref.rmax = tc.rmax;
    for (auto& l : splitLines(tbl)) ref.addTableLine(l);
    for (auto& l : splitLines(lv)) ref.addLeafLine(l);
    ref.visit(KP, B, F, t);
    expect(ref.errors == 0 && (u64)ref.pStates == r1.pStates && (u64)ref.qStates == r1.qStates && (u64)ref.leafCount == r1.leavesChecked,
           std::string("tree ") + tc.name + ": counts equal to the naive reference re-expansion");
    u64 nvoid = 0;
    for (int i = 0; i < 64; i++)
      for (int k = 0; k < NCH; k++) nvoid += r1.st.voids[k][i];
    voidTotal += nvoid;
    info += std::string(" ") + tc.name + ":P=" + std::to_string(r1.pStates) + ",Q=" + std::to_string(r1.qStates) + ",leaves=" +
            std::to_string(r1.leavesChecked) + ",void=" + std::to_string(nvoid) + ",maxt=" + std::to_string(r1.st.maxT);
    // ---- mutations that must be rejected
    std::vector<std::string> tl = splitLines(tbl), ll = splitLines(lv);
    // reachable table entries / leaves (in canonical form) according to the reference
    std::vector<size_t> reachT, reachL;
    for (size_t i = 0; i < tl.size(); i++) {
      size_t sp = tl[i].rfind(' ');
      if (ref.seen.count("Q " + tl[i].substr(0, sp))) reachT.push_back(i);
    }
    for (size_t i = 0; i < ll.size(); i++)
      if (ref.seen.count("P " + ll[i])) reachL.push_back(i);
    expect(reachT.size() == r1.qStates, "every visited Q-state has exactly one table line");
    expect(reachL.size() == r1.leavesChecked, "every checked leaf has exactly one leaf line");
    Rng rng(1000 + tc.rmax);
    int rejected = 0, tried = 0;
    auto mustFail = [&](const std::string& T, const std::string& L, int rmax, const char* needle, const char* what) {
      EngineConfig c2 = cfg;
      c2.rmax = rmax;
      c2.threads = 2;
      EngineResult r = runEngineOnText(T, L, c2);
      tried++;
      bool found = false;
      for (auto& e : r.errors)
        if (e.msg.find(needle) != std::string::npos) found = true;
      if (!r.ok && r.errorCount > 0 && found)
        rejected++;
      else
        expect(false, std::string("tree ") + tc.name + ": mutation not rejected: " + what);
    };
    for (int rep = 0; rep < 3 && !reachT.empty(); rep++) {
      // (1) delete a reachable table entry -> strategy hole
      std::vector<std::string> t2 = tl;
      size_t victim = reachT[rng.below((int)reachT.size())];
      t2.erase(t2.begin() + victim);
      mustFail(joinLines(t2), lv, tc.rmax, "strategy hole", "deleted table entry");
      // (2) move onto an SP stone / onto a revealed FP stone -> rejected at load time as a malformed line
      {
        std::vector<std::string> t3 = tl;
        std::string& line = t3[victim];
        int tt;
        CellSet b, f;
        size_t sp = line.rfind(' ');
        CHECK(parseState3(line.substr(0, sp), tt, b, f));
        uint8_t cells[NC];
        int nb = b.list(cells);
        if (nb > 0) {
          line = line.substr(0, sp) + " " + std::to_string((int)cells[rng.below(nb)]);
          EngineConfig c2 = cfg;
          c2.threads = 1;
          Engine en;
          EngineResult r = runEngineOnText(joinLines(t3), lv, c2, &en);
          tried++;
          if (!r.ok && en.tableLoad.bad == 1)
            rejected++;
          else
            expect(false, std::string("tree ") + tc.name + ": table move on an occupied cell not rejected");
        }
      }
    }
    if (!reachL.empty()) {
      // (3) delete a reachable leaf at t = R_max (if there is one) -> "t >= R_max"; otherwise any leaf must
      //     still lead to a rejection or to a longer but valid expansion; only the first case is asserted
      for (size_t i : reachL) {
        int tt;
        CellSet b, f;
        CHECK(parseState3(ll[i], tt, b, f));
        if (tt == tc.rmax) {
          std::vector<std::string> l2 = ll;
          l2.erase(l2.begin() + i);
          mustFail(tbl, joinLines(l2), tc.rmax, "R_max", "deleted leaf at t = R_max");
          break;
        }
      }
      // (4) R_max lowered below the depth of the tree
      if (r1.st.maxT > t) mustFail(tbl, lv, t, "R_max", "R_max lowered to the root's t");
    }
    // (5) a bogus leaf: an expanded (non-leaf) P-state that is not LEAF is listed as a leaf
    {
      LeafChecker lc;
      bool done = false;
      for (const std::string& s : ref.seen) {
        if (s[0] != 'P') continue;
        std::string k = s.substr(2);
        if (ref.leaves.count(k)) continue;
        int tt;
        CellSet b, f;
        CHECK(parseState3(k, tt, b, f));
        if (lc.check(b, f, tt)) continue;
        mustFail(tbl, lv + k + "\n", tc.rmax, "listed leaf is not LEAF", "bogus leaf");
        done = true;
        break;
      }
      if (!done) info += "(no non-LEAF inner P-state for the bogus-leaf test)";
    }
    // (6) malformed lines
    {
      EngineConfig c2 = cfg;
      Engine en;
      EngineResult r = runEngineOnText(tbl + "7 0,1,2 - 300\n", lv, c2, &en);
      tried++;
      if (!r.ok && en.tableLoad.bad == 1) rejected++;
      else expect(false, "malformed table line not rejected");
      Engine en2;
      r = runEngineOnText(tbl, lv + "3 0,1 -\n", c2, &en2);
      tried++;
      if (!r.ok && en2.leafLoad.bad == 1) rejected++;
      else expect(false, "malformed leaf line not rejected");
    }
    // (7) the same proof written in another frame (random symmetry applied to every line, lines shuffled,
    //     descending cell order) must still be accepted with identical counts
    {
      auto mapSet = [&](const CellSet& s, int g, bool desc) {
        CellSet im = imageSet(g, s);
        uint8_t c[NC];
        int n = im.list(c);
        if (n == 0) return std::string("-");
        std::string r;
        for (int i = 0; i < n; i++) {
          if (i) r += ',';
          r += std::to_string((int)c[desc ? n - 1 - i : i]);
        }
        return r;
      };
      std::vector<std::string> t2, l2;
      for (auto& line : tl) {
        int tt;
        CellSet b, f;
        size_t sp = line.rfind(' ');
        CHECK(parseState3(line.substr(0, sp), tt, b, f));
        int m = atoi(line.c_str() + sp + 1);
        int g = rng.below(NG);
        t2.push_back(std::to_string(tt) + "  " + mapSet(b, g, true) + "\t" + mapSet(f, g, false) + " " + std::to_string((int)PERM[g][m]) + "\r");
      }
      for (auto& line : ll) {
        int tt;
        CellSet b, f;
        CHECK(parseState3(line, tt, b, f));
        int g = rng.below(NG);
        l2.push_back(std::to_string(tt) + " " + mapSet(b, g, false) + " " + mapSet(f, g, true));
      }
      for (size_t i = t2.size(); i > 1; i--) std::swap(t2[i - 1], t2[rng.below((int)i)]);
      t2.push_back("# comment line");
      t2.push_back("");
      EngineConfig c2 = cfg;
      c2.threads = 3;
      Engine en;
      EngineResult r = runEngineOnText(joinLines(t2), joinLines(l2), c2, &en);
      expect(r.ok && r.pStates == r1.pStates && r.qStates == r1.qStates && r.leavesChecked == r1.leavesChecked,
             std::string("tree ") + tc.name + ": proof rewritten in random frames is accepted with identical counts");
    }
    info += "(" + std::to_string(rejected) + "/" + std::to_string(tried) + " mutations rejected)";
  }
  // a non-leaf P-state in which a LOST pattern fits must be rejected: 10 far-away SP stones, no revealed
  // stone, t = 10 (E2 needs 11 stones, see testLost); with 9 stones and t = 9 nothing fits
  {
    EngineConfig cfg;
    cfg.rmax = 12;
    cfg.threads = 1;
    int t;
    CHECK(parseState3("10 185,186,187,188,189,190,191,192,193,194 -", t, cfg.rootB, cfg.rootF));
    cfg.rootT = t;
    EngineResult r = runEngineOnText("", "", cfg);
    bool lost = false;
    for (auto& e : r.errors)
      if (e.msg.find("LOST") != std::string::npos) lost = true;
    expect(!r.ok && lost && r.pStates == 1 && r.qStates == 0, "P-state with a fitting LOST pattern is rejected and not expanded");
    CHECK(parseState3("9 185,186,187,188,189,190,191,192,193 -", t, cfg.rootB, cfg.rootF));
    cfg.rootT = t;
    cfg.maxErrors = 1000000;
    cfg.rmax = 10;
    r = runEngineOnText("", "", cfg);
    // (the root is expanded, so its Q-child and its threat children are visited; the Q-child has no table
    // entry, and the threat children at t = 10 = R_max are rejected without being expanded)
    // errors: one per child P-state (R_max) plus the strategy hole = number of P-states
    expect(!r.ok && r.complete && r.qStates == 1 && r.pStates > 1 && r.errorCount == r.pStates && r.errors.size() == 50,
           "one stone less: no LOST pattern at the root, the root is expanded");
    // a VOID root (5 revealed stones on a live line) is reported as such and nothing is expanded
    CHECK(parseState3("5 185,186,187,188,189 0,1,2,3,4", t, cfg.rootB, cfg.rootF));
    cfg.rootT = t;
    r = runEngineOnText("", "", cfg);
    expect(r.rootVoid && r.pStates == 0 && r.qStates == 0, "VOID root");
    // a root violating |B| = t is an error
    CHECK(parseState3("4 185,186,187,188,189 -", t, cfg.rootB, cfg.rootF));
    cfg.rootT = t;
    r = runEngineOnText("", "", cfg);
    expect(!r.ok && r.errorCount == 1, "root with |B| != t is rejected");
  }
  // an empty table must fail at the first Q-state
  {
    EngineConfig cfg;
    cfg.rmax = 3;
    cfg.threads = 2;
    EngineResult r = runEngineOnText("", "", cfg);
    bool hole = false;
    for (auto& e : r.errors)
      if (e.msg.find("strategy hole") != std::string::npos && e.state == "1 - -") hole = true;
    expect(!r.ok && hole && r.pStates == 1 && r.qStates == 1, "empty table: rejected with a strategy hole at Q(-,-,1)");
    cfg.rmax = 0;
    r = runEngineOnText("", "", cfg);
    expect(!r.ok && r.errorCount == 1 && r.errors[0].msg.find("R_max") != std::string::npos, "R_max = 0: rejected at the root");
  }
  expect(voidTotal > 20, "VOID children occur in the test trees");
  info += " | leaves re-checked by the unpruned reference in --crosscheck runs: " + std::to_string(crossLeavesTotal);
  report("tree expansion", f0, info);
}

static int selftest(int level) {
  double t0 = nowSec();
  printf("treecheck selftest (%s)\n", level <= 0 ? "quick" : (level == 1 ? "default" : "full"));
  g_testStart = t0;
  testGeometry();
  testCanon(level);
  testAdmissible(level);
  testLeafRegression();
  testLeafBrute(level);
  testLeafFlowRef(level);
  testLost(level);
  testChildren(level);
  testTree(level);
  printf("selftest: %s (%d failures, %.1f s)\n", g_fail == 0 ? "ALL PASSED" : "FAILED", g_fail, nowSec() - t0);
  return g_fail == 0 ? 0 : 1;
}

// =====================================================================================================
// 14. Command line
// =====================================================================================================

static void usage() {
  fprintf(stderr,
          "usage:\n"
          "  treecheck --table FILE --leaves FILE --rmax R [--threads T] [--out FILE.json] [--root \"t B F\"]\n"
          "            [--exact] [--fast] [--busy-only] [--max-errors N] [--progress SEC] [--time-limit SEC]\n"
          "            [--crosscheck [--crosscheck-leaves N]]\n"
          "      re-expands the proof tree of SPEC section 4; exit code 0 iff ok\n"
          "      --exact      visited set with full keys instead of 128-bit hashes\n"
          "      (default)    every flow used for a LEAF = true verdict is validated literally; --fast skips this\n"
          "      --busy-only  a table move on a non-busy cell is an error (by default such moves are only counted)\n"
          "      --crosscheck (slow) additionally re-derives, with the naive reference code, the canonical form of\n"
          "                   every state, the VOID test of every child, LOST and the children of every expanded\n"
          "                   P-state, and LEAF of every N-th leaf (unpruned, independent matching code)\n"
          "  treecheck --leafcheck [--verbose] [--fast] [--repeat N]  < lines \"k B F\"\n"
          "  treecheck --lostcheck                             < lines \"t B F\"\n"
          "  treecheck --selftest [--quick | --full]\n"
          "  treecheck --gen-tree --root \"t B F\" --rmax R --out-table FILE --out-leaves FILE [--force-root]\n"
          "            [--all-cells] [--node-limit N]     (test scaffolding: naive solver, small trees only)\n");
}

static std::string cellsJson(const CellSet& s) {
  uint8_t c[NC];
  int n = s.list(c);
  std::string r = "[";
  for (int i = 0; i < n; i++) {
    if (i) r += ",";
    r += std::to_string((int)c[i]);
  }
  return r + "]";
}

static int leafcheckMode(bool verbose, bool paranoid, int repeat) {
  LeafChecker lc;
  lc.paranoid = paranoid;
  std::string line;
  u64 n = 0, bad = 0;
  double t0 = nowSec();
  while (std::getline(std::cin, line)) {
    int k;
    CellSet B, F;
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
    if (!parseState3(line, k, B, F)) {
      bad++;
      printf("{\"error\":\"malformed instance line\"}\n");
      continue;
    }
    LeafOut lo;
    bool v = lc.check(B, F, k, &lo);
    for (int r = 1; r < repeat; r++) {
      bool v2 = lc.check(B, F, k, nullptr);
      CHECK(v2 == v);
    }
    n++;
    printf("{\"k\":%d,\"B\":\"%s\",\"F\":\"%s\",\"leaf\":%s,\"reason\":%d", k, setStr(B).c_str(), setStr(F).c_str(), v ? "true" : "false", lo.reason);
    if (verbose) {
      printf(",\"sur\":%d,\"S\":[", lo.sur);
      for (size_t i = 0; i < lo.S.size(); i++) printf("%s%s", i ? "," : "", cellsJson(LSET[lo.S[i]]).c_str());
      printf("]");
    }
    printf("}\n");
  }
  double dt = nowSec() - t0;
  fprintf(stderr, "leafcheck: %llu instances (%llu malformed), %llu LEAF evaluations in %.3f s (%.1f us each), %llu BFS, %llu single / %llu pair / %llu triple flows\n",
          (unsigned long long)n, (unsigned long long)bad, (unsigned long long)lc.nCalls, dt, lc.nCalls ? dt * 1e6 / lc.nCalls : 0.0,
          (unsigned long long)lc.nBfs, (unsigned long long)lc.nSingle, (unsigned long long)lc.nPair, (unsigned long long)lc.nTriple);
  return bad ? 2 : 0;
}

static int lostcheckMode() {
  std::string line;
  u64 bad = 0;
  while (std::getline(std::cin, line)) {
    int t;
    CellSet B, F;
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
    if (!parseState3(line, t, B, F)) {
      bad++;
      printf("{\"error\":\"malformed instance line\"}\n");
      continue;
    }
    LostOut lo;
    bool v = lostCheck(B, F, t, &lo);
    printf("{\"t\":%d,\"B\":\"%s\",\"F\":\"%s\",\"lost\":%s", t, setStr(B).c_str(), setStr(F).c_str(), v ? "true" : "false");
    if (v && lo.pattern == 2) {
      printf(",\"pattern\":\"E2\",\"l\":%s,\"a\":%s,\"b\":%s,\"x\":%d,\"e\":%d,\"c\":%d,\"Sl\":%s,\"Sa\":%s,\"Sb\":%s,\"stones\":%d",
             cellsJson(LSET[lo.ln[0]]).c_str(), cellsJson(LSET[lo.ln[1]]).c_str(), cellsJson(LSET[lo.ln[2]]).c_str(), lo.x, lo.e, lo.c,
             cellsJson(lo.S[0]).c_str(), cellsJson(lo.S[1]).c_str(), cellsJson(lo.S[2]).c_str(), lo.stones);
    } else if (v) {
      printf(",\"pattern\":\"E3\",\"a\":%s,\"a2\":%s,\"b\":%s,\"b2\":%s,\"x\":%d,\"c\":%d,\"c2\":%d,\"Sa\":%s,\"Sa2\":%s,\"Sb\":%s,\"Sb2\":%s,\"stones\":%d",
             cellsJson(LSET[lo.ln[0]]).c_str(), cellsJson(LSET[lo.ln[1]]).c_str(), cellsJson(LSET[lo.ln[2]]).c_str(),
             cellsJson(LSET[lo.ln[3]]).c_str(), lo.x, lo.c, lo.c2, cellsJson(lo.S[0]).c_str(), cellsJson(lo.S[1]).c_str(),
             cellsJson(lo.S[2]).c_str(), cellsJson(lo.S[3]).c_str(), lo.stones);
    } else {
      printf(",\"pattern\":null");
    }
    printf("}\n");
  }
  return bad ? 2 : 0;
}

int main(int argc, char** argv) {
  initGeometry();
  std::string tablePath, leavesPath, outPath, rootStr, outTable, outLeaves;
  int rmax = -1, threads = 1, repeat = 1;
  // every flow behind a "LEAF = true" verdict is validated literally unless --fast is given
  bool exact = false, paranoid = true, busyOnly = false, doLeaf = false, doLost = false, doSelf = false, quick = false, full = false;
  bool verbose = false, genTree = false, forceRoot = false, allCellsFlag = false, crosscheck = false;
  int leafSample = 0;
  u64 maxErrors = 50, nodeLimit = 0;
  double progress = 30, timeLimit = 0;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        usage();
        exit(2);
      }
      return argv[++i];
    };
    if (a == "--table") tablePath = next();
    else if (a == "--leaves") leavesPath = next();
    else if (a == "--out") outPath = next();
    else if (a == "--rmax") rmax = atoi(next().c_str());
    else if (a == "--threads") threads = atoi(next().c_str());
    else if (a == "--root") rootStr = next();
    else if (a == "--exact") exact = true;
    else if (a == "--paranoid") paranoid = true;
    else if (a == "--fast") paranoid = false;
    else if (a == "--busy-only") busyOnly = true;
    else if (a == "--crosscheck") crosscheck = true;
    else if (a == "--crosscheck-leaves") leafSample = atoi(next().c_str());
    else if (a == "--max-errors") maxErrors = strtoull(next().c_str(), nullptr, 10);
    else if (a == "--progress") progress = atof(next().c_str());
    else if (a == "--time-limit") timeLimit = atof(next().c_str());
    else if (a == "--leafcheck") doLeaf = true;
    else if (a == "--lostcheck") doLost = true;
    else if (a == "--selftest") doSelf = true;
    else if (a == "--quick") quick = true;
    else if (a == "--full") full = true;
    else if (a == "--verbose") verbose = true;
    else if (a == "--repeat") repeat = atoi(next().c_str());
    else if (a == "--gen-tree") genTree = true;
    else if (a == "--force-root") forceRoot = true;
    else if (a == "--all-cells") allCellsFlag = true;
    else if (a == "--node-limit") nodeLimit = strtoull(next().c_str(), nullptr, 10);
    else if (a == "--out-table") outTable = next();
    else if (a == "--out-leaves") outLeaves = next();
    else {
      fprintf(stderr, "treecheck: unknown argument %s\n", a.c_str());
      usage();
      return 2;
    }
  }
  if (doSelf) return selftest(quick ? 0 : (full ? 2 : 1));
  if (doLeaf) return leafcheckMode(verbose, paranoid, repeat);
  if (doLost) return lostcheckMode();

  int rootT = 0;
  CellSet rootB, rootF;
  if (!rootStr.empty() && !parseState3(rootStr, rootT, rootB, rootF)) {
    fprintf(stderr, "treecheck: cannot parse --root \"%s\" (expected \"t B F\")\n", rootStr.c_str());
    return 2;
  }
  if (rmax < 0 || rmax > 60 || rootT > 60 || threads < 1 || threads > 256) {
    usage();
    return 2;
  }

  if (genTree) {
    if (outTable.empty() || outLeaves.empty()) {
      usage();
      return 2;
    }
    TreeGen gen;
    gen.rmax = rmax;
    gen.allCells = allCellsFlag;
    gen.nodeLimit = nodeLimit;
    double t0 = nowSec();
    bool won = gen.solveP(rootB, rootF, rootT, forceRoot);
    fprintf(stderr, "gen-tree: root %s, %llu nodes, %zu table entries, %zu leaves, %.1f s%s\n", won ? "WON" : "not proved",
            (unsigned long long)gen.nodes, gen.table.size(), gen.leaves.size(), nowSec() - t0, gen.aborted ? " (node limit hit)" : "");
    FILE* f = fopen(outTable.c_str(), "w");
    CHECK(f != nullptr);
    std::string s = gen.tableText();
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
    f = fopen(outLeaves.c_str(), "w");
    CHECK(f != nullptr);
    s = gen.leavesText();
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
    return won ? 0 : 1;
  }

  if (tablePath.empty() || leavesPath.empty()) {
    usage();
    return 2;
  }
  std::unique_ptr<Engine> en(new Engine());
  en->cfg.rmax = rmax;
  en->cfg.threads = threads;
  en->cfg.exact = exact;
  en->cfg.paranoid = paranoid;
  en->cfg.busyOnly = busyOnly;
  en->cfg.crosscheck = crosscheck;
  en->cfg.leafSample = leafSample;
  en->cfg.maxErrors = maxErrors ? maxErrors : 1;
  en->cfg.progressSec = progress;
  en->cfg.timeLimit = timeLimit;
  en->cfg.rootT = rootT;
  en->cfg.rootB = rootB;
  en->cfg.rootF = rootF;
  double tl0 = nowSec();
  {
    FILE* f = fopen(tablePath.c_str(), "rb");
    if (!f) {
      fprintf(stderr, "treecheck: cannot open table %s\n", tablePath.c_str());
      return 2;
    }
    loadKeys(f, true, threads, en->table, en->tableLoad);
    fclose(f);
    f = fopen(leavesPath.c_str(), "rb");
    if (!f) {
      fprintf(stderr, "treecheck: cannot open leaf file %s\n", leavesPath.c_str());
      return 2;
    }
    loadKeys(f, false, threads, en->leaves, en->leafLoad);
    fclose(f);
  }
  double loadSec = nowSec() - tl0;
  fprintf(stderr, "[treecheck] loaded %llu table entries (%llu bad lines, %llu not canonical, %llu duplicates, %llu conflicts) and %llu leaves (%llu bad lines) in %.1f s\n",
          (unsigned long long)en->tableLoad.entries, (unsigned long long)en->tableLoad.bad, (unsigned long long)en->tableLoad.noncanon,
          (unsigned long long)en->table.dups, (unsigned long long)en->table.conflicts, (unsigned long long)en->leafLoad.entries,
          (unsigned long long)en->leafLoad.bad, loadSec);
  EngineResult r = en->run();
  std::string js = resultJson(*en, r, loadSec);
  if (!outPath.empty()) {
    FILE* f = fopen(outPath.c_str(), "w");
    if (!f) {
      fprintf(stderr, "treecheck: cannot write %s\n", outPath.c_str());
      printf("%s\n", js.c_str());
      return 2;
    }
    fprintf(f, "%s\n", js.c_str());
    fclose(f);
  }
  printf("%s\n", js.c_str());
  return r.ok ? 0 : 1;
}

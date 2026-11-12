// match.hpp -- SPEC 7.2: live lines, slack, admissibility, pairings (bipartite b-matching),
// exact decision of C(B,F,k) by enumeration.  Header-only, namespace ttt, thread-safe
// (no shared mutable state; everything lives on the caller's stack).
#pragma once
#include "geom.hpp"

namespace ttt {

// ---------------------------------------------------------------------------------------------
// Live lines, slack, admissibility (SPEC 3)
// ---------------------------------------------------------------------------------------------

// Ascending ids of the lines with no cell of B; returns their number.
inline int live_lines(const CellSet& B, int out[NL]) {
  const Geom& g = G();
  uint8_t dead[NL];
  memset(dead, 0, sizeof dead);
  B.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) dead[g.lc[c][i]] = 1; });
  int n = 0;
  for (int l = 0; l < NL; l++) if (!dead[l]) out[n++] = l;
  return n;
}

// slack(B) = (216 - |B|) - 2 |L(B)|
inline int slack(const CellSet& B) {
  int tmp[NL];
  return (NC - B.count()) - 2 * live_lines(B, tmp);
}

// X is B-admissible: X and B disjoint, every live line has at most 4 cells of X, and no danger
// pair (two distinct live lines with exactly 4 cells of X each, sharing a cell that is not in X).
inline bool admissible(const CellSet& B, const CellSet& X) {
  if (X.intersects(B)) return false;
  const Geom& g = G();
  int8_t cnt[NL];
  memset(cnt, 0, sizeof cnt);
  B.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]] = -64; });  // dead lines stay < 0
  bool ok = true, any4 = false;
  X.for_each([&](int c) {
    for (int i = 0; i < g.nlc[c]; i++) {
      int v = ++cnt[g.lc[c][i]];
      if (v > 4) ok = false;
      if (v == 4) any4 = true;
    }
  });
  if (!ok) return false;
  if (!any4) return true;
  for (int a = 0; a < NL; a++) {
    if (cnt[a] != 4) continue;
    for (int i = 0; i < 6; i++) {
      int x = g.line[a][i];
      if (X.test(x)) continue;
      for (int j = 0; j < g.nlc[x]; j++) { int b = g.lc[x][j]; if (b != a && cnt[b] == 4) return false; }
    }
  }
  return true;
}

// ---------------------------------------------------------------------------------------------
// The matcher: every live line must own 2 cells that are not blocked (blocked = in X or in B).
// Kuhn-style augmenting depth-first search from the line side.  A line that is reached once in
// a search is never entered again in the same search (visit stamps on lines).
// ---------------------------------------------------------------------------------------------
namespace detail {

constexpr int16_t FREE = -1;      // owner[] value of a cell that is available and unassigned
constexpr int16_t BLOCKED = -2;   // owner[] value of a cell of X or B

// Static tables of the matcher.
struct CellInc {
  uint8_t line[NC][8];            // the lines through a cell, padded with the dummy line id NL
  uint8_t bit[NC][8];             // 1 << (index of the cell in G().line[line]); 0 for the padding
  uint8_t ord[NL][6];             // cells of a line, cells on few lines first (greedy start)
  uint8_t lord[NL];               // all lines, lines with few busy cells first (greedy start)
  CellInc() {
    const Geom& g = G();
    for (int c = 0; c < NC; c++)
      for (int i = 0; i < 8; i++) {
        line[c][i] = (uint8_t)NL;
        bit[c][i] = 0;
        if (i >= g.nlc[c]) continue;
        int l = g.lc[c][i];
        line[c][i] = (uint8_t)l;
        for (int j = 0; j < 6; j++) if (g.line[l][j] == c) bit[c][i] = (uint8_t)(1 << j);
      }
    int nbusy[NL], idx[NL];
    for (int l = 0; l < NL; l++) {
      int v[6];
      nbusy[l] = 0;
      for (int j = 0; j < 6; j++) { v[j] = g.line[l][j]; nbusy[l] += (g.busyIdx[v[j]] >= 0); }
      std::stable_sort(v, v + 6, [&](int a, int b) { return g.nlc[a] < g.nlc[b]; });
      for (int j = 0; j < 6; j++) ord[l][j] = (uint8_t)v[j];
      idx[l] = l;
    }
    std::stable_sort(idx, idx + NL, [&](int a, int b) { return nbusy[a] < nbusy[b]; });
    for (int i = 0; i < NL; i++) lord[i] = (uint8_t)idx[i];
  }
};
inline const CellInc& cell_inc() { static CellInc t; return t; }

struct Matcher {
  const Geom& g;
  const CellInc& ci;
  int nl;                 // number of live lines
  int16_t lines[NL];      // live line ids, ascending
  uint8_t dead[NL];       // 1 = the line contains a cell of B
  int16_t owner[NC];      // FREE, BLOCKED, or the id of the live line that owns the cell
  uint32_t lvis_[NL + 2]; // visit stamp of each line (index l + 2; the two extra slots stay 0)
  uint8_t fm_[NL + 3];    // 6-bit mask of the FREE cells of each line (index l + 2; slots 0, 1 stay 0; last = dummy)
  uint32_t stamp;         // stamp of the current search (>= 1 during a search)
  int nvis;               // number of lines reached in the current search (roots included)
  uint32_t* logp;         // undo log cursor (used by augL only)

  Matcher() : g(G()), ci(cell_inc()), nl(0), stamp(0), nvis(0), logp(nullptr) {}
  Matcher(const Matcher&) = delete;
  Matcher& operator=(const Matcher&) = delete;

  uint32_t& lvis(int l) { return lvis_[l + 2]; }
  uint32_t lvis(int l) const { return lvis_[l + 2]; }
  uint8_t& fm(int l) { return fm_[l + 2]; }

  void init(const CellSet& B, const CellSet& X) {
    memset(dead, 0, sizeof dead);
    B.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) dead[g.lc[c][i]] = 1; });
    nl = 0;
    for (int l = 0; l < NL; l++) if (!dead[l]) lines[nl++] = (int16_t)l;
    memset(owner, 0xff, sizeof owner);  // FREE
    X.for_each([&](int c) { owner[c] = BLOCKED; });
    B.for_each([&](int c) { owner[c] = BLOCKED; });
    memset(lvis_, 0, sizeof lvis_);
    memset(fm_, 0, sizeof fm_);         // valid after recount_fm()
    stamp = 0;
    nvis = 0;
  }

  // ---- bookkeeping of free cells
  void take(int c, int l) {                 // FREE -> owned by l (or BLOCKED)
    owner[c] = (int16_t)l;
    const uint8_t* ls = ci.line[c];
    const uint8_t* bs = ci.bit[c];
    for (int i = 0; i < 7; i++) fm(ls[i]) &= (uint8_t)~bs[i];
  }
  void set_free(int c) {                    // owned / blocked -> FREE
    owner[c] = FREE;
    const uint8_t* ls = ci.line[c];
    const uint8_t* bs = ci.bit[c];
    for (int i = 0; i < 7; i++) fm(ls[i]) |= bs[i];
  }
  void recount_fm() {                       // free masks of the live lines from owner[]
    memset(fm_, 0, sizeof fm_);
    for (int i = 0; i < nl; i++) {
      int l = lines[i];
      const int* cs = g.line[l];
      unsigned m = 0;
      for (int j = 0; j < 6; j++) m |= unsigned(owner[cs[j]] == FREE) << j;
      fm(l) = (uint8_t)m;
    }
  }

  // Give line l one more cell (alternating path to a free cell).  The caller has stamped l.
  // Order of exploration: a free cell of l itself; a neighbour (the owner of a cell of l) that
  // has a free cell; then depth-first through the remaining neighbours.  Every line is entered
  // at most once per search and nothing changes before the search succeeds, so a failed search
  // is exhaustive: the stamped lines are closed under "owner of a non-blocked cell of the line".
  bool aug(int l) {
    const int* cs = g.line[l];
    unsigned f = fm(l);
    if (f) { take(cs[__builtin_ctz(f)], l); return true; }
    int o[6];
    unsigned cand2 = 0, cand3 = 0;
    for (int i = 0; i < 6; i++) {
      int oi = owner[cs[i]];
      o[i] = oi;
      unsigned ok = unsigned(oi >= 0) & unsigned(oi != l) & unsigned(lvis(oi) != stamp);
      cand3 |= ok << i;
      cand2 |= (ok & unsigned(fm(oi) != 0)) << i;
    }
    if (cand2) {                            // a neighbour takes one of its free cells and hands over cs[i]
      int i = __builtin_ctz(cand2), oi = o[i];
      take(g.line[oi][__builtin_ctz(fm(oi))], oi);
      owner[cs[i]] = (int16_t)l;
      return true;
    }
    while (cand3) {
      int i = __builtin_ctz(cand3), oi = o[i];
      cand3 &= cand3 - 1;
      if (lvis(oi) == stamp) continue;
      lvis(oi) = stamp;
      nvis++;
      if (aug(oi)) { owner[cs[i]] = (int16_t)l; return true; }
    }
    return false;
  }

  // Same search, but every change of owner[] is recorded in the undo log.
  bool augL(int l) {
    const int* cs = g.line[l];
    unsigned f = fm(l);
    if (f) { int c = cs[__builtin_ctz(f)]; *logp++ = (uint32_t(c) << 16) | uint16_t(FREE); take(c, l); return true; }
    int o[6];
    unsigned cand2 = 0, cand3 = 0;
    for (int i = 0; i < 6; i++) {
      int oi = owner[cs[i]];
      o[i] = oi;
      unsigned ok = unsigned(oi >= 0) & unsigned(oi != l) & unsigned(lvis(oi) != stamp);
      cand3 |= ok << i;
      cand2 |= (ok & unsigned(fm(oi) != 0)) << i;
    }
    if (cand2) {
      int i = __builtin_ctz(cand2), oi = o[i];
      int c = g.line[oi][__builtin_ctz(fm(oi))];
      *logp++ = (uint32_t(c) << 16) | uint16_t(FREE);
      take(c, oi);
      *logp++ = (uint32_t(cs[i]) << 16) | uint16_t(oi);
      owner[cs[i]] = (int16_t)l;
      return true;
    }
    while (cand3) {
      int i = __builtin_ctz(cand3), oi = o[i];
      cand3 &= cand3 - 1;
      if (lvis(oi) == stamp) continue;
      lvis(oi) = stamp;
      nvis++;
      if (augL(oi)) { *logp++ = (uint32_t(cs[i]) << 16) | uint16_t(oi); owner[cs[i]] = (int16_t)l; return true; }
    }
    return false;
  }
  // Undo every logged change (restores owner[] and the free masks).
  void undo_to(uint32_t* mark) {
    while (logp > mark) {
      --logp;
      int c = (int)(*logp >> 16);
      int16_t old = (int16_t)(uint16_t)(*logp & 0xffff);
      if (old == FREE) set_free(c); else owner[c] = old;
    }
  }

  // One fresh search from l.  On failure the lines with lvis == stamp form a set R with
  // l in R such that every non-blocked cell of N(R) is owned by a line of R.
  bool gain1(int l) { ++stamp; lvis(l) = stamp; nvis = 1; return aug(l); }
  bool gain1L(int l) { ++stamp; lvis(l) = stamp; nvis = 1; return augL(l); }

  // Build a pairing (2 cells per live line).  Returns -1 on success; otherwise the id of a line
  // that cannot be served, and the lines with lvis == stamp are a Hall violator.
  // Greedy start (first fit): lines with few busy cells first, each line prefers its cells
  // that lie on few lines.  The order only affects speed, never the result.
  int build() {
    uint8_t have[NL];
    for (int q = 0; q < NL; q++) {
      int l = ci.lord[q];
      if (dead[l]) continue;
      const uint8_t* cs = ci.ord[l];
      int n = 0;
      for (int j = 0; j < 6 && n < 2; j++) if (owner[cs[j]] == FREE) { owner[cs[j]] = (int16_t)l; n++; }
      have[l] = (uint8_t)n;
    }
    recount_fm();
    for (int i = 0; i < nl; i++) {
      int l = lines[i];
      for (int n = have[l]; n < 2; n++) if (!gain1(l)) return l;
    }
    return -1;
  }

  // Lines reached by the last search(es) that used the current stamp.
  void reached(std::vector<int>& out) const {
    out.clear();
    for (int i = 0; i < nl; i++) if (lvis(lines[i]) == stamp) out.push_back(lines[i]);
  }
};

// |N(S) \ X| - 2|S| for a list of lines (direct recomputation from the definition).
inline int surplus_of(const std::vector<int>& S, const CellSet& X) {
  const Geom& g = G();
  CellSet N;
  for (int l : S) N = N | g.lineSet[l];
  return N.minus(X).count() - 2 * (int)S.size();
}

}  // namespace detail

// ---------------------------------------------------------------------------------------------
// Pairings
// ---------------------------------------------------------------------------------------------
struct Pairing {
  bool ok;
  int16_t owner[NC];       //  ok: owner[c] = live line that cell c is assigned to (2 cells per live line), else -1
  std::vector<int> hall;   // !ok: line ids of a set S with |N(S) \ X| < 2|S|
};

inline Pairing find_pairing(const CellSet& B, const CellSet& X) {
  Pairing P;
  detail::Matcher M;
  M.init(B, X);
  int bad = M.build();
  if (bad < 0) {
    P.ok = true;
    for (int c = 0; c < NC; c++) P.owner[c] = M.owner[c] >= 0 ? M.owner[c] : (int16_t)-1;
  } else {
    P.ok = false;
    for (int c = 0; c < NC; c++) P.owner[c] = -1;
    M.reached(P.hall);
    if (detail::surplus_of(P.hall, X) >= 0) { fprintf(stderr, "match.hpp: internal error (bad Hall violator)\n"); abort(); }
  }
  return P;
}

// ---------------------------------------------------------------------------------------------
// enum_check: exact decision of C(B,F,k) = "every B-admissible X with F <= X, |X| <= k has a
// pairing" by depth-first enumeration of the stones of X \ F in ascending cell order.
//
// Why the enumeration is exact (the comment required by SPEC 7.2):
//  (1) Only cells of N(L(B)) are candidates.  A stone on a cell that lies on no live line changes
//      neither admissibility (which only looks at live lines) nor any set N(S) \ X, so an
//      admissible X has a pairing iff F u ((X \ F) n N(L(B))) has one; that set is admissible,
//      contains F and is not larger.
//  (2) Admissibility is downward closed.  Hence every admissible X = F u {c1 < ... < cm} is a node
//      of the tree: all its prefixes F u {c1..ci} are admissible, so none of them is skipped.
//      Conversely, when a placement breaks a cap or creates a danger pair, the node and all its
//      supersets are inadmissible and are rightly skipped.
//  (3) Deficiency is monotone: def(X) = max_S (2|S| - |N(S) \ X|) and X <= X' imply
//      def(X) <= def(X').  Therefore a node X of size < k is covered in two ways: it is itself a
//      node whose pairing is maintained (so a violator of size < k is found at the node where it
//      first arises, and its admissible supersets need not be searched - they are violators
//      too), and whenever the matcher holds a pairing for X u {c} the same assignment is a
//      pairing for X.  At every visited node the matcher holds a pairing for the current X or the
//      search stops with X as the violator.
//  (4) Incremental repair is exact.  The matcher holds a pairing for X; the new stone c either is
//      unassigned (the same assignment is a pairing for X u {c}) or is owned by one line l, which
//      then misses one cell.  A pairing for X u {c} exists iff there is an alternating path from l
//      to an unassigned cell (max-flow / Berge); if the exhaustive search fails, the reached lines
//      R satisfy |N(R) \ (X u {c})| = 2|R| - 1, a Hall violator, which is returned in S.
//  (5) Last stone: if c is unassigned nothing has to be checked (no children, pairing unchanged),
//      so such placements are not even counted as nodes.
// If F itself is not B-admissible or |F| > k, no X exists and C holds vacuously (status 1).
// ---------------------------------------------------------------------------------------------
struct EnumResult {
  int status;           // 1: C(B,F,k) holds; 0: fails (X = admissible violator, S = Hall-violating lines); -1: node limit hit
  CellSet X;
  std::vector<int> S;
  uint64_t nodes;       // number of stone sets whose pairing was examined / repaired
};

namespace detail {

struct EnumCtx {
  Matcher M;
  int8_t cnt[NL];        // |X n l| for live lines (dead lines hold a large negative number)
  CellSet X;
  int ncand;
  uint8_t cand[NC];      // candidate cells, ascending
  uint64_t nodes, limit;
  int status;
  std::vector<uint32_t> log;

  // May the stone c be added to the admissible set X (c not in X, B)?
  bool can_place(int c) const {
    const Geom& g = M.g;
    const int n = g.nlc[c];
    const int* ls = g.lc[c];
    bool three = false;
    for (int i = 0; i < n; i++) {
      int v = cnt[ls[i]];
      if (v >= 4) return false;        // cap
      if (v == 3) three = true;
    }
    if (!three) return true;
    for (int i = 0; i < n; i++) {
      int a = ls[i];
      if (cnt[a] != 3) continue;       // a becomes a line with exactly 4 stones
      for (int j = 0; j < 6; j++) {
        int x = g.line[a][j];
        if (x == c || X.test(x)) continue;
        for (int q = 0; q < g.nlc[x]; q++) { int b = g.lc[x][q]; if (b != a && cnt[b] == 4) return false; }
      }
    }
    return true;
  }

  // Returns true when the search must stop (violator found or node limit hit).
  bool rec(int start, int left) {
    const Geom& g = M.g;
    for (int ci = start; ci < ncand; ci++) {
      int c = cand[ci];
      int o = M.owner[c];
      if (left == 1 && o == FREE) continue;          // (5)
      if (!can_place(c)) continue;                   // (2)
      nodes++;
      if (limit && nodes > limit) { status = -1; return true; }
      X.set(c);
      for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]]++;
      uint32_t* mark = M.logp;
      *M.logp++ = (uint32_t(c) << 16) | uint16_t(o);
      if (o == FREE) M.take(c, BLOCKED); else M.owner[c] = BLOCKED;
      if (o >= 0 && !M.gain1L(o)) { status = 0; return true; }   // (4): X is the violator
      if (left > 1 && rec(ci + 1, left - 1)) return true;
      M.undo_to(mark);
      for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]]--;
      X.reset(c);
    }
    return false;
  }
};

}  // namespace detail

inline EnumResult enum_check(const CellSet& B, const CellSet& F, int k, uint64_t node_limit = 0) {
  EnumResult R;
  R.status = 1;
  R.nodes = 0;
  int nf = F.count();
  if (nf > k || !admissible(B, F)) return R;          // no admissible X at all: vacuously true
  const Geom& g = G();
  detail::EnumCtx E;
  E.M.init(B, F);
  E.X = F;
  E.nodes = 1;
  E.limit = node_limit;
  E.status = 1;
  int bad = E.M.build();
  if (bad >= 0) {
    R.status = 0;
    R.X = F;
    E.M.reached(R.S);
    R.nodes = 1;
    if (detail::surplus_of(R.S, R.X) >= 0) { fprintf(stderr, "match.hpp: internal error (bad Hall violator)\n"); abort(); }
    return R;
  }
  int depth = k - nf;
  if (depth > 0) {
    for (int l = 0; l < NL; l++) E.cnt[l] = E.M.dead[l] ? (int8_t)-64 : (int8_t)0;
    F.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) E.cnt[g.lc[c][i]]++; });
    CellSet N;
    for (int i = 0; i < E.M.nl; i++) N = N | g.lineSet[E.M.lines[i]];
    N = N.minus(F);
    E.ncand = 0;
    N.for_each([&](int c) { E.cand[E.ncand++] = (uint8_t)c; });
    if (depth > E.ncand) depth = E.ncand;
    E.log.resize((size_t)(depth + 1) * (NL + 4));
    E.M.logp = E.log.data();
    if (depth > 0) E.rec(0, depth);
  }
  R.status = E.status;
  R.nodes = E.nodes;
  if (E.status == 0) {
    R.X = E.X;
    E.M.reached(R.S);
    if (detail::surplus_of(R.S, R.X) >= 0 || !admissible(B, R.X) || R.X.count() > k || !F.subset_of(R.X)) {
      fprintf(stderr, "match.hpp: internal error (bad enum violator)\n");
      abort();
    }
  }
  return R;
}

}  // namespace ttt

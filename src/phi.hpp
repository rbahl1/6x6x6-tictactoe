// phi.hpp -- SPEC 7.3: the predicate LEAF(B,F,k) of SPEC 3, decided exactly by integer max-flow.
// Header-only, namespace ttt, thread-safe (all state on the caller's stack, no heap allocation
// in leaf_holds).
//
// LEAF(B,F,k), theta = k - |F|:
//   1. F is B-admissible;
//   2. a pairing for (B,F) exists;
//   3. every F-connected set S of >= 3 live lines has sur_F(S) = |N(S) \ F| - 2|S| >= theta.
//
// How 3 is decided (SPEC 3, facts).  Keep an assignment in which every live line owns 2 cells
// (a pairing for (B,F)).  For a set T of lines let mu(T) = min over S >= T of sur_F(S); it is the
// number of extra cells the lines of T can collect when their capacity is unbounded (max-flow =
// min-cut: a cut is a set S >= T of lines together with N(S) \ F).  Condition 3 holds iff
// mu(T) >= theta for every F-connected triple T (every F-connected S with >= 3 lines contains an
// F-connected triple; conversely a minimising S >= T may be replaced by its F-connected component
// that contains T, because the other components have surplus >= 0 by Hall).
//
// Facts used by the implementation:
//  (a) mu is monotone: T <= T' implies mu(T) <= mu(T').  So a line with mu({l}) >= theta ("safe
//      line") or a pair with mu({a,b}) >= theta ("safe pair") makes every triple containing it
//      safe.
//  (b) When a search for one more cell from a root fails, the set R of reached lines is closed:
//      every non-F cell of N(R) is owned by a line of R.  Hence sur_F(R) = number of extra cells
//      held by the lines of R, and R is F-connected (each reached line shares a non-F cell with
//      the line it was reached from).  If |R| >= 3 and that surplus is < theta, R is a witness
//      against 3.  Every "false" returned with reason 3 is backed by such a set S, and its size,
//      F-connectivity and surplus are recomputed from the definition before returning.
//  (c) A closed set stays closed while other roots are augmented (their paths cannot enter it),
//      so the extra cells of a triple can be collected line by line: a first, then b, then c;
//      mu({a,b,c}) is the total.  Releasing the extra cells of the last line returns to a
//      maximum flow for the remaining ones, so no state has to be copied.  (This is used for
//      speed only: before "false" is returned for a triple, one joint search from all three
//      lines certifies that no augmenting path is left.)
//  (d) "true" is only returned after every F-connected triple of unsafe lines has actually
//      collected theta extra cells (an explicit flow), or was covered by (a).  When exactly one
//      more cell is needed, the flow is not carried out: it suffices that the line reaches an
//      unassigned cell along an alternating path (can_reach, memoised while the assignment is
//      unchanged).
//  (e) Every F-connected triple is examined exactly once, at its lexicographically smallest
//      F-adjacent pair (a,b), a < b: the third line c is F-adjacent to a or to b, and
//      {a,b} is the smallest pair iff c > a and not (c F-adjacent to a and c < b).
#pragma once
#include "match.hpp"

namespace ttt {

struct PhiResult {
  bool holds;
  int reason;            // 0: LEAF holds. 1: F not B-admissible. 2: no pairing for (B,F) (S = Hall violator).
                         // 3: S = an F-connected set of >= 3 live lines with sur_F(S) = sur < k-|F|.
  std::vector<int> S;
  int sur;               // reason 3: sur_F(S); reason 2: sur_F(S) (negative); reason 0: k-|F|; reason 1: 0
};

namespace detail {

struct LineMask {
  uint64_t w[3];
  LineMask() : w{0, 0, 0} {}
  void set(int l) { w[l >> 6] |= uint64_t(1) << (l & 63); }
  bool test(int l) const { return (w[l >> 6] >> (l & 63)) & 1; }
  int count() const { return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) + __builtin_popcountll(w[2]); }
  bool operator==(const LineMask& o) const { return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2]; }
};

// The common cell of two lines, or -1.
struct LineX {
  int16_t x[NL][NL];
  LineX() {
    const Geom& g = G();
    for (int a = 0; a < NL; a++) for (int b = 0; b < NL; b++) x[a][b] = -1;
    for (int c = 0; c < NC; c++)
      for (int i = 0; i < g.nlc[c]; i++) for (int j = 0; j < g.nlc[c]; j++)
        if (i != j) x[g.lc[c][i]][g.lc[c][j]] = (int16_t)c;
  }
};
inline const LineX& LX() { static LineX t; return t; }

struct PhiCore {
  Matcher M;
  CellSet F;
  int theta;
  int reason, sur;
  LineMask S;
  uint32_t canst_[NL + 2];   // canst(l) == cstamp: l reaches an unassigned cell in the current assignment
  uint32_t cstamp;           // bumped whenever the assignment changes

  PhiCore() : theta(0), reason(0), sur(0), cstamp(0) {}
  uint32_t& canst(int l) { return canst_[l + 2]; }

  LineMask reached() const {
    LineMask R;
    for (int i = 0; i < M.nl; i++) if (M.lvis(M.lines[i]) == M.stamp) R.set(M.lines[i]);
    return R;
  }
  // sur_F(R), straight from the definition
  int sur_of(const LineMask& R) const {
    CellSet N;
    int n = 0;
    for (int l = 0; l < NL; l++) if (R.test(l)) { N = N | M.g.lineSet[l]; n++; }
    return N.minus(F).count() - 2 * n;
  }
  // F-connected component of l0 inside the live lines allowed by `within` (nullptr = all live lines)
  LineMask component(int l0, const LineMask* within) const {
    const Geom& g = M.g;
    LineMask R;
    int st[NL], n = 0;
    R.set(l0);
    st[n++] = l0;
    while (n) {
      int l = st[--n];
      for (int j = 0; j < 6; j++) {
        int c = g.line[l][j];
        if (F.test(c)) continue;
        for (int q = 0; q < g.nlc[c]; q++) {
          int a = g.lc[c][q];
          if (M.dead[a] || R.test(a) || (within && !within->test(a))) continue;
          R.set(a);
          st[n++] = a;
        }
      }
    }
    return R;
  }
  bool fconnected(const LineMask& R) const {
    for (int l = 0; l < NL; l++) if (R.test(l)) return component(l, &R) == R;
    return true;
  }
  // A live line with at least two F-neighbours (the centre of an F-connected triple), or -1.
  int triple_centre() const {
    const Geom& g = M.g;
    for (int i = 0; i < M.nl; i++) {
      int b = M.lines[i], n = 0;
      for (int j = 0; j < 6; j++) {
        int c = g.line[b][j];
        if (M.owner[c] == BLOCKED) continue;
        for (int q = 0; q < g.nlc[c]; q++) { int a = g.lc[c][q]; n += (a != b && !M.dead[a]); }
      }
      if (n >= 2) return b;
    }
    return -1;
  }
  // Report the witness R against condition 3 (verified from the definition).
  bool fail3(const LineMask& R) {
    int n = R.count(), s = sur_of(R);
    bool live = true;
    for (int l = 0; l < NL; l++) if (R.test(l) && M.dead[l]) live = false;
    if (n < 3 || s >= theta || !live || !fconnected(R)) {
      fprintf(stderr, "phi.hpp: internal error (bad witness: %d lines, sur %d, theta %d)\n", n, s, theta);
      abort();
    }
    reason = 3;
    sur = s;
    S = R;
    return false;
  }
  // Give back the extra cells of line l (keep 2).
  void release(int l) {
    ++cstamp;
    const int* cs = M.g.line[l];
    unsigned m = 0;
    for (int j = 0; j < 6; j++) m |= unsigned(M.owner[cs[j]] == l) << j;
    m &= m - 1;
    m &= m - 1;
    while (m) { M.set_free(cs[__builtin_ctz(m)]); m &= m - 1; }
  }
  // One more cell for line l (changes the assignment).
  bool gain(int l) { ++cstamp; return M.gain1(l); }

  // Does line l reach an unassigned cell along an alternating path (i.e. would gain(l) succeed)?
  // No change of the assignment.  Lines on a successful path are remembered until the
  // assignment changes.  The caller has stamped l in M.
  bool reach(int l) {
    const Geom& g = M.g;
    if (M.fm(l) != 0 || canst(l) == cstamp) { canst(l) = cstamp; return true; }
    const int* cs = g.line[l];
    int o[6];
    unsigned cand2 = 0, cand3 = 0;
    for (int i = 0; i < 6; i++) {
      int oi = M.owner[cs[i]];
      o[i] = oi;
      unsigned ok = unsigned(oi >= 0) & unsigned(oi != l) & unsigned(M.lvis(oi) != M.stamp);
      cand3 |= ok << i;
      cand2 |= (ok & (unsigned(M.fm(oi) != 0) | unsigned(canst(oi) == cstamp))) << i;
    }
    if (cand2) { canst(o[__builtin_ctz(cand2)]) = cstamp; canst(l) = cstamp; return true; }
    while (cand3) {
      int i = __builtin_ctz(cand3), oi = o[i];
      cand3 &= cand3 - 1;
      if (M.lvis(oi) == M.stamp) continue;
      M.lvis(oi) = M.stamp;
      M.nvis++;
      if (reach(oi)) { canst(l) = cstamp; return true; }
    }
    return false;
  }
  bool can_reach(int l) {
    if (canst(l) == cstamp) return true;
    ++M.stamp;
    M.lvis(l) = M.stamp;
    M.nvis = 1;
    return reach(l);
  }

  // Decide LEAF(B,F,k).  fast = true: the answer only (reason may be 2 or 3 without a witness
  // when the cheap counting rejection applies); fast = false: reason and witness as in SPEC 7.3.
  bool run(const CellSet& B, const CellSet& F_, int k, bool fast) {
    F = F_;
    theta = k - F.count();
    reason = 0;
    sur = 0;
    S = LineMask();
    if (!admissible(B, F)) { reason = 1; return false; }
    M.init(B, F);
    const Geom& g = M.g;
    // number of available cells minus 2 per live line = number of cells any pairing leaves unassigned
    CellSet N;
    for (int i = 0; i < M.nl; i++) N = N | g.lineSet[M.lines[i]];
    const int tfree = N.minus(F).count() - 2 * M.nl;
    if (fast && theta > 0 && tfree < theta) {
      if (tfree < 0) { reason = 2; return false; }                 // Hall fails for S = L(B)
      // If a pairing exists, every F-connected component K has sur_F(K) = its unassigned cells
      // <= tfree < theta; so any component with >= 3 lines refutes 3.  If no pairing exists, 2 fails.
      if (triple_centre() >= 0) { reason = 3; return false; }
    }
    if (M.build() >= 0) {
      reason = 2;
      S = reached();
      sur = sur_of(S);
      if (sur >= 0) { fprintf(stderr, "phi.hpp: internal error (bad Hall violator)\n"); abort(); }
      return false;
    }
    if (theta <= 0) { sur = theta; return true; }
    memset(canst_, 0, sizeof canst_);
    cstamp = 1;
    if (tfree < theta) {
      int l = triple_centre();
      if (l < 0) { sur = theta; return true; }                     // no F-connected triple at all
      return fail3(component(l, nullptr));
    }

    // ---- level 1a: lines that are unsafe for a static reason, sur_F({l}) = |l \ F| - 2 < theta
    //      (for theta >= 5 that is every line)
    uint8_t unsafe[NL];
    uint8_t todo[NL];
    int nunsafe = 0, ntodo = 0;
    memset(unsafe, 0, sizeof unsafe);
    for (int i = 0; i < M.nl; i++) {
      int l = M.lines[i];
      const int* cs = g.line[l];
      int nav = 0;
      for (int j = 0; j < 6; j++) nav += (M.owner[cs[j]] != BLOCKED);
      if (nav - 2 < theta) { unsafe[l] = 1; nunsafe++; }
      else todo[ntodo++] = (uint8_t)l;
    }
    // ---- triples of statically unsafe lines first: they are cheap and catch local violations
    if (nunsafe >= 3 && !triples(unsafe)) return false;
    // ---- level 1b: mu({l}) >= theta for the remaining lines (theta <= 4 here)
    int nflow = 0;
    for (int i = 0; i < ntodo; i++) {
      int l = todo[i];
      if (__builtin_popcount(M.fm(l)) >= theta) continue;             // theta unassigned cells on l itself
      if (theta == 1 && can_reach(l)) continue;
      int got = 0;
      while (got < theta && gain(l)) got++;
      if (got < theta) {
        if (M.nvis >= 3) return fail3(reached());                     // (b)
        unsafe[l] = 1;
        nflow++;
      }
      release(l);
    }
    // ---- lines that turned out unsafe by flow (rare): redo the triples with the full unsafe set
    if (nflow > 0 && nunsafe + nflow >= 3 && !triples(unsafe)) return false;
    sur = theta;
    return true;
  }

  // Third line c of a triple {a,b,c}; a and b hold their maximum.  Returns false with a
  // verified witness if the three lines cannot collect need3 more cells.
  bool third(int a, int b, int c, int need3) {
    if (need3 == 1 && can_reach(c)) return true;
    int gc = 0;
    for (;;) {
      while (gc < need3 && gain(c)) gc++;
      if (gc >= need3) break;
      // c is exhausted: one joint search from all three lines certifies the maximum
      ++cstamp;
      ++M.stamp;
      M.lvis(a) = M.lvis(b) = M.lvis(c) = M.stamp;
      M.nvis = 3;
      if (M.aug(c) || M.aug(a) || M.aug(b)) { gc++; continue; }   // (not expected, see (c))
      return fail3(reached());
    }
    release(c);
    return true;
  }

  // Levels 2 and 3: every F-connected triple of lines marked unsafe collects theta extra cells,
  // or the search stops with a verified witness (returns false).  See (e) for the enumeration.
  // Lines not marked are never members of a triple here; marking a safe line is harmless.
  bool triples(const uint8_t* unsafe) {
    const Geom& g = M.g;
    const LineX& lx = LX();
    for (int ai = 0; ai < M.nl; ai++) {
      int a = M.lines[ai];
      if (!unsafe[a]) continue;
      uint8_t na[48];                      // unsafe F-neighbours of a
      int nna = 0, nbig = 0;
      const int* ca = g.line[a];
      for (int j = 0; j < 6; j++) {
        int x = ca[j];
        if (M.owner[x] == BLOCKED) continue;
        for (int q = 0; q < g.nlc[x]; q++) {
          int l = g.lc[x][q];
          if (l != a && !M.dead[l] && unsafe[l]) { na[nna++] = (uint8_t)l; nbig += (l > a); }
        }
      }
      if (nbig == 0) continue;             // a is in no pair (a,b) with a < b
      int fa = 0;
      while (fa < theta && gain(a)) fa++;
      if (fa >= theta) { release(a); continue; }                        // a is safe after all
      if (M.nvis >= 3) return fail3(reached());
      const int need = theta - fa;
      for (int i = 0; i < nna; i++) {
        int b = na[i];
        if (b < a) continue;
        if (need == 1 && can_reach(b)) continue;                        // safe pair
        int gb = 0;
        while (gb < need && gain(b)) gb++;
        if (gb >= need) { release(b); continue; }                       // safe pair
        if (M.nvis >= 3) return fail3(reached());
        const int need3 = need - gb;
        // third lines that are F-adjacent to a
        for (int j = 0; j < nna; j++) {
          int c = na[j];
          if (c <= b) continue;            // c < a, c == b, or the pair (a,c) is smaller than (a,b)
          if (!third(a, b, c, need3)) return false;
        }
        // third lines that are F-adjacent to b but not to a
        const int* cb = g.line[b];
        for (int j = 0; j < 6; j++) {
          int x = cb[j];
          if (M.owner[x] == BLOCKED) continue;
          for (int q = 0; q < g.nlc[x]; q++) {
            int c = g.lc[x][q];
            if (c <= a || c == b || M.dead[c] || !unsafe[c]) continue;  // c < a: done at a smaller pair
            int y = lx.x[a][c];
            if (y >= 0 && M.owner[y] != BLOCKED) continue;              // F-adjacent to a: handled above
            if (!third(a, b, c, need3)) return false;
          }
        }
        release(b);
      }
      release(a);
    }
    return true;
  }
};

}  // namespace detail

// LEAF(B,F,k) with reason and witness.
inline PhiResult phi_check(const CellSet& B, const CellSet& F, int k) {
  detail::PhiCore P;
  PhiResult R;
  R.holds = P.run(B, F, k, false);
  R.reason = P.reason;
  R.sur = P.sur;
  for (int l = 0; l < NL; l++) if (P.S.test(l)) R.S.push_back(l);
  return R;
}

// LEAF(B,F,k), answer only.
inline bool leaf_holds(const CellSet& B, const CellSet& F, int k) {
  detail::PhiCore P;
  return P.run(B, F, k, true);
}

// min(cap, min of sur_F(S) over F-connected sets S of >= 3 live lines).  Requires F B-admissible
// and a pairing for (B,F); returns -1 if that precondition is violated.
inline int phi_value(const CellSet& B, const CellSet& F, int cap) {
  int nf = F.count();
  int v = cap;
  for (;;) {
    detail::PhiCore P;
    if (P.run(B, F, nf + v, false)) return v;
    if (P.reason != 3) return -1;
    v = P.sur;   // a set with surplus sur < v exists; is sur the minimum?
  }
}

}  // namespace ttt

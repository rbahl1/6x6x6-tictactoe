// certify -- task 6b: an EXACT decision procedure for C(B,F,k) (SPEC 3):
//     "every B-admissible X with F <= X and |X| <= k has a pairing".
// Integer arithmetic only (bitsets, counters, augmenting paths); no floating point anywhere in
// the decision (timing statistics aside).
//
// Reads instance lines "k B F" from stdin and prints one JSON line per instance:
//   {"k":..,"B":"..","F":"..","decision":"holds"|"fails"|"undecided"|"void",
//    "method":"phi"|"witness"|"enum"|"bnb","nodes":n,"X":"..","S":[[6 cells],..],
//    "leaf":true|false,"reason":r,"theta":k-|F|}          (+ "us":microseconds with --stats)
//   decision "fails": X is a B-admissible set with F <= X, |X| <= k and no pairing, S is a set of
//                     live lines with |N(S) \ X| < 2|S|.  Both are re-verified with the exact
//                     library predicates (admissible, find_pairing) and from the definition
//                     before the line is printed; if that check fails the program aborts.
//   decision "void":  F is not B-admissible (or meets B, or |F| > k): there is no admissible X at
//                     all and C holds vacuously.  Reported separately, method "phi" (reason 1).
//   "leaf"/"reason":  LEAF(B,F,k) and the reason code of phi_check (SPEC 7.3).
//   "nodes":          stone placements tried by the witness search of step 2, plus the nodes of
//                     enum_check or of the branch and bound (stone sets whose pairing was examined).
//
// The procedure:
//   1. LEAF(B,F,k) true (phi_check)                                  -> holds / phi
//      (LEAF => C is SPEC 3, proved in PROOF.md.)
//   2. LEAF false.  Reason 2 (no pairing for (B,F)): X = F itself    -> fails / witness.
//      Reason 3 (S F-connected, >= 3 lines, sur_F(S) = s < k-|F|): depth-first search for s+1
//      stones on N(S) \ F such that F + stones is admissible (caps and danger rule on ALL live
//      lines).  Then |N(S) \ X| = 2|S| - 1.  Found                   -> fails / witness.
//   3. Otherwise, if k-|F| <= 3: enum_check (match.hpp)              -> holds / enum, fails / enum.
//   4. Otherwise a complete branch and bound over the free stones    -> holds / bnb, fails / bnb,
//      or "undecided" when the node limit (--max-nodes, default 2000000) is hit.
//
// ---------------------------------------------------------------------------------------------
// Why the branch and bound is exact.
//
// Notation: X admissible, r >= 1.  sur_X(S) = |N(S) \ X| - 2|S|.  mu_X(T) = min over S >= T of
// sur_X(S).  A set of live lines is X-connected iff it is connected through common cells outside X.
//
// FACT.  Let X be B-admissible with a pairing for (B,X).  Let Y be disjoint from X and B, X+Y
// B-admissible, (B,X+Y) without a pairing, and Y inclusion-minimal with these properties.  Then
// there is an X-connected set S of >= 3 live lines with Y <= N(S) and sur_X(S) = |Y| - 1.
// Consequently every cell of Y lies on a line that belongs to an X-connected triple T of live
// lines with mu_X(T) <= |Y| - 1.
//
// Proof.  Put X' = X+Y.  No pairing: by Hall there is S0 with |N(S0) \ X'| < 2|S0|.  Split S0
// into its X'-connected components S1..Sm.  Two different components share no cell outside X'
// (they would be X'-adjacent), so the sets N(Si) \ X' are pairwise disjoint and
// sum_i (|N(Si) \ X'| - 2|Si|) < 0: some component S = Si has |N(S) \ X'| <= 2|S| - 1.
// |S| = 1 is impossible (cap: a live line keeps >= 2 cells outside X').  |S| = 2 is impossible:
// the two lines a,b share a cell c outside X', so |N(S) \ X'| = |a \ X'| + |b \ X'| - 1 >= 3,
// and = 3 only if both lines carry exactly 4 stones, a danger pair.  Hence |S| >= 3, and S is
// X-connected because X <= X'.  For y in Y the set X' - y is admissible (subset) and, by
// minimality of Y, has a pairing, so |N(S) \ (X' - y)| >= 2|S|.  As |N(S) \ (X' - y)| =
// |N(S) \ X'| + [y in N(S)], every y lies in N(S) and |N(S) \ X'| = 2|S| - 1.  Therefore
// sur_X(S) = |N(S) \ X'| + |Y| - 2|S| = |Y| - 1.  Finally, in a connected graph with >= 3
// vertices every vertex lies in a connected 3-subset; so each line l of S lies in an X-connected
// triple T <= S, and mu_X(T) <= sur_X(S) = |Y| - 1.                                          qed
//
// NODE (X, r, Z): X admissible with a pairing, r >= 1 stones left, Z a set of forbidden cells.
// Question Q(X,r,Z): is there Y, disjoint from X, B and Z, |Y| <= r, X+Y admissible, (B,X+Y)
// without a pairing?  C(B,F,k) fails iff (B,F) has no pairing or Q(F, k-|F|, {}) is true
// (F admissible, k > |F|).
//
// Candidate cells of the node: the cells, not in X or Z, of the "marked" lines = live lines that
// belong to an X-connected set S of >= 3 live lines with sur_X(S) <= r-1 (equivalently: to an
// X-connected triple T with mu_X(T) <= r-1; for "=>" take a triple inside S, for "<=" take a
// minimiser S >= T and drop the X-components that do not contain T, each has surplus >= 0 by
// Hall).  Let c1..cm be the candidates with X+ci admissible, in any fixed order.
//
// CLAIM.  Q(X,r,Z) is true iff for some i: (B,X+ci) has no pairing, or r > 1 and
// Q(X+ci, r-1, Z + {c1..c(i-1)}) is true.
// "<=" is trivial (the Y of the child plus ci; the admissible X+ci itself in the first case).
// "=>": take Y as in Q and shrink it to an inclusion-minimal Y' (non-empty since X has a
// pairing; subsets stay admissible and disjoint from Z).  By the FACT (|Y'| <= r) every cell of
// Y' lies on a marked line, and X+y is admissible for y in Y', so Y' <= {c1..cm}.  Let ci be its
// first element in the order.  If Y' = {ci} then (B,X+ci) has no pairing.  Otherwise r > 1,
// (B,X+ci) has a pairing by minimality, and Y'' = Y' - ci avoids Z + {c1..c(i-1)}, has
// <= r-1 cells, X+ci+Y'' is admissible without a pairing: Q(X+ci, r-1, Z+{c1..c(i-1)}).      qed
// Candidates with X+ci inadmissible are skipped with all their supersets: inadmissibility is
// upward closed (a subset of an admissible set is admissible).
//
// Two exact shortcuts on top of the CLAIM:
//  (R) relaxation.  All Y' above lie inside Q = {c1..cm}.  If (B, X+Q) has a pairing, the same
//      assignment is a pairing for every X+Y', Y' <= Q, so Q(X,r,Z) is false.
//  (L) last stone, r = 1.  For a pairing of (B,X) and a cell c not in X: (B,X+c) has no pairing
//      iff c is assigned to a line o that cannot gain a third cell by an alternating path.
//      (If c is unassigned the pairing survives.  If o can gain a cell, the path does not use c
//      -- it only passes through cells owned by other lines or free -- so after removing c the
//      line o is repaired.  If o cannot gain, removing c leaves o with one cell and no augmenting
//      path, and a maximum matching is deficient.)  So Q(X,1,Z) is true iff such a c exists
//      outside Z with X+c admissible; no marking is needed.
//
// What is certified by what:
//  * "fails" never rests on the search: the final X is re-checked with admissible() and
//    find_pairing() and the Hall set S from the definition.
//  * "holds" by bnb rests on (i) the completeness of the marking and (ii) the CLAIM.  A line is
//    left unmarked only if every X-connected triple through it is "safe", and a triple is
//    declared safe only with an explicit flow in hand: the matcher holds an assignment in which
//    every live line owns >= 2 cells outside X and the lines of the triple (or one or two of
//    them: mu is monotone in T) own r extra cells; then |N(S) \ X| >= 2|S| + r for every S >= T.
//    A line is marked only together with an explicit set S (a closed set of a failed
//    augmenting search) whose size, liveness, X-connectivity and surplus <= r-1 are recomputed
//    from the definition.  Every X-connected triple is visited exactly once, at its
//    lexicographically smallest X-adjacent pair (a,b), a < b (same scheme as phi.hpp (e)): the
//    third line c is X-adjacent to a with c > b, or X-adjacent to b, not to a, with c > a.
//
// Options:
//   --max-nodes N   node limit of the branch and bound (default 2000000; 0 = unlimited)
//   --stats         add "us" to every record and print per-method timing to stderr at the end
//   --no-witness    skip the witness search of step 2 for reason 3 (testing)
//   --force-bnb     use the branch and bound instead of enum_check for k-|F| <= 3 (testing)
//   --bnb-only      skip steps 1-3 altogether: the branch and bound decides everything (testing;
//                   LEAF is still computed for the "leaf" field)
//   --no-relax      switch off shortcut (R) (testing)
//   --paranoid      extra internal consistency checks (slow)
//   --check-marker N  compare the marked lines of the first N markings of every instance with a
//                   naive reference (fresh matcher per X-connected triple; very slow, testing)
//   --quiet         omit S
#include "phi.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
using namespace ttt;
using ttt::detail::BLOCKED;
using ttt::detail::FREE;
using ttt::detail::Matcher;

static void die(const char* msg) {
  fprintf(stderr, "certify: internal error: %s\n", msg);
  abort();
}

// ---------------------------------------------------------------------------------------------
// An admissible stone set with its per-line counters.
// ---------------------------------------------------------------------------------------------
struct Stones {
  CellSet X;
  int8_t cnt[NL];   // |X n l| for live lines; dead lines stay negative

  void init(const CellSet& B, const CellSet& X0) {
    const Geom& g = G();
    memset(cnt, 0, sizeof cnt);
    B.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]] = -64; });
    X = X0;
    X0.for_each([&](int c) { for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]]++; });
  }
  // X admissible, c not in X or B.  Is X+c admissible?
  // Cap: no live line through c has 4 stones already.  Danger: a new danger pair needs a line a
  // through c that goes from 3 to 4 stones and a line b with 4 stones meeting a in a cell
  // x != c outside X (b does not contain c: two lines through c meet only in c, which is in X+c).
  bool can_place(int c) const {
    const Geom& g = G();
    const int n = g.nlc[c];
    const int* ls = g.lc[c];
    bool three = false;
    for (int i = 0; i < n; i++) {
      int v = cnt[ls[i]];
      if (v >= 4) return false;
      if (v == 3) three = true;
    }
    if (!three) return true;
    for (int i = 0; i < n; i++) {
      int a = ls[i];
      if (cnt[a] != 3) continue;
      for (int j = 0; j < 6; j++) {
        int x = g.line[a][j];
        if (x == c || X.test(x)) continue;
        for (int q = 0; q < g.nlc[x]; q++) { int b = g.lc[x][q]; if (b != a && cnt[b] == 4) return false; }
      }
    }
    return true;
  }
  void place(int c) { const Geom& g = G(); X.set(c); for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]]++; }
  void unplace(int c) { const Geom& g = G(); X.reset(c); for (int i = 0; i < g.nlc[c]; i++) cnt[g.lc[c][i]]--; }
};

// ---------------------------------------------------------------------------------------------
// Step 2: search for `need` stones on the given cells such that the stone set stays admissible.
// ---------------------------------------------------------------------------------------------
struct WitnessSearch {
  Stones st;
  std::vector<int> cand;
  uint64_t nodes, limit;

  bool rec(size_t start, int left) {
    if (left == 0) return true;
    for (size_t i = start; i + (size_t)left <= cand.size(); i++) {
      int c = cand[i];
      if (!st.can_place(c)) continue;
      if (++nodes > limit) return false;
      st.place(c);
      if (rec(i + 1, left - 1)) return true;
      st.unplace(c);
      if (nodes > limit) return false;
    }
    return false;
  }
  // Tries to add `need` cells of `pool` to F.  On success st.X is the admissible result.
  bool run(const CellSet& B, const CellSet& F, const CellSet& pool, int need, uint64_t lim) {
    const Geom& g = G();
    st.init(B, F);
    nodes = 0;
    limit = lim;
    cand.clear();
    pool.for_each([&](int c) { if (!F.test(c) && !B.test(c)) cand.push_back(c); });
    // least constrained cells first: few stones on the fullest live line through the cell,
    // then few live lines through the cell, then the cell id
    auto key = [&](int c) {
      int mx = 0, nlive = 0;
      for (int i = 0; i < g.nlc[c]; i++) { int v = st.cnt[g.lc[c][i]]; if (v >= 0) { nlive++; if (v > mx) mx = v; } }
      return mx * 1000000 + nlive * 1000 + c;
    };
    std::sort(cand.begin(), cand.end(), [&](int a, int b) { return key(a) < key(b); });
    return rec(0, need);
  }
};

// ---------------------------------------------------------------------------------------------
// Marked lines of a node: live lines that belong to an X-connected set S of >= 3 live lines with
// sur_X(S) <= r-1.  The matcher must hold a pairing for (B,X) on entry and holds one on exit.
// ---------------------------------------------------------------------------------------------
struct Marker {
  Matcher& M;
  const CellSet& X;
  const int r;
  const bool paranoid;
  uint8_t mark[NL];      // 1 = member of an explicit X-connected S, |S| >= 3, sur_X(S) <= r-1
  int16_t val[NL];       // smallest such surplus seen for the line (ordering heuristic only)
  uint8_t us[NL];        // 1 = live line in an X-component with >= 3 lines, not certified safe on its own
  uint8_t nb[NL][40];    // live X-neighbours of each live line (ascending)
  uint8_t nnb[NL];
  int nmarked;
  std::vector<int> R;

  Marker(Matcher& m, const CellSet& x, int r_, bool par) : M(m), X(x), r(r_), paranoid(par), nmarked(0) {}

  int ext(int l) const {                       // cells owned by l beyond its 2
    const int* cs = M.g.line[l];
    int n = 0;
    for (int j = 0; j < 6; j++) n += (M.owner[cs[j]] == l);
    return n - 2;
  }
  void release(int l) {                        // give back the extra cells of l (keep 2)
    const int* cs = M.g.line[l];
    unsigned m = 0;
    for (int j = 0; j < 6; j++) m |= unsigned(M.owner[cs[j]] == l) << j;
    m &= m - 1;
    m &= m - 1;
    while (m) { M.set_free(cs[__builtin_ctz(m)]); m &= m - 1; }
  }
  bool adjacent(int a, int c) const {          // X-adjacent
    int y = detail::LX().x[a][c];
    return y >= 0 && !X.test(y);
  }
  // The matcher state is a valid assignment: every owned cell lies on its owner, is outside X,
  // every live line owns >= 2 cells, only the listed lines own more.
  void check_state(int a, int b, int c) const {
    const Geom& g = M.g;
    int own[NL];
    memset(own, 0, sizeof own);
    for (int x = 0; x < NC; x++) {
      int o = M.owner[x];
      if (o == FREE) { if (X.test(x)) die("free cell in X"); continue; }
      if (o == BLOCKED) continue;
      if (o < 0 || o >= NL || M.dead[o] || !g.lineSet[o].test(x) || X.test(x)) die("bad owner");
      own[o]++;
    }
    for (int i = 0; i < M.nl; i++) {
      int l = M.lines[i];
      if (own[l] < 2) die("line with < 2 cells");
      if (own[l] > 2 && l != a && l != b && l != c) die("unexpected extra cells");
    }
  }
  // Mark the lines of S after verifying, from the definition, that S is an X-connected set of
  // >= 3 live lines with sur_X(S) = s <= r-1.
  void markset(const std::vector<int>& S, int s) {
    const Geom& g = M.g;
    if ((int)S.size() < 3 || s > r - 1) die("markset: not a witness");
    if (detail::surplus_of(S, X) != s) die("markset: surplus mismatch");
    uint8_t in[NL], seen[NL];
    memset(in, 0, sizeof in);
    memset(seen, 0, sizeof seen);
    for (int l : S) { if (l < 0 || l >= NL || M.dead[l] || in[l]) die("markset: bad line"); in[l] = 1; }
    int st[NL], n = 0, got = 1;
    seen[S[0]] = 1;
    st[n++] = S[0];
    while (n) {
      int l = st[--n];
      for (int j = 0; j < 6; j++) {
        int c = g.line[l][j];
        if (X.test(c)) continue;
        for (int q = 0; q < g.nlc[c]; q++) {
          int a = g.lc[c][q];
          if (in[a] && !seen[a]) { seen[a] = 1; st[n++] = a; got++; }
        }
      }
    }
    if (got != (int)S.size()) die("markset: not X-connected");
    for (int l : S) {
      if (!mark[l]) { mark[l] = 1; nmarked++; }
      if (s < val[l]) val[l] = (int16_t)s;
    }
  }
  // After a failed search: the reached lines form a closed X-connected set.
  void mark_reached() {
    M.reached(R);
    if ((int)R.size() < 3) return;
    int s = detail::surplus_of(R, X);
    if (s <= r - 1) markset(R, s);
  }
  // The triple {a,b,c}: a and b hold what they could get.  Either the three lines collect r
  // extra cells (safe, explicit flow), or a joint search from all three fails and its closed
  // set R has sur_X(R) <= r-1 (then R is marked).  If the joint search fails with
  // sur_X(R) >= r, the three lines own >= r extra cells (all other lines own exactly 2 and R is
  // closed), so the triple is safe as well.
  void third(int a, int b, int c) {
    for (;;) {
      if (ext(a) + ext(b) + ext(c) >= r) break;                // safe
      if (M.gain1(c)) continue;
      ++M.stamp;
      M.lvis(a) = M.lvis(b) = M.lvis(c) = M.stamp;
      M.nvis = 3;
      if (M.aug(c) || M.aug(a) || M.aug(b)) continue;          // (not expected: sequential flows are maximal)
      if (paranoid) check_state(a, b, c);
      mark_reached();
      if (!(mark[a] && mark[b] && mark[c]) && ext(a) + ext(b) + ext(c) < r) die("third: unsafe triple not marked");
      break;
    }
    release(c);
  }

  void run() {
    const Geom& g = M.g;
    memset(mark, 0, sizeof mark);
    memset(us, 0, sizeof us);
    memset(nnb, 0, sizeof nnb);
    for (int l = 0; l < NL; l++) val[l] = 32767;
    nmarked = 0;
    // ---- X-neighbours
    for (int i = 0; i < M.nl; i++) {
      int a = M.lines[i], n = 0;
      for (int j = 0; j < 6; j++) {
        int x = g.line[a][j];
        if (X.test(x)) continue;
        for (int q = 0; q < g.nlc[x]; q++) { int l = g.lc[x][q]; if (l != a && !M.dead[l]) nb[a][n++] = (uint8_t)l; }
      }
      std::sort(nb[a], nb[a] + n);
      nnb[a] = (uint8_t)n;
    }
    // ---- X-components: fewer than 3 lines -> in no triple; surplus <= r-1 -> all lines marked
    {
      uint8_t seen[NL];
      memset(seen, 0, sizeof seen);
      std::vector<int> K;
      for (int i = 0; i < M.nl; i++) {
        int l0 = M.lines[i];
        if (seen[l0]) continue;
        K.clear();
        K.push_back(l0);
        seen[l0] = 1;
        for (size_t h = 0; h < K.size(); h++) {
          int l = K[h];
          for (int j = 0; j < nnb[l]; j++) { int a = nb[l][j]; if (!seen[a]) { seen[a] = 1; K.push_back(a); } }
        }
        if (K.size() < 3) continue;
        for (int l : K) us[l] = 1;
        int s = detail::surplus_of(K, X);
        if (s <= r - 1) markset(K, s);
      }
    }
    // ---- single lines: safe iff the line alone collects r extra cells
    for (int i = 0; i < M.nl; i++) {
      int l = M.lines[i];
      if (!us[l] || mark[l]) continue;
      const int* cs = g.line[l];
      int nav = 0;
      for (int j = 0; j < 6; j++) nav += !X.test(cs[j]);
      if (nav - 2 < r) continue;                                // sur_X({l}) < r: cannot be safe alone
      if (__builtin_popcount(M.fm(l)) >= r) { us[l] = 0; continue; }   // r unassigned cells on l itself
      while (ext(l) < r && M.gain1(l)) {}
      if (ext(l) >= r) { if (paranoid) check_state(l, l, l); us[l] = 0; }
      else mark_reached();
      release(l);
    }
    // ---- pairs and triples of the remaining lines
    {
      int nus = 0;
      for (int i = 0; i < M.nl; i++) nus += us[M.lines[i]];
      if (nus == nmarked) return;                               // nothing left to decide
    }
    for (int ai = 0; ai < M.nl; ai++) {
      int a = M.lines[ai];
      if (!us[a]) continue;
      bool aflow = false, asafe = false;
      auto flow_a = [&]() {
        aflow = true;
        while (ext(a) < r && M.gain1(a)) {}
        if (ext(a) >= r) { if (paranoid) check_state(a, a, a); asafe = true; us[a] = 0; }
        else mark_reached();
      };
      for (int bi = 0; bi < nnb[a] && !asafe; bi++) {
        int b = nb[a][bi];
        if (b < a || !us[b]) continue;
        bool bflow = false, bsafe = false;
        auto flow_b = [&]() {
          bflow = true;
          while (ext(a) + ext(b) < r && M.gain1(b)) {}
          if (ext(a) + ext(b) >= r) { if (paranoid) check_state(a, b, b); bsafe = true; }
          else mark_reached();
        };
        auto visit = [&](int c) {                               // returns false when a or the pair is safe
          if (mark[a] && mark[b] && mark[c]) return true;
          if (!aflow) { flow_a(); if (asafe) return false; }
          if (!bflow) { flow_b(); if (bsafe) return false; }
          if (mark[a] && mark[b] && mark[c]) return true;
          third(a, b, c);
          return true;
        };
        bool go = true;
        // third lines X-adjacent to a: c > b (otherwise (a,c) is the smaller pair)
        for (int j = 0; j < nnb[a] && go; j++) {
          int c = nb[a][j];
          if (c <= b || !us[c]) continue;
          go = visit(c);
        }
        // third lines X-adjacent to b but not to a: c > a (otherwise (c,b) is the smaller pair)
        for (int j = 0; j < nnb[b] && go; j++) {
          int c = nb[b][j];
          if (c <= a || !us[c] || adjacent(a, c)) continue;
          go = visit(c);
        }
        if (bflow) release(b);
      }
      if (aflow) release(a);
    }
  }
};

// ---------------------------------------------------------------------------------------------
// Reference for the marking (testing, --check-marker): every X-connected triple {m,p,q} (m its
// centre) gets a fresh matcher and collects extra cells from the three lines until no line can
// gain; the triple is unsafe iff fewer than r cells were collected.  ref[l] = 1 iff l lies in an
// unsafe triple.  No pruning, no shared state between triples.
// ---------------------------------------------------------------------------------------------
static void reference_marks(const CellSet& B, const CellSet& X, int r, uint8_t ref[NL]) {
  const Geom& g = G();
  memset(ref, 0, NL);
  int live[NL], nl = live_lines(B, live);
  uint8_t isl[NL];
  memset(isl, 0, sizeof isl);
  for (int i = 0; i < nl; i++) isl[live[i]] = 1;
  for (int i = 0; i < nl; i++) {
    int m = live[i];
    std::vector<int> nbr;
    for (int j = 0; j < 6; j++) {
      int x = g.line[m][j];
      if (X.test(x)) continue;
      for (int q = 0; q < g.nlc[x]; q++) { int l = g.lc[x][q]; if (l != m && isl[l]) nbr.push_back(l); }
    }
    for (size_t u = 0; u < nbr.size(); u++)
      for (size_t v = u + 1; v < nbr.size(); v++) {
        int t[3] = {m, nbr[u], nbr[v]};
        if (ref[t[0]] && ref[t[1]] && ref[t[2]]) continue;
        Matcher M;
        M.init(B, X);
        if (M.build() >= 0) die("reference_marks: no pairing");
        int tot = 0;
        bool progress = true;
        while (progress && tot < r) {
          progress = false;
          for (int z = 0; z < 3 && tot < r; z++)
            if (M.gain1(t[z])) { tot++; progress = true; }
        }
        if (tot < r) ref[t[0]] = ref[t[1]] = ref[t[2]] = 1;
      }
  }
}

// ---------------------------------------------------------------------------------------------
// Step 4: branch and bound.
// ---------------------------------------------------------------------------------------------
struct BnB {
  CellSet B;
  Stones st;
  uint64_t nodes, limit;
  int status;            // 1: no violator (C holds); 0: violator st.X found; -1: node limit hit
  bool relax, paranoid;
  uint64_t n_relax, n_mark, n_last;
  uint64_t check_marker = 0, n_checked = 0;   // --check-marker N: compare the first N markings of the instance with the reference

  // Q(X = st.X, r, Z).  Returns true when the search must stop (status 0 or -1).
  bool rec(int r, const CellSet& Z) {
    const Geom& g = G();
    if (limit && nodes >= limit) { status = -1; return true; }
    nodes++;
    Matcher M;
    M.init(B, st.X);
    if (M.build() >= 0) { status = 0; return true; }            // (B,X) has no pairing: X is a violator
    if (r <= 0) return false;
    if (r == 1) {                                               // shortcut (L)
      n_last++;
      // Pass 1: which lines can gain a third cell?  ("Can gain" does not depend on the pairing:
      // it says that the maximum flow with capacity 3 on that line is 2|L|+1.)  A successful
      // search changes the pairing; the extra cell is given back at once.
      uint8_t stuck[NL];
      memset(stuck, 0, sizeof stuck);
      bool any = false;
      for (int i = 0; i < M.nl; i++) {
        int l = M.lines[i];
        if (M.fm(l)) continue;                                  // a free cell on l: l can gain
        if (!M.gain1(l)) { stuck[l] = 1; any = true; continue; }
        const int* cs = g.line[l];
        unsigned m = 0;
        for (int j = 0; j < 6; j++) m |= unsigned(M.owner[cs[j]] == l) << j;
        m &= m - 1;
        m &= m - 1;
        while (m) { M.set_free(cs[__builtin_ctz(m)]); m &= m - 1; }
      }
      if (!any) return false;
      // Pass 2: in the final pairing, a cell owned by a stuck line, outside Z, placeable.
      for (int c = 0; c < NC; c++) {
        int o = M.owner[c];
        if (o < 0 || !stuck[o] || Z.test(c) || !st.can_place(c)) continue;
        st.place(c);                                            // violator (re-verified by the caller)
        status = 0;
        return true;
      }
      return false;
    }
    n_mark++;
    Marker mk(M, st.X, r, paranoid);
    mk.run();
    if (n_mark <= check_marker) {                               // testing: compare with the naive reference
      uint8_t ref[NL];
      reference_marks(B, st.X, r, ref);
      for (int l = 0; l < NL; l++)
        if (ref[l] != mk.mark[l]) {
          fprintf(stderr, "certify: marker mismatch at line %d (ref %d, marker %d), r = %d, X = %s\n", l, ref[l], mk.mark[l], r,
                  to_str(st.X).c_str());
          die("marker differs from the reference");
        }
      n_checked++;
    }
    if (mk.nmarked == 0) return false;
    // candidates: cells of marked lines, not in X or Z, individually placeable
    int cand[NC], key[NC], nc = 0;
    {
      int best[NC];
      for (int c = 0; c < NC; c++) best[c] = 1 << 20;
      for (int i = 0; i < M.nl; i++) {
        int l = M.lines[i];
        if (!mk.mark[l]) continue;
        for (int j = 0; j < 6; j++) { int c = g.line[l][j]; if (mk.val[l] < best[c]) best[c] = mk.val[l]; }
      }
      CellSet Q;
      for (int c = 0; c < NC; c++) {
        if (best[c] == (1 << 20) || st.X.test(c) || Z.test(c) || !st.can_place(c)) continue;
        cand[nc] = c;
        key[c] = best[c] * 256 + c;
        nc++;
        Q.set(c);
      }
      if (nc == 0) return false;
      if (relax) {                                              // shortcut (R)
        Matcher M2;
        M2.init(B, st.X | Q);
        if (M2.build() < 0) { n_relax++; return false; }
      }
    }
    std::sort(cand, cand + nc, [&](int a, int b) { return key[a] < key[b]; });   // tightest sets first
    CellSet Zc = Z;
    for (int i = 0; i < nc; i++) {
      int c = cand[i];
      st.place(c);
      if (rec(r - 1, Zc)) return true;
      st.unplace(c);
      Zc.set(c);
    }
    return false;
  }
};

// ---------------------------------------------------------------------------------------------
// The driver
// ---------------------------------------------------------------------------------------------
struct Options {
  uint64_t max_nodes = 2000000;
  bool stats = false, no_witness = false, force_bnb = false, bnb_only = false, relax = true, paranoid = false, quiet = false;
  uint64_t witness_limit = 200000;
  uint64_t check_marker = 0;
};

struct Result {
  std::string decision, method;
  uint64_t nodes = 0;
  CellSet X;
  std::vector<int> S;
  bool leaf = false;
  int reason = 0, theta = 0;
};

// Final check of a violator from the definition; fills S.  Aborts if X is not a legal violator.
static void confirm_violator(const CellSet& B, const CellSet& F, int k, const CellSet& X, std::vector<int>& S) {
  if (!F.subset_of(X) || X.count() > k || X.intersects(B)) die("violator: F <= X, |X| <= k or X n B = {} violated");
  if (!admissible(B, X)) die("violator is not B-admissible");
  Pairing P = find_pairing(B, X);
  if (P.ok) die("violator has a pairing");
  S = P.hall;
  int live[NL], nl = live_lines(B, live);
  uint8_t isl[NL];
  memset(isl, 0, sizeof isl);
  for (int i = 0; i < nl; i++) isl[live[i]] = 1;
  CellSet N;
  for (int l : S) { if (!isl[l]) die("Hall set contains a dead line"); N = N | G().lineSet[l]; }
  if (N.minus(X).count() >= 2 * (int)S.size()) die("Hall set is not a violator");
}

static Result certify(const CellSet& B, const CellSet& F, int k, const Options& o) {
  Result R;
  R.theta = k - F.count();
  if (F.intersects(B) || F.count() > k || !admissible(B, F)) {
    R.decision = "void";
    R.method = "phi";
    R.reason = 1;
    return R;
  }
  PhiResult ph = phi_check(B, F, k);
  R.leaf = ph.holds;
  R.reason = ph.reason;
  if (!o.bnb_only) {
    if (ph.holds) { R.decision = "holds"; R.method = "phi"; return R; }                         // step 1
    if (ph.reason == 2) {                                                                       // step 2, X = F
      R.decision = "fails";
      R.method = "witness";
      R.X = F;
      confirm_violator(B, F, k, R.X, R.S);
      return R;
    }
    if (ph.reason != 3) die("unexpected phi reason");
    if (!o.no_witness) {                                                                        // step 2, reason 3
      int need = ph.sur + 1;
      if (need < 1 || need > R.theta) die("phi witness with a surplus out of range");
      CellSet N;
      for (int l : ph.S) N = N | G().lineSet[l];
      WitnessSearch W;
      bool found = W.run(B, F, N, need, o.witness_limit);
      R.nodes = W.nodes;
      if (found) {
        R.decision = "fails";
        R.method = "witness";
        R.X = W.st.X;
        confirm_violator(B, F, k, R.X, R.S);
        return R;
      }
    }
    if (R.theta <= 3 && !o.force_bnb) {                                                         // step 3
      EnumResult e = enum_check(B, F, k, 0);
      R.method = "enum";
      R.nodes += e.nodes;
      if (e.status == 1) { R.decision = "holds"; return R; }
      if (e.status != 0) die("enum_check without a node limit returned -1");
      R.decision = "fails";
      R.X = e.X;
      confirm_violator(B, F, k, R.X, R.S);
      return R;
    }
  }
  BnB bb;                                                                                       // step 4
  bb.B = B;
  bb.st.init(B, F);
  bb.nodes = 0;
  bb.limit = o.max_nodes;
  bb.status = 1;
  bb.relax = o.relax;
  bb.paranoid = o.paranoid;
  bb.n_relax = bb.n_mark = bb.n_last = 0;
  bb.check_marker = o.check_marker;
  bb.rec(R.theta, CellSet());
  R.method = "bnb";
  R.nodes += bb.nodes;
  if (bb.status == 1) { R.decision = "holds"; return R; }
  if (bb.status == -1) { R.decision = "undecided"; return R; }
  R.decision = "fails";
  R.X = bb.st.X;
  confirm_violator(B, F, k, R.X, R.S);
  return R;
}

int main(int argc, char** argv) {
  Options o;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--max-nodes" && i + 1 < argc) o.max_nodes = (uint64_t)strtod(argv[++i], nullptr);   // accepts 2e6
    else if (a == "--witness-limit" && i + 1 < argc) o.witness_limit = strtoull(argv[++i], nullptr, 10);
    else if (a == "--check-marker" && i + 1 < argc) o.check_marker = strtoull(argv[++i], nullptr, 10);
    else if (a == "--stats") o.stats = true;
    else if (a == "--no-witness") o.no_witness = true;
    else if (a == "--force-bnb") o.force_bnb = true;
    else if (a == "--bnb-only") o.bnb_only = true;
    else if (a == "--no-relax") o.relax = false;
    else if (a == "--paranoid") o.paranoid = true;
    else if (a == "--quiet") o.quiet = true;
    else {
      fprintf(stderr, "usage: certify [--max-nodes N] [--stats] [--no-witness] [--force-bnb] [--bnb-only] [--no-relax]\n"
                      "               [--witness-limit N] [--paranoid] [--check-marker N] [--quiet] < instances   (lines: k B F)\n");
      return 2;
    }
  }
  const Geom& g = G();
  struct Acc { uint64_t n = 0, us = 0, mx = 0, nodes = 0; };
  std::map<std::string, Acc> acc;
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream is(line);
    int k;
    std::string sb, sf;
    if (!(is >> k >> sb >> sf)) { fprintf(stderr, "certify: bad instance line: %s\n", line.c_str()); return 2; }
    CellSet B = parse_set(sb), F = parse_set(sf);
    auto t0 = std::chrono::steady_clock::now();
    Result r = certify(B, F, k, o);
    uint64_t us = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count();
    std::string out = "{\"k\":" + std::to_string(k) + ",\"B\":\"" + to_str(B) + "\",\"F\":\"" + to_str(F) + "\",\"decision\":\"" +
                      r.decision + "\",\"method\":\"" + r.method + "\",\"nodes\":" + std::to_string(r.nodes) + ",\"X\":\"" +
                      (r.decision == "fails" ? to_str(r.X) : std::string("-")) + "\",\"S\":[";
    if (!o.quiet)
      for (size_t i = 0; i < r.S.size(); i++) {
        if (i) out += ",";
        out += "[";
        for (int j = 0; j < 6; j++) { if (j) out += ","; out += std::to_string(g.line[r.S[i]][j]); }
        out += "]";
      }
    out += std::string("],\"leaf\":") + (r.leaf ? "true" : "false") + ",\"reason\":" + std::to_string(r.reason) +
           ",\"theta\":" + std::to_string(r.theta);
    if (o.stats) out += ",\"us\":" + std::to_string(us);
    out += "}";
    puts(out.c_str());
    Acc& a = acc[r.decision + "/" + r.method];
    a.n++;
    a.us += us;
    a.nodes += r.nodes;
    if (us > a.mx) a.mx = us;
  }
  if (o.stats) {
    fprintf(stderr, "%-20s %8s %14s %12s %12s %14s\n", "decision/method", "count", "total_us", "avg_us", "max_us", "nodes");
    for (auto& kv : acc)
      fprintf(stderr, "%-20s %8llu %14llu %12llu %12llu %14llu\n", kv.first.c_str(), (unsigned long long)kv.second.n,
              (unsigned long long)kv.second.us, (unsigned long long)(kv.second.us / (kv.second.n ? kv.second.n : 1)),
              (unsigned long long)kv.second.mx, (unsigned long long)kv.second.nodes);
  }
  return 0;
}

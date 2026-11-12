// search.cpp -- tasks 4+5: AND-OR search for the lazy-revelation game of SPEC.md section 4.
//
//   bin/search --rmax R --maxcand K --threads T --out DIR [--time-limit SEC] [--nonbusy]
//              [--relcand K2] [--order 0|1] [--tt-bits N] [--selfcheck] [--progress SEC]
//
// Everything is done in the canonical frame of the current state (SPEC 6): a state is canonicalised
// on entry, its children are generated in that frame and canonicalised again when they are entered.
//
// Soundness notes (what may and what may not be trusted):
//  * A state is recorded WON only through SPEC 4: P1 (leaf_holds) or P2 (t < R_max, LOST false, all
//    non-VOID children (a),(c),(d) WON), Q: a table move m with (i) and (ii).
//  * Everything else (candidate ordering, --maxcand, the "relaxed" pre-pass, the leaf screen, the
//    potential bound) can only turn a state into FAILED (= "not proved by this run"), never into WON.
//  * The relaxed game (types 2,3 in the transposition table) is the same game without LOST and without
//    the forced children (c),(d).  real-WON => relaxed-WON (same candidate lists), so a relaxed FAILED
//    is used to cut real states and to filter table moves.  Relaxed results are never written out.
//  * The transposition table stores 64+48 hash bits per state.  The emitted tree is re-walked from the
//    root (emit*) and can be re-verified by --selfcheck (string-keyed maps, naive child enumeration,
//    phi_check) and by the independent verify/treecheck.
#include "geom.hpp"
#include "canon.hpp"
#include "match.hpp"
#include "phi.hpp"
#include <omp.h>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace ttt;

// ------------------------------------------------------------------------------------------ options
static int RMAX = 9, MAXCAND = 4, RELCAND = 0, NTHREADS = 1, ORDER = 1, TTBITS = 25, NBEXTRA = 6;
static bool NONBUSY = false, SELFCHECK = false, NOEMIT = false;
static double TLIMIT = 0, PROGRESS = 30;
static std::string OUTDIR = "results/search";

// ------------------------------------------------------------------------------------------ stats
struct alignas(64) Stats {
  uint64_t calls = 0, pExp = 0, qExp = 0, relP = 0, relQ = 0, leafTests = 0, leafTrue = 0, screenRej = 0,
           ttHit = 0, threatCh = 0, dangerCh = 0, voidCh = 0, lostExact = 0, lostTrue = 0, potCut = 0,
           relCut = 0, rmaxCut = 0;
  double leafSec = 0;
  uint64_t histP[16] = {0}, histLeaf[16] = {0}, histFail[16] = {0};
};
static Stats gst[64];
static inline Stats& ST() { return gst[omp_get_thread_num()]; }
static double tStart;
static std::atomic<bool> gAbort{false};
static std::atomic<int> gAbortWhy{0};  // 1 time limit, 2 TT full
static thread_local const std::atomic<bool>* tlCancel = nullptr;
static inline bool stopNow() {
  return gAbort.load(std::memory_order_relaxed) || (tlCancel && tlCancel->load(std::memory_order_relaxed));
}

// ------------------------------------------------------------------------------------------ hashing / TT
struct Key { uint64_t a, b; bool operator==(const Key& o) const { return a == o.a && b == o.b; } };
struct KeyHash { size_t operator()(const Key& k) const { return (size_t)(k.a ^ (k.b * 0x9E3779B97F4A7C15ULL)); } };
static inline uint64_t smix(uint64_t x) {
  x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33; return x;
}
// type: 0 = P real, 1 = Q real, 2 = P relaxed, 3 = Q relaxed
static inline Key hkey(const CellSet& B, const CellSet& F, int t, int type) {
  uint64_t tag = (uint64_t)(t * 8 + type);
  uint64_t a = 0x9E3779B97F4A7C15ULL + tag, b = 0xC2B2AE3D27D4EB4FULL ^ (tag << 32);
  for (int i = 0; i < 4; i++) { a = smix(a ^ B.w[i]); b = smix(b + B.w[i] * 0x9FB21C651E98DF25ULL + i); }
  for (int i = 0; i < 4; i++) { a = smix(a ^ F.w[i] ^ 0xA5A5A5A5A5A5A5A5ULL); b = smix(b + F.w[i] * 0xD6E8FEB86659FD93ULL + 7 + i); }
  if (!a) a = 1;
  return Key{a, b};
}
enum { S_NONE = 0, S_NOTLEAF = 1, S_FAILED = 2, S_WONLEAF = 3, S_WON = 4 };
struct Slot { std::atomic<uint64_t> a, b; };
static Slot* tt; static uint64_t ttMask; static std::atomic<uint64_t> ttCount{0};
static inline int ttGet(const Key& k, int* mv = nullptr) {
  uint64_t i = k.a & ttMask, chk = k.b >> 16;
  for (;;) {
    uint64_t a = tt[i].a.load(std::memory_order_acquire);
    if (!a) return S_NONE;
    if (a == k.a) {
      uint64_t b = tt[i].b.load(std::memory_order_acquire);
      if (!b) return S_NONE;
      if ((b >> 16) == chk) { if (mv) *mv = (int)(b & 255); return (int)((b >> 8) & 255); }
    }
    i = (i + 1) & ttMask;
  }
}
static inline void ttPut(const Key& k, int st, int mv = 0) {
  uint64_t i = k.a & ttMask, chk = k.b >> 16;
  uint64_t nb = (chk << 16) | ((uint64_t)st << 8) | (uint64_t)mv;
  for (;;) {
    uint64_t a = tt[i].a.load(std::memory_order_acquire);
    if (!a) {
      uint64_t z = 0;
      if (tt[i].a.compare_exchange_strong(z, k.a)) {
        a = k.a;
        uint64_t n = ttCount.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n > (ttMask + 1) / 10 * 9) { gAbortWhy = 2; gAbort = true; }
      } else a = z;
    }
    if (a == k.a) {
      uint64_t b = tt[i].b.load(std::memory_order_acquire);
      for (;;) {
        if (b && (b >> 16) != chk) break;                       // another state with the same 64-bit key
        if (b && (int)((b >> 8) & 255) >= st) return;           // never downgrade
        if (tt[i].b.compare_exchange_weak(b, nb)) return;
      }
    }
    i = (i + 1) & ttMask;
  }
}

// ------------------------------------------------------------------------------------------ line info
struct LL { int n; int id[NL]; int8_t j[NL]; bool live[NL]; };
static inline void liveInfo(const CellSet& B, const CellSet& F, LL& L) {
  const Geom& g = G(); L.n = 0;
  for (int l = 0; l < NL; l++) {
    bool lv = !g.lineSet[l].intersects(B); L.live[l] = lv; L.j[l] = 0;
    if (lv) { L.id[L.n++] = l; L.j[l] = (int8_t)(g.lineSet[l] & F).count(); }
  }
}
// total surplus |N(L)\F| - 2|L| of all live lines, and number of live lines
static inline int totalSur(const CellSet& B, const CellSet& F, int* nlOut = nullptr, CellSet* Nout = nullptr) {
  const Geom& g = G(); CellSet N; int nl = 0;
  for (int l = 0; l < NL; l++) if (!g.lineSet[l].intersects(B)) { N = N | g.lineSet[l]; nl++; }
  if (nlOut) *nlOut = nl;
  if (Nout) *Nout = N;
  return N.minus(F).count() - 2 * nl;
}
// Necessary condition for LEAF(B,F,t) whenever some F-connected component has >= 3 lines (sur is additive
// over F-components and >= 0 on each one when a pairing exists).  Used only to skip leaf tests, i.e. it can
// only make the search report FAILED / NOTLEAF, never WON.
static inline bool leafScreen(const CellSet& B, const CellSet& F, int t) {
  int nl; int s = totalSur(B, F, &nl);
  if (nl < 3) return true;
  return s >= t - F.count();
}
// leaf test of a canonical P-state with caching in the real P entry
static inline bool leafTest(const CellSet& Bc, const CellSet& Fc, int t, const Key& kP) {
  Stats& s = ST();
  if (!leafScreen(Bc, Fc, t)) { s.screenRej++; ttPut(kP, S_NOTLEAF); return false; }
  double t0 = omp_get_wtime();
  bool r = leaf_holds(Bc, Fc, t);
  s.leafSec += omp_get_wtime() - t0; s.leafTests++;
  if (r) { s.leafTrue++; s.histLeaf[t < 16 ? t : 15]++; ttPut(kP, S_WONLEAF); } else ttPut(kP, S_NOTLEAF);
  return r;
}
static bool leafNow(const CellSet& B2, const CellSet& F, int t) {
  if (!leafScreen(B2, F, t)) return false;
  CanonResult cr = canon_pair(B2, F); Key k = hkey(cr.B, cr.F, t, 0);
  int st = ttGet(k);
  if (st == S_WONLEAF) return true;
  if (st != S_NONE) return false;
  return leafTest(cr.B, cr.F, t, k);
}

// ------------------------------------------------------------------------------------------ LOST (E2, E3)
// Exact: returns true iff a pattern of SPEC 4 fits.  F must be B-admissible (state not VOID).
static bool lostCheck(const CellSet& B, const CellSet& F, int t, const LL& L) {
  const Geom& g = G();
  const int nF = F.count(), lim = t + 1, budget = lim - nF;
  int maxj = 0;
  for (int i = 0; i < L.n; i++) if (L.j[L.id[i]] > maxj) maxj = L.j[L.id[i]];
  const int m1 = 5 - maxj;                              // cheapest threat line (new stones)
  const int p1 = std::max(1, 2 * (4 - maxj));           // cheapest danger pair (>= 1: F is admissible)
  const bool tryE2 = (m1 + p1 - 2 <= budget), tryE3 = (2 * p1 - 4 <= budget);
  if (!tryE2 && !tryE3) return false;
  ST().lostExact++;
  if (tryE2) {
    for (int c = 0; c < NC; c++) {
      if (B.test(c) || F.test(c)) continue;
      int n = g.nlc[c];
      for (int ia = 0; ia < n; ia++) for (int ib = 0; ib < n; ib++) {
        if (ia == ib) continue;
        int a = g.lc[c][ia], b = g.lc[c][ib];
        if (!L.live[a] || !L.live[b]) continue;
        if (L.j[a] > 4 || L.j[b] > 4) continue;
        int base = (4 - L.j[a]) + (4 - L.j[b]);
        if (base + m1 - 2 > budget) continue;
        for (int qa = 0; qa < 6; qa++) {
          int oa = g.line[a][qa]; if (oa == c || F.test(oa)) continue;
          CellSet Sa = g.lineSet[a]; Sa.reset(c); Sa.reset(oa);
          for (int qb = 0; qb < 6; qb++) {
            int ob = g.line[b][qb]; if (ob == c || F.test(ob)) continue;
            CellSet FS = F | Sa | g.lineSet[b]; FS.reset(c); FS.reset(ob);
            if (FS.count() + m1 - 2 > lim) continue;
            for (int qx = 0; qx < 6; qx++) {
              int x = g.line[a][qx]; if (x == c || x == oa) continue;       // x in Sa
              for (int il = 0; il < g.nlc[x]; il++) {
                int l = g.lc[x][il]; if (l == a || !L.live[l]) continue;     // l != b since x not in b
                for (int qe = 0; qe < 6; qe++) {
                  int e = g.line[l][qe];
                  if (e == x || F.test(e) || g.lineSet[b].test(e)) continue;
                  CellSet U = FS | g.lineSet[l]; U.reset(e);                 // e is on neither a nor b
                  if (U.count() <= lim) { ST().lostTrue++; return true; }
                }
              }
            }
          }
        }
      }
    }
  }
  if (tryE3) {
    for (int x = 0; x < NC; x++) {
      if (B.test(x)) continue;
      int n = g.nlc[x];
      for (int i1 = 0; i1 < n; i1++) for (int i2 = i1 + 1; i2 < n; i2++) {
        int a = g.lc[x][i1], a2 = g.lc[x][i2];
        if (!L.live[a] || !L.live[a2]) continue;
        for (int qc = 0; qc < 6; qc++) {
          int c = g.line[a][qc]; if (c == x || F.test(c)) continue;
          for (int ib = 0; ib < g.nlc[c]; ib++) {
            int b = g.lc[c][ib]; if (b == a || !L.live[b]) continue;         // b != a2 since c not on a2
            int base1 = (4 - L.j[a]) + (4 - L.j[b]);
            if (base1 + p1 - 4 > budget) continue;
            for (int qc2 = 0; qc2 < 6; qc2++) {
              int c2 = g.line[a2][qc2]; if (c2 == x || F.test(c2)) continue;
              for (int ib2 = 0; ib2 < g.nlc[c2]; ib2++) {
                int b2 = g.lc[c2][ib2]; if (b2 == a2 || b2 == b || !L.live[b2]) continue;
                int base = base1 + (4 - L.j[a2]) + (4 - L.j[b2]);
                if (base - 4 > budget) continue;
                bool c2OnB = g.lineSet[b].test(c2), cOnB2 = g.lineSet[b2].test(c);
                for (int q1 = 0; q1 < 6; q1++) {
                  int oa = g.line[a][q1]; if (oa == c || oa == x || F.test(oa)) continue;
                  CellSet S1 = F | g.lineSet[a]; S1.reset(c); S1.reset(oa);
                  for (int q2 = 0; q2 < 6; q2++) {
                    int oa2 = g.line[a2][q2]; if (oa2 == c2 || oa2 == x || F.test(oa2)) continue;
                    CellSet Sa2 = g.lineSet[a2]; Sa2.reset(c2); Sa2.reset(oa2);
                    CellSet S2 = S1 | Sa2;
                    for (int q3 = 0; q3 < 6; q3++) {
                      int ob = g.line[b][q3]; if (ob == c || F.test(ob)) continue;
                      if (c2OnB && ob != c2) continue;
                      CellSet Sb = g.lineSet[b]; Sb.reset(c); Sb.reset(ob);
                      CellSet S3 = S2 | Sb;
                      if (S3.count() > lim) continue;
                      for (int q4 = 0; q4 < 6; q4++) {
                        int ob2 = g.line[b2][q4]; if (ob2 == c2 || F.test(ob2)) continue;
                        if (cOnB2 && ob2 != c) continue;
                        CellSet Sb2 = g.lineSet[b2]; Sb2.reset(c2); Sb2.reset(ob2);
                        if ((S3 | Sb2).count() <= lim) { ST().lostTrue++; return true; }
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

// ------------------------------------------------------------------------------------------ LOST, naive
// Literal transcription of SPEC 4 (E2), (E3), written separately from lostCheck.  Used by --selfcheck and
// compared with lostCheck on random states by --lost-selftest.
static bool lostNaive(const CellSet& B, const CellSet& F, int t) {
  const Geom& g = G();
  const int lim = t + 1, nF = F.count();
  std::vector<int> live; int jj[NL]; int maxj = 0;
  for (int l = 0; l < NL; l++) {
    jj[l] = -1;
    if (!g.lineSet[l].intersects(B)) { live.push_back(l); jj[l] = (g.lineSet[l] & F).count(); maxj = std::max(maxj, jj[l]); }
  }
  // (E2) threat on l (reply e) + danger (a,b) with common cell c, x = l ∩ a
  for (int l : live) for (int qe = 0; qe < 6; qe++) {
    int e = g.line[l][qe]; if (F.test(e)) continue;
    CellSet Sl = g.lineSet[l]; Sl.reset(e);
    CellSet U1 = F | Sl; int n1 = U1.count(); if (n1 > lim) continue;
    for (int qx = 0; qx < 6; qx++) {
      int x = g.line[l][qx]; if (x == e) continue;
      for (int ia = 0; ia < g.nlc[x]; ia++) {
        int a = g.lc[x][ia]; if (a == l || jj[a] < 0) continue;
        for (int qc = 0; qc < 6; qc++) {
          int c = g.line[a][qc]; if (c == x || c == e || F.test(c)) continue;
          for (int ib = 0; ib < g.nlc[c]; ib++) {
            int b = g.lc[c][ib]; if (b == a || b == l || jj[b] < 0) continue;
            if (g.lineSet[b].test(x) || g.lineSet[b].test(e)) continue;
            if (n1 + (4 - jj[a]) + (4 - jj[b]) - 2 > lim) continue;         // the three stone sets share at most 2 cells
            for (int q1 = 0; q1 < 6; q1++) {
              int oa = g.line[a][q1]; if (oa == c) continue;
              CellSet Sa = g.lineSet[a]; Sa.reset(c); Sa.reset(oa);
              if (!Sa.test(x) || !(F & g.lineSet[a]).subset_of(Sa)) continue;
              for (int q2 = 0; q2 < 6; q2++) {
                int ob = g.line[b][q2]; if (ob == c) continue;
                CellSet Sb = g.lineSet[b]; Sb.reset(c); Sb.reset(ob);
                if (!(F & g.lineSet[b]).subset_of(Sb)) continue;
                if ((int)(U1 | Sa | Sb).count() <= lim) return true;
              }
            }
          }
        }
      }
    }
  }
  // (E3) dangers (a,b;c) and (a2,b2;c2), x = a ∩ a2
  if (4 * (4 - maxj) - 4 > lim - nF) return false;                          // four stone sets share at most 4 cells
  for (size_t i1 = 0; i1 < live.size(); i1++) for (size_t i2 = i1 + 1; i2 < live.size(); i2++) {
    int a = live[i1], a2 = live[i2];
    CellSet I = g.lineSet[a] & g.lineSet[a2]; if (I.count() != 1) continue;
    int x = I.cells()[0];
    for (int qc = 0; qc < 6; qc++) {
      int c = g.line[a][qc]; if (c == x || F.test(c)) continue;
      for (int ib = 0; ib < g.nlc[c]; ib++) {
        int b = g.lc[c][ib]; if (b == a || b == a2 || jj[b] < 0 || g.lineSet[b].test(x)) continue;
        for (int qc2 = 0; qc2 < 6; qc2++) {
          int c2 = g.line[a2][qc2]; if (c2 == x || c2 == c || F.test(c2)) continue;
          for (int ib2 = 0; ib2 < g.nlc[c2]; ib2++) {
            int b2 = g.lc[c2][ib2]; if (b2 == a2 || b2 == a || b2 == b || jj[b2] < 0 || g.lineSet[b2].test(x)) continue;
            if ((4 - jj[a]) + (4 - jj[a2]) + (4 - jj[b]) + (4 - jj[b2]) - 4 > lim - nF) continue;
            for (int q1 = 0; q1 < 6; q1++) for (int q2 = 0; q2 < 6; q2++) for (int q3 = 0; q3 < 6; q3++) for (int q4 = 0; q4 < 6; q4++) {
              int oa = g.line[a][q1], oa2 = g.line[a2][q2], ob = g.line[b][q3], ob2 = g.line[b2][q4];
              if (oa == c || ob == c || oa2 == c2 || ob2 == c2) continue;
              CellSet Sa = g.lineSet[a]; Sa.reset(c); Sa.reset(oa);
              CellSet Sa2 = g.lineSet[a2]; Sa2.reset(c2); Sa2.reset(oa2);
              CellSet Sb = g.lineSet[b]; Sb.reset(c); Sb.reset(ob);
              CellSet Sb2 = g.lineSet[b2]; Sb2.reset(c2); Sb2.reset(ob2);
              if (!Sa.test(x) || !Sa2.test(x)) continue;
              CellSet all = Sa | Sa2 | Sb | Sb2;
              if (all.test(c) || all.test(c2)) continue;
              if (!(F & g.lineSet[a]).subset_of(Sa) || !(F & g.lineSet[a2]).subset_of(Sa2) ||
                  !(F & g.lineSet[b]).subset_of(Sb) || !(F & g.lineSet[b2]).subset_of(Sb2)) continue;
              if ((int)(F | all).count() <= lim) return true;
            }
          }
        }
      }
    }
  }
  return false;
}
static int lostSelftest(int N, unsigned seed) {
  const Geom& g = G();
  std::mt19937 rng(seed); int nTrue = 0, nDiff = 0, done = 0;
  while (done < N) {
    CellSet B, F;
    int nb = rng() % 6; for (int i = 0; i < nb; i++) B.set(rng() % NC);
    int nlines = 1 + rng() % 5;
    for (int i = 0; i < nlines; i++) {
      int l = rng() % NL; if (g.lineSet[l].intersects(B)) continue;
      int cnt = 1 + rng() % 4;
      for (int k = 0; k < cnt; k++) F.set(g.line[l][rng() % 6]);
    }
    int extra = rng() % 3; for (int i = 0; i < extra; i++) F.set(rng() % NC);
    F = F.minus(B);
    if (!admissible(B, F)) continue;
    int t = F.count() + rng() % 9;
    LL L; liveInfo(B, F, L);
    bool r1 = lostCheck(B, F, t, L), r2 = lostNaive(B, F, t);
    done++; if (r2) nTrue++;
    if (r1 != r2) { nDiff++; if (nDiff <= 10) fprintf(stderr, "LOST MISMATCH: t=%d B=%s F=%s lostCheck=%d naive=%d\n", t, to_str(B).c_str(), to_str(F).c_str(), (int)r1, (int)r2); }
  }
  fprintf(stderr, "[lost-selftest] %d random admissible states, %d LOST (naive), %d mismatches\n", done, nTrue, nDiff);
  return nDiff;
}

// ------------------------------------------------------------------------------------------ forced children
struct Child { CellSet B, F; int16_t kind, c, a, b, oa, ob; };   // kind 0: threat (line a, reply c); 1: danger
static void genForced(const CellSet& B, const CellSet& F, int t, const LL& L, std::vector<Child>& out) {
  const Geom& g = G();
  const int budget = t + 1 - F.count();
  for (int i = 0; i < L.n; i++) {
    int l = L.id[i];
    if (5 - L.j[l] > budget) continue;
    for (int q = 0; q < 6; q++) {
      int e = g.line[l][q]; if (F.test(e)) continue;
      Child ch; ch.B = B; ch.B.set(e); ch.F = F | g.lineSet[l]; ch.F.reset(e);
      ch.kind = 0; ch.c = (int16_t)e; ch.a = (int16_t)l; ch.b = -1; ch.oa = ch.ob = -1;
      out.push_back(ch);
    }
  }
  for (int c = 0; c < NC; c++) {
    if (B.test(c) || F.test(c)) continue;
    int n = g.nlc[c];
    for (int ia = 0; ia < n; ia++) for (int ib = ia + 1; ib < n; ib++) {
      int a = g.lc[c][ia], b = g.lc[c][ib];
      if (!L.live[a] || !L.live[b]) continue;
      if (L.j[a] > 4 || L.j[b] > 4) continue;
      if ((4 - L.j[a]) + (4 - L.j[b]) > budget) continue;
      for (int qa = 0; qa < 6; qa++) {
        int oa = g.line[a][qa]; if (oa == c || F.test(oa)) continue;
        for (int qb = 0; qb < 6; qb++) {
          int ob = g.line[b][qb]; if (ob == c || F.test(ob)) continue;
          Child ch; ch.B = B; ch.B.set(c);
          ch.F = F | g.lineSet[a] | g.lineSet[b]; ch.F.reset(c); ch.F.reset(oa); ch.F.reset(ob);
          ch.kind = 1; ch.c = (int16_t)c; ch.a = (int16_t)a; ch.b = (int16_t)b; ch.oa = (int16_t)oa; ch.ob = (int16_t)ob;
          out.push_back(ch);
        }
      }
    }
  }
}

// ------------------------------------------------------------------------------------------ good busy sets
static std::vector<uint32_t> good8, good9;
static void loadGood(int k, std::vector<uint32_t>& out) {
  mkdir("results", 0755); mkdir("results/search_cache", 0755);
  std::string path = "results/search_cache/good" + std::to_string(k) + ".txt";
  FILE* f = fopen(path.c_str(), "r");
  if (f) {
    unsigned long n = 0; unsigned m;
    if (fscanf(f, "%lu", &n) == 1) { while (fscanf(f, "%x", &m) == 1) out.push_back(m); }
    fclose(f);
    if (out.size() == n && n > 0) return;
    out.clear();
  }
  double t0 = omp_get_wtime();
  std::vector<uint32_t> reps = busy_orbit_reps(k);
  std::vector<char> ok(reps.size(), 0);
  #pragma omp parallel for schedule(dynamic, 4) num_threads(NTHREADS)
  for (size_t i = 0; i < reps.size(); i++) {
    CellSet Bs = busy_set(reps[i]);
    ok[i] = (leafScreen(Bs, CellSet(), k) && leaf_holds(Bs, CellSet(), k)) ? 1 : 0;
  }
  std::unordered_set<uint32_t> s; int ngood = 0;
  for (size_t i = 0; i < reps.size(); i++) if (ok[i]) { ngood++; for (int gi = 0; gi < NG; gi++) s.insert(busy_image(gi, reps[i])); }
  out.assign(s.begin(), s.end()); std::sort(out.begin(), out.end());
  fprintf(stderr, "[good%d] %zu orbit reps, %d LEAF-good, %zu masks, %.1f s\n", k, reps.size(), ngood, out.size(), omp_get_wtime() - t0);
  std::string tmp = path + ".tmp" + std::to_string((long)getpid());
  f = fopen(tmp.c_str(), "w");
  if (f) { fprintf(f, "%zu\n", out.size()); for (uint32_t m : out) fprintf(f, "%x\n", m); fclose(f); rename(tmp.c_str(), path.c_str()); }
}

// ------------------------------------------------------------------------------------------ candidates
// Ordered table moves for the Q-state (B,F,t) (canonical frame).  Deterministic function of the state.
static int orderCands(const CellSet& B, const CellSet& F, int t, int* out, int maxOut) {
  const Geom& g = G();
  uint32_t bb = busy_mask(B), fb = busy_mask(F);
  int c8[NB] = {0}, c9[NB] = {0};
  int nb = __builtin_popcount(bb);
  if (nb < 8) for (uint32_t m : good8) if ((m & bb) == bb && !(m & fb)) { uint32_t r = m & ~bb; while (r) { c8[__builtin_ctz(r)]++; r &= r - 1; } }
  if (nb < 9) for (uint32_t m : good9) if ((m & bb) == bb && !(m & fb)) { uint32_t r = m & ~bb; while (r) { c9[__builtin_ctz(r)]++; r &= r - 1; } }
  struct C { uint64_t score; int m; };
  C cs[NC]; int n = 0;
  for (int i = 0; i < NB; i++) {
    if (((bb | fb) >> i) & 1) continue;
    int m = g.busy[i];
    CellSet B2 = B; B2.set(m);
    uint64_t anti = B.test(g.antip[m]) ? 1 : 0;
    uint64_t sl = (uint64_t)(slack(B2) + 2000);
    uint64_t a8 = std::min(c8[i], (1 << 20) - 1), a9 = std::min(c9[i], (1 << 20) - 1);
    cs[n].score = (anti << 58) | (a8 << 36) | (a9 << 16) | sl; cs[n].m = m; n++;
  }
  std::sort(cs, cs + n, [](const C& x, const C& y) { return x.score != y.score ? x.score > y.score : x.m < y.m; });
  // leaf bit, tested in score order; once maxOut immediate leaves are known the list is complete, the
  // remaining candidates would be cut off anyway (same result as testing all of them, fewer leaf tests)
  int nleaf = 0;
  for (int i = 0; i < n && nleaf < maxOut; i++) {
    CellSet B2 = B; B2.set(cs[i].m);
    if (leafNow(B2, F, t)) { cs[i].score |= (1ull << 62); nleaf++; }
  }
  std::sort(cs, cs + n, [](const C& x, const C& y) { return x.score != y.score ? x.score > y.score : x.m < y.m; });
  if (!NONBUSY) {
    if (n > maxOut) n = maxOut;
    for (int i = 0; i < n; i++) out[i] = cs[i].m;
    return n;
  }
  // --nonbusy: a second ranked list H = cells (busy or not) that lie on the most lines of the violator set S
  // returned by phi_check(B,F,t) (a closed F-connected set with too little surplus, or a Hall violator).
  // Final order: immediate leaves first, then the busy list and H interleaved.
  C hs[NC]; int nh = 0;
  if (nleaf < maxOut) {
    PhiResult pr = phi_check(B, F, t);
    if (!pr.holds && pr.reason >= 2) {
      int hits[NC]; memset(hits, 0, sizeof hits);
      for (int l : pr.S) for (int q = 0; q < 6; q++) hits[g.line[l][q]]++;
      for (int c = 0; c < NC; c++) {
        if (!hits[c] || B.test(c) || F.test(c)) continue;
        int d = 0;
        for (int i = 0; i < g.nlc[c]; i++) if (!g.lineSet[g.lc[c][i]].intersects(B)) d++;
        hs[nh].score = (uint64_t)(hits[c] * 16 + d); hs[nh].m = c; nh++;
      }
      std::sort(hs, hs + nh, [](const C& x, const C& y) { return x.score != y.score ? x.score > y.score : x.m < y.m; });
      if (nh > NBEXTRA) nh = NBEXTRA;
    }
  }
  CellSet used; int no = 0;
  bool anyComp = false;
  for (int i = 0; i < n; i++) if ((cs[i].score >> 16) & ((1ull << 40) - 1)) anyComp = true;
  bool hLeaf[NC];
  for (int i = 0; i < nh; i++) {
    hLeaf[i] = false;
    if (g.busyIdx[hs[i].m] >= 0) continue;            // busy cells already carry their leaf bit in cs
    CellSet B2 = B; B2.set(hs[i].m);
    hLeaf[i] = leafNow(B2, F, t);
  }
  for (int i = 0; i < n && no < maxOut; i++) if (cs[i].score >> 62) { out[no++] = cs[i].m; used.set(cs[i].m); }
  for (int i = 0; i < nh && no < maxOut; i++) if (hLeaf[i] && !used.test(hs[i].m)) { out[no++] = hs[i].m; used.set(hs[i].m); }
  int ia = 0, ih = 0; bool turnA = anyComp;
  while (no < maxOut && (ia < n || ih < nh)) {
    if ((turnA && ia < n) || ih >= nh) {
      if (ia < n) { int m = cs[ia++].m; if (!used.test(m)) { out[no++] = m; used.set(m); turnA = false; } }
    } else {
      int m = hs[ih++].m; if (!used.test(m)) { out[no++] = m; used.set(m); turnA = true; }
    }
  }
  return no;
}

// Upper bound on how much (total surplus - theta) can grow with r more SP stones (table moves only):
// a stone on d live lines changes the surplus by at most 2d-1 and theta by +1; revealing a cell that is
// on no live line gains 1.  If the deficit cannot be closed no LEAF is reachable without forced moves.
static bool potentialOK(const CellSet& B, const CellSet& F, int t) {
  const Geom& g = G();
  int r = RMAX - t; if (r <= 0) return true;
  int nl; CellSet N; int s = totalSur(B, F, &nl, &N);
  int deficit = (t - (int)F.count()) - s;
  if (deficit <= 0 || nl < 3) return true;
  int gains[NC]; int n = 0, free0 = 0;
  for (int c = 0; c < NC; c++) {
    if (B.test(c) || F.test(c)) continue;
    if (!NONBUSY && g.busyIdx[c] < 0) continue;
    int d = 0;
    for (int i = 0; i < g.nlc[c]; i++) if (!g.lineSet[g.lc[c][i]].intersects(B)) d++;
    if (d == 0) { free0++; continue; }
    if (2 * d - 2 > 0) gains[n++] = 2 * d - 2;
  }
  std::sort(gains, gains + n, [](int x, int y) { return x > y; });
  int tot = free0;
  for (int i = 0; i < n && i < r; i++) tot += gains[i];
  return tot >= deficit;
}

// ------------------------------------------------------------------------------------------ progress
static void printProgress(double now) {
  Stats a;
  for (int i = 0; i < 64; i++) {
    const Stats& s = gst[i];
    a.calls += s.calls; a.pExp += s.pExp; a.qExp += s.qExp; a.relP += s.relP; a.relQ += s.relQ; a.leafTests += s.leafTests;
    a.leafTrue += s.leafTrue; a.screenRej += s.screenRej; a.ttHit += s.ttHit; a.threatCh += s.threatCh; a.dangerCh += s.dangerCh;
    a.voidCh += s.voidCh; a.lostExact += s.lostExact; a.lostTrue += s.lostTrue; a.leafSec += s.leafSec; a.potCut += s.potCut;
    a.relCut += s.relCut; a.rmaxCut += s.rmaxCut;
    for (int k = 0; k < 16; k++) { a.histP[k] += s.histP[k]; a.histLeaf[k] += s.histLeaf[k]; a.histFail[k] += s.histFail[k]; }
  }
  std::string h, hl, hf;
  for (int k = 0; k <= RMAX && k < 16; k++) {
    h += std::to_string(a.histP[k]) + (k < RMAX ? "," : ""); hl += std::to_string(a.histLeaf[k]) + (k < RMAX ? "," : "");
    hf += std::to_string(a.histFail[k]) + (k < RMAX ? "," : "");
  }
  fprintf(stderr, "[%.0fs] calls=%lu P=%lu Q=%lu relP=%lu relQ=%lu tt=%lu ttHit=%lu leafTests=%lu(true %lu, screened %lu, %.1f cpu-s) thr=%lu dng=%lu void=%lu lostExact=%lu lostTrue=%lu potCut=%lu relCut=%lu rmaxCut=%lu\n"
                  "        P-expanded by t: [%s]  leaves by t: [%s]  failed by t: [%s]\n",
          now - tStart, a.calls, a.pExp, a.qExp, a.relP, a.relQ, (unsigned long)ttCount.load(), a.ttHit, a.leafTests, a.leafTrue, a.screenRej, a.leafSec,
          a.threatCh, a.dangerCh, a.voidCh, a.lostExact, a.lostTrue, a.potCut, a.relCut, a.rmaxCut, h.c_str(), hl.c_str(), hf.c_str());
  fflush(stderr);
}
static std::atomic<double> nextPrint{0};
static void tick() {
  double now = omp_get_wtime();
  if (TLIMIT > 0 && now - tStart > TLIMIT && !gAbort.load()) { gAbortWhy = 1; gAbort = true; }
  double nx = nextPrint.load();
  if (now >= nx && nextPrint.compare_exchange_strong(nx, now + PROGRESS)) printProgress(now);
}

// ------------------------------------------------------------------------------------------ the search
static int proveP(const CellSet& B0, const CellSet& F0, int t, bool rel, bool trunk);
static int proveQ(const CellSet& B0, const CellSet& F0, int t, bool rel, bool trunk);

static int runForced(std::vector<Child>& ch, int t, bool trunk) {
  int n = (int)ch.size();
  if (!n) return 1;
  if (trunk && NTHREADS > 1 && n >= 8 && !omp_in_parallel()) {
    std::atomic<int> res{1}; std::atomic<bool> cancel{false};
    #pragma omp parallel for schedule(dynamic, 1) num_threads(NTHREADS)
    for (int i = 0; i < n; i++) {
      if (cancel.load(std::memory_order_relaxed) || gAbort.load(std::memory_order_relaxed)) continue;
      Child& c = ch[i];
      if (!admissible(c.B, c.F)) { ST().voidCh++; continue; }
      if (c.kind == 0) ST().threatCh++; else ST().dangerCh++;
      const std::atomic<bool>* save = tlCancel; tlCancel = &cancel;
      int r = proveP(c.B, c.F, t + 1, false, false);
      tlCancel = save;
      if (r == 0) { res.store(0); cancel.store(true); }
      else if (r < 0) { int one = 1; res.compare_exchange_strong(one, -1); }
    }
    if (res.load() == 1 && gAbort.load()) return -1;
    return res.load();
  }
  for (int i = 0; i < n; i++) {
    Child& c = ch[i];
    if (!admissible(c.B, c.F)) { ST().voidCh++; continue; }
    if (c.kind == 0) ST().threatCh++; else ST().dangerCh++;
    int r = proveP(c.B, c.F, t + 1, false, false);
    if (r != 1) return r;
  }
  return 1;
}

// returns 1 WON, 0 FAILED (not proved under this run's R_max / candidate limits), -1 aborted/cancelled
static int proveP(const CellSet& B0, const CellSet& F0, int t, bool rel, bool trunk) {
  if (stopNow()) return -1;
  Stats& s = ST();
  if ((++s.calls & 255) == 0) tick();
  CanonResult cr = canon_pair(B0, F0);
  const CellSet& B = cr.B; const CellSet& F = cr.F;
  Key kP = hkey(B, F, t, 0);
  int st = ttGet(kP);
  if (st >= S_WONLEAF) { s.ttHit++; return 1; }
  if (!rel && st == S_FAILED) { s.ttHit++; return 0; }
  Key kR = hkey(B, F, t, 2);
  int sr = ttGet(kR);
  if (rel) { if (sr == S_WON) return 1; if (sr == S_FAILED) return 0; }
  if (st == S_NONE) { if (leafTest(B, F, t, kP)) return 1; }
  if (t >= RMAX) { s.rmaxCut++; if (!rel) s.histFail[t < 16 ? t : 15]++; ttPut(rel ? kR : kP, S_FAILED); return 0; }
  // relaxed game: only child (a)
  if (sr == S_NONE) {
    s.relP++;
    int r;
    if (!potentialOK(B, F, t)) { s.potCut++; r = 0; }
    else r = proveQ(B, F, t + 1, true, false);
    if (r < 0) return -1;
    sr = r ? S_WON : S_FAILED;
    ttPut(kR, sr);
  }
  if (rel) return sr == S_WON ? 1 : 0;
  if (sr == S_FAILED) { s.relCut++; s.histFail[t < 16 ? t : 15]++; ttPut(kP, S_FAILED); return 0; }
  LL L; liveInfo(B, F, L);
  if (lostCheck(B, F, t, L)) { s.histFail[t < 16 ? t : 15]++; ttPut(kP, S_FAILED); return 0; }
  s.pExp++; s.histP[t < 16 ? t : 15]++;
  std::vector<Child> ch; genForced(B, F, t, L, ch);
  int res = 1;
  if (ORDER == 0) res = proveQ(B, F, t + 1, false, trunk);
  if (res == 1) res = runForced(ch, t, trunk);
  if (res == 1 && ORDER != 0) res = proveQ(B, F, t + 1, false, trunk);
  if (res < 0) return -1;
  if (!res) s.histFail[t < 16 ? t : 15]++;
  ttPut(kP, res ? S_WON : S_FAILED);
  return res;
}

static int proveQ(const CellSet& B0, const CellSet& F0, int t, bool rel, bool trunk) {
  if (stopNow()) return -1;
  Stats& s = ST();
  if ((++s.calls & 255) == 0) tick();
  CanonResult cr = canon_pair(B0, F0);
  const CellSet& B = cr.B; const CellSet& F = cr.F;
  Key kQ = hkey(B, F, t, 1);
  int st = ttGet(kQ);
  if (st == S_WON) return 1;
  if (!rel && st == S_FAILED) return 0;
  Key kR = hkey(B, F, t, 3);
  int sr = ttGet(kR);
  if (rel) { if (sr == S_WON) return 1; if (sr == S_FAILED) return 0; s.relQ++; }
  else { if (sr == S_FAILED) { ttPut(kQ, S_FAILED); return 0; } s.qExp++; }
  int cand[NC]; int nc = orderCands(B, F, t, cand, RELCAND);
  bool canReveal = (int)F.count() < t;
  int tried = 0;
  for (int i = 0; i < nc; i++) {
    int m = cand[i];
    CellSet B2 = B; B2.set(m);
    CellSet F2 = F; F2.set(m);
    bool need2 = canReveal && admissible(B, F2);
    if (!rel) {
      int r = proveP(B2, F, t, true, false); if (r < 0) return -1; if (!r) continue;
      if (need2) { r = proveQ(B, F2, t, true, false); if (r < 0) return -1; if (!r) continue; }
      if (++tried > MAXCAND) break;
    }
    int r = proveP(B2, F, t, rel, trunk); if (r < 0) return -1; if (!r) continue;
    if (need2) { r = proveQ(B, F2, t, rel, trunk); if (r < 0) return -1; if (!r) continue; }
    ttPut(rel ? kR : kQ, S_WON, m);
    return 1;
  }
  ttPut(rel ? kR : kQ, S_FAILED);
  return 0;
}

// ------------------------------------------------------------------------------------------ emit the tree
static std::unordered_set<Key, KeyHash> seenEmit;
static FILE *fTbl, *fLeaf;
static uint64_t emP = 0, emQ = 0, emLeaf = 0, emErr = 0, emThreat = 0, emDanger = 0, emVoid = 0; static int emMaxT = 0;
static void emitQ(const CellSet& B0, const CellSet& F0, int t);
static void emitP(const CellSet& B0, const CellSet& F0, int t) {
  CanonResult cr = canon_pair(B0, F0); const CellSet& B = cr.B; const CellSet& F = cr.F;
  Key kP = hkey(B, F, t, 0);
  if (!seenEmit.insert(kP).second) return;
  if (t > emMaxT) emMaxT = t;
  int st = ttGet(kP);
  if (st < S_WONLEAF) { proveP(B, F, t, false, false); st = ttGet(kP); }
  if (st == S_WONLEAF) { emLeaf++; fprintf(fLeaf, "%d %s %s\n", t, to_str(B).c_str(), to_str(F).c_str()); return; }
  if (st != S_WON) { emErr++; fprintf(stderr, "EMIT ERROR: P-state not WON: %d %s %s\n", t, to_str(B).c_str(), to_str(F).c_str()); return; }
  emP++;
  LL L; liveInfo(B, F, L);
  std::vector<Child> ch; genForced(B, F, t, L, ch);
  for (Child& c : ch) {
    if (!admissible(c.B, c.F)) { emVoid++; continue; }
    if (c.kind == 0) emThreat++; else emDanger++;
    emitP(c.B, c.F, t + 1);
  }
  emitQ(B, F, t + 1);
}
static void emitQ(const CellSet& B0, const CellSet& F0, int t) {
  CanonResult cr = canon_pair(B0, F0); const CellSet& B = cr.B; const CellSet& F = cr.F;
  Key kQ = hkey(B, F, t, 1);
  if (!seenEmit.insert(kQ).second) return;
  int m = 0; int st = ttGet(kQ, &m);
  if (st != S_WON) { proveQ(B, F, t, false, false); st = ttGet(kQ, &m); }
  if (st != S_WON) { emErr++; fprintf(stderr, "EMIT ERROR: Q-state not WON: %d %s %s\n", t, to_str(B).c_str(), to_str(F).c_str()); return; }
  emQ++;
  fprintf(fTbl, "%d %s %s %d\n", t, to_str(B).c_str(), to_str(F).c_str(), m);
  CellSet B2 = B; B2.set(m);
  emitP(B2, F, t);
  if ((int)F.count() < t) { CellSet F2 = F; F2.set(m); if (admissible(B, F2)) emitQ(B, F2, t); }
}

// ------------------------------------------------------------------------------------------ selfcheck
// Separate code path: reads the two emitted files into string-keyed maps and re-expands the tree of
// SPEC 4 with a naive child enumeration; leaves are re-tested with phi_check (not leaf_holds).
// LOST is re-evaluated with lostNaive (literal transcription of E2/E3).
static std::unordered_map<std::string, int> scTbl; static std::unordered_set<std::string> scLeaves, scSeen;
static uint64_t scP = 0, scQ = 0, scLeaf = 0, scErr = 0;
static std::string skey(int t, const CellSet& B, const CellSet& F) { return std::to_string(t) + " " + to_str(B) + " " + to_str(F); }
static void scFail(const char* what, int t, const CellSet& B, const CellSet& F) {
  if (scErr++ < 20) fprintf(stderr, "SELFCHECK ERROR: %s at %s\n", what, skey(t, B, F).c_str());
}
static void checkQ(const CellSet& B0, const CellSet& F0, int t);
static void checkP(const CellSet& B0, const CellSet& F0, int t) {
  const Geom& g = G();
  CanonResult cr = canon_pair(B0, F0); CellSet B = cr.B, F = cr.F;
  if ((int)B.count() != t || (int)F.count() > t || B.intersects(F)) { scFail("bad P-state shape", t, B, F); return; }
  std::string k = "P" + skey(t, B, F);
  if (!scSeen.insert(k).second) return;
  if (scLeaves.count(skey(t, B, F))) {
    scLeaf++;
    PhiResult r = phi_check(B, F, t);
    if (!r.holds) scFail("listed leaf is not LEAF", t, B, F);
    return;
  }
  scP++;
  if (t >= RMAX) { scFail("P-state at R_max is not a leaf", t, B, F); return; }
  if (lostNaive(B, F, t)) { scFail("LOST pattern fits", t, B, F); return; }
  // (c) threats
  for (int l = 0; l < NL; l++) {
    if (g.lineSet[l].intersects(B)) continue;
    for (int q = 0; q < 6; q++) {
      int e = g.line[l][q]; if (F.test(e)) continue;
      CellSet F2 = F | g.lineSet[l]; F2.reset(e);
      if ((int)F2.count() > t + 1) continue;
      CellSet B2 = B; B2.set(e);
      if (!admissible(B2, F2)) continue;
      checkP(B2, F2, t + 1);
    }
  }
  // (d) dangers
  for (int a = 0; a < NL; a++) {
    if (g.lineSet[a].intersects(B)) continue;
    for (int b = a + 1; b < NL; b++) {
      if (g.lineSet[b].intersects(B)) continue;
      CellSet I = g.lineSet[a] & g.lineSet[b];
      if (I.count() != 1) continue;
      int c = I.cells()[0]; if (F.test(c)) continue;
      for (int qa = 0; qa < 6; qa++) for (int qb = 0; qb < 6; qb++) {
        int oa = g.line[a][qa], ob = g.line[b][qb];
        if (oa == c || ob == c) continue;
        CellSet Qa = g.lineSet[a]; Qa.reset(c); Qa.reset(oa);
        CellSet Qb = g.lineSet[b]; Qb.reset(c); Qb.reset(ob);
        if (!(F & g.lineSet[a]).subset_of(Qa) || !(F & g.lineSet[b]).subset_of(Qb)) continue;
        CellSet F2 = F | Qa | Qb;
        if ((int)F2.count() > t + 1) continue;
        CellSet B2 = B; B2.set(c);
        if (!admissible(B2, F2)) continue;
        checkP(B2, F2, t + 1);
      }
    }
  }
  checkQ(B, F, t + 1);
}
static void checkQ(const CellSet& B0, const CellSet& F0, int t) {
  CanonResult cr = canon_pair(B0, F0); CellSet B = cr.B, F = cr.F;
  if ((int)B.count() != t - 1 || (int)F.count() > t || B.intersects(F)) { scFail("bad Q-state shape", t, B, F); return; }
  std::string k = "Q" + skey(t, B, F);
  if (!scSeen.insert(k).second) return;
  scQ++;
  auto it = scTbl.find(skey(t, B, F));
  if (it == scTbl.end()) { scFail("Q-state missing in table", t, B, F); return; }
  int m = it->second;
  if (m < 0 || m >= NC || B.test(m) || F.test(m)) { scFail("illegal table move", t, B, F); return; }
  if (!NONBUSY && G().busyIdx[m] < 0) { scFail("non-busy table move", t, B, F); return; }
  CellSet B2 = B; B2.set(m);
  checkP(B2, F, t);
  if ((int)F.count() < t) { CellSet F2 = F; F2.set(m); if (admissible(B, F2)) checkQ(B, F2, t); }
}
static bool selfcheck(const std::string& tblPath, const std::string& leafPath) {
  char buf[8192];
  FILE* f = fopen(tblPath.c_str(), "r"); if (!f) return false;
  while (fgets(buf, sizeof buf, f)) {
    int t, m; char sb[4096], sf[4096];
    if (sscanf(buf, "%d %4095s %4095s %d", &t, sb, sf, &m) != 4) { scErr++; continue; }
    scTbl[skey(t, parse_set(sb), parse_set(sf))] = m;
  }
  fclose(f);
  f = fopen(leafPath.c_str(), "r"); if (!f) return false;
  while (fgets(buf, sizeof buf, f)) {
    int t; char sb[4096], sf[4096];
    if (sscanf(buf, "%d %4095s %4095s", &t, sb, sf) != 3) { scErr++; continue; }
    scLeaves.insert(skey(t, parse_set(sb), parse_set(sf)));
  }
  fclose(f);
  checkP(CellSet(), CellSet(), 0);
  fprintf(stderr, "[selfcheck] P=%lu Q=%lu leaves=%lu errors=%lu (table %zu, leaf file %zu)\n", scP, scQ, scLeaf, scErr, scTbl.size(), scLeaves.size());
  return scErr == 0;
}

// ------------------------------------------------------------------------------------------ failing branch
static std::string jstate(const char* type, int t, const CellSet& B, const CellSet& F, const std::string& why) {
  return std::string("{\"type\":\"") + type + "\",\"t\":" + std::to_string(t) + ",\"B\":\"" + to_str(B) + "\",\"F\":\"" + to_str(F) + "\",\"why\":\"" + why + "\"}";
}
// status lookups without searching: 1 won, 0 failed, -1 unknown
static int lookP(const CellSet& B0, const CellSet& F0, int t, bool rel) {
  CanonResult cr = canon_pair(B0, F0);
  int st = ttGet(hkey(cr.B, cr.F, t, 0));
  if (st >= S_WONLEAF) return 1;
  if (!rel && st == S_FAILED) return 0;
  if (st != S_NONE && t >= RMAX) return 0;
  if (rel || st == S_NOTLEAF) { int sr = ttGet(hkey(cr.B, cr.F, t, 2)); if (sr == S_WON && rel) return 1; if (sr == S_FAILED) return 0; }
  return -1;
}
static int lookQ(const CellSet& B0, const CellSet& F0, int t, bool rel) {
  CanonResult cr = canon_pair(B0, F0);
  int st = ttGet(hkey(cr.B, cr.F, t, 1));
  if (st == S_WON) return 1;
  if (!rel && st == S_FAILED) return 0;
  int sr = ttGet(hkey(cr.B, cr.F, t, 3));
  if (sr == S_WON && rel) return 1;
  if (sr == S_FAILED) return 0;
  return -1;
}
static void explainQ(const CellSet& B0, const CellSet& F0, int t, bool rel, std::vector<std::string>& out, int depth);
static void explainP(const CellSet& B0, const CellSet& F0, int t, bool rel, std::vector<std::string>& out, int depth) {
  const Geom& g = G();
  if (depth > 60) return;
  CanonResult cr = canon_pair(B0, F0); CellSet B = cr.B, F = cr.F;
  const char* ty = rel ? "P-relaxed" : "P";
  int st = ttGet(hkey(B, F, t, 0));
  if (st >= S_WONLEAF) { out.push_back(jstate(ty, t, B, F, "WON (not a failure)")); return; }
  if (st == S_NONE) { out.push_back(jstate(ty, t, B, F, "unexplored (search aborted)")); return; }
  if (t >= RMAX) { out.push_back(jstate(ty, t, B, F, "R_max reached and not a LEAF")); return; }
  int sr = ttGet(hkey(B, F, t, 2));
  if (rel || sr == S_FAILED) {
    if (sr == S_NONE) { out.push_back(jstate(ty, t, B, F, "unexplored (search aborted)")); return; }
    if (!potentialOK(B, F, t)) { out.push_back(jstate(ty, t, B, F, "no LEAF reachable by R_max even without forced moves (surplus potential bound)")); return; }
    out.push_back(jstate(ty, t, B, F, "table moves alone (no forced moves) cannot reach a LEAF by R_max; child (a) follows"));
    explainQ(B, F, t + 1, true, out, depth + 1); return;
  }
  LL L; liveInfo(B, F, L);
  if (lostCheck(B, F, t, L)) { out.push_back(jstate(ty, t, B, F, "LOST pattern fits")); return; }
  std::vector<Child> ch; genForced(B, F, t, L, ch);
  int unknown = -1;
  for (size_t i = 0; i < ch.size(); i++) {
    Child& c = ch[i];
    if (!admissible(c.B, c.F)) continue;
    int r = lookP(c.B, c.F, t + 1, false);
    if (r == 0) {
      std::string why;
      if (c.kind == 0) { why = "threat child fails: FP completes 5 on line"; for (int q = 0; q < 6; q++) why += (q ? "," : " ") + std::to_string(g.line[c.a][q]); why += " forcing SP to " + std::to_string(c.c); }
      else { why = "danger child fails: lines"; for (int q = 0; q < 6; q++) why += (q ? "," : " ") + std::to_string(g.line[c.a][q]); why += " and"; for (int q = 0; q < 6; q++) why += (q ? "," : " ") + std::to_string(g.line[c.b][q]);
             why += " forcing SP to " + std::to_string(c.c) + " (cells left empty: " + std::to_string(c.oa) + "," + std::to_string(c.ob) + ")"; }
      out.push_back(jstate(ty, t, B, F, why));
      explainP(c.B, c.F, t + 1, false, out, depth + 1); return;
    }
    if (r < 0 && unknown < 0) unknown = (int)i;
  }
  int rq = lookQ(B, F, t + 1, false);
  if (rq == 0) { out.push_back(jstate(ty, t, B, F, "child (a) fails")); explainQ(B, F, t + 1, false, out, depth + 1); return; }
  out.push_back(jstate(ty, t, B, F, "undecided (search aborted below this state)"));
  if (unknown >= 0) explainP(ch[unknown].B, ch[unknown].F, t + 1, false, out, depth + 1);
  else if (rq < 0) explainQ(B, F, t + 1, false, out, depth + 1);
}
static void explainQ(const CellSet& B0, const CellSet& F0, int t, bool rel, std::vector<std::string>& out, int depth) {
  if (depth > 60) return;
  CanonResult cr = canon_pair(B0, F0); CellSet B = cr.B, F = cr.F;
  const char* ty = rel ? "Q-relaxed" : "Q";
  int cand[NC]; int nc = orderCands(B, F, t, cand, RELCAND);
  bool canReveal = (int)F.count() < t;
  std::string why = "candidates:";
  int pick = -1, pickKind = 0; bool pickRel = rel; int tried = 0;
  for (int i = 0; i < nc; i++) {
    int m = cand[i]; CellSet B2 = B; B2.set(m); CellSet F2 = F; F2.set(m);
    bool need2 = canReveal && admissible(B, F2);
    std::string v;
    if (!rel) {
      int r1 = lookP(B2, F, t, true), r2 = need2 ? lookQ(B, F2, t, true) : 1;
      if (r1 == 0) v = "(i) fails without forced moves";
      else if (r2 == 0) v = "(ii) fails without forced moves";
      if (!v.empty()) { if (pick < 0) { pick = i; pickKind = (r1 == 0) ? 1 : 2; pickRel = true; } why += " " + std::to_string(m) + ":" + v + ";"; continue; }
      if (++tried > MAXCAND) { why += " " + std::to_string(m) + ":not tried (maxcand);"; continue; }
    }
    int r1 = lookP(B2, F, t, rel), r2 = need2 ? lookQ(B, F2, t, rel) : 1;
    if (r1 == 0) v = "(i) fails"; else if (r1 < 0) v = "(i) undecided"; else if (r2 == 0) v = "(ii) fails"; else if (r2 < 0) v = "(ii) undecided"; else v = "ok?";
    why += " " + std::to_string(m) + ":" + v + ";";
    if ((pick < 0 || (pickRel && !rel)) && r1 != 1) { pick = i; pickKind = 1; pickRel = rel; }
    else if ((pick < 0 || (pickRel && !rel)) && r2 != 1) { pick = i; pickKind = 2; pickRel = rel; }
  }
  if (nc == 0) why += " none";
  out.push_back(jstate(ty, t, B, F, why));
  if (pick < 0) return;
  int m = cand[pick]; CellSet B2 = B; B2.set(m); CellSet F2 = F; F2.set(m);
  if (pickKind == 1) explainP(B2, F, t, pickRel, out, depth + 1); else explainQ(B, F2, t, pickRel, out, depth + 1);
}

// ------------------------------------------------------------------------------------------ main
int main(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto nxt = [&]() -> const char* { if (i + 1 >= argc) { fprintf(stderr, "missing value for %s\n", a.c_str()); exit(2); } return argv[++i]; };
    if (a == "--rmax") RMAX = atoi(nxt());
    else if (a == "--maxcand") MAXCAND = atoi(nxt());
    else if (a == "--relcand") RELCAND = atoi(nxt());
    else if (a == "--threads") NTHREADS = atoi(nxt());
    else if (a == "--out") OUTDIR = nxt();
    else if (a == "--time-limit") TLIMIT = atof(nxt());
    else if (a == "--order") ORDER = atoi(nxt());
    else if (a == "--tt-bits") TTBITS = atoi(nxt());
    else if (a == "--progress") PROGRESS = atof(nxt());
    else if (a == "--nbextra") NBEXTRA = atoi(nxt());
    else if (a == "--nonbusy") NONBUSY = true;
    else if (a == "--selfcheck") SELFCHECK = true;
    else if (a == "--noemit") NOEMIT = true;
    else if (a == "--lost-selftest") { int n = atoi(nxt()); G(); return lostSelftest(n, 12345) ? 1 : 0; }
    else { fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
  }
  if (RMAX > 14) { fprintf(stderr, "rmax too large\n"); return 2; }
  if (NTHREADS < 1) NTHREADS = 1;
  if (NTHREADS > 64) NTHREADS = 64;
  if (RELCAND <= 0) RELCAND = 2 * MAXCAND;
  if (RELCAND < MAXCAND) RELCAND = MAXCAND;
  omp_set_num_threads(NTHREADS);
  G();
  canon_pair(CellSet(), CellSet());   // force table construction before any parallel region
  tStart = omp_get_wtime();
  size_t nslots = (size_t)1 << TTBITS;
  tt = (Slot*)calloc(nslots, sizeof(Slot));
  if (!tt) { fprintf(stderr, "cannot allocate TT\n"); return 2; }
  ttMask = nslots - 1;
  loadGood(8, good8); loadGood(9, good9);
  fprintf(stderr, "[search] rmax=%d maxcand=%d relcand=%d threads=%d order=%d nonbusy=%d tt=2^%d good8=%zu good9=%zu (setup %.1f s)\n",
          RMAX, MAXCAND, RELCAND, NTHREADS, ORDER, (int)NONBUSY, TTBITS, good8.size(), good9.size(), omp_get_wtime() - tStart);
  tStart = omp_get_wtime();
  nextPrint = tStart + PROGRESS;
  int res = proveP(CellSet(), CellSet(), 0, false, true);
  double secs = omp_get_wtime() - tStart;
  printProgress(omp_get_wtime());
  bool proved = (res == 1);
  fprintf(stderr, "[search] root %s after %.1f s\n", proved ? "WON" : (res == 0 ? "FAILED" : "ABORTED"), secs);

  mkdir(OUTDIR.c_str(), 0755);
  std::vector<std::string> branch;
  bool scOK = true; bool scRun = false;
  if (proved && !NOEMIT) {
    gAbort = false; TLIMIT = 0;
    fTbl = fopen((OUTDIR + "/strategy.tbl").c_str(), "w"); fLeaf = fopen((OUTDIR + "/leaves.txt").c_str(), "w");
    if (!fTbl || !fLeaf) { fprintf(stderr, "cannot write to %s\n", OUTDIR.c_str()); return 2; }
    emitP(CellSet(), CellSet(), 0);
    fclose(fTbl); fclose(fLeaf);
    fprintf(stderr, "[emit] P(expanded)=%lu Q=%lu leaves=%lu threat children=%lu danger children=%lu void=%lu max t=%d errors=%lu\n",
            emP, emQ, emLeaf, emThreat, emDanger, emVoid, emMaxT, emErr);
    if (emErr) proved = false;
    if (proved && SELFCHECK) { scRun = true; scOK = selfcheck(OUTDIR + "/strategy.tbl", OUTDIR + "/leaves.txt"); if (!scOK) proved = false; }
  } else if (!proved) {
    gAbort = false; TLIMIT = 0;
    explainP(CellSet(), CellSet(), 0, false, branch, 0);
  }
  Stats a;
  for (int i = 0; i < 64; i++) {
    const Stats& s = gst[i];
    a.calls += s.calls; a.pExp += s.pExp; a.qExp += s.qExp; a.relP += s.relP; a.relQ += s.relQ; a.leafTests += s.leafTests; a.leafTrue += s.leafTrue;
    a.screenRej += s.screenRej; a.threatCh += s.threatCh; a.dangerCh += s.dangerCh; a.voidCh += s.voidCh; a.leafSec += s.leafSec; a.lostExact += s.lostExact; a.lostTrue += s.lostTrue;
  }
  struct rusage ru; getrusage(RUSAGE_SELF, &ru);
  FILE* f = fopen((OUTDIR + "/stats.json").c_str(), "w");
  if (f) {
    fprintf(f, "{\"proved\":%s,\"rmax\":%d,\"p_states\":%lu,\"q_states\":%lu,\"leaves\":%lu,\"seconds\":%.2f,",
            proved ? "true" : "false", RMAX, proved ? emP + emLeaf : a.pExp, proved ? emQ : a.qExp, proved ? emLeaf : a.leafTrue, secs);
    fprintf(f, "\"result\":\"%s\",\"abort_reason\":\"%s\",\"maxcand\":%d,\"relcand\":%d,\"threads\":%d,\"order\":%d,\"nonbusy\":%s,",
            res == 1 ? "WON" : (res == 0 ? "FAILED" : "ABORTED"), gAbortWhy.load() == 1 ? "time-limit" : (gAbortWhy.load() == 2 ? "tt-full" : ""), MAXCAND, RELCAND, NTHREADS, ORDER, NONBUSY ? "true" : "false");
    fprintf(f, "\"tree\":{\"p_expanded\":%lu,\"q\":%lu,\"leaves\":%lu,\"threat_children\":%lu,\"danger_children\":%lu,\"void_children\":%lu,\"max_t\":%d},", emP, emQ, emLeaf, emThreat, emDanger, emVoid, emMaxT);
    fprintf(f, "\"searched\":{\"p_expanded\":%lu,\"q_expanded\":%lu,\"relaxed_p\":%lu,\"relaxed_q\":%lu,\"tt_entries\":%lu,\"leaf_tests\":%lu,\"leaf_true\":%lu,\"leaf_screened\":%lu,\"leaf_cpu_seconds\":%.1f,"
               "\"threat_children\":%lu,\"danger_children\":%lu,\"void_children\":%lu,\"lost_exact\":%lu,\"lost_true\":%lu},",
            a.pExp, a.qExp, a.relP, a.relQ, (unsigned long)ttCount.load(), a.leafTests, a.leafTrue, a.screenRej, a.leafSec, a.threatCh, a.dangerCh, a.voidCh, a.lostExact, a.lostTrue);
    fprintf(f, "\"selfcheck\":%s,\"peak_rss_mb\":%.0f,\"failing_branch\":[", scRun ? (scOK ? "\"ok\"" : "\"FAILED\"") : "null", ru.ru_maxrss / 1024.0);
    for (size_t i = 0; i < branch.size(); i++) fprintf(f, "%s%s", i ? ",\n" : "\n", branch[i].c_str());
    fprintf(f, "]}\n");
    fclose(f);
  }
  if (!proved) { fprintf(stderr, "[search] failing branch (%zu states):\n", branch.size()); for (auto& b : branch) fprintf(stderr, "   %s\n", b.c_str()); }
  fprintf(stderr, "[search] proved=%d seconds=%.1f peak_rss=%.0f MB\n", (int)proved, secs, ru.ru_maxrss / 1024.0);
  return proved ? 0 : 1;
}

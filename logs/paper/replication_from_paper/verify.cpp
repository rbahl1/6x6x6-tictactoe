// verify.cpp -- independent replication verifier, written from paper.pdf alone.
// Implements: geometry (Sec. 2.1), group G (Sec. 2.3), canonical form (Def. 2.7, brute force
// over the 1,152 maps), admissibility (Def. 4.1), LEAF (Def. 4.7 / Algorithm 2), LOST (Def. 5.2),
// and Algorithm 1 (Def. 5.6).  Exact integer arithmetic only; visited set keyed by full states.
//
// usage:  verify selftest            geometry / group / canon / LEAF / LOST test vectors
//         verify leaftable           orbit counts and Table 5 (leaf table of busy sets)
//         verify run strategy.tbl leaves.txt [rmax]
#include <bits/stdc++.h>
using namespace std;
typedef vector<uint8_t> VS;   // a set of cells: ascending list of ids

struct M4 { uint64_t w[4]; };
static inline M4 mzero() { M4 m; m.w[0] = m.w[1] = m.w[2] = m.w[3] = 0; return m; }
static inline void mset(M4& m, int i) { m.w[i >> 6] |= 1ULL << (i & 63); }
static inline M4 mor(const M4& a, const M4& b) { M4 m; for (int i = 0; i < 4; i++) m.w[i] = a.w[i] | b.w[i]; return m; }
static inline int mpop(const M4& a) { return __builtin_popcountll(a.w[0]) + __builtin_popcountll(a.w[1]) + __builtin_popcountll(a.w[2]) + __builtin_popcountll(a.w[3]); }

static int NL = 0;
static int LC[160][6];          // cells of each line (ascending)
static int LKIND[160];          // number of active coordinates
static bool ONL[160][216];
static M4 LMASK[160];
static vector<int> THR[216];    // lines through a cell
static int COMMON[160][160];    // the common cell of two distinct lines, or -1
static vector<array<uint8_t, 216>> G;
static bool BUSY[216];

static int cid(int x, int y, int z) { return 36 * x + 6 * y + z; }
static void die(const string& s) { printf("FATAL: %s\n", s.c_str()); exit(2); }

static void buildGeometry() {
  map<array<int, 6>, int> S;
  for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) for (int f3 = 0; f3 < 8; f3++) {
    int f[3] = {f1, f2, f3}; int act = 0;
    for (int i = 0; i < 3; i++) if (f[i] >= 6) act++;
    if (!act) continue;
    array<int, 6> cells;
    for (int t = 0; t < 6; t++) {
      int c[3];
      for (int i = 0; i < 3; i++) c[i] = f[i] < 6 ? f[i] : (f[i] == 6 ? t : 5 - t);
      cells[t] = cid(c[0], c[1], c[2]);
    }
    sort(cells.begin(), cells.end());
    S[cells] = act;
  }
  NL = 0;
  for (auto& kv : S) {
    if (NL >= 160) die("too many lines");
    LMASK[NL] = mzero();
    for (int i = 0; i < 6; i++) { LC[NL][i] = kv.first[i]; ONL[NL][kv.first[i]] = true; mset(LMASK[NL], kv.first[i]); }
    LKIND[NL] = kv.second; NL++;
  }
  for (int l = 0; l < NL; l++) for (int i = 0; i < 6; i++) THR[LC[l][i]].push_back(l);
  for (int a = 0; a < NL; a++) for (int b = 0; b < NL; b++) {
    COMMON[a][b] = -1; if (a == b) continue;
    int n = 0;
    for (int i = 0; i < 6; i++) if (ONL[b][LC[a][i]]) { COMMON[a][b] = LC[a][i]; n++; }
    if (n > 1) die("two lines share more than one cell");
  }
  for (int x = 0; x < 6; x++) for (int y = 0; y < 6; y++) for (int z = 0; z < 6; z++) {
    int lx = min(x, 5 - x), ly = min(y, 5 - y), lz = min(z, 5 - z);
    BUSY[cid(x, y, z)] = (lx == ly && ly == lz);
  }
  // group G = { Sigma_s o P_r o Pi_pi }
  set<array<uint8_t, 216>> seen;
  int perms[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  for (int s0 = 0; s0 < 6; s0++) for (int s1 = 0; s1 < 6; s1++) for (int s2 = 0; s2 < 6; s2++) {
    if (s1 == s0 || s1 == 5 - s0) continue;
    if (s2 == s0 || s2 == 5 - s0 || s2 == s1 || s2 == 5 - s1) continue;
    int s[6]; s[0] = s0; s[1] = s1; s[2] = s2; s[5] = 5 - s0; s[4] = 5 - s1; s[3] = 5 - s2;
    for (int p = 0; p < 6; p++) for (int r = 0; r < 8; r++) {
      array<uint8_t, 216> g;
      for (int x = 0; x < 6; x++) for (int y = 0; y < 6; y++) for (int z = 0; z < 6; z++) {
        int c[3] = {x, y, z}, d[3];
        for (int i = 0; i < 3; i++) d[perms[p][i]] = c[i];
        for (int j = 0; j < 3; j++) if (r >> j & 1) d[j] = 5 - d[j];
        for (int j = 0; j < 3; j++) d[j] = s[d[j]];
        g[cid(x, y, z)] = (uint8_t)cid(d[0], d[1], d[2]);
      }
      if (seen.insert(g).second) G.push_back(g);
    }
  }
}

// ---------- canonical form (Def. 2.7): minimum over all g of (gB, gF) ----------
// order: A < A' iff the smallest cell of the symmetric difference is in A.  With cell i stored at
// bit 215-i of a 256-bit integer this is "larger integer = smaller set".
static inline void rset(M4& m, int cell) { int p = 215 - cell; m.w[p >> 6] |= 1ULL << (p & 63); }
static inline int rcmp(const M4& a, const M4& b) {   // -1: a precedes b
  for (int i = 3; i >= 0; i--) if (a.w[i] != b.w[i]) return a.w[i] > b.w[i] ? -1 : 1;
  return 0;
}
static int canon(const VS& B, const VS& F, VS& cB, VS& cF) {
  int best = -1; M4 bb = mzero(), bf = mzero();
  for (size_t g = 0; g < G.size(); g++) {
    const auto& p = G[g]; M4 mb = mzero();
    for (uint8_t b : B) rset(mb, p[b]);
    int c = best < 0 ? -1 : rcmp(mb, bb);
    if (c > 0) continue;
    M4 mf = mzero();
    for (uint8_t f : F) rset(mf, p[f]);
    if (c < 0 || rcmp(mf, bf) < 0) { best = (int)g; bb = mb; bf = mf; }
  }
  cB.clear(); cF.clear();
  for (uint8_t b : B) cB.push_back(G[best][b]);
  for (uint8_t f : F) cF.push_back(G[best][f]);
  sort(cB.begin(), cB.end()); sort(cF.begin(), cF.end());
  return best;
}
// literal reference version: sorted lists compared lexicographically (sets of equal size)
static void canonRef(const VS& B, const VS& F, VS& cB, VS& cF) {
  bool first = true;
  for (auto& p : G) {
    VS b, f;
    for (uint8_t x : B) b.push_back(p[x]);
    for (uint8_t x : F) f.push_back(p[x]);
    sort(b.begin(), b.end()); sort(f.begin(), f.end());
    if (first || b < cB || (b == cB && f < cF)) { cB = b; cF = f; first = false; }
  }
}

// ---------- admissibility (Def. 4.1) ----------
static bool admissible(const VS& B, const VS& F) {
  bool inB[216] = {}, inF[216] = {};
  for (uint8_t b : B) inB[b] = true;
  for (uint8_t f : F) { if (inB[f]) return false; inF[f] = true; }
  int four[160], nf = 0;
  for (int l = 0; l < NL; l++) {
    int cnt = 0; bool live = true;
    for (int i = 0; i < 6; i++) { if (inB[LC[l][i]]) live = false; if (inF[LC[l][i]]) cnt++; }
    if (!live) continue;
    if (cnt > 4) return false;
    if (cnt == 4) four[nf++] = l;
  }
  for (int i = 0; i < nf; i++) for (int j = i + 1; j < nf; j++) {
    int c = COMMON[four[i]][four[j]];
    if (c >= 0 && !inF[c]) return false;
  }
  return true;
}

// ---------- LEAF (Algorithm 2) ----------
struct LeafCtx {
  bool inF[216]; bool live[160]; int L[160]; int nL;
};
static bool augment(int* own, const int* src, int nsrc, const LeafCtx& cx) {
  int par[160], parc[160], q[160]; bool vis[160];
  for (int i = 0; i < NL; i++) vis[i] = false;
  int qh = 0, qt = 0;
  for (int i = 0; i < nsrc; i++) if (!vis[src[i]]) { vis[src[i]] = true; par[src[i]] = -1; parc[src[i]] = -1; q[qt++] = src[i]; }
  while (qh < qt) {
    int l = q[qh++];
    for (int i = 0; i < 6; i++) {
      int c = LC[l][i];
      if (cx.inF[c]) continue;
      int o = own[c];
      if (o < 0) {
        int cl = l, cc = c;
        while (true) { own[cc] = cl; if (par[cl] < 0) break; int nc = parc[cl]; cl = par[cl]; cc = nc; }
        return true;
      }
      if (!vis[o]) { vis[o] = true; par[o] = l; parc[o] = c; q[qt++] = o; }
    }
  }
  return false;
}
// literal check of a final assignment (Remark 7.3): returns true iff own2 is a U-assignment of excess theta
static bool validateAssign(const int* own2, const int* U, int nU, int theta, const LeafCtx& cx) {
  int cnt[160]; for (int i = 0; i < NL; i++) cnt[i] = 0;
  for (int c = 0; c < 216; c++) if (own2[c] >= 0) {
    int l = own2[c];
    if (!cx.live[l] || !ONL[l][c] || cx.inF[c]) return false;
    cnt[l]++;
  }
  bool inU[160]; for (int i = 0; i < NL; i++) inU[i] = false;
  for (int i = 0; i < nU; i++) inU[U[i]] = true;
  int sumU = 0, distinctU = 0;
  for (int i = 0; i < cx.nL; i++) {
    int l = cx.L[i];
    if (inU[l]) { if (cnt[l] < 2) return false; sumU += cnt[l]; distinctU++; }
    else if (cnt[l] != 2) return false;
  }
  return distinctU == nU && sumU == 2 * nU + theta;
}
struct LeafStat { long long single = 0, triple = 0, validated = 0, singleH1 = 0, singleH3 = 0, singleTh2 = 0, weakLines = 0, singleNotTriv = 0, pairA = 0, tripleA = 0, pairB = 0, tripleB = 0; };
static int gain(const int* own, const int* U, int nU, int theta, const LeafCtx& cx, LeafStat* st) {
  int own2[216]; memcpy(own2, own, sizeof(own2));
  int n = 0;
  while (n < theta && augment(own2, U, nU, cx)) n++;
  if (n == theta) {
    if (!validateAssign(own2, U, nU, theta, cx)) die("internal: flow validation failed");
    if (st) st->validated++;
  }
  return n;
}
// returns 0 if LEAF(B,F,k) holds, else the number i of the first failing condition (Li)
// mode 0: no pruning; 1: pruning by strong lines (Algorithm 2 as printed); 2: also by strong F-adjacent pairs; 3: by all pairs of the triple
static int leafTest(const VS& B, const VS& F, int k, LeafStat* st = nullptr, int mode = 1) {
  if ((int)F.size() > k) die("leafTest: k < |F|");
  if (!admissible(B, F)) return 1;
  LeafCtx cx; bool inB[216] = {};
  for (int c = 0; c < 216; c++) cx.inF[c] = false;
  for (uint8_t b : B) inB[b] = true;
  for (uint8_t f : F) cx.inF[f] = true;
  cx.nL = 0;
  for (int l = 0; l < NL; l++) {
    bool lv = true;
    for (int i = 0; i < 6; i++) if (inB[LC[l][i]]) lv = false;
    cx.live[l] = lv; if (lv) cx.L[cx.nL++] = l;
  }
  int own[216]; for (int c = 0; c < 216; c++) own[c] = -1;
  for (int i = 0; i < cx.nL; i++) for (int j = 0; j < 2; j++) { int l = cx.L[i]; if (!augment(own, &l, 1, cx)) return 2; }
  if (!validateAssign(own, nullptr, 0, 0, cx)) die("internal: pairing validation failed");
  int theta = k - (int)F.size();
  if (theta <= 0) return 0;
  bool weak[160]; for (int i = 0; i < NL; i++) weak[i] = false;
  for (int i = 0; i < cx.nL; i++) {
    int l = cx.L[i];
    if (st && mode == 1) {
      st->single++;
      int freeCells = 0, unassigned = 0;
      for (int j = 0; j < 6; j++) if (!cx.inF[LC[l][j]]) { freeCells++; if (own[LC[l][j]] < 0) unassigned++; }
      if (freeCells - 2 >= theta) st->singleH1++;
      if (unassigned < theta) st->singleH3++;
      if (theta >= 2) st->singleTh2++;
      if (theta >= 2 && freeCells - 2 >= theta) st->singleNotTriv++;
    }
    if (mode == 0 || gain(own, &l, 1, theta, cx, mode == 1 ? st : nullptr) < theta) { weak[l] = true; if (st && mode == 1) st->weakLines++; }
  }
  vector<signed char> pm; if (mode >= 2) pm.assign(160 * 160, -1);
  auto pairStrong = [&](int a, int b) -> bool {
    if (theta > 8) return false;
    if (a > b) swap(a, b);
    signed char& v = pm[a * 160 + b];
    if (v < 0) { int U[2] = {a, b}; v = gain(own, U, 2, theta, cx, nullptr) >= theta; if (st) { if (mode == 2) st->pairA++; else st->pairB++; } }
    return v;
  };
  for (int i = 0; i < cx.nL; i++) {
    int b = cx.L[i]; if (!weak[b]) continue;
    int nb[160], nn = 0;
    for (int j = 0; j < cx.nL; j++) {
      int a = cx.L[j]; if (a == b || !weak[a]) continue;
      int c = COMMON[a][b];
      if (c >= 0 && !cx.inF[c]) nb[nn++] = a;
    }
    for (int x = 0; x < nn; x++) for (int y = x + 1; y < nn; y++) {
      int a = nb[x], c = nb[y];
      int cc = COMMON[a][c];
      if (cc >= 0 && !cx.inF[cc] && a < b) continue;   // pairwise adjacent triple: test once (centre = smallest line)
      int T[3] = {a, b, c};
      if (mode >= 2 && (pairStrong(a, b) || pairStrong(b, c))) continue;
      if (mode == 3 && pairStrong(a, c)) continue;
      if (st) { if (mode == 1) st->triple++; else if (mode == 2) st->tripleA++; else if (mode == 3) st->tripleB++; }
      if (gain(own, T, 3, theta, cx, mode == 1 ? st : nullptr) < theta) return 3;
    }
  }
  return 0;
}
// direct evaluation of Def. 4.7 for few live lines (all subsets)
static int leafBrute(const VS& B, const VS& F, int k) {
  if (!admissible(B, F)) return 1;
  bool inB[216] = {}; M4 mF = mzero();
  for (uint8_t b : B) inB[b] = true;
  for (uint8_t f : F) mset(mF, f);
  vector<int> L;
  for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (inB[LC[l][i]]) lv = false; if (lv) L.push_back(l); }
  int n = L.size(); if (n > 20) die("leafBrute: too many lines");
  vector<M4> U((size_t)1 << n); U[0] = mzero();
  vector<uint32_t> adj(n, 0);
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) if (i != j) {
    int c = COMMON[L[i]][L[j]];
    if (c >= 0 && !(mF.w[c >> 6] >> (c & 63) & 1)) adj[i] |= 1u << j;
  }
  int theta = k - (int)F.size();
  bool fail3 = false;
  for (uint32_t S = 1; S < (1u << n); S++) {
    int lb = __builtin_ctz(S);
    U[S] = mor(U[S & (S - 1)], LMASK[L[lb]]);
    M4 x = U[S]; for (int i = 0; i < 4; i++) x.w[i] &= ~mF.w[i];
    int sur = mpop(x) - 2 * __builtin_popcount(S);
    if (sur < 0) return 2;
    if (__builtin_popcount(S) >= 3 && sur < theta) {
      uint32_t comp = 1u << lb, fr = comp;
      while (fr) { int v = __builtin_ctz(fr); fr &= fr - 1; uint32_t nw = adj[v] & S & ~comp; comp |= nw; fr |= nw; }
      if (comp == S) fail3 = true;
    }
  }
  return fail3 ? 3 : 0;
}

// ---------- LOST (Def. 5.2), literal; returns bit 0: an E2 pattern fits, bit 1: an E3 pattern fits ----------
static int lostTest(const VS& B, const VS& F, int t) {
  bool inB[216] = {}, inF[216] = {}; M4 mF = mzero();
  for (uint8_t b : B) inB[b] = true;
  for (uint8_t f : F) { inF[f] = true; mset(mF, f); }
  bool live[160]; int cf[160]; int nF = F.size(); int res = 0;
  for (int l = 0; l < NL; l++) { live[l] = true; cf[l] = 0; for (int i = 0; i < 6; i++) { if (inB[LC[l][i]]) live[l] = false; if (inF[LC[l][i]]) cf[l]++; } }
  auto minus2 = [&](int l, int c1, int c2) { M4 m = mzero(); for (int i = 0; i < 6; i++) if (LC[l][i] != c1 && LC[l][i] != c2) mset(m, LC[l][i]); return m; };
  // E2: lines l,a,b; cells x,e,c
  for (int l = 0; l < NL && !(res & 1); l++) if (live[l]) for (int ei = 0; ei < 6 && !(res & 1); ei++) {
    int e = LC[l][ei]; if (inF[e]) continue;
    M4 u1 = mor(mF, minus2(l, e, -1));
    if (mpop(u1) > t + 1) continue;
    for (int a = 0; a < NL && !(res & 1); a++) if (live[a] && a != l) {
      int x = COMMON[l][a]; if (x < 0 || x == e) continue;
      for (int b = 0; b < NL && !(res & 1); b++) if (live[b] && b != a && b != l) {
        int c = COMMON[a][b]; if (c < 0 || c == x || c == e || inF[c]) continue;
        if (ONL[b][x] || ONL[b][e]) continue;
        for (int ai = 0; ai < 6 && !(res & 1); ai++) {
          int a1 = LC[a][ai]; if (a1 == c || a1 == x || inF[a1]) continue;
          M4 u2 = mor(u1, minus2(a, c, a1));
          if (mpop(u2) > t + 1) continue;
          for (int bi = 0; bi < 6; bi++) {
            int b1 = LC[b][bi]; if (b1 == c || inF[b1]) continue;
            M4 u3 = mor(u2, minus2(b, c, b1));
            if (mpop(u3) <= t + 1) { res |= 1; break; }
          }
        }
      }
    }
  }
  // E3: lines a,a',b,b'; cells x,c,c'
  for (int x = 0; x < 216 && !(res & 2); x++) {
    for (size_t i = 0; i < THR[x].size() && !(res & 2); i++) for (size_t j = i + 1; j < THR[x].size() && !(res & 2); j++) {
      int a = THR[x][i], a2 = THR[x][j];
      if (!live[a] || !live[a2] || cf[a] > 4 || cf[a2] > 4) continue;
      if (nF + (4 - cf[a]) + (4 - cf[a2]) - (inF[x] ? 0 : 1) > t + 1) continue;   // |F u Sa u Sa'| (identity)
      for (int b = 0; b < NL && !(res & 2); b++) if (live[b] && b != a && b != a2) {
        int c = COMMON[a][b]; if (c < 0 || c == x || inF[c]) continue;
        if (ONL[b][x]) continue;
        for (int b2 = 0; b2 < NL && !(res & 2); b2++) if (live[b2] && b2 != a && b2 != a2 && b2 != b) {
          int c2 = COMMON[a2][b2]; if (c2 < 0 || c2 == x || c2 == c || inF[c2]) continue;
          if (ONL[b2][x]) continue;
          for (int pi = 0; pi < 6 && !(res & 2); pi++) {
            int p = LC[a][pi]; if (p == c || p == x || inF[p] || p == c2) continue;
            M4 sa = minus2(a, c, p); if (ONL[a][c2]) continue;   // c' must not be in Sa (cannot happen)
            for (int pj = 0; pj < 6 && !(res & 2); pj++) {
              int p2 = LC[a2][pj]; if (p2 == c2 || p2 == x || inF[p2]) continue;
              if (ONL[a2][c]) continue;
              M4 u2 = mor(mor(mF, sa), minus2(a2, c2, p2));
              if (mpop(u2) > t + 1) continue;
              for (int qi = 0; qi < 6 && !(res & 2); qi++) {
                int q = LC[b][qi]; if (q == c || inF[q]) continue;
                if (ONL[b][c2] && q != c2) continue;
                M4 u3 = mor(u2, minus2(b, c, q));
                if (mpop(u3) > t + 1) continue;
                for (int qj = 0; qj < 6; qj++) {
                  int q2 = LC[b2][qj]; if (q2 == c2 || inF[q2]) continue;
                  if (ONL[b2][c] && q2 != c) continue;
                  M4 u4 = mor(u3, minus2(b2, c2, q2));
                  if (mpop(u4) <= t + 1) { res |= 2; break; }
                }
              }
            }
          }
        }
      }
    }
  }
  return res;
}

// ---------- parsing ----------
static bool parseInt(const string& s, int& v) {
  if (s.empty() || s.size() > 4) return false;
  for (char ch : s) if (ch < '0' || ch > '9') return false;
  if (s.size() > 1 && s[0] == '0') return false;
  v = atoi(s.c_str()); return true;
}
static bool parseSet(const string& s, VS& out) {
  out.clear(); if (s == "-") return true;
  size_t i = 0;
  while (true) {
    size_t j = s.find(',', i);
    string tok = s.substr(i, j == string::npos ? string::npos : j - i);
    int v; if (!parseInt(tok, v) || v > 215) return false;
    if (!out.empty() && v <= out.back()) return false;
    out.push_back((uint8_t)v);
    if (j == string::npos) break;
    i = j + 1;
  }
  return true;
}
static VS S(const string& s) { VS v; if (!parseSet(s, v)) die("bad set literal " + s); return v; }
static string setStr(const VS& v) { if (v.empty()) return "-"; string s; for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += to_string(v[i]); } return s; }
static vector<string> splitSp(const string& s) { vector<string> r; size_t i = 0; while (true) { size_t j = s.find(' ', i); r.push_back(s.substr(i, j == string::npos ? string::npos : j - i)); if (j == string::npos) break; i = j + 1; } return r; }
static string key(char type, int t, const VS& B, const VS& F) {
  string k; k.push_back(type); k.push_back((char)t); k.push_back((char)B.size());
  k.append((const char*)B.data(), B.size()); k.append((const char*)F.data(), F.size()); return k;
}
static bool disjoint(const VS& a, const VS& b) { for (uint8_t x : a) if (binary_search(b.begin(), b.end(), x)) return false; return true; }
static VS plusCell(const VS& a, int c) { VS r = a; r.push_back((uint8_t)c); sort(r.begin(), r.end()); return r; }
static bool has(const VS& a, int c) { return binary_search(a.begin(), a.end(), (uint8_t)c); }

static int nfail = 0;
static void check(bool ok, const string& what) { printf("  [%s] %s\n", ok ? "ok" : "MISMATCH", what.c_str()); if (!ok) nfail++; }

// ---------- self tests ----------
static void selftest() {
  printf("== geometry ==\n");
  int k1 = 0, k2 = 0, k3 = 0; for (int l = 0; l < NL; l++) { if (LKIND[l] == 1) k1++; if (LKIND[l] == 2) k2++; if (LKIND[l] == 3) k3++; }
  check(NL == 148 && k1 == 108 && k2 == 36 && k3 == 4, "148 lines = 108 axis + 36 face diagonals + 4 space diagonals (got " + to_string(NL) + "=" + to_string(k1) + "+" + to_string(k2) + "+" + to_string(k3) + ")");
  int n7 = 0, n4 = 0, n3 = 0, inc = 0; bool busyOk = true;
  for (int c = 0; c < 216; c++) { int d = THR[c].size(); inc += d; if (d == 7) n7++; else if (d == 4) n4++; else if (d == 3) n3++; if ((d == 7) != BUSY[c]) busyOk = false; }
  check(n7 == 24 && n4 == 144 && n3 == 48 && inc == 888, "cells on 7/4/3 lines: 24/144/48, 888 incidences");
  int busyIds[24] = {0, 5, 30, 35, 43, 46, 61, 64, 86, 87, 92, 93, 122, 123, 128, 129, 151, 154, 169, 172, 180, 185, 210, 215};
  bool bl = busyOk; { int j = 0; for (int c = 0; c < 216; c++) if (BUSY[c]) { if (j >= 24 || busyIds[j] != c) bl = false; j++; } if (j != 24) bl = false; }
  check(bl, "busy cells = cells on 7 lines = the 24 ids of Appendix A.1");
  int noBusyAxis = 0; for (int l = 0; l < NL; l++) if (LKIND[l] == 1) { int nb = 0; for (int i = 0; i < 6; i++) nb += BUSY[LC[l][i]]; if (nb == 0) noBusyAxis++; }
  check(noBusyAxis == 72, "72 axis lines without busy cell (Lemma 2.5b)");
  printf("== group ==\n");
  check(G.size() == 1152, "|G| = 1152 (got " + to_string(G.size()) + ")");
  { bool ok = true; set<array<int, 6>> ls; for (int l = 0; l < NL; l++) { array<int, 6> a; for (int i = 0; i < 6; i++) a[i] = LC[l][i]; ls.insert(a); }
    for (auto& g : G) { bool bij[216] = {}; for (int c = 0; c < 216; c++) bij[g[c]] = true; for (int c = 0; c < 216; c++) if (!bij[c]) ok = false;
      for (int l = 0; l < NL; l++) { array<int, 6> a; for (int i = 0; i < 6; i++) a[i] = g[LC[l][i]]; sort(a.begin(), a.end()); if (!ls.count(a)) ok = false; } }
    check(ok, "every g in G is a bijection mapping lines onto lines"); }
  { set<array<uint8_t, 216>> gs(G.begin(), G.end()); bool ok = true;
    for (size_t i = 0; i < G.size() && ok; i++) for (size_t j = 0; j < G.size(); j++) { array<uint8_t, 216> h; for (int c = 0; c < 216; c++) h[c] = G[i][G[j][c]]; if (!gs.count(h)) { ok = false; break; } }
    check(ok, "G is closed under composition (all 1152^2 products)"); }
  { int orb[216]; for (int c = 0; c < 216; c++) orb[c] = c;
    for (auto& g : G) for (int c = 0; c < 216; c++) orb[g[c]] = min(orb[g[c]], orb[c]);
    for (int it = 0; it < 5; it++) for (auto& g : G) for (int c = 0; c < 216; c++) { int m = min(orb[g[c]], orb[c]); orb[g[c]] = m; orb[c] = m; }
    map<int, int> sz; for (int c = 0; c < 216; c++) sz[orb[c]]++;
    check(sz.size() == 3 && sz[cid(0, 0, 0)] == 24 && sz[cid(0, 0, 1)] == 144 && sz[cid(0, 1, 2)] == 48, "3 cell orbits with representatives (0,0,0),(0,0,1),(0,1,2), sizes 24/144/48"); }
  printf("== canonical form (Appendix A.2) ==\n");
  struct CV { const char *b, *f, *cb, *cf; } cv[] = {
    {"43", "0", "0", "43"},
    {"0,46,92,123,169,180,215", "-", "0,5,61,93,122,154,210", "-"},
    {"0,35,46,92,123,169,180,215", "-", "0,5,61,93,122,154,210,215", "-"},
    {"5,30,43,64,86,93,128,169,180", "0,35,46,87,122,151,172,185,210", "0,35,43,64,86,93,123,154,210", "5,30,61,122,129,151,172,180,215"}};
  for (auto& v : cv) { VS cb, cf, rb, rf; canon(S(v.b), S(v.f), cb, cf); canonRef(S(v.b), S(v.f), rb, rf);
    check(cb == S(v.cb) && cf == S(v.cf) && rb == cb && rf == cf, string("canon(") + v.b + " | " + v.f + ") = " + setStr(cb) + " | " + setStr(cf)); }
  { // random agreement of the fast canon with the literal one, and invariance under G
    mt19937 rng(12345); bool ok = true;
    for (int it = 0; it < 300; it++) { vector<int> cells(216); iota(cells.begin(), cells.end(), 0); shuffle(cells.begin(), cells.end(), rng);
      int nb = rng() % 11, nf2 = rng() % 11; VS B, F; for (int i = 0; i < nb; i++) B.push_back(cells[i]); for (int i = 0; i < nf2; i++) F.push_back(cells[nb + i]);
      sort(B.begin(), B.end()); sort(F.begin(), F.end()); VS cb, cf, rb, rf; canon(B, F, cb, cf); canonRef(B, F, rb, rf); if (cb != rb || cf != rf) ok = false;
      auto& g = G[rng() % G.size()]; VS B2, F2; for (auto x : B) B2.push_back(g[x]); for (auto x : F) F2.push_back(g[x]); sort(B2.begin(), B2.end()); sort(F2.begin(), F2.end());
      VS c2, f2; canon(B2, F2, c2, f2); if (c2 != cb || f2 != cf) ok = false; }
    check(ok, "fast canon == literal canon and canon(gB,gF)=canon(B,F) on 300 random pairs"); }
  printf("== T7 / T8 (Examples 3.8, 3.9, Computed Facts 3.10, 4.23) ==\n");
  VS T7 = S("0,46,92,123,169,180,215"), T8 = S("0,35,46,92,123,169,180,215");
  auto liveCount = [&](const VS& B) { int n = 0; for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (has(B, LC[l][i])) lv = false; n += lv; } return n; };
  check(liveCount(T7) == 104 && 216 - 7 - 2 * liveCount(T7) == 1, "|L(T7)| = 104, slack(T7) = 1");
  check(liveCount(T8) == 100 && 216 - 8 - 2 * liveCount(T8) == 8, "|L(T8)| = 100, slack(T8) = 8");
  check(leafTest(T7, {}, 0) == 0 && leafTest(T8, {}, 0) == 0, "pairings for (T7,0) and (T8,0) exist");
  check(leafTest(T7, {}, 1) == 0 && leafTest(T7, {}, 2) == 3, "LEAF(T7,0,1) true, LEAF(T7,0,2) false (L3)");
  for (const VS* T : {&T7, &T8}) { // connectedness and covering of L(T)
    vector<int> L; for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (has(*T, LC[l][i])) lv = false; if (lv) L.push_back(l); }
    set<int> comp = {L[0]}; bool grow = true; while (grow) { grow = false; for (int l : L) if (!comp.count(l)) for (int m : comp) if (COMMON[l][m] >= 0) { comp.insert(l); grow = true; break; } }
    M4 u = mzero(); for (int l : L) u = mor(u, LMASK[l]);
    check(comp.size() == L.size() && mpop(u) == 216 - (int)T->size(), "L(T" + to_string(T->size()) + ") is connected and covers all cells outside the set"); }
  printf("== LEAF test vectors (Table 11) ==\n");
  struct LV { int k; string b, f; int exp; };
  vector<LV> lv = {{7, "0,46,92,123,169,180,215", "-", 3}, {8, "0,35,46,92,123,169,180,215", "-", 0}, {9, "0,35,46,92,123,169,180,215", "-", 3}};
  for (int h : {5, 30, 43, 61, 64, 86, 87, 93, 122, 128, 129, 151, 154, 172, 185, 210}) lv.push_back({8, "0,35,46,92,123,169,180,215", to_string(h), 0});
  lv.push_back({8, "0,35,43,46,92,123,169,180", "215", 0});
  lv.push_back({8, "0,5,35,43,46,61,86,92", "123,129,169,172,180,210,215", 2});
  lv.push_back({9, "0,5,35,43,46,61,86,87,92", "123,129,154,169,172,180,210,215", 0});
  lv.push_back({10, "0,5,30,35,43,46,61,86,87,92", "123,128,129,154,169,172,180,210,215", 0});
  lv.push_back({9, "0,8,35,46,92,123,169,180,215", "44,80,116,152,188", 0});
  lv.push_back({8, "0,8,46,92,123,169,180,215", "44,80,116,152,188", 3});
  lv.push_back({0, "-", "-", 2});
  for (auto& v : lv) { int r = leafTest(S(v.b), S(v.f), v.k); int r2 = leafTest(S(v.b), S(v.f), v.k, nullptr, 0); if (leafTest(S(v.b), S(v.f), v.k, nullptr, 2) != r || leafTest(S(v.b), S(v.f), v.k, nullptr, 3) != r) r2 = -1;
    check(r == v.exp && r2 == v.exp, to_string(v.k) + " " + v.b + " " + v.f + " -> " + (r ? "false (" + to_string(r) + ")" : string("true")) + (r2 != r ? " [unpruned differs!]" : "")); }
  { // Example 4.11
    VS B; for (int c = 0; c < 216; c++) { int x = c / 36, y = c / 6 % 6, z = c % 6; bool inN = (x == 0) && (y == 0 || z == 0 || y == z); if (!inN) B.push_back(c); }
    check(B.size() == 200 && leafTest(B, {}, 11) == 3 && leafTest(B, {}, 10) == 0 && leafBrute(B, {}, 11) == 3 && leafBrute(B, {}, 10) == 0, "Example 4.11: LEAF(B,0,11) false by (L3), LEAF(B,0,10) true"); }
  printf("== Algorithm 2 against the direct definition (Def. 4.7) on random small instances ==\n");
  { mt19937 rng(777); int cnt[4] = {0, 0, 0, 0}, bad = 0, tot = 0;
    for (int it = 0; it < 20000; it++) {
      vector<int> ls; int target = 3 + rng() % 10; ls.push_back(rng() % NL);
      while ((int)ls.size() < target) { int l = rng() % NL; bool meets = false; for (int m : ls) if (m != l && COMMON[l][m] >= 0) meets = true; if (find(ls.begin(), ls.end(), l) != ls.end()) continue; if (meets || rng() % 4 == 0) ls.push_back(l); }
      bool inN[216] = {}; for (int l : ls) for (int i = 0; i < 6; i++) inN[LC[l][i]] = true;
      VS B, F; vector<int> free_; for (int c = 0; c < 216; c++) { if (!inN[c]) B.push_back(c); else free_.push_back(c); }
      if (rng() % 3 == 0) { int extra = rng() % 3; for (int i = 0; i < extra; i++) { int c = free_[rng() % free_.size()]; if (!has(B, c)) B = plusCell(B, c); } }
      int nlive = 0; for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (has(B, LC[l][i])) lv = false; nlive += lv; }
      if (nlive > 16 || nlive == 0) continue;
      int nf2 = (it % 2) ? rng() % 7 : rng() % 20; for (int i = 0; i < nf2; i++) { int c = free_[rng() % free_.size()]; if (!has(B, c) && !has(F, c)) F = plusCell(F, c); }
      int theta = (it % 3 == 0) ? rng() % 3 : rng() % 12; int k = F.size() + theta;
      int r1 = leafTest(B, F, k), r2 = leafBrute(B, F, k), r3 = leafTest(B, F, k, nullptr, 0), r4 = leafTest(B, F, k, nullptr, 2), r5 = leafTest(B, F, k, nullptr, 3); tot++; cnt[r2]++;
      if (r1 != r2 || r3 != r2 || r4 != r2 || r5 != r2) bad++;
    }
    check(bad == 0, to_string(tot) + " instances (true/L1/L2/L3 = " + to_string(cnt[0]) + "/" + to_string(cnt[1]) + "/" + to_string(cnt[2]) + "/" + to_string(cnt[3]) + "), mismatches: " + to_string(bad)); }
  printf("== LOST (Def. 5.2, Prop. 5.9, Appendix C.3 remark) on the empty record ==\n");
  { int r9 = lostTest({}, {}, 9), r10 = lostTest({}, {}, 10), r11 = lostTest({}, {}, 11), r12 = lostTest({}, {}, 12);
    printf("  LOST(0,0,t) pattern bits (1=E2, 2=E3): t=9:%d t=10:%d t=11:%d t=12:%d\n", r9, r10, r11, r12);
    check(r9 == 0, "no pattern fits 10 stones (Prop. 5.9c)");
    check((r10 & 1) && !(r10 & 2), "E2 attains 11 stones; no E3 with 11 stones");
    check(!(r11 & 2) && (r12 & 2), "an E3 pattern needs at least 13 stones on this board (stated in Sec. 5.6)"); }
}

// ---------- orbit counts and leaf table of busy sets (Appendix A.7, Table 5) ----------
static void leaftable() {
  printf("== orbits of busy k-sets ==\n");
  int bid[24], bidx[216], nb = 0; for (int c = 0; c < 216; c++) { bidx[c] = -1; if (BUSY[c]) { bidx[c] = nb; bid[nb++] = c; } }
  vector<array<uint8_t, 24>> GB; for (auto& g : G) { array<uint8_t, 24> p; for (int i = 0; i < 24; i++) { if (bidx[g[bid[i]]] < 0) die("busy not preserved"); p[i] = bidx[g[bid[i]]]; } GB.push_back(p); }
  // Burnside
  long long expOrb[4] = {527, 1057, 1669, 2468};
  for (int k = 7; k <= 10; k++) {
    __int128 tot = 0;
    for (auto& p : GB) { bool vis[24] = {}; vector<long long> poly(25, 0); poly[0] = 1;
      for (int i = 0; i < 24; i++) if (!vis[i]) { int len = 0, j = i; while (!vis[j]) { vis[j] = true; j = p[j]; len++; } for (int d = 24; d >= len; d--) poly[d] += poly[d - len]; }
      tot += poly[k]; }
    long long orb = (long long)(tot / 1152); bool exact = (tot % 1152 == 0);
    check(exact && orb == expOrb[k - 7], "Burnside: orbits of busy " + to_string(k) + "-sets = " + to_string(orb));
  }
  vector<uint8_t> visited(1u << 24, 0);
  long long expSlack[3] = {6, 947, 2185}, expLeaf[3] = {6, 172, 0};
  const char* reps8[6] = {"0,5,43,61,93,122,154,210", "0,5,43,61,93,122,154,215", "0,5,43,61,93,122,172,215", "0,5,43,92,93,122,172,210", "0,5,43,92,93,122,172,215", "0,5,61,93,122,154,210,215"};
  int osz8[6] = {288, 576, 288, 576, 576, 36};
  for (int k = 7; k <= 10; k++) {
    struct Orb { VS rep; int size; int slack; int leaf; };
    vector<Orb> orbs;
    uint32_t v = (1u << k) - 1;
    while (v < (1u << 24)) {
      if (!visited[v]) {
        // orbit of v; canonical rep = smallest in the set order = lexicographically smallest sorted list
        VS cells; for (int i = 0; i < 24; i++) if (v >> i & 1) cells.push_back(i);
        set<uint32_t> orb; VS best;
        for (auto& p : GB) { uint32_t m = 0; for (uint8_t c : cells) m |= 1u << p[c]; if (orb.insert(m).second) { visited[m] = 1; VS im; for (int i = 0; i < 24; i++) if (m >> i & 1) im.push_back(bid[i]); if (best.empty() || im < best) best = im; } }
        Orb o; o.rep = best; o.size = orb.size(); int live = 0; for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (has(best, LC[l][i])) lv = false; live += lv; }
        o.slack = 216 - k - 2 * live; o.leaf = -1; orbs.push_back(o);
      }
      uint32_t c = v & -v, r = v + c; v = (((r ^ v) >> 2) / c) | r;
    }
    check((long long)orbs.size() == expOrb[k - 7], "enumeration: orbits of busy " + to_string(k) + "-sets = " + to_string(orbs.size()));
    if (k == 7) continue;
    #pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < orbs.size(); i++) orbs[i].leaf = (leafTest(orbs[i].rep, {}, k) == 0);
    long long ns = 0, nl = 0, nsets = 0, leafNoSlack = 0; for (auto& o : orbs) { if (o.slack >= k) ns++; if (o.leaf) { nl++; nsets += o.size; if (o.slack < k) leafNoSlack++; } }
    check(ns == expSlack[k - 8], "k=" + to_string(k) + ": orbits with slack >= k: " + to_string(ns));
    check(nl == expLeaf[k - 8] && leafNoSlack == 0, "k=" + to_string(k) + ": orbits with LEAF(B,0,k): " + to_string(nl) + " (containing " + to_string(nsets) + " sets)");
    if (k == 8) { vector<pair<VS, int>> got; for (auto& o : orbs) if (o.leaf) got.push_back({o.rep, o.size}); sort(got.begin(), got.end());
      vector<pair<VS, int>> want; for (int i = 0; i < 6; i++) want.push_back({S(reps8[i]), osz8[i]}); sort(want.begin(), want.end());
      check(got == want && nsets == 2340, "k=8: the six representatives O1..O6 and orbit sizes 288,576,288,576,576,36 (2,340 sets)"); }
    if (k == 9) check(nsets == 83904, "k=9: the 172 orbits contain 83,904 sets");
  }
}

// ---------- Algorithm 1 ----------
struct State { char type; int t; VS B, F; };
static int run(const char* tblFile, const char* leafFile, int RMAX) {
  unordered_map<string, pair<int, int>> Tbl;   // key -> (move, line number)
  unordered_map<string, int> Lv;                // key -> line number
  vector<string> tblLines, leafLines;
  int badLines = 0, nonCanon = 0, dup = 0;
  { ifstream in(tblFile); if (!in) die("cannot open table"); string line; int ln = 0;
    while (getline(in, line)) { ln++; tblLines.push_back(line); auto f = splitSp(line); int t, m; VS B, F;
      if (f.size() != 4 || !parseInt(f[0], t) || !parseSet(f[1], B) || !parseSet(f[2], F) || !parseInt(f[3], m) || m > 215 || t < 1 || (int)B.size() != t - 1 || (int)F.size() > t || !disjoint(B, F) || has(B, m) || has(F, m)) { badLines++; continue; }
      VS cb, cf; canon(B, F, cb, cf); if (cb != B || cf != F) nonCanon++;
      if (!Tbl.insert({key('Q', t, B, F), {m, ln}}).second) dup++; } }
  { ifstream in(leafFile); if (!in) die("cannot open leaves"); string line; int ln = 0;
    while (getline(in, line)) { ln++; leafLines.push_back(line); auto f = splitSp(line); int t; VS B, F;
      if (f.size() != 3 || !parseInt(f[0], t) || !parseSet(f[1], B) || !parseSet(f[2], F) || (int)B.size() != t || (int)F.size() > t || !disjoint(B, F)) { badLines++; continue; }
      VS cb, cf; canon(B, F, cb, cf); if (cb != B || cf != F) nonCanon++;
      if (!Lv.insert({key('P', t, B, F), ln}).second) dup++; } }
  printf("table lines: %zu, leaf lines: %zu, malformed: %d, non-canonical: %d, repeated keys: %d\n", tblLines.size(), leafLines.size(), badLines, nonCanon, dup);

  const int R = 16;
  long long Pall[R] = {}, Pleaf[R] = {}, Pexp[R] = {}, Qall[R] = {}, thr[R] = {}, dng[R] = {}, voidii[R] = {}, voidc = 0, voidd = 0, voida = 0, voidi = 0;
  long long chA = 0, chC = 0, chD = 0, chI = 0, chII = 0, qWithII = 0, lostTrue = 0;
  long long leafTheta[R][R] = {}; long long nonbusyMove[R] = {}; long long leafNonBusy[R] = {};
  LeafStat lst;
  vector<char> tblUsed(tblLines.size() + 1, 0), leafUsed(leafLines.size() + 1, 0);
  unordered_set<string> Seen; deque<State> W;
  auto push = [&](char type, int t, const VS& B, const VS& F, char kind) -> bool {
    // shape assertions
    if ((type == 'P' && (int)B.size() != t) || (type == 'Q' && (int)B.size() != t - 1) || (int)F.size() > t || !disjoint(B, F)) die("shape violation");
    if (!admissible(B, F)) return false;                    // void
    VS cb, cf; canon(B, F, cb, cf);
    if (Seen.insert(key(type, t, cb, cf)).second) W.push_back({type, t, cb, cf});
    (void)kind; return true;
  };
  { VS e; push('P', 0, e, e, 'r'); }
  string reject;
  while (!W.empty() && reject.empty()) {
    State s = W.front(); W.pop_front();
    const VS& B = s.B; const VS& F = s.F; int t = s.t;
    if (s.type == 'P') {
      Pall[t]++;
      auto it = Lv.find(key('P', t, B, F));
      if (it != Lv.end()) {
        leafUsed[it->second] = 1;
        int r = leafTest(B, F, t, &lst);
        if (r != 0) { reject = "LEAF fails (L" + to_string(r) + ") at leaf " + to_string(t) + " " + setStr(B) + " " + setStr(F); break; }
        if (leafTest(B, F, t, &lst, 2) != 0 || leafTest(B, F, t, &lst, 3) != 0) die("pruning variants of the leaf test disagree");
        Pleaf[t]++; leafTheta[t][t - (int)F.size()]++;
        int nbz = 0; for (uint8_t b : B) if (!BUSY[b]) nbz++; leafNonBusy[min(nbz, R - 1)]++;
        continue;
      }
      if (!(t < RMAX)) { reject = "unlisted P-state with t >= Rmax: " + to_string(t) + " " + setStr(B) + " " + setStr(F); break; }
      if (lostTest(B, F, t) != 0) { lostTrue++; reject = "LOST holds at " + to_string(t) + " " + setStr(B) + " " + setStr(F); break; }
      Pexp[t]++;
      bool inB[216] = {}, inF[216] = {}; for (uint8_t b : B) inB[b] = true; for (uint8_t f : F) inF[f] = true;
      bool live[160]; int cf[160];
      for (int l = 0; l < NL; l++) { live[l] = true; cf[l] = 0; for (int i = 0; i < 6; i++) { if (inB[LC[l][i]]) live[l] = false; if (inF[LC[l][i]]) cf[l]++; } }
      // (a)
      chA++; if (!push('Q', t + 1, B, F, 'a')) { voida++; reject = "void child (a)"; break; }
      // (c)
      set<string> kids;
      for (int l = 0; l < NL; l++) if (live[l]) for (int i = 0; i < 6; i++) {
        int e = LC[l][i]; if (inF[e]) continue;
        VS F2 = F; for (int j = 0; j < 6; j++) if (j != i && !inF[LC[l][j]]) F2.push_back(LC[l][j]);
        sort(F2.begin(), F2.end());
        if ((int)F2.size() > t + 1) continue;
        VS B2 = plusCell(B, e);
        if (!kids.insert(key('P', t + 1, B2, F2)).second) die("duplicate child generated (c)");
        chC++; if (push('P', t + 1, B2, F2, 'c')) thr[t + 1]++; else voidc++;
      }
      // (d)
      for (int c = 0; c < 216; c++) if (!inB[c] && !inF[c]) {
        const vector<int>& tl = THR[c];
        for (size_t x = 0; x < tl.size(); x++) for (size_t y = x + 1; y < tl.size(); y++) {
          int a = tl[x], b = tl[y]; if (!live[a] || !live[b]) continue;
          if ((int)F.size() + (4 - cf[a]) + (4 - cf[b]) > t + 1) continue;      // identity |F u Qa u Qb| (checked literally below)
          for (int i = 0; i < 6; i++) { int a1 = LC[a][i]; if (a1 == c || inF[a1]) continue;
            for (int j = 0; j < 6; j++) { int b1 = LC[b][j]; if (b1 == c || inF[b1]) continue;
              VS F2 = F;
              for (int u = 0; u < 6; u++) { int q = LC[a][u]; if (q != c && q != a1 && !inF[q]) F2.push_back(q); }
              for (int u = 0; u < 6; u++) { int q = LC[b][u]; if (q != c && q != b1 && !inF[q]) F2.push_back(q); }
              sort(F2.begin(), F2.end());
              if (adjacent_find(F2.begin(), F2.end()) != F2.end()) die("Qa and Qb not disjoint");
              if ((int)F2.size() != (int)F.size() + (4 - cf[a]) + (4 - cf[b])) die("stone-bound identity violated");
              if ((int)F2.size() > t + 1) continue;
              VS B2 = plusCell(B, c);
              if (!kids.insert(key('P', t + 1, B2, F2)).second) die("duplicate child generated (d)");
              chD++; if (push('P', t + 1, B2, F2, 'd')) dng[t + 1]++; else voidd++;
            } }
        }
      }
    } else {
      Qall[t]++;
      auto it = Tbl.find(key('Q', t, B, F));
      if (it == Tbl.end()) { reject = "no table entry for Q-state " + to_string(t) + " " + setStr(B) + " " + setStr(F); break; }
      int m = it->second.first; tblUsed[it->second.second] = 1;
      if (m < 0 || m > 215 || has(B, m) || has(F, m)) { reject = "illegal table move"; break; }
      if (!BUSY[m]) nonbusyMove[t]++;
      chI++; if (!push('P', t, plusCell(B, m), F, 'i')) { voidi++; reject = "void child (i)"; break; }
      if ((int)F.size() < t) { qWithII++; chII++; if (!push('Q', t, B, plusCell(F, m), 'j')) voidii[t]++; }
    }
  }
  if (!reject.empty()) { printf("REJECT: %s\n", reject.c_str()); return 1; }
  // ---------- report ----------
  long long eP[11] = {1, 2, 5, 14, 35, 181, 1718, 7227, 34801, 93124, 16027}, eL[11] = {0, 0, 0, 0, 0, 0, 0, 0, 3382, 87314, 16027};
  long long eQ[11] = {0, 2, 5, 14, 35, 101, 446, 3487, 14362, 62381, 12331}, eT[11] = {0, 0, 0, 0, 0, 824, 4075, 14460, 47020, 72352, 4767};
  long long eD[11] = {0, 0, 0, 0, 0, 0, 0, 0, 20416, 6060, 1799}, eV[11] = {0, 0, 0, 0, 0, 0, 1, 6, 54, 88, 2};
  printf("ACCEPT (Rmax = %d)\n", RMAX);
  printf("\nTable 3 (mine / paper):\n  t   P-states        leaves          expanded        Q-states        threat(c)       danger(d)       void(ii)\n");
  long long sP = 0, sL = 0, sE = 0, sQ = 0, sT = 0, sD = 0, sV = 0; bool allOk = true;
  for (int t = 0; t <= 10; t++) {
    bool ok = Pall[t] == eP[t] && Pleaf[t] == eL[t] && Pexp[t] == eP[t] - eL[t] && Qall[t] == eQ[t] && thr[t] == eT[t] && dng[t] == eD[t] && voidii[t] == eV[t];
    allOk = allOk && ok;
    printf("  %2d  %6lld/%-6lld  %6lld/%-6lld  %6lld/%-6lld  %6lld/%-6lld  %6lld/%-6lld  %6lld/%-6lld  %3lld/%-3lld %s\n", t, Pall[t], eP[t], Pleaf[t], eL[t], Pexp[t], eP[t] - eL[t], Qall[t], eQ[t], thr[t], eT[t], dng[t], eD[t], voidii[t], eV[t], ok ? "ok" : "MISMATCH");
    sP += Pall[t]; sL += Pleaf[t]; sE += Pexp[t]; sQ += Qall[t]; sT += thr[t]; sD += dng[t]; sV += voidii[t];
  }
  for (int t = 11; t < R; t++) if (Pall[t] || Qall[t]) allOk = false;
  check(allOk, "all per-round numbers of Table 3 / Appendix A.5");
  check(sP == 153135 && sL == 106723 && sE == 46412 && sQ == 93164 && sP + sQ == 246299, "totals: P " + to_string(sP) + ", leaves " + to_string(sL) + ", expanded " + to_string(sE) + ", Q " + to_string(sQ) + ", visited " + to_string(sP + sQ));
  check(sT == 143498 && sD == 28275 && sV == 151, "threat children " + to_string(sT) + ", danger children " + to_string(sD) + ", void (ii) " + to_string(sV));
  check(voidc == 0 && voidd == 0 && voida == 0 && voidi == 0, "no void child of type (a),(c),(d),(i): " + to_string(voida) + "/" + to_string(voidc) + "/" + to_string(voidd) + "/" + to_string(voidi));
  check(chA == 46412 && chC == 143498 && chD == 28275 && chI == 93164 && chII == 54482, "children a/c/d/i/ii generated: " + to_string(chA) + "/" + to_string(chC) + "/" + to_string(chD) + "/" + to_string(chI) + "/" + to_string(chII));
  check(qWithII == 54482, "Q-states with |F| < t: " + to_string(qWithII));
  long long eTh[3][9] = {{1862, 940, 340, 119, 69, 36, 12, 3, 1}, {53397, 26291, 6102, 1189, 260, 63, 11, 1, 0}, {9582, 5472, 825, 126, 14, 6, 2, 0, 0}};
  bool thOk = true; printf("\nTable 4 (leaves by t and theta), mine:\n");
  for (int t = 8; t <= 10; t++) { printf("  t=%2d:", t); for (int th = 0; th <= 8; th++) { printf(" %lld", leafTheta[t][th]); if (leafTheta[t][th] != eTh[t - 8][th]) thOk = false; } printf("\n"); }
  for (int t = 0; t < R; t++) for (int th = 0; th < R; th++) if ((t < 8 || t > 10 || th > 8) && leafTheta[t][th]) thOk = false;
  check(thOk, "Table 4 reproduced in every cell");
  check(nonbusyMove[7] == 16 && nonbusyMove[8] == 399 && nonbusyMove[9] == 11 && nonbusyMove[10] == 8 && nonbusyMove[1] + nonbusyMove[2] + nonbusyMove[3] + nonbusyMove[4] + nonbusyMove[5] + nonbusyMove[6] == 0,
        "non-busy table moves in rounds 7..10: " + to_string(nonbusyMove[7]) + "," + to_string(nonbusyMove[8]) + "," + to_string(nonbusyMove[9]) + "," + to_string(nonbusyMove[10]));
  check(leafNonBusy[0] == 15794 && leafNonBusy[1] == 84556 && leafNonBusy[2] == 6343 && leafNonBusy[3] == 30, "leaves by number of non-busy Breaker stones 0/1/2/3: " + to_string(leafNonBusy[0]) + "/" + to_string(leafNonBusy[1]) + "/" + to_string(leafNonBusy[2]) + "/" + to_string(leafNonBusy[3]));
  long long tu = 0, lu = 0; for (size_t i = 1; i < tblUsed.size(); i++) tu += tblUsed[i]; for (size_t i = 1; i < leafUsed.size(); i++) lu += leafUsed[i];
  check(tblLines.size() == 93164 && leafLines.size() == 106723 && badLines == 0 && nonCanon == 0 && dup == 0, "files: 93,164 + 106,723 well-formed canonical distinct lines");
  check(tu == (long long)tblLines.size() && lu == (long long)leafLines.size(), "every line of both files is reached (" + to_string(tu) + ", " + to_string(lu) + ")");
  check(tblLines.size() >= 9791 && tblLines[0] == "1 - - 0" && tblLines[2353] == "5 0,46,169,215 - 92" && tblLines[9790] == "7 0,35,46,92,123,169 5,30,43,180,185,210,215 50", "table lines 1, 2354, 9791 as printed in A.3");
  check(leafLines.size() >= 8185 && leafLines[0] == "9 0,1,5,30,43,93,122,172,210 7,13,14,19,21,25,28,31,35" && leafLines[8184] == "8 0,5,61,93,122,154,210,215 -" && leafLines.back() == "9 0,35,43,64,86,93,123,154,210 5,30,61,122,129,151,172,180,215", "leaf lines 1, 8185, last as printed in A.4");
  printf("\nleaf-test statistics: single-line gains %lld (paper, main verifier: 869,185), triple gains %lld (pruning by strong lines only; paper's 7,391 uses a second rule), validated true flows %lld\n", lst.single, lst.triple, lst.validated);
  printf("  variants: single-line gains restricted to lines with |l\\F|-2 >= theta: %lld; to lines with fewer than theta unassigned free cells: %lld\n", lst.singleH1, lst.singleH3);
  printf("  with pair rule on the two F-adjacent pairs: pair flows %lld, triple flows %lld; on all three pairs: pair flows %lld, triple flows %lld (paper: 77,268 and 7,391)\n", lst.pairA, lst.tripleA, lst.pairB, lst.tripleB);
  printf("  single-line gains at leaves with theta >= 2 only: %lld; of these on lines with |l\\F|-2 >= theta: %lld; total number of weak lines: %lld\n", lst.singleTh2, lst.singleNotTriv, lst.weakLines);
  check(lst.single == 869185 || lst.singleH1 == 869185 || lst.singleH3 == 869185 || lst.singleTh2 == 869185 || lst.singleNotTriv == 869185, "single-line flows = 869,185 (implementation statistic of the paper's main verifier)");
  check((lst.pairA == 77268 && lst.tripleA == 7391) || (lst.pairB == 77268 && lst.tripleB == 7391), "pair flows = 77,268 and triple flows = 7,391 (implementation statistic)");
  // Lines A and B of Section 6.6 (followed in canonical frames)
  { VS B, F; int t = 0; bool ok = true;
    while (ok) { if (Lv.count(key('P', t, B, F))) break; if (t >= RMAX) { ok = false; break; } auto it = Tbl.find(key('Q', t + 1, B, F)); if (it == Tbl.end()) { ok = false; break; } VS cb, cf; canon(plusCell(B, it->second.first), F, cb, cf); B = cb; F = cf; t++; }
    check(ok && t == 8 && setStr(B) == "0,5,61,93,122,154,210,215" && F.empty(), "Line A: ends after round 8 in leaf 8 0,5,61,93,122,154,210,215 -"); }
  { VS B, F; int t = 0; bool ok = true;
    while (ok) { if (Lv.count(key('P', t, B, F))) break; if (t >= RMAX) { ok = false; break; } auto it = Tbl.find(key('Q', t + 1, B, F)); if (it == Tbl.end()) { ok = false; break; }
      VS cb, cf; canon(B, plusCell(F, it->second.first), cb, cf); auto it2 = Tbl.find(key('Q', t + 1, cb, cf)); if (it2 == Tbl.end() || (int)cf.size() != t + 1) { ok = false; break; }
      VS cb2, cf2; canon(plusCell(cb, it2->second.first), cf, cb2, cf2); B = cb2; F = cf2; t++; }
    string got = to_string(t) + " " + setStr(B) + " " + setStr(F);
    check(ok && got == leafLines.back(), "Line B: ends after round 9 in the last line of the leaf file (got " + got + ")"); }
  return 0;
}

int main(int argc, char** argv) {
  buildGeometry();
  string mode = argc > 1 ? argv[1] : "";
  if (mode == "selftest") selftest();
  else if (mode == "leaftable") leaftable();
  else if (mode == "run" && argc >= 4) { int r = run(argv[2], argv[3], argc > 4 ? atoi(argv[4]) : 10); if (r) return r; }
  else { printf("usage: verify selftest | leaftable | run strategy.tbl leaves.txt [rmax]\n"); return 2; }
  printf("\n%s (%d mismatches)\n", nfail ? "SOME CHECKS FAILED" : "ALL CHECKS PASSED", nfail);
  return nfail ? 1 : 0;
}

// logs/review/lead/lead_leaf.cpp -- lead reviewer's own recomputation of LEAF(B,F,k) (SPEC section 3).
// Written from SPEC sections 1 and 3 and PROOF sections 3-4 only; shares no code with src/, verify/, sim/.
//
//   g++ -O2 -std=c++17 -o lead_leaf lead_leaf.cpp
//   ./lead_leaf [--prune1] [--every N] < lines "k B F"      -> one line per instance: "<k> <B> <F> <0|1> <reason>"
//
// LEAF(B,F,k) is evaluated as follows (theta = k-|F|):
//   1. F is B-admissible: literal definition (cap on every live line; every PAIR of live lines with exactly 4
//      cells of F each is tested for a common cell outside F).
//   2. pairing for (B,F): Kuhn's algorithm, every live line is matched twice; the result is re-validated.
//   3. for EVERY unordered triple {a,b,c} of live lines that is F-connected (at least two of its three pairs
//      share a cell outside F): starting from the pairing, augmenting paths from the three lines are searched
//      until theta additional cells have been collected.  If this fails for one triple, LEAF is false.
//      Soundness of "true" (PROOF 4.1/4.2 "<="): an F-connected S with |S| >= 3 contains an F-connected triple
//      T; in the final assignment the lines of S hold 2|S|+theta distinct cells of N(S)\F, so sur_F(S) >= theta.
//      Exactness of "false": when no augmenting path is left the reached lines R hold all of N(R)\F
//      (re-checked below from the definition: sur_F(R) < theta, R is F-connected, |R| >= 3).
//   --prune1 (optional): a triple containing a line l whose own gain is >= theta is skipped
//      (mu({l}) >= theta implies mu(T) >= theta; the single-line flow is an explicit flow too).
//   Without --prune1 there is no pruning rule at all.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>

static const int NC = 216;
static int NL = 0;
static int LINE[200][6];
static bool ONLINE[200][NC];
static std::vector<int> CL[NC];
static int MEETC[200][200];

static void geom() {
  // lines as arithmetic progressions: start cell p, direction d in {-1,0,1}^3 \ {0}, first non-zero component +1,
  // p - d outside the board, p + 5d inside.
  std::set<std::array<int, 6>> seen;
  for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++)
      for (int dz = -1; dz <= 1; dz++) {
        if (!dx && !dy && !dz) continue;
        int first = dx ? dx : (dy ? dy : dz);
        if (first < 0) continue;
        for (int x = 0; x < 6; x++)
          for (int y = 0; y < 6; y++)
            for (int z = 0; z < 6; z++) {
              int px = x - dx, py = y - dy, pz = z - dz;
              bool prevIn = px >= 0 && px < 6 && py >= 0 && py < 6 && pz >= 0 && pz < 6;
              if (prevIn) continue;
              int ex = x + 5 * dx, ey = y + 5 * dy, ez = z + 5 * dz;
              if (ex < 0 || ex > 5 || ey < 0 || ey > 5 || ez < 0 || ez > 5) continue;
              std::array<int, 6> a;
              for (int i = 0; i < 6; i++) a[i] = 36 * (x + i * dx) + 6 * (y + i * dy) + (z + i * dz);
              std::sort(a.begin(), a.end());
              seen.insert(a);
            }
      }
  NL = 0;
  for (auto& a : seen) {
    for (int i = 0; i < 6; i++) LINE[NL][i] = a[i];
    NL++;
  }
  if (NL != 148) { fprintf(stderr, "geometry: %d lines\n", NL); exit(3); }
  memset(ONLINE, 0, sizeof ONLINE);
  for (int l = 0; l < NL; l++)
    for (int i = 0; i < 6; i++) { ONLINE[l][LINE[l][i]] = true; CL[LINE[l][i]].push_back(l); }
  for (int a = 0; a < NL; a++)
    for (int b = 0; b < NL; b++) {
      MEETC[a][b] = -1;
      if (a == b) continue;
      int n = 0;
      for (int i = 0; i < 6; i++) if (ONLINE[b][LINE[a][i]]) { n++; MEETC[a][b] = LINE[a][i]; }
      if (n > 1) { fprintf(stderr, "two lines share %d cells\n", n); exit(3); }
    }
}

static bool parseSet(const std::string& s, std::vector<int>& out) {
  out.clear();
  if (s == "-") return true;
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, ',')) {
    if (tok.empty()) return false;
    for (char ch : tok) if (ch < '0' || ch > '9') return false;
    int v = atoi(tok.c_str());
    if (v < 0 || v >= NC) return false;
    out.push_back(v);
  }
  return true;
}

struct Inst {
  bool inB[NC], inF[NC];
  int nF;
  std::vector<int> live;
  bool isLive[200];
  int owner[NC];
  // BFS scratch
  int par_cell[200], par_line[NC];
  bool vl[200], vc[NC];
  std::vector<int> reached;
  long long nflows = 0, nbfs = 0;

  bool admissible() {
    for (int c = 0; c < NC; c++) if (inB[c] && inF[c]) return false;
    std::vector<int> four;
    for (int l : live) {
      int n = 0;
      for (int i = 0; i < 6; i++) n += inF[LINE[l][i]];
      if (n > 4) return false;
      if (n == 4) four.push_back(l);
    }
    for (size_t i = 0; i < four.size(); i++)
      for (size_t j = i + 1; j < four.size(); j++) {
        int c = MEETC[four[i]][four[j]];
        if (c >= 0 && !inF[c]) return false;
      }
    return true;
  }
  // one augmenting path from the given source lines; true iff a free cell was reached (and the path applied)
  bool augment(const int* src, int ns, bool apply) {
    nbfs++;
    for (int l : live) vl[l] = false;
    memset(vc, 0, sizeof vc);
    std::vector<int> q;
    for (int i = 0; i < ns; i++) { vl[src[i]] = true; par_cell[src[i]] = -1; q.push_back(src[i]); }
    for (size_t h = 0; h < q.size(); h++) {
      int l = q[h];
      for (int i = 0; i < 6; i++) {
        int c = LINE[l][i];
        if (inF[c] || vc[c] || owner[c] == l) continue;
        vc[c] = true;
        par_line[c] = l;
        int o = owner[c];
        if (o < 0) {
          if (apply) {
            int cur = c;
            for (;;) {
              int pl = par_line[cur];
              owner[cur] = pl;
              if (par_cell[pl] < 0) break;
              cur = par_cell[pl];
            }
          }
          return true;
        }
        if (!vl[o]) { vl[o] = true; par_cell[o] = c; q.push_back(o); }
      }
    }
    reached = q;
    return false;
  }
  bool validPairing() {
    int cnt[200];
    memset(cnt, 0, sizeof cnt);
    for (int c = 0; c < NC; c++) {
      int o = owner[c];
      if (o < 0) continue;
      if (!isLive[o] || inF[c] || inB[c] || !ONLINE[o][c]) return false;
      cnt[o]++;
    }
    for (int l : live) if (cnt[l] != 2) return false;
    return true;
  }
  int sur(const std::vector<int>& S) {
    bool in[NC];
    memset(in, 0, sizeof in);
    int n = 0;
    for (int l : S) for (int i = 0; i < 6; i++) { int c = LINE[l][i]; if (!inF[c] && !in[c]) { in[c] = true; n++; } }
    return n - 2 * (int)S.size();
  }
  bool fadj(int a, int b) { int c = MEETC[a][b]; return c >= 0 && !inF[c]; }
  bool fconnected(const std::vector<int>& S) {
    if (S.empty()) return false;
    std::vector<char> v(S.size(), 0);
    std::vector<int> st{0};
    v[0] = 1;
    size_t n = 1;
    while (!st.empty()) {
      int i = st.back(); st.pop_back();
      for (size_t j = 0; j < S.size(); j++) if (!v[j] && fadj(S[i], S[j])) { v[j] = 1; n++; st.push_back((int)j); }
    }
    return n == S.size();
  }
  // returns reason: 0 LEAF holds, 1 F not admissible, 2 no pairing, 3 C_Phi fails, 4 k < |F|
  int leaf(int k, bool prune1) {
    if (k < nF) return 4;
    if (!admissible()) return 1;
    for (int c = 0; c < NC; c++) owner[c] = -1;
    for (int l : live)
      for (int r = 0; r < 2; r++)
        if (!augment(&l, 1, true)) {
          if (!(sur(reached) < 0)) { fprintf(stderr, "internal: no pairing but no Hall violator\n"); exit(4); }
          return 2;
        }
    if (!validPairing()) { fprintf(stderr, "internal: invalid pairing\n"); exit(4); }
    int theta = k - nF;
    if (theta <= 0) return 0;
    int base[NC];
    memcpy(base, owner, sizeof base);
    std::vector<char> strong(200, 0);
    if (prune1)
      for (int l : live) {
        memcpy(owner, base, sizeof base);
        int g = 0;
        while (g < theta && augment(&l, 1, true)) g++;
        if (g >= theta) {
          // explicit flow: l holds 2+theta cells, every other live line exactly 2
          int cnt[200];
          memset(cnt, 0, sizeof cnt);
          for (int c = 0; c < NC; c++) if (owner[c] >= 0) cnt[owner[c]]++;
          for (int m : live) if (cnt[m] != (m == l ? 2 + theta : 2)) { fprintf(stderr, "internal: bad single flow\n"); exit(4); }
          strong[l] = 1;
        }
      }
    int n = (int)live.size();
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) {
        int a = live[i], b = live[j];
        bool ab = fadj(a, b);
        for (int h = j + 1; h < n; h++) {
          int c = live[h];
          int na = (int)ab + (int)fadj(a, c) + (int)fadj(b, c);
          if (na < 2) continue;
          if (prune1 && (strong[a] || strong[b] || strong[c])) continue;
          nflows++;
          memcpy(owner, base, sizeof base);
          int src[3] = {a, b, c};
          int g = 0;
          while (g < theta && augment(src, 3, true)) g++;
          if (g < theta) {
            std::vector<int> R = reached;
            int s = sur(R);
            if (!(s == g && s < theta && R.size() >= 3 && fconnected(R))) { fprintf(stderr, "internal: bad cut certificate\n"); exit(4); }
            return 3;
          }
          // validate the flow literally
          int cnt[200];
          memset(cnt, 0, sizeof cnt);
          for (int x = 0; x < NC; x++) {
            int o = owner[x];
            if (o < 0) continue;
            if (!isLive[o] || inF[x] || !ONLINE[o][x]) { fprintf(stderr, "internal: bad flow\n"); exit(4); }
            cnt[o]++;
          }
          int tot = 0;
          for (int m : live) {
            if (m == a || m == b || m == c) tot += cnt[m];
            else if (cnt[m] != 2) { fprintf(stderr, "internal: bad flow count\n"); exit(4); }
          }
          if (tot != 6 + theta) { fprintf(stderr, "internal: bad flow total\n"); exit(4); }
        }
      }
    return 0;
  }
};

int main(int argc, char** argv) {
  bool prune1 = false;
  int every = 1;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--prune1") prune1 = true;
    else if (a == "--every" && i + 1 < argc) every = atoi(argv[++i]);
    else { fprintf(stderr, "unknown argument %s\n", argv[i]); return 2; }
  }
  geom();
  std::string line;
  long long n = 0, used = 0, cnt[5] = {0, 0, 0, 0, 0}, flows = 0, bfs = 0;
  static Inst I;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    n++;
    if ((n - 1) % every) continue;
    std::stringstream ss(line);
    std::string ks, bs, fs;
    ss >> ks >> bs >> fs;
    std::vector<int> B, F;
    if (!parseSet(bs, B) || !parseSet(fs, F)) { fprintf(stderr, "bad line %lld\n", n); return 2; }
    int k = atoi(ks.c_str());
    memset(I.inB, 0, sizeof I.inB);
    memset(I.inF, 0, sizeof I.inF);
    for (int c : B) I.inB[c] = true;
    for (int c : F) I.inF[c] = true;
    I.nF = (int)F.size();
    I.live.clear();
    for (int l = 0; l < NL; l++) {
      bool lv = true;
      for (int i = 0; i < 6; i++) if (I.inB[LINE[l][i]]) lv = false;
      I.isLive[l] = lv;
      if (lv) I.live.push_back(l);
    }
    int r = I.leaf(k, prune1);
    cnt[r]++;
    used++;
    printf("%d %s %s %d %d\n", k, bs.c_str(), fs.c_str(), r == 0 ? 1 : 0, r);
  }
  flows = I.nflows; bfs = I.nbfs;
  fprintf(stderr, "lead_leaf: %lld lines read, %lld evaluated (every %d), prune1=%d: LEAF true %lld, reasons 1/2/3/4 = %lld/%lld/%lld/%lld, triple flows %lld, BFS %lld\n",
          n, used, every, (int)prune1, cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], flows, bfs);
  return 0;
}

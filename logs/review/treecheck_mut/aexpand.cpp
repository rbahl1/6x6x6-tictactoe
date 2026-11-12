// aexpand.cpp -- auditor's independent STRUCTURAL re-expansion of the proof tree (SPEC 4), written from
// SPEC.md only (own geometry, own group as a closure of generators, brute-force canonical form over all
// 1152 maps, literal children).  It does NOT recompute LEAF (a listed leaf is taken as a leaf); it checks
// the tree structure and produces the counters that treecheck reports, so that treecheck's child
// generation / VOID filter / visited set can be compared on the real tree:
//   p_states, q_states, leaves, threat children, danger children, VOID children by type, all by t.
// LOST (literal E2/E3) is evaluated on every LOSTSTEP-th expanded P-state.
//
//   aexpand TABLE LEAVES RMAX [LOSTSTEP]
// g++ -O2 -std=c++17 -o aexpand aexpand.cpp
#include <bits/stdc++.h>
using namespace std;
typedef bitset<216> BS;
typedef array<uint64_t, 8> Key;     // B words (cell c -> bit 63-(c%64) of word c/64), then F words

static vector<array<int, 6>> LN;
static vector<BS> LB;
static vector<vector<int>> THRU(216);
static int MEETC[148][148];
static vector<array<uint8_t, 216>> GRP;

static void geom() {
  set<array<int, 6>> s;
  for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++)
      for (int dz = -1; dz <= 1; dz++) {
        if (!dx && !dy && !dz) continue;
        for (int c0 = 0; c0 < 216; c0++) {
          int x = c0 / 36, y = (c0 / 6) % 6, z = c0 % 6;
          array<int, 6> a;
          bool ok = true;
          for (int i = 0; i < 6 && ok; i++) {
            int X = x + i * dx, Y = y + i * dy, Z = z + i * dz;
            if (X < 0 || X > 5 || Y < 0 || Y > 5 || Z < 0 || Z > 5) ok = false;
            else a[i] = 36 * X + 6 * Y + Z;
          }
          if (!ok) continue;
          sort(a.begin(), a.end());
          s.insert(a);
        }
      }
  LN.assign(s.begin(), s.end());
  if (LN.size() != 148) exit(3);
  for (size_t l = 0; l < LN.size(); l++) {
    BS b;
    for (int c : LN[l]) { b.set(c); THRU[c].push_back((int)l); }
    LB.push_back(b);
  }
  for (int a = 0; a < 148; a++)
    for (int b = 0; b < 148; b++) {
      MEETC[a][b] = -1;
      if (a == b) continue;
      BS I = LB[a] & LB[b];
      if (I.count() > 1) exit(3);
      if (I.count() == 1)
        for (int c = 0; c < 216; c++)
          if (I[c]) MEETC[a][b] = c;
    }
  // group = closure of: swap x,y; rotate x->y->z; reverse x; three generators of the relabellings s with s(5-i)=5-s(i)
  auto mk = [](function<array<int, 3>(int, int, int)> f) {
    array<uint8_t, 216> g;
    for (int c = 0; c < 216; c++) {
      array<int, 3> r = f(c / 36, (c / 6) % 6, c % 6);
      g[c] = (uint8_t)(36 * r[0] + 6 * r[1] + r[2]);
    }
    return g;
  };
  auto relab = [&](array<int, 6> sgm) { return mk([sgm](int x, int y, int z) { return array<int, 3>{sgm[x], sgm[y], sgm[z]}; }); };
  vector<array<uint8_t, 216>> gens = {
      mk([](int x, int y, int z) { return array<int, 3>{y, x, z}; }),
      mk([](int x, int y, int z) { return array<int, 3>{y, z, x}; }),
      mk([](int x, int y, int z) { return array<int, 3>{5 - x, y, z}; }),
      relab({1, 0, 2, 3, 5, 4}), relab({0, 2, 1, 4, 3, 5}), relab({5, 1, 2, 3, 4, 0})};
  array<uint8_t, 216> id;
  for (int c = 0; c < 216; c++) id[c] = (uint8_t)c;
  set<array<uint8_t, 216>> seen = {id};
  vector<array<uint8_t, 216>> fr = {id};
  while (!fr.empty()) {
    vector<array<uint8_t, 216>> nx;
    for (auto& g : fr)
      for (auto& h : gens) {
        array<uint8_t, 216> k;
        for (int c = 0; c < 216; c++) k[c] = h[g[c]];
        if (seen.insert(k).second) nx.push_back(k);
      }
    fr.swap(nx);
  }
  GRP.assign(seen.begin(), seen.end());
  if (GRP.size() != 1152) { fprintf(stderr, "group size %zu\n", GRP.size()); exit(3); }
  set<array<int, 6>> ls(LN.begin(), LN.end());
  for (auto& g : GRP)
    for (auto& l : LN) {
      array<int, 6> im;
      for (int i = 0; i < 6; i++) im[i] = g[l[i]];
      sort(im.begin(), im.end());
      if (!ls.count(im)) exit(3);
    }
}

static inline void addCell(uint64_t* w, int c) { w[c >> 6] |= 1ULL << (63 - (c & 63)); }

// canonical key: SPEC 6 -- smallest image pair, B first then F, in the order of SPEC 2.4 (the set that contains
// the smallest differing cell is the smaller one) = lexicographically LARGEST word vector in this bit order
static Key canonKey(const vector<int>& B, const vector<int>& F, int* gOut = nullptr) {
  Key best;
  bool have = false;
  int bg = -1;
  for (int gi = 0; gi < 1152; gi++) {
    const auto& g = GRP[gi];
    uint64_t wb[4] = {0, 0, 0, 0};
    for (int c : B) addCell(wb, g[c]);
    int cmp = 0;
    if (have) {
      for (int i = 0; i < 4 && !cmp; i++)
        if (wb[i] != best[i]) cmp = wb[i] > best[i] ? 1 : -1;
      if (cmp < 0) continue;
    }
    uint64_t wf[4] = {0, 0, 0, 0};
    for (int c : F) addCell(wf, g[c]);
    if (have && cmp == 0) {
      for (int i = 0; i < 4 && !cmp; i++)
        if (wf[i] != best[4 + i]) cmp = wf[i] > best[4 + i] ? 1 : -1;
      if (cmp <= 0) continue;
    }
    for (int i = 0; i < 4; i++) { best[i] = wb[i]; best[4 + i] = wf[i]; }
    have = true;
    bg = gi;
  }
  if (gOut) *gOut = bg;
  return best;
}
static void keyToSets(const Key& k, vector<int>& B, vector<int>& F) {
  B.clear();
  F.clear();
  for (int c = 0; c < 216; c++) {
    if (k[c >> 6] >> (63 - (c & 63)) & 1) B.push_back(c);
    if (k[4 + (c >> 6)] >> (63 - (c & 63)) & 1) F.push_back(c);
  }
}
static BS toBS(const vector<int>& v) {
  BS b;
  for (int c : v) b.set(c);
  return b;
}
static vector<int> toVec(const BS& b) {
  vector<int> v;
  for (int c = 0; c < 216; c++)
    if (b[c]) v.push_back(c);
  return v;
}
static bool parseSet(const string& s, vector<int>& out) {
  out.clear();
  if (s == "-") return true;
  size_t p = 0;
  while (p < s.size()) {
    size_t q = s.find(',', p);
    if (q == string::npos) q = s.size();
    if (q == p) return false;
    int v = 0;
    for (size_t i = p; i < q; i++) {
      if (!isdigit((unsigned char)s[i])) return false;
      v = v * 10 + (s[i] - '0');
    }
    if (v > 215) return false;
    out.push_back(v);
    p = q + 1;
  }
  return true;
}

static bool admissibleLit(const BS& B, const BS& X) {
  if ((B & X).any()) return false;
  vector<int> four;
  for (int l = 0; l < 148; l++) {
    if ((LB[l] & B).any()) continue;
    size_t c = (LB[l] & X).count();
    if (c > 4) return false;
    if (c == 4) four.push_back(l);
  }
  for (size_t i = 0; i < four.size(); i++)
    for (size_t j = i + 1; j < four.size(); j++) {
      BS I = LB[four[i]] & LB[four[j]];
      if (I.any() && (I & ~X).any()) return false;
    }
  return true;
}

// ---- literal LOST (same transcription as in abrute.cpp)
static vector<BS> stoneSets(int line, int without, int must, const BS& F) {
  vector<BS> r;
  for (int m = 0; m < 64; m++) {
    if (__builtin_popcount(m) != 4) continue;
    BS s;
    for (int i = 0; i < 6; i++)
      if ((m >> i) & 1) s.set(LN[line][i]);
    if (without >= 0 && s[without]) continue;
    if (must >= 0 && !s[must]) continue;
    if (((F & LB[line]) & ~s).any()) continue;
    r.push_back(s);
  }
  return r;
}
static bool fitsE2(const vector<int>& live, const BS& F, int t) {
  const size_t budget = (size_t)t + 1;
  for (int l : live)
    for (int a : live) {
      if (a == l) continue;
      int x = MEETC[l][a];
      if (x < 0) continue;
      for (int b : live) {
        if (b == l || b == a) continue;
        int c = MEETC[a][b];
        if (c < 0 || c == x || LB[b][x] || F[c]) continue;
        for (int e : LN[l]) {
          if (e == x || e == c || LB[b][e] || F[e]) continue;
          BS Sl = LB[l];
          Sl.reset(e);
          BS u0 = F | Sl;
          if (u0.count() > budget) continue;
          for (const BS& Sa : stoneSets(a, c, x, F)) {
            BS u1 = u0 | Sa;
            if (u1.count() > budget) continue;
            for (const BS& Sb : stoneSets(b, c, -1, F))
              if ((u1 | Sb).count() <= budget) return true;
          }
        }
      }
    }
  return false;
}
static bool fitsE3(const vector<int>& live, const BS& F, int t) {
  const size_t budget = (size_t)t + 1;
  for (int a : live)
    for (int a2 : live) {
      if (a2 == a) continue;
      int x = MEETC[a][a2];
      if (x < 0) continue;
      for (int b : live) {
        if (b == a || b == a2) continue;
        int c = MEETC[a][b];
        if (c < 0 || c == x || F[c] || LB[b][x]) continue;
        vector<BS> SA = stoneSets(a, c, x, F), SB = stoneSets(b, c, -1, F);
        bool any = false;
        for (const BS& Sa : SA)
          for (const BS& Sb : SB)
            if ((F | Sa | Sb).count() <= budget) any = true;
        if (!any) continue;
        for (int b2 : live) {
          if (b2 == a || b2 == a2 || b2 == b) continue;
          int c2 = MEETC[a2][b2];
          if (c2 < 0 || c2 == x || c2 == c || F[c2] || LB[b2][x]) continue;
          vector<BS> SA2 = stoneSets(a2, c2, x, F), SB2 = stoneSets(b2, c2, -1, F);
          for (const BS& Sa : SA) {
            if (Sa[c2]) continue;
            for (const BS& Sa2 : SA2) {
              if (Sa2[c]) continue;
              BS u2 = F | Sa | Sa2;
              if (u2.count() > budget) continue;
              for (const BS& Sb : SB) {
                if (Sb[c2]) continue;
                BS u3 = u2 | Sb;
                if (u3.count() > budget) continue;
                for (const BS& Sb2 : SB2) {
                  if (Sb2[c]) continue;
                  if ((u3 | Sb2).count() <= budget) return true;
                }
              }
            }
          }
        }
      }
    }
  return false;
}

struct St {
  Key k;
  int t, kind;   // kind 0 = P, 1 = Q
  bool operator<(const St& o) const { return tie(kind, t, k) < tie(o.kind, o.t, o.k); }
};

int main(int argc, char** argv) {
  if (argc < 4) { fprintf(stderr, "usage: aexpand TABLE LEAVES RMAX [LOSTSTEP]\n"); return 2; }
  geom();
  int rmax = atoi(argv[3]);
  int lostStep = argc > 4 ? atoi(argv[4]) : 0;
  map<pair<int, Key>, int> table;
  set<pair<int, Key>> leaves;
  long tLines = 0, lLines = 0, tNon = 0, lNon = 0, bad = 0, tDup = 0, lDup = 0;
  {
    ifstream f(argv[1]);
    string line;
    while (getline(f, line)) {
      if (line.empty()) continue;
      istringstream is(line);
      int t, m;
      string b, fs;
      vector<int> B, F;
      if (!(is >> t >> b >> fs >> m) || !parseSet(b, B) || !parseSet(fs, F)) { bad++; continue; }
      tLines++;
      int g;
      Key k = canonKey(B, F, &g);
      vector<int> cb, cf;
      keyToSets(k, cb, cf);
      if (cb != B || cf != F) tNon++;
      if ((int)B.size() != t - 1 || (int)F.size() > t) bad++;
      if (!table.emplace(make_pair(t, k), (int)GRP[g][m]).second) tDup++;
    }
  }
  {
    ifstream f(argv[2]);
    string line;
    while (getline(f, line)) {
      if (line.empty()) continue;
      istringstream is(line);
      int t;
      string b, fs;
      vector<int> B, F;
      if (!(is >> t >> b >> fs) || !parseSet(b, B) || !parseSet(fs, F)) { bad++; continue; }
      lLines++;
      Key k = canonKey(B, F);
      vector<int> cb, cf;
      keyToSets(k, cb, cf);
      if (cb != B || cf != F) lNon++;
      if ((int)B.size() != t) bad++;
      if (!leaves.insert(make_pair(t, k)).second) lDup++;
    }
  }
  fprintf(stderr, "aexpand: %ld table lines (%ld not canonical under my brute-force form, %ld duplicate keys), %ld leaf lines (%ld not canonical, %ld duplicates), %ld bad\n",
          tLines, tNon, tDup, lLines, lNon, lDup, bad);
  const int T = 64;
  vector<long> P(T, 0), Q(T, 0), LEAVES(T, 0), THR(T, 0), DNG(T, 0), VC(T, 0), VD(T, 0), VII(T, 0);
  long errors = 0, lostChecked = 0, lostTrue = 0, expanded = 0, usedTable = 0;
  set<St> seen;
  vector<St> stack;
  auto push = [&](int kind, int t, const BS& B, const BS& F) {
    St s;
    s.k = canonKey(toVec(B), toVec(F));
    s.t = t;
    s.kind = kind;
    if (seen.insert(s).second) stack.push_back(s);
  };
  push(0, 0, BS(), BS());
  set<pair<int, Key>> tableUsed, leavesUsed;
  while (!stack.empty()) {
    St s = stack.back();
    stack.pop_back();
    vector<int> Bv, Fv;
    keyToSets(s.k, Bv, Fv);
    BS B = toBS(Bv), F = toBS(Fv);
    int t = s.t;
    if (s.kind == 0) {
      P[t]++;
      if ((int)Bv.size() != t || (int)Fv.size() > t || !admissibleLit(B, F)) { errors++; continue; }
      if (leaves.count(make_pair(t, s.k))) {
        LEAVES[t]++;
        leavesUsed.insert(make_pair(t, s.k));
        continue;
      }
      if (t >= rmax) { errors++; continue; }
      expanded++;
      vector<int> live;
      for (int l = 0; l < 148; l++)
        if (!(LB[l] & B).any()) live.push_back(l);
      if (lostStep > 0 && expanded % lostStep == 0) {
        lostChecked++;
        if (fitsE2(live, F, t) || fitsE3(live, F, t)) { lostTrue++; errors++; }
      }
      // (a)
      push(1, t + 1, B, F);
      // (c) every live line l, every e in l \ F with |F u (l \ {e})| <= t+1
      for (int l : live)
        for (int e : LN[l]) {
          if (F[e]) continue;
          BS Fc = F | LB[l];
          Fc.reset(e);
          if ((int)Fc.count() > t + 1) continue;
          BS Bc = B;
          Bc.set(e);
          if (!admissibleLit(Bc, Fc)) { VC[t + 1]++; continue; }
          THR[t + 1]++;
          push(0, t + 1, Bc, Fc);
        }
      // (d) every unordered pair of live lines meeting in exactly one cell c not in F; all Qa, Qb
      for (size_t i = 0; i < live.size(); i++)
        for (size_t j = i + 1; j < live.size(); j++) {
          int a = live[i], b = live[j];
          int c = MEETC[a][b];
          if (c < 0 || F[c]) continue;
          vector<BS> QA = stoneSets(a, c, -1, F);
          for (const BS& Qa : QA) {
            BS u = F | Qa;
            if ((int)u.count() > t + 1) continue;     // exact: the union only grows
            for (const BS& Qb : stoneSets(b, c, -1, F)) {
              BS Fc = u | Qb;
              if ((int)Fc.count() > t + 1) continue;
              BS Bc = B;
              Bc.set(c);
              if (!admissibleLit(Bc, Fc)) { VD[t + 1]++; continue; }
              DNG[t + 1]++;
              push(0, t + 1, Bc, Fc);
            }
          }
        }
    } else {
      Q[t]++;
      if (t < 1 || (int)Bv.size() != t - 1 || (int)Fv.size() > t || !admissibleLit(B, F)) { errors++; continue; }
      auto it = table.find(make_pair(t, s.k));
      if (it == table.end()) { errors++; continue; }
      tableUsed.insert(make_pair(t, s.k));
      usedTable++;
      int m = it->second;
      if (B[m] || F[m]) { errors++; continue; }
      BS B1 = B;
      B1.set(m);
      if (!admissibleLit(B1, F)) { errors++; continue; }
      push(0, t, B1, F);
      if ((int)Fv.size() < t) {
        BS F1 = F;
        F1.set(m);
        if (!admissibleLit(B, F1)) VII[t]++;
        else push(1, t, B, F1);
      }
    }
  }
  auto sum = [](const vector<long>& v) { long s = 0; for (long x : v) s += x; return s; };
  auto arr = [&](const char* name, const vector<long>& v, int upto) {
    printf("\"%s\":[", name);
    for (int i = 0; i <= upto; i++) printf("%s%ld", i ? "," : "", v[i]);
    printf("]");
  };
  int mt = 0;
  for (int i = 0; i < T; i++)
    if (P[i] || Q[i]) mt = i;
  printf("{\"errors\":%ld,\"p_states\":%ld,\"q_states\":%ld,\"leaves\":%ld,\"expanded_p\":%ld,\"threat_children\":%ld,\"danger_children\":%ld,"
         "\"void_threat\":%ld,\"void_danger\":%ld,\"void_reveal\":%ld,\"states_visited\":%zu,\"max_t\":%d,\"table_lines\":%ld,\"table_noncanonical\":%ld,"
         "\"table_entries_used\":%zu,\"leaf_lines\":%ld,\"leaf_noncanonical\":%ld,\"leaf_entries_used\":%zu,\"lost_literal_checked\":%ld,\"lost_literal_true\":%ld,\"by_t\":{",
         errors, sum(P), sum(Q), sum(LEAVES), expanded, sum(THR), sum(DNG), sum(VC), sum(VD), sum(VII), seen.size(), mt, tLines, tNon, tableUsed.size(),
         lLines, lNon, leavesUsed.size(), lostChecked, lostTrue);
  arr("p_states", P, mt); printf(",");
  arr("q_states", Q, mt); printf(",");
  arr("leaves", LEAVES, mt); printf(",");
  arr("threat_children", THR, mt); printf(",");
  arr("danger_children", DNG, mt); printf(",");
  vector<long> vs(T, 0);
  for (int i = 0; i < T; i++) vs[i] = VC[i] + VD[i] + VII[i];
  arr("void_skipped", vs, mt);
  printf("}}\n");
  return errors ? 1 : 0;
}

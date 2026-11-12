// ref_audit.cpp -- referee's independent implementation, written from the paper only
// (Sections 2, 4, 5, Algorithms 1 and 2, Appendix A).  Shares no code with the project.
//
// modes:
//   ref_audit expand TABLE LEAVES RMAX     Algorithm 1 + Algorithm 2 on every leaf; prints counters
//   ref_audit leaf                         reads "k B F" lines from stdin, prints LEAF and failing condition
//   ref_audit canon                        reads "B F" lines from stdin, prints canonical form
//   ref_audit enum MAXPOS                  reads "k B F" lines, decides C(B,F,k) by enumeration (Remark 4.5 (iv))
//   ref_audit geom                         prints basic geometry facts
#include <bits/stdc++.h>
using namespace std;
typedef vector<int> VI;

static vector<array<int,6>> LN;          // lines
static vector<VI> linesOf(216);          // lines through a cell
static vector<array<unsigned char,216>> G; // group as cell permutations
static vector<vector<int>> meetc;        // meetc[a][b] = common cell or -1

static void build_geom() {
    set<array<int,6>> S;
    // each component: 0..5 constant, 6 = id, 7 = rho
    for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) for (int f3 = 0; f3 < 8; f3++) {
        if (f1 < 6 && f2 < 6 && f3 < 6) continue;
        array<int,6> l;
        for (int t = 0; t < 6; t++) {
            int c[3]; int f[3] = {f1, f2, f3};
            for (int i = 0; i < 3; i++) c[i] = f[i] < 6 ? f[i] : (f[i] == 6 ? t : 5 - t);
            l[t] = 36 * c[0] + 6 * c[1] + c[2];
        }
        sort(l.begin(), l.end());
        S.insert(l);
    }
    LN.assign(S.begin(), S.end());
    for (size_t i = 0; i < LN.size(); i++) for (int c : LN[i]) linesOf[c].push_back((int)i);
    meetc.assign(LN.size(), vector<int>(LN.size(), -1));
    for (size_t a = 0; a < LN.size(); a++) for (size_t b = 0; b < LN.size(); b++) if (a != b) {
        int cnt = 0, cc = -1;
        for (int x : LN[a]) for (int y : LN[b]) if (x == y) { cnt++; cc = x; }
        if (cnt > 1) { fprintf(stderr, "two lines share two cells\n"); exit(2); }
        meetc[a][b] = cc;
    }
    // group
    set<array<unsigned char,216>> GS;
    int p[6] = {0,1,2,3,4,5};
    vector<array<int,6>> H;
    do {
        bool ok = true;
        for (int i = 0; i < 6; i++) if (p[5 - i] != 5 - p[i]) ok = false;
        if (ok) { array<int,6> s; for (int i = 0; i < 6; i++) s[i] = p[i]; H.push_back(s); }
    } while (next_permutation(p, p + 6));
    if (H.size() != 48) { fprintf(stderr, "|H| != 48\n"); exit(2); }
    int pi[3] = {0,1,2};
    do {
        for (int r = 0; r < 8; r++) for (auto &s : H) {
            array<unsigned char,216> g;
            for (int c = 0; c < 216; c++) {
                int co[3] = {c / 36, (c / 6) % 6, c % 6}, d[3];
                for (int i = 0; i < 3; i++) d[pi[i]] = co[i];
                for (int i = 0; i < 3; i++) if (r >> i & 1) d[i] = 5 - d[i];
                for (int i = 0; i < 3; i++) d[i] = s[d[i]];
                g[c] = (unsigned char)(36 * d[0] + 6 * d[1] + d[2]);
            }
            GS.insert(g);
        }
    } while (next_permutation(pi, pi + 3));
    G.assign(GS.begin(), GS.end());
}

static bool busy(int c) {
    int x = c / 36, y = (c / 6) % 6, z = c % 6;
    auto lam = [](int i) { return min(i, 5 - i); };
    return lam(x) == lam(y) && lam(y) == lam(z);
}

struct Pos {            // membership arrays
    bool inB[216], inF[216];
    VI B, F;
    Pos(const VI &b, const VI &f) : B(b), F(f) {
        memset(inB, 0, sizeof inB); memset(inF, 0, sizeof inF);
        for (int c : B) inB[c] = true;
        for (int c : F) inF[c] = true;
    }
    bool live(int l) const { for (int c : LN[l]) if (inB[c]) return false; return true; }
};

// Definition 4.1
static bool admissible(const Pos &P) {
    for (int c : P.F) if (P.inB[c]) return false;
    VI four;
    for (size_t l = 0; l < LN.size(); l++) {
        if (!P.live(l)) continue;
        int n = 0; for (int c : LN[l]) if (P.inF[c]) n++;
        if (n > 4) return false;
        if (n == 4) four.push_back((int)l);
    }
    for (size_t i = 0; i < four.size(); i++) for (size_t j = i + 1; j < four.size(); j++) {
        int c = meetc[four[i]][four[j]];
        if (c >= 0 && !P.inF[c]) return false;
    }
    return true;
}

// ---------- Algorithm 2 ----------
struct Flow {
    const Pos &P;
    VI L;                 // live lines
    int own[216];
    Flow(const Pos &p) : P(p) { for (int i = 0; i < 216; i++) own[i] = -1;
        for (size_t l = 0; l < LN.size(); l++) if (P.live(l)) L.push_back((int)l); }
};
static bool augment(const Pos &P, int *own, const VI &U) {
    static thread_local int parL[160], parC[160]; static thread_local char vis[160];
    memset(vis, 0, sizeof vis);
    deque<int> q;
    for (int l : U) { vis[l] = 1; parL[l] = -1; parC[l] = -1; q.push_back(l); }
    while (!q.empty()) {
        int l = q.front(); q.pop_front();
        for (int c : LN[l]) {
            if (P.inF[c]) continue;
            int o = own[c];
            if (o < 0) {
                own[c] = l;
                while (parL[l] >= 0) { int cc = parC[l], pl = parL[l]; own[cc] = pl; l = pl; }
                return true;
            }
            if (!vis[o]) { vis[o] = 1; parL[o] = l; parC[o] = c; q.push_back(o); }
        }
    }
    return false;
}
static int gain(const Pos &P, const int *own, const VI &U, int theta) {
    int o2[216]; memcpy(o2, own, sizeof o2);
    int n = 0;
    while (n < theta && augment(P, o2, U)) n++;
    return n;
}
static long long g_single = 0, g_triple = 0;
// returns 0 if LEAF holds, else the number of the first failing condition (1,2,3)
static int leaf_test(const VI &B, const VI &F, int k) {
    Pos P(B, F);
    if (!admissible(P)) return 1;
    Flow fl(P);
    for (int l : fl.L) for (int i = 0; i < 2; i++) if (!augment(P, fl.own, VI{l})) return 2;
    // sanity: own is a pairing
    { map<int,int> cnt; for (int c = 0; c < 216; c++) if (fl.own[c] >= 0) {
          if (P.inF[c] || P.inB[c]) { fprintf(stderr, "bad own\n"); exit(2); }
          bool on = false; for (int x : LN[fl.own[c]]) if (x == c) on = true;
          if (!on) { fprintf(stderr, "bad own2\n"); exit(2); }
          cnt[fl.own[c]]++; }
      for (int l : fl.L) if (cnt[l] != 2) { fprintf(stderr, "not a pairing\n"); exit(2); } }
    int theta = k - (int)F.size();
    if (theta <= 0) return 0;
    VI Lw; vector<char> weak(LN.size(), 0);
    for (int l : fl.L) { g_single++; if (gain(P, fl.own, VI{l}, theta) < theta) { Lw.push_back(l); weak[l] = 1; } }
    for (int b : Lw) {
        VI nb;
        for (int a : Lw) if (a != b) { int c = meetc[a][b]; if (c >= 0 && !P.inF[c]) nb.push_back(a); }
        for (size_t i = 0; i < nb.size(); i++) for (size_t j = i + 1; j < nb.size(); j++) {
            g_triple++;
            if (gain(P, fl.own, VI{nb[i], b, nb[j]}, theta) < theta) return 3;
        }
    }
    return 0;
}

// ---------- canonical form by brute force ----------
static void canon(const VI &B, const VI &F, VI &cB, VI &cF) {
    bool first = true; VI gb(B.size()), gf(F.size());
    for (auto &g : G) {
        for (size_t i = 0; i < B.size(); i++) gb[i] = g[B[i]];
        sort(gb.begin(), gb.end());
        if (!first && gb > cB) continue;
        for (size_t i = 0; i < F.size(); i++) gf[i] = g[F[i]];
        sort(gf.begin(), gf.end());
        if (first || gb < cB || (gb == cB && gf < cF)) { cB = gb; cF = gf; first = false; }
    }
}

static VI parse_set(const string &s) {
    VI v; if (s == "-") return v;
    stringstream ss(s); string tok;
    while (getline(ss, tok, ',')) v.push_back(stoi(tok));
    return v;
}
static string set_str(const VI &v) {
    if (v.empty()) return "-";
    string s; for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += to_string(v[i]); }
    return s;
}
static string key(char ty, int t, const VI &B, const VI &F) {
    return string(1, ty) + " " + to_string(t) + " " + set_str(B) + " " + set_str(F);
}

struct St { char ty; int t; VI B, F; };

static int do_expand(const char *tblf, const char *lvf, int RMAX) {
    unordered_map<string,int> Tbl; unordered_map<string,int> Lv; // value: move / reached flag
    unordered_map<string,int> tblReached;
    long long tlines = 0, llines = 0, malformed = 0, dupT = 0, dupL = 0;
    {
        ifstream in(tblf); string line;
        while (getline(in, line)) {
            tlines++;
            stringstream ss(line); int t, m; string b, f;
            if (!(ss >> t >> b >> f >> m)) { malformed++; continue; }
            VI B = parse_set(b), F = parse_set(f);
            if (!is_sorted(B.begin(), B.end()) || !is_sorted(F.begin(), F.end()) || (int)B.size() != t - 1 || (int)F.size() > t) malformed++;
            string k = key('Q', t, B, F);
            if (Tbl.count(k)) dupT++;
            Tbl[k] = m;
        }
    }
    {
        ifstream in(lvf); string line;
        while (getline(in, line)) {
            llines++;
            stringstream ss(line); int t; string b, f;
            if (!(ss >> t >> b >> f)) { malformed++; continue; }
            VI B = parse_set(b), F = parse_set(f);
            if (!is_sorted(B.begin(), B.end()) || !is_sorted(F.begin(), F.end()) || (int)B.size() != t || (int)F.size() > t) malformed++;
            string k = key('P', t, B, F);
            if (Lv.count(k)) dupL++;
            Lv[k] = 0;
        }
    }
    printf("table_lines %lld leaf_lines %lld malformed %lld dup_table %lld dup_leaf %lld\n", tlines, llines, malformed, dupT, dupL);

    const int T = 12;
    long long nP[T] = {0}, nLeaf[T] = {0}, nExp[T] = {0}, nQ[T] = {0}, thr[T] = {0}, dng[T] = {0}, vii[T] = {0}, vc[T] = {0}, vd[T] = {0}, qii[T] = {0};
    long long nonbusy[T] = {0};
    long long errors = 0, leaf_fail = 0, leaf_by_theta[12] = {0};
    long long leafT_theta[T][12]; memset(leafT_theta, 0, sizeof leafT_theta);
    long long childA = 0, childI = 0, childII = 0;
    unordered_set<string> Seen; deque<St> W;
    auto push = [&](char ty, int t, const VI &B, const VI &F) -> int {
        // returns 0 if void, 1 otherwise
        if ((ty == 'P' && (int)B.size() != t) || (ty == 'Q' && (int)B.size() != t - 1) || (int)F.size() > t) { fprintf(stderr, "shape error\n"); exit(2); }
        Pos P(B, F);
        if (!admissible(P)) return 0;
        St s; s.ty = ty; s.t = t; canon(B, F, s.B, s.F);
        string k = key(ty, t, s.B, s.F);
        if (Seen.insert(k).second) W.push_back(std::move(s));
        return 1;
    };
    push('P', 0, VI(), VI());
    while (!W.empty()) {
        St s = W.front(); W.pop_front();
        int t = s.t;
        Pos P(s.B, s.F);
        if (s.ty == 'P') {
            nP[t]++;
            string k = key('P', t, s.B, s.F);
            auto it = Lv.find(k);
            if (it != Lv.end()) {
                it->second = 1; nLeaf[t]++;
                int th = t - (int)s.F.size(); leaf_by_theta[th]++; leafT_theta[t][th]++;
                int r = leaf_test(s.B, s.F, t);
                if (r != 0) { leaf_fail++; errors++; printf("ERROR leaf fails (L%d): %s\n", r, k.c_str()); }
                continue;
            }
            nExp[t]++;
            if (!(t < RMAX)) { errors++; printf("ERROR unlisted P-state with t >= Rmax: %s\n", k.c_str()); continue; }
            // LOST: not evaluated; Proposition 5.9(c) needs t <= 9
            if (t > 9) { errors++; printf("ERROR t>9 expanded, LOST not implemented\n"); continue; }
            // child (a)
            { VI B = s.B, F = s.F; if (!push('Q', t + 1, B, F)) { errors++; printf("ERROR void child (a)\n"); } childA++; }
            // children (c)
            for (size_t l = 0; l < LN.size(); l++) {
                if (!P.live(l)) continue;
                for (int e : LN[l]) {
                    if (P.inF[e]) continue;
                    set<int> F2(s.F.begin(), s.F.end());
                    for (int c : LN[l]) if (c != e) F2.insert(c);
                    if ((int)F2.size() > t + 1) continue;
                    VI B2 = s.B; B2.push_back(e);
                    VI F2v(F2.begin(), F2.end());
                    if (push('P', t + 1, B2, F2v)) thr[t + 1]++; else vc[t + 1]++;
                }
            }
            // children (d)
            for (int c = 0; c < 216; c++) {
                if (P.inB[c] || P.inF[c]) continue;
                VI lv; for (int l : linesOf[c]) if (P.live(l)) lv.push_back(l);
                for (size_t i = 0; i < lv.size(); i++) for (size_t j = i + 1; j < lv.size(); j++) {
                    int a = lv[i], b = lv[j];
                    int fa = 0, fb = 0;
                    for (int x : LN[a]) if (P.inF[x]) fa++;
                    for (int x : LN[b]) if (P.inF[x]) fb++;
                    // cheap necessary bound (identity of Section 7.2); the literal test follows
                    if ((int)s.F.size() + (4 - fa) + (4 - fb) > t + 1) continue;
                    for (int a1 : LN[a]) { if (a1 == c || P.inF[a1]) continue;
                    for (int b1 : LN[b]) { if (b1 == c || P.inF[b1]) continue;
                        set<int> F2(s.F.begin(), s.F.end());
                        for (int x : LN[a]) if (x != c && x != a1) F2.insert(x);
                        for (int x : LN[b]) if (x != c && x != b1) F2.insert(x);
                        if ((int)F2.size() > t + 1) continue;
                        VI B2 = s.B; B2.push_back(c);
                        VI F2v(F2.begin(), F2.end());
                        if (push('P', t + 1, B2, F2v)) dng[t + 1]++; else vd[t + 1]++;
                    } }
                }
            }
        } else {
            nQ[t]++;
            string k = key('Q', t, s.B, s.F);
            auto it = Tbl.find(k);
            if (it == Tbl.end()) { errors++; printf("ERROR no table entry: %s\n", k.c_str()); continue; }
            tblReached[k] = 1;
            int m = it->second;
            if (m < 0 || m > 215 || P.inB[m] || P.inF[m]) { errors++; printf("ERROR illegal move: %s\n", k.c_str()); continue; }
            if (!busy(m)) nonbusy[t]++;
            { VI B2 = s.B; B2.push_back(m); if (!push('P', t, B2, s.F)) { errors++; printf("ERROR void child (i)\n"); } childI++; }
            if ((int)s.F.size() < t) {
                qii[t]++; childII++;
                VI F2 = s.F; F2.push_back(m);
                if (!push('Q', t, s.B, F2)) vii[t]++;
            }
        }
    }
    long long unreachedL = 0; for (auto &kv : Lv) if (!kv.second) unreachedL++;
    long long unreachedT = (long long)Tbl.size() - (long long)tblReached.size();
    auto row = [&](const char *name, long long *a) { long long tot = 0; printf("%-22s", name); for (int t = 0; t <= 10; t++) { printf(" %lld", a[t]); tot += a[t]; } printf("  total %lld\n", tot); };
    row("P-states", nP); row("leaves", nLeaf); row("expanded", nExp); row("Q-states", nQ);
    row("threat children", thr); row("danger children", dng); row("void (ii)", vii); row("void (c)", vc); row("void (d)", vd);
    row("Q with |F|<t", qii); row("nonbusy moves", nonbusy);
    printf("children a %lld i %lld ii %lld\n", childA, childI, childII);
    printf("states_visited %zu\n", Seen.size());
    printf("leaves by theta:"); for (int i = 0; i <= 10; i++) printf(" %lld", leaf_by_theta[i]); printf("\n");
    for (int t = 8; t <= 10; t++) { printf("leaves t=%d by theta:", t); for (int i = 0; i <= 8; i++) printf(" %lld", leafT_theta[t][i]); printf("\n"); }
    printf("unreached table entries %lld, unreached leaves %lld\n", unreachedT, unreachedL);
    printf("leaf failures %lld; single-line gains %lld; triple gains %lld\n", leaf_fail, g_single, g_triple);
    printf("errors %lld\n", errors);
    printf(errors == 0 && unreachedL == 0 && unreachedT == 0 ? "RESULT accept\n" : "RESULT reject-or-unreached\n");
    return errors ? 1 : 0;
}

// exact enumeration of C(B,F,k), Remark 4.5 (iv)
static long long enum_pos, enum_max;
static bool has_pairing(const VI &B, const VI &X) {
    Pos P(B, X); Flow fl(P);
    for (int l : fl.L) for (int i = 0; i < 2; i++) if (!augment(P, fl.own, VI{l})) return false;
    return true;
}
static int enum_rec(const VI &B, VI &X, int k, int from) { // 1 ok, 0 violated, -1 limit
    enum_pos++;
    if (enum_max && enum_pos > enum_max) return -1;
    if (!has_pairing(B, X)) return 0;
    if ((int)X.size() >= k) return 1;
    for (int c = from; c < 216; c++) {
        if (find(B.begin(), B.end(), c) != B.end() || find(X.begin(), X.end(), c) != X.end()) continue;
        X.push_back(c);
        Pos P(B, X);
        int r = 1;
        if (admissible(P)) r = enum_rec(B, X, k, c + 1);
        X.pop_back();
        if (r != 1) return r;
    }
    return 1;
}


// ---------- relaxed busy-only game (Section 6.5): exhaustive evaluation with memo ----------
static unordered_map<string,char> memoP, memoQ; static long long nPst = 0, nQst = 0, nLeafT = 0; static int BRMAX; static VI BUSYC;
static bool quick_reject(const VI &B, const VI &F, int k) {   // sound: literal (L3) for whole F-components
    Pos P(B, F); int theta = k - (int)F.size(); if (theta <= 0) return false;
    VI L; for (size_t l = 0; l < LN.size(); l++) if (P.live(l)) L.push_back((int)l);
    vector<int> comp(LN.size(), -1); int nc = 0;
    for (int s : L) if (comp[s] < 0) {
        VI st{s}; comp[s] = nc; VI mem; set<int> cells;
        while (!st.empty()) { int l = st.back(); st.pop_back(); mem.push_back(l);
            for (int c : LN[l]) { if (P.inF[c]) continue; cells.insert(c);
                for (int l2 : linesOf[c]) if (comp[l2] < 0 && P.live(l2)) { comp[l2] = nc; st.push_back(l2); } } }
        if (mem.size() >= 3 && (int)cells.size() - 2 * (int)mem.size() < theta) return true;
        nc++;
    }
    return false;
}
static bool bwinQ(const VI &B, const VI &F, int t);
static bool bwinP(const VI &B0, const VI &F0, int t) {
    VI B, F; canon(B0, F0, B, F); string k = key('P', t, B, F);
    auto it = memoP.find(k); if (it != memoP.end()) return it->second;
    nPst++;
    bool w = false;
    if (!quick_reject(B, F, t)) { nLeafT++; w = (leaf_test(B, F, t) == 0); }
    if (!w && t < BRMAX) w = bwinQ(B, F, t + 1);
    memoP[k] = w; return w;
}
static bool bwinQ(const VI &B0, const VI &F0, int t) {
    VI B, F; canon(B0, F0, B, F); string k = key('Q', t, B, F);
    auto it = memoQ.find(k); if (it != memoQ.end()) return it->second;
    nQst++;
    bool w = false;
    for (int m : BUSYC) {
        if (find(B.begin(), B.end(), m) != B.end() || find(F.begin(), F.end(), m) != F.end()) continue;
        if ((int)F.size() < t) {
            VI F2 = F; F2.push_back(m); Pos P2(B, F2);
            if (admissible(P2) && !bwinQ(B, F2, t)) continue;      // child (ii) fails
        }
        VI B2 = B; B2.push_back(m);
        if (bwinP(B2, F, t)) { w = true; break; }
    }
    memoQ[k] = w; return w;
}

int main(int argc, char **argv) {
    build_geom();
    string mode = argc > 1 ? argv[1] : "";
    if (mode == "geom") {
        int nb = 0, c7 = 0, c4 = 0, c3 = 0;
        for (int c = 0; c < 216; c++) { if (busy(c)) nb++; int n = linesOf[c].size(); if (n == 7) c7++; else if (n == 4) c4++; else if (n == 3) c3++; }
        printf("lines %zu group %zu busy %d cells_on_7 %d on_4 %d on_3 %d\n", LN.size(), G.size(), nb, c7, c4, c3);
        printf("busy ids:"); for (int c = 0; c < 216; c++) if (busy(c)) printf(" %d", c); printf("\n");
        // orbits of G on cells
        vector<int> orb(216, -1); int no = 0;
        for (int c = 0; c < 216; c++) if (orb[c] < 0) { for (auto &g : G) orb[g[c]] = no; printf("orbit rep %d (%d,%d,%d)\n", c, c / 36, (c / 6) % 6, c % 6); no++; }
        // group closure check on a sample: g*h in G
        set<array<unsigned char,216>> GS(G.begin(), G.end()); bool closed = true;
        for (size_t i = 0; i < G.size(); i += 37) for (size_t j = 0; j < G.size(); j += 41) { array<unsigned char,216> gh; for (int c = 0; c < 216; c++) gh[c] = G[i][G[j][c]]; if (!GS.count(gh)) closed = false; }
        printf("closure(sample) %d\n", (int)closed);
        // every g maps lines to lines
        set<array<int,6>> LS(LN.begin(), LN.end()); bool lin = true;
        for (auto &g : G) for (auto &l : LN) { array<int,6> m; for (int i = 0; i < 6; i++) m[i] = g[l[i]]; sort(m.begin(), m.end()); if (!LS.count(m)) lin = false; }
        printf("lines preserved %d\n", (int)lin);
        return 0;
    }
    if (mode == "busyonly") {
        BRMAX = atoi(argv[2]); for (int c = 0; c < 216; c++) if (busy(c)) BUSYC.push_back(c);
        bool w = bwinP(VI(), VI(), 0);
        printf("relaxed busy-only game, Rmax=%d: root %s; P-states %lld, Q-states %lld, LEAF tests %lld\n", BRMAX, w ? "WON" : "NOT WON", nPst, nQst, nLeafT);
        return 0;
    }
    if (mode == "expand") return do_expand(argv[2], argv[3], atoi(argv[4]));
    if (mode == "leaf") {
        string line;
        while (getline(cin, line)) {
            if (line.empty()) continue;
            stringstream ss(line); int k; string b, f; ss >> k >> b >> f;
            int r = leaf_test(parse_set(b), parse_set(f), k);
            if (r == 0) printf("%s : true\n", line.c_str()); else printf("%s : false (%d)\n", line.c_str(), r);
        }
        return 0;
    }
    if (mode == "canon") {
        string line;
        while (getline(cin, line)) {
            if (line.empty()) continue;
            stringstream ss(line); string b, f; ss >> b >> f;
            VI cB, cF; canon(parse_set(b), parse_set(f), cB, cF);
            printf("%s | %s -> %s | %s\n", b.c_str(), f.c_str(), set_str(cB).c_str(), set_str(cF).c_str());
        }
        return 0;
    }
    if (mode == "enum") {
        enum_max = argc > 2 ? atoll(argv[2]) : 0;
        string line; long long n = 0, holds = 0, fails = 0, lim = 0, maxpos = 0, totpos = 0;
        bool verbose = argc > 3;
        while (getline(cin, line)) {
            if (line.empty()) continue;
            stringstream ss(line); int k; string b, f; ss >> k >> b >> f;
            VI B = parse_set(b), X = parse_set(f);
            Pos P(B, X);
            n++; enum_pos = 0;
            int r = admissible(P) ? enum_rec(B, X, k, 0) : 1;
            // NOTE: cells are added in increasing order but must exceed nothing in F: restart from 0 is
            // correct only if we forbid cells below the last added one; F itself is fixed.
            if (r == 1) holds++; else if (r == 0) fails++; else lim++;
            maxpos = max(maxpos, enum_pos); totpos += enum_pos;
            if (verbose) printf("%s : %s positions %lld\n", line.c_str(), r == 1 ? "holds" : r == 0 ? "FAILS" : "limit", enum_pos);
        }
        printf("instances %lld holds %lld fails %lld limit %lld max_positions %lld total_positions %lld\n", n, holds, fails, lim, maxpos, totpos);
        return 0;
    }
    fprintf(stderr, "usage\n"); return 2;
}

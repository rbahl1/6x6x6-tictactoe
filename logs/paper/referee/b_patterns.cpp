// Referee 1b: literal enumeration of the patterns E2 and E3 of Definition 5.2 for B = F = empty
// (all 148 lines live), with own geometry code.  Counts the distinct stones of every pattern.
// Checks Prop. 5.9(a),(b) (>= 11 and >= 12), Prop. C.3(b),(c) (E3 >= 13, E2 = 11 attained) and
// the numbers of Computed Fact C.4.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <algorithm>
using namespace std;
typedef array<int,6> Line;
static vector<Line> L;
static int meetc[148][148]; // common cell or -1
static bool on[148][216];

static void build() {
    set<array<int,6>> S;
    // components: 0..5 constant, 6 = t, 7 = 5-t
    for (int f0 = 0; f0 < 8; f0++) for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) {
        if (f0 < 6 && f1 < 6 && f2 < 6) continue;
        array<int,6> l;
        for (int t = 0; t < 6; t++) {
            int f[3] = {f0, f1, f2}, c[3];
            for (int i = 0; i < 3; i++) c[i] = f[i] < 6 ? f[i] : (f[i] == 6 ? t : 5 - t);
            l[t] = 36 * c[0] + 6 * c[1] + c[2];
        }
        sort(l.begin(), l.end());
        S.insert(l);
    }
    for (auto &l : S) L.push_back(l);
    if (L.size() != 148) { printf("line count %zu\n", L.size()); throw 1; }
    for (int a = 0; a < 148; a++) for (int c = 0; c < 216; c++) on[a][c] = false;
    for (int a = 0; a < 148; a++) for (int c : L[a]) on[a][c] = true;
    for (int a = 0; a < 148; a++) for (int b = 0; b < 148; b++) {
        meetc[a][b] = -1;
        if (a == b) continue;
        int n = 0, cc = -1;
        for (int c : L[a]) if (on[b][c]) { n++; cc = c; }
        if (n > 1) { printf("two lines share two cells\n"); throw 1; }
        if (n == 1) meetc[a][b] = cc;
    }
}

// all 4-subsets of line `ln` minus cell `excl`, optionally required to contain `must` (-1: none)
static vector<array<int,4>> subsets4(int ln, int excl, int must) {
    vector<int> cells;
    for (int c : L[ln]) if (c != excl) cells.push_back(c);
    vector<array<int,4>> out;
    if (cells.size() != 5) throw 2;
    for (int drop = 0; drop < 5; drop++) {
        array<int,4> s; int k = 0; bool has = (must < 0);
        for (int i = 0; i < 5; i++) if (i != drop) { s[k++] = cells[i]; if (cells[i] == must) has = true; }
        if (has) out.push_back(s);
    }
    return out;
}

int main() {
    build();
    // ---------------- E2 ----------------
    map<int, uint64_t> h2;
    uint64_t n2 = 0;
    for (int l = 0; l < 148; l++) for (int a = 0; a < 148; a++) {
        if (a == l) continue;
        int x = meetc[l][a];
        if (x < 0) continue;                       // l cap a = {x}
        for (int e : L[l]) {
            if (e == x) continue;                  // x, e distinct; e in l
            for (int b = 0; b < 148; b++) {
                if (b == l || b == a) continue;    // pairwise distinct lines
                int c = meetc[a][b];
                if (c < 0) continue;               // a cap b = {c}
                if (c == x || c == e) continue;    // pairwise distinct cells
                if (on[b][x] || on[b][e]) continue;// x not in b, e not in b
                auto SA = subsets4(a, c, x);       // S_a in a\{c}, |S_a| = 4, x in S_a
                auto SB = subsets4(b, c, -1);      // S_b in b\{c}, |S_b| = 4
                for (auto &sa : SA) for (auto &sb : SB) {
                    bool m[216] = {false}; int cnt = 0;
                    for (int q : L[l]) if (q != e && !m[q]) { m[q] = true; cnt++; }   // S_l = l \ {e}
                    for (int q : sa) if (!m[q]) { m[q] = true; cnt++; }
                    for (int q : sb) if (!m[q]) { m[q] = true; cnt++; }
                    h2[cnt]++; n2++;
                }
            }
        }
    }
    printf("E2 patterns (B=F=empty): %llu\n", (unsigned long long)n2);
    for (auto &p : h2) printf("   %d stones: %llu\n", p.first, (unsigned long long)p.second);
    // ---------------- E3 ----------------
    map<int, uint64_t> h3;
    uint64_t n3 = 0;
    for (int a = 0; a < 148; a++) for (int a2 = 0; a2 < 148; a2++) {
        if (a2 == a) continue;
        int x = meetc[a][a2];
        if (x < 0) continue;                       // a cap a' = {x}
        for (int b = 0; b < 148; b++) {
            if (b == a || b == a2) continue;
            int c = meetc[a][b];
            if (c < 0 || c == x) continue;         // a cap b = {c}, c != x
            if (on[b][x]) continue;                // x not in b
            for (int b2 = 0; b2 < 148; b2++) {
                if (b2 == a || b2 == a2 || b2 == b) continue;
                int c2 = meetc[a2][b2];
                if (c2 < 0 || c2 == x || c2 == c) continue; // a' cap b' = {c'}, cells distinct
                if (on[b2][x]) continue;           // x not in b'
                auto SA = subsets4(a, c, x), SA2 = subsets4(a2, c2, x);
                auto SB = subsets4(b, c, -1), SB2 = subsets4(b2, c2, -1);
                for (auto &sa : SA) {
                    if (find(sa.begin(), sa.end(), c2) != sa.end()) continue;   // c' in no stone set
                    for (auto &sa2 : SA2) {
                        if (find(sa2.begin(), sa2.end(), c) != sa2.end()) continue;
                        for (auto &sb : SB) {
                            if (find(sb.begin(), sb.end(), c2) != sb.end()) continue;
                            for (auto &sb2 : SB2) {
                                if (find(sb2.begin(), sb2.end(), c) != sb2.end()) continue;
                                bool m[216] = {false}; int cnt = 0;
                                for (int q : sa)  if (!m[q]) { m[q] = true; cnt++; }
                                for (int q : sa2) if (!m[q]) { m[q] = true; cnt++; }
                                for (int q : sb)  if (!m[q]) { m[q] = true; cnt++; }
                                for (int q : sb2) if (!m[q]) { m[q] = true; cnt++; }
                                h3[cnt]++; n3++;
                            }
                        }
                    }
                }
            }
        }
    }
    printf("E3 patterns (B=F=empty): %llu\n", (unsigned long long)n3);
    for (auto &p : h3) printf("   %d stones: %llu\n", p.first, (unsigned long long)p.second);
    return 0;
}

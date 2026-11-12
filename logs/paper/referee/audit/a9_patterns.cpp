// referee: literal enumeration of the patterns E2 and E3 of Definition 5.2 for B = F = empty (Computed Fact C.4)
#include <bits/stdc++.h>
using namespace std;
typedef bitset<216> BS;
int main() {
    set<array<int,6>> S;
    for (int f1 = 0; f1 < 8; f1++) for (int f2 = 0; f2 < 8; f2++) for (int f3 = 0; f3 < 8; f3++) {
        if (f1 < 6 && f2 < 6 && f3 < 6) continue; array<int,6> l; int f[3] = {f1, f2, f3};
        for (int t = 0; t < 6; t++) { int c[3]; for (int i = 0; i < 3; i++) c[i] = f[i] < 6 ? f[i] : (f[i] == 6 ? t : 5 - t); l[t] = 36 * c[0] + 6 * c[1] + c[2]; }
        sort(l.begin(), l.end()); S.insert(l);
    }
    vector<array<int,6>> LN(S.begin(), S.end()); int NL = LN.size();
    vector<vector<int>> meet(NL, vector<int>(NL, -1));
    for (int a = 0; a < NL; a++) for (int b = 0; b < NL; b++) if (a != b) for (int x : LN[a]) for (int y : LN[b]) if (x == y) meet[a][b] = x;
    auto on = [&](int l, int c) { for (int x : LN[l]) if (x == c) return true; return false; };
    // all 4-subsets of a line minus one cell c (5 cells): given by the omitted cell
    auto sets4 = [&](int l, int c) { vector<BS> r; for (int om : LN[l]) { if (om == c) continue; BS s; for (int x : LN[l]) if (x != c && x != om) s.set(x); r.push_back(s); } return r; };
    // E2
    map<int,long long> h2; long long n2 = 0;
    for (int l = 0; l < NL; l++) for (int a = 0; a < NL; a++) { if (a == l) continue; int x = meet[l][a]; if (x < 0) continue;
        for (int b = 0; b < NL; b++) { if (b == l || b == a) continue; int c = meet[a][b]; if (c < 0) continue; if (on(b, x)) continue;
            for (int e : LN[l]) { if (on(b, e)) continue; if (e == x || e == c || x == c) continue;
                BS Sl; for (int y : LN[l]) if (y != e) Sl.set(y);
                for (auto &Sa : sets4(a, c)) { if (!Sa.test(x)) continue;
                    for (auto &Sb : sets4(b, c)) { n2++; h2[(int)(Sl | Sa | Sb).count()]++; } } } } }
    printf("E2 patterns %lld:", n2); for (auto &kv : h2) printf("  %d stones: %lld", kv.first, kv.second); printf("\n");
    // E3
    map<int,long long> h3; long long n3 = 0;
    for (int a = 0; a < NL; a++) for (int a1 = 0; a1 < NL; a1++) { if (a1 == a) continue; int x = meet[a][a1]; if (x < 0) continue;
        for (int b = 0; b < NL; b++) { if (b == a || b == a1) continue; int c = meet[a][b]; if (c < 0 || on(b, x)) continue;
            for (int b1 = 0; b1 < NL; b1++) { if (b1 == a || b1 == a1 || b1 == b) continue; int c1 = meet[a1][b1]; if (c1 < 0 || on(b1, x)) continue;
                if (x == c || x == c1 || c == c1) continue;
                vector<BS> A, A1, Bv, B1;
                for (auto &s : sets4(a, c)) if (s.test(x) && !s.test(c) && !s.test(c1)) A.push_back(s);
                for (auto &s : sets4(a1, c1)) if (s.test(x) && !s.test(c) && !s.test(c1)) A1.push_back(s);
                for (auto &s : sets4(b, c)) if (!s.test(c) && !s.test(c1)) Bv.push_back(s);
                for (auto &s : sets4(b1, c1)) if (!s.test(c) && !s.test(c1)) B1.push_back(s);
                for (auto &sa : A) for (auto &sa1 : A1) { BS u = sa | sa1; for (auto &sb : Bv) { BS u2 = u | sb; for (auto &sb1 : B1) { n3++; h3[(int)(u2 | sb1).count()]++; } } } } } }
    printf("E3 patterns %lld:", n3); for (auto &kv : h3) printf("  %d stones: %lld", kv.first, kv.second); printf("\n");
}

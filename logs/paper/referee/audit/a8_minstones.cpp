// referee: maximum number of lines killed by j cells on the n^3 board (branch and bound), own geometry
#include <bits/stdc++.h>
using namespace std;
int n, NC; vector<vector<int>> LN; vector<vector<int>> lo; vector<int> ord; int best; vector<int> deg;
vector<int> cnt; long long nodes;
void rec(int pos, int left, int cur) {
    nodes++;
    if (cur > best) best = cur;
    if (!left) return;
    for (int i = pos; i < NC; i++) {
        int c = ord[i];
        // bound: remaining cells have degree <= deg[c] (sorted descending)
        if (cur + left * deg[c] <= best) return;
        int g = 0; for (int l : lo[c]) if (cnt[l]++ == 0) g++;
        rec(i + 1, left - 1, cur + g);
        for (int l : lo[c]) cnt[l]--;
    }
}
int main(int argc, char **argv) {
    n = atoi(argv[1]); int j = atoi(argv[2]); NC = n * n * n;
    set<vector<int>> S;
    for (int f1 = 0; f1 < n + 2; f1++) for (int f2 = 0; f2 < n + 2; f2++) for (int f3 = 0; f3 < n + 2; f3++) {
        if (f1 < n && f2 < n && f3 < n) continue;
        vector<int> l; int f[3] = {f1, f2, f3};
        for (int t = 0; t < n; t++) { int c[3]; for (int i = 0; i < 3; i++) c[i] = f[i] < n ? f[i] : (f[i] == n ? t : n - 1 - t); l.push_back(n * n * c[0] + n * c[1] + c[2]); }
        sort(l.begin(), l.end()); S.insert(l);
    }
    LN.assign(S.begin(), S.end()); lo.assign(NC, {});
    for (size_t i = 0; i < LN.size(); i++) for (int c : LN[i]) lo[c].push_back(i);
    deg.resize(NC); for (int c = 0; c < NC; c++) deg[c] = lo[c].size();
    ord.resize(NC); iota(ord.begin(), ord.end(), 0); stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return deg[a] > deg[b]; });
    cnt.assign(LN.size(), 0); best = 0; rec(0, j, 0);
    int need = (int)LN.size() - (NC - j) / 2;   // 2|L(B)| <= n^3 - j  <=>  killed >= lines - floor((n^3-j)/2)
    printf("n=%d lines=%zu stones=%d max_killed=%d needed=%d nodes=%lld -> %s\n", n, LN.size(), j, best, need, nodes, best >= need ? "counting condition CAN hold" : "counting condition fails for every set");
}

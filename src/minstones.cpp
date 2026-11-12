// minstones.cpp -- regression 1 (SPEC section 8 item 1).  Self-contained, general board size n.
//
// Board: n x n x n cells, cell (x,y,z) has id x*n*n + y*n + z.
// Lines: all sets {(f1(t),f2(t),f3(t)) : t=0..n-1}, each fi a constant, t or n-1-t, not all constant,
//        duplicates removed: ((n+2)^3 - n^3)/2 lines.
// Question: the minimum number j of second-player stones B such that a *pairing* exists on the otherwise
//        empty board: every line containing no stone of B (a live line) gets 2 cells of its own, the cells
//        lie on the line, are not in B, and no cell is used twice.
//
// Both directions are established by exact integer computation and reported separately.
//
//  (lower)  For every size j < min the program enumerates ALL j-subsets of cells up to the following sound
//           reductions, and finds no set with a pairing.
//           * Counting test.  A pairing uses 2 distinct cells per live line, so 2*live <= n^3 - j, i.e. the
//             stones must kill at least need(j) = L - floor((n^3 - j)/2) lines.
//           * Bound.  Stones are added in a fixed cell order (positions strictly increasing), so every set
//             is generated exactly once.  The marginal kill count m(c) = number of live lines through c can
//             only decrease when stones are added.  Hence the number of lines finally killed by a set that
//             extends the current partial set by the cell at position p and r-1 further cells at positions
//             > p is at most  K + m(c_p) + (sum of the r-1 largest current m over positions > p).
//             If that is < need(j), the extension is skipped.
//           * Symmetry, first stone only.  Every generator of the symmetry group is checked by the program
//             to be a bijection of the cells that maps every line onto a line, so the group preserves the
//             existence of pairings.  The cell order lists the orbits one after another (orbit after orbit,
//             the representative first inside its orbit).  Let B be any set and c its first cell in the
//             order; c lies in the earliest orbit that B meets.  Take a group element g mapping c to the
//             representative of that orbit: g(B) meets the same orbits (orbits are invariant), so the first
//             cell of g(B) in the order is the representative.  Thus it suffices to enumerate the sets whose
//             first cell is an orbit representative; all other cells are unrestricted (later positions).
//           * Every set that survives the counting test is decided exactly by bipartite matching.  A
//             negative answer is not trusted: the alternating tree of the failed augmentation yields a set
//             S of live lines with |N(S) \ B| < 2|S| (a Hall violator), which is re-counted from scratch.
//           (Remark, not used: a pairing for B is also a pairing for B + {b}, so "no set of size min-1"
//            would already imply the lower bound.  The program nevertheless exhausts every size j < min.)
//
//  (upper)  The first set of size min found by the same enumeration, together with a pairing computed by
//           bipartite matching (Kuhn augmenting paths).  The pairing is re-verified by a separate checker
//           that uses only the definition.
//
// Usage: bin/minstones [--n 5,6,7,8] [--out results/regression/minstones.json] [--time-limit SEC_PER_N]
// Output: the JSON file, a second file <out minus .json>_pairings.json with the witness pairings, and a
//         PASS/FAIL table on stdout.  Exit code 0 iff every n with an expected value passes.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

static double cpu_now() {
    timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

[[noreturn]] static void die(const char* msg) {
    fprintf(stderr, "INTERNAL ERROR: %s\n", msg);
    exit(2);
}

// ----------------------------------------------------------------------------------------------- board

struct Board {
    int n = 0, N = 0, L = 0, maxdeg = 0;
    vector<int> lineFlat;            // L*n cell ids, each line ascending, lines sorted lexicographically
    vector<vector<int>> cl;          // lines through a cell
    vector<vector<int>> gens;        // verified generators of the symmetry group (cell permutations)
    long long groupOrder = 0;        // order of the generated group (closure, informational)
    vector<int> orbitOf;             // orbit index per cell
    int nOrbits = 0;
    vector<int> order;               // position -> cell
    vector<char> isRep;              // position -> is the first cell of its orbit in the order

    int cid(int x, int y, int z) const { return (x * n + y) * n + z; }
    array<int, 3> xyz(int c) const { return {c / (n * n), (c / n) % n, c % n}; }
    const int* line(int l) const { return &lineFlat[(size_t)l * n]; }

    explicit Board(int n_) : n(n_), N(n_ * n_ * n_) {
        build_lines();
        build_symmetry();
        build_order();
    }

    void build_lines() {
        set<vector<int>> S;
        // f in 0..n-1: constant; n: t; n+1: n-1-t
        auto ev = [&](int f, int t) { return f < n ? f : (f == n ? t : n - 1 - t); };
        for (int f1 = 0; f1 < n + 2; f1++)
            for (int f2 = 0; f2 < n + 2; f2++)
                for (int f3 = 0; f3 < n + 2; f3++) {
                    if (f1 < n && f2 < n && f3 < n) continue;
                    vector<int> v;
                    for (int t = 0; t < n; t++) v.push_back(cid(ev(f1, t), ev(f2, t), ev(f3, t)));
                    sort(v.begin(), v.end());
                    if (adjacent_find(v.begin(), v.end()) != v.end()) die("line with repeated cell");
                    S.insert(v);
                }
        L = (int)S.size();
        for (auto& v : S) lineFlat.insert(lineFlat.end(), v.begin(), v.end());
        cl.assign(N, {});
        for (int l = 0; l < L; l++)
            for (int i = 0; i < n; i++) cl[line(l)[i]].push_back(l);
        maxdeg = 0;
        for (int c = 0; c < N; c++) maxdeg = max(maxdeg, (int)cl[c].size());
    }

    // A generator is accepted only after checking that it is a bijection mapping every line onto a line.
    void add_generator(const vector<int>& p) {
        vector<char> seen(N, 0);
        for (int c = 0; c < N; c++) {
            if (p[c] < 0 || p[c] >= N || seen[p[c]]) die("generator is not a bijection");
            seen[p[c]] = 1;
        }
        set<vector<int>> S;
        for (int l = 0; l < L; l++) S.insert(vector<int>(line(l), line(l) + n));
        for (int l = 0; l < L; l++) {
            vector<int> v(n);
            for (int i = 0; i < n; i++) v[i] = p[line(l)[i]];
            sort(v.begin(), v.end());
            if (!S.count(v)) die("generator does not map lines to lines");
        }
        gens.push_back(p);
    }

    void build_symmetry() {
        vector<int> p(N);
        // coordinate swaps x<->y and y<->z
        for (int c = 0; c < N; c++) { auto a = xyz(c); p[c] = cid(a[1], a[0], a[2]); }
        add_generator(p);
        for (int c = 0; c < N; c++) { auto a = xyz(c); p[c] = cid(a[0], a[2], a[1]); }
        add_generator(p);
        // reversal of the x coordinate
        for (int c = 0; c < N; c++) { auto a = xyz(c); p[c] = cid(n - 1 - a[0], a[1], a[2]); }
        add_generator(p);
        // relabellings s of {0..n-1} with s(n-1-i) = n-1-s(i), applied to every coordinate
        int m = n / 2;
        auto relabel = [&](const vector<int>& s) {
            for (int i = 0; i < n; i++)
                if (s[n - 1 - i] != n - 1 - s[i]) die("relabelling does not commute with reversal");
            for (int c = 0; c < N; c++) { auto a = xyz(c); p[c] = cid(s[a[0]], s[a[1]], s[a[2]]); }
            add_generator(p);
        };
        for (int i = 0; i + 1 < m; i++) {      // exchange the pairs {i,n-1-i} and {i+1,n-2-i}
            vector<int> s(n);
            for (int k = 0; k < n; k++) s[k] = k;
            swap(s[i], s[i + 1]);
            swap(s[n - 1 - i], s[n - 2 - i]);
            relabel(s);
        }
        for (int i = 0; i < m; i++) {          // exchange i and n-1-i
            vector<int> s(n);
            for (int k = 0; k < n; k++) s[k] = k;
            swap(s[i], s[n - 1 - i]);
            relabel(s);
        }
        // orbits of cells: union-find over the generators
        vector<int> par(N);
        for (int c = 0; c < N; c++) par[c] = c;
        auto find = [&](int a) { while (par[a] != a) a = par[a] = par[par[a]]; return a; };
        for (auto& g : gens)
            for (int c = 0; c < N; c++) { int a = find(c), b = find(g[c]); if (a != b) par[max(a, b)] = min(a, b); }
        orbitOf.assign(N, -1);
        nOrbits = 0;
        for (int c = 0; c < N; c++) if (find(c) == c) orbitOf[c] = nOrbits++;
        for (int c = 0; c < N; c++) orbitOf[c] = orbitOf[find(c)];
        // group order by closure (informational sanity value; the search only uses the orbits)
        set<vector<uint16_t>> seen;
        vector<vector<uint16_t>> todo;
        vector<uint16_t> id(N);
        for (int c = 0; c < N; c++) id[c] = (uint16_t)c;
        seen.insert(id);
        todo.push_back(id);
        while (!todo.empty()) {
            vector<uint16_t> a = todo.back();
            todo.pop_back();
            for (auto& g : gens) {
                vector<uint16_t> b(N);
                for (int c = 0; c < N; c++) b[c] = (uint16_t)g[a[c]];
                if (seen.insert(b).second) todo.push_back(b);
            }
        }
        groupOrder = (long long)seen.size();
    }

    // Order: orbits sorted by (degree descending, smallest cell id), cells ascending inside an orbit.
    void build_order() {
        vector<int> deg(nOrbits, -1), mn(nOrbits, N);
        for (int c = 0; c < N; c++) {
            int o = orbitOf[c];
            if (deg[o] >= 0 && deg[o] != (int)cl[c].size()) die("degree not constant on an orbit");
            deg[o] = (int)cl[c].size();
            mn[o] = min(mn[o], c);
        }
        vector<int> orb(nOrbits);
        for (int o = 0; o < nOrbits; o++) orb[o] = o;
        sort(orb.begin(), orb.end(), [&](int a, int b) { return deg[a] != deg[b] ? deg[a] > deg[b] : mn[a] < mn[b]; });
        order.clear();
        isRep.clear();
        for (int o : orb) {
            bool first = true;
            for (int c = 0; c < N; c++)
                if (orbitOf[c] == o) { order.push_back(c); isRep.push_back(first); first = false; }
        }
        if ((int)order.size() != N) die("order is not a permutation");
    }
};

// -------------------------------------------------------------------------------------------- matching

// Exact pairing test: each live line (cnt[l] == 0) is split into two copies, each copy must be matched to
// a distinct non-stone cell of the line.  Kuhn's augmenting paths.
struct Matcher {
    const Board& b;
    vector<int> cellMatch;   // cell -> copy id (2*line + k) or -1
    vector<int> copyMatch;   // copy -> cell or -1
    vector<int> vis;
    int stamp = 0;
    const char* blocked = nullptr;
    vector<int> hall;        // on failure: live lines S with |N(S) \ B| < 2|S|
    uint64_t hallVerified = 0;

    explicit Matcher(const Board& b_) : b(b_), cellMatch(b_.N, -1), copyMatch(2 * b_.L, -1), vis(b_.N, 0) {}

    bool aug(int u) {
        const int* ln = b.line(u >> 1);
        for (int i = 0; i < b.n; i++) {
            int c = ln[i];
            if (!blocked[c] && cellMatch[c] < 0) { cellMatch[c] = u; copyMatch[u] = c; return true; }
        }
        for (int i = 0; i < b.n; i++) {
            int c = ln[i];
            if (blocked[c] || vis[c] == stamp) continue;
            vis[c] = stamp;
            if (aug(cellMatch[c])) { cellMatch[c] = u; copyMatch[u] = c; return true; }
        }
        return false;
    }

    // cnt[l] = number of stones on line l; stone[c] = 1 iff c in B.  Returns true iff a pairing exists.
    bool run(const int* cnt, const char* stone) {
        blocked = stone;
        fill(cellMatch.begin(), cellMatch.end(), -1);
        fill(copyMatch.begin(), copyMatch.end(), -1);
        for (int l = 0; l < b.L; l++) {
            if (cnt[l]) continue;
            for (int k = 0; k < 2; k++) {
                ++stamp;
                if (!aug(2 * l + k)) {
                    // Every copy reached by the failed search had all its non-stone cells visited, and every
                    // visited cell is matched to a reached copy: the reached lines form a Hall violator.
                    vector<char> inS(b.L, 0);
                    inS[l] = 1;
                    for (int c = 0; c < b.N; c++)
                        if (vis[c] == stamp && !stone[c]) inS[cellMatch[c] >> 1] = 1;
                    hall.clear();
                    for (int q = 0; q < b.L; q++) if (inS[q]) hall.push_back(q);
                    verify_hall(cnt, stone);
                    return false;
                }
            }
        }
        return true;
    }

    // Recount the violator from the definition: all lines live, |union of their cells minus B| < 2|S|.
    void verify_hall(const int* cnt, const char* stone) {
        vector<char> in(b.N, 0);
        int cells = 0;
        for (int l : hall) {
            if (cnt[l]) die("Hall set contains a dead line");
            for (int i = 0; i < b.n; i++) {
                int c = b.line(l)[i];
                if (!stone[c] && !in[c]) { in[c] = 1; cells++; }
            }
        }
        if (!(cells < 2 * (int)hall.size())) die("Hall violator does not verify");
        hallVerified++;
    }
};

// Independent check of a pairing from the definition.  owner[c] = line id or -1.
static bool check_pairing(const Board& b, const vector<int>& B, const vector<int>& owner, int* liveOut, int* unassignedOut) {
    vector<char> stone(b.N, 0);
    for (int c : B) {
        if (c < 0 || c >= b.N || stone[c]) return false;
        stone[c] = 1;
    }
    vector<int> got(b.L, 0);
    int unassigned = 0;
    for (int c = 0; c < b.N; c++) {
        int l = owner[c];
        if (l < 0) { if (!stone[c]) unassigned++; continue; }
        if (stone[c] || l >= b.L) return false;               // stones are unavailable
        bool on = false;
        for (int i = 0; i < b.n; i++) on |= (b.line(l)[i] == c);
        if (!on) return false;                                // a cell may only serve a line through it
        got[l]++;                                             // one owner per cell by construction of owner[]
    }
    int live = 0;
    for (int l = 0; l < b.L; l++) {
        bool dead = false;
        for (int i = 0; i < b.n; i++) dead |= (bool)stone[b.line(l)[i]];
        if (dead) { if (got[l]) return false; continue; }
        live++;
        if (got[l] != 2) return false;                        // every live line exactly 2 cells
    }
    if (liveOut) *liveOut = live;
    if (unassignedOut) *unassignedOut = unassigned;
    return true;
}

// Pairing for an explicit stone set (used for the witnesses and for T7).
static bool pairing_for(const Board& b, const vector<int>& B, vector<int>& owner, vector<int>& hall) {
    vector<int> cnt(b.L, 0);
    vector<char> stone(b.N, 0);
    for (int c : B) {
        if (stone[c]) die("repeated stone");
        stone[c] = 1;
        for (int l : b.cl[c]) cnt[l]++;
    }
    Matcher M(b);
    bool ok = M.run(cnt.data(), stone.data());
    owner.assign(b.N, -1);
    hall.clear();
    if (ok) {
        for (int c = 0; c < b.N; c++) if (M.cellMatch[c] >= 0) owner[c] = M.cellMatch[c] >> 1;
    } else {
        hall = M.hall;
    }
    return ok;
}

// ---------------------------------------------------------------------------------------------- search

struct Search {
    const Board& b;
    Matcher M;
    int j = 0, need = 0;
    vector<int> cnt, m, chosen;
    vector<char> stone;
    vector<vector<int>> okbuf;
    uint64_t nodes = 0, tests = 0, countingPruned = 0, passCount = 0;
    bool aborted = false;
    double deadline = 0;
    // Options (defaults = the proof search).  The other settings exist for --selftest and for the
    // informational "maximum number of killed lines" value only.
    bool useSym = true;       // first stone restricted to orbit representatives
    bool useBound = true;     // marginal-sum bound
    bool countOnly = false;   // no matching: count (passCount) the sets that pass the counting test
    bool stopAtFirst = false; // countOnly: stop at the first set that passes
    bool matchAll = false;    // run the matching test even for sets that fail the counting test
    int needOverride = INT32_MIN;

    explicit Search(const Board& b_) : b(b_), M(b_) {}

    void add(int c, int& K) {
        stone[c] = 1;
        for (int l : b.cl[c])
            if (cnt[l]++ == 0) {
                K++;
                const int* ln = b.line(l);
                for (int i = 0; i < b.n; i++) m[ln[i]]--;
            }
    }
    void remove(int c) {
        stone[c] = 0;
        for (int l : b.cl[c])
            if (--cnt[l] == 0) {
                const int* ln = b.line(l);
                for (int i = 0; i < b.n; i++) m[ln[i]]++;
            }
    }

    // k stones chosen (the last one at position lastPos), K lines killed.  Returns true iff a set with a
    // pairing was found (then `chosen` holds it).
    bool dfs(int k, int lastPos, int K) {
        nodes++;
        if ((nodes & 0xFFFF) == 0 && deadline > 0 && cpu_now() > deadline) { aborted = true; return false; }
        int r = j - k;
        if (r == 0) {
            if (K < need && !matchAll) { countingPruned++; return false; }   // counting test
            if (countOnly) { passCount++; return stopAtFirst; }
            tests++;
            return M.run(cnt.data(), stone.data());
        }
        vector<int>& ok = okbuf[k];
        ok.clear();
        int hist[64];
        memset(hist, 0, sizeof(int) * (b.maxdeg + 1));
        int avail = 0;                                           // number of positions > p
        for (int p = b.N - 1; p > lastPos; p--) {
            int c = b.order[p];
            if (avail >= r - 1 && (k > 0 || !useSym || b.isRep[p])) {
                int rem = r - 1, s = 0;                          // sum of the r-1 largest m over positions > p
                for (int v = b.maxdeg; v >= 1 && rem > 0; v--) {
                    int t = min(rem, hist[v]);
                    s += t * v;
                    rem -= t;
                }
                if (!useBound || K + m[c] + s >= need) ok.push_back(p); else countingPruned++;
            }
            hist[m[c]]++;
            avail++;
        }
        for (int i = (int)ok.size() - 1; i >= 0; i--) {          // ascending positions
            int p = ok[i], c = b.order[p], K2 = K;
            add(c, K2);
            chosen.push_back(c);
            if (dfs(k + 1, p, K2)) return true;
            chosen.pop_back();
            remove(c);
            if (aborted) return false;
        }
        return false;
    }

    bool run(int j_, double deadline_) {
        j = j_;
        need = needOverride != INT32_MIN ? needOverride : b.L - (b.N - j) / 2;
        deadline = deadline_;
        cnt.assign(b.L, 0);
        stone.assign(b.N, 0);
        m.assign(b.N, 0);
        for (int c = 0; c < b.N; c++) m[c] = (int)b.cl[c].size();
        chosen.clear();
        okbuf.assign(j + 1, {});
        nodes = tests = countingPruned = passCount = 0;
        aborted = false;
        return dfs(0, -1, 0);
    }
};

// ------------------------------------------------------------------------------------------------ main

struct PerJ { int j, need; uint64_t nodes, tests; double sec; bool found, aborted; };

struct Result {
    int n = 0, lines = 0, linesFormula = 0, expected = -1;
    int minStones = -1;               // -1: not determined
    int provedLower = 0;              // all sizes < provedLower exhausted without a pairing
    bool lowerComplete = false, upperOk = false, pass = false;
    vector<int> witness;
    vector<int> owner;
    int live = 0, unassigned = 0;
    uint64_t lbNodes = 0, lbTests = 0, hallVerified = 0, wNodes = 0, wTests = 0;
    double lbSec = 0, ubSec = 0, totalSec = 0;
    long long groupOrder = 0;
    int nOrbits = 0;
    int maxKillsBelow = -1;           // max number of lines killed by min-1 stones (informational), -1 unknown
    int needBelow = 0;                // kills needed by the counting test with min-1 stones
    vector<PerJ> perJ;
};

static string coords(const Board& b, const vector<int>& cells) {
    string s = "[";
    for (size_t i = 0; i < cells.size(); i++) {
        auto a = b.xyz(cells[i]);
        char buf[64];
        snprintf(buf, sizeof buf, "%s[%d,%d,%d]", i ? "," : "", a[0], a[1], a[2]);
        s += buf;
    }
    return s + "]";
}

// ---------------------------------------------------------------------------------------------- selftest

// Plain enumeration, independent of Search: histogram of the number of killed lines over all j-subsets.
static void kill_histogram(const Board& b, int j, vector<uint64_t>& hist) {
    hist.assign(b.L + 1, 0);
    vector<int> idx(j), mark(b.L, 0);
    int stamp = 0;
    for (int i = 0; i < j; i++) idx[i] = i;
    if (j > b.N) return;
    while (true) {
        ++stamp;
        int K = 0;
        for (int i = 0; i < j; i++)
            for (int l : b.cl[idx[i]])
                if (mark[l] != stamp) { mark[l] = stamp; K++; }
        hist[K]++;
        int i = j - 1;
        while (i >= 0 && idx[i] == b.N - j + i) i--;
        if (i < 0) break;
        idx[i]++;
        for (int q = i + 1; q < j; q++) idx[q] = idx[q - 1] + 1;
    }
}

// Minimum number of stones with the given search settings (maxj = give up above this size, returns -1).
static int min_stones_with(const Board& b, bool sym, bool bound, bool matchAll, int maxj, uint64_t* tests) {
    Search S(b);
    S.useSym = sym;
    S.useBound = bound;
    S.matchAll = matchAll;
    if (tests) *tests = 0;
    for (int j = 0; j <= maxj; j++) {
        bool found = S.run(j, 0);
        if (tests) *tests += S.tests;
        if (found) return j;
    }
    return -1;
}

static bool selftest() {
    bool all = true;
    double t0 = cpu_now();
    // A. The marginal-sum bound never removes a set that passes a counting threshold T:
    //    #(j-sets killing >= T lines) by branch and bound == the same number from the plain histogram.
    //    Also: with the first-stone symmetry restriction a set is found iff that number is positive.
    struct Case { int n, jmax, depthT; };
    Case cases[] = {{3, 6, 1000}, {4, 4, 6}, {5, 4, 5}, {6, 3, 5}};
    for (const Case& cs : cases) {
        Board b(cs.n);
        uint64_t compared = 0, bad = 0;
        for (int j = 1; j <= cs.jmax; j++) {
            vector<uint64_t> hist;
            kill_histogram(b, j, hist);
            int maxK = 0;
            for (int K = 0; K <= b.L; K++) if (hist[K]) maxK = K;
            for (int T = maxK + 1; T >= max(1, maxK - cs.depthT); T--) {
                uint64_t expect = 0;
                for (int K = T; K <= b.L; K++) expect += hist[K];
                Search S(b);
                S.useSym = false; S.countOnly = true; S.needOverride = T;
                S.run(j, 0);
                Search S2(b);
                S2.countOnly = true; S2.stopAtFirst = true; S2.needOverride = T;
                bool found = S2.run(j, 0);
                compared++;
                if (S.passCount != expect || found != (expect > 0)) {
                    bad++;
                    printf("   MISMATCH n=%d j=%d T=%d: plain %llu, branch-and-bound %llu, symmetric search found=%d\n",
                           cs.n, j, T, (unsigned long long)expect, (unsigned long long)S.passCount, (int)found);
                }
            }
        }
        printf("selftest A n=%d (j=1..%d): %llu thresholds compared, %llu mismatches -> %s\n", cs.n, cs.jmax,
               (unsigned long long)compared, (unsigned long long)bad, bad ? "FAIL" : "ok");
        if (bad) all = false;
        fflush(stdout);
    }
    // B. Small boards: the minimum with all reductions == without symmetry == (n=3) brute force that runs
    //    the matching on every subset in order of size, with no counting test, no bound, no symmetry.
    for (int n : {2, 3, 4}) {
        Board b(n);
        uint64_t t1, t2, t3 = 0;
        int m1 = min_stones_with(b, true, true, false, b.N, &t1);
        int m2 = min_stones_with(b, false, true, false, b.N, &t2);
        int m3 = (n <= 3) ? min_stones_with(b, false, false, true, b.N, &t3) : m1;
        bool ok = (m1 == m2 && m1 == m3 && m1 >= 0);
        printf("selftest B n=%d (%d lines): min = %d (sym+bound, %llu matching tests), %d (bound only, %llu), %s%d%s -> %s\n",
               n, b.L, m1, (unsigned long long)t1, m2, (unsigned long long)t2,
               n <= 3 ? "" : "[brute force skipped] ", m3, n <= 3 ? " (brute force over all subsets)" : "", ok ? "ok" : "FAIL");
        if (n <= 3) printf("              brute force matching tests: %llu\n", (unsigned long long)t3);
        if (!ok) all = false;
        fflush(stdout);
    }
    // C. The regression sizes again without the symmetry restriction.
    for (int n : {5, 6, 7, 8}) {
        Board b(n);
        uint64_t t1, t2;
        int m1 = min_stones_with(b, true, true, false, b.N, &t1);
        int m2 = min_stones_with(b, false, true, false, b.N, &t2);
        bool ok = (m1 == m2 && m1 >= 0);
        printf("selftest C n=%d: min = %d with first-stone symmetry, %d without -> %s\n", n, m1, m2, ok ? "ok" : "FAIL");
        if (!ok) all = false;
        fflush(stdout);
    }
    printf("selftest: %s (%.1f cpu s)\n", all ? "PASS" : "FAIL", cpu_now() - t0);
    return all;
}

int main(int argc, char** argv) {
    vector<int> ns = {5, 6, 7, 8};
    string out = "results/regression/minstones.json";
    double timeLimit = 0;   // CPU seconds per n for the search, 0 = unlimited
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "--n" && i + 1 < argc) {
            ns.clear();
            string s = argv[++i];
            size_t pos = 0;
            while (pos < s.size()) {
                size_t q = s.find(',', pos);
                if (q == string::npos) q = s.size();
                ns.push_back(atoi(s.substr(pos, q - pos).c_str()));
                pos = q + 1;
            }
        } else if (a == "--out" && i + 1 < argc) {
            out = argv[++i];
        } else if (a == "--time-limit" && i + 1 < argc) {
            timeLimit = atof(argv[++i]);
        } else if (a == "--selftest") {
            return selftest() ? 0 : 1;
        } else {
            fprintf(stderr, "usage: %s [--n 5,6,7,8] [--out FILE.json] [--time-limit SEC_PER_N] | --selftest\n", argv[0]);
            return 2;
        }
    }
    map<int, int> expected = {{5, 9}, {6, 7}, {7, 3}, {8, 0}};
    vector<Result> results;
    string t7line;

    for (int n : ns) {
        if (n < 2 || n > 12) { fprintf(stderr, "n must be in 2..12\n"); return 2; }
        double t0 = cpu_now();
        Board b(n);
        Result R;
        R.n = n;
        R.lines = b.L;
        R.linesFormula = ((n + 2) * (n + 2) * (n + 2) - n * n * n) / 2;
        R.expected = expected.count(n) ? expected[n] : -1;
        R.groupOrder = b.groupOrder;
        R.nOrbits = b.nOrbits;
        printf("== n=%d: %d cells, %d lines (formula %d), max lines per cell %d, symmetry group order %lld "
               "(%zu verified generators), %d cell orbits\n",
               n, b.N, b.L, R.linesFormula, b.maxdeg, b.groupOrder, b.gens.size(), b.nOrbits);
        fflush(stdout);

        Search S(b);
        double deadline = timeLimit > 0 ? t0 + timeLimit : 0;
        for (int j = 0; j <= b.N; j++) {
            double tj = cpu_now();
            bool found = S.run(j, deadline);
            double sec = cpu_now() - tj;
            R.perJ.push_back({j, S.need, S.nodes, S.tests, sec, found, S.aborted});
            printf("   j=%d: need >= %d killed lines; nodes %llu, matching tests %llu, %.2f s -> %s\n", j, S.need,
                   (unsigned long long)S.nodes, (unsigned long long)S.tests, sec,
                   found ? "PAIRING FOUND" : (S.aborted ? "ABORTED (time limit)" : "exhausted, no set has a pairing"));
            fflush(stdout);
            if (found) {
                R.minStones = j;
                R.lowerComplete = true;
                R.provedLower = j;
                R.witness = S.chosen;
                R.wNodes = S.nodes;
                R.wTests = S.tests;
                R.ubSec = sec;
                break;
            }
            if (S.aborted) { R.provedLower = j; break; }
            R.lbNodes += S.nodes;
            R.lbTests += S.tests;
            R.lbSec += sec;
            R.provedLower = j + 1;
        }
        R.hallVerified = S.M.hallVerified;

        if (R.minStones >= 0) {
            // upper bound: recompute the pairing for the witness from scratch and verify it independently
            vector<int> hall;
            sort(R.witness.begin(), R.witness.end());
            bool ok = pairing_for(b, R.witness, R.owner, hall);
            R.upperOk = ok && (int)R.witness.size() == R.minStones &&
                        check_pairing(b, R.witness, R.owner, &R.live, &R.unassigned);
            if (R.minStones == 0) {
                printf("   LOWER: nothing to prove (minimum 0)\n");
            } else {
                // informational: the largest number of lines that min-1 stones can kill (same branch and
                // bound, counting only, threshold lowered until a set is found)
                int jb = R.minStones - 1;
                R.needBelow = b.L - (b.N - jb) / 2;
                Search C(b);
                C.countOnly = true;
                C.stopAtFirst = true;
                double dl = cpu_now() + 60;
                for (int T = R.needBelow - 1; T >= 0; T--) {
                    C.needOverride = T;
                    bool found = C.run(jb, dl);
                    if (C.aborted) break;
                    if (found) { R.maxKillsBelow = T; break; }
                }
                printf("   LOWER: no set of fewer than %d stones has a pairing (sizes 0..%d exhausted: %llu nodes, "
                       "%llu sets passed the counting test and were refuted by exact matching with a re-counted "
                       "Hall violator; %.2f s)\n",
                       R.minStones, jb, (unsigned long long)R.lbNodes, (unsigned long long)R.lbTests, R.lbSec);
                if (R.maxKillsBelow >= 0)
                    printf("          %d stones kill at most %d lines, the counting test needs %d\n", jb,
                           R.maxKillsBelow, R.needBelow);
            }
            printf("   UPPER: B = %s (%d stones), %d live lines, pairing %s, %d cells unassigned\n",
                   coords(b, R.witness).c_str(), R.minStones, R.live,
                   R.upperOk ? "found and verified" : "NOT VERIFIED", R.unassigned);
        } else {
            printf("   NOT DETERMINED within the time limit; proved: minimum >= %d\n", R.provedLower);
        }
        R.totalSec = cpu_now() - t0;
        R.pass = R.lines == R.linesFormula && R.lowerComplete && R.upperOk &&
                 (R.expected < 0 || R.minStones == R.expected);
        results.push_back(R);

        if (n == 6) {
            int T7[7][3] = {{0, 0, 0}, {5, 5, 5}, {1, 1, 4}, {4, 4, 1}, {2, 3, 2}, {3, 2, 3}, {5, 0, 0}};
            vector<int> B, owner, hall;
            for (auto& t : T7) B.push_back(b.cid(t[0], t[1], t[2]));
            bool ok = pairing_for(b, B, owner, hall);
            int live = 0, un = 0;
            bool ver = ok && check_pairing(b, B, owner, &live, &un);
            char buf[512];
            if (ok)
                snprintf(buf, sizeof buf, "n=6 T7 = %s: pairing exists: %s (verified: %s), live lines %d, slack %d, "
                         "unassigned cells %d (expected: yes, 1) -> %s",
                         coords(b, B).c_str(), "yes", ver ? "yes" : "NO", live, (b.N - 7) - 2 * live, un,
                         (ver && un == 1) ? "PASS" : "FAIL");
            else
                snprintf(buf, sizeof buf, "n=6 T7: NO pairing (Hall violator of %zu lines) -> FAIL", hall.size());
            t7line = buf;
            printf("   %s\n", buf);
        }
        fflush(stdout);
    }

    // ------------------------------------------------------------------------------------------ output
    std::error_code ec;
    auto parent = std::filesystem::path(out).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    FILE* f = fopen(out.c_str(), "w");
    if (!f) { fprintf(stderr, "cannot write %s\n", out.c_str()); return 2; }
    fprintf(f, "[\n");
    for (size_t i = 0; i < results.size(); i++) {
        const Result& R = results[i];
        Board b(R.n);
        fprintf(f, " {\"n\":%d,\"lines\":%d,", R.n, R.lines);
        if (R.minStones >= 0) fprintf(f, "\"min\":%d,", R.minStones); else fprintf(f, "\"min\":null,");
        fprintf(f, "\"witness\":%s,", coords(b, R.witness).c_str());
        fprintf(f, "\"lower_bound_nodes\":%llu,\"matching_tests\":%llu,\"seconds\":%.3f,\"pass\":%s,",
                (unsigned long long)R.lbNodes, (unsigned long long)R.lbTests, R.totalSec, R.pass ? "true" : "false");
        if (R.expected >= 0) fprintf(f, "\"expected\":%d,", R.expected); else fprintf(f, "\"expected\":null,");
        fprintf(f, "\"lower_bound_complete\":%s,\"proved_min_at_least\":%d,\"upper_bound_verified\":%s,",
                R.lowerComplete ? "true" : "false", R.provedLower, R.upperOk ? "true" : "false");
        fprintf(f, "\"lower_bound_seconds\":%.3f,\"witness_search_nodes\":%llu,\"witness_search_matching_tests\":%llu,"
                   "\"witness_search_seconds\":%.3f,",
                R.lbSec, (unsigned long long)R.wNodes, (unsigned long long)R.wTests, R.ubSec);
        if (R.maxKillsBelow >= 0)
            fprintf(f, "\"max_kills_with_min_minus_1_stones\":%d,\"kills_needed_with_min_minus_1_stones\":%d,",
                    R.maxKillsBelow, R.needBelow);
        fprintf(f, "\"hall_violators_verified\":%llu,\"live_lines\":%d,\"unassigned_cells\":%d,"
                   "\"group_order\":%lld,\"cell_orbits\":%d,\"per_size\":[",
                (unsigned long long)R.hallVerified, R.live, R.unassigned, R.groupOrder, R.nOrbits);
        for (size_t q = 0; q < R.perJ.size(); q++) {
            const PerJ& P = R.perJ[q];
            fprintf(f, "%s{\"j\":%d,\"need_kills\":%d,\"nodes\":%llu,\"matching_tests\":%llu,\"seconds\":%.3f,\"result\":\"%s\"}",
                    q ? "," : "", P.j, P.need, (unsigned long long)P.nodes, (unsigned long long)P.tests, P.sec,
                    P.found ? "pairing" : (P.aborted ? "aborted" : "exhausted"));
        }
        fprintf(f, "]}%s\n", i + 1 < results.size() ? "," : "");
    }
    fprintf(f, "]\n");
    fclose(f);

    string pout = out;
    if (pout.size() > 5 && pout.substr(pout.size() - 5) == ".json") pout = pout.substr(0, pout.size() - 5);
    pout += "_pairings.json";
    f = fopen(pout.c_str(), "w");
    if (f) {
        // cells as ids x*n*n + y*n + z; one entry per live line: its cells and the 2 cells assigned to it
        fprintf(f, "[\n");
        for (size_t i = 0; i < results.size(); i++) {
            const Result& R = results[i];
            Board b(R.n);
            fprintf(f, " {\"n\":%d,\"stones\":[", R.n);
            for (size_t q = 0; q < R.witness.size(); q++) fprintf(f, "%s%d", q ? "," : "", R.witness[q]);
            fprintf(f, "],\"pairing\":[");
            if (R.upperOk) {
                bool firstLine = true;
                for (int l = 0; l < b.L; l++) {
                    vector<int> mine;
                    for (int k = 0; k < b.n; k++) if (R.owner[b.line(l)[k]] == l) mine.push_back(b.line(l)[k]);
                    if (mine.empty()) continue;
                    fprintf(f, "%s{\"line\":[", firstLine ? "" : ",");
                    firstLine = false;
                    for (int k = 0; k < b.n; k++) fprintf(f, "%s%d", k ? "," : "", b.line(l)[k]);
                    fprintf(f, "],\"cells\":[%d,%d]}", mine[0], mine.size() > 1 ? mine[1] : -1);
                }
            }
            fprintf(f, "]}%s\n", i + 1 < results.size() ? "," : "");
        }
        fprintf(f, "]\n");
        fclose(f);
    }

    printf("\n  n  lines  expected  computed  lower-bound nodes  matching tests  cpu s   result\n");
    bool all = true;
    for (const Result& R : results) {
        char ex[16], mn[16];
        if (R.expected >= 0) snprintf(ex, sizeof ex, "%d", R.expected); else snprintf(ex, sizeof ex, "-");
        if (R.minStones >= 0) snprintf(mn, sizeof mn, "%d", R.minStones); else snprintf(mn, sizeof mn, ">=%d", R.provedLower);
        printf(" %2d  %5d  %8s  %8s  %17llu  %14llu  %6.2f   %s\n", R.n, R.lines, ex, mn,
               (unsigned long long)R.lbNodes, (unsigned long long)R.lbTests, R.totalSec, R.pass ? "PASS" : "FAIL");
        if (R.expected >= 0 && !R.pass) all = false;
    }
    if (!t7line.empty()) {
        printf("%s\n", t7line.c_str());
        if (t7line.find("PASS") == string::npos) all = false;
    }
    printf("wrote %s and %s\n", out.c_str(), pout.c_str());
    return all ? 0 : 1;
}

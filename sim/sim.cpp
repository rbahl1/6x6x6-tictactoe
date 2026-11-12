// sim/sim.cpp -- independent simulator for the 6x6x6 draw proof (task 7; audited and extended by the
// simulator review, see logs/review/sim_audit.md).
//
// Written ONLY from SPEC.md sections 1, 3, 5, 6 and 7.6.  It does not include, read or translate anything
// under src/ or verify/: own geometry, own symmetry group, own canonical form, own bipartite matching.
// It does not implement the abstract tree of SPEC 4 either: the adversaries only see real positions and use
// sigma itself (the loaded table) as an oracle.
//
// It plays complete games in which the second player (SP) follows the strategy sigma of SPEC section 5
// (strategy.tbl + leaves.txt) and the first player (FP) is an adversary.  A game in which
//   * FP owns a complete line,
//   * a table entry is missing (strategy hole),
//   * a leaf is reached for which the real position has no pairing,
//   * the real X is not B-admissible after a TABLE-mode move of SP,
//   * the table proposes a cell that is already in B or F (illegal table move),
//   * PAIRING mode: after SP's reply a live line has lost one of its two assigned cells or holds 5 FP stones,
//   * the simulator itself misbehaves (internal_error: occupied cell proposed by an adversary or by sigma,
//     bookkeeping invariant, invalid computed pairing, final position not a full board without an FP line)
// is a BUG: its full move list is written to the output and the exit code is 1.  The same holds for a bug
// that is only met in a look-ahead simulation of an adversary (recorded with its move list on the spot).
// Table and leaf entries are stored with their exact keys (t, B, F).
//
// Usage:
//   bin/sim --table F --leaves F --games N --seed S --adversary NAME --threads T --out FILE.json
//           [--script c1,c2,..] [--script-only] [--trace] [--p 0.1] [--search-depth 2] [--search-width 8]
//           [--tree-playouts 16] [--mix-weights r,g,s,t,x,d,dd,c,tr] [--verify-canonical] [--no-cache]
//           [--paranoid] [--batch 64] [--no-guidance] [--leaf-stress 4] [--max-bugs 20] [--bugs-out FILE.jsonl]
//           [--uncovered-out FILE] [--progress]
//   bin/sim --selftest [--seed S] [--tmpdir DIR] [--keep]
//   --script       forced first FP moves (a move on an occupied cell is skipped); with --script-only the game stops
//                  when the script is exhausted (replay / minimisation of a recorded bug: use its "fp_moves")
//   --trace        prints every round of game 0 to stderr
//   --paranoid     cross-checks the canonical form (naive loop over the 1152 maps), the line counters, admissibility
//                  (directly from the sets) and the matcher (plain Kuhn) at every step; about 10x slower
//   --batch        the coverage bitmaps that guide tree/danger/double/corner are updated after every batch of
//                  games; a game depends only on (seed, game index, batch size), never on --threads
//   --no-guidance  the adversaries do not see the coverage bitmaps
//   --uncovered-out FILE  the table / leaf entries that no real game met ("Q t B F m probed", "P t B F probed")
//   --leaf-stress K  at every leaf of a real game with t > |F|: C(B,F,t) is tested on K further admissible X (the
//                  unrevealed stones are replaced); a failure is a certificate bug (exit code 1); 0 switches it off
// Adversaries (all moves are checked for legality by play(); all reproducible from --seed, --batch and the game
// index, independent of --threads):
//   random  uniform empty cell
//   greedy  maximises the weighted count of own stones on live lines (ties random)
//   steal   antipode of SP's last move, else a random empty busy cell, else greedy
//   threat  takes an immediate win if there is one; otherwise prefers making 5 on a live line, creating a
//           danger pair, 3s/4s on intersecting live lines; with probability --p a random move.
//   search  clones the game and looks ahead --search-depth FP moves (sigma answers), candidates are the cells
//           that advance the cheapest LOST pattern (threat+danger or two dangers).
//   danger  two live lines with 4 stones each through an empty cell, the last stone creates the pair; lines with
//           few busy cells; 0..2 filler moves (mostly steals of the cell sigma is about to play)
//   double  a threat at round 5..7, then a second threat through a revealed stone; variants: the two lines
//           interleaved, threat then danger, threat - steals - threat
//   corner  takes busy cells (corners of a face, antipodes of SP's stones, the cells sigma is about to play) to
//           force occupied reveals and non-busy table moves, then threats / dangers; most of its scripted games
//           follow the "steal + threat" schedule (1..5 steals and one threat line in random order), which is
//           where most leaves of the proof tree are
//   tree    randomised best-first search over whole TABLE phases with sigma as oracle (--tree-playouts playouts
//           per plan, the best is mutated); the value rewards table/leaf keys not hit so far, a late leaf, forced
//           events, occupied reveals, non-busy table moves and near misses.  danger/double/corner use the same
//           search with their own bias in part of their games.
//   mix     one of the nine per game with probabilities --mix-weights
//           (default 0.03,0.03,0.04,0.05,0.02,0.09,0.09,0.15,0.50)
// Output (JSON): the SPEC 7.6 counters plus coverage (distinct table keys / leaf keys hit, by t and by t-|F|),
// histogram of t-|F| at the leaf, non-busy table moves, rounds with >= 2 occupied reveals, leaves reached after
// a threat / a danger / two threats, events by round, near misses, per-adversary counters and CPU time.
// Exit code: 0 = no bug; 1 = bug games, look-ahead bugs, leaf stress failures, paranoid failures, or anomalies in the files
// (conflicting duplicates, entries of impossible size, non-canonical entries with --verify-canonical);
// 2 = usage / file error; 3 = internal error (geometry, game count).

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <ctime>
#include <omp.h>
#include <unistd.h>

namespace {

typedef uint64_t u64;
constexpr int NC = 216, NL = 148, NB = 24, NG = 1152;

// ----------------------------------------------------------------------------------------------------
// cell sets
// ----------------------------------------------------------------------------------------------------
struct Bits {
    u64 w[4];
    bool test(int c) const { return (w[c >> 6] >> (c & 63)) & 1; }
    void set(int c) { w[c >> 6] |= 1ULL << (c & 63); }
    void reset(int c) { w[c >> 6] &= ~(1ULL << (c & 63)); }
    int count() const {
        return __builtin_popcountll(w[0]) + __builtin_popcountll(w[1]) + __builtin_popcountll(w[2]) +
               __builtin_popcountll(w[3]);
    }
    bool empty() const { return (w[0] | w[1] | w[2] | w[3]) == 0; }
    bool meets(const Bits& o) const {
        return ((w[0] & o.w[0]) | (w[1] & o.w[1]) | (w[2] & o.w[2]) | (w[3] & o.w[3])) != 0;
    }
    bool operator==(const Bits& o) const {
        return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3];
    }
    bool operator!=(const Bits& o) const { return !(*this == o); }
    Bits operator|(const Bits& o) const { return Bits{{w[0] | o.w[0], w[1] | o.w[1], w[2] | o.w[2], w[3] | o.w[3]}}; }
    Bits operator&(const Bits& o) const { return Bits{{w[0] & o.w[0], w[1] & o.w[1], w[2] & o.w[2], w[3] & o.w[3]}}; }
};
static const Bits EMPTYSET = {{0, 0, 0, 0}};

// SPEC 2.4 / 6: let p be the smallest cell whose membership differs; the set containing p is the smaller.
static inline int cmp_set(const Bits& a, const Bits& b) {
    for (int i = 0; i < 4; i++)
        if (a.w[i] != b.w[i]) {
            u64 d = a.w[i] ^ b.w[i];
            u64 low = d & (~d + 1);
            return (a.w[i] & low) ? -1 : 1;
        }
    return 0;
}
static inline int cells_of(const Bits& s, uint8_t* out) {
    int n = 0;
    for (int i = 0; i < 4; i++) {
        u64 x = s.w[i];
        while (x) {
            out[n++] = (uint8_t)(i * 64 + __builtin_ctzll(x));
            x &= x - 1;
        }
    }
    return n;
}
static std::string set_str(const Bits& s) {
    uint8_t c[NC];
    int n = cells_of(s, c);
    if (!n) return "-";
    std::string r;
    for (int i = 0; i < n; i++) {
        if (i) r += ',';
        r += std::to_string((int)c[i]);
    }
    return r;
}

// ----------------------------------------------------------------------------------------------------
// geometry (SPEC 1), built from scratch
// ----------------------------------------------------------------------------------------------------
static inline int cid(int x, int y, int z) { return 36 * x + 6 * y + z; }

static uint8_t LN[NL][6];   // cells of a line, ascending; lines sorted lexicographically
static Bits LB[NL];         // the same as a set
static int NLC[NC];         // number of lines through a cell
static uint8_t LC[NC][7];   // lines through a cell, ascending line index
static int BUSY[NB];        // busy cells, ascending
static bool ISBUSY[NC];
static uint8_t ANTI[NC];
static uint8_t GM[NG][NC];  // GM[g][c] = image of cell c under map g
static uint8_t GI[NG][NC];  // inverse map
alignas(64) static uint8_t GT[NC][NG];  // transposed: GT[c][g] = GM[g][c]
static int IOTA[NG];
static Bits FULLSET;
static int g_nlines = 0, g_nmaps = 0, g_nbusy = 0;
static int g_axis = 0, g_face = 0, g_space = 0;

static void build_geometry() {
    // lines: every coordinate is a constant (0..5), t (6) or 5-t (7); not all constant
    std::set<std::array<uint8_t, 6>> ls;
    std::map<std::array<uint8_t, 6>, int> kind;
    for (int f1 = 0; f1 < 8; f1++)
        for (int f2 = 0; f2 < 8; f2++)
            for (int f3 = 0; f3 < 8; f3++) {
                int moving = (f1 >= 6) + (f2 >= 6) + (f3 >= 6);
                if (!moving) continue;
                std::array<uint8_t, 6> a;
                for (int t = 0; t < 6; t++) {
                    int x = f1 < 6 ? f1 : (f1 == 6 ? t : 5 - t);
                    int y = f2 < 6 ? f2 : (f2 == 6 ? t : 5 - t);
                    int z = f3 < 6 ? f3 : (f3 == 6 ? t : 5 - t);
                    a[t] = (uint8_t)cid(x, y, z);
                }
                std::sort(a.begin(), a.end());
                ls.insert(a);
                kind[a] = moving;
            }
    g_nlines = (int)ls.size();
    if (g_nlines != NL) {
        fprintf(stderr, "geometry: %d lines instead of %d\n", g_nlines, NL);
        exit(3);
    }
    int li = 0;
    for (const auto& a : ls) {
        int k = kind[a];
        if (k == 1) g_axis++; else if (k == 2) g_face++; else g_space++;
        LB[li] = EMPTYSET;
        for (int i = 0; i < 6; i++) {
            LN[li][i] = a[i];
            LB[li].set(a[i]);
        }
        li++;
    }
    memset(NLC, 0, sizeof NLC);
    for (int l = 0; l < NL; l++)
        for (int i = 0; i < 6; i++) {
            int c = LN[l][i];
            if (NLC[c] >= 7) {
                fprintf(stderr, "geometry: cell %d on more than 7 lines\n", c);
                exit(3);
            }
            LC[c][NLC[c]++] = (uint8_t)l;
        }
    // busy cells and antipodes
    g_nbusy = 0;
    FULLSET = EMPTYSET;
    for (int x = 0; x < 6; x++)
        for (int y = 0; y < 6; y++)
            for (int z = 0; z < 6; z++) {
                int c = cid(x, y, z);
                FULLSET.set(c);
                ANTI[c] = (uint8_t)cid(5 - x, 5 - y, 5 - z);
                int a = std::min(x, 5 - x), b = std::min(y, 5 - y), d = std::min(z, 5 - z);
                ISBUSY[c] = (a == b && b == d);
                if (ISBUSY[c]) {
                    if (g_nbusy < NB) BUSY[g_nbusy] = c;
                    g_nbusy++;
                }
            }
    if (g_nbusy != NB) {
        fprintf(stderr, "geometry: %d busy cells\n", g_nbusy);
        exit(3);
    }
    // symmetry group: coordinate permutation, reversal of a subset, relabelling s with s(5-i) = 5-s(i)
    std::set<std::array<uint8_t, NC>> ms;
    int perm[3] = {0, 1, 2};
    std::vector<std::array<int, 3>> perms;
    do {
        perms.push_back({perm[0], perm[1], perm[2]});
    } while (std::next_permutation(perm, perm + 3));
    std::vector<std::array<int, 6>> relabels;
    for (const auto& pp : perms)
        for (int fl = 0; fl < 8; fl++) {
            std::array<int, 6> s;
            for (int i = 0; i < 3; i++) {
                int v = pp[i];
                if ((fl >> i) & 1) v = 5 - v;
                s[i] = v;
                s[5 - i] = 5 - v;
            }
            relabels.push_back(s);
        }
    for (const auto& p : perms)
        for (int rev = 0; rev < 8; rev++)
            for (const auto& s : relabels) {
                std::array<uint8_t, NC> m;
                for (int x = 0; x < 6; x++)
                    for (int y = 0; y < 6; y++)
                        for (int z = 0; z < 6; z++) {
                            int co[3] = {x, y, z};
                            int d[3];
                            for (int i = 0; i < 3; i++) {
                                d[i] = co[p[i]];
                                if ((rev >> i) & 1) d[i] = 5 - d[i];
                                d[i] = s[d[i]];
                            }
                            m[cid(x, y, z)] = (uint8_t)cid(d[0], d[1], d[2]);
                        }
                ms.insert(m);
            }
    g_nmaps = (int)ms.size();
    if (g_nmaps != NG) {
        fprintf(stderr, "geometry: %d maps instead of %d\n", g_nmaps, NG);
        exit(3);
    }
    int gi = 0;
    for (const auto& m : ms) {
        for (int c = 0; c < NC; c++) {
            GM[gi][c] = m[c];
            GI[gi][m[c]] = (uint8_t)c;
            GT[c][gi] = m[c];
        }
        IOTA[gi] = gi;
        gi++;
    }
    // checked at every start (not only in the self-test): two lines share at most one cell, busy cells are on 7
    // lines and all other cells on 3 or 4, every map is a bijection that sends lines to lines
    for (int a = 0; a < NL; a++)
        for (int b = a + 1; b < NL; b++)
            if ((LB[a] & LB[b]).count() > 1) {
                fprintf(stderr, "geometry: lines %d and %d share two cells\n", a, b);
                exit(3);
            }
    for (int c = 0; c < NC; c++)
        if (ISBUSY[c] ? NLC[c] != 7 : (NLC[c] != 3 && NLC[c] != 4)) {
            fprintf(stderr, "geometry: cell %d is on %d lines\n", c, NLC[c]);
            exit(3);
        }
    {
        std::set<std::array<uint8_t, 6>> lset;
        for (int l = 0; l < NL; l++) {
            std::array<uint8_t, 6> a;
            memcpy(a.data(), LN[l], 6);
            lset.insert(a);
        }
        for (int g = 0; g < NG; g++) {
            Bits im = EMPTYSET;
            for (int c = 0; c < NC; c++) im.set(GM[g][c]);
            bool ok = (im == FULLSET);
            for (int l = 0; l < NL && ok; l++) {
                std::array<uint8_t, 6> a;
                for (int i = 0; i < 6; i++) a[i] = GM[g][LN[l][i]];
                std::sort(a.begin(), a.end());
                ok = lset.count(a) > 0;
            }
            if (!ok) {
                fprintf(stderr, "geometry: map %d is not a line-preserving bijection\n", g);
                exit(3);
            }
        }
    }
}

static inline Bits image_cells(int g, const uint8_t* cells, int n) {
    Bits r = EMPTYSET;
    const uint8_t* m = GM[g];
    for (int i = 0; i < n; i++) r.set(m[cells[i]]);
    return r;
}
static Bits image_set(int g, const Bits& s) {
    uint8_t c[NC];
    int n = cells_of(s, c);
    return image_cells(g, c, n);
}

// ----------------------------------------------------------------------------------------------------
// canonical form (SPEC 6): smallest (g(B), g(F)) over the 1152 maps, B-images first, F-images second
// ----------------------------------------------------------------------------------------------------
struct CanonRes {
    Bits B, F;
    int g;
};

// keeps in cand[0..nc) the maps whose image of the cell list is the smallest; best = that image
static void refine(int* cand, int& nc, const uint8_t* cells, int n, Bits& best) {
    best = EMPTYSET;
    if (n == 0) return;
    if (nc == NG) {
        // the smallest image contains the smallest reachable cell: filter by the minimum image cell first
        alignas(64) uint8_t mn[NG];
        memset(mn, 255, sizeof mn);
        for (int i = 0; i < n; i++) {
            const uint8_t* row = GT[cells[i]];
#pragma omp simd
            for (int g = 0; g < NG; g++) mn[g] = row[g] < mn[g] ? row[g] : mn[g];
        }
        uint8_t m = 255;
        for (int g = 0; g < NG; g++) m = mn[g] < m ? mn[g] : m;
        nc = 0;
        for (int g = 0; g < NG; g++)
            if (mn[g] == m) cand[nc++] = g;
    }
    int k = 0;
    for (int i = 0; i < nc; i++) {
        int g = cand[i];
        Bits img = image_cells(g, cells, n);
        if (k == 0) {
            best = img;
            cand[k++] = g;
        } else {
            int c = cmp_set(img, best);
            if (c < 0) {
                best = img;
                k = 0;
                cand[k++] = g;
            } else if (c == 0)
                cand[k++] = g;
        }
    }
    nc = k;
}

static CanonRes canon(const Bits& B, const Bits& F) {
    int cand[NG];
    memcpy(cand, IOTA, sizeof cand);
    int nc = NG;
    uint8_t cells[NC];
    CanonRes r;
    int n = cells_of(B, cells);
    refine(cand, nc, cells, n, r.B);
    n = cells_of(F, cells);
    refine(cand, nc, cells, n, r.F);
    r.g = cand[0];
    return r;
}

// reference implementation for the self-test: plain loop over all maps
static CanonRes canon_naive(const Bits& B, const Bits& F) {
    CanonRes r;
    r.g = -1;
    for (int g = 0; g < NG; g++) {
        Bits b = image_set(g, B), f = image_set(g, F);
        if (r.g < 0) {
            r.B = b; r.F = f; r.g = g;
            continue;
        }
        int c = cmp_set(b, r.B);
        if (c == 0) c = cmp_set(f, r.F);
        if (c < 0) {
            r.B = b; r.F = f; r.g = g;
        }
    }
    return r;
}

// ----------------------------------------------------------------------------------------------------
// 128-bit keyed hash table for the strategy table and the leaf file
// ----------------------------------------------------------------------------------------------------
static inline u64 mixa(u64 x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}
static inline u64 mixb(u64 x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}
static inline void key128(int t, const Bits& B, const Bits& F, u64& h1, u64& h2) {
    u64 a = 0x243f6a8885a308d3ULL ^ (u64)(t + 1), b = 0x13198a2e03707344ULL + (u64)(t + 1) * 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 4; i++) {
        a = mixa(a ^ B.w[i]);
        b = mixb((b << 7 | b >> 57) + B.w[i] + 0x632be59bd9b4e019ULL * (u64)(i + 1));
    }
    for (int i = 0; i < 4; i++) {
        a = mixa((a + 0x9e3779b97f4a7c15ULL) ^ F.w[i]);
        b = mixb((b << 11 | b >> 53) + F.w[i] + 0xa0761d6478bd642fULL * (u64)(i + 5));
    }
    h1 = a;
    h2 = b;
    if (h1 == 0 && h2 == 0) h2 = 1;
}

// Exact store of the entries of a file (strategy.tbl or leaves.txt): the entries are kept with their full
// key (t, B, F) and are compared exactly; the hash only selects the probe sequence.  (The original version
// compared 128-bit hashes only.)  The index of an entry (file order, duplicates removed) is used for the
// coverage statistics.
struct Entry {
    Bits B, F;
    int16_t t;
    uint8_t m;
};
struct Store {
    std::vector<Entry> ent;
    std::vector<uint32_t> slot;   // entry index + 1, 0 = free
    u64 mask = 0;
    size_t n = 0;
    long conflicts = 0, duplicates = 0;
    void init(size_t expected) {
        size_t cap = 16;
        while (cap < 2 * expected + 16) cap <<= 1;
        slot.assign(cap, 0);
        ent.clear();
        ent.reserve(expected);
        mask = cap - 1;
        n = 0;
        conflicts = duplicates = 0;
    }
    static u64 hash(int t, const Bits& B, const Bits& F) {
        u64 h1, h2;
        key128(t, B, F, h1, h2);
        return h1 ^ (h2 >> 1);
    }
    void insert(int t, const Bits& B, const Bits& F, uint8_t v) {
        u64 i = hash(t, B, F) & mask;
        for (;;) {
            uint32_t s = slot[i];
            if (!s) {
                ent.push_back(Entry{B, F, (int16_t)t, v});
                slot[i] = (uint32_t)ent.size();
                n = ent.size();
                return;
            }
            const Entry& e = ent[s - 1];
            if (e.t == t && e.B == B && e.F == F) {
                duplicates++;
                if (e.m != v) conflicts++;
                return;
            }
            i = (i + 1) & mask;
        }
    }
    // index of the entry with exactly this key, or -1
    int find(int t, const Bits& B, const Bits& F) const {
        if (slot.empty()) return -1;
        u64 i = hash(t, B, F) & mask;
        for (;;) {
            uint32_t s = slot[i];
            if (!s) return -1;
            const Entry& e = ent[s - 1];
            if (e.t == t && e.B == B && e.F == F) return (int)(s - 1);
            i = (i + 1) & mask;
        }
    }
};

struct LoadInfo {
    long lines = 0, noncanonical = 0;
    long badShape = 0;   // entries whose sizes do not fit a state: Q needs |B| = t-1, P needs |B| = t; both |F| <= t
};

static bool parse_set_tok(const char* p, const char* e, Bits& out) {
    out = EMPTYSET;
    if (e - p == 1 && *p == '-') return true;
    if (p == e) return false;
    while (p < e) {
        if (*p < '0' || *p > '9') return false;
        int v = 0;
        while (p < e && *p >= '0' && *p <= '9') {
            v = v * 10 + (*p - '0');
            if (v >= NC) return false;
            p++;
        }
        if (out.test(v)) return false;
        out.set(v);
        if (p < e) {
            if (*p != ',') return false;
            p++;
            if (p == e) return false;
        }
    }
    return true;
}

// loads "t B F m" (withMove) or "t B F" lines; exits with code 2 on a malformed file
static LoadInfo load_file(const std::string& path, bool withMove, Store& map, bool verifyCanon) {
    LoadInfo info;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path.c_str());
        exit(2);
    }
    std::string buf;
    {
        char tmp[1 << 16];
        size_t r;
        while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) buf.append(tmp, r);
    }
    fclose(f);
    size_t nl = 0;
    for (char ch : buf) nl += (ch == '\n');
    map.init(nl + 1);
    struct Ent {
        Bits B, F;
        int t, m;
    };
    std::vector<Ent> batch;
    auto flush = [&]() {
        if (verifyCanon) {
            long bad = 0;
            long nb = (long)batch.size();
#pragma omp parallel for schedule(static) reduction(+ : bad)
            for (long i = 0; i < nb; i++) {
                CanonRes c = canon(batch[i].B, batch[i].F);
                if (c.B != batch[i].B || c.F != batch[i].F) bad++;
            }
            info.noncanonical += bad;
        }
        for (const Ent& e : batch) {
            if (e.B.count() != (withMove ? e.t - 1 : e.t) || e.F.count() > e.t) info.badShape++;
            map.insert(e.t, e.B, e.F, (uint8_t)e.m);
        }
        batch.clear();
    };
    const char* p = buf.data();
    const char* end = p + buf.size();
    long lineno = 0;
    while (p < end) {
        const char* le = (const char*)memchr(p, '\n', end - p);
        if (!le) le = end;
        lineno++;
        const char* q = p;
        const char* qe = le;
        while (qe > q && (qe[-1] == '\r' || qe[-1] == ' ' || qe[-1] == '\t')) qe--;
        while (q < qe && (*q == ' ' || *q == '\t')) q++;
        if (q < qe && *q != '#') {
            const char* tok[5];
            const char* tend[5];
            int nt = 0;
            const char* s = q;
            while (s < qe && nt < 5) {
                while (s < qe && (*s == ' ' || *s == '\t')) s++;
                if (s >= qe) break;
                tok[nt] = s;
                while (s < qe && *s != ' ' && *s != '\t') s++;
                tend[nt] = s;
                nt++;
            }
            bool ok = (nt == (withMove ? 4 : 3));
            Ent e;
            e.t = 0;
            e.m = 0;
            if (ok) {
                const char* a = tok[0];
                if (a == tend[0]) ok = false;
                for (; a < tend[0]; a++) {
                    if (*a < '0' || *a > '9') { ok = false; break; }
                    e.t = e.t * 10 + (*a - '0');
                    if (e.t > 1000) { ok = false; break; }
                }
            }
            if (ok) ok = parse_set_tok(tok[1], tend[1], e.B) && parse_set_tok(tok[2], tend[2], e.F);
            if (ok && e.B.meets(e.F)) ok = false;
            if (ok && withMove) {
                const char* a = tok[3];
                if (a == tend[3]) ok = false;
                for (; a < tend[3]; a++) {
                    if (*a < '0' || *a > '9') { ok = false; break; }
                    e.m = e.m * 10 + (*a - '0');
                    if (e.m >= NC) { ok = false; break; }
                }
            }
            if (!ok) {
                fprintf(stderr, "%s:%ld: malformed line\n", path.c_str(), lineno);
                exit(2);
            }
            batch.push_back(e);
            info.lines++;
            if (batch.size() >= 65536) flush();
        }
        p = le + 1;
    }
    flush();
    return info;
}

// ----------------------------------------------------------------------------------------------------
// pairing (SPEC 3): every live line gets 2 cells of l \ X, no cell twice.  Kuhn augmenting paths.
// ----------------------------------------------------------------------------------------------------
struct Matcher {
    int16_t cellSlot[NC];   // slot (2*liveIndex + j) holding the cell, or -1
    int seen[NC];
    int stamp;
    const Bits* X;
    int16_t slotLine[2 * NL];
    bool try_slot(int s) {
        int l = slotLine[s];
        for (int i = 0; i < 6; i++) {   // a free cell first
            int c = LN[l][i];
            if (cellSlot[c] < 0 && !X->test(c)) {
                cellSlot[c] = (int16_t)s;
                return true;
            }
        }
        for (int i = 0; i < 6; i++) {
            int c = LN[l][i];
            if (X->test(c) || seen[c] == stamp) continue;
            seen[c] = stamp;
            if (try_slot(cellSlot[c])) {
                cellSlot[c] = (int16_t)s;
                return true;
            }
        }
        return false;
    }
};

// partner[c] = the other cell assigned to the same line (or -1).  Returns false iff no pairing exists.
// Maximum bipartite matching between the 2 slots of every live line and the cells of l \ X: a greedy start
// (cells on few live lines first) followed by Kuhn's augmenting paths for the slots still unmatched.  A slot
// for which no augmenting path exists stays unmatched in every maximum matching, so the answer is exact.
static bool find_pairing(const Bits& B, const Bits& X, int16_t* partner) {
    Matcher M;
    M.X = &X;
    M.stamp = 0;
    uint8_t deg[NC];
    for (int c = 0; c < NC; c++) {
        M.cellSlot[c] = -1;
        M.seen[c] = 0;
        partner[c] = -1;
        deg[c] = 0;
    }
    int ns = 0;
    for (int l = 0; l < NL; l++)
        if (!LB[l].meets(B)) {
            M.slotLine[ns++] = (int16_t)l;
            M.slotLine[ns++] = (int16_t)l;
            for (int i = 0; i < 6; i++) deg[LN[l][i]]++;
        }
    if (NC - B.count() - X.count() < ns) return false;   // fewer free cells than slots
    int16_t todo[2 * NL];
    int nt = 0;
    for (int s = 0; s < ns; s++) {
        int l = M.slotLine[s], bc = -1, bd = 99;
        for (int i = 0; i < 6; i++) {
            int c = LN[l][i];
            if (M.cellSlot[c] < 0 && !X.test(c) && deg[c] < bd) {
                bd = deg[c];
                bc = c;
            }
        }
        if (bc >= 0) M.cellSlot[bc] = (int16_t)s;
        else todo[nt++] = (int16_t)s;
    }
    for (int i = 0; i < nt; i++) {
        M.stamp++;
        if (!M.try_slot(todo[i])) return false;
    }
    int16_t first[2 * NL];
    for (int s = 0; s < ns; s++) first[s] = -1;
    for (int c = 0; c < NC; c++) {
        int s = M.cellSlot[c];
        if (s < 0) continue;
        int base = s & ~1;   // both slots of a line
        if (first[base] < 0)
            first[base] = (int16_t)c;
        else {
            partner[c] = first[base];
            partner[first[base]] = (int16_t)c;
        }
    }
    return true;
}

// reference matcher for the self-test: plain Kuhn without any heuristics, returns only existence
static bool find_pairing_ref(const Bits& B, const Bits& X) {
    int owner[NC], seen[NC], stamp = 0;
    for (int c = 0; c < NC; c++) owner[c] = -1, seen[c] = 0;
    std::vector<int> sl;
    for (int l = 0; l < NL; l++)
        if (!LB[l].meets(B)) { sl.push_back(l); sl.push_back(l); }
    struct R {
        static bool go(int s, const std::vector<int>& sl, const Bits& X, int* owner, int* seen, int stamp) {
            int l = sl[s];
            for (int i = 0; i < 6; i++) {
                int c = LN[l][i];
                if (X.test(c) || seen[c] == stamp) continue;
                seen[c] = stamp;
                if (owner[c] < 0 || go(owner[c], sl, X, owner, seen, stamp)) {
                    owner[c] = s;
                    return true;
                }
            }
            return false;
        }
    };
    for (int s = 0; s < (int)sl.size(); s++) {
        stamp++;
        if (!R::go(s, sl, X, owner, seen, stamp)) return false;
    }
    return true;
}

// ----------------------------------------------------------------------------------------------------
// game state and the strategy sigma (SPEC 5)
// ----------------------------------------------------------------------------------------------------
// Bug classes (every one makes the exit code non-zero):
//   fp_line                  FP owns a complete line
//   hole                     no table entry for the canonical Q-state
//   pairing_failure          a leaf was reached and the real position (B,X) has no pairing
//   admissibility_failure    X is not B-admissible after a TABLE-mode move of SP
//   illegal_table_move       the table proposes a cell of B or F
//   pairing_invariant_failure  PAIRING mode: after SP's reply a live line has lost one of its two assigned cells
//                            to FP, or a live line holds 5 FP stones (FP wins with his next move).  This is the
//                            invariant of the pairing strategy; checking it makes the detection of a PAIRING-mode
//                            loss independent of the adversary's skill.
//   internal_error           the simulator itself misbehaved: an adversary proposed an occupied cell, sigma
//                            proposed an occupied cell, a bookkeeping invariant (|B| = |X| = t, F subset of X)
//                            failed, the computed pairing is not a valid pairing, the final position is not a
//                            full board, or a --paranoid cross-check disagreed.
enum Status { ST_OK = 0, ST_END, ST_FP_LINE, ST_HOLE, ST_PAIRING_FAIL, ST_ADMISS_FAIL, ST_ILLEGAL, ST_PAIR_BROKEN, ST_INTERNAL };
static inline bool is_bug(Status s) { return s >= ST_FP_LINE; }
static const char* status_name(Status s) {
    switch (s) {
        case ST_OK: return "ok";
        case ST_END: return "end";
        case ST_FP_LINE: return "fp_line";
        case ST_HOLE: return "hole";
        case ST_PAIRING_FAIL: return "pairing_failure";
        case ST_ADMISS_FAIL: return "admissibility_failure";
        case ST_ILLEGAL: return "illegal_table_move";
        case ST_PAIR_BROKEN: return "pairing_invariant_failure";
        case ST_INTERNAL: return "internal_error";
    }
    return "?";
}
// reasons of ST_INTERNAL
enum {
    IE_NONE = 0, IE_PLAY_AFTER_END, IE_FP_OCCUPIED, IE_NO_EMPTY, IE_SP_OCCUPIED, IE_BOOKKEEPING, IE_PARANOID,
    IE_PAIRING_INVALID, IE_FINAL_POSITION
};
static const char* internal_name(int r) {
    static const char* nm[] = {"none", "play_after_end", "fp_move_on_occupied_cell", "no_empty_cell_for_sp",
                               "sp_move_on_occupied_cell", "bookkeeping_invariant", "paranoid_cross_check",
                               "computed_pairing_invalid", "final_position"};
    return (r >= 0 && r <= IE_FINAL_POSITION) ? nm[r] : "?";
}

constexpr int MAXEV = 6;
struct Game {
    Bits X, B, F;          // FP stones, SP stones, revealed FP stones (F is frozen once in PAIRING mode)
    uint8_t xc[NL], bc[NL];
    int16_t partner[NC];
    uint8_t moves[NC];     // all stones in the order played (FP, SP, FP, SP, ...)
    int nm;                // number of recorded moves
    int ns;                // stones on the board
    int t;                 // round counter
    int lastSP;
    int leafRound;         // round at which PAIRING mode started, -1 while in TABLE mode
    bool pairing;
    Status st;
    // ---- instrumentation (does not influence sigma)
    int leafIdx;           // index of the leaf entry, -1 none, -2 benchmark leaf
    int leafF;             // |F| at the leaf
    int internal;          // reason of ST_INTERNAL
    uint16_t nOcc, nTable; // occupied reveals, table moves played
    uint16_t novelQ, novelPQ;   // table keys met that are not in the coverage snapshot (real / incl. look-ahead)
    uint8_t leafNovel, leafNovelP;
    uint8_t nThreat, nDanger;   // forced events so far
    uint8_t nMultiOcc;     // rounds with >= 2 occupied reveals
    uint8_t maxOcc;        // largest number of occupied reveals in one round
    uint8_t nNonBusy;      // table moves played on non-busy cells
    uint8_t lastEvent;     // last round: 0 table move, 1 threat, 2 danger, 3 PAIRING mode
    uint8_t lastOcc;       // occupied reveals in the last round
    uint8_t minEmpty;      // min over rounds (after SP's reply) of min over live lines of the cells FP still needs
    uint8_t near4;         // TABLE-mode rounds after which some live line holds 4 FP stones
    uint8_t nEv, evKind[MAXEV], evRound[MAXEV];   // the forced events in order (kind 1 threat, 2 danger)
};

struct Stats {
    long games = 0, fp_lines = 0, holes = 0, pairing_fail = 0, adm_fail = 0, illegal = 0, pair_broken = 0, internal = 0, ended = 0;
    long threat = 0, danger = 0, occupied = 0, rounds = 0, table_moves = 0, noLeaf = 0;
    long nonbusy_moves = 0, nonbusy_games = 0, multi_occ_rounds = 0, multi_occ_games = 0, near4_rounds = 0;
    long leaf_after_threat = 0, leaf_after_danger = 0, leaf_after_two_threats = 0, leaf_after_threat_and_danger = 0;
    long leafHist[NC + 2];
    long uHist[NC + 2];        // t - |F| at the leaf
    long evHist[2][16];        // threat / danger events by round (rounds >= 15 in the last bin)
    long sig[4][3];            // leaves by (number of threats capped at 3, number of dangers capped at 2)
    long maxOccHist[8];        // games by the largest number of occupied reveals in one round (capped at 7)
    long minEmptyHist[8];      // games by minEmpty
    double cpu = 0;
    int maxTableRound = 0;
    Stats() {
        memset(leafHist, 0, sizeof leafHist);
        memset(uHist, 0, sizeof uHist);
        memset(evHist, 0, sizeof evHist);
        memset(sig, 0, sizeof sig);
        memset(maxOccHist, 0, sizeof maxOccHist);
        memset(minEmptyHist, 0, sizeof minEmptyHist);
    }
    void add(const Stats& o) {
        games += o.games; fp_lines += o.fp_lines; holes += o.holes; pairing_fail += o.pairing_fail;
        adm_fail += o.adm_fail; illegal += o.illegal; pair_broken += o.pair_broken; internal += o.internal;
        ended += o.ended; threat += o.threat; danger += o.danger;
        occupied += o.occupied; rounds += o.rounds; table_moves += o.table_moves; noLeaf += o.noLeaf;
        nonbusy_moves += o.nonbusy_moves; nonbusy_games += o.nonbusy_games; multi_occ_rounds += o.multi_occ_rounds;
        multi_occ_games += o.multi_occ_games; near4_rounds += o.near4_rounds;
        leaf_after_threat += o.leaf_after_threat; leaf_after_danger += o.leaf_after_danger;
        leaf_after_two_threats += o.leaf_after_two_threats; leaf_after_threat_and_danger += o.leaf_after_threat_and_danger;
        for (int i = 0; i < NC + 2; i++) { leafHist[i] += o.leafHist[i]; uHist[i] += o.uHist[i]; }
        for (int i = 0; i < 16; i++) { evHist[0][i] += o.evHist[0][i]; evHist[1][i] += o.evHist[1][i]; }
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++) sig[i][j] += o.sig[i][j];
        for (int i = 0; i < 8; i++) { maxOccHist[i] += o.maxOccHist[i]; minEmptyHist[i] += o.minEmptyHist[i]; }
        cpu += o.cpu;
        maxTableRound = std::max(maxTableRound, o.maxTableRound);
    }
    long bugs() const { return fp_lines + holes + pairing_fail + adm_fail + illegal + pair_broken + internal; }
    // per-game instrumentation, taken from the finished game
    void absorb(const Game& g) {
        games++;
        rounds += g.t;
        nonbusy_moves += g.nNonBusy;
        nonbusy_games += (g.nNonBusy > 0);
        multi_occ_rounds += g.nMultiOcc;
        multi_occ_games += (g.nMultiOcc > 0);
        near4_rounds += g.near4;
        maxOccHist[std::min<int>(g.maxOcc, 7)]++;
        minEmptyHist[std::min<int>(g.minEmpty, 7)]++;
        for (int i = 0; i < g.nEv; i++) evHist[g.evKind[i] - 1][std::min<int>(g.evRound[i], 15)]++;
        if (g.leafRound >= 0) {
            leafHist[g.leafRound]++;
            uHist[g.leafRound - g.leafF]++;
            sig[std::min<int>(g.nThreat, 3)][std::min<int>(g.nDanger, 2)]++;
            leaf_after_threat += (g.nThreat >= 1);
            leaf_after_danger += (g.nDanger >= 1);
            leaf_after_two_threats += (g.nThreat >= 2);
            leaf_after_threat_and_danger += (g.nThreat >= 1 && g.nDanger >= 1);
        } else
            noLeaf++;
        switch (g.st) {
            case ST_FP_LINE: fp_lines++; break;
            case ST_HOLE: holes++; break;
            case ST_PAIRING_FAIL: pairing_fail++; break;
            case ST_ADMISS_FAIL: adm_fail++; break;
            case ST_ILLEGAL: illegal++; break;
            case ST_PAIR_BROKEN: pair_broken++; break;
            case ST_INTERNAL: internal++; break;
            case ST_END: ended++; break;
            default: internal++; break;   // a game must not stop in state "ok"
        }
    }
};

struct CacheEnt {
    Bits B, F;
    int32_t t;
    int32_t idx;
    int16_t val;
    uint8_t kind, valid;
};
constexpr int CACHE_BITS = 14;

struct Ctx {
    const Store* table = nullptr;
    const Store* leaves = nullptr;
    bool useCache = true;
    int fakeLeafRound = -1;   // >= 0: benchmark-only total policy (see lookup_q), never used in real runs
    bool paranoid = false;    // cross-check canon, the line counters, admissibility and the matcher by brute force
    bool checkCounts = true;  // |B| = |X| = t after SP's move (switched off only for hand-made self-test positions)
    int leafStress = 0;       // > 0: at every leaf of a real game with t > |F|, test C(B,F,t) on that many other X
    // coverage snapshots (frozen while a batch of games runs; nullptr = no coverage guidance):
    const uint8_t* covQ = nullptr;   // table keys hit in real games
    const uint8_t* covL = nullptr;   // leaf keys hit in real games
    const uint8_t* prbQ = nullptr;   // table keys touched in real games or look-ahead simulations
    const uint8_t* prbL = nullptr;
};
static std::string bug_json(long idx, u64 gseed, const char* advName, const Game& g);
static long g_paranoidFail = 0;
struct TS {   // per-thread scratch
    std::vector<CacheEnt> cache;
    long canonCalls = 0, cacheHits = 0;
    // coverage: bit 0 = hit in a real game, bit 1 = touched in a look-ahead simulation (all TABLE-mode detectors
    // and the pairing computation at the leaf run there as well)
    std::vector<uint8_t> seenQ, seenL;
    std::vector<uint32_t> newRealQ, newRealL, newProbeQ, newProbeL;   // first hits of this thread since the last merge
    // bugs met in look-ahead simulations.  The adversaries steer into them, but a steered-into bug can be missed
    // (the candidate set of the next decision is random), so they are recorded on the spot with their move list.
    long laBugs = 0;
    std::vector<std::string> laBugJson;
    // leaf stress test (see leaf_stress)
    long stressTests = 0, stressFail = 0;
    std::vector<std::string> stressJson;
    static void mark(std::vector<uint8_t>& seen, std::vector<uint32_t>& nr, std::vector<uint32_t>& np, int idx, bool real) {
        if (idx < 0) return;
        if ((size_t)idx >= seen.size()) seen.resize(std::max<size_t>(2 * seen.size(), (size_t)idx + 1024), 0);
        uint8_t bit = real ? 1 : 2;
        if (seen[idx] & bit) return;
        seen[idx] |= bit;
        (real ? nr : np).push_back((uint32_t)idx);
    }
    void mark_q(int idx, bool real) { mark(seenQ, newRealQ, newProbeQ, idx, real); }
    void mark_l(int idx, bool real) { mark(seenL, newRealL, newProbeL, idx, real); }
};

// real-frame table move for Q(B,F,t), or -1 (hole); *idxOut = index of the table entry (-1 none)
static int lookup_q(const Ctx& ctx, TS& ts, const Bits& B, const Bits& F, int t, int* idxOut = nullptr) {
    CacheEnt* ce = nullptr;
    if (ctx.useCache) {
        if (ts.cache.empty()) ts.cache.assign((size_t)1 << CACHE_BITS, CacheEnt{EMPTYSET, EMPTYSET, 0, 0, 0, 0, 0});
        u64 h = mixa(B.w[0] ^ mixa(B.w[1] ^ mixa(B.w[2] ^ mixa(B.w[3] + (u64)t))));
        h ^= mixb(F.w[0] ^ mixb(F.w[1] ^ mixb(F.w[2] ^ mixb(F.w[3] + 1))));
        ce = &ts.cache[h & (((u64)1 << CACHE_BITS) - 1)];
        if (ce->valid && ce->kind == 1 && ce->t == t && ce->B == B && ce->F == F) {
            ts.cacheHits++;
            if (idxOut) *idxOut = ce->idx;
            return ce->val;
        }
    }
    CanonRes c = canon(B, F);
    ts.canonCalls++;
    if (ctx.paranoid) {
        CanonRes n = canon_naive(B, F);
        if (n.B != c.B || n.F != c.F || image_set(c.g, B) != c.B || image_set(c.g, F) != c.F) {
#pragma omp atomic
            g_paranoidFail++;
        }
    }
    int idx = ctx.table->find(t, c.B, c.F);
    int m = idx >= 0 ? (int)ctx.table->ent[idx].m : -1;
    if (m < 0 && ctx.fakeLeafRound >= 0) {
        // benchmark only: smallest busy cell outside B and F in the canonical frame, else the smallest cell
        for (int i = 0; i < NB && m < 0; i++)
            if (!c.B.test(BUSY[i]) && !c.F.test(BUSY[i])) m = BUSY[i];
        for (int q = 0; q < NC && m < 0; q++)
            if (!c.B.test(q) && !c.F.test(q)) m = q;
    }
    int r = m < 0 ? -1 : GI[c.g][m];
    if (ce) *ce = CacheEnt{B, F, t, idx, (int16_t)r, 1, 1};
    if (idxOut) *idxOut = idx;
    return r;
}
// index of the leaf entry for P(B,F,t); -1 = not a leaf; -2 = benchmark-only leaf
static int lookup_leaf(const Ctx& ctx, TS& ts, const Bits& B, const Bits& F, int t) {
    CacheEnt* ce = nullptr;
    if (ctx.useCache) {
        if (ts.cache.empty()) ts.cache.assign((size_t)1 << CACHE_BITS, CacheEnt{EMPTYSET, EMPTYSET, 0, 0, 0, 0, 0});
        u64 h = mixa(B.w[0] ^ mixa(B.w[1] ^ mixa(B.w[2] ^ mixa(B.w[3] + (u64)t + 77777))));
        h ^= mixb(F.w[0] ^ mixb(F.w[1] ^ mixb(F.w[2] ^ mixb(F.w[3] + 2))));
        ce = &ts.cache[h & (((u64)1 << CACHE_BITS) - 1)];
        if (ce->valid && ce->kind == 2 && ce->t == t && ce->B == B && ce->F == F) {
            ts.cacheHits++;
            return ce->idx;
        }
    }
    CanonRes c = canon(B, F);
    ts.canonCalls++;
    if (ctx.paranoid) {
        CanonRes n = canon_naive(B, F);
        if (n.B != c.B || n.F != c.F) {
#pragma omp atomic
            g_paranoidFail++;
        }
    }
    int r = ctx.leaves->find(t, c.B, c.F);
    if (r < 0 && ctx.fakeLeafRound >= 0 && t >= ctx.fakeLeafRound) r = -2;
    if (ce) *ce = CacheEnt{B, F, t, r, 0, 2, 1};
    return r;
}

static void game_init(Game& g) {
    memset(&g, 0, sizeof g);
    g.X = g.B = g.F = EMPTYSET;
    for (int c = 0; c < NC; c++) g.partner[c] = -1;
    g.nm = 0; g.ns = 0; g.t = 0; g.lastSP = -1; g.leafRound = -1; g.pairing = false; g.st = ST_OK;
    g.leafIdx = -1; g.leafF = 0; g.internal = IE_NONE; g.minEmpty = 6;
}
static inline void put_x(Game& g, int c) {   // FP stone (no bookkeeping of rounds)
    g.X.set(c);
    g.ns++;
    for (int i = 0; i < NLC[c]; i++) g.xc[LC[c][i]]++;
}
static inline void put_b(Game& g, int c) {   // SP stone
    g.B.set(c);
    g.ns++;
    for (int i = 0; i < NLC[c]; i++) g.bc[LC[c][i]]++;
}
static inline bool is_empty_cell(const Game& g, int c) { return !g.X.test(c) && !g.B.test(c); }
static inline Status internal_error(Game& g, int reason) {
    g.internal = reason;
    return g.st = ST_INTERNAL;
}

// danger pair with the smallest shared empty cell; lines a < b are the two smallest lines (= smallest cell
// lists, lines are sorted lexicographically) through that cell.  Returns the cell or -1.
static int find_danger(const Game& g, int& a, int& b) {
    int n4 = 0;
    for (int l = 0; l < NL; l++) n4 += (!g.bc[l] && g.xc[l] == 4);
    if (n4 < 2) return -1;
    for (int c = 0; c < NC; c++) {
        if (!is_empty_cell(g, c)) continue;
        int k = 0, la = -1, lb = -1;
        for (int i = 0; i < NLC[c]; i++) {
            int l = LC[c][i];
            if (!g.bc[l] && g.xc[l] == 4) {
                if (k == 0) la = l; else if (k == 1) lb = l;
                k++;
            }
        }
        if (k >= 2) {
            a = la; b = lb;
            return c;
        }
    }
    return -1;
}
// B-admissibility of X from the line counters (X and B are disjoint by construction);
// *maxLive = largest number of FP stones on a live line
static bool admissible_counts(const Game& g, int* maxLive = nullptr) {
    int mx = 0;
    for (int l = 0; l < NL; l++)
        if (!g.bc[l] && g.xc[l] > mx) mx = g.xc[l];
    if (maxLive) *maxLive = mx;
    if (mx >= 5) return false;
    int a, b;
    return find_danger(g, a, b) < 0;
}
// stand-alone version
static bool admissible(const Bits& B, const Bits& X) {
    if (B.meets(X)) return false;
    Game g;
    game_init(g);
    uint8_t cs[NC];
    int n = cells_of(B, cs);
    for (int i = 0; i < n; i++) put_b(g, cs[i]);
    n = cells_of(X, cs);
    for (int i = 0; i < n; i++) put_x(g, cs[i]);
    return admissible_counts(g);
}
// brute-force danger detection over pairs of lines, directly from the sets: smallest shared empty cell or -1
static int brute_danger(const Bits& B, const Bits& X) {
    int best = -1;
    for (int a = 0; a < NL; a++) {
        if (LB[a].meets(B) || (LB[a] & X).count() != 4) continue;
        for (int b = a + 1; b < NL; b++) {
            if (LB[b].meets(B) || (LB[b] & X).count() != 4) continue;
            Bits i = LB[a] & LB[b];
            if (i.empty()) continue;
            uint8_t cs[8];
            cells_of(i, cs);
            int c = cs[0];
            if (X.test(c)) continue;
            if (best < 0 || c < best) best = c;
        }
    }
    return best;
}
// definition of SPEC 3, directly from the sets (no counters)
static bool admissible_brute(const Bits& B, const Bits& X) {
    if (B.meets(X)) return false;
    for (int l = 0; l < NL; l++)
        if (!LB[l].meets(B) && (LB[l] & X).count() >= 5) return false;
    return brute_danger(B, X) < 0;
}
// pairing validity: every live line has exactly two assigned cells in l \ X, partner symmetric
static bool pairing_valid(const Bits& B, const Bits& X, const int16_t* partner) {
    int cnt[NL];
    memset(cnt, 0, sizeof cnt);
    for (int c = 0; c < NC; c++) {
        int p = partner[c];
        if (p < 0) continue;
        if (p >= NC || p == c || partner[p] != c) return false;
        if (X.test(c) || B.test(c)) return false;
        if (c < p) {
            int l = -1;
            for (int i = 0; i < NLC[c] && l < 0; i++)
                if (LB[LC[c][i]].test(p)) l = LC[c][i];
            if (l < 0 || LB[l].meets(B)) return false;
            cnt[l]++;
        }
    }
    for (int l = 0; l < NL; l++)
        if (cnt[l] != (LB[l].meets(B) ? 0 : 1)) return false;
    return true;
}
// --paranoid: the incremental data of the game against the sets
static bool paranoid_ok(const Game& g) {
    for (int l = 0; l < NL; l++)
        if (g.xc[l] != (LB[l] & g.X).count() || g.bc[l] != (LB[l] & g.B).count()) return false;
    if (g.ns != g.X.count() + g.B.count() || g.nm != g.ns) return false;
    Bits seen = EMPTYSET;
    for (int i = 0; i < g.nm; i++) {
        int c = g.moves[i];
        if (seen.test(c)) return false;
        seen.set(c);
        if ((i & 1) ? !g.B.test(c) : !g.X.test(c)) return false;
    }
    if (!g.pairing && admissible_brute(g.B, g.X) != admissible_counts(g)) return false;
    return true;
}

static int smallest_empty(const Game& g) {
    for (int i = 0; i < 4; i++) {
        u64 e = ~(g.X.w[i] | g.B.w[i]) & FULLSET.w[i];
        if (e) return i * 64 + __builtin_ctzll(e);
    }
    return -1;
}

// sigma steps 1 and 2 (TABLE mode, FP's stone already placed): the forced reply, or -1 if there is none.
// 1. threat: a live line with 5 cells of X -> its sixth cell, F gets the 5 stones.
// 2. danger: the danger pair with the smallest shared empty cell c -> c, F gets the 8 stones.
static int forced_reply(Game& g, Stats* st) {
    for (int l = 0; l < NL; l++)
        if (!g.bc[l] && g.xc[l] == 5) {
            int reply = -1;
            for (int i = 0; i < 6; i++)
                if (!g.X.test(LN[l][i])) reply = LN[l][i];
            g.F = g.F | (LB[l] & g.X);
            if (st) st->threat++;
            g.nThreat++;
            g.lastEvent = 1;
            if (g.nEv < MAXEV) { g.evKind[g.nEv] = 1; g.evRound[g.nEv++] = (uint8_t)std::min(g.t, 255); }
            return reply;
        }
    int a, b;
    int c = find_danger(g, a, b);
    if (c >= 0) {
        g.F = g.F | (LB[a] & g.X) | (LB[b] & g.X);
        if (st) st->danger++;
        g.nDanger++;
        g.lastEvent = 2;
        if (g.nEv < MAXEV) { g.evKind[g.nEv] = 2; g.evRound[g.nEv++] = (uint8_t)std::min(g.t, 255); }
    }
    return c;
}

static void leaf_stress(const Game& g, const Ctx& ctx, TS& ts);

// start of a round (FP to move): board full -> END; TABLE mode and P(B,F,t) is a leaf -> PAIRING mode
static Status round_start(Game& g, const Ctx& ctx, TS& ts, bool real = false) {
    if (g.ns >= NC) return g.st = ST_END;
    if (!g.pairing) {
        int idx = lookup_leaf(ctx, ts, g.B, g.F, g.t);
        if (idx != -1) {
            g.leafIdx = idx;
            g.leafF = g.F.count();
            if (idx >= 0) {
                if (ctx.covL && !ctx.covL[idx]) g.leafNovel = 1;
                if (ctx.prbL && !ctx.prbL[idx]) g.leafNovelP = 1;
                ts.mark_l(idx, real);
            }
            bool ok = find_pairing(g.B, g.X, g.partner);
            if (ctx.paranoid && ok != find_pairing_ref(g.B, g.X)) return internal_error(g, IE_PARANOID);
            if (!ok) return g.st = ST_PAIRING_FAIL;
            if (!pairing_valid(g.B, g.X, g.partner)) return internal_error(g, IE_PAIRING_INVALID);
            g.pairing = true;
            g.leafRound = g.t;
            if (real && ctx.leafStress > 0 && g.t > g.leafF) leaf_stress(g, ctx, ts);
        }
    }
    return g.st = ST_OK;
}

// FP plays x, SP answers with sigma, then the next round is started.  st != nullptr marks a move of a real
// game (statistics, coverage); st == nullptr is a look-ahead simulation of an adversary.
static Status play_impl(Game& g, int x, const Ctx& ctx, TS& ts, Stats* st) {
    if (g.st != ST_OK) return internal_error(g, IE_PLAY_AFTER_END);
    if (x < 0 || x >= NC || !is_empty_cell(g, x)) return internal_error(g, IE_FP_OCCUPIED);
    put_x(g, x);
    g.moves[g.nm++] = (uint8_t)x;
    g.t++;
    for (int i = 0; i < NLC[x]; i++)
        if (g.xc[LC[x][i]] == 6) return g.st = ST_FP_LINE;
    int reply = -1;
    g.lastOcc = 0;
    if (g.pairing) {
        g.lastEvent = 3;
        int p = g.partner[x];
        if (p >= 0 && is_empty_cell(g, p)) reply = p;
        else reply = smallest_empty(g);
        if (reply < 0) return internal_error(g, IE_NO_EMPTY);   // 216 is even: SP always finds a cell
        put_b(g, reply);
    } else {
        // 1. threat, 2. danger
        reply = forced_reply(g, st);
        // 3. table
        if (reply < 0) {
            g.lastEvent = 0;
            for (;;) {
                int idx = -1;
                int m = lookup_q(ctx, ts, g.B, g.F, g.t, &idx);
                if (m < 0) return g.st = ST_HOLE;
                if (idx >= 0) {
                    if (ctx.covQ && !ctx.covQ[idx]) g.novelQ++;
                    if (ctx.prbQ && !ctx.prbQ[idx]) g.novelPQ++;
                    ts.mark_q(idx, st != nullptr);
                }
                if (g.B.test(m) || g.F.test(m)) return g.st = ST_ILLEGAL;
                if (g.X.test(m)) {
                    g.F.set(m);
                    if (st) st->occupied++;
                    g.nOcc++;
                    g.lastOcc++;
                    continue;
                }
                reply = m;
                break;
            }
            if (st) st->table_moves++;
            g.nTable++;
            if (g.lastOcc >= 2) g.nMultiOcc++;
            if (g.lastOcc > g.maxOcc) g.maxOcc = g.lastOcc;
            if (!ISBUSY[reply]) g.nNonBusy++;
        }
        if (reply < 0 || reply >= NC || !is_empty_cell(g, reply)) return internal_error(g, IE_SP_OCCUPIED);
        put_b(g, reply);
        if (st && g.t > st->maxTableRound) st->maxTableRound = g.t;
    }
    g.moves[g.nm++] = (uint8_t)reply;
    g.lastSP = reply;
    if (!g.pairing) {
        // bookkeeping invariants (I2), (I3) of PROOF 8.9: |B| = |X| = t, B and X disjoint, F a subset of X
        if (g.B.meets(g.X) || (g.F & g.X) != g.F || g.F.count() > g.t ||
            (ctx.checkCounts && (g.B.count() != g.t || g.X.count() != g.t)))
            return internal_error(g, IE_BOOKKEEPING);
        int mx = 0;
        if (!admissible_counts(g, &mx)) return g.st = ST_ADMISS_FAIL;
        if (6 - mx < g.minEmpty) g.minEmpty = (uint8_t)(6 - mx);
        if (mx == 4 && g.near4 < 255) g.near4++;
    } else {
        // invariant of the pairing strategy: a line that is still live keeps both assigned cells empty.  Only the
        // lines through x can have changed for the worse (SP's stone only kills lines).
        int p = g.partner[x];
        for (int i = 0; i < NLC[x]; i++) {
            int l = LC[x][i];
            if (g.bc[l]) continue;
            if (p >= 0 && LB[l].test(p)) return g.st = ST_PAIR_BROKEN;   // x was one of the two cells of the live line l
            if (g.xc[l] >= 5) return g.st = ST_PAIR_BROKEN;
            if (6 - g.xc[l] < g.minEmpty) g.minEmpty = (uint8_t)(6 - g.xc[l]);
        }
    }
    if (ctx.paranoid && !paranoid_ok(g)) return internal_error(g, IE_PARANOID);
    return round_start(g, ctx, ts, st != nullptr);
}
static Status play(Game& g, int x, const Ctx& ctx, TS& ts, Stats* st) {
    Status s = play_impl(g, x, ctx, ts, st);
    if (!st && is_bug(s)) {
        ts.laBugs++;
        if (ts.laBugJson.size() < 64) ts.laBugJson.push_back(bug_json(-1, 0, "lookahead", g));
    }
    return s;
}

// ----------------------------------------------------------------------------------------------------
// random numbers
// ----------------------------------------------------------------------------------------------------
struct Rng {
    u64 s[4];
    explicit Rng(u64 seed) {
        u64 z = seed;
        for (int i = 0; i < 4; i++) {
            z += 0x9e3779b97f4a7c15ULL;
            u64 x = z;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
            s[i] = x ^ (x >> 31);
        }
    }
    static inline u64 rotl(u64 x, int k) { return (x << k) | (x >> (64 - k)); }
    u64 next() {
        u64 r = rotl(s[1] * 5, 7) * 9, t = s[1] << 17;
        s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return r;
    }
    int below(int n) { return (int)(((unsigned __int128)next() * (u64)n) >> 64); }
    double uniform() { return (double)(next() >> 11) * (1.0 / 9007199254740992.0); }
};

// Leaf stress test (added by the audit; SPEC 3 only).  A leaf P(B,F,t) of the proof tree claims C(B,F,t): EVERY
// B-admissible X with F a subset of X and |X| <= t has a pairing.  The game itself tests this for one X, the real
// one.  Here the t - |F| unrevealed stones are replaced by other admissible choices (uniform; packed on one live
// line and its neighbours; packed on live lines that already carry revealed stones) and the pairing is computed
// again.  A failure is a bug of the leaf certificate (LEAF claimed although C fails), whether or not that X can
// arise in real play.  Deterministic: the random choices are seeded by the position.
static void leaf_stress(const Game& g, const Ctx& ctx, TS& ts) {
    int u = g.t - g.F.count();
    if (u <= 0) return;
    Rng r(mixa(g.B.w[0] ^ mixb(g.B.w[1] ^ mixa(g.B.w[2] ^ mixb(g.B.w[3])))) ^ mixb(g.X.w[0] ^ mixa(g.X.w[1] ^ mixb(g.X.w[2] ^ mixa(g.X.w[3] + 99)))));
    Game base;
    game_init(base);
    uint8_t cs[NC];
    int n = cells_of(g.B, cs);
    for (int i = 0; i < n; i++) put_b(base, cs[i]);
    n = cells_of(g.F, cs);
    for (int i = 0; i < n; i++) put_x(base, cs[i]);
    int live[NL], nl = 0;
    for (int l = 0; l < NL; l++)
        if (!base.bc[l]) live[nl++] = l;
    if (!nl) return;
    for (int k = 0; k < ctx.leafStress; k++) {
        Game q = base;
        int placed = 0, mode = r.below(3), line = -1;
        for (int tries = 0; tries < 80 && placed < u; tries++) {
            int c;
            if (mode == 0) {
                int e = NC - q.ns;
                if (e <= 0) break;
                int kth = r.below(e);
                c = -1;
                for (int w = 0; w < 4 && c < 0; w++) {
                    u64 em = ~(q.X.w[w] | q.B.w[w]) & FULLSET.w[w];
                    int pc = __builtin_popcountll(em);
                    if (kth < pc) {
                        while (kth--) em &= em - 1;
                        c = w * 64 + __builtin_ctzll(em);
                    } else
                        kth -= pc;
                }
            } else {
                if (line < 0 || r.below(4) == 0) {
                    line = live[r.below(nl)];
                    if (mode == 2)
                        for (int j = 0; j < 8; j++) {   // prefer lines that already carry stones
                            int l2 = live[r.below(nl)];
                            if (q.xc[l2] > q.xc[line] && q.xc[l2] <= 3) line = l2;
                        }
                }
                c = LN[line][r.below(6)];
            }
            if (c < 0 || !is_empty_cell(q, c)) continue;
            put_x(q, c);
            if (!admissible_counts(q)) {   // undo
                q.X.reset(c);
                q.ns--;
                for (int i = 0; i < NLC[c]; i++) q.xc[LC[c][i]]--;
                continue;
            }
            placed++;
        }
        if (placed < u) continue;
        ts.stressTests++;
        int16_t partner[NC];
        if (!find_pairing(q.B, q.X, partner)) {
            ts.stressFail++;
            if (ts.stressJson.size() < 64)
                ts.stressJson.push_back("{\"kind\":\"leaf_stress_failure\",\"t\":" + std::to_string(g.t) + ",\"B\":\"" + set_str(g.B) +
                                        "\",\"F\":\"" + set_str(g.F) + "\",\"X\":\"" + set_str(q.X) + "\",\"real_X\":\"" + set_str(g.X) + "\"}");
        } else if (!pairing_valid(q.B, q.X, partner)) {
#pragma omp atomic
            g_paranoidFail++;
        }
    }
}

// ----------------------------------------------------------------------------------------------------
// adversaries
// ----------------------------------------------------------------------------------------------------
enum Adv {
    ADV_RANDOM = 0, ADV_GREEDY, ADV_STEAL, ADV_THREAT, ADV_SEARCH, ADV_DANGER, ADV_DOUBLE, ADV_CORNER, ADV_TREE,
    ADV_MIX, ADV_COUNT = ADV_MIX
};
static const char* ADV_NAME[] = {"random", "greedy", "steal", "threat", "search", "danger", "double", "corner", "tree", "mix"};

struct Params {
    double p = 0.1;          // threat adversary: probability of a random move
    int depth = 2;           // search adversary
    int width = 8;
    int playouts = 16;       // tree adversary and the searched halves of danger/double/corner: playouts per plan
    // "mix": random, greedy, steal, threat, search, danger, double, corner, tree
    double mixw[ADV_COUNT] = {0.03, 0.03, 0.04, 0.05, 0.02, 0.09, 0.09, 0.15, 0.50};
};

static int nth_empty(const Game& g, int k) {
    for (int i = 0; i < 4; i++) {
        u64 e = ~(g.X.w[i] | g.B.w[i]) & FULLSET.w[i];
        int pc = __builtin_popcountll(e);
        if (k < pc) {
            while (k--) e &= e - 1;
            return i * 64 + __builtin_ctzll(e);
        }
        k -= pc;
    }
    return -1;
}
static int random_empty(const Game& g, Rng& rng) { return nth_empty(g, rng.below(NC - g.ns)); }

static const int64_t GW[6] = {1, 8, 64, 512, 4096, 1000000000000LL};
// score of an empty cell for FP.  Greedy part: sum over the live lines through c of GW[own stones on it].
// Threat part: bonus for making 5 on a live line, for creating a danger pair (4 + 4 through an empty cell)
// and for 3s/4s on intersecting live lines.
static int64_t cell_score(const Game& g, int c, bool threatMode) {
    static const int64_t TW[5][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
                                     {0, 0, 0, 2000, 20000}, {0, 0, 0, 20000, 3000000}};
    int64_t s = 0;
    for (int i = 0; i < NLC[c]; i++) {
        int l = LC[c][i];
        if (g.bc[l]) continue;
        int k = g.xc[l];
        s += GW[k];
        if (!threatMode || k < 2 || k >= 5) continue;
        int ka = k + 1;
        if (ka == 5) { s += 1000000; continue; }
        for (int j = 0; j < 6; j++) {
            int d = LN[l][j];
            if (d == c || g.X.test(d)) continue;
            for (int q = 0; q < NLC[d]; q++) {
                int b = LC[d][q];
                if (b == l || g.bc[b]) continue;
                int kb = g.xc[b];
                if (kb >= 3 && kb <= 4) s += TW[ka][kb];
            }
        }
    }
    return s;
}
static int best_scored(const Game& g, Rng& rng, bool threatMode) {
    int64_t best = -1;
    int bc = -1, ties = 0;
    for (int w = 0; w < 4; w++) {
        u64 e = ~(g.X.w[w] | g.B.w[w]) & FULLSET.w[w];
        while (e) {
            int c = w * 64 + __builtin_ctzll(e);
            e &= e - 1;
            int64_t s = cell_score(g, c, threatMode);
            if (s > best) { best = s; bc = c; ties = 1; }
            else if (s == best && rng.below(++ties) == 0) bc = c;
        }
    }
    return bc;
}
// the k best scored empty cells (random tie-breaking) plus nrand further random empty cells
static int top_cands(const Game& g, Rng& rng, int k, int nrand, int* out) {
    struct E { int64_t s; uint32_t r; int c; };
    E e[NC];
    int n = 0;
    for (int w = 0; w < 4; w++) {
        u64 em = ~(g.X.w[w] | g.B.w[w]) & FULLSET.w[w];
        while (em) {
            int c = w * 64 + __builtin_ctzll(em);
            em &= em - 1;
            e[n++] = E{cell_score(g, c, true), (uint32_t)rng.next(), c};
        }
    }
    if (k > n) k = n;
    std::partial_sort(e, e + k, e + n, [](const E& a, const E& b) { return a.s != b.s ? a.s > b.s : a.r < b.r; });
    int m = 0;
    for (int i = 0; i < k; i++) out[m++] = e[i].c;
    for (int i = 0; i < nrand && k + i < n; i++) {   // random distinct cells among the rest
        int j = k + i + rng.below(n - k - i);
        std::swap(e[k + i], e[j]);
        out[m++] = e[k + i].c;
    }
    return m;
}

// What happens if FP plays the empty cell x in TABLE mode (no table lookup needed):
// 0: no forced event; 1: a threat or danger that SP answers, X stays admissible;
// 2: FP wins material: a line is completed, or X is not admissible after the forced reply (LOST pattern).
static int forcing_result(const Game& g, int x) {
    bool f = false;
    for (int i = 0; i < NLC[x]; i++) {
        int l = LC[x][i];
        if (g.bc[l]) continue;
        int k = g.xc[l];
        if (k == 5) return 2;
        if (k == 4) f = true;
        else if (k == 3 && !f) {
            for (int j = 0; j < 6 && !f; j++) {
                int d = LN[l][j];
                if (d == x || g.X.test(d)) continue;
                for (int q = 0; q < NLC[d]; q++) {
                    int b = LC[d][q];
                    if (b != l && !g.bc[b] && g.xc[b] == 4) { f = true; break; }
                }
            }
        }
    }
    if (!f) return 0;
    Game h = g;
    put_x(h, x);
    int r = forced_reply(h, nullptr);
    if (r < 0) return 0;
    put_b(h, r);
    return admissible_counts(h) ? 1 : 2;
}
// an immediately winning cell for FP in TABLE mode, or -1; forcing (non-winning) cells are collected on request
static int find_winner(const Game& g, int* forcing = nullptr, int* nforcing = nullptr) {
    int n4 = 0;
    for (int l = 0; l < NL; l++) n4 += (!g.bc[l] && g.xc[l] >= 4);
    if (nforcing) *nforcing = 0;
    if (!n4) return -1;
    for (int w = 0; w < 4; w++) {
        u64 e = ~(g.X.w[w] | g.B.w[w]) & FULLSET.w[w];
        while (e) {
            int c = w * 64 + __builtin_ctzll(e);
            e &= e - 1;
            int r = forcing_result(g, c);
            if (r == 2) return c;
            if (r == 1 && forcing) forcing[(*nforcing)++] = c;
        }
    }
    return -1;
}

// Distance to a LOST pattern.  FP wins in TABLE mode by one move x that creates two forced events which a
// single SP stone cannot answer: threat + danger (E2) or two dangers (E3).  For an empty cell x and a live
// line a through x:   T(a) = 4 - |X∩a|            stones missing for "x makes 5 on a",
//                      D(a,x) = 3 - |X∩a| + min over empty c in a, c != x, live b through c of (4 - |X∩b|)
//                                                   stones missing for "x makes a danger pair (a,b) at c".
// cost(x) = min over lines i != j through x of min(T(a_i), D(a_i,x)) + D(a_j,x)   (two threats cannot be built).
// mincost = min over x; cand = the empty cells that bring one of the cheapest targets one stone closer.
struct Plan {
    int mincost, ntargets, ncand;
    uint8_t cand[NC];
};
static void make_plan(const Game& g, Rng& rng, Plan& P, bool wantCands) {
    const int INF = 50;
    // per cell: the two most advanced live lines through it (value 4 - stones)
    int8_t cv1[NC], cv2[NC];
    int16_t cl1[NC], cl2[NC];
    for (int c = 0; c < NC; c++) {
        cv1[c] = cv2[c] = INF;
        cl1[c] = cl2[c] = -1;
        if (g.X.test(c) || g.B.test(c)) continue;
        for (int q = 0; q < NLC[c]; q++) {
            int b = LC[c][q];
            if (g.bc[b] || g.xc[b] > 4) continue;
            int v = 4 - g.xc[b];
            if (v < cv1[c]) { cv2[c] = cv1[c]; cl2[c] = cl1[c]; cv1[c] = (int8_t)v; cl1[c] = (int16_t)b; }
            else if (v < cv2[c]) { cv2[c] = (int8_t)v; cl2[c] = (int16_t)b; }
        }
    }
    int8_t T[NL], D1[NL], D2[NL];
    int16_t C1[NL], C2[NL], B1[NL], B2[NL];
    for (int a = 0; a < NL; a++) {
        T[a] = D1[a] = D2[a] = INF;
        C1[a] = C2[a] = B1[a] = B2[a] = -1;
        if (g.bc[a] || g.xc[a] > 4) continue;
        int s = g.xc[a];
        T[a] = (int8_t)(4 - s);
        if (s > 3) continue;
        int best = INF, second = INF;
        for (int i = 0; i < 6; i++) {
            int c = LN[a][i];
            if (g.X.test(c)) continue;
            int bcst, bb;
            if (cl1[c] != a) { bcst = cv1[c]; bb = cl1[c]; }
            else { bcst = cv2[c]; bb = cl2[c]; }
            if (bcst < best) {
                second = best; C2[a] = C1[a]; B2[a] = B1[a];
                best = bcst; C1[a] = (int16_t)c; B1[a] = (int16_t)bb;
            } else if (bcst < second) {
                second = bcst; C2[a] = (int16_t)c; B2[a] = (int16_t)bb;
            }
        }
        if (best < INF) D1[a] = (int8_t)(3 - s + best);
        if (second < INF) D2[a] = (int8_t)(3 - s + second);
    }
    struct Tg { int16_t x, ai, aj; bool isT; };
    Tg tg[8];
    int ntg = 0;
    P.mincost = 99;
    P.ntargets = 0;
    P.ncand = 0;
    for (int w = 0; w < 4; w++) {
        u64 e = ~(g.X.w[w] | g.B.w[w]) & FULLSET.w[w];
        while (e) {
            int x = w * 64 + __builtin_ctzll(e);
            e &= e - 1;
            int nl = NLC[x];
            int Tv[7], Dv[7], Bl[7];
            int m1 = INF, m2 = INF;
            for (int j = 0; j < nl; j++) {
                int a = LC[x][j];
                bool f1 = C1[a] != x;
                Tv[j] = T[a];
                Dv[j] = f1 ? D1[a] : D2[a];
                Bl[j] = f1 ? B1[a] : B2[a];
                int v = std::min(Tv[j], Dv[j]);
                if (v < m1) { m2 = m1; m1 = v; }
                else if (v < m2) m2 = v;
            }
            if (m1 + m2 > P.mincost) continue;   // lower bound: the two smallest event costs
            for (int j = 0; j < nl; j++) {
                if (Dv[j] >= INF) continue;
                for (int i = 0; i < nl; i++) {
                    if (i == j) continue;
                    int cost = INF;
                    bool isT = false;
                    if (Tv[i] < INF) {   // E2: threat on line i, danger (line j, Bl[j])
                        bool ok = true;
                        if (Tv[i] == 0) {   // the sixth cell of the threat line must not lie on the danger partner
                            const Bits& la = LB[LC[x][i]];
                            const Bits& lb = LB[Bl[j]];
                            for (int q = 0; q < 4; q++)
                                if (la.w[q] & lb.w[q] & ~g.X.w[q]) ok = false;
                        }
                        if (ok) { cost = Tv[i] + Dv[j]; isT = true; }
                    }
                    if (i < j && Dv[i] < INF && Bl[i] != Bl[j] && Dv[i] + Dv[j] < cost) {   // E3: two dangers
                        cost = Dv[i] + Dv[j];
                        isT = false;
                    }
                    if (cost > P.mincost) continue;
                    if (cost < P.mincost) { P.mincost = cost; P.ntargets = 0; ntg = 0; }
                    P.ntargets++;
                    if (!wantCands) continue;
                    Tg t{(int16_t)x, (int16_t)LC[x][i], (int16_t)LC[x][j], isT};
                    if (ntg < 8) tg[ntg++] = t;
                    else {
                        int r = rng.below(P.ntargets);
                        if (r < 8) tg[r] = t;
                    }
                }
            }
        }
    }
    if (!wantCands) return;
    Bits need = EMPTYSET;
    auto addLine = [&](int l, int ex1, int ex2) {
        for (int i = 0; i < 6; i++) {
            int c = LN[l][i];
            if (c != ex1 && c != ex2 && !g.X.test(c)) need.set(c);
        }
    };
    for (int k = 0; k < ntg; k++) {
        int x = tg[k].x;
        for (int side = 0; side < 2; side++) {
            int a = side ? tg[k].aj : tg[k].ai;
            if (side == 0 && tg[k].isT) {
                if (T[a] > 0) addLine(a, x, -1);
                continue;
            }
            bool f1 = C1[a] != x;
            int c = f1 ? C1[a] : C2[a], b = f1 ? B1[a] : B2[a];
            if (g.xc[a] < 3) addLine(a, x, c);
            if (g.xc[b] < 4) addLine(b, c, -1);
        }
    }
    P.ncand = cells_of(need, P.cand);
    for (int i = P.ncand - 1; i > 0; i--) std::swap(P.cand[i], P.cand[rng.below(i + 1)]);
}

// static evaluation for FP: staying in TABLE mode dominates, then the distance to a LOST pattern, then lines
static int64_t eval_fp(const Game& g, const Plan* pl) {
    int mx = 0, n4 = 0, n3 = 0, n2 = 0;
    for (int l = 0; l < NL; l++) {
        if (g.bc[l]) continue;
        int k = g.xc[l];
        if (k > mx) mx = k;
        n4 += (k >= 4); n3 += (k == 3); n2 += (k == 2);
    }
    int64_t v = 1000LL * mx + 300LL * n4 + 20LL * n3 + n2;
    if (g.pairing || !pl) return v;
    return v + 10000000 + 100000LL * (20 - std::min(pl->mincost, 20)) + 100LL * std::min(pl->ntargets, 50);
}
constexpr int64_t BUGV = 1000000000000LL;

// candidate moves of a TABLE-mode node: cells of the cheapest LOST-pattern plans, plus random cells
static int node_cands(const Game& g, const Plan& pl, Rng& rng, int k, int nrand, int* out) {
    int n = 0;
    for (int i = 0; i < pl.ncand && n < k; i++) out[n++] = pl.cand[i];
    if (n == 0) n = top_cands(g, rng, k, 0, out);
    for (int i = 0; i < nrand; i++) {
        int c = random_empty(g, rng);
        bool dup = false;
        for (int j = 0; j < n; j++) dup |= (out[j] == c);
        if (!dup) out[n++] = c;
    }
    return n;
}

static int64_t search_value(const Game& g, int d, const Ctx& ctx, TS& ts, Rng& rng, const Params& P) {
    if (g.pairing) return eval_fp(g, nullptr);
    {   // the next FP move breaks admissibility or completes a line: play it in a clone so that it is recorded
        int w = find_winner(g);
        if (w >= 0) {
            Game h = g;
            play(h, w, ctx, ts, nullptr);
            return BUGV + d;
        }
    }
    Plan pl;
    make_plan(g, rng, pl, d > 0);
    if (d <= 0) return eval_fp(g, &pl);
    int cs[NC];
    int n = node_cands(g, pl, rng, std::max(3, P.width / 2), 1, cs);
    int64_t best = -BUGV;
    for (int i = 0; i < n; i++) {
        Game h = g;
        Status s = play(h, cs[i], ctx, ts, nullptr);
        int64_t v = is_bug(s) ? BUGV + d : (s == ST_END ? eval_fp(h, nullptr) : search_value(h, d - 1, ctx, ts, rng, P));
        if (v > best) best = v;
    }
    return best;
}
// sigma is deterministic: clone the game, try a sampled set of moves, let sigma answer, look ahead
static int search_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P) {
    if (g.pairing) return best_scored(g, rng, true);   // no table left to attack: cheap line building
    int forcing[NC], nforcing = 0;
    int w = find_winner(g, forcing, &nforcing);
    if (w >= 0) return w;
    Plan pl;
    make_plan(g, rng, pl, true);
    int cs[NC];
    int n = node_cands(g, pl, rng, P.width, 1, cs);
    if (nforcing) {   // one forcing move (threat or danger that SP can answer): exercises the reveal branches
        int c = forcing[rng.below(nforcing)];
        bool dup = false;
        for (int j = 0; j < n; j++) dup |= (cs[j] == c);
        if (!dup) cs[n++] = c;
    }
    int64_t best = -2 * BUGV;
    int bc = cs[0], ties = 0;
    for (int i = 0; i < n; i++) {
        Game h = g;
        Status s = play(h, cs[i], ctx, ts, nullptr);
        int64_t v = is_bug(s) ? BUGV + P.depth
                              : (s == ST_END ? eval_fp(h, nullptr) : search_value(h, P.depth - 1, ctx, ts, rng, P));
        if (v > best) { best = v; bc = cs[i]; ties = 1; }
        else if (v == best && rng.below(++ties) == 0) bc = cs[i];
    }
    return bc;
}

// ----------------------------------------------------------------------------------------------------
// adversaries added by the audit (danger, double, corner, tree).  They use the same information as "search":
// the real position and sigma itself as an oracle (clone the game, let sigma answer; look up the table for
// the cell sigma will play next).  Nothing here knows the abstract tree of SPEC 4.
// ----------------------------------------------------------------------------------------------------
static uint8_t LBUSY[NL];   // number of busy cells on a line (0, 2 or 6)
static void init_adv_tables() {
    for (int l = 0; l < NL; l++) {
        LBUSY[l] = 0;
        for (int i = 0; i < 6; i++) LBUSY[l] += ISBUSY[LN[l][i]];
    }
}

// The empty cell sigma plays next if FP's move is quiet (no threat, no danger) and elsewhere: the table move of
// Q(B,F,t) after the reveals of the stones FP already holds.  -1: hole or illegal entry ahead.
static int predict_table(const Ctx& ctx, TS& ts, const Bits& B, Bits F, const Bits& X, int t, Bits* Fout = nullptr) {
    for (int it = 0; it <= NC; it++) {
        int m = lookup_q(ctx, ts, B, F, t);
        if (m < 0 || B.test(m) || F.test(m)) return -1;
        if (!X.test(m)) {
            if (Fout) *Fout = F;
            return m;
        }
        F.set(m);
    }
    return -1;
}
// steal now: the cell sigma wants in this round (playing it is revealed at once and sigma looks up again)
static int steal_now(const Game& g, const Ctx& ctx, TS& ts) {
    if (g.pairing) return -1;
    return predict_table(ctx, ts, g.B, g.F, g.X, g.t + 1);
}
// pre-steal: the cell sigma wants d rounds from now if all rounds up to then are quiet (d = 0: steal_now).
// Playing it now gives an occupied reveal d rounds later; several such stones give rounds with two or more
// reveals (a call made after such a stone is placed returns the cell sigma tries after that reveal).
static int steal_ahead(const Game& g, const Ctx& ctx, TS& ts, int d) {
    if (g.pairing) return -1;
    Bits B = g.B, F = g.F;
    for (int i = 0; i < d; i++) {
        Bits F1;
        int m = predict_table(ctx, ts, B, F, g.X, g.t + 1 + i, &F1);
        if (m < 0) return -1;
        B.set(m);
        F = F1;
        if (lookup_leaf(ctx, ts, B, F, g.t + 1 + i) != -1) return -1;   // TABLE mode would be over
    }
    int c = predict_table(ctx, ts, B, F, g.X, g.t + 1 + d);
    return (c >= 0 && is_empty_cell(g, c)) ? c : -1;
}
static int steal_next(const Game& g, const Ctx& ctx, TS& ts) { return steal_ahead(g, ctx, ts, 1); }
// a steal with a random delay: 45% now, else 1..6 rounds ahead (falls back to shorter delays)
static int steal_random(const Game& g, const Ctx& ctx, TS& ts, Rng& rng) {
    static const int cum[7] = {45, 65, 77, 85, 91, 96, 100};
    int r = rng.below(100), d = 0;
    while (r >= cum[d]) d++;
    for (; d >= 0; d--) {
        int c = steal_ahead(g, ctx, ts, d);
        if (c >= 0) return c;
    }
    return -1;
}
// look-ahead: what happens if FP plays x now.  0: TABLE mode goes on, 1: a leaf is reached, 2: bug, 3: game over
static int probe(const Game& g, int x, const Ctx& ctx, TS& ts) {
    Game h = g;
    Status s = play(h, x, ctx, ts, nullptr);
    if (is_bug(s)) return 2;
    if (s == ST_END) return 3;
    return h.pairing ? 1 : 0;
}
// first candidate that exposes a bug, else the first after which sigma is still in TABLE mode, else fallback
static int choose_alive(const Game& g, const int* cands, int n, int fallback, const Ctx& ctx, TS& ts) {
    int alive = -1;
    for (int i = 0; i < n; i++) {
        if (cands[i] < 0 || !is_empty_cell(g, cands[i])) continue;
        bool dup = false;
        for (int j = 0; j < i; j++) dup |= (cands[j] == cands[i]);
        if (dup) continue;
        int r = probe(g, cands[i], ctx, ts);
        if (r == 2) return cands[i];
        if (r == 0 && alive < 0) alive = cands[i];
    }
    if (alive >= 0) return alive;
    if (fallback >= 0 && is_empty_cell(g, fallback)) return fallback;
    for (int i = 0; i < n; i++)
        if (cands[i] >= 0 && is_empty_cell(g, cands[i])) return cands[i];
    return -1;
}

// per-game memory of the plan-based adversaries
struct AdvState {
    bool init = false;
    int variant = 0;
    int busyW = 6;                                   // penalty per busy cell on a target line (sigma likes busy cells)
    int l1 = -1, e1 = -1, l2 = -1, e2 = -1;         // threat targets: line and the cell left empty
    int da = -1, db = -1, dc = -1, dsa = -1, dsb = -1;   // danger target: lines a, b, shared cell c, cells left empty
    int first = -1;                                  // double: the stone shared by the two threat lines
    int fillers = 0;                                 // filler moves to spend before the event
    int face = 0, stealRounds = 0;                   // corner
    bool greedyEnd = false;                          // PAIRING mode: greedy line building instead of random moves
    bool searched = false;                           // danger / double / corner: use the playout search (see plan_search)
    bool noSearch = false;                           // set for the scripted policy when it is used inside a playout
    bool stealThreat = false;                        // corner: this game follows the "steal + threat" schedule
    int planLen = 0, planT0 = -1;                    // tree: FP moves found by the search, planned from round planT0
    uint8_t plan[24];
};

static bool threat_ok(const Game& g, int l, int e) {
    return l >= 0 && e >= 0 && !g.bc[l] && g.xc[l] <= 4 && is_empty_cell(g, e) && LB[l].test(e);
}
// the empty cells of l other than e (the stones FP still has to place; the last one makes the threat)
static int threat_cells(const Game& g, int l, int e, int* out) {
    int n = 0;
    for (int i = 0; i < 6; i++) {
        int c = LN[l][i];
        if (c != e && !g.X.test(c)) out[n++] = c;
    }
    return n;
}
// cheapest threat target: few missing stones, few busy cells, random tie-breaking.  through >= 0: only lines
// through that cell.  avoid: a line that must not be chosen.
static bool pick_threat(const Game& g, Rng& rng, int busyW, int avoid, int through, int& l, int& e, int missW = 24) {
    int best = 1 << 30, bl = -1;
    for (int q = 0; q < NL; q++) {
        if (g.bc[q] || g.xc[q] > 4 || q == avoid) continue;
        if (through >= 0 && !LB[q].test(through)) continue;
        int cost = missW * (5 - g.xc[q]) + busyW * LBUSY[q] + rng.below(16);
        if (cost < best) { best = cost; bl = q; }
    }
    if (bl < 0) return false;
    int em[6], n = 0;
    for (int i = 0; i < 6; i++)
        if (!g.X.test(LN[bl][i])) em[n++] = LN[bl][i];
    l = bl;
    e = em[rng.below(n)];
    return true;
}
static bool danger_ok(const Game& g, const AdvState& as) {
    return as.da >= 0 && as.db >= 0 && !g.bc[as.da] && !g.bc[as.db] && g.xc[as.da] <= 4 && g.xc[as.db] <= 4 &&
           is_empty_cell(g, as.dc) && is_empty_cell(g, as.dsa) && is_empty_cell(g, as.dsb);
}
// the stones still missing for "4 on a without c and sa, 4 on b without c and sb"
static int danger_cells(const Game& g, const AdvState& as, int* out) {
    int n = 0;
    for (int i = 0; i < 6; i++) {
        int c = LN[as.da][i];
        if (c != as.dc && c != as.dsa && !g.X.test(c)) out[n++] = c;
    }
    for (int i = 0; i < 6; i++) {
        int c = LN[as.db][i];
        if (c != as.dc && c != as.dsb && !g.X.test(c)) out[n++] = c;
    }
    return n;
}
// cheapest danger target: two live lines through an empty cell c, each with at most 4 FP stones
static bool pick_danger(const Game& g, Rng& rng, int busyW, AdvState& as) {
    int best = 1 << 30, ba = -1, bb = -1, bcell = -1;
    for (int w = 0; w < 4; w++) {
        u64 em = ~(g.X.w[w] | g.B.w[w]) & FULLSET.w[w];
        while (em) {
            int c = w * 64 + __builtin_ctzll(em);
            em &= em - 1;
            int ls[7], n = 0;
            for (int i = 0; i < NLC[c]; i++) {
                int l = LC[c][i];
                if (!g.bc[l] && g.xc[l] <= 4) ls[n++] = l;
            }
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++) {
                    int miss = 8 - g.xc[ls[i]] - g.xc[ls[j]];
                    if (miss <= 0) continue;
                    int cost = 24 * miss + busyW * (LBUSY[ls[i]] + LBUSY[ls[j]]) + rng.below(16);
                    if (cost < best) { best = cost; ba = ls[i]; bb = ls[j]; bcell = c; }
                }
        }
    }
    if (ba < 0) return false;
    as.da = ba; as.db = bb; as.dc = bcell;
    int em[6], n = 0;
    for (int i = 0; i < 6; i++)
        if (LN[ba][i] != bcell && !g.X.test(LN[ba][i])) em[n++] = LN[ba][i];
    as.dsa = em[rng.below(n)];
    n = 0;
    for (int i = 0; i < 6; i++)
        if (LN[bb][i] != bcell && !g.X.test(LN[bb][i])) em[n++] = LN[bb][i];
    as.dsb = em[rng.below(n)];
    return true;
}
static int random_empty_busy(const Game& g, Rng& rng) {
    int eb[NB], n = 0;
    for (int i = 0; i < NB; i++)
        if (is_empty_cell(g, BUSY[i])) eb[n++] = BUSY[i];
    return n ? eb[rng.below(n)] : -1;
}
static int random_antipode(const Game& g, Rng& rng) {
    int cs[NC], n = 0;
    uint8_t bcs[NC];
    int nb = cells_of(g.B, bcs);
    for (int i = 0; i < nb; i++)
        if (is_empty_cell(g, ANTI[bcs[i]])) cs[n++] = ANTI[bcs[i]];
    return n ? cs[rng.below(n)] : -1;
}
// a filler move that keeps away from the cells in "keep" (cells the plan needs empty)
static int filler_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Bits& keep) {
    for (int tries = 0; tries < 6; tries++) {
        int r = rng.below(100), c = -1;
        if (r < 78) c = steal_random(g, ctx, ts, rng);
        else if (r < 86) c = random_empty_busy(g, rng);
        else if (r < 92) c = random_antipode(g, rng);
        else c = random_empty(g, rng);
        if (c >= 0 && is_empty_cell(g, c) && !keep.test(c)) return c;
    }
    for (int tries = 0; tries < 20; tries++) {
        int c = random_empty(g, rng);
        if (!keep.test(c)) return c;
    }
    return random_empty(g, rng);
}
static int end_move(const Game& g, Rng& rng, const AdvState& as) {
    return as.greedyEnd ? best_scored(g, rng, true) : random_empty(g, rng);
}
// after the planned events: a (further) threat on the cheapest line, usually one through revealed stones; steals
static int followup_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, AdvState& as) {
    if (!threat_ok(g, as.l2, as.e2) && !pick_threat(g, rng, as.busyW, -1, -1, as.l2, as.e2)) return random_empty(g, rng);
    int cells[6];
    int n = threat_cells(g, as.l2, as.e2, cells);
    if (n == 0) return random_empty(g, rng);
    if (n == 1) return cells[0];   // fires
    int cands[4], k = 0;
    cands[k++] = cells[rng.below(n)];
    cands[k++] = cells[rng.below(n)];
    cands[k++] = steal_now(g, ctx, ts);
    cands[k++] = steal_next(g, ctx, ts);
    return choose_alive(g, cands, k, cands[0], ctx, ts);
}

static int plan_search(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, int mode, AdvState& as);
static int st_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, AdvState& as);
// The three adversaries below have a scripted half (cheap, follows the description literally) and a searched half
// (the playout search of the tree adversary with a bias towards the events the adversary is named after; it finds
// the variants that need help from sigma's own moves, e.g. a danger pair at round 9 or 10).
// "danger": two live lines with 4 stones each through an empty cell, the last stone creates the pair; lines
// with few busy cells; 0..2 filler moves (steals, busy cells, random) in between.
static int danger_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, AdvState& as) {
    if (!as.init) {
        as.init = true;
        int r = rng.below(100);
        as.fillers = r < 40 ? 0 : (r < 75 ? 1 : 2);
        r = rng.below(100);
        as.busyW = r < 40 ? 6 : (r < 60 ? 3 : (r < 80 ? 1 : 0));
        as.greedyEnd = rng.below(10) < 3;
        as.variant = rng.below(4);   // 0: after the danger go for a threat; 1..3: further dangers / threats mixed
        as.searched = !as.noSearch && rng.below(10) < 6;
    }
    if (g.pairing) return end_move(g, rng, as);
    if (as.searched) return plan_search(g, ctx, ts, rng, P, 1 /* SM_DANGER */, as);
    if (g.nDanger == 0 || as.variant >= 2) {
        if (!danger_ok(g, as) && !pick_danger(g, rng, as.busyW, as)) return followup_move(g, ctx, ts, rng, as);
        int cells[12];
        int n = danger_cells(g, as, cells);
        if (n > 0) {
            Bits keep = EMPTYSET;
            keep.set(as.dc); keep.set(as.dsa); keep.set(as.dsb);
            for (int i = 0; i < n; i++) keep.set(cells[i]);
            bool spend = as.fillers > 0 && rng.below(n + as.fillers) < as.fillers;
            int cands[5], k = 0;
            if (spend) {
                cands[k++] = filler_move(g, ctx, ts, rng, keep);
                as.fillers--;
            } else if (n == 1)
                return cells[0];   // creates the danger pair
            int build = cells[rng.below(n)];
            if (n > 1) {
                cands[k++] = build;
                cands[k++] = cells[rng.below(n)];
            }
            cands[k++] = steal_now(g, ctx, ts);
            return choose_alive(g, cands, k, n == 1 ? cells[0] : build, ctx, ts);
        }
    }
    return followup_move(g, ctx, ts, rng, as);
}

// "double": a threat as early as possible (round 5..7), then a second threat through one of the revealed stones;
// variants: the two lines built interleaved; threat then danger (cannot succeed while sigma's TABLE mode lasts at
// most 10 rounds: 5 + 3 + 3 stones are needed); threat, steals, threat.
static int double_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, AdvState& as) {
    if (!as.init) {
        as.init = true;
        as.variant = rng.below(8);   // 0..2 sequential, 3..5 interleaved, 6 threat then danger, 7 threat, steals, threat
        as.fillers = rng.below(3);
        if (as.variant >= 3 && as.variant <= 5) as.fillers = rng.below(2);
        int r = rng.below(100);
        as.busyW = r < 60 ? 6 : (r < 85 ? 2 : 0);
        as.greedyEnd = rng.below(10) < 3;
        as.searched = !as.noSearch && rng.below(10) < 6;
        if (!as.searched && !g.pairing && pick_threat(g, rng, as.busyW, -1, -1, as.l1, as.e1)) {
            int cells[6];
            int n = threat_cells(g, as.l1, as.e1, cells);
            if (n > 0) {
                as.first = cells[rng.below(n)];
                if (pick_threat(g, rng, as.busyW, as.l1, as.first, as.l2, as.e2)) {
                    // the second line must keep the shared stone
                    for (int tries = 0; tries < 8 && as.e2 == as.first; tries++) as.e2 = LN[as.l2][rng.below(6)];
                    if (as.e2 == as.first || !is_empty_cell(g, as.e2)) as.l2 = -1;
                }
            }
        }
    }
    if (g.pairing) return end_move(g, rng, as);
    if (as.searched) return plan_search(g, ctx, ts, rng, P, 2 /* SM_DOUBLE */, as);
    if (as.first >= 0 && is_empty_cell(g, as.first) && threat_ok(g, as.l1, as.e1)) {
        int c = as.first;
        as.first = -1;
        return c;   // the shared stone first: the two lines can never form a danger pair
    }
    as.first = -1;
    if (g.nThreat == 0) {
        if (!threat_ok(g, as.l1, as.e1)) {
            if (!pick_threat(g, rng, as.busyW, -1, -1, as.l1, as.e1)) return random_empty(g, rng);
        }
        int c1[6], c2[6];
        int n1 = threat_cells(g, as.l1, as.e1, c1), n2 = 0;
        bool second = threat_ok(g, as.l2, as.e2) && as.l2 != as.l1;
        if (second) n2 = threat_cells(g, as.l2, as.e2, c2);
        Bits keep = EMPTYSET;
        keep.set(as.e1);
        for (int i = 0; i < n1; i++) keep.set(c1[i]);
        if (second) {
            keep.set(as.e2);
            for (int i = 0; i < n2; i++) keep.set(c2[i]);
        }
        int cands[5], k = 0, main = -1;
        bool spend = as.fillers > 0 && rng.below(n1 + as.fillers) < as.fillers;
        if (spend) {
            as.fillers--;
            cands[k++] = filler_move(g, ctx, ts, rng, keep);
        }
        bool interleave = as.variant >= 3 && as.variant <= 5 && second && n2 > 1;
        if (interleave && rng.below(n1 + n2 - 1) >= n1) main = c2[rng.below(n2)];
        else main = c1[rng.below(n1)];
        if (n1 == 1 && main == c1[0] && !spend) return main;   // the threat
        cands[k++] = main;
        if (n1 > 1) cands[k++] = c1[rng.below(n1)];
        cands[k++] = steal_now(g, ctx, ts);
        return choose_alive(g, cands, k, main, ctx, ts);
    }
    // after the first threat
    if (as.variant == 6 && g.nDanger == 0) {
        if (danger_ok(g, as) || pick_danger(g, rng, as.busyW, as)) {
            int cells[12];
            int n = danger_cells(g, as, cells);
            if (n == 1) return cells[0];
            if (n > 1) {
                int cands[3] = {cells[rng.below(n)], cells[rng.below(n)], steal_now(g, ctx, ts)};
                return choose_alive(g, cands, 3, cands[0], ctx, ts);
            }
        }
    }
    if (as.variant == 7 && as.fillers > 0) {
        as.fillers--;
        int cands[3] = {steal_now(g, ctx, ts), steal_next(g, ctx, ts), random_empty_busy(g, rng)};
        int c = choose_alive(g, cands, 3, -1, ctx, ts);
        if (c >= 0) return c;
    }
    return followup_move(g, ctx, ts, rng, as);
}

// "corner": takes busy cells aggressively (the four corners of a face, antipodes of SP's stones, the cells
// sigma is about to play) for 2..5 rounds to force occupied reveals and non-busy table moves, then builds
// threats / dangers, preferably on lines through its own stones.
static int corner_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, AdvState& as) {
    if (!as.init) {
        as.init = true;
        as.face = rng.below(6);
        as.stealRounds = 2 + rng.below(4);
        as.variant = rng.below(3);   // 0, 1: threats afterwards; 2: a danger afterwards
        as.busyW = rng.below(3);     // the lines through corners are the cheap ones here
        as.greedyEnd = rng.below(10) < 3;
        as.searched = !as.noSearch && rng.below(10) < 5;
        as.stealThreat = !as.noSearch && !as.searched && rng.below(5) < 3;
        if (as.stealThreat) as.init = false;   // st_move makes its own choices
    }
    if (as.stealThreat) return st_move(g, ctx, ts, rng, as);
    if (g.pairing) return end_move(g, rng, as);
    if (as.searched) return plan_search(g, ctx, ts, rng, P, 3 /* SM_CORNER */, as);
    if (g.t < as.stealRounds) {
        int corners[4], nc = 0;
        int axis = as.face / 2, side = (as.face & 1) ? 5 : 0;
        for (int u = 0; u < 2; u++)
            for (int v = 0; v < 2; v++) {
                int co[3];
                co[axis] = side;
                co[(axis + 1) % 3] = u ? 5 : 0;
                co[(axis + 2) % 3] = v ? 5 : 0;
                int c = cid(co[0], co[1], co[2]);
                if (is_empty_cell(g, c)) corners[nc++] = c;
            }
        int cands[4], k = 0;
        int r = rng.below(100);
        int corner = nc ? corners[rng.below(nc)] : -1;
        int anti = (g.lastSP >= 0 && is_empty_cell(g, ANTI[g.lastSP]) && rng.below(2)) ? (int)ANTI[g.lastSP] : random_antipode(g, rng);
        int sn = steal_now(g, ctx, ts), sx = steal_next(g, ctx, ts), rb = random_empty_busy(g, rng);
        if (r < 55) { cands[k++] = corner; cands[k++] = anti; cands[k++] = sn; }
        else if (r < 75) { cands[k++] = anti; cands[k++] = corner; cands[k++] = sn; }
        else if (r < 90) { cands[k++] = sn; cands[k++] = corner; cands[k++] = anti; }
        else { cands[k++] = sx; cands[k++] = sn; cands[k++] = corner; }
        cands[k++] = rb;
        int c = choose_alive(g, cands, k, -1, ctx, ts);
        if (c >= 0) return c;
    }
    if (as.variant == 2 && g.nDanger == 0) {
        if (danger_ok(g, as) || pick_danger(g, rng, as.busyW, as)) {
            int cells[12];
            int n = danger_cells(g, as, cells);
            if (n == 1) return cells[0];
            if (n > 1) {
                int cands[3] = {cells[rng.below(n)], cells[rng.below(n)], steal_now(g, ctx, ts)};
                return choose_alive(g, cands, 3, cands[0], ctx, ts);
            }
        }
    }
    return followup_move(g, ctx, ts, rng, as);
}

// "steal + threat" (the scripted policy behind most of the proof tree: by far the most leaves of the tree have
// one threat line and 3 or 4 stones revealed by occupied table moves): FP takes 1..5 times the cell sigma is
// about to play (occupied reveal) and builds one threat line with quiet stones, in a random schedule; after the
// threat the remaining steals, then further threats.
static int st_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, AdvState& as) {
    if (!as.init) {
        as.init = true;
        int r = rng.below(100);
        as.fillers = r < 8 ? 1 : (r < 22 ? 2 : (r < 47 ? 3 : (r < 85 ? 4 : 5)));
        as.variant = rng.below(5);   // 0: steals first; 1..3: random interleaving; 4: threat first
        as.busyW = rng.below(2) ? 0 : rng.below(7);
        as.stealRounds = rng.below(2) ? 0 : 24;   // weight of the missing stones in the choice of the line (0: any line)
        as.greedyEnd = rng.below(10) < 3;
    }
    if (g.pairing) return end_move(g, rng, as);
    if (g.nThreat > 0) {
        if (as.fillers > 0) {
            as.fillers--;
            int c = steal_random(g, ctx, ts, rng);
            if (c >= 0 && is_empty_cell(g, c)) return c;
        }
        return followup_move(g, ctx, ts, rng, as);
    }
    if (!threat_ok(g, as.l1, as.e1) && !pick_threat(g, rng, as.busyW, -1, -1, as.l1, as.e1, as.stealRounds)) return random_empty(g, rng);
    int cells[6];
    int b = threat_cells(g, as.l1, as.e1, cells), sLeft = as.fillers;
    bool doSteal;
    if (sLeft == 0) doSteal = false;
    else if (as.variant == 0) doSteal = true;
    else if (as.variant == 4) doSteal = false;
    else doSteal = rng.below(sLeft + b) < sLeft;
    int build = cells[rng.below(b)];
    if (doSteal) {
        int c = steal_random(g, ctx, ts, rng);
        if (c >= 0 && is_empty_cell(g, c) && c != as.e1) {
            int pr = probe(g, c, ctx, ts);
            if (pr == 0 || pr == 2) {
                as.fillers--;
                return c;
            }
        }
    }
    if (b == 1) return build;   // the threat
    int pr = probe(g, build, ctx, ts);
    if (pr == 1 && sLeft > 0) {   // the quiet stone would end TABLE mode: try a steal instead
        int c = steal_now(g, ctx, ts);
        if (c >= 0 && is_empty_cell(g, c) && c != as.e1 && probe(g, c, ctx, ts) != 1) {
            as.fillers--;
            return c;
        }
    }
    return build;
}

// ---- "tree": randomised best-first search over complete TABLE phases, with sigma (the loaded table) as oracle.
// A playout continues the game with a random style (forcing moves, steals of the cells sigma is about to play,
// threat / danger building, cells of the cheapest LOST pattern, busy cells, random cells) until sigma reaches a
// leaf.  The best playout so far is mutated (a random prefix is kept).  The value of a playout rewards table keys
// and leaf keys that no game has hit yet (coverage snapshot), a late leaf, forced events (two threats most),
// occupied reveals, non-busy table moves and near misses (rounds after which a live line holds 4 FP stones, i.e.
// only 2 cells are missing).  A playout that ends in a bug is played out for real at once.
// The same search with other style and value biases is the "searched" half of danger / double / corner:
//   SM_DANGER  danger pairs, most of all at round 9 or 10 (they need a reveal that postpones the leaf)
//   SM_DOUBLE  two threats, the second one late
//   SM_CORNER  occupied reveals, rounds with several reveals, non-busy table moves
enum { SM_TREE = 0, SM_DANGER, SM_DOUBLE, SM_CORNER };
constexpr int NSTYLE = 8;
struct PoStyle {
    double w[NSTYLE];   // force, steal now, pre-steal, threat build, danger build, busy cell, random, LOST-pattern cell
    AdvState as;
};
static void random_style(Rng& rng, PoStyle& ps, int mode) {
    static const double base[NSTYLE] = {1.0, 0.9, 0.5, 1.0, 0.7, 0.3, 0.3, 0.4};
    for (int i = 0; i < NSTYLE; i++) {
        double u = rng.uniform();
        ps.w[i] = base[i] * u * u;
    }
    int r = rng.below(10);
    if (r == 0) ps.w[3] += 3;          // threat specialist
    else if (r == 1) ps.w[4] += 3;     // danger specialist
    else if (r == 2) ps.w[1] += 2;     // thief
    else if (r == 3) { ps.w[1] += 1; ps.w[2] += 1.5; }
    else if (r == 4) ps.w[7] += 2;     // tactician
    if (mode == SM_DANGER) { ps.w[4] += 2.5 + 2 * rng.uniform(); ps.w[3] *= 0.3; ps.w[0] *= 0.5; }
    if (mode == SM_DOUBLE) { ps.w[3] += 2.5 + 2 * rng.uniform(); ps.w[4] *= 0.3; }
    if (mode == SM_CORNER) { ps.w[1] += 1.5 * rng.uniform(); ps.w[2] += 1.5 * rng.uniform(); ps.w[5] += 1.5 * rng.uniform(); }
    ps.as = AdvState();
    ps.as.busyW = rng.below(8);
}
static int playout_move(const Game& h, const Ctx& ctx, TS& ts, Rng& rng, PoStyle& ps) {
    int forcing[NC], nf = 0;
    int w = find_winner(h, forcing, &nf);
    if (w >= 0) return w;   // breaks admissibility or completes a line: a bug if it works
    double tot = 0;
    for (int i = 0; i < NSTYLE; i++) tot += ps.w[i];
    double r = rng.uniform() * tot;
    int type = 6;
    for (int i = 0; i < NSTYLE; i++) {
        if (r < ps.w[i]) { type = i; break; }
        r -= ps.w[i];
    }
    int c = -1;
    AdvState& as = ps.as;
    switch (type) {
        case 0:
            if (nf) c = forcing[rng.below(nf)];
            break;
        case 1: c = steal_now(h, ctx, ts); break;
        case 2: c = steal_ahead(h, ctx, ts, 1 + rng.below(6)); break;
        case 3: {
            if (!threat_ok(h, as.l1, as.e1)) pick_threat(h, rng, as.busyW, -1, -1, as.l1, as.e1);
            if (threat_ok(h, as.l1, as.e1)) {
                int cells[6];
                int n = threat_cells(h, as.l1, as.e1, cells);
                if (n) c = cells[rng.below(n)];
            }
            break;
        }
        case 4: {
            if (!danger_ok(h, as)) pick_danger(h, rng, as.busyW, as);
            if (danger_ok(h, as)) {
                int cells[12];
                int n = danger_cells(h, as, cells);
                if (n) c = cells[rng.below(n)];
            }
            break;
        }
        case 5: c = random_empty_busy(h, rng); break;
        case 7: {
            Plan pl;
            make_plan(h, rng, pl, true);
            if (pl.ncand) c = pl.cand[0];
            break;
        }
        default: break;
    }
    if (c < 0 || !is_empty_cell(h, c)) c = random_empty(h, rng);
    return c;
}
static double playout_value(const Game& h, Rng& rng, int mode) {
    double v = 100.0 * h.novelQ + 40.0 * h.novelPQ + 150.0 * h.leafNovel + 60.0 * h.leafNovelP;
    v += 6.0 * (h.leafRound >= 0 ? h.leafRound : h.t);
    v += 10.0 * h.nThreat + 14.0 * h.nDanger + (h.nThreat >= 2 ? 25.0 : 0.0) + (h.nThreat && h.nDanger ? 200.0 : 0.0);
    v += 2.0 * h.nOcc + 6.0 * h.nMultiOcc + 5.0 * h.nNonBusy + 3.0 * h.near4;
    int lastT = 0, lastD = 0;
    for (int i = 0; i < h.nEv; i++) {
        if (h.evKind[i] == 1) lastT = h.evRound[i];
        else lastD = h.evRound[i];
    }
    if (mode == SM_DANGER) v += 300.0 * (h.nDanger > 0) + 300.0 * (lastD >= 9) + 200.0 * (lastD >= 10);
    if (mode == SM_DOUBLE) v += 40.0 * h.nThreat + 300.0 * (h.nThreat >= 2) + 150.0 * (h.nThreat >= 2 && lastT >= 10) + 60.0 * (lastT >= 10);
    if (mode == SM_CORNER) v += 20.0 * h.nOcc + 60.0 * h.nMultiOcc + 80.0 * h.nNonBusy;
    return v + 3.0 * rng.uniform();
}
// the planned move for this round, searching (again) when there is no valid plan.
// A playout is produced either by a random style (playout_move) or by the scripted policy of danger / double /
// corner with fresh random choices (lines, cells left empty, fillers); the mix depends on the mode.
static int plan_search(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, int mode, AdvState& as) {
    if (as.planT0 >= 0) {
        int i = g.t - as.planT0;
        if (i >= 0 && i < as.planLen && is_empty_cell(g, as.plan[i])) return as.plan[i];
    }
    static const int GENW[4][5] = {   // weights of: random style, scripted danger, double, corner, steal + threat
        {20, 12, 13, 10, 45}, {25, 75, 0, 0, 0}, {20, 0, 60, 0, 20}, {20, 0, 0, 30, 50}};
    const int MAXLEN = (int)sizeof as.plan;
    uint8_t best[sizeof as.plan], seq[sizeof as.plan];
    int bestLen = 0;
    double bestV = -1;
    bool bug = false;
    for (int it = 0; it < P.playouts && !bug; it++) {
        Game h = g;
        int len = 0;
        if (bestLen > 0 && rng.below(10) < 5) {   // mutate the best playout: keep a random proper prefix
            int keep = rng.below(bestLen);
            for (int i = 0; i < keep && h.st == ST_OK && !h.pairing; i++) {
                play(h, best[i], ctx, ts, nullptr);
                seq[len++] = best[i];
            }
        }
        int r = rng.below(100), gen = 0;
        for (; gen < 4; gen++) {
            if (r < GENW[mode][gen]) break;
            r -= GENW[mode][gen];
        }
        PoStyle ps;
        random_style(rng, ps, mode);
        AdvState sub;
        sub.noSearch = true;
        while (h.st == ST_OK && !h.pairing && len < MAXLEN) {
            int x;
            if (gen == 0) x = playout_move(h, ctx, ts, rng, ps);
            else {
                x = find_winner(h);
                if (x < 0) {
                    if (gen == 1) x = danger_move(h, ctx, ts, rng, P, sub);
                    else if (gen == 2) x = double_move(h, ctx, ts, rng, P, sub);
                    else if (gen == 3) x = corner_move(h, ctx, ts, rng, P, sub);
                    else x = st_move(h, ctx, ts, rng, sub);
                }
            }
            if (x < 0 || !is_empty_cell(h, x)) x = random_empty(h, rng);
            play(h, x, ctx, ts, nullptr);
            seq[len++] = (uint8_t)x;
        }
        double v = is_bug(h.st) ? 1e18 : playout_value(h, rng, mode);
        if (is_bug(h.st)) bug = true;
        if (v > bestV) {
            bestV = v;
            bestLen = len;
            memcpy(best, seq, len);
        }
        // good enough: stop early when the playout reaches a leaf no game has hit yet (plus the event the mode
        // is about), so that the CPU time goes into more games instead of more playouts
        bool modeEvent = mode == SM_TREE || (mode == SM_DANGER && h.nDanger > 0) || (mode == SM_DOUBLE && h.nThreat >= 2) ||
                         (mode == SM_CORNER && h.nOcc >= 2);
        if (modeEvent && ((h.leafNovel && h.novelQ >= 2) || (it >= 3 && h.leafNovel && h.novelQ >= 1) || (it >= 7 && (h.leafNovel || h.novelQ >= 1)))) break;
    }
    if (bestLen == 0) return random_empty(g, rng);
    as.planT0 = g.t;
    as.planLen = bestLen;
    memcpy(as.plan, best, bestLen);
    return as.plan[0];
}
static int tree_move(const Game& g, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, AdvState& as) {
    if (!as.init) {
        as.init = true;
        as.greedyEnd = rng.below(10) < 3;
    }
    if (g.pairing) return end_move(g, rng, as);
    return plan_search(g, ctx, ts, rng, P, SM_TREE, as);
}

static int choose_move(const Game& g, int adv, const Ctx& ctx, TS& ts, Rng& rng, const Params& P, AdvState& as) {
    switch (adv) {
        case ADV_RANDOM:
            return random_empty(g, rng);
        case ADV_GREEDY:
            return best_scored(g, rng, false);
        case ADV_STEAL: {
            if (g.lastSP >= 0 && is_empty_cell(g, ANTI[g.lastSP])) return ANTI[g.lastSP];
            int eb[NB], n = 0;
            for (int i = 0; i < NB; i++)
                if (is_empty_cell(g, BUSY[i])) eb[n++] = BUSY[i];
            if (n) return eb[rng.below(n)];
            return best_scored(g, rng, false);
        }
        case ADV_THREAT: {
            if (!g.pairing) {
                int w = find_winner(g);
                if (w >= 0) return w;
            }
            if (rng.uniform() < P.p) return random_empty(g, rng);
            return best_scored(g, rng, true);
        }
        case ADV_SEARCH:
            return search_move(g, ctx, ts, rng, P);
        case ADV_DANGER:
            return danger_move(g, ctx, ts, rng, P, as);
        case ADV_DOUBLE:
            return double_move(g, ctx, ts, rng, P, as);
        case ADV_CORNER:
            return corner_move(g, ctx, ts, rng, P, as);
        case ADV_TREE:
            return tree_move(g, ctx, ts, rng, P, as);
    }
    return random_empty(g, rng);
}

// checks of a finished game that do not depend on the incremental data: 216 distinct moves, FP and SP
// alternate, the board is full, every line holds an SP stone (so FP owns none)
static bool final_position_ok(const Game& g) {
    if (g.nm != NC || g.ns != NC) return false;
    Bits X = EMPTYSET, B = EMPTYSET;
    for (int i = 0; i < g.nm; i++) {
        int c = g.moves[i];
        if (X.test(c) || B.test(c)) return false;
        if (i & 1) B.set(c); else X.set(c);
    }
    if (X != g.X || B != g.B || X.count() != NC / 2 || B.count() != NC / 2) return false;
    for (int l = 0; l < NL; l++)
        if (!LB[l].meets(B)) return false;
    return true;
}

// one complete game; script = forced first FP moves (skipped when the cell is not empty).
// scriptOnly: the game stops (status "ok", not counted as finished) when the script is exhausted.
static Status run_game(Game& g, int adv, const Ctx& ctx, TS& ts, Rng& rng, const Params& P,
                       const std::vector<int>* script, Stats& st, bool scriptOnly = false, FILE* trace = nullptr) {
    game_init(g);
    AdvState as;
    Status s = round_start(g, ctx, ts, true);
    while (s == ST_OK) {
        int x = -1;
        if (script && g.t < (int)script->size() && is_empty_cell(g, (*script)[g.t])) x = (*script)[g.t];
        if (x < 0 && scriptOnly) break;
        if (x < 0) x = choose_move(g, adv, ctx, ts, rng, P, as);
        bool wasPairing = g.pairing;
        Bits F0 = g.F;
        s = play(g, x, ctx, ts, &st);   // an occupied or out-of-range x is caught there (internal_error)
        if (trace) {
            static const char* ev[] = {"table", "threat", "danger", "pairing"};
            Bits dF = EMPTYSET;
            for (int i = 0; i < 4; i++) dF.w[i] = g.F.w[i] & ~F0.w[i];
            fprintf(trace, "round %d: FP %d  SP %d  [%s%s] occupied_reveals=%d revealed=%s |F|=%d status=%s%s\n", g.t, x,
                    (g.nm % 2 == 0 && g.nm > 0) ? (int)g.moves[g.nm - 1] : -1, wasPairing ? "PAIRING " : "TABLE ",
                    wasPairing ? "" : ev[g.lastEvent], (int)g.lastOcc, set_str(dF).c_str(), g.F.count(), status_name(s),
                    (!wasPairing && g.pairing) ? "  -> leaf, PAIRING mode from the next round" : "");
        }
    }
    if (s == ST_END && !final_position_ok(g)) s = internal_error(g, IE_FINAL_POSITION);
    if (s == ST_OK && scriptOnly) {   // script exhausted: not a finished game
        st.games++;
        st.rounds += g.t;
        return s;
    }
    st.absorb(g);
    return s;
}

struct BugRec {
    long game;
    std::string json;
};
static std::string bug_json(long idx, u64 gseed, const char* advName, const Game& g) {
    std::string s = "{\"game\":" + std::to_string(idx) + ",\"game_seed\":" + std::to_string(gseed) +
                    ",\"adversary\":\"" + advName + "\",\"kind\":\"" + status_name(g.st) + "\"";
    if (g.st == ST_INTERNAL) s += std::string(",\"reason\":\"") + internal_name(g.internal) + "\"";
    s += ",\"round\":" + std::to_string(g.t) + ",\"mode\":\"" + (g.pairing ? "PAIRING" : "TABLE") +
         "\",\"leaf_round\":" + std::to_string(g.leafRound) + ",\"B\":\"" + set_str(g.B) +
         "\",\"F\":\"" + set_str(g.F) + "\",\"X\":\"" + set_str(g.X) + "\"";
    if (g.st == ST_HOLE || g.st == ST_PAIRING_FAIL || g.st == ST_ILLEGAL) {
        CanonRes c = canon(g.B, g.F);
        s += ",\"canonical_state\":\"" + std::to_string(g.t) + " " + set_str(c.B) + " " + set_str(c.F) + "\"";
    }
    s += ",\"fp_moves\":\"";
    for (int i = 0; i < g.nm; i += 2) {
        if (i) s += ',';
        s += std::to_string((int)g.moves[i]);
    }
    s += "\",\"moves\":[";
    for (int i = 0; i < g.nm; i++) {
        if (i) s += ',';
        s += std::to_string((int)g.moves[i]);
    }
    s += "]}";
    return s;
}

static inline u64 game_seed(u64 seed, long idx) { return mixa(seed * 0x9e3779b97f4a7c15ULL + 0x1234567ULL) ^ mixb((u64)idx + 0x51ed27ULL); }

static int pick_adv(int adv, Rng& rng, const Params& P) {
    if (adv != ADV_MIX) return adv;
    double tot = 0;
    for (int i = 0; i < ADV_COUNT; i++) tot += P.mixw[i];
    double r = rng.uniform() * tot;
    int last = 0;
    for (int i = 0; i < ADV_COUNT; i++) {
        if (P.mixw[i] <= 0) continue;
        if (r < P.mixw[i]) return i;
        r -= P.mixw[i];
        last = i;
    }
    return last;
}

// ----------------------------------------------------------------------------------------------------
// self-test
// ----------------------------------------------------------------------------------------------------
static int g_fail = 0;
#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            g_fail++;                                     \
            printf("  FAIL: ");                           \
            printf(__VA_ARGS__);                          \
            printf("\n");                                 \
        }                                                 \
    } while (0)

static double wall_now() { return omp_get_wtime(); }
// CPU time of the calling thread: the machine is shared, wall-clock times would measure the other jobs
static double now() {
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static const int T8C[8] = {0, 215, 46, 169, 92, 123, 180, 35};   // (0,0,0),(5,5,5),(1,1,4),(4,4,1),(2,3,2),(3,2,3),(5,0,0),(0,5,5)

static Bits set_of(const int* c, int n) {
    Bits b = EMPTYSET;
    for (int i = 0; i < n; i++) b.set(c[i]);
    return b;
}
static void random_state(Rng& rng, int nb, int nf, bool busyBias, Bits& B, Bits& F) {
    B = F = EMPTYSET;
    while (B.count() < nb) {
        int c = (busyBias && rng.below(4) != 0) ? BUSY[rng.below(NB)] : rng.below(NC);
        B.set(c);
    }
    while (F.count() < nf) {
        int c = rng.below(NC);
        if (!B.test(c)) F.set(c);
    }
}
static void setup_game(Game& g, const Bits& B, const Bits& X, const Bits& F, int t) {
    game_init(g);
    uint8_t cs[NC];
    int n = cells_of(B, cs);
    for (int i = 0; i < n; i++) put_b(g, cs[i]);
    n = cells_of(X, cs);
    for (int i = 0; i < n; i++) put_x(g, cs[i]);
    g.F = F;
    g.t = t;
}

struct SynTable {
    std::string tbl, leaves;   // file paths
    int bstar[8];
    int decoy;
};
static void write_text(const std::string& path, const std::string& s) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "cannot write %s\n", path.c_str());
        exit(2);
    }
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
}
static std::string q_line(int t, const Bits& B, const Bits& F, int realMove) {
    CanonRes c = canon(B, F);
    return std::to_string(t) + " " + set_str(c.B) + " " + set_str(c.F) + " " + std::to_string((int)GM[c.g][realMove]) + "\n";
}
static std::string p_line(int t, const Bits& B, const Bits& F) {
    CanonRes c = canon(B, F);
    return std::to_string(t) + " " + set_str(c.B) + " " + set_str(c.F) + "\n";
}
// synthetic table: SP plays h(T8) in the given order.  variant 0: F stays empty, leaf P(h(T8),{},8).
// variant 1: at round 4 the table first proposes the decoy h((1,1,1)); if FP holds it, it is revealed and
// the table continues with F = {decoy}; leaf P(h(T8),{decoy},8).
// variant 2 (negative): like 0 but the round-8 entry is missing (hole).
// variant 3 (negative): like 0 but P(first 7 cells, {}, 7) is declared a leaf (slack 1: almost never a pairing).
static SynTable make_syn(const std::string& dir, const std::string& name, int h, int variant) {
    SynTable s;
    for (int j = 0; j < 8; j++) s.bstar[j] = GM[h][T8C[j]];
    s.decoy = GM[h][cid(1, 1, 1)];
    std::string tbl, lv;
    Bits B = EMPTYSET, F = EMPTYSET;
    for (int j = 0; j < 8; j++) {
        int t = j + 1;
        if (variant == 1 && j == 3) {
            tbl += q_line(t, B, F, s.decoy);
            F.set(s.decoy);
        }
        if (!(variant == 2 && j == 7)) tbl += q_line(t, B, F, s.bstar[j]);
        B.set(s.bstar[j]);
        if (variant == 3 && j == 6) lv += p_line(7, B, F);
    }
    lv += p_line(8, B, F);
    s.tbl = dir + "/" + name + ".tbl";
    s.leaves = dir + "/" + name + ".leaves";
    write_text(s.tbl, "# synthetic self-test table\n" + tbl);
    write_text(s.leaves, lv);
    return s;
}
// Plays a scripted game (FP moves fp[], SP table proposals spTable[] in order) and writes the table entries
// sigma will ask for (threats and dangers are answered by forced_reply, a proposal on an FP stone is recorded
// as an occupied reveal) and the final P-state as the leaf.  Returns false if the script does not work out.
static bool record_syn(const std::vector<int>& fp, const std::vector<int>& spTable, std::string& tbl, std::string& leaf,
                       Bits& Bfin, Bits& Ffin) {
    Game g;
    game_init(g);
    size_t k = 0;
    tbl.clear();
    for (size_t r = 0; r < fp.size(); r++) {
        int x = fp[r];
        if (!is_empty_cell(g, x)) return false;
        put_x(g, x);
        g.t++;
        for (int i = 0; i < NLC[x]; i++)
            if (g.xc[LC[x][i]] == 6) return false;
        int reply = forced_reply(g, nullptr);
        while (reply < 0) {
            if (k >= spTable.size()) return false;
            int m = spTable[k++];
            if (g.B.test(m) || g.F.test(m)) return false;
            tbl += q_line(g.t, g.B, g.F, m);
            if (g.X.test(m)) g.F.set(m);
            else reply = m;
        }
        put_b(g, reply);
        if (!admissible_counts(g)) return false;
    }
    if (k != spTable.size()) return false;
    leaf = p_line(g.t, g.B, g.F);
    Bfin = g.B;
    Ffin = g.F;
    return true;
}

// a random 8-move FP opening that avoids the cells of Bst; biased towards heavy lines
static std::vector<int> random_script(Rng& rng, const Bits& Bst, int forced) {
    std::vector<int> live;
    for (int l = 0; l < NL; l++)
        if (!LB[l].meets(Bst)) live.push_back(l);
    for (;;) {
        Bits X = EMPTYSET;
        if (forced >= 0) X.set(forced);
        int style = rng.below(4);
        auto addLine = [&](int k) {
            int l = live[rng.below((int)live.size())];
            int skip1 = rng.below(6), skip2 = rng.below(6);
            int added = 0;
            for (int i = 0; i < 6 && added < k; i++) {
                if (k == 4 && (i == skip1)) continue;
                if (k == 3 && (i == skip1 || i == skip2)) continue;
                X.set(LN[l][i]);
                added++;
            }
        };
        if (style == 1) addLine(4);
        if (style == 2) { addLine(3); addLine(3); }
        if (style == 3) { addLine(4); addLine(4); }
        if (X.count() > 8 || X.meets(Bst)) continue;
        while (X.count() < 8) {
            int c = rng.below(NC);
            if (!Bst.test(c)) X.set(c);
        }
        if (!admissible(Bst, X)) continue;
        uint8_t cs[NC];
        int n = cells_of(X, cs);
        std::vector<int> v(cs, cs + n);
        for (int i = n - 1; i > 0; i--) std::swap(v[i], v[rng.below(i + 1)]);
        if (forced >= 0) {   // the decoy must be played in rounds 1..4
            int pos = (int)(std::find(v.begin(), v.end(), forced) - v.begin());
            std::swap(v[pos], v[rng.below(4)]);
        }
        return v;
    }
}

static int selftest(u64 seed, const std::string& tmpdirArg, bool keep) {
    Rng rng(seed);
    double t0 = now();
    printf("== sim self-test (seed %" PRIu64 "); all timings are single-thread CPU time ==\n", seed);

    // ---------------- A. geometry
    printf("[A] geometry\n");
    CHECK(g_nlines == 148, "148 lines");
    CHECK(g_axis == 108 && g_face == 36 && g_space == 4, "108/36/4 split, got %d/%d/%d", g_axis, g_face, g_space);
    {
        int share2 = 0;
        for (int a = 0; a < NL; a++)
            for (int b = a + 1; b < NL; b++)
                if ((LB[a] & LB[b]).count() > 1) share2++;
        CHECK(share2 == 0, "two lines share more than one cell (%d pairs)", share2);
        for (int l = 0; l + 1 < NL; l++)
            CHECK(memcmp(LN[l], LN[l + 1], 6) < 0, "lines not sorted at %d", l);
        int nb = 0, bad = 0, tot = 0;
        for (int c = 0; c < NC; c++) {
            tot += NLC[c];
            if (ISBUSY[c]) {
                nb++;
                if (NLC[c] != 7) bad++;
            } else if (NLC[c] != 3 && NLC[c] != 4)
                bad++;
            if (ANTI[ANTI[c]] != c || ANTI[c] == c) bad++;
            if (ISBUSY[c] != ISBUSY[ANTI[c]]) bad++;
        }
        CHECK(nb == 24, "24 busy cells, got %d", nb);
        CHECK(bad == 0, "line counts per cell (%d bad)", bad);
        CHECK(tot == 148 * 6, "incidence total %d", tot);
        // maps
        CHECK(g_nmaps == 1152, "1152 maps");
        std::map<std::array<uint8_t, 6>, int> lineId;
        for (int l = 0; l < NL; l++) {
            std::array<uint8_t, 6> a;
            memcpy(a.data(), LN[l], 6);
            lineId[a] = l;
        }
        int notBij = 0, notLine = 0, busyBad = 0, antiBad = 0;
        std::set<std::array<uint8_t, NC>> all;
        for (int g = 0; g < NG; g++) {
            Bits im = EMPTYSET;
            std::array<uint8_t, NC> arr;
            for (int c = 0; c < NC; c++) {
                im.set(GM[g][c]);
                arr[c] = GM[g][c];
                if (GI[g][GM[g][c]] != c) notBij++;
                if (ISBUSY[c] != ISBUSY[GM[g][c]]) busyBad++;
                if (GM[g][ANTI[c]] != ANTI[GM[g][c]]) antiBad++;
            }
            all.insert(arr);
            if (im != FULLSET) notBij++;
            for (int l = 0; l < NL; l++) {
                std::array<uint8_t, 6> a;
                for (int i = 0; i < 6; i++) a[i] = GM[g][LN[l][i]];
                std::sort(a.begin(), a.end());
                if (!lineId.count(a)) notLine++;
            }
        }
        CHECK(all.size() == 1152, "maps distinct");
        CHECK(notBij == 0, "maps are bijections");
        CHECK(notLine == 0, "maps send lines to lines (%d failures)", notLine);
        CHECK(busyBad == 0 && antiBad == 0, "maps preserve busy cells and antipodes");
        // closure: composition and inverse of maps are maps (random sample of pairs, all inverses)
        int notClosed = 0;
        for (int g = 0; g < NG; g++) {
            std::array<uint8_t, NC> arr;
            for (int c = 0; c < NC; c++) arr[c] = GI[g][c];
            if (!all.count(arr)) notClosed++;
        }
        for (int it = 0; it < 20000; it++) {
            int g = rng.below(NG), h = rng.below(NG);
            std::array<uint8_t, NC> arr;
            for (int c = 0; c < NC; c++) arr[c] = GM[g][GM[h][c]];
            if (!all.count(arr)) notClosed++;
        }
        CHECK(notClosed == 0, "group closure (%d failures)", notClosed);
        // the 24 busy cells form one orbit; the identity is map 0
        Bits orb = EMPTYSET;
        for (int g = 0; g < NG; g++) orb.set(GM[g][0]);
        int ob = 0;
        for (int i = 0; i < NB; i++) ob += orb.test(BUSY[i]);
        CHECK(orb.count() == 24 && ob == 24, "orbit of the corner = busy cells");
        printf("    lines %d (axis %d, face %d, space %d), busy %d, maps %d\n", g_nlines, g_axis, g_face, g_space, nb, g_nmaps);
    }

    // ---------------- B. canonical form
    printf("[B] canonical form\n");
    {
        int bad = 0;
        const int N = 3000;
        for (int it = 0; it < N; it++) {
            Bits B, F;
            random_state(rng, rng.below(16), rng.below(14), rng.below(2), B, F);
            CanonRes c = canon(B, F), n = canon_naive(B, F);
            if (c.B != n.B || c.F != n.F) bad++;
            if (image_set(c.g, B) != c.B || image_set(c.g, F) != c.F) bad++;
            int g = rng.below(NG);
            Bits B2 = image_set(g, B), F2 = image_set(g, F);
            CanonRes c2 = canon(B2, F2);
            if (c2.B != c.B || c2.F != c.F) bad++;
            if (image_set(c2.g, B2) != c.B || image_set(c2.g, F2) != c.F) bad++;
            CanonRes c3 = canon(c.B, c.F);
            if (c3.B != c.B || c3.F != c.F) bad++;
            // inverse translation: g^-1 of a canonical cell is a real cell with the right image
            int m = rng.below(NC);
            if (GM[c.g][GI[c.g][m]] != m) bad++;
        }
        CHECK(bad == 0, "canonical form: %d failures in %d random states", bad, N);
        // order of SPEC 2.4 on hand-made sets
        int a1[] = {0, 5}, a2[] = {1, 2}, a3[] = {0, 7}, a4[] = {0};
        CHECK(cmp_set(set_of(a1, 2), set_of(a2, 2)) < 0, "order: {0,5} < {1,2}");
        CHECK(cmp_set(set_of(a1, 2), set_of(a3, 2)) < 0, "order: {0,5} < {0,7}");
        CHECK(cmp_set(set_of(a1, 2), set_of(a4, 1)) < 0, "order: {0,5} < {0} (contains the differing cell)");
        CHECK(cmp_set(set_of(a4, 1), EMPTYSET) < 0 && cmp_set(EMPTYSET, EMPTYSET) == 0, "order: {0} < {}");
        // B dominates F
        {
            Bits B = EMPTYSET, F = EMPTYSET;
            B.set(cid(5, 5, 5));
            F.set(cid(0, 0, 0));
            CanonRes c = canon(B, F);
            CHECK(c.B.test(0) && c.B.count() == 1, "canonical B of a single corner is {0}");
            CHECK(c.F.test(1) || c.F.test(5) || c.F.test(215) || c.F.count() == 1, "canonical F is a single cell");
        }
        // timing
        std::vector<std::pair<Bits, Bits>> sts;
        for (int it = 0; it < 20000; it++) {
            Bits B, F;
            random_state(rng, 6 + rng.below(9), rng.below(13), true, B, F);
            sts.push_back({B, F});
        }
        double ta = now();
        u64 acc = 0;
        for (const auto& s : sts) acc += canon(s.first, s.second).B.w[0];
        double tb = now();
        printf("    canon: %.2f microseconds per call (20000 states, |B| 6..14 mostly busy, |F| 0..12) [chk %" PRIu64 "]\n",
               (tb - ta) * 1e6 / sts.size(), acc & 0xffff);
        std::vector<std::pair<Bits, Bits>> sts2;
        for (int it = 0; it < 20000; it++) {
            Bits B, F;
            random_state(rng, 10 + rng.below(20), rng.below(20), false, B, F);
            sts2.push_back({B, F});
        }
        ta = now();
        for (const auto& s : sts2) acc += canon(s.first, s.second).B.w[0];
        tb = now();
        printf("    canon: %.2f microseconds per call (20000 states, |B| 10..29 uniform cells, |F| 0..19)\n",
               (tb - ta) * 1e6 / sts2.size());
        ta = now();
        for (int it = 0; it < 300; it++) acc += canon_naive(sts[it].first, sts[it].second).B.w[0];
        tb = now();
        printf("    naive reference: %.1f microseconds per call [chk %" PRIu64 "]\n", (tb - ta) * 1e6 / 300, acc & 0xffff);
    }

    // ---------------- C. pairing
    printf("[C] pairing\n");
    Bits T8 = set_of(T8C, 8), T7 = set_of(T8C, 7);
    {
        int16_t partner[NC];
        bool ok = find_pairing(T8, EMPTYSET, partner);
        CHECK(ok, "T8 with X = {} has a pairing");
        CHECK(ok && pairing_valid(T8, EMPTYSET, partner), "T8 pairing is valid");
        int live8 = 0, live7 = 0;
        for (int l = 0; l < NL; l++) {
            live8 += !LB[l].meets(T8);
            live7 += !LB[l].meets(T7);
        }
        CHECK(208 - 2 * live8 == 8, "slack(T8) = 8, got %d", 208 - 2 * live8);
        CHECK(209 - 2 * live7 == 1, "slack(T7) = 1, got %d", 209 - 2 * live7);
        CHECK(!find_pairing(EMPTYSET, EMPTYSET, partner), "empty board has no pairing");
        ok = find_pairing(T7, EMPTYSET, partner);
        CHECK(ok && pairing_valid(T7, EMPTYSET, partner), "T7 with X = {} has a valid pairing");
        for (int j = 0; j < 7; j++) {
            Bits b = T7;
            b.reset(T8C[j]);
            CHECK(!find_pairing(b, EMPTYSET, partner), "6 stones (T7 minus one) have no pairing");
        }
        // random positions: every reported pairing must be valid
        int found = 0, invalid = 0;
        for (int it = 0; it < 3000; it++) {
            Bits B, X;
            random_state(rng, 8 + rng.below(8), rng.below(12), true, B, X);
            if (find_pairing(B, X, partner)) {
                found++;
                if (!pairing_valid(B, X, partner)) invalid++;
            }
        }
        CHECK(invalid == 0, "random pairings valid");
        printf("    random positions: %d of 3000 have a pairing, all valid\n", found);
        // fast matcher against the plain reference matcher (existence), positions near the Hall boundary
        {
            int disagree = 0, yes = 0;
            const int NN = 20000;
            for (int it = 0; it < NN; it++) {
                Bits B, X;
                int kind = rng.below(3);
                if (kind == 0) random_state(rng, 6 + rng.below(6), rng.below(10), true, B, X);
                else {
                    B = kind == 1 ? T8 : T7;
                    X = EMPTYSET;
                    int nx = rng.below(kind == 1 ? 14 : 3);
                    while (X.count() < nx) {
                        int c = rng.below(NC);
                        if (!B.test(c)) X.set(c);
                    }
                }
                bool a = find_pairing(B, X, partner), b = find_pairing_ref(B, X);
                if (a != b) disagree++;
                if (a) {
                    yes++;
                    if (!pairing_valid(B, X, partner)) invalid++;
                }
            }
            CHECK(disagree == 0, "fast matcher vs reference matcher: %d disagreements", disagree);
            CHECK(invalid == 0, "pairings valid");
            printf("    fast matcher vs plain Kuhn reference: %d positions, %d with a pairing, %d disagreements\n", NN, yes, disagree);
        }
        // T8, X = a danger pair (8 stones, not admissible): some have no pairing (SPEC 8.3: worst without the
        // danger constraints is 1), all are rejected by admissible()
        long dp = 0, dpNo = 0, dpAdm = 0;
        for (int a = 0; a < NL; a++) {
            if (LB[a].meets(T8)) continue;
            for (int b = a + 1; b < NL; b++) {
                if (LB[b].meets(T8)) continue;
                Bits i = LB[a] & LB[b];
                if (i.empty()) continue;
                uint8_t ic[8];
                cells_of(i, ic);
                int c = ic[0];
                for (int sa = 0; sa < 6; sa++) {
                    if (LN[a][sa] == c) continue;
                    for (int sb = 0; sb < 6; sb++) {
                        if (LN[b][sb] == c) continue;
                        Bits X = LB[a] | LB[b];
                        X.reset(c);
                        X.reset(LN[a][sa]);
                        X.reset(LN[b][sb]);
                        dp++;
                        if (admissible(T8, X)) dpAdm++;
                        if (!find_pairing(T8, X, partner)) dpNo++;
                        else if (!pairing_valid(T8, X, partner)) invalid++;
                    }
                }
            }
        }
        CHECK(dpAdm == 0, "danger pairs are not admissible (%ld accepted)", dpAdm);
        CHECK(dpNo == dp, "a danger pair never has a pairing (both lines need the shared cell)");
        printf("    T8 + danger pair (8 stones): %ld positions, %ld without pairing (expected: all), %ld admissible\n", dp, dpNo, dpAdm);
        // T8, random admissible X with 8 stones (biased to heavy lines): a pairing must exist (worst(T8,{},8) = 0)
        long adm = 0, noPair = 0;
        double ta = now();
        double tpair = 0;
        for (int it = 0; it < 30000; it++) {
            std::vector<int> v = random_script(rng, T8, -1);
            Bits X = EMPTYSET;
            for (int c : v) X.set(c);
            adm++;
            double tq = now();
            bool okp = find_pairing(T8, X, partner);
            tpair += now() - tq;
            if (!okp) noPair++;
            else if (!pairing_valid(T8, X, partner)) invalid++;
        }
        (void)ta;
        CHECK(noPair == 0, "T8: %ld admissible 8-stone X without pairing", noPair);
        CHECK(invalid == 0, "pairings valid");
        printf("    T8 + %ld random admissible 8-stone X: all have a valid pairing; find_pairing %.1f microseconds\n",
               adm, tpair * 1e6 / adm);
        // T7 with 7 FP stones: SPEC 8.2 says worst = 6, so admissible X without pairing must be easy to find
        long t7no = 0;
        for (int it = 0; it < 2000; it++) {
            Bits X = EMPTYSET;
            while (X.count() < 7) {
                int c = rng.below(NC);
                if (!T7.test(c)) X.set(c);
            }
            if (admissible(T7, X) && !find_pairing(T7, X, partner)) t7no++;
        }
        CHECK(t7no > 0, "T7 with 7 random FP stones: some admissible X has no pairing");
        printf("    T7 + 2000 random 7-stone X: %ld admissible without pairing\n", t7no);
    }

    // ---------------- D. threat / danger / admissibility on hand-made positions
    printf("[D] threat, danger, admissibility\n");
    {
        Store emptyT, emptyL;
        emptyT.init(1);
        emptyL.init(1);
        Ctx ctx;
        ctx.table = &emptyT;
        ctx.leaves = &emptyL;
        ctx.checkCounts = false;   // hand-made positions: |X| differs from |B|
        TS ts;
        Stats st;
        std::vector<int> live;
        for (int l = 0; l < NL; l++)
            if (!LB[l].meets(T8)) live.push_back(l);
        // D1: threat on every live line and every choice of the sixth cell
        int bad = 0;
        for (int l : live)
            for (int e = 0; e < 6; e++) {
                Bits X = LB[l];
                X.reset(LN[l][e]);
                int last = LN[l][e == 0 ? 1 : 0];
                X.reset(last);
                Game g;
                setup_game(g, T8, X, EMPTYSET, 8);
                Stats s1;
                Status s = play(g, last, ctx, ts, &s1);
                Bits F5 = LB[l];
                F5.reset(LN[l][e]);
                if (s != ST_OK || g.lastSP != LN[l][e] || g.F != F5 || s1.threat != 1 || s1.danger != 0 || !g.B.test(LN[l][e])) bad++;
            }
        CHECK(bad == 0, "threat reply (%d failures)", bad);
        // D2: danger on every intersecting pair of live lines
        bad = 0;
        long nd = 0;
        for (int a : live)
            for (int b : live) {
                if (a >= b) continue;
                Bits i = LB[a] & LB[b];
                if (i.empty()) continue;
                uint8_t ic[8];
                cells_of(i, ic);
                int c = ic[0];
                int sa = rng.below(6), sb = rng.below(6);
                if (LN[a][sa] == c || LN[b][sb] == c) continue;
                Bits X = LB[a] | LB[b];
                X.reset(c);
                X.reset(LN[a][sa]);
                X.reset(LN[b][sb]);
                Bits F8 = X;
                int last = -1;
                for (int k = 0; k < 6; k++)
                    if (X.test(LN[b][k])) last = LN[b][k];
                X.reset(last);
                if (!admissible(T8, X)) { bad++; continue; }
                Game g;
                setup_game(g, T8, X, EMPTYSET, 8);
                Stats s1;
                Status s = play(g, last, ctx, ts, &s1);
                nd++;
                if (s != ST_OK || g.lastSP != c || g.F != F8 || s1.danger != 1 || s1.threat != 0) bad++;
            }
        CHECK(bad == 0, "danger reply (%d failures in %ld positions)", bad, nd);
        printf("    threat reply on %zu live lines x 6 cells, danger reply on %ld line pairs\n", live.size(), nd);
        // D3: find_danger against the brute-force pair scan, admissible() against a direct definition
        bad = 0;
        long withDanger = 0;
        for (int it = 0; it < 4000; it++) {
            Bits B, X;
            random_state(rng, 6 + rng.below(6), 0, true, B, X);
            std::vector<int> lv;
            for (int l = 0; l < NL; l++)
                if (!LB[l].meets(B)) lv.push_back(l);
            int nl = 1 + rng.below(4);
            for (int k = 0; k < nl; k++) {
                int l = lv[rng.below((int)lv.size())];
                int skip = rng.below(6), skip2 = rng.below(8);
                for (int i = 0; i < 6; i++)
                    if (i != skip && i != skip2) X.set(LN[l][i]);
            }
            for (int k = rng.below(5); k > 0; k--) {
                int c = rng.below(NC);
                if (!B.test(c)) X.set(c);
            }
            Game g;
            setup_game(g, B, X, EMPTYSET, 0);
            int a, b2;
            int c = find_danger(g, a, b2);
            int cb = brute_danger(B, X);
            if (c != cb) bad++;
            if (c >= 0) {
                withDanger++;
                if (a >= b2 || !LB[a].test(c) || !LB[b2].test(c) || (LB[a] & X).count() != 4 || (LB[b2] & X).count() != 4) bad++;
                for (int i = 0; i < NLC[c]; i++) {   // a, b are the two smallest qualifying lines through c
                    int l = LC[c][i];
                    if (!LB[l].meets(B) && (LB[l] & X).count() == 4 && l != a && l < b2) bad++;
                }
            }
            bool over = false;
            for (int l = 0; l < NL; l++)
                if (!LB[l].meets(B) && (LB[l] & X).count() >= 5) over = true;
            if (admissible(B, X) != (!over && cb < 0)) bad++;
        }
        CHECK(bad == 0, "danger detection / admissibility vs brute force (%d failures)", bad);
        printf("    4000 random heavy positions, %ld with a danger pair\n", withDanger);
        // D4: hand-made admissibility cases
        {
            int l = live[0];
            Bits X5 = LB[l];
            X5.reset(LN[l][0]);
            CHECK(!admissible(T8, X5), "5 on a live line is not admissible");
            Bits Xd = EMPTYSET;   // 5 stones on a dead line: admissible
            int dead = -1;
            for (int q = 0; q < NL && dead < 0; q++)
                if ((LB[q] & T8).count() == 1) dead = q;
            for (int i = 0; i < 6; i++)
                if (!T8.test(LN[dead][i])) Xd.set(LN[dead][i]);
            CHECK(Xd.count() == 5 && admissible(T8, Xd), "5 on a dead line is admissible");
            Bits Xb = EMPTYSET;
            Xb.set(T8C[0]);
            CHECK(!admissible(T8, Xb), "X meeting B is not admissible");
            CHECK(admissible(T8, EMPTYSET), "empty X is admissible");
            // two 4-lines sharing a cell that is IN X: admissible
            int found = 0;
            for (int a : live) {
                for (int b : live) {
                    if (a >= b) continue;
                    Bits i = LB[a] & LB[b];
                    if (i.empty()) continue;
                    uint8_t ic[8];
                    cells_of(i, ic);
                    Bits X = EMPTYSET;
                    X.set(ic[0]);
                    int ka = 1, kb = 1;
                    for (int k = 0; k < 6 && ka < 4; k++)
                        if (LN[a][k] != ic[0]) { X.set(LN[a][k]); ka++; }
                    for (int k = 0; k < 6 && kb < 4; k++)
                        if (LN[b][k] != ic[0]) { X.set(LN[b][k]); kb++; }
                    if (brute_danger(T8, X) >= 0) continue;   // accidental third line
                    bool over = false;
                    for (int q : live) over |= (LB[q] & X).count() >= 5;
                    if (over) continue;
                    CHECK(admissible(T8, X), "4+4 through a stone is admissible");
                    found = 1;
                    break;
                }
                if (found) break;
            }
            CHECK(found, "constructed a 4+4-through-a-stone position");
        }
        // D5: double threat -> SP blocks one, the position is not admissible; then FP completes the line
        {
            int done = 0;
            for (int a : live) {
                for (int b : live) {
                    if (a >= b) continue;
                    Bits i = LB[a] & LB[b];
                    if (i.empty()) continue;
                    uint8_t ic[8];
                    cells_of(i, ic);
                    int x = ic[0];
                    Bits X = EMPTYSET;
                    int ka = 0, kb = 0, ea = -1, eb = -1;
                    for (int k = 0; k < 6; k++)
                        if (LN[a][k] != x) { if (ka < 4) { X.set(LN[a][k]); ka++; } else ea = LN[a][k]; }
                    for (int k = 0; k < 6; k++)
                        if (LN[b][k] != x) { if (kb < 4) { X.set(LN[b][k]); kb++; } else eb = LN[b][k]; }
                    Game g;
                    setup_game(g, T8, X, EMPTYSET, 8);
                    Stats s1;
                    Status s = play(g, x, ctx, ts, &s1);
                    CHECK(s == ST_ADMISS_FAIL, "double threat gives an admissibility failure, got %s", status_name(s));
                    CHECK(g.lastSP == ea, "SP blocks the first threat line");
                    Game g2;
                    Bits X2 = LB[b];
                    X2.reset(eb);
                    setup_game(g2, T8, X2, EMPTYSET, 8);
                    s = play(g2, eb, ctx, ts, &s1);
                    CHECK(s == ST_FP_LINE, "completing a line is detected, got %s", status_name(s));
                    done = 1;
                    break;
                }
                if (done) break;
            }
            CHECK(done, "double-threat test constructed");
        }
        // D6: two disjoint dangers created by one move -> SP takes the smallest shared cell, failure reported
        {
            int done = 0;
            for (int x = 0; x < NC && !done; x++) {
                if (T8.test(x)) continue;
                for (int i1 = 0; i1 < NLC[x] && !done; i1++)
                    for (int i2 = i1 + 1; i2 < NLC[x] && !done; i2++) {
                        int b1 = LC[x][i1], b2 = LC[x][i2];
                        if (LB[b1].meets(T8) || LB[b2].meets(T8)) continue;
                        for (int j1 = 0; j1 < 6 && !done; j1++)
                            for (int j2 = 0; j2 < 6 && !done; j2++) {
                                int c1 = LN[b1][j1], c2 = LN[b2][j2];
                                if (c1 == x || c2 == x) continue;
                                for (int q1 = 0; q1 < NLC[c1] && !done; q1++)
                                    for (int q2 = 0; q2 < NLC[c2] && !done; q2++) {
                                        int a1 = LC[c1][q1], a2 = LC[c2][q2];
                                        if (a1 == b1 || a2 == b2 || a1 == a2) continue;
                                        if (LB[a1].meets(T8) || LB[a2].meets(T8)) continue;
                                        if (LB[a1].meets(LB[a2]) || LB[a1].meets(LB[b2]) || LB[a2].meets(LB[b1])) continue;
                                        Bits X = EMPTYSET;
                                        int k = 0;
                                        for (int q = 0; q < 6 && k < 3; q++)
                                            if (LN[b1][q] != x && LN[b1][q] != c1) { X.set(LN[b1][q]); k++; }
                                        k = 0;
                                        for (int q = 0; q < 6 && k < 3; q++)
                                            if (LN[b2][q] != x && LN[b2][q] != c2) { X.set(LN[b2][q]); k++; }
                                        k = 0;
                                        for (int q = 0; q < 6 && k < 4; q++)
                                            if (LN[a1][q] != c1) { X.set(LN[a1][q]); k++; }
                                        k = 0;
                                        for (int q = 0; q < 6 && k < 4; q++)
                                            if (LN[a2][q] != c2) { X.set(LN[a2][q]); k++; }
                                        if (X.count() != 14 || !admissible(T8, X)) continue;
                                        Bits X1 = X;
                                        X1.set(x);
                                        bool over = false;
                                        for (int q : live) over |= (LB[q] & X1).count() >= 5;
                                        if (over) continue;
                                        int cmin = brute_danger(T8, X1);
                                        if (cmin != std::min(c1, c2)) continue;
                                        Game g;
                                        setup_game(g, T8, X, EMPTYSET, 14);
                                        Stats s1;
                                        Status s = play(g, x, ctx, ts, &s1);
                                        CHECK(s == ST_ADMISS_FAIL, "double danger gives an admissibility failure, got %s", status_name(s));
                                        CHECK(g.lastSP == cmin, "SP takes the smallest danger cell");
                                        CHECK(s1.danger == 1 && g.F.count() == 8, "danger reveal of 8 stones");
                                        done = 1;
                                    }
                            }
                    }
            }
            CHECK(done, "double-danger test constructed");
        }
    }

    // ---------------- E. end-to-end dry runs with synthetic tables
    printf("[E] end-to-end dry runs (synthetic table, B = image of T8, leaf at round 8)\n");
    std::string dir = tmpdirArg;
    if (dir.empty()) {
        const char* td = getenv("TMPDIR");
        dir = std::string(td && *td ? td : "/tmp") + "/sim_selftest_" + std::to_string((long)getpid());
    }
    std::string mk = "mkdir -p '" + dir + "'";
    if (system(mk.c_str()) != 0) {
        fprintf(stderr, "cannot create %s\n", dir.c_str());
        return 2;
    }
    std::vector<std::string> files;
    Params P;
    double advTime[ADV_COUNT] = {0};
    long advGames[ADV_COUNT] = {0};
    long advRounds[ADV_COUNT] = {0};
    {
        const int NFRAMES = 4, PER = 250;
        long total = 0, bugs = 0, occ = 0, occExpected = 0, leaf8 = 0, rejected = 0, nonId = 0;
        for (int fr = 0; fr < NFRAMES; fr++) {
            int h = fr == 0 ? 0 : rng.below(NG);
            for (int variant = 0; variant < 2; variant++) {
                SynTable syn = make_syn(dir, "syn_f" + std::to_string(fr) + "_v" + std::to_string(variant), h, variant);
                files.push_back(syn.tbl);
                files.push_back(syn.leaves);
                Store tbl, lvs;
                LoadInfo li1 = load_file(syn.tbl, true, tbl, true);
                LoadInfo li2 = load_file(syn.leaves, false, lvs, true);
                CHECK(li1.lines == (variant == 1 ? 9 : 8) && li2.lines == 1, "synthetic file sizes");
                CHECK(li1.noncanonical == 0 && li2.noncanonical == 0, "synthetic files are canonical");
                CHECK(tbl.conflicts == 0, "no conflicting entries");
                Ctx ctx;
                ctx.table = &tbl;
                ctx.leaves = &lvs;
                Bits Bst = set_of(syn.bstar, 8);
                // is the canonical frame different from the real frame at the leaf?
                {
                    CanonRes c = canon(Bst, EMPTYSET);
                    if (c.B != Bst) nonId++;
                }
                for (int adv = 0; adv < ADV_COUNT; adv++) {
                    TS ts;
                    for (int gi = 0; gi < PER; gi++) {
                        // find an opening that passes the 8 scripted rounds without any forced event
                        std::vector<int> script;
                        for (;;) {
                            script = random_script(rng, Bst, variant == 1 ? syn.decoy : -1);
                            Game g;
                            Stats s0;
                            game_init(g);
                            Status s = round_start(g, ctx, ts);
                            for (int r = 0; r < 8 && s == ST_OK && !g.pairing; r++) s = play(g, script[r], ctx, ts, &s0);
                            if (s == ST_OK && g.pairing && g.leafRound == 8 && s0.threat == 0 && s0.danger == 0 &&
                                s0.occupied == (variant == 1 ? 1 : 0) && g.B == Bst)
                                break;
                            rejected++;
                        }
                        Game g;
                        Stats st;
                        Rng grng(rng.next());
                        double ta = now();
                        Status s = run_game(g, adv, ctx, ts, grng, P, &script, st);
                        advTime[adv] += now() - ta;
                        advGames[adv]++;
                        advRounds[adv] += g.t;
                        total++;
                        if (s != ST_END) {
                            bugs++;
                            if (bugs <= 3) printf("    BUG %s\n", bug_json(total, 0, ADV_NAME[adv], g).c_str());
                        }
                        if (g.ns != NC && s == ST_END) bugs++;
                        occ += st.occupied;
                        occExpected += (variant == 1);
                        leaf8 += st.leafHist[8];
                        // the legality of every move and the final position
                        Bits seen = EMPTYSET;
                        bool legal = true;
                        for (int i = 0; i < g.nm; i++) {
                            if (seen.test(g.moves[i])) legal = false;
                            seen.set(g.moves[i]);
                        }
                        if (!legal || g.X.meets(g.B)) bugs++;
                        bool fpl = false;
                        for (int l = 0; l < NL; l++) fpl |= ((LB[l] & g.X) == LB[l]);
                        if (fpl) bugs++;
                    }
                }
            }
        }
        CHECK(bugs == 0, "dry run: %ld bugs in %ld games", bugs, total);
        CHECK(occ == occExpected, "occupied reveals %ld, expected %ld", occ, occExpected);
        CHECK(leaf8 == total, "all games reach the leaf at round 8 (%ld of %ld)", leaf8, total);
        CHECK(nonId >= 1, "at least one frame differs from the canonical frame");
        printf("    %ld games (4 frames x 2 variants x %d adversaries x %d), bugs %ld, occupied reveals %ld (expected %ld), "
               "leaf at round 8 in %ld, rejected openings %ld, non-identity frames %ld of 8\n",
               total, (int)ADV_COUNT, PER, bugs, occ, occExpected, leaf8, rejected, nonId);
        for (int a = 0; a < ADV_COUNT; a++)
            printf("    PAIRING-mode speed, adversary %-6s: %8.0f games/s single thread (%.1f rounds per game)\n",
                   ADV_NAME[a], advGames[a] / advTime[a], (double)advRounds[a] / advGames[a]);
    }

    // ---------------- E2. scripted threat and danger in TABLE mode, table entries with a revealed F afterwards
    printf("[E2] end-to-end with a threat (round 5) or a danger (round 8) before the leaf (round 9)\n");
    {
        long total = 0, bugs = 0, thr = 0, dng = 0, leaf9 = 0, rejected = 0, pfAdmissible = 0, skipped = 0;
        const int PER = 100;
        for (int fr = 0; fr < 4; fr++) {
            int h = fr == 0 ? 0 : rng.below(NG);
            std::vector<int> bst;
            for (int j = 0; j < 8; j++) bst.push_back(GM[h][T8C[j]]);
            Bits Bst = EMPTYSET;
            for (int c : bst) Bst.set(c);
            for (int scen = 0; scen < 2; scen++) {
                // the forced part of the FP script and the revealed set
                std::vector<int> forcedPart;
                Bits Bfin = EMPTYSET, Ffin = EMPTYSET;
                std::string tbl, leaf;
                int nfree = 0;
                bool have = false;
                if (scen == 0) {   // SPEC 8.7: B = T8 + (0,1,2), F = (1..5,1,2), k = 9 has worst 0
                    for (int x = 1; x <= 5; x++) forcedPart.push_back(GM[h][cid(x, 1, 2)]);
                    nfree = 4;
                    for (int tries = 0; tries < 1000 && !have; tries++) {
                        std::vector<int> fp = forcedPart;
                        while ((int)fp.size() < 9) {
                            int c = rng.below(NC);
                            if (!Bst.test(c) && std::find(fp.begin(), fp.end(), c) == fp.end() && c != GM[h][cid(0, 1, 2)]) fp.push_back(c);
                        }
                        have = record_syn(fp, bst, tbl, leaf, Bfin, Ffin);
                    }
                    CHECK(have && Ffin.count() == 5 && Bfin.count() == 9, "threat scenario recorded");
                } else {   // a danger pair whose leaf has a pairing for every admissible ninth stone
                    std::vector<int> live;
                    for (int l = 0; l < NL; l++)
                        if (!LB[l].meets(Bst)) live.push_back(l);
                    for (int tries = 0; tries < 3000 && !have; tries++) {
                        int a = live[rng.below((int)live.size())], b = live[rng.below((int)live.size())];
                        if (a == b) continue;
                        Bits ib = LB[a] & LB[b];
                        if (ib.empty()) continue;
                        uint8_t ic[8];
                        cells_of(ib, ic);
                        int c = ic[0], sa = LN[a][rng.below(6)], sb = LN[b][rng.below(6)];
                        if (sa == c || sb == c) continue;
                        Bits X8 = LB[a] | LB[b];
                        X8.reset(c); X8.reset(sa); X8.reset(sb);
                        Bits B9 = Bst;
                        B9.set(c);
                        bool all = true;
                        int16_t partner[NC];
                        for (int y = 0; y < NC && all; y++) {
                            if (B9.test(y) || X8.test(y)) continue;
                            Bits X9 = X8;
                            X9.set(y);
                            if (admissible(B9, X9) && !find_pairing(B9, X9, partner)) all = false;
                        }
                        if (!all) continue;
                        uint8_t cs[NC];
                        int n = cells_of(X8, cs);
                        forcedPart.assign(cs, cs + n);
                        std::vector<int> fp = forcedPart;
                        for (int y = 0; y < NC; y++)
                            if (!B9.test(y) && !X8.test(y)) { fp.push_back(y); break; }
                        have = record_syn(fp, bst, tbl, leaf, Bfin, Ffin) && Ffin == X8 && Bfin == B9;
                        nfree = 1;
                    }
                    if (!have) {
                        skipped++;
                        continue;
                    }
                }
                std::string name = dir + "/syn_ev_f" + std::to_string(fr) + "_s" + std::to_string(scen);
                write_text(name + ".tbl", tbl);
                write_text(name + ".leaves", leaf);
                files.push_back(name + ".tbl");
                files.push_back(name + ".leaves");
                Store tb, lv;
                LoadInfo l1 = load_file(name + ".tbl", true, tb, true), l2 = load_file(name + ".leaves", false, lv, true);
                CHECK(l1.noncanonical == 0 && l2.noncanonical == 0 && l1.lines == 8 && l2.lines == 1, "event scenario files");
                Ctx ctx;
                ctx.table = &tb;
                ctx.leaves = &lv;
                for (int adv = 0; adv < ADV_COUNT; adv++) {
                    TS ts;
                    for (int gi = 0; gi < PER; gi++) {
                        std::vector<int> script;
                        for (;;) {
                            script = forcedPart;
                            for (int i = (int)script.size() - 1; i > 0; i--) std::swap(script[i], script[rng.below(i + 1)]);
                            Bits used = Bfin;
                            for (int c : script) used.set(c);
                            for (int q = 0; q < nfree; q++) {
                                int c;
                                do c = rng.below(NC); while (used.test(c));
                                used.set(c);
                                script.push_back(c);
                            }
                            Bits X = EMPTYSET;
                            for (int c : script) X.set(c);
                            if (!admissible(Bfin, X)) { rejected++; continue; }
                            Game g;
                            Stats s0;
                            game_init(g);
                            Status st0 = round_start(g, ctx, ts);
                            for (int r = 0; r < 9 && st0 == ST_OK && !g.pairing; r++) st0 = play(g, script[r], ctx, ts, &s0);
                            if (st0 == ST_PAIRING_FAIL && g.B == Bfin && g.F == Ffin) pfAdmissible++;   // must not happen
                            if (st0 == ST_OK && g.pairing && g.leafRound == 9 && g.B == Bfin && g.F == Ffin &&
                                s0.threat == (scen == 0) && s0.danger == (scen == 1) && s0.occupied == 0)
                                break;
                            rejected++;
                        }
                        Game g;
                        Stats st;
                        Rng grng(rng.next());
                        Status sres = run_game(g, adv, ctx, ts, grng, P, &script, st);
                        total++;
                        if (sres != ST_END || g.ns != NC) {
                            bugs++;
                            if (bugs <= 3) printf("    BUG %s\n", bug_json(total, 0, ADV_NAME[adv], g).c_str());
                        }
                        thr += st.threat;
                        dng += st.danger;
                        leaf9 += st.leafHist[9];
                        bool fpl = false;
                        for (int l = 0; l < NL; l++) fpl |= ((LB[l] & g.X) == LB[l]);
                        if (fpl) bugs++;
                    }
                }
            }
        }
        CHECK(bugs == 0, "event scenarios: %ld bugs in %ld games", bugs, total);
        CHECK(pfAdmissible == 0, "event scenarios: %ld admissible leaf positions without pairing", pfAdmissible);
        CHECK(leaf9 == total, "event scenarios: leaf at round 9 in %ld of %ld games", leaf9, total);
        CHECK(thr + dng == total, "event scenarios: one forced event per game");
        CHECK(skipped <= 2, "danger scenario could not be constructed in %ld frames", skipped);
        printf("    %ld games, bugs %ld, threat events %ld, danger events %ld, leaf at round 9 in %ld, rejected openings %ld, "
               "admissible leaf positions without pairing %ld, danger scenarios skipped %ld of 4\n",
               total, bugs, thr, dng, leaf9, rejected, pfAdmissible, skipped);
    }

    // ---------------- F. free play and negative tests
    printf("[F] free play and negative tests\n");
    {
        SynTable syn = make_syn(dir, "syn_free", 0, 0);
        files.push_back(syn.tbl);
        files.push_back(syn.leaves);
        Store tbl, lvs;
        load_file(syn.tbl, true, tbl, false);
        load_file(syn.leaves, false, lvs, false);
        Ctx ctx;
        ctx.table = &tbl;
        ctx.leaves = &lvs;
        // free play from move 1: the tiny table has holes; the search adversary must find them
        for (int adv = 0; adv < ADV_COUNT; adv++) {
            TS ts;
            Stats st;
            double ta = now();
            int illegalMoves = 0;
            for (int gi = 0; gi < 200; gi++) {
                Game g;
                Rng grng(rng.next());
                run_game(g, adv, ctx, ts, grng, P, nullptr, st);
                Bits seen = EMPTYSET;
                for (int i = 0; i < g.nm; i++) {
                    if (seen.test(g.moves[i])) illegalMoves++;
                    seen.set(g.moves[i]);
                }
            }
            double tb = now();
            printf("    free play %-6s: 200 games, ended %ld, holes %ld, pairing_failures %ld, admissibility %ld, fp_lines %ld, "
                   "%.0f games/s, canon calls %ld, cache hits %ld\n",
                   ADV_NAME[adv], st.ended, st.holes, st.pairing_fail, st.adm_fail, st.fp_lines, 200 / (tb - ta),
                   ts.canonCalls, ts.cacheHits);
            CHECK(illegalMoves == 0, "adversary %s played an occupied cell", ADV_NAME[adv]);
            CHECK(st.fp_lines == 0 && st.illegal == 0, "free play: no FP line can arise against sigma here");
            if (adv == ADV_SEARCH) CHECK(st.holes >= 180, "search adversary finds the holes (%ld of 200)", st.holes);
        }
        // cache on/off must give identical games
        {
            long diff = 0;
            for (int gi = 0; gi < 100; gi++) {
                u64 sd = rng.next();
                Game g1, g2;
                TS ts1, ts2;
                Stats s1, s2;
                Ctx c2 = ctx;
                c2.useCache = false;
                Rng r1(sd), r2(sd);
                int adv = gi % ADV_COUNT;
                run_game(g1, adv, ctx, ts1, r1, P, nullptr, s1);
                run_game(g2, adv, c2, ts2, r2, P, nullptr, s2);
                if (g1.nm != g2.nm || memcmp(g1.moves, g2.moves, g1.nm) != 0 || g1.st != g2.st) diff++;
            }
            CHECK(diff == 0, "cache on/off give identical games (%ld differ)", diff);
        }
        // negative 1: missing round-8 entry -> every scripted game is a hole
        // negative 2: P(7 stones) declared a leaf -> pairing failures
        for (int variant = 2; variant <= 3; variant++) {
            SynTable sn = make_syn(dir, "syn_neg" + std::to_string(variant), 0, variant);
            files.push_back(sn.tbl);
            files.push_back(sn.leaves);
            Store t2, l2;
            load_file(sn.tbl, true, t2, false);
            load_file(sn.leaves, false, l2, false);
            Ctx c2;
            c2.table = &t2;
            c2.leaves = &l2;
            Bits Bst = set_of(sn.bstar, 8);
            TS ts;
            Stats st;
            for (int gi = 0; gi < 300; gi++) {
                std::vector<int> script;
                for (;;) {   // scripts without forced events in the first 7 rounds
                    script = random_script(rng, Bst, -1);
                    Bits B = EMPTYSET;
                    bool ok = true;
                    Bits X = EMPTYSET;
                    for (int r = 0; r < 8 && ok; r++) {
                        X.set(script[r]);
                        for (int l = 0; l < NL; l++)
                            if ((LB[l] & X).count() >= 4) ok = false;
                        B.set(sn.bstar[r]);
                    }
                    if (ok) break;
                }
                Game g;
                Rng grng(rng.next());
                run_game(g, gi % ADV_COUNT, c2, ts, grng, P, &script, st);
            }
            if (variant == 2) {
                CHECK(st.holes == 300, "negative test: 300 holes expected, got %ld", st.holes);
                printf("    missing entry: holes %ld of 300 games\n", st.holes);
            } else {
                CHECK(st.pairing_fail >= 250, "negative test: pairing failures expected, got %ld", st.pairing_fail);
                printf("    bogus 7-stone leaf: pairing_failures %ld of 300 games (ended %ld)\n", st.pairing_fail, st.ended);
            }
        }
    }
    // ---------------- G. cost of the search adversary in TABLE mode (benchmark-only total policy, bugs are expected)
    printf("[G] search adversary cost in TABLE mode (fake total policy, leaf forced at round 14)\n");
    {
        Store emptyT, emptyL;
        emptyT.init(1);
        emptyL.init(1);
        Ctx ctx;
        ctx.table = &emptyT;
        ctx.leaves = &emptyL;
        ctx.fakeLeafRound = 14;
        for (int depth = 2; depth <= 3; depth++) {
            Params Q = P;
            Q.depth = depth;
            TS ts;
            double tsum = 0, tgame = 0;
            long calls = 0, ngames = depth == 2 ? 60 : 15, tableRounds = 0;
            for (long gi = 0; gi < ngames; gi++) {
                Game g;
                Stats st;
                Rng grng(rng.next());
                game_init(g);
                double tg0 = now();
                Status s = round_start(g, ctx, ts);
                while (s == ST_OK && !g.pairing) {
                    double ta = now();
                    AdvState as;
                    int x = choose_move(g, ADV_SEARCH, ctx, ts, grng, Q, as);
                    tsum += now() - ta;
                    calls++;
                    CHECK(is_empty_cell(g, x), "search adversary move is legal");
                    s = play(g, x, ctx, ts, &st);
                }
                tableRounds += g.t;
                tgame += now() - tg0;
            }
            printf("    depth %d width %d: %.2f ms per TABLE-mode decision (%ld decisions, %.1f TABLE rounds per game, "
                   "%.1f ms per game before the leaf, canon calls %ld, cache hits %ld)\n",
                   depth, Q.width, tsum * 1e3 / std::max(1L, calls), calls, (double)tableRounds / ngames,
                   tgame * 1e3 / ngames, ts.canonCalls, ts.cacheHits);
        }
    }
    // ---------------- H. adversary strength against a deliberately weak SP (same benchmark-only policy, leaf at 24)
    // FP "wins" in TABLE mode = admissibility failure (double threat / threat + danger / two dangers) or FP line.
    printf("[H] adversaries against a weak total policy (SP = smallest free busy cell in the canonical frame)\n");
    {
        Store emptyT, emptyL;
        emptyT.init(1);
        emptyL.init(1);
        Ctx ctx;
        ctx.table = &emptyT;
        ctx.leaves = &emptyL;
        ctx.fakeLeafRound = 24;
        long wins[ADV_COUNT];
        for (int adv = 0; adv < ADV_COUNT; adv++) {
            TS ts;
            Stats st;
            const int NGm = 100;
            double ta = now();
            for (int gi = 0; gi < NGm; gi++) {
                Game g;
                Rng grng(rng.next());
                run_game(g, adv, ctx, ts, grng, P, nullptr, st);
            }
            wins[adv] = st.adm_fail + st.fp_lines;
            printf("    %-6s: %d games: FP wins in TABLE mode %ld (admissibility %ld, fp_lines %ld), pairing_failures at the forced "
                   "leaf %ld, completed %ld, threat events %ld, danger events %ld, %.1f ms per game\n",
                   ADV_NAME[adv], NGm, wins[adv], st.adm_fail, st.fp_lines, st.pairing_fail, st.ended, st.threat, st.danger,
                   (now() - ta) * 1e3 / NGm);
        }
        CHECK(wins[ADV_SEARCH] >= 90, "search adversary beats the weak policy in TABLE mode (%ld of 100)", wins[ADV_SEARCH]);
    }
    // ---------------- I. checks added by the audit
    printf("[I] audit additions: exact store, illegal FP move, pairing invariant, final position, coverage marks, look-ahead bugs\n");
    {
        // I1: exact keys
        Store stx;
        stx.init(4);
        Bits b1 = EMPTYSET, f1 = EMPTYSET, f2 = EMPTYSET;
        b1.set(0); b1.set(215);
        f1.set(7);
        f2.set(8);
        stx.insert(3, b1, f1, 46);
        stx.insert(3, b1, f1, 46);
        stx.insert(3, b1, f1, 47);
        stx.insert(3, b1, EMPTYSET, 9);
        CHECK(stx.n == 2 && stx.duplicates == 2 && stx.conflicts == 1, "store: duplicates and conflicts counted");
        CHECK(stx.find(3, b1, f1) == 0 && stx.ent[0].m == 46, "store: exact hit");
        CHECK(stx.find(3, b1, f2) < 0 && stx.find(4, b1, f1) < 0 && stx.find(3, f1, b1) < 0, "store: near keys are misses");
        // I2: an FP move on an occupied cell is an internal error, not a silent corruption
        Store emptyT, emptyL;
        emptyT.init(1);
        emptyL.init(1);
        Ctx ctx;
        ctx.table = &emptyT;
        ctx.leaves = &emptyL;
        ctx.checkCounts = false;
        TS ts;
        {
            Game g;
            setup_game(g, T8, EMPTYSET, EMPTYSET, 8);
            Stats s1;
            Status s = play(g, T8C[0], ctx, ts, &s1);
            CHECK(s == ST_INTERNAL && g.internal == IE_FP_OCCUPIED, "FP move on an SP stone is reported");
            Game g2;
            setup_game(g2, T8, EMPTYSET, EMPTYSET, 8);
            put_x(g2, 100);
            s = play(g2, 100, ctx, ts, &s1);
            CHECK(s == ST_INTERNAL && g2.internal == IE_FP_OCCUPIED, "FP move on an FP stone is reported");
        }
        // I3: PAIRING-mode invariant.  Positive: FP takes one cell of a pair, SP takes the other.  Negative: a
        // corrupted position in which FP already holds the partner is reported.
        {
            Game g;
            setup_game(g, T8, EMPTYSET, EMPTYSET, 8);
            bool ok = find_pairing(T8, EMPTYSET, g.partner);
            g.pairing = true;
            g.leafRound = 8;
            int tested = 0, bad = 0;
            int se = smallest_empty(g);
            for (int l = 0; l < NL && ok; l++) {
                if (LB[l].meets(T8) || LB[l].test(se)) continue;
                int pc = -1, qc = -1;
                for (int i = 0; i < 6; i++) {
                    int c = LN[l][i];
                    if (g.partner[c] >= 0 && LB[l].test(g.partner[c])) { pc = c; qc = g.partner[c]; }
                }
                if (pc < 0) { bad++; continue; }
                Stats s1;
                Game h = g;
                Status s = play(h, pc, ctx, ts, &s1);
                if (s != ST_OK || h.lastSP != qc) bad++;
                Game k = g;
                put_x(k, qc);   // corrupt: FP "already holds" the partner
                s = play(k, pc, ctx, ts, &s1);
                if (s != ST_PAIR_BROKEN) bad++;
                tested++;
            }
            CHECK(ok && tested >= 50 && bad == 0, "pairing invariant: %d lines tested, %d failures", tested, bad);
            // five FP stones on a live line in PAIRING mode are reported even without partner information
            int l = -1;
            for (int q = 0; q < NL && l < 0; q++)
                if (!LB[q].meets(T8) && !LB[q].test(se)) l = q;
            Game h = g;
            for (int c = 0; c < NC; c++) h.partner[c] = -1;
            for (int i = 0; i < 4; i++) put_x(h, LN[l][i]);
            Stats s1;
            Status s = play(h, LN[l][4], ctx, ts, &s1);
            CHECK(s == ST_PAIR_BROKEN, "5 FP stones on a live line in PAIRING mode are reported, got %s", status_name(s));
            printf("    pairing invariant: %d live lines, positive and negative case each\n", tested);
        }
        // I4: final position check
        {
            Game g;
            game_init(g);
            for (int c = 0; c < NC; c++) {
                g.moves[g.nm++] = (uint8_t)c;
                if (c & 1) put_b(g, c); else put_x(g, c);
            }
            // cells alternate along z: every line along z has both colours, but lines inside a z-layer parity do not
            bool fpl = false;
            for (int l = 0; l < NL; l++) fpl |= !LB[l].meets(g.B);
            CHECK(fpl && !final_position_ok(g), "final position with an FP line is rejected");
            g.nm--;
            CHECK(!final_position_ok(g), "final position with 215 moves is rejected");
        }
        // I5: coverage marks of a scripted game on the synthetic table with a decoy (9 entries, 1 leaf)
        {
            int h = rng.below(NG);
            SynTable syn = make_syn(dir, "syn_cov", h, 1);
            files.push_back(syn.tbl);
            files.push_back(syn.leaves);
            Store tb, lv;
            load_file(syn.tbl, true, tb, true);
            load_file(syn.leaves, false, lv, true);
            Ctx c2;
            c2.table = &tb;
            c2.leaves = &lv;
            std::vector<uint8_t> cq(tb.n + 1, 0), cl(lv.n + 1, 0);
            c2.covQ = c2.prbQ = cq.data();
            c2.covL = c2.prbL = cl.data();
            Bits Bst = set_of(syn.bstar, 8);
            TS t2;
            std::vector<int> script;
            Game g;
            for (;;) {
                script = random_script(rng, Bst, syn.decoy);
                Stats s0;
                game_init(g);
                Status s = round_start(g, c2, t2);
                for (int r = 0; r < 8 && s == ST_OK && !g.pairing; r++) s = play(g, script[r], c2, t2, nullptr);
                if (s == ST_OK && g.pairing && g.leafRound == 8 && g.nThreat == 0 && g.nDanger == 0 && g.nOcc == 1) break;
            }
            CHECK(g.novelQ == 9 && g.leafNovel == 1 && g.leafIdx == 0 && g.leafF == 1, "novelty counters of the scripted game (%d, %d)", (int)g.novelQ, (int)g.leafNovel);
            long probeQ = 0, realQ = 0;
            for (uint8_t v : t2.seenQ) { probeQ += (v >> 1) & 1; realQ += v & 1; }
            CHECK(probeQ == 9 && realQ == 0 && t2.newProbeQ.size() == 9 && t2.newProbeL.size() == 1 && t2.newRealQ.empty(), "look-ahead marks (%ld, %ld)", probeQ, realQ);
            Stats st;
            Rng grng(rng.next());
            Status s = run_game(g, ADV_RANDOM, c2, t2, grng, P, &script, st);
            realQ = 0;
            for (uint8_t v : t2.seenQ) realQ += v & 1;
            CHECK(s == ST_END && realQ == 9 && t2.newRealQ.size() == 9 && t2.newRealL.size() == 1, "real-game marks (%ld)", realQ);
            CHECK(st.uHist[7] == 1 && st.leafHist[8] == 1 && st.sig[0][0] == 1 && st.maxOccHist[1] == 1 && st.ended == 1, "per-game instrumentation");
            // script-only replay stops after the script
            Stats st2;
            Game g3;
            Rng r3(1);
            std::vector<int> pre(script.begin(), script.begin() + 5);
            s = run_game(g3, ADV_RANDOM, c2, t2, r3, P, &pre, st2, true);
            CHECK(s == ST_OK && g3.t == 5 && st2.ended == 0 && st2.bugs() == 0, "script-only replay");
        }
        // I6: bugs seen only in look-ahead are recorded (free play on the tiny table: holes everywhere)
        {
            SynTable syn = make_syn(dir, "syn_la", 0, 0);
            files.push_back(syn.tbl);
            files.push_back(syn.leaves);
            Store tb, lv;
            load_file(syn.tbl, true, tb, false);
            load_file(syn.leaves, false, lv, false);
            Ctx c2;
            c2.table = &tb;
            c2.leaves = &lv;
            for (int adv : {(int)ADV_SEARCH, (int)ADV_TREE, (int)ADV_DANGER}) {
                TS t2;
                Stats st;
                for (int gi = 0; gi < 20; gi++) {
                    Game g;
                    Rng grng(rng.next());
                    run_game(g, adv, c2, t2, grng, P, nullptr, st);
                }
                CHECK(t2.laBugs > 0 && !t2.laBugJson.empty(), "%s: look-ahead bugs are recorded (%ld)", ADV_NAME[adv], t2.laBugs);
                CHECK(st.holes >= 15, "%s: holes of the tiny table are found in real play (%ld of 20)", ADV_NAME[adv], st.holes);
            }
        }
        // I8: leaf stress test.  Positive: C(T8,{},8) holds, no failure.  Negative: slack(T7) = 1, so with two
        // unrevealed stones almost every admissible X has no pairing although the real X = {} has one.
        {
            Ctx c2 = ctx;
            c2.leafStress = 50;
            TS t2;
            Game g;
            setup_game(g, T8, EMPTYSET, EMPTYSET, 8);
            leaf_stress(g, c2, t2);
            CHECK(t2.stressTests == 50 && t2.stressFail == 0, "leaf stress on (T8,{},8): %ld failures in %ld tests", t2.stressFail, t2.stressTests);
            TS t3;
            Game g7;
            setup_game(g7, T7, EMPTYSET, EMPTYSET, 2);
            leaf_stress(g7, c2, t3);
            CHECK(t3.stressTests == 50 && t3.stressFail >= 40 && !t3.stressJson.empty(), "leaf stress on (T7,{},2): %ld failures in %ld tests", t3.stressFail, t3.stressTests);
            printf("    leaf stress: (T8,{},8) %ld failures / %ld tests (expected 0), (T7,{},2) %ld failures / %ld tests (expected most)\n",
                   t2.stressFail, t2.stressTests, t3.stressFail, t3.stressTests);
        }
        // I7: the new adversaries against the weak total policy (leaf forced at round 12): what they are built for
        {
            Ctx c2;
            c2.table = &emptyT;
            c2.leaves = &emptyL;
            c2.fakeLeafRound = 12;
            long dangerGames = 0, twoThreat = 0, cornerOcc = 0, treeEvents = 0;
            const int NGm = 200;
            for (int adv : {(int)ADV_DANGER, (int)ADV_DOUBLE, (int)ADV_CORNER, (int)ADV_TREE}) {
                TS t2;
                Stats st;
                double ta = now();
                long illegalMoves = 0;
                for (int gi = 0; gi < NGm; gi++) {
                    Game g;
                    Rng grng(rng.next());
                    run_game(g, adv, c2, t2, grng, P, nullptr, st);
                    if (g.st == ST_INTERNAL) illegalMoves++;
                    if (adv == ADV_DANGER) dangerGames += (g.nDanger > 0);
                    if (adv == ADV_DOUBLE) twoThreat += (g.nThreat >= 2);
                    if (adv == ADV_CORNER) cornerOcc += g.nOcc;
                    if (adv == ADV_TREE) treeEvents += g.nThreat + g.nDanger;
                }
                CHECK(illegalMoves == 0, "%s: no internal error", ADV_NAME[adv]);
                printf("    %-6s vs weak policy: %d games, threat events %ld, danger events %ld, occupied reveals %ld, admissibility "
                       "failures %ld, pairing failures %ld, %.2f ms per game\n",
                       ADV_NAME[adv], NGm, st.threat, st.danger, st.occupied, st.adm_fail, st.pairing_fail, (now() - ta) * 1e3 / NGm);
            }
            CHECK(dangerGames >= NGm / 2, "danger adversary creates danger pairs (%ld of %d games)", dangerGames, NGm);
            CHECK(twoThreat >= NGm / 4, "double adversary makes two threats (%ld of %d games)", twoThreat, NGm);
            CHECK(cornerOcc >= NGm, "corner adversary forces occupied reveals (%ld in %d games)", cornerOcc, NGm);
            CHECK(treeEvents >= NGm / 2, "tree adversary forces events (%ld in %d games)", treeEvents, NGm);
        }
    }
    if (!keep)
        for (const auto& f : files) unlink(f.c_str());
    else
        printf("    synthetic files kept in %s\n", dir.c_str());
    if (!keep && tmpdirArg.empty()) rmdir(dir.c_str());

    printf("== self-test %s (%d failed checks, %.1f s) ==\n", g_fail ? "FAILED" : "passed", g_fail, now() - t0);
    return g_fail ? 1 : 0;
}

// ----------------------------------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------------------------------
static void usage() {
    fprintf(stderr,
            "usage: sim --table F --leaves F --games N --seed S --adversary NAME --threads T --out FILE.json\n"
            "           [--script c1,c2,..] [--script-only] [--trace] [--p 0.1] [--search-depth 2] [--search-width 8]\n"
            "           [--tree-playouts 16] [--mix-weights r,g,s,t,x,d,dd,c,tr] [--verify-canonical] [--no-cache]\n"
            "           [--paranoid] [--batch 64] [--no-guidance] [--leaf-stress 4] [--max-bugs 20] [--bugs-out FILE.jsonl]\n"
            "           [--uncovered-out FILE] [--progress]\n"
            "       sim --selftest [--seed S] [--tmpdir DIR] [--keep]\n"
            "adversaries: random greedy steal threat search danger double corner tree mix\n"
            "exit code: 0 no bug; 1 bug games, look-ahead bugs, leaf stress failures, paranoid failures or file anomalies;\n"
            "           2 usage / file error; 3 internal error (geometry, game count)\n");
}

static std::string hist_json(const long* h, int n) {
    std::string s = "{";
    bool first = true;
    for (int i = 0; i < n; i++)
        if (h[i]) {
            if (!first) s += ",";
            first = false;
            s += "\"" + std::to_string(i) + "\":" + std::to_string(h[i]);
        }
    return s + "}";
}

}   // namespace

int main(int argc, char** argv) {
    std::string tablePath, leavesPath, outPath, advName = "mix", scriptStr, tmpdir, bugsOut, uncoveredOut;
    long games = 1000, batch = 64;
    int leafStress = 4;
    u64 seed = 1;
    int threads = 1, maxBugs = 20;
    bool doSelftest = false, verifyCanon = false, noCache = false, progress = false, keep = false;
    bool paranoid = false, guidance = true, scriptOnly = false, trace = false;
    Params P;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value for %s\n", name);
                usage();
                exit(2);
            }
            return argv[++i];
        };
        if (a == "--table") tablePath = need("--table");
        else if (a == "--leaves") leavesPath = need("--leaves");
        else if (a == "--games") games = atol(need("--games"));
        else if (a == "--seed") seed = strtoull(need("--seed"), nullptr, 10);
        else if (a == "--adversary") advName = need("--adversary");
        else if (a == "--threads") threads = atoi(need("--threads"));
        else if (a == "--out") outPath = need("--out");
        else if (a == "--script") scriptStr = need("--script");
        else if (a == "--script-only") scriptOnly = true;
        else if (a == "--trace") trace = true;
        else if (a == "--p") P.p = atof(need("--p"));
        else if (a == "--search-depth") P.depth = atoi(need("--search-depth"));
        else if (a == "--search-width") P.width = atoi(need("--search-width"));
        else if (a == "--tree-playouts") P.playouts = atoi(need("--tree-playouts"));
        else if (a == "--batch") batch = atol(need("--batch"));
        else if (a == "--leaf-stress") leafStress = atoi(need("--leaf-stress"));
        else if (a == "--mix-weights") {
            std::string w = need("--mix-weights");
            int k = 0;
            size_t pos = 0;
            for (int q = 0; q < ADV_COUNT; q++) P.mixw[q] = 0;
            while (k < ADV_COUNT && pos <= w.size()) {
                size_t e = w.find(',', pos);
                if (e == std::string::npos) e = w.size();
                P.mixw[k++] = atof(w.substr(pos, e - pos).c_str());
                pos = e + 1;
            }
            double tot = 0;
            for (int q = 0; q < ADV_COUNT; q++) {
                if (P.mixw[q] < 0) tot = -1e9;
                tot += P.mixw[q];
            }
            if (k < 1 || pos <= w.size() || tot <= 0) {
                fprintf(stderr, "--mix-weights needs 1..%d non-negative numbers with a positive sum (missing ones are 0)\n", (int)ADV_COUNT);
                return 2;
            }
        } else if (a == "--max-bugs") maxBugs = atoi(need("--max-bugs"));
        else if (a == "--bugs-out") bugsOut = need("--bugs-out");
        else if (a == "--uncovered-out") uncoveredOut = need("--uncovered-out");
        else if (a == "--tmpdir") tmpdir = need("--tmpdir");
        else if (a == "--selftest") doSelftest = true;
        else if (a == "--verify-canonical") verifyCanon = true;
        else if (a == "--no-cache") noCache = true;
        else if (a == "--paranoid") paranoid = true;
        else if (a == "--no-guidance") guidance = false;
        else if (a == "--progress") progress = true;
        else if (a == "--keep") keep = true;
        else {
            fprintf(stderr, "unknown option %s\n", a.c_str());
            usage();
            return 2;
        }
    }
    build_geometry();
    init_adv_tables();
    if (doSelftest) return selftest(seed, tmpdir, keep);

    int adv = -1;
    for (int i = 0; i <= ADV_MIX; i++)
        if (advName == ADV_NAME[i]) adv = i;
    if (adv < 0 || tablePath.empty() || leavesPath.empty() || games < 0 || threads < 1 || P.depth < 1 || P.width < 1 ||
        P.playouts < 1 || batch < 1 || maxBugs < 0) {
        usage();
        return 2;
    }
    std::vector<int> script;
    if (!scriptStr.empty()) {
        size_t pos = 0;
        while (pos < scriptStr.size()) {
            size_t e = scriptStr.find(',', pos);
            if (e == std::string::npos) e = scriptStr.size();
            std::string tok = scriptStr.substr(pos, e - pos);
            bool digits = !tok.empty();
            for (char ch : tok) digits &= (ch >= '0' && ch <= '9');
            int c = digits ? atoi(tok.c_str()) : -1;
            if (c < 0 || c >= NC) {
                fprintf(stderr, "bad cell in --script\n");
                return 2;
            }
            script.push_back(c);
            pos = e + 1;
        }
    }
    if (scriptOnly && script.empty()) {
        fprintf(stderr, "--script-only needs --script\n");
        return 2;
    }
    omp_set_num_threads(threads);
    double tl = wall_now();
    Store table, leaves;
    LoadInfo li1 = load_file(tablePath, true, table, verifyCanon);
    LoadInfo li2 = load_file(leavesPath, false, leaves, verifyCanon);
    double loadSec = wall_now() - tl;
    fprintf(stderr, "loaded %ld table lines (%zu distinct, %ld duplicates, %ld conflicting), %ld leaf lines (%zu distinct, %ld duplicates) in %.1f s\n",
            li1.lines, table.n, table.duplicates, table.conflicts, li2.lines, leaves.n, leaves.duplicates, loadSec);
    long fileAnomalies = table.conflicts + li1.badShape + li2.badShape + li1.noncanonical + li2.noncanonical;
    if (fileAnomalies)
        fprintf(stderr, "FILE ANOMALIES: %ld conflicting table duplicates, %ld + %ld entries with impossible sizes, %ld + %ld non-canonical entries\n",
                table.conflicts, li1.badShape, li2.badShape, li1.noncanonical, li2.noncanonical);
    Ctx ctx;
    ctx.table = &table;
    ctx.leaves = &leaves;
    ctx.useCache = !noCache;
    ctx.paranoid = paranoid;
    ctx.leafStress = std::max(0, leafStress);

    // coverage: the bitmaps are only updated between batches of games, so every game sees a snapshot that is a
    // deterministic function of (seed, game index, batch size): results do not depend on --threads.
    std::vector<uint8_t> covQ(table.n + 1, 0), covL(leaves.n + 1, 0), prbQ(table.n + 1, 0), prbL(leaves.n + 1, 0);
    if (guidance) {
        ctx.covQ = covQ.data();
        ctx.covL = covL.data();
        ctx.prbQ = prbQ.data();
        ctx.prbL = prbL.data();
    }

    std::vector<TS> tss(threads);
    std::vector<std::vector<Stats>> sts(threads, std::vector<Stats>(ADV_COUNT));
    std::vector<std::vector<BugRec>> tbugs(threads);
    std::vector<std::vector<long>> tAdvBugs(threads, std::vector<long>(ADV_COUNT, 0));
    FILE* bf = nullptr;
    if (!bugsOut.empty()) {
        bf = fopen(bugsOut.c_str(), "wb");
        if (!bf) {
            fprintf(stderr, "cannot write %s\n", bugsOut.c_str());
            return 2;
        }
    }
    double tg = wall_now();
    long nextProgress = std::max(1L, games / 20);
    for (long b0 = 0; b0 < games; b0 += batch) {
        long b1 = std::min(games, b0 + batch);
#pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
        for (long i = b0; i < b1; i++) {
            int tid = omp_get_thread_num();
            TS& ts = tss[tid];
            Game g;
            u64 gs = game_seed(seed, i);
            Rng rng(gs);
            int a = pick_adv(adv, rng, P);
            double c0 = now();
            Status s = run_game(g, a, ctx, ts, rng, P, script.empty() ? nullptr : &script, sts[tid][a], scriptOnly,
                                (trace && i == 0) ? stderr : nullptr);
            sts[tid][a].cpu += now() - c0;
            if (is_bug(s)) {
                tAdvBugs[tid][a]++;
                std::string js = bug_json(i, gs, ADV_NAME[a], g);
                if (bf) {
#pragma omp critical(bugfile)
                    {
                        fputs(js.c_str(), bf);
                        fputc('\n', bf);
                        fflush(bf);
                    }
                }
                std::vector<BugRec>& mybugs = tbugs[tid];
                mybugs.push_back(BugRec{i, std::move(js)});
                if ((int)mybugs.size() > 4 * maxBugs + 16) {
                    std::sort(mybugs.begin(), mybugs.end(), [](const BugRec& x, const BugRec& y) { return x.game < y.game; });
                    mybugs.resize(maxBugs);
                }
            }
        }
        for (TS& ts : tss) {
            for (uint32_t idx : ts.newRealQ) covQ[idx] = prbQ[idx] = 1;
            for (uint32_t idx : ts.newRealL) covL[idx] = prbL[idx] = 1;
            for (uint32_t idx : ts.newProbeQ) prbQ[idx] = 1;
            for (uint32_t idx : ts.newProbeL) prbL[idx] = 1;
            ts.newRealQ.clear(); ts.newRealL.clear(); ts.newProbeQ.clear(); ts.newProbeL.clear();
        }
        if (progress && b1 >= nextProgress) {
            long hq = 0, hl = 0;
            for (size_t k = 0; k < table.n; k++) hq += covQ[k];
            for (size_t k = 0; k < leaves.n; k++) hl += covL[k];
            fprintf(stderr, "  %ld / %ld games, %.0f s, table keys hit %ld / %zu, leaf keys hit %ld / %zu\n", b1, games,
                    wall_now() - tg, hq, table.n, hl, leaves.n);
            while (nextProgress <= b1) nextProgress += std::max(1L, games / 20);
        }
    }
    double sec = wall_now() - tg;

    Stats total;
    std::vector<Stats> perAdv(ADV_COUNT);
    long advBugs[ADV_COUNT] = {0};
    std::vector<BugRec> bugs;
    long canonCalls = 0, cacheHits = 0, laBugs = 0, stressTests = 0, stressFail = 0;
    std::vector<std::string> laJson, stressJson;
    for (int t = 0; t < threads; t++) {
        for (int a = 0; a < ADV_COUNT; a++) {
            perAdv[a].add(sts[t][a]);
            total.add(sts[t][a]);
            advBugs[a] += tAdvBugs[t][a];
        }
        canonCalls += tss[t].canonCalls;
        cacheHits += tss[t].cacheHits;
        laBugs += tss[t].laBugs;
        for (auto& j : tss[t].laBugJson) laJson.push_back(j);
        stressTests += tss[t].stressTests;
        stressFail += tss[t].stressFail;
        for (auto& j : tss[t].stressJson) stressJson.push_back(j);
        for (auto& b : tbugs[t]) bugs.push_back(std::move(b));
    }
    std::sort(laJson.begin(), laJson.end());
    std::sort(stressJson.begin(), stressJson.end());
    if (bf) {
        for (auto& j : laJson) {
            fputs(j.c_str(), bf);
            fputc('\n', bf);
        }
        for (auto& j : stressJson) {
            fputs(j.c_str(), bf);
            fputc('\n', bf);
        }
        fclose(bf);
    }
    if ((int)stressJson.size() > maxBugs) stressJson.resize(maxBugs);
    std::sort(bugs.begin(), bugs.end(), [](const BugRec& x, const BugRec& y) { return x.game < y.game; });
    if ((int)bugs.size() > maxBugs) bugs.resize(maxBugs);
    if ((int)laJson.size() > maxBugs) laJson.resize(maxBugs);

    // coverage tables
    const int TMAX = 32;
    long qTot[TMAX] = {0}, qHit[TMAX] = {0}, qPrb[TMAX] = {0}, lTot[TMAX] = {0}, lHit[TMAX] = {0}, lPrb[TMAX] = {0};
    long luTot[TMAX] = {0}, luHit[TMAX] = {0}, luPrb[TMAX] = {0};
    long qHitAll = 0, qPrbAll = 0, lHitAll = 0, lPrbAll = 0, nbTot = 0, nbHit = 0, nbPrb = 0, qF0Tot = 0, qF0Hit = 0;
    for (size_t k = 0; k < table.n; k++) {
        const Entry& e = table.ent[k];
        int t = std::min<int>(e.t, TMAX - 1);
        qTot[t]++; qHit[t] += covQ[k]; qPrb[t] += prbQ[k];
        qHitAll += covQ[k]; qPrbAll += prbQ[k];
        if (!ISBUSY[e.m]) { nbTot++; nbHit += covQ[k]; nbPrb += prbQ[k]; }
        if (e.F.empty()) { qF0Tot++; qF0Hit += covQ[k]; }
    }
    for (size_t k = 0; k < leaves.n; k++) {
        const Entry& e = leaves.ent[k];
        int t = std::min<int>(e.t, TMAX - 1);
        int u = std::min<int>(std::max(0, e.t - e.F.count()), TMAX - 1);
        lTot[t]++; lHit[t] += covL[k]; lPrb[t] += prbL[k];
        luTot[u]++; luHit[u] += covL[k]; luPrb[u] += prbL[k];
        lHitAll += covL[k]; lPrbAll += prbL[k];
    }
    auto cov_json = [&](const long* tot, const long* hit, const long* prb) {
        std::string s = "{";
        bool first = true;
        for (int t = 0; t < TMAX; t++)
            if (tot[t]) {
                if (!first) s += ",";
                first = false;
                s += "\"" + std::to_string(t) + "\":[" + std::to_string(hit[t]) + "," + std::to_string(prb[t]) + "," + std::to_string(tot[t]) + "]";
            }
        return s + "}";
    };

    if (!uncoveredOut.empty()) {   // the entries no real game has met: "Q t B F m probed" / "P t B F probed"
        FILE* f = fopen(uncoveredOut.c_str(), "wb");
        if (!f) {
            fprintf(stderr, "cannot write %s\n", uncoveredOut.c_str());
            return 2;
        }
        for (size_t k = 0; k < table.n; k++)
            if (!covQ[k]) {
                const Entry& e = table.ent[k];
                fprintf(f, "Q %d %s %s %d %d\n", (int)e.t, set_str(e.B).c_str(), set_str(e.F).c_str(), (int)e.m, (int)prbQ[k]);
            }
        for (size_t k = 0; k < leaves.n; k++)
            if (!covL[k]) {
                const Entry& e = leaves.ent[k];
                fprintf(f, "P %d %s %s %d\n", (int)e.t, set_str(e.B).c_str(), set_str(e.F).c_str(), (int)prbL[k]);
            }
        fclose(f);
    }
    long expectGames = games;
    bool countOk = (total.games == expectGames) && (scriptOnly || total.ended + total.bugs() == total.games);

    std::string js = "{";
    js += "\"games\":" + std::to_string(total.games);
    js += ",\"adversary\":\"" + advName + "\"";
    js += ",\"seed\":" + std::to_string(seed);
    js += ",\"threads\":" + std::to_string(threads);
    js += ",\"fp_lines\":" + std::to_string(total.fp_lines);
    js += ",\"holes\":" + std::to_string(total.holes);
    js += ",\"pairing_failures\":" + std::to_string(total.pairing_fail);
    js += ",\"admissibility_failures\":" + std::to_string(total.adm_fail);
    js += ",\"illegal_table_moves\":" + std::to_string(total.illegal);
    js += ",\"pairing_invariant_failures\":" + std::to_string(total.pair_broken);
    js += ",\"internal_errors\":" + std::to_string(total.internal);
    js += ",\"bug_games\":" + std::to_string(total.bugs());
    js += ",\"lookahead_bugs\":" + std::to_string(laBugs);
    js += ",\"paranoid_failures\":" + std::to_string(g_paranoidFail);
    js += ",\"leaf_stress_tests\":" + std::to_string(stressTests) + ",\"leaf_stress_failures\":" + std::to_string(stressFail);
    js += ",\"file_anomalies\":" + std::to_string(fileAnomalies);
    js += ",\"game_count_ok\":" + std::string(countOk ? "true" : "false");
    js += ",\"completed_games\":" + std::to_string(total.ended);
    js += ",\"leaf_round_hist\":";
    {
        std::string h = hist_json(total.leafHist, NC + 2);
        if (total.noLeaf) {
            h.pop_back();
            if (h.size() > 1) h += ",";
            h += "\"none\":" + std::to_string(total.noLeaf) + "}";
        }
        js += h;
    }
    js += ",\"threat_events\":" + std::to_string(total.threat);
    js += ",\"danger_events\":" + std::to_string(total.danger);
    js += ",\"occupied_reveals\":" + std::to_string(total.occupied);
    js += ",\"table_moves\":" + std::to_string(total.table_moves);
    js += ",\"max_table_round\":" + std::to_string(total.maxTableRound);
    js += ",\"total_rounds\":" + std::to_string(total.rounds);
    char tmp[512];
    snprintf(tmp, sizeof tmp, ",\"games_per_second\":%.1f,\"seconds\":%.2f,\"load_seconds\":%.2f", sec > 0 ? total.games / sec : 0.0, sec, loadSec);
    js += tmp;
    js += ",\"canon_calls\":" + std::to_string(canonCalls) + ",\"cache_hits\":" + std::to_string(cacheHits);
    js += ",\"table_entries\":" + std::to_string(table.n) + ",\"leaf_entries\":" + std::to_string(leaves.n);
    js += ",\"table_conflicts\":" + std::to_string(table.conflicts);
    js += ",\"table_duplicates\":" + std::to_string(table.duplicates) + ",\"leaf_duplicates\":" + std::to_string(leaves.duplicates);
    js += ",\"bad_shape_entries\":" + std::to_string(li1.badShape + li2.badShape);
    if (verifyCanon) js += ",\"noncanonical_entries\":" + std::to_string(li1.noncanonical + li2.noncanonical);
    // ---- coverage and instrumentation
    js += ",\n\"coverage\":{\"note\":\"hit = met in a real game; probed = met in a real game or in a look-ahead simulation of sigma (all TABLE-mode detectors and the pairing computation run there too); arrays are [hit, probed, total]\"";
    js += ",\"table_keys\":[" + std::to_string(qHitAll) + "," + std::to_string(qPrbAll) + "," + std::to_string(table.n) + "]";
    js += ",\"leaf_keys\":[" + std::to_string(lHitAll) + "," + std::to_string(lPrbAll) + "," + std::to_string(leaves.n) + "]";
    snprintf(tmp, sizeof tmp, ",\"table_keys_hit_fraction\":%.4f,\"leaf_keys_hit_fraction\":%.4f,\"table_keys_probed_fraction\":%.4f,\"leaf_keys_probed_fraction\":%.4f",
             table.n ? (double)qHitAll / table.n : 0.0, leaves.n ? (double)lHitAll / leaves.n : 0.0,
             table.n ? (double)qPrbAll / table.n : 0.0, leaves.n ? (double)lPrbAll / leaves.n : 0.0);
    js += tmp;
    js += ",\"table_keys_by_t\":" + cov_json(qTot, qHit, qPrb);
    js += ",\"leaf_keys_by_t\":" + cov_json(lTot, lHit, lPrb);
    js += ",\"leaf_keys_by_t_minus_F\":" + cov_json(luTot, luHit, luPrb);
    js += ",\"table_entries_with_nonbusy_move\":[" + std::to_string(nbHit) + "," + std::to_string(nbPrb) + "," + std::to_string(nbTot) + "]";
    js += ",\"table_entries_with_empty_F\":[" + std::to_string(qF0Hit) + "," + std::to_string(qF0Tot) + "]";
    js += "}";
    js += ",\n\"leaf_t_minus_F_hist\":" + hist_json(total.uHist, NC + 2);
    js += ",\"nonbusy_table_moves\":" + std::to_string(total.nonbusy_moves);
    js += ",\"games_with_nonbusy_table_move\":" + std::to_string(total.nonbusy_games);
    js += ",\"rounds_with_2plus_occupied_reveals\":" + std::to_string(total.multi_occ_rounds);
    js += ",\"games_with_2plus_occupied_reveals_in_a_round\":" + std::to_string(total.multi_occ_games);
    js += ",\"max_occupied_reveals_in_a_round_hist\":" + hist_json(total.maxOccHist, 8);
    js += ",\"leaves_after_threat\":" + std::to_string(total.leaf_after_threat);
    js += ",\"leaves_after_danger\":" + std::to_string(total.leaf_after_danger);
    js += ",\"leaves_after_two_threats\":" + std::to_string(total.leaf_after_two_threats);
    js += ",\"leaves_after_threat_and_danger\":" + std::to_string(total.leaf_after_threat_and_danger);
    js += ",\"leaf_event_signature\":{";
    {
        bool first = true;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++)
                if (total.sig[i][j]) {
                    if (!first) js += ",";
                    first = false;
                    js += "\"T" + std::to_string(i) + "D" + std::to_string(j) + "\":" + std::to_string(total.sig[i][j]);
                }
    }
    js += "}";
    js += ",\"threat_events_by_round\":" + hist_json(total.evHist[0], 16);
    js += ",\"danger_events_by_round\":" + hist_json(total.evHist[1], 16);
    js += ",\"min_cells_missing_on_a_live_line_hist\":" + hist_json(total.minEmptyHist, 8);
    js += ",\"table_rounds_ending_with_4_on_a_live_line\":" + std::to_string(total.near4_rounds);
    js += ",\n\"per_adversary\":{";
    bool first = true;
    for (int a = 0; a < ADV_COUNT; a++)
        if (perAdv[a].games) {
            const Stats& s = perAdv[a];
            if (!first) js += ",";
            first = false;
            long l10 = 0;
            for (int r = 10; r < NC + 2; r++) l10 += s.leafHist[r];
            snprintf(tmp, sizeof tmp,
                     "\n\"%s\":{\"games\":%ld,\"bugs\":%ld,\"threat_events\":%ld,\"danger_events\":%ld,\"occupied_reveals\":%ld,"
                     "\"leaves_after_two_threats\":%ld,\"leaves_after_danger\":%ld,\"leaf_round_ge_10\":%ld,\"nonbusy_table_moves\":%ld,"
                     "\"rounds_with_2plus_occupied_reveals\":%ld,\"cpu_seconds\":%.2f,\"cpu_ms_per_game\":%.3f}",
                     ADV_NAME[a], s.games, advBugs[a], s.threat, s.danger, s.occupied, s.leaf_after_two_threats, s.leaf_after_danger,
                     l10, s.nonbusy_moves, s.multi_occ_rounds, s.cpu, s.games ? 1e3 * s.cpu / s.games : 0.0);
            js += tmp;
        }
    js += "}";
    snprintf(tmp, sizeof tmp, ",\n\"params\":{\"p\":%g,\"search_depth\":%d,\"search_width\":%d,\"tree_playouts\":%d,\"batch\":%ld,\"guidance\":%s,"
             "\"paranoid\":%s,\"cache\":%s,\"script_len\":%d,\"script_only\":%s,\"leaf_stress\":%d,\"mix_weights\":[",
             P.p, P.depth, P.width, P.playouts, batch, guidance ? "true" : "false", paranoid ? "true" : "false",
             noCache ? "false" : "true", (int)script.size(), scriptOnly ? "true" : "false", ctx.leafStress);
    js += tmp;
    for (int a = 0; a < ADV_COUNT; a++) {
        snprintf(tmp, sizeof tmp, "%s%g", a ? "," : "", P.mixw[a]);
        js += tmp;
    }
    js += "]}";
    js += ",\n\"bugs\":[";
    for (size_t i = 0; i < bugs.size(); i++) {
        if (i) js += ",";
        js += "\n" + bugs[i].json;
    }
    js += "],\"lookahead_bug_records\":[";
    for (size_t i = 0; i < laJson.size(); i++) {
        if (i) js += ",";
        js += "\n" + laJson[i];
    }
    js += "],\"leaf_stress_records\":[";
    for (size_t i = 0; i < stressJson.size(); i++) {
        if (i) js += ",";
        js += "\n" + stressJson[i];
    }
    js += "]}\n";
    if (!outPath.empty()) {
        FILE* f = fopen(outPath.c_str(), "wb");
        if (!f) {
            fprintf(stderr, "cannot write %s\n", outPath.c_str());
            return 2;
        }
        fputs(js.c_str(), f);
        fclose(f);
    } else
        fputs(js.c_str(), stdout);
    fprintf(stderr,
            "%ld games, %ld bug games (fp_lines %ld, holes %ld, pairing %ld, admissibility %ld, illegal %ld, pairing_invariant %ld, "
            "internal %ld), look-ahead bugs %ld, paranoid failures %ld, file anomalies %ld, leaf stress failures %ld of %ld tests, %.1f games/s\n"
            "coverage: table keys hit %ld / %zu (probed %ld), leaf keys hit %ld / %zu (probed %ld); threat events %ld, danger events %ld, "
            "leaves after two threats %ld\n",
            total.games, total.bugs(), total.fp_lines, total.holes, total.pairing_fail, total.adm_fail, total.illegal, total.pair_broken,
            total.internal, laBugs, g_paranoidFail, fileAnomalies, stressFail, stressTests, sec > 0 ? total.games / sec : 0.0, qHitAll, table.n, qPrbAll, lHitAll,
            leaves.n, lPrbAll, total.threat, total.danger, total.leaf_after_two_threats);
    if (!countOk) {
        fprintf(stderr, "INTERNAL ERROR: %ld games counted, %ld requested, %ld finished, %ld bugs\n", total.games, expectGames, total.ended, total.bugs());
        return 3;
    }
    return (total.bugs() || laBugs || g_paranoidFail || fileAnomalies || stressFail) ? 1 : 0;
}

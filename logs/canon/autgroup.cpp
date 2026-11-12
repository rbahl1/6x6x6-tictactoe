// autgroup.cpp -- scratch program of task 1 (not part of the proof): order of the FULL automorphism group
// of the line hypergraph (216 cells, 148 lines) by individualisation + colour refinement on the
// cell-line incidence graph, using orbit-stabiliser:  |Aut| = prod over the base points v_i of
// |orbit of v_i under the pointwise stabiliser of v_1..v_{i-1}|.
// Build: g++ -O2 -std=c++17 -I src -o logs/canon/autgroup logs/canon/autgroup.cpp
#include "geom.hpp"
#include <map>
using namespace ttt;
static const int NV = NC + NL;
static std::vector<int> adj[NV];
static long nodes = 0;

struct Col { std::vector<int> a, b; int ncol; };   // colours of the source side and of the target side

// joint refinement; returns false if the two sides become distinguishable
static bool refine(Col& c) {
  for (;;) {
    std::map<std::vector<int>, int> ids;
    std::vector<std::vector<int>> sa(NV), sb(NV);
    for (int side = 0; side < 2; side++) {
      const std::vector<int>& col = side ? c.b : c.a;
      for (int v = 0; v < NV; v++) {
        std::vector<int> s;
        for (int u : adj[v]) s.push_back(col[u]);
        std::sort(s.begin(), s.end());
        s.insert(s.begin(), col[v]);
        ids[s] = 0;
        (side ? sb : sa)[v] = s;
      }
    }
    int n = 0;
    for (auto& p : ids) p.second = n++;
    std::vector<int> ha(n, 0), hb(n, 0);
    for (int v = 0; v < NV; v++) { c.a[v] = ids[sa[v]]; c.b[v] = ids[sb[v]]; ha[c.a[v]]++; hb[c.b[v]]++; }
    if (ha != hb) return false;
    if (n == c.ncol) return true;
    c.ncol = n;
  }
}

// first colour class with more than one vertex (smallest class first, ties by colour); -1 if discrete
static int pick_class(const Col& c) {
  std::vector<int> h(c.ncol, 0);
  for (int v = 0; v < NV; v++) h[c.a[v]]++;
  int best = -1;
  for (int k = 0; k < c.ncol; k++) if (h[k] > 1 && (best < 0 || h[k] < h[best])) best = k;
  return best;
}

static bool is_automorphism(const Col& c) {
  const Geom& geo = G();
  std::vector<int> inv(c.ncol), pi(NV);
  for (int v = 0; v < NV; v++) inv[c.b[v]] = v;
  for (int v = 0; v < NV; v++) pi[v] = inv[c.a[v]];
  for (int l = 0; l < NL; l++) {
    int im = pi[NC + l] - NC;
    if (im < 0) return false;
    for (int j = 0; j < 6; j++) { int x = pi[geo.line[l][j]]; if (x >= NC || !geo.lineSet[im].test(x)) return false; }
  }
  return true;
}

static bool exists(Col c) {     // is there an isomorphism compatible with the colourings?
  nodes++;
  if (!refine(c)) return false;
  int k = pick_class(c);
  if (k < 0) return is_automorphism(c);
  int v = -1;
  for (int u = 0; u < NV; u++) if (c.a[u] == k) { v = u; break; }
  for (int w = 0; w < NV; w++) if (c.b[w] == k) {
    Col d = c; d.a[v] = d.ncol; d.b[w] = d.ncol; d.ncol++;
    if (exists(d)) return true;
  }
  return false;
}

int main() {
  const Geom& geo = G();
  for (int l = 0; l < NL; l++) for (int j = 0; j < 6; j++) { adj[NC + l].push_back(geo.line[l][j]); adj[geo.line[l][j]].push_back(NC + l); }
  Col c; c.a.assign(NV, 0); c.b.assign(NV, 0);
  for (int l = 0; l < NL; l++) c.a[NC + l] = c.b[NC + l] = 1;
  c.ncol = 2;
  unsigned long long order = 1;
  for (int level = 0;; level++) {
    if (!refine(c)) { printf("internal error\n"); return 1; }
    int k = pick_class(c);
    if (k < 0) break;
    int v = -1, size = 0, orbit = 0;
    for (int u = 0; u < NV; u++) if (c.a[u] == k) { if (v < 0) v = u; size++; }
    for (int w = 0; w < NV; w++) if (c.b[w] == k) {
      Col d = c; d.a[v] = d.ncol; d.b[w] = d.ncol; d.ncol++;
      if (exists(d)) orbit++;
    }
    printf("level %d: base vertex %d (%s), colour class size %d, orbit size %d\n", level, v, v < NC ? "cell" : "line", size, orbit);
    order *= orbit;
    c.a[v] = c.ncol; c.b[v] = c.ncol; c.ncol++;      // fix v pointwise
  }
  printf("full automorphism group order = %llu   (G has %d; search nodes %ld)\n", order, NG, nodes);
  printf("%s\n", order == (unsigned long long)NG ? "EQUAL: the 1152 maps are the full automorphism group" : "DIFFERENT from 1152");
  return 0;
}

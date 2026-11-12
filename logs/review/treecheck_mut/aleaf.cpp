// aleaf.cpp -- auditor's UNPRUNED flow reference for LEAF(B,F,k) on real-size instances (SPEC 3, "given 1 and 2,
// condition 3 is equivalent to: every F-connected triple T has mu(T) >= k-|F|").  Own geometry, explicit flow
// network (source -> line cap 2, line -> cell cap 1 for cells of the line outside F, cell -> sink cap 1), BFS
// augmenting paths (Edmonds-Karp).  mu(T) is measured by adding a node X with source -> X of capacity theta and
// X -> each line of T with unbounded capacity on top of the saturated base flow.  No pruning rule is used: every
// triple {a,b,c} of live lines in which at least two of the three pairs are F-adjacent is tested.
//   aleaf < "k B F" lines   -> one line "reason minmu_capped triples" per instance
// g++ -O2 -std=c++17 -o aleaf aleaf.cpp
#include <bits/stdc++.h>
using namespace std;
typedef bitset<216> BS;
static vector<array<int, 6>> LN;
static vector<BS> LB;

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
          if (ok) { sort(a.begin(), a.end()); s.insert(a); }
        }
      }
  LN.assign(s.begin(), s.end());
  if (LN.size() != 148) exit(3);
  for (auto& l : LN) {
    BS b;
    for (int c : l) b.set(c);
    LB.push_back(b);
  }
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
static bool parseSet(const string& s, BS& out) {
  out.reset();
  if (s == "-") return true;
  size_t p = 0;
  while (p < s.size()) {
    size_t q = s.find(',', p);
    if (q == string::npos) q = s.size();
    int v = atoi(s.substr(p, q - p).c_str());
    if (v < 0 || v > 215) return false;
    out.set(v);
    p = q + 1;
  }
  return true;
}

struct Net {
  struct E { int to, cap; };
  vector<E> e;
  vector<vector<int>> adj;
  vector<pair<int, int>> log;     // (edge index, old cap)
  bool logging = false;
  int add(int u, int v, int cap) {
    e.push_back({v, cap});
    adj[u].push_back((int)e.size() - 1);
    e.push_back({u, 0});
    adj[v].push_back((int)e.size() - 1);
    return (int)e.size() - 2;
  }
  void setcap(int idx, int cap) {
    if (logging) log.push_back({idx, e[idx].cap});
    e[idx].cap = cap;
  }
  bool augment(int s, int t) {
    vector<int> pe(adj.size(), -1);
    vector<char> vis(adj.size(), 0);
    deque<int> q = {s};
    vis[s] = 1;
    while (!q.empty() && !vis[t]) {
      int u = q.front();
      q.pop_front();
      for (int id : adj[u])
        if (e[id].cap > 0 && !vis[e[id].to]) {
          vis[e[id].to] = 1;
          pe[e[id].to] = id;
          q.push_back(e[id].to);
        }
    }
    if (!vis[t]) return false;
    for (int v = t; v != s;) {
      int id = pe[v];
      setcap(id, e[id].cap - 1);
      setcap(id ^ 1, e[id ^ 1].cap + 1);
      v = e[id ^ 1].to;
    }
    return true;
  }
  void rollback() {
    for (size_t i = log.size(); i-- > 0;) e[log[i].first].cap = log[i].second;
    log.clear();
  }
};

int main(int argc, char** argv) {
  const bool fast1 = argc > 1 && string(argv[1]) == "fast1";   // exact reformulation for theta = 1 (see below)
  geom();
  string line;
  while (getline(cin, line)) {
    if (line.empty()) continue;
    istringstream is(line);
    int k;
    string bs, fs;
    BS B, F;
    if (!(is >> k >> bs >> fs) || !parseSet(bs, B) || !parseSet(fs, F)) { printf("-1 0 0\n"); continue; }
    if (!admissibleLit(B, F)) { printf("1 0 0\n"); continue; }
    vector<int> live;
    for (int l = 0; l < 148; l++)
      if (!(LB[l] & B).any()) live.push_back(l);
    int n = (int)live.size();
    // nodes: 0 = source, 1 = sink, 2 = X, 3..3+n-1 lines, then cells
    Net net;
    int cellBase = 3 + n;
    net.adj.assign(cellBase + 216, {});
    vector<int> srcEdge(n);
    for (int i = 0; i < n; i++) {
      srcEdge[i] = net.add(0, 3 + i, 2);
      for (int c : LN[live[i]])
        if (!F[c]) net.add(3 + i, cellBase + c, 1);
    }
    for (int c = 0; c < 216; c++)
      if (!F[c] && !B[c]) net.add(cellBase + c, 1, 1);
    int base = 0;
    {
      // greedy start: every line takes up to 2 still unused cells; the BFS augmentations below complete the
      // flow to a maximum one (the result is a maximum flow whatever the start)
      for (int i = 0; i < n; i++) {
        int got = 0;
        for (int id : net.adj[3 + i]) {
          if (got == 2) break;
          if (id & 1) continue;                       // reverse edge of source -> line
          int cellNode = net.e[id].to;
          if (net.e[id].cap != 1) continue;
          int sinkEdge = -1;
          for (int id2 : net.adj[cellNode])
            if (!(id2 & 1) && net.e[id2].to == 1 && net.e[id2].cap == 1) sinkEdge = id2;
          if (sinkEdge < 0) continue;
          net.e[id].cap--; net.e[id ^ 1].cap++;
          net.e[sinkEdge].cap--; net.e[sinkEdge ^ 1].cap++;
          net.e[srcEdge[i]].cap--; net.e[srcEdge[i] ^ 1].cap++;
          got++;
          base++;
        }
      }
    }
    while (net.augment(0, 1)) base++;
    if (base != 2 * n) { printf("2 0 0\n"); continue; }
    int theta = k - (int)F.count();
    if (theta <= 0) { printf("0 0 0\n"); continue; }
    int sx = net.add(0, 2, 0);
    vector<int> xl(n);
    for (int i = 0; i < n; i++) xl[i] = net.add(2, 3 + i, 0);
    vector<vector<char>> adjF(n, vector<char>(n, 0));
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        if (i != j && (LB[live[i]] & LB[live[j]] & ~F).any()) adjF[i][j] = 1;
    long triples = 0;
    int minmu = theta;
    if (fast1 && theta == 1) {
      // theta = 1: an augmenting path from X starts with an edge X -> line of T, so mu(T) >= 1 iff some line of T
      // has a residual path to the sink in the base network.  One backward search from the sink finds these lines.
      vector<char> can(net.adj.size(), 0);
      deque<int> q = {1};
      can[1] = 1;
      while (!q.empty()) {
        int v = q.front();
        q.pop_front();
        for (int id : net.adj[v]) {
          // edge id goes v -> u; its partner id^1 goes u -> v: u can step to v if cap(id^1) > 0
          int u = net.e[id].to;
          if (!can[u] && net.e[id ^ 1].cap > 0) { can[u] = 1; q.push_back(u); }
        }
      }
      for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
          for (int h = j + 1; h < n; h++) {
            if (can[3 + i] || can[3 + j] || can[3 + h]) continue;
            if (adjF[i][j] + adjF[j][h] + adjF[i][h] < 2) continue;
            triples++;
            minmu = 0;
          }
      printf("%d %d %ld\n", minmu >= theta ? 0 : 3, minmu, triples);
      continue;
    }
    net.logging = true;
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++)
        for (int h = j + 1; h < n; h++) {
          if (adjF[i][j] + adjF[j][h] + adjF[i][h] < 2) continue;
          triples++;
          net.setcap(sx, theta);
          net.setcap(xl[i], 1000);
          net.setcap(xl[j], 1000);
          net.setcap(xl[h], 1000);
          int got = 0;
          while (got < theta && net.augment(0, 1)) got++;
          net.rollback();
          minmu = min(minmu, got);
        }
    printf("%d %d %ld\n", minmu >= theta ? 0 : 3, minmu, triples);
  }
  return 0;
}

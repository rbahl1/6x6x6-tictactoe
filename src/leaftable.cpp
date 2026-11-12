// leaftable.cpp -> bin/leaftable.  Task 1.
// For K in {8,9,10,...}: for every orbit representative B of the busy K-sets compute slack(B) and
// LEAF(B, {}, K) (leaf_holds from phi.hpp).  If slack(B) < K the answer is false without calling it:
//   sur_{}(L(B)) = |N(L(B))| - 2|L(B)| <= (216 - |B|) - 2|L(B)| = slack(B).  For F = {} the components of
//   L(B) have pairwise disjoint cell sets, so sur(L(B)) is the sum of the sur of the components; a
//   component of 1 or 2 lines has sur 4 or 7 > 0, and LEAF requires sur >= K for every component of >= 3
//   lines.  With |B| <= 24 busy cells at least 148 - 7*|B| lines are live; for |B| <= 10 that is >= 78
//   lines, which cannot all lie in components of <= 2 lines (39 * 11 > 216 cells), so LEAF implies
//   slack(B) >= sur(L(B)) >= K.
// With --no-shortcut leaf_holds is called for every orbit and the shortcut is checked against it.
//
// Usage:
//   bin/leaftable --k K [--shard i --nshards N] [--out DIR] [--no-shortcut]
//        writes DIR/phi_k<K>.shard<i>of<N>.jsonl, one line {"k":K,"B":"cells","slack":s,"leaf":bool}
//        per orbit representative with index = i (mod N); lines already present are skipped (resumable).
//   bin/leaftable --k K --list FILE      writes the instance lines "K B -" of all orbit representatives.
//   bin/leaftable --k K --merge [--out DIR]
//        merges all DIR/phi_k<K>.shard*of*.jsonl into DIR/phi_k<K>.jsonl (sorted by cmp of B), checks
//        that every orbit representative is present exactly once with consistent values, prints a summary.
// --k accepts a comma list (8,9,10).  Default DIR = results/leaftable.
#include "canon.hpp"
#include "match.hpp"
#include "phi.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace ttt;

struct Row { int k; std::string B; int slack; bool leaf; };

static bool parse_row(const std::string& line, Row& r) {
  auto num = [&](const char* key, int& out) {
    size_t p = line.find(key);
    if (p == std::string::npos) return false;
    out = atoi(line.c_str() + p + strlen(key));
    return true;
  };
  if (!num("\"k\":", r.k) || !num("\"slack\":", r.slack)) return false;
  size_t p = line.find("\"B\":\"");
  if (p == std::string::npos) return false;
  size_t q = line.find('"', p + 5);
  if (q == std::string::npos) return false;
  r.B = line.substr(p + 5, q - p - 5);
  size_t l = line.find("\"leaf\":");
  if (l == std::string::npos) return false;
  if (line.compare(l + 7, 4, "true") == 0) r.leaf = true;
  else if (line.compare(l + 7, 5, "false") == 0) r.leaf = false;
  else return false;
  return line.find('}', l) != std::string::npos;      // complete line
}

static std::string row_json(const Row& r) {
  std::ostringstream o;
  o << "{\"k\":" << r.k << ",\"B\":\"" << r.B << "\",\"slack\":" << r.slack << ",\"leaf\":" << (r.leaf ? "true" : "false") << "}";
  return o.str();
}

static std::string coords(const CellSet& s) {
  std::string o;
  s.for_each([&](int c) {
    if (!o.empty()) o += ' ';
    o += '(' + std::to_string(cx(c)) + ',' + std::to_string(cy(c)) + ',' + std::to_string(cz(c)) + ')';
  });
  return o;
}

static int do_list(int k, const std::string& file) {
  std::vector<uint32_t> reps = busy_orbit_reps(k);
  std::ofstream f(file);
  if (!f) { fprintf(stderr, "cannot write %s\n", file.c_str()); return 1; }
  for (uint32_t m : reps) f << k << ' ' << to_str(busy_set(m)) << " -\n";
  f.close();
  printf("k=%d: wrote %zu instance lines to %s\n", k, reps.size(), file.c_str());
  return 0;
}

static int do_shard(int k, int shard, int nshards, const std::string& dir, bool shortcut) {
  std::filesystem::create_directories(dir);
  std::string path = dir + "/phi_k" + std::to_string(k) + ".shard" + std::to_string(shard) + "of" + std::to_string(nshards) + ".jsonl";
  std::map<std::string, int> done;
  {
    std::ifstream in(path);
    std::string line;
    std::vector<std::string> good;
    bool truncated = false;
    while (std::getline(in, line)) {
      Row r;
      if (parse_row(line, r) && r.k == k) { done[r.B] = 1; good.push_back(line); }
      else if (!line.empty()) truncated = true;
    }
    in.close();
    if (truncated) {            // drop incomplete lines of an interrupted run
      std::ofstream out(path, std::ios::trunc);
      for (auto& l : good) out << l << '\n';
    }
  }
  std::vector<uint32_t> reps = busy_orbit_reps(k);
  FILE* out = fopen(path.c_str(), "a");
  if (!out) { fprintf(stderr, "cannot write %s\n", path.c_str()); return 1; }
  auto t0 = std::chrono::steady_clock::now();
  long n = 0, skipped = 0, nslack = 0, nleaf = 0, bad = 0;
  for (size_t i = 0; i < reps.size(); i++) {
    if ((int)(i % nshards) != shard) continue;
    CellSet B = busy_set(reps[i]);
    Row r; r.k = k; r.B = to_str(B);
    if (done.count(r.B)) { skipped++; continue; }
    r.slack = slack(B);
    if (r.slack >= k) { nslack++; r.leaf = leaf_holds(B, CellSet(), k); }
    else if (shortcut) r.leaf = false;
    else {
      r.leaf = leaf_holds(B, CellSet(), k);
      if (r.leaf) { bad++; fprintf(stderr, "SHORTCUT VIOLATION: slack %d < k=%d but leaf_holds is true: B=%s\n", r.slack, k, r.B.c_str()); }
    }
    if (r.leaf) nleaf++;
    fprintf(out, "%s\n", row_json(r).c_str());
    fflush(out);
    n++;
  }
  fclose(out);
  double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  printf("k=%d shard %d/%d: %zu orbits in total, computed %ld (skipped %ld already present), slack>=k %ld, LEAF true %ld, %.2f s -> %s\n",
         k, shard, nshards, reps.size(), n, skipped, nslack, nleaf, sec, path.c_str());
  return bad ? 2 : 0;
}

static int do_merge(int k, const std::string& dir) {
  namespace fs = std::filesystem;
  std::string prefix = "phi_k" + std::to_string(k) + ".shard";
  std::map<std::string, Row> rows;
  long nfiles = 0, nlines = 0, conflicts = 0, badlines = 0;
  if (fs::exists(dir)) for (auto& e : fs::directory_iterator(dir)) {
    std::string name = e.path().filename().string();
    if (name.compare(0, prefix.size(), prefix) != 0 || name.size() < 6 || name.substr(name.size() - 6) != ".jsonl") continue;
    nfiles++;
    std::ifstream in(e.path());
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty()) continue;
      Row r;
      if (!parse_row(line, r) || r.k != k) { badlines++; continue; }
      nlines++;
      auto it = rows.find(r.B);
      if (it == rows.end()) rows[r.B] = r;
      else if (it->second.slack != r.slack || it->second.leaf != r.leaf) conflicts++;
    }
  }
  std::vector<uint32_t> reps = busy_orbit_reps(k);
  std::string path = dir + "/phi_k" + std::to_string(k) + ".jsonl";
  std::ofstream out(path);
  long missing = 0, nslack = 0, nleaf = 0, slackbad = 0;
  std::vector<CellSet> leaves;
  std::map<int, std::pair<long, long>> bySlack;      // slack -> (orbits, LEAF true)
  for (uint32_t m : reps) {                          // reps are sorted by cmp
    CellSet B = busy_set(m);
    auto it = rows.find(to_str(B));
    if (it == rows.end()) { missing++; continue; }
    const Row& r = it->second;
    out << row_json(r) << '\n';
    if (r.slack >= k) nslack++;
    if (r.leaf) { nleaf++; leaves.push_back(B); if (r.slack < k) slackbad++; }
    bySlack[r.slack].first++;
    if (r.leaf) bySlack[r.slack].second++;
    rows.erase(it);
  }
  out.close();
  long extra = (long)rows.size();
  printf("k=%d merge: %ld shard files, %ld lines, %zu orbit representatives; missing %ld, extra (non-representative) %ld, "
         "conflicting duplicates %ld, unparsable lines %ld -> %s\n",
         k, nfiles, nlines, reps.size(), missing, extra, conflicts, badlines, path.c_str());
  printf("k=%d summary: orbits %zu, slack >= k: %ld, LEAF(B,-,k) true: %ld%s\n", k, reps.size(), nslack, nleaf,
         slackbad ? "  [ERROR: LEAF true with slack < k]" : "");
  printf("k=%d slack histogram (slack: orbits / LEAF true):", k);
  for (auto& p : bySlack) printf("  %d: %ld/%ld", p.first, p.second.first, p.second.second);
  printf("\n");
  if (nleaf <= 40) for (auto& B : leaves) printf("  LEAF-true k=%d: B=%s  = %s\n", k, to_str(B).c_str(), coords(B).c_str());
  int rc = (missing || extra || conflicts || badlines || slackbad) ? 2 : 0;
  if (k == 8) {     // sanity: the orbit of T8 must be LEAF-true
    CellSet T8 = {cid(0,0,0), cid(5,5,5), cid(1,1,4), cid(4,4,1), cid(2,3,2), cid(3,2,3), cid(5,0,0), cid(0,5,5)};
    CanonResult c = canon_pair(T8, CellSet());
    bool found = false;
    for (auto& B : leaves) if (B == c.B) found = true;
    printf("k=8 sanity: canonical form of T8 = %s = %s, slack %d, in the LEAF-true list: %s\n",
           to_str(c.B).c_str(), coords(c.B).c_str(), slack(c.B), found ? "yes" : "NO");
    if (!found) rc = 2;
  }
  return rc;
}

int main(int argc, char** argv) {
  std::vector<int> ks;
  int shard = 0, nshards = 1;
  std::string dir = "results/leaftable", listFile;
  bool merge = false, shortcut = true;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string { if (i + 1 >= argc) { fprintf(stderr, "missing value for %s\n", a.c_str()); exit(1); } return argv[++i]; };
    if (a == "--k") {
      std::string v = next(); size_t p = 0;
      while (p < v.size()) { size_t q = v.find(',', p); if (q == std::string::npos) q = v.size(); ks.push_back(atoi(v.substr(p, q - p).c_str())); p = q + 1; }
    }
    else if (a == "--shard") shard = atoi(next().c_str());
    else if (a == "--nshards") nshards = atoi(next().c_str());
    else if (a == "--out") dir = next();
    else if (a == "--list") listFile = next();
    else if (a == "--merge") merge = true;
    else if (a == "--no-shortcut") shortcut = false;
    else { fprintf(stderr, "unknown option %s\n", a.c_str()); return 1; }
  }
  if (ks.empty()) { fprintf(stderr, "usage: leaftable --k K[,K..] [--shard i --nshards N] [--out DIR] [--no-shortcut] | --list FILE | --merge\n"); return 1; }
  if (nshards < 1 || shard < 0 || shard >= nshards) { fprintf(stderr, "bad shard\n"); return 1; }
  if (!listFile.empty() && ks.size() != 1) { fprintf(stderr, "--list needs a single --k\n"); return 1; }
  int rc = 0;
  for (int k : ks) {
    if (k < 0 || k > NB) { fprintf(stderr, "bad k %d\n", k); return 1; }
    int r;
    if (!listFile.empty()) r = do_list(k, listFile);
    else if (merge) r = do_merge(k, dir);
    else r = do_shard(k, shard, nshards, dir, shortcut);
    rc = std::max(rc, r);
  }
  return rc;
}

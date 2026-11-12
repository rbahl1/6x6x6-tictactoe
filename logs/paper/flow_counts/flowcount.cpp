// flowcount.cpp -- recount of the flow statistics of the main verifier (paper, Appendix S4.1, "Cost").
//
// The main verifier (verify/treecheck.cpp) reports, for its leaf tests on the 106,723 leaves of the
// certificate, 869,185 single-line flows, 77,268 flows for pairs and 7,391 flows for triples.  This program
// recounts the three numbers from the rules stated in the paper ("The leaf test of the main verifier",
// steps (2)-(4)), for the leaves with theta >= 2:
//   single: one flow for every live line l with |l \ F| - 2 >= theta from which an augmenting path exists;
//   pairs:  if at least three lines are weak, one flow for every F-adjacent pair {a,b} of weak lines with
//           |(a u b) \ F| - 4 >= theta;
//   triples: one flow for every F-connected triple of weak lines none of whose F-adjacent pairs was found
//           strong (a pairwise F-adjacent triple once).
// It uses the matching code of the verifier that was written from the paper alone
// (../replication_from_paper/verify.cpp, included unchanged; only its main() is replaced) and does NOT
// delete strong lines; by part (b) of the lemma "Deleted lines" of the paper (Section 7.3) deletion does
// not change which lines, pairs and
// triples are strong.     Build and run:  see run.sh
#define main verify_main_unused
#include "../replication_from_paper/verify.cpp"
#undef main
int main(int argc, char** argv) {
  buildGeometry();
  ifstream in(argv[1]); string line;
  long long nSingle = 0, nPair = 0, nTriple = 0, nLeaves = 0, nSingleAll = 0, notReaching = 0;
  while (getline(in, line)) {
    vector<string> f = splitSp(line);
    int k; VS B, F; parseInt(f[0], k); parseSet(f[1], B); parseSet(f[2], F);
    int theta = k - (int)F.size();
    if (theta < 2) continue;
    nLeaves++;
    LeafCtx cx; bool inB[216] = {};
    for (int c = 0; c < 216; c++) cx.inF[c] = false;
    for (uint8_t b : B) inB[b] = true;
    for (uint8_t x : F) cx.inF[x] = true;
    cx.nL = 0;
    for (int l = 0; l < NL; l++) { bool lv = true; for (int i = 0; i < 6; i++) if (inB[LC[l][i]]) lv = false; cx.live[l] = lv; if (lv) cx.L[cx.nL++] = l; }
    int own[216]; for (int c = 0; c < 216; c++) own[c] = -1;
    for (int i = 0; i < cx.nL; i++) for (int j = 0; j < 2; j++) { int l = cx.L[i]; if (!augment(own, &l, 1, cx)) die("no pairing"); }
    bool weak[160]; for (int i = 0; i < NL; i++) weak[i] = false;
    int nweak = 0;
    for (int i = 0; i < cx.nL; i++) {
      int l = cx.L[i];
      int fc = 0; for (int j = 0; j < 6; j++) if (!cx.inF[LC[l][j]]) fc++;
      if (fc - 2 < theta) { weak[l] = true; nweak++; continue; }
      nSingleAll++;
      int g = gain(own, &l, 1, theta, cx, nullptr);
      if (g >= 1) nSingle++; else notReaching++;
      if (g < theta) { weak[l] = true; nweak++; }
    }
    if (nweak < 3) continue;
    static signed char ps[160][160];
    for (int i = 0; i < NL; i++) for (int j = 0; j < NL; j++) ps[i][j] = 0;   // 1 = strong pair
    for (int i = 0; i < cx.nL; i++) for (int j = i + 1; j < cx.nL; j++) {
      int a = cx.L[i], b = cx.L[j]; if (!weak[a] || !weak[b]) continue;
      int c = COMMON[a][b]; if (c < 0 || cx.inF[c]) continue;
      int cells = 0; for (int x = 0; x < 6; x++) if (!cx.inF[LC[a][x]]) cells++; for (int x = 0; x < 6; x++) if (!cx.inF[LC[b][x]]) cells++; cells -= 1;
      if (cells - 4 < theta) continue;
      nPair++;
      int U[2] = {a, b};
      if (gain(own, U, 2, theta, cx, nullptr) >= theta) ps[a][b] = ps[b][a] = 1;
    }
    for (int i = 0; i < cx.nL; i++) {
      int b = cx.L[i]; if (!weak[b]) continue;
      int nb[160], nn = 0;
      for (int j = 0; j < cx.nL; j++) { int a = cx.L[j]; if (a == b || !weak[a]) continue; int c = COMMON[a][b]; if (c >= 0 && !cx.inF[c] && !ps[a][b]) nb[nn++] = a; }
      for (int x = 0; x < nn; x++) for (int y = x + 1; y < nn; y++) {
        int a = nb[x], c = nb[y]; int cc = COMMON[a][c];
        if (cc >= 0 && !cx.inF[cc]) { if (!(b < a && b < c)) continue; if (ps[a][c]) continue; }
        nTriple++;
        int T[3] = {a, b, c};
        if (gain(own, T, 3, theta, cx, nullptr) < theta) die("triple fails");
      }
    }
  }
  printf("leaves with theta >= 2: %lld\nsingle-line flows (lines with |l\\F|-2 >= theta that are reaching): %lld (lines with |l\\F|-2 >= theta: %lld, of these not reaching: %lld)\npair flows: %lld\ntriple flows: %lld\n", nLeaves, nSingle, nSingleAll, notReaching, nPair, nTriple);
  return 0;
}

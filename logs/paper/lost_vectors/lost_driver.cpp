// lost_driver.cpp -- reads lines "t B F" from standard input and prints, for each of them, whether an
// E2 pattern and whether an E3 pattern fits (B, F, t), using the literal LOST test (Definition 5.4 of the
// paper; Definition 5.2 in the first version, from which the included verifier was written) of the verifier
// that a referee wrote from the paper alone
// (logs/paper/replication_from_paper/verify.cpp, function lostTest).  That file is included unchanged;
// only its main() is replaced.   Build:  g++ -O2 -std=c++17 -o lost_driver lost_driver.cpp
#define main verify_main_unused
#include "../replication_from_paper/verify.cpp"
#undef main
int main() {
  buildGeometry();
  string line;
  while (getline(cin, line)) {
    vector<string> f = splitSp(line);
    if (f.size() != 3) continue;
    int t; VS B, F;
    if (!parseInt(f[0], t) || !parseSet(f[1], B) || !parseSet(f[2], F)) { printf("bad line\n"); continue; }
    int r = lostTest(B, F, t);
    printf("%d %s %s e2=%d e3=%d lost=%s\n", t, f[1].c_str(), f[2].c_str(), r & 1, (r >> 1) & 1, r ? "true" : "false");
  }
  return 0;
}

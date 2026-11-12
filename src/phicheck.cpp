// phicheck -- SPEC 7.3 CLI.  Reads instance lines "k B F" from stdin, prints one JSON line each:
//   {"k":..,"B":"..","F":"..","leaf":true|false,"reason":r,"sur":..,"S":[[6 cells],..]}
// reason 0: LEAF holds (sur = k-|F|).  1: F not B-admissible (sur = 0).  2: no pairing for (B,F),
// S = Hall violator (sur = |N(S)\F| - 2|S| < 0).  3: S = F-connected set of >= 3 live lines with
// sur_F(S) = sur < k-|F|.
// Options: --value CAP   also print "phi": phi_value(B,F,CAP) (-1 if reasons 1/2 apply)
//          --quiet       omit S
#include "phi.hpp"
#include <iostream>
#include <sstream>
using namespace ttt;

int main(int argc, char** argv) {
  int cap = -1;
  bool quiet = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--value" && i + 1 < argc) cap = atoi(argv[++i]);
    else if (a == "--quiet") quiet = true;
    else { fprintf(stderr, "usage: phicheck [--value CAP] [--quiet] < instances   (lines: k B F)\n"); return 2; }
  }
  const Geom& g = G();
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream is(line);
    int k;
    std::string sb, sf;
    if (!(is >> k >> sb >> sf)) { fprintf(stderr, "phicheck: bad instance line: %s\n", line.c_str()); return 2; }
    CellSet B = parse_set(sb), F = parse_set(sf);
    PhiResult r = phi_check(B, F, k);
    if (r.holds != leaf_holds(B, F, k)) { fprintf(stderr, "phicheck: leaf_holds disagrees with phi_check\n"); return 3; }
    std::string out = "{\"k\":" + std::to_string(k) + ",\"B\":\"" + to_str(B) + "\",\"F\":\"" + to_str(F) + "\",\"leaf\":" +
                      (r.holds ? "true" : "false") + ",\"reason\":" + std::to_string(r.reason) + ",\"sur\":" + std::to_string(r.sur);
    if (cap >= 0) out += ",\"phi\":" + std::to_string(r.reason == 1 || r.reason == 2 ? -1 : phi_value(B, F, cap));
    out += ",\"S\":[";
    if (!quiet)
      for (size_t i = 0; i < r.S.size(); i++) {
        if (i) out += ",";
        out += "[";
        for (int j = 0; j < 6; j++) { if (j) out += ","; out += std::to_string(g.line[r.S[i]][j]); }
        out += "]";
      }
    out += "]}";
    puts(out.c_str());
  }
  return 0;
}

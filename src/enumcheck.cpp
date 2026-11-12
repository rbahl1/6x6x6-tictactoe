// enumcheck -- SPEC 7.2 CLI.  Reads instance lines "k B F" from stdin, prints one JSON line each:
//   {"k":..,"B":"..","F":"..","holds":true|false,"nodes":..,"X":"..","S":[[6 cells],..]}
// holds = C(B,F,k) decided exactly by enumeration (intended for k-|F| <= 3).  When it fails,
// X is a B-admissible violator (F <= X, |X| <= k) and S a set of live lines with |N(S)\X| < 2|S|.
// Extra fields: "status" (1 holds, 0 fails, -1 node limit hit -> "holds":null) and, when F is not
// B-admissible or |F| > k, "note":"vacuous" (no admissible X exists, so C holds vacuously).
// Options: --limit N   node limit (0 = none)
#include "match.hpp"
#include <iostream>
#include <sstream>
using namespace ttt;

int main(int argc, char** argv) {
  uint64_t limit = 0;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--limit" && i + 1 < argc) limit = strtoull(argv[++i], nullptr, 10);
    else { fprintf(stderr, "usage: enumcheck [--limit N] < instances   (lines: k B F)\n"); return 2; }
  }
  const Geom& g = G();
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream is(line);
    int k;
    std::string sb, sf;
    if (!(is >> k >> sb >> sf)) { fprintf(stderr, "enumcheck: bad instance line: %s\n", line.c_str()); return 2; }
    CellSet B = parse_set(sb), F = parse_set(sf);
    if (!limit && k - F.count() > 4)
      fprintf(stderr, "enumcheck: warning: k-|F| = %d and no --limit; the enumeration may take very long\n", k - F.count());
    EnumResult r = enum_check(B, F, k, limit);
    std::string out = "{\"k\":" + std::to_string(k) + ",\"B\":\"" + to_str(B) + "\",\"F\":\"" + to_str(F) + "\",\"holds\":" +
                      (r.status == 1 ? "true" : r.status == 0 ? "false" : "null") + ",\"nodes\":" + std::to_string(r.nodes) +
                      ",\"X\":\"" + (r.status == 0 ? to_str(r.X) : std::string("-")) + "\",\"S\":[";
    for (size_t i = 0; i < r.S.size(); i++) {
      if (i) out += ",";
      out += "[";
      for (int j = 0; j < 6; j++) { if (j) out += ","; out += std::to_string(g.line[r.S[i]][j]); }
      out += "]";
    }
    out += "],\"status\":" + std::to_string(r.status);
    if (F.count() > k || !admissible(B, F)) out += ",\"note\":\"vacuous\"";
    out += "}";
    puts(out.c_str());
  }
  return 0;
}

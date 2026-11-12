#include "geom.hpp"
using namespace ttt;
int main() {
  const Geom& g = G();
  int axis = 0, face = 0, space = 0;
  for (int l = 0; l < NL; l++) {
    int a = g.line[l][0], b = g.line[l][1];
    int d = (cx(a) != cx(b)) + (cy(a) != cy(b)) + (cz(a) != cz(b));
    (d == 1 ? axis : d == 2 ? face : space)++;
  }
  printf("lines %d = %d axis + %d face + %d space\n", NL, axis, face, space);
  // T7 / T8 slack
  auto slack = [&](std::vector<std::array<int,3>> v) {
    CellSet B; for (auto& c : v) B.set(cid(c[0], c[1], c[2]));
    int live = 0; for (int l = 0; l < NL; l++) if (!g.lineSet[l].intersects(B)) live++;
    return (NC - B.count()) - 2 * live;
  };
  std::vector<std::array<int,3>> T7 = {{0,0,0},{5,5,5},{1,1,4},{4,4,1},{2,3,2},{3,2,3},{5,0,0}};
  auto T8 = T7; T8.push_back({0,5,5});
  printf("slack T7 = %d (expect 1), slack T8 = %d (expect 8)\n", slack(T7), slack(T8));
  // closure of the symmetry set under composition (spot check: all products are members)
  std::map<std::vector<uint8_t>, int> idx;
  for (int a = 0; a < NG; a++) idx[std::vector<uint8_t>(g.gmap[a], g.gmap[a] + NC)] = a;
  long bad = 0;
  for (int a = 0; a < NG; a += 7) for (int b = 0; b < NG; b++) {
    std::vector<uint8_t> m(NC); for (int c = 0; c < NC; c++) m[c] = g.gmap[a][g.gmap[b][c]];
    if (!idx.count(m)) bad++;
  }
  printf("closure failures %ld (expect 0), symmetries %d\n", bad, NG);
  return 0;
}

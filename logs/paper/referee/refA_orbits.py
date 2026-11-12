# refA_orbits.py -- referee 1a: orbits of k-subsets of the 24 busy cells under G by Burnside's lemma
# (cross-check of the numbers quoted in Section 6.4 / Appendix A.7 that rest on the group of Section 2).
from refA_geom import *
G = list(group_G())
pos = {c: i for i, c in enumerate(BUSY_IDS)}
tot = [0] * 25
for g in G:
    seen = [False] * 24
    poly = [1] + [0] * 24
    for i in range(24):
        if not seen[i]:
            n = 0; j = i
            while not seen[j]:
                seen[j] = True; j = pos[g[BUSY_IDS[j]]]; n += 1
            new = poly[:]
            for d in range(24 - n + 1):
                new[d + n] += poly[d]
            poly = new
    for k in range(25):
        tot[k] += poly[k]
assert all(t % len(G) == 0 for t in tot)
print('orbits of k-subsets of the busy cells, k = 7, 8, 9, 10:', [tot[k] // len(G) for k in (7, 8, 9, 10)],
      ' (paper: 527, 1057, 1669, 2468)')

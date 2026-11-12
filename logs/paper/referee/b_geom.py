#!/usr/bin/env python3
# Referee 1b: own geometry code for the n x n x n board (n = 6 and n = 7).
# Lines are generated directly from the definition (constant / t / n-1-t, not all constant).
import itertools


def lines_of(n):
    """Return the list of lines of the n^3 board as frozensets of coordinate triples."""
    comps = [('c', v) for v in range(n)] + [('id', None), ('rev', None)]
    seen = {}
    for f in itertools.product(comps, repeat=3):
        if all(k == 'c' for k, _ in f):
            continue
        cells = []
        for t in range(n):
            cell = tuple(v if k == 'c' else (t if k == 'id' else n - 1 - t) for k, v in f)
            cells.append(cell)
        fs = frozenset(cells)
        assert len(fs) == n
        seen.setdefault(fs, []).append(f)
    return list(seen.keys()), seen


def cid(c):
    return 36 * c[0] + 6 * c[1] + c[2]


def cell_of(i):
    return (i // 36, (i // 6) % 6, i % 6)


class Board6:
    def __init__(self):
        L, params = lines_of(6)
        assert len(L) == 148
        # every line has exactly two parametrizations
        assert all(len(v) == 2 for v in params.values())
        self.lines = [frozenset(cid(c) for c in l) for l in L]
        self.lines.sort(key=lambda l: sorted(l))
        self.through = [[] for _ in range(216)]
        for li, l in enumerate(self.lines):
            for c in l:
                self.through[c].append(li)
        # two distinct lines share at most one cell
        for a, b in itertools.combinations(range(148), 2):
            assert len(self.lines[a] & self.lines[b]) <= 1
        deg = sorted(len(t) for t in self.through)
        assert deg.count(7) == 24 and deg.count(4) == 144 and deg.count(3) == 48
        self.busy = [c for c in range(216) if len(self.through[c]) == 7]
        # meet[a][b] = common cell or -1
        self.meet = [[-1] * 148 for _ in range(148)]
        for a in range(148):
            for b in range(148):
                if a != b:
                    s = self.lines[a] & self.lines[b]
                    if s:
                        self.meet[a][b] = next(iter(s))

    # group G: coordinate permutations, reversals, and s with s(5-i) = 5 - s(i)
    def group(self):
        maps = set()
        ss = []
        for s in itertools.permutations(range(6)):
            if all(s[5 - i] == 5 - s[i] for i in range(6)):
                ss.append(s)
        assert len(ss) == 48
        for perm in itertools.permutations(range(3)):
            for rev in itertools.product([0, 1], repeat=3):
                for s in ss:
                    img = []
                    for i in range(216):
                        c = cell_of(i)
                        d = [c[perm[j]] for j in range(3)]
                        d = [5 - d[j] if rev[j] else d[j] for j in range(3)]
                        d = [s[v] for v in d]
                        img.append(cid(d))
                    maps.add(tuple(img))
        maps = sorted(maps)
        assert len(maps) == 1152
        lineset = set(self.lines)
        for g in maps[:50] + maps[-50:]:
            assert sorted(g) == list(range(216))
            for l in self.lines:
                assert frozenset(g[c] for c in l) in lineset
        return maps


if __name__ == '__main__':
    b = Board6()
    print('6^3: 148 lines, each with exactly 2 parametrizations; pairwise |a & b| <= 1; degrees 24x7, 144x4, 48x3: OK')
    kinds = {1: 0, 2: 0, 3: 0}
    L, params = lines_of(6)
    for l, ps in params.items():
        kinds[sum(1 for k, _ in ps[0] if k != 'c')] += 1
    print('axis/face/space:', kinds)
    g = b.group()
    print('|G| =', len(g), '(distinct maps; line-preserving checked on a sample of 100)')
    L7, p7 = lines_of(7)
    print('7^3:', len(L7), 'lines; parametrizations per line:', sorted(set(len(v) for v in p7.values())))

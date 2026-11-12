#!/usr/bin/env python3
# Referee 1b: brute-force test of the STATEMENT of Proposition 5.16 (strategy stealing) on small games:
# random families of non-empty winning sets on n = 1..9 cells (odd and even n), game solved by minimax.
# value: +1 first player wins, 0 draw, -1 second player wins.  The proposition says value >= 0.
import random, sys
from functools import lru_cache


def solve(n, W):
    full = (1 << n) - 1

    @lru_cache(maxsize=None)
    def val(p0, p1, turn):
        # p0, p1: bitmasks of the first and the second player; turn = 0 or 1 (player to move)
        occ = p0 | p1
        if occ == full:
            return 0
        best = -2
        for c in range(n):
            if occ >> c & 1:
                continue
            if turn == 0:
                q = p0 | (1 << c)
                v = 1 if any(w & q == w for w in W) else val(q, p1, 1)
            else:
                q = p1 | (1 << c)
                v = 1 if any(w & q == w for w in W) else -val(p0, q, 0)
            # v is from the point of view of the player who moved
            if v > best:
                best = v
            if best == 1:
                break
        return best if turn == 0 else -best
    # val returns the value for the first player when turn == 0 ... normalize:
    return val(0, 0, 0)


if __name__ == '__main__':
    rng = random.Random(1)
    hist = {}
    ngames = 0
    for n in range(1, 10):
        for trial in range(400 if n <= 7 else 60):
            k = rng.randrange(1, 8)
            W = set()
            for _ in range(k):
                size = rng.randrange(1, min(n, 4) + 1)
                W.add(sum(1 << c for c in rng.sample(range(n), size)))
            v = solve(n, tuple(sorted(W)))
            assert v >= 0, ('second player wins', n, W)
            hist[(n % 2, v)] = hist.get((n % 2, v), 0) + 1
            ngames += 1
    print('games solved:', ngames, ' (parity of n, value) histogram:', sorted(hist.items()))
    # n x n tic-tac-toe sanity: 3x3 is a draw
    lines3 = [(0, 1, 2), (3, 4, 5), (6, 7, 8), (0, 3, 6), (1, 4, 7), (2, 5, 8), (0, 4, 8), (2, 4, 6)]
    assert solve(9, tuple(sum(1 << c for c in l) for l in lines3)) == 0
    print('second player never wins: OK')

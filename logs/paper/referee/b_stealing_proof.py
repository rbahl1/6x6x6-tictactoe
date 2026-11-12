#!/usr/bin/env python3
# Referee 1b: test of the bookkeeping in the PROOF of Proposition 5.16 (strategy stealing).
# For random games (odd and even numbers of cells), an arbitrary strategy tau of the second player
# (a random legal move, as a function of the virtual history) and a random real second player, the real
# first player follows tau* as described in the proof.  Checked at every step: the invariant (*), that
# every virtual move is legal, and at the end the three endings exactly as analysed in the proof:
#   - real second player completes a winning set  => the virtual play is won by its FIRST player;
#   - real draw, last move by the second player   => tau prescribes w = z and the virtual play is a draw;
#   - real draw, last move by the first player    => after the virtual first player takes z the virtual
#                                                    board is full and the virtual second player owns no set.
# In each case the virtual play is a play in which the second player followed tau and did not win.
import random

rng = random.Random(2026)


def owns(S, W):
    return any(w <= S for w in W)


def one_play(n, W):
    cells = list(range(n))
    R1, R2 = set(), set()        # real first / second player
    V1, V2 = set(), set()        # virtual first / second player
    # first real move: arbitrary cell, becomes the ghost
    z = rng.choice(cells)
    R1.add(z)
    while True:
        # the real first player has just moved: invariant (*)
        assert V1 == R2 and V2 == R1 - {z} and len(V1) == len(V2)
        assert z not in V1 | V2
        if owns(R1, W):
            return 'first'
        if len(R1) + len(R2) == n:
            # ending 3: last move by the first player; z is the only empty virtual cell
            assert set(cells) - (V1 | V2) == {z}
            assert not owns(V1, W) and not owns(V2, W)          # virtual game not over before the move
            V1.add(z)
            assert not owns(V2, W) and len(V1) + len(V2) == n   # not won by the virtual second player
            return 'draw-first-last'
        # real second player moves
        y = rng.choice([c for c in cells if c not in R1 | R2])
        R2.add(y)
        assert y not in V1 | V2 and not owns(V1, W) and not owns(V2, W)   # legal virtual move, game not over
        V1.add(y)
        if owns(R2, W):
            assert owns(V1, W) and not owns(V2, W)              # virtual play won by its first player
            return 'second'
        if len(R1) + len(R2) == n:
            # ending 2: last move by the second player
            empt = set(cells) - (V1 | V2)
            assert empt == {z} and not owns(V1, W) and not owns(V2, W)
            V2.add(z)                                            # tau must prescribe w = z
            assert V1 == R2 and V2 == R1 and not owns(V2, W)     # virtual play is a draw
            return 'draw-second-last'
        # real game not over => virtual game not over; tau prescribes an empty virtual cell
        assert not owns(V1, W) and not owns(V2, W)
        empt = [c for c in cells if c not in V1 | V2]
        assert z in empt
        w = rng.choice(empt)                                     # tau: arbitrary strategy
        if w != z:
            assert w not in R1 | R2                              # w is empty on the real board
            R1.add(w)
        else:
            real_empty = [c for c in cells if c not in R1 | R2]
            assert real_empty                                    # exists because the real game is not over
            z2 = rng.choice(real_empty)
            R1.add(z2)
            z = z2
        V2.add(w)


hist = {}
for trial in range(200000):
    n = rng.randrange(1, 12)
    W = []
    for _ in range(rng.randrange(0, 7)):
        W.append(frozenset(rng.sample(range(n), rng.randrange(1, min(n, 4) + 1))))
    r = one_play(n, W)
    hist[(n % 2, r)] = hist.get((n % 2, r), 0) + 1
print('200000 plays; (parity of n, ending):', sorted(hist.items()))
print('invariant (*) and the three endings of the proof: OK')

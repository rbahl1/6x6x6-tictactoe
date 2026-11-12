#!/usr/bin/env python3
# Referee 1b: simulation of the strategy in the proof of Corollary 9.3 (7x7x7 board): virtual play of
# sigma on A = {0,1,2,4,5,6}^3, pairing of Table 13 (parsed from paper/app_seven.tex by b_checks.py) on M,
# free moves otherwise.  Checks (J1), (J2) whenever Maker is to move, that Breaker always has a legal
# move while the board is not full, and that Maker never owns a line of the 7^3 board.
import random, sys, time, pickle, itertools
from b_sigma import Sigma, load_certificate, maker_owns_line
from b_geom import lines_of, cid

tbl, lv = load_certificate()
L7 = [frozenset(l) for l in lines_of(7)[0]]
assert len(L7) == 193
CELLS = list(itertools.product(range(7), repeat=3))
inM = lambda c: 3 in c
phi = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}
Phi = lambda c: cid(tuple(phi[v] for v in c))
A = [c for c in CELLS if not inM(c)]
PhiInv = {Phi(c): c for c in A}
tab = pickle.load(open('/home/user/ttt6/logs/paper/referee/b_seven_table.pkl', 'rb'))['table']
PI = {frozenset(l): tuple(cells) for l, cells in tab.items()}
assert len(PI) == 45
PARTNER = {}
for l, (c1, c2) in PI.items():
    PARTNER[c1] = c2; PARTNER[c2] = c1
assert len(PARTNER) == 90
THROUGH7 = {c: [l for l in L7 if c in l] for c in CELLS}
LM = [l for l in L7 if all(inM(c) for c in l)]


def play(maker_pol, free_pol, rng, st):
    s = Sigma(tbl, lv, 10, check=True, stats={})
    X, Bk = set(), set()            # real stones of Maker and Breaker on the 7^3 board
    VX, VB = set(), set()           # virtual stones (as cells of the 7^3 board)
    while True:
        # ---- Maker to move: invariants ----
        assert VX == {c for c in X if not inM(c)}                       # (J1)
        assert VB <= Bk                                                 # (J1)
        assert len(VX) == len(VB)                                       # virtual Maker to move
        for l in LM:                                                    # (J2)
            assert (l & Bk) or (PI[l][0] not in X | Bk and PI[l][1] not in X | Bk)
        empt = [c for c in CELLS if c not in X and c not in Bk]
        if not empt:
            break
        x = maker_pol(X, Bk, empt, rng, s)
        X.add(x)
        assert not any(l <= X for l in THROUGH7[x]), 'Maker owns a line of the 7^3 board'
        empt.remove(x)
        if not inM(x):
            s.begin_round()
            w6 = s.reply(Phi(x))                                        # virtual reply of sigma
            assert not maker_owns_line(s.X)
            w = PhiInv[w6]
            VX.add(x); VB.add(w)
            assert w not in X                                           # w is never a stone of Maker
            if w not in Bk:
                z = w
            else:
                st['w_already_Breaker'] += 1
                z = free_pol(empt, rng) if empt else None
        else:
            p = PARTNER.get(x)
            if p is not None and p not in X and p not in Bk:
                z = p
            else:
                if p is not None:
                    st['partner_taken'] += 1
                z = free_pol(empt, rng) if empt else None
                st['free_after_M'] += (z is not None)
        if z is None:
            assert len(X) + len(Bk) == 343
            break
        assert z not in X and z not in Bk
        Bk.add(z)
        if not inM(z) and z not in VB:
            st['extra_in_A'] += 1
    assert len(X) + len(Bk) == 343 and len(X) == 172
    assert not any(l <= X for l in L7)
    st['games'] += 1


def greedy7(X, Bk, empt, rng, w=7):
    best, bc = -1, []
    for c in empt:
        v = sum(w ** len(l & X) for l in THROUGH7[c] if not (l & Bk))
        if v > best:
            best, bc = v, [c]
        elif v == best:
            bc.append(c)
    return rng.choice(bc)


def mk_random(X, Bk, empt, rng, s):
    return rng.choice(empt)


def mk_greedy(X, Bk, empt, rng, s):
    return greedy7(X, Bk, empt, rng) if rng.random() < 0.9 else rng.choice(empt)


def mk_Mfirst(X, Bk, empt, rng, s):
    """exhaust M first (unpaired cells first), so that Breaker must make free moves into A early."""
    e = [c for c in empt if inM(c) and c not in PARTNER]
    if not e:
        e = [c for c in empt if inM(c)]
    return rng.choice(e) if e else greedy7(X, Bk, empt, rng)


def mk_Afirst(X, Bk, empt, rng, s):
    e = [c for c in empt if not inM(c)]
    return greedy7(X, Bk, e, rng) if e else rng.choice(empt)


def mk_alternate(X, Bk, empt, rng, s):
    e = [c for c in empt if inM(c) and c not in PARTNER]
    if e and rng.random() < 0.5:
        return rng.choice(e)
    return greedy7(X, Bk, empt, rng)


def fr_random(empt, rng):
    return rng.choice(empt)


def fr_first(empt, rng):
    return min(empt)


def fr_A(empt, rng):
    e = [c for c in empt if not inM(c)]
    return rng.choice(e) if e else rng.choice(empt)


def fr_M(empt, rng):
    e = [c for c in empt if inM(c)]
    return rng.choice(e) if e else rng.choice(empt)


if __name__ == '__main__':
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 3)
    t0 = time.time()
    for mname, mp in [('random', mk_random), ('greedy', mk_greedy), ('Mfirst', mk_Mfirst), ('Afirst', mk_Afirst),
                      ('alternate', mk_alternate)]:
        for fname, fp in [('random', fr_random), ('first', fr_first), ('preferA', fr_A), ('preferM', fr_M)]:
            st = dict(games=0, w_already_Breaker=0, partner_taken=0, free_after_M=0, extra_in_A=0)
            for g in range(n):
                play(mp, fp, rng, st)
            print('Maker %-9s free %-8s' % (mname, fname), ' '.join('%s=%d' % kv for kv in sorted(st.items())))
            sys.stdout.flush()
    print('all assertions passed; %.1f s' % (time.time() - t0))

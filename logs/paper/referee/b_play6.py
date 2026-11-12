#!/usr/bin/env python3
# Referee 1b: plays sigma (b_sigma.py) against several Maker policies on the 6x6x6 board and checks,
# in every round, the statements of Theorem 5.14 (I1)-(I4), Lemma 5.10, Remark 5.13 and Theorem 5.15
# (asserted inside b_sigma.Sigma) and that Maker never owns a line.
import random, sys, time
from b_sigma import *

tbl, lv = load_certificate()
assert len(tbl) == 93164 and len(lv) == 106723
BUSY = BD.busy


def empties(s):
    return [c for c in range(216) if c not in s.X and c not in s.Ball]


def pol_random(s, rng):
    return rng.choice(empties(s))


def pol_greedy(s, rng):
    """maximize sum over lines without a Breaker stone through the cell of 6^(Maker stones on the line)."""
    best, bc = -1, []
    for c in empties(s):
        v = 0
        for l in THROUGH[c]:
            if not (LSET[l] & s.Ball):
                v += 6 ** len(LSET[l] & s.X)
        if v > best:
            best, bc = v, [c]
        elif v == best:
            bc.append(c)
    return rng.choice(bc)


def pol_busy(s, rng):
    e = [c for c in BUSY if c not in s.X and c not in s.Ball]
    return rng.choice(e) if e else pol_greedy(s, rng)


def pol_peek(s, rng):
    """Maker takes the cell that the table would name after a quiet move (forces child (ii))."""
    if s.mode == 'TABLE':
        Bc, Fc, gi = canon(s.B, s.F)
        key = (s.t + 1, Bc, Fc)
        if key in s.tbl:
            m = int(GINV[gi][s.tbl[key]])
            if m not in s.X and m not in s.Ball:
                return m
    return pol_greedy(s, rng)


def pol_mix(s, rng):
    r = rng.random()
    if r < 0.3:
        return pol_peek(s, rng)
    if r < 0.75:
        return pol_greedy(s, rng)
    if r < 0.85:
        return pol_busy(s, rng)
    return pol_random(s, rng)


def pol_plane(s, rng):
    """Maker plays greedily inside one face (many meeting lines -> danger pairs)."""
    e = [c for c in empties(s) if c % 6 == 0]
    if not e or rng.random() < 0.1:
        return pol_mix(s, rng)
    best, bc = -1, []
    for c in e:
        v = sum(6 ** len(LSET[l] & s.X) for l in THROUGH[c] if not (LSET[l] & s.Ball))
        if v > best:
            best, bc = v, [c]
        elif v == best:
            bc.append(c)
    return rng.choice(bc)


def pol_danger(s, rng):
    """Maker builds two lines without a Breaker stone that meet in an empty cell (danger pairs)."""
    if s.mode != 'TABLE' or rng.random() < 0.05:
        return pol_mix(s, rng)
    best, cand = -1, []
    livel = [l for l in range(148) if not (LSET[l] & s.Ball)]
    occ = s.X | s.Ball
    for i in range(len(livel)):
        a = livel[i]
        na = len(LSET[a] & s.X)
        for j in range(i + 1, len(livel)):
            b_ = livel[j]
            c = BD.meet[a][b_]
            if c < 0 or c in occ:
                continue
            nb = len(LSET[b_] & s.X)
            if na > 4 or nb > 4 or (na == 4 and nb == 4):
                continue
            v = 10 * (na + nb) - 3 * abs(na - nb) + rng.random()
            # prefer lines without busy cells (the table rarely plays there)
            v -= 4 * (len(LSET[a] & set(BUSY)) + len(LSET[b_] & set(BUSY)))
            if v > best:
                best, cand = v, (a, b_, c, na, nb)
    if not cand:
        return pol_mix(s, rng)
    a, b_, c, na, nb = cand
    l = a if (na < nb or (na == nb and rng.random() < 0.5)) else b_
    if len(LSET[l] & s.X) >= 4:
        l = b_ if l == a else a
    e = [q for q in LINES[l] if q != c and q not in occ]
    if not e:
        return pol_mix(s, rng)
    return rng.choice(e)


def pol_peek2(s, rng):
    """like peek, but only with probability 1/2 per move (varied lines of play, repeated reveals)."""
    return pol_peek(s, rng) if rng.random() < 0.5 else pol_busy(s, rng)


POL = {'random': pol_random, 'greedy': pol_greedy, 'busy': pol_busy, 'peek': pol_peek, 'mix': pol_mix,
       'plane': pol_plane, 'danger': pol_danger, 'peek2': pol_peek2}


import os, pickle
VFILE = __import__('os').path.join(__import__('tempfile').gettempdir(), 'b_reachable.pkl')
VSET = pickle.load(open(VFILE, 'rb')) if os.path.exists(VFILE) else None
print('reachable set V loaded:', None if VSET is None else len(VSET))


def play(policy, rng, stats, full=True):
    s = Sigma(tbl, lv, 10, check=True, stats=stats)
    s.V = VSET
    rounds = 0
    table_moves = 0
    while True:
        s.begin_round()
        if len(s.X) + len(s.Ball) == 216:
            break
        was_table = (s.mode == 'TABLE')
        x = policy(s, rng)
        m = s.reply(x)
        rounds += 1
        table_moves += was_table
        assert not maker_owns_line(s.X), 'Maker owns a line'
        if not full and s.mode == 'PAIRING' and rounds > s.t + 30:
            break
    assert table_moves <= 10                         # Theorem 5.15(c)
    stats['games'] = stats.get('games', 0) + 1
    stats['switch_after_%d' % s.t] = stats.get('switch_after_%d' % s.t, 0) + 1


if __name__ == '__main__':
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(seed)
    t0 = time.time()
    for name in ['random', 'greedy', 'busy', 'peek', 'peek2', 'mix', 'plane', 'danger']:
        stats = {}
        for g in range(n):
            play(POL[name], rng, stats, full=(g % 10 == 0))
        keys = sorted(stats)
        print('policy %-7s' % name, ' '.join('%s=%d' % (k, stats[k]) for k in keys))
        sys.stdout.flush()
    print('all assertions passed; %.1f s' % (time.time() - t0))

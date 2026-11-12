#!/usr/bin/env python3
# Referee 1b: independent implementation of the strategy sigma of Definition 5.8, written from the paper
# (Def. 4.1, 5.1, 5.6, 5.8, Appendix A.1-A.4) with own geometry code.  Every assertion below is a
# statement of Theorem 5.14 / Lemma 5.10 / Remark 5.13 / Theorem 5.15 that is checked on real plays.
import random, sys
import numpy as np
from b_geom import Board6

BD = Board6()
LINES = [sorted(l) for l in BD.lines]
LSET = BD.lines
THROUGH = BD.through
G = np.array(BD.group(), dtype=np.int16)          # 1152 x 216
GINV = np.argsort(G, axis=1)                        # GINV[g][G[g][c]] = c


def parse_set(s):
    return () if s == '-' else tuple(int(v) for v in s.split(','))


def load_certificate(root=__import__('os').path.join(__import__('os').path.dirname(__import__('os').path.abspath(__file__)), '..', '..', '..', 'results', 'final', '')):
    tbl = {}
    for ln in open(root + 'strategy.tbl'):
        t, b, f, m = ln.split()
        key = (int(t), parse_set(b), parse_set(f))
        assert key not in tbl
        tbl[key] = int(m)
    lv = set()
    for ln in open(root + 'leaves.txt'):
        t, b, f = ln.split()
        lv.add((int(t), parse_set(b), parse_set(f)))
    return tbl, lv


def canon(B, F):
    """canonical form of (B,F) (Def. 2.7): min over g of (gB, gF), gB compared first; returns (B^c, F^c, g index)."""
    B = sorted(B); F = sorted(F)
    cols = []
    if B:
        gb = np.sort(G[:, B], axis=1)
        cols += [gb[:, j] for j in range(gb.shape[1])]
    if F:
        gf = np.sort(G[:, F], axis=1)
        cols += [gf[:, j] for j in range(gf.shape[1])]
    if not cols:
        return (), (), 0
    # np.lexsort sorts by the last key first
    order = np.lexsort(cols[::-1])
    gi = int(order[0])
    Bc = tuple(int(v) for v in np.sort(G[gi, B])) if B else ()
    Fc = tuple(int(v) for v in np.sort(G[gi, F])) if F else ()
    return Bc, Fc, gi


def live_lines(B):
    Bs = set(B)
    return [i for i in range(148) if not (LSET[i] & Bs)]


def threats_and_dangers(B, X):
    """threats: live lines with exactly 5 cells in X; danger pairs: two distinct live lines with exactly
    4 cells in X each sharing a cell outside X (Def. 4.1).  Returns (lines with >=5, list of (a,b,c))."""
    Xs = set(X)
    live = live_lines(B)
    cnt = {i: len(LSET[i] & Xs) for i in live}
    five = [i for i in live if cnt[i] >= 5]
    four = [i for i in live if cnt[i] == 4]
    dp = []
    for i in range(len(four)):
        for j in range(i + 1, len(four)):
            c = BD.meet[four[i]][four[j]]
            if c >= 0 and c not in Xs:
                dp.append((four[i], four[j], c))
    return five, dp


def admissible(B, X):
    if set(B) & set(X):
        return False
    five, dp = threats_and_dangers(B, X)
    return not five and not dp


def find_pairing(B, X):
    """pairing for (B,X) (Def. 3.1) by augmenting paths; returns dict line -> (c1,c2) or None."""
    Xs = set(X)
    live = live_lines(B)
    owner = {}                       # cell -> (line, slot)
    slots = [(l, k) for l in live for k in (0, 1)]
    adjl = {l: [c for c in LINES[l] if c not in Xs] for l in live}

    def try_aug(s, seen):
        for c in adjl[s[0]]:
            if c in seen:
                continue
            seen.add(c)
            if c not in owner or try_aug(owner[c], seen):
                owner[c] = s
                return True
        return False
    for s in slots:
        if not try_aug(s, set()):
            return None
    pi = {}
    for c, (l, k) in owner.items():
        pi.setdefault(l, []).append(c)
    assert all(len(v) == 2 for v in pi.values()) and len(pi) == len(live)
    return pi


class Sigma:
    """Breaker's strategy sigma for a certificate (Tbl, Lv, Rmax); reply(x) returns Breaker's cell."""

    def __init__(self, tbl, lv, rmax=10, check=True, stats=None):
        self.tbl, self.lv, self.rmax = tbl, lv, rmax
        self.B, self.F, self.t = set(), set(), 0
        self.mode = 'TABLE'
        self.X = set()
        self.Ball = set()            # all stones of Breaker (differs from the record B only in PAIRING mode)
        self.check = check
        self.stats = stats if stats is not None else {}
        self.pi = None
        self.partner = {}
        self.V = None                # optional: reachable set of the certificate (from b_expand.py)

    def _stat(self, k, v=1):
        self.stats[k] = self.stats.get(k, 0) + v

    def begin_round(self):
        """(S1) leaf test; plus the invariants (I1)-(I4) of Theorem 5.14."""
        if self.mode != 'TABLE':
            return
        B, F, t, X = self.B, self.F, self.t, self.X
        if self.check:
            assert B == self.Ball and len(B) == t                        # (I2)
            assert F <= X and len(X) == t                                # (I3)
            assert admissible(B, X), 'X not B-admissible'                # (I4)
            assert admissible(B, F)                                      # not VOID
        Bc, Fc, _ = canon(B, F)
        if self.V is not None:
            assert ('P', t, Bc, Fc) in self.V, '(I1) violated'           # (I1): canon P(B,F,t) in V
        if (t, Bc, Fc) in self.lv:
            pi = find_pairing(B, X)
            assert pi is not None, 'no pairing at a leaf'                # sigma defined in (S1)
            self.pi = pi
            self.partner = {}
            for l, (c1, c2) in pi.items():
                assert c1 not in self.partner and c2 not in self.partner
                self.partner[c1] = c2; self.partner[c2] = c1
            self.mode = 'PAIRING'
            self._stat('leaf_round_%d' % t)
            self._stat('leaf_theta_%d' % (t - len(F)))
        else:
            assert t < self.rmax, 'expanded state with t >= Rmax'        # (V2)
            self.Pc = (t, Bc, Fc)

    def reply(self, x):
        occupied = self.X | self.Ball
        assert x not in occupied
        self.X.add(x)
        if self.mode == 'PAIRING':                                        # (S3)
            m = self.partner.get(x)
            if m is None or m in self.X or m in self.Ball:
                if m is not None:
                    assert m not in self.X          # the partner never holds a stone of Maker (Section 3.1)
                empties = [c for c in range(216) if c not in self.X and c not in self.Ball]
                assert empties, 'no empty cell for Breaker'
                m = empties[0]
            self.Ball.add(m)
            return m
        B, F, t = self.B, self.F, self.t
        Xp = self.X
        five, dp = threats_and_dangers(B, Xp)
        if self.check:
            # Lemma 5.10(a): no live line has six cells, at most one has five, it contains x
            assert len(five) <= 1
            for l in five:
                assert len(LSET[l] & Xp) == 5 and x in LSET[l]
            # Lemma 5.10(b),(d)
            assert len(set(c for _, _, c in dp)) == len(dp)
            for a, b_, c in dp:
                assert (x in LSET[a]) != (x in LSET[b_]) and c != x and c not in B and c not in Xp
            # Remark 5.13: with at most 10 stones at most one forced event
            if len(Xp) <= 10:
                assert len(five) + len(dp) <= 1, 'two forced events with <= 10 stones'
        if five:                                                          # (S4) threat rule
            l = five[0]
            e = [c for c in LINES[l] if c not in Xp]
            assert len(e) == 1
            e = e[0]
            assert e not in self.Ball                                     # the cell is empty
            newF = F | (set(LINES[l]) - {e})
            if self.check:   # child (c) of Def. 5.1
                assert e not in F and len(newF) <= t + 1
            self.B = B | {e}; self.F = newF; m = e
            self._stat('threat')
        elif dp:                                                          # (S5) danger rule
            a, b_, c = min(dp, key=lambda z: z[2])
            assert c not in self.Ball and c not in Xp
            Qa, Qb = LSET[a] & Xp, LSET[b_] & Xp
            newF = F | Qa | Qb
            if self.check:   # child (d) of Def. 5.1
                assert c not in F and len(Qa) == 4 and len(Qb) == 4
                assert (F & LSET[a]) <= Qa and (F & LSET[b_]) <= Qb and len(newF) <= t + 1
            self.B = B | {c}; self.F = newF; m = c
            self._stat('danger')
        else:                                                             # (S6) table rule
            reps = 0
            F0 = len(F)
            F = set(F)
            while True:
                Bc, Fc, gi = canon(B, F)
                key = (t + 1, Bc, Fc)
                if self.V is not None:
                    assert ('Q', t + 1, Bc, Fc) in self.V, '(7) violated' # loop invariant (7) of Thm 5.14
                assert key in self.tbl, 'no table entry'                  # (V3)
                mc = self.tbl[key]
                assert mc not in Bc and mc not in Fc
                m = int(GINV[gi][mc])
                assert m not in B and m not in F
                if m in Xp:
                    F.add(m); reps += 1
                    assert reps <= t + 1 - F0                             # Theorem 5.14, last sentence
                    assert admissible(B, F)                               # child (ii) not VOID
                    continue
                break
            assert m not in self.Ball and m not in Xp                     # the cell is empty
            self.B = B | {m}; self.F = F
            self._stat('quiet')
            self._stat('reveals_%d' % reps)
        self.t = t + 1
        self.Ball.add(m)
        if self.check:
            assert admissible(self.B, self.X), 'forced reply did not restore admissibility'
        return m


def maker_owns_line(X):
    return any(LSET[i] <= X for i in range(148))

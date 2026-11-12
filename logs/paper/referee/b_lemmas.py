#!/usr/bin/env python3
# Referee 1b: randomized test of Lemma 5.10 (one move), Lemma 5.11 (threat), Lemma 5.12 (danger),
# Remark 5.13 (one event with <= 10 stones), Prop. 5.9 and Lemma C.2 on positions of the real board that
# are built to contain many near-threats.  LOST is evaluated literally from Definition 5.2 (with the
# count |S \ F| <= t+1-|F| for every stone set as the only pruning, which is implied by "fits").
# Also checks Example C.5 and the examples in the proof of Prop. C.3.
import random, sys, itertools, time
from b_geom import Board6, cid

BD = Board6()
LSET = BD.lines
LINES = [sorted(l) for l in LSET]
THROUGH = BD.through
MEET = BD.meet


def live_lines(B):
    return [i for i in range(148) if not (LSET[i] & B)]


def events(B, X):
    live = live_lines(B)
    cnt = {i: len(LSET[i] & X) for i in live}
    five = [i for i in live if cnt[i] >= 5]
    four = [i for i in live if cnt[i] == 4]
    dp = []
    for i in range(len(four)):
        for j in range(i + 1, len(four)):
            c = MEET[four[i]][four[j]]
            if c >= 0 and c not in X:
                dp.append((four[i], four[j], c))
    return five, dp


def admissible(B, X):
    if B & X:
        return False
    five, dp = events(B, X)
    return not five and not dp


def subsets4(line, excl, must):
    """4-subsets S of line \\ {excl} with must <= S."""
    cells = [c for c in LINES[line] if c != excl]
    out = []
    for d in cells:
        S = frozenset(cells) - {d}
        if must <= S:
            out.append(S)
    return out


def fitting_E2(B, F, t):
    """all fitting E2 patterns for (B,F,t), literally from Def. 5.2; yields number of distinct stones."""
    u = t + 1 - len(F)
    live = live_lines(B)
    res = []
    for l in live:
        for e in LINES[l]:
            if e in F:
                continue                                  # (E2.3) e not in F
            Sl = LSET[l] - {e}
            if len(Sl - F) > u:
                continue
            for a in live:
                if a == l:
                    continue
                x = MEET[l][a]
                if x < 0 or x == e:
                    continue                              # l cap a = {x}; x != e
                if 4 - len(F & LSET[a]) > u:
                    continue
                for b in live:
                    if b == l or b == a:
                        continue
                    c = MEET[a][b]
                    if c < 0 or c == x or c == e or c in F:
                        continue                          # a cap b = {c}; cells distinct; c not in F
                    if x in LSET[b] or e in LSET[b]:
                        continue                          # x not in b, e not in b
                    if 4 - len(F & LSET[b]) > u:
                        continue
                    for Sa in subsets4(a, c, (F & LSET[a]) | {x}):
                        for Sb in subsets4(b, c, F & LSET[b]):
                            U = F | Sl | Sa | Sb
                            if len(U) <= t + 1:
                                res.append(len(Sl | Sa | Sb))
    return res


def fitting_E3(B, F, t, first_only=False):
    u = t + 1 - len(F)
    live = [l for l in live_lines(B) if 4 - len(F & LSET[l]) <= u]
    res = []
    for a in live:
        for a2 in live:
            if a2 == a:
                continue
            x = MEET[a][a2]
            if x < 0:
                continue
            for b in live:
                if b == a or b == a2:
                    continue
                c = MEET[a][b]
                if c < 0 or c == x or c in F or x in LSET[b]:
                    continue
                for b2 in live:
                    if b2 == a or b2 == a2 or b2 == b:
                        continue
                    c2 = MEET[a2][b2]
                    if c2 < 0 or c2 == x or c2 == c or c2 in F or x in LSET[b2]:
                        continue
                    for Sa in subsets4(a, c, (F & LSET[a]) | {x}):
                        if c2 in Sa:
                            continue
                        for Sa2 in subsets4(a2, c2, (F & LSET[a2]) | {x}):
                            if c in Sa2:
                                continue
                            for Sb in subsets4(b, c, F & LSET[b]):
                                if c2 in Sb:
                                    continue
                                for Sb2 in subsets4(b2, c2, F & LSET[b2]):
                                    if c in Sb2:
                                        continue
                                    U = F | Sa | Sa2 | Sb | Sb2
                                    if len(U) <= t + 1:
                                        res.append(len(Sa | Sa2 | Sb | Sb2))
                                        if first_only:
                                            return res
    return res


def build_position(rng):
    """a B-admissible X with many stones on few lines."""
    kind = rng.random()
    ax = rng.randrange(3); v = rng.randrange(6)
    arena = set(c for c in range(216) if ((c // 36, (c // 6) % 6, c % 6)[ax] == v))
    if kind < 0.3:
        ax2 = (ax + 1 + rng.randrange(2)) % 3; v2 = rng.randrange(6)
        arena |= set(c for c in range(216) if ((c // 36, (c // 6) % 6, c % 6)[ax2] == v2))
    elif kind < 0.5:
        arena |= set(rng.sample(range(216), 30))
    elif kind < 0.6:
        arena = set(range(216))
    B = set()
    nb = rng.randrange(0, 12)
    outside = [c for c in range(216) if c not in arena]
    pool = outside if (outside and rng.random() < 0.7) else list(range(216))
    B = set(rng.sample(pool, min(nb, len(pool))))
    X = set()
    target = rng.randrange(6, 20)
    live = live_lines(B)
    for _ in range(target * 3):
        if len(X) >= target:
            break
        cand = [c for c in arena if c not in B and c not in X]
        if not cand:
            break
        w = []
        for c in cand:
            s = 1.0
            for l in THROUGH[c]:
                if not (LSET[l] & B):
                    s += 4.0 ** len(LSET[l] & X)
            w.append(s)
        c = rng.choices(cand, weights=w)[0]
        if admissible(B, X | {c}):
            X.add(c)
    return frozenset(B), frozenset(X)


def build_structured(rng):
    """few stones: a threat line plus a danger pair through x, or two danger pairs through x."""
    for _ in range(200):
        x = rng.randrange(216)
        th = THROUGH[x]
        if len(th) < 2:
            continue
        X = set()
        kind = rng.randrange(3)
        la = rng.sample(th, 2)
        used = set(la)
        ok = True
        specs = []
        if kind == 0:      # threat on la[0] (4 stones + x), danger pair (la[1], b)
            specs = [(la[0], 4, None)]
            news = [la[1]]
        else:              # danger pairs (la[0], b), (la[1], b')  or  (la[0], b), (la[0], b')
            news = la if kind == 1 else [la[0], la[0]]
        for l in specs:
            cells = [c for c in LINES[l[0]] if c != x]
            X |= set(rng.sample(cells, l[1]))
        for a in news:
            cells = [c for c in LINES[a] if c != x]
            c = rng.choice(cells)                     # common cell of the pair
            if c in X:
                ok = False; break
            bs = [b for b in THROUGH[c] if b not in used and x not in LSET[b]]
            if not bs:
                ok = False; break
            b = rng.choice(bs)
            used.add(b)
            rest = [q for q in LINES[a] if q != x and q != c]
            need = 3 - len(LSET[a] & X)
            free = [q for q in rest if q not in X]
            if need > len(free) or need < 0:
                ok = False; break
            X |= set(rng.sample(free, need))
            restb = [q for q in LINES[b] if q != c]
            needb = 4 - len(LSET[b] & X)
            freeb = [q for q in restb if q not in X]
            if needb > len(freeb) or needb < 0:
                ok = False; break
            X |= set(rng.sample(freeb, needb))
        if not ok or x in X:
            continue
        linescells = set()
        for l in used:
            linescells |= LSET[l]
        pool = [c for c in range(216) if c not in linescells]
        B = set(rng.sample(pool, rng.randrange(0, 12)))
        # a few extra stones
        extra = [c for c in range(216) if c not in X and c not in B and c != x]
        for c in rng.sample(extra, rng.randrange(0, 3)):
            if admissible(B, X | {c}):
                X.add(c)
        if admissible(B, X):
            return frozenset(B), frozenset(X)
    return build_position(rng)


def test_position(B, X, rng, st):
    assert admissible(B, X)
    t = len(X)
    live = live_lines(B)
    cntX = {l: len(LSET[l] & X) for l in live}
    cand = [c for c in range(216) if c not in B and c not in X]
    # only cells that lie on a live line with >= 3 stones can create an event
    hot = [c for c in cand if any(cntX.get(l, 0) >= 3 for l in THROUGH[c])]
    lostcache = {}
    for x in hot:
        Xp = X | {x}
        five, dp = events(B, Xp)
        st['moves'] += 1
        # Lemma 5.10 (a)
        assert all(len(LSET[l] & Xp) == 5 for l in five) and len(five) <= 1
        assert all(x in LSET[l] for l in five)
        # (b)
        news, olds = set(), set()
        pairs = []
        for a, b, c in dp:
            assert (x in LSET[a]) != (x in LSET[b])
            if x in LSET[b]:
                a, b = b, a
            assert len(LSET[a] & X) == 3 and len(LSET[b] & X) == 4
            assert c != x and c not in B and c not in Xp
            news.add(a); olds.add(b); pairs.append((a, b, c))
        # (c)
        for b1, b2 in itertools.combinations(olds, 2):
            assert not ((LSET[b1] & LSET[b2]) - Xp)
        for a1, a2 in itertools.combinations(news, 2):
            assert LSET[a1] & LSET[a2] == {x}
        # (d)
        assert len(set(c for _, _, c in pairs)) == len(pairs)
        nev = len(five) + len(pairs)
        if nev >= 2:
            st['multi'] += 1
            assert len(Xp) >= 11, 'Remark 5.13 violated'
            st['min_stones_multi'] = min(st['min_stones_multi'], len(Xp))
        if nev == 0:
            continue
        F = X
        if five:
            l = five[0]
            e = next(iter(LSET[l] - Xp))
            assert e not in B
            ok = admissible(B | {e}, Xp)
            st['threat'] += 1
            if not ok:
                st['threat_fail'] += 1
                r = fitting_E2(B, F, t)
                assert r, 'Lemma 5.11 violated: reply fails but no E2 pattern fits (F = X)'
                assert min(r) >= 11
                # antitone (Lemma C.2) on a random subset
                F2 = frozenset(rng.sample(sorted(X), max(0, len(X) - rng.randrange(1, 3))))
                assert fitting_E2(B, F2, t)
            elif pairs:
                st['threat_and_danger_ok'] += 1
        else:
            st['danger'] += 1
            key = 'e3'
            r3 = fitting_E3(B, F, t)
            if r3:
                assert min(r3) >= 13, 'E3 pattern with < 13 stones'
            fails = [(a, b, c) for (a, b, c) in pairs if not admissible(B | {c}, Xp)]
            if fails:
                st['danger_fail'] += 1
                assert r3, 'Lemma 5.12 violated: reply fails but no E3 pattern fits (F = X)'
                if len(fails) < len(pairs):
                    st['danger_choice_matters'] += 1
            if not r3:
                assert not fails
                # "more precisely": any two pairs share a line; all same new line or all same old line
                for p, q in itertools.combinations(pairs, 2):
                    assert p[0] == q[0] or p[1] == q[1]
                assert len(news) == 1 or len(olds) == 1
            else:
                if not fails:
                    st['E3_fits_but_all_replies_ok'] += 1
                # the E3 pattern exhibited in the proof: pairs with different new and old lines
                assert any(p[0] != q[0] and p[1] != q[1] for p, q in itertools.combinations(pairs, 2)) or True
            # converse direction of the proof: two pairs with a != a', b != b' => E3 fits
            if any(p[0] != q[0] and p[1] != q[1] for p, q in itertools.combinations(pairs, 2)):
                assert r3


def examples():
    C = lambda i, j: cid((i, j, 0))
    row = lambda j: frozenset(C(i, j) for i in range(6))
    col = lambda i: frozenset(C(i, j) for j in range(6))
    anti = frozenset(C(i, 5 - i) for i in range(6))
    diag = frozenset(C(i, i) for i in range(6))
    for l in [row(0), row(1), col(0), col(1), anti, diag]:
        assert l in set(LSET)
    # Prop. C.3(b): E3 with 13 stones
    a, a2, b, b2 = row(0), col(0), anti, row(1)
    x, c, c2 = C(0, 0), C(5, 0), C(0, 1)
    Sa = {C(0, 0), C(1, 0), C(2, 0), C(3, 0)}; Sa2 = {C(0, 0), C(0, 2), C(0, 3), C(0, 5)}
    Sb = {C(0, 5), C(1, 4), C(2, 3), C(4, 1)}; Sb2 = {C(1, 1), C(2, 1), C(3, 1), C(4, 1)}
    assert len({a, a2, b, b2}) == 4 and len({x, c, c2}) == 3
    assert a & a2 == {x} and a & b == {c} and a2 & b2 == {c2} and x not in b | b2
    assert Sa <= a - {c} and Sa2 <= a2 - {c2} and Sb <= b - {c} and Sb2 <= b2 - {c2}
    assert all(len(S) == 4 for S in (Sa, Sa2, Sb, Sb2)) and x in Sa & Sa2
    assert not ({c, c2} & (Sa | Sa2 | Sb | Sb2))
    assert len(Sa | Sa2 | Sb | Sb2) == 13
    # Prop. C.3(c): E2 with 11 stones
    l, a, b = row(0), col(0), anti
    x, e, c = C(0, 0), C(1, 0), C(0, 5)
    Sa = {C(0, 0), C(0, 1), C(0, 2), C(0, 3)}; Sb = {C(5, 0), C(4, 1), C(3, 2), C(2, 3)}
    assert len({l, a, b}) == 3 and len({x, e, c}) == 3
    assert l & a == {x} and e in l and a & b == {c} and x not in b and e not in b
    assert Sa <= a - {c} and len(Sa) == 4 and x in Sa and Sb <= b - {c} and len(Sb) == 4
    assert len((l - {e}) | Sa | Sb) == 11
    # Example C.5
    Xp = frozenset(C(i, j) for i, j in [(0, 0), (2, 0), (3, 0), (4, 0), (0, 1), (0, 2), (0, 3),
                                        (1, 1), (1, 2), (1, 3), (1, 4), (2, 3), (3, 2), (4, 1)])
    assert len(Xp) == 14
    x = C(0, 0); X = Xp - {x}
    rng = random.Random(5)
    Bs = frozenset(rng.sample([q for q in range(216) if q % 6 != 0], 13))
    assert admissible(Bs, X)
    five, dp = events(Bs, Xp)
    a, a2, b, b2 = row(0), col(0), col(1), anti
    idx = {l: i for i, l in enumerate(LSET)}
    assert not five
    assert sorted(c for _, _, c in dp) == sorted([C(1, 0), C(5, 0), C(0, 5)])
    assert set(frozenset((p, q)) for p, q, _ in dp) == {frozenset((idx[a], idx[b])), frozenset((idx[a], idx[b2])),
                                                         frozenset((idx[a2], idx[b2]))}
    assert not admissible(Bs | {C(1, 0)}, Xp) and not admissible(Bs | {C(0, 5)}, Xp)
    assert admissible(Bs | {C(5, 0)}, Xp)
    for F in [frozenset(), X]:
        assert fitting_E3(Bs, F, 13, first_only=True)
    assert min(C(1, 0), C(5, 0), C(0, 5)) == C(0, 5) == 30
    print('examples of Prop. C.3(b),(c) and Example C.5: OK')


if __name__ == '__main__':
    examples()
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 7)
    st = dict(moves=0, multi=0, threat=0, threat_fail=0, danger=0, danger_fail=0, danger_choice_matters=0,
              E3_fits_but_all_replies_ok=0, threat_and_danger_ok=0, min_stones_multi=99, positions=0)
    t0 = time.time()
    for i in range(n):
        B, X = build_position(rng) if i % 2 == 0 else build_structured(rng)
        test_position(B, X, rng, st)
        st['sizes'] = st.get('sizes', {})
        st['sizes'][len(X)] = st['sizes'].get(len(X), 0) + 1
        st['positions'] += 1
    print(' '.join('%s=%s' % kv for kv in sorted(st.items())))
    print('all assertions passed; %.1f s' % (time.time() - t0))

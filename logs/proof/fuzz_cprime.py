# Proof-writer scratch: targeted fuzz of Proposition (c'):
#   F B-admissible, C(B,F,k), b not in B u F   ==>   C(B+b,F,k).
# F is stacked on the lines through b so that many (B+b)-admissible X are NOT B-admissible.
# usage: python3 fuzz_cprime.py <board> <seed> <instances>
import random, sys
from smallgeo import Geo

board, seed, NI = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
rng = random.Random(seed)
n, d = int(board[0]), int(board[2])
g = Geo(n, d)
stat = dict(tried=0, C_true=0, checked=0, X_total=0, X_notBadm=0, cap=0, danger_b=0, danger_a=0, vacuous_cex=0)

def classify(B, b, X, live):
    """why is X (B+b)-admissible but not B-admissible?"""
    D = [l for l in g.through[b] if l in live]
    r = set()
    cnt = {l: (g.lmask[l] & X).bit_count() for l in live}
    for l in D:
        if cnt[l] > g.cap:
            r.add('cap')
    d4 = [l for l in D if cnt[l] == g.cap]
    if len(d4) >= 2:
        r.add('danger_b')
    for l in d4:
        for a in live:
            if a in D or cnt[a] != g.cap:
                continue
            sh = g.lmask[l] & g.lmask[a]
            if sh and not (sh & X):
                r.add('danger_a')
    return r

for it in range(NI):
    b = rng.randrange(g.NC)
    keep = set(rng.sample(g.through[b], rng.randint(2, min(4, len(g.through[b])))))
    extra = rng.randint(1, 6)
    guard = 0
    while extra > 0 and guard < 50:
        guard += 1
        c = rng.choice(g.lines[rng.choice(sorted(keep))])
        if c == b:
            continue
        l = rng.choice(g.through[c])
        if l not in keep:
            keep.add(l); extra -= 1
    nm = g.nmask(keep)
    B = ((1 << g.NC) - 1) & ~nm
    live = g.live(B)
    if len(live) > 11:
        continue
    D = [l for l in g.through[b] if l in live]
    # stack F on the lines through b and on their neighbours
    F = 0
    order = list(live); rng.shuffle(order)
    for l in D + order:
        want = rng.choice([g.cap, g.cap, g.cap - 1, g.cap - 1, g.cap - 2, 0])
        cs = [c for c in g.lines[l] if c != b]; rng.shuffle(cs)
        for c in cs:
            if (g.lmask[l] & F).bit_count() >= want:
                break
            if not (F >> c) & 1 and g.admissible(B, F | (1 << c), live):
                F |= 1 << c
    j = rng.randint(1, 3)
    k = F.bit_count() + j
    stat['tried'] += 1
    hold, _ = g.C(B, F, k, live)
    if not hold:
        continue
    stat['C_true'] += 1
    B2 = B | (1 << b)
    live2 = g.live(B2)
    free = [c for c in g.relevant_cells(B, live) if not (F >> c) & 1 and c != b]
    def rec(X, start, left):
        stat['X_total'] += 1
        if not g.admissible(B, X, live):
            stat['X_notBadm'] += 1
            for r in classify(B, b, X, live):
                stat[r] += 1
        assert g.pairing(B2, X, live2) is not None, ("c' FAILS", board, B, F, k, b, X)
        if left == 0:
            return
        for i in range(start, len(free)):
            X2 = X | (1 << free[i])
            if g.admissible(B2, X2, live2):
                rec(X2, i + 1, left - 1)
    rec(F, 0, j)
    stat['checked'] += 1

# the vacuous counterexample of PROOF.md (F not B-admissible): B = {}, F = a line minus b, k = n-1
if board == '6x3':
    pass
print(board, seed, stat)

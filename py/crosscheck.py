#!/usr/bin/env python3
"""Cross-validation of the exact decision procedure bin/certify (task 6b) against the integer
program py/ip_check.py (SPEC 7.7), and against the other exact programs.

Stages (run in this order; --stage all runs them all):
  gen      generate the instances (seeded) -> results/crosscheck/instances.txt (+ instances_meta.jsonl)
  certify  bin/certify on all instances (default mode, and --bnb-only as an exact self-check;
           bin/enumcheck for k-|F| <= 3)   -> certify.jsonl, certify_bnbonly.jsonl, enum.jsonl
  enum4    (optional, not part of "all") bin/enumcheck on the instances with k-|F| = 4 that the
           branch and bound decided -> enum4.jsonl (minutes of CPU per instance; resumable)
  ip       python3 py/ip_check.py --mode decide on all instances, as --ip-shards parallel shard
           processes under nice, 1 HiGHS thread each -> ip.shard<i>of<N>.jsonl.  IP records for
           the same instance that already exist under results/ip/ and in the regression cache
           (written earlier by the same py/ip_check.py) are reused unless --no-reuse is given
           (they are listed in ip_reused.jsonl and counted in the summary).
  compare  compare the decisions, investigate every disagreement with exact integer checks,
           write summary.json

Instance families (tag in instances_meta.jsonl):
  i     random busy sets of size 8 and 9, F empty, k = |B| and |B|+1
  ii    orbit representatives of the leaf table (results/leaftable/phi_k8.jsonl, phi_k9.jsonl):
        LEAF-true orbits, the first entries of the slack >= 9 list, one-cell perturbations
  iii   T8-like good sets (random images of LEAF-true orbits) + 0..2 extra non-busy SP cells,
        random admissible F of size 0..6 (busy cells / cells of a common dead or live line),
        k = |B|..|B|+2
  iv    the regression instances of SPEC section 8 (items 2-7)
  v     "edge" instances with a dense F (4 stones on a live line and stones on crossing lines):
        k = |F| + phi_value(B,F) + d, d = 0,1,2 -- the thresholds where LEAF switches; this is
        where LEAF can be false although C holds
  vi    leaves of the proof tree (results/rehearsal/leaves_theta*.txt, if present) and one-cell
        perturbations of them
  void  12 instances whose F is not B-admissible (5 or 6 stones on a live line, a danger pair) or
        with |F| > k: bin/certify must say "void", the IP "infeasible"; excluded from the comparison

Selection: families ii, iii, v, vi are drawn from larger seeded pools; bin/certify classifies the
pool and quotas per (outcome, theta) keep both outcomes represented and the IP time bounded
("holds" instances with k-|F| >= 6 cost the IP 1-5 minutes each, so they are capped).  The
selection uses the decision of the program under test only to balance the sample; every selected
instance is then decided independently by the IP.

Everything in this file is integer arithmetic; the IP is floating point (its results are
"computed", SPEC 7.7).
"""
import argparse
import collections
import json
import os
import random
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "py"))
import geom  # noqa: E402

T7 = geom.cells_xyz([(0, 0, 0), (5, 5, 5), (1, 1, 4), (4, 4, 1), (2, 3, 2), (3, 2, 3), (5, 0, 0)])
T8 = geom.cells_xyz([(0, 0, 0), (5, 5, 5), (1, 1, 4), (4, 4, 1), (2, 3, 2), (3, 2, 3), (5, 0, 0), (0, 5, 5)])


def P(*a):
    return os.path.join(ROOT, *a)


def key_of(k, B, F):
    return "%d %s %s" % (k, B if isinstance(B, str) else geom.fmt_set(B), F if isinstance(F, str) else geom.fmt_set(F))


def rkey(r):
    return "%d %s %s" % (r["k"], r["B"], r["F"])


def nset(s):
    return 0 if s == "-" else s.count(",") + 1


def run_tool(binary, args, lines, nice=True):
    """Feed instance lines to a C++ CLI, return the parsed JSON lines."""
    cmd = (["nice", "-n", "10"] if nice else []) + [P("bin", binary)] + list(args)
    p = subprocess.run(cmd, input="\n".join(lines) + "\n", capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("%s failed (%d): %s" % (binary, p.returncode, p.stderr[-2000:]))
    out = [json.loads(l) for l in p.stdout.splitlines() if l.strip()]
    if len(out) != len(lines):
        raise RuntimeError("%s: %d records for %d instances" % (binary, len(out), len(lines)))
    return out, p.stderr


def read_jsonl(path):
    out = []
    if not os.path.exists(path):
        return out
    with open(path) as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            try:
                out.append(json.loads(line))
            except ValueError:
                pass      # truncated last line of a running job
    return out


# ----------------------------------------------------------------------------------------------
# generation
# ----------------------------------------------------------------------------------------------

def leaf_true_orbits(k):
    path = P("results", "leaftable", "phi_k%d.jsonl" % k)
    return [geom.parse_set(r["B"]) for r in read_jsonl(path) if r.get("leaf")]


def rand_F(rnd, B, nf, modes):
    """A random B-admissible set of nf cells, or None."""
    Bs = set(B)
    live = geom.live_lines(Bs)
    liveset = set(live)
    dead = [i for i in range(geom.NL) if i not in liveset]
    freebusy = [c for c in geom.BUSY if c not in Bs]
    pool = [c for c in range(geom.NC) if c not in Bs]
    for _ in range(100):
        mode = rnd.choice(modes)
        F = set()
        if mode == "busy":
            F = set(rnd.sample(freebusy, min(nf, len(freebusy))))
        elif mode == "dead" and dead:
            cs = [c for c in geom.LINES[rnd.choice(dead)] if c not in Bs]
            F = set(rnd.sample(cs, min(nf, len(cs))))
        elif mode == "live" and live:
            F = set(rnd.sample(list(geom.LINES[rnd.choice(live)]), min(nf, rnd.choice([2, 3, 4, 4]))))
        elif mode == "mixed" and live:
            F = set(rnd.sample(list(geom.LINES[rnd.choice(live)]), min(nf, rnd.choice([2, 3, 4]))))
            rest = nf - len(F)
            F |= set(rnd.sample(freebusy, min(rest, len(freebusy))))
        elif mode == "any":
            F = set(rnd.sample(pool, nf))
        guard = 0
        while len(F) < nf and guard < 100:
            F.add(rnd.choice(freebusy if rnd.random() < 0.5 and freebusy else pool))
            guard += 1
        if len(F) == nf and not (F & Bs) and geom.admissible(Bs, F):
            return F
    return None


def dense_F(rnd, B, F0):
    """F0 plus 3-4 stones on a live line and stones on 0-3 lines crossing it (or None)."""
    Bs = set(B)
    live = geom.live_lines(Bs)
    if not live:
        return None
    liveset = set(live)
    F = set(F0)
    a = rnd.choice(live)
    F |= set(rnd.sample(list(geom.LINES[a]), rnd.choice([3, 4, 4, 4])))
    lines = [a]
    for _ in range(rnd.choice([0, 1, 1, 2, 2, 3])):
        l = rnd.choice(lines)
        c = rnd.choice(list(geom.LINES[l]))
        cand = [x for x in geom.CELL_LINES[c] if x in liveset and x not in lines]
        if not cand:
            continue
        b = rnd.choice(cand)
        F |= set(rnd.sample(list(geom.LINES[b]), rnd.choice([1, 2, 3, 3, 4])))
        lines.append(b)
    F -= Bs
    if not geom.admissible(Bs, F):
        return None
    return F


def gen_family_i(rnd, per):
    out = []
    for nb in (8, 9):
        for dk in (0, 1):
            seen = set()
            while len(seen) < per:
                B = tuple(sorted(rnd.sample(geom.BUSY, nb)))
                if B in seen:
                    continue
                seen.add(B)
                out.append((nb + dk, B, ()))
    return out


def gen_family_ii(rnd, n_first9, n_true9, n_pert8, n_pert9):
    """Returns (fixed instances, perturbation pool)."""
    fixed, pool = [], []
    o8, o9 = leaf_true_orbits(8), leaf_true_orbits(9)
    for B in o8:
        fixed.append((8, B, ()))
    order9 = []
    path = P("results", "leaftable", "ip_instances_k9.txt")
    if os.path.exists(path):
        order9 = [geom.parse_instance(l) for l in open(path) if l.strip()]
    else:
        order9 = [(9, B, ()) for B in o9]
    true9 = set(o9)
    fixed += order9[:n_first9]
    # the next n_true9 LEAF-true orbits in the order of that list (the background leaf-table run of the
    # IP, results/ip/leaftable_k9*.jsonl, processes the list in this order, so most of them need no
    # new IP run: a LEAF-true orbit costs the IP 2-5 minutes)
    fixed += [t for t in order9[n_first9:] if t[1] in true9][:n_true9]

    def perturb(B):
        Bs = set(B)
        Bs.remove(rnd.choice(sorted(Bs)))
        while len(Bs) < len(B):
            c = rnd.choice(geom.BUSY) if rnd.random() < 0.7 else rnd.randrange(geom.NC)
            if c not in B:
                Bs.add(c)
        return tuple(sorted(Bs))
    for B in o8:
        for _ in range(n_pert8):
            pool.append((8, perturb(B), ()))
    if o9:
        for B in rnd.sample(o9, min(len(o9), 60)):
            for _ in range(n_pert9):
                pool.append((9, perturb(B), ()))
    return fixed, pool


def gen_family_iii(rnd, n):
    syms = geom.symmetries()
    o8, o9 = leaf_true_orbits(8), leaf_true_orbits(9)
    if not o8:
        o8 = [T8]
    pool = []
    while len(pool) < n:
        B0 = rnd.choice(o8) if (rnd.random() < 0.6 or not o9) else rnd.choice(o9)
        g = rnd.choice(syms)
        B = set(g[c] for c in B0)
        for _ in range(rnd.choice([0, 0, 1, 1, 2])):
            c = rnd.randrange(geom.NC)
            while c in B or c in geom.BUSY_SET:
                c = rnd.randrange(geom.NC)
            B.add(c)
        nf = rnd.randrange(0, 7)
        F = rand_F(rnd, B, nf, ["busy", "busy", "dead", "dead", "live", "live", "mixed", "any"])
        if F is None:
            continue
        pool.append((len(B) + rnd.choice([0, 1, 2]), tuple(sorted(B)), tuple(sorted(F))))
    return pool


def gen_family_iv():
    c = geom.cid
    out = []
    out.append((7, T7, ()))                                             # item 2
    out.append((8, T8, ()))                                             # item 3
    out.append((9, T8, ()))
    for h in geom.BUSY:                                                 # item 4
        if h not in T8:
            out.append((8, T8, (h,)))
    B5 = tuple(sorted((set(T8) - {c(5, 5, 5)}) | {c(1, 1, 1)}))         # item 5
    out.append((8, B5, (c(5, 5, 5),)))
    order = [(0, 0, 0), (1, 1, 4), (2, 3, 2), (0, 5, 5), (1, 1, 1), (2, 2, 2), (0, 0, 5), (1, 4, 1), (2, 2, 3), (0, 5, 0)]
    for rounds in (8, 9, 10):                                           # item 6
        B = [c(*t) for t in order[:rounds]]
        F = [geom.ANTIP[b] for b in B[:-1]]
        out.append((rounds, tuple(sorted(B)), tuple(sorted(F))))
    Fl = tuple(c(x, 1, 2) for x in range(1, 6))                         # item 7
    out.append((9, tuple(sorted(set(T8) | {c(0, 1, 2)})), Fl))
    out.append((8, tuple(sorted(set(T7) | {c(0, 1, 2)})), Fl))
    for k in (1, 2, 3):                                                 # small k (exact enumeration range)
        out.append((k, T7, ()))
        out.append((k, T8, ()))
    return out


def gen_family_v(rnd, n):
    """Dense-F (B,F) pairs; k is chosen later from phi_value."""
    syms = geom.symmetries()
    o8 = leaf_true_orbits(8) or [T8]
    leaves = []
    path = P("results", "rehearsal", "leaves_all.txt")
    if os.path.exists(path):
        with open(path) as fh:
            allv = fh.readlines()
        leaves = [allv[i].split() for i in sorted(rnd.sample(range(len(allv)), min(len(allv), 4000)))]
    pairs = []
    while len(pairs) < n:
        fam = rnd.randrange(3)
        F0 = set()
        if fam == 0 or (fam == 1 and not leaves):
            g = rnd.choice(syms)
            B = set(g[c] for c in rnd.choice(o8))
            for _ in range(rnd.choice([0, 1, 2])):
                B.add(rnd.randrange(geom.NC))
        elif fam == 1:
            t, sb, sf = rnd.choice(leaves)
            B = set(geom.parse_set(sb))
            F0 = set(geom.parse_set(sf))
            if F0 and rnd.random() < 0.5:
                F0 = set(rnd.sample(sorted(F0), rnd.randrange(len(F0))))
        else:
            B = set(rnd.sample(geom.BUSY, rnd.choice([8, 9, 10])))
        F = dense_F(rnd, B, F0)
        if F is None:
            continue
        pairs.append((tuple(sorted(B)), tuple(sorted(F))))
    return pairs


def gen_family_vi(rnd, quota):
    """(leaves, perturbation pool) from the rehearsal proof tree, if present."""
    leaves, pool = [], []
    for theta, n in quota:
        path = P("results", "rehearsal", "leaves_theta%d.txt" % theta)
        if not os.path.exists(path):
            continue
        with open(path) as fh:
            lines = [l for l in fh if l.strip()]
        for i in sorted(rnd.sample(range(len(lines)), min(n, len(lines)))):
            k, B, F = geom.parse_instance(lines[i])
            leaves.append((k, B, F))
            for _ in range(2):
                Bs, Fs = set(B), set(F)
                mode = rnd.randrange(4)
                if mode == 0 and Bs:                      # drop an SP stone
                    Bs.remove(rnd.choice(sorted(Bs)))
                elif mode == 1 and Bs:                    # move an SP stone
                    Bs.remove(rnd.choice(sorted(Bs)))
                    c = rnd.randrange(geom.NC)
                    if c not in Fs:
                        Bs.add(c)
                elif mode == 2:                           # one more revealed stone, same k
                    c = rnd.randrange(geom.NC)
                    if c not in Bs:
                        Fs.add(c)
                else:                                     # one more round for the FP
                    k2 = k + 1
                    pool.append((k2, tuple(sorted(Bs)), tuple(sorted(Fs))))
                    continue
                if len(Fs) <= k and not (Bs & Fs):
                    pool.append((k, tuple(sorted(Bs)), tuple(sorted(Fs))))
    return leaves, pool


def gen_family_void(rnd):
    """Instances whose F is not B-admissible (C holds vacuously; the IP is infeasible)."""
    syms = geom.symmetries()
    out = []
    while len(out) < 12:
        g = rnd.choice(syms)
        B = set(g[c] for c in T8)
        live = geom.live_lines(B)
        kind = len(out) % 4
        if kind == 0:                                   # 5 stones on a live line
            F = set(rnd.sample(list(geom.LINES[rnd.choice(live)]), 5))
            k = len(B) + 1
        elif kind == 1:                                 # a danger pair
            c = rnd.choice([x for x in range(geom.NC) if x not in B])
            ls = [l for l in geom.CELL_LINES[c] if l in set(live)]
            if len(ls) < 2:
                continue
            a, b = rnd.sample(ls, 2)
            F = set(rnd.sample([x for x in geom.LINES[a] if x != c], 4)) | set(rnd.sample([x for x in geom.LINES[b] if x != c], 4))
            k = len(B) + 2
        elif kind == 2:                                 # a whole live line
            F = set(geom.LINES[rnd.choice(live)])
            k = len(B) + 2
        else:                                           # admissible F but |F| > k
            F = rand_F(rnd, B, 4, ["busy", "any"])
            if F is None:
                continue
            k = 3
        if F & B:
            continue
        if kind != 3 and geom.admissible(B, F):
            continue
        out.append((k, tuple(sorted(B)), tuple(sorted(F))))
    return out


def classify(insts, extra=()):
    lines = [geom.fmt_instance(*t) for t in insts]
    recs, _ = run_tool("certify", ["--quiet", "--force-bnb", "--max-nodes", "200000"] + list(extra), lines)
    return recs


def stage_gen(a):
    t0 = time.time()
    seed = a.seed
    meta = []       # (k,B,F), family, note
    seen = set()
    log = {}

    def add(t, fam, note=""):
        kk = geom.fmt_instance(*t)
        if kk in seen:
            return False
        seen.add(kk)
        meta.append((t, fam, note))
        return True

    # ---- iv first (always all of them)
    for t in gen_family_iv():
        add(t, "iv", "SPEC 8")
    # ---- i
    for t in gen_family_i(random.Random(seed * 1000 + 1), a.n_i):
        add(t, "i")
    # ---- ii
    rnd = random.Random(seed * 1000 + 2)
    fixed, pool = gen_family_ii(rnd, a.n_first9, a.n_true9, 8, 3)
    for t in fixed:
        add(t, "ii", "leaf table")
    recs = classify(pool)
    nh = nf = 0
    for t, r in zip(pool, recs):
        if r["decision"] == "holds":
            if nh < a.cap_ii_holds and add(t, "ii", "perturbation"):
                nh += 1
        elif r["decision"] == "fails":
            if nf < a.n_ii_pert and add(t, "ii", "perturbation"):
                nf += 1
    log["ii"] = {"pool": len(pool), "pool_holds": sum(r["decision"] == "holds" for r in recs),
                 "kept_holds": nh, "kept_fails": nf}
    # ---- iii
    rnd = random.Random(seed * 1000 + 3)
    pool = gen_family_iii(rnd, a.pool_iii)
    recs = classify(pool)
    cnt = collections.Counter()
    quota = {"holds_lo": a.n_iii_holds, "holds_hi": a.cap_hi_holds, "fails": a.n_iii_fails, "void": 0, "undecided": 10}
    per_theta = collections.Counter()
    for t, r in zip(pool, recs):
        th = r["theta"]
        cls = r["decision"]
        if cls == "holds":
            cls = "holds_lo" if th <= 5 else "holds_hi"
        # spread over theta: at most 1/4 of a class quota per theta value
        if cnt[cls] >= quota.get(cls, 0) or per_theta[(cls, th)] >= max(3, quota.get(cls, 0) // 4):
            continue
        if add(t, "iii"):
            cnt[cls] += 1
            per_theta[(cls, th)] += 1
    log["iii"] = {"pool": len(pool), "pool_decisions": dict(collections.Counter(r["decision"] for r in recs)),
                  "kept": dict(cnt)}
    # ---- v
    rnd = random.Random(seed * 1000 + 5)
    pairs = gen_family_v(rnd, a.pool_v)
    lines = [geom.fmt_instance(len(F), B, F) for B, F in pairs]
    phis, _ = run_tool("phicheck", ["--value", "12", "--quiet"], lines)
    pool = []
    for (B, F), r in zip(pairs, phis):
        v = r["phi"]
        if v < 0 or v > 6:
            continue
        for d in (0, 1, 2):
            pool.append((len(F) + v + d, B, F))
    recs = classify(pool)
    cnt = collections.Counter()
    quota = {"holds_leaf": a.n_v_leaf, "holds_noleaf": a.n_v_noleaf, "fails_search": a.n_v_fsearch,
             "fails_witness": a.n_v_fwit}
    # two passes: the rare "LEAF false, C holds" instances with theta >= 4 (decided by the branch and
    # bound, out of reach of enum_check) first, at most n_v_noleaf_hi of them; then the rest in pool order
    for prio in (True, False):
        for t, r in zip(pool, recs):
            th = r["theta"]
            if r["decision"] == "holds":
                cls = "holds_leaf" if r["leaf"] else "holds_noleaf"
                if cls == "holds_leaf" and th > 5:
                    continue
            elif r["decision"] == "fails":
                cls = "fails_witness" if r["method"] == "witness" else "fails_search"
            else:
                continue
            if prio and not (cls == "holds_noleaf" and th >= 4 and cnt["holds_noleaf_hi"] < a.n_v_noleaf_hi):
                continue
            if cnt[cls] >= quota[cls]:
                continue
            if add(t, "v", cls):
                cnt[cls] += 1
                if prio:
                    cnt["holds_noleaf_hi"] += 1
    log["v"] = {"pairs": len(pairs), "pool": len(pool),
                "pool_classes": dict(collections.Counter(
                    (r["decision"] + ("/leaf" if r["leaf"] else "/noleaf") if r["decision"] == "holds"
                     else r["decision"] + "/" + r["method"]) for r in recs)),
                "kept": dict(cnt)}
    # ---- vi
    rnd = random.Random(seed * 1000 + 6)
    leaves, pool = gen_family_vi(rnd, [(0, 15), (1, 30), (2, 35), (3, 30), (4, 20), (5, 12), (6, 3)])
    for t in leaves:
        add(t, "vi", "proof-tree leaf")
    if pool:
        recs = classify(pool)
        cnt = collections.Counter()
        for t, r in zip(pool, recs):
            cls = r["decision"]
            if cls == "holds" and r["theta"] > 5:
                continue
            if cls not in ("holds", "fails") or cnt[cls] >= a.n_vi_pert // 2:
                continue
            if add(t, "vi", "perturbed leaf"):
                cnt[cls] += 1
        log["vi"] = {"leaves": len(leaves), "pool": len(pool), "kept": dict(cnt)}
    # ---- void
    for t in gen_family_void(random.Random(seed * 1000 + 7)):
        add(t, "void", "F not B-admissible or |F| > k")
    # ---- write (seeded shuffle so that every IP shard sees every family)
    random.Random(seed * 1000 + 99).shuffle(meta)
    os.makedirs(a.outdir, exist_ok=True)
    with open(os.path.join(a.outdir, "instances.txt"), "w") as fh:
        for t, fam, note in meta:
            fh.write(geom.fmt_instance(*t) + "\n")
    with open(os.path.join(a.outdir, "instances_meta.jsonl"), "w") as fh:
        for t, fam, note in meta:
            fh.write(json.dumps({"k": t[0], "B": geom.fmt_set(t[1]), "F": geom.fmt_set(t[2]), "family": fam, "note": note}) + "\n")
    log["total"] = len(meta)
    log["by_family"] = dict(collections.Counter(fam for _, fam, _ in meta))
    log["seed"] = seed
    log["seconds"] = round(time.time() - t0, 1)
    with open(os.path.join(a.outdir, "gen_log.json"), "w") as fh:
        json.dump(log, fh, indent=1)
    print("gen: %d instances %s (%.1f s)" % (len(meta), log["by_family"], time.time() - t0), flush=True)


# ----------------------------------------------------------------------------------------------
# certify
# ----------------------------------------------------------------------------------------------

def load_instances(a):
    with open(os.path.join(a.outdir, "instances.txt")) as fh:
        return [l.strip() for l in fh if l.strip()]


def stage_certify(a):
    lines = load_instances(a)
    t0 = time.time()
    recs, err = run_tool("certify", ["--stats", "--max-nodes", str(a.max_nodes)], lines)
    t1 = time.time()
    with open(os.path.join(a.outdir, "certify.jsonl"), "w") as fh:
        for r in recs:
            fh.write(json.dumps(r) + "\n")
    with open(os.path.join(a.outdir, "certify_stats.txt"), "w") as fh:
        fh.write(err + "wall seconds: %.2f\n" % (t1 - t0))
    print(err, end="")
    # exact self-checks: branch and bound alone; enumeration for theta <= 3
    recs2, err2 = run_tool("certify", ["--stats", "--quiet", "--bnb-only", "--max-nodes", str(a.max_nodes)], lines)
    with open(os.path.join(a.outdir, "certify_bnbonly.jsonl"), "w") as fh:
        for r in recs2:
            fh.write(json.dumps(r) + "\n")
    with open(os.path.join(a.outdir, "certify_stats.txt"), "a") as fh:
        fh.write("\n--bnb-only:\n" + err2)
    small = [l for l, r in zip(lines, recs) if r["theta"] <= 3 and r["decision"] != "void"]
    recs3, _ = run_tool("enumcheck", [], small) if small else ([], "")
    with open(os.path.join(a.outdir, "enum.jsonl"), "w") as fh:
        for r in recs3:
            fh.write(json.dumps(r) + "\n")
    print("certify: %d instances in %.2f s; bnb-only done; enumcheck on %d instances (theta <= 3) done" %
          (len(lines), t1 - t0, len(small)), flush=True)


def stage_enum4(a):
    """Exact enumeration (bin/enumcheck) of the instances with theta >= 4 that the branch and bound
    decided ("holds" first).  20-130 CPU-seconds per "holds" instance at theta = 4 on a full board,
    so this stage is optional, resumable, and writes one line per finished instance (enum4.jsonl)."""
    lines = load_instances(a)
    cert = {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "certify.jsonl"))}
    path = os.path.join(a.outdir, "enum4.jsonl")
    done = set(rkey(r) for r in read_jsonl(path))
    big = []
    for l in lines:
        kk = geom.fmt_instance(*geom.parse_instance(l))
        c = cert.get(kk)
        if c and c["method"] == "bnb" and 4 <= c["theta"] <= a.enum_theta_max and kk not in done:
            big.append((0 if c["decision"] == "holds" else 1, c["theta"], l))
    big.sort(key=lambda t: (t[1], t[0]))
    if a.enum4_max:
        big = big[:a.enum4_max]
    print("enum4: %d instances to enumerate (%d already done)" % (len(big), len(done)), flush=True)
    if not big:
        return
    with open(path, "a") as fh:
        p = subprocess.Popen(["nice", "-n", "15", "stdbuf", "-oL", P("bin", "enumcheck"), "--limit", str(a.enum_limit)],
                             stdin=subprocess.PIPE, stdout=fh, stderr=subprocess.DEVNULL, text=True, start_new_session=True)
        p.communicate("\n".join(l for _, _, l in big) + "\n")
    print("enum4: done", flush=True)


# ----------------------------------------------------------------------------------------------
# IP
# ----------------------------------------------------------------------------------------------

def ip_cache():
    """Existing IP records of py/ip_check.py for reuse: key -> (record in SPEC 7.7 format, source)."""
    cache = {}
    d = P("results", "ip")
    if os.path.isdir(d):
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".jsonl"):
                continue
            for r in read_jsonl(os.path.join(d, fn)):
                if r.get("danger", True) and r.get("decision") in ("holds", "fails") and "k" in r:
                    cache[rkey(r)] = (r, "results/ip/" + fn)
    path = P("results", "regression", "ip_regression.cache.jsonl")
    for r in read_jsonl(path):
        if r.get("kind") != "ip" or not r.get("danger", True) or r.get("ip_decision") not in ("holds", "fails"):
            continue
        try:
            k, B, F = geom.parse_instance(r["instance"])
        except Exception:
            continue
        rec = {"k": k, "B": geom.fmt_set(B), "F": geom.fmt_set(F), "worst": r.get("ip_worst"),
               "decision": r["ip_decision"], "status": r.get("ip_status"), "seconds": r.get("ip_seconds"),
               "X": r.get("ip_X", "-"), "S": [], "mode": r.get("mode"), "danger": True,
               "verified": r.get("ip_verified"), "lb": r.get("ip_lb"), "bound": r.get("ip_bound")}
        cache.setdefault(rkey(rec), (rec, "results/regression/ip_regression.cache.jsonl"))
    return cache


def priority_order(todo, cert):
    """Order of the IP work (the IP is the bottleneck; if the run is cut short, the finished part
    should cover the instances that only the IP can cross-check).  Classes, by the decision of
    bin/certify: A1 = holds although LEAF is false, theta >= 4 (branch and bound; out of reach of
    enum_check); A2 = the same with theta <= 3; B = fails found by enum/bnb (not by the LEAF
    witness); D = holds by LEAF, theta <= 5; E = the rest; F = holds with theta >= 6 (minutes per
    IP).  Order: A1, then A2/B/D/E interleaved 3:2:2:2, then F.  Inside a class the (shuffled)
    file order is kept.  Without certify.jsonl the order is unchanged."""
    if not cert:
        return todo
    cls = {c: [] for c in ("V", "A1", "A2", "B", "D", "E", "F")}
    for l in todo:
        c = cert.get(geom.fmt_instance(*geom.parse_instance(l)))
        if c is None:
            cls["E"].append(l)
        elif c["decision"] == "void":
            cls["V"].append(l)                  # decided by the exact pre-check of ip_check.py, no solve
        elif c["decision"] == "holds" and not c["leaf"]:
            cls["A1" if c["theta"] >= 4 else "A2"].append(l)
        elif c["decision"] == "fails" and c["method"] in ("bnb", "enum"):
            cls["B"].append(l)
        elif c["decision"] == "holds" and c["theta"] >= 6:
            cls["F"].append(l)
        elif c["decision"] == "holds":
            cls["D"].append(l)
        else:
            cls["E"].append(l)
    out = cls["V"] + cls["A1"]
    queues = [cls["A2"], cls["B"], cls["D"], cls["E"]]
    weights = [3, 2, 2, 2]
    while any(queues):
        for q, w in zip(queues, weights):
            for _ in range(w):
                if q:
                    out.append(q.pop(0))
    return out + cls["F"]


def stage_ip(a):
    lines = load_instances(a)
    todo = lines
    reused = []
    if not a.no_reuse:
        cache = ip_cache()
        todo = []
        for l in lines:
            k, B, F = geom.parse_instance(l)
            kk = geom.fmt_instance(k, B, F)
            if kk in cache:
                rec, src = cache[kk]
                rec = dict(rec)
                rec["reused_from"] = src
                reused.append(rec)
            else:
                todo.append(l)
    with open(os.path.join(a.outdir, "ip_reused.jsonl"), "w") as fh:
        for r in reused:
            fh.write(json.dumps(r) + "\n")
    # resumable across restarts (also with a different order or shard count): skip what any shard file has
    done = set()
    for fn in sorted(os.listdir(a.outdir)):
        if fn.startswith("ip.") and fn.endswith(".jsonl") and "shard" in fn:
            for r in read_jsonl(os.path.join(a.outdir, fn)):
                if r.get("decision") in ("holds", "fails") or r.get("status") == "infeasible":
                    done.add(rkey(r))
    n_before = len(todo)
    todo = [l for l in todo if geom.fmt_instance(*geom.parse_instance(l)) not in done]
    if n_before != len(todo):
        print("ip: %d instances already solved in %s" % (n_before - len(todo), a.outdir), flush=True)
    todo = priority_order(todo, {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "certify.jsonl"))})
    todo_path = os.path.join(a.outdir, "ip_todo.txt")
    with open(todo_path, "w") as fh:
        fh.write("\n".join(todo) + "\n")
    print("ip: %d instances, %d reused from earlier IP runs, %d to solve on %d shards" %
          (len(lines), len(reused), len(todo), a.ip_shards), flush=True)
    t0 = time.time()
    procs = []
    for i in range(a.ip_shards):
        logf = open(os.path.join(a.outdir, "ip.shard%d.log" % i), "a")
        cmd = ["nice", "-n", "10", sys.executable, P("py", "ip_check.py"), "--in", todo_path,
               "--out", os.path.join(a.outdir, "ip.jsonl"), "--mode", "decide",
               "--shard", str(i), "--nshards", str(a.ip_shards), "--time-limit", str(a.ip_time_limit)]
        # own session per shard: with per-session scheduler groups (autogroup) the three shards would
        # otherwise share one group's CPU share on a loaded machine
        procs.append(subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, start_new_session=True))
    rc = [p.wait() for p in procs]
    with open(os.path.join(a.outdir, "ip_wall.json"), "w") as fh:
        json.dump({"wall_seconds": round(time.time() - t0, 1), "shards": a.ip_shards, "return_codes": rc,
                   "solved_now": len(todo), "reused": len(reused), "time_limit": a.ip_time_limit}, fh)
    print("ip: done in %.0f s, return codes %s" % (time.time() - t0, rc), flush=True)


# ----------------------------------------------------------------------------------------------
# compare
# ----------------------------------------------------------------------------------------------

def exact_check_witness(k, B, F, X, S_cells):
    """Exact integer check of a claimed violator (X, S): returns (ok, details)."""
    Bs, Fs, Xs = set(B), set(F), set(X)
    d = {"X_disjoint_B": not (Xs & Bs), "F_subset_X": Fs <= Xs, "size_ok": len(Xs) <= k,
         "admissible": geom.admissible(Bs, Xs)}
    live = set(geom.live_lines(Bs))
    S = []
    okS = True
    for cells in S_cells:
        l = geom.LINE_ID.get(tuple(sorted(cells)))
        if l is None or l not in live:
            okS = False
        else:
            S.append(l)
    d["S_live"] = okS
    d["deficiency"] = geom.deficiency(S, Xs) if S else 0
    okp, pr = geom.find_pairing(Bs, Xs) if d["X_disjoint_B"] else (True, None)
    d["pairing_exists"] = bool(okp)
    ok = all(d[x] for x in ("X_disjoint_B", "F_subset_X", "size_ok", "admissible")) and not okp
    return ok, d


def stage_compare(a):
    lines = load_instances(a)
    meta = {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "instances_meta.jsonl"))}
    cert = {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "certify.jsonl"))}
    bnbo = {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "certify_bnbonly.jsonl"))}
    enum = {rkey(r): r for r in read_jsonl(os.path.join(a.outdir, "enum.jsonl"))}
    for r in read_jsonl(os.path.join(a.outdir, "enum4.jsonl")):
        enum[rkey(r)] = r
    ip = {}
    for r in read_jsonl(os.path.join(a.outdir, "ip_reused.jsonl")):
        ip[rkey(r)] = r
    for fn in sorted(os.listdir(a.outdir)):
        if fn.startswith("ip.") and fn.endswith(".jsonl") and "shard" in fn:
            for r in read_jsonl(os.path.join(a.outdir, fn)):
                kk = rkey(r)
                # keep the last decided record per instance (a timeout may be followed by a re-solve)
                if r.get("decision") in ("holds", "fails") or kk not in ip:
                    ip[kk] = r
    keys = []
    for l in lines:
        k, B, F = geom.parse_instance(l)
        keys.append(geom.fmt_instance(k, B, F))

    S = collections.OrderedDict()
    gl = os.path.join(a.outdir, "gen_log.json")
    if os.path.exists(gl):
        S["generation"] = json.load(open(gl))
    S["instances"] = len(keys)
    dec = collections.Counter(cert[k]["decision"] for k in keys)
    S["holds"], S["fails"], S["undecided"], S["void"] = dec["holds"], dec["fails"], dec["undecided"], dec["void"]
    by_method = collections.Counter(cert[k]["decision"] + "/" + cert[k]["method"] for k in keys)
    S["by_method"] = dict(sorted(by_method.items()))
    S["by_family"] = {}
    for k in keys:
        f = meta[k]["family"]
        S["by_family"].setdefault(f, collections.Counter())[cert[k]["decision"]] += 1
    S["by_family"] = {f: dict(c) for f, c in sorted(S["by_family"].items())}
    th = collections.Counter((cert[k]["decision"], cert[k]["theta"]) for k in keys)
    S["by_theta"] = {d: {str(t): th[(d, t)] for (dd, t) in sorted(th) if dd == d} for d in ("holds", "fails", "undecided", "void")}

    # ---- certify vs IP
    disagreements, agree = [], collections.Counter()
    ip_missing, ip_undecided, void_mismatch = [], [], []
    ip_secs = collections.defaultdict(list)
    n_reused = 0
    cmp_by_method = collections.Counter()
    for k in keys:
        c = cert[k]
        r = ip.get(k)
        if r is None:
            ip_missing.append(k)
            continue
        if r.get("reused_from"):
            n_reused += 1
        if c["decision"] == "void":
            if r.get("status") != "infeasible":
                void_mismatch.append({"instance": k, "certify": "void", "ip_status": r.get("status"), "ip_decision": r.get("decision")})
            else:
                agree["void"] += 1
            continue
        if r.get("status") == "infeasible":
            void_mismatch.append({"instance": k, "certify": c["decision"], "ip_status": "infeasible"})
            continue
        if r.get("decision") not in ("holds", "fails"):
            ip_undecided.append({"instance": k, "certify": c["decision"], "ip_status": r.get("status"), "seconds": r.get("seconds")})
            continue
        if r.get("seconds") is not None and not r.get("reused_from"):
            ip_secs[r["decision"]].append(r["seconds"])
        if c["decision"] == "undecided":
            agree["certify_undecided"] += 1
            continue
        if c["decision"] == r["decision"]:
            agree[c["decision"]] += 1
            cmp_by_method[c["decision"] + "/" + c["method"]] += 1
            continue
        # ---- a disagreement: go to the bottom with exact integer checks
        kk, B, F = geom.parse_instance(k)
        d = {"instance": k, "family": meta[k]["family"], "certify": c["decision"], "certify_method": c["method"],
             "ip": r["decision"], "ip_status": r.get("status")}
        if r["decision"] == "fails":
            ok, det = exact_check_witness(kk, B, F, geom.parse_set(r.get("X", "-")), r.get("S", []))
            d["ip_witness_exact_check"] = det
            d["verdict"] = ("the IP's X is a genuine admissible violator: bin/certify is WRONG" if ok else
                            "the IP's X is not a valid violator (see ip_witness_exact_check): the IP record is wrong")
        else:
            ok, det = exact_check_witness(kk, B, F, geom.parse_set(c.get("X", "-")), c.get("S", []))
            d["certify_witness_exact_check"] = det
            d["verdict"] = ("certify's X is a genuine admissible violator (checked with the independent Python geometry): "
                            "the IP's 'holds' is WRONG" if ok else "certify's witness fails the independent check: bin/certify is WRONG")
        disagreements.append(d)
    S["disagreements"] = disagreements
    S["ip"] = {
        "compared": agree["holds"] + agree["fails"] + len(disagreements),
        "agree_holds": agree["holds"], "agree_fails": agree["fails"], "agree_void": agree["void"],
        "agree_by_certify_method": dict(sorted(cmp_by_method.items())),
        "void_mismatches": void_mismatch,
        "ip_missing": len(ip_missing),
        "ip_missing_instances": [{"instance": k, "certify": cert[k]["decision"] + "/" + cert[k]["method"],
                                  "theta": cert[k]["theta"], "family": meta[k]["family"]} for k in ip_missing],
        "ip_undecided": ip_undecided,
        "reused_records": n_reused,
        "reused_by_source": dict(collections.Counter(ip[k]["reused_from"] for k in keys if k in ip and ip[k].get("reused_from"))),
        "seconds": {d: {"n": len(v), "total": round(sum(v), 1), "median": sorted(v)[len(v) // 2], "max": max(v)}
                    for d, v in ip_secs.items() if v},
    }
    wall = os.path.join(a.outdir, "ip_wall.json")
    if os.path.exists(wall):
        S["ip"]["run"] = json.load(open(wall))

    # ---- independent Python re-check of every certify witness
    nw = bad = 0
    for k in keys:
        c = cert[k]
        if c["decision"] != "fails":
            continue
        kk, B, F = geom.parse_instance(k)
        ok, det = exact_check_witness(kk, B, F, geom.parse_set(c["X"]), c["S"])
        nw += 1
        if not ok or det["deficiency"] < 1 or not det["S_live"]:
            bad += 1
            S.setdefault("bad_certify_witnesses", []).append({"instance": k, "check": det})
    S["certify_witnesses_rechecked_in_python"] = {"n": nw, "bad": bad}

    # ---- C++ re-check of every IP witness: (B, X_ip) must be admissible without a pairing,
    #      i.e. bin/phicheck on the instance "|X| B X" must answer reason 2
    ipw = []
    for k in keys:
        r = ip.get(k)
        if r is None or r.get("decision") != "fails" or cert[k]["decision"] == "void" or r.get("X", "-") in ("-", None):
            continue
        kk, B, F = geom.parse_instance(k)
        X = geom.parse_set(r["X"])
        if set(F) <= set(X) and len(X) <= kk:
            ipw.append((k, geom.fmt_instance(len(X), B, X)))
        else:
            S.setdefault("bad_ip_witnesses", []).append({"instance": k, "why": "F not in X or |X| > k"})
    if ipw:
        recs, _ = run_tool("phicheck", ["--quiet"], [l for _, l in ipw], nice=False)
        badw = [{"instance": k, "phicheck_reason": r["reason"]} for (k, _), r in zip(ipw, recs) if r["reason"] != 2]
        if badw:
            S.setdefault("bad_ip_witnesses", []).extend(badw)
        S["ip_witnesses_rechecked_in_cpp"] = {"n": len(ipw), "bad": len(badw)}

    # ---- exact vs exact
    mm = []
    cnt = collections.Counter()
    for k in keys:
        c, b = cert[k], bnbo.get(k)
        if b is None:
            continue
        cnt[(c["decision"], b["decision"])] += 1
        if "undecided" in (c["decision"], b["decision"]):
            continue
        if c["decision"] != b["decision"]:
            mm.append({"instance": k, "default": c["decision"] + "/" + c["method"], "bnb_only": b["decision"]})
        # LEAF true must give "no marked line at the root" (1 node)
        if c["leaf"] and b["decision"] == "holds" and b["nodes"] != 1:
            mm.append({"instance": k, "note": "LEAF true but the branch and bound needed %d nodes" % b["nodes"]})
    S["bnb_only_vs_default"] = {"pairs": {"%s|%s" % kv: n for kv, n in sorted(cnt.items())}, "mismatches": mm,
                                "bnb_only_nodes_max": max([b["nodes"] for b in bnbo.values()] or [0])}
    mm = []
    n = 0
    for k in keys:
        e = enum.get(k)
        c = cert[k]
        if e is None or c["decision"] in ("void", "undecided"):
            continue
        n += 1
        ed = "holds" if e["status"] == 1 else "fails" if e["status"] == 0 else "undecided"
        if ed == "undecided":
            continue
        if ed != c["decision"]:
            mm.append({"instance": k, "certify": c["decision"] + "/" + c["method"], "enumcheck": ed})
        b = bnbo.get(k)
        if b is not None and b["decision"] != "undecided" and b["decision"] != ed:
            mm.append({"instance": k, "bnb_only": b["decision"], "enumcheck": ed})
    eth = collections.Counter()
    for k in keys:
        e = enum.get(k)
        if e is not None and cert[k]["decision"] not in ("void", "undecided"):
            eth["theta %d: %s" % (cert[k]["theta"], "node limit" if e["status"] == -1 else
                                  cert[k]["decision"] + "/" + cert[k]["method"])] += 1
    S["enumcheck_vs_certify"] = {"n": n, "mismatches": mm, "by_theta_and_certify_method": dict(sorted(eth.items())),
                                 "note": "all non-void instances with theta <= 3; theta >= 4 only where bnb decided"}

    # ---- where LEAF is strictly weaker than C
    weak = []
    for k in keys:
        c = cert[k]
        if c["decision"] == "holds" and not c["leaf"]:
            r = ip.get(k, {})
            weak.append({"instance": k, "family": meta[k]["family"], "theta": c["theta"], "method": c["method"],
                         "nodes": c["nodes"], "bnb_only_nodes": bnbo.get(k, {}).get("nodes"),
                         "ip": r.get("decision"), "ip_seconds": r.get("seconds")})
    S["leaf_false_but_C_holds"] = {"n": len(weak), "by_theta": dict(sorted(collections.Counter(str(w["theta"]) for w in weak).items())),
                                   "by_method": dict(collections.Counter(w["method"] for w in weak)),
                                   "ip_confirms": sum(w["ip"] == "holds" for w in weak), "instances": weak}
    # ---- SPEC 3 fact: for F empty and k <= 10, C <=> LEAF
    fe = collections.Counter()
    viol = []
    for k in keys:
        c = cert[k]
        if c["F"] != "-" or c["decision"] not in ("holds", "fails"):
            continue
        tag = "k<=10" if c["k"] <= 10 else "k>=11"
        fe["%s leaf=%s %s" % (tag, c["leaf"], c["decision"])] += 1
        if c["k"] <= 10 and c["leaf"] != (c["decision"] == "holds"):
            viol.append(k)
    S["F_empty"] = {"counts": dict(sorted(fe.items())), "violations_of_C_iff_LEAF_for_k_le_10": viol}
    # ---- IP seconds by certify's theta (fresh solves only)
    ipt = collections.defaultdict(list)
    for k in keys:
        r = ip.get(k)
        if r and r.get("decision") in ("holds", "fails") and not r.get("reused_from") and r.get("seconds") is not None \
                and cert[k]["decision"] != "void":
            ipt["%s theta %02d" % (r["decision"], cert[k]["theta"])].append(r["seconds"])
    S["ip"]["seconds_by_theta"] = {t: {"n": len(v), "median": sorted(v)[len(v) // 2], "max": max(v)} for t, v in sorted(ipt.items())}
    # ---- timing of certify
    us = collections.defaultdict(list)
    for k in keys:
        c = cert[k]
        if "us" in c:
            us[c["decision"] + "/" + c["method"]].append(c["us"])
    S["certify_us"] = {m: {"n": len(v), "total": sum(v), "median": sorted(v)[len(v) // 2], "max": max(v)} for m, v in sorted(us.items())}
    with open(os.path.join(a.outdir, "summary.json"), "w") as fh:
        json.dump(S, fh, indent=1)
    brief = {k: v for k, v in S.items() if k not in ("leaf_false_but_C_holds", "by_theta", "certify_us")}
    brief["ip"] = {k: v for k, v in S["ip"].items() if k not in ("seconds_by_theta", "ip_undecided", "ip_missing_instances")}
    brief["ip"]["ip_undecided_n"] = len(S["ip"]["ip_undecided"])
    print(json.dumps(brief, indent=1)[:8000])
    print("leaf_false_but_C_holds: n=%d by_theta=%s ip_confirms=%d" % (
        S["leaf_false_but_C_holds"]["n"], S["leaf_false_but_C_holds"]["by_theta"], S["leaf_false_but_C_holds"]["ip_confirms"]))
    return 0 if not disagreements else 1


def main():
    ap = argparse.ArgumentParser(description="cross-validation certify vs IP (task 6b)")
    ap.add_argument("--stage", default="all", choices=["all", "gen", "certify", "ip", "compare", "enum4"])
    ap.add_argument("--seed", type=int, default=20261112)
    ap.add_argument("--outdir", default=P("results", "crosscheck"))
    ap.add_argument("--ip-shards", type=int, default=3)
    ap.add_argument("--ip-time-limit", type=float, default=300.0)
    ap.add_argument("--no-reuse", action="store_true", help="solve every instance now, ignore earlier IP records")
    ap.add_argument("--max-nodes", type=int, default=2000000)
    ap.add_argument("--enum-theta-max", type=int, default=4,
                    help="stage enum4: largest theta for which bnb-decided instances are re-decided by bin/enumcheck")
    ap.add_argument("--enum4-max", type=int, default=0, help="stage enum4: at most this many instances (0 = all)")
    ap.add_argument("--enum-limit", type=int, default=2000000000, help="node limit of that enumeration")
    # sample sizes
    ap.add_argument("--n-i", type=int, default=30, help="family i: instances per (size, k) combination")
    ap.add_argument("--n-first9", type=int, default=30)
    ap.add_argument("--n-true9", type=int, default=8)
    ap.add_argument("--n-ii-pert", type=int, default=70)
    ap.add_argument("--cap-ii-holds", type=int, default=1)
    ap.add_argument("--pool-iii", type=int, default=6000)
    ap.add_argument("--n-iii-holds", type=int, default=180)
    ap.add_argument("--cap-hi-holds", type=int, default=8)
    ap.add_argument("--n-iii-fails", type=int, default=200)
    ap.add_argument("--pool-v", type=int, default=80000)
    ap.add_argument("--n-v-leaf", type=int, default=60)
    ap.add_argument("--n-v-noleaf", type=int, default=160)
    ap.add_argument("--n-v-noleaf-hi", type=int, default=70)
    ap.add_argument("--n-v-fsearch", type=int, default=80)
    ap.add_argument("--n-v-fwit", type=int, default=60)
    ap.add_argument("--n-vi-pert", type=int, default=120)
    a = ap.parse_args()
    rc = 0
    if a.stage in ("all", "gen"):
        stage_gen(a)
    if a.stage in ("all", "certify"):
        stage_certify(a)
    if a.stage == "enum4":
        stage_enum4(a)
    if a.stage in ("all", "ip"):
        stage_ip(a)
    if a.stage in ("all", "compare"):
        rc = stage_compare(a)
    return rc


if __name__ == "__main__":
    sys.exit(main())

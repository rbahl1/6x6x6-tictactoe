#!/usr/bin/env python3
"""Regression tests for SPEC.md section 8, items 2-9 (item 1 belongs to src/minstones.cpp).

  nice -n 10 python3 py/regress.py [--procs 3] [--time-limit 600] [--out results/regression/ip_regression.json]

Every expected worst(B,F,k) is recomputed with the integer program (py/ip_check.py, full
mode, HiGHS, one thread per solve).  Exact integer checks (slack, pairing, orbit counts,
LEAF by max-flow) use py/geom.py only.  Output: a JSON list with one record per check
  {"item":..,"name":..,"expected":..,"got":..,"pass":bool,"seconds":.., ...}
and a PASS/FAIL table on stdout.
"""
import argparse
import itertools
import json
import math
import multiprocessing as mp
import os
import random
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geom  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEED9 = 20261112


def C(*triples):
    return geom.cells_xyz(list(triples))


# ----------------------------------------------------------------------------
# task definitions
# ----------------------------------------------------------------------------

def make_tasks():
    T7, T8 = geom.T7, geom.T8
    tasks = []

    def ip(item, name, B, F, k, expected, danger=True, mode="full"):
        tasks.append(dict(kind="ip", item=item, name=name, B=tuple(sorted(B)), F=tuple(sorted(F)),
                          k=k, expected=expected, danger=danger, mode=mode))

    # item 2
    tasks.append(dict(kind="slack", item="2", name="slack(T7)", B=T7, expected=1))
    tasks.append(dict(kind="pairing", item="2", name="pairing(T7, X=empty) exists", B=T7, X=(), expected=True))
    ip("2", "worst(T7,-,7)", T7, (), 7, 6)
    # item 3
    tasks.append(dict(kind="slack", item="3", name="slack(T8)", B=T8, expected=8))
    ip("3", "worst(T8,-,8)", T8, (), 8, 0)
    ip("3", "worst(T8,-,8) without danger constraints", T8, (), 8, 1, danger=False)
    ip("3", "worst(T8,-,9)", T8, (), 9, 1)
    # item 4
    for h in geom.BUSY:
        if h in T8:
            continue
        ip("4", "worst(T8,{%s},8)" % (",".join(map(str, geom.xyz(h)))), T8, (h,), 8, 0)
    # item 5
    B5 = tuple(sorted((set(T8) - set(C((5, 5, 5)))) | set(C((1, 1, 1)))))
    ip("5", "worst(T8-(5,5,5)+(1,1,1),{(5,5,5)},8)", B5, C((5, 5, 5)), 8, 0)
    # item 6: antipode stealing
    seq = [(0, 0, 0), (1, 1, 4), (2, 3, 2), (0, 5, 5), (1, 1, 1), (2, 2, 2), (0, 0, 5), (1, 4, 1),
           (2, 2, 3), (0, 5, 0)]
    for rounds, exp in ((8, 6), (9, 0), (10, 0)):
        B = C(*seq[:rounds])
        F = tuple(sorted(geom.ANTIP[geom.cid(*p)] for p in seq[:rounds - 1]))
        ip("6", "antipode stealing after %d rounds" % rounds, B, F, rounds, exp)
    # item 7: threat branch
    F7 = C(*[(x, 1, 2) for x in range(1, 6)])
    ip("7", "threat: worst(T8+(0,1,2),{(x,1,2):x=1..5},9)", tuple(sorted(set(T8) | set(C((0, 1, 2))))), F7, 9, 0)
    ip("7", "threat: worst(T7+(0,1,2),{(x,1,2):x=1..5},8)", tuple(sorted(set(T7) | set(C((0, 1, 2))))), F7, 8, 2)
    # item 8: orbit counts
    for k, exp in ((7, 527), (8, 1057), (9, 1669)):
        tasks.append(dict(kind="burnside", item="8", name="orbits of busy %d-subsets (Burnside)" % k, k=k, expected=exp))
        tasks.append(dict(kind="orbits", item="8", name="orbits of busy %d-subsets (direct enumeration)" % k, k=k, expected=exp))
    # item 9: 20 seeded random busy 8-sets
    rng = random.Random(SEED9)
    for i in range(20):
        B = tuple(sorted(rng.sample(geom.BUSY, 8)))
        ip("9", "random busy 8-set #%02d" % i, B, (), 8, None)
    # extra: decide mode must agree in sign with full mode on the basic instances
    ip("x", "decide: worst(T7,-,7) >= 1", T7, (), 7, "fails", mode="decide")
    ip("x", "decide: worst(T8,-,8) <= 0", T8, (), 8, "holds", mode="decide")
    ip("x", "decide: worst(T8,-,9) >= 1", T8, (), 9, "fails", mode="decide")
    # extra: exact brute force (matching enumeration, no floating point) for k - |F| <= 1.
    # Item 6 has exactly one free stone, so its three values are confirmed exactly.
    def brute(item, name, B, F, k, expected):
        tasks.append(dict(kind="brute", item=item, name=name, B=tuple(sorted(B)), F=tuple(sorted(F)),
                          k=k, expected=expected))
    for rounds, exp in ((8, 6), (9, 0), (10, 0)):
        B = C(*seq[:rounds])
        F = tuple(sorted(geom.ANTIP[geom.cid(*p)] for p in seq[:rounds - 1]))
        brute("6", "exact enumeration: antipode stealing after %d rounds" % rounds, B, F, rounds, exp)
    # IP against exact enumeration on instances with at most one free stone (expected = None:
    # the expected value is the exact one computed in the task)
    rng2 = random.Random(SEED9)
    R0 = tuple(sorted(rng2.sample(geom.BUSY, 8)))
    for name, B, F, k in (("empty board, k=0", (), (), 0), ("empty board, k=1", (), (), 1),
                          ("T7, k=1", T7, (), 1), ("T8, k=1", T8, (), 1),
                          ("random busy 8-set #00, k=1", R0, (), 1),
                          ("T7+(0,1,2), F=(x,1,2), k=6", tuple(sorted(set(T7) | set(C((0, 1, 2))))), F7, 6),
                          ("T8, F={(1,1,1)}, k=2", T8, C((1, 1, 1)), 2)):
        tasks.append(dict(kind="brute_vs_ip", item="x", name="IP = exact enumeration: " + name,
                          B=tuple(sorted(B)), F=tuple(sorted(F)), k=k, expected=None))
    for i, t in enumerate(tasks):
        t["idx"] = i
    return tasks


# ----------------------------------------------------------------------------
# orbit counting (item 8) with this module's own symmetry group
# ----------------------------------------------------------------------------

def burnside(k):
    perms = geom.busy_perms()
    total = 0
    for p in perms:
        seen = [False] * 24
        poly = [1] + [0] * k                 # prod over cycles of (1 + z^len), truncated at z^k
        for i in range(24):
            if seen[i]:
                continue
            n, j = 0, i
            while not seen[j]:
                seen[j] = True
                j = p[j]
                n += 1
            for d in range(k, n - 1, -1):
                poly[d] += poly[d - n]
        total += poly[k]
    assert total % len(perms) == 0, "Burnside sum not divisible by the group order"
    return total // len(perms), len(set(perms))


def _generators(perms):
    """A small generating set of the permutation group (verified by closure)."""
    distinct = sorted(set(perms))
    ident = tuple(range(24))
    gens = []
    group = {ident}
    for p in distinct:
        if p in group:
            continue
        gens.append(p)
        # closure
        frontier = list(group)
        group = set(group)
        while frontier:
            nxt = []
            for a in frontier:
                for g in gens:
                    b = tuple(g[a[i]] for i in range(24))
                    if b not in group:
                        group.add(b)
                        nxt.append(b)
            frontier = nxt
        if len(group) == len(distinct):
            break
    assert group == set(distinct), "generators do not generate the group"
    return gens, len(distinct)


def direct_orbits(k):
    perms = geom.busy_perms()
    gens, order = _generators(perms)
    tabs = []
    for g in gens:
        t = []
        for byte in range(3):
            row = [0] * 256
            for v in range(256):
                m = 0
                for bit in range(8):
                    if v >> bit & 1:
                        m |= 1 << g[8 * byte + bit]
                row[v] = m
            t.append(row)
        tabs.append(t)
    visited = bytearray(1 << 24)
    norb = 0
    nsets = 0
    sizes = {}
    for combo in itertools.combinations(range(24), k):
        m = 0
        for i in combo:
            m |= 1 << i
        if visited[m]:
            continue
        norb += 1
        visited[m] = 1
        stack = [m]
        size = 1
        while stack:
            cur = stack.pop()
            a, b, c = cur & 255, (cur >> 8) & 255, cur >> 16
            for t in tabs:
                im = t[0][a] | t[1][b] | t[2][c]
                if not visited[im]:
                    visited[im] = 1
                    stack.append(im)
                    size += 1
        assert order % size == 0, "orbit size does not divide the group order"
        sizes[size] = sizes.get(size, 0) + 1
        nsets += size
    assert nsets == math.comb(24, k), "orbits do not partition the k-subsets"
    return norb, len(gens), order, sizes


# ----------------------------------------------------------------------------
# worker
# ----------------------------------------------------------------------------

def run_task(t):
    t0 = time.time()
    rec = {"item": t["item"], "name": t["name"], "expected": t["expected"], "got": None,
           "pass": False, "seconds": 0.0, "idx": t["idx"], "kind": t["kind"]}
    try:
        if t["kind"] == "slack":
            rec["got"] = geom.slack(t["B"])
            rec["pass"] = rec["got"] == t["expected"]
            rec["instance"] = geom.fmt_set(t["B"])
        elif t["kind"] == "pairing":
            ok, _ = geom.find_pairing(t["B"], t["X"])
            rec["got"] = ok
            rec["pass"] = ok == t["expected"]
            rec["instance"] = geom.fmt_set(t["B"])
        elif t["kind"] == "burnside":
            n, distinct = burnside(t["k"])
            rec["got"] = n
            rec["pass"] = n == t["expected"]
            rec["note"] = "%d distinct permutations of the busy cells" % distinct
        elif t["kind"] == "orbits":
            n, ngen, order, sizes = direct_orbits(t["k"])
            rec["got"] = n
            rec["pass"] = n == t["expected"]
            rec["note"] = "%d generators, group order on busy cells %d, orbit sizes %s" % (
                ngen, order, json.dumps({str(s): c for s, c in sorted(sizes.items())}))
        elif t["kind"] == "brute":
            rec["got"] = geom.brute_worst(t["B"], t["F"], t["k"])
            rec["pass"] = rec["got"] == t["expected"]
            rec["instance"] = geom.fmt_instance(t["k"], t["B"], t["F"])
        elif t["kind"] == "brute_vs_ip":
            import ip_check
            exact = geom.brute_worst(t["B"], t["F"], t["k"])
            r = ip_check.solve_instance(t["B"], t["F"], t["k"], mode="full", time_limit=t["time_limit"])
            rec["expected"] = exact
            rec["got"] = r["worst"]
            rec["pass"] = exact is not None and r["worst"] == exact
            rec["instance"] = geom.fmt_instance(t["k"], t["B"], t["F"])
            rec["ip_status"] = r["status"]
            rec["ip_bound"] = r["bound"]
            rec["ip_seconds"] = r["seconds"]
        elif t["kind"] == "ip":
            import ip_check
            r = ip_check.solve_instance(t["B"], t["F"], t["k"], mode=t["mode"], danger=t["danger"],
                                        time_limit=t["time_limit"])
            rec["instance"] = geom.fmt_instance(t["k"], t["B"], t["F"])
            rec["mode"] = t["mode"]
            rec["danger"] = t["danger"]
            for key in ("worst", "decision", "status", "bound", "lb", "verified", "nodes", "note", "X"):
                rec["ip_" + key] = r[key]
            rec["ip_seconds"] = r["seconds"]
            rec["ip_S_size"] = len(r["S"])
            rec["timeout"] = r["status"] == "timeout"
            if t["mode"] == "decide":
                rec["got"] = r["decision"]
                rec["pass"] = r["decision"] == t["expected"]
            else:
                rec["got"] = r["worst"]
                if t["expected"] is None:
                    rec["pass"] = r["worst"] is not None       # item 9: evaluated in the summary
                else:
                    rec["pass"] = r["worst"] == t["expected"]
            # exact LEAF (max-flow) for the instance; only meaningful with the danger constraints
            if t["danger"] and t["mode"] == "full":
                t1 = time.time()
                holds, reason, info = geom.leaf_check(t["B"], t["F"], t["k"])
                rec["leaf"] = holds
                rec["leaf_reason"] = reason
                rec["leaf_seconds"] = round(time.time() - t1, 3)
                if reason == 3:
                    rec["leaf_witness"] = {"sur": info[1], "theta": t["k"] - len(t["F"]),
                                           "S": [list(geom.LINES[l]) for l in info[0]]}
                elif reason == 2:
                    rec["leaf_witness"] = {"hall": [list(geom.LINES[l]) for l in info]}
    except Exception as e:  # report, never hide
        rec["error"] = "%s: %s" % (type(e).__name__, e)
        rec["pass"] = False
    rec["seconds"] = round(time.time() - t0, 3)
    return rec


def _task_sig(t):
    inst = ""
    if "k" in t and "B" in t and "F" in t:
        inst = geom.fmt_instance(t["k"], t["B"], t["F"])
    elif "B" in t:
        inst = geom.fmt_set(t["B"])
    return "|".join([t["kind"], t["item"], t["name"], inst, str(t.get("mode", "")), str(t.get("danger", ""))])


def _sig(r):
    return "|".join([r["kind"], r["item"], r["name"], r.get("instance", ""), str(r.get("mode", "")),
                     str(r.get("danger", ""))])


def _init():
    try:
        os.nice(0)
    except Exception:
        pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--procs", type=int, default=3)
    ap.add_argument("--time-limit", type=float, default=600.0)
    ap.add_argument("--out", default=os.path.join(ROOT, "results", "regression", "ip_regression.json"))
    ap.add_argument("--only", default="", help="comma list of items to run (e.g. 2,3,8)")
    ap.add_argument("--cache", default=os.path.join(ROOT, "results", "regression", "ip_regression.cache.jsonl"),
                    help="finished checks are appended here and reused by later runs (resumable)")
    ap.add_argument("--fresh", action="store_true", help="ignore the cache and recompute everything")
    a = ap.parse_args()

    geom.selftest()
    tasks = make_tasks()
    if a.only:
        keep = set(a.only.split(","))
        tasks = [t for t in tasks if t["item"] in keep]
    for t in tasks:
        t["time_limit"] = a.time_limit
    t0 = time.time()
    recs = []
    # resumable: reuse finished checks of earlier runs (same check, same instance, same mode)
    cached = {}
    if not a.fresh and os.path.exists(a.cache):
        with open(a.cache) as fh:
            for line in fh:
                try:
                    r = json.loads(line)
                except ValueError:
                    continue
                if r.get("error") or r.get("timeout"):
                    continue
                cached[_sig(r)] = r
    todo = []
    for t in tasks:
        r = cached.get(_task_sig(t))
        if r is not None:
            r = dict(r)
            r["idx"] = t["idx"]
            r["cached"] = True
            recs.append(r)
        else:
            todo.append(t)
    if recs:
        print("reusing %d finished checks from %s; %d to run" % (len(recs), a.cache, len(todo)), flush=True)
    os.makedirs(os.path.dirname(os.path.abspath(a.cache)), exist_ok=True)
    with mp.Pool(a.procs, initializer=_init) as pool, open(a.cache, "a") as cfh:
        for rec in pool.imap_unordered(run_task, todo):
            rec["run_utc"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
            rec["cached"] = False
            recs.append(rec)
            cfh.write(json.dumps(rec) + "\n")
            cfh.flush()
            print("  [%3d/%d] item %-2s %-52s expected=%-6s got=%-6s %s  %.1f s" % (
                len(recs), len(tasks), rec["item"], rec["name"][:52], rec["expected"], rec["got"],
                "PASS" if rec["pass"] else "FAIL", rec["seconds"]), flush=True)
    recs.sort(key=lambda r: r["idx"])
    wall = time.time() - t0

    # derived checks -------------------------------------------------------
    derived = []
    nxt = len(recs)
    full = [r for r in recs if r["kind"] == "ip" and r.get("mode") == "full" and r.get("danger")
            and r.get("ip_worst") is not None and "leaf" in r]
    # (a) LEAF => C for every instance; for F = empty LEAF <=> (worst <= 0)
    bad, notes = [], []
    for r in full:
        fempty = r["instance"].split()[2] == "-"
        holds = r["ip_worst"] <= 0
        if r["leaf"] and not holds:
            bad.append(r["name"] + " (LEAF true but worst=%d)" % r["ip_worst"])
        if fempty and r["leaf"] != holds:
            bad.append(r["name"] + " (F empty, LEAF=%s, worst=%d)" % (r["leaf"], r["ip_worst"]))
        if (not fempty) and holds and not r["leaf"]:
            notes.append("%s: worst=0 but LEAF false, reason %d, witness sur=%s < theta=%s with |S|=%s" % (
                r["name"], r["leaf_reason"], r.get("leaf_witness", {}).get("sur"),
                r.get("leaf_witness", {}).get("theta"), len(r.get("leaf_witness", {}).get("S", []))))
    nleaf_true = sum(1 for r in full if r["leaf"])
    derived.append({"item": "leaf", "name": "LEAF (exact max-flow) consistent with the IP on all %d instances" % len(full),
                    "expected": "LEAF => worst<=0; F=empty: LEAF <=> worst<=0",
                    "got": "%d violations; LEAF true on %d, worst<=0 on %d" % (
                        len(bad), nleaf_true, sum(1 for r in full if r["ip_worst"] <= 0)),
                    "pass": not bad, "seconds": round(sum(r.get("leaf_seconds", 0) for r in full), 3),
                    "idx": nxt, "kind": "derived", "violations": bad, "leaf_false_with_worst0": notes})
    nxt += 1
    # (b) item 9 summary
    r9 = [r for r in recs if r["item"] == "9"]
    if r9:
        vals = [r["ip_worst"] for r in r9 if r.get("ip_worst") is not None]
        dist = {}
        for v in vals:
            dist[v] = dist.get(v, 0) + 1
        nbad = sum(1 for v in vals if v >= 1)
        n48 = sum(1 for v in vals if 4 <= v <= 8)
        derived.append({"item": "9", "name": "20 random busy 8-sets (seed %d): distribution of worst" % SEED9,
                        "expected": "mostly bad (worst 4-8)",
                        "got": "%d/%d bad, %d/%d with worst in 4..8, distribution %s" % (
                            nbad, len(r9), n48, len(r9),
                            json.dumps({str(k): dist[k] for k in sorted(dist)})),
                        "pass": len(vals) == len(r9) and 2 * nbad > len(r9) and 2 * n48 > len(r9),
                        "seconds": round(sum(r["seconds"] for r in r9), 3), "idx": nxt, "kind": "derived",
                        "distribution": {str(k): dist[k] for k in sorted(dist)}})
        nxt += 1
    recs += derived

    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, "w") as fh:
        json.dump(recs, fh, indent=1)
        fh.write("\n")

    # table ----------------------------------------------------------------
    print()
    print("%-4s %-58s %-10s %-10s %-5s %8s  %s" % ("item", "check", "expected", "got", "", "seconds", "IP status / LEAF"))
    print("-" * 130)
    for r in recs:
        extra = ""
        if r["kind"] in ("ip", "brute_vs_ip"):
            b = r.get("ip_bound")
            extra = "%s bound=%s" % (r.get("ip_status"), "%.3g" % b if isinstance(b, float) else b)
            if "leaf" in r:
                extra += " LEAF=%s" % r["leaf"]
                if not r["leaf"]:
                    extra += "(reason %d)" % r["leaf_reason"]
            if r.get("error"):
                extra += " ERROR " + r["error"]
        elif r.get("error"):
            extra = "ERROR " + r["error"]
        exp = "" if r["expected"] is None else str(r["expected"])
        got = str(r["got"])
        if r["kind"] == "derived":
            print("%-4s %s\n       expected: %s\n       got:      %s   -> %s" % (
                r["item"], r["name"], exp, got, "PASS" if r["pass"] else "FAIL"))
            for v in r.get("violations", []):
                print("       VIOLATION: " + v)
            for v in r.get("leaf_false_with_worst0", []):
                print("       note: " + v)
        else:
            print("%-4s %-58s %-10s %-10s %-5s %8.2f  %s" % (
                r["item"], r["name"][:58], exp[:10], got[:10], "PASS" if r["pass"] else "FAIL",
                r["seconds"], extra))
    npass = sum(1 for r in recs if r["pass"])
    timeouts = [r["name"] for r in recs if r.get("timeout")]
    ipsec = [r["ip_seconds"] for r in recs if r["kind"] in ("ip", "brute_vs_ip") and "ip_seconds" in r]
    print("-" * 130)
    print("%d/%d checks PASS, %d FAIL; wall %.1f s with %d processes" % (npass, len(recs), len(recs) - npass, wall, a.procs))
    if ipsec:
        s = sorted(ipsec)
        print("IP solves: %d, total %.1f s, min %.2f, median %.2f, max %.2f s; hit the %.0f s limit: %s" % (
            len(s), sum(s), s[0], s[len(s) // 2], s[-1], a.time_limit, timeouts if timeouts else "none"))
    print("written: " + a.out)
    return 0 if npass == len(recs) else 1


if __name__ == "__main__":
    sys.exit(main())

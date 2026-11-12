#!/usr/bin/env python3
"""Integer-program checker for worst(B,F,k)  (SPEC.md section 7.7).

Independent Python implementation (own geometry in py/geom.py, HiGHS via highspy).

Model (binary s_l, q_l for live lines l; binary n_c, x_c for cells c not in B):
    n_c >= s_l                (c in l)
    x_c <= n_c
    sum_c x_c <= k
    x_c = 1                   (c in F)
    sum_{c in l} x_c <= 4
    q_l >= sum_{c in l} x_c - 3
    q_a + q_b - x_c <= 1      (live a != b through c)        [dropped by --no-danger]
    maximise 2 sum s - sum n + sum x
The optimum is worst(B,F,k) = max over B-admissible X (F <= X, |X| <= k) and S of
2|S| - |N(S) \\ X|.

The IP is floating point: its results are *computed*, never *proved*.  Every answer is
post-verified in exact integer arithmetic as far as that is possible:
  * a solution with objective >= 1 is re-evaluated: X admissible (when the danger
    constraints are on), F <= X, |X| <= k and 2|S| - |N(S)\\X| recomputed from the sets;
  * a claim worst <= 0 cannot be verified from the solution; the reported dual bound is
    recorded ("bound").

CLI:
  python3 py/ip_check.py --in FILE --out FILE.jsonl [--mode full|decide] [--no-danger]
                         [--shard i --nshards N] [--time-limit SEC] [--prefilter]
Input: one instance per line "k B F" (blank lines and lines starting with '#' ignored).
Output: JSON lines
  {"k":..,"B":"..","F":"..","worst":int|null,"decision":"holds"|"fails"|null,
   "status":"..","seconds":..,"X":"..","S":[[6 cells],..], ...extra keys...}
With --shard i --nshards N only the instances with index = i (mod N) are processed and the
output goes to <out minus .jsonl>.shard<i>of<N>.jsonl.  Instances already present in the
output file are skipped (resumable).

API: solve_instance(B, F, k, mode="full", danger=True, time_limit=600.0) -> dict
"""
import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geom  # noqa: E402

import highspy  # noqa: E402

DEFAULT_TIME_LIMIT = 600.0


def build_model(B, F, k, danger=True):
    """Returns (lp, info) where info maps variables to column indices."""
    Bs = set(B)
    Fs = set(F)
    live = geom.live_lines(Bs)
    cells = [c for c in range(geom.NC) if c not in Bs]
    nl, ncell = len(live), len(cells)
    s_col = {l: i for i, l in enumerate(live)}
    q_col = {l: nl + i for i, l in enumerate(live)}
    n_col = {c: 2 * nl + i for i, c in enumerate(cells)}
    x_col = {c: 2 * nl + ncell + i for i, c in enumerate(cells)}
    ncol = 2 * nl + 2 * ncell

    cost = [0.0] * ncol
    lower = [0.0] * ncol
    upper = [1.0] * ncol
    for l in live:
        cost[s_col[l]] = 2.0
    for c in cells:
        cost[n_col[c]] = -1.0
        cost[x_col[c]] = 1.0
    for c in Fs:
        lower[x_col[c]] = 1.0

    inf = highspy.kHighsInf
    start, index, value, rlo, rup = [0], [], [], [], []

    def add_row(coefs, lo, up):
        for j, v in coefs:
            index.append(j)
            value.append(float(v))
        start.append(len(index))
        rlo.append(lo)
        rup.append(up)

    # n_c - s_l >= 0 for c in l
    for l in live:
        for c in geom.LINES[l]:
            add_row([(n_col[c], 1), (s_col[l], -1)], 0.0, inf)
    # n_c - x_c >= 0
    for c in cells:
        add_row([(n_col[c], 1), (x_col[c], -1)], 0.0, inf)
    # sum x <= k
    add_row([(x_col[c], 1) for c in cells], -inf, float(k))
    # sum_{c in l} x_c <= 4 ;  q_l - sum_{c in l} x_c >= -3
    for l in live:
        add_row([(x_col[c], 1) for c in geom.LINES[l]], -inf, 4.0)
        add_row([(q_col[l], 1)] + [(x_col[c], -1) for c in geom.LINES[l]], -3.0, inf)
    # danger: q_a + q_b - x_c <= 1 for live a != b through c
    ndanger = 0
    if danger:
        liveset = set(live)
        for c in cells:
            ls = [l for l in geom.CELL_LINES[c] if l in liveset]
            for i in range(len(ls)):
                for j in range(i + 1, len(ls)):
                    add_row([(q_col[ls[i]], 1), (q_col[ls[j]], 1), (x_col[c], -1)], -inf, 1.0)
                    ndanger += 1

    lp = highspy.HighsLp()
    lp.num_col_ = ncol
    lp.num_row_ = len(rlo)
    lp.col_cost_ = cost
    lp.col_lower_ = lower
    lp.col_upper_ = upper
    lp.row_lower_ = rlo
    lp.row_upper_ = rup
    lp.a_matrix_.format_ = highspy.MatrixFormat.kRowwise
    lp.a_matrix_.start_ = start
    lp.a_matrix_.index_ = index
    lp.a_matrix_.value_ = value
    lp.integrality_ = [highspy.HighsVarType.kInteger] * ncol
    lp.sense_ = highspy.ObjSense.kMaximize
    info = dict(live=live, cells=cells, s_col=s_col, q_col=q_col, n_col=n_col, x_col=x_col,
                ncol=ncol, nrow=len(rlo), ndanger=ndanger)
    return lp, info


def exact_value(B, F, k, X, S, danger=True):
    """Exact integer re-evaluation of a candidate (X, S).

    Returns (value, ok, why): value = 2|S| - |N(S)\\X|; ok iff (X,S) is a legal witness.
    """
    Bs, Fs, Xs = set(B), set(F), set(X)
    live = set(geom.live_lines(Bs))
    why = []
    if Xs & Bs:
        why.append("X meets B")
    if not Fs <= Xs:
        why.append("F not in X")
    if len(Xs) > k:
        why.append("|X| > k")
    if not set(S) <= live:
        why.append("S contains a dead line")
    if danger:
        if not geom.admissible(Bs, Xs):
            why.append("X not B-admissible")
    else:
        if any(len(geom.LINE_SET[l] & Xs) > 4 for l in live):
            why.append("cap exceeded")
    val = geom.deficiency(S, Xs)
    return val, (not why), ";".join(why)


def _status_name(h, st):
    try:
        return h.modelStatusToString(st)
    except Exception:
        return str(st)


def solve_instance(B, F, k, mode="full", danger=True, time_limit=DEFAULT_TIME_LIMIT,
                   prefilter=False, threads=1, log=False):
    """Solve worst(B,F,k) with HiGHS.  mode 'full' (optimal value) or 'decide' (sign only).

    Returns a dict with the SPEC 7.7 keys
        k, B, F, worst, decision, status, seconds, X, S
    plus: mode, danger, obj (HiGHS objective), bound (HiGHS dual bound), lb (exact value
    of the returned witness), verified (exact post-check passed), nodes, note.
    decision: "holds" (worst <= 0), "fails" (worst >= 1), or None (time limit / error).
    worst: exact integer in full mode; in decide mode 0 for "holds" and None for "fails"
    unless the search happened to close the gap ("lb" always carries the witness value).
    """
    t0 = time.time()
    B = tuple(sorted(B))
    F = tuple(sorted(F))
    Bs, Fs = set(B), set(F)
    rec = {"k": k, "B": geom.fmt_set(B), "F": geom.fmt_set(F), "worst": None, "decision": None,
           "status": "", "seconds": 0.0, "X": "-", "S": [], "mode": mode, "danger": bool(danger),
           "obj": None, "bound": None, "lb": None, "verified": False, "nodes": None, "note": ""}

    def done():
        rec["seconds"] = round(time.time() - t0, 3)
        return rec

    if Bs & Fs:
        rec["status"] = "invalid"
        rec["note"] = "B and F intersect"
        return done()
    # exact structural infeasibility (the IP would be infeasible): C holds vacuously
    f_adm = geom.admissible(Bs, Fs) if danger else all(
        len(geom.LINE_SET[l] & Fs) <= 4 for l in geom.live_lines(Bs))
    if len(Fs) > k or not f_adm:
        rec["status"] = "infeasible"
        rec["decision"] = "holds"
        rec["verified"] = True
        rec["note"] = "no admissible X: |F| > k" if len(Fs) > k else "no admissible X: F not B-admissible"
        return done()
    if prefilter:
        ok, pr = geom.find_pairing(Bs, Fs)
        if not ok:
            # exact certificate: X = F, S = Hall violator, value >= 1
            val, vok, why = exact_value(B, F, k, F, pr, danger)
            assert vok and val >= 1
            rec.update(status="prefilter", decision="fails", lb=val, verified=True,
                       X=geom.fmt_set(F), S=[list(geom.LINES[l]) for l in pr],
                       note="no pairing for (B,F); exact Hall violator")
            if mode != "full":
                return done()
            # in full mode the exact optimum is still wanted: fall through to the IP

    lp, info = build_model(B, F, k, danger)
    h = highspy.Highs()
    h.setOptionValue("output_flag", bool(log))
    h.setOptionValue("threads", int(threads))
    h.setOptionValue("time_limit", float(time_limit))
    h.setOptionValue("mip_rel_gap", 0.0)
    # objective is integral, so a gap below 1 proves optimality of the incumbent
    h.setOptionValue("mip_abs_gap", 0.99)
    if mode == "decide":
        # stop as soon as an incumbent with objective >= 1 exists
        h.setOptionValue("objective_target", 0.999)
    elif mode != "full":
        raise ValueError("mode must be 'full' or 'decide'")
    h.passModel(lp)
    h.run()
    st = h.getModelStatus()
    MS = highspy.HighsModelStatus
    hinfo = h.getInfo()
    name = _status_name(h, st)
    rec["nodes"] = int(hinfo.mip_node_count)
    have_sol = hinfo.primal_solution_status == highspy.kSolutionStatusFeasible
    obj = hinfo.objective_function_value if have_sol else None
    bound = hinfo.mip_dual_bound
    rec["obj"] = obj
    rec["bound"] = bound if abs(bound) < 1e30 else None

    if st == MS.kInfeasible:
        # should not happen after the exact check above
        rec["status"] = "infeasible"
        rec["note"] = "IP infeasible although F passed the exact admissibility check"
        return done()

    X, S, val, vok, why = (), [], None, False, "no solution"
    if have_sol:
        sol = h.getSolution().col_value
        X = tuple(c for c in info["cells"] if sol[info["x_col"][c]] > 0.5)
        S = [l for l in info["live"] if sol[info["s_col"][l]] > 0.5]
        val, vok, why = exact_value(B, F, k, X, S, danger)
        rec["lb"] = val
        rec["X"] = geom.fmt_set(X)
        rec["S"] = [list(geom.LINES[l]) for l in S]
        if not vok:
            rec["note"] = "witness failed exact check: " + why
        elif abs(val - obj) > 1e-6:
            rec["note"] = "exact value %d differs from solver objective %.6f" % (val, obj)

    if st == MS.kOptimal:
        rec["status"] = "optimal"
        if vok and abs(val - obj) < 1e-6 and bound < val + 0.995:
            rec["worst"] = val
            rec["decision"] = "fails" if val >= 1 else "holds"
            rec["verified"] = True if val >= 1 else False
            if val <= 0:
                # cannot be verified exactly from the solution; the dual bound is recorded
                rec["note"] = "worst<=0 rests on the floating-point dual bound %.6f" % bound
        else:
            rec["status"] = "optimal-unverified"
    elif st == MS.kObjectiveTarget or (mode == "decide" and vok and val is not None and val >= 1):
        rec["status"] = "target" if st == MS.kObjectiveTarget else name
        if vok and val >= 1:
            rec["decision"] = "fails"
            rec["verified"] = True
            if bound < val + 0.995:
                rec["worst"] = val
        else:
            rec["status"] = "target-unverified"
    elif st == MS.kTimeLimit:
        rec["status"] = "timeout"
        if vok and val is not None and val >= 1:
            rec["decision"] = "fails"
            rec["verified"] = True
        elif bound < 0.995:
            rec["decision"] = "holds"
            rec["worst"] = 0
    else:
        rec["status"] = name
        if vok and val is not None and val >= 1:
            rec["decision"] = "fails"
            rec["verified"] = True
        elif bound < 0.995 and have_sol:
            rec["decision"] = "holds"
            rec["worst"] = 0
    return done()


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------

def _key(k, B, F, mode, danger):
    return "%d %s %s" % (k, geom.fmt_set(B), geom.fmt_set(F))


def read_instances(path):
    out = []
    fh = sys.stdin if path == "-" else open(path)
    with fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            out.append(geom.parse_instance(line))
    return out


def shard_path(out, shard, nshards):
    if nshards <= 1:
        return out
    base = out[:-6] if out.endswith(".jsonl") else out
    return "%s.shard%dof%d.jsonl" % (base, shard, nshards)


def main(argv=None):
    ap = argparse.ArgumentParser(description="IP checker for worst(B,F,k) (SPEC 7.7)")
    ap.add_argument("--in", dest="inp", required=True, help="instance file ('-' = stdin)")
    ap.add_argument("--out", required=True, help="output JSON-lines file")
    ap.add_argument("--mode", choices=["full", "decide"], default="full")
    ap.add_argument("--no-danger", action="store_true", help="drop the danger constraints")
    ap.add_argument("--shard", type=int, default=0)
    ap.add_argument("--nshards", type=int, default=1)
    ap.add_argument("--time-limit", type=float, default=DEFAULT_TIME_LIMIT)
    ap.add_argument("--prefilter", action="store_true",
                    help="exact pre-check: no pairing for (B,F) => fails (decide mode skips the IP)")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args(argv)
    assert 0 <= a.shard < a.nshards

    inst = read_instances(a.inp)
    out = shard_path(a.out, a.shard, a.nshards)
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    danger = not a.no_danger
    donekeys = set()
    if os.path.exists(out):
        with open(out) as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                try:
                    r = json.loads(line)
                except ValueError:
                    continue          # truncated last line of an interrupted run
                if r.get("mode", a.mode) == a.mode and r.get("danger", danger) == danger \
                        and r.get("status") not in ("timeout",):
                    donekeys.add("%d %s %s" % (r["k"], r["B"], r["F"]))
    todo = [(i, t) for i, t in enumerate(inst) if i % a.nshards == a.shard]
    nskip = nrun = 0
    t_all = time.time()
    with open(out, "a") as fh:
        # make sure a truncated last line does not glue onto the next record
        if fh.tell() > 0:
            with open(out, "rb") as chk:
                chk.seek(-1, 2)
                if chk.read(1) != b"\n":
                    fh.write("\n")
        for i, (k, B, F) in todo:
            key = geom.fmt_instance(k, B, F)
            if key in donekeys:
                nskip += 1
                continue
            rec = solve_instance(B, F, k, mode=a.mode, danger=danger,
                                 time_limit=a.time_limit, prefilter=a.prefilter)
            fh.write(json.dumps(rec) + "\n")
            fh.flush()
            donekeys.add(key)
            nrun += 1
            if not a.quiet:
                print("[%d] %s -> worst=%s decision=%s status=%s lb=%s bound=%s  %.2f s"
                      % (i, key, rec["worst"], rec["decision"], rec["status"], rec["lb"],
                         "%.3f" % rec["bound"] if rec["bound"] is not None else None,
                         rec["seconds"]), flush=True)
    if not a.quiet:
        print("done: %d solved, %d skipped (already present), %.1f s total -> %s"
              % (nrun, nskip, time.time() - t_all, out), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())

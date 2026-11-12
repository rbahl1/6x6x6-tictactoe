# logs/search/NOTES.md — tasks 4+5, `src/search.cpp` → `bin/search`

All times UTC 2026-11-12. Numbers are from runs on the shared 4-core box (load average 5–8 while measuring,
so wall-clock ≈ CPU time even with `--threads 4`).

## How to run

```
make bin/search
nice -n 10 bin/search --rmax 10 --maxcand 4 --relcand 10 --nonbusy --threads 4 \
     --out results/search/r10_nb_k4 --selfcheck            # the proof run (root WON, ~90 s)
bin/search --lost-selftest 3000                            # lostCheck vs literal E2/E3 transcription
```

Run it from the project root (the cache `results/search_cache/` is a relative path).

Options: `--rmax R` `--maxcand K` `--threads T` `--out DIR` `--time-limit SEC` `--nonbusy` (SPEC 7.4) plus
`--relcand K2` (length of the ordered candidate list per Q-state, default 2K; the real search tries the
first K of those that survive the relaxed filter), `--nbextra N` (max. number of violator-set cells added
by `--nonbusy`, default 6), `--order 0|1` (0: child (a) before the forced children, 1 = default: forced
children first), `--tt-bits N` (default 25 = 512 MB table), `--selfcheck`, `--noemit`, `--progress SEC`.
Exit code 0 iff proved (and, with `--selfcheck`, the re-walk found no error).

Outputs in DIR: `strategy.tbl` (`t B F m`, canonical frame), `leaves.txt` (`t B F`), `stats.json`
(`proved, rmax, p_states, q_states, leaves, seconds, failing_branch` + counters, peak RSS).
`results/search_cache/good8.txt`, `good9.txt`: the LEAF-good busy 8- and 9-sets (masks over `G().busy`),
computed once (6 of 1057 orbits = 2340 masks for k=8; 172 of 1669 orbits = 83904 masks for k=9; 1.7 s).

## What the program does

* `proveP / proveQ` implement SPEC 4 literally, in the canonical frame of each state: VOID children are
  skipped (`admissible`), LEAF first (`leaf_holds`), R_max cut, LOST (E2, E3: cheap lower-bound screen, then an
  exact enumeration), children (c) threats, (d) dangers, (a); Q: table move m with (i) and (ii).
* Transposition table: lock-free open addressing, 16 bytes/entry (64-bit key + 48 check bits + status +
  move), keyed by `canon_pair` + t + type. Status NOTLEAF / FAILED / WON-LEAF / WON(+move). One run = one
  (R_max, maxcand), so FAILED needs no qualifier.
* Things that can only produce FAILED (never WON), i.e. heuristics that are irrelevant for soundness:
  * leaf screen: total surplus of all live lines ≥ t−|F| (necessary for LEAF) before calling `leaf_holds`;
  * **relaxed pre-pass**: the same game without LOST and without forced children (types 2, 3 in the table).
    real-WON ⇒ relaxed-WON, so a relaxed failure cuts the real P-state, and table moves whose (i) or (ii)
    fail in the relaxed game are skipped. This is what makes failure fast;
  * potential bound inside the relaxed game: a stone on d live lines raises (total surplus − θ) by at most
    2d−2; if the best r = R_max−t stones cannot close the deficit, fail;
  * candidate order at Q-states: (1) moves that give an immediate LEAF, (2) completes an antipodal pair,
    (3) number of LEAF-good busy 8-sets / 9-sets containing (B∩busy)+m and avoiding F, (4) slack.
    With `--nonbusy` a second list H is interleaved: the cells (busy or not) lying on the most lines of the
    violator set S returned by `phi_check(B,F,t)` (closed F-connected set with too little surplus / Hall
    violator), top `--nbextra`.
* Parallelism: the forced children of *trunk* P-states (reached by table moves and reveals only) run in an
  OpenMP `parallel for` (dynamic), shared table, cancellation flag per region. Duplicate work is possible,
  results are deterministic functions of the state.
* Output: a walk of the proof tree from the root (dedupe by canonical state) writes the two files.
* `--selfcheck`: reads the two files back into string-keyed maps and re-expands SPEC 4 from the root with a
  naive child enumeration (all lines × e; all line pairs × 4-subsets), `phi_check` for the leaves and
  `lostNaive` (a literal transcription of E2/E3) for LOST.
* On failure `stats.json.failing_branch` is the path root → failing state with the reason at every step
  (which threat/danger child, which candidates were tried and which obligation failed).
* Not implemented: the F-monotonicity shortcut (WON(B,F,t) ⇒ WON(B,F',t)); it is not needed.

## Results

| run | result | seconds | notes |
|---|---|---|---|
| `--rmax 9 --maxcand 4 --relcand 24` (busy only, relaxed game exhaustive over all 24 busy cells) | FAILED | 13.8 | fails already **without any forced move** |
| `--rmax 10 --maxcand 4 --relcand 24` (busy only) | FAILED | 21.8 | same branch |
| `--rmax 11 --maxcand 4 --relcand 8` (busy only) | FAILED | 23.9 | same branch |
| `--rmax 9 --maxcand 4 --relcand 10 --nonbusy` | FAILED | 11.7 | two successive threats, see below |
| `--rmax 9 --maxcand 6 --relcand 16 --nbextra 10 --nonbusy` | FAILED | 128.1 | same failing branch (`results/search/r9_nb_k6/stats.json`) |
| `--rmax 10 --maxcand 4 --relcand 10 --nonbusy` | **WON** | 87–89 | selfcheck ok, see below |

### Busy-only table moves cannot work (finding, verify independently)

With busy-only table moves the root fails for R_max = 9 and 10 even in the relaxed game (no threats, no
dangers, no LOST) with *all* 24 busy cells tried at every Q-state. Failing branch (in `logs/search/busyonly_r9_rel24.log`,
`busyonly_r10_rel24.log`): the reveals give the FP the four corners of one face (cells 180,185,210,215 = face
x=5). The 14 lines inside a face can only be killed by a stone inside that face, and the only busy cells of
a face are its 4 corners. With F = the 4 corners, S = those 14 lines has sur_F(S) = 36−4−28 = 4, while
θ = t−4 keeps growing, so LEAF is impossible for t > 8, and by t = 8 the SP does not have a LEAF either.
So `--nonbusy` is required, not optional. (SPEC 4 allows it "behind a flag"; noted in `logs/spec_issues.md`.)

### R_max = 9 with non-busy moves: FAILED (maxcand 4, relcand 10)

Failing branch: P(t=4, B=0,46,169,215, F=∅) → threat on line 1,7,13,19,25,31 (reply 1) → table moves 92, 123,
35 → P(t=8) → second threat on line 6,7,8,9,10,11 (reply 11) → P(t=9, B=0,1,11,35,46,92,123,169,180,
F=6,7,8,9,10,13,19,25,31) is not a LEAF at R_max.

### R_max = 10 with non-busy moves: root WON

Command: `bin/search --rmax 10 --maxcand 4 --relcand 10 --nonbusy --threads 2 --out results/search/r10_nb_k4 --selfcheck`
(log `logs/search/r10_nb_k4.log`). **Deliverable files: `results/search/r10_nb_k4/strategy.tbl`, `leaves.txt`,
`stats.json`.**

* Proof tree (distinct canonical states reachable from the root): 152,900 P-states = 46,219 expanded +
  106,681 leaves; 92,775 Q-states (= lines of `strategy.tbl`); 143,551 non-VOID threat children and 28,293
  danger children (counted with repetitions); max t = 10; 422 of the 92,775 table moves are non-busy
  (19 at t=7, 384 at t=8, 7 at t=9, 12 at t=10).
  Leaves by t: 3,382 (t=8), 86,931 (t=9), 16,368 (t=10). Leaves by θ = t−|F|: 64,903 (θ=0), 32,601 (1), 7,275 (2),
  1,427 (3), 338 (4), 107 (5), 25 (6), 4 (7), 1 (8).
  P-states by t (treecheck): 1,2,5,14,35,181,1715,7204,34588,92787,16368.
* Search effort (this run, older build that leaf-tested every candidate): 48,995 P-states and 95,347
  Q-states expanded, 49,153 / 97,716 relaxed P / Q, 1,197,557 table entries, 925,441 `leaf_holds` calls (925,212
  true, 46,499 more rejected by the screen, 183.7 CPU-s), 4,297 exact LOST enumerations (0 true), peak RSS 601 MB
  (512 MB of that is the fixed 2^25-slot table). 175.9 s wall with 2 threads while the box had load 8–11.
  Earlier identical runs: 87.4 s (`--threads 4`), 88.9 s (`--threads 3`).
* Current build (`bin/search`, candidate leaf tests done lazily in score order): **identical output files**
  (`cmp` of `results/search/r10_lazy_test/` against `r10_nb_k4/`), 80.8 s wall with 2 threads under the same
  load (68 s user CPU including emit and selfcheck), 654,555 leaf tests (79.1 CPU-s), 926,450 table entries.
* `--selfcheck`: P=46,219 Q=92,775 leaves=106,681, 0 errors (both builds; the second one with `lostNaive`).
* `bin/search --lost-selftest 3000`: 3000 random admissible states, 976 LOST, 0 mismatches between
  `lostCheck` and the literal transcription.
* The independent verifier (`bin/treecheck`, other agent, binary of 06:09) run by me on these files:
  `{"ok":true,"p_states":152900,"q_states":92775,"leaves_checked":106681,"max_t":10,"errors":[],"complete":true}`
  in 11.4 s (`results/search/r10_nb_k4/treecheck.json`, log `logs/search/treecheck_r10.log`); it also counts
  143,551 threat and 28,293 danger children and 422 non-busy moves. (Its owner should re-run it into
  `results/final/`.)

* `--maxcand 2 --relcand 6 --nonbusy --rmax 10`: root WON in 36.6 s (2 threads, load ≈ 5) with the *same*
  `strategy.tbl` (`cmp`), 434,318 leaf tests (28.9 CPU-s): in the winning tree the first surviving candidate
  always works. (`results/search/r10_nb_k2/stats.json`; the duplicate table/leaf files were deleted.)
* `results/search/r10_nb_k4/stats.json` is from a re-run of the installed binary with `--threads 4 --selfcheck`
  (96.4 s wall at load ≈ 8, 84 s user CPU in total, output `cmp`-identical; log
  `logs/search/r10_nb_k4_current_build.log`); `stats_firstbuild.json` is the 175.9 s run described above.
  Wall-clock times vary between 37 s and 176 s with the load of the shared box; CPU time is ≈ 60–85 s.

* **Headers changed at 06:14** (`match.hpp`, `phi.hpp`, performance tuning by their owner) and `make` rebuilt
  `bin/search` at 06:15:54. The rebuilt binary with the same command line again proves the root (82.8 s, 4 threads,
  selfcheck 0 errors) but emits a slightly different tree: 153,135 P-states (46,412 expanded + 106,723 leaves),
  93,164 Q-states, 143,498 threat and 28,275 danger children. Reason: with `--nonbusy` the candidate list uses
  the violator set S returned by `phi_check`, which is a witness and not unique, so table moves can differ
  between `phi.hpp` versions; LEAF itself did not change (all 106,681 leaves of the first tree are still LEAF
  under the new header, checked with a scratch program). This second tree is in
  `results/search/r10_nb_k4_rebuilt/` and `bin/treecheck` (binary of 06:15:43) also returns
  `"ok":true,"p_states":153135,"q_states":93164,"leaves_checked":106723,"max_t":10,"errors":[]` for it
  (`logs/search/treecheck_r10_rebuilt.log`). Either tree is a complete proof tree; `r10_nb_k4` is the one all the
  numbers above refer to. `bin/search --lost-selftest 1000` on the rebuilt binary: 331 LOST, 0 mismatches.

* Smoke test with the other agent's simulator (`bin/sim`, binary of 06:13:45, still under development) on
  `results/search/r10_nb_k4`: adversaries random / greedy / steal / threat, 2000 games each, seed 7: 0 FP lines,
  0 strategy holes, 0 pairing failures, 0 admissibility failures in all four (`results/search/r10_nb_k4/simtry_*.json`;
  leaf rounds 8–10, 2,743 threat events with `greedy`, 16,889 occupied reveals with `steal`). The official
  10^6-game run is the simulator task's job.

## Observations / diagnosis

* The coordinator's estimate "600 threats × subtree at every trunk state" is right in shape but the tree is small:
  after a threat u drops to 0–1, so the threat subtrees are a few states each; almost all cost is `leaf_holds`
  at t = 9 (≈0.12–0.25 ms per call under load).
* VOID never occurred among forced children of expanded states (treecheck: 149 VOID reveal children (ii)).
* LOST never fits in any visited state (screen passed 4,297 times, exact enumeration always false).
* R_max = 9 fails under maxcand 6 / relcand 16 because of the double threat above (first threat at t=4, second at
  t=8 through a stone of the first line); this was not searched exhaustively over all 216 cells, so R_max = 9 is
  "not proved", not "refuted".
* Performance is not an issue any more; if a smaller tree is wanted, try `--maxcand 2..3`.

## Open issues

* `leaf_holds`, `admissible`, `canon_pair` come from the other tasks' headers; `--selfcheck` shares them
  (it replaces `leaf_holds` by `phi_check` and the child/LOST enumeration by naive code). Full independence is
  only given by `verify/treecheck`.
* The leaf screen assumes that the live lines contain an F-connected component of ≥ 3 lines whenever there are
  ≥ 3 live lines; if that ever failed the only effect would be a missed LEAF (FAILED instead of WON).
* Parallelism is only over the forced children of trunk P-states; on the loaded box no speed-up was measurable.
* `--order 0` and the F-monotonicity shortcut were not evaluated.
* The emitted tree is a deterministic function of the sources only as long as `phi_check` returns the same
  witness S (see the 06:14 header change above). Re-running after another header change may give a third,
  equally valid tree; always re-run `--selfcheck` and `bin/treecheck` on the files that are finally used.
* `--nonbusy` without a violator-based list (the first version, "cells on the most loaded live lines") was
  replaced before it was measured.


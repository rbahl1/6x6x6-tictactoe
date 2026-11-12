# REPORT.md — 6×6×6 tic-tac-toe is a draw

Coordinator's final report, 2026-11-12 (UTC). Every claim is labelled **[proved]** (complete written proof in
PROOF.md, refereed), **[computed]** (result of a program run; the programs and the independent re-checks are
named), **[cited]** (standard result), or **[open]** (not established).

## 1. Status

**PROVED** (computer-assisted): in 6×6×6 tic-tac-toe the second player (SP) has a strategy under which the
first player (FP) never owns a complete line. With strategy stealing (FP can always avoid losing) the game is
a draw.

The claim rests on two parts:

1. **[proved]** PROOF.md, Theorem 0.2: if the root of the abstract game tree of SPEC §4 is WON for some R_max
   (hypothesis H), then the strategy σ of PROOF.md Definition 8.3 always prescribes an empty cell and FP never
   completes a line. Corollary 0.4 (draw) follows by strategy stealing **[proved in PROOF.md §0; classical,
   cited there]**. A hostile referee re-derived every numbered statement and found no false statement and no
   gap; 7 minor points were applied (§11).
2. **[computed]** Hypothesis H holds with R_max = 10. The certificate is `results/final/strategy.tbl`
   (93,164 table entries) and `results/final/leaves.txt` (106,723 leaves). It was verified in exact integer
   arithmetic by programs that share no code and were written by different agents: `verify/treecheck.cpp`
   (C++), `verify/spotcheck.py` (Python), and the re-expansions written by the verifier's auditor and by the
   lead reviewer (§4). Every leaf was recomputed by each of them.

What "computer-assisted" means here: the proof is as reliable as the written argument in PROOF.md and the
correctness of at least one of the three verifiers (plus compiler, Python interpreter and hardware). No
floating-point computation is part of the proof. The HiGHS integer programs, the simulator and the exact
enumeration are cross-checks only.

## 2. Method as executed, and where it deviates from the task statement

| Task statement | What was done | Why | Label |
|---|---|---|---|
| Leaf certificate C(B,F,k), decided by an integer program, later certified exactly | Leaf predicate **LEAF** (SPEC §3): F admissible, a pairing for (B,F) exists, and every F-connected set of ≥ 3 live lines has surplus ≥ k−\|F\|. Decided by integer max-flow over connected triples of lines. | LEAF ⇒ C (a Hall violator can be taken connected; 1 line breaks the cap, 2 lines form a danger pair). It is exact, fast (about 50 µs per leaf), monotone in B, F and k, and equal to C when F = ∅ and k ≤ 10. The IP became a cross-check. | [proved] PROOF §3–§6 |
| L3(c): C(B,F,k) ⇒ C(B∪{b},F,k) | Not used. Only the LEAF version is used. | L3(c) is false as literally stated when F is not B-admissible (C is then vacuous; counterexample in PROOF Prop. 5.4) and true when F is B-admissible. | [proved] PROOF §5 |
| Branch (b): FP takes a busy cell, revealed at once ("eager") | **Lazy revelation**: an FP stone is revealed only by a threat, a danger, or when SP's table move turns out to be occupied; SP then looks up again. | Removes the ~20-way branching per round, is sound because the leaf certificate only needs F ⊆ X, and makes non-busy SP moves expressible. The eager rule is the special case where SP probes every busy cell. | [proved] PROOF §8 |
| SP table moves are busy cells; non-busy moves only as a fallback | **Non-busy table moves are needed** and are used at 434 of the 93,164 table entries. | With busy-only moves the lazy game fails for R_max ≤ 10 even without threats and dangers (exhaustive over all 24 busy cells at every state): FP ends up holding the four corners of one face, whose 14 lines can only be killed from inside the face. | [computed] `bin/search` only, `logs/search/busyonly_r*_rel24.log`; not re-checked by a second program |
| Loss cases (e): two threats, threat + danger, two dangers at different cells | Two patterns E2 (threat + surviving danger) and E3 (two dangers on four distinct lines). | Two simultaneous threats would require a danger pair before the move. E2 needs ≥ 11 FP stones and E3 ≥ 12, so neither can occur at t ≤ 9; in a tree with R_max = 10 the loss test is never positive. | [proved] PROOF Lemmas 8.5–8.7, Prop. 8.4a |
| R_max = 9, then 10, then 11 | R_max = 10 succeeds. R_max = 9 failed with up to 6 candidate moves per state (double-threat branch: first threat at round 5, second at round 9). | Not exhaustive over all cells, so "R_max = 9 is impossible" is not established. | [computed] / [open] |
| Cloud threads sharing a repository | All compute ran locally on 4 cores. | User instruction during the run: no commits or pulls to any repository. No repository was created or touched. | — |
| Symmetry group of 1152 maps; optionally nauty | The full automorphism group of the cell–line incidence structure has order 1152 = the given group. | Own partition-refinement search (`logs/canon/autgroup.cpp`), no nauty. | [computed]; group properties [proved] PROOF §7 |

## 3. The certificate

`results/final/` (search: `bin/search --rmax 10 --maxcand 4 --relcand 10 --nonbusy`, 58 s, 601 MB).
sha256: `strategy.tbl` 13629393fcdcee87a03f2c9c73f640d9584d2134a46f90dad9393dc92c9ce3fc,
`leaves.txt` 3c14c7814e296e812b4fdf3cda2f446c6ce4865d15d351980e7fecc7690fd470.

| t | P-states | of which leaves | Q-states | threat children | danger children |
|---|---|---|---|---|---|
| 0 | 1 | 0 | 0 | 0 | 0 |
| 1 | 2 | 0 | 2 | 0 | 0 |
| 2 | 5 | 0 | 5 | 0 | 0 |
| 3 | 14 | 0 | 14 | 0 | 0 |
| 4 | 35 | 0 | 35 | 0 | 0 |
| 5 | 181 | 0 | 101 | 824 | 0 |
| 6 | 1,718 | 0 | 446 | 4,075 | 0 |
| 7 | 7,227 | 0 | 3,487 | 14,460 | 0 |
| 8 | 34,801 | 3,382 | 14,362 | 47,020 | 20,416 |
| 9 | 93,124 | 87,314 | 62,381 | 72,352 | 6,060 |
| 10 | 16,027 | 16,027 | 12,331 | 4,767 | 1,799 |
| total | 153,135 | 106,723 | 93,164 | 143,498 | 28,275 |

(States are counted up to the 1152 symmetries; children are counted at the round of the child, with
repetitions. P-state = FP to move, Q-state = SP to move.) 46,412 P-states are expanded. 151 children are
VOID (the table move would be a fifth revealed FP stone on a live line), all of type (ii). Leaves by
θ = t − |F| (number of unrevealed FP stones): 64,841 / 32,703 / 7,267 / 1,434 / 343 / 105 / 25 / 4 / 1 for
θ = 0..8. **[computed]** — identical numbers from all three verifiers.

The strategy in words: SP plays towards one of the six orbits of busy 8-sets with slack 8 (T8's orbit is one);
when a wanted cell is occupied it is revealed and SP re-plans; threats and dangers are answered by the forced
cell; where FP has taken busy cells that SP needs, SP uses non-busy cells on the lines of the low-surplus set.
Every branch reaches a LEAF state by round 10 at the latest, after which SP plays the pairing strategy (L1).

## 4. Verification of the certificate (hypothesis H) — all [computed], exact integer arithmetic

| Program | Author / independence | Mode | Result |
|---|---|---|---|
| `bin/treecheck` (`verify/treecheck.cpp`, C++, 4,012 lines) | separate agent; own geometry, group, canonical form, max-flow; never read `src/` or `sim/` | `--exact --paranoid` (visited set by full keys; every flow behind a LEAF verdict validated literally) | ok, 246,299 states, 0 errors, `results/final/treecheck.json` |
| same | | `--crosscheck --crosscheck-leaves 50`: all canonical forms by brute force over the 1152 maps; children and LOST of all 46,412 expanded states against literal reference code; 2,153 leaves with unpruned flows | ok, 945 s, `results/final/treecheck_crosscheck.json` |
| `verify/spotcheck.py` (pure Python) | third agent; written from SPEC §1, 3, 4, 6 only; shares no code with the other two | exhaustive: every reachable state expanded, every leaf recomputed, exact keys | ok, same counters as treecheck, 338 CPU-s, `results/final/spotcheck_full.json` |
| `bin/search --selfcheck` | the search's own re-walk (shares `src/` headers; not independent) | naive child enumeration, literal LOST | ok, P = 46,412, Q = 93,164, leaves = 106,723 |
| structural re-expansion by the treecheck auditor | fourth author, own geometry and brute-force canonical form | children and VOID counts in every round; own flow code on all leaves | reproduces every counter; all 106,723 leaves LEAF (`logs/review/treecheck_audit.md`) |
| `lead_expand.cpp` + `lead_leaf.cpp` by the lead reviewer (`logs/review/lead/`) | fifth author; own geometry, brute-force canonical form, exact keys, literal children; own max-flow | full re-expansion; LEAF on all leaves, and with no pruning rule at all on the 1,912 leaves with θ ≥ 3 | ok, 0 errors, every counter in every round equal to treecheck and spotcheck (`REVIEW.md` §2.3) |

Audit of the trust anchor `verify/treecheck.cpp` (xhigh, `logs/review/treecheck_audit.md`): no fatal or major
finding. 66 seeded mutations of the certificate files: 58 rejected; the 8 accepted ones neither remove nor
change an obligation (e.g. extra unreachable lines, an equivalent key in another frame). LEAF against brute
force on 53,755 evaluations and LOST against a literal transcription on 800 states: 0 mismatches. 12 source
mutants of the verifier: its self-test catches all 12.

## 5. How each leaf was certified

| θ = t−\|F\| | leaves | LEAF by exact max-flow (`src/phi.hpp`, treecheck, spotcheck.py; plus the auditor's flow code) | C by exact enumeration (`bin/enumcheck`) | C by HiGHS (floating point) |
|---|---|---|---|---|
| 0 | 64,841 | all | all | 300 sampled, all hold |
| 1 | 32,703 | all | all | 300 sampled, all hold |
| 2 | 7,267 | all | all | 150 sampled, all hold |
| 3 | 1,434 | all | all (739 s) | 80 sampled, all hold |
| 4 | 343 | all | — | all 343 hold |
| 5 | 105 | all | — | all 105 hold |
| 6 | 25 | all | — | all 25 hold |
| 7 | 4 | all | — | all 4 hold |
| 8 | 1 | all | — | holds |

So HiGHS covers every one of the 478 leaves that exact enumeration does not (θ ≥ 4), plus 830 sampled others:
1,308 leaves, all "holds".

Every leaf is certified by an exact method (integer max-flow, recomputed by three independent
implementations) **[computed]**; LEAF ⇒ C is **[proved]**. For 106,245 leaves (99.55 %) C itself is also
decided by exact enumeration of the free stones **[computed]**. No leaf relies on the integer program.
`bin/phicheck` and `bin/treecheck --leafcheck` also agree (verdict and reason) on 20,000 perturbed non-leaf
instances derived from the leaves (13,688 true; reasons 1/2/3 = 590/4,135/1,587).

## 6. Regression tests (task statement §3) — all pass, [computed]

| Item | Expected | Got | Program |
|---|---|---|---|
| Minimum SP stones for a pairing on the empty board, n = 5 / 6 / 7 / 8 | 9 / 7 / 3 / 0 | 9 / 7 / 3 / 0 (lower bounds by exhaustive counting search, upper bounds by explicit pairing) | `bin/minstones`, `results/regression/minstones.json` |
| slack(T7); pairing; worst(T7,∅,7) | 1; yes; 6 | 1; yes; 6 | `py/regress.py` (HiGHS), `results/regression/ip_regression.json` |
| slack(T8); worst(T8,∅,8); without danger constraints; worst(T8,∅,9) | 8; 0; 1; 1 | 8; 0; 1; 1 | same |
| worst(T8,{h},8), 16 busy h ∉ T8 | 0 | 0 (all 16) | same |
| worst(T8 − (5,5,5) + (1,1,1), {(5,5,5)}, 8) | 0 | 0 | same |
| Antipode stealing after 8 / 9 / 10 rounds | 6 / 0 / 0 | 6 / 0 / 0 (also by exact enumeration) | same |
| Threat branch k = 9; T7 variant k = 8 | 0; 2 | 0; 2 | same |
| Orbits of busy k-subsets, k = 7 / 8 / 9 | 527 / 1057 / 1669 | 527 / 1057 / 1669 (Burnside and direct enumeration, in C++ and in Python) | `bin/test_canon`, `py/regress.py` |
| Random busy 8-sets mostly bad, worst 4–8 | — | 20 of 20 bad; worst 2–12, 16 of 20 in 4..8 | same |

In every regression instance with worst ≤ 0, LEAF is true, and with worst ≥ 1 it is false (45 instances).
Differences from the task statement's experience: "holds" instances with a non-empty F and large θ take
26–155 s in HiGHS (not 1–12 s); the range of worst for random 8-sets is wider than 4–8.

## 7. Leaf table: LEAF(B,∅,|B|) for every orbit of busy sets — [computed]

| \|B\| | orbits | slack ≥ \|B\| | LEAF true (exact max-flow, `results/leaftable/phi_k*.jsonl`) | HiGHS on the orbits with slack ≥ \|B\| |
|---|---|---|---|---|
| 8 | 1,057 | 6 | 6 | 6 of 6 solved, all hold |
| 9 | 1,669 | 947 | 172 | 262 of 947 solved (254 fail, 8 hold), all equal to the max-flow table; stopped for lack of time |
| 10 | 2,468 | 2,185 | 0 | not run |

For F = ∅ and k ≤ 10, LEAF ⇔ C **[proved]**, so the max-flow column is the exact value of the table.
|B| = 10 is all false: a live triangle of lines takes 4 + 3 + 3 admissible stones, so C(B,∅,10) fails
whenever a live triangle exists **[proved]** (PROOF Remark 6.3), and every busy 10-set leaves one
**[computed]** (0 of 2,468 orbits LEAF-true; the lead reviewer's `triangle_k10.py` finds a live triangle in
2,468 of 2,468). Hand argument, not part of the refereed PROOF.md: a busy stone kills exactly 3 of the 36 face
diagonals, so 10 stones leave one alive; in its plane z = c the rows and columns with index outside {c, 5−c}
contain no busy cell, and such a row and column with r ≠ s, s ≠ 5−r form a triangle with the diagonal.
The table is not used by the proof.

## 8. Integer program versus exact certifier — [computed]

`bin/certify` (LEAF, explicit admissible violator, exact enumeration, or a complete branch and bound; all
integer) against `py/ip_check.py` (HiGHS, decide mode) on 1,291 seeded instances of 7 families
(`results/crosscheck/summary.json`): 656 hold, 623 fail, 12 void (F not admissible), none undecided by the certifier. HiGHS decided 1,275 of the 1,279 non-void instances (4 timeouts at 300 s, all F = ∅ with θ = 8–9 that hold by LEAF): **0 disagreements** (652 holds, 623 fails); it reports all 12 void instances as infeasible. All 623 IP violators were re-checked with the exact C++ predicates and all 623 certifier violators with the independent Python geometry.
Among the instances that hold, 160 have LEAF false and C true (decided by enumeration or branch and bound):
LEAF is strictly weaker than C when F ≠ ∅; no such state is needed by the certificate.
Together with §5–§7: the integer program and the exact methods agree on every instance on which both were run
(regression 45, leaf table 268, leaves 1,308, cross-check 1,275). "Holds" answers of the IP rest on
a floating-point bound and are labelled computed, never proved.

## 9. Simulator (task 7) — [computed]

`sim/sim.cpp` (independent C++, written from SPEC §1, 3, 5, 6; audited and extended with stronger
adversaries, `logs/review/sim_audit.md`) plays σ with the real pairing computation.

Official run: `results/final/sim/sim_mix_1M.json` (seed 20261112, 4 threads, 1,537 s, 651 games/s), made with
the source snapshot `results/final/sim/sim_snapshot_0806.cpp`; the final `sim/sim.cpp` is byte-identical to
that snapshot (md5 c2112bb91772a246dd155816c927736d). A separate 4,000-game run with `--paranoid
--verify-canonical` (`results/final/sim/sim_paranoid_4k.json`; the audit's
`logs/review/sim_audit_runs/final_paranoid4k.json`) checks the canonical form of all 199,887 lines of both
files against the simulator's own (0 non-canonical) and cross-checks canonical forms, admissibility and the
matcher against naive references (0 failures).

| | |
|---|---|
| games | 1,000,000 (all completed) |
| FP lines | **0** |
| strategy holes (missing table entry) | 0 |
| leaves without a pairing for the real position | 0 |
| non-admissible position after an SP table-mode move | 0 |
| illegal table moves / internal errors / bugs inside look-ahead | 0 / 0 / 0 |
| leaf reached at round 8 / 9 / 10 | 117,835 / 832,519 / 49,646 |
| threat events / danger events / occupied reveals | 1,121,484 / 85,806 / 2,254,365 |
| leaves after two threats / after a danger | 375,806 / 85,806 |
| non-busy table moves played | 36,890 (in 32,493 games) |
| unrevealed FP stones at the leaf, θ = 0..8 | 535,247 / 208,915 / 85,349 / 61,140 / 33,012 / 7,209 / 9,854 / 25,851 / 33,423 |
| distinct table keys met in real games | 78,957 of 93,164 (84.8 %); including look-ahead simulations of σ: 85,050 (91.3 %) |
| distinct leaf keys met in real games | 81,907 of 106,723 (76.7 %); including look-ahead: 88,071 (82.5 %) |

Adversaries (games): random 30,228; greedy 29,735; steal 40,256; threat 49,882; search (depth-2 look-ahead
against the deterministic σ) 20,019; danger 89,871; double (two successive threats) 90,004; corner (takes
the corners of a face and antipodes, then builds threats) 150,084; tree (uses the loaded table as an oracle
to steer into rarely visited states) 499,921. In addition 1,859,012 "leaf stress tests" (pairing recomputed
for sampled admissible completions at visited leaves) found no failure. Earlier runs: 3,000 and 4 × 2,000
games on the first tree, 200,000 and several 60,000-game runs during the audit, 20,000 games inside the
reproduction run: 0 bugs in every run.

The simulator did not reach 14,207 table states and 24,816 leaf states in real games
(`results/final/sim/sim_mix_1M_uncovered.txt`; almost all at t = 9 or 10 with |F| ≥ 8, i.e. after two
threats, a danger or long reveal chains: 67 % of the t = 10 table keys and 61 % of the t = 10 leaves were
hit). These states are covered only by the verifiers of §4. Minor audit finding left as is: a missing table
entry met while an adversary *predicts* σ's next move is not counted (the real game normally runs into the
same entry and counts it).

## 10. Search findings — [computed], `logs/search/NOTES.md`

* Busy-only table moves: root not WON for R_max = 9 and 10 (exhaustive in the game without forced events,
  14–22 s), nor for R_max = 11 with 8 candidates per state.
* Non-busy moves, R_max = 9: fails with 4 and with 6 candidates per state (12 s, 128 s).
* Non-busy moves, R_max = 10: root WON in 58–176 s wall depending on load (about 60–85 CPU-s); with
  `--maxcand 2` the same table in 37 s.
* 654,555 leaf tests, 926,450 transposition entries, 4,307 exact loss-pattern enumerations (0 positive).

## 11. Reviews

| Review | Scope | Verdict |
|---|---|---|
| Math referee (xhigh), `logs/review/math_referee.md` | every statement of PROOF.md re-derived; own brute-force checks on the real geometry | sound; 7 minor findings, all applied (PROOF.md revision log) |
| Search/core code audit (xhigh), `logs/review/search_core_audit.md` | `src/search.cpp`, `phi.hpp`, `match.hpp`, `canon.hpp` against SPEC §3, 4, 6; brute-force references on 588,000 instances | sound; 6 minor findings, none changes a result |
| Verifier audit (xhigh), `logs/review/treecheck_audit.md` | `verify/treecheck.cpp` line by line; 66 file mutations; 12 source mutants; own re-expansion | sound; 8 minor findings (below) |
| Third implementation (high), `verify/spotcheck.py` | exhaustive re-verification in Python | ok |
| Simulator audit (high), `logs/review/sim_audit.md` | SP side of `sim/sim.cpp` against SPEC §5 line by line, bug detectors, look-ahead paths, evidence files, self-test and a paranoid run | sound; minor findings: coverage is partial and must be stated (done in §9); audit runs with the same seed are not independent samples; the first audit agent was stopped by the coordinator while still tuning adversaries, and `sim/sim.cpp` was reset to the snapshot of the official run |
| **Task 8, lead reviewer (xhigh), `REVIEW.md`** | PROOF.md L1–L4 re-derived; the code implementing L4 read in `verify/treecheck.cpp` and `verify/spotcheck.py`; admissibility, symmetry, stop rule; own re-expansion and own LEAF on all leaves; own attack on the abstraction (1,860 games of σ in real coordinates); all evidence files recounted | **SIGNED OFF**, with the limits quoted below |

Limits stated in the sign-off (REVIEW.md §6): the result is computer-assisted (PROOF.md plus at least one
correct verification of H, with its compiler or interpreter and this one machine; nothing is formally
verified); it applies to the two certificate files with the recorded checksums; it is **not** a sign-off of
the tree the task statement describes (eager branch (b), busy-cell table moves), which was not proved; it
does not cover minimality of R_max, the claim that busy-only moves cannot work, or any HiGHS result.

Known minor issues left as they are (none affects the certified run): `treecheck` accepts a table with two
different moves for one key (counted, 0 in the certified run), leaves a stale `--out` file if it aborts,
parses numeric options leniently, and reports ok for a VOID `--root`; its table/leaf look-ups use 120-bit
keys even with `--exact` (SPEC §10.6 explains why this cannot accept a non-WON tree; spotcheck.py uses exact
keys). The search's `stats.json` counts only threat/danger VOID children (0), not the 151 reveal ones.

## 12. Acceptance criteria (task statement §6)

| Criterion | Status | Evidence |
|---|---|---|
| Every regression test of §3 passes | PASS | §6 |
| The AND–OR search proves a second-player win from the empty board under σ | PASS (lazy revelation, non-busy moves, R_max = 10) | §3, §4 |
| Every leaf certified by an exact method | PASS (integer max-flow ×3; exact enumeration for 99.55 %) | §5 |
| Integer program and exact certifier agree everywhere both were run | PASS (2,896 instance comparisons, 0 disagreements; 4 IP timeouts excluded) | §5–§8 |
| Task 7 finds no loss | PASS (10^6 games, 0 FP lines, 0 holes, 0 pairing or admissibility failures) | §9 |
| Task 8 signs off | PASS ("SIGNED OFF", REVIEW.md §6) | REVIEW.md |

## 13. What remains unverified or open

* **[open]** Whether R_max = 9 suffices, and whether busy-only moves suffice for some R_max ≥ 11 with all
  candidates, or in the eager variant of the task statement (branch (b)); the eager variant was not searched.
* **[computed, single program]** The failure of busy-only moves (§10) was established by `bin/search` only.
* **[computed, partial]** HiGHS leaf table: |B| = 9 covers 262 of 947 orbits; |B| = 10 not run (the exact
  max-flow table is complete for all three sizes).
* Trusted base of the proof: PROOF.md; one of `verify/treecheck.cpp`, `verify/spotcheck.py`; g++ 13.3 /
  CPython 3.13; the hardware. No formal (proof-assistant) verification was attempted.
* Strategy stealing is the classical argument; PROOF.md §0 contains it in full.
* The simulator is evidence, not proof; its coverage of the tree is stated in §9.

## 14. Compute and timings

4 cores (Xeon 2.1 GHz), 15 GB, one machine, no cloud sessions, no repository. Wall-clock from start to this
report: about 3 h 40 min (05:08–08:50 UTC). Agents: all on the model requested in the task statement; effort xhigh for the coordinator and tasks 4, 5, 6, 8 (and the
proof writer, referee and audits), high for tasks 1, 2, 3, 7. Heavy jobs: search 1 CPU-min; treecheck 8 CPU-s
(deep cross-check 16 min); spotcheck.py 6 CPU-min; enumeration of the θ = 3 leaves 12 min; HiGHS cross-checks
about 5 core-hours; simulator 10^6 games 26 min wall (31 CPU-min).

## 15. Reproduction

`scripts/reproduce.sh` rebuilds everything, runs all self-tests, regression 1, the leaf table, the search,
the two independent verifiers, the leaf cross-checks and the simulator into `results/repro/`, and compares
the tree with `results/final/` (about 55 minutes on 4 idle cores, of which 26 are the 10^6 simulated games;
`--with-ip` adds the HiGHS checks, hours).
Validated end to end on this machine with `GAMES=20000 THREADS=2` while other jobs were running: all 26 steps passed in under half an hour, and the regenerated tree is byte-identical to `results/final/` (`logs/final/reproduce_validation.log`). The full default run (10^6 games) and `--with-ip` were not run through the script; their commands are the ones used for the official results.

## 16. Files

* `PROOF.md` (mathematics), `SPEC.md` (definitions and interfaces; §10 errata), `REVIEW.md` (task 8),
  `PROGRESS.md` (decisions and timeline), `logs/spec_issues.md`, `logs/review/` (audit reports).
* Certificate: `results/final/strategy.tbl`, `results/final/leaves.txt`; verification outputs in
  `results/final/*.json`, leaf checks in `results/final/checks/`, simulator in `results/final/sim/`.
* Code: `src/` (geometry, canonical forms, matching, LEAF, search, certifier, leaf table, minimum stones),
  `verify/` (treecheck.cpp, spotcheck.py), `sim/sim.cpp`, `py/` (HiGHS checker, regression, cross-check),
  `scripts/` (reproduce.sh and helpers).

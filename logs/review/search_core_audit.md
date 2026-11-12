# Code audit: `src/search.cpp`, `src/phi.hpp`, `src/match.hpp`, `src/canon.hpp`

Reviewer role: adversarial code auditor for the AND-OR search and the core library (review part B).
Date 2026-11-12, 06:41-07:10 UTC. Files audited (md5 at start and at the end, unchanged in between):

| file | mtime (UTC) | md5 |
|---|---|---|
| `src/search.cpp` | 06:07:10 | 4766b324376f7cc804675cb6276d619c |
| `src/phi.hpp` | 06:26:55 | 467b9cd76d7196908b4daeba15e24f01 |
| `src/match.hpp` | 06:14:24 | 80e134d54c93b05bd78558220551726b |
| `src/canon.hpp` | 05:49:59 | a44071dda3696110623df5c8b020cc70 |

`src/phi.hpp` was re-checked at 07:08 UTC: not modified during the audit.

## Verdict

**sound-with-minor-issues.** I found no way for `bin/search` to record a state WON that is not WON in the sense
of SPEC 4, apart from a 112-bit hash collision in the transposition table (finding 1), and no difference
between `leaf_holds` / `phi_check` / `phi_value` / `admissible` / `find_pairing` / `enum_check` / `canon_pair`
and my own brute-force references on 588,000 random and adversarial instances. No fatal and no major finding.
Six minor findings (none changes a result). No file under `src/` was edited.

What this verdict does **not** cover is listed in "Not verified" at the end.

## What was done

All reviewer code is in `logs/review/search_core/` (`audit_core.cpp`, `audit_search.cpp`); build with
`g++ -O2 -std=c++17 -march=native -fopenmp -I src -o X logs/review/search_core/X.cpp`. Logs:
`audit_core_run1.log`, `audit_run2.log`, `audit_run3.log`, `audit_walk.log`, `r10_now.log`, `run_*.log`.
Own compute: about 10 CPU-minutes including compilation.

### 1. Existing self-checks, run by me

| command | result | CPU |
|---|---|---|
| `bin/test_core` (binary 06:29) | 56,082 checks, 0 failures, `TEST_CORE OK` | 27.3 s |
| `bin/test_canon` (binary 06:06) | `TEST_CANON PASSED (0 failed checks)` | 18.9 s |
| `bin/search --lost-selftest 2000` (binary 06:15, and again with the binary rebuilt at 06:58) | 2000 states, 638 LOST, 0 mismatches (both) | 4.7 s each |

### 2. Reading `search.cpp` against SPEC 4 (line numbers of the audited revision)

Every place that can produce WON:

* **P1** (`proveP` l.608, `leafTest` l.143): `leaf_holds(B,F,t)` on the canonical P-state, k = t, before the
  R_max cut and before LOST. LEAF is tested only on P-states (`leafNow` tests child (i) of a Q-state, a P-state).
* **P2** (l.609-633): requires `t < RMAX` (l.609), `lostCheck` false (l.623), all children of `genForced`
  (l.365) WON or VOID (`runForced` l.564), and child (a) `proveQ(B,F,t+1)` (l.627/629). `S_WON` is written
  only at l.632, only in the non-relaxed path (l.620 returns before it in the relaxed game).
* **Children (c)** (l.368-377): every live line, every e not in F, condition `5 - |F∩l| <= t+1-|F|`, which
  equals `|F ∪ (l∖{e})| <= t+1` because e ∉ F. **Children (d)** (l.378-397): every cell c ∉ B∪F, every unordered
  pair of live lines through c, every omitted cell oa ∈ a∖{c}, ob ∈ b∖{c} not in F (these are exactly the
  4-subsets containing F∩a, F∩b), condition `(4-|F∩a|)+(4-|F∩b|) <= t+1-|F|` = `|F∪Qa∪Qb| <= t+1` (Qa, Qb
  disjoint, c ∉ F). Nothing is skipped except VOID (`admissible(c.B, c.F)` with the child's B, l.573/586).
* **Q-states** (l.636-670): move m from `orderCands`, which only returns cells outside B∪F (l.442, l.474);
  (i) `proveP(B+m,F,t)`; (ii) required iff `|F| < t` and `F+m` is B-admissible (l.657), else VOID or absent.
  The move is stored and emitted in the canonical frame of the Q-state (l.665, l.704).
* **Abort/cancel**: a return value -1 is never treated as 1 (l.616, 630, 659-664, 579-582); no table entry
  is written on the -1 paths. A skipped iteration in the parallel loop implies `res == 0` (cancel) or a
  final -1 (`gAbort`, l.581).
* **LOST** (`lostCheck` l.163-258): I re-derived every side condition of E2/E3 and each of the five pruning
  inequalities (l.170, 183, 190, 220, 226). All are true lower bounds: `|Sl∖F| = 5-|F∩l| >= 5-maxj`,
  `|Sa∖F|+|Sb∖F| = (4-|F∩a|)+(4-|F∩b|) >= max(1, 2(4-maxj))` (>= 1 because F is admissible), at most 2 resp. 4
  shared cells. The conditions not tested explicitly (x ∉ b, e ≠ c, b ≠ a', b' ≠ a, x ∉ b∪b') follow from
  "two lines share at most one cell".
* **Heuristics** (each can only produce FAILED/NOTLEAF): leaf screen (l.137, result only `S_NOTLEAF`),
  relaxed pre-pass (types 2/3, separate keys; relaxed WON is never copied to a real key), potential bound
  (l.614, relaxed FAILED only), candidate limits `--maxcand/--relcand/--nbextra`, candidate order
  (good8/good9 cache, `phi_check` witness). `leafNow`'s answers are deterministic functions of the state.
* **Transposition table** (l.62-115): lock-free, slot key `a` never changes once set, check bits never change
  once set, status is only raised. See finding 1 for collisions.

### 3. Tests of `search.cpp` internals (`audit_search.cpp` includes `search.cpp` with `main` renamed)

| test | numbers | result |
|---|---|---|
| `lostCheck`, `lostNaive` on planted E2/E3 patterns at the tight t (`t+1 = |F ∪ stones|`), F = random part of the stones plus stones elsewhere, tight budgets 1..15+, max `|F∩line|` 0..4 | 3,000 patterns (1,440 E2, 1,560 E3) | all reported LOST |
| `lostCheck`, `lostNaive` vs my own transcription of E2/E3 (only monotone pruning), t = tight-1..tight-3, budget <= 6 | 4,118 comparisons, 1,275 LOST | 0 mismatches |
| `genForced` vs literal children (all pairs of live lines, all 4-subsets by bit mask) | 800 states, 5,709,658 distinct children (339,828 threat + 5,369,830 danger children generated, 72,358 of them VOID) | sets equal |
| TT stress, 4 threads, 20,000 keys (2,000 in one probe chain, 400 with identical 64-bit part) | 1.6 million operations | 0 inconsistent reads, 0 wrong final entries |
| fresh build of `search.cpp` with the current headers, `--rmax 10 --maxcand 4 --relcand 10 --nonbusy --threads 2 --selfcheck` | root WON in 60.3 s, selfcheck 0 errors | `strategy.tbl`, `leaves.txt` byte-identical to `results/final/` (md5 caa0bde1..., 95f25737...) |
| my own re-expansion of `results/final` from the root (literal admissibility, literal children, LEAF by `leaf_holds`) | 153,135 P-states = 46,412 expanded + 106,723 leaves; 93,164 Q-states; 143,498 threat and 28,275 danger children; 54,331 reveal children; 434 non-busy moves; max t 10 | 0 errors; counts equal `stats.json` and `treecheck.json` |

Two facts measured by the re-expansion that matter for the argument:

* **0 VOID children (c)/(d)** in the whole tree (with my literal admissibility): the VOID rule never removes
  a forced child in this certificate.
* **LOST cannot fit anywhere in an R_max = 10 tree**: expanded states have t <= 9, a pattern needs >= 11
  distinct stones (PROOF Lemma 8.4), so `|F ∪ stones| <= t+1 <= 10` is impossible. `lost_true = 0` in
  `stats.json` is therefore no evidence about `lostCheck`; the tests above are.

### 4. `phi.hpp` (LEAF)

Reading: every `return true` in `PhiCore::run` is justified: `theta <= 0` after admissibility and a built
pairing (l.233); no F-connected triple (l.238, `triple_centre` is exact: a connected triple has a vertex of
degree >= 2); end of `run` after `triples()` (l.276). A line left unmarked is safe by an explicit flow
(`popcount(fm) >= theta`, `can_reach` for theta = 1, or theta successful `gain`s). The triple enumeration
(comment (e)) is complete: the smallest F-adjacent pair (a,b) of a connected triple has a = the smallest
line, the third line is F-adjacent to a (then c > b, first loop l.335) or only to b (second loop l.341-351,
F-adjacency to a tested through the common cell). Stars through one cell, triangles and paths whose end
lines share a revealed cell are all enumerated (mutants M1, M5, M15 below). Restoration: every successful `gain` is followed by a `release` of the same line before
the state is reused; closed sets stay closed, so a and b stay at their maximum. Every `false` with reason 3
is re-verified from the definition (`fail3`) or aborts.

References (own geometry, own Kuhn matching, own subset enumeration, own BFS max-flow):

| reference | instances | reasons 0/1/2/3 | tight | result |
|---|---|---|---|---|
| direct enumeration of all subsets of the live lines, <= 16 lines | 200,000 (77,060 states) | 109,229 / 17,788 / 1,150 / 71,833 | 55,431 with min sur = theta, 55,070 with min sur = theta-1 | 0 failures |
| same, <= 20 lines | 12,002 | 6,583 / 982 / 69 / 4,368 | 3,369 / 3,346 | 0 failures |
| literal max-flow per F-connected triple, any size (9,183 mid-size and 4,602 search-like states, 9.5 million triples) | 29,661 | 16,978 / 2,651 / 2,490 / 7,542 | 7,542 / 7,542 | 0 failures |
| same, sampled lines of `results/final/leaves.txt` and of `results/search/r10_nb_k4/leaves.txt`, with k and k+1 | 1,100 | 1,004 / 0 / 0 / 96 | - | 0 failures |

Compared on every instance: `leaf_holds`, `phi_check().holds`, `phi_check().reason`, and the witness (Hall
violator with negative surplus; reason 3: >= 3 live lines, F-connected, `sur` as reported, < theta, >= the
true minimum). `phi_value` compared 70,562 times. The two references agree with each other on 15,535 states.
theta ranged over 0..11+ (LEAF true for every theta 0..10). Shapes included: 3-5 stones on one live line,
4+3 stones on two lines through an empty cell, 4+4 with the common cell revealed, F cells on dead lines,
F ∩ B ≠ ∅, non-busy B cells, heavy admissible fills.
Concurrency: 20,000 instances x 3 on 4 threads give the single-threaded answers (0 differences).

Mutation check of the test itself (copies of the headers in the scratchpad, never in `src/`): 11 seeded
bugs in `phi.hpp` are detected: third lines adjacent to b only dropped (M1); part of the third lines
around a dropped (M5); a-c adjacency through a revealed cell counted as F-adjacency (M15); safe-pair
shortcut for need <= 2 (M3); third-line shortcut for need <= 2 (M4); lines unsafe by flow treated as safe
(M2); no second `triples` pass (M13); first pass only for >= 4 unsafe lines (M12); free-cell shortcut off
by one (M9); `theta <= 1` shortcut (M11); no memo invalidation at all (M7+M8+M14). One more (blocked cells
not skipped among the neighbours of a, M16) aborts on the internal witness check. Three single removals of
`++cstamp` (in `release`, in `gain`, before the joint search) are not detected and are equivalent mutants:
each is individually redundant (freeing cells cannot invalidate "reaches a free cell", and every successful
`gain` is followed by a `release`); removing all three is detected (1,047 failures in 60,000 instances).

### 5. `match.hpp`

* `admissible`: 300,000 instances (200,306 admissible; 76,844 with a cap violation on a live line; dead
  lines with 5-6 stones, danger pairs with the common cell occupied, pairs with one dead line): 0 mismatches
  with the literal definition. "Exactly 4" is used after the cap test, so it equals "at least 4"; dead
  lines carry -64 and never reach 4. Two seeded bugs detected.
* `find_pairing`: 20,000 instances (6,349 with a pairing): `ok` equals my Kuhn matching; owners checked
  (each live line exactly 2 cells of its own, none in B or X, dead lines none); every Hall violator has
  negative surplus on live lines. On the admissible ones of 81,650 small states `ok` also equals "min over
  all non-empty S of sur >= 0" by enumeration.
* `enum_check`: 6,000 instances, depth k-|F| = 0..3, against the literal definition (every X ⊇ F, |X| <= k,
  any cell outside B, admissible ⇒ pairing; 4.7 million stone sets): 4,490 hold (517 vacuous), 1,510 fail,
  0 mismatches; every returned violator verified. The pairing is examined at every node; the only skipped
  placements are inadmissible ones and a last stone on an unassigned cell. A seeded bug "skip unassigned
  cells at every depth" is detected (it was not detected by my first, weaker generator; the numbers above
  are from the strengthened one).

### 6. `canon.hpp`

20,000 pairs against a brute force over all 1152 maps with my own order (10,211 pairs with more than one
map attaining the minimal B image; 4,702 with a non-trivial stabiliser of the canonical pair, up to 1152
maps; orbits of random subgroups, B empty, F empty, the full board, all busy cells, the 8 corners, lines,
antipodal F): `B`, `F` are the minimum, `g` is the smallest index attaining it, `image(g, input) = output`,
the result is invariant under a random symmetry of the input and idempotent. 0 failures. Two seeded bugs
detected. The fallback at l.163-167 (more than 216 tying pairs) is reached by the full board and is correct.
The reference geometry (148 lines, 1152 maps as a set, closed under composition, busy cells, `cmp`) equals
`geom.hpp`.

## Findings

| # | severity | location | what is wrong | recommended fix |
|---|---|---|---|---|
| 1 | minor | `src/search.cpp:62-115` (table), `677-708` (emit) | The table identifies a state by 64+48 hash bits only (type and t are hashed, not stored). A collision would make a state inherit WON, WON-LEAF or a move from another state. The emit walk does not re-test LEAF, LOST or t < R_max, so it would write such a state out; only `--selfcheck` or `bin/treecheck` would notice. Probability about n²/2¹¹³ ≈ 10⁻²² for 10⁶ entries. No data race found that can raise a status wrongly. | Treat `treecheck` (not `stats.json: proved`) as the result. Optionally re-run `leaf_holds`/`lostCheck` in `emitP`, or store the full canonical key. |
| 2 | minor | `src/search.cpp:137-141` (`leafScreen`), `509-528` (`potentialOK`) | The comment says the screen is necessary for LEAF "whenever some F-connected component has >= 3 lines", the code applies it whenever >= 3 lines are live. Reproducer: B = all cells except 0-5, 42-47, 84-89, F = -, k = 198: `bin/phicheck` says LEAF true, the screen (12 < 198) says no. Completeness only (WON → FAILED); impossible for t <= 14. | Fix the comment or test `triple_centre`-like connectivity. |
| 3 | minor | `src/search.cpp:163-258`, `results/final/stats.json` | For R_max = 10 LOST can never fit (>= 11 stones needed, t+1 <= 10), so the proof run exercises `lostCheck` only on its screen; `lost_true: 0` is not a test. | State this in REPORT.md; keep `--lost-selftest` as the evidence. |
| 4 | minor | `src/phi.hpp:233` | `leaf_holds` returns true for k < |F| (theta < 0) when conditions 1, 2 hold; SPEC 3 defines LEAF for k >= |F| only. Not reachable from the search (|F| <= t is an invariant). | Reject or document. |
| 5 | minor | `src/search.cpp:936` | `calloc` of an array of `std::atomic` without construction is formally undefined behaviour (works with g++ 13). | `new Slot[nslots]()`. |
| 6 | minor | `logs/search/NOTES.md:90`, `bin/search` | NOTES names `results/search/r10_nb_k4/` (older tree: 152,900 P-states, md5 3c7b1185...) as the deliverable; `results/final/` holds the tree of the tuned headers (153,135 P-states). `bin/search` was older than `phi.hpp` from 06:26 to 06:58. The tree depends on the `phi_check` witness (candidate order), so header changes change it. I rebuilt from the current sources and reproduced `results/final` byte for byte; the cache `results/search_cache/good{8,9}.txt` also equals a fresh computation. | Point NOTES/REPORT to `results/final` with its md5. |

## What the emit walk and `--selfcheck` catch

* Emit walk: a reachable state without a WON flag (it re-runs the search on it, else `EMIT ERROR`). It
  trusts the flags, so it does not catch a wrong flag (finding 1).
* `--selfcheck`: missing table entries, illegal moves, a listed leaf that is not LEAF (`phi_check`), a
  non-leaf at R_max, LOST (`lostNaive`), missing children (separate enumeration). It shares `canon_pair`,
  `admissible` and the `phi.hpp` core with the search, so it cannot catch a bug in those. `treecheck` and
  sections 4-6 above are the checks for that.

## Not verified

* `verify/treecheck.cpp`, `sim/sim.cpp`, `py/` and PROOF.md were not audited here (other reviewers).
* The subset-enumeration reference only reaches <= 20 live lines (large B, unlike search states). On
  full-size states the reference is a max-flow per triple, which shares the triple reduction of SPEC 3 with
  `phi.hpp` (validated against the subset enumeration on 15,535 small states). 29,661 + 1,100 full-size or
  mid-size instances were compared, not all 106,723 leaves.
* `lostCheck` was compared with my transcription only for budgets <= 6 below the planted size (4,711
  further comparisons skipped for cost); at the planted size budgets up to 15 were covered.
* Concurrency was examined by reading and by one 4-thread stress test, not by a model checker. The proof
  run was reproduced with 2 threads (identical output to the 4-thread run).
* Hash-collision behaviour is by analysis only.

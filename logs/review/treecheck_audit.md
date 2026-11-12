# Audit of `verify/treecheck.cpp` — adversarial code review, mutation tests, independent re-checks

Date: 2026-11-12. Role: adversarial code auditor / mutation tester for the independent verifier.
Scratch directory (all my code, logs and results): `logs/review/treecheck_mut/`.
`verify/treecheck.cpp` was not edited (sha256 `f4e2f34e5bd741ba…85c2f7` before and after the audit).
Everything below was run by me unless it says "read" or "not verified".

## 0. Verdict

**sound-with-minor-issues.**

I found no way in which `bin/treecheck --table … --leaves … --rmax 10 --exact --paranoid` can print `ok:true`
although the tree of SPEC §4 is not WON at the root. No fatal and no major finding. Eight minor findings
(documentation, diagnostics, robustness of the workflow around the program, blind spots of the program's own
tests). None of them affects the certificate `results/final/treecheck.json`.

Measured, in one place:

| check | result |
|---|---|
| `bin/treecheck` vs. a fresh compile of `verify/treecheck.cpp` with the Makefile flags | byte-identical (sha256 `ced94792…44be94`) |
| my re-run of the certifying command (2 threads) | ok, exit 0, 153,135 P / 93,164 Q / 106,723 leaves, 246,299 states; every non-timing key equal to `results/final/treecheck.json` |
| 66 seeded file / option mutations | 58 rejected (exit 1 or 2), 8 accepted; **none of the 8 removes or changes an obligation** (all 8 have counters identical to the baseline) |
| LEAF vs. my brute force over all subsets of live lines (≤ 16 resp. ≤ 20 live lines) | 53,755 evaluations on 8,920 distinct (B,F), reasons 0/1/2/3 = 31,409 / 5,312 / 1,236 / 15,798, tight cases θ = min sur: 5,970, θ = min sur + 1: 5,970; **0 mismatches, 0 bad certificates** |
| LOST vs. my literal E2/E3 transcription | 800 random admissible states, t = 9..13 (E2 fits 329, only E3 fits 23, false 448); **0 mismatches**, all 352 reported patterns re-verified literally |
| my independent structural re-expansion of the final tree (own geometry, group, brute-force canonical form, literal children) | **all counters equal to treecheck's, also by t**: P, Q, leaves, threat children 143,498, danger children 28,275, VOID children 0 / 0 / 151 |
| my independent flow reference for LEAF on the real leaves | all 106,723 leaves LEAF (θ = 0, 1: exact reformulation; θ ≥ 2: every F-connected triple, no pruning) — see §3.4 for the split and for what "independent" means here; 1,500 perturbed instances: same reason code as treecheck on all |
| 12 source mutants (one defect each) | `--selftest --quick` catches 12 / 12; on the final tree 1 changes the verdict, 7 change the headline counters, **4 change neither verdict nor headline counters** (2 LEAF defects, LOST always false, a stricter LEAF) — finding 7 |

CPU used for the experiments: about 24 CPU-minutes (budget 25), all under `nice -n 10`, at most one
2-thread job plus one single-thread job at a time.

## 1. Object of the audit

* `verify/treecheck.cpp`, 4,012 lines, sha256 `f4e2f34e5bd741bae5d31790a1f1af458cb4c74bc91df87ce4855a2aa885c2f7`.
* `bin/treecheck` (built 06:59:13, source last modified 06:57:02): I compiled the source again into
  `logs/review/treecheck_mut/tc_orig` with `g++ 13.3.0 -O2 -std=c++17 -march=native -fopenmp -Wall
  -Wno-unused-result`; `cmp` reports the two binaries identical. All my runs use `tc_orig`.
* Inputs: `results/final/strategy.tbl` (93,164 lines; sha256 `13629393…9ce3fc`), `results/final/leaves.txt`
  (106,723 lines; sha256 `3c14c781…0fd470`).
* Baseline: `tc_orig --table … --leaves … --rmax 10 --exact --paranoid --threads 2` → exit 0, `ok:true`,
  `complete:true`, 0 errors, p_states 153,135, q_states 93,164, leaves_checked 106,723, states_visited
  246,299, max_t 10, root `0 - -`, `root_void:false`, `exact_visited_set:true`, `paranoid_flows:true`,
  8.6 CPU-s. All keys except threads / timing are equal to `results/final/treecheck.json` (which was made with
  3 threads), including `leaf_bfs` = 8,549,521.

The reasoning principle used throughout: **the two files are untrusted hints.** For every possible content
of the files, `ok:true` must imply that the root is WON. So a defect is dangerous only if it (1) drops a
child, (2) calls a non-VOID child VOID, (3) says LEAF = true wrongly, (4) says LOST = false wrongly, (5)
accepts an illegal table move, (6) identifies two different states in the visited set, (7) leaves a queued
state unprocessed, or (8) reports ok without (1)–(7) having been established. Parsing problems, wrong
canonical forms, hash collisions in the look-ups, truncated files and the like can only turn a valid proof
into a rejected one. The audit was organised around (1)–(8).

## 2. Part 1 — reading audit

### 2a. Geometry, the 1152 maps, canonical form, loading

* **Lines** (`initGeometry`, 258–331): all (f1,f2,f3), fi ∈ {constant 0..5, t, 5−t}, not all constant; the
  sorted cell lists go into a map; `CHECK` 148 distinct, 108/36/4 by the number of varying coordinates;
  two cells on at most one line; two lines share at most one cell (checked twice, through the cell lists
  and through set intersections). Matches SPEC §1 literally.
* **Busy cells** (332–347): the SPEC formula, 24 cells on 7 lines, every other cell on 3 or 4. Busy cells
  play no role in the verification (only in the counter `nonbusy_moves` and in `--busy-only`).
* **Maps** (348–411): 6 coordinate permutations × 8 reversal masks × 48 relabellings, applied in the order of
  SPEC §1; `CHECK` 1152 distinct maps, each a bijection of the cells that sends every line onto a line. The
  program aborts if any of this fails, at every start.
* **Canonical form** (`canonState`, 473–535): candidate maps are restricted through precomputed orbit minima;
  whatever the search does, the result is `imageList(gb, B)`, `imageList(gb, F)` for **one** map `gb`
  (531–533). A wrong (non-minimal) result could therefore only cause missing look-ups or duplicate work,
  never the identification of two states from different orbits.
* **Independent confirmation**: my own geometry (lines as arithmetic progressions, group as the closure of six
  generators; `ageom.py`, `aexpand.cpp`) gives 148 lines and 1152 maps; under my brute-force canonical form
  (all 1152 maps, SPEC §2.4 order) all 93,164 table lines and all 106,723 leaf lines are canonical, and the
  re-expansion with this canonical form visits exactly the same number of states per round (§3.4).
* **Loading** (`parseEntry` 1419–1443, `loadKeys` 1445–1515, `KeyStore` 1373–1409):
  * A line is ignored iff it is empty / white space or its first token starts with `#`. Otherwise it must
    have exactly 4 (table) or 3 (leaf) tokens, t decimal ≤ 60, strict cell lists (ids ≤ 215, no duplicate,
    any order), B∩F = ∅, |F| ≤ t, table: |B| = t−1, m ≤ 215, m ∉ B∪F; leaf: |B| = t. Anything else is a
    *bad line*, and one bad line makes `ok` false (2385). Tested: mutations 21–24, 36, 52, 57, 62, 65, 66.
  * Every entry is re-canonicalised with this program's canonical form, the move with the same map (1437–1441).
    **Keys in a non-canonical frame are accepted** (counted in `table_noncanonical` / `leaf_noncanonical`;
    finding 8). Tested: mutations 46, 47, 49, 50 (accepted, identical counters) and 48 (stale move: rejected).
  * **`--exact` does not make the look-ups exact.** The visited set stores full keys with `--exact`
    (1976–1986); the table and the leaf file are always looked up through 120 bits of the state hash
    (1382, 1403, 2162, 2218). SPEC §10.6 says otherwise (finding 1). This is harmless for soundness: a move
    found through a collision is still checked for legality (2222) and its children are verified, and a state
    taken for a listed leaf through a collision must still pass LEAF (2164). For the certified run there is
    direct evidence that no collision played a role: entries = lines = visited Q-states = 93,164 and
    entries = lines = checked leaves = 106,723, and my re-expansion, which uses exact keys throughout, uses
    exactly 93,164 table entries and 106,723 leaves.
  * **Duplicate keys with different moves**: after sorting, the entry with the numerically smallest canonical
    move is kept, whatever the order in the file; `table_conflicts` is counted but does not enter `ok`
    (1379–1390, 2385; finding 2). Sound (the move that is used is verified) — tested: mutations 41, 42, 45
    (the smaller new move is used and fails), 43, 44 (the original, smaller move is used; accepted, conflicts = 1).
  * Identical duplicates are only counted (tested: 3 + 3 duplicated lines, ok, same counters).
  * **A leaf that is also a table key** cannot exist: the kind (P/Q) and t are hashed into the key (559–560)
    and are part of the exact visited key (1982), and |B| = t for a leaf but t−1 for a table key. Writing a
    table key into the leaf file gives a bad line → `ok:false` with 0 errors (mutation 36).
  * Read errors are not distinguished from end of file (1456; finding 4).

### 2b. P-state processing (`process`, 2151–2206; `genChildrenP`, 1307–1366; `pushChild`, 2119–2137)

Order of the tests for a dequeued P-state (B,F,t): size / disjointness (2153) → own VOID test, an error if
the queued state is VOID (2157) → leaf look-up (2162): listed ⇒ `LeafChecker::check(B,F,t)` recomputed, error
if false (2164–2179), no expansion → otherwise `t >= rmax` is an error (2183) → `lostCheck` true is an error
(2188) → children. Every early `return` in `process` except "listed leaf and LEAF holds" is preceded by
`addError`.

Children, compared with SPEC §4 clause by clause:

* (a) `emit(CH_A, B, F)` unconditionally (1317).
* (c) every line with no cell in B (1313, 1320), every e ∈ l with e ∉ F (1324), child (B∪{e}, F∪(l∖{e}))
  iff |F∪(l∖{e})| ≤ t+1 (1327). The early `continue` at 1321 (`5 − |F∩l| > t+1−|F|`) is not a heuristic: for
  e ∉ F, |F∪(l∖{e})| = |F| + 5 − |F∩l| exactly.
* (d) every cell c ∉ B∪F (1336), every unordered pair of live lines through c (1337–1342; two lines share at
  most one cell, so every pair of live lines meeting in a cell outside F occurs exactly once), all 5 × 5
  choices of the cell left out of a∖{c} resp. b∖{c}, the left-out cell not in F (⇔ F∩a ⊆ Qa since c ∉ F),
  child iff |F∪Qa∪Qb| ≤ t+1 (1357). The early `continue` at 1343 is again an identity:
  |F∪Qa∪Qb| = |F| + (4−|F∩a|) + (4−|F∩b|) because Qa∩Qb = ∅ and F∩a ⊆ Qa.
* No line class is treated specially (the loops run over all 148 lines; axis, face-diagonal and
  space-diagonal lines are not distinguished anywhere in sections 5–8 and 11 of the file), no pruning by
  symmetry (symmetric children are generated and merged later by the visited set), no pruning by cost, no
  special case for busy cells, lines with stones of F are not skipped, the stone bound is t+1 as in SPEC.
* The only place where a generated child is dropped is `pushChild` 2122–2125: `!admissible(Bc, Fc)`, with the
  child's own B (the stone e resp. c already added). `admissible` (602–617) is the definition of SPEC §3: B∩X = ∅,
  no live line with more than 4 cells of X, no two live lines with exactly 4 cells of X sharing a cell
  outside X (two such lines share a cell outside X iff their 2-cell complements meet; `seen` accumulates
  these complements).

Independent confirmation on the real tree (§3.4): my literal generator (all live lines; all unordered pairs of
live lines; all 4-subsets) and my literal admissibility test, run over the same tree, produce 143,498
non-VOID threat children, 28,275 non-VOID danger children, 0 VOID threat children, 0 VOID danger children and
151 VOID reveal children — equal to treecheck's counters in total and in every round. So in the certified run
no child of type (c) or (d) was dropped at all.

### 2c. Q-state processing (2207–2248)

Size test (2209), own VOID test (2213), table look-up — a missing entry is an error (2218), the move must be a
cell 0..215 outside B and F (2222; a move inside B∪F cannot even be loaded), child (i) P(B∪{m},F,t) is
pushed always and it is an *error* if this program finds it VOID (2237), child (ii) Q(B,F∪{m},t) is pushed
exactly when |F| < t (2243) and dropped only by the VOID test in `pushChild`. Matches SPEC §4.

Well-foundedness (needed for "all local conditions hold ⇒ WON"): P(t) → Q(t+1), P(t+1); Q(t) → P(t),
Q(t) with one more revealed stone. No cycle.

### 2d. LEAF (`LeafChecker`, 690–1074)

What a `true` verdict rests on (with `--paranoid`, which the certifying run uses):

1. `admissible(B,F)` (819).
2. A pairing is built and then **validated literally** (864, `validateFlow` 782–812: every owned cell lies on
   its owner, the owner is live, the cell is not in F, every live line owns exactly 2 cells; `owner[]` is an
   array, so no cell is owned twice).
3. θ = k − |F| ≤ 0 → true (867). Correct: with a pairing every S has sur_F(S) ≥ 0 ≥ θ.
4. θ ≥ 1: a line is declared *strong* only after a flow with gain θ from that line has been constructed and
   validated literally (964–966, 971–972); a pair only after a validated flow (1017–1019); every F-connected
   triple of non-strong lines without a strong pair needs a validated flow of gain θ (1052–1067), otherwise
   the verdict is false.
5. Fewer than 3 non-strong lines → true (998): every F-connected S with |S| ≥ 3 then contains a strong line.

Checks I made on this argument:

* **Completeness of the triple enumeration** (1028–1070). Every F-connected S with |S| ≥ 3 contains a line b
  with two F-neighbours a, c in S (a spanning tree of a connected graph with ≥ 3 vertices has a vertex of
  degree ≥ 2). The loop takes every weak b, collects every weak line a that shares a cell outside F with b
  and whose pair with b is not strong (1033–1041; `inF[c]` is the only filter, two lines share one cell so no
  line is listed twice), and tests every pair of such neighbours. The only triples skipped are (i) those
  containing a strong line or a strong pair — the two pruning rules of SPEC §3 — and (ii) triangles at a
  centre that is not their smallest line (1047), which are tested at the smallest line (there all three
  pairs are checked for strength, 1038 and 1048). F-adjacency is exactly "share a cell not in F".
* **"A strong line is removed from the network" (629–642).** I tried to refute it and could not; it is exact.
  Let D be the set of lines already deleted when T (a line, a pair or a triple) is tested, and let S be any
  set of live lines with T ⊆ S. If S avoids D, the validated flow for T relative to D gives the lines of S at
  least 2|S| + θ distinct cells of N(S)∖F.
  If S meets D, let d be the *first* deleted line in S: at the moment d was tested, no line of S had been
  deleted yet, so the validated flow from {d} (in which every other undeleted line, in particular every other
  line of S, owns exactly 2 cells) gives sur_F(S) ≥ θ. Hence the min over *all* S ⊇ T is ≥ θ. This uses only
  facts that `validateFlow` checks (deleted lines own nothing: 803; other lines exactly 2: 808; sources ≥ 2
  each and 2|T| + q in total: 805, 811). The comment's sentence "deleting strong lines never changes which
  of the remaining lines / pairs / triples have such flows" is true for the same reason (a set that contains
  a strong line has surplus ≥ θ anyway, so restricting the min-cut to the sets avoiding D does not change
  whether it is ≥ θ), but the `true` verdicts need only the direction shown above, and the `false` verdicts
  do not depend on it at all, because each of them comes with a set S that is re-evaluated from the
  definition (`CHECK(sur == got && sur < theta)`, `isFConnected`, 1056–1058; quick rejection
  913–914; Hall violator 853). Experiment: 5,970 brute-force instances with θ exactly one above the true
  minimum surplus (the cases in which a wrong shortcut would say `true`) — all answered `false`.
* The θ = 1 shortcut "reaching ⇒ strong" (961) relies on a reachability set computed before any deletion.
  Reaching is preserved under deletion (a path that used a cell of a deleted line now ends there), and with
  `--paranoid` the flow is constructed and validated anyway (964–966), so the certifying run does not rely on
  the shortcut.
* Fewer than 3 live lines / no live line: pairing still required (condition 2), then true. Tested by hand
  (216 SP stones; one live line with F = ∅, with 2, with 4 and with 5 of its cells in F) and compared with a
  literal Python evaluation: identical.
* State reuse between calls: `owner`, `inF`, `isLive`, `deleted`, `weak` (for live lines), `wpair` are
  re-initialised; the undo log is empty at every return (each `flow` is followed by `rollback`).

### 2e. LOST (1102–1296)

I compared `lostE2` / `lostE3` with SPEC §4 condition by condition: distinct live lines, x/e/c (resp. x/c/c')
pairwise distinct, x ∉ b (∪ b'), e ∉ b, e, c (c') ∉ F, x ∈ Sa (∩ Sa'), c and c' in no stone set, F∩line ⊆
stone set (encoded as "the left-out cell is not in F"), fit ⇔ |F ∪ stone sets| ≤ t+1. All present. The
screens are valid lower bounds: E2 needs at least (5−f_l)+(4−f_a)+(4−f_b)−2 new stones (Sl∩Sa = {x},
|Sl∩Sb| ≤ 1, Sa∩Sb = ∅), E3 at least Σ(4−f) − 4, and replacing unknown f by the largest values over the live
lines (`top[]`, 1112–1113 keeps the four largest) only weakens the bound. Experiment §3.2: 0 mismatches.

For the certified tree LOST is not load-bearing: an E2 pattern has ≥ 11 and an E3 pattern ≥ 12 distinct
stones, an expanded state has t ≤ 9, i.e. t+1 ≤ 10. I checked the two inequalities by hand and ran my literal
E2/E3 on every 20th expanded P-state of the final tree (2,320 states): never fits.

### 2f. Engine (1941–2388) and `main` (3864–4012)

* **Visited set**: with `--exact` the key is (B, F, t, kind) in full (1976–1986). Each shard has its own mutex.
* **Queue**: per-worker stacks plus a shared stack under one mutex. A worker counts as idle only inside the
  wait loop, with an empty local stack and an empty shared stack; `done` is set when all workers are idle
  (2262) or when `stop` is set (2282–2287).
* **Marked visited but never processed?** A state is inserted into the visited set exactly when it is
  queued (2128–2135, root 2321–2327). After the workers have joined, `complete` requires: shared stack empty,
  every local stack empty, `stop` not set (2356–2362, 2381); then `CHECK(visited == p_states + q_states)`
  (2384) — and the two counters are incremented at the very beginning of `process`, before any early
  return. So with `complete:true` every queued state was processed exactly once. `stop` is set by
  `--max-errors` (2114) and by `--time-limit` (2341); both give `complete:false`. Tested: `rmax_9`
  (50 errors, complete false), `time_limit_tiny` (0 errors, complete false, exit 1).
* **A worker records an error**: the state is not expanded, the error counter is > 0, `ok` is false.
* **A thread throws / a `CHECK` fails**: `std::terminate` / `abort`; nothing is printed, exit by signal
  (seen in the source-mutant run `fadj_dropped`: exit −6).
* **Files fail to load**: `fopen` failure → message, exit 2, no JSON (mutation 58). A previous `--out`
  file is then left in place (finding 3).
* **ok** (2385): `complete && error_count == 0 && table bad lines == 0 && leaf bad lines == 0`. **Exit code**
  (4011): 0 iff ok. In all 65 mutation runs that printed a JSON, the `ok` in the `--out` file equals the
  `ok` on stdout and exit code 0 ⇔ ok; the 66th (missing input file) exits with 2 and prints nothing.
* Not part of `ok`: `table_conflicts` (finding 2). A VOID root given with `--root` yields `ok:true` with
  0 states and `root_void:true` (finding 6); the default root is not VOID.
* Thread safety: each worker has its own `LeafChecker`; `lostCheck`, `genChildrenP`, `canonState`,
  `admissible` use only locals and read-only tables; statistics are per worker. Runs with 2 threads (mine)
  and 3 threads (stored JSON) give identical counters, equal to my single-threaded independent re-expansion.

## 3. Part 2 — experiments

### 3.1 (g) Mutation tests on copies of the final files

Driver `mut.py` (seed 20261112, every mutation reproducible from its record in `mutations.jsonl`); command
per mutation: `tc_orig --table T --leaves L --rmax R --exact --paranoid --threads 2 --progress 0 --out …`.
"hole" = `no table entry (strategy hole)`, "t>=Rmax" = `non-leaf P-state with t >= R_max`, "not LEAF" =
`listed leaf is not LEAF`.

| # | mutation | what was changed | exit | ok | errors | complete | error kinds (first 50) / load counters | P / Q / leaves visited |
|---|---|---|---|---|---|---|---|---|
| 1 | `del_table_t01` | delete table line 76112: "1 - 0 43" | 1 | false | 1 | true | hole x1 | 126731 / 76112 / 89271 |
| 2 | `del_table_t02` | delete table line 2: "2 0 - 215" | 1 | false | 1 | true | hole x1 | 60232 / 37092 / 42214 |
| 3 | `del_table_t03` | delete table line 72783: "3 0,46 30,169,215 35" | 1 | false | 1 | true | hole x1 | 150157 / 91301 / 104629 |
| 4 | `del_table_t04` | delete table line 42430: "4 0,46,169 86,92,215 93" | 1 | false | 1 | true | hole x1 | 151530 / 92243 / 105718 |
| 5 | `del_table_t05` | delete table line 76027: "5 0,5,61,210 93,151,154,180,215 46" | 1 | false | 1 | true | hole x1 | 152963 / 93081 / 106588 |
| 6 | `del_table_t06` | delete table line 39517: "6 0,46,92,123,169 35,43,215 185" | 1 | false | 1 | true | hole x1 | 151155 / 92095 / 105288 |
| 7 | `del_table_t07` | delete table line 57135: "7 0,5,43,61,93,172 30,86,122,154,180,210,215 87" | 1 | false | 1 | true | hole x1 | 153129 / 93158 / 106720 |
| 8 | `del_table_t08` | delete table line 78537: "8 0,7,46,92,123,169,215 1,13,19,25,31,86,93,151 122" | 1 | false | 1 | true | hole x1 | 153134 / 93164 / 106723 |
| 9 | `del_table_t09` | delete table line 70108: "9 0,5,42,61,93,122,172,215 43,44,45,46,47,87,154,210 151" | 1 | false | 1 | true | hole x1 | 153134 / 93163 / 106722 |
| 10 | `del_table_t10` | delete table line 61314: "10 0,5,30,43,82,93,123,128,215 77,86,87,92,97,102,122,185,210 172" | 1 | false | 1 | true | hole x1 | 153133 / 93163 / 106721 |
| 11 | `move_busy_t02` | table line 65211 "2 0 46,215 64": move 64 -> 172 (busy) | 1 | false | 1 | true | hole x1 | 147197 / 89837 / 102460 |
| 12 | `move_busy_t05` | table line 16486 "5 0,46,169,215 86,87,92,93 129": move 129 -> 185 (busy) | 1 | false | 1 | true | hole x1 | 153134 / 93164 / 106723 |
| 13 | `move_busy_t07` | table line 76398 "7 0,5,43,92,123,210 36,72,87,108,144,172,180 215": move 215 -> 46 (busy) | 1 | false | 1 | true | hole x1 | 153131 / 93160 / 106721 |
| 14 | `move_busy_t09` | table line 51727 "9 0,5,43,92,93,122,170,215 26,46,62,98,123,134,206,210 172": move 172 -> 61 (busy) | 1 | false | 2 | true | hole x2 | 153135 / 93165 / 106722 |
| 15 | `move_busy_t10` | table line 58173 "10 0,5,30,43,46,86,93,180,215 122,151,172,181,182,183,184,185,210 129": move 129 -> 169 (busy) | 1 | false | 2 | true | t>=Rmax x1, hole x1 | 153135 / 93164 / 106722 |
| 16 | `move_nonbusy_t03` | table line 47661 "3 0,46 169 215": move 215 -> 189 (non-busy) | 1 | false | 2 | true | hole x2 | 126345 / 77005 / 87311 |
| 17 | `move_nonbusy_t06` | table line 1516 "6 0,11,87,128,215 47,61,83,119,155,191 64": move 64 -> 75 (non-busy) | 1 | false | 1 | true | hole x1 | 153136 / 93165 / 106723 |
| 18 | `move_nonbusy_t08` | table line 25473 "8 0,5,49,64,123,151,215 48,50,51,52,53,92,129,210 93": move 93 -> 42 (non-busy) | 1 | false | 1 | true | hole x1 | 153136 / 93165 / 106723 |
| 19 | `move_nonbusy_t09` | table line 67889 "9 0,5,43,52,92,123,172,215 17,86,87,122,157,180,192,210 93": move 93 -> 24 (non-busy) | 1 | false | 5 | true | t>=Rmax x3, hole x2 | 153137 / 93165 / 106721 |
| 20 | `move_nonbusy_t10` | table line 73000 "10 0,5,30,43,64,86,151,185,215 61,129,172,210,211,212,213,214 87": move 87 -> 21 (non-busy) | 1 | false | 2 | true | hole x1, t>=Rmax x1 | 153133 / 93163 / 106720 |
| 21 | `move_onto_B_t06` | table line 31213 "6 0,5,61,154,210 35,86,87,93,215 92": move 92 -> 61 (a cell of B) | 1 | false | 1 | true | hole x1; table bad 1 | 153067 / 93130 / 106670 |
| 22 | `move_onto_B_t09` | table line 82346 "9 0,5,43,64,71,92,123,210 30,41,47,53,59,65,151,172,215 154": move 154 -> 210 (a cell of B) | 1 | false | 1 | true | hole x1; table bad 1 | 153134 / 93164 / 106722 |
| 23 | `move_onto_F_t07` | table line 6 "7 0,1,46,92,169,215 7,13,19,25,31 123": move 123 -> 31 (a cell of F) | 1 | false | 1 | true | hole x1; table bad 1 | 153135 / 93164 / 106723 |
| 24 | `move_onto_F_t10` | table line 60619 "10 0,5,30,43,53,87,122,128,215 41,47,59,65,71,93,129,172,185,210 46": move 46 -> 172 (a cell of F) | 1 | false | 1 | true | hole x1; table bad 1 | 153134 / 93164 / 106722 |
| 25 | `del_leaf_t08_1` | delete leaf line 41841: "8 0,5,44,61,93,122,154,210 8,80,116,152,169,172,188" | 1 | false | 1 | true | hole x1 | 153135 / 93165 / 106722 |
| 26 | `del_leaf_t08_2` | delete leaf line 98693: "8 0,5,43,61,93,122,154,215 42,44,45,46,47,180,185" | 1 | false | 1 | true | hole x1 | 153135 / 93165 / 106722 |
| 27 | `del_leaf_t09_1` | delete leaf line 73158: "9 0,5,43,64,92,93,122,196,210 16,35,52,88,123,124,160,172,215" | 1 | false | 1 | true | hole x1 | 153135 / 93165 / 106722 |
| 28 | `del_leaf_t09_2` | delete leaf line 74117: "9 0,5,31,43,92,93,122,154,215 61,67,103,123,129,139,175,210,211" | 1 | false | 1 | true | hole x1 | 153135 / 93165 / 106722 |
| 29 | `del_leaf_t10_1` | delete leaf line 63782: "10 0,1,5,30,64,87,92,151,172,215 43,85,86,123,127,169,180,185,210,211" | 1 | false | 1 | true | t>=Rmax x1 | 153135 / 93164 / 106722 |
| 30 | `del_leaf_t10_2` | delete leaf line 69793: "10 0,1,35,43,46,87,154,169,210,215 5,37,61,73,109,145,172,180,181,185" | 1 | false | 1 | true | t>=Rmax x1 | 153135 / 93164 / 106722 |
| 31 | `bogus_leaf_t04` | append leaf line "4 0,35,46,169 30,180,210,215" = child (i) of table line 88443, an expanded non-leaf P-state | 1 | false | 1 | true | not LEAF x1 | 152981 / 93079 / 106614 |
| 32 | `bogus_leaf_t06` | append leaf line "6 0,35,43,87,128,172 5,92,93,180,185,215" = child (i) of table line 47524, an expanded non-leaf P-state | 1 | false | 1 | true | not LEAF x1 | 153129 / 93157 / 106720 |
| 33 | `bogus_leaf_t08` | append leaf line "8 0,7,35,46,92,123,169,185 30,43,79,115,151,180,187,215" = child (i) of table line 1954, an expanded non-leaf P-state | 1 | false | 1 | true | not LEAF x1 | 153135 / 93164 / 106723 |
| 34 | `bogus_leaf_t09` | append leaf line "9 0,5,30,43,64,86,151,184,215 46,172,180,181,182,183,185,210" = child (i) of table line 75645, an expanded non-leaf P-state | 1 | false | 1 | true | not LEAF x1 | 153132 / 93161 / 106720 |
| 35 | `bogus_leaf_root` | append leaf line "0 - -" (the root itself) | 1 | false | 1 | true | not LEAF x1 | 1 / 0 / 0 |
| 36 | `qkey_as_leaf_same_t` | append leaf line "9 0,5,43,71,87,128,172,210 35,61,107,143,179,180,185,215" (key of table line 15551, \|B\| = t-1) | 1 | false | 0 | true | leaf bad 1 | 153135 / 93164 / 106723 |
| 37 | `qkey_as_leaf_t_minus_1` | append leaf line "7 0,1,61,93,122,154,215 35,37,73,109,145,181" (Q key of table line 34994 with t-1, i.e. the P-state P(B,F,t-1)) | 1 | false | 1 | true | not LEAF x1 | 153135 / 93163 / 106723 |
| 38 | `rmax_9` | unchanged files, `--rmax 9` | 1 | false | 50 | false | t>=Rmax x50 (stopped by --max-errors) | 474 / 254 / 314 |
| 39 | `rmax_8` | unchanged files, `--rmax 8` | 1 | false | 50 | false | t>=Rmax x50 (stopped by --max-errors) | 71 / 49 / 0 |
| 40 | `rmax_11` | unchanged files, `--rmax 11` (weaker bound; must still be accepted) | 0 | **true** | 0 | true | - | 153135 / 93164 / 106723 (= baseline) |
| 41 | `conflict_smaller_t08` | append table line "8 0,7,46,92,123,169,215 35,43,79,93,115,151,187 36" (same key as line 43737, original move 64; new move smaller) | 1 | false | 5 | true | hole x5; conflicts 1 | 153139 / 93169 / 106723 |
| 42 | `conflict_smaller_t09` | append table line "9 0,1,61,64,87,128,154,215 5,37,73,109,145,151,181 24" (same key as line 3866, original move 30; new move smaller) | 1 | false | 8 | true | t>=Rmax x6, hole x2; conflicts 1 | 153141 / 93166 / 106722 |
| 43 | `conflict_larger_t08` | append table line "8 0,8,46,61,93,122,215 44,80,116,152,154,185,188 192" (same key as line 37996, original move 169; new move larger) | 0 | **true** | 0 | true | conflicts 1 | 153135 / 93164 / 106723 (= baseline) |
| 44 | `conflict_larger_t10` | append table line "10 0,5,30,43,87,93,128,198,215 86,122,185,199,200,201,202,203,210 181" (same key as line 61051, original move 172; new move larger) | 0 | **true** | 0 | true | conflicts 1 | 153135 / 93164 / 106723 (= baseline) |
| 45 | `conflict_prepend_smaller_t07` | PREPEND table line "7 0,5,30,43,172,215 46,86,87,93,180,185,210 62" (same key as line 72443, original move 92) | 1 | false | 1 | true | hole x1; conflicts 1 | 153129 / 93158 / 106719 |
| 46 | `noncanon_table_t08_mapped` | table line 36304 rewritten in another frame, move mapped with the same map | 0 | **true** | 0 | true | table noncanon 1 | 153135 / 93164 / 106723 (= baseline) |
| 47 | `noncanon_table_t10_mapped` | table line 60855 rewritten in another frame, move mapped with the same map | 0 | **true** | 0 | true | table noncanon 1 | 153135 / 93164 / 106723 (= baseline) |
| 48 | `noncanon_table_t09_stale_move` | table line 7994 rewritten in another frame, move NOT mapped (effective canonical move 151 instead of 185) | 1 | false | 50 | false | t>=Rmax x49, hole x1; table noncanon 1 | 145488 / 87921 / 101682 |
| 49 | `noncanon_leaf_t09` | leaf line 50026 rewritten in another frame | 0 | **true** | 0 | true | leaf noncanon 1 | 153135 / 93164 / 106723 (= baseline) |
| 50 | `all_noncanon_descending_crlf` | every table and leaf line mapped by its own random group element, descending cell lists, lines shuffled, CRLF | 0 | **true** | 0 | true | table noncanon 93041; leaf noncanon 106591 | 153135 / 93164 / 106723 (= baseline) |
| 51 | `truncated_table_mid_line` | table cut after 2688252 of 5376504 bytes (inside a line) | 1 | false | 42 | true | hole x42; table bad 1 | 79075 / 47745 / 54892 |
| 52 | `truncated_table_last_line_short` | table cut inside its last line so that a 3-token line without newline remains | 1 | false | 1 | true | hole x1; table bad 1 | 153134 / 93164 / 106722 |
| 53 | `truncated_leaves_mid_line` | leaf file cut after 4296681 of 6445022 bytes (inside a line) | 1 | false | 50 | false | t>=Rmax x41, hole x9 | 64 / 38 / 0 |
| 54 | `truncated_leaves_at_line_end` | leaf file: last 1000 lines removed (clean cut) | 1 | false | 50 | false | t>=Rmax x41, hole x9 | 64 / 38 / 0 |
| 55 | `empty_leaves` | empty leaf file | 1 | false | 50 | false | t>=Rmax x41, hole x9 | 64 / 38 / 0 |
| 56 | `empty_table` | empty table | 1 | false | 1 | true | hole x1 | 1 / 1 / 0 |
| 57 | `swapped_files` | `--table leaves.txt --leaves strategy.tbl` | 1 | false | 1 | true | hole x1; table bad 106723; leaf bad 93164 | 1 / 1 / 0 |
| 58 | `missing_table_file` | `--table` points to a file that does not exist | 2 | - | - | - | no JSON printed, `--out` not written | - |
| 59 | `table_is_directory` | `--table` points to a directory | 1 | false | 1 | true | hole x1 (directory read as an empty table) | 1 / 1 / 0 |
| 60 | `time_limit_tiny` | unchanged files, `--time-limit 0.5` | 1 | false | 0 | false | timed_out | 6044 / 3201 / 4590 |
| 61 | `comment_out_table_line` | table line 83166 turned into a `#` comment | 1 | false | 1 | true | hole x1 | 153134 / 93164 / 106722 |
| 62 | `table_wrong_t` | table line 41544: t 9 -> 10 (\|B\| != t-1) | 1 | false | 1 | true | hole x1; table bad 1 | 153135 / 93163 / 106723 |
| 63 | `leaf_F_shrunk` | leaf line 91814: one revealed stone removed from F | 1 | false | 1 | true | hole x1 | 153135 / 93165 / 106722 |
| 64 | `extra_unreachable_garbage` | 2 + 2 extra well-formed lines for states that are not reachable; removes no obligation | 0 | **true** | 0 | true | table noncanon 2; leaf noncanon 2 | 153135 / 93164 / 106723 (= baseline) |
| 65 | `bom_prefix` | UTF-8 BOM in front of the table (first line becomes malformed) | 1 | false | 1 | true | hole x1; table bad 1 | 1 / 1 / 0 |
| 66 | `nul_byte_in_leaf_line` | NUL byte inserted into leaf line 29 | 1 | false | 1 | true | t>=Rmax x1; leaf bad 1 | 153135 / 93164 / 106722 |

Reading of the table:

* All 10 deleted table lines (t = 1..10), all 10 replaced moves (5 busy, 5 non-busy), all 4 moves onto B / F,
  all 6 deleted leaves (t = 8, 9, 10), all 5 bogus leaves, both Q-keys written as leaves, `--rmax 9` and
  `--rmax 8`, the three "smaller" conflicting duplicates, the stale-move frame change, all truncations, the
  empty files and the swap are **rejected** with exit code 1 (exit 2 for the missing file).
* The 8 accepted runs are: `--rmax 11`; two conflicting duplicates whose move is larger than the original (the
  original is used; reported as `table_conflicts: 1`); three single lines and then all lines rewritten in
  other frames with the move mapped; four extra lines for unreachable states. In all 8 the visited states,
  the leaves and all child counters are identical to the baseline, i.e. the same tree was verified. **No
  mutation that removes or changes a reachable obligation was accepted**, so the question "is the mutated
  tree still a valid proof" did not arise. (For the replaced moves I had prepared an independent check of the
  two children of the mutated Q-state with my own canonical form; it is recorded as `auditor_analysis` in
  `mutations.jsonl`: in all 10 cases child (i) is not a listed leaf, consistent with the rejections.)
* Rows where the counters equal the baseline although the run is rejected (23, 33, 36): the state whose
  entry was damaged is still counted, and its children are reachable through other parents.
* Non-canonical keys: **accepted** (re-canonicalised), not rejected.

### 3.2 (h) Brute-force checks of `--leafcheck` and `--lostcheck`

My code: `abrute.cpp` (own geometry, `std::bitset`, no code from the project). LEAF by the definition of
SPEC §3: literal admissibility; Hall's condition over **all** subsets of live lines; min of sur_F over all
F-connected subsets of size ≥ 3 (depth-first enumeration of all 2^n subsets). Instance generators: random
connected clusters of 3–14 lines, coordinate planes, diagonal planes, the star of a busy cell, two disjoint
clusters, cluster ∪ plane; B = complement (plus 0–3 cells inside); F in eight flavours (empty, small, large,
around the Hall threshold, only crossing cells — to cut F-adjacency —, 3–5 stones on one or two lines,
private cells, half of the region). Every instance is evaluated for several k.

| batch | seed | instances (distinct (B,F)) | live lines | evaluations | reasons 0 / 1 / 2 / 3 | θ = min sur (true) | θ = min sur + 1 (false) | mismatches | bad certificates |
|---|---|---|---|---|---|---|---|---|---|
| A (`--paranoid`, the default) | 20261112 | 6,000 (5,691) | 1..16 | 25,952 | 14,750 / 2,980 / 660 / 7,562 | 3,781 | 3,781 | 0 | 0 |
| A again with `--fast` | 20261112 | same | same | 25,952 | same | 3,781 | 3,781 | 0 | 0 |
| B (every θ from 0 to min sur + 4) | 777001 | 3,000 (2,835) | 1..16 | 26,040 | 15,654 / 2,154 / 516 / 7,716 | 1,929 | 1,929 | 0 | 0 |
| C | 990017 | 400 (394) | 1..20 | 1,763 | 1,005 / 178 / 60 / 520 | 260 | 260 | 0 | 0 |

"Mismatch" = `leaf` or `reason` differs from the brute force. "Bad certificate" = the set S printed with
`--verbose` for reason 2 / 3 fails my Python re-evaluation (lines live, distinct, sur < 0 resp. F-connected,
≥ 3 lines, sur equal to the printed value and < θ). In batch A treecheck ran 22,101 single-line, 94,758 pair
and 210,627 triple flows, so the strong-line / strong-pair / triple machinery was exercised, not only the
quick rejection.

LOST: `abrute lost 20261112 800` — literal transcription of E2 and E3 (all triples / quadruples of live
lines, all 4-subsets of a line as stone sets; the only pruning is that a partial union already larger than
t+1 cannot shrink). 800 states with |B| = t, |F| ≤ t, B∩F = ∅, F B-admissible (asserted again in Python),
t = 9..13 (86 / 129 / 160 / 221 / 204), generated by six generators (uniform; stones on chains of 3 or 4
lines; crossing pairs; seeded E2 and E3 patterns with r stones removed and the number of available new stones
r−1, r or r+1). Result: E2 fits 329, only E3 fits 23, LOST false 448 (all 86 states with t = 9 false).
`treecheck --lostcheck`: **0 mismatches** in `lost` and in the pattern type; each of the 352 reported patterns
satisfies every side condition of SPEC §4 in my Python re-evaluation (`cmp_lost.py`).

### 3.3 (i) Source mutation

`srcmut.py` writes copies of the source with one defect each (`src_mut/<name>.cpp`, diffs in
`src_mut/<name>.diff`), compiles them and runs `--selftest --quick` and the certifying command on the final
tree. Unmutated program: selftest quick passes (0 failures, 20.7 CPU-s). "FAIL lines" counts all output
lines containing `[FAIL]`, including the one summary line per failed section.

| mutant (one defect each) | `--selftest --quick` | final tree: ok / exit | counters that differ from the baseline run |
|---|---|---|---|
| `skip_danger`: child (d) is never generated | **caught**: exit 1, 10 FAIL lines in "children", "tree expansion" | true / 0 | p_states 153135 -> 144832; q_states 93164 -> 89062; leaves_checked 106723 -> 100471; states_visited 246299 -> 233894; danger_children 28275 -> 0 |
| `skip_ii`: child (ii) of a Q-state is never generated | **caught**: exit 1, 27 FAIL lines in "tree expansion" | true / 0 | p_states -> 6110; q_states -> 2193; leaves_checked -> 3917; states_visited -> 8303; threat_children -> 8170; danger_children -> 16250; void_skipped 151 -> 0 |
| `ii_offbyone`: child (ii) only when \|F\| < t−1 | **caught**: exit 1, 20 FAIL lines in "tree expansion" | true / 0 | p_states -> 77981; q_states -> 32569; leaves_checked -> 51447; states_visited -> 110550; threat_children -> 96881; danger_children -> 27468; void_skipped -> 33 |
| `leaf_norecompute`: a listed leaf is accepted without recomputing LEAF | **caught**: exit 1, 3 FAIL lines in "tree expansion" | true / 0 | **none of the headline counters**; only the work counters `leaf_bfs`, `leaf_triple_flows` 7391 -> 0 |
| `stone_t2`: (c),(d) with \|F ∪ …\| ≤ t+2 (too many children) | **caught**: exit 1, 19 FAIL lines in "children", "tree expansion" | **false** / 1 | 50 errors "state violates \|B\| = t, \|F\| <= t", run stopped |
| `stone_t0`: (c),(d) with \|F ∪ …\| ≤ t (children silently dropped) | **caught**: exit 1, 14 FAIL lines in "children", "tree expansion" | true / 0 | p_states -> 41286; q_states -> 29776; leaves_checked -> 29678; states_visited -> 71062; threat_children -> 33949; danger_children -> 245; void_skipped -> 145 |
| `no_facediag_threat`: (c) ignores the 36 face diagonals | **caught**: exit 1, 13 FAIL lines in "children", "tree expansion" | true / 0 | p_states -> 118426; q_states -> 73565; leaves_checked -> 81797; states_visited -> 191991; threat_children -> 106818; void_skipped -> 139 |
| `no_spacediag_threat`: (c) ignores the 4 space diagonals | **caught**: exit 1, 4 FAIL lines in "children" only | true / 0 | p_states -> 152032; q_states -> 92432; leaves_checked -> 105996; states_visited -> 244464; threat_children -> 141850 |
| `lost_false`: LOST always false | **caught**: exit 1, 7 FAIL lines in "LOST", "tree expansion" | true / 0 | **none at all** |
| `fadj_dropped`: LEAF treats lines sharing any cell (also a cell of F) as adjacent in the pair / triple enumeration (a *stricter* LEAF) | **caught** by an abort (`CHECK failed at line 1058: S.size() >= 3 && isFConnected(S, F)`), no FAIL line | true / 0 | **none of the headline counters**; `leaf_triple_flows` 7391 -> 9478 |
| `adm_overvoid`: two live lines with 4 stones sharing a *stone* also count as a danger pair (too many VOID states) | **caught**: exit 1, 4 FAIL lines in "admissibility", "LEAF vs exhaustive subsets" | true / 0 | p_states -> 153134; q_states -> 93163; leaves_checked -> 106722; states_visited -> 246297; void_skipped 151 -> 152 |
| `leaf_skip_triangles`: LEAF never tests triples of pairwise F-adjacent lines (a *weaker* LEAF) | **caught**: exit 1, 4 FAIL lines in "LEAF vs exhaustive subsets", "LEAF vs unpruned flows" | true / 0 | **none of the headline counters**; `leaf_triple_flows` 7391 -> 6884 |

Reading of the table:

* The self-test is sensitive to all 12 defects (one of them only through an abort, one only in one section).
* A checker with fewer obligations naturally still prints `ok:true` on a valid tree. What reveals such a
  defect in the real run is **only** the counters. For the seven child / VOID defects the headline counters
  move (for `adm_overvoid` by just 2 states out of 246,299), and they are pinned down by three independent
  re-expansions that report the same numbers: `bin/search` (`stats.json`: 153,135 / 93,164 / 106,723;
  143,498; 28,275 — read, not re-run), `verify/spotcheck.py` (read in PROGRESS; not re-run by me) and my own
  `aexpand` (§3.4, run by me, equal in every round).
* Three defects — two of them in the unsound direction — leave verdict *and* headline counters unchanged:
  `leaf_norecompute`, `leaf_skip_triangles`, `lost_false`. So the real run by itself says nothing about the
  quality of the LEAF and LOST code (finding 7). For LOST this is harmless at R_max = 10 (§2e). For LEAF the
  evidence is: the reading audit §2d, the brute force §3.2, the self-test, and the independent flow
  references (§3.4; plus `bin/phicheck` and `spotcheck.py`, read only).

### 3.4 Two additional independent checks (not in the task list, done because §3.3 shows where the real run is blind)

**Structural re-expansion** (`aexpand.cpp`, 72.8 CPU-s): own geometry, group generated from six generators,
canonical form by brute force over all 1152 maps with exact keys, literal children (a), (c), (d), (i), (ii),
literal admissibility, listed leaves taken as leaves (LEAF not recomputed here), literal E2/E3 on every 20th
expanded P-state. Result on the final files (`aexpand_final.json`):

| counter | treecheck (`treecheck.json`) | aexpand |
|---|---|---|
| errors | 0 | 0 |
| P-states by t (0..10) | 1, 2, 5, 14, 35, 181, 1718, 7227, 34801, 93124, 16027 | identical |
| Q-states by t | 0, 2, 5, 14, 35, 101, 446, 3487, 14362, 62381, 12331 | identical |
| leaves by t | 0,…,0, 3382, 87314, 16027 | identical |
| non-VOID threat children by t | 0,0,0,0,0, 824, 4075, 14460, 47020, 72352, 4767 (143,498) | identical |
| non-VOID danger children by t | 0,…,0, 20416, 6060, 1799 (28,275) | identical |
| VOID children by t (threat / danger / reveal totals) | 0,0,0,0,0,0, 1, 6, 54, 88, 2 (0 / 0 / 151) | identical (0 / 0 / 151) |
| table lines not canonical / entries used | 0 / (93,164 Q-states) | 0 / 93,164 |
| leaf lines not canonical / entries used | 0 / (106,723 leaves) | 0 / 106,723 |
| literal LOST on sampled expanded states | — | 2,320 states, 0 fit |

**Flow reference for LEAF on the real leaves** (`aleaf.cpp`): own geometry, explicit flow network (source →
line capacity 2, line → cell, cell → sink), BFS augmenting paths; μ(T) measured by an extra node X with
source → X of capacity θ and X → the three lines of T; for θ ≥ 2 **every** triple of live lines with at least
two F-adjacent pairs is tested, no pruning rule. For θ = 1 I used the exact reformulation "μ(T) ≥ 1 ⇔ some
line of T has a residual path to the sink" (one backward search), after confirming on 1,148 instances with
θ = 1 that it gives the same reason code as the unpruned loop.

| instances | count | my reference | treecheck `--leafcheck` |
|---|---|---|---|
| all leaves with θ = 0 (`checks/leaves_theta0.txt`) | 64,841 | all LEAF | all LEAF |
| all leaves with θ = 1 | 32,703 | all LEAF | all LEAF |
| all leaves with θ = 2, 3 (unpruned) | 7,267 + 1,434 | all LEAF (43,046,777 triples tested) | all LEAF (certifying run; `--leafcheck` not repeated on these) |
| all leaves with θ ≥ 4 | 478 | all LEAF | all LEAF |
| stratified sample incl. the above θ ≥ 4 (100 / 600 / 600 / 400 / 478 for θ = 0 / 1 / 2 / 3 / ≥4), unpruned | 2,178 | all LEAF (part of 16,062,694 triples tested) | all LEAF |
| random sample of `checks/perturbed.txt` | 1,500 | reasons 0/1/2/3 = 1,034 / 48 / 294 / 124 | same reason on all 1,500 |

(The nine `leaves_theta*.txt` files together are, as a set, exactly `results/final/leaves.txt`: checked.)
Limits of this check: it is my own code written during this audit and tested only against treecheck itself
and, for θ = 1, against its own unpruned mode; it shares the *idea* of augmenting paths with every other
implementation; it was not brute-forced.

## 4. Findings

All minor. "Location" refers to `verify/treecheck.cpp` unless stated otherwise.

| # | severity | location | problem | fix |
|---|---|---|---|---|
| 1 | minor (documentation) | `SPEC.md` §10.6; code 1372–1409, 1439–1441, 2162, 2218 | SPEC §10.6 says that with `--exact` "visited set and look-ups [are] keyed by full states, no hashing". Only the visited set is; table and leaf-file look-ups always go through 120 hash bits. Harmless for soundness (a wrongly found move is checked and its subtree verified; a wrongly "listed" state must pass LEAF), and for the certified run entries = lines = visited states and my exact-key re-expansion uses exactly the same 93,164 + 106,723 entries. PROOF.md §9.1 already states it correctly. | Correct the SPEC sentence (reported in `logs/spec_issues.md`, as SPEC asks), or patch P1 below. |
| 2 | minor | 1379–1390, 2385 | Two table lines with the same key and different moves are accepted silently: the numerically smallest canonical move is used regardless of file order (the comment "the first one is kept" refers to the sorted order), `table_conflicts` is reported but is not part of `ok`. Sound (the move used is verified; reproduced with mutations 41–45), but a table that is not a function should not pass as a certificate, and another reader (`sim`) may pick the other line. `table_conflicts` = 0 in the certified run. | Patch P2: make conflicts fail `ok`. |
| 3 | minor (workflow) | 3978–3991, 4000–4009; `CHECK` 82–89 | The `--out` file is written only at the very end. If the program exits earlier (exit 2 for an unreadable input or bad arguments; abort on a failed `CHECK` or an uncaught exception) a result file from an earlier run stays untouched. Reproduced: an existing `ok:true` JSON survived a run with a missing table (exit 2, nothing on stdout). A script that reads the file and ignores the exit code would see a stale `ok:true`. | Patch P3: write a failure placeholder first. Consumers should check the exit code and the input checksums (`table_fnv1a64`, `leaf_fnv1a64`, byte counts) in the JSON. |
| 4 | minor (diagnostics) | 1452–1456 | `fread` returning 0 is taken as end of file; `ferror` is never checked. A directory passed as `--table` is "loaded" as an empty table (mutation 59: exit 1 with a strategy hole instead of "cannot open"); an I/O error in the middle of a file would silently truncate it. Sound direction only (fewer entries can only add errors), but the error message is misleading. | Patch P4. |
| 5 | minor | 3886–3908 | Numeric options are parsed with `atoi` / `atof` / `strtoull` without validation: `--rmax 9junk` runs with R_max = 9, `--rmax junk` with 0 (both reproduced; both rejected the tree). The value actually used is in the JSON (`rmax`). | Use `strtol` with end-pointer check; reject trailing garbage. |
| 6 | minor (interface) | 2317–2318, 2381–2385 | With `--root` pointing to a VOID state the result is `ok:true`, exit 0, 0 states (`root_void:true`). This is SPEC ("VOID counts as won"), but `ok` alone is then not "the game is a draw". | Whoever cites a treecheck JSON must check `root == "0 - -"`, `root_void == false`, `rmax`, `complete` (all satisfied by `results/final/treecheck.json`). Optionally exit with a distinct code for a VOID root. |
| 7 | minor (blind spots of the checker's own evidence; not an error in the proof) | engine output; selftest 3464–3741 | (a) The verdict and all headline counters of the real run are insensitive to LEAF defects: the mutants `leaf_norecompute` and `leaf_skip_triangles` (unsound direction) give `ok:true` with identical `p_states`, `q_states`, `leaves_checked`, child and VOID counters; only `leaf_bfs` / `leaf_triple_flows` differ. "leaves_checked: 106723" is therefore no evidence that LEAF was evaluated correctly. (b) `lost_false` changes nothing at all in the real run (LOST cannot fit below 11 stones); the LOST code is exercised only by the self-test — it becomes load-bearing for any tree with R_max ≥ 11. (c) Defects that drop children keep `ok:true` and show only in the counters; `adm_overvoid` moves them by 2 states. So `ok:true` must be cited *together with* the counters and their agreement with independent re-expansions. (d) Self-test: `fadj_dropped` is caught only by an abort; `no_spacediag_threat` only by the "children" section (no quick test tree contains a threat on a space diagonal); the bogus-leaf test finds a suitable state in only 2 of the 6 quick trees (D, G; "no non-LEAF inner P-state" in A, B, C, H), which is all that catches `leaf_norecompute`. | (a) Keep citing the independent LEAF results next to the JSON; optionally add per-θ leaf counts and the numbers of single / pair / triple flows to the JSON so that two runs can be compared in more than one number. (b) State in PROOF/REPORT that LOST is vacuous at R_max = 10 (PROOF §9 does). (d) Add a test tree with a space-diagonal threat and make the bogus-leaf test mandatory in every tree. |
| 8 | minor (leniency) | 1436–1441 | Lines in a non-canonical frame are accepted (re-canonicalised, move mapped). Sound for "root is WON", but SPEC §5/§6 make the simulator look up the *canonical* key, so such a file would be a strategy hole for `sim`. Counted in the JSON (0 / 0 in the certified run; also 0 / 0 under my brute-force canonical form). | Treat `table_noncanonical + leaf_noncanonical > 0` as a failure in the certifying mode, or keep requiring "0 / 0" explicitly where the JSON is cited (PROOF §9.1 does). |

Patches (not applied; `verify/treecheck.cpp` is unchanged):

```diff
P2  (finding 2)  -- line 2385
-    res.ok = res.complete && res.errorCount == 0 && tableLoad.bad == 0 && leafLoad.bad == 0;
+    res.ok = res.complete && res.errorCount == 0 && tableLoad.bad == 0 && leafLoad.bad == 0 && table.conflicts == 0;
```

```diff
P3  (finding 3)  -- in main(), before the table is opened (line 3976)
+  if (!outPath.empty()) {                       // never leave the result of an earlier run behind
+    FILE* f0 = fopen(outPath.c_str(), "w");
+    if (!f0) { fprintf(stderr, "treecheck: cannot write %s\n", outPath.c_str()); return 2; }
+    fputs("{\"ok\":false,\"complete\":false,\"note\":\"run did not finish\"}\n", f0);
+    fclose(f0);
+  }
```

```diff
P4  (finding 4)  -- line 1456
-    if (n == 0) eof = true;
+    if (n == 0) {
+      if (ferror(f)) { fprintf(stderr, "treecheck: read error in %s\n", isTable ? "table" : "leaf file"); exit(2); }
+      eof = true;
+    }
```

```text
P1  (finding 1, optional)  -- exact look-ups under --exact
  In Engine: std::unordered_map<FullKey, uint8_t, FullKeyHash> tableExact; std::unordered_set<FullKey, FullKeyHash> leavesExact;
  filled in loadKeys from (cn.B, cn.F, t, kind) next to the hashed key; process() then uses
  cfg.exact ? tableExact.find(full key) : table.find(hash)   and the same for leaves (lines 2162, 2218).
```

Reproducer for finding 3: `cp base.json x.json; tc_orig --table /nonexistent --leaves results/final/leaves.txt
--rmax 10 --out x.json; echo $?` → 2, and `x.json` still contains `"ok":true`.
Reproducer for finding 2: mutation 43 (`python3 mut.py --only conflict_larger_t08`).
Reproducer for finding 7: `python3 srcmut.py build && python3 srcmut.py run leaf_norecompute leaf_skip_triangles lost_false`.

## 5. What I did not verify

* The mathematics around the tree (LEAF ⇒ C, soundness of SPEC §4 for the strategy of SPEC §5, "VOID states
  cannot arise", the PAIRING phase). Out of scope here; I only checked that treecheck implements SPEC §3/§4.
* LEAF on the real leaves by brute force — impossible (about 100 live lines). The real leaves were re-checked
  with my flow reference (§3.4), which is independent code but not a brute force.
* `--crosscheck` and `--selftest` default / full: not run by me. I read `logs/6c/selftest_default.log` and
  `selftest_full.log` (both "ALL PASSED", written 07:04–07:05 with the binary of 06:59); a `--crosscheck` run
  of the coordinator was still in progress while I worked.
* `bin/search` statistics, `verify/spotcheck.py`, `bin/phicheck` results: read, not re-run, and not used for
  any correctness argument above.
* Compiler, OpenMP runtime (the program uses `std::thread`, not OpenMP), hardware.
* Hash collisions in the non-`--exact` mode (not used by the certificate). I only observed that a run without
  `--exact` and with `--fast` gives the same headline counters.

## 6. Reproduction (all in `logs/review/treecheck_mut/`)

```
g++ -O2 -std=c++17 -march=native -fopenmp -Wall -Wno-unused-result -o tc_orig ../../../verify/treecheck.cpp ; cmp tc_orig ../../../bin/treecheck
./tc_orig --table ../../../results/final/strategy.tbl --leaves ../../../results/final/leaves.txt --rmax 10 --exact --paranoid --threads 2 --out base.json
python3 mut.py                                   # 66 mutations -> mutations.jsonl (appends)
g++ -O2 -std=c++17 -o abrute abrute.cpp
./abrute leaf 20261112 6000 16 leafA_inst.txt leafA_exp.txt        ; ./tc_orig --leafcheck --verbose < leafA_inst.txt > outA.jsonl ; python3 cmp_leaf.py leafA_inst.txt leafA_exp.txt outA.jsonl
./abrute leaf 777001 3000 16 leafB_inst.txt leafB_exp.txt all      ; (same two commands)
./abrute leaf 990017 400 20 leafC_inst.txt leafC_exp.txt           ; (same two commands)
./abrute lost 20261112 800 lost_inst.txt lost_exp.txt              ; ./tc_orig --lostcheck < lost_inst.txt > lost_out.jsonl ; python3 cmp_lost.py lost_inst.txt lost_exp.txt lost_out.jsonl
python3 srcmut.py build ; python3 srcmut.py run                    # -> srcmut_results.jsonl, src_mut/*.selftest.log
g++ -O2 -std=c++17 -o aexpand aexpand.cpp ; ./aexpand ../../../results/final/strategy.tbl ../../../results/final/leaves.txt 10 20 > aexpand_final.json
g++ -O2 -std=c++17 -o aleaf aleaf.cpp ; cat ../../../results/final/checks/leaves_theta{0,1}.txt | ./aleaf fast1 ; cat …/leaves_theta{2,3}.txt | ./aleaf ; ./aleaf < aleaf_inst.txt
```

Kept files: sources (`ageom.py`, `mut.py`, `srcmut.py`, `cmp_leaf.py`, `cmp_lost.py`, `abrute.cpp`,
`aexpand.cpp`, `aleaf.cpp`), `base.json`, `mutations.jsonl`, `mut_rest.log`, `srcmut_results.jsonl`,
`srcmut_run.log`, `src_mut/` (12 mutant sources, diffs, binaries, self-test logs), `leaf{A,B,C}_exp.txt`,
`leafC_inst.txt`, `leafC_out.jsonl`, `lost_*`, `aexpand_final.json`, `aleaf_*`. The instance files of
batches A and B (16 MB each) and the treecheck outputs for them were deleted after the comparison; I checked
that the commands above regenerate the instance and expectation files bit for bit.

## 7. CPU used (user + system, measured by the drivers or by `time`)

| item | CPU-s |
|---|---|
| compile original, baseline run, small option tests | 11 + 9 + ~12 |
| 66 mutation runs | 478 |
| brute-force LEAF batches (generation + treecheck + comparison) | ~10 |
| LOST batch (generation 23) and generator trials | ~50 |
| self-test quick, original | 21 |
| 12 source mutants: compile / self-test + final run | ~130 / 277 |
| structural re-expansion | 73 |
| flow reference: sample, θ = 0/1, θ = 2/3, validation of the θ = 1 reformulation | 96 + 25 + 194 + ~15 |
| compiles of my own tools, Python drivers and comparisons | ~45 |
| **total** | **about 1,450 CPU-s ≈ 24 CPU-minutes** |

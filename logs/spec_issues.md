
## Task 3 (py/ip_check.py, py/regress.py, py/geom.py) — notes, 2026-11-12

No blocking problem found; the following points are clarifications / deviations from the letter of SPEC.

1. SPEC 7.7 does not say what to output when the IP has no feasible point (F not B-admissible, or |F| > k).
   `ip_check.py` decides this exactly before calling HiGHS and writes `"status":"infeasible","decision":"holds",
   "worst":null` (C(B,F,k) is vacuously true).  Callers that treat such states as VOID should test `status`.
2. `decide` mode: `"worst"` is `0` for `"holds"` and `null` for `"fails"` (the search stops at the first incumbent
   >= 1, the optimum is not known); the exact value of the returned witness is in the extra key `"lb"`.
3. The JSON records carry extra keys after the SPEC ones: `mode, danger, obj, bound, lb, verified, nodes, note`.
   `verified` = the returned (X,S) was re-evaluated in integers (X admissible, F <= X, |X| <= k,
   2|S|-|N(S)\X| = reported value).  A `"holds"` answer cannot be verified from a solution; it rests on the
   floating-point bound.  HiGHS reports `mip_dual_bound` equal to the incumbent objective (about 1e-12) once the
   tree is exhausted, so the recorded `bound` carries no extra information beyond "< 1".
4. `--prefilter` (off by default) is implemented as the exact test "no pairing for (B,F)" => `"fails"` with the
   Hall violator as S and X = F (`"status":"prefilter"`); in `full` mode the IP is still solved for the value.
5. SPEC 8 item 9 says "worst 4-8".  Measured on 20 seeded random busy 8-sets (seed 20261112): all 20 bad,
   values 2,4,6,8,10,12 (16/20 inside 4..8; one 2, two 10, one 12).  The range in SPEC is a typical range, not a bound.
6. Timing: SPEC/PROGRESS expect 1-12 s per IP.  Measured (1 thread, nice, 3 solves in parallel, machine shared):
   instances with worst >= 1 take 0.03-23 s, but "holds" instances with a non-empty F are slower:
   item 4 (T8 + one revealed busy stone) 26-69 s each, item 5 155 s.  Budget the IP cross-check accordingly.

## Tasks 4+5 (src/search.cpp) — notes, 2026-11-12

1. **SPEC 4 default "table moves are busy cells" cannot prove the root.**  Measured with `bin/search`: with busy-only
   table moves the root fails for R_max = 9 and 10 already in the game *without any forced move* (no threats, no
   dangers, no LOST), all 24 busy cells tried at every Q-state (exhaustive; 13.8 s and 21.8 s; logs
   `logs/search/busyonly_r9_rel24.log`, `logs/search/busyonly_r10_rel24.log`).  Failing branch: the reveals give the FP the four
   corners of one face (e.g. cells 180,185,210,215).  The 14 lines inside a face are killed only by stones inside
   the face, whose only busy cells are the 4 corners; with F = the 4 corners that set has sur_F = 36-4-28 = 4 while
   theta = t-4 grows with t, so LEAF is impossible for t > 8.  Consequence: the proof needs `--nonbusy`
   (allowed by SPEC 4 "behind a flag"); `strategy.tbl` therefore contains non-busy table moves, which
   `verify/treecheck` and `sim/sim` must accept (SPEC 4/5 do, as written).
2. SPEC 7.4 lists only `--rmax --maxcand --threads --out --time-limit --nonbusy`; `bin/search` has additional
   options (`--relcand --nbextra --order --tt-bits --selfcheck --noemit --progress --lost-selftest`), and
   `stats.json` carries extra keys after the SPEC ones.  See `logs/search/NOTES.md`.
7. (task 3) Load dependence: HiGHS `time_limit` is wall-clock.  With the machine at load average ~9 (06:01-06:10 UTC)
   the same item-4 instances took 169-396 s wall instead of 26-69 s, so a 600 s limit can be hit by "holds"
   instances purely because of contention.  `ip_check.py` writes such records with `"status":"timeout",
   "decision":null` and re-solves them on the next (resumed) run; a merge step must keep the last record per instance.

## Proof writer (PROOF.md) — notes, 2026-11-12

Result of re-deriving SPEC 1, 3, 4, 5, 6 by hand: **no statement of SPEC was found false.** Every "Fact" of SPEC 3
and the soundness of SPEC 4 for the strategy of SPEC 5 are proved in PROOF.md. Points below are clarifications;
none requires a program change.

1. **L3(c) of the task statement (C(B,F,k) => C(B+b,F,k)) is false as literally stated and true with one extra
   hypothesis** (PROOF.md Proposition 5.4). Counterexample: B = {}, F = a line minus its cell b, k = 5:
   C({},F,5) is vacuously true (F is not {}-admissible), C({b},F,5) is false (F is {b}-admissible, 141 live lines
   need 282 cells, 210 available). If F is B-admissible the implication holds (hand proof; brute-force fuzz in
   `logs/proof/fuzz_cprime.py`, no failure). Not used by the tree (PROGRESS decision 05:25 stands: only LEAF's
   monotonicity is used).
2. LEAF condition 2 (pairing for (B,F)) is implied by conditions 1 and 3 when k >= |F| (PROOF.md Remark 3.3).
   Harmless redundancy; the max-flow test of condition 3 needs the pairing anyway.
3. SPEC 5, PAIRING mode: "take the other cell assigned to the same line" must be read as "if that cell is empty;
   otherwise any empty cell" — the other cell can already hold an SP stone taken earlier as an "any empty cell"
   move (its line is then dead). `sim/sim.cpp` implements it this way; Lemma 1.1 of PROOF.md proves this rule.
4. SPEC 4, LOST: the patterns E2/E3 are necessary for a loss of admissibility after SP's forced reply, not
   sufficient. E3 can fit although one danger cell resolves all pairs (pairs (a,b), (a',b'), (a,b'): the common cell
   of (a,b') kills all three); observed 622 times in 131 217 danger situations of `logs/proof/fuzz_l4.py`. LOST is
   conservative, which is sound; it can only make the root harder to prove. When LOST is false, *every* danger
   pair's cell works, so the tie-break "smallest c" of SPEC 5 is immaterial, and the pair with a given c is unique.
5. SPEC 6 / 7.5 / 7.6: `sim` canonicalises the real state while `search`/`treecheck` canonicalise a child computed
   in the canonical frame of its parent. The two agree because G is closed under composition (a group). PROOF.md
   Lemma 7.2 proves this by hand (G = D x (R semidirect P) / <-1>, order 48*48/2 = 1152); the frozen self-test only
   spot-checks it.
6. SPEC 7.2 sentence "any admissible X of size < k is covered because deficiency is monotone in X": monotonicity
   alone does not cover an admissible X of size < k that has no admissible extension of size k. The enumeration is
   exact because the pairing is examined at every node (this is what `src/match.hpp` does and says in its comment);
   an implementation testing only the size-k leaves would not be exact in general (PROOF.md Remark 2.3).
7. SPEC 3: worst(B,F,k) is a maximum over the empty family when F is not B-admissible (already item 1 of task 3).
8. The proof never uses that table moves are busy cells, so the `--nonbusy` tables of tasks 4+5 are covered.

## Task 7 (sim/sim.cpp) — notes, 2026-11-12

No blocking problem; clarifications of SPEC 5 as implemented in `bin/sim` (none is a deviation):

1. PAIRING mode: the partner of FP's cell can already hold an SP stone (an earlier "any empty cell" filler move may
   have landed on an assigned cell; that line is dead then).  `sim` then plays the smallest empty cell, as for an
   unassigned cell.
2. Step 2 tie-break "smallest c (then smallest line cell lists)": `sim` takes the smallest shared empty cell c and,
   among the live lines with exactly 4 FP stones through c, the two with the lexicographically smallest cell lists.
   (All choices are children (d) of the tree, so this cannot matter for soundness.)
3. Step 3: a table move m in B or in F is not covered by SPEC 5; `sim` counts it as a bug of its own kind
   (`illegal_table_moves`, exit code 1).  Non-busy table moves are accepted like busy ones.
4. Step 1 with two simultaneous threats: `sim` blocks the first threat line (smallest cell list) and then reports the
   position as an admissibility failure (a live line with 5 FP stones remains) without playing on.
5. Output: the JSON has the SPEC keys plus `seed, threads, illegal_table_moves, bug_games, completed_games,
   table_moves, max_table_round, total_rounds, seconds, load_seconds, canon_calls, cache_hits, table_entries,
   leaf_entries, table_conflicts, noncanonical_entries (with --verify-canonical), per_adversary, params`;
   `leaf_round_hist` has the extra key `"none"` for games that never reached a leaf.  The table and the leaf file
   are keyed by a 128-bit hash of (t,B,F), not by the exact key (allowed by the task statement).

## tasks 2 + 6a (match.hpp / phi.hpp) -- clarifications, no deviation from the predicates of SPEC 3

* 7.2 `enum_check`: if F is not B-admissible, or |F| > k, there is no admissible X at all, so C(B,F,k) holds
  vacuously and the function returns status 1 with nodes = 0. The CLI adds `"note":"vacuous"` in that case and
  always adds `"status"` (1 / 0 / -1; with -1 `"holds":null`). `py/ip_check.py` will report such instances as
  infeasible, so cross-checks should skip them or treat "infeasible" as "holds".
* 7.2 "any admissible X of size < k is covered because deficiency is monotone in X": the implementation
  checks the pairing at every node of the enumeration tree (every admissible F u {c1<..<cm}, m <= k-|F|), so
  sets of size < k are covered directly; the monotonicity is stated in the header comment but exactness does
  not depend on it. `nodes` counts the stone sets whose pairing was examined (a last stone on an unassigned
  cell changes nothing and is not counted). Only cells of N(L(B)) are candidates (a stone on a cell without a
  live line changes neither admissibility nor any N(S) \ X).
* 7.2 CLI: `bin/enumcheck` without `--limit N` does not terminate in practice for k-|F| >= 5 on a holding
  instance (C(208,5) stone sets); it prints a warning on stderr for k-|F| > 4. Use `--limit`.
* 7.3 `PhiResult.sur` is only specified for reason 3. Convention used: reason 0: k-|F|; reason 1: 0;
  reason 2: |N(S)\F| - 2|S| of the Hall violator (negative); reason 3: sur_F(S).
* 7.3 `phi_value` returns -1 when its precondition fails (F not admissible or no pairing).
* 7.3 `leaf_holds` returns the same boolean as `phi_check(...).holds` (tested), but takes a counting shortcut
  (unassigned cells < k-|F| and an F-connected triple exists => false) before building a pairing.

## Task 6c (verify/treecheck.cpp) — notes, 2026-11-12

No blocking problem found in SPEC 1, 3, 4, 6, 7.5.  Clarifications / deliberate choices:

1. `strategy.tbl` and `leaves.txt` entries are re-canonicalised with treecheck's own canonical form when they are
   loaded (the move is mapped with the same g), so entries in any frame are accepted.  This is lenient with respect
   to "its canonical form must be in strategy.tbl" but sound (WON is invariant under G).  The JSON keys
   `table_noncanonical` / `leaf_noncanonical` count entries that were not already canonical (0 for both
   `results/search/r10_nb_k4*` trees, i.e. `src/canon.hpp` and treecheck agree on the canonical form).
2. A malformed line in either file (wrong field count, cell out of range, duplicate cell, B∩F ≠ ∅, |B| ≠ t−1 resp. t,
   |F| > t, move in B∪F or > 215) makes `ok` false (`table_bad_lines`, `leaf_bad_lines`, `bad_lines`).
   Empty lines and lines starting with `#` are ignored.  Duplicate identical entries are only counted.
3. Non-busy table moves are accepted and counted (`nonbusy_moves`); `--busy-only` turns them into errors.
4. SPEC 4 remark "a pattern needs at least 11 resp. 12 distinct stones": E3 in fact needs at least 13 (four
   pairwise intersecting lines do not exist in this geometry: they would lie in one plane, whose two diagonals do
   not meet in a cell).  Harmless: 12 is still a valid lower bound and LOST is implemented from the definition.
5. Counting convention: treecheck's `void_skipped` includes VOID children of type (ii) (Q(B,F+m,t) with F+m not
   B-admissible): 149 resp. 151 for the two r10 trees, all of this type, while `stats.json` of bin/search reports
   `void_children: 0`.  All other counts (P-states, Q-states, leaves, threat / danger children) agree exactly.
6. Extra output keys follow the SPEC ones (`by_t`, `error_count`, `complete`, input sizes and FNV-1a checksums, ...).
   Extra options: `--exact` (visited set with full keys), `--fast` (skip the literal validation of the flows behind
   LEAF = true, which is on by default), `--crosscheck [--crosscheck-leaves N]` (slow second derivation of everything
   with naive reference code), `--root "t B F"`, `--leafcheck`, `--lostcheck`, `--selftest [--quick|--full]`,
   `--gen-tree` (test scaffolding).

## Proof writer (PROOF.md revision after the referee report) — notes, 2026-11-12 07:25

1. SPEC 10 erratum 6 says the certifying `bin/treecheck --exact` run has "visited set and look-ups keyed by full
   states, no hashing".  By the header and the code of `verify/treecheck.cpp` (struct `KeyStore`, `Visited`), `--exact`
   makes only the *visited set* use full keys; the look-ups in the strategy table and the leaf file go through
   120-bit keys with or without `--exact`.  The header argues that a collision there cannot make the program accept
   a state that is not WON (a found move is checked and its children are verified; a state taken for a leaf must pass
   LEAF), and `verify/spotcheck.py` (exact Python keys, every table and leaf line reached) confirms the files as such.
   PROOF.md 9.1 states it this way.  Suggested wording for the erratum: "(visited set keyed by full states)".
2. SPEC 4 remark "at least 11 resp. 12 distinct stones": proved with the exact count in PROOF.md Proposition 8.4a
   (E2: 12 - |S_l ∩ S_b|; E3: 15 - number of meeting pairs among (a,b'), (a',b), (b,b')).  On this board the minimum
   for E3 is 13, not 12 (literal enumeration `logs/proof/e23min.cpp`: 353,422,080 patterns, none with 12 stones;
   confirms item 4 of the task-6c notes); for E2 the minimum 11 is attained.  No program change.

## Task 6b (src/certify.cpp, py/crosscheck.py) — notes, 2026-11-12

No statement of SPEC was found false. Clarifications / deviations from the letter of the task statement:

1. `bin/certify` prints `"decision":"void"` not only when F is not B-admissible but also when F meets B or
   |F| > k (no admissible X exists in all three cases). `py/ip_check.py` answers `status:"infeasible"` for the
   first and the third case and `status:"invalid"` for B∩F ≠ ∅; the cross-check only uses the first and third.
2. Extra JSON keys after the required ones: `leaf` (LEAF(B,F,k)), `reason` (phi_check reason code), `theta`
   (k−|F|), and `us` with `--stats`. `nodes` counts the stone placements of the witness search plus the nodes of
   enum_check / of the branch and bound.
3. Step 4 (branch and bound): the "Fact" of the task statement is proved in the header of `src/certify.cpp` in the
   slightly stronger form "a minimal Y lies in N(S) for an X-connected S, |S| ≥ 3, with sur_X(S) = |Y|−1".
   Two exact shortcuts were added (both proved there): (R) if (B, X ∪ {all placeable candidates}) has a pairing the
   node holds; (L) with one stone left, X+c has no pairing iff c is assigned to a line that cannot gain a third
   cell, so no marking is needed at the last level.
4. The IP time expectation of the task statement ("1–12 s per instance") does not hold on this machine today:
   "holds" instances cost the IP a median of 3.9 s at θ = 3, 8.9 s at θ = 4, 92 s at θ = 6, 190 s at θ = 7, and
   2–5 min for F = ∅ with θ = 8–9 (4 of those hit the 300 s limit; wall-clock at load average 9–15); "fails"
   instances: median 5.8 s, maximum 291 s. `py/crosscheck.py` therefore (a) caps the number of "holds"
   instances with θ ≥ 6, (b) reuses the IP records that the same `py/ip_check.py` already wrote for identical
   instances under `results/ip/` and in `results/regression/ip_regression.cache.jsonl` (marked `reused_from`;
   `--no-reuse` solves everything again), (c) solves the instances that only the IP can cross-check first.
5. Scheduling pitfall (not a SPEC issue): processes started from one shell call share one scheduler group. Three
   IP shards started as children of one process got about 16 % of a core each; started with
   `start_new_session=True` they got 50–65 % each. `nice` alone does not arbitrate between different agents' jobs.
6. LEAF is strictly weaker than C only in rare, dense-F situations (lists in `results/crosscheck/summary.json`,
   key `leaf_false_but_C_holds`). Smallest pattern seen: three live lines through one empty cell, each with 3
   revealed stones, θ = 2 (sur_F = 1, but two more stones on the 7 free cells always break the cap or make a
   danger pair). For F = ∅ no such instance exists among all 1057 + 1669 busy orbits (k = |B| and |B|+1).

## Code audit of verify/treecheck.cpp (logs/review/treecheck_audit.md) — notes, 2026-11-12

1. **SPEC 10.6 is inaccurate.** It says that with `--exact` "visited set and look-ups [are] keyed by full states, no
   hashing". Only the visited set is keyed by full states; the look-ups in the strategy table and in the leaf file go
   through 120 bits of the state hash also with `--exact` (`KeyStore`, treecheck.cpp 1372-1409, used at 2162 and 2218).
   Harmless for soundness (a move found through a collision is still checked and its subtree verified; a state taken for a
   listed leaf must still pass LEAF) and without effect on the certified run (entries = lines = visited states; an
   independent re-expansion with exact keys uses exactly the same 93,164 table entries and 106,723 leaves).
   PROOF.md 9.1 states it correctly. No program change needed; the SPEC sentence should read "visited set keyed by full
   states".
2. SPEC 7.5 / 6: treecheck accepts table and leaf lines in a non-canonical frame (already item 1 of task 6c) and accepts
   two table lines with the same key and different moves (the numerically smallest canonical move is used, whatever the
   order in the file; counted as `table_conflicts`, not part of `ok`). Both counters are 0 for `results/final`.

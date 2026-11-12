# Audit of the independent simulator `sim/sim.cpp` (review part B, task 7)

Time-boxed audit, 2026-11-12 08:29 - 08:40 UTC. The official JSON `results/final/sim/sim_mix_1M.json` was complete
at 08:34 and is the source of the numbers in section 4.

Verdict: **sound-with-minor-issues**. The SP side implements SPEC section 5 (with erratum 10.1) literally;
no path was found on which a game ends or is dropped without its bug being counted; 0 bugs in every run.
The simulator is a sampling test: it did not exercise the whole certificate (see section 4).

## 1. Scope and what was read

* SPEC.md sections 1, 2.4 (set order), 3, 5, 6, 10 (errata).
* `results/final/sim/sim_snapshot_0806.cpp` (source of the official run; md5 c2112bb9...) and `sim/sim.cpp`.
  Read completely: geometry checks, set order, `canon`/`refine`, `Store`, `load_file`, `find_pairing`,
  `pairing_valid`, `Stats`, `lookup_q`, `lookup_leaf`, `find_danger`, `admissible_counts`, `forced_reply`,
  `round_start`, `play_impl`, `play`, `run_game`, `final_position_ok`, the game loop, counter merge and exit code
  in `main`. Read for the look-ahead question only: `forcing_result`, `find_winner`, `search_value`, `search_move`,
  `predict_table`, `steal_ahead`, `probe`, `plan_search`, `leaf_stress`. The remaining adversary code (move
  heuristics) and the 1,150 lines of self-test were not read line by line.
* `src/` and `verify/` were not opened.
* All JSON files in `logs/review/sim_audit_runs/` (and `old/`), `mutation_test.txt`, `logs/final/sim_mix_1M.log`.

Line numbers below refer to the snapshot `results/final/sim/sim_snapshot_0806.cpp`, which at the end of the audit
is byte-identical with `sim/sim.cpp` (section 2). The SP side was read in the 08:18 version of `sim/sim.cpp`, where
the same code sits 4 lines further down (four counter lines inserted in `Stats`); `diff` shows no other change
before line 1833.

## 2. Snapshot versus current source (the source moved twice during the audit)

* 08:28:45 `diff snapshot sim/sim.cpp`: 46 diff lines - two counters (`novel_leaf`, `novel_q`) in `Stats` and the
  per-adversary JSON, and the removal of `steal_random` (adversary steal timing). As announced.
* 08:29:41 `sim/sim.cpp` was modified again and `bin/sim` rebuilt at 08:29:54 while this audit was running (a
  process `bin/sim ... --out logs/review/sim_audit_runs/probe_focus.json` on `results/search/r10_nb_k4_rebuilt`
  was started at 08:29 by another process). The diff to the snapshot was then 65
  lines: additionally a "focused" steal+threat variant in `st_move` (`rho[]` in `AdvState`).
* In both versions every difference is in adversary code, statistics or JSON output. There is no difference in
  lines 1 - 837 and 882 - 1832 of the snapshot, i.e. geometry, canon, store, loader, matcher, `lookup_*`,
  `forced_reply`, `round_start`, `play_impl`, `play` are byte-identical, and so are `run_game`, the game loop and
  the exit code. The conformance reading below therefore holds for the snapshot (official run) and for both
  later versions.
* 08:36:25 `sim/sim.cpp` was replaced once more and `bin/sim` rebuilt at 08:36:34: since then both are
  byte-identical with the snapshot (`cmp`: source md5 c2112bb9..., binary md5 175114e4... = `sim_snapshot_0806.bin`)
  and no other `bin/sim` process is running. State at the end of the audit (08:37): current source = snapshot =
  source of the official run, diff empty.
* I pinned each binary I ran (section 5): 08:18 build, 08:29:54 build (md5 77bc4812..., source 5684e473...) and
  the final build (= snapshot).

## 3. Conformance of the SP side with SPEC section 5

| # | SPEC item | Location (snapshot) | Verdict |
|---|-----------|---------------------|---------|
| 1 | Leaf test at the start of every round in TABLE mode, P(B,F,t), including round 0 | `run_game` 2497 (before the first FP move); `play_impl` 1298 -> `round_start` 1195-1217; look-up 1198 via `lookup_leaf` 998-1023 (canon of (B,F), exact key t,B,F) | matches |
| 2 | Pairing on the real position (B, real X); missing pairing = bug | 1207 `find_pairing(g.B, g.X, ...)`; 1209 `ST_PAIRING_FAIL`; additionally 1210 `pairing_valid` (every live line exactly 2 assigned empty cells, no cell twice) else internal error | matches; stricter than SPEC |
| 3 | Switch to PAIRING mode, F frozen | 1211-1212 (`g.pairing = true`); F is only written in `forced_reply` and the table loop, both TABLE-mode only | matches |
| 4 | FP places x, t <- t+1 | 1224-1226; an occupied / out-of-range x is an internal error (1223), counted as a bug | matches |
| 5 | FP line detection | 1227-1228: after EVERY FP move, before the mode branch (both modes), over all lines through x (`LC[x]`, the only lines whose count changed), `xc == 6`. Line set = all 148 lines, built and counted at start (exit 3 otherwise). Independent end check `final_position_ok` 2477-2489: every one of the 148 lines holds an SP stone | matches |
| 6 | Threat: live line with 5 X -> SP takes the sixth cell, F gets the 5 stones, B gets the cell | `forced_reply` 1168-1179 (first live line with `xc == 5` in line order; reply = the cell not in X; F \|= line ∩ X); stone placed 1271 | matches. If two threats exist, the second is caught by item 10 |
| 7 | Danger: two live lines with exactly 4 X sharing an empty cell, smallest c, then smallest lines; F gets X∩a and X∩b | `find_danger` 1050-1070 (cells ascending, first empty cell with >= 2 live 4-lines, the two smallest line ids = smallest cell lists because lines are sorted lexicographically); 1181-1183 | matches |
| 8 | Table: canonical form of Q(B,F,t), move mapped back with the inverse map; occupied -> reveal and repeat; missing = hole | `lookup_q` 961-996 (`canon` 385, table find, `GI[c.g][m]` 992); loop 1244-1263: 1247 hole, 1253 move in B or F = `illegal_table_move`, 1254-1259 m in X -> `F.set(m)` and look up again, else play m | matches |
| 9 | Order threat -> danger -> table | 1240, 1242 | matches |
| 10 | After SP's TABLE-mode move X must be B-admissible | 1276-1282, executed after threat, danger and table replies alike; `admissible_counts` 1073-1081 (no live line with >= 5, no danger pair). Also |B| = |X| = t, B∩X = ∅, F ⊆ X (1278-1280) | matches |
| 11 | PAIRING mode: partner if empty, else any empty cell (erratum 10.1) | 1231-1237 (`partner[x]` if empty else smallest empty cell) | matches |
| 12 | PAIRING-mode safety beyond SPEC | 1286-1295: a live line through x must keep both its assigned cells empty and have < 5 X, else `pairing_invariant_failure` | extra check |
| 13 | End of game | board full 1196 (`ST_END`) or FP line 1228 | matches |
| 14 | Canonical form (SPEC 6 / 2.4) | `cmp_set` 118-125 (smallest differing cell, the set containing it is smaller), `refine` 347-383, applied to B then to F in `canon` 385-397; cross-checked against the plain 1152-map loop under `--paranoid` (974-981, 1010-1017) | matches |

Paths on which a game could end or be dropped silently - none found:

* `run_game` 2493-2524: the loop leaves only on a status != ok; `absorb` (872-906) counts every status, and
  "ok" is counted as internal error (904). `--script-only` is the only early exit and it is off in all runs
  (`script_len 0`). `main` checks `games counted == requested` and `ended + bugs == games` (4230) and exits 3
  otherwise (4378).
* No exceptions are used (no try/catch/throw); file and geometry errors exit 2 / 3.
* The bug list is truncated to `--max-bugs` records, the counters are not.
* Look-ahead: all look-ahead of sigma runs on value copies (`Game h = g`) through the same `play` wrapper
  (1300-1307) with `st == nullptr`: no statistics, coverage only in the "probed" bit (bit 1), never in the "hit"
  bit. Every bug status met there increments `laBugs` and stores the full move list (up to 64 per thread; the
  count is not truncated). Call sites: 1736, 1748, 1774 (search), 1847 (`probe`), `plan_search` (two calls).
  `laBugs` is in the JSON (`lookahead_bugs`) and in the exit code (4382). A look-ahead position cannot become a
  real move: `choose_move` receives `const Game&` and only returns a cell, which is then played by the real
  `play` with full checks.
* Exit code 4382: 1 if bug games, look-ahead bugs, paranoid failures, file anomalies or leaf-stress failures.

## 4. Evidence

All files were opened and the numbers copied from the JSON (not from logs). Certificate: `results/final/strategy.tbl`
93,164 lines (md5 caa0bde1...), `results/final/leaves.txt` 106,723 lines (md5 95f25737...); no duplicates, no
conflicts, no bad shapes in any run.

### 4.1 Official run and the largest audit runs

| | official `sim_mix_1M.json` | `probe_novelty.json` | `old/mix200k_weights_v1.json` | `mix60k_s4242_v3_b64.json` | `tree_p16_60k.json` |
|---|---|---|---|---|---|
| source | snapshot 0806 | 08:18 source, batch 2000 | earlier audit version | earlier audit version | earlier audit version |
| games / seed / adversary | 1,000,000 / 20261112 / mix | 200,000 / 20261112 / mix | 200,000 / 20261112 / mix | 60,000 / 4242 / mix | 60,000 / 3 / tree |
| fp_lines | 0 | 0 | 0 | 0 | 0 |
| holes | 0 | 0 | 0 | 0 | 0 |
| pairing_failures | 0 | 0 | 0 | 0 | 0 |
| admissibility_failures | 0 | 0 | 0 | 0 | 0 |
| illegal_table_moves | 0 | 0 | 0 | 0 | 0 |
| pairing_invariant_failures | 0 | 0 | 0 | 0 | 0 |
| internal_errors | 0 | 0 | 0 | 0 | 0 |
| lookahead_bugs | 0 | 0 | 0 | 0 | 0 |
| paranoid_failures / file_anomalies | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| leaf stress tests / failures | 1,859,012 / 0 | 417,852 / 0 | 490,756 / 0 | 151,516 / 0 | not run |
| game_count_ok, completed games | true, 1,000,000 | true, 200,000 | true, 200,000 | true, 60,000 | true, 60,000 |
| threat events | 1,121,484 | 202,970 | 174,789 | 51,972 | 46,060 |
| danger events | 85,806 | 18,121 | 24,992 | 7,803 | 3,405 |
| occupied reveals | 2,254,365 | 519,508 | 473,265 | 152,336 | 241,565 |
| rounds with >= 2 occupied reveals | 369,238 | 62,359 | 49,525 | 15,727 | 41,804 |
| table moves played / of these non-busy | 7,724,521 / 36,890 | 1,568,178 / 5,882 | 1,574,781 / 4,137 | 474,723 / 1,006 | 491,937 / 4,223 |
| leaves by round 8 / 9 / 10 | 117,835 / 832,519 / 49,646 | 23,934 / 162,863 / 13,203 | 35,316 / 154,806 / 9,878 | 8,624 / 48,254 / 3,122 | 2,056 / 54,486 / 3,458 |
| leaves after >= 1 threat | 745,678 | 148,471 | 131,843 | 42,065 | 38,284 |
| leaves after two threats | 375,806 | 54,499 | 42,946 | 9,907 | 7,776 |
| leaves after a danger | 85,806 | 18,121 | 24,992 | 7,803 | 3,405 |
| leaves after threat and danger | 0 | 0 | 0 | 0 | 0 |
| table keys hit (real games) / 93,164 | 78,957 (84.75 %) | 61,804 (66.3 %) | 59,098 (63.4 %) | 38,658 (41.5 %) | 30,478 (32.7 %) |
| table keys hit or probed in look-ahead | 85,050 (91.29 %) | 70,618 | 70,233 | 49,557 | 32,653 |
| leaf keys hit (real games) / 106,723 | 81,907 (76.75 %) | 56,359 (52.8 %) | 49,725 (46.6 %) | 28,222 (26.4 %) | 21,961 (20.6 %) |
| leaf keys hit or probed in look-ahead | 88,071 (82.52 %) | 66,331 | 65,679 | 40,980 | 25,440 |
| non-busy table entries hit / 434 | 392 | 322 | 317 | 209 | 300 |

Official run, games per adversary (all with 0 bugs): random 30,228; greedy 29,735; steal 40,256; threat 49,882;
search 20,019; danger 89,871; double 90,004; corner 150,084; tree 499,921 (mix weights 0.03, 0.03, 0.04, 0.05, 0.02,
0.09, 0.09, 0.15, 0.5). Threat events by round: 5: 237,226; 6: 140,242; 7: 115,036; 8: 188,355; 9: 434,015;
10: 6,610. Danger events by round: 8: 76,116; 9: 5,410; 10: 4,280. Leaf signatures: no forced event 168,516; one
danger 85,806; one threat 369,872; two threats 375,806. `max_table_round` 10, every game reached a leaf by round 10.
`sim_mix_1M_bugs.jsonl` is empty (0 bytes). The official command line had neither `--paranoid` nor
`--verify-canonical`; the log does not contain the exit code (it was echoed to the coordinator's terminal), but
bug_games = lookahead_bugs = paranoid = file_anomalies = stress failures = 0 and game_count_ok = true imply exit 0
by line 4382.

Other audit runs (all 0 bugs, 0 look-ahead bugs, 0 internal errors): `mix200k.json` (200,000 games, seed
20261112 - this is exactly the first 200,000 games of the official run: same binary logic, same per-adversary
counts, coverage 59,677 / 53,652 identical to the official log line at 200,000 games; it is NOT independent
evidence), `old/mix200k_prelim.json`, `old/mix200k_weights_v2.json` (200,000 each), `mix60k_s4242*.json` (4 x
60,000), `tree_p8/p16/p40`, `tree6k/20k`, `corner6k/20k`, `double6k/20k`, `danger6k`, `mix4k`, `paranoid4k`,
`det_t1/t3nc/t4` (3,000 games each with 1 thread, 3 threads + `--no-cache`, 4 threads: identical counters, so the
results do not depend on threads or on the look-up cache).

Why "leaves after threat and danger" is 0 and there are never three threats or two dangers: TABLE mode lasts at most
10 rounds (no table or leaf entry has t > 10), lines share at most one cell and the answered lines are dead, so
threat + danger needs >= 5 + 3 + 3 = 11 FP stones, three threats >= 5 + 4 + 3 = 12, two dangers >= 8 + 2 + 2 = 12.
The four observed signatures (none, T, TT, D) are all that can occur; each was exercised >= 85,000 times.

### 4.2 Fraction of the certificate exercised (official run)

| t | table keys hit / total | leaf keys hit / total |
|---|---|---|
| 1 - 6 | 603 / 603 (100 %) | - |
| 7 | 3,406 / 3,487 (97.7 %) | - |
| 8 | 13,509 / 14,362 (94.1 %) | 3,089 / 3,382 (91.3 %) |
| 9 | 53,158 / 62,381 (85.2 %) | 68,973 / 87,314 (79.0 %) |
| 10 | 8,281 / 12,331 (67.2 %) | 9,845 / 16,027 (61.4 %) |
| all | 78,957 / 93,164 (84.75 %) | 81,907 / 106,723 (76.75 %) |

Leaf keys by u = t - |F| (unrevealed FP stones at the leaf): u = 0: 45,054 / 64,841 (69.5 %); u = 1: 27,922 /
32,703 (85.4 %); u = 2: 7,029 / 7,267; u = 3: 1,424 / 1,434; u >= 4: 478 / 478 (100 %).

Not reached (`results/final/sim/sim_mix_1M_uncovered.txt`, 39,023 lines = 14,207 table + 24,816 leaf entries,
classified by me):

* table entries by t: 7: 81; 8: 853; 9: 9,223; 10: 4,050. By u = t - |F|: 0: 9,855; 1: 4,199; 2: 148; 3: 5.
  By |F|: 6: 15; 7: 361; 8: 3,344; 9: 7,851; 10: 2,636. 42 of them have a non-busy table move.
  6,093 were at least touched in a look-ahead simulation, 8,114 (8.7 % of the table) never at all.
* leaf entries by t: 8: 293; 9: 18,341; 10: 6,182. By u: 0: 19,787; 1: 4,781; 2: 238; 3: 10.
  By |F|: 6: 6; 7: 251; 8: 3,578; 9: 16,281; 10: 4,700. 6,164 touched in look-ahead, 18,652 (17.5 % of the leaf
  file) never at all.
* So the unexercised part is almost entirely late states (t = 9, 10) in which nearly every FP stone is revealed
  (|F| >= 8, u <= 1): states after two threats, after a danger, or after long chains of occupied reveals, at
  specific stone configurations the adversaries did not hit. Everything with t <= 6 and every leaf with u >= 4 was
  exercised.
* Union with the other-seed / other-version audit runs for which uncovered lists exist (`mix60k_s4242`,
  `old/mix200k_weights_v2`): still never hit in any real game: 11,809 table entries (12.7 %) and 21,681 leaves
  (20.3 %).
* Whether all uncovered entries are reachable in real play at all was not determined (the table is the abstract
  tree of SPEC 4; an entry is hit only if some real FP stone sequence makes sigma reveal exactly that F).
* A "hit" of a leaf key means: for the one real X of that game a pairing exists, is valid, and the pairing strategy
  then held to the full board against one continuation, plus up to 4 random admissible X ⊇ F of size t (leaf
  stress) when t > |F|. It is a sample of the leaf claim C(B,F,t), not a verification of it.

### 4.3 Mutation test (`mutation_test.txt`, run by the previous audit agent, not re-run)

M0 control exit 0; M1 missing table entry 929 holes; M2 wrong table move 328 bug games; M3 bogus leaf 678 pairing
failures; M4 missing leaf 224 holes; M5 illegal table move 1,222; M6 non-canonical entry, M7 conflicting duplicate,
M8 bad shape: each flagged as file anomaly; M9 missing deep entry 21 holes. All mutants exit 1.

## 5. Own runs (task 3)

`bin/sim` was rebuilt at 08:29:54 and again at 08:36:34 during the audit, so both runs were done with all three builds:

| binary | `--selftest` | `--games 4000 --seed 11 --adversary mix --threads 2 --paranoid --verify-canonical` |
|---|---|---|
| 08:18 build (46-line diff to the snapshot) | "self-test passed (0 failed checks, 16.0 s)", exit 0 | 0 bug games, 0 look-ahead bugs, 0 paranoid failures, 0 file anomalies, 0 / 11,948 leaf stress failures, exit 0 |
| 08:29:54 build, pinned copy (md5 77bc4812..., source md5 5684e473...) | "self-test passed (0 failed checks, 15.3 s)", exit 0 | 0 bug games (fp_lines 0, holes 0, pairing 0, admissibility 0, illegal 0, pairing_invariant 0, internal 0), look-ahead bugs 0, paranoid failures 0, noncanonical entries 0 (both files, 199,887 entries), file anomalies 0, leaf stress 0 / 12,404, threat events 3,120, danger events 328, occupied reveals 11,321, leaves by round 632 / 3,215 / 153, table keys hit 6,775, leaf keys hit 3,079, exit 0 |

| 08:36:34 build = snapshot 0806 (md5 175114e4..., the binary of the official run and the current `bin/sim`) | "self-test passed (0 failed checks, 14.9 s)", exit 0 | 0 bug games (fp_lines 0, holes 0, pairing 0, admissibility 0, illegal 0, pairing_invariant 0, internal 0), look-ahead bugs 0, paranoid failures 0, noncanonical entries 0, file anomalies 0, leaf stress 0 / 11,704, threat events 3,245, danger events 365, occupied reveals 10,803, non-busy table moves 48, leaves by round 644 / 3,237 / 119, leaves after two threats 430, after a danger 365, table keys hit 6,889, leaf keys hit 3,070, game_count_ok true, exit 0 |

`logs/review/sim_audit_runs/final_paranoid4k.json` holds the last run (snapshot binary = current `bin/sim`). `--paranoid` re-derives
after every move the line counters from the sets, the admissibility verdict by brute force over line pairs, the
canonical form by the plain loop over 1152 maps (including image(g,B) = canonical B for the map used to translate
the move) and the pairing existence with a second matcher. Own compute: about 2.2 CPU-minutes.

## 6. Findings

No fatal or major finding. Minor:

1. **Source and binary changed twice during the audit** (`sim/sim.cpp` 08:29:41 / `bin/sim` 08:29:54 by a
   process that also started a `probe_focus` run at 08:29; then 08:36:25 / 08:36:34 back to the snapshot). The
   statement "current source differs from the snapshot only in steal timing and two counters" was true at
   08:28:45 (46 diff lines), false between 08:29:41 and 08:36:25 (65 diff lines, an additional "focused" steal
   variant in `st_move`) and is now moot: at 08:37 `sim/sim.cpp` and `bin/sim` are byte-identical with the
   snapshot. The SP side was identical in all three versions. Fix: the final report should say "sim/sim.cpp =
   snapshot, md5 c2112bb9..." (re-check with `cmp` at freeze time) and drop the sentence about steal timing.
2. **Holes seen by the table predictor are not recorded** (snapshot 1798-1808 `predict_table`, 1818-1832
   `steal_ahead`): a missing or illegal entry met while predicting sigma's next table move returns -1 ("no steal")
   and is neither counted in `lookahead_bugs` nor reported. For the prediction of the current round this is the
   entry sigma will need after any quiet FP move, so the real game will normally run into it and count it, but this
   is not guaranteed (the fallback move may be a forcing move). No effect on reported counts being correct; it
   only means "look-ahead bugs 0" does not cover this predictor. Fix: count such events (e.g. `predicted_holes`) or
   play a quiet move in a clone through `play`.
3. **The official run has no `--verify-canonical` and no `--paranoid`.** Not a soundness problem (a non-canonical
   entry can only cause a hole; canonicity of both files was confirmed with the simulator's own canon by the snapshot binary in
   `final_paranoid4k.json`, and in `mix200k.json` and the `old/` runs: 0 non-canonical entries), but the final report
   should cite one of those runs for the file check and the 4,000-game paranoid runs for the cross-checks.
   In the non-paranoid runs FP-line detection is still double-checked independently of the incremental counters:
   `final_position_ok` recomputes the position from the move list and requires an SP stone on each of the 148 lines.
4. **Coverage is partial and must be stated as such**: 84.75 % of the table keys and 76.75 % of the leaf keys were
   exercised by real games of the official run (87.3 % / 79.7 % in the union with the audit runs that kept
   uncovered lists). 14,207 table entries and 24,816 leaves, almost all with t = 9 or 10 and |F| >= 8, were never
   played. The simulator is statistical evidence for the implementation chain (table format, canonical maps,
   reveal rule, pairing); the proof itself rests on treecheck / spotcheck, which reach every line.
5. **`mix200k.json` duplicates the first 200,000 games of the official run** (same seed, same adversary logic);
   `probe_novelty.json` and the `old/mix200k_*` runs share the seed (same adversary assignment per game, different
   adversary code). They must not be added to the 10^6 games as independent samples. Other seeds used by the audit runs: 4242
   (4 x 60,000, four adversary versions), 2, 3, 5, 11, 1.
6. The exit code of the official run is not in `logs/final/sim_mix_1M.log` (only implied by the JSON).

Not deviations (checked, for the record): with two simultaneous threats or a danger pair left after the forced
reply the game ends as `admissibility_failure`; with three or more live 4-lines through the smallest danger cell
only the two smallest lines are revealed, as SPEC 5 says; a table move inside B or F is `illegal_table_move`; the
"any empty cell" of PAIRING mode is the smallest empty cell.

## 7. Not verified

* The adversary heuristics (about 1,300 lines) and the self-test body were not read line by line; they cannot
  hide SP bugs (every real move goes through `play`), only limit coverage.
* Reachability of the 39,023 uncovered entries in real play; the abstract proof tree, LEAF and the flow arguments
  (treecheck's job, `src/` and `verify/` not opened).
* The mutation test was not re-run; the audit runs other than the two of section 5 were taken from their JSON.
* `find_pairing` was read but its exactness argument (Kuhn with greedy start) was only checked through the
  validity check of every computed pairing (a false "exists" is impossible) and the second matcher in the
  paranoid runs (a false "does not exist" would in any case be reported as a bug, not hidden).

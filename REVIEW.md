# REVIEW.md — Task 8, lead adversarial review

Reviewer: lead adversarial reviewer (task 8). 2026-11-12, 08:06–08:40 UTC. I own only this file; my scripts,
programs and logs are in `logs/review/lead/`. Nothing was committed, pushed or uploaded. Own compute: about
7.5 CPU-minutes, all under `nice -n 10`.

## 1. Scope and method

Claim under review: *in 6×6×6 tic-tac-toe the second player (SP) has a strategy under which the first player
(FP) never owns a complete line; hence (strategy stealing) the game is a draw.* The claim has two parts:
the mathematics of PROOF.md ("if the root of the tree of SPEC §4 is WON, the strategy σ works": Theorem 0.2)
and the computed hypothesis (H) ("the root is WON for R_max = 10", certificate `results/final/strategy.tbl`,
`results/final/leaves.txt`).

Versions reviewed (sha256, first 16 hex digits): PROOF.md `865e1b344e71b70c` (1,079 lines, the revision after
the referee report; for the later status-only edit see finding L7), SPEC.md `46cc8367678fba4e` (with §10 errata 1–7), `verify/treecheck.cpp`
`f4e2f34e5bd741ba` (the audited revision), `verify/spotcheck.py` `7cc772ae0c06f1c6`, `strategy.tbl`
`13629393fcdcee87`, `leaves.txt` `3c14c7814e296e81`.

Method. I treated the four review reports and `logs/spec_issues.md` as leads only.

1. I re-derived the mathematics that the theorem uses before comparing with PROOF.md (§2.1).
2. I read the code that the certificate rests on (`verify/treecheck.cpp`, `verify/spotcheck.py`) against SPEC §4
   clause by clause (§2.4), and the SP side of `sim/sim.cpp`.
3. I did not stop at reading: I wrote **my own two programs that re-verify hypothesis (H) completely** —
   `lead_expand.cpp` (re-expansion of the whole tree, exact keys, brute-force canonical form, literal children)
   and `lead_leaf.cpp` (LEAF for every line of `leaves.txt`, triple flows with no pruning rule beyond
   "a strong single line") — and **my own implementation of the real strategy σ** (`attack.py`) with which I
   attacked the abstraction in real coordinates (§2.2, §2.3).
4. I opened every evidence file named in the task and recounted the numbers from the raw files with my own
   scripts (§2.5); I re-ran the certifying `bin/treecheck` command.

What I did **not** do is listed in §5.

## 2. What I checked myself

### 2.1 Mathematics (re-derived, then compared with PROOF.md)

**(a) LEAF ⇒ C (PROOF §3, Theorem 3.2, lines 265–277). Correct.** My derivation: let X be B-admissible, F ⊆ X,
|X| ≤ k, and suppose S ⊆ L(B) violates Hall, 2|S| > |N(S)∖X|. Split S into classes of the relation "share a cell
outside X"; the sets N(Sᵢ)∖X are disjoint, so one class S′ is itself a violator. |S′| = 1 means a live line with
≥ 5 cells of X (cap). |S′| = 2: the lines share c ∉ X, |a∪b| = 11 and |(a∪b)∖X| ≤ 3 force |X∩a| + |X∩b| ≥ 8, i.e.
4 + 4 with the common cell free: a danger pair (or a cap violation). |S′| ≥ 3: S′ is connected through cells
outside X ⊇ F, hence F-connected, so sur_F(S′) ≥ k − |F| ≥ |X∖F| and |N(S′)∖X| ≥ |N(S′)∖F| − |X∖F| ≥ 2|S′|.
No off-by-one: at a leaf k = t = |X| and θ = t − |F| is exactly the number of unrevealed stones. Condition 2
of LEAF is redundant (take X = F), as Remark 3.3 says.

**(b) Triples and max-flow (PROOF §4).** For soundness only one direction matters: *the programs' "true" ⇒
C_Φ.* I verified it without max-flow/min-cut: if there is an assignment of cells (each on its line, outside F,
no cell twice) in which every live line outside T holds exactly 2 cells and the lines of T hold 2|T| + θ in
total, then for every S ⊇ T the lines of S hold 2|S| + θ distinct cells of N(S)∖F, so sur_F(S) ≥ θ. Every
F-connected S with |S| ≥ 3 contains an F-connected triple (a spanning tree has a vertex of degree ≥ 2). The two
pruning rules ("a line / a pair of T already has such an assignment") are the same statement for a subset of T.
treecheck's extra device (a strong line gives up its two cells and is "deleted") is sound by induction on the
order of deletion: a set S that contains a deleted line contains a line already proved strong. The converse
direction ("false" verdicts, Prop. 4.2(a), 4.3) is needed for completeness of the search only; I checked it
(the pairing hypothesis in 4.2(a) is used correctly; Prop. 4.3(c) is right as revised) but nothing in the
theorem depends on it.

**(c) Soundness of the lazy-revelation tree (PROOF §8, Theorem 8.9, lines 721–766). Correct.** Invariant at the
start of a round in TABLE mode: (I1) canon P(B,F,t) is a node of the reachable set V; (I2) B = SP's stones,
|B| = t; (I3) F ⊆ X, |X| = t; (I4) X is B-admissible. FP plays x, X′ = X + x.

* *Case analysis is complete.* A live line with 6 cells of X′ is impossible (cap 4 before the move); two live
  lines with 5 would both contain x and have had 4 + 4 stones sharing the free cell x — a danger pair before
  the move. In a danger pair of X′ exactly one line contains x (none: danger pair of X; both: the common cell
  would be x ∈ X′).
* *Threat.* σ takes the sixth cell e and reveals the 5 stones. The new state is the enumerated child (c) for
  (l, e): l live, e ∉ F because e ∉ X′, and F ∪ (l∖{e}) ⊆ X′ has ≤ t+1 cells. *Danger.* σ takes c and reveals
  X′∩a, X′∩b: child (d) with Q_a = X′∩a, Q_b = X′∩b (4 cells each, c in neither, F∩a ⊆ Q_a because F ⊆ X′,
  |F∪Q_a∪Q_b| ≤ t+1). All stone-count conditions of (c), (d), (ii) are instances of "a subset of X′, |X′| = t+1".
* *Stone counts (Prop. 8.4a, lines 604–635), re-derived.* Threat line l (5 stones) plus a surviving danger pair
  (a new, b old): l∩a = {x}, a∩b = {c} is free, |l∩b| ≤ 1, so ≥ 5 + 4 + 4 − 2 = 11 stones. Two danger pairs
  with different new and different old lines: ≥ 16 − 4 = 12. I add (not in PROOF.md, same counting): two
  danger pairs that share a line also need ≥ 11 stones (same new line: 4 + 4 + 4 − 1; same old line:
  7 + 4). **Hence with |X′| ≤ 10 there is at most one forcing event after any FP move: exactly one threat line
  and no danger pair, or exactly one danger pair, or nothing.** An expanded node has t < R_max = 10, so
  |X′| = t + 1 ≤ 10: after the forced reply X′ is admissible for the new B, LOST is false at every expanded
  node for a purely arithmetic reason, the tie-break of SPEC §5 step 2 is vacuous, and no child (c)/(d) reached
  in play is VOID. (My re-expansion finds 0 VOID children (c)/(d) in the whole tree, and in 1,860 adversarial
  games no position with two simultaneous forcing events occurred.)
* *Reveal loop at Q-states.* No threat and no danger pair means X′ is B-admissible. The table move m is outside
  B∪F. If m ∈ X′ then m ∈ X′∖F, so |F| < t+1 and child (ii) exists; it is not VOID because F + m ⊆ X′ and
  admissibility is inherited by subsets; the loop ends after ≤ t+1 reveals. If m ∉ X′ it is empty; child (i)
  is the next P-state and X′ stays admissible when B grows.
* *VOID.* A VOID state has an inadmissible F; a real play never enters one because F ⊆ X′ and X′ is admissible
  for the B of that state (subsets of admissible sets are admissible). Skipping VOID children is sound.
* *Stop rule.* LEAF is tested only at P-states, i.e. at the start of a round with FP to move, with k = t. There
  X is B-admissible, F ⊆ X, |X| = t, so C(B,F,t) gives a pairing for the *real* X; from then on Lemma 1.1
  applies (I re-derived it: invariant "every line live at the switch contains an SP stone or both its assigned
  cells are empty"; a line whose assigned cell FP takes gets an SP stone at once; the rule "partner if empty,
  otherwise any empty cell" always names an empty cell because an odd number of cells is occupied after an FP
  move). A non-leaf P-state must have t < R_max, so TABLE mode ends after at most 10 rounds.
* Strategy stealing (Prop. 0.3) and Corollary 0.4 are the standard arguments and are correct for this game.
* Symmetry (PROOF §7): every notion is defined through cell/line incidence, so WON is invariant under
  line-preserving bijections; that the 1152 maps form a group is needed only so that the canonical form of the
  real state equals the canonical form computed in the tree. I verified the group property completely by
  machine (§2.2, item 0) and followed the hand proof (Lemma 7.2).

**(d) Deviations from the task statement (PROGRESS.md "Decisions", REPORT.md §2). Sound and stated honestly**,
with two labelling remarks (findings L3, L4 in §3.2).

| deviation | my assessment |
|---|---|
| LEAF (integer max-flow) instead of the raw integer program | Sound: LEAF ⇒ C is proved and is all Theorem 8.9 needs. LEAF is only sufficient; the documents say so. The IP is a cross-check. |
| Lazy revelation instead of branch (b) | Sound: Theorem 8.9 is proved directly for the lazy tree and the lazy strategy. I re-derived Prop. 8.11 / Cor. 8.12 (lazy WON ⇒ the eager tree with the same obligations plus (b) is WON); they are correct but not needed. |
| Non-busy table moves | Sound: nothing in §8 uses that m is busy; rules (i), (ii) are identical for every cell. All three verifiers and my own programs accept any cell outside B∪F. |
| Loss patterns E2/E3 instead of the task's list (e) | Sound and conservative; and vacuous for R_max = 10 (above). "Two simultaneous threats" is impossible from an admissible position. |
| L3(c) not used | Correct decision; L3(c) is false as literally stated (C vacuous for inadmissible F), true for admissible F (Prop. 5.4, which I read but did not re-derive in full — it is not used). |
| Not searched / not established | The task's own tree (eager, busy-only) was not proved; R_max = 9 was not refuted exhaustively; the failure of busy-only moves comes from `bin/search` alone. All three are stated as [open] / single-program in REPORT.md §13 and PROOF.md §9.1. |

### 2.2 My attack on the abstraction (`logs/review/lead/attack.py`)

Own implementation, no project code: lines as arithmetic progressions; the group as the closure of six
generators; canonical form = minimum over all 1152 maps; own admissibility; own Kuhn matching; exact keys.

0. *Geometry.* 148 lines (108/36/4); cell degrees 3/4/7 for 48/144/24 cells; the generated group has 1152
   elements and **equals as a set** the 1152 maps of the SPEC formula — so that set is closed under composition
   and inverse (complete check, not a sample); every map is a line-preserving bijection.
1. *The attack.* σ of SPEC §5 (with §10.1) is played in **real coordinates**, the adversary knows σ and the
   table. Adversaries: `thief` (always takes the cell SP is about to play, following the whole pending reveal
   chain — maximal use of child (ii)); `force` (a 5th stone on a live line whenever possible, otherwise builds
   4- and 3-lines); `danger` (stacks 4 + 4 stones on two live lines without busy cells that meet in a free
   cell — heads straight for child (d)); `hide` (avoids every reveal and stacks 4/3/3 on live lines, so the
   leaf must hold with θ = t); `random`; `mix`. After every step: (A1) |B| = |X| = t, F ⊆ X, X B-admissible;
   (A2) a non-leaf P-state reached in play has t < 10 and its child (a) has a table entry, i.e. **the abstract
   state used by σ is a node of the tree**; (A3) no hole, legal move; (A4) at most one threat, X admissible
   after SP's reply; (A5) at a listed leaf a pairing for (B, real X) exists; (A6) *hostile X*: further
   admissible sets X′ ⊇ F, |X′| = t, stacked 4/3/3 on a random F-connected triple of live lines (the extremal
   shape of Lemma 6.1) must have a pairing; (A7) PAIRING mode to the full board against a greedy
   line-completing FP; (A8) at sampled non-leaf P-states *all* children (c), (d) are generated literally and
   must be VOID, a listed leaf, or an expanded node.

   | run | games | result | events |
   |---|---|---|---|
   | mix (seed 11) | 500 | 0 failures | 52 threats, 2 dangers, 862 reveals (chains up to 4), leaves at t = 8, 9 with θ = 0..8 |
   | thief (12) | 250 | 0 | 2,160 reveals in 2,251 table moves |
   | force (13) | 250 | 0 | 328 threat replies, leaves at t = 8, 9, 10 |
   | hide (14) | 250 | 0 | 188 leaves with θ = 8, 46 with θ = 7 |
   | danger (15) | 300 | 0 | 207 danger replies (child (d) in real play) |
   | random (16) | 150 | 0 | |
   | mix + A8 (21) | 160 | 0 | 258 P-states, 82,877 children (c)/(d): 74,198 expanded nodes, 8,679 leaves, 0 VOID |

   Totals: 1,860 games, 0 failures of A1–A8; 403 threat replies, 209 danger replies, 3,740 reveals (chains of
   up to 4 in one round), leaves reached at t = 8 / 9 / 10 in 1,080 / 762 / 18 games with every θ from 0 to 8;
   9,020 hostile sets X′ (up to 8 free stones), all with a pairing; 254 games played to the full board in
   PAIRING mode (25,286 FP moves; FP got 5 stones on an SP-free line, never 6). Files: `attack_*.json`,
   `run_attack.sh`. (The first six runs were made before A8 and one statistic were added to the script; the
   SP side and A1–A7 are unchanged.)
2. *Outcome.* I found no real position reachable under σ whose abstract state is not a node of the tree, none
   in which the real X is inadmissible at the start of a round or after SP's move, and none in which the real X
   has no pairing at a leaf. This is a test (samples), not a proof; the proof is §2.1(c) plus §2.3.
3. *Attacks by reasoning that I tried and that fail.* (i) F′ inadmissible although X′ is admissible — excluded
   by downward closure. (ii) A table move that lands on an FP stone lying on a live line with 4 revealed
   stones (child (ii) VOID, so no table entry): then X′ has 5 stones on a live line, which is the threat case,
   not the table case. (iii) A leaf entered in the middle of a round: σ and the tree both test leaves only with
   FP to move. (iv) A stabiliser of (B,F) making the translated move ambiguous: every choice of g gives a
   valid move (Lemma 7.4(b)); treecheck itself may verify a symmetric image of the file's move, which is the
   same subtree up to symmetry.

### 2.3 My own verification of hypothesis (H)

* **`lead_expand.cpp`** (own geometry, group, brute-force canonical form over all 1152 maps, exact keys,
  literal admissibility, literal children: all live lines × all e; all pairs of live lines × all 5 × 5
  four-subsets). Result on `results/final` with R_max = 10 (`lead_expand.json`, 44 CPU-s): **ok, 0 errors**;
  153,135 P-states (46,412 expanded, 106,723 leaves), 93,164 Q-states, 246,299 states; 143,498 threat and
  28,275 danger children (non-VOID, with repetitions), 54,331 reveal children, VOID children 0 / 0 / 151
  (threat / danger / reveal), 434 non-busy moves; every one of the 93,164 table lines and 106,723 leaf lines is
  well formed, canonical under my canonical form, unique, and reached. Every counter, in every round, equals
  `results/final/treecheck.json` (`by_t`) and `spotcheck_full.json`. The program does not evaluate LOST; it
  refuses R_max > 10 and relies on the arithmetic fact of §2.1(c).
* **`lead_leaf.cpp`** (own geometry, Kuhn pairing re-validated, augmenting paths from the three lines of every
  F-connected triple of live lines, every flow behind a "true" validated literally, every "false" backed by a
  cut re-evaluated from the definition). Results (`run_leaf.log`, `leaf_*.out`):
  * all 106,723 lines of `leaves.txt`, with the single rule "a triple containing a line whose own gain is ≥ θ
    is skipped": **all LEAF** (44 CPU-s);
  * with **no pruning rule at all**: all 1,912 leaves with θ ≥ 3 (θ = 3: 1,434; 4: 343; 5: 105; 6: 25; 7: 4;
    8: 1), every 4th leaf with θ = 2 (1,817) and every 20th with θ = 1 (1,636): all LEAF (26.5 million triple
    flows). For the single leaf with θ = 8 (`8 0,5,61,93,122,154,210,215 -`) my program and treecheck test the
    same number of triples, 5,652;
  * negative control: on 2,500 perturbed instances (`results/final/checks/perturbed.txt`, every 8th) my verdict
    and reason code equal those of `bin/treecheck --leafcheck` and `bin/phicheck` on all 2,500 (1,707 true;
    reasons 1/2/3 = 57/533/203).
* I re-ran the certifying command `bin/treecheck --table … --leaves … --rmax 10 --exact --paranoid`
  (`treecheck_rerun.json`): ok, complete, 0 errors, and every non-timing key equals
  `results/final/treecheck.json`. The input checksums in that JSON (FNV-1a 68da06fa790d1228 / 350f14ef656250bd,
  5,376,504 / 6,445,022 bytes) equal the checksums I computed from the present files (`recount.py`).

So (V1) LEAF at every reachable leaf, (V2) t < R_max at every expanded node (LOST false by arithmetic),
(V3) a legal table move at every reachable Q-state, and the closure of V have been established by treecheck,
by spotcheck.py, by the treecheck auditor's programs, and now by two programs of my own that share no code
with any of them.

### 2.4 Code versus L4 — what I read, and whether it matches SPEC §4 / PROOF Definition 8.1

`verify/treecheck.cpp` (line numbers of the audited revision):

| item | lines read | matches? |
|---|---|---|
| lines, "two cells on ≤ 1 line", 1152 maps, each a line-preserving bijection (asserted at every start) | 258–411 | yes. Closure under composition is not asserted there; not needed for the soundness of treecheck, proved in PROOF 7.2, verified completely by me |
| canonical form = least image under the 1152 maps, B first then F, order of SPEC 2.4 | 152–160 (`cmpSet`), 473–535 | yes; whatever is returned is the image under one map `gb` (531–533), so two inequivalent states can never be identified |
| table / leaf loading: shape checks, re-canonicalisation, move mapped with the same map | 1373–1409, 1419–1443, 1445–1515 | yes (move translated consistently, 1440). Look-ups are 120-bit hashes — harmless, see finding T1 |
| admissibility: B∩X = ∅, cap 4 on live lines, danger pair = two live lines with exactly 4 stones sharing a cell not in X | 602–617 | yes ("exactly 4" after the cap test; the 2-cell complements of two 4-lines meet iff they share a cell outside X) |
| LEAF: admissible, pairing built and validated literally, θ ≤ 0, strong lines, strong pairs, all remaining F-connected triples, every flow validated (`--paranoid`) | 623–653 (argument), 690–812, 815–1073 | yes; the triple enumeration (1028–1070) is complete: paths at their centre, pairwise adjacent triples at their smallest line |
| children (a), (c), (d) with the exact conditions | 1307–1366 | yes: (c) live l, e ∉ F, \|F∪(l∖{e})\| ≤ t+1 (1319–1331); (d) c ∉ B∪F, unordered pairs of live lines through c, left-out cells ≠ c and ∉ F, \|F∪Qa∪Qb\| ≤ t+1 (1335–1364); the two early `continue`s are exact identities |
| VOID test with the child's own B; only place where a child is dropped | 2119–2137 | yes |
| P-state: shape, own VOID test, **leaf look-up first and LEAF recomputed for every listed leaf reached**, else t < R_max, LOST false, all children pushed | 2151–2206 | yes; every early return except "listed leaf and LEAF holds" adds an error |
| Q-state: table entry, m ∉ B∪F, child (i) (error if VOID), child (ii) iff \|F\| < t unless VOID | 2207–2248 | yes |
| termination / `ok`: every queued state processed exactly once, `complete`, error count, bad lines | 2251–2298, 2356–2385 | yes; `table_conflicts` is not part of `ok` (finding T2; 0 here) |
| options, R_max, root | 3864–4012 | yes; `--paranoid` is the default, `--exact` switches the visited set to full keys (1972–1986) |
| LOST (E2/E3) | 1077–1088 only | **not read** (not load-bearing for R_max = 10; the auditor compared it with a literal transcription on 800 states) |

`verify/spotcheck.py`:

| item | lines read | matches? |
|---|---|---|
| lines, group (SPEC formula), self-test of bijectivity / line preservation | 46–108, 1013–1040 | yes (closure only sampled: 300 products) |
| canonical form (greedy over cell ids, B then F; result is the image under the surviving maps) | 131–161 | yes |
| admissibility | 180–196 | yes |
| LEAF: pairing (Kuhn), single lines, weak adjacent pairs, triples from their smallest adjacent pair, cut witness verified by counting | 199–386 | yes; I checked the completeness argument (a triple all of whose adjacent pairs are weak is tested from its smallest adjacent pair; a triple with a strong adjacent pair needs no test) |
| LOST | 418–546 | read once, not checked clause by clause (not load-bearing) |
| file checks (ascending lists, sizes, move outside B∪F, duplicates, canonical keys) | 657–701, 1220–1276 | yes |
| P-state: VOID test, leaf look-up, t < R_max, children (a), (c), (d), VOID test with the child's B | 763–860 | yes |
| Q-state: table entry, legality, (i), (ii) iff \|F\| < t unless VOID | 863–915 | yes |
| traversal, LEAF on all leaf lines (`--all-leaves`), `ok` = no error of any kind | 1278–1297, 1299–1385, 1403–1438 | yes. The skip `ck[1][0] > limit` (1374) cannot fire in `--full` mode (limit = R_max and no state with t > R_max is ever generated without an error) |

`sim/sim.cpp` (SP side only; lines 965–1028, 1054–1097, 1171–1315 of the version of 08:18, which are lines
961–1024, 1050–1093, 1167–1311 of the snapshot used for the official run): threat → danger → table look-up with
reveals → admissibility test after SP's move; PAIRING rule of SPEC §10.1; leaf test at the start of a round;
FP-line test after every FP move. Matches SPEC §5. Adversaries and statistics not read.

Symmetry handling, summarised: canonical form = least image pair under the 1152 maps in all programs (and in
my own, by brute force: all 199,887 file lines are canonical); the table move lives in the canonical frame of
its key and is applied there by the verifiers; σ translates it back with g⁻¹ for a g attaining the canonical
form; the 1152 maps are line-preserving bijections (asserted by every program) and form a group (PROOF 7.2;
my complete check).

Stop rule and R_max, summarised: leaves are P-states only; LEAF(B,F,t) is recomputed for every listed leaf
(all 106,723 are reachable and recomputed by treecheck, spotcheck.py and `lead_leaf`); a non-leaf P-state with
t ≥ 10 is an error in all programs; leaves occur at t = 8, 9, 10 only.

### 2.5 Evidence files opened and recounted (scripts `recount.py`, `recount_ip.py`, `triangle_k10.py`)

* `strategy.tbl`: 93,164 lines, 93,164 distinct keys, all well formed; by t = 1..10: 2 / 5 / 14 / 35 / 101 /
  446 / 3,487 / 14,362 / 62,381 / 12,331; 434 non-busy moves (16 / 399 / 11 / 8 at t = 7 / 8 / 9 / 10).
* `leaves.txt`: 106,723 lines, all distinct and well formed; by t: 3,382 (8), 87,314 (9), 16,027 (10); by
  θ = t − |F|: 64,841 / 32,703 / 7,267 / 1,434 / 343 / 105 / 25 / 4 / 1 for θ = 0..8 (106,245 with θ ≤ 3, 478
  with θ ≥ 4). Equal to PROOF §9.1 and REPORT §3.
* `results/final/stats.json` (search: proved, R_max 10, selfcheck ok), `treecheck.json`,
  `treecheck_crosscheck.json` (ok, complete, root "0 - -", not VOID, exact visited set, validated flows,
  0 non-canonical lines, 0 conflicts; with `--crosscheck` 246,299 brute-force canonical forms, 46,412 literal
  expansions, 2,153 leaves by unpruned reference), `spotcheck_full.json` (ok, 0 errors, all 106,723 leaves
  recomputed, children a/c/d/i/ii = 46,412 / 143,498 / 28,275 / 93,164 / 54,482, 151 VOID, 0 unreached lines).
* `checks/enum_theta{0,1,2,3}.jsonl`: 64,841 + 32,703 + 7,267 + 1,434 = 106,245 records, each matched to a line
  of `leaves.txt` with the right θ, all `holds` with status 1 (exact enumeration of C).
* IP on leaves (`ip_leaf_sample.jsonl`, `ip_leaf_theta_ge4_rest.shard*.jsonl`): 1,308 leaves with an IP record,
  all "holds": 300 / 300 / 150 / 80 sampled for θ = 0..3 and **all 478 leaves with θ ≥ 4**.
* `results/crosscheck/` recounted from `certify.jsonl` and the IP shards (final; the IP processes had ended
  when I recounted at 08:35 UTC): 1,291 instances; certifier 656 holds / 623 fails / 12 void; IP agrees on
  652 holds, 623 fails, 12 void (infeasible); 4 "holds" instances undecided by the IP (300 s time-outs, all
  F = ∅); **0 disagreements**. `summary.json` (08:25) says the same; it also records 488 instances with θ ≤ 3
  decided identically by `enumcheck` and `certify`, and 160 instances with LEAF false but C true.
* Leaf table: max-flow `phi_k{8,9,10}.jsonl` 6 / 172 / 0 LEAF among 1,057 / 1,669 / 2,468 orbits; IP
  `results/ip/leaftable_k*`: 6 of 6 (k = 8) and 262 of 947 (k = 9; 254 fail, 8 hold), all equal to the max-flow
  table; 685 orbits of k = 9 and all of k = 10 not run by the IP. Own check: each of the 2,468 orbit
  representatives of busy 10-sets leaves a live triangle, so C fails there by Remark 6.3.
* `results/regression/ip_regression.json`: 70 records, 70 pass; `minstones.json`: 9 / 7 / 3 / 0 for
  n = 5 / 6 / 7 / 8, lower and upper bounds complete.
* Simulator: see §4 (v).

## 3. Findings and dispositions

Dispositions: **fixed**; **documented** (known minor issue, recorded in SPEC §10 / PROOF / REPORT);
**irrelevant** (cannot affect the truth of the theorem and needs no action); **open**.

### 3.1 Findings of the reviews and of `logs/spec_issues.md`

| # | source | finding | disposition (verified by me) |
|---|---|---|---|
| M1 | math referee 1 | Theorem 0.2 named σ imprecisely (PAIRING rule), legality of σ not stated | fixed: Theorem 0.2 / Def. 8.3 / Thm 8.10(a); SPEC §10.1 |
| M2 | math referee 2 | "capacity ≥ 6" wrong for Prop. 4.3(c) | fixed in PROOF (lines 330–335, 367–373) |
| M3 | math referee 3 | existence claims in Remarks 3.3, 8.8 labelled PROVED | fixed: labels split, hand-checkable instance added |
| M4 | math referee 4 | "treecheck verifies validity" is a property of a program; `--exact` needed | fixed in PROOF §0.2, §9.2; the certifying run has `exact_visited_set: true` |
| M5 | math referee 5 | Lemma 1.1 uses \|B\| = \|X\| | fixed |
| M6 | math referee 6 | REPORT.md cited but missing | fixed: REPORT.md exists and is complete in its version of 08:37 (see L2 for one stale sentence) |
| M7 | math referee 7 | toy test hardly exercises VOID | fixed (Appendix A "Limits"); moreover no child (c)/(d) of the real tree is VOID |
| S1 | search audit 1 | 112-bit transposition keys in `bin/search`; emit walk trusts flags | irrelevant: the search is an untrusted generator; the certificate is verified with exact keys by spotcheck.py and by `lead_expand` |
| S2 | search audit 2 | `leafScreen` comment / code mismatch (completeness only) | irrelevant (can only turn WON into FAILED) |
| S3 | search audit 3 | `lost_true: 0` is no test of `lostCheck` | documented (PROOF §9.1, REPORT §2); LOST is vacuous at R_max = 10 |
| S4 | search audit 4 | `leaf_holds` true for k < \|F\| | irrelevant: unreachable; verifiers enforce \|F\| ≤ t; spotcheck.py and `lead_leaf` return "false" (reason 4) |
| S5 | search audit 5 | `calloc` of atomics (formal UB) | documented minor; untrusted generator |
| S6 | search audit 6 | NOTES.md points to an older tree; binary older than a header | documented; the tree was reproduced byte for byte by the auditor and by `scripts/reproduce.sh` (`logs/final/reproduce_validation.log`: "tree identical to results/final") |
| T1 | treecheck audit 1; spec_issues (proof writer rev. 1, audit 1) | SPEC §10.6 said look-ups are exact | fixed (SPEC §10.6 reworded). Residual hashing in look-ups is harmless, and my exact-key re-expansion uses exactly the same entries |
| T2 | treecheck audit 2 | `table_conflicts` not part of `ok` | documented (SPEC §10.7); 0 in the certified run; my loader and spotcheck.py find no duplicate key |
| T3 | treecheck audit 3 | stale `--out` file if the program aborts | documented (SPEC §10.7); I checked the input checksums in the JSON against the files and re-ran the command |
| T4 | treecheck audit 4 | read errors treated as end of file | documented; sound direction only |
| T5 | treecheck audit 5 | lenient numeric option parsing | documented; `"rmax":10` is recorded in the JSON |
| T6 | treecheck audit 6 | VOID `--root` yields ok | documented (SPEC §10.7); root "0 - -", `root_void: false` |
| T7 | treecheck audit 7 | the real run is blind to LEAF / LOST defects and to dropped children except through counters | documented; closed in substance by independent LEAF recomputations (spotcheck.py, auditor, `lead_leaf`) and independent re-expansions with identical per-round counters (spotcheck.py, auditor, `lead_expand`) |
| T8 | treecheck audit 8; task 6c note 1 | non-canonical lines accepted | documented; 0 / 0 in the run; all lines are canonical under four independently written canonical forms (treecheck, spotcheck.py, the auditor's, mine) |
| I1 | spec_issues task 3, items 1–4, 7 | IP output conventions; "holds" rests on a floating-point bound; time-outs | documented (SPEC §7.7, §10.4); the IP is not part of the proof |
| I2 | spec_issues task 3, item 5 | regression item 9: worst ranges over 2..12, not 4–8 | documented (REPORT §6); "mostly bad" holds (20 of 20 bad) |
| I3 | spec_issues task 3 item 6, task 6b item 4 | IP much slower than the task statement expects | documented; consequence: partial IP coverage (§5) |
| I4 | spec_issues tasks 4+5, item 1 | busy-only table moves cannot prove the root | documented (SPEC §10.2); established by `bin/search` only (§5) |
| I5 | spec_issues tasks 4+5, item 2; task 6c item 6; task 6b item 2; task 7 item 5 | extra options / JSON keys | irrelevant |
| I6 | spec_issues proof writer 1 | L3(c) false as literally stated | documented (PROOF Prop. 5.4); not used |
| I7 | proof writer 2 | LEAF condition 2 redundant | documented (SPEC §10.4) |
| I8 | proof writer 3; task 7 item 1 | PAIRING rule needs "if it is empty" | fixed (SPEC §10.1, Def. 8.3); sim and my `attack.py` implement it |
| I9 | proof writer 4; task 6c item 4; rev. note 2 | E2/E3 necessary, not sufficient; E3 needs ≥ 13 on this board | documented (SPEC §10.3, Remarks 8.8, 8.4b); conservative direction |
| I10 | proof writer 5 | canonical look-ups agree only because G is a group | fixed (Lemma 7.2); complete machine check by me |
| I11 | proof writer 6; tasks 2+6a | `enum_check` exact because the pairing is examined at every node | documented (SPEC §10.5, Remark 2.3) |
| I12 | proof writer 7; task 3 item 1; tasks 2+6a; task 6b item 1 | C vacuous for inadmissible F; "void"/"infeasible" conventions | documented (SPEC §10.4) |
| I13 | task 7 items 2–4 | simulator conventions (tie-break, illegal table move, two threats reported as admissibility failure) | irrelevant; with ≤ 10 stones there is never more than one forcing event |
| I14 | task 6c item 5 | `void_skipped` 151 (treecheck) versus `void_children` 0 (search) | documented (REPORT §11); my count: 0 / 0 / 151 |
| I15 | task 6c items 2, 3 | malformed lines fail; non-busy moves accepted | irrelevant (intended) |
| I16 | task 6b items 3, 5, 6 | branch-and-bound fact proved in `certify.cpp`; scheduling; LEAF strictly weaker than C for F ≠ ∅ | irrelevant to the theorem (cross-check program); 160 such instances listed in `summary.json` |
| X1 | third implementation (spotcheck) | LOST cannot fit when R_max = 10 | fixed: Prop. 8.4a, SPEC §10.3 |
| A0 | simulator audit (`logs/review/sim_audit.md`, final version of 08:37; a time-boxed second agent wrote it after the first audit agent had been stopped without a report, PROGRESS 08:25) | verdict sound-with-minor-issues, no fatal or major finding: the SP side conforms to SPEC §5 in 14 points; no path on which a game is dropped without being counted; 9 seeded mutants of files / strategy all detected | — (I read the SP side myself, §2.4, and agree with the conformance table) |
| A1 | sim audit 1 | `sim/sim.cpp` and `bin/sim` changed twice during the audit | fixed: at 08:38 `cmp` shows `sim/sim.cpp` and `bin/sim` byte-identical with the snapshot of the official run (see L1) |
| A2 | sim audit 2 | holes met by the adversaries' table predictor (`predict_table`) are not counted | irrelevant to the theorem; my `attack.py` asserts exactly this (A2: every non-leaf P-state reached has its child (a) in the table) and `lead_expand` verifies it for all 46,412 expanded P-states |
| A3 | sim audit 3 | the official run was made without `--verify-canonical` and `--paranoid` | documented; all 199,887 lines are canonical under four independently written canonical forms, the simulator found an entry at every look-up (0 holes), and the audit's 4,000-game paranoid run with the snapshot binary found 0 non-canonical entries and 0 paranoid failures |
| A4 | sim audit 4 | coverage is partial: 84.75 % of the table keys and 76.75 % of the leaf keys were met in real games; 14,207 table entries and 24,816 leaves (almost all with t = 9, 10 and \|F\| ≥ 8) were never played | documented (REPORT §9); the simulator is a sampling test, complete coverage comes from the verifiers |
| A5 | sim audit 5 | `mix200k.json` and other audit runs with seed 20261112 repeat games of the official run and are not independent samples | documented; I count only the official 10⁶ games (plus my own 1,860 with a different program) |
| A6 | sim audit 6 | the exit code of the official run is not in `logs/final/sim_mix_1M.log` | documented; exit 0 is implied by the JSON (all bug counters 0, `game_count_ok: true`) |

No finding of any review is both open and able to affect the truth of the theorem.

### 3.2 My own findings

| # | severity | finding | disposition |
|---|---|---|---|
| L1 | minor (process) | The 10⁶-game simulator run was made with `results/final/sim/sim_snapshot_0806.{cpp,bin}` (md5 of the source `c2112bb9…`) while `sim/sim.cpp` was being edited (08:18, 08:29, 08:33). I diffed snapshot and working source at 08:19, 08:25 and 08:35: every hunk lay in the header comment, the statistics (snapshot lines 838–881), the adversaries (1833–2315) or the usage text / `main`; **no hunk touched snapshot lines 882–1832**, which contain `lookup_q`, `lookup_leaf`, `find_danger`, `admissible_counts`, `forced_reply`, `round_start`, `play_impl`. | fixed: at 08:38 `sim/sim.cpp` and `bin/sim` are byte-identical with the snapshot (`cmp`) |
| L2 | minor (editorial) | REPORT.md was a draft with placeholders while I reviewed (versions of 08:08 and 08:29), and its version of 08:37 still said that the simulator snapshot "differs from the final `sim/sim.cpp`" although the two were identical by then. The version of 08:38 (sha256 `d6994b099ae4b7f0…`) has no placeholder and says "byte-identical"; the numbers I checked in it (IP comparisons 45 + 268 + 1,308 + 1,275 = 2,896; simulator block; certificate table) equal my recounts, and its §11 quotes this sign-off and its limits correctly. | fixed |
| L3 | minor (label) | REPORT §7 (versions up to 08:29): "\|B\| = 10 is all false because a live triangle of lines always exists … [proved] (PROOF §6)". PROOF Remark 6.3 proves only "if a live triangle exists, C fails for k ≥ 10". That every busy 10-set leaves a live triangle is computed (leaf table; confirmed by `triangle_k10.py`: 2,468 of 2,468), not proved. | fixed in the REPORT version of 08:37 (implication [proved], existence [computed]); the table is not used by the proof |
| L4 | minor (wording) | REPORT §1 and PROGRESS call the result "PROVED (computer-assisted)". Accurate only with the qualification that follows there (PROOF.md + one correct verifier + compiler/interpreter + hardware). The eager busy-only tree of the task statement itself was **not** proved; REPORT §13 says so. | documented; keep both qualifications next to the headline |
| L7 | minor (process) | PROOF.md was edited by the coordinator at 08:37:48, after I had reviewed it (new sha256 `57f336cf0ef2c3b0…`, 1,083 lines; revision-log item 13 "status phrases only"). I did not take that on trust: `proof_diff_check.py` undoes the five status phrases ("REPORT.md not yet written", "audit in progress") and the appended item 13 and obtains a file **byte-identical** with the version I reviewed (sha256 `865e1b34…`). No statement and no proof changed. Item 13's count "five programs of four authors" comes from an earlier wording of this file; precisely: treecheck, spotcheck.py, the auditor's `aexpand` + `aleaf`, my `lead_expand` + `lead_leaf`, by four authors. | documented here; my sign-off covers the mathematics of both versions |
| L5 | note | PROOF.md does not state the corollary "with ≤ 10 FP stones at most one forcing event exists" (two danger pairs sharing a line also need ≥ 11 stones). Not needed (Lemma 8.7 covers it), but it makes the tie-break of SPEC §5 step 2 and LOST vacuous for the certified tree in one line. | suggestion only |
| L6 | note | `spotcheck.py` and `treecheck.cpp` verify closure of the group only by sample / not at all at run time. | irrelevant for their soundness; closure proved (Lemma 7.2) and verified completely by the referee and by me |

## 4. Acceptance criteria of the task statement

| | criterion | status | evidence I opened / produced |
|---|---|---|---|
| (i) | every regression value of SPEC §8 reproduced | **PASS** | `results/regression/ip_regression.json` (70 of 70 records pass: items 2–9), `minstones.json` (item 1: 9 / 7 / 3 / 0). Item 9 is qualitative: 20 of 20 random busy 8-sets bad, worst 2–12, 16 of 20 in 4..8. LEAF is true on all 21 instances with worst ≤ 0 (so those zeros are exact); values ≥ 1 have integer-verified witnesses as lower bounds and a HiGHS bound as upper bound |
| (ii) | the AND–OR search proves a second-player win from the empty board | **PASS** — for the lazy-revelation tree of SPEC §4 with non-busy table moves, R_max = 10 (not for the eager busy-only tree of the task statement, which was not searched) | `results/final/stats.json` (`proved: true`); verified by `treecheck.json`, `treecheck_crosscheck.json`, `spotcheck_full.json`, my re-run `treecheck_rerun.json`, and my own `lead_expand.json` + `lead_leaf` |
| (iii) | every leaf certified by an exact method | **PASS** | integer max-flow LEAF on all 106,723 leaves by treecheck, spotcheck.py, `src/phi.hpp`, the auditor's `aleaf` and my `lead_leaf` (five implementations); exact enumeration of C on all 106,245 leaves with θ ≤ 3 (`checks/enum_theta*.jsonl`, recounted) |
| (iv) | IP and exact certifier agree wherever both were run | **PASS, with partial IP coverage** | cross-check: 1,287 of 1,291 instances decided by both (incl. 12 void), 0 disagreements, 4 IP time-outs; leaves: 1,308 IP "holds" including all 478 with θ ≥ 4; leaf table: 268 of 268; regression: 45 instances. Not run by the IP: 685 of 947 orbits for k = 9, all of k = 10, 105,415 leaves with θ ≤ 3 (those are covered by exact enumeration) |
| (v) | the simulator finds no loss in ≥ 10⁶ games | **PASS** | `results/final/sim/sim_mix_1M.json` (written 08:34:12; `logs/final/sim_mix_1M.log`; `game_count_ok: true`): 1,000,000 games completed (108,000,000 rounds, i.e. every game to the full board), seed 20261112, adversary mix (random 30,228 / greedy 29,735 / steal 40,256 / threat 49,882 / search 20,019 / danger 89,871 / double 90,004 / corner 150,084 / tree 499,921 games); `fp_lines` 0, `holes` 0, `pairing_failures` 0, `admissibility_failures` 0, `illegal_table_moves` 0, `pairing_invariant_failures` 0, `internal_errors` 0, `lookahead_bugs` 0, `file_anomalies` 0, 0 failures in 1,859,012 leaf stress tests; `sim_mix_1M_bugs.jsonl` is empty. 1,121,484 threat events, 85,806 danger events, 2,254,365 reveals, leaves at rounds 8 / 9 / 10 in 117,835 / 832,519 / 49,646 games, `leaves_after_threat_and_danger` 0 (as §2.1(c) predicts). Caveats: run without `--paranoid` / `--verify-canonical` (A3); coverage 84.75 % of table keys and 76.75 % of leaf keys (A4); the simulator audit (final, 08:37) has no fatal or major finding (A0). Independent of it: my 1,860 games with my own implementation of σ (§2.2) |
| (vi) | sign-off of task 8 | see §6 | this file |

## 5. Residual risks and what remains unverified

1. **Trusted base.** The theorem rests on PROOF.md (§§0–3, 7, 8 and Lemma 1.1, re-derived by the referee and
   by me) and on (H), which is a computation. (H) has now been established by the programs of four independent
   authors (treecheck; spotcheck.py; the auditor's `aexpand` + `aleaf`; my `lead_expand` + `lead_leaf`) plus
   the search's own self-check, in two languages. A common-mode error would have to be a shared misreading of
   SPEC §4 — which I compared with PROOF §8 clause by clause — or a fault of g++ 13.3 / CPython 3.13 / the one
   machine everything ran on. No formal (proof-assistant) verification exists. My own programs were written in
   about half an hour and tested only against the others (2,500 negative controls for `lead_leaf`).
2. **Floating point.** All HiGHS results ("holds" answers in particular) rest on floating-point bounds. None
   is used by the proof; every leaf is certified by integer max-flow, and 99.55 % of the leaves also by exact
   enumeration of C. For the 478 leaves with θ ≥ 4, C itself is certified only through LEAF ⇒ C (proved) plus
   HiGHS (computed).
3. **Partial IP coverage.** k = 9 leaf table 262 of 947 orbits, k = 10 not run, 4 cross-check instances timed
   out, most θ ≤ 3 leaves only sampled.
4. **R_max = 9 not refuted** (non-exhaustive failure with ≤ 6 candidates per state); minimality of R_max and of
   the table is open and irrelevant to the theorem.
5. **The task statement's own tree was not proved.** Neither the eager branch (b) nor busy-only table moves were
   used. Busy-only moves fail for R_max ≤ 10 in the lazy game according to `bin/search` alone (one program; the
   hand argument about the 14 lines of a face with four revealed corners, sur_F = 4, explains the failure for
   t > 8 but I did not re-run or re-derive the exhaustive part). The eager busy-only variant was not searched.
   The theorem does not depend on any of this.
6. **LOST code** of all programs is unexercised by this certificate and was not read by me in treecheck. It
   becomes load-bearing only for R_max ≥ 11.
7. **Not read / not audited by me:** `src/search.cpp`, `src/phi.hpp`, `src/match.hpp`, `src/canon.hpp`,
   `src/certify.cpp`, `py/*.py` (generator and cross-check programs; their outputs were recounted, not their
   code), the adversaries of `sim/sim.cpp`, the reference / self-test sections of treecheck (1525–1940,
   2400–3863), Prop. 5.4(ii) and Appendix A of PROOF.md (not used by the theorem).
8. **Simulator.** Evidence, not proof: 10⁶ games sample an astronomically larger space and met 85 % of the
   table keys and 77 % of the leaf keys. Its 128-bit hashed keys are immaterial. The audit of the simulator was
   time-boxed (about 10 minutes for the final report); its adversaries were not read line by line by anyone
   but their authors, which limits coverage claims, not the detection of SP-side bugs.
9. **Documentation.** The project's documents changed while I was reviewing them (REPORT.md four times,
   `sim/sim.cpp` four times, `sim_audit.md` twice, PROOF.md once — status phrases only, verified byte for byte,
   L7); everything in this file refers to the versions and times stated. Any later change to PROOF.md, SPEC §3–§5
   or the two certificate files is outside this sign-off.

## 6. Sign-off

**SIGNED OFF**

What I sign. *Let SP play the strategy σ of PROOF.md Definition 8.3 (SPEC §5 with the PAIRING rule of SPEC
§10.1) using the table `results/final/strategy.tbl` (sha256 `13629393fcdcee87…9ce3fc`, 93,164 entries) and the
leaf file `results/final/leaves.txt` (sha256 `3c14c7814e296e81…0fd470`, 106,723 leaves), R_max = 10. Then σ
always prescribes an empty cell, and in no play does FP ever own all six cells of one of the 148 lines of the
6×6×6 board. Consequently, by strategy stealing and determinacy, 6×6×6 tic-tac-toe is a draw.*

Why I am personally convinced.

1. I re-derived the chain "root WON ⇒ σ works ⇒ draw" (Lemma 1.1, Theorem 3.2, §7, Lemmas 8.5–8.7,
   Proposition 8.4a, Theorems 8.9 / 8.10, Proposition 0.3, Corollary 0.4) and found no gap. For R_max = 10 the
   argument is even simpler than written: with at most 10 FP stones there is never more than one forcing event.
2. Hypothesis (H) — the only computed ingredient — holds for these two files according to two programs I wrote
   myself for this review (`lead_expand.cpp`: complete re-expansion with exact keys, 246,299 states, 0 errors;
   `lead_leaf.cpp`: LEAF on all 106,723 leaves by validated integer flows), in agreement, counter by counter
   and round by round, with `bin/treecheck --exact --paranoid` (which I re-ran and whose obligations I read
   line by line), with `verify/spotcheck.py` (read) and with the treecheck auditor's independent programs.
3. My own implementation of σ, played in real coordinates against adversaries built to break the abstraction
   (1,860 games), and the project's simulator (10⁶ games) found no hole, no inadmissible position, no leaf
   without a pairing and no FP line.

Conditions and limits of this sign-off.

* It is a computer-assisted result: it holds provided PROOF.md is right (refereed, and re-derived by me) and at
  least one of the independent verifications of (H) is right, including its compiler or interpreter and the
  single machine on which everything ran. Nothing was formally verified.
* It applies to the two certificate files with the checksums above. A regenerated tree must be re-verified.
* It is **not** a sign-off of the tree the task statement describes (eager branch (b), busy-cell table
  moves): that tree was not proved. What was proved is a different, sound tree (lazy revelation, 434 non-busy
  table moves). Nor does it cover the minimality of R_max, the claim that busy-only moves cannot work (one
  program only), or any HiGHS result (floating point, cross-check only).
* All six acceptance criteria are met as stated in §4: (i) PASS, (ii) PASS for the lazy tree, (iii) PASS,
  (iv) PASS with partial IP coverage, (v) PASS (`results/final/sim/sim_mix_1M.json` is present; 10⁶ games, no
  loss), (vi) this sign-off. No condition on missing evidence remains.
* Open items when I closed this review (08:41 UTC): none. Everything I raised that needed action (L1, L2, L3,
  the unfinished simulator audit) had been resolved by then and was re-checked by me; the late edit of PROOF.md
  (L7) changes no statement. The known minor issues of §3.1 marked "documented" stay as they are.

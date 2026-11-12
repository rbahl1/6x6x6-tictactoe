# Mathematical referee report on PROOF.md

Reviewer role: hostile mathematical referee (review part A). Date 2026-11-12, about 06:28–06:53 UTC.

Version reviewed: `/home/user/ttt6/PROOF.md`, 762 lines, mtime 06:32:36 UTC,
sha256 `5afc08cdb90597d1affa80ae4e919e3cf9956ebec3cd0d1a06f5180f7ecc4fbc`.
The file was edited by its owner twice while I was reading it (06:11 version → 06:32 version: Appendix A
filled in, status item 1 and 4(g) added, wording of Lemmas 0.1, 1.2(c), 7.2, 8.2(a), Remark 2.3 touched).
I re-read the complete 06:32:36 text; everything below refers to it.

**Verdict: sound-with-minor-issues.** I found no error in any numbered statement and no gap in the chain
(H) ⇒ Theorem 0.2 ⇒ Corollary 0.4. All findings are about wording, labels and what the text claims about
programs. **I made no edit to PROOF.md** (no pure typo found; the file was being edited concurrently by its
owner, and every change I would make touches the content of a statement, so it is listed as a finding with
proposed text instead).

## 1. Method

1. Every numbered statement (0.1–0.4, 1.1, 1.2, 2.1–2.3, 3.1–3.3, 4.1–4.4, 5.1–5.4, 6.1–6.3, 7.1–7.4,
   8.1–8.12) was re-derived by hand before its proof was read, then the given proof was checked line by line.
2. SPEC §4 (tree) and §5 (strategy) were compared clause by clause with PROOF §8.1, Definition 8.3 and the
   literal checks inside Lemmas 8.6 and 8.7.
3. Counterexample search with throw-away scripts on the real 6×6×6 lines (own geometry `ref_geom.py`,
   identical line list to `py/geom.py`; nothing from `src/` or `logs/proof/` except where stated). Total
   compute about 5 CPU-minutes, all at nice 10.
4. Appendix A numbers were compared with the raw logs in `logs/proof/` (sums over the runs).
5. The core loop of `verify/treecheck.cpp` (`process`, `pushChild`, header comment) was read to compare what
   it checks with Definition 8.1. Nothing else of any program was audited.

## 2. Findings

| # | severity | location | what is wrong | recommended fix |
|---|---|---|---|---|
| 1 | minor | Theorem 0.2, Corollary 0.4 (statement) | The theorem is stated for "the strategy σ of SPEC §5". What is proved (Theorems 8.9, 8.10) is σ of Definition 8.3, i.e. SPEC §5 **with the PAIRING-mode rule of Lemma 1.1**. The literal SPEC §5 rule ("take the other cell assigned to the same line") prescribes an occupied cell when the partner already holds an SP filler stone; in my playouts from B ⊇ T8 this happened 3 410 times in 23 218 SP replies. The file says so in "Read this first" 4(b) and in Definition 8.3, but the main statement does not. The statement also omits that σ is a legal, total strategy (it is: Theorem 8.9 "every cell σ prescribes is empty", Lemma 1.1). | Theorem 0.2: "… If SP plays the strategy σ of Definition 8.3 (SPEC §5 run with the table and leaf file of a valid certificate, PAIRING-mode rule as in Lemma 1.1), then σ always prescribes an empty cell and at no moment does FP own all six cells of a line." Ask the coordinator to amend the PAIRING sentence of SPEC §5 (already in `logs/spec_issues.md`, proof-writer item 3). |
| 2 | minor | Proposition 4.3, definition of 𝒩_U: "(any capacity ≥ 6 gives the same)" | True for the flow value, i.e. for (a), (b), (d). False for (c): its proof uses "the arcs s→l (l ∈ U) are never saturated". With capacity exactly 6 and F∩l = ∅ a line l ∈ U can hold all six of its cells; then s→l is saturated, l is not reachable from s, and "U ⊆ R", "sur_F(R) = μ(U)" fail (U = {l}, l able to collect 4 extra cells: R = ∅, μ(U) = 4). `src/phi.hpp` uses unbounded capacity and verifies its witness from the definition, so no program is affected. | "(any capacity ≥ 6 gives the same maximum flow value, hence (a), (b); part (c) needs these arcs unsaturated, i.e. capacity ≥ 7, or R := lines reachable from s together with U)". |
| 3 | minor | Remark 3.3 (label PROVED), part (iii) "instances with C true and LEAF false occur, Appendix A"; likewise Remark 8.8(iii) "E3 may fit although SP has a saving move" | Two existence claims inside remarks labelled PROVED rest on program runs (Appendix A: 246 instances on generalised / restricted boards; 622 observations). The implications are proved; that such configurations exist is COMPUTED. | Label the existence clauses COMPUTED, or give a hand-checkable instance. For 3.3(iii) I verified this one (by hand and by enumerating all 45 292 admissible X): T = the three lines through cell 0 in the plane x = 0 ({0,…,5}, {0,6,12,18,24,30}, {0,7,14,21,28,35}), B = the 200 cells outside N(T), F = ∅, k = 11. The live lines are exactly those of T; sur_∅(T) = 16 − 6 = 10 < 11, so LEAF(B,∅,11) is false. Every B-admissible X lies in N(T) and has at most 10 cells (common cell p ∈ X: the three line counts sum to \|X\|+2 ≤ 12; p ∉ X: at most one line has 4 stones, \|X\| ≤ 4+3+3), so \|N(T)∖X\| ≥ 6 = 2\|T\|; one or two lines never violate Hall (Lemma 2.2). Hence C(B,∅,11) is true. |
| 4 | minor | §0.2 "Validity of the certificate files is exactly what `bin/treecheck` verifies"; §9 "the trusted base is `verify/treecheck.cpp`, its compiler and the machine" | (a) This is a claim about a program inside a mathematical text; it is not proved here and I could only check it by reading `process()` / `pushChild()` (their control flow does match Definition 8.1: state shape, leaf ⇒ LEAF recomputed, else t < R_max and LOST false, every child emitted by `genChildrenP` queued unless VOID, table entry present and outside B∪F, child (i), child (ii) iff \|F\| < t and not VOID; I did not read `genChildrenP`, the LEAF and LOST routines or the canonical form). (b) By its own header, treecheck keys the visited set by a 128-bit hash unless run with `--exact`; a collision would make it skip a state. §9 does not mention this probabilistic step. | §9, "What is not proved": add "the certifying treecheck run must use `--exact` (full keys); otherwise (H) additionally rests on the absence of a 128-bit hash collision (probability < 1e-22 by the program's own estimate)". Keep the sentence in §0.2 but label it as a property of the program to be established by the code audit, not by this file. |
| 5 | minor | Lemma 1.1, first sentence of the proof ("After each FP move an odd number ≤ 215 of cells is occupied") | Uses \|B\| = \|X\|, which is not in the hypotheses of the lemma as written ("a position with FP to move, SP stones B, FP stones X"). True for every game position with FP to move, and all the proof needs is "if the board is not full SP has a legal move; if it is full the game is over". | Add "(FP is to move, so \|B\| = \|X\|)" or replace the sentence by the weaker one. |
| 6 | minor | "Read this first" item 1 | Cites REPORT.md as the authoritative record; that file did not exist when I reviewed (06:50 UTC). The numbers quoted (153 135 P-states, 106 723 leaves, 93 164 Q-states, treecheck ok) agree with PROGRESS.md 06:18 but I did not verify them. | Update when REPORT.md exists; no mathematical content. |
| 7 | minor | Appendix A, `toy_tree.py` paragraph and "Limits of these checks" | The end-to-end toy test hardly exercises the VOID rule: among the logged runs with LOST on and a WON root, VOID children (c)/(d) occur in one tree only (2 children; `toy_run1.log`, start `['3','8']`); all other WON trees report `'void': 0`. The appendix lists other limits but not this one. The rule itself is proved (Lemma 2.1(a) with Lemmas 8.6, 8.7) and I found no way to break it. | Add to "Limits": "VOID children occurred in only one of the WON toy trees; the VOID rule is covered by the hand proof and by `bin/sim` on the real tree." |

No fatal and no major finding.

## 3. The eight points of the brief

**1. Section 1 (L1, L2).** Lemma 1.1 is correct. Invariant (∗) is the right one: it is stated for the fixed
set L(B) of lines live at the switch, a line that receives an SP stone drops out for good, filler moves can
only put an SP stone on a line (never hurt), and when FP takes a pairing cell whose partner is already SP's,
that line already contains an SP stone, so nothing is needed. The partner is never an FP stone (if FP had
taken it earlier while (∗) held for the line, SP would have answered with x). A pairing at a moment when FP
is to move suffices: the induction starts there. Lemma 1.2: the cut computation and the deficiency formula
2\|L\| − δ are correct; (d) has the right direction. See finding 5 for the only nit.

**2. Section 3.** Lemma 3.1 holds for any Y. In Theorem 3.2 the reduction to an X-component is what makes
the 2-line case go through: two lines whose common cell is in X are *not* X-adjacent, hence different
components, each handled by the 1-line case; two X-adjacent lines share a cell outside X and Lemma 2.2(b)
applies. X-connected ⇒ F-connected has the right direction (F ⊆ X). The inequality chain
sur_F(S′) ≥ k − \|F\| ≥ \|X∖F\| ≥ \|(X∖F)∩N(S′)\| has no off-by-one (θ = k − \|F\| = number of unrevealed stones,
and LEAF is used with k = t = \|X\|). Remark 3.3(ii) is correct.

**3. Section 4.** 4.2(a): the pairing hypothesis is used exactly where needed (sur_F ≥ 0 on the other
components) and is genuinely needed: in my brute-force run it failed for 167 triples of instances *without* a
pairing and for none of 25 814 triples with one. 4.2(b) "⇐" needs no pairing, as stated. 4.3(a),(b),(d)
are correct (order-independence is just "Ford–Fulkerson stops at a maximum flow"; lines outside U stay at 2
because simple augmenting paths never re-enter s); (c) is correct for unbounded capacity (finding 2).
4.4(a) is monotonicity in the right direction; 4.4(b) needs S to be a full F-component of L(B), which is
what the text says. No hidden assumption that F is admissible: admissibility is condition 1 of LEAF and is
not used in §4.

**4. Section 5.** (M1)–(M3) correct; (M1) needs \|F′\| ≤ k and F′ admissible, both stated. Proposition 5.4:
**I believe it.** (i) is a correct counterexample. For (ii) I re-checked the construction: the choice of
l₀, the count \|R\| ≤ 2\|D₅\| + \|D₄\| − 1 + ε, Claim 1 (the only possible new danger pair is (l₀, m′) and it is
excluded by r₀ resp. r*), and the three cases of Claim 2. One sentence is slightly loose ("m′ = a₀ by (β)"
presupposes that a₀ exists; if it does not, there is no such m′ at all), which does not affect the proof.
Brute force: 2 373 instances aimed at the non-trivial case, 21 635 (B+b)-admissible X that are not
B-admissible (8 840 by the cap, 12 795 by a danger pair), no counterexample; plus 21 650 untargeted checks.

**5. Section 6.** Classification triangle / star / path and the counts 15, 16, 16 are right; each line has
≥ 4 private cells; X_T gives 4,3,3 on the triple and ≤ 3 on every other line, so exactly one line has 4 and
there is no danger pair. Proposition 6.2 and Remark 6.3 follow. Computed: 25 176 connected triples
(1 464 + 1 464 + 22 248), all three choices of the 4-stone line admissible on the empty board.

**6. Section 7.** 7.1, 7.2 correct (sρ = ρs is exactly the defining property of H; G_Ρ is normalised by
G_Π; intersection {id, Σ_ρ}). I checked closure completely, not by spot check: the group generated by six
generators taken from G has order 1152 and equals G. 7.4(a) is the coset argument; 7.4(b) holds for any g
attaining the canonical form, including a non-trivial stabiliser (two choices of g differ by an element of
the stabiliser of (B,F), which maps valid table moves to valid table moves).

**7. Section 8.** SPEC §4 versus §8.1: children (a), (c), (d), (i), (ii) agree clause by clause, including
"e ∈ l∖F", "c ∉ F", "F∩a ⊆ Q_a", "\|F ∪ …\| ≤ t+1", "(ii) iff \|F\| < t". SPEC §5 versus Definition 8.3 and
Theorem 8.9: agree, except for the PAIRING rule (finding 1). E2 and E3 are checked literally in Lemmas 8.6
and 8.7 and every clause of SPEC is covered (lines live and distinct; x, e, c resp. x, c, c′ distinct;
l∩a = {x}; e ∉ b follows from the choice of the pair, e ∉ a from l∩a = {x}; stone sets = X′∩line, so the
conditions on F and the count ≤ t+1 hold because everything is inside X′).

Case analysis of one FP move from an admissible position (Lemma 8.5 and Theorem 8.9): complete.
* FP cannot complete a line (cap 4) and cannot make two 5-lines (they would have been a danger pair).
* 5-line plus danger pairs: after the block at e only pairs avoiding e survive; each is an E2 instance.
* Several danger pairs: every pair has one new line (through x) and one old line; if E3 does not fit, all
  pairs share the new line or all share the old line, and then any pair's cell kills all of them. Three
  pairs (a,b), (a′,b′), (a,b′) make E3 fit although one cell would save SP; the tree is conservative there.
* A move onto a cell that is the empty cell of several 3- and 4-lines, a move creating a 4-line through
  revealed stones, a busy-cell move: all are instances of the three cases; the revealed sets are X′∩l resp.
  X′∩a ∪ X′∩b, and the abstract child is the enumerated child (c) resp. (d) with its count condition true.
* Reveal loop of step 3: invariant (J) is preserved by each reveal (child (ii) exists because m ∈ X′∖F,
  and is not VOID because F+m ⊆ X′), and the loop ends after at most t+1 reveals.
* VOID: a real play never reaches a VOID state, by Lemma 2.1(a) and "X′ admissible for the new B", which is
  where LOST false is used. I tried to break this with F′ inadmissible but X′ admissible; downward closure
  excludes it (a line with 4 cells of F′ and a common cell in X′∖F′ would have 5 cells of X′).
* R_max: an expanded node has t < R_max, so a P-state with t = R_max in V must be a listed leaf; correct.
* Switch to PAIRING: at the start of a round, FP to move, X B-admissible, \|X\| = t, F ⊆ X, LEAF(B,F,t) ⇒
  C(B,F,t) ⇒ pairing for (B,X); Lemma 1.1 applies with B = all SP stones. After that FP never owns a line.
* Lemma 8.2 (rank, certificate ⇔ WON) is correct; the equivalence is not even needed for Theorem 8.9,
  which only uses validity.
* Proposition 8.11 and Corollary 8.12 are correct (LOST is antitone in F; the inductions go down in rank).

**8. Sections 0 and 9.** Theorem 0.2 has the right hypothesis ((H), with the true geometry of Lemma 0.1);
see finding 1 for the name of the strategy. Proposition 0.3 is the standard ghost-stone argument and is
correct for this game (same winning sets for both players, an extra stone never hurts; the case "no real
empty cell" cannot even occur on 216 cells). Corollary 0.4 is correct. COMPUTED dependencies: only (H);
the geometry is proved by hand. PROVED labels that rest on runs: finding 3 only. Unstated dependency:
finding 4.

Which parts the theorem actually uses: Lemma 0.1, 1.1, 1.2(b), 2.1, 2.2, 3.1, Theorem 3.2, Lemmas 7.1–7.4,
Definition 8.1, Lemmas 8.5–8.7, Theorems 8.9, 8.10, Proposition 0.3, Corollary 0.4. Section 4 is not used by
the theorem but is what justifies that the programs' triple / max-flow test decides LEAF, i.e. it belongs to
the correctness argument for (V1). Sections 5, 6, 8.4 are not used.

## 4. Computations (scripts and logs in `logs/review/`)

| script | what | result |
|---|---|---|
| `ref_struct.py` (`ref_struct_run.log`) | (G1), (G2); 296 triples, each line from exactly 2; 2304 parameter triples → 1152 maps, all bijective and line-preserving; generated group = G; connected triples; Lemma 6.1; instance for Remark 3.3(iii); Proposition 5.4(i) | all as stated; 25 176 triples = 1 464 + 1 464 + 22 248; 0 inadmissible placements in 75 528; 141 live lines, 210 free cells |
| `ref_l4.py` seeds 11, 12 (`ref_l4_run_seed*.log`) | Lemmas 8.5–8.7 with an independent **literal search** for E2/E3 (all lines, cells, stone sets), F = X, X minus 1/2/3 stones, random half of X | 1 400 positions, 253 245 FP moves; 9 094 threats, 1 495 leave X′ inadmissible, E2 fits in all; 3 662 danger situations (951 with several pairs), 507 with a non-working pair, E3 fits in all; "reply c works ⇔ every other pair shares a line with it" always; children (c), (d) conditions and non-VOID always; no assertion failed |
| `ref_flow.py` seeds 21, 22 (`ref_flow_run_seed*.log`) | §§1–5 by enumeration of all subsets of live lines, real lines, B leaving 3–9 live lines, θ ≤ 3 | 6 500 instances; 1.2(b) 6 500; LEAF ⇒ C 5 173; C without LEAF 220; 4.2(a) 25 814 triples (and 167 failures without a pairing, as expected); 4.2(b) 5 954; augmentation count = μ and witness R for 180 698 sets U; 4.4(b) 25 814; (M1) 17 740; (M3) 31 038; 5.4(ii) 21 650; 3.3(ii) 5 173; no assertion failed |
| `ref_p54.py` seed 42 (`ref_p54_run_seed42.log`) | Proposition 5.4(ii), F stacked on the lines through b | 2 373 instances with C true; 21 635 new X; no counterexample |
| `ref_pairing.py` seed 31 (`ref_pairing_run_seed31.log`) | Lemma 1.1 played out on the real board, adversarial filler cells, three FP styles | 240 games, 23 218 FP moves, 5 232 fillers, partner already SP's 3 410 times, FP reaches 5 on an SP-free line but never 6, (∗) holds at every FP turn |
| `ref_lazy.py` (`ref_lazy_run.log`) | Proposition 8.11 on 7 toy trees (AG(2,4), 4 classes, R_max 8). **Not independent**: uses the WON recursion of `logs/proof/toy_tree.py`, which I read | 688 P-checks, 1 193 Q-checks, no failure |

Appendix A against the raw logs: every figure I could recompute matches (fuzz_l4: 7 972 990 moves, 363 222
threats, 45 922, 62 306, 131 217, 21 056, 7 694, 16 768, 622; fuzz_cprime: 5 574, 447 456, 244 718, 91 691,
148 773, 18 042; fuzz_small: 12 500, 8 660, 246, 9 871, 87 145, 7 313, 9 901, 167 373, 130 838; the 50 763
comparisons for Proposition 6.2 are the logged 63 263 minus the 12 500 vacuous k = 4 comparisons;
check_misc: 25 176 = 1 464 + 1 464 + 22 248). I did not re-run those scripts. I read `toy_tree.py`: it is a
literal implementation of SPEC §4/§5 for line length n. Coverage remark: among its runs with LOST switched on
and a WON root (`toy_run1.log`, `toy_run2.log`), VOID children (c)/(d) occurred in exactly one tree (2 VOID
children); all other WON trees have `'void': 0`. So the end-to-end toy test hardly exercises the VOID rule;
its soundness rests on the hand argument (Lemma 2.1(a) with Lemmas 8.6, 8.7), which I checked, on my
`ref_l4.py` assertion "the real child (c)/(d) is never VOID when X′ is admissible after SP's reply", and on
`bin/sim` runs against the real tree (not reviewed by me). Appendix A should say this. (Six further runs of
`toy_tree.py 4a4 8 lost 4 <seed>`, seeds 21–26, `logs/review/toy_void/toy_void_h4.log`: all roots WON, no
failure, again no VOID child; the 16-cell boards are too small to produce them in WON trees.)

## 5. Not verified

* **(H) itself.** I did not run `bin/search` or `bin/treecheck` and did not audit their LEAF, LOST,
  child-generation or canonical-form code (only treecheck's `process()` / `pushChild()` and header were read).
* That `sim/sim.cpp` implements σ of Definition 8.3.
* Proposition 6.2 was not brute-forced for k > 3 by anyone; it rests on the hand proof plus the exhaustive
  check of Lemma 6.1.
* Corollary 8.12 was checked by hand only; Proposition 8.11 by hand and by the small non-independent run above.
* The CITED theorems (max-flow/min-cut with integrality, determinacy of finite games) were taken as given.
* My fuzz runs are samples (seeds and sizes above), on positions with at most 9 live lines for the
  enumeration-based checks.

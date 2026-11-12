# PROOF.md — the mathematical part of the proof that 6×6×6 tic-tac-toe is a draw

Definitions are exactly those of SPEC.md §§1, 3, 4, 5, 6 (read with the errata of SPEC §10); this file
proves statements about them and changes none. Every claim carries a label: **PROVED** (complete proof in
this file), **COMPUTED** (established by a program run; the program and its output file are named; for
hypothesis (H), the only computed fact the theorem uses, §9 names the independent programs that re-check
it), **CITED** (standard theorem).

Notation. `B+b` = B ∪ {b}. 𝓛 = the set of lines, L(B) = the live lines (no cell in B), N(S) = union of the
lines of S. For a cell set Y and S ⊆ L(B): def_Y(S) = 2|S| − |N(S)∖Y| and sur_Y(S) = |N(S)∖Y| − 2|S| = −def_Y(S).
θ = k − |F|. "Line" always means one of the 148 lines of SPEC §1.

## Read this first: status and findings

1. **The theorem is conditional.** Theorem 0.2 assumes hypothesis (H): *the root P(∅,∅,0) is WON for some
   R_max* (SPEC §4). (H) is a COMPUTED fact; nothing in this file proves it. **Status of (H) at the time of
   this revision (2026-11-12, about 07:25 UTC) — all of the following is COMPUTED**, by programs that were
   written and run by other agents; the proof writer did not audit them (what he re-read and recounted
   himself is stated in §9.1):
   * *Certificate, R_max = 10* (found by `bin/search --nonbusy`): `results/final/strategy.tbl` (93 164
     Q-states, one table move each; 434 of these moves are non-busy cells) and `results/final/leaves.txt`
     (106 723 leaves). Reachable set: 153 135 P-states (46 412 expanded, 106 723 leaves) and 93 164
     Q-states, 246 299 states in all.
   * *Verified by* (i) `bin/treecheck --exact --paranoid` (`results/final/treecheck.json`: ok, 246 299
     states, 143 498 threat and 28 275 danger children, 151 VOID children, all of type (ii)); (ii)
     `verify/spotcheck.py`, a third implementation in pure Python by a different author, in exhaustive
     mode: the same counters, LEAF recomputed for all 106 723 leaves; (iii) the search's own `--selfcheck`.
   * *Leaves* by t: 3 382 (t = 8), 87 314 (t = 9), 16 027 (t = 10); by θ = t − |F|: 64 841 / 32 703 / 7 267 /
     1 434 / 343 / 105 / 25 / 4 / 1 for θ = 0, …, 8.
   * *Busy-only table moves do NOT suffice*: with table moves restricted to the 24 busy cells the root is
     not WON for any R_max ≤ 10 (exhaustive failure, already in the lazy game without forced events;
     `logs/search/busyonly_r10_rel24.log`). The certified strategy therefore uses non-busy table moves.
     This is covered: nothing in §8 uses that table moves are busy cells (item 4(g)).
   * Further cross-checks (simulator games, integer programs, exact enumeration of C on the leaves) are
     recorded in REPORT.md by the coordinator; they are
     **not part of the proof**.

   Details, file names and the precise meaning of each item: §9.1.
2. **No statement of SPEC §§1, 3, 4, 5, 6 was found to be false.** Every "Fact" of SPEC §3 is proved in §§2–6,
   the soundness of the tree of SPEC §4 for the strategy σ of SPEC §5 (read with erratum 1 of SPEC §10;
   Definition 8.3) in §8, and the three geometry facts by hand in Lemma 0.1 and §7 (so the theorem does not
   rest on a computed geometry).
3. **L3(c) of the task statement is false as literally stated, and true with one extra hypothesis**
   (Proposition 5.4, new). `C(B,F,k) ⇒ C(B+b,F,k)` fails when F is not B-admissible (then C(B,F,k) is vacuously
   true; explicit counterexample with B = ∅). It **holds whenever F is B-admissible** (complete proof, plus a
   brute-force fuzz test, Appendix A). Neither direction is used: the proof tree only uses the monotonicity of
   LEAF (Proposition 5.1), exactly as decided in PROGRESS.md.
4. Clarifications (none requires a change of a program; also appended to `logs/spec_issues.md`):
   * (a) LEAF condition 2 (a pairing for (B,F)) is implied by conditions 1 and 3 (Remark 3.3). It is harmless,
     and the max-flow algorithm of §4 needs the pairing anyway.
   * (b) SPEC §5, PAIRING mode, "take the other cell assigned to the same line": that cell may already hold an
     SP stone (taken earlier as an "any empty cell" move). The rule proved in Lemma 1.1 is "take it *if it is
     empty*, otherwise any empty cell" (this is what `sim/sim.cpp` does). SPEC §10, erratum 1, now states
     this rule; Theorem 0.2 is stated for the strategy with this rule (Definition 8.3).
   * (c) The patterns E2/E3 are *necessary* for a loss of admissibility, not sufficient: E3 can fit although
     one of the danger cells would resolve every danger pair (Remark 8.8; that this occurs is COMPUTED, it
     was observed in the fuzz test). LOST is therefore conservative, which is all that soundness needs.
     A pattern needs at least 11 stones, so LOST is false at every P-state with t ≤ 9, in particular at
     every expanded state of the certified tree with R_max = 10 (Proposition 8.4a).
   * (d) worst(B,F,k) is a maximum over the empty family when F is not B-admissible; C(B,F,k) is then
     vacuously true (already noted by task 3).
   * (e) The canonical-form look-ups of `sim` and `treecheck` agree only because G is closed under composition
     (a group). This is proved by hand (Lemma 7.2), not only spot-checked.
   * (f) SPEC §7.2 ("any admissible X of size < k is covered because deficiency is monotone"): monotonicity
     alone does not cover an admissible X of size < k that has no admissible extension of size k. The
     enumeration is exact because the pairing is examined at **every** node (Remark 2.3; `match.hpp` does so).
   * (g) Nothing in §8 uses that table moves are busy cells. This matters: busy-only tables cannot prove the
     root for R_max ≤ 10 (COMPUTED, item 1 and §9.1), and the certificate uses 434 non-busy table moves.
5. Self-checks. Every lemma that can be brute-forced was fuzz-tested by scripts under `logs/proof/`
   (Appendix A: measured numbers; no counterexample to any statement of this file was found).
6. This is the revision after the mathematical referee report (`logs/review/math_referee.md`: no false
   statement, 7 minor findings) and the observations of the third-implementation reviewer. Every change is
   listed in the Revision log at the end of the file. New material: Proposition 8.4a, Remark 8.4b, §9.1.

## 0. Statement

### 0.1 The game and the geometry

Cells, lines, the group G and the rounds are as in SPEC §1. FP and SP alternately occupy an empty cell, FP
first. The first player who owns all six cells of a line wins; if all 216 cells are occupied and nobody owns
a line, the game is a draw.

**Lemma 0.1 (geometry). PROVED** (also COMPUTED: the constructor of `src/geom.hpp` aborts unless (G1), (G2)
and the first half of (G3) hold; `logs/proof/check_misc.py` re-derives all of it with its own geometry;
SPEC §7.5–7.7 require `treecheck`, `sim` and `py/geom.py` to carry their own geometry as well).

* (G1) There are exactly 148 lines (108 + 36 + 4), each consisting of 6 distinct cells.
* (G2) Two distinct cells lie on at most one line. Equivalently, two distinct lines share at most one cell.
* (G3) Every map of G is a bijection of the cells that maps each line onto a line, and G is closed under
  composition and inversion (proved in Lemmas 7.1, 7.2).

*Proof of (G1), (G2).* A line is ℓ(f) = {f(t) : t = 0..5} for a triple f = (f₁,f₂,f₃) in which each fᵢ is a
constant, t, or 5−t, not all constant. The six cells are distinct because a non-constant coordinate takes six
distinct values. Let p = f(t₁) ≠ q = f(t₂) be two cells of ℓ(f). A coordinate i is constant iff pᵢ = qᵢ (its
value is then pᵢ). For a non-constant coordinate, fᵢ = t iff pᵢ = t₁ and fᵢ = 5−t iff pᵢ = 5−t₁; these
exclude each other because t₁ ≠ 5−t₁ (5 is odd). Let i₀ be the first coordinate with pᵢ ≠ qᵢ. The reversed
triple f̄(t) = f(5−t) defines the same line, so we may assume f_{i₀} = t; then t₁ = p_{i₀}, and every fᵢ is
determined by p and q. Hence the pair {f, f̄} is determined by any two cells of the line: this is (G2), and it
shows that every line arises from exactly two triples, so there are (8³ − 6³)/2 = 148 lines
(3·36 with one, 3·6·4/2 = 36 with two, 8/2 = 4 with three non-constant coordinates). ∎

### 0.2 The theorem

**Hypothesis (H). COMPUTED.** For some R_max the root P(∅,∅,0) is WON in the sense of SPEC §4. Equivalently
(Lemma 8.2) there is a *valid certificate* (strategy table, leaf file, R_max; Definition 8.1).

*How (H) is established (not by this file).* Validity of the certificate files `results/final/strategy.tbl`,
`results/final/leaves.txt` with R_max = 10 is what the program `bin/treecheck` is designed to verify. That
its answer "ok" really means "valid in the sense of Definition 8.1" is a **property of a program**. It is
not proved in this file. It is the subject of the code audit of `verify/treecheck.cpp`
(`logs/review/treecheck_audit.md`: no fatal or major finding), and it is corroborated by two
further re-implementations of the same check that share no code with `treecheck` and report the same
counters: `verify/spotcheck.py` (pure Python, a different author, written from SPEC alone, exhaustive mode)
and the `--selfcheck` pass of `bin/search` (which shares its core headers with the search, not with
`treecheck`). The certifying `treecheck` run uses `--exact` (the visited set is keyed by the full states,
no hashing) and `--paranoid` (every flow behind a verdict "LEAF holds" is constructed and validated
literally); the meaning of the two options is quoted from the program's own description. Details and the
measured numbers: §9.1.

**Theorem 0.2 (main). PROVED from (H) and Lemma 0.1.** Assume (H), and fix a valid certificate. Let SP play
the strategy σ of Definition 8.3: SPEC §5 run with the table and the leaf file of the certificate, where the
PAIRING-mode rule is the one of Lemma 1.1 — *take the partner cell if it is empty, otherwise any empty cell*
(SPEC §10, erratum 1, states the same rule). Then:

* (a) σ is a legal, total strategy: whenever SP is to move, σ prescribes a cell, and this cell is empty. (In
  particular σ never runs into a strategy hole, a certificate bug or a LOST-pattern bug of SPEC §5.)
* (b) In every play, at no moment does FP own all six cells of a line.

In particular FP has no winning strategy.

The proof is Theorem 8.10. It uses (H), Lemma 0.1 and nothing else that is computed: every lemma below is
proved by hand. The only CITED results are max-flow/min-cut with integrality (equivalently Hall's theorem)
and the determinacy of finite games (for Corollary 0.4 only).

**Proposition 0.3 (strategy stealing). PROVED.** Consider any positional game on a finite board with one
family 𝓗 of winning sets for both players: the players alternately occupy empty cells, the first to own a
whole member of 𝓗 wins, a full board without a winner is a draw. Then SP has no winning strategy.

*Proof.* Suppose τ is a winning strategy for SP. FP plays as follows. One of his stones is marked as the
*ghost* g; the *virtual position* is the real position without the ghost stone, and in the virtual game the
real SP moves first. FP's first move is arbitrary and becomes the ghost. Whenever FP is to move (the real SP
has just moved; this is a first-player move of the virtual game), let w be the answer of τ in the virtual
position. If w ≠ g, the cell w is empty on the real board and FP plays it. If w = g, FP regards the ghost
stone as the virtual answer and plays any real empty cell g′, which becomes the new ghost; if there is no
real empty cell, the real board is full, and so is the virtual board after the virtual answer w.
By construction the virtual play is a legal play in which the second player follows τ; the real SP owns
exactly the stones of the virtual first player; the real FP owns the stones of the virtual second player
plus the ghost (an extra stone never takes anything away from its owner). Since τ wins, the virtual second
player completes some W ∈ 𝓗 at a moment up to which the virtual first player — the real SP — owns no member of
𝓗. The real FP owns W no later than that moment. So FP wins the real game, i.e. FP also has a winning
strategy. Letting the two winning strategies play each other is a contradiction. ∎

**Corollary 0.4 (draw). PROVED from (H); CITED: determinacy of finite games of perfect information
(Zermelo 1913; backward induction on the finite game tree).** Assume (H). Then 6×6×6 tic-tac-toe is a draw:
each player has a strategy with which he never loses. For SP such a strategy is σ of Definition 8.3 (the
strategy of Theorem 0.2).

*Proof.* By determinacy, either SP has a winning strategy or FP has a strategy that never loses; the first
is excluded by Proposition 0.3. σ is a legal strategy for SP (Theorem 0.2(a)) under which FP never owns a
line (Theorem 0.2(b)), so SP never loses. ∎

**What the result depends on.** (1) (H): root WON, i.e. a valid certificate — COMPUTED, re-checked by
`bin/treecheck` (which re-evaluates LEAF at every leaf, LOST at every expanded node and the presence of all
children; that this amounts to validity is a property of the program, see the paragraph after (H) and §9)
and by two further implementations. (2) Lemma 0.1: 148 lines, two cells share at most one line, the 1152
maps are line-preserving bijections forming a group — PROVED here, and COMPUTED independently by every
program. Nothing else.

### 0.3 Context (CITED, not used in the proof)

Hales and Jewett [HJ] introduced pairing strategies and used Hall's theorem to show that n^d tic-tac-toe is a
draw when n is large compared with d; their Ramsey-type theorem shows conversely that for fixed n the first
player wins when d is large (no draw position exists, plus Proposition 0.3). Beck's book [B] is the standard
reference for both and for strategy stealing. On the empty 6×6×6 board no pairing exists (216 < 2·148); the
proof here first plays a finite table of SP moves until a pairing exists for *every* admissible continuation.

## 1. Pairings (L1) and Hall's theorem for 2-fold matchings (L2)

A pairing for (B,X) is a family π(l) ⊆ l∖X, |π(l)| = 2 (l ∈ L(B)), with the sets π(l) pairwise disjoint.
All cells of π(l) are empty (not in X by definition, not in B because l is live).

**Lemma 1.1 (L1, pairing strategy). PROVED.** Consider a position of the game with FP to move (so
|B| = |X|), SP stones B, FP stones X, and a pairing π for (B,X). From now on let SP answer each FP move x by:
*if x ∈ π(l) for a line l (unique) and the other cell of π(l) is empty, take that cell; otherwise take any
empty cell.* Then this rule always names an empty cell, and FP never owns all six cells of a line.

*Proof.* FP is to move, so |B| = |X|, and the players alternate from now on; hence after each FP move an
odd number of cells, i.e. at most 215 of the 216, is occupied, and SP has a legal move. (Only this is used:
after an FP move the board is not full. The hypothesis |B| = |X| holds in every game position with FP to
move, in particular where Theorem 8.10 applies the lemma.) Claim (∗):
whenever FP is to move, every l ∈ L(B) contains an SP stone or both cells of π(l) are empty. (∗) holds at
the start. Let (∗) hold, let FP play x and SP reply z, and let l ∈ L(B). If l already contained an SP stone
it still does. Otherwise both cells of π(l) were empty. If x ∈ π(l), the other cell y of π(l) is empty, so
z = y and l now contains an SP stone. If x ∉ π(l), then after z either z ∈ π(l) ⊆ l (l contains an SP stone)
or both cells of π(l) are still empty. So (∗) persists. Now suppose FP completes a line l with a move x. Then
l contains no SP stone, so l ∈ L(B) (lines outside L(B) contain a stone of B forever), and by (∗) both cells
of π(l) were empty just before x. But then five cells of l were FP's and only one was empty. Contradiction. ∎

**Lemma 1.2 (L2: Hall / Kőnig–Ore for the 2-fold matching, and its max-flow form). PROVED from the max-flow
min-cut theorem with integral flows (CITED: Ford–Fulkerson [FF]); (b) is Hall's theorem [H] for the graph in
which every live line is doubled.** Let B∩X = ∅, L = L(B), and δ(B,X) = max over S ⊆ L of def_X(S)
= max_S (2|S| − |N(S)∖X|) ≥ 0 (S = ∅).

* (a) The maximum number of cells that can be assigned to live lines — each line receiving at most 2 cells
  of l∖X, no cell assigned twice — equals 2|L| − δ(B,X).
* (b) A pairing for (B,X) exists iff δ(B,X) = 0, i.e. iff |N(S)∖X| ≥ 2|S| for every S ⊆ L(B).
* (c) The number in (a) is the value of a maximum flow in the network 𝒩(B,X): source s, a node for each l ∈ L,
  a node for each cell c ∈ N(L)∖X, sink t; arcs s→l of capacity 2, l→c of capacity 1 for c ∈ l∖X, c→t of
  capacity 1.
* (d) Deficiency is monotone in X: X ⊆ X′ implies def_X(S) ≤ def_{X′}(S) for every S, hence δ(B,X) ≤ δ(B,X′)
  (for B∩X′ = ∅).

*Proof.* Integral flows in 𝒩(B,X) are exactly the assignments of (a) (flow 1 on l→c means "c is assigned to
l"), so by integrality the maximum in (a) is the maximum flow value. Let A ∋ s, A ∌ t be a cut and
S = A ∩ L. Its capacity is 2|L∖S| + #{(l,c) : l ∈ S, c ∈ l∖X, c ∉ A} + #{c ∈ A}. Each cell c ∈ N(S)∖X
contributes at least 1 to the last two terms (the arc c→t if c ∈ A, an arc l→c with l ∈ S otherwise), so
the capacity is ≥ 2|L| − 2|S| + |N(S)∖X| = 2|L| − def_X(S), with equality for A = {s} ∪ S ∪ (N(S)∖X). Hence the
minimum cut is 2|L| − δ(B,X); max-flow = min-cut gives (a) and (c); (b) is the case "flow value 2|L|".
(d): N(S)∖X′ ⊆ N(S)∖X. ∎

## 2. Admissibility

X is B-admissible iff X∩B = ∅, every live line has at most 4 cells of X (*cap*), and there is no *danger
pair*: two distinct live lines with exactly 4 cells of X each, sharing a cell outside X.

**Lemma 2.1. PROVED.**

* (a) *Downward closed in X.* If X is B-admissible and X′ ⊆ X, then X′ is B-admissible.
* (b) *Monotone in B.* If X is B-admissible, B ⊆ B′ and B′∩X = ∅, then X is B′-admissible.
* (c) *Inadmissible supersets.* If F is not B-admissible, no X ⊇ F is B-admissible; then C(B,F,k) holds
  vacuously for every k, and every state (B,F,·) is VOID.

*Proof.* (a) X′∩B = ∅ and the cap are clear. If a ≠ b were a danger pair for X′ with common cell c ∉ X′, then
|a∩X| ≥ 4 and |b∩X| ≥ 4, so both are exactly 4 by the cap for X; hence a∩X = a∩X′ and c ∉ X: a danger pair
for X. (b) L(B′) ⊆ L(B), and cap and danger pairs only concern live lines. (c) is the contrapositive of (a). ∎

**Lemma 2.2 (one or two lines cannot violate Hall). PROVED.** Let X be B-admissible.

* (a) |l∖X| ≥ 2 for every l ∈ L(B).
* (b) If a ≠ b are live lines sharing a cell c ∉ X, then |(a∪b)∖X| ≥ 4.

*Proof.* (a) is the cap. (b) By (G2) a∩b = {c}, so |a∪b| = 11 and, as c ∉ X, |X∩(a∪b)| = |X∩a| + |X∩b| ≤ 8,
with equality only if both lines have exactly 4 cells of X — a danger pair. So |X∩(a∪b)| ≤ 7. ∎

**Remark 2.3 (exact enumeration of C; used by `enum_check` only). PROVED.** By Lemma 2.1(a) every admissible
X = F ∪ {c₁ < … < c_m} is reached by adding c₁, …, c_m in this order through admissible sets, so a
depth-first enumeration that skips inadmissible placements visits every admissible X ⊇ F with |X| ≤ k.
C(B,F,k) holds iff a pairing exists at every visited node. The search may stop at the first node without a
pairing (that X is a violator, and by Lemma 1.2(d) so are all its supersets). Testing only the nodes of
size exactly k would *not* be exact in general (an admissible X of smaller size need not have an admissible
extension of size k).

## 3. LEAF ⇒ C

Two live lines are *Y-adjacent* iff they share a cell outside Y; "Y-connected" and "Y-components" refer to
this graph (SPEC uses Y = F; the proof below also uses Y = X).

**Lemma 3.1 (additivity over components). PROVED.** Let Y be a set of cells, S ⊆ L(B), and S₁,…,S_r the
Y-components of S. Then the sets N(Sᵢ)∖Y are pairwise disjoint, so |N(S)∖Y| = Σᵢ |N(Sᵢ)∖Y| and
sur_Y(S) = Σᵢ sur_Y(Sᵢ), def_Y(S) = Σᵢ def_Y(Sᵢ).

*Proof.* A cell c ∉ Y in N(Sᵢ) ∩ N(Sⱼ), i ≠ j, would lie on a line of Sᵢ and on a line of Sⱼ, which would
then be Y-adjacent. ∎

**Theorem 3.2 (LEAF ⇒ C). PROVED.** Let k ≥ |F|. If C_Φ(B,F,k) holds (every F-connected set S of at least 3
live lines has sur_F(S) ≥ k − |F|), then C(B,F,k) holds. In particular LEAF(B,F,k) ⇒ C(B,F,k).

*Proof.* Let X be B-admissible with F ⊆ X and |X| ≤ k, and suppose (B,X) has no pairing. By Lemma 1.2(b)
there is S ⊆ L(B) with def_X(S) > 0. By Lemma 3.1 (with Y = X) some X-component S′ of S has def_X(S′) > 0,
i.e. |N(S′)∖X| < 2|S′|.

* |S′| = 1, S′ = {l}: |l∖X| ≤ 1, contradicting Lemma 2.2(a).
* |S′| = 2, S′ = {a,b}: a and b are X-adjacent, so they share a cell c ∉ X, and |(a∪b)∖X| ≤ 3, contradicting
  Lemma 2.2(b). (Unwound: both lines have exactly 4 cells of X — a danger pair — or one has ≥ 5.)
* |S′| ≥ 3: S′ is X-connected, hence F-connected, because F ⊆ X (a common cell outside X is outside F).
  By C_Φ, |N(S′)∖F| − 2|S′| ≥ k − |F| ≥ |X∖F| ≥ |(X∖F) ∩ N(S′)|, hence
  |N(S′)∖X| = |N(S′)∖F| − |(X∖F) ∩ N(S′)| ≥ 2|S′|. Contradiction. ∎

**Remark 3.3. (i), (ii) PROVED; (iii): the instance below is PROVED by hand (and COMPUTED), the further
instances of Appendix A are COMPUTED only.** (i) The proof uses only condition 3 of LEAF; if condition 1
fails, C(B,F,k) is vacuous (Lemma 2.1(c)). (ii) Conditions 1 and 3 imply condition 2: apply the proof with
X = F. (iii) LEAF is only sufficient for C in general. What is proved is the implication LEAF ⇒ C
(Theorem 3.2); the converse can fail, because a set S with small sur_F(S) is harmless when the free cells of
N(S) cannot take k − |F| further stones admissibly. That the converse *does* fail is an existence claim:

* *A hand-checkable instance* (found by the referee, `logs/review/math_referee.md`, finding 3; re-verified
  for this revision). Let T consist of the three lines through cell 0 in the plane x = 0, i.e.
  {0,1,2,3,4,5}, {0,6,12,18,24,30} and {0,7,14,21,28,35}; |N(T)| = 16. Let B be the 200 cells outside N(T),
  F = ∅, k = 11. (This is an instance of the predicates, not a position of the game tree.)
  *LEAF(B,∅,11) is false:* a line outside T meets each line of T in at most one cell (G2), so it has at
  most 3 cells in N(T) and contains a cell of B; hence L(B) = T. T is ∅-connected (common cell 0) and
  sur_∅(T) = 16 − 6 = 10 < 11, so condition 3 fails.
  *C(B,∅,11) is true:* let X be B-admissible, so X ⊆ N(T). If 0 ∈ X, the three numbers |X∩l|, l ∈ T, are
  ≤ 4 and add up to |X| + 2, so |X| ≤ 10. If 0 ∉ X, at most one line of T has 4 cells of X (two would be a
  danger pair with common cell 0), so |X| ≤ 4 + 3 + 3 = 10. Hence |N(T)∖X| ≥ 6 = 2|T|. A single line has at
  least 2 cells outside X (Lemma 2.2(a)). Two lines a, b ∈ T have |(a∪b)∖X| ≥ 4: by Lemma 2.2(b) if 0 ∉ X,
  and because |X∩(a∪b)| = |X∩a| + |X∩b| − 1 ≤ 7 if 0 ∈ X. So |N(S)∖X| ≥ 2|S| for every S ⊆ T, and (B,X) has a
  pairing (Lemma 1.2(b)).
  COMPUTED (`logs/proof/check_rev.py`, own geometry): L(B) = T; of the 2¹⁶ subsets of N(T), 45 292 are
  B-admissible, the largest have 10 cells, and each of them satisfies Hall's condition and has an explicitly
  constructed pairing. The referee reports the same number 45 292.
* *Further instances, COMPUTED only:* 246 instances with C true and LEAF false among the 12 500 random
  instances of `fuzz_small.py` on generalised / restricted boards (Appendix A); 220 in the referee's
  `ref_flow.py`.

§6 shows that this cannot happen for F = ∅, k ≤ 10.

## 4. Triples and the max-flow computation of μ

For U ⊆ L(B) let μ(U) = min over all S with U ⊆ S ⊆ L(B) of sur_F(S) (B, F fixed).

**Lemma 4.1. PROVED.** Every F-connected set S of at least 3 live lines contains an F-connected triple.

*Proof.* A spanning tree of the F-adjacency graph on S has at least 3 vertices, hence a vertex of degree ≥ 2;
it and two of its neighbours form an F-connected triple. ∎

**Proposition 4.2 (triple reduction). PROVED.** Assume a pairing for (B,F) exists. Then

* (a) for every F-connected U, μ(U) = min of sur_F(S) over the *F-connected* S ⊇ U; and
* (b) C_Φ(B,F,k) holds iff μ(T) ≥ k − |F| for every F-connected triple T of live lines.

*Proof.* (a) "≤" is trivial. Let S ⊇ U be arbitrary and S₀ the F-component of S that contains U (U is
F-connected, so it lies in one component). By Lemma 3.1 sur_F(S) = sur_F(S₀) + Σ sur_F(Sᵢ) over the other
components, and each sur_F(Sᵢ) ≥ 0 by Lemma 1.2(b) because a pairing for (B,F) exists. So sur_F(S) ≥ sur_F(S₀)
with S₀ ⊇ U F-connected. (b) "⇒": for an F-connected triple T, (a) gives an F-connected S₀ ⊇ T with
sur_F(S₀) = μ(T); |S₀| ≥ 3, so sur_F(S₀) ≥ k − |F| by C_Φ. "⇐" (no pairing needed): an F-connected S with
|S| ≥ 3 contains an F-connected triple T (Lemma 4.1) and sur_F(S) ≥ μ(T) ≥ k − |F|. ∎

**Proposition 4.3 (μ is a max-flow; augmentation from a pairing counts it). PROVED from max-flow/min-cut
(CITED [FF]).** Let π be a pairing for (B,F) and U ⊆ L(B). Let 𝒩_U be the network 𝒩(B,F) of Lemma 1.2(c) in
which the arcs s→l, l ∈ U, have infinite capacity. (Any finite capacity ≥ 6 on these arcs gives the same
maximum flow value, because at most |l∖F| ≤ 6 units can pass through a line l; hence (a), (b) and also (d)
hold unchanged. Part (c) needs these arcs to be unsaturated, which is guaranteed for capacity ≥ 7; with
capacity exactly 6, (c) holds if R is defined as the set of lines reachable from s *together with U*. See
the end of the proof of (c).)

* (a) The maximum flow value of 𝒩_U is 2|L(B)| + μ(U).
* (b) π is a feasible integral flow of value 2|L(B)|. Starting from π, every augmenting path raises the
  value by exactly 1, and augmenting until no augmenting path is left always performs exactly μ(U)
  augmentations, in whatever order the paths are chosen. In terms of assignments: the lines outside U keep
  exactly 2 cells, the lines of U collect additional cells, and μ(U) is the maximum total number of
  additional cells. In particular μ(U) ≥ θ iff θ successive augmentations succeed.
* (c) When no augmenting path is left, let R be the set of lines reachable from s in the residual graph.
  Then U ⊆ R, every cell of N(R)∖F is assigned to a line of R, and sur_F(R) = μ(U). If U is F-connected, so
  is R. (This R is the witness S of `phi_check`, reason 3.)
* (d) *Closed sets.* Call a set R′ of lines closed (for the current assignment) if every cell of N(R′)∖F is
  assigned to a line of R′. No augmenting path contains a line of a closed set; hence later augmentations
  change nothing inside R′, and R′ stays closed. (This is what allows `phi.hpp` to collect the extra cells
  of a triple line by line.)

*Proof.* (a) As in Lemma 1.2: a cut A of finite capacity has U ⊆ S := A∩L(B), and its capacity is at least
2|L(B)∖S| + |N(S)∖F| = 2|L(B)| + sur_F(S), with equality for A = {s} ∪ S ∪ (N(S)∖F). (b) π sends 2 units
through every s→l, which is feasible. Capacities are integers and every s–t path ends with an arc c→t of
capacity 1, so each augmentation adds exactly 1; the Ford–Fulkerson procedure stops exactly at a maximum
flow, i.e. after (2|L(B)| + μ(U)) − 2|L(B)| steps. A simple augmenting path never returns to s, so the flow on
the arcs s→l never decreases: lines outside U stay saturated with 2 cells. Concretely an augmenting path is
l₀ ∈ U, a cell c₁ ∈ l₀∖F not held by l₀, its holder l₁, a cell c₂ ∈ l₁∖F not held by l₁, …, ending in an
unassigned cell; cᵢ is re-assigned to l_{i−1}. (c) U ⊆ R because the arcs s→l (l ∈ U) are never saturated:
their capacity is infinite (a capacity ≥ 7 suffices, the flow through l being at most |l∖F| ≤ 6).
If l ∈ R and c ∈ l∖F, then c is held by l or c is reachable; a reachable cell is assigned (else there is an
augmenting path), and its holder is reachable through the backward arc, so it lies in R. Thus the lines of R
hold exactly the cells of N(R)∖F: sur_F(R) = (number of cells held by R) − 2|R| = (extra cells of U) = μ(U),
using U ⊆ R, that lines outside U hold exactly 2 cells, and (b).
A line of R∖U is not entered from s (its arc s→l is saturated) and t is not reachable, so it was entered
through the backward arc from one of its cells c ∉ F, and c was reached from another line of R: every line
of R∖U is F-adjacent to a line of R reached before it. So R is F-connected if U is.
*Capacity exactly 6.* Then an arc s→l, l ∈ U, is saturated only if l holds all six of its cells (so
F∩l = ∅). Such a line need not be reachable (example: U = {l}, F∩l = ∅, l can collect all its 4 other
cells: then nothing is reachable from s, R = ∅, while μ(U) = 4), so (c) fails as stated. But all cells of
l∖F are then held by l itself, and the three arguments above go through word for word for R⁺ = R ∪ U in
place of R: U ⊆ R⁺ trivially, every cell of N(R⁺)∖F is held by a line of R⁺, sur_F(R⁺) = μ(U), and R⁺ is
F-connected if U is. (By its header comment `src/phi.hpp` uses unbounded capacity and recomputes
F-connectivity and surplus of its witness from the definition, so no program is affected.)
(d) After a line of R′ an augmenting path continues with a cell of N(R′)∖F, which is assigned, then with
its holder, again a line of R′, and so on; it can never end in an unassigned cell. So no augmentation
re-assigns a cell of N(R′)∖F. ∎

**Corollary 4.4 (pruning rules and quick rejection). PROVED.**

* (a) U ⊆ U′ ⊆ L(B) implies μ(U) ≤ μ(U′) (minimum over a smaller family). Hence μ({l}) ≥ θ for one line
  l ∈ T, or μ({a,b}) ≥ θ for two lines a,b ∈ T, implies μ(T) ≥ θ. μ({l}) and μ({a,b}) are computed by
  Proposition 4.3 with U = {l}, {a,b}.
* (b) Let π be a pairing for (B,F), T an F-connected triple, and S the F-component of L(B) (not merely of
  some subset) that contains T. Then the number of cells of N(S)∖F that π leaves unassigned equals
  sur_F(S). Hence if it is < θ, then sur_F(S) < θ and C_Φ(B,F,k) fails (S is F-connected, |S| ≥ 3).
  (Counting the cells of F∩N(S) as "unassigned" as well only weakens the test; it stays valid.)

*Proof of (b).* If c ∈ N(S)∖F is assigned to a line l′, then l′ shares the cell c ∉ F with a line of S, so
l′ ∈ S because S is a whole component. Conversely the 2|S| cells assigned to lines of S lie in N(S)∖F.
So exactly 2|S| cells of N(S)∖F are assigned. ∎

Together: given conditions 1 and 2, condition 3 of LEAF is decided exactly, in integer arithmetic, by
Proposition 4.2(b) and 4.3(b), with the shortcuts of Corollary 4.4.

## 5. Monotonicity; the statements L3

**Proposition 5.1 (monotonicity of LEAF). PROVED.** Assume LEAF(B,F,k).

* (M1) If F ⊆ F′, F′ is B-admissible and |F′| ≤ k, then LEAF(B,F′,k).
* (M2) If |F| ≤ k′ ≤ k, then LEAF(B,F,k′).
* (M3) If b ∉ F, then LEAF(B+b,F,k).

*Proof.* (M1) Condition 1 is assumed. Condition 2: C(B,F,k) holds (Theorem 3.2) and F′ is an admissible set
with F ⊆ F′, |F′| ≤ k. Condition 3: an F′-connected S with |S| ≥ 3 is F-connected, and
sur_{F′}(S) = sur_F(S) − |N(S)∩(F′∖F)| ≥ (k − |F|) − |F′∖F| = k − |F′|. (M2) Conditions 1, 2 do not involve k,
and k′ − |F| ≤ k − |F|. (M3) Condition 1: Lemma 2.1(b). Condition 2: L(B+b) ⊆ L(B); restrict the pairing
(no line of L(B+b) contains b, so b is not among the assigned cells that remain). Condition 3: a set of
lines live for B+b is live for B, and F-adjacency and sur_F do not depend on B. ∎

**Proposition 5.2 (L3(a), L3(b)). PROVED (trivial).** (a) C(B,∅,k) ⇒ C(B,F,k): the sets X ⊇ F form a
subfamily. (b) C(B,F,k) ⇒ C(B,F,k′) for k′ ≤ k: |X| ≤ k′ implies |X| ≤ k.

**5.3 The obstacle for L3(c): C(B,F,k) ⇒ C(B+b,F,k).** A set X that is (B+b)-admissible need not be
B-admissible: the lines through b are dead for B+b, so X may hold 5 stones on such a line, or 4 stones on it
together with 4 stones on a line crossing it in a free cell — both forbidden before b was placed. C(B,F,k)
says nothing about such X, so the implication does not follow by restriction. It is **not used** anywhere:
the tree uses (M3) for LEAF only. For completeness the question is settled here.

**Proposition 5.4 (L3(c) settled). PROVED; not used.** Let b ∉ B ∪ F.

* (i) The implication fails in general: for a line l, b ∈ l, B = ∅, F = l∖{b}, k = 5, C(∅,F,5) is true and
  C({b},F,5) is false.
* (ii) If F is B-admissible, then C(B,F,k) ⇒ C(B+b,F,k).

*Proof of (i).* F has 5 cells on the ∅-live line l, so no X ⊇ F is ∅-admissible and C(∅,F,5) is vacuous.
F is {b}-admissible: a line not through b meets l in at most one cell (G2). ({b},F) has no pairing: at
least 148 − 7 = 141 lines are live and need 282 cells, but only 210 cells lie outside B ∪ F. ∎

*Proof of (ii).* Let X ⊇ F be (B+b)-admissible with |X| ≤ k (so b ∉ X) and S ⊆ L(B+b). We construct a
B-admissible X° with F ⊆ X° ⊆ X and a set S′ ⊆ L(B) with def_{X°}(S′) ≥ def_X(S). Then def_X(S) ≤ 0 by
C(B,F,k) and Lemma 1.2(b), and since S was arbitrary, (B+b,X) has a pairing.

Let D be the set of B-live lines through b; L(B) is the disjoint union of L(B+b) and D, and two lines of D
meet exactly in b (G2). Each l ∈ D has at most 5 cells of X. Put D₅ = {l ∈ D : |l∩X| = 5} and
D₄ = {l ∈ D : |l∩X| = 4}; for l ∈ D₄ write l∖X = {b, c_l}.

* (α) Every line of D has at most 4 cells of F, and at most one has exactly 4 (two would be a danger pair of
  F for B, sharing b ∉ F).
* (β) For l ∈ D₄ at most one line a ∈ L(B+b) passes through c_l and has |a∩X| = 4 (two would be a danger pair
  of X for B+b, sharing c_l ∉ X). Call it a_l if it exists.

If D₄ ∪ D₅ = ∅, take X° = X and S′ = S: X is B-admissible, because lines of D have at most 3 cells of X and
two lines of L(B+b) cannot form a danger pair. Otherwise choose l₀ ∈ D₄ ∪ D₅: the line of D with 4 cells of
F if there is one (it lies in D₄ ∪ D₅ since F ⊆ X); else a line of D₅ if D₅ ≠ ∅; else any line of D₄. By (α)
every l ∈ D∖{l₀} has |F∩l| ≤ 3. Let R ⊆ X∖F consist of

* two cells of (l∩X)∖F for each l ∈ D₅∖{l₀}, and one cell of (l∩X)∖F for each l ∈ D₄∖{l₀};
* if l₀ ∈ D₅: one cell r₀ of (l₀∩X)∖F (it exists, |F∩l₀| ≤ 4);
* if l₀ ∈ D₄ and a₀ := a_{l₀} exists: one cell r* of ((l₀∪a₀)∩X)∖F. It exists: otherwise F would contain the
  four X-cells of l₀ and the four X-cells of a₀, and (l₀,a₀) would be a danger pair of F for B (both lines
  are B-live and share c_{l₀} ∉ F).

Let ε = 1 if the last case applies and ε = 0 otherwise, and X° = X∖R. In all cases |R| ≤ 2|D₅| + |D₄| − 1 + ε.

*Claim 1: X° is B-admissible.* Cap: lines of L(B+b) have ≤ 4 cells of X ⊇ X°; lines of D∖{l₀} have ≤ 3 cells
of X°, l₀ has ≤ 4. Suppose m, m′ is a danger pair of X° for B with common cell c ∉ X°. Both in D: impossible,
only l₀ can have 4. Both in L(B+b): each has ≥ 4, hence exactly 4, cells of X, no cell of R, and c ∉ X — a
danger pair of X for B+b. So m = l₀ has exactly 4 cells of X°, and m′ ∈ L(B+b) has exactly 4 cells of X and
m′∩R = ∅; also c ≠ b as b ∉ m′. If l₀ ∈ D₅, then l₀∖X° = {b, r₀}, so c = r₀ ∈ m′∩R: contradiction. If l₀ ∈ D₄,
no cell of l₀ was removed, l₀∖X° = {b, c_{l₀}}, so c = c_{l₀} and m′ = a₀ by (β); but then r* ∈ l₀ ∪ a₀ was
removed from l₀ or from m′: contradiction.

*Claim 2: S′ := S ∪ D₄ ∪ D₅ (∪ {a₀} if ε = 1) satisfies def_{X°}(S′) ≥ def_X(S).* Every cell of N(S′)∖X° lies
in N(S)∖X, or in R, or in N(D₄∪D₅)∖X = {b} ∪ {c_l : l ∈ D₄}, or (if ε = 1) in a₀∖X = {c_{l₀}, z₀}. Recall
S∩D = ∅. If ε = 0: |N(S′)∖X°| ≤ |N(S)∖X| + (2|D₅| + |D₄| − 1) + 1 + |D₄| and |S′| = |S| + |D₄| + |D₅|.
If ε = 1 and a₀ ∉ S: one more line and one more cell (z₀), so |N(S′)∖X°| ≤ |N(S)∖X| + (2|D₅| + |D₄|) + 1 +
|D₄| + 1 and |S′| = |S| + |D₄| + |D₅| + 1. If ε = 1 and a₀ ∈ S: c_{l₀}, z₀ ∈ N(S)∖X already, so
|N(S′)∖X°| ≤ |N(S)∖X| + (2|D₅| + |D₄|) + 1 + (|D₄| − 1) and |S′| = |S| + |D₄| + |D₅|. In each case
|N(S′)∖X°| − |N(S)∖X| ≤ 2(|S′| − |S|). ∎

Consequently C is monotone in B along every real play (there F ⊆ X is always B-admissible), but the proof
tree does not rely on it.

## 6. F = ∅ and k ≤ 10: LEAF is equivalent to C

With F = ∅, "F-connected" means connected through common cells. By (G2) a connected triple T = {a₁,a₂,a₃}
is a *triangle* (three pairwise meeting lines, three different meeting cells, |N(T)| = 15), a *star* (one
common cell, |N(T)| = 16) or a *path* (a₁∩a₃ = ∅, |N(T)| = 16). The *private* cells of aᵢ are those on no
other line of T; each line has at least 6 − 2 = 4 of them.

**Lemma 6.1. PROVED** (and COMPUTED for all 25 176 connected triples of the empty board, `check_misc.py`).
Let T be a connected triple of B-live lines and X_T a set consisting of 4 private cells of a₁, 3 private
cells of a₂ and 3 private cells of a₃. Then |X_T| = 10, X_T ⊆ N(T), and X_T is B-admissible.

*Proof.* X_T∩B = ∅ because the lines are live. A private cell of aⱼ is not on aᵢ (i ≠ j), so a₁, a₂, a₃ have
4, 3, 3 cells of X_T. Any other line meets each aᵢ in at most one cell (G2), so it has at most 3. Thus the
cap holds and only one line has 4 cells of X_T: no danger pair. ∎

**Proposition 6.2. PROVED.** For k ≤ 10: C(B,∅,k) ⇔ LEAF(B,∅,k).

*Proof.* "⇐" is Theorem 3.2. "⇒": condition 1 is trivial (F = ∅); condition 2 is C applied to X = ∅. Suppose
condition 3 fails: there is a connected S ⊆ L(B), |S| ≥ 3, with sur_∅(S) ≤ k − 1 ≤ 9. If sur_∅(S) < 0, X = ∅
has no pairing. Otherwise put j = sur_∅(S) + 1 ≤ k ≤ 10, choose a connected triple T ⊆ S (Lemma 4.1) and a
j-element subset X of a set X_T as in Lemma 6.1. X is B-admissible (Lemma 2.1(a)), |X| ≤ k, X ⊆ N(T) ⊆ N(S),
and |N(S)∖X| = |N(S)| − j = 2|S| − 1 < 2|S|. So C(B,∅,k) fails. ∎

**Remark 6.3. PROVED.** (i) If L(B) contains a triangle T, then sur_∅(T) = 15 − 6 = 9 < 10, so LEAF(B,∅,10)
and C(B,∅,k) fail for every k ≥ 10: with F = ∅ a leaf at round 10 or later requires that no live triangle is
left. (ii) The bound 10 is |X_T|; for k ≥ 11 or F ≠ ∅ the construction is not available and LEAF is only
sufficient (Remark 3.3(iii)).

## 7. Symmetry

Let H be the group of the 48 permutations s of {0..5} with s(5−i) = 5−s(i), ρ ∈ H the reversal i ↦ 5−i.
The maps of G are g = Σ_s ∘ Ρ_r ∘ Π_π: Π_π permutes the three coordinates (π ∈ S₃), Ρ_r applies ρ to the
coordinates in r ⊆ {0,1,2}, Σ_s applies s ∈ H to every coordinate.

**Lemma 7.1. PROVED; COMPUTED (constructor of `geom.hpp`).** Each g ∈ G is a bijection of the cells and maps
every line onto a line; hence g permutes the 148 lines.

*Proof.* Bijectivity is clear. Let ℓ(f) be a line. Π_π permutes f₁,f₂,f₃. Ρ_r replaces fᵢ by 5 − fᵢ for i ∈ r,
which maps constant ↦ constant, t ↦ 5−t, 5−t ↦ t. Σ_s maps a constant to a constant, t to s(t) and 5−t to
s(5−t) = 5 − s(t); with the new parameter t′ = s(t), which runs through 0..5, the image is again the set of
a triple of constants, t′, 5−t′. In all cases not all coordinates are constant. An injective map sends a
6-cell line onto a 6-cell line, and a bijection of the cells induces an injection, hence a permutation, of 𝓛. ∎

**Lemma 7.2 (G is a group). PROVED; COMPUTED (spot check in `geom_selftest`, `check_misc.py`).** G is closed
under composition and inversion, and |G| = 1152.

*Proof.* G_Σ = {Σ_s}, G_Ρ = {Ρ_r}, G_Π = {Π_π} are subgroups of the symmetric group of the cells. Σ_s commutes
with every Π_π, and with every Ρ_r because sρ = ρs for s ∈ H. Π_π Ρ_r Π_π⁻¹ is again a reversal of a set of
coordinates. Hence G_Ρ G_Π = G_Π G_Ρ is a subgroup, G_Σ commutes with it elementwise, and G = G_Σ·G_Ρ·G_Π is a
subgroup. For the order: |G_Σ| = 48 and |G_Ρ G_Π| = 8·6 = 48 (a coordinate permutation fixes (0,0,0), a
non-trivial reversal does not). If Σ_s = Ρ_r Π_π, evaluate both sides at the cells (i,i,i): the left side is
(s(i),s(i),s(i)), the right side has the coordinate 5−i in the positions of r and i elsewhere; as i ≠ 5−i,
either r = ∅ and s = id, or r = {0,1,2} and s = ρ. So G_Σ ∩ G_Ρ G_Π = {id, Σ_ρ} and |G| = 48·48/2 = 1152. ∎

**Lemma 7.3 (invariance). PROVED.** For g ∈ G: L(gB) = g(L(B)), N(gS) = g(N(S)), and X is B-admissible iff gX
is gB-admissible; π is a pairing for (B,X) iff g∘π∘g⁻¹ is one for (gB,gX). Consequently C, sur, F-adjacency,
C_Φ, LEAF, VOID, LOST, the sets of children (a),(c),(d),(i),(ii) of SPEC §4, and WON are invariant:
P(B,F,t) is WON iff P(gB,gF,t) is WON, and likewise for Q-states; m is a valid table move for Q(B,F,t) iff
g(m) is one for Q(gB,gF,t).

*Proof.* All these notions are defined through membership of cells in sets, cardinalities, and the set 𝓛,
which g permutes (Lemma 7.1). For WON use induction on the rank of Lemma 8.2: g maps the children of a
state bijectively onto the children of its image. ∎

**Lemma 7.4 (canonical forms and move translation). PROVED.** Let canon(B,F) be the smallest pair (gB,gF),
g ∈ G, in the order of SPEC §6.

* (a) canon(hB,hF) = canon(B,F) for every h ∈ G. (So the canonical form of a child computed in the
  canonical frame of its parent — what `search` and `treecheck` do — equals the canonical form of the real
  child — what `sim` computes.)
* (b) Let g be **any** element of G with (gB,gF) = canon(B,F) = (Bᶜ,Fᶜ), and let mᶜ be a valid table move for
  Q(Bᶜ,Fᶜ,t). Then m = g⁻¹(mᶜ) is a valid table move for Q(B,F,t), and g maps the children (i), (ii) of
  Q(B,F,t) via m onto the children (i), (ii) of Q(Bᶜ,Fᶜ,t) via mᶜ.

*Proof.* (a) {g∘h : g ∈ G} = G because G is a group (Lemma 7.2). (b) is Lemma 7.3 applied to g⁻¹; note
g(B+m) = Bᶜ+mᶜ and g(F+m) = Fᶜ+mᶜ. If the stabiliser of (Bᶜ,Fᶜ) is non-trivial, different admissible choices of
g give moves m, m′ = h(m) with h in the stabiliser of (B,F); both are valid, and the statement never used
uniqueness of g. ∎

## 8. L4: soundness of the lazy-revelation tree for the real strategy σ

### 8.1 The tree and its certificates

States, VOID, the children, WON and LOST are as in SPEC §4. For reference: the children of a P-state
(B,F,t) are (a) Q(B,F,t+1); (c) P(B+e, F∪(l∖{e}), t+1) for l ∈ L(B), e ∈ l∖F, |F∪(l∖{e})| ≤ t+1; (d)
P(B+c, F∪Q_a∪Q_b, t+1) for live a ≠ b with a∩b = {c}, c ∉ F, Q_a ⊆ a∖{c}, Q_b ⊆ b∖{c}, |Q_a| = |Q_b| = 4,
F∩a ⊆ Q_a, F∩b ⊆ Q_b, |F∪Q_a∪Q_b| ≤ t+1. The children of a Q-state (B,F,t) via m ∉ B∪F are (i) P(B+m,F,t) and,
if |F| < t, (ii) Q(B,F+m,t). The proof never uses that table moves are busy cells.

**Definition 8.1 (certificate).** A certificate is a triple (Tbl, Lv, R_max): Tbl maps canonical Q-states to
cells, Lv is a set of canonical P-states. Its *reachable set* V is the smallest set of canonical states with:
canon(root) ∈ V; if P ∈ V∖Lv, the canonical form of every non-VOID child (a), (c), (d) of P is in V; if
Q ∈ V and Tbl(Q) = m is defined, the canonical forms of child (i) and of child (ii) (if it exists and is not
VOID) via m are in V. The certificate is **valid** iff (V1) LEAF holds for every P ∈ V∩Lv; (V2) every
P = (B,F,t) ∈ V∖Lv has t < R_max and LOST(B,F,t) false; (V3) for every Q = (B,F,t) ∈ V, Tbl(Q) is defined and
not in B∪F. (States that are in Lv but not reachable are irrelevant; `leaves.txt` may contain such.)

**Lemma 8.2. PROVED.** (a) WON is well defined: with rank(P(B,F,t)) = (2(R_max − t), 0) and
rank(Q(B,F,t)) = (2(R_max − t) + 1, t − |F|), ordered lexicographically, the definition of WON for a state only
refers to states of smaller rank; as there are finitely many states, the recursion is well founded.
(b) The root is WON for R_max iff a valid certificate (·,·,R_max) exists; every state in the reachable set
of a valid certificate is WON.

*Proof.* (a) Children (a), (c), (d) of P(B,F,t) exist only for t < R_max and have ranks (2(R_max−t)−1, ·),
(2(R_max−t)−2, 0); children (i), (ii) of Q(B,F,t) have ranks (2(R_max−t), 0) and (2(R_max−t)+1, t−|F|−1).
(b) "⇐": induction on the rank inside V, using Lemma 7.3 to pass between a child and its canonical form:
P ∈ V∩Lv is WON by (P1); P ∈ V∖Lv by (P2), its non-VOID children being in V; Q ∈ V by the move Tbl(Q) (child
(i) is never VOID, Lemma 2.1(b)). "⇒": let Lv be the canonical P-states with LEAF, and choose for every WON
canonical Q-state a move witnessing it; by induction along the definition of V all states of V are WON, so
(V1)–(V3) hold. ∎

So (H) is the statement "a valid certificate exists". From now on a valid certificate is fixed.

**Definition 8.3 (σ).** σ is the strategy of SPEC §5 run with Tbl and Lv; in PAIRING mode the rule is the one
of Lemma 1.1: if FP's cell x is assigned to a line and the other cell assigned to that line is empty, take
it; otherwise take any empty cell. (The original sentence of SPEC §5 omitted "if it is empty" and would
prescribe an occupied cell when the partner already holds an SP stone; SPEC §10, erratum 1, now states the
rule as here.) In step 3, "look up the canonical form" means: compute g with g(B,F) = canon(B,F), read
mᶜ = Tbl(canon Q(B,F,t)), and use m = g⁻¹(mᶜ). This σ is the strategy of Theorem 0.2.

**Lemma 8.4 (size of the patterns; justifies the cheap rejection of SPEC §4). PROVED.** Let u = t+1−|F|.
A fitting pattern E2 has |S_l ∪ S_a ∪ S_b| ≥ 11 and needs
(5 − |F∩l|) + (4 − |F∩a|) + (4 − |F∩b|) − 2 ≤ u; a fitting E3 has |S_a ∪ S_{a′} ∪ S_b ∪ S_{b′}| ≥ 12.

*Proof.* E2: S_l∩S_a ⊆ l∩a = {x}; S_a∩S_b ⊆ (a∩b)∖{c} = ∅; |S_l∩S_b| ≤ |l∩b| ≤ 1 (G2). So the union has
≥ 5+4+4−2 = 11 cells. As e, c ∉ F, F∩l ⊆ S_l, F∩a ⊆ S_a, F∩b ⊆ S_b, the stone sets contain
5−|F∩l|, 4−|F∩a|, 4−|F∩b| cells outside F, of which at most 2 coincide; all of them must fit into the
t+1−|F| stones outside F. E3: S_a∩S_{a′} = {x}, S_a∩S_b = S_{a′}∩S_{b′} = ∅, and the three remaining pairs
share at most one cell each: ≥ 16 − 4 = 12. (The count is redone with all details in Proposition 8.4a.) ∎

**Proposition 8.4a (number of stones of a pattern; LOST is false for t ≤ 9). PROVED.** (Numbered 8.4a so
that the numbers 8.5–8.12, which SPEC §10 and the review reports cite, stay unchanged.) Consider any
pattern E2 or E3 of SPEC §4 for a state (B,F,t), whether it fits or not.

* (a) E2: |S_l ∪ S_a ∪ S_b| = 12 − |S_l∩S_b|, and |S_l∩S_b| ≤ 1. So an E2 pattern has 11 or 12 distinct stones.
* (b) E3: |S_a ∪ S_{a′} ∪ S_b ∪ S_{b′}| = 15 − |S_a∩S_{b′}| − |S_{a′}∩S_b| − |S_b∩S_{b′}|, and each of the three
  intersections has at most one cell. So an E3 pattern has between 12 and 15 distinct stones.
* (c) Hence every pattern has at least 11 distinct stones (E2 at least 11, E3 at least 12; these are
  *lower bounds*, see Remark 8.4b), a fitting pattern requires t + 1 ≥ 11, and **LOST(B,F,t) is false
  whenever t + 1 ≤ 10, i.e. at every P-state with t ≤ 9.** In particular, in a tree with R_max = 10, LOST is
  false at every expanded P-state (an expanded state has t < R_max), so the requirement "LOST(B,F,t) is
  false" of (P2) and (V2) is automatically satisfied there.

*Proof.* The lines of a pattern are pairwise distinct, so any two of them share at most one cell (G2).

(a) |S_l| = 5 and |S_a| = |S_b| = 4. The three pairs of stone sets:
S_l∩S_a = {x} — it is contained in l∩a = {x}, and x belongs to both: x ∈ S_a by definition, and x ∈ S_l = l∖{e}
because x ∈ l and x ≠ e. S_a∩S_b = ∅ — it is contained in a∩b = {c}, and c ∉ S_a. S_l∩S_b ⊆ l∩b has at most one
cell (it may or may not be empty). The triple intersection lies in S_a∩S_b = ∅. Inclusion–exclusion gives
|S_l ∪ S_a ∪ S_b| = (5 + 4 + 4) − (1 + 0 + |S_l∩S_b|) + 0.

(b) Each of the four stone sets has 4 cells. The six pairs: S_a∩S_{a′} = {x} — contained in a∩a′ = {x}, and
x ∈ S_a∩S_{a′} by definition. S_a∩S_b = ∅ — contained in a∩b = {c}, and c is in no stone set. S_{a′}∩S_{b′} = ∅ —
likewise with c′. The remaining pairs (S_a,S_{b′}), (S_{a′},S_b), (S_b,S_{b′}) lie on pairs of distinct lines
and have at most one common cell each; nothing in the definition forces them to be disjoint. Every three of
the four stone sets include S_a and S_b, or S_{a′} and S_{b′} (the triples are {a,a′,b}, {a,a′,b′}, {a,b,b′},
{a′,b,b′}), so all triple intersections and the fourfold intersection are empty. Inclusion–exclusion gives
16 − (1 + 0 + 0 + |S_a∩S_{b′}| + |S_{a′}∩S_b| + |S_b∩S_{b′}|).

(c) A pattern fits iff |F ∪ (all stone sets of the pattern)| ≤ t + 1 (SPEC §4), and the left side is at
least the number of distinct stones, which is ≥ 11 by (a) and ≥ 12 by (b). So LOST(B,F,t) true implies
t + 1 ≥ 11. ∎

**Remark 8.4b (the bound 12 for E3 is not attained on this board). COMPUTED; not used.** The program
`logs/proof/e23min.cpp` (own geometry) enumerates literally all patterns with their stone sets on the empty
board, B = F = ∅, where all 148 lines are live. Every pattern of every state (B,F,t) is among them, since B
only removes lines and the conditions on F only restrict the stone sets. Result (`e23min.log`): E2 —
5 152 320 patterns, 562 176 with 11 distinct stones and 4 590 144 with 12; E3 — 353 422 080 patterns
(the pair (a,a′) ordered), with 13 / 14 / 15 distinct stones in 10 948 608 / 67 627 008 / 274 846 464 of them
and **none with 12**. So on this board the minimum is 11 for E2 (attained) and 13 for E3. A second
count, on the level of lines (`logs/proof/check_rev.py`, Python): among the 926 640 ordered line
configurations (a,a′,b,b′) of E3, none has a∩b′, a′∩b and b∩b′ all non-empty, which 12 stones would
require by Proposition 8.4a(b). (Reason, as a sketch only: the four lines would then meet pairwise in six
different cells, so they would lie in one plane and have four different directions; the planes of the cube
that contain four line directions are 6×6 grids, whose two diagonals have no common cell because 6 is even.
This was pointed out by the author of `treecheck`, `logs/spec_issues.md`, task 6c, item 4.) Nothing depends
on this remark: the programs implement LOST from the definition, and Proposition 8.4a(c) only needs the
bound 11.

### 8.2 One FP move from an admissible position

**Lemma 8.5. PROVED.** Let X be B-admissible, x ∉ B∪X, X′ = X+x.

* (a) No live line has 6 cells of X′. At most one live line has 5 cells of X′; it contains x.
* (b) In every danger pair of X′ (for B) exactly one line contains x. It (the *new* line) has exactly 3
  cells of X; the other (the *old* line) does not contain x and has exactly 4 cells of X. The common cell c
  is different from x and empty.
* (c) Two different old lines (of danger pairs of X′) do not share a cell outside X′. Two different new
  lines share exactly the cell x.
* (d) Different danger pairs of X′ have different common cells c.

*Proof.* (a) Live lines have ≤ 4 cells of X, so ≤ 5 of X′, and 5 only if they contain x and have exactly 4
cells of X. Two such lines would share x ∉ X: a danger pair of X. (b) Let a, b be a danger pair of X′ with
common cell c ∉ X′. If neither contains x, it is a danger pair of X. If both contain x, then c = x ∈ X′ by
(G2). The rest is counting; c ∉ B because the lines are live. (c) Old lines have exactly 4 cells of X; if two
of them shared a cell outside X′ ⊇ X they would be a danger pair of X. New lines contain x; apply (G2).
(d) Let (a,b), (a′,b′) be danger pairs with the same c (a, a′ new). a and a′ contain x and c ≠ x, so a = a′
by (G2). Then b, b′ are old lines through c ∉ X′, so b = b′ by (c). ∎

**Lemma 8.6 (threat). PROVED.** In the situation of Lemma 8.5 let F ⊆ X, t = |X|, let l be a live line with
5 cells of X′ and e its sixth cell. Then e is empty, and either X′ is (B+e)-admissible or the pattern E2 fits
for (B,F,t).

*Proof.* e ∉ X′ and e ∉ B (l is live). X′∩(B+e) = ∅. A line that is live for B+e and has ≥ 5 cells of X′ would
be l (Lemma 8.5(a)), but e ∈ l. So if X′ is not (B+e)-admissible, there is a danger pair a, b of X′ for B
with e ∉ a∪b; let a be its new line, b its old line and c the common cell (Lemma 8.5(b)). Check E2 literally:
l, a, b are live for B and distinct (e ∈ l only). x ∈ l∩a, so l∩a = {x} (G2); e ∈ l; a∩b = {c} (G2); x ∉ b;
e ∉ b. x ≠ e (x ∈ X′), x ≠ c (c ∉ X′), c ≠ e (c ∈ a, e ∉ a). S_l = l∖{e} = X′∩l; S_a = X′∩a has 4 cells,
contains x, not c; S_b = X′∩b has 4 cells, not c. e, c ∉ F because F ⊆ X′; F∩a ⊆ S_a, F∩b ⊆ S_b for the same
reason. Finally F∪S_l∪S_a∪S_b ⊆ X′, so its size is ≤ t+1: the pattern fits. ∎

**Lemma 8.7 (danger). PROVED.** In the situation of Lemma 8.5 let F ⊆ X, t = |X|, assume that no live line
has 5 cells of X′, and that X′ has at least one danger pair for B. Then for the common cell c of **any**
danger pair (a,b), either X′ is (B+c)-admissible or the pattern E3 fits for (B,F,t). More precisely, if E3
does not fit, then all danger pairs have the same new line or all have the same old line.

*Proof.* c is empty (Lemma 8.5(b)), X′∩(B+c) = ∅, and the cap holds by assumption. The danger pairs of X′ for
B+c are the danger pairs for B none of whose lines contains c. Let (a′,b′) with common cell c′ be one (a, a′
new; b, b′ old). Then a′ ≠ a and b′ ≠ b, since c ∈ a∩b. Check E3 literally: a, a′, b, b′ are live for B and
pairwise distinct (new lines contain x, old ones do not). a∩a′ = {x}, a∩b = {c}, a′∩b′ = {c′} (G2); x ∉ b∪b′;
x, c, c′ are pairwise distinct (c, c′ ∉ X′; c ≠ c′ by Lemma 8.5(d)). The stone sets X′∩a, X′∩a′, X′∩b, X′∩b′
have 4 cells each, the first two contain x, none contains c or c′ (c, c′ ∉ X′). c, c′ ∉ F and F∩a ⊆ X′∩a etc.
because F ⊆ X′; the union of F and the stone sets lies in X′, of size t+1. So E3 fits.
So if E3 does not fit, every other danger pair contains a or b, hence c, and X′ is (B+c)-admissible.
For the last sentence: the check above only used a ≠ a′ and b ≠ b′, so if E3 does not fit, any two danger
pairs share the new line or the old line (not both, a pair being determined by its lines). If two pairs
share the new line a₁ (so their old lines b₁ ≠ b₂ differ), a third pair with another new line would need
both old lines b₁ and b₂; so all pairs have the new line a₁. Otherwise no two pairs share the new line, and
then any two share the old line. ∎

**Remark 8.8 (LOST is conservative; the choice of the pair is immaterial). (i), (ii) and the implication
in (iii) PROVED; that the configuration of (iii) occurs is COMPUTED.** (i) Lemma 8.7 shows
that when LOST is false every danger pair's cell c works, so the tie-breaking "smallest c" of SPEC §5 is
irrelevant for soundness, and by Lemma 8.5(d) the pair with a given c is unique. (ii) By Lemma 8.5(c) the cell
c of a pair (a,b) lies on no line of another pair (a′,b′) unless a′ = a or b′ = b ("c ∈ b′ forces b = b′").
(iii) E3 is necessary, not sufficient, for SP to have no saving move. PROVED: if, in the situation of Lemma
8.7, the danger pairs of X′ are exactly (a,b), (a′,b′), (a,b′) with a ≠ a′ and b ≠ b′, then E3 fits (by the
literal check in the proof of Lemma 8.7, applied to (a,b) and (a′,b′)), although the common cell of (a,b′)
lies on a line of each of the three pairs, so that X′ is admissible after SP takes it. The tree then
declares the state LOST although the position is not lost; this only makes (H) harder to satisfy.
COMPUTED: such positions exist on the 6×6×6 board — in 622 of the 131 217 danger situations of
`fuzz_l4.py` (Appendix A) E3 fitted while some pair's cell resolved every pair; no hand-checkable instance
is given here. By Proposition 8.4a such a position has at least 12 FP stones, so it cannot occur at an
expanded state of a tree with R_max = 10; the conservativeness is immaterial for the certified tree.

### 8.3 The invariant

**Theorem 8.9 (L4). PROVED (given a valid certificate).** Let SP play σ. As long as σ is in TABLE mode, at
the start of every round (FP to move; t rounds completed, X = FP's stones, B, F as kept by σ):

* (I1) the canonical form of P(B,F,t) lies in the reachable set V (so P(B,F,t) is a non-VOID WON node);
* (I2) B is exactly the set of SP's stones, |B| = t;
* (I3) F ⊆ X and |X| = t;
* (I4) X is B-admissible.

Moreover σ never meets a strategy hole, a LOST-pattern bug or a certificate bug, and every cell σ prescribes
is empty.

*Proof.* Induction on t. For t = 0 everything is empty and canon(root) ∈ V. Assume (I1)–(I4) at the start of
round t+1 and write Pᶜ = canon P(B,F,t) = g(P(B,F,t)) ∈ V.

*Leaf.* If Pᶜ ∈ Lv, then LEAF(B,F,t) holds ((V1) and Lemma 7.3), so C(B,F,t) holds (Theorem 3.2). X is
B-admissible, F ⊆ X, |X| = t, so (B,X) has a pairing: σ computes one and switches to PAIRING mode. Nothing more
is claimed for this branch here.

*Expanded node.* Otherwise t < R_max and LOST(B,F,t) is false ((V2), Lemma 7.3), and the canonical form of
every non-VOID child of P(B,F,t) is in V (the children of Pᶜ are the g-images of those of P(B,F,t); Lemma
7.4(a)). FP plays an empty cell x; X′ = X+x, |X′| = t+1. Exactly one of the following three cases holds;
σ distinguishes them on the real board (steps 3, 1, 2 of SPEC §5).

*Case 1: no live line has 5 cells of X′ and X′ has no danger pair.* Then X′ is B-admissible. σ is in step 3
with the Q-state (B,F,t+1), which is child (a) and non-VOID, so its canonical form is in V. We show that
step 3 preserves the loop invariant "(J): canon Q(B,F,t+1) ∈ V, F ⊆ X′, X′ B-admissible", and terminates.
By (V3) and Lemma 7.4(b) the table yields a cell m ∉ B∪F (no hole). If m ∈ X′, then m ∈ X′∖F, so |F| < t+1
and child (ii) Q(B,F+m,t+1) exists; it is not VOID because F+m ⊆ X′ is B-admissible (Lemma 2.1(a)); so its
canonical form is in V and (J) holds for F+m. As |F| grows and F ⊆ X′, this happens at most t+1 times. If
m ∉ X′, then m is empty; SP takes it. Child (i) P(B+m,F,t+1) has its canonical form in V, X′ is
(B+m)-admissible (Lemma 2.1(b)), and (I1)–(I4) hold for round t+2.

*Case 2: a live line l has 5 cells of X′.* It is unique and contains x; σ takes its sixth cell e (empty) and
sets F′ = F ∪ (l∖{e}), B′ = B+e. Since LOST(B,F,t) is false, X′ is B′-admissible (Lemma 8.6). The new state
P(B′,F′,t+1) is a child (c) of P(B,F,t): l is live, e ∈ l∖F (e ∉ X′ ⊇ F), and F ∪ (l∖{e}) ⊆ X′ has at most
t+1 cells. It is not VOID because F′ ⊆ X′ (Lemma 2.1(a)). So (I1)–(I4) hold for round t+2.

*Case 3: no live line has 5 cells of X′, but X′ has danger pairs.* σ picks a danger pair (a,b) with common
cell c, takes c (empty) and sets F′ = F ∪ (X′∩a) ∪ (X′∩b), B′ = B+c. Since LOST(B,F,t) is false, X′ is
B′-admissible (Lemma 8.7). The new state is a child (d) of P(B,F,t) with Q_a = X′∩a, Q_b = X′∩b: a∩b = {c},
c ∉ F, |Q_a| = |Q_b| = 4, c ∉ Q_a∪Q_b, F∩a ⊆ Q_a, F∩b ⊆ Q_b, and F∪Q_a∪Q_b ⊆ X′ has at most t+1 cells. It is not
VOID because F′ ⊆ X′. So (I1)–(I4) hold for round t+2.

In all cases the stone-count conditions of (c), (d), (ii), E2, E3 were verified from "everything is a subset
of X′ and |X′| = t+1"; VOID children were never entered; and after SP's move X′ is admissible for the new B,
so σ reports no LOST-pattern bug. ∎

**Theorem 8.10 (= Theorem 0.2). PROVED from (H).** Let SP play σ (Definition 8.3). Then (a) whenever SP is
to move, σ prescribes a cell and this cell is empty; (b) FP never owns all six cells of a line.

*Proof.* (b) A line owned by FP contains no SP stone. While σ is in TABLE mode, X is B-admissible at the
start of each round (I4), so after FP's move every live line has at most 5 cells of X′ (Lemma 8.5(a)), and
every other line contains an SP stone: FP owns no line after his move, nor after SP's reply. By (V2) a
P-state of V that is not a leaf has t < R_max, so TABLE mode lasts at most R_max rounds: either the board is
full before, or at the start of some round σ switches to PAIRING mode. At that moment FP is to move, B is
the set of all SP stones, |B| = |X| = t ((I2), (I3)), and σ holds a pairing for (B,X) (Theorem 8.9, leaf
case); Lemma 1.1 applies from then on.
(a) In TABLE mode this is the last sentence of Theorem 8.9: the look-ups never fail (no strategy hole), the
reveal loop of step 3 terminates, the cells e, c, m that σ takes are empty, no LOST-pattern bug is reported,
and at a leaf the pairing that σ needs exists (no certificate bug). In PAIRING mode the rule of Lemma 1.1
names an empty cell whenever SP is to move (first sentence of its proof). ∎

### 8.4 Lazy monotonicity and the eager rule

**Proposition 8.11 (lazy monotonicity). PROVED; not needed for Theorem 0.2.** Let F ⊆ F′, F′ B-admissible.
(a) If P(B,F,t) is WON and |F′| ≤ t, then P(B,F′,t) is WON. (b) If Q(B,F,t) is WON and |F′| ≤ t, then
Q(B,F′,t) is WON.

*Proof.* Induction on the rank (Lemma 8.2) of the state with F. First, LOST is antitone in F: if a pattern
fits for (B,F′,t), the same lines, cells and stone sets fit for (B,F,t) (the conditions "∉ F", "F∩a ⊆ S_a" and
"|F ∪ …| ≤ t+1" are inherited by subsets).
(a) If LEAF(B,F,t): LEAF(B,F′,t) by (M1). Otherwise t < R_max, LOST(B,F,t) is false, hence LOST(B,F′,t) is
false. Child (a): Q(B,F,t+1) is WON, so Q(B,F′,t+1) is WON by (b) (smaller rank). A child (c) of (B,F′,t),
given by (l,e): then e ∉ F and |F∪(l∖{e})| ≤ |F′∪(l∖{e})| ≤ t+1, so (l,e) also gives a child (c) of (B,F,t),
which is WON or VOID. If the child for F′ is not VOID, its revealed set F′∪(l∖{e}) is (B+e)-admissible and
contains F∪(l∖{e}); so the child for F is not VOID (Lemma 2.1(a)), hence WON, and the child for F′ is WON by
induction. Children (d): the same argument with (a,b,c,Q_a,Q_b).
(b) Let m be a table move for Q(B,F,t). If m ∈ F′: |F| < |F′| ≤ t, so child (ii) Q(B,F+m,t) exists; it is not
VOID (F+m ⊆ F′), hence WON, and Q(B,F′,t) is WON by induction (F+m ⊆ F′, smaller rank). If m ∉ F′: use the
same m for F′. Child (i): P(B+m,F,t) is WON and F′ is (B+m)-admissible, so P(B+m,F′,t) is WON by (a). Child
(ii), if |F′| < t: if F′+m is not B-admissible it is VOID; otherwise F+m ⊆ F′+m is admissible, child (ii) for
F exists (|F| < t) and is WON, and Q(B,F′+m,t) is WON by induction. ∎

**Corollary 8.12 (the eager rule (b) of the task statement). PROVED.** Let WON⁺ be defined like WON with the
additional requirement in (P2) that every child (b) Q(B, F+h, t+1), h a busy cell not in B∪F, is WON⁺ or
VOID ("FP took the busy cell h, and it is revealed at once"). Then WON ⇒ WON⁺ for every state; in
particular the eager tree has a WON root whenever the lazy tree has.

*Proof.* Induction on the rank. The requirements of WON⁺ other than (b) follow from those of WON by
induction. For (b): Q(B,F,t+1) is WON (child (a)); if F+h is B-admissible, Q(B,F+h,t+1) is WON by Proposition
8.11(b), hence WON⁺ by induction (it has smaller rank than P(B,F,t)); otherwise it is VOID. ∎

The eager rule is the special case in which SP probes all busy cells after every FP move: he may at any
time replace F by any F′ with F ⊆ F′ ⊆ X (the invariants (I2)–(I4) are unaffected, and (I1) survives in the
sense "P(B,F′,t) is WON" by Proposition 8.11), but then he needs table entries for the states with F′. The
lazy tree avoids the extra branching and needs no such entries; Theorem 8.9 is self-contained without 8.11.

## 9. What is and is not proved

| claim | status | where / which program |
|---|---|---|
| Geometry (G1)–(G3): 148 lines, two cells on ≤ 1 line, 1152 line-preserving bijections forming a group | PROVED; also COMPUTED | Lemma 0.1, 7.1, 7.2; `geom.hpp` constructor, `logs/proof/check_misc.py`; own geometry required of `treecheck`, `sim`, `py/geom.py` |
| L1 pairing strategy; L2 Hall/deficiency/max-flow | PROVED (CITED: max-flow/min-cut, integrality) | Lemmas 1.1, 1.2 |
| Admissibility: downward closed, monotone in B | PROVED | Lemma 2.1 |
| LEAF ⇒ C | PROVED | Theorem 3.2 |
| Triple reduction, μ(T) by augmentation, pruning, quick rejection | PROVED | Propositions 4.2, 4.3, Corollary 4.4 |
| LEAF monotone in F, k, B | PROVED | Proposition 5.1 |
| L3(a),(b); L3(c): false in general, true for B-admissible F | PROVED, (c) not used | Propositions 5.2, 5.4 |
| F = ∅, k ≤ 10: C ⇔ LEAF | PROVED | Proposition 6.2 |
| Symmetry: invariance, canonical forms, move translation | PROVED | Lemmas 7.3, 7.4 |
| Patterns E2 / E3 have ≥ 11 / ≥ 12 distinct stones; LOST false for t ≤ 9 | PROVED (minimum 13 for E3 on this board: COMPUTED, not used) | Lemma 8.4, Proposition 8.4a; Remark 8.4b |
| L4: σ is legal and total, stays inside the verified tree; FP never owns a line | PROVED from (H) | Theorems 8.9, 8.10 |
| Lazy monotonicity, eager rule | PROVED, not needed | Proposition 8.11, Corollary 8.12 |
| Strategy stealing ⇒ draw | PROVED (CITED: determinacy) | Proposition 0.3, Corollary 0.4 |
| Existence of instances with C true and LEAF false; of positions where E3 fits but SP has a saving move | first: PROVED (one instance by hand) and COMPUTED; second: COMPUTED only; neither is used | Remarks 3.3(iii), 8.8(iii) |
| **(H): a valid certificate exists (root WON)** | **COMPUTED — the only computed ingredient** | found by `bin/search --nonbusy`, R_max = 10; re-checked by `bin/treecheck --exact --paranoid` (independent: own geometry, own LEAF max-flow, own LOST, own canonical forms), by `verify/spotcheck.py` (third implementation, exhaustive) and by `bin/search --selfcheck`; §9.1 |

### 9.1 The certified tree (COMPUTED)

Everything in this subsection is COMPUTED. The programs were written and run by other agents. For this
revision the proof writer (a) read the result files named below, (b) recounted from the two certificate
files, with a 20-line script of his own, the number of lines, the leaves by t and by θ and the number of
non-busy table moves — all equal to the figures below —, and (c) compared sha256 sums of certificate files
where stated. He did not run or audit `bin/search`, `bin/treecheck` or `verify/spotcheck.py`.

* **Certificate.** `results/final/strategy.tbl`: 93 164 lines, one per Q-state, each with its table move;
  434 of the table moves are non-busy cells (16 at t = 7, 399 at t = 8, 11 at t = 9, 8 at t = 10).
  `results/final/leaves.txt`: 106 723 leaves. R_max = 10. Produced by
  `bin/search --rmax 10 --maxcand 4 --relcand 10 --nonbusy` (`results/final/stats.json`,
  `logs/final/search.log`).
* **Reachable set** (Definition 8.1): 153 135 P-states, of which 46 412 are expanded and 106 723 are leaves,
  and 93 164 Q-states; 246 299 states in all; largest t = 10.
* **Leaves** by t: 3 382 (t = 8), 87 314 (t = 9), 16 027 (t = 10). By θ = t − |F| (the number of unrevealed FP
  stones the leaf must withstand): 64 841 / 32 703 / 7 267 / 1 434 / 343 / 105 / 25 / 4 / 1 for θ = 0, …, 8.
* **Verification (i): `bin/treecheck --exact --paranoid`**, `results/final/treecheck.json`
  (`logs/final/treecheck.log`): ok, no error, 246 299 states visited (153 135 P, 93 164 Q), 106 723 leaves
  recomputed, 143 498 threat children (c) and 28 275 danger children (d) (non-VOID, counted with
  repetitions), 151 VOID children, all of type (ii) (no child (c) or (d) of the tree is VOID), no table or
  leaf line malformed, non-canonical or duplicated. Options, by the program's own description: `--exact`
  keys the visited set by the full states instead of 128-bit hashes, which removes the only probabilistic
  step of the traversal; `--paranoid` constructs and literally validates every flow behind a verdict
  "LEAF holds". (The look-ups in the table and the leaf file go through 120-bit hashes also with `--exact`;
  the header of `verify/treecheck.cpp` argues that a collision there cannot make the program accept a state
  that is not WON, because a move that is found is checked and its children are verified, and a state taken
  for a listed leaf must pass the LEAF computation. Verification (ii) uses exact keys throughout.)
* **Verification (ii): `verify/spotcheck.py`**, a third implementation: pure Python, written by a different
  author from SPEC §§1, 3, 4, 6 alone, own geometry, canonical form, max-flow and LOST, exact keys. Exhaustive
  mode (`--full --all-leaves`): ok, 246 299 states, 46 412 expanded P-states, 93 164 Q-states, 143 498
  children (c), 28 275 children (d), 151 VOID children (all of type (ii)), LEAF recomputed for all 106 723
  leaves, every table line and every leaf line reached and in canonical form
  (`results/final/spotcheck_full.json`, `logs/final/spotcheck_full.log`). All counters equal those of
  `treecheck`. (An earlier exhaustive run, `logs/review/spotcheck/provisional_full.json`, gave the same
  result on `results/search/r10_nb_k4_rebuilt/`, whose two files are byte-identical with the two files in
  `results/final/` — same sha256, compared for this revision.)
* **Verification (iii): `bin/search --selfcheck`**, a re-walk of the emitted files inside the search
  program with string-keyed maps, a naive child enumeration and a literal transcription of E2/E3: 46 412
  expanded P-states, 93 164 Q-states, 106 723 leaves, 0 errors (`logs/final/search.log`; `"selfcheck":"ok"` in
  `stats.json`). It shares the LEAF code (`src/phi.hpp`) and the geometry with the search, so it is
  independent of `treecheck` and `spotcheck.py`, not of `bin/search`.
* **LOST.** By Proposition 8.4a (PROVED), LOST is false at every expanded state of a tree with R_max = 10.
  Consistently the search ran its exact LOST enumeration 4 307 times and never found a fitting pattern
  (`"lost_exact":4307,"lost_true":0` in `stats.json`). The LOST code of the programs is therefore not
  really exercised by this certificate, and its correctness is not needed for (H) with R_max = 10.
* **Busy-only table moves do NOT suffice.** With table moves restricted to the 24 busy cells — the default
  of SPEC §4 — the root is not WON for any R_max ≤ 10: `bin/search` fails already in the relaxed lazy game
  in which FP never forces (no threats, no dangers, no LOST), with all 24 busy cells tried at every Q-state
  (exhaustive; `logs/search/busyonly_r9_rel24.log`, `logs/search/busyonly_r10_rel24.log`,
  `results/search/busyonly_r10_rel24/stats.json`; a tree for R_max is also a tree for every larger R_max, so
  the failure for 10 covers all smaller values). The failing branch lets FP hold the four corners of a
  face. This is a result of `bin/search` alone; no independent program has re-checked it, and it concerns
  only the necessity of non-busy moves, not the theorem. The certified strategy therefore uses non-busy
  table moves (434 of them, SPEC §10 erratum 2). This is covered by the proof: §8 never uses that a table
  move is a busy cell (the rules (i), (ii) are the same for every cell m ∉ B∪F).
* **Other cross-checks.** Simulator games against several adversaries (`bin/sim`), integer programs
  (`py/ip_check.py`), exact enumeration of C on the leaves (`bin/enumcheck`), `treecheck --crosscheck`, the
  code audits and the regression values are recorded by the coordinator in REPORT.md. They are tests of the files and
  of the programs and are **not part of the proof**; this file makes no claim about their outcome.

### 9.2 What re-checks the computed part, and what is not proved

Where the final claim rests on computation, and what re-checks it:

1. **LEAF at every reachable leaf** ((V1)): exact integer max-flow in `treecheck` and, independently, in
   `spotcheck.py`; the same predicate is computed by `src/phi.hpp` (`phicheck`, `search --selfcheck`). The
   integer program `py/ip_check.py` decides C itself on samples; it is floating point and counts as a
   plausibility check only (SPEC §7.7).
2. **LOST false at every expanded node, every child present, a table move for every Q-state** ((V2), (V3) and
   the closure of V): `treecheck` and `spotcheck.py` each re-expand the whole tree from the root. For
   R_max = 10 the first of these conditions also follows from Proposition 8.4a.
3. **σ as a program**: `bin/sim` plays σ against adversaries and reports holes, pairing failures, admissibility
   failures and FP lines. By Theorem 8.9 none can occur if the certificate is valid; the simulator is a test of
   the files and of the programs, not part of the proof.
4. Regression values of SPEC §8 and the equivalence of §6 serve to cross-validate `phi.hpp`, `match.hpp` and
   the IP; the theorem does not depend on them.

What is **not** proved here:

* (H) itself.
* The correctness of the programs. In particular the statement "`bin/treecheck` answers ok only for a valid
  certificate (Definition 8.1)" is a property of a program. It is established — to the extent that it is —
  by the code audit of `verify/treecheck.cpp` (`logs/review/treecheck_audit.md`, completed after
  this revision; the mathematical referee read only its `process()` / `pushChild()` and header) and by the
  agreement with two further re-implementations (`verify/spotcheck.py`, `bin/search --selfcheck`), not by
  this file. The trusted base is one independent verifier — `verify/treecheck.cpp` with its compiler, or
  `verify/spotcheck.py` with the Python interpreter — and the machine.
* Nothing about hashing is proved or assumed here. The certifying `treecheck` run must therefore use
  `--exact` (it does: `"exact_visited_set":true` in `results/final/treecheck.json`; also
  `"paranoid_flows":true`). Without `--exact`, (H) would additionally rest on the absence of a collision of
  128-bit hashes in the visited set (probability < 1e-22 by the program's own estimate). For the hashed
  look-ups that remain with `--exact` see §9.1, verification (i).
* Minimality of R_max or of the table; anything about states for which LEAF fails (LEAF is only sufficient
  for C, Remark 3.3); whether SP could do with less than the conservative LOST patterns (Remark 8.8).

The proof makes no use of floating-point results, of the busy cells, of the regression values, or of L3(c).

## Appendix A. Self-checks run by the proof writer (COMPUTED, plausibility only)

Scripts are in `logs/proof/` (Python, and one C++ file `e23min.cpp`; own geometry, brute force; nothing of
`src/` is used; usage in the first lines of each script; raw outputs in `check_misc.log`,
`fuzz_*_run1.log`, `toy_run1.log`, `toy_run2.log`, `check_rev.log`, `e23min.log`). They are not part of the
proof; they were written to try to break the statements above. No assertion failed.

* **`check_misc.py`** (real board). 148 lines; cell degrees 3 (48 cells), 4 (144), 7 (the 24 busy cells);
  the 2304 triples (π, r, s) give 1152 distinct maps, all bijective and line-preserving; 46 080 random
  products and all 1152 inverses lie in G. Lemma 6.1 for all 25 176 connected triples of the empty board
  (1464 triangles, 1464 stars, 22 248 paths), each with the three choices of the 4-stone line: no
  inadmissible placement. The counterexample of Proposition 5.4(i): F not ∅-admissible, {b}-admissible, no
  pairing for ({b},F) (141 live lines, 210 free cells).
* **`fuzz_small.py`** (definitions generalised to line length n: cap n−2, 2 cells per line; boards 4×4×4,
  6×6, 5×5, and 6×6×6 / 5×5×5 with all lines but at most 9 killed; at most 11 live lines, so that every
  quantity is computed by brute force over all sets of live lines and all X). 12 500 random instances
  (B,F,k) with k − |F| ≤ 3 (≤ 2 on the two restricted boards). Checked: Lemma 1.2(b) against the
  brute-force deficiency (12 500); LEAF ⇒ C
  (8660 instances with LEAF, all satisfy C; 246 instances have C without LEAF); Remark 3.3(ii) (8660);
  Proposition 6.2 for k ≤ 3 only (50 763 comparisons); Proposition 4.2(b) (9871); μ by augmentation =
  brute-force minimum for 87 145 F-connected triples and their 1- and 2-line subsets; Corollary 4.4(b)
  (87 145); (M1) 7313, (M2) 9901, (M3) 167 373 checks; Proposition 5.4(ii) for 130 838 quadruples (B,F,k,b).
* **`fuzz_cprime.py`** (Proposition 5.4(ii), F stacked on the lines through b; boards 4³, 5³, 6³, 6²
  restricted to at most 11 live lines; k − |F| ≤ 3). 5574 instances with C(B,F,k) true; 447 456 sets X that
  are (B+b)-admissible, 244 718 of them not B-admissible (cap on a line through b: 91 691; danger pair at b:
  148 773; danger pair (l₀,a₀): 18 042). Every one has a pairing for B+b.
* **`fuzz_l4.py`** (real board, the case analysis of §8.2 with the literal definitions of SPEC §3/§4).
  45 000 random positions (B up to 24 stones, X B-admissible with 6–45 stones concentrated on a few lines, F
  a random subset of X) and all 7 972 990 FP moves from them. Lemma 8.5 held everywhere. 363 222 threats:
  X′ was inadmissible after SP's forced reply in 45 922 of them, exactly when a surviving danger pair
  existed, and each of the 62 306 surviving pairs satisfied every clause of E2. 131 217 danger situations
  (21 056 with several pairs): for every choice of the pair, admissibility after SP's reply was equivalent
  to "every other pair shares a line with it"; two pairs with different new and different old lines
  occurred in 7694 situations and satisfied every clause of E3 (16 768 pattern instances); in 622 of these
  some pair's cell still resolved everything (Remark 8.8(iii)). The conditions of the children (c), (d) and
  their being non-VOID held in all admissible cases.
* **`toy_tree.py`** (end-to-end test of SPEC §4 + §5, line length 4). WON is evaluated by the literal
  recursion, then σ is played against **every** FP move sequence, PAIRING mode included, asserting (I1)–(I4),
  "no hole", "pairing exists at a leaf", "X′ admissible after SP's move", "no FP line". On the affine plane
  AG(2,4) with 4 parallel classes (16 lines), 36 random start positions with 1–3 revealed stones per side,
  R_max = 8: 27 roots WON; σ visited 2744 TABLE positions (466 threats, 429 dangers, 340 lazy reveals), 2386
  leaf arrivals and 274 734 PAIRING positions without any failure. Control run with LOST switched off: all
  36 roots "WON" and σ ran into 1044 positions with X′ inadmissible — the test does detect an unsound tree.
  (AG(2,4) with all 5 classes, 36 starts: no root WON with LOST; with LOST off 36 "WON" and 13 314 such
  positions.)
  Standard 4×4 board: root WON with R_max = 2 (34 992 PAIRING positions, no FP line); AG(2,4) with 3
  classes: root WON (32 reveals, 86 751 PAIRING positions, no failure). 5×5 torus (line length 5) with 4
  parallel classes: root WON; TABLE mode searched exhaustively (2600 positions, 899 lazy reveals, 2275 leaf
  arrivals, a pairing at each), PAIRING mode not searched (too large for this script).
* **`check_rev.py`, `e23min.cpp`** (real board; added for the revision; outputs `check_rev.log`,
  `e23min.log`). `check_rev.py`: the instance of Remark 3.3(iii) — L(B) = T, sur_∅(T) = 10, 45 292
  B-admissible sets X among the 2¹⁶ subsets of N(T), the largest with 10 cells, each with Hall's condition
  and an explicit pairing; and the line-level count of Remark 8.4b (257 616 line configurations (l,a,b,e) of
  E2, 35 136 of them with l∩b ≠ ∅; 926 640 ordered line configurations of E3, with 0 / 1 / 2 / 3 meeting
  pairs among (a,b′), (a′,b), (b,b′) in 583 488 / 213 264 / 129 888 / 0 cases). `e23min.cpp`: the literal
  enumeration of all E2 and E3 patterns with their stone sets on the empty board quoted in Remark 8.4b
  (minimum 11 distinct stones for E2, 13 for E3).

Limits of these checks: small boards and small k − |F| (≤ 3, ≤ 2 on the restricted 3-dimensional boards)
for the brute-force parts; Proposition 6.2 was only brute-forced for k ≤ 3; the end-to-end test uses line
lengths 4 and 5, not 6; Proposition 8.11 and Corollary 8.12 were not tested by program (the referee ran
Proposition 8.11 on 7 toy trees with a script that reuses the WON recursion of `toy_tree.py`).
**VOID children are hardly exercised by the toy trees:** among the runs of `toy_tree.py` with LOST switched
on and a WON root, VOID children of type (c)/(d) occurred in one tree only (2 children; `toy_run1.log`,
board `4a4`, start `['3','8']`); all other such trees report `'void': 0`, and so do six further runs made
by the referee (`logs/review/toy_void/toy_void_h4.log`); the 16-cell boards are too small to produce them
in WON trees. (The script's counter does not count VOID children of type (ii).) The VOID rule is therefore
covered by the hand proof (Lemma 2.1(a) with Lemmas 8.6 and 8.7, used in Theorem 8.9), by the referee's
assertion in `logs/review/ref_l4.py` that the real child (c)/(d) is never VOID when X′ is admissible after
SP's reply, and by `bin/sim` on the real tree — not by the toy test. In the certified tree itself no child
(c) or (d) is VOID; its 151 VOID children are all of type (ii) (§9.1).

## References

* [B] J. Beck, *Combinatorial Games: Tic-Tac-Toe Theory*, Cambridge University Press, 2008.
* [FF] L. R. Ford, D. R. Fulkerson, Maximal flow through a network, *Canad. J. Math.* 8 (1956), 399–404
  (max-flow = min-cut; integral capacities give an integral maximum flow).
* [H] P. Hall, On representatives of subsets, *J. London Math. Soc.* 10 (1935), 26–30.
* [HJ] A. W. Hales, R. I. Jewett, Regularity and positional games, *Trans. Amer. Math. Soc.* 106 (1963),
  222–229.
* E. Zermelo, Über eine Anwendung der Mengenlehre auf die Theorie des Schachspiels, *Proc. 5th Int. Congress
  of Mathematicians* (1913), 501–504 (determinacy of finite games; any textbook proof by backward induction).

## Revision log

Revision of 2026-11-12 (after `logs/review/math_referee.md`, which reviewed the version with sha256
`5afc08cd…fbc`, 762 lines, and found no false statement). No numbered statement was renumbered. The
statements of the reviewed version whose content changed are: Theorem 0.2 / Corollary 0.4 / Theorem 8.10
(item 1: strategy named precisely, part (a) added), the parenthetical of Proposition 4.3 (item 2:
corrected), the labels of Remarks 3.3 and 8.8 (item 3), the hypothesis of Lemma 1.1 (item 5: made
explicit). Everything else that changed is status text, §9 and Appendix A. New numbers: 8.4a, 8.4b, §9.1,
§9.2 (the heading 9.2 was put over the two closing paragraphs of the old §9).

1. *Theorem 0.2, Corollary 0.4, Theorem 8.10, Definition 8.3* (referee finding 1). The theorem is now stated
   for the strategy σ of Definition 8.3 — SPEC §5 with the PAIRING-mode rule of Lemma 1.1 ("take the partner
   cell if it is empty, otherwise any empty cell"; SPEC §10 erratum 1 now says the same) — and has a new
   part (a): σ is a legal, total strategy (it always prescribes an empty cell). Theorem 8.10 states and
   proves (a) from the last sentence of Theorem 8.9 and from Lemma 1.1; the conclusion of Lemma 1.1 now
   says explicitly that its rule names an empty cell. Corollary 0.4 names σ.
2. *Proposition 4.3* (finding 2). The parenthetical "(any capacity ≥ 6 gives the same)" was wrong for part
   (c). Now: capacity ≥ 6 gives the same maximum flow value, hence (a), (b), (d); (c) needs the arcs s→l,
   l ∈ U, unsaturated (capacity ≥ 7), or R replaced by R ∪ U. The proof of (c) was re-checked and expanded
   (why U ⊆ R, why lines of R∖U are F-adjacent to earlier lines of R, the variant for capacity exactly 6
   with the counterexample to the old wording).
3. *Remarks 3.3(iii) and 8.8(iii)* (finding 3). The labels now separate the PROVED implications from the
   existence claims. 3.3(iii): the referee's hand-checkable instance (three lines through cell 0 in the
   plane x = 0, B = the 200 cells outside their union, F = ∅, k = 11) was added with a complete hand proof
   and re-verified by a new script (`logs/proof/check_rev.py`: 45 292 admissible X, all with a pairing); the
   instances of Appendix A are labelled COMPUTED. 8.8(iii): the implication is stated precisely and
   labelled PROVED; that such positions occur is labelled COMPUTED (622 observations).
4. *§0.2 and §9* (finding 4). "Validity of the certificate files is exactly what `bin/treecheck` verifies" is
   now presented as a property of a program, established by the code audit
   (`logs/review/treecheck_audit.md`, completed: sound, minor findings only) and by two further re-implementations, not by this file.
   The certifying run uses `--exact` and `--paranoid`; what would be assumed without `--exact` is stated.
5. *Lemma 1.1* (finding 5). The hypothesis |B| = |X| (FP to move in a game position) is now explicit in the
   statement and in the first sentence of the proof.
6. *"Read this first" item 1 and §9* (finding 6 and the final run). The provisional status text was
   replaced by the final computed facts, labelled COMPUTED, with file names (new §9.1): certificate sizes,
   434 non-busy table moves, R_max = 10, 153 135 P-states (46 412 expanded), the three verifications
   (`treecheck --exact --paranoid`, `spotcheck.py` exhaustive, `search --selfcheck`), leaves by t and by θ,
   the failure of busy-only table moves, and the statement that the simulator / IP / enumeration
   cross-checks belong to REPORT.md and not to the proof. Items 2, 4(b), 4(c), 4(g) of "Read this first"
   were adjusted accordingly; item 6 was added.
7. *Appendix A* (finding 7). New limit: VOID children (c)/(d) occurred in only one WON toy tree. The new
   scripts `check_rev.py` and `e23min.cpp` are listed.
8. *New Proposition 8.4a* (observation of the third-implementation reviewer). PROVED: an E2 pattern has
   exactly 12 − |S_l∩S_b| ≥ 11 distinct stones, an E3 pattern 15 − (number of meeting pairs among
   (a,b′), (a′,b), (b,b′)) ≥ 12, with the list of which stone sets can overlap and why; hence LOST(B,F,t) is
   false for t ≤ 9, in particular at every expanded state of a tree with R_max = 10. Lemma 8.4 is unchanged
   (one cross-reference added).
9. *New Remark 8.4b* (observation of the author of `treecheck`). COMPUTED, not used: on this board no E3
   pattern has 12 stones; the minimum is 13 (literal enumeration `logs/proof/e23min.cpp`, line-level count
   in `check_rev.py`); for E2 the minimum 11 is attained.
10. The table of §9 has three new or changed rows (stone counts, legality of σ, existence claims) and an
    extended row for (H); the paragraph "What is not proved here" was turned into a list and extended by
    the program-property statement of item 4 and by the `--exact` condition. The definition of the label
    COMPUTED in the header and the reference to the errata of SPEC §10 were adjusted.
11. Outside this file: the scripts `logs/proof/check_rev.py` and `logs/proof/e23min.cpp` with their logs
    were added, and two notes were appended to `logs/spec_issues.md` (the wording of SPEC §10 erratum 6
    about hashed look-ups, see §9.1; the exact stone counts of Proposition 8.4a / Remark 8.4b).
12. Not changed (word for word as reviewed by the referee): §0.1, Proposition 0.3, §0.3, Lemma 1.2, §2,
    Lemma 3.1, Theorem 3.2, Lemma 4.1, Proposition 4.2, Corollary 4.4, §§5–7, Definition 8.1, Lemma 8.2,
    Lemmas 8.5–8.7, Theorem 8.9, Proposition 8.11, Corollary 8.12, the References.
13. *Coordinator, 08:38 UTC, status phrases only* (no statement or proof touched): the five places that said
    REPORT.md was "not yet written" or the audit of `verify/treecheck.cpp` was "in progress" now say that
    REPORT.md exists and that the audit is complete (no fatal or major finding). After this revision the lead
    reviewer (REVIEW.md) and the verifier's auditor each re-established (H) with programs of their own, so (H)
    has been verified by five programs of four authors besides the search's self-check; see REPORT.md §4.

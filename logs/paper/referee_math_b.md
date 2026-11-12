# Referee report 1b — "The 6x6x6 tic-tac-toe game is a draw"

Manuscript: `paper/paper.pdf` (73 pp.), sources `paper/*.tex`, state of 2026-11-12.
Numbers of statements are those of the PDF; source locations are `file:line`.

## Scope

Theorem 1.1 and Corollary 1.2 (statements and dependencies); Section 5 in full (Def. 5.1-5.3, Lemma 5.4,
Lemma 5.5, Def. 5.6, Lemma 5.7, Def. 5.8, Prop. 5.9, Lemmas 5.10-5.12, Remark 5.13, Theorems 5.14, 5.15,
Prop. 5.16, Cor. 5.17, proof of Theorem 1.1, Remarks 5.18, 5.21, Prop. 5.19, Cor. 5.20); Appendix C.2 and
C.3 (Lemma C.2, proofs of 5.19 / 5.20, Prop. C.3, Fact C.4, Example C.5); Section 9.1 (Lemmas 9.1, 9.2,
Cor. 9.3, Remark 9.4) with Appendix D / Table 13; Prop. 9.5; Algorithm 1 and Prop. 7.1.

Not in my scope and only read as far as they are used: Sections 2-4 (I checked the statements that Section 5
quotes, and read the proofs of Lemma 3.2, Lemma 4.2, Theorem 4.9, Lemma 4.19, Cor. 4.21, Lemma 2.8), the
predicate LEAF as an algorithm (Algorithm 2, Prop. 7.2), Sections 6, 8, Appendices A, B, C.1.

## Verdict for this part

**No fatal and no major finding.** Every statement in the scope was re-derived before its proof was read,
the proofs were then read line by line, and every statement that can be tested on the real board was tested
with independent code (own geometry, own canonical form, own implementation of sigma and of Algorithm 1).
All tests passed. I found 3 minor points (two remarks in Section 9.1 that claim slightly more than is
proved or checked, one unproved side remark in Section 7.1) and 6 editorial points. Recommendation for this
part: **minor revision**.

## What was checked, and how

### A. By hand (re-derivation, then line-by-line reading)

* **Theorem 1.1, dependencies.** The chain is: Fact 6.1 (root WON for R_max = 10) -> Lemma 5.7(c) (valid
  certificate) -> Def. 5.8 (sigma) -> Theorem 5.14 (invariant (I1)-(I4); uses Lemma 5.5, Lemma 5.7(a),(b),
  Lemmas 5.10-5.12, Lemma 4.2(a),(b), Theorem 4.9, Lemma 2.8) -> Theorem 5.15 (uses Lemma 3.2) -> first
  assertion; second assertion: Theorem 5.15 + Prop. 5.16 + Zermelo -> Cor. 5.17. Geometry enters only through
  Lemma 2.1(c) and Lemma 2.6. I found no circular reference and no statement used before it is proved
  (Remark 5.13 and the comments after Def. 5.8 point forward to Theorem 5.14 / Lemma 5.10, but nothing in
  the proofs depends on them). Besides Fact 6.1 nothing computed is used. The literature results used are
  exactly those named in Section 1.6 (max-flow/min-cut with integrality, via Lemma 3.3; determinacy).
* **Lemma 5.4** (rank): all five kinds of children checked; finiteness holds.
* **Lemma 5.5** (new): (a),(b) are invariance of incidence notions (Lemma 4.24, Lemma 2.6), (c) by
  induction on the rank, (d) is Lemma 2.8(a). Complete.
* **Def. 5.6 / Lemma 5.7** (new): (a) the set of canonical non-VOID states is closed under (R0)-(R2) (uses
  that child (i) and the canonical form of a non-VOID state are non-VOID); (b) induction on the rank, each
  of (V1)-(V3) is used exactly where needed; (c) with Lv = all canonical states with LEAF and Tbl = a table
  move at each canonical WON Q-state, W = canonical non-VOID WON states is closed under (R0)-(R2), so
  V is contained in W, and (V1)-(V3) follow. The definition agrees with Def. 5.3: "valid certificate exists"
  iff "root WON".
* **Prop. 5.9**: both inclusion-exclusion counts re-derived (E2: 13 - 1 - |S_l cap S_b|; E3: 16 - 1 - three
  intersections of size <= 1, all triple intersections empty because each triple contains (a,b) or (a',b')).
* **Lemmas 5.10-5.12**: every sub-claim re-derived; in 5.11 and 5.12 every condition (E2.1)-(E2.3),
  (E3.1)-(E3.3), pairwise distinctness of lines and cells, and "fits" was checked against Def. 5.2.
* **Remark 5.13** (new): the four cases (threat + pair; two pairs with the same new line; with the same old
  line; with different new and old lines) are exhaustive and the counts 11, 11, 11, 12 are right.
* **Theorem 5.14**: invariant and all cases checked: leaf (C(B,F,t) is applied to X, which is admissible,
  contains F and has t cells); threat (child (c) with |F u (l\{e})| <= t+1 because the set lies in X');
  danger (child (d), Q_a = X' cap a, Q_b = X' cap b, all five conditions); quiet move with repeated reveals
  (loop invariant (7): canon Q(B,F,t+1) in V and F subset X'; child (ii) exists because m in X'\F forces
  |F| < t+1; it is not VOID because F+m subset X' and X' is B-admissible; at most |X'\F| repetitions).
  VOID states are never entered; the table move is empty (m not in B u F by (V3), m not in X' in the
  branch where it is played, B = all stones of Breaker by (I2)); LOST false is used only through Lemmas
  5.11/5.12 with the hypotheses F subset X, t = |X| of those lemmas satisfied by (I3).
* **Theorem 5.15**, in particular (c) (new): a round that begins in TABLE mode and does not switch has
  canon P(B,F,t) in V\Lv, hence t < R_max by (V2). Correct.
* **Prop. 5.16** (new proof): invariant (*), legality of every virtual move, and the three endings, for
  both parities of the number of cells (odd case: last move by the first player, the virtual first player
  takes the ghost cell z; one-cell board included). Correct.
* **Cor. 5.17, proof of Theorem 1.1**: correct.
* **Lemma C.2, Prop. 5.19, Cor. 5.20**: inductions on the rank of the state with the smaller F; every
  appeal to the induction hypothesis has smaller rank and satisfies "F' admissible, |F'| <= t"; Prop. 4.18(a)
  is applicable. Correct.
* **Prop. C.3** (new): (a) coplanarity argument (both cases), the claim that the plane W contains a vector
  with one non-zero coordinate (support argument and the parity contradiction), the list of D cap W in the
  four cases (exactly four vectors up to sign, two of them u, u' with u_i = u_k and u'_i = -u'_k), and the
  final contradiction 2 q_i = 5; (b),(c) and both examples. Correct.
* **Example C.5** (new): all line counts, the three danger pairs, the three replies. Correct.
* **Lemma 9.1**: (a)-(d) for all lines (constant coordinate 3; otherwise axis lines, face and space
  diagonals alike: the argument does not distinguish the types). **Lemma 9.2**: the rule with psi and the
  residues of s - r modulo 6. **Cor. 9.3**: (J1), (J2) and the points I was asked to consider:
  (1) if sigma prescribes a cell w that Breaker already holds, Breaker makes a free move and (J1) still
  holds because (J1) only asks that virtual Breaker stones are real Breaker stones; (2) w is never a stone
  of Maker, because Maker's stones in A are exactly the virtual Maker's stones and w is empty in the
  virtual position; (3) parity: the virtual play alternates on its own (it advances only when Maker moves in
  A), after the virtual Maker's move it has an odd number <= 215 of stones, so sigma prescribes an empty
  virtual cell; when the part in which Maker moved prescribes nothing and the other part is full, the free
  move is any empty cell, and if there is none the board (343 cells, Maker moves last) is full. The
  completion argument for a line not in M treats both x in A and x in M (the middle cell). Correct.
* **Prop. 9.5** (new): the unrevealed branch, Prop. 3.6, Cor. 4.21 (needs three live lines: 99 are live),
  and the count "seven cells meet at most 46 lines" (non-busy cell: 6*7+4; all busy: three of one level,
  three distinct lines counted twice). Correct.
* **Algorithm 1 / Prop. 7.1** against Def. 5.1 and Def. 5.6: lines 10-17 enumerate exactly the children
  (a),(c),(d) (Q_a = a\{c,a'} with a' not in F is exactly "Q_a subset a\{c}, |Q_a| = 4, F cap a subset Q_a";
  c not in B is automatic for live lines), lines 19-22 are (V3) and (R2), line 25 discards exactly the VOID
  children. The proof Seen = V is correct.

### B. By program (all under `logs/paper/referee/`, prefix `b_`; own code, exact integer arithmetic)

| script | what it tests | result |
|---|---|---|
| `b_geom.py` | 148 lines, two parametrizations each, any two lines share <= 1 cell, degrees 24x7/144x4/48x3, 1152 distinct maps of G; 193 lines of 7^3, two parametrizations each | OK |
| `b_checks.py` | (1) Prop. C.3(a): all 984 sets of four pairwise meeting lines are concurrent (984 = sum over cells of C(deg,4)); (2) Prop. 9.5: 7 busy cells meet at most 44 lines, 6 busy cells at most 39 (so 7 cells always meet <= 46, in fact <= 44, lines); (3) Lemma 9.1(b),(c),(d) on all 193 lines; Table 13 **parsed from `paper/app_seven.tex`**: 45 distinct lines = exactly the lines inside M, 90 distinct cells, each on its line, 37 cells of M unassigned incl. the centre; the rule in the proof of Lemma 9.2 reproduces the table | OK |
| `b_patterns.cpp` | literal enumeration of Def. 5.2 for B = F = empty: E2: 5,152,320 patterns, 562,176 with 11 and 4,590,144 with 12 stones; E3: 353,422,080 patterns, 10,948,608 / 67,627,008 / 274,846,464 with 13 / 14 / 15 stones, none with 12 | agrees exactly with Prop. 5.9, Prop. C.3(b),(c), Fact C.4 |
| `b_lemmas.py` | Lemma 5.10(a)-(d), Lemma 5.11, Lemma 5.12 (incl. the "more precisely" part), Remark 5.13, Lemma C.2, with LOST evaluated literally from Def. 5.2 at F = X; 6,000 admissible positions with 5-19 stones built to contain near-threats, 46,562 moves, 24,280 threats (4,061 where the reply fails; an E2 pattern fits in every one), 8,861 danger moves (1,455 where a reply fails; an E3 pattern fits in every one; 25 where the choice of the pair matters, as in Example C.5), 7,472 moves with two or more forced events, the smallest with 11 stones; also the examples of Prop. C.3 and Example C.5 literally | all assertions pass |
| `b_sigma.py`, `b_play6.py` | own implementation of sigma (Def. 5.8, files read per Appendix A) against 8 Maker policies (random, greedy, busy cells, "take the cell the table is about to name", two-line danger builder, ...): 8,000 games, 64,659 table moves (21,621 with 1-6 reveals in one round), 3,570 threat replies, 691 danger replies; in every round: (I1) and the loop invariant (7) by direct look-up of canon P(B,F,t) and canon Q(B,F,t+1) in the reachable set V computed by `b_expand.py`, (I2)-(I4), child conditions of Def. 5.1, t < 10 at expanded states, reply cell empty, number of repetitions <= t+1-|F|, admissibility restored, at most one forced event (Remark 5.13), pairing exists at the leaf, at most 10 table moves, the partner never holds a Maker stone, Maker never owns a line (800 games played to the full board) | all assertions pass; switch after round 8 / 9 / 10 in 3,189 / 4,702 / 109 games |
| `b_play7.py` | the strategy of Cor. 9.3 with the parsed Table 13 and sigma as virtual player: 1,000 games to the full board, 5 Maker policies x 4 free-move policies (incl. "exhaust M first", "free moves into A"); (J1), (J2) before every Maker move, legality of every reply, no Maker line; thousands of cases "sigma prescribes a cell Breaker already holds" and "partner already taken" occurred | all assertions pass |
| `b_expand.py` | Algorithm 1 transcribed from the printed pseudocode (LEAF not evaluated: (L1) on all leaves, (L2) on 1,500 leaves only); writes the reachable set V (246,299 canonical states) to the scratch file `/tmp/scratch/b_reachable.pkl` (regenerated by every run), which `b_play6.py` reads if present | 153,135 P-states (46,412 expanded, 106,723 leaves), 93,164 Q-states, per-round counts of P, Q, leaves, threat children (143,498), danger children (28,275), children (ii) (54,482, 151 VOID), no VOID child (c)/(d), 434 non-busy table moves, leaves by theta: all equal to Appendix A.5; every line of both files is reached; t < R_max and (V3) hold on the whole reachable set |
| `b_invariance.py` | Lemma 5.5(a),(b),(d) on 300 random P-states (729k children) and random g; canonical-form test vectors of Appendix A.2 | OK |
| `b_stealing.py`, `b_stealing_proof.py` | statement of Prop. 5.16 by minimax on 2,920 random games with 1-9 cells; bookkeeping of its proof (invariant (*), legality of virtual moves, the three endings) on 200,000 random plays with 1-11 cells, both parities | OK |

Running time: `b_expand.py` 3.4 min, `b_play6.py` 2.3 min, `b_lemmas.py` 1.3 min, `b_play7.py` 0.9 min,
everything else under 20 s (one core, `nice -n 10`). Logs are next to the scripts (`b_*.log`).

What these tests do **not** establish: (V1), i.e. LEAF at the 106,723 leaves (condition (L3) was not
evaluated by me), and nothing about Sections 2-4 beyond the statements used.

## Findings

### 1. (minor) Remark 9.4(iii) claims an implication that the proof does not establish

*Location:* Remark 9.4(iii), `paper/sec_discussion.tex:186-190`.

*Problem:* "that a draw on the board n^d implies a draw on the board (n+1)^d is stated as a hypothesis by
Golomb and Hales. The proof above establishes this implication for d = 3 and n = 6, in the stronger form
for Breaker's win." The proof of Corollary 9.3 shows: *if Breaker wins the Maker-Breaker game on 6^3 as
second player, then Breaker wins it on 7^3*. Both the hypothesis and the conclusion are strengthened, so
this is not a stronger form of "draw implies draw", and it does not establish "draw implies draw": the
argument needs a strategy under which the first player never completes a line, and a drawing strategy of
the ordinary game need not have this property (it may rely on counter-threats). The sentence would be
quoted as "monotonicity is proved for n = 6", which is not what is proved.

*Correction:*
```latex
The proof above establishes the analogous implication for \Breaker's win in the Maker--Breaker game,
for $d=3$ and $n=6$: if \Breaker{} wins on the $6\times6\times6$ board as the second player, then he
wins on the $7\times7\times7$ board. It does not show that a draw of the ordinary game on $6^3$ implies
a draw on $7^3$, and it gives no information on the $5\times5\times5$ board.
```

### 2. (minor) Remark 9.4(ii): the cited program does not check the printed Table 13 nor Lemma 9.1(d)

*Location:* Remark 9.4(ii), `paper/sec_discussion.tex:178-184`; program `scripts/check_7cube_midplanes.py`,
output `logs/paper/check_7cube_midplanes.out`.

*Problem:* The text says "The two lemmas were also confirmed by a short program ..., which enumerates the
193 lines, checks the statements (b)-(d) of Lemma 9.1 and finds a pairing of the 45 lines in M by bipartite
matching." The program computes *some* pairing by augmenting paths; this is a different pairing from
Table 13 (its output assigns, e.g., the centre (3,3,3) to the line (3,t,t), whereas Appendix D states that
the centre is not assigned). It never reads the table. Hence it confirms the existence of a pairing, not
Lemma 9.2, which is a statement about Table 13; and it confirms that 45 lines lie in M, not statement (d)
of Lemma 9.1 ("are the 45 lines listed in Table 13"). Since Corollary 9.3 rests on the printed table, the
only check of the printed table so far is the hand check.

I checked the printed table by parsing `paper/app_seven.tex` (`b_checks.py`): the 45 entries are 45
distinct lines, they are exactly the lines of the 7^3 board contained in M, the 90 cells are distinct,
each lies on its line, and the rule stated in the proof of Lemma 9.2 reproduces every entry. So the lemma is
true; only the remark is inaccurate.

*Correction:* either extend the script so that it reads the table from `app_seven.tex` and checks it
(as `logs/paper/referee/b_checks.py` does), or write
```latex
A short program (\fn{scripts/check_7cube_midplanes.py} in the project repository) enumerates the $193$
lines, confirms the statements (b) and (c) of \cref{lem:seven-decomp} and that exactly $45$ lines are
contained in $M$, and confirms by bipartite matching that these $45$ lines admit a pairing inside $M$.
It does not read \cref{tab:seven-pairing}; the table itself is verified by the proof of
\cref{lem:seven-pairing}.
```

### 3. (minor) Section 7.1: the remark on non-minimal representatives is not covered by Proposition 7.1

*Location:* `paper/sec_verification.tex:40-42` ("For soundness it is not even necessary that the
representative stored for a state is the minimal image; it is enough that it is the image of the state
under one element of G.").

*Problem:* The statement is true, but it is asserted without proof, and Proposition 7.1 does not prove
it: its proof shows Seen = V, where V consists of canonical forms (Def. 5.6), and its conclusion is "the
triple is a valid certificate". For a verifier that stores an arbitrary image, Seen is in general not V
(several images of one state may be stored, and look-ups in Tbl and Lv are made at states that need not be
canonical), so neither the proof nor the conclusion of Proposition 7.1 applies literally. In a section whose
purpose is to say exactly what an implementation may do, this should be either proved or removed.

*Correction (proof to add after Proposition 7.1, or delete the sentence):*
```latex
\begin{remark}
Suppose that in \cref{alg:verify} the function $\canon$ is replaced by any function $r$ that maps every
state to the image of this state under some element of $\G$ (which may depend on the state), and that the
algorithm returns \accept. Then the root is still \WON. Indeed, every state $s$ that was processed is
\WON, by induction on the rank: if $s\in\Lv$, then $\LEAF$ was required; otherwise, if $s$ is a P-state,
$t<\Rmax$ and the falsity of $\LOST$ were required, and for every child $s'$ of $s$ that is not \VOID{} the
state $r(s')$ was processed and has smaller rank, so that $r(s')$ and, by \cref{lem:game-invariance}\,(c),
$s'$ are \WON; if $s$ is a Q-state, the required cell $m=\Tbl(s)\notin B\cup F$ is a table move by the same
argument. The triple need not be a valid certificate in the sense of \cref{def:certificate}, whose
reachable set consists of canonical forms.
\end{remark}
```

### 4. (editorial) Lemma 9.1 and the number 193: what is proved for the 7x7x7 board and what is cited

*Location:* `paper/sec_discussion.tex:14-19` ("There are 193 lines [GolombHales, Theorem 1]"), `:59-70`
(Lemma 9.1(d) and the sentence after the proof).

*Problem:* For the board of side 7 the paper proves neither that a line has exactly two parametrizations
nor the number of lines; Lemma 2.1 cannot be quoted, because its proof uses that 5 is odd
("$t_1\ne 5-t_1$"), and on the board of side 7 the parameter 3 is fixed by $\rho_7$. Nothing in Corollary 9.3
needs these facts, but two sentences presuppose them: statement (d) speaks of "the 45 lines listed", i.e.
it presupposes that the 45 triples of the table are different lines, and "193 - 148 = 45 ..., in agreement
with (d)" uses the cited number. (Both are true: `b_geom.py`, `b_checks.py`.) The number 193 is thus a
third quotation from the literature, which Section 1.6 (`paper/sec_intro.tex:182-185`) does not list; this is
harmless only because the corollary does not use it.

*Correction:* make the count a consequence instead of a quotation, e.g. after the proof of Lemma 9.1:
```latex
The $45$ triples in (d) define $45$ different lines. Indeed, a line determines the coordinates in which
all its cells agree, that is, the constant components of $f$ and their values; and if the first active
component of $f$ is the identity, then $f(t)$ is the cell of the line whose first active coordinate is
$t$, so the line determines $f$. Together with (c) this shows that $\Qseven$ has $148+45=193$ lines; this
number is not used below.
```

### 5. (editorial) Notation clashes that can mislead an implementer

*Locations and corrections:*
* Algorithm 1, lines 14-15 (`paper/sec_verification.tex:69-71`) and the comment "The children"
  (`:100-103`): $a'$, $b'$ denote *cells* ("$a'\in a\setminus(F\cup\{c\})$"), whereas in Def. 5.2,
  Lemma 5.12, Remarks 5.13, 5.18, Prop. C.3 and Example C.5 $a'$, $b'$ are *lines*. Rename the two cells,
  e.g. $p\in a\setminus(F\cup\{c\})$, $q\in b\setminus(F\cup\{c\})$, $Q_a\gets a\setminus\{c,p\}$,
  $Q_b\gets b\setminus\{c,q\}$.
* Lemma 5.4, proof (`paper/sec_game.tex:243`): "a state has $t\le|\Q|+1$" — the board $\Q$ is printed as
  $Q$, next to Q-states $\mathsf{Q}(B,F,t)$ and the sets $Q_a$, $Q_b$. Write "$t\le 217$".

### 6. (editorial) Definition 5.8: the letter t after the switch to PAIRING mode

*Location:* `paper/sec_game.tex:349-351` and `:379-380`.

*Problem:* "Round $t+1$ (for $t=0,1,2,\dots$) runs as follows" uses $t$ both as the third component of the
record and as the number of completed rounds. They agree in TABLE mode (Theorem 5.14, (I2), (I3)), but the
comment after the definition says that in PAIRING mode the record is no longer changed, so from then on
"round $t+1$" does not refer to the record. Nothing depends on it, but (S1) ("$\canon\Pst(B,F,t)\in\Lv$")
and Theorem 5.15(c) ("from round $\Rmax+1$ on") are read with the wrong $t$ by a literal reader.

*Correction:* in Def. 5.8 write "Let $t$ denote the number of completed rounds; in \TABLE{} mode it is the
third component of the record. ... In \PAIRING{} mode the pair $(B,F)$ of the record is no longer changed."
In Theorem 5.15(c) write "in every round after round $\Rmax$ the reply of \Breaker{} is made in \PAIRING{}
mode" (at the beginning of round $\Rmax+1$ the mode may still be \TABLE{} until (S1) is carried out).

### 7. (editorial) Theorem 5.14: "repeated at most t+1-|F| times"

*Location:* `paper/sec_game.tex:603-606`.

*Problem:* The bound counts re-entries of rule (S6) (= reveals), with $F$ the revealed set at the
beginning of the round; the number of table look-ups is one larger. The proof says so, the statement does
not. An implementer who bounds the number of look-ups by $t+1-|F|$ would reject legal plays: already in
round 1 ($t=0$, $F=\emptyset$), if Maker takes the cell $(0,0,0)$ that the table names, rule (S6) is entered
twice while $t+1-|F|=1$ (my simulation reached 6 reveals, i.e. 7 look-ups, in one round).

*Correction:* "... and rule \ref{S:table} reveals at most $t+1-|F|$ stones, where $F$ is the revealed set at
the beginning of the round; so it is entered at most $t+2-|F|$ times."

### 8. (editorial) Proof of Theorem 1.1 should use the explicit certificate

*Location:* `paper/sec_game.tex:824-831`; cf. Fact 6.1 (`paper/sec_certificate.tex:19-26`).

*Problem:* The proof passes from "the root is WON" to "there is a valid certificate" by Lemma 5.7(c) and
takes sigma for *that* certificate, which is the non-explicit one built in the proof of Lemma 5.7(c)
(Lv = all canonical states with LEAF, Tbl chosen arbitrarily). Fact 6.1 states directly that the explicit
triple (Tbl, Lv, 10) of the two files is valid, and everything the paper says about "the" strategy
(abstract: within ten rounds, 93,164 entries; Section 1.4: switch after round 8, 9 or 10; Sections 6-8)
refers to that triple. The detour is logically correct but makes Theorem 1.1 speak about a different
strategy from the one described in the rest of the paper.

*Correction:*
```latex
\begin{proof}[Proof of \cref{thm:main}]
By \cref{fact:H} the triple $(\Tbl,\Lv,10)$ given by the two certificate files is a valid certificate.
Let $\sigma$ be the strategy of \cref{def:sigma} for this certificate, played by \Breaker{} as the second
player. By \cref{thm:sigma}, ... By \cref{cor:draw}, the ordinary game is a draw. (By
\cref{lem:certificate}\,(c) the same conclusion holds whenever the root is \WON{} for some $\Rmax$.)
\end{proof}
```

### 9. (editorial) Small points

* Remark 5.13 (`paper/sec_game.tex:584-585`): "require at least 12 stones by the proof of Lemma 5.12 and
  Prop. 5.9(b)" is correct; since Prop. C.3(b) is quoted two lines after Prop. 5.9, one could add "(in fact
  13, Prop. C.3)". No change needed for the argument.
* Prop. C.3(a) (`paper/app_extra_B.tex:105`): say "any four *distinct* lines"; the proof uses distinctness
  in its first sentence.
* Corollary 9.3, rule (i) (`paper/sec_discussion.tex:119-121`): add half a sentence why $\sigma$
  prescribes a cell at all: "(after the move of the virtual \Maker{} an odd number of cells of $A$, hence at
  most $215$, are occupied in the virtual position, so the virtual \Breaker{} is to move on a board that is
  not full)". The proof uses it silently in "By Theorem 1.1 the cell $w$ prescribed by $\sigma$ is empty in
  the virtual position".
* Section 5.6, last paragraph (`paper/sec_game.tex:441`): "of which at most two coincide" — more
  precisely, the three sets $S_l\setminus F$, $S_a\setminus F$, $S_b\setminus F$ overlap in at most two
  cells ($x$, and the cell of $S_l\cap S_b$); the inequality that follows is correct.

## Statements that are new relative to PROOF.md: result of the hostile reading

| statement | result |
|---|---|
| Lemma 5.5 (symmetry of the abstract game) | correct; tested numerically (`b_invariance.py`) |
| Def. 5.6 (R0)-(R2), (V1)-(V3) and Lemma 5.7 | correct; agrees with Def. 5.3 in both directions; (R1)/(R2)/(V2)/(V3) reproduced on the real files by `b_expand.py` |
| Remark 5.13 | correct; no counterexample in 46,562 (random positions) + 68,920 (plays of sigma) tested moves; bound 11 attained |
| Prop. C.3 incl. "four pairwise meeting lines are concurrent" | correct; brute force: 984 of 984 |
| Example C.5 | correct (checked literally) |
| Prop. 5.16, both parities | correct; statement and proof bookkeeping tested |
| Theorem 5.15(c) | correct (wording: finding 6) |
| Lemma 9.1, Lemma 9.2 / Table 13, Cor. 9.3, (J1), (J2) | correct; table parsed and checked; strategy simulated (findings 1, 2, 4 concern remarks only) |
| Prop. 9.5 | correct; the true maximum for 7 busy cells is 44 lines |
| Prop. 7.1 / Algorithm 1 | correct; literal transcription reproduces all counts of Appendix A.5 (finding 3 concerns a side remark) |

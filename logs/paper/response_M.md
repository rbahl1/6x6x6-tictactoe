# Response of reviser M (mathematical sections) to the referee reports

Files owned in this phase: `paper/sec_board.tex`, `sec_pairing.tex`, `sec_leaf.tex`, `sec_game.tex`,
`app_extra_A.tex`, `app_extra_B.tex`, `app_seven.tex`, Section 9.1 and Proposition 9.5 of
`sec_discussion.tex`; appended lines in `paper/macros.tex`; one word in the wrapper `app_extra.tex`;
`scripts/check_7cube_midplanes.py`. Corrected LaTeX for the findings located in files of reviser C is in
`logs/paper/revision_M_for_C.tex` (blocks (C1)-(C21)); reviser C has integrated (C1)-(C7), (C12), (C13).

Line numbers are those of the files at the end of this phase. Statement numbers are those of the refereed
PDF (73 pp.); **the numbering inside Section 5 has shifted** because of a new lemma, a new example and a
new figure (old Def. 5.2 = new 5.4, old Lemma 5.7 = new 5.9, old Def. 5.8 = new 5.10, old Prop. 5.9 = new
5.11, old Lemmas 5.10-5.12 = new 5.12-5.14, old Remark 5.13 = new 5.15, old Thm 5.14 = new 5.16, old Thm
5.15 = new 5.17, ...). Section 4 and Section 9 keep their numbers.

State of the build: every owned file compiles with `paper/build_one.sh`; `latexmk -pdf paper.tex` runs
without error, undefined reference or overfull box (86 pages at the time of writing, including the
concurrent changes of reviser C).

All checks by program mentioned below were made with code of my own (own geometry, group and canonical
form: `logs/paper/revM_checks/geom.py`); scripts and output are in `logs/paper/revM_checks/` (`run.log`).

---

## 1. referee_math_a.md

| # | finding | what was done | where |
|---|---|---|---|
| A1 | MAJOR: the leaf test of the main verifier deletes strong lines; the argument is not in the paper | New subsubsection "Deleting strong lines" with Lemma (Deleted lines) and proof, written against `verify/treecheck.cpp` (comment 628-652, `validateFlow` 782-812, deletion 954-987). Part (a) is the soundness of true verdicts, part (b) the exactness; the induction on the order of deletion and the properties to validate are stated; the treatment of theta = 1 is described. Also: new text of Remark 7.3 (covers the pair rule, points to the lemma), Table 7 first row, "Cost", trust sentences of 7.7 and 1.6 | `revision_M_for_C.tex` (C1)-(C4), (C9); integrated by C in `sec_verification.tex:300-349` (`lem:deleted`), `:276-290` |
| A2 | missing step in the proof of Prop. 7.2 (lines 3-5 are not the max-flow algorithm of Lemma 3.3) | replacement text with the direct Hall-violator argument (reachable set R, at most 2 cells per line, l holds at most one), no appeal to max-flow min-cut | `revision_M_for_C.tex` (C5) |
| A3 | Figure 2 contradicts the proof (s = 5 - r) | figure redrawn with r = 2, s = 0 as proposed; caption names the three meeting cells and says that the same lines form a triangle with the other diagonal; checked by program for both diagonals | `sec_leaf.tex:657-666` |
| A4 | "LEAF is strictly weaker than C when F is non-empty" (Section 7.6) | replacement sentence ("strictly stronger condition ... not only in the situation of Example 4.11") | `revision_M_for_C.tex` (C8) |
| A5 | missing hypothesis "F admissible" in two summaries | Remark 4.10 (iv): "agree for B-admissible F and theta = 0", with the reason in parentheses. The second place (Section 9.2, "LEAF versus C") is in the part of `sec_discussion.tex` owned by C | `sec_leaf.tex:233-236`; `revision_M_for_C.tex` (C11) |
| A6 | Corollary 4.21 applied outside its range (k >= 11) | "For k <= 10 it follows from Corollary 4.21 ..., and for k >= 11 it then follows from Proposition 4.18 (b)" | `sec_leaf.tex:703-706` |
| A7 | Prop. 4.22 far from sharp (optional) | adopted: the proposition is now stated for |B| <= 17 with the referee's argument (the 36 face diagonals are the pairs inside six classes of four busy cells; a set meeting all six pairs of a class has three cells of it), written without the symbol K_4; a sentence after the proof says that 17 cannot be improved in this step. Checked by program: the graph (busy cells, face diagonals) consists of six complete graphs on four vertices, minimum cover 18, 4,096 minimum covers; the triangle of the proof for all 36 diagonals and all admissible (r,s). Label `prop:busy10` unchanged | `sec_leaf.tex:605-626`, `:671-673` |
| A8 | order of the counts in Lemma 2.3 | "24 cells with one level, 144 cells with exactly two and 48 cells with three distinct levels" | `sec_board.tex:156-157` |
| A9 | Remark 4.5 (ii): F' must be disjoint from B | added | `sec_leaf.tex:108` |

## 2. referee_math_b.md

| # | finding | what was done | where |
|---|---|---|---|
| B1 | Remark 9.4 (iii) claims "draw implies draw" | rewritten: the two hypotheses of Golomb-Hales, their Theorem 3 (credit, with the mid-plane decomposition), what the proof of Cor. 9.3 is, and exactly what it establishes ("if Breaker wins on 6^3 as second player, then Breaker wins on 7^3"; not "draw implies draw"; nothing for 5^3). Facts taken from `logs/paper/related_work.md` items c.6, c.7 (verified from the source there) | `sec_discussion.tex:201-212` |
| B2 | Remark 9.4 (ii): the program does not check the printed table nor (d) | both alternatives of the referee: (1) `scripts/check_7cube_midplanes.py` now parses `paper/app_seven.tex` and checks the table as printed: 45 entries, lines pairwise different, exactly the lines contained in M, both cells on their line, 90 cells pairwise different, 37 cells of M unassigned (centre among them), and the rule of the proof of Lemma 9.2 reproduces every entry; it also checks "two parametrizations per line", Lemma 9.1 (b) (the cell in M is f(3)) and (c). Three seeded mutations of the table are rejected. Output regenerated: `logs/paper/check_7cube_midplanes.out` (exit 0). (2) the remark now says exactly this, and that the matching part shows existence only | `scripts/check_7cube_midplanes.py`; `sec_discussion.tex:189-199`; `app_seven.tex:11` |
| B3 | Section 7.1: unproved remark on non-minimal representatives | remark with proof (induction on the rank; the root is fixed by G) and replacement of the two sentences of 7.1 by a pointer | `revision_M_for_C.tex` (C6); integrated by C (`rem:any-image`) |
| B4 | 7x7x7: two parametrizations per line and the number 193 | proved instead of quoted: "normalized" parametrization (first active component the identity), which a line determines; hence two parametrizations and (9^3-7^3)/2 = 193 lines, "in accordance with" Golomb-Hales Theorem 1, "not used below". Lemma 9.1 (d) now says that the 45 entries are 45 different lines, and the proof says why (normalized parametrizations); the sentence after the proof derives 148 + 45 = 193 | `sec_discussion.tex:19-25`, `:45-46`, `:72-77` |
| B5 | notation clashes: a', b' as cells in Algorithm 1; the board symbol in the proof of Lemma 5.4 | Algorithm 1: cells renamed p, q (text for C). Lemma 5.4: "t <= 217, because B is a set of t or t-1 cells". In addition the board is no longer written Q (see N4 below) | `revision_M_for_C.tex` (C7); `sec_game.tex:418` |
| B6 | Def. 5.8: the letter t after the switch; Thm 5.15 (c) | Def. 5.8: "let t denote the number of completed rounds; in TABLE mode this is the third component of the record"; comment: in PAIRING mode the record is no longer used. Thm 5.15 (c): "in every round after round R_max the reply of Breaker is made in PAIRING mode"; proof completed accordingly | `sec_game.tex:530-531`, `:559-561`, `:909-910`, `:918-921` |
| B7 | Thm 5.14: "repeated at most t+1-|F| times" | "reveals at most t+1-|F| stones, F being the revealed set at the beginning of the round; so this rule is entered at most t+2-|F| times"; same in the proof | `sec_game.tex:796-799`, `:881` |
| B8 | proof of Theorem 1.1 should use the explicit certificate | proof now starts from Computed Fact 6.1 (the triple of the two files is valid), uses sigma for this certificate, and a paragraph after the proof keeps the general implication (Lemma 5.7 (b), (c)) | `sec_game.tex:1020-1032` |
| B9 | small points | Remark 5.13: "(in fact 13 on this board; Prop. C.3)"; Prop. C.3 (a): "four distinct lines" in statement and proof; Cor. 9.3 rule (i): parenthesis on the odd number of occupied cells; Section 5.6: "they overlap in at most two cells (the cell x, and the cell of S_l and S_b if there is one)" | `sec_game.tex:770`; `app_extra_B.tex:105`, `:113`; `sec_discussion.tex:127-129`; `sec_game.tex:623-624` |

## 3. referee_journal.md (Sections 2-5, Appendix C, and the items assigned to M)

| # | item | what was done | where |
|---|---|---|---|
| F1 / R5 | one formulation of the computed statement | see B8 | `sec_game.tex:1020-1032` |
| E5 | Section 4.3 justifies only Algorithm 2 | first sentence of 4.3 says so and that it can be skipped; the opening of Section 4 says which parts Theorem 1.1 uses. Moving 4.3 and deleting the five-step list are moves between files (D3): not done, see "Not applied" | `sec_leaf.tex:16-20`, `:262-264` |
| E7 | roadmap for Section 5 | added, with the note for R_max <= 10 (LOST false, at most one forced event; Definition of the patterns may be skipped). To make the "may skip" route self-contained, Remark 5.13 now ends with the direct reason why the forced reply restores admissibility when there are at most ten stones | `sec_game.tex:16-29`, `:775-779` |
| E8 | worked example at the end of 5.2 | Example "The first rounds on the real board": root and its only child; table move (0,0,0), children (i) and (ii), table move (1,1,1); quiet rounds to B_4; an occupied cell ((5,5,5) found occupied and revealed, then (1,1,4)); a threat child of P(B_4,-,4) (line {(0,i,1)}, e = (0,0,1); 122 live lines, 732 children (c), none void); a danger child of P(B_7,-,7) (a = {(i,2,0)}, b = {(2,j,0)}, c = (2,2,0); 104 live lines, 624 children (c), 16,250 children (d) = 25 x 650, none void). Every cell, every table entry (read from `results/final/strategy.tbl` through my own canonical form), every condition of Def. 5.1 and every count was checked by `logs/paper/revM_checks/sec5_example.py`; the play agrees with Table 6 and with the first play of Section 6.6 | `sec_game.tex:223-281` |
| E9 (i) | meaning of the children explained 15 lines late | the paragraph "The meaning is as follows" now follows Definition 5.1 directly | `sec_game.tex:118-127` |
| E9 (ii) | patterns hard to read | "may skip" note (E7) and a new figure of an E2 pattern with 11 stones and an E3 pattern with 13 stones (those of Prop. C.3); both checked literally against Def. 5.2 by `pattern_figure.py`, including the statements of the caption | `sec_game.tex:330-377` |
| E9 (iii) | Remark 5.13 as a list | done | `sec_game.tex:755-772` |
| E9 (iv), typo table | "would be void" | "would be vacuous" | `sec_game.tex:1091` |
| E1 (idea of LEAF) | assigned to M for Section 4 | the idea is explained in one paragraph before Definitions 4.6/4.7 (connected violator; one or two lines cannot violate; surplus at least theta) | `sec_leaf.tex:130-139` |
| N1 | "certificate" for one thing only | In my files "certificate" now means the triple only. C, C_Phi, LEAF are "the pairing condition", "the surplus condition", "the leaf condition" (title of Section 4, Definitions 4.4 and 4.7, Remark 4.10, 4.4, 4.5, Table 2, Sections 3, 5, C.1, wrapper of Appendix C); "witness" in Section 3.4; the opening of Section 4 states the convention. Wording for C's files in `revision_M_for_C.tex` (header, (C2), (C15)) | `sec_leaf.tex:1`, `:11-20`, `:90-97`, `:159`, `:225`, `:233`, `:502`; `sec_pairing.tex:226`; `sec_game.tex:7`, `:397`; `app_extra_A.tex:5-7`, `:172-174`; `app_extra.tex:4`; `sec_board.tex:30` |
| N2 | K (subgroup) against calligraphic K (busy cells) | the subgroup is now G_0 | `sec_board.tex:244-245`, `:286-298` |
| N3 | sigma used for a surplus value and for elements of G_Sigma | `sec_leaf`: sur_F(S) = j; `sec_board`: elements u, u' of G_Sigma and v, v' of G_0 | `sec_leaf.tex:152`; `sec_board.tex:288-289` |
| N4 | the letter Q | the board is now written Omega (one line appended to `macros.tex`: `\renewcommand{\Q}{\Omega}`; all files use the macro, so the change is global and consistent; the 7x7x7 board is Omega_7). Lemma 5.4 no longer mentions the board. Q_a, Q_b are kept: see "Not applied" | `macros.tex:65-68`; `sec_game.tex:418` |
| N5 | Phi used for the map A -> board | the map is now iota | `sec_discussion.tex:34` and 6 further places in 9.1 |
| N6 | labels (R1)-(R3) in Appendix C.1 | renamed (C1)-(C3) | `app_extra_A.tex:85-136` |
| N7 | "line" for file lines and lines of play | not in my files, except that the new Example 5.3 refers to Section 6.6 by `\cref{ssec:walk}` and not by "Line A" | - |
| N8 (optional) | l / ell; "id"; H | line variables are l everywhere in my files (Section 2.1, Section 9.1; the symbol ell(f) for the line of a parametrization is kept); "id" of a cell is introduced in Section 2.5 with the remark that the identity is written id (roman); the set H of the eager variant is now Z | `sec_board.tex:86-98`, `:332-333`; `sec_discussion.tex` (28 places in 9.1); `sec_game.tex:1074-1100`; `app_extra_B.tex:76` |
| T1 | Figure 2 never cited | cited at the end of the proof of Prop. 4.22 | `sec_leaf.tex:643` |
| T2 / E6 | Table 2 never cited, stops at Section 4 | extended by theta, P- and Q-states, Q_a/Q_b, VOID, E2/E3/LOST, WON, R_max, rk, (Tbl, Lv, R_max), V, sigma; moved to the beginning of Section 2 (top of a page); cited from Section 2, from the roadmap of Section 5 and (by C) from Section 1.8 | `sec_board.tex:9-49`; `sec_game.tex:21-22`; `revision_M_for_C.tex` (C12) |
| typo table | Lemma 2.3 order | see A8 | `sec_board.tex:156` |
| typo table | sentences beginning with "And"/"But" | all seven in my files joined or reworded | `sec_pairing.tex:59-61`; `sec_leaf.tex:436`, `:707`; `sec_game.tex:896`; `app_extra_A.tex:34`, `:135`; `app_extra_B.tex:215` |
| pronouns | "he/his/himself" | removed everywhere in my files ("Breaker"/"Maker" repeated, or "this player") | `sec_pairing.tex:3-5`, `:15`, `:59`, `:161`, `:231`, `:243`; `sec_leaf.tex:3-4`, `:42`; `sec_discussion.tex:130-133`, `:174-179` |
| style | "upwards" | "upward" in both captions | `sec_board.tex:191`; `sec_leaf.tex:664` |
| P2 / R3 | credit to Golomb-Hales for 6 -> 7 | see B1 | `sec_discussion.tex:201-212` |
| P3 | "in a preprint" | done in 9.1 | `sec_discussion.tex:8` |
| P9 | Zermelo and backward induction | "the determinacy of such games, which is usually attributed to Zermelo and is proved by backward induction on the game tree" | `sec_game.tex:1013-1016`; `sec_discussion.tex:176-178` |
| P10 | Dvorak-Valla against Computed Fact 2.9 | the paper now says that the order 1,152 was not compared with the general description of that paper (the literature report did not extract the theorem; I did not either) | `sec_board.tex:413-415` |
| R2 / D4 | "project repository" | "archive" in all six places of my files | `sec_board.tex:404`; `sec_pairing.tex:220`; `sec_leaf.tex:696`; `app_extra_A.tex:178`; `app_extra_B.tex:181`; `sec_discussion.tex:191` |
| file names broken across lines | `logs/core/test_core.log`, `scripts/check_7cube_midplanes.py` | kept on one line (`\mbox`); sentence reworded so that no overfull box results | `sec_leaf.tex:697`; `sec_discussion.tex:190-191` |

## 4. referee_replication.md

| # | finding | what was done | where |
|---|---|---|---|
| 3 | Def. 5.2: may x belong to F? | one sentence after the definition: the definition does not require x not in F; in Lemmas 5.11/5.12 x is unrevealed; admitting x in F only makes LOST true more often; an implementation should evaluate the definition as it stands (this is what `verify/spotcheck.py` does, lines 418-600) | `sec_game.tex:324-328` |
| 6 | Push drops void children (a), (i) silently | sentence after Definition 5.6: a child (a) or (i) of a non-void state is never void, no state of V is void, so discarding and reporting an error are the same | `sec_game.tex:481-486` |
| 7 | counting conventions; "Different data may define the same child" | that sentence was wrong for the states that matter. New Lemma "Different data define different children" (for a P-state that is not VOID: different (l,e), different ({a,b},Q_a,Q_b), and no child (c) is a child (d)), with proof; a sentence on VOID parents (where it can fail) and on canonical forms. Tested on 157 expanded states of the certificate, on P(B_7,-,7) (16,874 children) and on 250 random non-void states: no duplicate; a void state with a duplicate exhibited. Wording for the caption of Table 3 and for A.5 sent to C | `sec_game.tex:176-221`; `revision_M_for_C.tex` (C13) |
| 8 | K clash | see N2 | `sec_board.tex:286-298` |
| 9 | Lemma 2.3 order | see A8 | `sec_board.tex:156` |

## 5. referee_audit.md (items located in my files)

| # | finding | what was done | where |
|---|---|---|---|
| F2 | "project repository" | see R2/D4 above; the list of files cited in my sections that must be in the archive is in `revision_M_for_C.tex` (C20) | six places |
| F7 | Computed Fact 4.23: "the same values are recorded in ... ip_regression.json" | re-read from the raw files: `logs/core/test_core.log` lines 4, 13, 14 contain all four values; `results/regression/ip_regression.json` records (T8,-,8) true and (T8,-,9) false (records idx 4 and 6) and, for T7, only k = 7. New text: "The same four values are recorded in logs/core/test_core.log; the regression file ... records the two values for T8." | `sec_leaf.tex:696-698` |
| F10 | "first proved" without "preprint" in 9.1 | see P3 | `sec_discussion.tex:8` |
| F16 | credit of the 6 -> 7 step | see B1 | `sec_discussion.tex:201-212` |

No number was changed in my files except by these re-readings and by my own computations for the new
example (122, 732, 104, 624, 16,250, 650), which are in `logs/paper/revM_checks/run.log`.

## 6. Not applied, or applied differently, with reasons

1. **N4, "at least rename Q_a, Q_b".** Not renamed. They occur in Algorithm 1 and its comments
   (`sec_verification.tex`, owned by C), in the three verifiers written from the paper during refereeing and
   in their reports; a rename in my files alone would have made Definition 5.1 and Algorithm 1 disagree. The
   clash named by the coordinator (board symbol against Q-states and Q_a) is removed by renaming the board to
   Omega. Remaining use of the letter: Q-states (sans serif, always with arguments), the sets Q_a, Q_b
   (listed in Table 2), and Q as a name for a Q-state in Definition 5.6 / Lemma 5.7.
2. **N4, the letter P** (P-states, the maps P_r, the conditions (P1), (P2)): unchanged; the three are set in
   different fonts and never meet in one statement.
3. **E5 (move Section 4.3 to 7.3; delete the five-step list at the end of 4.3), E10 (move the end of Section
   5.10 to the appendix), table of Section 6 of the journal report:** moves between files; by decision D3
   they are made by reviser C in the second phase. I added the reading guides instead (see E5, E7).
4. **Replication finding 3, second half (test vectors for LOST with a positive answer):** belongs to
   Appendix A.6 (file of C); not produced by me. Recorded in `revision_M_for_C.tex` (C21).
5. **Optional suggestion "present the case R_max <= 10 directly and move LOST to an appendix":** not done (a
   rewrite of Section 5 and of the verifiers' description; the note of E7 and the new end of Remark 5.13 give
   the reader the short route).
6. **Optional remark "Breaker also wins when moving first":** not added; it is true by the usual
   extra-stone argument, but that argument is not written out in the paper for the 6x6x6 board, and I did not
   want to add an unproved statement next to the main theorem.
7. **A5, second location, and one remaining pronoun** ("from round 8 on he can have a danger pair") are in
   the part of `sec_discussion.tex` owned by C: (C11), (C21).

## 7. Doubts

* The board symbol Omega is a visible global change made through `macros.tex`. If the coordinator prefers
  another symbol, the last line of `macros.tex` is the only place to change.
* Proposition 4.22 is now stronger than before (17 instead of 11). The proof is the referee's and was
  re-derived and tested by me; Section 6.4 uses it only for ten cells.
* `paper/paper.txt` was not regenerated by me.

## 8. For reviser C

See `logs/paper/revision_M_for_C.tex`: (C1)-(C9) corrected LaTeX for `sec_verification.tex` and the trust
sentences; (C11) Section 9.2; (C12) cite Table 2; (C13) counting convention; (C14) board symbol; (C15)
terminology; (C16) Zermelo; (C17) new labels `lem:children-distinct`, `ex:first-rounds`, `fig:patterns` and
the shift of numbers in Section 5; (C18) "strong" now also defined in 4.3; (C19) Prop. 4.22; (C20) files for
the archive; (C21) open items.

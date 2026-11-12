# Referee report (journal / publication aspects)

Manuscript: "The 6x6x6 tic-tac-toe game is a draw" (`paper/paper.pdf`, 73 pages, built 2026-11-12 16:33 UTC).
Assignment: the paper as a publication (claim and framing, positioning, exposition, presentation,
reproducibility statement and declarations). Proofs line by line, numbers and replication are covered by
other referees; where I touch numbers it is only for internal consistency of the presentation.

What I did: read the whole text (`paper/paper.txt`, all 3,796 lines) and the LaTeX sources; rendered all 73
pages (`pdftoppm -r 60`, some at 110 dpi) and inspected the title page, all thirteen tables, the three
figures, both algorithms and the appendices; compared every literature statement with
`logs/paper/related_work.md`; read `logs/paper/issues.md`, `paper/OUTLINE.md`, the head of `PROGRESS.md`,
`REVIEW.md`, `README.md`; listed the contents of `dist/ttt6-artifact.tar.gz`; scanned `paper.log` (no
overfull/underfull box, no undefined reference, no BibTeX warning) and the fonts (Latin Modern and AMS
symbol fonts, all embedded).
Nothing under `paper/` was edited.

## Recommendation

**Major revision** (acceptable in principle; no new mathematics and no new computation is requested).

The paper is unusually careful about what it claims. Theorem 1.1 and Corollary 1.2 are stated precisely, the
single computed ingredient is isolated and named, the search is explicitly untrusted, and Section 7 tells a
skeptical reader what a verifier must not get wrong. The writing is plain and free of sales language. I have
three reasons for not recommending "minor revision":

1. **Disclosure and the meaning of "independent" (R1).** The trust argument of Section 7 rests on redundancy
   between "independently written" programs, an "internal review" and "audits". The single declaration at
   the end of Section 9 does not tell the reader what these words mean here, and it says less than the
   project's own records, which the archive ships (`PROGRESS.md`, `REPORT.md`, `REVIEW.md`).
2. **Availability of the evidence (R2).** Material that the paper cites as evidence is not in the archive
   but "in the project repository", an object the paper never defines and gives no address for: two of the
   four verifiers of Computed Fact 6.1, the programs behind Computed Facts 2.9 and C.4, and the logs and
   the confirming script cited for Computed Facts 3.10 and 4.23. Licence and persistent identifier are not
   mentioned at all.
3. **Length and structure (R10).** 73 pages, of which 18 (Sections 6 to 8) read as a laboratory report and
   13 are appendices, 6 of them (Appendix C) "not used". For JCTA or Combinatorica this is too long; for EJC
   or Experimental Mathematics it is acceptable only with a clear split into article and supplement. About
   17 to 20 pages can move without losing a proof needed for the theorem or anything needed to replicate.

All three are repairable by editing. The remaining points are minor or editorial.

---

## 1. The claim and its framing

**Statement of the results.** Theorem 1.1 (p. 2–3) is precise: it says what "winning strategy" means (an
empty cell is always prescribed; Maker never owns six cells of a line) and what "draw" means (each player
has a strategy with which this player never loses). Corollary 1.2 is abbreviated but points to the full
statement, Corollary 9.3, which correctly adds "and the board is not full" for the 343-cell board. The tag
"(computer-assisted)" on the theorem and "(computer-assisted, via Theorem 1.1)" on the corollary is honest
and I would keep it.

**Where "computer-assisted" is explained.** In the right places: directly after the theorem (Section 1.3),
in Section 1.6 (proved / computed / trusted), at the head of Section 5, after the proof in Section 5.9, in
Section 6.1, in Section 7.7 and in Section 9.3. The dependence on Computed Fact 6.1 is stated consistently
in all of them. Three small inconsistencies remain:

* (F1) *Two formulations of the computed statement.* Section 1.3 and the proof in Section 5.9 use "the root
  is WON for R_max = 10" and reach a certificate through Lemma 5.7(c), which produces an *abstract*
  certificate (all LEAF states as leaves, table moves chosen arbitrarily). Section 1.6, Computed Fact 6.1
  ("More precisely ...") and all of Section 7 use "the two files form a valid certificate". The two are
  equivalent by Lemma 5.7, but the detour hides that the strategy of the theorem is the strategy of the
  distributed files, and it adds a step (5.7(c)) the theorem does not need. Fix: make "the two files form a
  valid certificate (Tbl, Lv, 10)" the computed statement everywhere and let "the root is WON" be its
  corollary by Lemma 5.7(b). Concretely, `sec_game.tex:825-827` (p. 36): replace the first two sentences
  of the proof by "By Computed Fact 6.1 the triple (Tbl, Lv, 10) given by the two certificate files is a
  valid certificate. Let sigma be the strategy of Definition 5.8 for this certificate, played by Breaker as
  the second player."; `sec_intro.tex:86-93` (p. 3): "... and prove by hand: *if a valid certificate exists,
  then both assertions of Theorem 1.1 hold* (Theorems 5.14 and 5.15, Corollary 5.17). That the two
  certificate files form a valid certificate with R_max = 10 is Computed Fact 6.1."
* (F2) Section 1.6 (`sec_intro.tex:187-188`): "Statements established by running programs are called
  Computed Facts". Not all are: the leaf table (Table 5), the two observations of Section 6.5, the orbit
  counts of A.7, the minimum numbers of stones of A.8 (quoted in Section 9.3 as "computed") and everything in
  Sections 7.5–7.6 are computed statements in running text. Replacement: "Mathematical statements that we
  establish by running programs and state formally are called Computed Facts and name the programs that
  established them. Sections 6 to 8 and Appendices A and B also report computational results in running text
  and in tables, each with its source; none of these is used in the proof."
* (F3) Section 6.1 (`sec_certificate.tex:44-45`, p. 38): "the only statement on which Theorem 1.1 depends
  that is not proved in this paper" contradicts Section 1.6, which lists two theorems quoted from the
  literature. Add "apart from the two standard theorems quoted in Section 1.6".

**Is anything oversold?** Very little.

* (F4) Section 6.3, last bullet (`sec_certificate.tex:190-191`, p. 41): "Section 6.5 explains why non-busy
  table moves cannot be avoided." Section 6.5 itself says the statement comes from one unverified program
  and covers R_max <= 10 only (for R_max = 11 the run was not exhaustive). Replacement: "Section 6.5 reports
  a computation which indicates that non-busy table moves cannot be avoided for R_max <= 10."
* (F5) Section 6.5 (`sec_certificate.tex:273`, p. 42): the run-in heading "Busy cells alone do not suffice
  for R_max <= 10" states the observation as a fact. Replacement: "Busy cells alone seem not to suffice for
  R_max <= 10 (single program)". The caveat should also name what the exhaustiveness rests on besides the
  program: the unproved claim "a state that is WON is also WON in the relaxed game" (`:93`) and the pruning
  bound 2d - 2 of the relaxed game (`:99`), neither of which is proved in the paper.
* (F6) Section 6.2 (`sec_certificate.tex:121-123`, p. 40) and Section 8.4 (`sec_repro.tex:169`, p. 54): "The
  output is a deterministic function of the source code ... and of the options" / "The search is
  deterministic". For a multi-threaded search with a shared lock-free transposition table this is a design
  claim that the paper does not justify; what was observed is byte identity in two runs. Either give the
  one-sentence reason (which thread finishes first cannot change which move is recorded), or write "in all
  our runs (with two and with four threads) the two files were identical byte for byte".
* The novelty claim ("to our knowledge ... we found no earlier proof or claim") is worded as the literature
  report allows. The sentence "5x5x5 is now the only n x n x n game whose outcome is unknown" is qualified
  by "to our knowledge" in the abstract and in Section 1.3. Fine.

**Trusted base.** Stated plainly, three times (1.6, 7.7, 9.3; the third could be shortened to a pointer).
The list of eight "dangerous defects" in Section 7.1 is exactly what a skeptical reader needs: the theorem
fails only if a written proof in Sections 2–5 is wrong, or Algorithms 1–2 do not check Definition 5.6, or
every implementation has one of those eight defects. I suggest one sentence to this effect in Section 1.6,
with a pointer to 7.1. What is missing from the trusted base is the subject of R1.

## 2. Positioning

I compared every literature statement in Sections 1, 2.6, 3, 5.9 and 9 with `related_work.md`. **No
statement tagged UNVERIFIED there is asserted in the paper.** Strategy stealing is not attributed to
Hales–Jewett; "Open Problem 3.2" of Beck, Mahalko's thesis and the Hex results are not used; the
Hales–Jewett bounds are given with the two secondary sources that were actually read ("see [7, p. 300] and
[15, ...]"); the 7x7x7 result is called a preprint in Section 1.2 and in Table 1. Points that remain:

* (P1) **Credit to [5] for the plan on 6x6x6** (`sec_intro.tex:69-72`, p. 2). The paper quotes only the last
  sentence of [5, Section 6.2]. According to the literature report (verified from the source), the same
  passage outlines the plan this paper carries out: Breaker occupies cells on the four space diagonals in
  his first moves, and "It is possible that Breaker could, within the next few rounds of play, eliminate
  enough additional winning lines to establish a viable pairing strategy, while making sure to block any
  potential threats from Maker." The certified strategy opens exactly so (Section 6.6, Line A). Replacement
  for the last two sentences of the paragraph: "They list n = 5 and n = 6 as the two remaining open cases.
  For n = 6 they sketch a plan [5, Section 6.2]: Breaker first occupies cells on the four space diagonals;
  then "it is possible that Breaker could, within the next few rounds of play, eliminate enough additional
  winning lines to establish a viable pairing strategy, while making sure to block any potential threats
  from Maker", and "with sufficient care beyond the scope of what we are willing to attempt, it may be
  possible to resolve the 6^3 game computationally". The present paper carries out this plan."
* (P2) **Credit to Golomb–Hales for the step 6 -> 7** (Remark 9.4(iii), `sec_discussion.tex:186-190`, p. 58).
  "The proof above establishes this implication for d = 3 and n = 6" undersells [9]: by the literature
  report (verified from the source), [9, Theorem 3] proves that for even n this hypothesis follows from a
  second one (a draw on n^k implies a draw on n^(k-1)), by precisely the mid-plane decomposition, with the
  second player replying in the sub-board in which the first player has moved. Replacement for the second
  sentence: "For even n, Golomb and Hales show that it follows from a second hypothesis, that a draw on n^d
  implies a draw on n^(d-1) [9, Theorem 3]; their proof uses the mid-plane decomposition. The proof of
  Corollary 9.3 is this argument, carried out for the Maker–Breaker game with n = 6 and d = 3, where the
  boards 6^2 and 6^1 are handled by the explicit pairing of Table 13 and Breaker's free moves are treated
  explicitly."
* (P3) Section 9.1, first paragraph (`sec_discussion.tex:7-8`, p. 55): "was first proved by" -> "was first
  proved, in a preprint, by", as in Section 1.2.
* (P4) *Winning Ways* [4] is cited twice as a source for the pairing strategy for n >= 8 (Table 1,
  `sec_intro.tex:46`, and `:56-57`). The literature report did not read it; its role rests on the sentence
  of [9] "Some of this material was included in [4]". Until someone has opened the book: Table 1 "[9]";
  text "... which goes back to Golomb [9, p. 181]; see also [4, Chapter 22]".
* (P5) Hales–Jewett [10] and Patashnik [16] were not read (HTTP errors). Citing a theorem through secondary
  sources is acceptable in a draft, not in a journal: obtain both before submission and cite theorem
  numbers.
* (P6) **Missing context** (one paragraph, end of Section 1.2 or in 9.2). A referee from the games community
  will ask how the method relates to earlier computer-assisted results on positional games. All entries are
  already in `refs.bib` with verified records: the survey of solved games (van den Herik, Uiterwijk, van
  Rijswijck 2002); Go-Moku and threat-space / proof-number search (Allis et al. 1994, 1996), because the
  paper's threats, forced replies and AND–OR search are the same vocabulary; and draw proofs by
  matching-type arguments (Uiterwijk's "set matching", Hsu et al. 2020 for the (7,7,5)- and (8,8,5)-games),
  which the literature report itself calls "conceptually close to our leaf certificates" and deserving "one
  sentence". The text must stay within what was verified (titles and records; Uiterwijk's arXiv version was
  read).
* (P7) Optional references, bibliographic data to be checked by the authors: the certificate-plus-independent
  -checker architecture is that of "certifying algorithms" (McConnell, Mehlhorn, Näher, Schweitzer, Computer
  Science Review 2011) and of proof-producing SAT solving (Heule, Kullmann, Marek, 2016); lazy revelation
  resembles relevance-zone and null-move proof search in k-in-a-row solvers (Wu and Lin, Connect6). One
  sentence each would help readers place the method.
* (P8) Krivelevich [15] (`refs.bib:208`) is cited as an arXiv preprint although theorem numbers are quoted
  from it four times. Cite the published version (Proceedings of the ICM, Seoul 2014; volume and pages to
  be taken from the proceedings) together with the arXiv number, and check that the numbering agrees.
* (P9) "by Zermelo's theorem [19] (backward induction on the game tree)" (`sec_game.tex:818`, p. 36; also
  `sec_discussion.tex:167`). Zermelo's 1913 paper does not argue by backward induction; this is a known
  misattribution. Replacement: "by the determinacy of finite games with perfect information, usually
  attributed to Zermelo [19] and proved by backward induction on the game tree".
* (P10) Section 2.6: the paper cites Dvořák–Valla [6] for the automorphism groups of all boards n^d and then
  proves |Aut| = 1,152 by a home-made program (Computed Fact 2.9). Either check that the theorem of [6]
  gives 1,152 for n = 6, d = 3 and cite it (then the Computed Fact is unnecessary), or say that the
  comparison was not made.

The standard tools are cited adequately: Hall [11], Ford–Fulkerson [8], Schrijver [17], Hopcroft–Karp [13],
HiGHS [14]. The comparison with [5] in Section 1.5 is fair and specific.

## 3. Exposition

**The introduction.** Section 1.4 does its job in one page: no pairing on the empty board; threats and
danger pairs; admissibility; the leaf certificate; lazy revelation; the certificate. A combinatorialist can
follow it. Four improvements:

* (E1) The one idea that makes LEAF work is not in the introduction. Add after `sec_intro.tex:130`: "The
  reason is short: a set of lines violating Hall's condition can be taken connected; one or two lines
  cannot violate it when Maker's stones are admissible; and a connected set of at least three lines whose
  surplus is at least the number of unrevealed stones cannot be spoiled by them."
* (E2) "round" is used from the abstract on and never defined. Add in Section 1.3: "A round consists of a
  move of Maker followed by a move of Breaker."
* (E3) `sec_intro.tex:121-123`: "two such lines with four stones of Maker each" reads at first as "two lines
  with five stones". Write "two lines without a stone of Breaker, each with four stones of Maker, that meet
  in an empty cell".
* (E4) A forward pointer from 1.4 to Figure 3 (one round of the abstract game) and to Table 6 (the first
  three rounds of the actual table) would give the reader a picture on page 3.

**Order of material.** Sound: board, pairings, leaf conditions, abstract game and proof of the theorem, then
the computed fact and its verification. Two changes would help.

* (E5) Section 4.3 (pp. 17–21, four pages: U-assignments, Proposition 4.15, Remark 4.16, Corollary 4.17, the
  five-step "exact test") justifies only Algorithm 2. The paper says so, but at the end of the opening
  paragraph. A reader who wants the theorem should be told in the first sentence that 4.3 can be skipped.
  Better: move 4.3 to Section 7.3, next to Algorithm 2, and delete the five-step list at
  `sec_leaf.tex:452-476`, which describes Algorithm 2 a second time. Section 4 then has six pages and
  contains exactly what Theorem 1.1 uses (4.1, 4.2, 4.6) plus 4.4–4.5.
* (E6) Table 2 (notation) floats into the discussion of Lemma 2.8 on p. 10, covers Sections 2–4 only, and is
  never cited. See T2 below.

**Section 5 (14 pages).** Readable, and the informal Section 5.1 is good. It needs a roadmap and an example:

* (E7) Add a roadmap after the opening paragraph (`sec_game.tex`, before "Conventions"): "Section 5.1
  explains the idea. Sections 5.2–5.4 define states, children, the label WON and certificates; Section 5.5
  defines the strategy sigma. Sections 5.6–5.7 analyze one move of Maker; Section 5.8 proves soundness and
  Section 5.9 Theorem 1.1. For R_max <= 10, the case used in this paper, the predicate LOST is identically
  false (Proposition 5.9(c)) and a move of Maker creates at most one forced event (Remark 5.13); a reader
  interested only in Theorem 1.1 may skip Definition 5.2 and alternative (2) of Lemmas 5.11 and 5.12."
* (E8) A worked example at the end of Section 5.2, taken from Table 6: the root P(∅,∅,0); its only child
  Q(∅,∅,1) with table move (0,0,0); child (i) P({(0,0,0)},∅,1); child (ii) Q(∅,{(0,0,0)},1) with table move
  (1,1,1); and one threat child. At present the first concrete state appears in Section 6.6, twenty pages
  after the definition, and Section 5 has no pointer to it.
* (E9) Where a reader gets lost: (i) Definition 5.1(d): the conditions on Q_a, Q_b are explained only
  fifteen lines later ("The meaning is as follows"); move that paragraph directly after the list of
  children. (ii) Definition 5.2 (E2/E3): the heaviest definition of the paper, given before Lemmas 5.11 and
  5.12 motivate it and vacuous in the application; a small figure of the two patterns and the "may skip"
  note of E7 would help. (iii) Remark 5.13: four counts in one paragraph; set them as a list. (iv) Remark
  5.21 (`sec_game.tex:890`): "requirement (ii) would be void" uses "void" in the ordinary sense in a paper
  where VOID is a technical term; write "vacuous".
* (E10) Section 5.10 from "The abstract game is monotone in the revealed set" on (Proposition 5.19,
  Corollary 5.20, Remark 5.21, about one page) is not used and explains why the children are lettered (a),
  (c), (d). Move it to Appendix C.2 (or the supplement) and keep two sentences.

**Algorithms in Section 7.** Both can be implemented from the page. Details:

* (E11) Algorithm 1, lines 14–15 (`sec_verification.tex:70-71`, `:102-103`): a′ and b′ are *cells* here; in
  Definition 5.2 and Lemma 5.12 a′, b′ are *lines*. Rename to u ∈ a, v ∈ b.
* (E12) Algorithm 2, line 9 (`sec_verification.tex:177`): "triple T = {a, b, c}" uses c for a *line*; c is a
  cell everywhere else. Write T = {l_1, l_2, l_3} "in which l_2 shares a cell outside F with l_1 and with
  l_3".
* (E13) "strong" appears in the comment of line 8 and is defined only inside the proof of Proposition 7.2
  (`:208`); "require" is explained after the algorithm (`:45-46`), with the letter X, which is Maker's stone
  set everywhere else. Define both before the algorithms; write "require ⟨condition⟩".
* (E14) Proposition 7.2 is stated at the bottom of p. 46 and proved on p. 47, after the algorithm float.

**Sections 6–8 (18 pages) and redundancy.** See R10. The redundancies I found:

* validity of a certificate is defined in Definition 5.6, paraphrased in 6.1 and restated in full in 7.1;
* the leaf test is described at the end of 4.3 and again as Algorithm 2; Definition 4.14 is restated in 7.3;
* Table 3 is repeated, transposed, in Appendix A.5, and its totals again in 8.5 and 9.2;
* the SHA-256 checksums are printed in 1.7 and in 6.1;
* "57 P-states", "143,498 / 28,275 against 46,412", "60.8%" appear in 6.3 and almost verbatim in 9.2 ("The
  shape of the certificate"); "160 of 656" and "99.55%" in 7.6 and again in 9.2 ("LEAF versus C");
* the trusted base is stated in 1.6, 7.7 and 9.3;
* Appendix A.1 repeats Section 2 (intended, so that the appendix stands alone; acceptable if A goes to the
  supplement).

## 4. Presentation

**Title.** Fine. The running head and the archive's `README.md` say "6x6x6 tic-tac-toe is a draw"; make the
three agree.

**Abstract** (`sec_front.tex:12-24`). Accurate in every clause; about 200 words, which is at the upper limit
for JCTA and Combinatorica and fine for EJC. Two changes: "The table has 93,164 entries" -> "Up to the
symmetries of the board, the table has 93,164 entries"; "a certificate derived from Hall's condition" -> "a
condition derived from Hall's theorem" (see N1).

**MSC, keywords.** 91A46, 05C57, 05D15, 68V05 are defensible. Add 91A24 (Positional games); consider 05C65
(Hypergraphs) instead of 05C57, since the game is played on a hypergraph and not on a graph. Keywords: add
"strong draw".

**Tables and figures.** All legible at 60 dpi; captions self-contained; numbers right-aligned; `booktabs`
throughout.

* (T1) Figure 2 (`sec_leaf.tex:617-633`, p. 23) is never cited. Add "(Figure 2)" after "So T = {d, a, b} is
  a triangle of live lines".
* (T2) Table 2 (`sec_board.tex:371-394`, p. 10) is never cited and stops at Section 4. Cite it in Section
  1.8 ("The notation is collected in Table 2") and add the symbols of Section 5: θ, P(B,F,t), Q(B,F,t),
  VOID, WON, LOST, R_max, (Tbl, Lv, R_max), V, σ, rk. Place it at the end of Section 1 or as an appendix
  table.
* (T3) Tables 4 and 5 (p. 41) float into the middle of a sentence of the bullet list of Section 6.3, and
  Table 5 precedes its own subsection 6.4. Use `[t]` after the list or `\FloatBarrier`.
* (T4) Table 10 (`sec_repro.tex:140-166`, caption `:163-165`, p. 54): the text says "all 26 steps passed"; the table has eleven
  rows. Add to the caption "(the 26 steps of the script are grouped into eleven rows)".
* (T5) Running times of the same program differ without explanation: treecheck "14.8 s on three threads (8.2
  CPU-seconds)" in Table 7 (`sec_verification.tex:262-263`), "3 seconds ... with four threads (11
  CPU-seconds)" in 8.3 (`sec_repro.tex:100-102`), "15 s" in Table 10, "a few seconds" in 1.7. A wall-clock
  time above the CPU time on three threads looks like an error. Give one reference timing (idle machine)
  and mark the others as "loaded machine".
* (T6) Table 7 is dense but readable; the row "the same with --crosscheck" belongs under the first row
  (indent it) so that "the first five" in the text (`:315`) is not read as five programs.

**Notation.** Mostly consistent. The overloads that can mislead:

* (N1) **"certificate"** has four meanings: the triple (Tbl, Lv, R_max) (Definition 5.6); the predicates C,
  C_Φ, LEAF ("leaf certificate", "the certificate C", "pairing certificate C", Sections 1, 4, 6.4, 7.6,
  B); a pairing ("A pairing is a certificate that is easy to check", 3.4); and flow assignments (Remark
  7.3). The same predicate LEAF is called "leaf certificate" (1, 4), "predicate" (5, 7), "leaf predicate"
  (6.3, B) and "leaf condition" (6.6). Reserve "certificate" for the triple; call C, C_Φ, LEAF "leaf
  conditions" throughout (title of Section 4, Definitions 4.4 and 4.7, Table 2, abstract, 1.4, 6.4, 7.6, B,
  C.1), and write "witness" in 3.4 and in Remark 7.3.
* (N2) K: the subgroup K = G_P G_Π (`sec_board.tex:242-254`, proof of Lemma 2.6(b),(c)) and the set of busy
  cells (calligraphic K, used in part (d) of the same lemma). Rename the subgroup, e.g. G_0.
* (N3) σ: a surplus value (`sec_leaf.tex:135`), elements of G_Σ (`sec_board.tex:244-245`), and the strategy.
  Use s or u at `sec_leaf.tex:135`.
* (N4) Q: the board, Q-states (sans serif), the stone sets Q_a, Q_b, and Q_7. P: P-states, the maps P_r,
  the conditions (P1), (P2). At least rename Q_a, Q_b (for instance R_a, R_b; "revealed").
* (N5) Φ: the least surplus Φ(B,F) (Definition 4.7) and the map Φ: A -> Q (`sec_discussion.tex:28`). Rename
  the map (ι, or φ^3).
* (N6) Labels (R1)–(R3) in Appendix C.1 (`app_extra_A.tex:85-90`, used to `:130`) against (R0)–(R2) of Definition 5.6;
  rename to (C1)–(C3).
* (N7) "line": a line of the board, a line of a file ("every line of both files is reachable",
  `sec_certificate.tex:25`; "table lines", "leaf lines", `sec_verification.tex:273, 317`), and a line of
  play ("Line A", "Line B", `sec_certificate.tex:368-397`). Write "entry" for file lines and "Play A",
  "Play B".
* (N8) l and ℓ: lines are l in Sections 3–8 and ℓ in Sections 2.1 and 9.1 and Appendix D. "id": the identity
  map (Section 2) and the number of a cell (Definition 5.8); introduce "id" for cells in Section 2.5, where
  the number 36x + 6y + z is defined. H: the group of Section 2.3 and the set H ⊆ Q of the eager variant.

**Pronouns and style.** Breaker and Maker are "he" in the abstract and Sections 1, 3, 4, 6, 9 and Appendix
A (nearly 30 times); Sections 5, 7, 8 avoid pronouns ("this player", "of their own"). Decide once, say so in
Section 1.1 if "he" is kept. "Maker stones", "Breaker stones" (Sections 6, A; 17 times) against "stones of
Maker", "Maker's stones" elsewhere: unify. Spaced em dashes in Sections 6–7 and A.3, none elsewhere.
"upwards", "afterwards" beside "toward", "backward".

**Typos and grammar.** I found no misspelled word (word list of the sources checked by eye; no spell checker
is installed) and no doubled word. What I found:

| location | problem | fix |
|---|---|---|
| Lemma 2.3, proof, `sec_board.tex:114`, p. 7 | "which gives 24, 48 and 144 cells": order differs from the statement ("24, 144 and 48 cells of these three kinds") | "which gives 24 cells with one level, 48 with three distinct levels and 144 with exactly two" |
| Section 6.3, `sec_certificate.tex:185`, p. 41 | "the set T_8 of Section 6.4" | "the set T_8 of Example 3.9" |
| Remark 5.21, `sec_game.tex:890`, p. 37 | "would be void" (ordinary sense) | "would be vacuous" |
| A.4, `app_formats.tex:102`, p. 61 | "|B| = t - 1 resp. |B| = t" | "|B| = t - 1 for a Q-state and |B| = t for a P-state" |
| 7.5, `sec_verification.tex:334, 340, 347`, pp. 48–49 | sentences begin with numerals ("66 seeded ...", "58 were ...", "8 were ...", "12 variants ...") | "In all, 66 ...", "Of these, 58 ...", "Eight ...", "Twelve ..." |
| 6.3, `sec_certificate.tex:186` | "434 of the 93,164 table moves are ..." begins with a numeral | "Of the 93,164 table moves, 434 are ..." |
| 1.5 (`sec_intro.tex:169`), 9.2 ("And LEAF is monotone") and eight more in Sections 1, 3, 4, 5, 9, C | sentences beginning with "And" or "But" | join to the preceding sentence |
| refs [4], `refs.bib:88` | "2 ed." | `edition = {2nd}` |
| 7.2, `sec_verification.tex:45-46` | "require X" | "require ⟨condition⟩" |

**LaTeX problems visible in the PDF.**

* Theorem 1.1 is split between pages 2 and 3 after its second line (`sec_intro.tex:79`). The main theorem
  should not break; move Table 1 or add `\needspace`.
* File names break at awkward points: "results/ final/stats.json" (p. 40), "spotcheck_ full.json" (pp. 38,
  54), "logs/core/ test_core.log" (p. 23), "ip_regression. json" (p. 65), "scripts/
  check_7cube_midplanes.py" (pp. 57–58).
* Floats that interrupt sentences: Table 2 (p. 10), Tables 4–5 (p. 41), Algorithm 1 (p. 45), Table 7 (p.
  49). Normal LaTeX behaviour; fix only T3.
* No overfull or underfull box, no font substitution, no widow or stranded heading (checked the last two
  lines of all 73 pages). `\raggedbottom` gives uneven page heights; the journal class will undo this.
* PDF metadata: author and keywords empty.

**References.** 19 entries, amsplain; complete for what is cited. Inconsistent author names: "L. V. Allis"
[1] and "Louis Victor Allis" [2]; "Alfred W. Hales" [9] and "A. W. Hales" [10]; initials only in [7, 8, 10,
11, 14], full first names elsewhere. Choose one form. [15]: see P8. [4]: see P4 and the table above. No
DOIs; add them if the journal wants them.

## 5. Reproducibility statement and declarations

**(R1) "Use of software tools"** (`sec_discussion.tex:310-314`, p. 60). The paragraph is true as far as it
goes, and it is too little, for three reasons.

*It is narrower than the project's own records.* It names "the search program, the verifiers and the other
programs, the computations ... and the drafting of the manuscript". `PROGRESS.md`, `REPORT.md` and
`REVIEW.md`, all in the archive, record in addition that the written mathematics (`PROOF.md`, from which
Sections 2–5 were ported), the "internal review" (the mathematical referee report, the code audits, the
mutation experiments, the two re-expansion programs of Table 7) and the literature search were produced by
software agents, and that the whole project ran within one day. A reader who opens the archive finds more
than the paper tells.

*It does not say what "independent" means.* Sections 1.6, 6.1, 7.4, 7.5 and 8.1 speak of "independently
written programs", "an internal review", "audits", "review reports". In a mathematics paper these words are
read as: different people. Here they mean: separate sessions of software agents, working from the same
written specification, without access to each other's code (`REPORT.md:22, 80-82`). That is a real form of
independence, and the agreement of all counters is strong evidence that the programs expand the same tree.
It does not exclude a common misreading of the specification by programs produced by the same kind of
tool, and the reader is entitled to weigh that. Section 7.4 refers to the declaration "on how the programs
were produced", and the declaration does not answer.

*It lacks what journals require.* The name and version of the tool; what the human author(s) did, in
particular whether the proofs of Sections 2–5 were checked line by line by a person. "Who take
responsibility for the content" needs that sentence behind it.

Required:

1. Replace the paragraph by a section `\section*{Declarations}` placed after Section 9 (it is now a
   `\subsection*` inside Section 9, visually the last items of "Open problems and limitations"), with the
   following text; brackets are for the authors to complete or correct, and nothing in it should be kept
   that is not true:

   > **Use of AI-based tools.** This work was carried out with AI-based software agents ([name and version
   > of the model and of the agent framework, to be supplied]), directed by the author(s). The agents wrote
   > all programs of the supplementary archive, among them the search and both verifiers, ran the
   > computations, drafted the proofs of Sections 2–5 and 9 and of the appendices, searched the literature
   > and drafted the manuscript. The internal review mentioned in Sections 1.6, 6.1, 7.4, 7.5 and 8.1 (the
   > audits, the mutation experiments and the two re-expansions of Table 7) was carried out by such agents
   > as well, in sessions separate from those that produced the material under review. Throughout the
   > paper, "independently written" means: written in a separate session, from the written specification,
   > without access to the source code of the other programs; [all sessions used the same model — confirm
   > or correct]. The author(s) [state what was checked by hand, for example: have checked the proofs of
   > Sections 2–5 and of Corollary 9.3 line by line and have run both verifiers of Section 8.3 themselves]
   > and take full responsibility for the content of the paper.

2. Section 7.4, "Independence" (`sec_verification.tex:302-312`, p. 48): after the pointer to the declaration
   add: "Independence means here that the programs share no code and were written without access to each
   other's sources. It does not exclude a common misreading of the specification. Against that stand the
   precise statement of the algorithms in this paper, the agreement of the counters in every round, the
   comparisons with brute force of Section 7.5, and the possibility of writing one's own verifier (Section
   8.5)."
3. Section 1.6, "Trusted" (`sec_intro.tex:198-202`, p. 5): add "The programs, including all verifiers, were
   written with AI-based tools; see the Declarations and Section 7.4 for the sense in which they are
   independent."
4. First use of "internal review" (`sec_intro.tex:191-193`, p. 4): "and by two further programs written
   separately during a review of the project (Section 7.4)".
5. Add the usual placeholders next to the author line: acknowledgments, funding, competing interests.

**(R2) "Data and code availability"** (`sec_discussion.tex:316-320`, p. 60; also Section 1.7,
`sec_intro.tex:206-208`, and Section 8.1, `sec_repro.tex:61-68`). The two placeholders "[repository URL to
be added]" are visibly marked, which is acceptable. Not acceptable for a journal:

* *No licence and no persistent identifier are mentioned*, not even as placeholders; the archive contains no
  licence file.
* *"The project repository".* Computed Facts 2.9, 3.10, 4.23 and C.4, the test at the end of C.1 and Remark
  9.4(ii) name files "in the project repository" (`sec_board.tex:359`, `sec_pairing.tex:220`,
  `sec_leaf.tex:659`, `app_extra_A.tex:178`, `app_extra_B.tex:180-181`, `sec_discussion.tex:180`),
  and Section 8.1 says they are not in the archive. The paper never says what or where this repository is;
  according to `PROGRESS.md` no repository exists. For Computed Facts 2.9 and C.4 the program itself is
  missing from the archive; for 3.10 and 4.23 the cited logs and the confirming script are. A Computed Fact
  whose program the reader cannot obtain is not a Computed Fact in the paper's own sense ("name the
  programs that established them").
* *Two of the four verifiers are not available.* Section 1.6 and Computed Fact 6.1 cite "two further
  independent programs written during an internal review" as evidence; Section 8.1 says they are not in the
  archive. Evidence cited for the main computed statement must be inspectable.
* *The archive in `dist/` is out of date.* It contains 14 of the 19 source files of the paper
  (`sec_front.tex`, `sec_intro.tex`, `sec_discussion.tex`, `app_extra.tex`, `app_seven.tex` and `paper.pdf`
  are missing), against Table 9 ("paper/ — sources of this paper"). `issues.md` (E4.2) knows this; it must
  be rebuilt before submission, and the size "about 3.5 MB" re-measured.

Required:

1. Add to the archive (about 100 KB): `logs/canon/autgroup.{cpp,log}`, `logs/core/test_core.log`,
   `logs/minstones/run.log`, `logs/paper/secA_check1.py`, `logs/paper/secA_checks.log`,
   `logs/proof/e23min.{cpp,log}`, `logs/proof/fuzz_cprime.py`, `scripts/check_7cube_midplanes.py`, and the
   two re-expansions with their outputs (`logs/review/treecheck_mut/aexpand.cpp`, `aleaf.cpp`,
   `aexpand_final.json`; `logs/review/lead/lead_expand.cpp`, `lead_leaf.cpp`, `lead_expand.json`). Then
   replace "of the project repository" by "of the archive" in the six places above, delete
   `sec_repro.tex:62-68` from "and the small programs written during the internal review" to "project
   repository.", and add a licence file.
2. Replace the paragraph "Data and code availability" by:

   > **Data and code availability.** The certificate (two text files), the source code of all programs, the
   > outputs of all runs reported in Sections 6 to 8 and in the appendices, and the scripts that reproduce
   > them are contained in the supplementary archive `ttt6-artifact.tar.gz` described in Section 8 (version
   > [to be supplied]; SHA-256 [to be supplied]). The archive is deposited at [DOI of a permanent archive,
   > to be supplied] and is also available at [repository URL to be added]. The programs are released under
   > [licence to be supplied], the certificate and the other data files under [licence to be supplied]. The
   > SHA-256 checksums of the two certificate files are given in Section 1.7.

3. In Section 1.7 replace "it is available at [repository URL to be added]" by "see 'Data and code
   availability' at the end of the paper".

## 6. Length (R10) and what can move

Page budget now: Section 1: 4; 2: 5; 3: 4; 4: 10; 5: 14; 6: 6; 7: 8; 8: 3; 9: 5; Appendix A: 3.5; B: 2.5;
C: 6; D and references: 1.5.

None of the following removes a proof needed for Theorem 1.1 or Corollary 9.3, and nothing needed to
replicate is lost: what moves goes to a supplement (to be posted with the archive and with the arXiv
version), and Algorithms 1 and 2 with their correctness proofs, Appendix A.2–A.4 and A.6 (file formats and
test vectors) and the totals of Table 3 stay in the article.

| item | pages | proposal | saving |
|---|---|---|---|
| Appendix C.1 (monotonicity of C in B; "not used anywhere") | 66–68 | supplement, or a separate note | 2.5 |
| Appendix C.2 together with Proposition 5.19, Corollary 5.20, Remark 5.21 | 37–38, 68–69 | supplement; two sentences stay in 5.10 | 2 |
| Appendix C.3 (exact stone numbers of E2/E3, Computed Fact C.4, Example C.5) | 69–72 | supplement; Remark 5.18(ii) keeps a pointer | 2.5 |
| Appendix B (integer program, a cross-check) | 63–65 | supplement; four lines stay in 7.6 | 2.5 |
| Section 6.2 (how the search works) | 38–40 | half a page stays (it is not part of the proof); the rest to the supplement | 1 |
| Section 6.5 (two single-program observations) | 42–43 | one remark of six lines; details to the supplement | 0.7 |
| Sections 7.5–7.6 (audits, mutations, cross-checks) | 48–51 | one page with Table 8; details to the supplement | 2 |
| Section 7.7, "Known minor issues" | 52 | to `README.md`; one sentence stays | 0.3 |
| Section 8 (Tables 9, 10, command lines) | 52–55 | one page (what to run, what to expect, Section 8.5); the rest is `README.md` | 2 |
| Appendix A.1, A.5, A.7, A.8 | 60–63 | supplement (A.5 duplicates Table 3) | 1.5 |
| Section 4.3: duplicate description of the leaf test | 21 | delete (E5) | 0.5 |
| Section 2.6 (full automorphism group, not used) | 10 | one remark | 0.3 |
| Section 9.2, "The shape of the certificate" and "LEAF versus C" | 58–59 | cut the repetitions of 6.3 and 7.6 | 0.5 |
| total | | | about 18 |

Result: an article of about 55 pages (introduction 4, mathematics 33, certificate and verification 10,
7x7x7 and remarks 4, formats and references 4) and a supplement of about 18 pages. For EJC the authors may
keep more; for JCTA or Combinatorica I would also move the proof of Proposition 5.16 (strategy stealing,
one page; a standard result, for which [15, Theorem 3.1] can be cited) to an appendix. I do not ask for
that.

## 7. Required changes and optional suggestions

### Required

| # | change | location |
|---|---|---|
| R1 | Declaration on AI-based tools: complete, name the tool, say what "independent" and "internal review" mean, say what the authors checked; own section; pointers from 1.6 and 7.4 (text in Section 5 above) | `sec_discussion.tex:310-314`; `sec_verification.tex:302-312`; `sec_intro.tex:191-193, 198-202` |
| R2 | Availability: licence, DOI, version and checksum of the archive as marked placeholders; put the cited programs into the archive and remove "project repository"; rebuild the archive (text in Section 5 above) | `sec_discussion.tex:316-320`; `sec_intro.tex:206-208`; `sec_repro.tex:61-68`; six places listed above |
| R3 | Credit: the plan of [5, Section 6.2] for 6x6x6 (P1); Golomb–Hales Theorem 3 (P2); "in a preprint" (P3) | `sec_intro.tex:69-72`; `sec_discussion.tex:186-190, 7-8` |
| R4 | One paragraph of context on computer-assisted results for positional games (P6); published version of [15] (P8); *Winning Ways* (P4); read [10], [16] (P5) | `sec_intro.tex` after `:72`; `refs.bib:208, 85-90` |
| R5 | One formulation of the computed statement (F1); wording of the convention on Computed Facts (F2); "only statement" (F3) | `sec_game.tex:825-827`; `sec_intro.tex:86-93, 187-188`; `sec_certificate.tex:44-45` |
| R6 | Remove the three overstatements (F4, F5, F6) | `sec_certificate.tex:190-191, 273, 121-123`; `sec_repro.tex:169` |
| R7 | "certificate" for one thing only (N1); remove the clashes N2, N3, N5, N6, N7, E11, E12 | see Section 4 |
| R8 | Cite Figure 2 and Table 2; extend Table 2 to Section 5 (T1, T2); Table 10 caption (T4); timings (T5) | `sec_leaf.tex:617-633`; `sec_board.tex:371-394`; `sec_repro.tex:163-165`; `sec_verification.tex:262-263` |
| R9 | Roadmap and worked example in Section 5 (E7, E8); the idea of LEAF and the definition of "round" in the introduction (E1, E2) | `sec_game.tex` (opening, end of 5.2); `sec_intro.tex:130, 76` |
| R10 | Shorten: move about 18 pages to a supplement (table in Section 6) | Sections 6–8, Appendices A–C |
| R11 | Typos and grammar of the table in Section 4; Theorem 1.1 must not break across pages | as listed |

### Optional

* Move Section 4.3 to Section 7.3 (E5); move the eager variant out of Section 5 (E10).
* Present the case R_max <= 10 directly through Remark 5.13 and put LOST and the patterns E2, E3 into an
  appendix as the generalization to larger R_max. This would shorten Section 5 by two to three pages and
  remove from the article a predicate that is identically false in the application and whose code "is not
  exercised" (Section 7.7). It is a larger rewrite; the note of E7 is the minimum.
* A figure of the patterns E2 and E3 (E9).
* Zermelo (P9); Dvořák–Valla (P10); references on certifying algorithms and relevance-zone search (P7).
* MSC 91A24; keyword "strong draw"; abstract "up to the symmetries of the board".
* One convention for pronouns, for "Maker's stones", for l and ℓ, for dashes (Section 4).
* One sentence in Section 1.6 on what must be wrong for the theorem to fail, with a pointer to Section 7.1.
* A remark after Theorem 1.1 that Breaker then also wins when he moves first.
* State the machine precisely in Section 8.2 (CPU model, operating system and version).
* Fix the float placement of Tables 4 and 5 (T3); indent the second row of Table 7 (T6); move Proposition
  7.2 next to its proof (E14); avoid the breaks inside file names.

# Response to the five referee reports

Manuscript: "The 6x6x6 tic-tac-toe game is a draw". Reports: `logs/paper/referee_math_a.md`,
`referee_math_b.md`, `referee_audit.md`, `referee_journal.md`, `referee_replication.md` (all on the first
version, 73 pages, kept in `logs/paper/paper_v1/`).

This document merges the two change logs of the first phase of the revision (`logs/paper/response_M.md`
for Sections 2 to 5, Section 9.1 and old Appendix C; `logs/paper/response_C.md` for the other parts) with
the actions of the second phase (restructuring into main part and supplementary material, consistency
pass, final build, archive). It is organised by report and by the referee's own numbering. Every finding
has a status:

* **done** - corrected as the referee proposed, or in a way that goes further;
* **done differently** - the problem is removed, but not in the way proposed; the reason is given;
* **not done** - with the reason. All such findings are collected once more in Section 6.

Numbers of statements, tables and sections in the column "finding" are those of the first version, as in
the reports. Numbers in the column "what was done, and where" are those of the final PDF
(`paper/paper.pdf`, 91 pages); Section 8 below maps old numbers to new ones. Source files are in `paper/`.

## 0. The revised paper in brief

| | |
|---|---|
| structure | ONE PDF in two parts. **Main part**, pages 1-70: Sections 1-9, the unnumbered section "Declarations", Appendix A (file formats, canonical form, test vectors), Appendix B (pairing table for the 7x7x7 board), references. **Supplementary material**, pages 71-91: Appendices S1-S7 after the references, with their own heading, an overview table and their own numbering (statements S1.1, ..., tables S1, ...). Nothing was deleted; every moved item left a pointer or a summary in the main part (Section 7.1 below). |
| what the main part contains | everything the proof of Theorem 1.1 and of Corollary 9.3 uses (Sections 2-5, Computed Fact 6.1, Section 9.1, Appendix B) and everything a replicator needs (definitions, Algorithms 1 and 2 with correctness proofs, the lemma on deleted lines, the list of verifiers, file formats down to the byte level, canonical form, test vectors for the canonical form, for LEAF and for LOST, the numbers of states and children in every round) |
| build | `latexmk -C && latexmk -pdf` in `paper/`: 91 pages; no undefined reference or citation, no multiply defined label, no LaTeX or BibTeX warning, no overfull and no underfull box |
| cross-references | `logs/paper/check_refs.py`: 250 labels, 1,138 references to 223 targets, 73 citations of 26 keys; every target exists, no label is defined twice, every source file is input exactly once |
| archive | `ttt6-artifact.tar.gz`: 314 files (MANIFEST.sha256 included), about 6.3 MB, about 38 MB unpacked; tested in a fresh directory (`logs/paper/artifact_test_revision.log`) |

Decisions that were fixed before the revision, and their status:

| decision | status | what was done |
|---|---|---|
| D1 Declarations | done | Unnumbered section "Declarations" after Section 9 (`sec_discussion.tex`). Its paragraph "Use of AI-based tools" says that AI-based software agents, directed by the author(s), wrote all programs of the archive (the search and all verifiers), ran the computations, drafted the proofs, searched the literature, drafted the manuscript, and carried out the internal review and the internal refereeing in sessions separate from those that produced the material under review; it defines "independently written", says that this does not exclude a common misreading of the specification, and says what stands against that. The two placeholders for the names of the tools and for what the author(s) checked are kept verbatim. Nothing in the paper says that the authors checked anything. Pointers from Sections 1.4, 1.6, 7.4, 7.7, 9.3. |
| D2 three programs written from the paper | done differently | Reported in Computed Fact 6.1, Sections 1.6, 7.4 (Table 7, "Independence", "Agreement"), 7.5, 7.7, 8.5, Appendix A.5 and A.6. Only what the reports say is stated. The reports support "written from the paper alone, without access to the specification or to any program" for the replication verifier (`verify.cpp`) only; the audit referee had read `verify/treecheck.cpp` and `spotcheck.py` for the audit, and referee B worked inside the project tree. The paper therefore says "alone" for the first program, and for the other two that they were written from the text of the paper by referees who had the project files at their disposal and that they share no code with the project. The third program (`b_expand.py`) has no leaf test and is called a transcription of Algorithm 1, not a verifier. Because the five reports come from an internal refereeing, the paper says so ("internal refereeing of the first version") and the Declarations say who carried it out. |
| D3 length: one PDF, nothing deleted | done | See Section 7.1. |
| D4 availability | done | Every file named in the paper is in the archive (`scripts/make_artifact.sh` checks this for every file name with a directory and stops otherwise); "project repository" occurs nowhere; licence and DOI/URL are marked placeholders in "Data and code availability" and in `README.md`. |

Placeholders in the final paper, all in square brackets: the author names (title page, and with
acknowledgments, funding and competing interests at the end of the Declarations), the names of the tools,
the statement of what the author(s) checked, the DOI/URL of the archive, the licence.

---

## 1. referee_math_a.md (Sections 2-4, Appendix B, Appendix C.1, Algorithm 2)

| # | finding | status | what was done, and where |
|---|---|---|---|
| 1 | MAJOR. The leaf test of the main verifier deletes strong lines; the argument is not in the paper; Remark 7.3, "Accelerations", "Cost", Table 7 row 1 and the trust sentences do not match the program | done | New subsubsection "Deleting strong lines" in Section 7.3 with Lemma 7.5 (Deleted lines) and its proof: (a) soundness of true verdicts, (b) exactness; the induction on the order of deletion and the properties to be validated are stated after the lemma. Remark 7.4 covers the pair rule and points to the lemma. Table 7, row 1, names the deletion, the backward search for theta = 1 and the literal validation. Section 7.7 says which implementation uses which variant; Section 1.6 mentions the variant. The step-by-step description of the test of `verify/treecheck.cpp` and the explanation of the flow counts (869,185 = single-line flows for theta >= 2 from lines with at least theta + 2 cells outside F) are in Appendix S4.1; a summary is in Section 7.3 (`sec_verification.tex`, `supp_checks.tex`). |
| 2 | Missing step in the proof of Prop. 7.2: lines 3-5 are not the max-flow algorithm of Lemma 3.3 | done | Proof of Proposition 7.3: the pairing loop is argued directly; if `Augment` fails, the set of lines reachable from l violates Hall's condition. No appeal to max-flow min-cut. |
| 3 | Figure 2 contradicts the proof (r = 2, s = 3 from the same pair) | done | Figure 2 redrawn with r = 2, s = 0; the caption names the three meeting cells and says that the same lines form a triangle with the other diagonal. Checked by program for both diagonals (`logs/paper/revM_checks/`). |
| 4 | "LEAF is strictly weaker than C when F is non-empty" (Section 7.6) | done | "LEAF is a strictly stronger condition than C (it fails on instances on which C holds), not only in the situation of Example 4.11" (Section 7.6 and Appendix S4.3). |
| 5 | Missing hypothesis "F admissible" in Remark 4.10 (iv) and in Section 9.2 | done | Remark 4.10 (iv): "agree for B-admissible F and theta = 0", with the reason; Section 9.2 ("LEAF versus C"): "agree, for admissible F, when all stones are revealed, and when none is revealed and t <= 10". |
| 6 | Corollary 4.21 applied outside its range (k >= 11) | done | Text after Computed Fact 4.23: for k <= 10 by Corollary 4.21, for k >= 11 by Proposition 4.18 (b). |
| 7 | Proposition 4.22 is far from sharp (optional) | done | Proposition 4.22 is now stated for at most 17 busy cells, with the referee's argument (the 36 face diagonals are the pairs inside six classes of four busy cells), re-derived and checked by program; a sentence says that 17 cannot be improved in this step. Section 6.4 uses it for ten cells only. |
| 8 | Order of the counts in the proof of Lemma 2.3 | done | "24 cells with one level, 144 cells with exactly two and 48 cells with three distinct levels". |
| 9 | Remark 4.5 (ii): F' must be disjoint from B | done | Added. |

## 2. referee_math_b.md (Theorem 1.1, Section 5, Appendix C.2-C.3, Section 9, Algorithm 1)

| # | finding | status | what was done, and where |
|---|---|---|---|
| 1 | Remark 9.4 (iii) claims "draw implies draw" | done | Remark 9.4 (iii) rewritten: the two hypotheses of Golomb and Hales, their Theorem 3 (with credit for the mid-plane decomposition), what the proof of Corollary 9.3 is, and exactly what it establishes: if Breaker wins on 6^3 as second player, then Breaker wins on 7^3; not "draw implies draw"; nothing for 5^3. |
| 2 | Remark 9.4 (ii): the cited program does not check the printed table nor Lemma 9.1 (d) | done | Both alternatives: `scripts/check_7cube_midplanes.py` now parses the table from `paper/app_seven.tex` and checks it as printed (45 different lines, exactly the lines inside M; both cells on their line; 90 different cells; the rule of the proof of Lemma 9.2 reproduces every entry; three seeded mutations are rejected), and the remark says exactly this and that the matching part of the script shows existence only. In the second phase the script and its output were updated to the new name of the appendix (Appendix B) and rerun: all checks pass (`logs/paper/check_7cube_midplanes.out`). |
| 3 | Section 7.1: unproved side remark on non-minimal representatives | done | Remark 7.2 with proof (induction on the rank; the root is fixed by G); the sentence in Section 7.1 is a pointer to it. |
| 4 | 7x7x7: two parametrizations per line and the number 193 are quoted, not proved | done | Section 9.1 proves both by means of normalized parametrizations; "in accordance with" Golomb-Hales, "not used below". Lemma 9.1 (d) says that the 45 entries are 45 different lines, and the proof says why. |
| 5 | Notation: a', b' as cells in Algorithm 1; the board symbol in the proof of Lemma 5.4 | done | Algorithm 1 and the comment "The children": the two cells are p, q. Lemma 5.6: "t <= 217, because B is a set of t or t-1 cells". The board is now written Omega (see journal N4). |
| 6 | Definition 5.8: the letter t after the switch; Theorem 5.15 (c) | done | Definition 5.10: "let t denote the number of completed rounds; in TABLE mode this is the third component of the record"; Theorem 5.17 (c): "in every round after round R_max the reply of Breaker is made in PAIRING mode", proof completed. |
| 7 | Theorem 5.14: "repeated at most t+1-|F| times" | done | Theorem 5.16: the rule reveals at most t+1-|F| stones and is entered at most t+2-|F| times; same in the proof. |
| 8 | The proof of Theorem 1.1 should use the explicit certificate | done | The proof starts from Computed Fact 6.1 (the triple of the two files is a valid certificate) and uses sigma for this certificate; a paragraph after the proof keeps the general implication (Lemma 5.9 (b), (c)). |
| 9 | Small points (Remark 5.13; Prop. C.3 (a) "distinct"; Cor. 9.3 rule (i); Section 5.6 "at most two coincide") | done | Remark 5.15: "(in fact 13 on this board)"; Proposition S2.5 (a): "four distinct lines"; Corollary 9.3, rule (i): parenthesis on the odd number of occupied cells; Section 5.6: "they overlap in at most two cells". |

## 3. referee_audit.md (every number and factual claim)

| # | finding | status | what was done, and where |
|---|---|---|---|
| F1 | The description of the leaf test of the main verifier and of its validation is not what the program does | done | See math A, finding 1: Remark 7.4, Lemma 7.5, Table 7 row 1, Section 7.7; the steps (1)-(4) of the program, read from `verify/treecheck.cpp`, are in Appendix S4.1. |
| F2 | Computed Facts 2.9 and C.4 (and files cited for 3.10, 4.23, C.1, Remark 9.4, and logs behind printed numbers, and the two re-expansions) are not in the archive | done | All ten files, the two re-expansions with their outputs, and the logs behind printed running times and counters are packaged (`scripts/make_artifact.sh`; Table S3 and `README.md` list them); "project repository" is replaced by "archive" in the six places. |
| F3 | "12 are void" is wrong for 3 of the 12 instances | done | "12 are degenerate (in 9 of them F is not B-admissible, in 3 of them |F| > k; the integer program is infeasible in both cases)"; "non-degenerate" for the 1,279 (Appendix S4.3, Appendix S5.4). Recounted from `results/crosscheck/certify.jsonl`. |
| F4 | The flow counts are not "over all 106,723 leaves" | done | "Cost" (Appendix S4.1): the counts are for the 9,179 leaves with theta >= 2; the 32,703 leaves with theta = 1 are not included; 8,549,521 searches in all (`results/final/treecheck.json`, `logs/repro/tc_all.log`). |
| F5 | "Same counters in every round" is not supported for `spotcheck.py` and for the self-check | done | Table 7 and its caption, "Agreement" in Section 7.4, the caption of Table 3 and the provenance of Computed Fact 6.1 say program by program what is reported per round and what in total. |
| F6 | "about 90 CPU-seconds" holds for one re-expansion only | done | Section 8.5: "1.5 to 5 CPU-minutes"; Table 7: "about 5 CPU-minutes" and "44 + 44 CPU-seconds". |
| F7 | Computed Fact 4.23: "the same values are recorded in ip_regression.json" | done | Re-read from the raw files: "The same four values are recorded in logs/core/test_core.log; the regression file ... records the two values for T8." |
| F8 | Description of the exact enumeration | done | Appendix S4.3: the two reductions are named; "examines 1.39 million sets"; `enumcheck` and `certify` are said to be built on the library of the search. |
| F9 | "930,120 table entries" | done | "930,120 transposition-table entries" (Appendix S3.1, "The run"). |
| F10 | The 7x7x7 result is a preprint | done | Abstract: "first obtained in a recent preprint"; Section 9.1: "first proved, in a preprint, by". |
| F11 | Reference entries [4], [17], [19] rest on unverified data | done | Winning Ways is cited only as "see also [Chapter 22]"; the attribution of the pairing for n >= 8 rests on Golomb-Hales p. 181, also in Table 1. The records of Winning Ways, Schrijver and Zermelo were checked with generic bibliographic queries; the ISBN of Winning Ways in `refs.bib` was wrong and is corrected. The comments in `refs.bib` say that these sources were not read. |
| F12 | The archive predates the final sources; re-check Section 8.1 | done | Rebuilt in the second phase; Section 8.1 and `README.md` give the size and the number of files; `make_artifact.sh` refuses a PDF older than the sources and a wrong number of files. Tested in a fresh directory (Section 7.4 below). |
| F13 | `--paranoid` is the default and is not in the usage text | done differently | Computed Fact 6.1, Section 8.3, Appendix S4.1 and `README.md` say that the option is the default. The usage text of `verify/treecheck.cpp` was not changed: any change would alter the checksum of the main verifier, to which all logs and both internal audits refer. |
| F14 | README versus paper (timings, title) | done | README: title of the paper; 12 s compile, 11 CPU-s / 3 s, 12 s self-test, 2 min 14 s. One reference timing in Section 8.3; the other runs are explained in Appendix S6.2. |
| F15 | Placeholders | done | See Section 0: all placeholders are in square brackets; URL, DOI and licence in "Data and code availability". |
| F16 | Remark 9.4 (iii): credit to Golomb-Hales Theorem 3 | done | See math B, finding 1. |
| F17 | "76 s for theta = 7" | done | "77 s for theta = 7 (four instances, between 37 and 133 s)" (Appendix S5.4), recomputed from the result files. |
| Sect. 4 | Claims the auditor could not verify (busy-only search etc.) | done | No change of substance was needed: the paper says that the busy-only observation rests on one program. Its wording was weakened further (journal F4, F5); the auditor's repeat of the run is cited (Appendix S3.2). |

## 4. referee_journal.md (publication aspects)

### 4.1 The claim and its framing (Section 1 of the report)

| # | finding | status | what was done, and where |
|---|---|---|---|
| F1 | Two formulations of the computed statement | done | One formulation: Computed Fact 6.1 is "(Tbl, Lv, 10) is a valid certificate"; "the root is WON" is its last sentence (Lemma 5.9 (b)). Section 1.3 and the proof of Theorem 1.1 (Section 5.9) use it. |
| F2 | "Statements established by running programs are called Computed Facts" is not accurate | done | Section 1.6 as proposed; in the second phase "the appendices, those of the supplementary material included" was added, and the heading page of the supplement repeats the convention. |
| F3 | "the only statement ... not proved in this paper" | done | "Apart from the two standard theorems quoted in Section 1.6, ..." (Section 6.1). |
| F4 | "Section 6.5 explains why non-busy table moves cannot be avoided" | done | "A computation with the search program indicates that non-busy table moves cannot be avoided for R_max <= 10 (Remark 6.2)" (Section 6.3). |
| F5 | Heading "Busy cells alone do not suffice" | done | "Busy cells alone seem not to suffice for R_max <= 10" (Remark 6.2 (i); Appendix S3.2 with "(single program)"); the caveat names the three things the conclusion rests on, among them the unproved pruning bound; Section 9.3 is weakened in the same way. |
| F6 | "The search is deterministic" | done | Sections 6.2, 8.4 and Appendices S3.1, S6.3: designed so that the recorded move does not depend on the order of the threads; determinism not proved; what was observed is byte identity of a run with two threads. |
| - | One sentence in 1.6 on what must be wrong for the theorem to fail (suggestion) | done | Section 1.6, "Trusted", with a pointer to the eight defects of Section 7.1. |

### 4.2 Positioning (Section 2 of the report)

| # | finding | status | what was done, and where |
|---|---|---|---|
| P1 | Credit to Cain, Raymond and Kallersjo for the plan on 6x6x6 | done | Section 1.2 quotes the plan (two sentences verbatim, as verified in the literature report) and says "The present paper carries out this plan"; repeated in Section 1.5. |
| P2 | Credit to Golomb-Hales for the step 6 -> 7 | done | Remark 9.4 (iii), see math B, finding 1. |
| P3 | "in a preprint" in Section 9.1 | done | Done. |
| P4 | Winning Ways cited for content that was not read | done | "see also"; Table 1 cites Golomb-Hales only (audit F11). |
| P5 | Hales-Jewett 1963 and Patashnik 1980 were not read | not done | Neither paper could be obtained. The Hales-Jewett bounds are still given through the two secondary sources that were read, and the text says so by "see". This remains a task for the authors before submission. |
| P6 | Missing context on computer-assisted results | done | New paragraph at the end of Section 1.2 (survey of solved games; Qubic; Go-Moku with threat-space and proof-number search; set matching; the (7,7,5)- and (8,8,5)-games). Nothing that the literature report marks as unverified is asserted. |
| P7 | Optional references (certifying algorithms, proofs by satisfiability solving, relevance-zone search) | done differently | One sentence with two references whose records were retrieved by DOI (certifying algorithms; a large proof by satisfiability solving); the sentence says only what the titles say. The relevance-zone reference was not added, because its record was not checked. |
| P8 | The survey of Krivelevich is cited as a preprint | done differently | The entry is the proceedings version (Proc. ICM Seoul 2014, vol. IV, pp. 355-379). It carries the note that the theorem numbers are those of the preprint, because the proceedings version was not read and the agreement of the numbering could not be checked. |
| P9 | Zermelo and backward induction | done | "the determinacy of such games, which is usually attributed to Zermelo and is proved by backward induction on the game tree" (proof of Corollary 5.19; proof of Corollary 9.3); Section 1.6 does not name Zermelo. |
| P10 | Dvorak-Valla against Computed Fact 2.9 | done differently | The second alternative of the referee: the paper says that the order 1,152 was not compared with the general description of that paper (Appendix S1.1). The Computed Fact stays; it is not used in the proof. |

### 4.3 Exposition (Section 3 of the report)

| # | finding | status | what was done, and where |
|---|---|---|---|
| E1 | The idea that makes LEAF work is not in the introduction | done | Section 1.4 (what LEAF requires and the three-step reason); also one paragraph before Definitions 4.6/4.7. |
| E2 | "round" is never defined | done | Section 1.3. |
| E3 | "two such lines with four stones of Maker each" | done | "two live lines, each with four stones of Maker, that meet in an empty cell" (Section 1.4). |
| E4 | Forward pointers to Figure 3 and Table 6 | done | Section 1.4. |
| E5 | Section 4.3 justifies only Algorithm 2: say so first; better move it to 7.3 and delete the five-step list | done differently | The first sentence of Section 4.3 and the opening of Section 4 say that the subsection is needed only for the verification and can be skipped. The five-step list is no longer in Section 4.3: it moved to Appendix S1.2 (it could not be deleted under decision D3), and a short paragraph with a pointer stays. Section 4.3 was not moved to Section 7.3: Lemma 4.12 is used in Section 4.5 (proof of Proposition 4.20) and Section 4 would be renumbered against the reports and against the three programs written from the first version. |
| E6 | Table 2 floats, covers Sections 2-4 only, is never cited | done | Extended by the notation of Section 5, moved to the beginning of Section 2, cited from Sections 1.8, 2 and 5. |
| E7 | Roadmap for Section 5 | done | Added, with the short route for R_max <= 10; Remark 5.15 ends with the direct reason why the forced reply restores admissibility with at most ten stones. |
| E8 | Worked example at the end of Section 5.2 | done | Example 5.3 (root, occupied cell, threat child, danger child on the real board); every cell, table entry and count was checked by program against `results/final/strategy.tbl` (`logs/paper/revM_checks/sec5_example.py`). |
| E9 | (i) explanation of the children 15 lines late; (ii) patterns hard to read; (iii) Remark 5.13 as a list; (iv) "would be void" | done | (i) the paragraph follows Definition 5.1 directly; (ii) "may skip" note and new Figure 4 with an E2 pattern of 11 and an E3 pattern of 13 stones, checked against the definition; (iii) list; (iv) "vacuous" (now in Remark S2.3). |
| E10 | The end of Section 5.10 (Proposition 5.19, Corollary 5.20, Remark 5.21) is not used | done | Second phase: moved to Appendix S2.1, where the statements now precede their proofs; one paragraph with pointers stays in Section 5.10. |
| E11 | Algorithm 1: a', b' are cells | done | p, q. |
| E12 | Algorithm 2, line 9: c for a line | done | T = {l_1, l_2, l_3}. |
| E13 | "strong" and "require" used before they are defined | done | "require <condition>" is explained before Algorithm 1; "strong" (lines and pairs) is defined before Algorithm 2 and in Section 4.3. |
| E14 | Proposition 7.2 is separated from its proof by the float | done | The proposition is kept with its proof; in the final PDF Proposition 7.3 and the beginning of its proof are on page 51. |
| - | Redundancies listed at the end of Section 3 | done, one item kept | The description of the leaf test at the end of Section 4.3 moved to S1.2; the transposed copy of Table 3 moved to S7.2; the checksums are printed once (Section 1.7); the repetitions of Sections 6.3 and 7.6 in Section 9.2 are cut; the trusted base in Section 9.3 is a pointer; old Appendix A.1 moved to S7.1. Kept: the restatement of validity in Section 7.1, so that Section 7 can be implemented on its own (the replication referee worked from it). |

### 4.4 Presentation (Section 4 of the report)

| # | finding | status | what was done, and where |
|---|---|---|---|
| title | Running head and README differ from the title | done | The three agree. |
| abstract | "Up to the symmetries of the board ..."; "a condition derived from Hall's theorem" | done | Both. |
| MSC, keywords | 91A24; 05C65 instead of 05C57; "strong draw" | done | All three. |
| T1 | Figure 2 never cited | done | Cited at the end of the proof of Proposition 4.22. |
| T2 | Table 2 never cited | done | See E6. |
| T3 | Tables 4 and 5 float into the list of Section 6.3 | done | Table 4 follows the list; Table 5 stands inside Section 6.4. Second phase: Table 8 was moved to its first mention in Section 7.6 so that it no longer floats into Section 7.7. |
| T4 | Table 10: 26 steps, eleven rows | done | Caption of Table S4. |
| T5 | Running times of treecheck differ without explanation | done | One reference timing in Section 8.3; the other runs are compared in Appendix S6.2; Table 7 quotes the CPU time of the stored run only. That the machine was loaded during the stored run is worded as an inference. |
| T6 | Table 7: the `--crosscheck` row | done | Indented; the table is grouped by headings. |
| N1 | "certificate" has four meanings | done | "Certificate" is the triple (Tbl, Lv, R_max) only. C, C_Phi, LEAF are the pairing condition, the surplus condition and the leaf condition (title of Section 4, definitions, Table 2, abstract, all sections); "witness" in Section 3.4, Remark 7.4, Section 9.2. Second phase: the last other use in the paper ("proof certificate" in Section 1.2) and two in `README.md` ("pairing certificate") were reworded. |
| N2 | K for a subgroup and for the busy cells | done | The subgroup is G_0. |
| N3 | sigma for a surplus value and for elements of a subgroup | done | j; u, u', v, v'. |
| N4 | Q (board, Q-states, Q_a, Q_b, Q_7) and P | done differently | The board is now written Omega (one macro), so that Q stands for Q-states and for the sets Q_a, Q_b only; the 7x7x7 board is Omega_7. Q_a, Q_b were not renamed: Definition 5.1, Algorithm 1, the three programs written from the paper and the five reports use these names, and Table 2 lists them. P-states, the maps P_r and the conditions (P1), (P2) are unchanged: different fonts, never in one statement. (The report lists N4 among the overloads but not among the required changes.) |
| N5 | Phi for the map A -> board | done | iota. |
| N6 | Labels (R1)-(R3) in Appendix C.1 | done | (C1)-(C3) (Appendix S1.3). |
| N7 | "line" for file lines and for lines of play | done | "entries" for the lines of the two files (defined in Appendix A.2); "Play A", "Play B". |
| N8 | l and ell; "id"; H | done | Lines are l everywhere (ell(f) is kept for the line of a parametrization); the id of a cell is introduced in Section 2.5; the set of the eager variant is Z. |
| style | Pronouns; "Maker stones"; spaced dashes; "upwards", "afterwards" | done | No "he/his" for the players anywhere; "stones of Maker" / "Maker's stones"; no spaced dashes in text; "upward". |
| typos | The nine rows of the table | done | All nine. The last sentence beginning with "But" (proof of Proposition 9.5) was reworded in the second phase. |
| LaTeX | Theorem 1.1 split across pages | done | In one piece on page 3. |
| LaTeX | File names broken inside a name | done | File names break after a slash or a comma only. |
| LaTeX | PDF metadata: author and keywords empty | done differently | Keywords are set; the author field stays empty until the authors are supplied (placeholder). |
| refs | Inconsistent author names; [15]; [4]; no DOIs | done | Initials throughout; [15] see P8; [4] "2nd ed."; DOIs are in `refs.bib`, the bibliography style does not print them. |

### 4.5 Reproducibility statement and declarations (Section 5 of the report)

| # | finding | status | what was done, and where |
|---|---|---|---|
| R1.1 | Replace the paragraph "Use of software tools" by a section "Declarations" with the proposed text | done | See D1. The bracketed parts of the referee's text are the two placeholders; the sentence on what the authors checked is not asserted. |
| R1.2 | Section 7.4, "Independence": say what independence means and what it does not exclude | done | Section 7.4, including the weaker sense in which the two re-expansions are independent (their sessions had read programs under review). |
| R1.3 | Section 1.6, "Trusted": programs written with AI-based tools | done | Added, with pointers. |
| R1.4 | First use of "internal review" | done | "two further programs written separately during a review of the project (Section 7.4)". |
| R1.5 | Placeholders for acknowledgments, funding, competing interests | done differently | One placeholder at the end of the Declarations for the author information as a whole (names and affiliations, acknowledgments, funding, competing interests), so that the paper has placeholders only for the five things listed in Section 0. |
| R2 | No licence, no persistent identifier; "project repository"; two verifiers not available; archive out of date | done | See D4 and audit F2, F12. |
| R2.1 | Add the listed files; delete the paragraph of Section 8.1; add a licence file | done differently | Files added, paragraph deleted. No licence file was added: the choice of the licence is the authors'; the paper and `README.md` carry the placeholder. |
| R2.2 | New text of "Data and code availability" | done differently | As proposed, with placeholders for the DOI/URL and for the licence only. No placeholder for the version and the checksum of the archive: the archive contains the paper, so its checksum cannot be printed in the paper; the paragraph says this and points to `MANIFEST.sha256`. |
| R2.3 | Section 1.7: point to "Data and code availability" | done | Done. |

### 4.6 Length (Section 6 of the report): the table of items that can move

Decision D3: one PDF, nothing deleted; the items move to the supplementary material and leave a pointer.

| item of the table | status | where it is now; what stays in the main part |
|---|---|---|
| Appendix C.1 (monotonicity of C in B) | done | Appendix S1.3. Pointers in Remark 4.5 (ii) and after Proposition 4.18. |
| Appendix C.2 with Proposition 5.19, Corollary 5.20, Remark 5.21 | done | Appendix S2.1 (statements, comparison of the variants, proofs). One paragraph stays in Section 5.10. |
| Appendix C.3 (stone numbers of E2/E3, Computed Fact C.4, Example C.5) | done | Appendix S2.2. Pointers after Proposition 5.11, in Remark 5.15 and in Remark 5.20 (ii). |
| Appendix B (integer program) | done | Appendix S5. One paragraph stays in Section 7.6. |
| Section 6.2 (how the search works) | done | Appendix S3.1 in full. Three paragraphs (about half a page) stay in Section 6.2. |
| Section 6.5 (two single-program observations) | done | Appendix S3.2 in full. Remark 6.2 stays. |
| Sections 7.5-7.6 (audits, mutations, cross-checks) | done | Appendix S4.2 and S4.3 in full. Summaries with Table 8 stay (a little more than one page). |
| Section 7.7, "Known minor issues" | done | Appendix S4.4 and `README.md`. Three sentences stay. |
| Section 8 (Tables 9, 10, command lines) | done | Tables 9 and 10 (now S3, S4), the complete outputs and the comparison of running times are in Appendix S6. Section 8 keeps what to run, what to expect and "Writing your own verifier" (two pages). |
| Appendix A.1, A.5, A.7, A.8 | done | Appendix S7.1-S7.4. Of old A.5 only the transposed copy of Table 3 moved; the totals and the counting conventions stay in Appendix A.4, and the per-round numbers stay in Table 3. The id of a cell is restated at the head of Appendix A. |
| Section 4.3: duplicate description of the leaf test | done | Appendix S1.2 (see E5). |
| Section 2.6 (full automorphism group) | done | Remark 2.9 stays; the Computed Fact and the remarks on the literature are in Appendix S1.1. |
| Section 9.2, "The shape of the certificate", "LEAF versus C" | done | The repetitions of Sections 6.3 and 7.6 are cut; every number that was cut is printed in Section 6.3 or 7.6. |
| in addition (not in the table) | - | The step-by-step description of the leaf test of the main verifier and its flow statistics ("Cost") moved from Section 7.3 to Appendix S4.1; a summary stays. Lemma 7.5, which justifies the variant, stays in Section 7.3. |

Result: main part 70 pages, supplementary material 21 pages. The referee estimated an article of about 55
pages from the 73 pages of the first version by moving about 18 pages. The first phase of the revision
added about 13 pages of material that the reports asked for (the lemma on distinct children, the worked
example, the figure of the patterns, the lemma on deleted lines, the defects of the input files, the
extended Table 7, the LOST test vectors, the context paragraph, the Declarations), almost all of it in the
main part, and the supplement now holds 21 pages.

### 4.7 The lists of Section 7 of the report

Required changes R1-R11: R1 see 4.5; R2 see 4.5 and D4; R3 see P1-P3; R4 see P4-P8 (P5 not done); R5 see
F1-F3; R6 see F4-F6; R7 see N1-N3, N5-N7, E11, E12; R8 see T1, T2, T4, T5; R9 see E1, E2, E7, E8; R10 see
4.6; R11 see "typos" and "LaTeX".

| optional suggestion | status | remark |
|---|---|---|
| Move Section 4.3 to Section 7.3; move the eager variant out of Section 5 | first: not done; second: done | See E5, E10. |
| Present the case R_max <= 10 directly and put LOST and the patterns into an appendix | not done | It would be a rewrite of Section 5, of Algorithm 1 and of the description of the verifiers, all of which were checked by the referees in their present form. The roadmap of Section 5 and the end of Remark 5.15 give the short route. |
| A figure of the patterns | done | Figure 4. |
| Zermelo; Dvorak-Valla; references on certifying algorithms and relevance-zone search | done / done differently | See P9, P10, P7. |
| MSC 91A24; keyword; abstract | done | |
| One convention for pronouns, "Maker's stones", l and ell, dashes | done | |
| One sentence in Section 1.6 on what must be wrong for the theorem to fail | done | |
| A remark after Theorem 1.1 that Breaker also wins when moving first | done | Section 1.3, as a statement quoted from the survey of Krivelevich, with "We do not use this"; it is not proved in the paper. |
| State the machine precisely in Section 8.2 | not done | The logs record only "4 cores, Xeon 2.1 GHz, 15 GB"; an operating-system version that no log of the runs records was not printed. |
| Float placement of Tables 4 and 5; second row of Table 7; Proposition 7.2 next to its proof; breaks in file names | done | See T3, T6, E14, "LaTeX". |

## 5. referee_replication.md (a verifier written from the paper alone)

| # | finding | status | what was done, and where |
|---|---|---|---|
| 1 | The flow counts of Section 7.3 cannot be reproduced from the text | done | The exact rules are printed (steps (1)-(4) in Appendix S4.1), the counts are called statistics of one implementation, and the numbers for Algorithm 2 as printed are given from the report (4,063,331 single-line gains, 692,694 triples). The three numbers were recounted from the printed rules with the matching code of the replication verifier, without deletion of lines: 869,185 / 77,268 / 7,391 (`logs/paper/flow_counts/`). |
| 2 | Table 11: B and F in one column | done | Table 9: B and F in separate columns, k in its own column; the caption says that the fourth row stands for 16 instances and that there are 26 instances. |
| 3 | No test vector for LOST; may x belong to F? | done | Appendix A.6 with Table 10: eight instances with t >= 9 (four true, four false), computed with `bin/treecheck --lostcheck` and cross-checked with two other implementations, which agree on verdict and kind of pattern (`logs/paper/lost_vectors/`, `run.sh` repeats it). A sentence after Definition 5.4 says that the definition does not require x outside F and that an implementation should evaluate it as it stands. |
| 4 | Which defects of a file must lead to rejection | done | Section 7.1, "Defects of the input files"; Appendix A.3 defines "well formed". |
| 5 | Byte-level format | done | First paragraph of Appendix A (ASCII, line feed after every line, no carriage returns, single spaces, decimal without leading zeros); checked on both files. |
| 6 | Push drops void children (a), (i) silently | done | Comment in line 25 of Algorithm 1; item "VOID" after the algorithm; a sentence after Definition 5.8. |
| 7 | Counting conventions; "Different data may define the same child" | done | That sentence was wrong for the states that matter. New Lemma 5.2 ("Different data define different children") with proof; one wording in the caption of Table 3, in the item "Counting" of Section 7.2 and in Appendix A.4. The replication verifier asserts the statement at all 46,412 expanded states. |
| 8 | K (two meanings) | done | See journal N2. |
| 9 | Lemma 2.3: order of the counts | done | See math A, finding 8. |
| 10 | Timings of the main verifier | done | See journal T5. |
| 11 | Artefacts of the text extraction | no change needed | The PDF is correct; `paper/paper.txt` was regenerated from the final PDF. |

## 6. Findings that were not applied, or were applied differently

1. **Journal P5** (read Hales-Jewett 1963 and Patashnik 1980): not done; the sources could not be obtained.
   The bounds are quoted through two secondary sources that were read. To be done by the authors.
2. **Journal, optional: precise machine description in Section 8.2**: not done; no log records it.
3. **Journal, optional: present R_max <= 10 directly, LOST in an appendix**: not done (rewrite of checked
   material); a reading guide instead.
4. **Journal E5, second half** (move Section 4.3 to Section 7.3): not done; Section 4.5 uses Lemma 4.12, and
   the numbering of Section 4 is that of the reports. The duplicate five-step list did leave Section 4.3.
5. **Journal N4** (rename Q_a, Q_b; the letter P): the board symbol was changed instead.
6. **Journal R1.5, R2.1, R2.2**: no licence file; no placeholder for the version and checksum of the
   archive; one placeholder for the author information instead of three.
7. **Journal P7, P8, P10**: one of the three optional references was not added; the proceedings version of
   the survey is cited with the theorem numbers of the preprint; the order of the automorphism group was
   not compared with the general result.
8. **Audit F13**: the usage text of `verify/treecheck.cpp` is unchanged; the paper and README say that the
   option is the default.
9. **Decision D2**: "written from the paper alone" is said of the replication verifier only (Section 0).
10. **Decision D3 against the referee's "delete"**: nothing was deleted; what the referee proposed to delete
    or to move to `README.md` is in the supplementary material (and, for the known issues and the commands,
    also in `README.md`).

Points on which the authors may want to decide differently: the symbol Omega for the board (one line of
`paper/macros.tex`); the stronger form of Proposition 4.22 (17 instead of 11 busy cells; the proof is the
referee's, re-derived and checked by program); the wording "internal refereeing" for the five reports.

## 7. Second phase: restructuring, consistency pass, build, archive

### 7.1 What moved, and what was left behind

Source files of the supplement: `supp_front.tex` (heading, overview, numbering), `supp_math.tex` (S1, S2;
inputs `app_extra_A.tex`, `app_extra_B.tex`), `supp_search.tex` (S3), `supp_checks.tex` (S4), `app_ip.tex`
(S5), `supp_repro.tex` (S6), `supp_tables.tex` (S7). The wrapper `app_extra.tex` no longer exists.

| moved material (first revision) | now | left in the main part |
|---|---|---|
| Section 2.6, Computed Fact 2.9 | S1.1 | Remark 2.9 |
| five-step list at the end of Section 4.3 | S1.2 | one paragraph in Section 4.3 |
| Appendix C.1 | S1.3 | pointers |
| end of Section 5.10; Appendix C.2 | S2.1 | one paragraph in Section 5.10 |
| Appendix C.3 | S2.2 | pointers |
| Section 6.2 from "The basic recursion" on | S3.1 | outline of the search and of the run in Section 6.2 |
| Section 6.5 | S3.2 | Remark 6.2 |
| "The leaf test of the main verifier", "Cost" (Section 7.3) | S4.1 | summary at the end of Section 7.3; Lemma 7.5 stays |
| Section 7.5 | S4.2 | summary in Section 7.5 |
| Section 7.6 without Table 8 | S4.3 | summary in Section 7.6 with Table 8 |
| "Known minor issues" (Section 7.7) | S4.4 | three sentences in Section 7.7 |
| Appendix B | S5 | one paragraph in Section 7.6 |
| Table 9, complete outputs, reference timings, Table 10 and the details of Section 8.4 | S6.1-S6.3 | commands, expected output and summaries in Sections 8.1, 8.3, 8.4 |
| Appendix A.1; the transposed table of A.5; A.8; A.9 | S7.1-S7.4 | id of a cell at the head of Appendix A; totals and conventions in A.4 |

Control: the sources of the first phase and the final sources were compared sentence by sentence. Of
3,392 sentences and fragments, 63 no longer occur verbatim. They are: sentences in which a pointer was
changed; headings; the opening paragraphs of Sections 6, 7, 8 and of the old Appendix C, which were
rewritten for the new structure; the organisation paragraph of Section 1.8; the two paragraphs of Section
9.2 from which the repeated numbers were cut (57 P-states; 246,299 states; 143,498 and 28,275 against
46,412 children; 60.8%; the single leaf with eight unrevealed stones; 160 of 656; 99.55%: all of them are
printed in Section 6.3 or 7.6); two placeholders (Section 0); and three captions or sentences reworded for
the layout.

### 7.2 Consistency pass

* Terminology: "certificate" in one sense (see N1); "entries" for the lines of the files; no "project
  repository"; "archive" throughout.
* Notation: the board is Omega in all files (macro); lines are l; no sentence begins with "And" or "But".
* Numbers: all thousands separators in place; no number of a computation was changed in the second phase.
  The only new numbers are the size and the number of files of the archive (measured) and page numbers.
* Convention on Computed Facts: the five Computed Facts (3.10, 4.23, 6.1, S1.1, S2.6) name their programs;
  the supplement repeats the convention on its first page.
* Cross-references: all through labels; statement numbers quoted outside the paper were updated
  (`README.md`, comments in `scripts/make_artifact.sh`, `scripts/check_7cube_midplanes.py` and its output,
  `logs/paper/flow_counts/`).
* Table of contents: the entry "Declarations" appeared twice and appears once; the supplement has its own
  entry.
* Layout: Table 8 at its first mention; captions of Tables 7 and 11 reworded (an underfull box; a last
  page with a single reference); a rule on the order of two start-up hooks removes a font-expansion notice
  of pdfTeX that was present in all earlier builds. The text of the PDF is not affected by the rule.

### 7.3 Build

`cd paper && latexmk -C && latexmk -pdf -interaction=nonstopmode paper.tex`: 91 pages; `paper.log` has no
warning, no overfull and no underfull box; `paper.blg` has no warning. `paper/paper.txt` is the output of
`pdftotext -layout`. Pages 1, 44, 49, 51, 54, 65 and 71 (title page, Table 3, both algorithms, Table 7,
the Declarations, the first page of the supplement) were rendered and inspected.

### 7.4 Archive

`scripts/make_artifact.sh` builds `ttt6-artifact.tar.gz` and `MANIFEST.sha256`: 314 files (the manifest
included), about 6.3 MB, about 38 MB unpacked. New in the list: `logs/paper/check_refs.py`,
`logs/paper/revision_M_for_C.tex`, `logs/paper/artifact_test_revision.log`, this document; the new source
files of the paper are taken by pattern. The script now also stops if the number of files printed in the
paper and in `README.md` is not the number of files of the archive.

Test of the unpacked archive in a fresh directory (`logs/paper/artifact_test_revision.log`; four idle
cores):

| test | result |
|---|---|
| `sha256sum -c MANIFEST.sha256` | all entries OK |
| `make bin/treecheck` | 11 s |
| the certifying command of Section 8.3 | exit code 0; `"ok":true`; 246,299 states (153,135 P-states, 93,164 Q-states, 106,723 leaves checked); 2.8 s of wall-clock time, 10.4 CPU-seconds; all fields except those for threads and timing equal to `results/final/treecheck.json` |
| `python3 verify/spotcheck.py --selftest` | `selftest OK`, 11 s |
| the verifier written from the paper alone, `logs/paper/replication_from_paper/verify.cpp` | compiles in 7 s; `ACCEPT (Rmax = 10)`; 246,299 states; every per-round number of Table 3 and every cell of Table 4 reproduced; 48 CPU-seconds. Its exit code is 1 because it also compares two flow statistics of the main verifier that were printed in the first version (replication finding 1); the same output is in the referee's own log |
| `logs/paper/lost_vectors/run.sh` | the three implementations agree on the 8 test vectors; 0 mismatches on the 800 audit instances |
| `logs/paper/check_refs.py`, `scripts/check_7cube_midplanes.py` | all checks passed |
| search of all text files and of the text of both PDFs for names of products, vendors and models | none found |

In three files of the referee material the absolute path of a temporary working directory is replaced by
`/tmp/scratch` while the archive is staged; `sha256sum -c MANIFEST.sha256` therefore succeeds in the
unpacked archive and fails for exactly these three files in the project tree.

## 8. Numbers of the first version and of the final version

Sections 1-4 and 9 keep their numbers, except that Computed Fact 2.9 is now Remark 2.9 (the Computed Fact
is S1.1). In Section 5 two statements were inserted (Lemma 5.2, Example 5.3):

| first version | final | | first version | final |
|---|---|---|---|---|
| Definition 5.2 (patterns, LOST) | 5.4 | | Lemma 5.12 (danger) | 5.14 |
| Definition 5.3 (WON) | 5.5 | | Remark 5.13 | 5.15 |
| Lemma 5.4 (rank) | 5.6 | | Theorem 5.14 (soundness) | 5.16 |
| Lemma 5.5 (symmetry) | 5.7 | | Theorem 5.15 (the strategy) | 5.17 |
| Definition 5.6 (certificate) | 5.8 | | Proposition 5.16 (strategy stealing) | 5.18 |
| Lemma 5.7 | 5.9 | | Corollary 5.17 (draw) | 5.19 |
| Definition 5.8 (sigma) | 5.10 | | Remark 5.18 | 5.20 |
| Proposition 5.9 (stones of a pattern) | 5.11 | | Proposition 5.19, Corollary 5.20, Remark 5.21 | S2.1, S2.2, S2.3 |
| Lemma 5.10 (one move) | 5.12 | | Lemma 5.11 (threat) | 5.13 |

| first version | final |
|---|---|
| Computed Fact 6.1 | 6.1 (new: Remark 6.2) |
| Proposition 7.1; Proposition 7.2; Remark 7.3 | 7.1 (new: Remark 7.2); 7.3; 7.4 (new: Lemma 7.5) |
| Tables 1-8 | 1-8 |
| Table 9 (archive), Table 10 (reproduction) | S3, S4 |
| Table 11 (LEAF vectors) | 9 (new: Table 10, LOST vectors) |
| Table 12 (integer program) | S2 |
| Table 13 (7x7x7 pairing) | 11 |
| Figures 1-3 | 1-3 (new: Figure 4, patterns) |
| Appendix A.1 | S7.1 |
| Appendix A.2, A.3, A.4, A.5, A.6 | A.1, A.2, A.3, A.4, A.5 (new: A.6, LOST vectors) |
| Appendix A.7, A.8 | S7.3, S7.4 |
| Appendix B, Proposition B.1 | S5, Proposition S5.1 |
| Appendix C.1, Proposition C.1 | S1.3, Proposition S1.2 |
| Appendix C.2, Lemma C.2 | S2.1, Lemma S2.4 |
| Appendix C.3, Proposition C.3, Computed Fact C.4, Example C.5 | S2.2, S2.5, S2.6, S2.7 |
| Appendix D | Appendix B |

## Second round

Two second-round reports were applied: `logs/paper/referee2_math.md` (checker X, findings X1-X10) and
`logs/paper/referee2_quality.md` (checker Y, findings Y1-Y25). One line per finding, added when the finding
was handled. "checked" names the script or command with which a new example, vector or number was checked;
the scripts are in `logs/paper/round2_checks/`.

| finding | status | what was done |
|---|---|---|
| X1 | done | Section 5.2, remark after Lemma 5.2: one example for each of the three failures at a VOID P-state (the interrupted first attempt had inserted the text; it was kept and the example for (b) was made precise: at most four cells of F on b and t >= \|F\|+3, so that the two data really define a child). Checked: `r2_children.py` (literal children of Definition 5.1 on the real board; (b) also at the threshold t = \|F\|+3 and below it). |
| X7 | done | Roadmap of Section 5: "Section 5.6 counts the stones of a pattern, Section 5.7 analyzes what one move of Maker can do ...". |
| X2 | done | Appendix A.6, Table 10: ninth vector (t = 10, \|B\| = 10, \|F\| = 9, F is B-admissible, LOST true, eight E2 patterns fit and each of them has x in F, no E3 pattern fits) and a caption sentence; "eight instances" -> "nine instances". Added to `logs/paper/lost_vectors/vectors.txt`; `run.sh` rerun (three implementations agree on 9 instances, among them `bin/treecheck --lostcheck`; 0 mismatches on the 800 audit instances). Checked independently: `r2_lost.cpp` (literal evaluation of Definition 5.4 that counts all fitting patterns), output `r2_lost.log`. |
| X8 | done | Caption of Table 10: LOST(B,F,t) is defined for arbitrary disjoint B, F and t >= 0; in the first two instances t, B, F are such arbitrary inputs and not states, to be read without the test \|B\| = t of Appendix A.3; in the other seven \|B\| = t and F is B-admissible (checked: `r2_lost.log`). The vectors themselves were not changed. |
| X10 | done | `logs/paper/lost_vectors/lost_driver.cpp`: comment now cites Definition 5.4 (Definition 5.2 in the first version, from which the included verifier was written). The verifier `logs/paper/replication_from_paper/verify.cpp` itself is a referee's program and keeps the numbering of the first version. |
| X3 | done | Section 7.3, list of properties validated behind a true verdict: "every line of T holds at least two cells" added (first alternative of the report; the program checks it, `verify/treecheck.cpp`, validateFlow). |
| X4 | done | Section 7.1: the sentence on errors that can only cause a rejection no longer names the computation of canonical forms; a non-minimal canonical form that is an image under G is harmless (Remark 7.2), anything else falls under (1) or (6). |
| X5 | done | Remark 7.2: the induction now first says that every processed state is the root or r(s') for a non-VOID child s' and hence not VOID (Lemma 5.7 (a)). |
| X9 | done | Section 7.3 now gives the same reason for the quick rejection as Appendix S4.1: it violates (L3) by definition. |
| Y2 | done | Section 6.4: the pointer for the leaf-table values is now Appendix S7.3 (ssec:fmt-orbits). |
| Y14 | done | Section 7.3, "Accelerations": the first of the two equivalent sentences deleted. |
| Y15 | done | Supplement front text: "...in another arrangement; Table S1 gives an overview." |
| Y19 | done | No sentence or item head begins with the small-caps label any more: Section 5.2 ("A VOID state is not examined further and counts as harmless"), Section 6.3 ("Children that are VOID."), Section 7.2 ("The VOID test."). Row labels of tables were left as they are. |
| Y12 | partly | Verbatim repetitions between main part and supplement removed, one side each: Section 6.2 (the announcement replaced by the pointer to S3.1); the statement on determinism is now made once, in Section 6.2 (Section 8.4 and S3.1 point to it; S6.3 keeps one clause); end of Section 7.3 (cost sentence replaced by a pointer to S4.1); S4.2 (introduction and the sentence on the consequence for the leaf test now point to Section 7.5); S1.2 (the two sentences on positive answers now point to Section 4.3 and Remark 7.4); S7.2 (conventions of counting now point to Appendix A.4, which a replicator needs). Where the repeated sentence carries an argument about trust or is needed by a replicator, the copy in the supplement was cut, not the one in the main part. No proof, definition, algorithm, file-format description or test vector was moved or cut, and no page target was pursued; the summaries of Sections 7.5 and 7.6 are not verbatim copies and were kept. The length of the main part is therefore essentially unchanged (see the final page counts below). |
| X6 | done | Section 4.5, text after Figure 2: "The number 17 is best possible", with the short justification of the report (every triangle contains a face diagonal). Checked: `r2_prop422.py` (1,464 triangles, each with a face diagonal; all 4,096 sets of three cells from each of the six classes have 18 cells, meet every face diagonal and leave no live triangle; the three facts used in the justification). The exhaustive statement of the report (no busy set of at most 17 cells works; exactly 4,096 sets of 18) is not printed, since it is the proposition itself and a count that the paper does not need. |
| Y1 | done | Declarations: the sentence now says exactly what Section 7.4 and the result files support: agreement of the numbers of states in every round among the independently written programs of Table 7, and of the numbers of children in every round among those that report the children by round (`verify/spotcheck.py` reports the children in total only; `results/final/spotcheck_full.json`). The self-check of the search (totals only) is not an independently written program and is not covered by the sentence. |
| Y3, Y17 | done | "preprint" added wherever the 7x7x7 result of the 2025 preprint, its computation, its idea or its Theorem 6.1 is used (Sections 1.3, 1.5 and 9.2). A refereed source for the degree form of the criterion was not substituted, because none was read. |
| Y4 | done | Reference [19]: page range moved into the note, so that the note is printed last. |
| Y5 | partly | Reference [15]: editors added (N. Creignou, D. Le Berre; from the DOI record of the volume, 10.1007/978-3-319-40970-2, retrieved for this purpose). The volume number of the series is still not printed: neither DOI record gives it, and a second bibliographic database did not answer. To be supplied by the authors. |
| Y6 | done | "proved by hand", "verified by hand", "checked by hand" replaced throughout by wording about the nature of the argument ("proved in the text, without a computer", "written proofs", "can be checked without a computer", "proved in the text, without a computation"): `sec_intro.tex` (four places), `sec_board.tex`, `sec_leaf.tex`, `sec_game.tex` (two places), `sec_discussion.tex` (Section 9.1 and Remark 9.4 (ii)). Who produced and who checked what is said in the Declarations only. (The sentence on Patashnik's "expert knowledge supplied by hand" in Section 1.4 describes that work and was kept.) |
| Y13 | done | Section 8.3: "The following two commands ... build the main verifier and run it; an accepting run establishes Computed Fact 6.1." |
| Y16 | done | Declarations: "first mentioned in Section 1.6"; the replication session "was given only the PDF of the paper, a text extraction of it and the two certificate files". |
| Y18 | done | "license" (American spelling) in Section 8.1, in the Declarations and in README.md; the placeholder is otherwise unchanged. |
| Y20 | done | Section 9.1: "a line of the 6x6x6 board". |
| Y7, Y25 | done | Section 8.5 now says precisely: the report's sentence holds except for two statistics of the main verifier (flows for pairs and for triples) that were printed in the first version without their rules; the program prints ACCEPT and reproduces all per-round counts; its exit code is 1 and its log ends with one reported mismatch because it also compares its own flow counts with these two implementation statistics. Table 7 (row "replication") points to this; README.md says the same. |
| Y8 | done | The archived copies of `logs/paper/referee/b_sigma.py` and `b_expand.py` (and of `b_play6.py`, which reads the file that `b_expand.py` writes) are rewritten while staging (`scripts/make_artifact.sh`, "runnable copies"): the certificate is read relative to the script and the reachable set goes to the system's temporary directory, so that `python3 logs/paper/referee/b_expand.py` runs from the root of the unpacked archive (tested there; see `logs/paper/artifact_test_final.log`). The files in the project tree are unchanged. For the other programs of the referees the caption of Table S3 and README.md ("Running the programs of the referees") now state plainly that they contain the absolute path of the project directory and how they must be run. |
| Y10 | done | Table 11 (Appendix B) is set with `[H]` (package float) and now stands before the reference list. |
| Y11 | not done (as before) | Two fetches of the article page of Hales-Jewett 1963 at the publisher's site were tried in this round; both were refused (HTTP 403, automated-access check). Patashnik 1980 is behind a paywall and was not tried. The wording with "see" through the secondary sources that were read (Erdos-Selfridge 1973, Krivelevich 2014; Allis 1994 for Patashnik) is kept. The two originals remain to be consulted by the authors before submission. |
| Y21 | done | `\needlines` before Proposition 4.13, before the heading of Section 5.6 and before Example S2.7; the last lines of all pages were scanned again after the final build (no statement head or heading with fewer than two lines of its text at a page bottom). |
| Y22 | done | Table S3: the caption is set like the other captions (short captions of long tables are now centred; this one became longer and is set as an indented paragraph), says "main contents; the complete list is in README.md", and the table now lists `referee2_*.md`, `referee2/`, `round2_checks/`, `related_work.md`, `issues.md`, `revision_M_for_C.tex`, `revM_checks/`, `check_refs.py` and the three test logs of the archive. |
| Y23 | done | Package upquote: straight quotes in the verbatim program output of S6.2; Section 8.3 says "prints one line of JSON on standard output". |
| Y24 | done | Role labels removed from the comments of `paper/paper.tex`, `macros.tex`, `sec_front.tex`, `sec_intro.tex`, `sec_game.tex`, `sec_discussion.tex`. README.md has a new paragraph "Words used in the project documents" (the task statement, the user, the role labels, the harness), and its file map now lists `revision_M_for_C.tex`. The project documents themselves (PROGRESS.md, REVIEW.md, SPEC.md, logs/) are archived unchanged. |
| Y9 | done | The reference running times of Section 8.3, Appendix S6.2 and README.md now cite `logs/paper/artifact_test_final.log`, the record of a test of the final build of the archive (written anew in this round and contained in the archive), and name the logs of the two earlier builds (`artifact_test.log`: first version; `artifact_test_revision.log`: revision) as such. Because running times differ from run to run by one or two seconds, Section 8.3 and README.md give round figures (compilation about 10 seconds; run less than 3 seconds of wall-clock time and about 10 CPU-seconds with four threads; self-test about 10 seconds), Appendix S6.2 states the ranges observed in the three logged tests (compilation 9 to 12 seconds; run 2 to 3 seconds and 9 to 11 CPU-seconds; self-test 9 to 12 seconds; exhaustive Python run 5 to 6.5 CPU-minutes), and the logs hold the exact times. |

### Second round: summary

* Applied: X1-X10 (all); Y1-Y4, Y6-Y10, Y13-Y25. Partly applied: Y5 (editors added; the volume number of
  the series could not be confirmed from the DOI records and is not printed), Y12 (verbatim repetitions
  removed, no page target). Not applied: Y11 (journal P5: the originals of Hales-Jewett 1963 and Patashnik
  1980 were again not accessible; they remain to be consulted by the authors; the text cites them through
  the secondary sources with "see").
* No statement or proof on which Theorem 1.1 depends was weakened or moved. The mathematical changes are
  additions (three examples after Lemma 5.2, the sharpness remark after Proposition 4.22, one clause in
  Remark 7.2, one property in the list after Lemma 7.5) and one corrected sentence in Section 7.1. Every
  new example and the new test vector were checked by program (`logs/paper/round2_checks/`,
  `logs/paper/lost_vectors/run.log`).
* Build: `latexmk -C` and a build from scratch; no undefined reference or citation, no duplicate label,
  no overfull or underfull box; `logs/paper/check_refs.py` and `scripts/check_7cube_midplanes.py` pass.
  The PDF has 92 pages: main part pp. 1-71, supplementary material pp. 72-92. The main part is one page
  longer than before this round (70): the additions asked for in this round outweigh the removed
  repetitions, and the table of Appendix B no longer floats into the reference list.
* Archive: 355 files (`MANIFEST.sha256` included), about 6.3 MB compressed and about 38 MB unpacked; it now
  also contains the two reports of the second round with the programs of the two checkers
  (`logs/paper/referee2*`), the checks of this round (`logs/paper/round2_checks/`) and the record of the
  test of the final build (`logs/paper/artifact_test_final.log`). Placeholders: unchanged (authors and
  affiliations; name and version of the tools; what the author(s) checked personally; license; DOI/URL).

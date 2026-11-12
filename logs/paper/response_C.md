# Response of reviser C to the referee reports

Scope of reviser C in this phase: `paper/sec_front.tex`, `sec_intro.tex`, `sec_certificate.tex`,
`sec_verification.tex`, `sec_repro.tex`, `app_formats.tex`, `app_ip.tex`, `refs.bib`, `paper/sec_discussion.tex`
except Section 9.1 and Proposition 9.5, `README.md`, `scripts/make_artifact.sh`, `MANIFEST.sha256`; lines appended
to `paper/macros.tex`. Line numbers below refer to the files as they are at the end of this phase; "v1" refers to
the refereed version in `logs/paper/paper_v1/`. Nothing was committed, pushed or uploaded. The archive in `dist/`
and `MANIFEST.sha256` were **not** rebuilt (this is done in the second phase, after the restructuring of decision
D3); `scripts/make_artifact.sh` was tested into a scratch directory with its new option `--out`.

State of the build: each of my files compiles with `paper/build_one.sh`; the whole paper builds (a scratch copy
of `paper/*.tex`, `refs.bib`; pdflatex, bibtex, pdflatex three times): 86 pages, no undefined reference or
citation, no BibTeX warning, no overfull box, no float too large. `paper/paper.pdf` itself was not rebuilt by me
(reviser M was editing at the same time).

Every number that I changed or added was re-read from the raw file named with it.

## 0. Coordinator decisions

| decision | what was done | where |
|---|---|---|
| D1 Declarations | new unnumbered section "Declarations" after Section 9, first paragraph "Use of AI-based tools": agents wrote all programs (search and all verifiers), ran the computations, drafted the proofs, searched the literature, drafted the manuscript, and carried out the internal review and the internal refereeing in separate sessions; definition of "independently written"; what it does not exclude and what stands against that. The two placeholders `[name and version of the tools, to be supplied by the authors]` and `[statement of what the author(s) checked personally, to be supplied by the authors]` are kept verbatim, each on one source line. Nothing says that the authors checked anything. Pointers from Sections 1.4, 1.6, 7.4, 7.7, 9.3. | `paper/sec_discussion.tex:333-362`, `paper/sec_discussion.tex:338`, `paper/sec_discussion.tex:361`; pointers `paper/sec_intro.tex:189`, `paper/sec_intro.tex:251`, `paper/sec_verification.tex:504-533`, `paper/sec_verification.tex:713` |
| D2 three verifiers written from the paper | reported in Computed Fact 6.1 (provenance), Section 1.6, Table 7 (three new rows under their own heading), Sections 7.4 (Independence, Agreement), 7.5, 7.7, 8.5 and Appendix A.6. Only what the reports say is stated: (i) `verify.cpp`, 706 lines, its author had only the PDF (and its text extraction) and the two certificate files, accept, 246,299 states, every entry of Tables 3 and 4, 42 CPU-s on one thread; (ii) `ref_audit.cpp` (474 lines in all), written from Sections 2, 4, 5, Algorithms 1, 2 and Appendix A, accept, 0 errors, every entry of Table 3, 63 CPU-s, LOST not evaluated (the program refuses t > 9; read in the source, line 263-264); (iii) `b_expand.py` (161 lines plus `b_sigma.py`, `b_geom.py`), Algorithm 1 only, **no leaf test**, every count of Appendix A.5, 3.4 min. **Deviation from the wording of D2, with reason:** D2 calls all three "written from the text of the paper alone, without access to ... any program". The reports support this only for (i). The audit referee read `verify/treecheck.cpp` and `spotcheck.py` for the audit (referee_audit.md, F1 and Section 2), and referee B worked inside the project tree. The paper therefore says "alone" only for (i) and says for (ii), (iii) that they were written from the text of the paper by referees who had the project files at their disposal and that they share no code with the project. (iii) is called a transcription of Algorithm 1, not a verifier. Because the five reports are internal (agent sessions), the paper speaks of the "internal refereeing of the first version" and the Declarations say who carried it out, so that no reader takes these programs for the work of journal referees. | `paper/sec_certificate.tex:34`, `paper/sec_intro.tex:237`, `paper/sec_verification.tex:471-493`, `paper/sec_verification.tex:522`, `paper/sec_repro.tex:231-244`, `paper/app_formats.tex:163` |
| D3 length | not part of this phase (second phase). Nothing was moved between files. To prepare it: the validity definition is still restated in 7.1, Table 3 is still repeated in A.5, "Known minor issues" is still in 7.7 (a copy is now also in `README.md`). | - |
| D4 availability | (a) `scripts/make_artifact.sh` packages every file cited in the paper (list in Section 5 below) and checks this automatically for every `\fn{...}` with a directory; (b) "project repository" no longer occurs in any file of the paper (my files and, checked by grep, those of reviser M); "artifact" is now "archive" in my files (the file name `ttt6-artifact.tar.gz` and the labels are unchanged); (c) the placeholders `[licence to be chosen by the authors]` and `[DOI / repository URL to be added]` are in "Data and code availability"; README keeps "Licence: to be chosen by the authors." and has the DOI placeholder. No licence text was added. | `paper/sec_discussion.tex:364-374`; `paper/sec_repro.tex:15`; `README.md:245` |

## 1. referee_audit.md

| # | finding | what was done | where |
|---|---|---|---|
| F1 | description of the leaf test of the main verifier and of its literal validation | Remark 7.4 ("witnesses", pair rule covered, main verifier validates the modified properties), new subsubsection "Deleting strong lines" with Lemma (Deleted lines) and proof (LaTeX of reviser M, C1-C2, integrated; `--paranoid` is called the default there), new subsubsection "The leaf test of the main verifier" that states steps (1)-(4) as read from `verify/treecheck.cpp:866-1073` (quick rejection by components; backward search; lines with \|l\\F\|-2 < theta and pairs with \|(a u b)\\F\|-4 < theta weak without a flow; theta = 1; deletion; pairs; triples; validation). Table 7, row 1 rewritten. Section 7.7 now names which implementation implements which variant. | `paper/sec_verification.tex:276`, `paper/sec_verification.tex:300-349`, `paper/sec_verification.tex:351-376`, `paper/sec_verification.tex:437`, `paper/sec_verification.tex:701-707` |
| F2 | Computed Facts 2.9, C.4 etc. rest on files that are not in the archive | all ten files, the two re-expansions with their outputs, and the logs behind printed numbers (`logs/repro/tc_all.log`, `logs/final/*.log`, `logs/search/*.log`, `NOTES.md`, `logs/paper/artifact_test.log`) are in the list of `make_artifact.sh`; Table 9 and README list them; the paragraph "Not included ... project repository" of Section 8.1 is deleted. The six places in Sections 2-5, C, 9.1 are in files of reviser M (done there; `grep -rn repository paper/*.tex` finds only the DOI placeholder). | `scripts/make_artifact.sh:84-118`, `paper/sec_repro.tex:74-92`, `paper/sec_repro.tex:95` |
| F3 | "12 are void" is wrong for 3 of 12 | "12 are degenerate (in 9 of them F is not B-admissible, in 3 of them \|F\| > k; the integer program is infeasible in both cases)"; "non-degenerate" for the 1,279, also in Appendix B.4. Recounted from `results/crosscheck/certify.jsonl` (12 records with decision void; 3 of them with k = 3 and \|F\| = 4). `certify.jsonl`, `instances_meta.jsonl` and the answers of the integer program are now in the archive. | `paper/sec_verification.tex:631-635`, `paper/app_ip.tex:137` |
| F4 | the flow counts are not "over all 106,723 leaves" | "Cost" rewritten: the counts are for the 9,179 leaves with theta >= 2; the 32,703 leaves with theta = 1 are not included; 8,549,521 searches in all (`results/final/treecheck.json`: `leaf_bfs`; `logs/repro/tc_all.log`). See also replication finding 1. | `paper/sec_verification.tex:385-394` |
| F5 | "same counters in every round" not supported for `spotcheck.py` and the self-check | Table 7: row of `spotcheck.py` "same states and VOID children in every round; children ... in total"; self-check "(totals only)"; caption defines the two phrases; "Agreement" rewritten program by program; caption of Table 3: "identical numbers from the programs of Table 7 that report them per round"; Computed Fact 6.1: "same numbers of states and of children in every round" only for the re-expansions and the two verifiers written from the paper. | `paper/sec_verification.tex:452`, `paper/sec_verification.tex:535-545`, `paper/sec_certificate.tex:163` |
| F6 | "about 90 CPU-seconds" holds for one re-expansion only | "run in 1.5 to 5 CPU-minutes (the unpruned leaf test of the first accounts for most of the difference)"; Table 7: "about 5 CPU-minutes" for re-expansion I (73 + 25 + 194 + part of 96 CPU-s, `logs/review/treecheck_audit.md` Section 7), "44 + 44 CPU-seconds" for II. | `paper/sec_repro.tex:226-227` |
| F7 | Computed Fact 4.23, `ip_regression.json` | file of reviser M (`sec_leaf.tex`). | - |
| F8 | description of the exact enumeration | "up to two reductions that do not change the answer (cells on no live line are skipped, and a set whose last stone lies on a cell that is unassigned in the current pairing is not examined)"; "examines 1.39 million sets"; new sentence: `enumcheck` and `certify` are built on the library of the search and are not independent of it. | `paper/sec_verification.tex:607-618` |
| F9 | "930,120 table entries" | "930,120 transposition-table entries" (`results/final/stats.json`: `tt_entries`). | `paper/sec_certificate.tex:124` |
| F10 | "in a preprint" | abstract: "a result first obtained in a recent preprint of Cain, Raymond and Kallersjo". Section 9.1 is in the part of reviser M. | `paper/sec_front.tex:26` |
| F11 | references [4], [17], [19] | [4] *Winning Ways*: cited only as "see also [4, Chapter 22]"; the pairing for n >= 8 is attributed to Golomb via Golomb-Hales p. 181 alone, also in Table 1. A generic bibliographic web query confirmed edition, volume, publisher, place and that Chapter 22 "Lines and Squares" is in Vol. 3 of the second edition; it also showed that the ISBN in `refs.bib` was wrong (ISBN-10 is 1-56881-143-8; the entry now has the ISBN-13 978-1-56881-143-7; check digit recomputed by hand). The chapter was still not read, so the content attribution stays with Golomb-Hales. [17] Schrijver and [19] Zermelo: checked with generic bibliographic web queries (publisher's page and catalogues: Springer 2003, Algorithms and Combinatorics 24, ISBN 978-3-540-44389-6; reference lists: Proc. Fifth ICM, vol. 2, Cambridge Univ. Press 1913, pp. 501-504). The comments in `refs.bib` now say `[WEB]` and say that neither source was read. Queries were titles and authors only. | `paper/sec_intro.tex:57`, `paper/sec_intro.tex:46`, `paper/refs.bib:85-97`, `paper/refs.bib:432`, `paper/refs.bib:471` |
| F12 | the archive predates the final sources | not rebuilt in this phase (see top). The size in Section 8.1 is now given by the macros `\ArchiveSize`, `\ArchiveUnpacked` (appended to `macros.tex`), set from a test build of the final file list into a scratch directory (304 files, 6,189,511 bytes, 38,184 KB unpacked, with the stale `paper.pdf` of 16:33); the script prints the numbers and a reminder. `make_artifact.sh` now refuses to package a `paper.pdf` that is older than the sources. **To do in phase 2:** rebuild, update the two macros, repeat the test of the unpacked archive (`logs/paper/artifact_test.log` still describes the old archive of 83 files). | `paper/macros.tex:77`, `scripts/make_artifact.sh:146` |
| F13 | `--paranoid` is the default | said in Computed Fact 6.1, in Section 7.3 (step (2)), in Section 8.3 and in README. The usage text of `verify/treecheck.cpp` was not changed (the file is not mine in this phase, and a change would alter the checksum of the main verifier that all logs refer to). | `paper/sec_certificate.tex:29`, `paper/sec_repro.tex:117`, `README.md:76` |
| F14 | README versus paper | README: title equal to the paper title; 12 s compile, 11 CPU-s / 3 s wall, self-test 12 s, 2 min 14 s (`logs/paper/artifact_test.log`: 12.2 s, 2.86 s / 10.6 CPU-s, 12.0 s, 2 min 13.7 s). Header of `refs.bib` and of `make_artifact.sh`: title corrected. Paper: one reference timing in Section 8.3, the other runs marked as loaded machine; Table 7 quotes only the CPU time of the stored run. | `README.md:1`, `README.md:75`, `paper/sec_repro.tex:132-142` |
| F15 | placeholders | authors: unchanged placeholder on the title page (the PDF author field stays empty for the same reason; keywords are now in the PDF metadata); URL, DOI, licence, version and checksum of the archive: marked placeholders in the Declarations; acknowledgments, funding, competing interests: placeholder. | `paper/sec_front.tex:11`, `paper/sec_discussion.tex:378` |
| F16 | Remark 9.4 (iii) | part of reviser M (Section 9.1). | - |
| F17 | "76 s for theta = 7" | "77 s for theta = 7 (four instances, between 37 and 133 s)"; recomputed from `results/final/checks/ip_leaf_theta_ge4_rest.shard*.jsonl`: 37.213, 48.230, 104.780, 132.946, median 76.505. | `paper/app_ip.tex:132` |
| Sect. 4 | claims the auditor could not verify | no change needed; the paper already says that the busy-only observation rests on one program. Its wording was weakened further (journal F4, F5). The repeat of the busy-only run by the audit referee (16.3 s, same counters) is now cited instead of the unlogged repeat. | `paper/sec_certificate.tex:295` |

## 2. referee_journal.md

| # | finding | what was done | where |
|---|---|---|---|
| R1 | declaration on AI-based tools | see D1. Also: first use of "internal review" in 1.6 reworded ("two further programs written separately during a review of the project"); Section 7.4 defines independence, says in which weaker sense the two re-expansions are independent (their sessions had read programs under review), and adds the paragraph on a common misreading; placeholders for acknowledgments, funding, competing interests. | `paper/sec_intro.tex:236`, `paper/sec_verification.tex:529` |
| R2 | availability | see D4. Section 1.7 points to "Data and code availability"; Section 8.1 says that every file named in the paper is in the archive; the two re-expansions and the programs of the Computed Facts are packaged. Licence, DOI/URL, version and checksum are marked placeholders. The archive is rebuilt in phase 2. | `paper/sec_intro.tex:259`, `paper/sec_repro.tex:18` |
| R3 / P1 | credit to [5] for the plan on 6x6x6 | Section 1.2: the plan of [5, Section 6.2] is quoted (opening on the four space diagonals; both sentences verbatim from the literature report, item 0.2); "The present paper carries out this plan." Section 1.5 repeats the credit. | `paper/sec_intro.tex:70-76`, `paper/sec_intro.tex:196` |
| R3 / P2, P3 | Golomb-Hales Theorem 3; "in a preprint" in 9.1 | part of reviser M (Section 9.1); abstract done (audit F10). | - |
| R4 / P6 | context on computer-assisted results | new paragraph at the end of Section 1.2: survey of solved games; Qubic (search plus expert knowledge, via Allis 1994 pp. 95-96); Go-Moku with threat-space and proof-number search; Uiterwijk's set matching; Hsu et al. for the (7,7,5)- and (8,8,5)-games (mentioned without any statement on their method, which was not verified). Nothing tagged UNVERIFIED in `related_work.md` is asserted (no Hex, no Mahalko, no Open Problem 3.2). | `paper/sec_intro.tex:78-92` |
| R4 / P7 | optional references | one sentence with two references whose records I retrieved from Crossref by DOI on 2026-11-12 (McConnell, Mehlhorn, Naher, Schweitzer, Comput. Sci. Rev. 5 (2011) 119-161; Heule, Kullmann, Marek, SAT 2016, pp. 228-245); texts not read, the sentence says only what the titles say. The relevance-zone reference was not added (not checked). | `paper/sec_intro.tex:86`, `paper/refs.bib:490` |
| R4 / P8 | published version of [15] | entry changed to the proceedings version (Proc. ICM Seoul 2014, Vol. IV, pp. 355-379, Kyung Moon Sa; found with a generic bibliographic web query, two concordant sources); the entry carries the note that theorem numbers are those of the preprint arXiv:1404.2731, because the proceedings version was not read and agreement of the numbering could not be checked. | `paper/refs.bib:215-223` |
| R4 / P4 | *Winning Ways* | see audit F11. | - |
| R4 / P5 | read Hales-Jewett [10] and Patashnik [16] | **not done**: both were unreachable for the literature report and I did not obtain them either. The paper continues to give the Hales-Jewett bounds through the two secondary sources that were read and says so by "see". This remains a task for the authors before submission. | `paper/sec_intro.tex:31` |
| R5 / F1 | one formulation of the computed statement | Computed Fact 6.1 is now "The certificate is valid": the two files form a valid certificate (Tbl, Lv, 10); "the root is WON" is its last sentence, by Lemma 5.9 (b) (`lem:certificate`). Section 1.3: "if a valid certificate exists, then both assertions hold (soundness theorem, strategy theorem, draw corollary) ... that the two certificate files form a valid certificate with R_max = 10 is Computed Fact 6.1". The proof in `sec_game.tex` is reviser M's. | `paper/sec_certificate.tex:19-26`, `paper/sec_intro.tex:109-116` |
| R5 / F2 | convention on Computed Facts | Section 1.6 as proposed ("Mathematical statements that we establish by running programs and state formally are called Computed Facts ... Sections 6 to 8 and the appendices also report computational results in running text and in tables, each with its source; none of these is used in the proof"). | `paper/sec_intro.tex:226` |
| R5 / F3 | "only statement" | "Apart from the two standard theorems quoted in Section 1.6, ...". | `paper/sec_certificate.tex:44` |
| R6 / F4 | 6.3 last bullet | "Section 6.5 reports a computation which indicates that non-busy table moves cannot be avoided for R_max <= 10." | `paper/sec_certificate.tex:201` |
| R6 / F5 | heading of 6.5 | "Busy cells alone seem not to suffice for R_max <= 10 (single program)"; the caveat names the three things the conclusion rests on: the search program in this mode; WON implies relaxed-WON (now with its one-line reason, induction on the rank, in 6.2); the pruning bound of the relaxed game, said to be unproved and applied 48 times in that run (`logs/search/busyonly_r10_rel24.log`: `potCut=48`). Section 9.3 "Busy cells" weakened in the same way. | `paper/sec_certificate.tex:284`, `paper/sec_certificate.tex:296-299`, `paper/sec_certificate.tex:95`, `paper/sec_discussion.tex:311` |
| R6 / F6 | determinism of the search | 6.2: "designed so that the move recorded for a state does not depend on the order in which the threads finish; we have not proved that the output is a deterministic function ...; what we observed is that a later run from scratch with two threads instead of four reproduced both files byte for byte" (`logs/final/reproduce_validation.log`: THREADS=2, "PASS tree identical to results/final"; `results/final/stats.json`: threads 4). 8.4 likewise. | `paper/sec_certificate.tex:126-130`, `paper/sec_repro.tex:209` |
| R7 / N1 | "certificate" in one sense | in my files "certificate" is only the triple; C is "the pairing condition", C_Phi "the surplus condition", LEAF "the leaf condition" (the three words agreed with reviser M); "witness" in Remark 7.4 and in 9.2. Abstract: "a condition derived from Hall's theorem", "positions that satisfy this condition". | `paper/sec_front.tex:22`, `paper/sec_intro.tex:148`, `paper/sec_certificate.tex:232`, `paper/app_ip.tex:4` |
| R7 / N7 | "line" | lines of the two files are "entries" (defined in A.3, used in 6.1, 7.1, 7.4, 7.5, 7.7, 8.5, A); "Line A/B" are "Play A/B"; "Two complete plays". "line" for the output line of a program was kept. | `paper/app_formats.tex:61`, `paper/sec_certificate.tex:384` |
| R7 / E11, E12, E13 | notation in the algorithms | Algorithm 1: the two left-out cells are p, q (as reviser M proposed); Algorithm 2: T = {l_1, l_2, l_3}; "require <condition>"; "strong" (lines and pairs) defined before Algorithm 2. | `paper/sec_verification.tex:91`, `paper/sec_verification.tex:229`, `paper/sec_verification.tex:67`, `paper/sec_verification.tex:198` |
| R7 / N2-N6, N8 | other clashes | in files of reviser M. | - |
| R8 / T2 | cite Table 2 | Section 1.8: "The notation is collected in Table 2." | `paper/sec_intro.tex:286` |
| R8 / T3 | floats of Tables 4, 5 | Table 4 now follows the list of 6.3, Table 5 stands inside 6.4 (`[ht]`). | `paper/sec_certificate.tex:222`, `paper/sec_certificate.tex:248` |
| R8 / T4 | Table 10 | caption: "The 26 steps of the script are grouped into eleven rows." | `paper/sec_repro.tex:205` |
| R8 / T5 | timings of treecheck | one reference timing (Section 8.3: 12 s, 3 s, 11 CPU-s, from the test of the unpacked archive); the stored run (three threads, 14.8 s wall for 8.2 CPU-s) and the run inside `reproduce.sh` are explained as runs on a loaded machine; the statement that the machine was loaded during the stored run is worded as an inference ("a wall-clock time above the CPU time indicates ..."), because no log records the load of that run. | `paper/sec_repro.tex:132` |
| R8 / T6 | Table 7 | grouped by headings (archive / internal review / written from the paper / not independent of the search); the `--crosscheck` row is indented; the table is now a `longtable` over two pages with the caption in the style of the other tables (a caption macro for longtables was appended to `macros.tex`). Table 9 is a `longtable` as well. | `paper/sec_verification.tex:409`, `paper/macros.tex:82` |
| R8 / T1 | Figure 2 | file of reviser M. | - |
| R9 / E1, E2 | idea of LEAF, definition of "round" | Section 1.4 states what LEAF requires and the three-step reason; Section 1.3 defines a round. Also E3 (danger pair reworded with "live" lines) and E4 (pointers to Figure 3 and Table 6). | `paper/sec_intro.tex:160-163`, `paper/sec_intro.tex:97`, `paper/sec_intro.tex:151`, `paper/sec_intro.tex:172` |
| R9 / E7, E8 | roadmap and example in Section 5 | reviser M. | - |
| R10 | length | second phase (D3). | - |
| R11 | typos, grammar | `sec_certificate` "the set T_8 of Example 3.9"; "Of the 93,164 table moves, 434 are ..."; 7.5: "In all, 66 ...", "Of these, 58 ...", "Eight ...", "Twelve ..."; A.4 "|B| = t-1 for a Q-state and |B| = t for a P-state"; [4] `edition = {2nd}`; "require <condition>"; sentences beginning with "And"/"But" in my files (1.4, 1.5, 9.2) joined or reworded; spaced em dashes removed (6.4, 7.5, A.3, A.4); "afterwards" removed. | `paper/sec_certificate.tex:196`, `paper/sec_certificate.tex:197`, `paper/sec_verification.tex:560`, `paper/app_formats.tex:111` |
| R11 | Theorem 1.1 must not break | `\needlines{7}` before the theorem (macro appended to `macros.tex`) and `\interlinepenalty=10000` inside it; in the test build the theorem stands in one piece on page 3. | `paper/sec_intro.tex:100`, `paper/macros.tex:73` |
| Sect. 4 | title, abstract, MSC, keywords | running head = title; abstract: "Up to the symmetries of the board, the table has 93,164 entries", "a condition derived from Hall's theorem"; MSC: 91A24 added, 05C65 instead of 05C57; keyword "strong draw". | `paper/sec_front.tex:2`, `paper/sec_front.tex:6` |
| Sect. 4 | pronouns, "Maker stones" | no "he/his" for the players in my files (abstract, 1, 6, 9.2-9.3, A); "stones of Maker" / "Maker's stones" throughout my files. | checked by grep |
| Sect. 4 | file names broken inside a name | `\UrlBreaks` restricted to slash and comma (appended to `macros.tex`); one paragraph of 7.4 reworded to avoid the only overfull box this caused. | `paper/macros.tex:88` |
| Sect. 4 | references | author names with initials throughout (`refs.bib`); [15] published version; [4] edition. DOIs are in the `.bib` file but `amsplain` does not print them. | `paper/refs.bib:10` |
| optional | one sentence on what must be wrong for the theorem to fail; remark that Breaker also wins moving first; trusted base in 9.3 as a pointer | done (the remark cites the survey [15], whose statement was read in the preprint; "We do not use this"). | `paper/sec_intro.tex:248`, `paper/sec_intro.tex:118`, `paper/sec_discussion.tex:324` |
| optional | machine stated precisely in 8.2 | **not done**: the logs of the project record only "4 cores, Xeon 2.1 GHz, 15 GB"; I did not want to print an operating-system version that no log of the runs records. | - |
| optional | Zermelo (P9) | Section 1.6 does not name Zermelo ("the determinacy of finite games with perfect information"); the two places with the citation are in files of reviser M. | `paper/sec_intro.tex:224` |

## 3. referee_replication.md

| # | finding | what was done | where |
|---|---|---|---|
| 1 | flow counts of Section 7.3 cannot be reproduced from the text | the exact rules are now printed (steps (1)-(4) of "The leaf test of the main verifier"), the counts are called statistics of this implementation, and the numbers for Algorithm 2 as printed are given from the replication report (4,063,331 single-line gains, 692,694 triples). In addition I recounted the three numbers from the printed rules with the matching code of the replication verifier, without deletion of lines: 869,185 / 77,268 / 7,391, i.e. all three are reproduced (`logs/paper/flow_counts/flowcount.cpp`, `run.sh`, `run.log`; 9,179 leaves with theta >= 2; all lines with at least theta+2 cells outside F are reaching). | `paper/sec_verification.tex:351`, `paper/sec_verification.tex:391` |
| 2 | Table 11 layout | B and F in separate columns, k in its own column; the caption says that the fourth row stands for 16 instances and that there are 26 instances. | `paper/app_formats.tex:173-199` |
| 3 | no LOST test vector | new Appendix A.7 with Table 12: eight instances with t >= 9 (four true, four false; E2, E3 only; with and without revealed stones), computed with `bin/treecheck --lostcheck` and cross-checked with two other implementations (function `lost` of `verify/spotcheck.py`; literal test of the replication verifier). All three agree on verdict and kind of pattern; treecheck and the literal test also agree on the 800 audit instances (0 mismatches). Everything is in `logs/paper/lost_vectors/` (`run.sh` repeats it; it also ran in the unpacked test archive). The sentence on x and F is in Definition 5.4 (reviser M) and is repeated in A.7. | `paper/app_formats.tex:216-252`, logs/paper/lost_vectors/run.log |
| 4 | which defects of a file must lead to rejection | new subsubsection "Defects of the input files" in 7.1 (soundness needs no test of the files; establishing Computed Fact 6.1 as stated needs rejection or report of malformed, non-canonical, repeated and unreached entries); A.4 defines "well formed" and points there. | `paper/sec_verification.tex:37-56`, `paper/app_formats.tex:109` |
| 5 | byte-level format | first paragraph of Appendix A (ASCII, LF after every line including the last, no CR, no empty lines, no tabs, single spaces, decimal without leading zeros); verified on both files by a script (0 CR, final LF, 0 non-ASCII bytes, character set = digits, comma, hyphen, space, LF). | `paper/app_formats.tex:6-10` |
| 6 | Push and VOID children of type (a)/(i) versus (R2) | comment in line 25 of Algorithm 1 ("never for a child (a) or (i)"); the item "VOID" explains why (Lemma 4.2) and says that an implementation should treat such a child as an internal error and reject. | `paper/sec_verification.tex:109`, `paper/sec_verification.tex:135` |
| 7 | counting conventions | one wording in the caption of Table 3, in the new item "Counting" of 7.2 and in A.5, with the new Lemma 5.2 of reviser M (`lem:children-distinct`). | `paper/sec_certificate.tex:166`, `paper/sec_verification.tex:139`, `paper/app_formats.tex:140` |
| 8, 9 | K, Lemma 2.3 | files of reviser M. | - |
| 10 | timings | see journal T5 / audit F14. | - |
| 11 | text extraction | none needed. | - |

## 4. referee_math_a.md and referee_math_b.md (items in `sec_verification.tex`, `sec_discussion.tex` 9.2)

The LaTeX of reviser M (`logs/paper/revision_M_for_C.tex`, blocks C1-C16) was integrated as follows.

| item | what was done | where |
|---|---|---|
| A, Finding 1 (C1-C4, C9) | lemma on deleted strong lines with proof; Remark 7.4; Table 7 row 1; "Accelerations" (fourth device); "Cost"; trust items of 7.7; sentence in 1.6. Two changes to M's text: "with the option `--paranoid` that was used in the certifying run" became a statement that this is the default (audit F13), and it was moved into the step list; "over all 106,723 leaves" in C4 was replaced by the correct range (audit F4). | `paper/sec_verification.tex:314`, `paper/sec_intro.tex:234` |
| A, Finding 2 (C5) | proof of Proposition 7.3: the pairing loop is argued directly (reachable set as a Hall violator). The proposition is kept with its proof by `\needlines` and `\nopagebreak` (journal E14). | `paper/sec_verification.tex:254-265` |
| A, Finding 4 (C8) | "strictly stronger condition than C ..., not only in the situation of Example 4.11". | `paper/sec_verification.tex:640` |
| A, Finding 5 (C11) | 9.2: "agree, for admissible F, when all stones are revealed, and when none is revealed and t <= 10". | `paper/sec_discussion.tex:284` |
| B, Finding 3 (C6) | the unproved sentence in 7.1 is replaced by a pointer; new Remark 7.2 (representatives that are not minimal) after Proposition 7.1; label `lem:rank` checked. | `paper/sec_verification.tex:62`, `paper/sec_verification.tex:171` |
| B, Finding 5 (C7) | Algorithm 1 and the comment "The children": cells p, q. | `paper/sec_verification.tex:123` |
| C12-C16 | Table 2 cited; counting convention; board written `\Q` only; terminology; Zermelo wording not needed in my files. | see above |

## 5. Archive (D4)

`scripts/make_artifact.sh` (rewritten list, new options `--out DIR`, `--dry-run`, `--allow-stale-pdf`):

* **added to the archive:** `logs/canon/autgroup.{cpp,log}`, `logs/core/test_core.log`, `logs/minstones/run.log`,
  `logs/paper/secA_check1.py`, `logs/paper/secA_checks.log`, `logs/proof/e23min.{cpp,log}`,
  `logs/proof/fuzz_cprime.py` with the module it imports (`smallgeo.py`) and its log, `scripts/check_7cube_midplanes.py`
  with its output; the two review re-expansions with their result files and the 800 LOST instances of the audit;
  `logs/paper/replication_from_paper/`; `logs/paper/referee/` and `logs/paper/referee/audit/` (text files only:
  executables and one pickled file are filtered out); the five referee reports; `logs/paper/response*.md` and
  the check scripts `logs/paper/revM_checks/` that the response of reviser M cites; `logs/paper/related_work.md`
  and `logs/paper/issues.md`, which the reports cite;
  `logs/paper/paper_v1/` (the refereed version, to which the reports' page and line numbers refer);
  `logs/paper/lost_vectors/`, `logs/paper/flow_counts/` (new evidence of this revision); `logs/final/*.log`,
  `logs/repro/*.log`, `logs/search/*.log`, `logs/search/NOTES.md`, `logs/paper/artifact_test.log` (logs behind
  printed running times and counters); `results/crosscheck/certify.jsonl`, `instances_meta.jsonl`,
  `ip.shard*of3.jsonl`, `ip_reused.jsonl` (evidence for audit F3); `paper/*.tex`, `refs.bib`, `paper.bbl`,
  `paper.pdf` (now required; the script stops if the PDF is older than the sources).
* **automatic check:** every argument of `\fn{...}` in `paper/*.tex` that contains a directory and no wildcard
  must be in the file list, otherwise the script stops. At the moment the check passes.
* **neutral paths:** three files of the referee material (`logs/paper/referee/b_expand.py`, `b_play6.py`,
  `logs/paper/referee_math_b.md`) contain the absolute path of a temporary working directory, part of which is
  the name of a software product. Since no product name may appear in the archive, the script replaces this
  path by `/tmp/scratch` in the staged copies (the files in the project tree are untouched) and stops if such
  a path is left. Consequence: `sha256sum -c MANIFEST.sha256` succeeds in the unpacked archive; from the project
  root it fails for exactly these files. README says so.
* **scan for names:** the unpacked test archive (all text files, and the text of both PDFs) was searched by hand
  for names of AI products, vendors and models: none found. The scan is not automated in the script, because a
  list of such names in the script would itself be a mention. It should be repeated on the final archive.
* **test:** `scripts/make_artifact.sh --out <scratch> --allow-stale-pdf`, final list: 304 files, 6,189,511 bytes,
  38 MB unpacked; `sha256sum -c`: 304 of 304 OK; name scan clean. In an earlier test of the same kind (292
  files) I also ran `make bin/treecheck` and the certifying command of Section 8.3 in the unpacked copy (exit 0,
  `"ok":true`) and `logs/paper/lost_vectors/run.sh` (all agree). `dist/` and the `MANIFEST.sha256` of the project
  root were not touched by these tests.
* `README.md`: title equal to the paper title; timings; `--paranoid`; directory map extended; byte-level format;
  LOST vectors; "Known minor issues of the tools" (copy of Section 7.7, for the second phase); availability
  placeholder; the licence line is unchanged. No licence text was added.
* `MANIFEST.sha256`: unchanged in this phase (it still describes the archive of 16:05); it is regenerated by the
  rebuild in phase 2.

## 6. Disagreements, deviations and open points

1. **D2, "written from the paper alone"** holds for the replication verifier only; see the row D2 above.
2. **"refereeing"**: the paper says "internal refereeing of the first version" and the Declarations say that
   it was carried out by agent sessions. I consider this necessary for honesty once programs written by
   "referees" are cited as evidence.
3. **Journal P5** (read Hales-Jewett 1963 and Patashnik 1980): not done; for the authors.
4. **Journal, machine description** (optional): not done, no record.
5. **Audit F13, usage text of treecheck**: not changed (file not mine; would change the verifier's checksum).
6. **`paper/sec_discussion.tex`**: I edited only "The shape of the certificate" (two phrases), "LEAF versus C",
   9.3 ("Busy cells", "Formal verification") and replaced the two unnumbered subsections at the end by the
   section "Declarations"; each edit was an exact string replacement on the file as re-read at that moment.
7. **Numbers that phase 2 must re-check after the restructuring:** `\ArchiveSize`, `\ArchiveUnpacked`
   (`paper/macros.tex`); statement, table and section numbers quoted outside the paper, namely in `README.md`
   (Computed Facts 2.9 and 6.1, Table 7, Sections 7.3 and 7.7), in the comments of `scripts/make_artifact.sh`
   (Sections 1.7, 6-8, 7.3, Table 7, Computed Facts 2.9, 3.10, 4.23, C.4, Remark 9.4, Appendix A, C.1) and in
   `logs/paper/flow_counts/flowcount.cpp`, `logs/paper/lost_vectors/run.sh` (Section 7.3, Appendix A); the
   sentence of Section 8.5 that the ambiguities "have been removed in the present version".
8. **Appended to `paper/macros.tex`** (nothing else was changed there): `\needlines`, `\ArchiveSize`,
   `\ArchiveUnpacked`, the caption format of longtables, `\UrlBreaks`.
9. **New files written by me:** `logs/paper/lost_vectors/` (8 files), `logs/paper/flow_counts/` (3 files),
   this document.

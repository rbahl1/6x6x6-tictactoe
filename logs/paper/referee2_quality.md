# Second-round report, checker Y: closure of the first-round findings and publication quality

Manuscript: "The 6x6x6 tic-tac-toe game is a draw", revised version, `paper/paper.pdf` (91 pages; main part
pp. 1-70, supplementary material pp. 71-91). Nothing under `paper/` and no project document was edited.
Own scripts: `logs/paper/referee2/` (`y_numbers.py`, `y_lost.py`, `y_paths.py`).

**Recommendation: minor revision.** No fatal finding. One finding is "major" on the scale of the first-round
audit (an unsupported factual claim, in the Declarations; one sentence). All others are minor or editorial.

## 1. What was checked, and how

* **First-round findings.** Every finding of the five reports was looked up in the revised text through
  `logs/paper/response_to_referees.md`: all fatal/major ones (math A 1; audit F1-F6; journal R1-R11) and
  nearly all minor/editorial ones (math A 2-9, math B 1-9, audit F7-F17, replication 1-10, journal F1-F6,
  P1-P10, E1-E14, T1-T6, N1-N8, the nine typo rows, the LaTeX items). New proofs were read for correctness:
  Lemma 7.5 (deleted lines) with the induction after it, proof of Proposition 7.3, Remark 7.2, Lemma 5.2,
  Proposition 4.22 (17 busy cells), Section 9.1 (normalized parametrizations, Lemma 9.1 (d)), Remark 5.15.
  No error found in them.
* **Reading.** Whole text of the PDF (`paper/paper.txt`, identical to `pdftotext -layout` of the PDF in the
  archive); abstract, Section 1, Section 9 and the Declarations word by word.
* **Numbers** (recounted from raw files): 9 + 3 degenerate instances and 656/623/160 (`results/crosscheck/
  certify.jsonl`, own admissibility test); Table 4, 434 non-busy moves, 54,482, 15,794/84,556/6,343/30, byte
  counts and SHA-256 (the two certificate files); every field of `treecheck.json`, `treecheck_crosscheck.json`,
  `spotcheck_full.json`, `stats.json`, `sim_mix_1M.json` that the paper quotes; flow counts 869,185 / 77,268 /
  7,391 over 9,179 leaves and 8,549,521 searches (`logs/repro/tc_all.log`, and `logs/paper/flow_counts/run.sh`
  rerun in the unpacked archive: same numbers, 0 non-reaching lines); timings (12 s / 3 s / 11 CPU-s / 12 s /
  2 min 14 s / 6.3 CPU-min: `logs/paper/artifact_test.log`; 11 s / 10.6 CPU-s: audit report; 8.2 CPU-s:
  `logs/final/treecheck.log`; 945 s, 339 and 211 CPU-s: JSON files); IP medians of S5.4; enumeration maximum
  1,394,130; Example 5.3 (122, 732, 104, 624, 650, 16,250: own count); column sums of Tables 3, 4, 8; line
  counts of all programs of Table 7; 11 programs, 26 steps. All agree with the paper.
* **LOST test vectors (Table 10).** Own literal implementation of Definition 5.4 (`y_lost.py`): all eight
  verdicts and pattern kinds as printed (E2 with 11 stones; "E3 only" with 13). `logs/paper/lost_vectors/run.sh`
  rerun: three implementations agree, 0 mismatches on the 800 audit instances.
* **Three programs written from the paper.** Table 7, Sections 1.6, 6.1, 7.4, 7.5, 8.5, A.5, A.6, S4.1, S4.2
  against `referee_replication.md`, `referee_audit.md`, `referee_math_b.md` and the logs: 706 / 474 / 161
  lines, 42 and 63 CPU-s, 3.4 min, 19,995 instances, 4,063,331 / 692,694, (L1) on all and (L2) on 1,500 leaves,
  "PDF and text extraction only" - the paper says nothing beyond the reports (one precision: finding 7).
* **Declarations and placeholders.** Exactly five kinds of placeholders, all bracketed (authors: title page,
  running head, Declarations; tools; what the author(s) checked; DOI/URL; licence). Search of both PDFs, all
  sources, README.md and all 314 files of the unpacked archive for product, vendor and model names, e-mail
  addresses and personal names: **no hit** (only cited authors and names in the literature report).
* **Archive.** Unpacked in a scratch directory: 314 files, 6,283,795 bytes, 38 MB; `sha256sum -c
  MANIFEST.sha256` all OK (313 entries); `make bin/treecheck` 9.4 s; the certifying command exactly as printed
  in Section 8.3: exit 0, `"ok":true`, 246,299 states, 2.7 s wall, 10.0 CPU-s, all fields equal to
  `results/final/treecheck.json` except `threads`, `states_per_thread`, `seconds`, `load_seconds`,
  `states_per_second`; `python3 verify/spotcheck.py --selftest`: `selftest OK`, 10 s;
  `scripts/check_7cube_midplanes.py`: all checks passed; every path printed in the sources (`\fn`, `\path`,
  `\texttt`) exists in the archive (binaries `bin/*` and `results/repro/` are generated); `paper/paper.pdf` in
  the archive is byte-identical to `paper/paper.pdf`; README agrees with the paper (title, statement numbers,
  commands, sizes, timings). `latexmk` log: no warning, no overfull/underfull box; `check_refs.py` passes.
* **Layout.** All 91 pages rendered at 50 dpi; looked at pp. 1, 2, 3, 7, 9, 25, 28, 31, 44-46, 49, 51, 53,
  54, 56, 65, 68-71, 87-89 (some again at 70-80 dpi); last lines of all pages scanned for stranded headings.
* **Language, bibliography.** Mechanical scans (sentence-initial numerals, "And/But", pronouns, doubled
  words, spaced dashes, BrE/AmE, hyphenation, rare-word list) plus reading; all 26 entries of `paper.bbl`.

## 2. Findings

| # | severity | location | problem | correction |
|---|---|---|---|---|
| 1 | major | Declarations, "Use of AI-based tools", par. 2, p. 65; `sec_discussion.tex:350-351` | "the agreement of the numbers of states and of children in every round among all verifiers" is not true of `verify/spotcheck.py`, which reports the children in total only (`spotcheck_full.json`). This is audit F5 (marked done); Section 7.4 and Table 7 are right, the new sentence is not. | "the agreement of the numbers of states in every round among all verifiers and of the numbers of children in every round among those that report them (\cref{ssec:impl})" |
| 2 | minor | Section 6.4, last par., p. 45; `sec_certificate.tex:222-223` | "its values serve as test vectors (Appendix A)": the leaf-table values moved to Appendix S7.3; Appendix A has none. Pointer left over from the restructuring. | `(\cref{ssec:fmt-orbits})` |
| 3 | minor | Section 9.2, p. 63; `sec_discussion.tex:230-232` | The degree form of the Erdos-Selfridge criterion is "quoted from [7, Theorem 6.1]", i.e. a classical theorem rests on the unrefereed preprint, which is not called a preprint here (audit F10, journal P3). | "(quoted from the preprint~\cite[Theorem~6.1]{CainRaymondKallersjo2025})"; better a refereed source ([9] or [5]) once read |
| 4 | minor | Reference [19], p. 70; `refs.bib:215-224` | The note is printed inside the entry, before the pages: "Seoul, 2014, Theorem numbers quoted here ... arXiv:1404.2731, pp. 355-379." | drop `pages`; `note = {pp.~355--379. Theorem numbers quoted here are those of the preprint arXiv:1404.2731}` |
| 5 | minor | Reference [15]; `refs.bib:503-513` | Incomplete against [2], [11]: no editors, no volume of the series. | add `editor = {Creignou, N. and Le Berre, D.}`, `volume = {9710}` (to be confirmed from the DOI record) |
| 6 | minor | Remark 9.4 (ii), `sec_discussion.tex:189-190`; also `sec_intro.tex:111, 213-215`, `sec_board.tex:7`, `sec_game.tex:13, 1035` | "Lemmas 9.1 and 9.2 are verified by hand", "proved by hand": read as an assertion that a person checked them; the Declarations say that the proofs were drafted by AI-based agents and leave open what the author(s) checked. | "are proved in the text, without a computation"; "\emph{Written proofs.}"; "has a written proof" |
| 7 | minor | Section 8.5, Table 7 row "replication"; `logs/paper/replication_from_paper/run.log` | The archived log of this verifier ends with "SOME CHECKS FAILED (1 mismatches)" and exit code 1 (comparison with two flow statistics of the first version). Explained only in `artifact_test_revision.log`, which paper and Table S3 do not mention. | add in 8.5: "(Its log ends with one reported mismatch and exit code~$1$: besides verifying the certificate the program compares its flow statistics with two statistics of the main verifier printed in the first version; see \cref{ssec:supp-mainleaf}.)"; same in README |
| 8 | minor | Table 7 row "transcription"; archive `logs/paper/referee/b_sigma.py:21`, `b_expand.py:159` | `b_expand.py` reads the certificate from the absolute path `/home/user/ttt6/results/final/` and writes to `/tmp/scratch/`; it cannot be run in the unpacked archive. Same for `audit/a*.py`, `b_checks.py`, `b_play7.py` (needs `b_seven_table.pkl`, not archived), `refA_leaves.py`. | say so in README ("unpack or link the archive at /home/user/ttt6 and create /tmp/scratch"), or rewrite the paths while staging |
| 9 | minor | Section 8.3, `sec_repro.tex:56-58`; S6.2, `supp_repro.tex:95-98` | Reference timing cites `logs/paper/artifact_test.log` as "a test of the unpacked archive"; that log is of the archive of the first version (83 files, 3.4 MB), not of the archive of Section 8.1. | "(\fn{logs/paper/artifact_test.log}, a test of the archive of the first version; the test of the present archive, \fn{logs/paper/artifact_test_revision.log}, gave $11$ seconds, $3$ seconds and $10$ CPU-seconds)" |
| 10 | minor | p. 69-70; `app_seven.tex:14` | Table 11 floats into the reference list (between [8] and [9]); p. 69 is left one fifth empty. | `\usepackage{float}` and `\begin{table}[H]` |
| 11 | minor | response, journal P5 ("not done") | Reason "could not be obtained" is weak for Hales-Jewett 1963 (Trans. AMS back volumes are freely accessible). Wording with "see" is acceptable; must be closed by the authors. | - |
| 12 | minor | response, journal R10 ("done") | Main part is 70 pages (first version 73; aim about 55). Summaries in the main part repeat the supplement verbatim: 6.2 / S3 (`sec_certificate.tex:55-60, 72-77`), 7.5 / S4.2, end of 4.3 / S1.2, end of 7.3 / S4.1, 8.4 / S6.3, A.4 / S7.2; the sentence on determinism four times. In 6.2 "We describe the search because ..." now announces a description that is in S3.1. | 6.2: "The search is described in \cref{ssec:supp-search-details} for readers who want to reproduce or extend the computation."; cut the repeats on one side |
| 13 | editorial | Section 8.3, `sec_repro.tex:32` | "The following two commands ... each establish Computed Fact 6.1": the two commands are `make` and the run. | "The following two commands, run in the unpacked archive, build the main verifier and run it; an accepting run establishes \cref{fact:H}." |
| 14 | editorial | Section 7.3, `sec_verification.tex:299-302` | Two consecutive sentences say the same ("uses a fourth device ..." / "uses one further device ..."). | delete the first |
| 15 | editorial | `supp_front.tex:29-30` | "(Table S1)" reads as if Table S1 repeated information of the main part. | "... in another arrangement; \cref{tab:supp-overview} gives an overview." |
| 16 | editorial | Declarations, `sec_discussion.tex:338-343` | "to which Sections 1.6, 6.1, 7.4 and 8.5 refer": also 7.5, 7.7, A.5, A.6, S3.2, S4.1, S4.2, S6.2; "given only the PDF" omits the text extraction named in 7.4. | "first mentioned in \cref{ssec:intro-nature}"; "was given only the PDF of the paper, a text extraction of it and the two certificate files" |
| 17 | editorial | Section 1.3, `sec_intro.tex:123-124` | "the result of [7]" without "preprint" where the result is used. | "the result of the preprint~\cite{CainRaymondKallersjo2025}" |
| 18 | editorial | `sec_repro.tex:19`, `sec_discussion.tex:367-368`, `README.md:258-260` | "licence" (British) in a text in American spelling. | "license" |
| 19 | editorial | `sec_game.tex:165`; `sec_certificate.tex:133`; `sec_verification.tex:133` | Sentences begin with the small-caps label "void" (lower case in print). | "A \VOID{} state is not examined further and counts as harmless." |
| 20 | editorial | Section 9.1, `sec_discussion.tex:76` | "a line of the 6x6x6 array" (elsewhere "board"). | "board" |
| 21 | editorial | pp. 19, 34; `sec_leaf.tex:286`, `sec_game.tex:575-577` | Head of Proposition 4.13 stranded at the page bottom; heading 5.6 with one line of Proposition 5.11 at the page bottom. | `\needlines{4}` before each |
| 22 | editorial | Table S3, p. 88; `supp_repro.tex:14 ff.` | Caption flush left, all other captions centred or indented; table omits `logs/paper/revM_checks/`, `check_refs.py`, `related_work.md`, `issues.md`, `artifact_test_revision.log`, `revision_M_for_C.tex`. | centre the caption; "main contents; complete list in README.md" |
| 23 | editorial | S6.2, `supp_repro.tex:109-113`; Section 8.3 | Verbatim output shows curly quotes in the Python dictionary; "prints one line of JSON" (a second line goes to standard error). | `\usepackage{upquote}`; "prints one line of JSON on standard output" |
| 24 | editorial | archive: `paper/paper.tex:29-34`, headers of `sec_front/intro/game/discussion.tex`, `macros.tex:1, 63, 68`; `PROGRESS.md:5, 25, 40`, `REVIEW.md:100, 340, 368, 418`, `SPEC.md:245`, `logs/spec_issues.md` | Internal role labels "(editor)", "(author A)", "(author B)", "reviser M/C" in the sources; "the task statement", "the user" in the documents (neither in the archive nor explained); `revision_M_for_C.tex` not in the README map. No name of a product, vendor, model or person. | remove the labels; one sentence in README |
| 25 | editorial | Section 8.5, `sec_repro.tex:117-119` | "reproduced every count without a correction" quotes the report, which excepts two flow statistics of the main verifier. | add "(except two flow statistics of the main verifier, \cref{ssec:supp-mainleaf})" |

## 3. Findings of the first round: status

Done and verified: all others, in particular math A 1 (Lemma 7.5, proof correct), audit F1-F4, F6-F17,
replication 1-10, math B 1-9, journal R1-R9, R11. Done with a remainder: audit F5 (finding 1), audit F10 /
journal P3 (findings 3, 17), journal P8 (finding 4), journal R10 (finding 12). Not done, reason weak: journal
P5 (finding 11). Not done, reason accepted: machine description, direct presentation of R_max <= 10, E5 second
half, N4, R1.5, R2.1, R2.2, P7, P10, audit F13.

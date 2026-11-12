# Referee report: audit of numbers, computations, programs, files and literature

Manuscript: "The 6x6x6 tic-tac-toe game is a draw" (`paper/paper.pdf`, 73 pages, built 2026-11-12 16:33 UTC;
text `paper/paper.txt`; sources `paper/*.tex`).
Assignment: verify every number and every factual claim about computations, programs, files and literature in
Sections 1, 6, 7, 8, 9, Appendices A, B and in every Computed Fact. Proofs are covered by other referees.
Nothing under `paper/` was edited; nothing was committed, pushed or uploaded.

All my scripts and their outputs are in `logs/paper/referee/audit/` (index in section 5 below). Everything
marked "recomputed" was computed by my own code from the raw files; my code shares nothing with the project
(written from the paper: Sections 2, 4, 5, Algorithms 1 and 2, Appendix A).

## 0. Recommendation

**Minor revision.** No fatal finding. Every headline number and every number of the certificate was
recomputed independently and is correct; the certifying command of Section 8.3 was rebuilt from the unpacked
archive and reproduces `results/final/treecheck.json` exactly (all fields except threads/timing). Six findings
are "major" on the scale of the assignment (a wrong or unsupported claim in the body); all of them are
corrected by changing a sentence or by adding small files to the archive, and none touches Theorem 1.1,
Computed Fact 6.1, or a number of Tables 3 to 6, 8, 11, 12.

Headline results of my own computations:

| what | result |
|---|---|
| SHA-256, bytes, lines of the two certificate files | as printed (Sections 1.7, 6.1): `13629393...9ce3fc`, 5,376,504 B, 93,164 lines; `3c14c781...0fd470`, 6,445,022 B, 106,723 lines |
| my own implementation of Algorithm 1 + Algorithm 2 (`ref_audit.cpp`, 63 CPU-s; brute-force canonical forms, exact string keys, literal children, LEAF literally as Algorithm 2 on every leaf) | **accept**, 0 errors; 246,299 states = 153,135 P (46,412 expanded, 106,723 leaves) + 93,164 Q; every per-round entry of Table 3 / A.5 (P, leaves, Q, threat, danger, void (ii)) identical; 0 void children (c)/(d); 54,482 children (ii), 151 void; every line of both files canonical, unique, reached |
| certifying command of Section 8.3 in the unpacked `dist/ttt6-artifact.tar.gz` | compile 11.0 s; exit 0; `ok:true`; all non-timing fields equal to `results/final/treecheck.json`; 5.6 s wall, 10.6 CPU-s (4 threads) |
| `python3 verify/spotcheck.py --selftest` | `selftest OK`, 10.6 s; 6,000 leaf-vs-brute instances, 1,200 LOST instances |

## 1. Findings

Severity scale of the assignment: fatal = a headline number or claim is wrong; major = a wrong number/claim in
the body or an unsupported claim; minor = rounding, wording, path typo; editorial.
"Impact" is my own addition.

### Major

**F1. What the main verifier validates for a "true" verdict is not what Remark 7.3 says, and the leaf test
of `treecheck` is not Algorithm 2 plus the listed accelerations.**
Location: Section 7.3, Remark 7.3 (p. 47-48; `sec_verification.tex:213-225`, "the main verifier does so"),
paragraph "Accelerations" (p. 48), Table 7 row 1 (p. 49; `sec_verification.tex:258-262`, "Algorithm 2 with both
pruning rules, every flow behind a true verdict validated literally (Remark 7.3)").
Claim: a true verdict is validated by checking three properties of the final assignment: no cell assigned
twice, every live line outside U holds exactly two cells, the lines of U hold 2|U| + theta cells.
Evidence: `verify/treecheck.cpp` 626-651 (comment), 782-812 (`validateFlow`), 954-987. A line found strong is
*deleted*: it gives up its two cells and joins a set D. All later flows (single lines, pairs, triples) are
computed and validated "relative to D": `validateFlow` requires that deleted lines own nothing
(`cnt[l] == 0`), other non-source lines own exactly 2, and the sources own 2|T| + q. This is a different
property, and it proves sur_F(S) >= theta only together with an induction over the order of deletion (S meets D:
S contains a strong line; otherwise the counting of Remark 7.3). That induction is in the source comment and
nowhere in the paper; Proposition 7.2 and Remark 7.3 do not cover it. Further accelerations that the paper
does not mention: for theta = 1 strong lines are found by one backward search from the free cells (with
`--paranoid` a flow of gain 1 is then built and validated for each of them); lines with |l \ F| - 2 < theta
and pairs with |(a u b) \ F| - 4 < theta are declared weak without a flow; the quick rejection by whole
F-components is always applied.
What is *not* affected: the obligations. I checked the code path: every F-connected triple of weak lines with
no strong F-adjacent pair is tested (path triples from their centre, triangles once from their smallest
line), which is (L3) by Propositions 4.13 and 4.15 and Corollary 4.17. My literal implementation of
Algorithm 2 agrees with `bin/treecheck --leafcheck` in verdict and failing condition on all 20,000 perturbed
instances (per instance), on all 106,723 leaves, on the 5,194 leaf-table instances and on the 1,288
cross-check instances with k >= |F|.
Corrected text (Accelerations / Remark 7.3): "The main verifier uses one more acceleration: a line found
strong gives up its two cells and is removed (set D); later flows are computed relative to D, and the
literal validation requires that the lines of D hold nothing, every other live line outside U holds exactly
two cells and the lines of U hold 2|U| + theta cells. This proves sur_F(S) >= theta for every S containing U: if S
meets D it contains a strong line, otherwise the counting above applies; the claim for the lines of D follows
by induction on the order in which they were removed." Table 7, row 1: "Algorithm 2 with both pruning rules
and removal of strong lines; every flow behind a true verdict validated literally (Remark 7.3, in the form
relative to the removed lines)".
Impact: none on the result (sound, and LEAF is recomputed for every leaf by four other implementations, one of
them mine); but Section 7.7 asks the reader to trust "an implementation of these two algorithms", so the
description must match the program.

**F2. Computed Facts 2.9 and C.4 are established only by programs that the reader cannot obtain.**
Location: Computed Fact 2.9 (p. 10), 3.10 (p. 13), 4.23 (p. 23-24), C.4 (p. 71), end of Appendix C.1 (p. 68),
Remark 9.4 (ii) (p. 57-58), Section 8.1 (p. 52; `sec_repro.tex:61-68`).
Claim: "Computed Facts ... name the programs that established them" (Section 1.6); the programs are "kept in
the directories logs/canon/, logs/core/, logs/minstones/, logs/paper/, logs/proof/ and scripts/ of the project
repository".
Evidence: all ten files exist in the project tree (`logs/canon/autgroup.{cpp,log}`, `logs/proof/e23min.{cpp,log}`,
`logs/core/test_core.log`, `logs/minstones/run.log`, `logs/paper/secA_check1.py`, `logs/paper/secA_checks.log`,
`logs/proof/fuzz_cprime.py`, `scripts/check_7cube_midplanes.py`) and say what the paper reports, but none is
in `dist/ttt6-artifact.tar.gz`, and the paper gives no location for a "project repository" (the only
availability statement is for the archive, with a placeholder URL). For Computed Facts 2.9 and C.4 the named
program is the *only* evidence. From the reader's side these two Computed Facts are unsupported.
I reproduced both independently: 1,152 automorphisms by backtracking over collinearity-preserving bijections
(`a10_aut_and_seven.py`); E2: 5,152,320 patterns (562,176 / 4,590,144 with 11 / 12 stones), E3: 353,422,080
(10,948,608 / 67,627,008 / 274,846,464 with 13 / 14 / 15), none with 12 (`a9_patterns.cpp`, literal
enumeration). The fuzz numbers of Appendix C.1 (5,574; 447,456; 244,718) are the sums of the four lines of
`logs/proof/fuzz_cprime_run1.log`.
Corrected text: add the ten files (46 kB) to the archive (e.g. under `extra/`), list them in Table 9, and
replace "of the project repository" by "of the artifact" in the five places.
Also absent from the archive, although printed numbers rest on them: `logs/repro/tc_all.log` (869,185 /
77,268 / 7,391 flows; 0.1 ms per leaf), `logs/final/*.log` (8.2 CPU-s of Table 7; 26 minutes of the
simulator), `logs/search/*.log` and `NOTES.md` (all numbers of Section 6.5), `logs/paper/artifact_test.log`
(12 s, 3 s, 2 min 14 s of Section 8.3), and the sources and outputs of the two re-expansions of Table 7
(`logs/review/treecheck_mut/aexpand.cpp`, `aleaf.cpp`, `aexpand_final.json`; `logs/review/lead/lead_expand.cpp`,
`lead_leaf.cpp`, `lead_expand.json`; 54 kB), which Computed Fact 6.1 cites as further evidence.
Impact: reproducibility of the non-load-bearing Computed Facts; Theorem 1.1 is not affected.

**F3. "12 are void (F is not B-admissible)" is wrong for 3 of the 12 instances.**
Location: Section 7.6, third bullet (p. 50; `sec_verification.tex:394`); repeated in Appendix B.4 ("1,279
non-void instances").
Evidence: `results/crosscheck/certify.jsonl`: of the 12 records with decision "void", 9 have an inadmissible
F and 3 have k = 3 < |F| = 4 (theta = -1) with a B-admissible F (my code: F admissible, a pairing for (B, F)
exists); `instances_meta.jsonl` labels the family "F not B-admissible or |F| > k".
Corrected text: "... and 12 are degenerate (in 9 of them F is not B-admissible, in 3 of them |F| > k; the
integer program is infeasible in both cases, Proposition B.1)". Use "non-degenerate" for the 1,279.
Impact: none on any count (656 / 623 / 12; 1,275 of 1,279; 652 / 623; 4 time-outs, all with F empty and LEAF
true; 160 holding instances with LEAF false were all recomputed and are correct).

**F4. The flow counts of Section 7.3 are not "over all 106,723 leaves".**
Location: Section 7.3, "Cost" (p. 48; `sec_verification.tex:240-241`).
Claim: "over all 106,723 leaves the main verifier computes 869,185 single-line flows, 77,268 flows for pairs
and only 7,391 flows for triples".
Evidence: the three numbers are the counters of `bin/treecheck --leafcheck` (`logs/repro/tc_all.log`;
reproduced by me). The counter of single-line flows is incremented only for theta >= 2
(`treecheck.cpp:961-975`). I ran the program on subsets: the 9,179 leaves with theta >= 2 alone give
869,185 / 77,268 / 7,391; the 32,703 leaves with theta = 1 give 0 / 0 / 0 although the (default, "paranoid")
verifier builds and validates one single-line flow of gain 1 for every reaching line there; the run on these
leaves makes 4,076,278 of the 8,549,521 augmenting-path searches of the whole run (pairing, the flows of
gain 1 and the cross-check of the non-reaching lines), none of which is counted as a single-line flow.
Corrected text: "over the 9,179 leaves with theta >= 2 the main verifier computes 869,185 single-line flows,
77,268 flows for pairs and only 7,391 flows for triples; for the 32,703 leaves with theta = 1 one augmentation
per line suffices (these are not included in the count). In all, the run makes 8,549,521 augmenting-path
searches."
Impact: descriptive only.

**F5. "Same counters in every round" is not supported for `verify/spotcheck.py` (nor for the self-check of
the search).**
Location: Table 7 and its caption (p. 49; `sec_verification.tex:294`), Section 7.4 "Agreement" (p. 48;
`sec_verification.tex:315`: "The first five report the same number of states in every round and for every type
of child"), caption of Table 3 (p. 40; `sec_certificate.tex:155-156`: "identical numbers from every verifier of
Table 7").
Evidence: `results/final/spotcheck_full.json` contains per-round numbers of expanded P-states, leaves and
Q-states and of void children (ii) (all equal to Table 3), but for the children of types (a), (c), (d), (i),
(ii) only totals (46,412 / 143,498 / 28,275 / 93,164 / 54,482); `spotcheck.py` has no per-round child counter.
`bin/search --selfcheck` reports totals only (the text of 7.4 says so, the caption of Table 3 does not).
Per-round threat and danger counts are reported by `treecheck` (both runs), by the two re-expansions
(`aexpand_final.json`, `lead_expand.json`: `by_t`) and now by my re-expansion.
Corrected text: Table 7, row 3: "accept; same states and void children in every round; children ... (totals)";
7.4: "The two treecheck runs and the two re-expansions report the same numbers in every round and for every
type of child; spotcheck.py reports the same states and void children in every round and the same totals of
children; the self-check of the search reports the same totals"; Table 3 caption: "identical numbers from the
programs of Table 7 that report them per round".
Impact: none (the numbers are right).

**F6. "about 90 CPU-seconds" holds for one of the two re-expansions only.**
Location: Section 8.5 (p. 55; `sec_repro.tex:183-184`).
Evidence: re-expansion II: 44 + 44 CPU-s (`REVIEW.md` 2.3) - correct. Re-expansion I: expansion 73 CPU-s,
leaf reference 25 CPU-s (theta = 0, 1) + 194 CPU-s (theta = 2, 3, unpruned) + part of 96 CPU-s (theta >= 4)
(`logs/review/treecheck_audit.md` section 7), i.e. about 300 CPU-s for a complete pass.
Corrected text: "and run in 1.5 to 5 CPU-minutes (the unpruned leaf test of the first accounts for most of the
difference)". For orientation: my own version (`ref_audit.cpp`, about 300 lines of C++ for the expansion and the
leaf test, Algorithm 2 with strong-line pruning only) needs 63 CPU-s.
Impact: none.

### Minor

**F7. Computed Fact 4.23, "The same values are recorded in ... ip_regression.json"** (p. 23;
`sec_leaf.tex:659-660`). `results/regression/ip_regression.json` records LEAF for (T8, -, 8), (T8, -, 9) and
(T7, -, 7) and the value worst(T7, -, 1) = 0, but not LEAF(T7, -, 1) and LEAF(T7, -, 2);
`logs/core/test_core.log` has all four. Corrected: "The same values are recorded in logs/core/test_core.log;
the regression file ip_regression.json contains those for T8."

**F8. Description of the exact enumeration** (Section 7.6, p. 50; `sec_verification.tex:377-381`).
"enumerates ... all B-admissible sets X containing F with |X| <= t" and "The largest single enumeration visits
1.39 million positions". `bin/enumcheck` (`src/match.hpp:309-400`) restricts the candidates to cells on live
lines and does not visit a set whose last cell is unassigned in the current pairing (both sound, argued in
the source); its counter for the largest leaf is 1,394,130. My literal enumeration (Remark 4.5 (iv), no
pruning) of that leaf visits 1,394,408 admissible sets; on a random sample of 362 leaves (300 / 60 / 2 with
theta = 1 / 2 / 3) it confirms "holds" everywhere, with 1 to 60,773 more sets per leaf than the program counts.
Also: `enumcheck` and `certify` are built on the library of the search (`src/match.hpp`, `src/phi.hpp`); the
paper should say so where it lists them as cross-checks (7.6, Table 8). Corrected: "... enumerates all
B-admissible sets X (up to two reductions that do not change the answer: cells on no live line are skipped,
and a last stone on an unassigned cell is not examined) ... The largest single enumeration examines 1.39
million sets."

**F9. "created 930,120 table entries"** (Section 6.2, p. 40; `sec_certificate.tex:119`). These are
transposition-table entries (`stats.json`: `tt_entries`); everywhere else "table entries" are the 93,164 lines
of the strategy table. Corrected: "930,120 transposition-table entries".

**F10. The 7x7x7 result is an unrefereed preprint; say so wherever it is called "first obtained/proved".**
Abstract (`sec_front.tex:22`: "a result first obtained by Cain, Raymond and Kallersjo") and Section 9.1
(`sec_discussion.tex:8`: "was first proved by"). Sections 1.2, Table 1 and the reference list say "preprint".
The literature report requires this wording; arXiv today (my query): only v1 (25 Sep 2025), no journal
reference. Corrected: "a result first obtained in a recent preprint of Cain, Raymond and Kallersjo".

**F11. Three reference entries rest on data that the literature report tags as not verified.**
[4] Winning Ways, 2nd ed., Vol. 3, Chapter 22 (cited twice for the pairing for n >= 8; `sec_intro.tex:46,57`):
chapter text not read, place of publication from memory; the only verified source for the attribution is
Golomb-Hales p. 181, who cite the 1982 edition, Vol. 2, pp. 667-679. [17] Schrijver and [19] Zermelo: `[MEM]`
in `refs.bib`. The three entries agree with what I know of these works, but I did not re-check them online
(web use was restricted to the three citations below). Either verify them or cite [4] as "see also".
Everything else agrees with `logs/paper/related_work.md`: every quotation is exact, every attribution is the
recommended one, "to our knowledge" is used for both novelty claims, Hales-Jewett is cited through the two
secondary sources that were read. Double-checked by me with generic bibliographic queries: arXiv:2509.21494
(title, three authors, v1 only, math.CO, no journal reference); Golomb-Hales, More Games of No Chance, MSRI
Publ. 42 (2002), pp. 167-182; Patashnik, Math. Mag. 53 (1980), no. 4, 202-216. All three entries are correct.

**F12. The archive in `dist/` predates the final sources; re-check Section 8.1 after rebuilding.**
`dist/ttt6-artifact.tar.gz` (16:05; 3,435,847 bytes; 83 files; 30.6 MB unpacked; `sha256sum -c
MANIFEST.sha256`: 82 of 82 OK) contains 14 of the 19 `.tex` files (no `sec_front`, `sec_intro`,
`sec_discussion`, `app_seven`, `app_extra`) and no `paper.pdf`. Its other contents are exactly those of
Table 9. With `paper.pdf` (981 kB, hardly compressible) the archive will be about 4.4 MB, not "about 3.5 MB"
(`sec_repro.tex:11`); the README and `logs/paper/artifact_test.log` numbers ("83 files") change as well.

**F13. `--paranoid`** is accepted by `treecheck` but is the default and is not in its usage text (only
`--fast`, which switches it off). Harmless; say "(the default)" in 6.1/8.3 or add the option to the usage text.

**F14. README versus paper.** README: compile "about 15 seconds", run "about 10 CPU-seconds", self-test
"about 15 seconds"; paper (8.3): 12 s, 11 CPU-s, 12 s (source `logs/paper/artifact_test.log`: 12.2 s, 10.6
CPU-s, 12.0 s; my runs: 11.0 s, 10.6 CPU-s, 10.6 s). README and the header of `refs.bib` call the paper
"6x6x6 tic-tac-toe is a draw"; the title is "The 6x6x6 tic-tac-toe game is a draw".

### Editorial

**F15.** Placeholders: authors (title page and running head), "[repository URL to be added]" (Sections 1.7 and
9.3; `sec_intro.tex:208`, `sec_discussion.tex:319`), licence in README.

**F16.** Remark 9.4 (iii) (`sec_discussion.tex:188`): Golomb and Hales prove (Theorem 3) that for even n
their Hypothesis 1 implies Hypothesis 2, by the same decomposition; with the known draws on 6^2 and 6^1 this
is the implication "6^3 draw => 7^3 draw" in the draw form. The paper credits the decomposition; one more
clause ("which is the argument of [9, Theorem 3] in the Maker-Breaker form") would exclude any reading as a
new implication.

**F17.** Appendix B.4: "76 s for theta = 7" is the median 76.505 s of four values (rounds to 77); "about 12.5 s
for theta = 4, 5" (medians 12.67 and 12.44) is fine.

## 2. Algorithm 1 and Algorithm 2 against the programs (obligations)

| obligation (Algorithm 1 / Definition 5.6) | `verify/treecheck.cpp` | `verify/spotcheck.py` | verdict |
|---|---|---|---|
| root = P(empty, empty, 0); every queued state canonical | 2127-2136, `--root` default; with `--crosscheck` brute-force minimum checked | traversal from the root, `canon` | same |
| listed leaf: require LEAF(B,F,t), recomputed | 2162-2180 | part B: LEAF recomputed for *every* line of the leaf file (a superset of the reached leaves; all are reached) | same |
| unlisted P: require t < Rmax | 2183 | `check_P` 779, also for children (814-818, 895-899) | same |
| require LOST false | 2188 | 784-787 | same (vacuous for Rmax = 10) |
| child (a) | 1317 | 792-798 | same |
| children (c): live l, e in l \ F, \|F u (l \ {e})\| <= t+1 | 1319-1331 (shortcut 5 - \|F n l\| > room is the identity of 7.2) | 821-832 | same |
| children (d): c not in B u F, unordered pairs of live lines through c, left-out cells not in F u {c}, \|F u Qa u Qb\| <= t+1 | 1335-1364 | 835-858 | same |
| void: only test for dropping a child, on the child's own sets | `pushChild` 2122 | `p_child` 803, `check_Q` 904 | same |
| Q: table entry defined, a cell, not in B u F | 2218-2225 | 875-881 | same |
| child (i) (void = error), child (ii) iff \|F\| < t | 2235-2247 | 884-914 | same |
| LEAF: (L1), (L2), every F-connected triple without strong line / strong F-adjacent pair has gain theta | 815-1073 (see F1 for the mechanism) | 292-386 (literal single / pair / triple augmentations on copies) | same obligations; description of treecheck incomplete (F1) |

My implementation of Algorithm 1 exactly as printed (lines 1-27, with the void test of line 25 as the only
filter) reproduces every counter of `treecheck.json`, so the pseudocode omits no obligation that the programs
check, and the programs check none that the pseudocode omits. Table look-ups in `treecheck` go through
120-bit hashes (Section 7.7 says so; my program and `spotcheck.py` use exact keys and use the same entries).

## 3. Claims verified as correct (compact)

Recomputed by my own code unless marked (file) = read off the cited result file, (log) = read off a log,
(doc) = compared with the cited document, (run) = I ran the project's program.

**Sections 1, 6.1, 8.3 (certificate).** SHA-256 of both files; byte and line counts; FNV-1a-64 of both files
as in `treecheck.json`; 93,164 distinct keys, 106,723 distinct leaves, all lines well formed and canonical.
**Table 3 / A.5.** all 11 x 7 entries and totals; 246,299 states; 57 P-states up to round 4; no void (c)/(d);
151 void (ii), all of them because m lies on a live line with four cells of F (classified, `a11_void.py`);
54,482 Q-states with |F| < t.
**Table 4 / A.5.** all 27 entries and totals; 60.8 % (60.756); 99.55 % (106,245); 478 leaves with theta >= 4;
1,912 with theta >= 3; unique theta = 8 leaf `8 0,5,61,93,122,154,210,215 -`.
**Section 6.3.** 434 non-busy table moves (16 / 399 / 11 / 8 in rounds 7-10), none in rounds 1-6; leaves with
0 / 1 / 2 / 3 non-busy Breaker stones: 15,794 / 84,556 / 6,343 / 30.
**Table 5, Section 6.4, A.7.** orbit numbers 527, 1,057, 1,669, 2,468 (Burnside, my group); the 1,057 + 1,669
+ 2,468 records of `phi_k*.jsonl` are distinct canonical busy sets (hence complete); slack fields correct;
slack >= k: 6 / 947 / 2,185; LEAF by my Algorithm 2: 6 / 172 / 0, record by record; O1..O6 canonical, orbit
sizes 288, 576, 288, 576, 576, 36 (sum 2,340), 100 live lines each, slack 8 = maximum for busy 8-sets; O6 =
canon(T8), four antipodal pairs, the others three; 83,904 sets in the 172 orbits.
**Table 6, Section 6.6.** all 2 + 5 + 14 entries; 35 entries in round 4; the six quoted entries; Line A (eight
moves, T8, leaf line as quoted) and Line B (first wanted cells, nine stones, revealed stones, leaf = last
line of the file, slack 9, theta = 0) followed in the table with my own canonical forms; the quoted moves are
among the moves allowed by the choice of the symmetry at every step.
**Section 6.2 (file/log).** 57.9 s, 601 MB, 49,397 / 96,487 / 48,937 / 97,478, 655,010 leaf tests, 25.7 CPU-s,
930,120 (F9), options of the run; 112 hash bits; candidate limits; byte-identical regeneration (log).
**Section 6.5.** (run) `bin/search --rmax 10 --maxcand 4 --relcand 24` rebuilt from the archive: FAILED,
12,437 relaxed P- and 14,196 relaxed Q-states, 16.3 s (logs: 21.8 s; repeat of 14.2 s recorded in
`logs/paper/issues.md` only); (run) phicheck on the final position returns the 14 lines of the face x = 5,
surplus 4, theta = 6; 14 lines, 36 cells, sur_F = 4 recomputed; Rmax = 11 with relcand 8 and both Rmax = 9
runs (128.1 s; double threat, replies in rounds 5 and 9, shared stone) (log).
**Section 7.3-7.5.** 0.1 ms per leaf (96.3 us, log; 75 us in my run); 211 CPU-s / 2 ms (file); Table 7: line
counts 4,012 / 1,496 / 450 + 227 / 367 + 307 / 4,383 (sim), 14.8 s and 8.2 CPU-s (log), 945 s, 2,153, 339
CPU-s, 44 + 44 CPU-s (doc); 66 = 58 + 8 mutations with the exact breakdown 10 / 10 / 4 / 6 / 5 / 2 / 2 / 3 /
1 / 9 / 5 / 1 and 1 / 2 / 3 + 1 / 1 (doc, row by row); 12 source mutants: 12 caught, 1 verdict, 7 counters, 4
neither, as listed (doc); 53,755 = 25,952 + 26,040 + 1,763; 5,970 = 3,781 + 1,929 + 260; at most 20 live
lines; 800 LOST states, t = 9..13 (doc); 6,000 (run); 20,000 perturbed instances: 13,688 / 590 / 4,135 /
1,587 recomputed with my Algorithm 2, per instance equal to `treecheck --leafcheck`.
**Section 7.6, Table 8.** enumeration files = exactly the leaves with theta <= 3 (64,841 + 32,703 + 7,267 +
1,434 = 106,245), all hold, maximum 1,394,130; 1,308 = 478 + 830 IP answers, all "holds", samples 300 / 300 /
150 / 80, all 478 leaves with theta >= 4 covered; leaf table: 6 hold, 262 = 254 + 8, agreement 268 of 268;
cross-check: 1,291 = 656 + 623 + 12, 1,275 of 1,279 (652 + 623), 4 time-outs (F empty, LEAF true), 623
witnesses re-checked both ways, 160 holding instances with LEAF false (recomputed); simulator (file): 10^6
games, nine adversaries, all five failure counters 0, 1,121,484 / 85,806 / 2,254,365, 117,835 / 832,519 /
49,646, 78,957 (84.8 %) and 81,907 (76.7 %), 14,207 and 24,816 not played (93.4 % and 98.8 % of them in
rounds 9-10), 26 minutes; `sim_snapshot_0806.cpp` identical to `sim/sim.cpp`.
**Section 7.7.** audit findings behind "Known minor issues" (doc); `void_children: 0` in `stats.json`.
**Section 8.** machine (4 cores, Xeon 2.1 GHz, 15 GB), g++ 13.3, CPython 3.13, HiGHS 1.16; (run) fresh build
of the eleven programs in the unpacked archive: 56 s; Table 10 from the time stamps of `logs/repro/`: 3.3
min, 1 s, 4 s, 1.6 min (search 59.4 s), 15 s, 8.5 min (327 CPU-s), 20 s, 40 s, 8 min, 22 s; 26 PASS lines,
about 23 minutes; 12 s / 3 s / 11 CPU-s / 12 s / 2 min 14 s / 6.3 CPU-min (log); every path, program and
option named in the paper and in README exists (94 paths checked; options of treecheck, spotcheck, search);
README entry points (run): phicheck, `treecheck --leafcheck`, enumcheck (199 nodes), `treecheck --selftest
--quick` (passes, 26 CPU-s).
**Section 9.** 127 cells in M, 45 lines in M, 148 lines mapped bijectively onto the lines of Q; Table 13
parsed from the PDF text: 45 distinct lines inside M, each with two of its own cells, 90 distinct cells, the
centre and 36 further cells unassigned; 148/64 = 2.3125, 109/32, 193/128, 155 >= 64; 109 lines, 13 lines
through the centre; 3.5 * 10^7 = 22,163,142 + 12,824,370 (doc).
**Appendix A.** busy ids; three orbits and representatives; 2,304 descriptions, 1,152 maps; four canonical-form
test vectors; quoted lines 1, 2,354, 9,791 of the table and 1, 8,185, last of the leaf file with their
coordinates and interpretation; Table 11: all LEAF values and failing conditions with my Algorithm 2 (26
instances), "worst" column (file); A.8: 49 < 51, 39 < 43, 19 < 23 by my own branch and bound, all four
pairings of `minstones_pairings.json` checked against Definition 3.1 (58 / 104 / 169 / 244 lines; 488 cells).
**Appendix B.** 616 variables, 1,605 constraints, 596 of type (14) for T8 (recomputed); gaps 0 / 0.99, target
0.999, one thread (source); Table 12 and B.3 (70 records pass; 45 instances, LEAF true on 21; 20 random sets,
worst 2..12; seven brute-force comparisons, 80 and 81); B.4 medians and maxima recomputed from the result
files (0.03 / 0.10 / 1.8 / 2.8 / 12.6 / 117 / 76 s; 52 s; 931 s; 19 s; 26-69 s; 155 s; 18-297 s; 97-323 s).
**Computed Facts.** 2.9: orbit sizes 4, 3, 2, 6, 4, 2 (log), order 1,152 (recomputed). 3.10: pairings (logs;
LEAF(T7,-,1) and LEAF(T8,-,8) recomputed and run). 4.23: true / false / true / false with witnesses of 100 and
104 lines, surplus 8 and 1 (run and recomputed). 6.1: recomputed in full. C.4: recomputed in full. Each
Computed Fact names its programs; none is used as a proved statement, and no proved statement of Sections
6-9 or of the appendices rests silently on a computation (the computed statements of 6.4, 6.5, A.7, A.8 and
9.3 are labelled as such).

## 4. Claims I could not verify

1. Section 6.5, "within the relaxed game with busy table moves the result is exhaustive" (and its consequence
   in 9.3). I reproduced the run and its counters with the project's program, but an independent proof needs
   a second program: my naive memoised evaluation of the relaxed busy-only game did not terminate within 280
   CPU-s even for Rmax = 8. The paper already says that this rests on one program.
2. The two Rmax = 9 search runs and the Rmax = 11 busy-only run: logs read, not re-run.
3. All HiGHS computations (about 3 core-hours on the leaves, 1.3 hours on the cross-check, the leaf table and
   the regression values): result files recounted, nothing re-solved.
4. The 10^6 simulated games and the 4,000-game run: result files only.
5. The mutation experiments and brute-force comparisons of `logs/review/treecheck_audit.md` and the runs of
   the two review re-expansions: reports and their JSON outputs read, nothing re-run (my own re-expansion is
   a further independent one).
6. `treecheck --crosscheck` (945 s) and the exhaustive `spotcheck.py` run (about 6 CPU-minutes): outputs
   read, not re-run (outside the CPU budget for the paper's commands).
7. The repeat of the busy-only run "for this paper" (14.2 s): recorded in `logs/paper/issues.md`, no log file.
8. Literature beyond the three citations checked online: contents of Hales-Jewett 1963, Winning Ways ch. 22,
   Beck beyond p. 14, and the numbers quoted from Cain et al. rest on `logs/paper/related_work.md`.
9. Exact enumeration of C on all 106,245 leaves: I re-enumerated a sample of 363 leaves only (about 2
   CPU-minutes); the full literal enumeration would take hours with my unoptimised code.

## 5. Files of this audit (`logs/paper/referee/audit/`)

| file | content |
|---|---|
| `ref_audit.cpp`, `expand.out` | my Algorithm 1 + 2: full re-expansion (modes `expand`, `leaf`, `canon`, `enum`, `geom`, `busyonly`) and its output |
| `a1_filestats.py/.out` | checksums, line counts, Tables 3 (Q column), 4, 6, non-busy moves and stones, quoted lines |
| `a3_lines_of_play.py/.out` | Lines A and B, T7, T8, O1..O6, IP size for T8, face obstruction |
| `a4_checks.py/.out` | `results/final/checks`: enumeration and integer program on the leaves, Table 8, B.4 timings |
| `enum_sample.*`, `enum_max.txt` | literal enumeration of C on 362 sampled leaves and on the largest one |
| `crosscheck_leaf.out` | my LEAF on the 1,291 cross-check instances |
| `a6_leaftable.py/.out`, `lt_*` | Table 5: Burnside, canonicity, slack, LEAF, orbit sizes |
| `perturbed_leaf.out` | my LEAF on the 20,000 perturbed instances |
| `a8_minstones.cpp` | maximum number of lines killed by j stones, n = 5, 6, 7 |
| `a9_patterns.cpp/.out` | Computed Fact C.4 |
| `a10_aut_and_seven.py/.out` | Computed Fact 2.9; Lemma 9.1, Table 13; Erdos-Selfridge sums |
| `a11_void.py/.out` | classification of the 151 void children |
| `a12_fnv.c/.out` | FNV-1a-64 of the certificate files |
| `artifact_files.txt`, `paper_paths.txt` | file list of the archive; paths named in the paper |

Commands of the paper that I ran (in a scratch copy of the unpacked archive, about 2.6 CPU-minutes in all):
`make bin/treecheck`; the certifying command of 8.3; `python3 verify/spotcheck.py --selftest`; a fresh build
of all eleven programs; the README entry points; `bin/treecheck --selftest --quick`; `bin/treecheck
--leafcheck` on the leaves; the busy-only search of Section 6.5.

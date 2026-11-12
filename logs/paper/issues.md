
## Author B (paper/sec_game.tex, paper/app_extra_B.tex)

### B1. Findings about PROOF.md while porting (no false statement; one minor gap; one imprecision)

1. **Proposition 0.3 (strategy stealing), minor gap for boards with an odd number of cells.** PROOF.md lets
   the virtual play run until the virtual second player completes a winning set and concludes "the real FP
   owns W no later than that moment". This presupposes that the real play lasts that long. On a board with
   an odd number of cells the real board can become full after a move of the real first player while the
   ghost stone is still on the board; then the virtual play is unfinished (one empty cell, the ghost cell,
   virtual first player to move). The clause "if there is no real empty cell, the real board is full" in
   PROOF.md does not cover this, and it is in fact vacuous (when the first player is to move in an
   unfinished real game there is always a real empty cell). The statement is true; the paper's proof
   (prop:stealing) treats the three possible endings of the real play separately and closes the case.
   Irrelevant for the 216-cell board (even), so Corollary 0.4 and the main theorem are not affected.
2. **Definition 8.1, reachable set.** The closure rule for Q-states uses the children via m = Tbl(Q) without
   requiring m not in B u F; for an invalid certificate these "children" need not be states. The paper adds
   the proviso to rule (R2) of def:certificate, so that V is defined for every certificate; for valid
   certificates nothing changes ((V3)). sec_verification.tex states the closure rule without the proviso,
   which is equivalent for the purpose there (the verifier rejects at such a Q-state).
3. Everything else in PROOF.md sections 0.2-0.4 and 8 (8.1-8.12, 8.4a, 8.4b) was re-derived and ported
   without change of content.

### B2. Additions that go beyond PROOF.md (all proved in the text; checked by scripts of my own)

1. **rem:one-event** (Section 5): with |X'| <= 10 a move of Maker from an admissible position creates at most
   one forced event (one threat, or one danger pair, or nothing). This is the observation of REVIEW.md
   section 2.1(c); PROOF.md does not state it. Proof in the remark (counts 11, 11, 11, 12). Fuzz-checked
   (750,000 moves, own geometry): no counterexample.
2. **ex:conservative** (Appendix C, my part): a hand-checkable position (13 + 13 stones, all in the plane
   z = 0) in which an E3 pattern fits although one of the three replies restores admissibility. PROOF.md
   Remark 8.8(iii) had this only as COMPUTED (622 observations of fuzz_l4.py); the paper now proves the
   existence claim by the example and no longer cites the computation. In this position the tie-break
   "smallest id" of sigma picks a losing reply, which illustrates that the choice matters only at LOST states.
3. **prop:e23-min** (Appendix C, my part): hand proof that on this board any four pairwise meeting lines are
   concurrent, hence every E3 pattern has at least 13 stones (PROOF.md Remark 8.4b gives a sketch only),
   with explicit patterns attaining 13 (E3) and 11 (E2). The computed histogram of e23min.cpp is kept as
   Computed Fact fact:e23 (I re-ran the program: identical output). Own check: all 984 quadruples of
   pairwise meeting lines are concurrent; the 13 line directions span 9 planes with four directions.
4. **lem:game-invariance** (Section 5): invariance of VOID, LOST, the children and WON under G, and of
   canonical forms; this is the second half of PROOF Lemma 7.3 / 7.4, which has to live in Section 5 because
   the notions are defined there. sec_verification.tex ("Verifiers work with canonical forms ...") could cite
   it instead of lem:group, lem:canon.
5. thm:sigma has an extra part (c): Breaker makes at most R_max moves in TABLE mode.

### B3. Labels defined in my files (for citation by others)

Fixed labels of OUTLINE: def:states, def:won, def:lost, prop:lost-stones, def:sigma, lem:one-move, lem:threat,
lem:danger, thm:soundness, def:certificate, thm:main-proof, prop:stealing, cor:draw.
Further labels: sec:game, sec:lazy, sec:states, sec:won, sec:cert, sec:sigma, sec:stones, sec:one-move,
sec:soundness, sec:game-remarks (subsections); fig:round; def:children (alias of def:states: the children
(a), (c), (d), (i), (ii) are part of Definition def:states, as sec_verification.tex assumes); lem:rank
(well-foundedness, rank function rk); lem:game-invariance; lem:certificate ((a) no state of V is VOID, (b)
valid certificate => every state of V is WON, (c) root WON => a valid certificate exists); thm:sigma (the
precise form of the first assertion of thm:main); rem:one-event; rem:conservative; prop:lazy-monotone;
cor:eager; rem:eager; in the appendix: sec:extra-lazy, sec:extra-e23, lem:lost-antitone, prop:e23-min,
fact:e23, ex:conservative. Items of lists: (S1)-(S6) of def:sigma (labels S:leaf, S:maker, S:pairing,
S:threat, S:danger, S:table), (I1)-(I4) of thm:soundness (I:V, I:B, I:F, I:adm), (R0)-(R2), (V1)-(V3),
(P1), (P2), (E2.1)-(E2.3), (E3.1)-(E3.3).

**thm:main-proof** is the label of the subsection "Proof of Theorem thm:main" (5.9), so \ref{thm:main-proof}
prints the subsection number; the proof environment itself is headed "Proof of Theorem \ref{thm:main}".
The proof assumes that thm:main reads: if the root is WON for some R_max (Computed Fact fact:H for
R_max = 10), then Breaker, moving second, has a strategy that always prescribes an empty cell and under
which Maker never owns all six cells of a line; consequently the game is a draw.

### B4. Macros and notation

* Local \providecommand at the top of both files: \Etwo (\mathrm{E2}), \Ethree (\mathrm{E3}), \TABLE
  (\textsc{table}), \PAIRING (\textsc{pairing}), \WONp (\WON^{+}), \rk (\operatorname{rk}, the rank; not
  \rho, which Section 2 uses for the reversal). Candidates for macros.tex; nothing else is needed.
* Modes are typeset in small caps like \WON and \VOID ("table mode", "pairing mode"); other sections write
  about "the table" in plain words, which is consistent.
* Lines are called l, a, b, a', b' (as in Sections 3 and 4); images under g are written gB, gF (as in
  Section 2); the stone sets of a danger child are Q_a, Q_b (as in SPEC and sec_verification.tex), those of
  the patterns are S_l, S_a, S_b, ... . The triple (B,F,t) kept by Breaker is called his "record".
* References are written "Lemma~\ref{...}" etc., not \cref, because cleveref has no name for the "fact"
  environment; the editor may convert.
* Large numbers in fact:e23 are typeset with thin spaces (5\,152\,320); sec_certificate.tex uses commas.
  The editor should pick one convention.

### B5. Things the editor should check at integration

* I cite parts of other authors' lemmas by letter: lem:geometry(c), lem:adm-closed(a)/(b), lem:canon(a)/(b)/(c),
  lem:invariance(a),(b),(e), lem:hall(c), prop:leaf-monotone(a), lem:group. These match the files as of
  2026-11-12 15:45 UTC; if a list is reordered, the letters must follow.
* Citations used: Beck2008, Krivelevich2014 (Theorem 3.1), Zermelo1913 (all present in refs.bib). I do not
  attribute strategy stealing to Hales-Jewett, because logs/paper/related_work.md marks that as unverified.
* sec_pairing.tex says that lem:pairing "is applied in thm:soundness"; it is applied in the proof of
  thm:sigma (the leaf case of thm:soundness points there). Either reference is acceptable.
* fact:e23 names logs/proof/e23min.cpp and logs/proof/e23min.log; if the artifact moves these files, the
  paths in app_extra_B.tex must follow.
* Length: Section 5 is about 13.5 pages in the test compile (guide: 10-13); Appendix part B about 3.5 pages.

## Author C (paper/sec_certificate.tex, sec_verification.tex, sec_repro.tex, app_formats.tex, app_ip.tex, README.md, scripts/make_artifact.sh, MANIFEST.sha256)

### C1. Macros, packages, labels

1. No change to `macros.tex` or `paper.tex` is needed. Each of my five files defines, guarded by
   `\@ifundefined`, a robust helper `\fn{...}` (= `\path{...}`, for file names and options, usable in
   captions); `sec_verification.tex` also `\providecommand`s `\accept`, `\reject`, and `app_ip.tex` `\worst`.
   If the editor prefers, move these four lines to `macros.tex`.
2. `\LEAF`, `\LOST`, `\CC` of `macros.tex` are math-only (`\mathsf`); in running text they must be written
   `$\LEAF$`. (Cost me one failed compile; other files may have the same trap.)
3. Labels defined in my files. Fixed by OUTLINE: `fact:H`, `tab:tree`, `alg:verify`, `alg:leaf`,
   `tab:verifiers`, `tab:leaves`, `sec:repro`. Sections: `sec:certificate`, `sec:verification`,
   `sec:formats` (Appendix A), `sec:ip` (Appendix B). Subsections: `ssec:fact`, `ssec:search`, `ssec:shape`,
   `ssec:leaftable`, `ssec:observations`, `ssec:walk`; `ssec:what`, `ssec:algverify`, `ssec:algleaf`,
   `ssec:impl`, `ssec:audits`, `ssec:cross`, `ssec:trust`; `ssec:artifact`, `ssec:requirements`,
   `ssec:certifying`, `ssec:reproduce`, `ssec:own`; `ssec:fmt-cells`, `ssec:fmt-canon`, `ssec:fmt-table`,
   `ssec:fmt-leaves`, `ssec:fmt-targets`, `ssec:fmt-leaf`, `ssec:fmt-orbits`, `ssec:fmt-minstones`;
   `ssec:ip-model`, `ssec:ip-solver`, `ssec:ip-regression`, `ssec:ip-times`. Statements:
   `prop:verify-correct` (Algorithm 1 accepts => valid certificate), `prop:leaf-correct` (Algorithm 2
   decides LEAF), `rem:true-cert`, `prop:ip-exact` (the 0-1 program computes worst(B,F,k)). Tables:
   `tab:leaf-theta` (leaves by t and theta), `tab:leaftable`, `tab:opening`, `tab:artifact`,
   `tab:reproduce`, `tab:leafvectors`, `tab:ip-regression`. Equations `eq:ip-obj` ... `eq:ip-danger`.
4. Labels of other authors that I cite beyond the fixed list: `sec:board`, `sec:pairing`, `sec:leaf`,
   `sec:game`, `ssec:deciding`, `def:U-assignment`, `lem:triple`, `lem:certificate`, `lem:game-invariance`,
   `cor:slack-necessary`, `prop:busy10`, `ex:T7`, `ex:T8`, `fact:T8-leaf`. Citation key used:
   `HuangfuHall2018`.

### C2. Numbers that differ from REPORT.md (recounted from the files)

1. REPORT section 15 / PROGRESS 08:35: "all 29 steps passed in 26 minutes". `logs/final/reproduce_validation.log`
   contains 26 PASS lines (the script has 26 steps), and the time stamps of `logs/repro/*.log` span
   08:06:32 (first rebuilt binary) to 08:29:52, i.e. about 23 minutes. The paper and README.md say
   "all 26 steps passed, in about 23 minutes".
2. `scripts/reproduce.sh` (header): "Requirements: g++ >= 9 ..., python3 (>= 3.8)". `verify/spotcheck.py` uses
   `int.bit_count()`, which needs Python >= 3.10. The paper and README.md say "Python 3.10 or later" and
   "a C++17 compiler with OpenMP (g++ 13.3 was used)". The script itself is not mine and was not edited.
3. REPORT section 7 calls the statement "every busy 10-set fails" computed plus an unrefereed hand argument.
   Author A's `prop:busy10` now proves it (for up to 11 busy cells); Section 6.4 cites the proposition and
   keeps the computation (0 of 2,468 orbits) as a confirmation.
4. Not printed because not traceable to a result file: the 739 s of the theta = 3 enumeration (PROGRESS
   only; the validation run needed about 8 minutes for that step by time stamps), the "about 5 core-hours"
   of HiGHS, the "26.5 million triple flows" total (I give no total), the 5,652 triples of the theta = 8 leaf.
5. All other numbers of REPORT sections 3-10 that I print were recounted from the raw files and agree:
   certificate line counts and checksums, per-round counters, leaves by t and theta, 434 non-busy moves
   (16/399/11/8), 151 VOID children (all of type (ii), all "fifth revealed stone on a live line"),
   enumeration 106,245, IP on leaves 1,308 (478 + 830), leaf table 6/947/2185 and 6/172/0, IP leaf table
   6 of 6 and 262 of 947 (254 fail, 8 hold), cross-check 1,291 = 656 + 623 + 12, 1,275 compared, 4 time-outs,
   160 with LEAF false and C true, simulator counters and coverage, 66 mutations (58 + 8), 12 source mutants.
   New numbers (not in REPORT): leaves by (t, theta) (tab:leaf-theta); 54,482 Q-states with |F| < t; orbit
   sizes 288/576/288/576/576/36 of the six good 8-sets (sum 2,340 = the search's good8 count); number of
   non-busy Breaker stones at the leaves (15,794 / 84,556 / 6,343 / 30 leaves with 0 / 1 / 2 / 3); the two
   lines of play of Section 6.6 (followed through the table with a script of mine); the IP model size for
   T8 (616 variables, 1,605 rows); IP running times by theta (medians recomputed from the result files).

### C3. The archive (scripts/make_artifact.sh)

1. Contents follow the coordinator's list with one addition: `results/ip/leaftable_k8.jsonl` and
   `results/ip/leaftable_k9.shard*.jsonl` (190 KB) are included, because the paper prints "6 of 6" and
   "262 of 947 (254 fail, 8 hold)" and no other file in the list backs these numbers. Remove the two lines
   from the script if this is not wanted.
2. Left out from `results/final/` as regenerable or duplicate: `checks/leaves_all.txt` (byte-identical to
   `leaves.txt`; the script checks this), `checks/leaves_theta*.txt` and `checks/perturbed.txt`
   (`scripts/leaf_instances.py` writes them), `sim/sim_snapshot_0806.bin`, `sim/sim_snapshot_0806.cpp`
   (byte-identical to `sim/sim.cpp`; the script checks this). From `paper/` only `*.tex`, `*.bib`
   (`*.bst`, `*.cls`, `*.sty` if any) and `paper.pdf` if present are taken; not `OUTLINE.md`, `build/`.
3. Per the instruction, no logs other than `logs/review/*.md` and `logs/spec_issues.md` are included.
   Consequence: the sources and outputs of the two review re-expansions cited in tab:verifiers
   (`logs/review/treecheck_mut/aexpand.cpp`, `aleaf.cpp`, `aexpand_final.json`; `logs/review/lead/lead_expand.cpp`,
   `lead_leaf.cpp`, `lead_expand.json`, `run_leaf.log`; about 60 KB) and the mutation records
   (`logs/review/treecheck_mut/mutations.jsonl`, `srcmut_results.jsonl`) are not in the archive; the paper
   says so (Sections 7.4 and 8.1) and points to the review reports. The same holds for `logs/final/*.log`,
   `logs/repro/*.log` (time stamps used for tab:reproduce) and `logs/search/*`. If the coordinator wants
   these in the archive, add the paths to the `required` array of the script and adjust the sentence
   "Not included ..." in `sec_repro.tex` and README.md.
4. The archive must be rebuilt after the paper is assembled (`paper/paper.pdf` is added automatically when it
   exists); `MANIFEST.sha256` changes with every change of a packaged file.

## Author A — Sections 2 (board), 3 (pairing), 4 (leaf), Appendix C.1 (`app_extra_A.tex`)

### A.1 Errors or gaps in PROOF.md §0.1, §§1–7

None found. Every statement was re-derived by hand while porting and re-tested by brute force with scripts of
my own (own geometry; `logs/paper/secA_check1.py`, `secA_check2.py`, `secA_check3.py`, output
`logs/paper/secA_checks.log`; no assertion failed). Remarks that change no statement:

1. PROOF Lemma 7.2, order of G: "either r = ∅ and s = id, or r = {0,1,2} and s = ρ" tacitly uses that then
   Π_π = id (because Σ_s = Ρ_r in both cases). The paper says so (`lem:group`(c)).
2. PROOF Lemma 2.2(b) is stated only for two live lines with a common cell outside X. The paper states it for
   any two distinct live lines and as an equivalence: X is B-admissible iff Hall's condition holds for all sets
   of one or two live lines (`lem:one-two`). The converse direction is new (three lines of proof) and gives
   "a pairing for (B,X) exists ⇒ X is B-admissible", which is the motivation asked for in the OUTLINE.
3. PROOF Lemma 6.1 assumes a connected triple; connectivity is not used (`lem:433` is stated for any three
   distinct live lines).
4. PROOF Proposition 4.3 is ported in its corrected form but with a self-contained proof that does not cite
   max-flow/min-cut: `prop:maxflow` is stated for "U-assignments" (lines outside U hold exactly two cells,
   lines of U at least two) and proves (a) excess ≤ sur_F(S) for all S ⊇ U by counting, (b) closed sets
   persist [= PROOF 4.3(d)], (c) the witness R [= PROOF 4.3(c), with R defined as the set of lines reachable
   from U, U included, so that the capacity-6 pitfall cannot occur], (d) exactly μ(U) augmentations in any
   order [= PROOF 4.3(b)], (e) the lines of U may be exhausted one after another [the consequence of (d) that
   PROOF mentions for `phi.hpp`]. The flow formulation, the value 2|L(B)| + μ(U) [= PROOF 4.3(a)] and the
   capacity ≥ 7 / exactly 6 caveat are in `rem:flow`.
5. REPORT §7 "hand argument, not part of the refereed PROOF.md" (every busy 10-set leaves a live triangle):
   verified step by step and written as `prop:busy10`, for every set of at most 11 busy cells (11·3 = 33 < 36
   face diagonals). So "LEAF(B,∅,10) is false for every busy 10-set" is now a proved statement, not only a
   computed one. Choice of the row and column: r and s from the two different pairs {i,5−i} of values whose
   level differs from that of the constant coordinate; this gives s ∉ {r, 5−r} and covers both diagonals.

### A.2 Decisions and deviations from the OUTLINE

* `lem:invariance` is stated and proved **at the end of Section 4** (`ssec:invariance`), after all notions
  are defined. Section 2 contains only the basic relations L(gB) = gL(B), N(gS) = gN(S) (`eq:inv-basic`) and a
  forward reference.
* `lem:canon` (Section 2) is stated at the level of cells and sets: (a) canon(hB,hF) = canon(B,F);
  (b) g(B+m) = Bᶜ+mᶜ, g(F+m) = Fᶜ+mᶜ for m = g⁻¹(mᶜ), hence equal canonical forms of the successor pairs;
  (c) two maps attaining the canonical form differ by a symmetry of (B,F). The statement "m is a valid table
  move iff mᶜ is" is Section 5's (`lem:game-invariance`). The canonical form itself is `def:canon`
  (sec_game.tex says "defined in Lemma lem:canon" — harmless, the editor may change it to `def:canon`).
* Additions not listed in the OUTLINE (all proved, all short): `cor:collinear` (collinearity test),
  `lem:degree` (lines through a cell: 7/4/3 for 24/144/48 cells), `lem:busy` (busy cells per line; two busy
  cells are collinear iff same level or same space diagonal), `lem:slack-bound` (a pairing for (B,X) needs
  |X| ≤ slack(B)), `prop:seven` (fewer than 7 stones: slack < 0), `lem:busy-slack`
  (slack(B) = 13|B| − 80 − 2p(B) for busy B with ≤ 2 cells per space diagonal; makes the counts for T7, T8
  two-line computations), `cor:slack-necessary` (LEAF(B,∅,k), k ≤ 10 ⇒ slack(B) ≥ k; already cited by
  sec_certificate.tex), the notation Φ(B,F) = least surplus of an F-connected set of ≥ 3 live lines, so that
  C_Φ(B,F,k) ⇔ Φ(B,F) ≥ k − |F| (`def:leaf`), `rem:C` (trivial monotonicity of C, worst(B,F,k), exact
  enumeration = PROOF Remark 2.3), `rem:leaf` (iii): LEAF = C for θ = 0.
* Computed Facts in my sections: `fact:aut` (|Aut| = 1152; `logs/canon/autgroup.cpp`, `.log`; not used),
  `fact:T-pairings` (pairings for T7, T8), `fact:T8-leaf` (LEAF(T8,∅,8); L(T8), L(T7) connected with surplus
  8 and 1; Φ(T8,∅) = 8, Φ(T7,∅) = 1). That LEAF(T8,∅,9) and LEAF(T7,∅,k), k ≥ 2, fail is *proved*
  (`cor:slack-necessary`), not computed.
* Lengths in the isolated test compile: Section 2: 6 pages (with one figure and the notation table),
  Section 3: 4, Section 4: 11 (one figure), Appendix C.1: 3. Longer than the guide because of the additions
  above and of fully written-out proofs.

### A.3 For the editor / author C

* No new macro or package is needed. Local notation: λ(i) = min(i,5−i) (level), Π_π, Ρ_r, Σ_s, H, K (Section 2
  only), δ(B,X), Φ(B,F), μ(U), e(α), R_α(U) (Sections 3–4). `Y+c` for Y ∪ {c} is defined in `ssec:live`.
* Cross-file labels I cite: `sec:game`, `sec:verification`, `sec:certificate`, `sec:formats`, `sec:ip`,
  `alg:leaf`, `thm:main`, `thm:soundness`, `lem:threat`, `lem:danger`, `prop:lost-stones`,
  `prop:lazy-monotone`. All exist in the current files except `thm:main` (introduction).
* Bibliography keys used: HalesJewett1963, Beck2008, Hall1935, FordFulkerson1962, HopcroftKarp1973,
  Schrijver2003, Silver1967, DvorakValla2021 (all in refs.bib). About DvorakValla2021 I only say that the
  automorphism groups of the boards n^d "were characterized" there and that we did not rely on it; I did not
  check that their theorem gives 1152 for n = 6, d = 3 (cf. the note in related_work.md).
* **Artifact:** the Computed Facts `fact:T-pairings` and `fact:T8-leaf` name `logs/paper/secA_check1.py` and
  `logs/paper/secA_checks.log` (the latter contains the outputs of `bin/phicheck` and
  `bin/treecheck --leafcheck` on the four instances (T8,∅,8), (T8,∅,9), (T7,∅,1), (T7,∅,2)); `fact:aut` names
  `logs/canon/autgroup.cpp` and `autgroup.log`; `app_extra_A.tex` names `logs/proof/fuzz_cprime.py`. These
  files, together with `logs/core/test_core.log`, `logs/minstones/run.log` and
  `results/regression/ip_regression.json`, should be in the artifact (or the sentences adjusted).
* Figures: `fig:busy` (Section 2: the six layers with the 24 busy cells and T7/T8), `fig:triangle`
  (Section 4: the triangle of `prop:busy10`). Table: `tab:notation` (end of Section 2).
* Follow-up to B5 (third bullet): sec_pairing.tex now says that `lem:pairing` is applied in the proof of
  `thm:sigma`.
* Remark on B4 ("cleveref has no name for the fact environment"): in a test compile with the preamble of
  paper.tex, `\cref{fact:x}` prints "Computed Fact 1.1" (cleveref takes the name from `\newtheorem`), so `\cref`
  can be used for facts as well. My files write `Computed Fact~\ref{...}` in the two places where a fact is cited.

### C4. Test of the archive

`logs/paper/artifact_test.log` is the output of a scripted test of `dist/ttt6-artifact.tar.gz` in a fresh
directory (16:00 UTC): `sha256sum -c MANIFEST.sha256` ok (82 entries); `make bin/treecheck` 12 s; the
certifying treecheck command: exit 0, ok, 246,299 states, 2.9 s wall on 4 threads (10.6 CPU-s), all keys
except threads/timing equal to `results/final/treecheck.json`; `python3 verify/spotcheck.py --selftest`:
`selftest OK`, 12 s; exhaustive `spotcheck.py` with 3 processes: ok, 2 min 14 s (6.3 CPU-min), all keys
except argv/procs/timing equal to `results/final/spotcheck_full.json`. A full build of all eleven programs
in the unpacked archive took 55 s, and the README's other entry points (`bin/phicheck`, `bin/enumcheck`,
`bin/treecheck --leafcheck`, `bin/geom_selftest`) work there. The timings quoted in `sec_repro.tex` are
those of this log. The archive was rebuilt afterwards (only `paper/*.tex` of mine changed); it has to be
rebuilt again whenever a packaged file changes (PROGRESS.md changed once while I was testing) and after
`paper/paper.pdf` has been built.

I also re-ran `bin/search --rmax 10 --maxcand 4 --relcand 24` (busy-only, relaxed game exhaustive) for
Section 6.5: FAILED in 14.2 s with the same counters as `results/search/busyonly_r10_rel24/stats.json`
(12,437 relaxed P-states, 14,196 relaxed Q-states), and checked with `bin/phicheck` that the witness for
the final state of the failing branch is exactly the 14 lines of the face x = 5 with surplus 4.

### C5. Scripts behind the new numbers

`logs/paper/secC_recount.py` (own geometry, brute-force canonical form; reads only result files; 8 s) and its
output `logs/paper/secC_recount.log` reproduce every number of my sections that is not already a field of a
result JSON: counts by t and theta, non-busy moves and stones, the classification of the 151 VOID children,
the two lines of play of Section 6.6, the orbit sizes of the six good 8-sets, the enumeration / IP / leaf
table / cross-check counts, IP running times by theta, and the canonical-form test vectors of Appendix A.
The LEAF test vectors of Appendix A were computed with `bin/phicheck`, `bin/treecheck --leafcheck` and
`verify/spotcheck.py` (`leaf_check`), which agree on verdict and failing condition for all of them.

## Editor (paper/sec_front.tex, sec_intro.tex, sec_discussion.tex, app_extra.tex, app_seven.tex, paper.tex, macros.tex; assembly)

### E1. Result of the assembly

`paper/paper.pdf`: 73 pages, zero LaTeX/BibTeX warnings, no undefined or multiply defined label, no overfull
or underfull box (`latexmk -pdf -interaction=nonstopmode paper.tex`); text in `paper/paper.txt`. Structure:
front matter with table of contents; Sections 1-9; Appendices A (formats), B (integer program), C
(supplementary results, parts A and B), D (pairing for the mid-planes of 7^3, new); references at the end.

### E2. Mathematical findings

1. No discrepancy between Section 7 (`ssec:what`, `alg:verify`) and Section 5 (`def:states`, `def:certificate`):
   children (a), (c), (d), (i), (ii), the VOID test, (R0)-(R2) with the proviso `m notin B u F`, (V1)-(V3)
   agree clause by clause, also with SPEC section 4. No proof of authors A, B, C was changed.
2. New statements by the editor, all with complete proofs in `sec_discussion.tex`:
   * `lem:seven-decomp` (mid-plane decomposition of 7^3), `lem:seven-pairing`, `cor:seven`. The corollary is
     proved with a *virtual play* on the 6^3 part: Breaker's extra stones (free moves, needed e.g. when Maker
     fills the last of the 127 cells of M) are ignored by the virtual play until sigma asks for them. The
     pairing in `tab:seven-pairing` is NOT the one printed by `scripts/check_7cube_midplanes.py`: it is a
     structured pairing (rows get residues 1, 2 of s-r mod 6, columns 4, 5, the two diagonals 0 and 3, in each
     mid-plane; two cells on each of the three axes through the centre), which can be checked by hand in a
     few lines. It was verified by a script of mine both from the rule and by parsing the LaTeX table; the
     project script was re-run (exit 0) and confirms the decomposition and the existence of a pairing.
   * `prop:rmax8`: if the root is WON for R_max, then R_max >= 8 (follow the branch with F empty; a leaf at
     t = 7 would need slack >= 7 by `cor:slack-necessary`, but 7 cells meet at most 46 lines, so slack <= 5).
     Sanity check: the maximum over the 346,104 sets of 7 busy cells is 44 lines (slack 1).
3. `cor:seven` uses `prop:stealing` on a board with an odd number of cells (343); author B's proof treats
   that case explicitly (issue B1.1), and I re-read it with this use in mind.

### E3. Changes in other authors' files (no change of mathematical content)

* Local macros moved to `macros.tex` (`\Etwo`, `\Ethree`, `\TABLE`, `\PAIRING`, `\WONp`, `\rk`, `\fn`, `\accept`,
  `\reject`, `\worst`); `\code{..}` replaced by `\fn{..}` everywhere; `\fn` is always upright; small caps
  (`\WON`, `\VOID`, ...) are always upright (removes the font substitution warnings in italic text).
* References: `sec_game.tex` and `app_extra_B.tex` converted from `Lemma~\ref{..}(a)` to `\cref{..}\,(a)`;
  `Computed Fact~\ref` -> `\cref` everywhere; `lem:canon` -> `def:canon` where the canonical form is meant.
* Numbers: comma as thousands separator throughout (1,152; fact:e23; Appendix A); the per-round list in
  `ssec:fmt-targets` is now a table.
* Computed Facts: new macro `\established` (statement italic, provenance upright); titles capitalised; a few
  untitled propositions/examples got titles (`prop:verify-correct`, `prop:leaf-correct`, `prop:ip-exact`,
  `prop:e23-min`, `ex:conservative`, `fact:aut`).
* `sec_pairing.tex`: "Pairing strategies were introduced ... by Hales and Jewett" -> "go back to" (their
  paper was not read by the literature agent); "forced answer" -> "forced reply".
* `sec_verification.tex` (Independence): "written by a different author" / "by two further authors, each with
  his own geometry" reworded neutrally, with a pointer to the declaration on software tools; "E2", "E3" set
  as `$\Etwo$`, `$\Ethree$`.
* `sec_repro.tex`: the second copy of the SHA-256 table replaced by a reference to Section 6.1; "of his own"
  -> "of their own"; new sentence on files that are not in the archive (see E4.1).
* `sec_board.tex`, `sec_pairing.tex`, `sec_leaf.tex`, `app_extra_A.tex`, `app_extra_B.tex`: "of the artifact"
  -> "of the project repository" for files under `logs/` (see E4.1); two occurrences of
  `\code{bin/treecheck -{}-leafcheck}` split into program and option.
* `sec_game.tex`: "nonempty" -> "non-empty". `sec_certificate.tex`: "towards" -> "toward"; two tables `[tbp]`.
* `paper.tex`: table of contents, `\raggedbottom`, a patch that puts 6pt above table captions (amsart assumes
  captions on top of tables), bibliography after the appendices, `app_seven.tex`.

### E4. Open points for the coordinator

1. **Archive.** The Computed Facts `fact:aut`, `fact:T-pairings`, `fact:T8-leaf`, `fact:e23`, the fuzz test at the
   end of Appendix C.1 and `rem:seven` name files that `scripts/make_artifact.sh` does not package:
   `logs/canon/autgroup.{cpp,log}`, `logs/core/test_core.log`, `logs/minstones/run.log`,
   `logs/paper/secA_check1.py`, `logs/paper/secA_checks.log`, `logs/proof/e23min.{cpp,log}`,
   `logs/proof/fuzz_cprime.py`, `scripts/check_7cube_midplanes.py`. The paper now says that they are "kept in
   ... the project repository" and are not in the archive (Section 8.1). Either add them to the `required`
   list of the script (then change that sentence back) or keep the sentence. None is used by `thm:main`.
2. **MANIFEST.sha256 / dist/ttt6-artifact.tar.gz are stale**: every file under `paper/` changed and
   `paper/paper.pdf` now exists. Rebuild with `scripts/make_artifact.sh` (not done by the editor: not my files).
3. Placeholders: author line; "[repository URL to be added]" (Section 1.7 and "Data and code availability").
4. Literature statements that rest on secondary sources only (see `related_work.md`): the Hales-Jewett bounds
   (cited with Erdos-Selfridge p. 300 and Krivelevich Thm 2.1/Cor 3.4), Patashnik's result, the presence of the
   8^3 pairing in *Winning Ways* (cited next to Golomb-Hales because the task asked for it), the degree form of
   the Erdos-Selfridge criterion (quoted from Cain et al., Theorem 6.1), "about 3.5 * 10^7 positions" for the 7^3
   computation. The novelty claim is worded "to our knowledge ... we found no earlier proof or claim".

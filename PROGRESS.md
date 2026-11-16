# PROGRESS.md — coordinator log (times UTC, 2026-11-12)

## Decisions

* 05:10 **Compute is local only.** The user instructed: no commits and no pulls to any repository until a
  repository is named. Cloud threads/sessions could only share code and results through a repository, so
  section 5's first two options are unavailable. Everything runs on this machine: 4 cores, 15 GB RAM.
  No git repository is created. Nothing leaves this machine.
* 05:10 **Expected time (reported before starting compute):** leaf table by exact max-flow: minutes.
  IP cross-checks (HiGHS, 1–12 s each): 2–3 core-hours, run in the background at low priority.
  AND–OR search: unknown, estimated 10–60 minutes on 4 cores per attempt. Simulator, 10^6 games: 10–20
  minutes. Total target 3–4 hours wall-clock.
* 05:25 **Leaf predicate is LEAF = C_Φ (SPEC §3), not the raw IP.** Reason: a Hall violator can be taken
  connected; with 1 or 2 lines it contradicts admissibility; so "every connected set of ≥ 3 live lines has
  surplus ≥ k − |F|" is sufficient for C(B,F,k). It is decided exactly by integer max-flow (no floating
  point), it is monotone in B, F and k, and for F = ∅, k ≤ 10 it is equivalent to C. The integer program
  stays as an independent cross-check and for the regression values.
* 05:25 **L3(c) is not used for C itself.** Adding an SP stone b makes lines through b dead, which makes
  more X admissible (5 FP stones on a dead line), so C(B,F,k) ⇒ C(B∪{b},F,k) has no easy proof and may be
  false. The monotonicity is proved and used for LEAF only.
* 05:25 **Lazy revelation replaces branch (b).** The abstract state only records FP stones that were
  revealed by a threat, a danger, or by SP trying to take an occupied cell. FP taking a busy cell is an
  ordinary absorbed move; when SP's table move is occupied, the stone is revealed and SP looks up again.
  This is sound (F ⊆ X is all the leaf certificate needs), it removes the ~20-way branching of (b) at every
  round, and it makes non-busy SP moves expressible. The user's eager rule is the special case where SP
  probes every busy cell every round.
* 05:25 **Loss patterns (e) are the two that can really occur** given the invariant "X is admissible at the
  start of every round": threat + surviving danger (≥ 11 stones) and two dangers with four distinct lines
  (≥ 12 stones). Two simultaneous threats would need a danger pair before the move. Details in PROOF.md.
* 05:25 **The whole proof tree is re-verified by an independent program** (`verify/treecheck.cpp`, own
  geometry and own max-flow, written without reading `src/`). This is the exact certification of every leaf.
* 05:30 Workflow concurrency is capped at 2 agents per workflow on this 4-core machine, so the build runs as
  several workflows in parallel.

## Timeline

* 05:08 start. 05:10 user: no repositories. 05:20 `src/geom.hpp` written and self-tested (148 lines =
  108+36+4, 1152 symmetries closed under composition and inverse, slack(T7)=1, slack(T8)=8).
* 05:35 SPEC.md frozen.
* 05:45 Wave 1 launched as four workflows (8 agents, all on the model requested in the task statement):
  * `ttt6-core`: tasks 2+6a (match.hpp, phi.hpp; xhigh) → task 6b (certify + IP cross-check; xhigh); task 1 (canon.hpp, leaf table; high).
  * `ttt6-search-proof`: tasks 4+5 (search.cpp; xhigh); PROOF.md (xhigh).
  * `ttt6-ip-sim`: task 3 (HiGHS IP + regression; high); task 7 (simulator; high).
  * `ttt6-verify-regress`: task 6c (independent tree verifier; xhigh); regression 1 (minimum stones for n=5..8; high).
* 06:02 Modules ready: canon.hpp (05:47), ip_check.py (05:48), match.hpp + phi.hpp (06:01; 36030 checks, 0 failures).
  Full automorphism group of the cell–line incidence graph computed by the task-1 agent: order 1152 = G.
* 06:02 Leaf table by exact max-flow (LEAF(B,∅,|B|) per orbit): k=8: 6 of 1057 orbits (exactly the 6 with
  slack ≥ 8; T8's orbit is one). k=9: 172 of 1669 (947 have slack ≥ 9). k=10: 0 of 2468, as predicted by
  the triangle argument.
* 06:02 Regression (SPEC §8): all IP values reproduced (items 2–9); minimum stones 9/7/3/0 for n=5/6/7/8.
  IP timing differs from the task statement: instances that hold with a non-empty F and large k−|F| take
  26–155 s; instances with k−|F| ≤ 1 take < 0.1 s.
* 06:01 First search result (search agent, to be verified independently): busy-only table moves fail even
  without forced events for R_max = 9, 10, 11 (lazy revelation; FP steals busy cells). With non-busy table
  moves allowed the root is WON at R_max = 10 in 87 s: 152,900 P-states, 92,775 Q-states, 106,681 leaves.
* 06:06 IP cross-check of the leaf table started in the background (decide mode, nice 15):
  k=8 (6 orbits with slack ≥ 8), then k=9 (947 orbits with slack ≥ 9), 2 processes.
* 06:11 PROOF.md written (proof writer). Findings: no SPEC statement false; L3(c) is false as literally
  stated when F is not B-admissible (C is then vacuous) and proved when F is B-admissible (Prop. 5.4, not
  used by the tree); E2/E3 are necessary, not sufficient, for a loss (LOST is conservative).
* 06:18 Search agent: busy-only table moves fail (exhaustively, lazy game, even without forced events) for
  R_max ≤ 10; with non-busy moves R_max = 9 fails on a double-threat branch (maxcand 6, not exhaustive);
  R_max = 10 proves the root. After the core headers were tuned at 06:14 the tree is 153,135 P-states
  (106,723 leaves) and 93,164 Q-states; `bin/treecheck` (independent) reports ok on it.
* 06:20 Coordinator rehearsal on that tree: `bin/sim` 3,000 mixed games, 0 bugs, 0 holes, canonical forms of
  all table lines agree with the simulator's own. `bin/phicheck` (src) and `bin/treecheck --leafcheck`
  (independent) agree on all 106,723 leaves (all LEAF) and on 20,000 perturbed instances (13,688 true;
  reasons 1/2/3 identical).
* 06:25 Review part A launched (math referee, xhigh; third independent Python spot-check, high).
* 06:24 Exact enumeration (`bin/enumcheck`, decides C itself) on all 104,811 provisional leaves with
  k−|F| ≤ 2: all hold (40 s).
* 06:25 IP leaf table: k=8 complete (6 orbits with slack ≥ 8, all "holds", 18–298 s each under load) and
  equal to the max-flow table. k=9 runs in the background; "holds" instances take 100–320 s, so k=9 will be
  reported with partial coverage and k=10 only through the triangle argument plus the max-flow table.
* 06:34 Review part B launched (code audit of search.cpp / phi.hpp / match.hpp / canon.hpp, xhigh;
  simulator conformance audit plus new adversaries "danger", "double", "corner", "tree", high).
* 06:36 IP check of a stratified sample of 889 provisional leaves started (k−|F| = 0..8); first timings
  0.02–5 s per leaf.
* Decision: the final tree is generated only after the core headers stop changing (the non-busy candidate
  list uses the non-unique violator set returned by phi_check, so the emitted tree depends on the header
  version; LEAF itself does not). Every tree is verified after the fact by treecheck, so this affects
  reproducibility of the file, not soundness.
* 06:57 Review part A done.
  * Math referee (xhigh): verdict sound-with-minor-issues; every numbered statement re-derived, no false
    statement, no gap from (H) to the theorem; 7 minor findings (theorem should name the PAIRING rule of
    Lemma 1.1; a parenthetical about capacity 6 in Prop. 4.3; two existence claims are COMPUTED not PROVED;
    treecheck must run with --exact; wording). To be applied to PROOF.md after the final run.
  * Third implementation `verify/spotcheck.py` (pure Python, written from SPEC only): exhaustive run on the
    provisional tree, ok, 246,299 states, all 106,723 leaves recomputed; every counter equals treecheck's.
    Observation: LOST cannot fit at an expanded state when R_max = 10 (E2 needs ≥ 11 stones, E3 ≥ 12).
  * SPEC.md §10 errata added (no program change).
* 07:03 Final tree generated with the final headers: `results/final/strategy.tbl` (93,164 Q-states, 434
  non-busy table moves) and `results/final/leaves.txt` (106,723 leaves), R_max = 10, 153,135 P-states
  (46,412 expanded), search 58 s, selfcheck ok. Byte-identical (md5) to the provisional tree
  `results/search/r10_nb_k4_rebuilt`, so the rehearsal checks apply to it unchanged.
* 07:08 Tree-verifier agent done (verify/treecheck.cpp, 4,012 lines, never read src/ or sim/).
  Official run on the final tree: `bin/treecheck --exact --paranoid`: ok, 246,299 states, 143,498 threat
  and 28,275 danger children, 151 VOID children (all type (ii)), 0 errors (`results/final/treecheck.json`).
* 07:10 Review part C launched: adversarial audit + mutation testing of treecheck.cpp (xhigh); PROOF.md
  maintenance applying the referee's 7 minor findings (xhigh). In the background: treecheck deep
  cross-check mode and the exhaustive Python spot-check on the final tree; exact enumeration of the 1,434
  leaves with k−|F| = 3.
* 07:12 Exact enumeration of C on the leaves with k−|F| = 3: all 1,434 hold (739 s). Together with the
  earlier run: C itself is decided exactly on all 106,245 leaves with k−|F| ≤ 3.
* 07:12 IP (HiGHS, decide mode) on a stratified sample of 889 leaves (300/300/150/80/30/16/8/4/1 for
  k−|F| = 0..8): all "holds". IP on the remaining 419 leaves with k−|F| ≥ 4 started.
* 07:21 `verify/spotcheck.py` exhaustive run on the final tree: ok, 246,299 states, 46,412 P-states
  expanded, 93,164 Q-states, 106,723 leaves recomputed, children a/c/d/i/ii = 46,412/143,498/28,275/
  93,164/54,482, 151 VOID; identical to treecheck's counters (`results/final/spotcheck_full.json`).
* 07:10 Search/core code audit report written (logs/review/search_core_audit.md): sound-with-minor-issues,
  no fatal or major finding; brute-force references agree on 588,000 instances.
* 07:25 `bin/treecheck --crosscheck --crosscheck-leaves 50 --exact --paranoid` on the final tree: ok
  (all 246,299 canonical forms by brute force, literal children/LOST for all 46,412 expanded P-states,
  2,153 leaves with unpruned flows), 945 s.
* 07:26 PROOF.md revised (referee's 7 findings applied; new Proposition 8.4a: E2 needs ≥ 11 and E3 ≥ 12
  stones, so LOST is false for t ≤ 9; final computed facts in §9.1; revision log at the end).
* 08:03 Audit of verify/treecheck.cpp (xhigh) done: sound-with-minor-issues, no fatal or major finding.
  66 file mutations: 58 rejected, 8 accepted, none of which removes or changes an obligation; LEAF against
  brute force on 53,755 evaluations and LOST on 800 states: 0 mismatches; 12 source mutants: self-test catches
  12/12; an independent structural re-expansion by the auditor reproduces every counter. Minor findings
  (conflicting duplicate table keys not part of `ok`, stale --out file on abort, option parsing, VOID root)
  are recorded in REPORT.md; treecheck.cpp is left at the audited revision. SPEC §10.6 reworded, §10.7 added.
* 08:03 Certifier vs HiGHS (task 6b), interim: 1,291 instances (656 hold, 623 fail, 12 void), 1,252 compared,
  0 disagreements; 160 instances have LEAF false but C true (LEAF is strictly weaker than C when F ≠ ∅).
* 08:05 HiGHS leaf table stopped: k=8 6/6, k=9 262/947 (254 fail, 8 hold), all equal to the max-flow table.
* 08:05 Task 8 (lead reviewer, xhigh) launched; it writes REVIEW.md.
* 08:06 `scripts/reproduce.sh` written; validation run started (GAMES=20000).
* 08:09 10^6-game simulator run started on the final tree with a snapshot of the audited simulator
  (`results/final/sim/sim_snapshot_0806.cpp`; the simulator audit agent was still tuning adversaries).
  Decision: not waiting for the audit agent's last edits, because the run needs 40–60 minutes at the measured
  260–415 games/s and the audit's later changes concern adversary weights, not the SP side.
* 08:25 Task 6b final: `bin/certify` vs HiGHS on 1,291 instances: 1,275 compared, 0 disagreements
  (652 holds, 623 fails), 12 void (IP infeasible on all), 4 IP timeouts (F = ∅, θ = 8–9, hold by LEAF).
* 08:28 HiGHS on leaves finished: all 478 leaves with k−|F| ≥ 4 and 830 sampled leaves with k−|F| ≤ 3
  hold (1,308 leaves, 0 disagreements with LEAF).
* 08:25 The simulator-audit agent of review part B was stopped by the coordinator: after finishing the
  conformance pass and the new adversaries it kept iterating on adversary coverage without writing its
  report. 08:29 a time-boxed agent was launched to write `logs/review/sim_audit.md` (conformance reading of
  the SP side, evidence summary, self-test and a paranoid run with the current binary). The source used for
  the 10^6 run differs from the current `sim/sim.cpp` only in adversary steal timing and two counters (diff
  checked by the coordinator).
* 08:34 10^6-game simulator run finished: 0 FP lines, 0 holes, 0 pairing failures, 0 admissibility failures,
  0 look-ahead bugs; 1,121,484 threat events, 85,806 danger events, 2,254,365 occupied reveals; 78,957 of
  93,164 table keys and 81,907 of 106,723 leaf keys met in real games (`results/final/sim/sim_mix_1M.json`).
* 08:35 `scripts/reproduce.sh` validation (GAMES=20000, THREADS=2): all 26 steps passed in under half an hour; the
  regenerated tree is byte-identical to `results/final/`.
* 08:36 The first simulator-audit agent was found still editing `sim/sim.cpp` (two hours, adversary tuning,
  no report); its workflow was stopped. Its last source is kept as
  `logs/review/sim_audit_runs/sim_latest_unvalidated_0834.cpp`; `sim/sim.cpp` was reset to the snapshot used
  for the official run (md5 c2112bb9…), rebuilt, self-test passed, 4,000-game paranoid run on the final tree:
  0 bugs. The time-boxed second agent delivered `logs/review/sim_audit.md`: sound-with-minor-issues.
* 08:37 **Task 8: SIGNED OFF** (`REVIEW.md` §6). The lead reviewer re-derived the proof chain, wrote its own
  re-expansion (`lead_expand.cpp`: 246,299 states, 0 errors, all counters equal) and its own LEAF
  (`lead_leaf.cpp`: all 106,723 leaves; no pruning at all on the 1,912 leaves with θ ≥ 3), and its own
  implementation of σ in real coordinates against 6 adversaries (1,860 games, 0 failures). Limits of the
  sign-off: computer-assisted; applies to the two files with the recorded checksums; it is not a sign-off of
  the eager busy-only tree of the task statement, which was not proved.
* 08:40 REPORT.md completed (label of the |B| = 10 statement corrected to [computed] as the reviewer asked);
  stale status phrases in PROOF.md updated (revision log item 13).

## Final status

PROVED (computer-assisted), all six acceptance criteria PASS; see REPORT.md. Wall-clock 05:08–08:40 UTC.
Nothing was committed, pushed or uploaded; no repository exists in this directory.
* 08:41 All workflows finished. Simulator audit (second agent): sound-with-minor-issues. Lead reviewer's final
  check covered the REPORT.md version of 08:38 and the PROOF.md status edits (no statement changed). No
  background job is running.

## Paper (requested by the user after the proof run)

* 15:10 User asked for a journal-quality paper from which the proof can be replicated. Decisions:
  * Format: LaTeX (`amsart`) with a compiled PDF in `paper/`, kept local. TeX Live was installed with apt
    (it was not on the machine). A hosted document was not created because of the standing instruction to keep
    everything private.
  * The paper names the players Maker (first) and Breaker (second); proved statements are theorems, anything
    established by running programs is a "Computed Fact" naming the programs; the main theorem is stated as
    computer-assisted and depends on exactly one computed fact (the root is WON for R_max = 10).
  * Novelty is claimed only as far as a literature search supports it (separate agent, generic bibliographic
    queries only).
  * Model identifiers were removed from PROGRESS.md and REPORT.md (two places).
* 15:25 `paper/OUTLINE.md`, `paper/macros.tex`, `paper/paper.tex`, `paper/build_one.sh` written by the
  coordinator. Writing wave launched: author A (board, pairings, leaf certificates), author B (abstract game,
  soundness), author C (certificate, verification, reproducibility, appendices, README, artifact archive),
  literature agent (related work, refs.bib).
* Plan after the writing wave: editor (front matter, introduction, discussion, assembly, compile); then four
  referees with different lenses (mathematical fidelity against PROOF.md; every number against the result
  files; journal standards; replication from the paper alone: an agent that sees only the PDF text and the
  two certificate files writes its own verifier); then revision and a final check.
* 15:58 Authors A and B done (Sections 2-5, Appendix C parts; 6 + 4 + 11 + 14 pages in test compiles). Both
  re-derived every proof while porting. Findings about PROOF.md: one minor gap in the strategy-stealing
  proposition for boards with an odd number of cells (irrelevant for 216 cells; closed in the paper) and one
  imprecision in the definition of the reachable set for invalid certificates (proviso added). Additions
  proved in the paper: at most one forced event with ≤ 10 Maker stones; every E3 pattern has ≥ 13 stones on
  this board (hand proof); a hand-checkable example that LOST is conservative; LEAF fails for every set of
  at most 11 busy cells with F = ∅, k = 10.
* 15:58 Literature report (`logs/paper/related_work.md`, 36 BibTeX entries): no earlier proof or claim for
  6^3 found. Directly relevant: Cain, Raymond, Källersjö, arXiv:2509.21494 (Sept 2025, unrefereed preprint)
  prove the Breaker win for 7^3 by "3–4 opening stones, then a pairing" and list 5^3 and 6^3 as the two
  remaining open cases. Golomb–Hales (2002) mark 5^3, 6^3, 7^3 as conjectured draws; Beck (2008) calls 5^3
  open. The novelty claim in the paper is worded "to our knowledge".
* 16:00 Coordinator check (`scripts/check_7cube_midplanes.py`, exact): the 7^3 board splits into the 127
  cells M with a coordinate equal to 3 and a 6×6×6 array A; 45 lines lie in M and admit a pairing inside M,
  and each of the other 148 lines meets A in a line of the array. Hence a second-player blocking strategy on
  6^3 gives one on 7^3 (decomposition after Golomb–Hales). The paper states this as a corollary with proof.
* 16:01 Editor launched (front matter, introduction, discussion with the 7^3 corollary, assembly, full PDF).
* 16:05 Author C done (Sections 6–8, Appendices A–B, README.md, `scripts/make_artifact.sh`,
  `dist/ttt6-artifact.tar.gz` 3.4 MB, tested in a fresh directory: checksums ok, treecheck ok in 2.9 s,
  spotcheck.py exhaustive ok). Every printed number re-read from the raw files. Corrections taken over into
  REPORT.md / reproduce.sh: the validation run has 26 steps (not 29); spotcheck.py needs Python ≥ 3.10.
* 16:34 Editor done: `paper/paper.pdf`, 73 pages, clean build (no undefined references, no warnings).
  Theorem 1.1 (computer-assisted): Breaker, moving second, wins the Maker–Breaker game on the 148 lines of
  the 6×6×6 board; hence the ordinary game is a draw. Corollary 9.3: the same for 7×7×7 via the mid-plane
  decomposition with an explicit hand-checkable pairing of the 45 mid-plane lines (Appendix D).
  New, so far unrefereed statements: Lemmas 9.1, 9.2, Corollary 9.3, Proposition 9.5 (R_max ≥ 8), the
  correctness propositions for Algorithms 1 and 2 and for the integer program, and the additions of the
  section authors. Novelty wording: "to our knowledge".
* 16:36 Five referees launched in three workflows: mathematics a (Sections 2–4, Appendices B, C.1,
  Algorithm 2), mathematics b (Theorem 1.1, Section 5, Appendix C.2, Section 9, Algorithm 1), audit of
  every number and factual claim against the raw files, journal standards, and replication from the paper
  alone (`/home/user/ttt6_replication/`: the PDF, its text and the two certificate files only; the agent
  must write its own verifier and may not read anything else of the project).
* 17:27 All five referee reports are in (`logs/paper/referee_*.md`):
  * Mathematics a (Sections 2–4, App. B, C.1, Algorithm 2): minor revision; every statement correct and no gap
    in a proof; one major finding about exposition: the main verifier's leaf test deletes strong lines, which
    the printed Algorithm 2 and Remark 7.3 do not describe (lemma supplied; `spotcheck.py` does implement
    Algorithm 2 as printed).
  * Mathematics b (Theorem 1.1, Section 5, App. C.2–C.3, Section 9, Algorithm 1): minor revision; no fatal or
    major finding; all new statements (Cor. 9.3, Prop. 9.5, E3 ≥ 13 stones, …) confirmed by hand and by program.
  * Numbers and claims: minor revision; every headline number correct; six body statements to reword.
  * Journal standards: major revision on disclosure, availability and length (no objection to the mathematics).
  * Replication from the paper alone: replicated. A 706-line C++ verifier written only from the PDF accepts
    the certificate and reproduces every per-round count; 11 minor ambiguities. The audit referee and
    mathematics referee b also wrote their own implementations of Algorithm 1 from the paper; both accept.
* Decisions for the revision: (1) the declaration on AI-based tools is rewritten to say plainly what was
  produced by software agents and what "independently written" means, with bracketed placeholders for the
  tool names and for what the author(s) checked; no product names are inserted by the coordinator. (2) One
  PDF: main part plus a clearly separated supplement part, nothing deleted (the user asked for a paper
  "containing everything"). (3) All files cited by Computed Facts go into the archive, together with the
  verifiers written from the paper alone.
* 19:08 Revision finished (it took 100 minutes, far longer than the 45 I had planned for). Result:
  `paper/paper.pdf`, 91 pages in one file: main part pp. 1–70 (Sections 1–9, Declarations, Appendix A file
  formats and test vectors, Appendix B the 7×7×7 pairing table, references) and supplementary material
  pp. 71–91 (S1–S7; nothing was deleted). Clean build. Every first-round finding has an entry in
  `logs/paper/response_to_referees.md`. Notable changes: a lemma covering the main verifier's deletion of
  strong lines; Proposition 4.22 strengthened to 17 busy cells; a worked example on the real board and a
  roadmap in Section 5; eight LOST test vectors; the Declarations section with bracketed placeholders for
  tool names and for what the author(s) checked; the board is written Ω.
  Archive rebuilt: `dist/ttt6-artifact.tar.gz`, 314 files, 6.3 MB; tested in a fresh directory (checksums
  313/313, treecheck ok in 2.7 s, spotcheck self-test ok, the verifier written from the PDF accepts).
  Not applied (left for the authors): obtaining the original Hales–Jewett 1963 and Patashnik 1980 texts
  (their statements are quoted through secondary sources); a machine/OS description; a licence file.
* 19:12 Second-round check launched: (X) the mathematics added or changed in the revision and the dependency
  chain of Theorem 1.1 after the restructuring; (Y) closure of the first-round findings, numbers changed in
  the revision, placeholders, archive, layout, language; followed by a final fix-and-rebuild pass.
* 19:53 Second-round check finished. Both checkers: minor revision, no fatal finding.
  * Checker X (mathematics added or changed in the revision): every new or changed statement and proof it
    checked is correct (deleted-lines lemma tested against the verifier's code and by brute force;
    Proposition 4.22 confirmed exhaustively over all 2^24 busy sets and shown sharp; worked example, LOST
    vectors, 7×7×7 proofs recomputed). Dependency chain: everything Theorem 1.1 uses is stated and proved in
    the main part (pp. 7–42), exactly one Computed Fact enters, nothing needed was moved to the supplement.
    10 minor/editorial findings.
  * Checker Y (closure, quality, archive): all first-round major findings closed; one sentence in the
    Declarations overstated agreement "of children in every round among all verifiers" (spotcheck.py
    reports children only in total); "proved by hand" could be read as "checked by a person"; a table
    floating into the references; small leftovers of the restructuring. 11 minor, 13 editorial.
* 19:53 The final fix-and-rebuild agent stopped with "out of usage credits" after editing
  `paper/sec_game.tex` only (the paper still builds, 91 pages; PDF and archive of 19:06 unchanged).
* 19:56 After the user's "try again": retry launched as a single agent (effort high), told to check each
  finding against the current text first, to record each handled finding at once in
  `logs/paper/response_to_referees.md`, then rebuild PDF and archive and test the archive.
* 20:50 Final pass finished. All 10 findings of checker X and 22 of 25 of checker Y applied in full, two in
  part, one not (the original texts of Hales–Jewett 1963 and Patashnik 1980 could not be fetched; their
  statements are quoted through secondary sources with "see"; to be consulted by the authors).
  "Proved by hand" was replaced by wording about the nature of the argument; only the Declarations say who
  produced and who checked what. The sentence in the Declarations about agreement among verifiers now
  claims children per round only for the programs that report them.
* 20:54 Coordinator's own check of the final state: `paper/paper.pdf` 92 pages (main part pp. 1–71,
  supplement pp. 72–92), sha256 91dd89e2…9839, no LaTeX warnings, no undefined references; the only
  placeholders are authors/affiliations, name and version of the tools, what the author(s) checked
  personally, license, DOI/URL; no product or model names in the PDF text. Archive unpacked in a scratch
  directory: every manifest entry OK, PDF inside byte-identical to `paper/paper.pdf`.

## Final status of the paper

`paper/paper.pdf` (92 pages) and `dist/ttt6-artifact.tar.gz` (code, certificate, paper sources, referee
reports and responses). Refereed internally in two rounds: five first-round reports (two on the mathematics,
one audit of every number, one on journal standards, one replication from the PDF alone that produced an
independent accepting verifier) and two second-round checks. No report found a false statement or a gap in
a proof. Left for the authors: the placeholders above; consulting the two original sources; choosing a
license; the volume number of one proceedings reference. Nothing was committed, pushed or uploaded.

## Revision 2026-11-16: explicit strategy for the first player

* The user asked whether the strategy is constructive. The paper obtained the first player's non-losing
  strategy from determinacy (Zermelo), which does not exhibit it. Added Remark 5.20 (`rem:first-player`,
  `paper/sec_game.tex`, after Corollary 5.19): the first player plays an arbitrary first stone (the ghost)
  and then follows sigma in a virtual play in which the second player is Maker, exactly as in the proof of
  Proposition 5.18; the same device applies to the 7x7x7 strategy (one sentence added to the proof of
  Corollary 9.3, `paper/sec_discussion.tex`). The remark also states what playing the strategy costs.
* Measured for that statement with `sim/sim.cpp` (one core, position cache off, random opponent): about
  12,000 complete games per second, 38 MB of memory.
* `paper/paper.pdf` rebuilt: 93 pages (main part pp. 1-72, supplement from p. 73), no LaTeX warnings,
  bibliography unchanged; `MANIFEST.sha256` updated for the three changed paper files and this file.

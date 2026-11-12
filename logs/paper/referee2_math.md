# Second-round report, checker X (mathematics added or changed in the revision)

Recommendation: minor revision

## Summary

Checker X (mathematics added or changed in the revision; integrity of the proof of Theorem 1.1). Verdict: every new or changed statement and proof that I checked is correct; no fatal or major finding. Ten minor or editorial findings follow. I did not write logs/paper/referee2_math.md (the harness forbids report files); this structured result is the report. My scripts and logs are in /home/user/ttt6/logs/paper/referee2/ (files x_*; x_run_all.sh reruns everything; about 2 CPU-minutes in total, all with nice -n 10). The y_* files in that directory are not mine. I also deleted a __pycache__ directory there, which may have held another checker's cache. Nothing under paper/ and no project document was touched.

METHOD. I diffed every paper/*.tex against logs/paper/paper_v1/, read the new files in full, re-derived each new proof by hand, and tested on the real geometry with my own code (x_geom.py: 148 lines from parametrizations, 1,152 maps, brute-force canonical form).

(1) Section 7.
- Lemma 7.5 (deleted lines): proof re-derived and correct, including the induction on the order of deletion.
- Proposition 7.3, Hall-violator step: correct. The reachable set R has |N(R)\F| <= 2|R|-1.
- Remark 7.2: correct (one missing clause, finding 5).
- verify/treecheck.cpp, LeafChecker::check (lines 815-1073), read against Section 7.3 and S4.1 steps (1)-(4): the description matches the code, including validateFlow relative to the deleted lines and the literal re-evaluation of false verdicts.
- Brute force (x_leaf.py): 2,000 instances with 3-14 live lines, 11,433 evaluations, against a literal evaluation of Definition 4.7 over all sets of live lines. Algorithm 2 as printed, my own implementation of the deleted-lines variant, and bin/treecheck --leafcheck all agree in verdict and failing condition: 0 mismatches.
- Lemma 7.5 was asserted at 87,834 flows and in 21,589 direct tests of part (b). In 801 cases part (b) fails once D contains a line that is not strong, so the hypothesis is needed.
- Statistics recounted from leaves.txt: 4,063,331 and 869,185 confirmed.

(2) Proposition 4.22: proof correct.
- Exhaustive over all 2^24 busy sets (x_prop422.py): every set with at most 17 cells leaves a live triangle.
- The smallest busy set without a live triangle has 18 cells (exactly 4,096 such sets), so 17 is sharp for the proposition itself (finding 6).
- Figure 2 is correct, including the caption's meeting cells and the remark on the other diagonal (checked by program and on the rendered page).

(3) Section 5.
- Lemma 5.2: proof correct for non-VOID parents, for (a), (b), (c) separately and jointly. Confirmed on 400 random non-VOID states (88,048 children (c), 716,999 children (d), all distinct). For VOID parents each of (a), (b), (c) can fail; the text's example shows (a) only (finding 1).
- Example 5.3: every cell, line, count and table entry confirmed against strategy.tbl with my own canonical form (122 and 104 live lines; 732, 624 and 16,250 children; 650 meeting pairs; none VOID; all named moves; Table 6).
- Table 3, row t=5, recomputed (35 / 824 / 101 / 181).
- Figure 4: both pictures satisfy Definition 5.4 literally, with 11 and 13 stones, and the caption's admissibility claims hold. The minima 11 and 13 were confirmed by enumeration.
- The sentence on x in F, Definition 5.10, Theorems 5.16 and 5.17 (c), Remark 5.15 and the restated proof of Theorem 1.1 are correct.

(4) Section 9.1.
- The normalized-parametrization argument, the count 193, Lemma 9.1 (a)-(d), Lemma 9.2, Corollary 9.3 and Proposition 9.5 were re-derived and are correct. In Proposition 9.5 the bound 46 holds; the true maximum for busy 7-sets is 44.
- Remark 9.4 (ii) describes the script accurately. I did not check Remark 9.4 (iii)'s account of Golomb and Hales against the source.
- scripts/check_7cube_midplanes.py passes on the printed table. It rejects 9 of my 10 mutated copies and accepts the one that only swaps the two cells of an entry (the same pairing), as it should.
- My own parser and rule transcription (x_seven.py) agree with the table.

(5) Appendix A, recomputed with my own code.
- The 4 canonical-form vectors and the quoted file entries with their positions.
- All 26 LEAF vectors with the failing condition. The worst values 80 and 6 (antipode stealing, 8 rounds) are confirmed exactly; 6, 1 and 2 by a bound argument plus a witness search.
- All 8 LOST vectors, by a literal C++ evaluation of Definition 5.4 (x_lost.cpp), including the kind of pattern and the stone counts.
- Byte-level format, well-formedness and the file-derived numbers of Tables 3 and 4 and of Section 6.3.
- The LOST table does not exercise x in F (finding 2).

(6) Supplement: only notation and wording changed in the moved statements (H to Z, R to C labels, "four distinct lines"). Proposition S2.5 (a) confirmed by brute force.

(7) Dependency chain (x_deps.py plus a manual pass).
- Theorem 1.1 uses: Computed Fact 6.1; Definitions 2.7, 3.1, 4.1, 4.4, 4.6, 4.7, 5.1, 5.4, 5.5, 5.8, 5.10; Lemmas 2.1, 2.6 (with 2.2, 2.3, 2.5 for its part (d)), 2.8, 3.2, 3.3, 4.2, 4.3, 4.8, 4.24, 5.6, 5.7, 5.9, 5.12, 5.13, 5.14; Theorems 4.9, 5.16, 5.17; Proposition 5.18; Corollary 5.19; and the two quoted theorems (max-flow min-cut with integrality, determinacy).
- All of these are stated and proved on pages 7-42. No statement or proof in the chain refers to the supplement, and nothing needed was moved or lost.
- Exactly one Computed Fact enters.
- The only cycle in the reference graph (Theorem 5.16 and Theorem 5.17) is a forward pointer, not a logical use.
- Corollary 9.3 adds Lemmas 9.1 and 9.2 and Table 11 (pages 60-61 and 70), all in the main part, and again only Computed Fact 6.1.
- Sections 2 and 3 changed only cosmetically.
- Algorithms 1 and 2 as printed agree clause by clause with Definitions 5.1, 5.4, 5.5, 5.8 and 4.7, and with the restatement of validity in Section 7.1.

## Findings

### 1. [minor] Section 5.2, sentence after the proof of Lemma 5.2, p. 29; paper/sec_game.tex:218-221

The text says that for a VOID P-state statements (a) and (b) can fail, 'for instance if F contains the sets l\{e} and l'\{e}'. That instance makes only (a) fail: for F = (l\{e}) u (l'\{e}) statement (b) still holds (checked by program). Statement (c), which the text does not mention, can fail as well. Each failure needs its own example; all three were confirmed by program (x_sec5.py).

**Fix.** For a \VOID{} P-state all three statements can fail. Statement (a) fails if $F$ contains $l\setminus\{e\}$ and $l'\setminus\{e\}$ for two live lines $l\ne l'$ through a cell $e\notin F$: the pairs $(l,e)$ and $(l',e)$ define the same child. Statement (b) fails if three live lines $a,a_1,b$ pass through a cell $c\notin F$ and $F$ has four cells on $a$ and four cells on $a_1$: then $(\{a,b\},F\cap a,Q_b)$ and $(\{a_1,b\},F\cap a_1,Q_b)$ define the same child. Statement (c) fails if $F\supseteq(l\setminus\{c\})\cup Q_a\cup Q_b$ for three live lines $l,a,b$ through $c\notin F$. This is irrelevant, because the children of \VOID{} states are never considered.

### 2. [minor] Appendix A.6, Table 10, p. 69; paper/app_formats.tex:190-225 (rows 210-218)

A.6 says 'The cell x of a pattern may belong to F', and this was the replication referee's question, but none of the eight vectors tests it. My literal evaluator shows that in rows 1-4, 6 and 8 no fitting pattern has x in F, and in rows 5 and 7 fitting patterns with x not in F exist as well (2 of 4 E2 patterns; 1 of 3 E3 patterns). An implementation that wrongly adds the requirement x not in F therefore reproduces the whole table. Such instances are easy to find: all 108 candidates of x_lost_xinF.py qualify. Example: t = 10, B = 54,55,127,131,133,142,143,157,164,214, F = 0,12,18,90,108,120,144,150,180. This is a P-state shape (|B| = t) with F B-admissible. LOST is true, 8 E2 patterns fit, every one of them has x = (0,0,0) in F, and no E3 pattern fits. bin/treecheck --lostcheck, the lost function of spotcheck.py and the replication verifier all return true on it.

**Fix.** Add a ninth row and a sentence to the caption:
\texttt{10} & \fn{54,55,127,131,133,142,143,157,164,214} & \fn{0,12,18,90,108,120,144,150,180} & true & $\Etwo$; $x\in F$ in every fitting pattern\\
Caption: ``In the last instance every fitting pattern has $x\in F$; an implementation that requires $x\notin F$ returns false there.'' Also add the line to logs/paper/lost_vectors/vectors.txt and rerun run.sh.

### 3. [minor] Section 7.3, text after Lemma 7.5, p. 53; paper/sec_verification.tex:347-350 (definition at 305-309, Lemma 7.5 (a) at 319)

The list of properties 'to be validated behind a true verdict' omits 'every line of T holds at least two cells'. That property is part of the definition of a T-assignment relative to D, and hence of the hypothesis of Lemma 7.5 (a) as stated. The program does check it (validateFlow, verify/treecheck.cpp:804-806, CHECK(cnt[l] >= 2)). As printed, an object with the listed properties need not be a T-assignment relative to D, so part (a) does not apply to it literally. The proof of (a) uses only the total 2|T|+theta, so nothing is wrong mathematically.

**Fix.** Either add the property to the list: ``\dots; every other live line outside $T$ holds exactly two cells; every line of $T$ holds at least two cells; and the lines of $T$ hold $2|T|+\theta$ cells together.'' Or add after the proof of (a): ``The proof of (a) uses only that the lines of $L_D\setminus T$ hold exactly two cells each and that the lines of $T$ hold at least $2|T|+\theta$ cells together.''

### 4. [minor] Section 7.1, p. 48; paper/sec_verification.tex:35-36, next to the new pointer to Remark 7.2 at lines 62-64

The sentence 'Errors in parsing, in the computation of canonical forms, or truncated files can only turn a valid certificate into a rejected one' is false as stated and contradicts item (6) of the list just before it. A canonical-form routine that returns a state which is not an image of its argument under G, or that maps two inequivalent states to one key, makes the verifier skip an obligation. Only the error covered by the new Remark 7.2 (an image that is not the minimal one) is harmless. The sentence is from the first version, but the revision removed the two sentences that qualified it.

**Fix.** Errors in parsing and truncated files can only turn a valid certificate into a rejected one. The same holds for a canonical form that is not minimal, as long as it is the image of the state under some element of $\G$ (\cref{rem:any-image}); a routine that returns anything else falls under (1) or (6).

### 5. [minor] Remark 7.2, p. 50; paper/sec_verification.tex:175-177

The induction 'every state s that was processed is WON' applies the label WON, which Definition 5.5 defines only for states that are not VOID. The proof does not say that processed states are not VOID. This is true, but it needs Lemma 5.7 (a) because r need not be the canonical form.

**Fix.** Insert after ``Indeed,'': ``every state that was processed is the root or $r(s')$ for a child $s'$ that is not \VOID, hence it is not \VOID{} (\cref{lem:game-invariance}\,(a)); and every such state $s$ is \WON, by induction on the rank of $s$ \dots''

### 6. [minor] Section 4.5, text after Figure 2, p. 25; paper/sec_leaf.tex:656-657

The paper says only that 17 'cannot be improved in the first step of the proof'. In fact 17 is best possible for Proposition 4.22 itself. Every triangle contains a face diagonal: the 1,464 triangles are of types axis-axis-face (1,080), axis-face-space (360) and face-face-face (24). So a set of three cells from each of the six classes leaves no live triangle. Exhaustive computation over all 2^24 busy sets (x_prop422.py): the smallest busy set without a live triangle has 18 cells, and there are exactly 4,096 = 4^6 such sets. Optional strengthening; the present sentence undersells the result.

**Fix.** The number $17$ is best possible: three cells from each of the six classes form a set $B$ of $18$ busy cells that meets every face diagonal, and then $\live{B}$ contains no triangle, because every triangle contains a face diagonal. (Three axis lines that meet pairwise have three different directions and pass through one cell; the space diagonals are pairwise disjoint; and if two axis lines met each other in a cell $r$ and a space diagonal in cells $p\ne q$, then $p$ and $q$ would differ in at most two coordinates, whereas two cells of a space diagonal differ in all three.)

### 7. [editorial] Section 5, Roadmap, p. 26; paper/sec_game.tex:20-21

'Sections 5.6 and 5.7 analyze what one move of Maker can do to an admissible position' is inaccurate. Section 5.6 (Proposition 5.11) counts the stones of a pattern and derives that LOST is false for t <= 9; only Section 5.7 analyzes one move of Maker.

**Fix.** \Cref{sec:stones} counts the stones of a pattern, \cref{sec:one-move} analyzes what one move of \Maker{} can do to an admissible position, \cref{sec:soundness} proves soundness, \dots

### 8. [editorial] Appendix A.6, p. 69; paper/app_formats.tex:193 and caption 221-223

The first two vectors ('9 - -' and '10 - -') are not states: B is empty while t is 9 or 10. The caption says an instance is a line 't B F' 'in the notation of the files', but by A.3 such a line is not a well-formed leaf entry (|B| = t is required). A replicator who feeds the vectors through the file parser will reject them. LOST(B,F,t) is defined for all disjoint B, F and t >= 0, so the vectors are legitimate; the text should say so.

**Fix.** Add to the caption: ``$\LOST(B,F,t)$ is defined for arbitrary disjoint sets $B$, $F$ and $t\ge0$ (\cref{def:lost}); the first two instances are not states ($|B|\ne t$) and must be read without the test $|B|=t$ of \cref{ssec:fmt-leaves}.''

### 9. [editorial] Section 7.3, 'The leaf test of the main verifier, and its cost', p. 53; paper/sec_verification.tex:355-356

The quick rejection is justified by Corollary 4.17 (b), which counts unassigned cells. The program evaluates sur_F(S) of the component directly (verify/treecheck.cpp:904-918), and Appendix S4.1 correctly says 'this is a violation of (L3) by definition'. The two places should agree.

**Fix.** \dots leads to the verdict false at once (it violates (L3) of \cref{def:leaf} by definition); \dots

### 10. [editorial] Archive file named in Appendix A.6: logs/paper/lost_vectors/lost_driver.cpp:2

The comment says 'the literal LOST test (Definition 5.2 of the paper)'. In the revised paper Definition 5.2 no longer exists as such; the patterns and LOST are Definition 5.4. The response to the referees says that statement numbers quoted outside the paper were updated; this file, which was new in the revision, was missed. The other revision-era files I searched (flow_counts, revM_checks, check_7cube_midplanes.py) carry current numbers.

**Fix.** Change the comment to '(Definition 5.4 of the paper; Definition 5.2 in the first version, from which the included verifier was written)'.

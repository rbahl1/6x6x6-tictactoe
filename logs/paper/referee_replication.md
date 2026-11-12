# Replication report: "The 6x6x6 tic-tac-toe game is a draw"

Replication of Computed Fact 6.1 from the paper alone (paper.pdf / paper.txt, strategy.tbl, leaves.txt).
No other project file was read; no web search was made.

**Recommendation: replicated.**
My own verifier accepts the certificate, and every number of the proof that the paper prints was reproduced.
The only printed numbers I could not reproduce are two implementation statistics of the authors' main verifier
(77,268 pair flows, 7,391 triple flows); they are not part of the proof.

## 0. What was done

| item | value |
|---|---|
| program | `work/verify.cpp` (706 lines of C++17, g++ 13.3, `-O2`), written from the paper only; helper `work/table6_check.py` |
| own code for | lines, group G, canonical form (brute force over 1,152 maps), admissibility, LEAF (Algorithm 2), LOST (Def. 5.2, literal), Algorithm 1 |
| arithmetic | integers only; visited set and file look-ups keyed by the full state (no hashing of states to short keys) |
| checksums | SHA-256 of both files equal to the values printed in Sections 1.7 and 6.1; byte counts 5,376,504 and 6,445,022 as printed |
| final run | `verify run ../strategy.tbl ../leaves.txt 10` : 42 CPU-seconds, one thread (this includes three pruning variants of the leaf test and the literal LOST test) |
| logs | `work/selftest.log`, `work/leaftable.log`, `work/run.log`, `work/table6.log` |

Soundness-relevant choices in my verifier: children (c), (d) are generated literally from Definition 5.1 (the
stone-bound identities of Section 7.2 are used only as a pre-filter and asserted against the literal union at
every generated child); the void test uses the child's own sets; a void child (a) or (i) is an error; every
"true" flow of the leaf test (the pairing and all 4,734,197 single-line and triple flows) is checked literally
as in Remark 7.3; LOST is evaluated at all 46,412 expanded states with a literal search for E2 and E3 patterns.

## 1. Outcome

**ACCEPT** for Rmax = 10. (Also: Rmax = 9 is rejected, Rmax = 11 is accepted, as Section 7.5 reports.)

Per round, mine = paper in every cell of Table 3 / Appendix A.5:

| t | P-states | leaves | expanded | Q-states | threat (c) | danger (d) | void (ii) |
|---|---|---|---|---|---|---|---|
| 0 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |
| 1 | 2 | 0 | 2 | 2 | 0 | 0 | 0 |
| 2 | 5 | 0 | 5 | 5 | 0 | 0 | 0 |
| 3 | 14 | 0 | 14 | 14 | 0 | 0 | 0 |
| 4 | 35 | 0 | 35 | 35 | 0 | 0 | 0 |
| 5 | 181 | 0 | 181 | 101 | 824 | 0 | 0 |
| 6 | 1,718 | 0 | 1,718 | 446 | 4,075 | 0 | 1 |
| 7 | 7,227 | 0 | 7,227 | 3,487 | 14,460 | 0 | 6 |
| 8 | 34,801 | 3,382 | 31,419 | 14,362 | 47,020 | 20,416 | 54 |
| 9 | 93,124 | 87,314 | 5,810 | 62,381 | 72,352 | 6,060 | 88 |
| 10 | 16,027 | 16,027 | 0 | 12,331 | 4,767 | 1,799 | 2 |
| total | 153,135 | 106,723 | 46,412 | 93,164 | 143,498 | 28,275 | 151 |

### Matches (all reproduced exactly)

- States visited 246,299 = 153,135 P-states (46,412 expanded, 106,723 leaves) + 93,164 Q-states.
- Table 3 / A.5: all 77 per-round entries and all totals.
- Children generated a/c/d/i/ii = 46,412 / 143,498 / 28,275 / 93,164 / 54,482; 151 void, all of type (ii); no void child of type (a), (c), (d), (i).
- 54,482 Q-states have |F| < t.
- Table 4 (leaves by t and theta): all 27 cells; column totals 64,841, 32,703, 7,267, 1,434, 343, 105, 25, 4, 1; exactly one leaf with theta = 8.
- LEAF holds at all 106,723 leaves (verdict identical with pruning by strong lines, by strong lines and adjacent pairs, by strong lines and all pairs).
- LOST is false at all 46,412 expanded states (literal E2/E3 search).
- Both files: 93,164 and 106,723 lines, all well formed, all canonical under my canonical form, no repeated key, every line reached.
- Non-busy table moves: 434 = 16, 399, 11, 8 in rounds 7, 8, 9, 10; none in rounds 1 to 6.
- Leaves by number of Breaker stones on non-busy cells 0/1/2/3: 15,794 / 84,556 / 6,343 / 30.
- Sample lines of A.3 (lines 1, 2,354, 9,791 of the table) and A.4 (lines 1, 8,185 and the last of the leaf file).
- Table 6: all 21 entries of rounds 1 to 3, and the four round-4 entries quoted in Section 6.6; the file has 2, 5, 14, 35 entries for rounds 1 to 4.
- Line A ends after round 8 in the leaf `8 0,5,61,93,122,154,210,215 -`; Line B ends after round 9 in the last line of the leaf file (followed in canonical frames; the fixed-frame coordinate lists of Section 6.6 were not re-derived).
- Derived numbers in the text that follow from the above: 57 P-states up to round 4, 60.8% of leaves with theta = 0, 106,245 leaves with theta <= 3, 478 with theta >= 4, 1,912 with theta >= 3.
- "869,185 single-line flows" (Section 7.3): reproduced, but only after guessing two shortcuts that the paper does not describe (see finding 1 below).

### Mismatches

- "77,268 flows for pairs and 7,391 flows for triples" (Section 7.3): **not reproduced.** With the second pruning rule applied to the two F-adjacent pairs of a triple I get 76,060 pair flows and 7,591 triple flows; applied to all three pairs, 80,517 and 611. With Algorithm 2 exactly as printed (strong lines only) there are 692,694 triple flows.
- No other mismatch.

## 2. Test vectors (all run before the full run; all reproduced)

Geometry and group (Sections 2, A.1):
- 148 lines = 108 + 36 + 4; two distinct lines share at most one cell; 888 incidences; 24 / 144 / 48 cells on 7 / 4 / 3 lines; the 24 busy ids of A.1; 72 axis lines without a busy cell.
- |G| = 1,152 distinct maps from the 2,304 triples (s, r, pi); every map is a bijection that maps lines onto lines; G is closed under composition (all 1,152^2 products); three cell orbits of sizes 24, 144, 48 with the stated representatives.

Canonical form (A.2): all four vectors, with a fast bitmask implementation and with a literal one (sorted lists); both agree on 300 random pairs and are invariant under G.

T7 / T8 (Examples 3.8, 3.9, Computed Facts 3.10, 4.23): 104 and 100 live lines, slack 1 and 8, pairings exist, L(T7) and L(T8) connected and covering, LEAF(T7,0,1) true, LEAF(T7,0,2) false by (L3), LEAF(T8,0,8) true, LEAF(T8,0,9) false by (L3).

LEAF (Table 11): all 26 instances (11 rows, the fourth standing for 16 instances), including which condition fails first. Example 4.11 (LEAF false at k = 11 by (L3), true at k = 10).

Orbit counts and leaf table (A.7, Table 5, Section 6.4): 527, 1,057, 1,669, 2,468 orbits of busy k-sets (Burnside and enumeration); slack >= k for 6, 947, 2,185 orbits; LEAF for 6, 172, 0 orbits; the six representatives O1..O6 with orbit sizes 288, 576, 288, 576, 576, 36 (2,340 sets); the 172 orbits for k = 9 contain 83,904 sets.

LOST (Proposition 5.9 and the remark after it): on the empty record no pattern fits 10 stones, an E2 pattern fits 11 stones, no E3 pattern fits 11 or 12 stones, an E3 pattern fits 13 stones.

Not from the paper, added as my own safeguard: Algorithm 2 (three pruning variants and the unpruned version) against a direct evaluation of Definition 4.7 over all subsets of live lines on 19,995 random instances with at most 16 live lines (16,325 true, 715 / 2 / 2,953 failing L1 / L2 / L3): no mismatch.

Not attempted: A.8 (minimum number of stones on the boards n = 5, 7, 8), the "worst" column of Table 11 (integer program), Computed Fact 2.9 (full automorphism group), the fixed-frame move lists of Lines A and B.

## 3. Places where the paper was ambiguous, incomplete, inconsistent or wrong

No fatal and no major finding. Nothing had to be guessed for the verifier itself: the first implementation written from the text passed every test vector and reproduced every count without any correction.

| # | where | severity | problem | what I assumed | suggested change |
|---|---|---|---|---|---|
| 1 | Section 7.3, "Cost" (p. 48) | minor (but the only printed numbers that cannot be reproduced from the text) | "869,185 single-line flows, 77,268 flows for pairs and only 7,391 flows for triples" do not follow from Algorithm 2 as printed, which makes 4,063,331 single-line flows (one per live line at each of the 41,882 leaves with theta >= 1) and 692,694 triple flows. 869,185 is exactly the number of live lines with at least theta + 2 cells outside F at the leaves with theta >= 2, so the main verifier evidently skips lines that cannot be strong and treats theta = 1 without single-line flows. Neither shortcut is described. The pair and triple counts could not be matched (76,060 / 7,591 and 80,517 / 611 with two natural readings of the second pruning rule). | Treated as statistics of one program, not as numbers a verifier must reproduce. | State the exact rules behind the three numbers (which lines get a flow, how theta = 1 is handled, which pairs are tested and in which order, whether pairwise adjacent triples are deduplicated), or say explicitly that the numbers depend on undocumented details of treecheck, and give the numbers for Algorithm 2 as printed. |
| 2 | Appendix A.6, Table 11 (p. 62) | minor | Five instances "k B F" are broken over two lines; the second line is the field F, but nothing says so, and it could be read as a continuation of B. The fourth row is a schema for 16 instances. | Second line = F (confirmed by |B| = k in the "antipode stealing" description and by the results). | Print B and F in separate columns, or mark the break. |
| 3 | Definition 5.2 and Appendix A | minor | No test vector for LOST with a positive answer, and LOST is never true in the certified run, so a LOST implementation cannot be validated from the paper beyond "E2 attains 11 stones, E3 needs 13". The definition does not say whether x may belong to F; read literally it may. | Literal reading (x in F allowed, which only makes LOST true more often). | Add a few instances (B, F, t) with LOST true and false for t >= 10, and one sentence on x and F. |
| 4 | Algorithm 1 input, A.3, A.4, Section 7.7 | minor | Algorithm 1 takes "a table" (a map) and "a set", but what a verifier must do with a file that is not a map or not canonical (repeated key with a different move, malformed line, non-canonical line, unreachable line) is left open: A.4 calls the checks "reasonable", Section 7.7 says the main verifier accepts some of these. | Counted malformed, non-canonical and repeated lines and reported them (all zero); they do not enter the visited set. | State the required behaviour: which defects of a file must lead to rejection and which are harmless. |
| 5 | A.3, A.4 | minor | The byte-level format is not specified (line terminator, final newline, encoding). | LF, final newline, ASCII (true for both files). | One sentence. |
| 6 | Algorithm 1, line 25, against Definition 5.6 (R2) and Section 7.2 | minor | Push silently drops any void child, also of type (a) and (i), while (R2) puts child (i) into V unconditionally and the comment says implementations treat a void child (i) as an error. The three agree only via Lemma 4.2. | Void child (a) or (i) = reject (never occurred). | Say in Algorithm 1 that a void child (a) or (i) is an error, or refer to Lemma 4.2 in line 25. |
| 7 | Table 3 caption against A.5 | minor | "with repetitions (counted every time it is generated)" against "for every pair of an expanded state and one of its non-void children (the children of one state are pairwise distinct)". The two agree only because different data (l, e) or (a, b, Qa, Qb) of a non-void parent give different children, which the paper asserts without proof ("Different data may define the same child", Section 5.2, even suggests the opposite). | Counted every generated data tuple; my verifier asserts that no child is generated twice from one state (true at all 46,412 expanded states). | Prove or state the distinctness in one sentence and use one wording. |
| 8 | Section 2.3 and Definition 2.4 (p. 7, 8) | minor | Calligraphic K (busy cells) and italic K (the subgroup G_P G_Pi) are used within the same proof; in the extracted text they are the same letter. | Context. | Rename the subgroup. |
| 9 | Lemma 2.3 (p. 6, 7) | minor | The statement lists the cell counts as 24, 144, 48 (for 7, 4, 3 lines), the proof ends with "24, 48 and 144" in a different order of cases. | Correct in both places, order differs. | Use one order. |
| 10 | Sections 7.4, 8.3 | minor | Timings of the main verifier are inconsistent: 14.8 s on three threads with 8.2 CPU-seconds (Table 7) against 3 s on four threads with 11 CPU-seconds (Section 8.3). | Ignored. | Reconcile or mark as different runs. |
| 11 | paper.txt only | minor | Extraction artefacts: "106 games" for 10^6, the primes of (E3.2) on a separate line ("Sa' subset a \ {c }"). The PDF is correct. | Used the PDF. | None for the paper. |

Explicit answers to the points asked about:

- **Canonical form and order on sets**: unambiguous (Section 2.5, Definition 2.7, A.2 agree; four test vectors). Only sets of equal size are ever compared.
- **File formats**: clear apart from findings 4 and 5.
- **Frame of the table move**: clear (canonical frame of its own state; Section 7.2, A.3).
- **Children (a), (c), (d), (i), (ii)**: Definition 5.1 and Algorithm 1 agree and were implementable literally.
- **VOID**: clear (the child's own F against the child's own B; Definition 4.1 includes F disjoint from B).
- **LOST**: implementable; see finding 3.
- **What is recomputed at a leaf**: clear, LEAF(B, F, k) with k = t (Definition 5.3, Algorithm 1 line 6). A listed leaf is not expanded even if t < Rmax, and no bound on t is required for a leaf; Definition 5.6 and Algorithm 1 agree.
- **F-adjacency, augmenting paths**: clear (Definition 4.6, Definition 4.14, Section 7.3). The search from a set U is a multi-source search; the pairing loop calls Augment twice per live line.
- **Indices, rounds, sizes**: clear (|B| = t for P-states, |B| = t - 1 for Q-states; children counted at the round of the child; void (ii) at the round of the parent, which is the same number).

## 4. The mathematical argument of Sections 2 to 5

I read Sections 2 to 5 while implementing and found no step that I could not follow. Points I checked with some care: Theorem 4.9 (the three cases |S'| = 1, 2, >= 3); Proposition 4.13 (a), which needs the pairing for (B, F); Proposition 4.15 (b) to (e); Proposition 5.9 (confirmed numerically on the empty record: minimum 11 stones for E2, 13 for E3); Lemmas 5.10 to 5.12; the three cases of Theorem 5.14; Lemma 5.7 (a), (b). This was a reading for implementation, not a referee's proof check.

One observation, not an error: the claim "the children of one state are pairwise distinct" (A.5) is true for non-void parents but is not proved in the paper (finding 7); it does not affect soundness.

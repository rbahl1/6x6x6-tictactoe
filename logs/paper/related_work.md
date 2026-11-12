# Related work for "6×6×6 tic-tac-toe is a draw" — literature report

Literature agent, 2026-11-12. Written for the author of the introduction and of the discussion section.
Companion file: `paper/refs.bib` (36 entries, compiles without BibTeX warnings under `amsplain`).

Status tags used below:

* **VERIFIED (source)** — I read the statement in the source itself (PDF text extracted locally); the quote is exact.
* **VERIFIED (secondary)** — I read it in a reliable secondary source that reports the original; quote and URL given.
* **VERIFIED (record)** — bibliographic data only, from the Crossref or arXiv API.
* **UNVERIFIED** — recalled from memory or seen only in a search-engine summary; do not rely on the wording.

Notation: the paper's n^d (side n, dimension d). Hales–Jewett, Erdős–Selfridge and Patashnik write k^n
(side k, dimension n); Golomb–Hales write n^k (side n, dimension k). Quotes keep the source's notation.

---

## 0. Headline findings (read this first)

1. **No prior proof or claim that 6^3 is a draw was found.** Searches of arXiv (API listing of every paper
   matching "tic-tac-toe", "Maker-Breaker", "positional game(s)", "pairing strategy" up to 2026-11-12), web
   search (several phrasings, incl. MathOverflow / Math.StackExchange / GitHub / blogs), Wikipedia (3D
   tic-tac-toe, n^d game), and the papers below turned up nothing. The absence is a search result, not a
   proof of absence.

2. **There is one directly relevant recent paper that the introduction MUST cite and position against:**
   J. W. Cain, I. M. Raymond, N. C. Källersjö, *Cubic tic-tac-toe: A matching-based approach*,
   arXiv:2509.21494v1 [math.CO], 25 Sep 2025 (21 pp., 4 figures; MSC 05D15, 91A46). Key `CainRaymondKallersjo2025`.
   VERIFIED (source), full text read. As of 2026-11-12 the arXiv API shows only v1 and no journal reference;
   Crossref has no record of a published version. It is therefore an **unrefereed preprint**.
   * It proves (computer-assisted) that **Breaker wins the 7^3 Maker–Breaker game**, hence 7^3 is a draw:
     > "Theorem 4.1. Under Maker-Breaker rules, Breaker has a winning strategy in 7^3 tic-tac-toe."
     > "Corollary 4.2. Under standard rules, either player can force a draw in 7^3 tic-tac-toe."
   * Method: Breaker plays 3 or 4 prescribed stones on the space diagonals (or the centre), after which a
     pairing exists for *every* Maker continuation; pairings found by Hopcroft–Karp on
     22,163,142 + 12,824,370 positions, about 5 hours on a laptop. No threats have to be handled because
     after 4 moves each "surviving winning line contains at least 3 unoccupied cells".
   * It states explicitly that **5^3 and 6^3 are open** and that 6^3 might be attackable:
     > Abstract: "it is known that the first player has a winning strategy if n ≤ 4 and that either player can
     > force a draw if n ≥ 8. The question of whether the first player has a winning strategy if n = 5, 6 or 7
     > has remained open. Here, we prove that the first player does not have a winning strategy if n = 7."
     > §6: "... (ii) the only two remaining open cases for the n^3 Maker-Breaker game: n = 5 and n = 6."
     > §6.2: "Now, n = 5 and n = 6 are the only unresolved cases for which it is unknown whether Breaker has a
     > winning strategy in Maker-Breaker tic-tac-toe on an n^3 board. It is believed that in both of these
     > cases, Breaker has a winning strategy."
     > §6.2: "The 6^3 game is not quite as hopeless. The game board has 148 winning lines and 216 cells. Unlike
     > the n^3 boards with n ≥ 3 odd, there is no central cell of degree 13. The largest number of winning
     > lines passing through a cell is 7, and the four superdiagonals do not intersect one another. As in the
     > 7^3 game, in their first four moves, Breaker can arrange to occupy at least one cell on each
     > superdiagonal in such a way that 27 winning lines are eliminated. After both players have made four
     > moves, this means that there are 121 survivors and 208 unoccupied cells, and Breaker is not in imminent
     > danger of losing on the next move. It is possible that Breaker could, within the next few rounds of
     > play, eliminate enough additional winning lines to establish a viable pairing strategy, while making
     > sure to block any potential threats from Maker. With sufficient care beyond the scope of what we are
     > willing to attempt, it may be possible to resolve the 6^3 game computationally."
   * Consequence for our paper: (i) the *idea* "a few Breaker stones, then a pairing, checked by bipartite
     matching" is theirs for 7^3 (and goes back to the folklore 4^2 argument they attribute to D. Galvin,
     1999); our contribution is that for 6^3 this needs a genuinely deeper strategy tree (threats and danger
     pairs must be answered, pairing only after up to R_max rounds, lazy revelation, certificates), exactly
     the "sufficient care" they declined to attempt. (ii) Our theorem settles one of the two cases they list
     as open. (iii) The figures "27 lines eliminated / 121 survivors / 208 cells" are *their* numbers for
     their 4-stone opening on 6^3; I did not re-derive them — check against the project's own data before
     repeating them.

3. **Status of d = 3 in one line each** (details in §c):
   n = 3 first-player win (trivial); n = 4 first-player win (Patashnik 1980; Allis–Schoo 1992 / Allis 1994);
   **n = 5 open, conjectured draw**; **n = 6 open, conjectured draw — until this work**; n = 7 conjectured
   draw until 2025, Breaker win proved in the preprint above; n ≥ 8 draw by an explicit pairing strategy
   (Golomb, 1970; published in Winning Ways 1982 and Golomb–Hales 2002).

4. **A corollary worth a remark (my own derivation from Golomb–Hales, not in the literature in this form):**
   a Breaker win on 6^3 implies a Breaker win on 7^3 by the "central decomposition" in the proof of
   Golomb–Hales Theorem 3. This gives a proof of the 7^3 result that is independent of the preprint. See §c.7.

---

## a. Hales and Jewett (1963)

A. W. Hales, R. I. Jewett, *Regularity and positional games*, Trans. Amer. Math. Soc. 106 (1963), no. 2,
222–229, doi:10.1090/S0002-9947-1963-0143712-1. Key `HalesJewett1963`. VERIFIED (record).
**The full text could not be read** (ams.org returned HTTP 403 to every fetch); all content statements below
are VERIFIED (secondary).

1. **Pairing-strategy draw for large side.** In the paper's notation: n^d is a draw (the second player can
   force a draw by a pairing strategy) if **n ≥ 3^d − 1 for odd n**, or **n ≥ 2^(d+1) − 2 for even n**.
   * VERIFIED (secondary): Erdős–Selfridge 1973, p. 300 (text read,
     https://users.renyi.hu/~p_erdos/1973-10.pdf): "Hales and Jewett [5] investigated n-dimensional
     Tick-Tack-Toe in a hypercube of side k. They proved that, if k ≥ 3^n − 1 (k odd) or k ≥ 2^(n+1) − 2
     (k even), the second player can force a tie, but for each k there is an n_k so that for n > n_k the
     first player can win." (The file is a scan whose text layer loses superscripts; I therefore inspected
     the page image of p. 300: it reads k ≥ 3^n − 1 (k odd) or k ≥ 2^(n+1) − 2 (k even), and "for n ≥ n_k
     the first player can win".)
   * VERIFIED (secondary): Krivelevich 2014 (arXiv:1404.2731, text read), Theorem 2.1: "If n ≥ 3^d − 1, then
     the n^d game is a draw." with the proof idea: "the maximum vertex degree in the game hypergraph of n^d
     is (3^d − 1)/2 for odd n and is 2^d − 1 for even n; then a straightforward application of Hall's theorem
     shows that one can assign to every geometric line l a pair of points B(l) ⊂ l so that the assigned pairs
     are disjoint. ... This is a good example of the so called pairing strategy argument".
   * Search-engine summary of Patashnik 1980 agrees (threshold 3^n − 1 for odd k, 2^(n+1) − 2 for even k,
     via Hall's theorem); Patashnik's text itself was not readable (HTTP 503). UNVERIFIED as a quote.
   * Consistency check (mine): each line has n cells and each cell lies on at most Δ lines, so any m lines
     cover at least mn/Δ cells; Hall's condition for two representatives per line holds if n ≥ 2Δ, i.e.
     n ≥ 3^d − 1 (odd n, Δ = (3^d − 1)/2) or n ≥ 2^(d+1) − 2 (even n, Δ = 2^d − 1). The bounds requested in
     the task statement are therefore correct.
   * **For d = 3 the Hales–Jewett theorem gives only: even n ≥ 14, odd n ≥ 27.** (For d = 2: even n ≥ 6,
     odd n ≥ 9.) The statement "n ≥ 8 is a pairing draw for d = 3" is NOT due to Hales–Jewett 1963; see §b.
2. **First-player win for large dimension.** The Hales–Jewett theorem (every k-colouring of [n]^d contains
   a monochromatic line once d ≥ d0(k, n)) with k = 2 shows there is no drawn final position, and strategy
   stealing then gives a first-player win.
   * VERIFIED (secondary): Krivelevich 2014, Theorem 3.3 and "Corollary 3.4. For every n there exists
     d0 = d0(n) such that for every d ≥ d0 the game n^d is First Player's win." with the comment "which was
     apparently the original motivation behind [43]".
   * Also Erdős–Selfridge quote above ("for each k there is an n_k so that for n > n_k the first player can
     win") and Cain et al. §1.
3. **The Hales–Jewett conjecture.** VERIFIED (source), Golomb–Hales 2002, p. 168: "The Hales–Jewett Conjecture
   is that for every k, when n ≥ 2/(2^(1/k) − 1), ... the second player can force a draw. A stronger conjecture
   would be that for each k, for all n ≥ n_k = ⌈2/(2^(1/k) − 1)⌉, a draw for the second player by 'pairing
   strategy' can be found." Erdős–Selfridge 1973, p. 300, also refer to "their conjecture that the second
   player can force a draw if k > 2(2^(1/n) − 1)^(−1)". For d = 3 the threshold is n ≥ 8, so this
   conjecture says nothing about n = 5, 6, 7. Beck's book has a section "34 The Hales–Jewett Conjecture"
   (pp. 462–469; title VERIFIED from the table of contents; text not read).
4. Strategy stealing for these games is usually credited to Hales–Jewett 1963 for positional games in general
   and to Nash for Hex (Krivelevich 2014: "the so-called strategy stealing principle, observed by Nash";
   Beck 2008, nutshell chapter: "John Nash ... proved, by a pioneering application of the Strategy Stealing
   Argument, that Hex is a first player win"). VERIFIED (source) for the two quotes; that Hales–Jewett 1963
   itself contains the argument is UNVERIFIED (their paper was not readable).

## b. Number of lines; the counting condition for a pairing; d = 3

1. **Number of lines** = ((n+2)^d − n^d)/2. VERIFIED (source): Golomb–Hales 2002, Theorem 1 ("The number of
   winning paths on the n^k hypercube is ½((n + 2)^k − n^k)", two proofs); Erdős–Selfridge 1973, p. 300;
   Cain et al. 2025, Lemma 2.1 (citing Golomb–Hales). For d = 3 this is 3n² + 6n + 4 (Golomb–Hales attribute
   the d = 3 formula to Mercer and Kolb, *Mathematics Teacher* 64 (1971) 119–122; not checked):
   n = 5: 109 lines / 125 cells; **n = 6: 148 / 216**; n = 7: 193 / 343; n = 8: 244 / 512 (recomputed).
2. **Maximum degree.** Odd n: (3^d − 1)/2, attained only at the centre; even n: 2^d − 1, attained at cells
   all of whose coordinates lie in {c, n+1−c} for a common c. VERIFIED (source): Cain et al., Lemma 2.2
   (they point to Beck 2008, p. 54); Krivelevich 2014 (quote in §a). For 6^3: maximum degree 7, attained
   exactly at the 24 cells on the four space diagonals (the paper's "busy cells").
3. **Counting condition.** A pairing needs two private cells per line, so n^d ≥ (n+2)^d − n^d, i.e.
   n ≥ 2/(2^(1/d) − 1). VERIFIED (source): Golomb–Hales, p. 168 and p. 173 ("a necessary condition for a
   pairing strategy to exist is that the number of cells must at least be equal to twice the number of winning
   paths"); Cain et al., eq. (1): "With d = 3, we find that n ≥ 8 is necessary. In particular, when n = 8,
   there are 244 winning lines and 8^3 = 512 cells in the board, and 2 × 244 ≤ 512."
   Numerically 2/(2^(1/3) − 1) = 7.69…; for n = 6: 2·148 = 296 > 216; n = 7: 386 > 343; n = 5: 218 > 125.
   So **no pairing strategy exists on the empty board for n ≤ 7** (d = 3).
4. **n ≥ 8 are pairing draws for d = 3.** VERIFIED (source), Golomb–Hales 2002:
   * p. 172: "For k = 3, the smallest 'board' with a pairing strategy is the 8^3, which has 512 cells and
     ½(10^3 − 8^3) = 244 paths. However, the four 'body diagonals' have 8 cells each, and if we dedicate all of
     these to their respective (non-overlapping) body diagonals, we are left with 512 − 32 = 480 cells, and
     244 − 4 = 240 paths, i.e., exactly two cells available per path." (explicit pairing in their Figs. 5–6.)
   * p. 175: "a pairing pattern on n^k extends to a pairing pattern on (n + 1)^k" (central enlargement), so
     all n ≥ 8 follow.
   * p. 173: "n_1 = 2, n_2 = 5, n_3 = 8, and we have seen that pairing strategies really do exist for these
     values of n_k. At the present time (July, 2000), successful pairing strategies have also been reported
     for the next two cases: n_4 = 11 and n_5 = 14."
   * Attribution (p. 181): "Several results presented at the January, 1972, MAA meeting in Las Vegas, Nevada
     (including the pairing pattern on the 8^3, and how to enlarge a pairing from n^k to (n + 1)^k) were
     contained in a letter dated April 6, 1970, from S. W. Golomb to J. L. Selfridge. Some of this material
     was included in [4]" ([4] = Winning Ways, 1982, "see especially Vol. II, pp. 667–679").
   * Cain et al. §2.5: "See [7] for an explicit example of a pairing strategy by which Breaker can win the 8^3
     game ... Generating an explicit pairing strategy for the 9^3 game is routine ... The explicit pairing
     strategies for the 8^3 and 9^3 games may be used to generate explicit pairing strategies for all n^3
     boards with n ≥ 10".
   * 8^3 also follows from the degree form of Erdős–Selfridge (244 + 7 = 251 < 256), as Cain et al. note (§d.4).
   **Recommended citation for "n ≥ 8": `GolombHales2002` (and `BerlekampConwayGuy2003`), not Hales–Jewett.**

## c. Known values for d = 3

| n | status before this paper | sources (tag) |
|---|---|---|
| 3 | first-player win, elementary; no drawn final position exists | Golomb–Hales p. 170 "for k = 3, no draw is possible on the 3^3 'board'" (VERIFIED source); Beck 2008 p. 1 "The 3 × 3 × 3 Tic-Tac-Toe is a trivial first player win" (VERIFIED source); explicit 4-move win in Cain et al. §2.5 |
| 4 | first-player win, computer proof | Patashnik 1980; Allis–Schoo 1992; Allis 1994 ch. 4 (see c.2) |
| 5 | **open**; conjectured draw | Beck 2008; Golomb–Hales Table 1; Dvořák–Valla 2021; Cain et al. 2025 (see c.3) |
| 6 | **open**; conjectured draw | Golomb–Hales Table 1; Cain et al. 2025 (see c.4) |
| 7 | conjectured draw; Breaker win proved in a 2025 preprint | Golomb–Hales Table 1; Cain et al. 2025 (see c.5) |
| ≥ 8 | draw by pairing strategy | §b.4 |

1. **3^3.** Also Golomb–Hales p. 180: "On the 3^3 board, the central cell is so powerful that if the first
   player is forbidden to occupy it on his initial move, the second player wins by occupying it in reply".
2. **4^3 (Qubic).**
   * Patashnik, *Qubic: 4×4×4 tic-tac-toe*, Math. Mag. 53 (1980), no. 4, 202–216,
     doi:10.1080/0025570X.1980.11976855. VERIFIED (record). Text not readable (503). Content VERIFIED
     (secondary) via Allis 1994, pp. 95–96 (text read): "Oren Patashnik solved qubic in 1977 and his solution
     was confirmed by Ken Thompson (Patashnik, 1980)."; "Using some 1500 hours of cpu time, and 2929 strategic
     moves, qubic was solved. The database with the solution tree has been checked by Ken Thompson, who
     confirmed Patashnik's results." Also Beck 2008, p. 1: "the 4 × 4 × 4 Tic-Tac-Toe is a very difficult first
     player win (computer-assisted proof by O. Patashnik in the late 1970s)"; Golomb–Hales p. 171: "It is
     known [1] that exhaustive computer searching has shown that the 4^3 'board' is in this category
     [draws possible, but first-player win]".
   * Second solution: L. V. Allis and P. N. A. Schoo, *Qubic solved again*, in Heuristic Programming in
     Artificial Intelligence 3 (1992), pp. 192–204 (VERIFIED from the bibliography of Allis 1994); L. V. Allis,
     *Searching for Solutions in Games and Artificial Intelligence*, PhD thesis, Rijksuniversiteit Limburg,
     Maastricht, 1994, ISBN 90-9007488-0, chapter 4 "Qubic" (VERIFIED source; proof-number search plus
     dependency-based search; "The number of automorphisms in qubic is surprisingly high: 192").
   * Wikipedia "3D tic-tac-toe" additionally credits E. D. Mahalko's 1976 M.Sc. thesis (Brigham Young Univ.)
     with a first, non-constructive ("weak") solution. UNVERIFIED beyond Wikipedia; I recommend not citing it.
   * The automorphism group of Qubic (order 192): R. Silver, Amer. Math. Monthly 74 (1967) 247–254
     (VERIFIED record). General n^d: P. Dvořák, T. Valla, *Automorphisms of the cube n^d*, Discrete Math. 344
     (2021) 112234 (VERIFIED record; arXiv:1610.05498v2 read). Useful for `lem:group` / the remark that
     G (order 1152 = 6 · 4 · 48) is the full automorphism group: Dvořák–Valla give "a complete
     characterization" of the automorphism group of n^d for all n, d. I did not extract their theorem
     statement; the section author should check that it yields order 1152 for n = 6, d = 3 before citing it
     for that number (by the analogy with Silver's 192 = 6 · 4 · 8 for n = 4 it should).
3. **5^3: open, conjectured draw.**
   * Beck 2008, "A summary of the book in a nutshell", p. 1 (VERIFIED source, publisher's reading sample and
     https://www.cambridge.org/core/books/combinatorial-games/summary-of-the-book-in-a-nutshell/84BA13FE2F78096C3B1F63E1E56CF656):
     "the 5 × 5 × 5 Tic-Tac-Toe is a hopeless open problem (it is conjectured to be a draw game). Note that
     there is a general recipe to analyze games: perform backtracking on the game-tree (or position graph).
     For the 5 × 5 × 5 Tic-Tac-Toe this requires about 3^125 steps, which is totally intractable."
   * Dvořák–Valla (arXiv:1610.05498v2, 10 Dec 2020; VERIFIED source): "The only (as far as we know)
     non-trivial solved 3-dimensional Tic-Tac-Toe is the game 4^3, which is called Qubic." and "the game 5^3,
     which is the smallest unsolved Tic-Tac-Toe game. While our effort of solving 5^3 is currently not yet
     successful ...".
   * Cain et al. 2025 §6.2 (quote in §0) and "Unfortunately, this [the matching approach] is not feasible for
     the 5^3 game, which has 109 winning lines but only 125 cells."
   * Beck's list of open problems is said to contain "Open Problem 3.2: Is it true that 5^3 Tic-Tac-Toe is a
     draw game? Is it true that 5^4 Tic-Tac-Toe is a first player win?" — **UNVERIFIED** (recalled wording;
     the pages 42–58 and 716–723 of the book were not accessible). A UC Irvine Math Circle handout lists the
     same two questions as unsolved (seen only in a search summary).
4. **6^3: open, conjectured draw; no earlier proof or claim found.**
   * Golomb–Hales 2002, Table 1 "Regions in n−k phase space. (Dotted boundaries and circled numbers are
     uncertain.)" (VERIFIED source; I inspected the page image, p. 176): for dimension k = 3 the entries are
     n = 3: **2**, n = 4: **3**, n = 5, 6, 7: **④ (circled, i.e. uncertain)**, n ≥ 8: **5**. Region 4 is defined
     on p. 171: "Although there is no trivial drawing strategy for the second player (as in region 5, below),
     the second player can always draw with best play." So Golomb and Hales *conjectured* in 2002 that 5^3,
     6^3 and 7^3 are (non-pairing) draws and marked this as uncertain. This is the best pre-2025 source for
     6^3 specifically.
   * Cain et al. 2025: quotes in §0 ("n = 5 and n = 6 are the only unresolved cases", "It is believed that in
     both of these cases, Breaker has a winning strategy", "it may be possible to resolve the 6^3 game
     computationally"). They also report heuristic evidence: under greedy play by both sides the
     Erdős–Selfridge danger drops below 1 after 31 rounds in both 5^3 and 6^3 ("we find that Breaker can
     arrange for D_i < 1 when i = 31") — evidence, not proof.
   * Beck 2008 does not, in the parts I could read, mention 6^3 separately.
   * Wikipedia ("3D tic-tac-toe", "Nd game") says nothing about 5^3, 6^3, 7^3.
5. **7^3.** Conjectured draw (Golomb–Hales Table 1, circled 4). Breaker win proved in Cain et al. 2025
   (preprint, computer-assisted; code at a URL given in the paper; not refereed as of today). The introduction
   should say "proved in a recent preprint", not "it is known".
6. **Monotonicity is not known.** Golomb–Hales §8 state as *hypotheses* (not theorems) that a draw on n^k
   implies a draw on n^(k−1) (Hypothesis 1) and on (n+1)^k (Hypothesis 2). They prove (Theorem 3) that for
   even n Hypothesis 1 implies Hypothesis 2, and remark (Notes 2–3) that "'n_1^k_1 is a draw' implies
   '(n_1 + 2)^k_1 is a draw' whether n_1 is even or odd" (sketch of proof given). VERIFIED (source). So a
   draw for 7^3 does not by itself give anything for 6^3 or 5^3, and the results for n = 5, 6, 7 are
   logically separate problems — except in the direction 6 → 7, see next item.
7. **Observation: 6^3 ⇒ 7^3 (derivation by the literature agent; please check before using).**
   Golomb–Hales, proof of Theorem 3 (p. 179): "the (n_1 + 1)^k_1 hypercube can be decomposed, relative to its
   k_1 mid-hyperplanes (which are unique because n_1 + 1 is odd) into 2^k_1 − 1 hypercubes of side n_1 and
   dimension j, 1 ≤ j ≤ k_1, plus the unique central cell. ... the second player draws by always replying to
   the first player in the same sub-hypercube where the first player has just moved, and using the drawing
   strategy for that sub-hypercube. ... Because n_1 + 1 is odd, every path in the (n_1 + 1)^k_1 hypercube
   requires a winning path in one of the 2^k − 1 sub-hypercubes".
   For n_1 = 6, k_1 = 3: 7^3 minus its centre splits into one 6^3 (216 cells), three 6^2 and three 6^1.
   I checked by a short script (not part of the artifact) that each of the 193 lines of 7^3 contains, after
   deleting its middle cell and closing the gap, a complete line of exactly one of these seven sub-boards.
   In the Maker–Breaker setting the argument is clean: Breaker answers in the sub-board where Maker moved,
   using the 6^3 strategy of this paper, the standard pairing on 6^2 (Golomb–Hales Fig. 4) and the trivial
   reply on a 6-cell line; a move at the centre, or a prescribed cell already owned, gives Breaker a free
   move, which cannot hurt Breaker. Hence **Breaker wins 6^3 ⇒ Breaker wins 7^3**, an independent proof of
   the theorem of Cain–Raymond–Källersjö. (Golomb and Hales phrase Theorem 3 for "draws" of the ordinary
   game; for that reading one needs second-player strategies that stop the first player from ever completing
   a line, which is exactly what a Breaker strategy is.) If the editor uses this, it should be stated as a
   remark/corollary with this two-line proof and attributed to the Golomb–Hales decomposition. The same
   decomposition with the 2·3 outer faces instead of the mid-planes gives 6 → 8 (already known by pairing).

## d. Terminology; Erdős–Selfridge

1. **Strong game vs. Maker–Breaker (weak) game.** VERIFIED (source), Krivelevich 2014 §3–4: in a *strong
   game* "The winner is the first player to occupy completely a winning set A ∈ H; if this has not happened
   till the end of the game, it is declared a draw. Tic-Tac-Toe and n^d are games of this type."; in the
   *Maker–Breaker game* "Maker wins if in the end of the game he has occupied fully a winning set A ∈ H,
   Breaker wins otherwise ... We assume that Maker moves first, unless said otherwise." and "if Breaker wins
   against Maker on H, then Second Player draws in the corresponding strong game, using the same strategy.
   ... Sometimes Maker-Breaker games are also called weak games". Also: "if Maker wins the game played on H
   as the second player, then he also has a winning strategy as the first player; an analogous statement is
   of course valid for Breaker as well."
2. **Weak Win / Strong Draw (Beck).** VERIFIED (source), Beck 2008, pp. 2–5: "it is reasonable, therefore,
   to call 'doing it, but not necessarily first' a Weak Win. If a player fails to achieve a Weak Win, we say
   the opponent forced (at least) a Strong Draw."; "A Weak Win Game, or simply a Weak Game, is played on an
   arbitrary finite hypergraph, the two players are called Maker and Breaker". In this language our theorem
   says: **6^3 is a Strong Draw (Breaker, moving second, wins the weak game), although it is not a pairing
   strategy draw on the empty board.** The term "pairing strategy draw" is used by Beck (UNVERIFIED for the
   exact pages) and, VERIFIED (source), by Mukkamala–Pálvölgyi 2010 / Kruczek–Sundberg 2008 ("pairing
   strategy draw", "Breaker can force a draw using a pairing strategy") and by Golomb–Hales ("draw by
   'pairing strategy'", Region 5).
3. **Strategy stealing.** VERIFIED (source): Krivelevich 2014, Theorem 3.1 "In a strong game played on (X, H),
   First Player can guarantee at least a draw." (proof by strategy stealing); Cain et al., Observation 3,
   state the form we need ("If under standard rules the second player has a strategy ... that is guaranteed
   to prevent the first player from winning, then the first player can adopt a minor adaptation ... meaning
   that both players can force a draw"). Careful wording for our paper: Breaker's strategy shows the second
   player never loses; strategy stealing shows the first player never loses; hence the value is a draw.
4. **Erdős–Selfridge criterion.** P. Erdős, J. L. Selfridge, *On a combinatorial game*, J. Combin. Theory
   Ser. A 14 (1973) 298–301. VERIFIED (source and record).
   * Original form: "THEOREM. m*(n) = 2^(n−1)", where m*(n) is the least number of n-sets for which the
     first player has a winning strategy; i.e. with fewer than 2^(n−1) winning sets of size n the second
     player can stop the first player. The general form (p. 299, page image inspected): "Let {A_k} be a
     family of sets |A_i| = n_i for which Σ_i 1/2^(n_i) < 1. Then the next player has a strategy which forces
     a draw. On the other hand if integers n_i are given for which Σ_i 1/2^(n_i) ≥ 1/2, we can find sets
     A_i, |A_i| = n_i, so that the first player has winning strategy." Here "the next player" is the player
     about to move, i.e. the blocker moving first: bound 1 if Breaker moves first, hence bound 1/2 if Maker
     moves first (Maker's first move at most doubles the sum).
   * Modern statement, VERIFIED (source), Krivelevich 2014, Theorem 4.2 (Maker moves first): "Let (X, H) be a
     hypergraph. If Σ_{A∈H} 2^(−|A|) < 1/2, then H is Breaker's win." This is the form relevant to our
     theorem (Breaker is the second player).
   * Degree form for uniform hypergraphs, VERIFIED (source) as quoted by Cain et al., Theorem 6.1 (citing
     Beck 2008, Section 10): "If Δ + |E(H)| < 2^n, then Breaker has a winning strategy." (n-uniform, Δ the
     maximum degree, Breaker second).
   * **Application to 6^3 (recomputed): Σ 2^(−6) over 148 lines = 148/64 = 2.3125 ≥ 1/2, and
     Δ + |E| = 7 + 148 = 155 ≥ 64 = 2^6. Neither form applies** (not even the first-player bound 1). For
     comparison: 5^3: 109/32 = 3.41; 7^3: 193/128 = 1.51 (Cain et al.: "D_0 = 193/128"); 8^3: 244/256 = 0.95
     (basic form fails, degree form 251 < 256 succeeds).
   * Erdős–Selfridge for n^d in general: "our theorem immediately implies that the second player can force a
     draw if k > c n log n" (their p. 300; side k, dimension n).
   * Beck's stronger asymptotic results (draw if d = O(n²/log n), weak win if d = Ω(n²)) are reported by
     Golomb–Hales p. 181 (VERIFIED source for the report; originals not read). They are asymptotic and say
     nothing about 6^3.

## e. Related computer-assisted results in positional games (context only)

All entries below are for one sentence of context; none is used in the proof.

* Qubic 4^3: Patashnik 1980; Allis–Schoo 1992; Allis 1994 (see c.2). Patashnik's proof was "a combination of
  human expert knowledge and a standard search algorithm" with 2929 hand-chosen strategic moves (Allis 1994,
  p. 96, VERIFIED source).
* 7^3: Cain–Raymond–Källersjö 2025 (matching-based, 3.5·10^7 positions) — the closest relative of our method.
* Connect-Four (7×6): first-player win; J. D. Allen 1989 and L. V. Allis, M.Sc. thesis 1988 (both VERIFIED
  from the bibliography of Allis 1994; keys `Allen1989`, `Allis1988`).
* Go-Moku (15×15, free-style): first-player win; Allis, van den Herik, Huntjens, Comput. Intell. 12 (1996)
  7–23 (VERIFIED record); threat-space search and proof-number search (Allis, van der Meulen, van den Herik,
  Artificial Intelligence 66 (1994) 91–124, VERIFIED record).
* Survey of solved games: van den Herik, Uiterwijk, van Rijswijck, *Games solved: Now and in the future*,
  Artificial Intelligence 134 (2002) 277–311 (VERIFIED record; text not read).
* Checkers: draw; Schaeffer et al., Science 317 (2007) 1518–1522 (VERIFIED record).
* m,n,k-games: Uiterwijk and van den Herik, *The advantage of the initiative*, Inform. Sci. 122 (2000) 43–58
  (VERIFIED record); (7,7,5) and (8,8,5) are draws: Hsu, Ko, Chen, Wei, Hsueh, Wu, Theoret. Comput. Sci. 815
  (2020) 79–94 (VERIFIED record).
* k-in-a-row on the infinite board: 9-in-a-row is a draw by a Hales–Jewett pairing, 8-in-a-row is a draw
  (T. G. L. Zetters; Guy–Selfridge–Zetters, Amer. Math. Monthly 87 (1980) 575–576), 5-in-a-row is a
  first-player win (via Go-Moku), 6 and 7 are open — VERIFIED (secondary): N. Do, "How to Win at
  Tic-Tac-Toe", Austral. Math. Soc. Gazette 32 (2005), Mathellaneous column (text read). Not in refs.bib
  except the Monthly item.
* Pairing strategies with other line systems: Kruczek–Sundberg, Electron. J. Combin. 15 (2008) N42;
  Mukkamala–Pálvölgyi, Electron. J. Combin. 17 (2010) N33 (both VERIFIED record). Computer-found
  generalisations of pairings for proving draws in k-in-a-row positions ("set matching", fewer than two
  markers per line): Uiterwijk, ICGA J. 40(3) 129–148 (VERIFIED record; conference version arXiv:1703.10678
  read). The latter is conceptually close to our leaf certificates (a draw proof by a matching-type
  certificate instead of search) and deserves one sentence.
* Hex on small boards (8×8: Henderson, Arneson, Hayward, IJCAI 2009; 9×9: Pawlewicz, Hayward 2013):
  **UNVERIFIED** (memory only; Crossref lookups failed). Not in refs.bib; either drop Hex or cite
  `vandenHerikUiterwijkvanRijswijck2002` generically.
* D. Galvin, "4^2 tic-tac-toe is a draw" (1999, unpublished note; cited by Cain et al. as [6]) — the
  one-stone-then-pairing argument for 4^2. Not in refs.bib (unpublished, not seen).

## f. Standard tools

| tool | reference | key | tag |
|---|---|---|---|
| Hall's theorem | P. Hall, J. London Math. Soc. 10 (1935) 26–30, doi:10.1112/jlms/s1-10.37.26 | `Hall1935` | VERIFIED (record) |
| Kőnig's theorem | D. Kőnig, Mat. Fiz. Lapok 38 (1931) 116–119 | `Konig1931` | UNVERIFIED (memory; standard) |
| max-flow min-cut, integrality | Ford, Fulkerson, Canad. J. Math. 8 (1956) 399–404; *Flows in Networks*, Princeton UP 1962 | `FordFulkerson1956`, `FordFulkerson1962` | VERIFIED (record) for the paper; book: year/publisher confirmed via a 1964 review record |
| bipartite matching algorithm | Hopcroft, Karp, SIAM J. Comput. 2 (1973) 225–231 | `HopcroftKarp1973` | VERIFIED (record) |
| textbook | Schrijver, *Combinatorial Optimization: Polyhedra and Efficiency*, Springer 2003; Lovász, Plummer, *Matching Theory*, North-Holland 1986 | `Schrijver2003`, `LovaszPlummer1986` | Schrijver: UNVERIFIED (memory; series "Algorithms and Combinatorics 24"); Lovász–Plummer: VERIFIED (record), series volume number 121 from memory |
| HiGHS | Huangfu, Hall, *Parallelizing the dual revised simplex method*, Math. Program. Comput. 10 (2018) 119–142, doi:10.1007/s12532-017-0130-5 | `HuangfuHall2018` | VERIFIED (record) |
| determinacy of finite games | Zermelo 1913 (cited in PROOF.md) | `Zermelo1913` | UNVERIFIED (memory; standard) |

The "bigamy" form of Hall's theorem used for pairings (two representatives per line) is stated by Cain et al.
as Proposition 5.1 with attribution "Theorem 1.4 in [Beck, *Ramsey Games*, Discrete Math. 249 (2002) 3–30]"
and "Beck refers to this result as the 'bigamy corollary' of Hall's theorem" (VERIFIED source for the
attribution; Beck's texts not read). PROOF.md cites Ford–Fulkerson 1956 for max-flow/min-cut; the OUTLINE's
provisional key is `FordFulkerson1962`; both keys exist.

---

## Recommended wording for the novelty claim

Defensible text (adapt freely; every clause is backed by a VERIFIED item above):

> For d = 3 the first player wins for n ≤ 4 — trivially for n ≤ 3, and by Patashnik's computer-assisted
> proof for Qubic, n = 4 \cite{Patashnik1980}, later reproduced by Allis and Schoo
> \cite{AllisSchoo1992,Allis1994} — while for n ≥ 8 the second player draws by an explicit pairing strategy
> \cite{GolombHales2002,BerlekampConwayGuy2003}. A counting argument shows that no pairing strategy exists
> on the empty board for n ≤ 7. The cases n = 5, 6, 7 have long been expected to be draws: Golomb and Hales
> \cite{GolombHales2002} place them, marked as uncertain, in the region where the second player draws
> without a pairing strategy, and Beck \cite{Beck2008} calls 5 × 5 × 5 "a hopeless open problem (it is
> conjectured to be a draw game)". Recently Cain, Raymond and Källersjö \cite{CainRaymondKallersjo2025}
> gave, in a preprint, a computer-assisted proof that Breaker wins the 7^3 Maker–Breaker game: after three or
> four prescribed Breaker moves a pairing strategy exists against every continuation. They list n = 5 and
> n = 6 as the two remaining open cases and remark that for 6^3 "it may be possible to resolve the 6^3 game
> computationally". The present paper does this. To our knowledge it is the first proof that 6^3 is a draw.
> The situation differs from 7^3, where three or four opening moves suffice and no threat can arise before
> the pairing takes over: on 6^3 at least seven Breaker stones are needed before any pairing can exist,
> Maker's threats have to be answered on the way, and in our certificate the pairing is reached only after
> up to ten rounds, along a strategy tree that we certify.

(The last sentence uses project facts — "fewer than 7 Breaker stones never admit a pairing", R_max = 10 —
taken from `paper/OUTLINE.md`; the editor must check them against PROOF.md / REPORT.md. The statement about
7^3 is from Cain et al., Step 1, Remark (ii).)

Rules for the editor:

* Say "to our knowledge" and "we found no earlier proof or claim"; do not say "famous open problem" or
  "open since 1963" (Hales–Jewett's paper was not readable; the earliest VERIFIED statement that singles
  out 6^3 is Golomb–Hales 2002, Table 1, and the first explicit "open" statement is Cain et al. 2025).
* Refer to the 7^3 result as a preprint. If the editor wants independence from it, use the remark of §c.7
  (6^3 ⇒ 7^3 via the Golomb–Hales decomposition) after checking it.
* After this paper, and granting 7^3 (preprint, or §c.7), **5^3 is the only n^3 game whose outcome is
  unknown**; say "to our knowledge" here too.
* Attribute "n ≥ 8" to Golomb (1970) / Winning Ways / Golomb–Hales, and the general bounds
  n ≥ 3^d − 1 (odd), n ≥ 2^(d+1) − 2 (even) to Hales–Jewett. For d = 3 the latter give only n ≥ 14 (even)
  and n ≥ 27 (odd).
* Erdős–Selfridge does not apply to 6^3 (148/64 = 2.3125 ≥ 1/2; 148 + 7 = 155 ≥ 64).
* Our result is a Breaker win in the Maker–Breaker game with Breaker moving second ("Strong Draw" in Beck's
  terminology), which is stronger than "the ordinary game is a draw".

## What I could NOT verify

1. **Hales–Jewett 1963, full text** (HTTP 403 from ams.org). The pairing bounds and the Ramsey corollary are
   quoted from Erdős–Selfridge 1973 and Krivelevich 2014; whether the 1963 paper itself states the conjecture
   n ≥ 2/(2^(1/d) − 1), the strategy-stealing argument, or anything about small cases is unverified.
2. **Patashnik 1980, full text** (HTTP 503 / TLS failure at the only free copy). Bibliographic data are from
   Crossref; content via Allis 1994, Beck 2008, Golomb–Hales 2002. A search summary says Patashnik restates
   the Hales–Jewett bounds; not quotable.
3. **Beck 2008 beyond pp. 1–14 and the table of contents** (cambridge.org asset server unreachable/403; a
   course copy returned 503). In particular unverified: the wording and number of "Open Problem 3.2"
   (5^3 draw? 5^4 win?), the content of Section 3 "Examples: Tic-Tac-Toe games" (pp. 42–58), Section 12
   "Ramsey Theory and Hypercube Tic-Tac-Toe", Section 34 "The Hales–Jewett Conjecture" (pp. 462–469), Beck's
   statement of the pairing theorem, any table of small cases, and whether the book mentions 6^3 or 7^3.
   If the editor has the book, check p. 54 (degrees), Section 3 and the open-problem list (pp. 716–723).
4. **Winning Ways, chapter 22 "Lines and Squares"** — text not read. That it contains the 8^3 pairing rests
   on Golomb–Hales ("Some of this material was included in [4]") for the 1982 edition (Vol. 2, pp. 667–679).
   Place of publication (Natick, MA) and the page range in the 2003 edition (believed to start at p. 731)
   are from memory; no pages are given in refs.bib.
5. **Hefetz–Krivelevich–Stojaković–Szabó 2014** — bibliographic data VERIFIED (record), text not read; I do
   not know what it says about 5^3–7^3. Krivelevich's 2014 ICM survey (read) says only that resolving n^d
   "for all pairs (d, n) appears to be well out of reach" and treats d = 2 and the two extremes.
6. **van den Herik–Uiterwijk–van Rijswijck 2002** — text not read (record only).
7. **Mahalko 1976** (Qubic) — Wikipedia only. **Hex references** — memory only. **Galvin 1999** — not seen.
8. **Kőnig 1931, Schrijver 2003, Zermelo 1913** — standard data from memory, not re-checked online;
   `LovaszPlummer1986` volume number and `FordFulkerson1962` details partly from memory.
9. **Uiterwijk's ICGA Journal article**: Crossref gives year 2019 for vol. 40 no. 3 (used in refs.bib); some
   citations give 2018.
10. **Cain et al. 2025**: I read the paper but did not examine its code or re-check its computation; its
    numbers for 6^3 ("27 winning lines are eliminated", "121 survivors and 208 unoccupied cells") are quoted,
    not confirmed. It is unrefereed.
11. OEIS and MathOverflow/Math.StackExchange: no dedicated entry or thread on the outcome of n^3 games was
    found by search; I did not browse those sites exhaustively.
12. Negative result of the prior-art search (no earlier 6^3 proof): based on the queries listed below; grey
    literature (theses, non-indexed web pages, non-English sources) may have been missed.

## Keys in refs.bib

Requested keys (all present): `HalesJewett1963`, `Patashnik1980`, `Allis1994`, `Beck2008`, `GolombHales2002`,
`BerlekampConwayGuy2003`, `Hall1935`, `FordFulkerson1962`, `HopcroftKarp1973`, `Schrijver2003`,
`ErdosSelfridge1973`, `HuangfuHall2018`, `HefetzKrivelevichStojakovicSzabo2014`.

Added keys:

* **`CainRaymondKallersjo2025`** — must be cited (7^3 preprint; states 6^3 open).
* `AllisSchoo1992` (Qubic solved again), `Silver1967`, `DvorakValla2021` (automorphism groups),
  `Krivelevich2014` (survey; source of several definitions quoted above), `Beck1981`, `Paul1978`.
* `KruczekSundberg2008`, `MukkamalaPalvolgyi2010`, `Uiterwijk2019` (pairing strategies and extensions).
* `vandenHerikUiterwijkvanRijswijck2002`, `AllisvandenHerikHuntjens1996`, `AllisvanderMeulenvandenHerik1994`,
  `Allis1988`, `Allen1989`, `SchaefferEtAl2007`, `UiterwijkvandenHerik2000`, `HsuEtAl2020`,
  `GuySelfridgeZetters1980` (context, §e).
* `FordFulkerson1956` (the paper PROOF.md cites), `Konig1931`, `LovaszPlummer1986`, `Zermelo1913`.

Each entry in refs.bib has a comment line recording how it was checked ([CR] Crossref record, [SRC] source
read, [SEC] taken from a reference list that was read, [MEM] memory). Uncited entries are harmless with
BibTeX. The file was test-compiled (`\nocite{*}`, `amsplain`, pdflatex + bibtex): 36 entries, no warnings.

## Sources read in full or in the relevant part (URLs)

* Golomb, Hales, Hypercube tic-tac-toe — https://library.slmath.org/books/Book42/files/golomb.pdf
* Cain, Raymond, Källersjö — https://arxiv.org/abs/2509.21494 (PDF v1)
* Erdős, Selfridge — https://users.renyi.hu/~p_erdos/1973-10.pdf
* Beck, reading sample "A summary of the book in a nutshell" —
  https://beckassets.blob.core.windows.net/product/readingsample/779369/9780521461009_excerpt_001.pdf ;
  table of contents — https://toc.library.ethz.ch/objects/pdf/z01_978-0-521-46100-9_01.pdf ;
  chapter page — https://www.cambridge.org/core/books/combinatorial-games/summary-of-the-book-in-a-nutshell/84BA13FE2F78096C3B1F63E1E56CF656
* Krivelevich, Positional games — https://arxiv.org/abs/1404.2731
* Allis, PhD thesis — https://project.dke.maastrichtuniversity.nl/games/files/phd/SearchingForSolutions.pdf
* Dvořák, Valla — https://arxiv.org/abs/1610.05498
* Uiterwijk, Set matching — https://arxiv.org/abs/1703.10678
* N. Do, How to win at tic-tac-toe — https://archive.austms.org.au/Publ/Gazette/2005/Jul05/mathellaneous.pdf
* Wikipedia: https://en.wikipedia.org/wiki/3D_tic-tac-toe , https://en.wikipedia.org/wiki/Nd_game ,
  https://en.wikipedia.org/wiki/Hales%E2%80%93Jewett_theorem ,
  https://en.wikipedia.org/wiki/Combinatorial_Games:_Tic-Tac-Toe_Theory
* Bibliographic records: api.crossref.org (by DOI / title), export.arxiv.org API.

Queries were generic bibliographic or topical strings only (e.g. "5x5x5 tic-tac-toe draw proof 6x6x6 7x7x7
open problem n^d game", "Hales Jewett Regularity and positional games 1963 pairing strategy", "Beck
Tic-Tac-Toe Theory open problem 5^3"); nothing about this project, its methods, files or people was sent.

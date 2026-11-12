# SPEC.md — frozen interface for the 6×6×6 draw proof

Status: FROZEN by the coordinator. If you believe something here is wrong, do not silently deviate:
write the issue to `logs/spec_issues.md` (append) and report it in your final answer.

Project root: `/home/user/ttt6`. No git, no pushes, no network uploads. 4 cores, 15 GB RAM, shared by
all agents: prefix every job that runs longer than a few seconds with `nice -n 10`, keep unit tests under
about one CPU-minute, and never start more than 2 heavy processes yourself unless your task says so.

Build: `make bin/<name>` compiles `src/<name>.cpp`, `verify/<name>.cpp` or `sim/<name>.cpp`
(g++ 13, `-O2 -std=c++17 -march=native -fopenmp`). Do not edit the `Makefile` or `src/geom.hpp`.
Python: 3.13 with `highspy` 1.16 installed (no numpy/scipy).

## 1. Board

* Cell (x,y,z) ∈ {0..5}³ has id `36x + 6y + z` (0..215). All files and all text formats use cell ids.
* Lines: all sets {(f1(t),f2(t),f3(t)) : t=0..5}, each fi a constant, t, or 5−t, not all constant; duplicates
  removed. 148 lines (108 axis, 36 face diagonals, 4 space diagonals). Two cells share at most one line.
* Busy cells: the 24 cells with min(x,5−x) = min(y,5−y) = min(z,5−z). Each is on 7 lines, every other
  cell on 3 or 4. antipode(x,y,z) = (5−x,5−y,5−z).
* Symmetry group G (1152 maps): coordinate permutation, then reversal of a subset of coordinates, then one
  relabelling s of {0..5} with s(5−i) = 5−s(i) applied to every coordinate. Every map sends lines to lines.
* Round t: the first player (FP) moves, then the second player (SP). After round t each has t stones.

## 2. `src/geom.hpp` (frozen, already written and tested)

`namespace ttt`. Constants `NC=216, NL=148, NB=24, NG=1152`; `cid(x,y,z)`, `cx,cy,cz`.

* `struct CellSet` — 4×u64 bitset: `test,set,reset,count,empty,intersects,subset_of,|,&,minus,==,for_each,cells()`.
* 2.4 Set order `cmp(a,b)`: let p be the smallest cell id whose membership differs; the set that contains p is
  the smaller. For sets of equal size this is the lexicographic order of the ascending id lists.
* `const Geom& G()` with `line[l][6]` (ascending cells; lines sorted lexicographically, ids 0..147),
  `lineSet[l]`, `nlc[c]`, `lc[c][7]`, `lineOf[c1][c2]` (line through two cells or −1), `busy[24]` (ascending),
  `busyIdx[c]`, `busySet`, `antip[c]`, `gmap[g][c]` (image of a cell), `gline[g][l]`, `ginv[g]`, `image(g,set)`.
* `to_str(CellSet)` / `parse_set(string)`: ascending ids joined by commas, `-` for the empty set.

Line ids and symmetry indices are internal to C++ programs that include `geom.hpp`. Interchange files never
contain them (lines are written as their 6 cell ids).

## 3. Definitions

B = SP stones, X = FP stones, B∩X = ∅. F ⊆ X = the *revealed* FP stones.

* Live line: a line with no cell of B. L(B) = set of live lines.
* N(S) = union of the cells of the lines in S (S ⊆ L(B)); it never meets B.
* X is **B-admissible** iff X∩B = ∅, every live line has at most 4 cells of X, and there is no *danger pair*:
  two distinct live lines, each with exactly 4 cells of X, sharing a cell that is not in X.
  (Subsets of admissible sets are admissible. Admissible w.r.t. B implies admissible w.r.t. any B' ⊇ B with B'∩X=∅.)
* A **pairing** for (B,X): an assignment of 2 cells of l∖X to every live line l, no cell assigned twice.
  It exists iff |N(S)∖X| ≥ 2|S| for every S ⊆ L(B) (Hall).
* slack(B) = (216 − |B|) − 2·|L(B)|.
* **C(B,F,k)**: every B-admissible X with F ⊆ X and |X| ≤ k has a pairing.
  worst(B,F,k) = max over such X and over S ⊆ L(B) of 2|S| − |N(S)∖X| (≥ 0, S = ∅). C ⇔ worst ≤ 0.
* sur_F(S) = |N(S)∖F| − 2|S|.
* Two live lines are **F-adjacent** iff they share a cell that is not in F. A set S of live lines is
  **F-connected** iff the F-adjacency graph on S is connected.
* **C_Φ(B,F,k)**: every F-connected set S of at least 3 live lines has sur_F(S) ≥ k − |F|.
* **LEAF(B,F,k)** (the predicate every program implements; k ≥ |F|):
  1. F is B-admissible, and
  2. a pairing for (B, X=F) exists, and
  3. C_Φ(B,F,k).

Facts (proved in PROOF.md; do not re-prove in code):

* LEAF(B,F,k) ⇒ C(B,F,k). (A Hall violator can be taken connected through cells outside X; with 1 line it
  breaks the cap, with 2 lines it is a danger pair or breaks the cap, with ≥3 lines it contradicts C_Φ.)
* Monotone: LEAF(B,F,k) ⇒ LEAF(B,F',k) for admissible F' ⊇ F (|F'| ≤ k); ⇒ LEAF(B,F,k') for |F| ≤ k' ≤ k;
  ⇒ LEAF(B∪{b},F,k) for b ∉ F.
* For F = ∅ and k ≤ 10, C(B,∅,k) ⇔ LEAF(B,∅,k) (a connected triple always admits 9 or 10 admissible stones).
  In general LEAF is only sufficient for C.
* Given 1 and 2, condition 3 is equivalent to: for every F-connected **triple** T of live lines,
  μ(T) := min over all S ⊇ T (S ⊆ L(B)) of sur_F(S) ≥ k − |F|. μ(T) is a max-flow: take a pairing for
  (B,F) (each live line holds 2 cells), then let the three lines of T take arbitrarily many cells; μ(T) is the
  number of additional cells that can be routed (augmenting paths). All arithmetic is on small integers.
  Valid pruning: μ({l}) ≥ θ for a line l ∈ T, or μ({a,b}) ≥ θ for two lines a,b ∈ T, implies μ(T) ≥ θ.
  Quick rejection: if the pairing leaves fewer than θ = k−|F| unassigned cells in N(S) for the F-connected
  component S of T, then sur_F(S) < θ.

## 4. The abstract game (lazy revelation)

FP stones that are not in F are *unrevealed*; the abstract game does not know where they are (they may be
on busy cells). u = t − |F|.

* **P-state (B,F,t)**: FP to move; |B| = t, |F| ≤ t, B∩F = ∅. Root: P(∅,∅,0).
* **Q-state (B,F,t)**: SP to move after a non-forcing FP move; |B| = t−1, |F| ≤ t, B∩F = ∅.
* **VOID**: a state whose F is not B-admissible. It cannot arise in real play; it counts as won and is
  neither expanded nor stored.

A non-VOID P-state (B,F,t) is **WON** iff

* (P1) LEAF(B,F,t); or
* (P2) t < R_max, LOST(B,F,t) is false, and every one of the following children is WON or VOID:
  * (a) Q(B, F, t+1).
  * (c) threat: for every live line l and every e ∈ l∖F with |F ∪ (l∖{e})| ≤ t+1:
    P(B∪{e}, F∪(l∖{e}), t+1).
  * (d) danger: for every unordered pair of live lines a ≠ b with a∩b = {c}, c ∉ F, and all Qa ⊆ a∖{c},
    Qb ⊆ b∖{c} with |Qa| = |Qb| = 4, F∩a ⊆ Qa, F∩b ⊆ Qb, |F∪Qa∪Qb| ≤ t+1:
    P(B∪{c}, F∪Qa∪Qb, t+1).

A non-VOID Q-state (B,F,t) is **WON** iff there is a cell m ∉ B∪F (the *table move*) such that

* (i) P(B∪{m}, F, t) is WON, and
* (ii) if |F| < t: Q(B, F∪{m}, t) is WON or VOID   (m turned out to be occupied by an unrevealed FP stone).

Table moves are busy cells by default; a non-busy m is allowed (same rules) but only behind a flag.

**LOST(B,F,t)** is true iff one of the following stone patterns fits. All lines are live w.r.t. B and
distinct, "stone set" means cells the FP would hold after his (t+1)-th move, and the pattern *fits* iff
|F ∪ (all stone sets of the pattern)| ≤ t+1.

* (E2) threat + danger. Lines l, a, b; cells x, e, c pairwise distinct; l∩a = {x}; e ∈ l; a∩b = {c}; x ∉ b;
  e ∉ b; stone sets Sl = l∖{e}, Sa ⊆ a∖{c} with |Sa| = 4 and x ∈ Sa, Sb ⊆ b∖{c} with |Sb| = 4;
  e ∉ F, c ∉ F, F∩a ⊆ Sa, F∩b ⊆ Sb.
* (E3) two dangers. Lines a, a', b, b'; cells x, c, c' pairwise distinct; a∩a' = {x}; a∩b = {c}; a'∩b' = {c'};
  x ∉ b∪b'; stone sets Sa ⊆ a∖{c}, Sa' ⊆ a'∖{c'}, Sb ⊆ b∖{c}, Sb' ⊆ b'∖{c'}, each of size 4, x ∈ Sa∩Sa',
  c and c' in no stone set; c, c' ∉ F; F∩a ⊆ Sa, F∩a' ⊆ Sa', F∩b ⊆ Sb, F∩b' ⊆ Sb'.

(A pattern needs at least 11 resp. 12 distinct stones, so with few unrevealed stones a cheap lower bound
`(5−|F∩l|) + (4−|F∩a|) + (4−|F∩b|) − 2 > t+1−|F|` etc. rejects almost every state immediately.)

The proof obligation: the root is WON for some R_max.

## 5. The real strategy σ (what the simulator plays)

SP keeps (B, F, t) and a mode flag. Initially B = F = ∅, t = 0, mode = TABLE.

At the start of each round (FP to move), in TABLE mode: if the canonical form of P(B,F,t) is in the leaf
file, compute a pairing for (B, X) with the real X (it must exist — otherwise report a certificate bug)
and switch to PAIRING mode.

FP places x; t ← t+1. Then:

* PAIRING mode: if x is a cell assigned to a line, take the other cell assigned to the same line; otherwise
  take any empty cell (e.g. the smallest id). (Lines containing an SP stone are dead and need nothing.)
* TABLE mode:
  1. Threat: if a live line has 5 cells of X, let e be its sixth cell: SP takes e;
     F ← F ∪ (the 5 X-cells of that line); B ← B∪{e}. Next round.
  2. Danger: else if there are danger pairs (live lines a,b with exactly 4 cells of X each, sharing an empty
     cell c): choose the pair with the smallest c (then smallest line cell lists); SP takes c;
     F ← F ∪ (X∩a) ∪ (X∩b); B ← B∪{c}. Next round.
  3. Otherwise look up the canonical form of Q(B,F,t) in the strategy table, map the move back to real
     coordinates: m. If m ∈ X (occupied): F ← F∪{m} and repeat step 3. Else SP takes m; B ← B∪{m}.
     A missing table entry is a *strategy hole* (a bug in the proof tree) and must be reported.
* After SP's move in TABLE mode the real X must be B-admissible; if not, report a LOST-pattern bug.

The game ends when the board is full or the FP owns a whole line (which must never happen).

## 6. Canonical forms and files

* Canonical form of a state (B,F,t): among the 1152 image pairs (g(B), g(F)) the smallest one, comparing the
  B-images with the order of 2.4 first and the F-images second. Any g attaining it may be used to translate
  moves: real move = g⁻¹(table move).
* Instance line (input of all checkers): `k B F` — three space-separated fields, B and F as cell lists
  (`0,215,34` ascending, or `-`).
* `strategy.tbl`: one line per WON Q-state in canonical form: `t B F m` (m = table move, a cell id in the
  canonical frame).
* `leaves.txt`: one line per P-state used as a leaf, canonical form: `t B F`.
* Result files are JSON lines under `results/<topic>/`; programs that take `--shard i --nshards N` process the
  items with index ≡ i (mod N), write `…shard<i>of<N>.jsonl`, skip items already present (resumable), and a
  merge step concatenates and sorts them.

## 7. Module interfaces

All C++ headers are header-only, `namespace ttt`, include `geom.hpp`, and must compile with `-Wall` cleanly.

### 7.1 `src/canon.hpp` (task 1)

```cpp
struct CanonResult { CellSet B, F; int g; };            // B = image(g,Bin), F = image(g,Fin)
CanonResult canon_pair(const CellSet& B, const CellSet& F);   // section 6; g = smallest index attaining it
uint32_t busy_mask(const CellSet& s);                   // bit i <-> G().busy[i]  (s ∩ busy)
CellSet  busy_set(uint32_t mask);
uint32_t busy_image(int g, uint32_t mask);              // byte-lookup tables
uint32_t busy_canon(uint32_t mask);                     // image whose cell set is smallest under cmp
std::vector<uint32_t> busy_orbit_reps(int k);           // canonical masks of all k-subsets, sorted by cmp of their sets
uint64_t state_hash(const CellSet& B, const CellSet& F, int t);  // good 64-bit mix of a (canonical) state
```
Target: `canon_pair` ≲ 20 µs for |B|+|F| ≤ 30 (it is the inner loop of the search).

### 7.2 `src/match.hpp` (task 2)

```cpp
int  live_lines(const CellSet& B, int out[NL]);          // ascending line ids, returns the count
int  slack(const CellSet& B);
bool admissible(const CellSet& B, const CellSet& X);
struct Pairing { bool ok; int16_t owner[NC]; std::vector<int> hall; };
//   ok: owner[c] = id of the live line cell c is assigned to (each live line exactly 2 cells), else -1
//  !ok: hall = line ids of a set S with |N(S)\X| < 2|S|
Pairing find_pairing(const CellSet& B, const CellSet& X);
struct EnumResult { int status; CellSet X; std::vector<int> S; uint64_t nodes; };
//   status 1: C(B,F,k) holds; 0: fails, X = admissible violator, S = Hall-violating lines; -1: node limit hit
EnumResult enum_check(const CellSet& B, const CellSet& F, int k, uint64_t node_limit = 0);
```
`enum_check` decides C exactly by depth-first enumeration of the k−|F| free stones (ascending cell order,
incremental matching repair, skip placements that break a cap or create a danger pair; any admissible X of
size < k is covered because deficiency is monotone in X — prove this in a comment — and orbit pruning of
the first stone under the stabiliser of (B,F) is optional). Intended for k−|F| ≤ 3.
CLI `bin/enumcheck < instances` → JSON lines `{"k":..,"B":"..","F":"..","holds":true|false,"nodes":..,"X":"..","S":[[6 cells],..]}`.

### 7.3 `src/phi.hpp` (task 6a)

```cpp
struct PhiResult { bool holds; int reason; std::vector<int> S; int sur; };
//  reason 0: LEAF holds. 1: F not B-admissible. 2: no pairing for (B,F) (S = Hall violator).
//  reason 3: S = an F-connected set of >= 3 live lines with sur_F(S) = sur < k-|F|.
PhiResult phi_check(const CellSet& B, const CellSet& F, int k);
bool leaf_holds(const CellSet& B, const CellSet& F, int k);    // == phi_check(...).holds, fastest path
int  phi_value(const CellSet& B, const CellSet& F, int cap);   // min(cap, min sur_F over F-connected S, |S|>=3); requires reasons 1,2 absent
```
CLI `bin/phicheck < instances` → JSON lines `{"k":..,"B":"..","F":"..","leaf":true|false,"reason":r,"sur":..,"S":[[6 cells],..]}`.
`leaf_holds` must be thread-safe (no shared mutable state) and fast: most calls in the search have
θ = k−|F| ≤ 4 or fail the quick rejection.

### 7.4 `src/search.cpp` (tasks 4+5) → `bin/search`

`bin/search --rmax R --maxcand K --threads T --out DIR [--time-limit SEC] [--nonbusy]` proves or fails the
root. Writes `DIR/strategy.tbl`, `DIR/leaves.txt` (states reachable in the proof tree; extra proven states are
allowed) and `DIR/stats.json` (`{"proved":bool,"rmax":..,"p_states":..,"q_states":..,"leaves":..,"seconds":..,
"failing_branch":[...]}`); exit code 0 iff proved.

### 7.5 `verify/treecheck.cpp` (independent verifier) → `bin/treecheck`

Single self-contained file; must not include anything from `src/` and must not be derived from it.
`bin/treecheck --table F --leaves F --rmax R --threads T --out results/final/treecheck.json` re-expands the
whole tree of section 4 from the root using only the table and the leaf file: every reachable non-VOID
P-state is either listed as a leaf (then LEAF is recomputed with its own max-flow) or is expanded with LOST
false and all children (a),(c),(d); every reachable non-VOID Q-state has a table entry and gets children (i),(ii).
Output `{"ok":bool,"p_states":..,"q_states":..,"leaves_checked":..,"max_t":..,"errors":[...]}`.

### 7.6 `sim/sim.cpp` (task 7, independent) → `bin/sim`

Single self-contained file written from sections 1, 3, 5, 6 only; must not include or read `src/`.
`bin/sim --table F --leaves F --games N --seed S --adversary NAME --threads T --out FILE.json`.

### 7.7 `py/ip_check.py` (task 3, independent Python implementation, own geometry)

Integer program for worst(B,F,k) (binary s_l, n_c, x_c, q_l over live lines / non-B cells):
n_c ≥ s_l (c ∈ l); x_c ≤ n_c; Σx ≤ k; x_c = 1 (c ∈ F); Σ_{c∈l} x_c ≤ 4; q_l ≥ Σ_{c∈l} x_c − 3;
q_a + q_b − x_c ≤ 1 for live a ≠ b through c; maximise 2Σs − Σn + Σx.
`python3 py/ip_check.py --in FILE --out FILE.jsonl [--mode full|decide] [--no-danger] [--shard i --nshards N]`
→ `{"k":..,"B":"..","F":"..","worst":int|null,"decision":"holds"|"fails","status":"..","seconds":..,"X":"..","S":[[6 cells],..]}`.
`decide` mode stops as soon as the sign is known (`mip_abs_gap = 0.99`, objective target ≥ 1 / bound < 1).
The IP is floating point: its results are labelled *computed*, never *proved*.

## 8. Regression values (section 3 of the task statement)

T7 = {(0,0,0),(5,5,5),(1,1,4),(4,4,1),(2,3,2),(3,2,3),(5,0,0)}, T8 = T7 ∪ {(0,5,5)}.

1. Minimum number of SP stones for a pairing on the empty board (X = ∅): 7 for n=6; 0 for n=8 (244 lines);
   3 for n=7; 9 for n=5.
2. slack(T7) = 1, pairing exists; worst(T7,∅,7) = 6.
3. slack(T8) = 8; worst(T8,∅,8) = 0; without the danger constraints 1; worst(T8,∅,9) = 1.
4. worst(T8,{h},8) = 0 for each of the 16 busy h ∉ T8.
5. worst(T8 − {(5,5,5)} + {(1,1,1)}, {(5,5,5)}, 8) = 0.
6. Antipode stealing, B = (0,0,0),(1,1,4),(2,3,2),(0,5,5),(1,1,1),(2,2,2),(0,0,5),(1,4,1) in this order,
   F = antipodes of all B cells except the last one played, k = |B|: worst = 6 after 8 rounds;
   adding (2,2,3): worst = 0 after 9 rounds (F = 8 antipodes); adding (0,5,0): worst = 0 after 10 rounds.
7. Threat branch: B = T8 ∪ {(0,1,2)}, F = {(x,1,2): x=1..5}, k = 9: worst 0. B = T7 ∪ {(0,1,2)}, same F, k = 8: worst 2.
8. Orbit counts of busy k-subsets under G: 527 (k=7), 1057 (k=8), 1669 (k=9).
9. Random busy 8-sets are mostly bad (worst 4–8).

Whenever worst ≤ 0 above, LEAF must also be checked and its value reported (it is allowed to be false only
if you can exhibit why; for F = ∅ it must agree with the sign of worst).

## 9. Ownership

| path | owner |
|---|---|
| `SPEC.md`, `PROGRESS.md`, `REPORT.md`, `Makefile`, `src/geom.hpp` | coordinator |
| `src/canon.hpp`, `src/test_canon.cpp`, `src/leaftable.cpp` | task 1 |
| `src/match.hpp`, `src/phi.hpp`, `src/enumcheck.cpp`, `src/phicheck.cpp`, `src/test_core.cpp` | tasks 2 + 6a |
| `py/ip_check.py`, `py/regress.py`, `py/geom.py` | task 3 |
| `src/search.cpp` (+ helper headers `src/search_*.hpp`) | tasks 4 + 5 |
| `src/certify.cpp`, `py/crosscheck.py` | task 6b |
| `verify/treecheck.cpp` | task 6c |
| `sim/sim.cpp` | task 7 |
| `src/minstones.cpp` | regression 1 |
| `PROOF.md` | proof writer; `REVIEW.md` | task 8 |

Write only files you own (plus scratch files under `logs/<your task>/` and results under `results/`).

## 10. Errata and clarifications (added by the coordinator after the freeze; no program behaviour changes)

1. §5, PAIRING mode: "take the other cell assigned to the same line" reads "take it **if it is empty**; otherwise
   take any empty cell". The partner can already hold an SP stone placed earlier as an "any empty cell" move
   (its line is then dead). This is the rule proved in PROOF.md Lemma 1.1 and implemented by `sim/sim.cpp`.
2. §4: table moves are **not** restricted to busy cells in the certified tree. Busy-only table moves cannot prove
   the root for R_max ≤ 10 (exhaustive search of the game without forced events), so `bin/search` runs with
   `--nonbusy`; the rules (i), (ii) are the same for every cell.
3. §4, LOST: E2 and E3 are necessary conditions for losing admissibility after a forced reply, not sufficient
   ones (PROOF.md Remark 8.8). "At least 11 resp. 12 distinct stones" are lower bounds; consequently LOST is
   false at every P-state with t ≤ 9, i.e. at every expanded state of a tree with R_max = 10.
4. §3: LEAF condition 2 is implied by conditions 1 and 3 (PROOF.md Remark 3.3); it is kept because the max-flow
   test needs the pairing. worst(B,F,k) is a maximum over an empty family when F is not B-admissible; C(B,F,k) is
   then vacuously true and the checkers report such instances separately ("void" / "infeasible").
5. §7.2: `enum_check` is exact because the pairing is examined at every node of the enumeration, not because of
   monotonicity alone (PROOF.md Remark 2.3).
6. §7.5: the certifying `bin/treecheck` run uses `--exact` (visited set keyed by full states) and `--paranoid`
   (every flow behind a LEAF verdict validated literally). The table and leaf-file look-ups of treecheck always
   use 120-bit keys; a collision there cannot make it accept a tree that is not WON (a looked-up move is still
   legality-checked and its subtree verified; a state taken for a listed leaf is still recomputed), and
   `verify/spotcheck.py` uses exact keys throughout and reaches every line of both files.
7. Whoever cites a treecheck result must check, besides `ok`: root = "0 - -", `root_void` false, `complete`
   true, `rmax`, `table_noncanonical` = `leaf_noncanonical` = 0, `table_conflicts` = 0, and the input checksums.

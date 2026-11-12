# The 6×6×6 tic-tac-toe game is a draw — supplementary archive

This directory contains everything needed to check, or to reproduce from scratch, the computer-assisted
proof described in the paper *The 6×6×6 tic-tac-toe game is a draw* (`paper/paper.pdf`, sources in
`paper/`). Every file that the paper names is contained in this archive under the path given there.

The paper is one PDF in two parts. The **main part** (Sections 1–9, the Declarations, Appendix A "File
formats, canonical form and test vectors", Appendix B with the pairing table for the 7×7×7 board, and the
references) contains everything that the proof of the theorem uses and everything that is needed to
replicate the verification. The **supplementary material** (Appendices S1–S7, after the references)
contains results that the theorem does not use, the description of the search, the details of the audits
and cross-checks, the integer program, the details of this archive and of the reproduction, and further
tables.

## The theorem

> **Theorem (computer-assisted).** In the Maker–Breaker game on the 6×6×6 board with its 148 lines,
> Breaker, moving second, has a strategy under which Maker never occupies a whole line. Consequently
> (strategy stealing) ordinary 6×6×6 tic-tac-toe is a draw.

The proof has two parts.

1. **Written proofs** (the paper; in an earlier form `PROOF.md`): if a valid certificate for a certain finite
   game tree ("the abstract game", lazy revelation of Maker's stones) exists, then an explicit strategy for
   Breaker never lets Maker complete a line.
2. **One computed fact** (Computed Fact 6.1 of the paper): the two certificate files form a valid
   certificate with R_max = 10; in particular the root of the abstract game is WON. The certificate
   consists of two text files,

   | file | content | SHA-256 |
   |---|---|---|
   | `results/final/strategy.tbl` | 93,164 Q-states with one table move each | `13629393fcdcee87a03f2c9c73f640d9584d2134a46f90dad9393dc92c9ce3fc` |
   | `results/final/leaves.txt` | 106,723 leaves | `3c14c7814e296e812b4fdf3cda2f446c6ce4865d15d351980e7fecc7690fd470` |

   and it is verified, in exact integer arithmetic, by two independently written programs in this
   directory: `verify/treecheck.cpp` (C++) and `verify/spotcheck.py` (Python). They share no code with
   each other or with the program that found the certificate. Five further programs have re-expanded the
   tree with the same counts (paper, Table 7): two written during the internal review of the project
   (`logs/review/treecheck_mut/`, `logs/review/lead/`) and three written from the text of the paper during
   an internal refereeing of its first version (`logs/paper/replication_from_paper/verify.cpp`,
   `logs/paper/referee/audit/ref_audit.cpp`, `logs/paper/referee/b_expand.py`; the last one has no leaf
   test).

All programs, the internal review and the internal refereeing were produced with AI-based tools; see the
section "Declarations" of the paper for what this means for the independence of the programs.

Nothing is formally verified in a proof assistant. No floating-point computation is part of the proof; the
integer programs (HiGHS) and the simulator are cross-checks only.

Naming: the paper calls the players **Maker** (first) and **Breaker** (second). The older documents and
the source code call them **FP** (first player) and **SP** (second player).

## Quick start

Requirements: a C++17 compiler with OpenMP (`g++` 13.3 was used), `make`, Python 3.10 or later (3.13 was
used). No other packages. All times below were measured on a four-core machine (Intel Xeon, 2.1 GHz).

### 1. Verify the certificate in under a minute (C++)

```
make bin/treecheck
bin/treecheck --table results/final/strategy.tbl --leaves results/final/leaves.txt \
              --rmax 10 --exact --paranoid --threads 4 --out treecheck.json
```

Expected: exit code 0 and one line of JSON that begins

```
{"ok":true,"p_states":153135,"q_states":93164,"leaves_checked":106723,"max_t":10,"errors":[],"error_count":0,"complete":true,
```

and contains `"root":"0 - -"`, `"root_void":false`, `"rmax":10`, `"exact_visited_set":true`,
`"paranoid_flows":true`, `"states_visited":246299`, `"table_noncanonical":0`, `"leaf_noncanonical":0`,
`"table_conflicts":0`, `"threat_children":143498`, `"danger_children":28275`, `"void_skipped":151`, and the
per-round counts

```
"p_states":[1,2,5,14,35,181,1718,7227,34801,93124,16027]
"q_states":[0,2,5,14,35,101,446,3487,14362,62381,12331]
"leaves":[0,0,0,0,0,0,0,0,3382,87314,16027]
```

Compiling takes about 10 seconds, the run about 10 CPU-seconds (less than 3 seconds of wall-clock time with four
threads when the cores are not occupied by other jobs; `logs/paper/artifact_test_final.log`, the test of
this archive). The option `--paranoid` (literal validation
of every flow behind a LEAF = true verdict) is the default of the program and is given only for emphasis;
`--fast` switches it off. Compare your output with `results/final/treecheck.json`; only the fields
`threads`, `states_per_thread`, `seconds`, `load_seconds`, `states_per_second` may differ. (That file was
produced with three threads on a loaded machine: 14.8 seconds of wall-clock time, 8.2 CPU-seconds.)

### 2. Verify the certificate with the independent Python verifier (a few minutes)

```
python3 verify/spotcheck.py --selftest
python3 verify/spotcheck.py --table results/final/strategy.tbl --leaves results/final/leaves.txt \
        --rmax 10 --samples 0 --seed 20261112 --procs 4 --full --all-leaves --out spotcheck_full.json
```

The self-test takes about 10 seconds and ends with `selftest OK`. The exhaustive run takes about 6
CPU-minutes (about one and a half minutes of wall-clock time with `--procs 4` in the test of the unpacked
archive, `logs/paper/artifact_test_final.log`) and its last line of output contains

```
ok=True  errors=0  p_expanded=46412 q_checked=93164 children={'a': 46412, 'c': 143498, 'd': 28275, 'i': 93164, 'ii': 54482} void=151 leaves_recomputed=106723
```

Compare with `results/final/spotcheck_full.json`.

### 3. Check the files you received

```
sha256sum -c MANIFEST.sha256
```

## Full reproduction

```
scripts/reproduce.sh                       # everything except the HiGHS checks
GAMES=20000 THREADS=2 scripts/reproduce.sh # the same with fewer simulated games
```

rebuilds all programs, runs their self-tests, recomputes the regression value "minimum number of Breaker
stones for a pairing" (9, 7, 3, 0 for n = 5, 6, 7, 8), the leaf table of busy sets, **re-runs the search**
(about one minute) and compares the regenerated certificate byte for byte with `results/final/`, runs both
verifiers on the regenerated certificate, runs the leaf cross-checks (two LEAF implementations on all
leaves and on 20,000 perturbed instances; exact enumeration of the pairing condition C on all 106,245
leaves with at most 3 unrevealed stones) and finally the simulator. Outputs go to `results/repro/`, logs to
`logs/repro/`; the script prints `PASS`/`FAIL` per step and `ALL STEPS PASSED` at the end. The reduced form
above was validated end to end: all 26 steps passed in about 23 minutes (two threads, on a machine that was
running other jobs). With the default 10^6 simulated games the simulator alone needs about 26 minutes on
four threads, so the whole script should take somewhat less than an hour on four idle cores.

### Optional: the HiGHS cross-checks

```
pip install highspy            # HiGHS 1.16 was used
scripts/reproduce.sh --with-ip # adds the integer-program checks; several core-hours
```

These checks decide the pairing condition C(B,F,k) by a 0-1 program (paper, Appendix S5 "The integer
program used as a cross-check") on the regression
instances, on 1,291 test instances, and on the leaves. Their "holds" answers rest on floating-point bounds.
They are not part of the proof.

## Directory map

| path | content |
|---|---|
| `README.md`, `MANIFEST.sha256` | this file; SHA-256 of every file |
| `paper/` | LaTeX sources of the paper and `paper.pdf` |
| `PROOF.md` | the mathematical part in its original form (players called FP/SP) |
| `SPEC.md` | definitions, file formats and program interfaces; §10 lists errata |
| `REPORT.md`, `REVIEW.md`, `PROGRESS.md` | results of the computation, the internal review, the project log |
| `Makefile` | `make bin/<name>` builds `src/<name>.cpp`, `verify/<name>.cpp` or `sim/<name>.cpp` |
| `verify/treecheck.cpp` | **verifier 1** (C++, self-contained): re-expands the whole tree, recomputes every leaf |
| `verify/spotcheck.py` | **verifier 2** (pure Python, self-contained) |
| `src/geom.hpp`, `canon.hpp`, `match.hpp`, `phi.hpp` | library of the search: geometry, canonical forms, pairings and exact enumeration, the LEAF test |
| `src/search.cpp` | the AND–OR search that produced the certificate (heuristic; not trusted) |
| `src/leaftable.cpp`, `phicheck.cpp`, `enumcheck.cpp`, `certify.cpp`, `minstones.cpp` | leaf table of busy sets; LEAF and exact enumeration from the command line; exact decision of C(B,F,k); minimum number of stones for a pairing |
| `src/geom_selftest.cpp`, `test_canon.cpp`, `test_core.cpp` | unit tests of the library |
| `sim/sim.cpp` | simulator: plays Breaker's real strategy against adversaries (self-contained) |
| `py/ip_check.py`, `geom.py`, `regress.py`, `crosscheck.py` | the integer program (HiGHS), its own geometry, the regression values, the comparison with `bin/certify` |
| `scripts/reproduce.sh` | one-command reproduction (see above) |
| `scripts/leaf_instances.py`, `compare_leafcheck.py` | helpers of `reproduce.sh` |
| `scripts/make_artifact.sh` | builds this archive and `MANIFEST.sha256` |
| `results/final/strategy.tbl`, `leaves.txt` | **the certificate** |
| `results/final/stats.json` | statistics of the search run that produced it |
| `results/final/treecheck.json`, `treecheck_crosscheck.json`, `spotcheck_full.json` | outputs of the verifiers |
| `results/final/checks/enum_theta{0,1,2,3}.jsonl` | exact enumeration of the pairing condition C on all leaves with θ ≤ 3 |
| `results/final/checks/ip_leaf_*` | HiGHS on all 478 leaves with θ ≥ 4 and on 830 sampled leaves (instances `.txt`, results `.jsonl`) |
| `results/final/sim/` | the 10^6-game simulator run (`sim_mix_1M.json`, list of table entries and leaves it did not reach) and a 4,000-game run with all internal checks switched on |
| `results/regression/` | regression values: minimum stones for n = 5..8 with explicit pairings; the integer-program values |
| `results/leaftable/phi_k{8,9,10}.jsonl` | LEAF(B,∅,k) for every orbit of busy k-sets (exact max-flow) |
| `results/ip/leaftable_k*.jsonl` | HiGHS on the leaf table (k = 8 complete, k = 9 partial) |
| `results/crosscheck/` | exact certifier versus integer program on 1,291 instances: `instances.txt`, `instances_meta.jsonl`, the answers of `bin/certify` (`certify.jsonl`) and of the integer program (`ip.shard*.jsonl`, `ip_reused.jsonl`), `summary.json` |
| `logs/review/*.md`, `logs/spec_issues.md` | audit reports (mathematics, search and library, verifier, simulator); issues raised against the specification |
| `logs/review/treecheck_mut/aexpand.cpp`, `aleaf.cpp`, `aexpand_final.json`, `lost_inst.txt` | re-expansion I of the paper's Table 7 (written by the auditor of `treecheck`), its output, and the 800 LOST instances of the audit |
| `logs/review/lead/lead_expand.cpp`, `lead_leaf.cpp`, `lead_expand.json` | re-expansion II of Table 7 (written by the lead reviewer) and its output |
| `logs/paper/referee_*.md` | the five reports of the internal refereeing of the first version of the paper (mathematics A and B, audit of all numbers, journal standards, replication from the paper alone) |
| `logs/paper/referee2_math.md`, `referee2_quality.md`, `logs/paper/referee2/` | the two reports of the second round, on the revised version (mathematics added in the revision; closure of the first-round findings and publication quality), and the programs of the two checkers with their outputs (`x_*`, `y_*`) |
| `logs/paper/round2_checks/` | the programs with which the changes of the second round were checked: the three examples after Lemma 5.2 (`r2_children.py`), a literal evaluation of LOST on the test vectors (`r2_lost.cpp`), the sharpness of Proposition 4.22 (`r2_prop422.py`), with their outputs |
| `logs/paper/response_to_referees.md`, `logs/paper/response_M.md`, `response_C.md` | what was changed in response to each finding: the merged response, organised by report and finding (its last section, "Second round", answers the two reports of the second round), and the two change logs of the first phase of the revision on which it is based |
| `logs/paper/revM_checks/`, `logs/paper/revision_M_for_C.tex`, `logs/paper/check_refs.py` | check scripts written for the revision of Sections 2-5; corrected LaTeX text that the response of the first phase of the revision cites; the script that checks every cross-reference, label and citation of the paper |
| `logs/paper/related_work.md`, `logs/paper/issues.md` | the literature report on which the statements about earlier work rest (with the status of every statement); the list of issues of the first version |
| `logs/paper/paper_v1/` | the first version of the paper (sources and PDF), to which the page and line numbers of the reports refer |
| `logs/paper/replication_from_paper/` | the verifier written from the PDF of the paper (and a text extraction of it) and the two certificate files alone (`verify.cpp`), with its logs. The program prints `ACCEPT` and reproduces all per-round counts; its exit code is 1 and `run.log` ends with "1 mismatches", because it also compares its own numbers of flows with two implementation statistics of `treecheck` that were printed in the first version of the paper (paper, Section 8.5 and Appendix S4.1) |
| `logs/paper/referee/` | the programs written by the other referees, with their outputs; `audit/ref_audit.cpp` and `b_expand.py` (with `b_sigma.py`, `b_geom.py`) are the two further programs of Table 7. `python3 logs/paper/referee/b_expand.py` runs from the root of the unpacked archive (about 3.5 minutes; needs `numpy`); see "Running the programs of the referees" below |
| `logs/paper/lost_vectors/` | the LOST test vectors of the paper's appendix on file formats, computed with three implementations (`run.sh` repeats the comparison) |
| `logs/paper/flow_counts/` | recount, from the rules stated in Appendix S4.1 of the paper, of the flow statistics of `treecheck` (869,185 / 77,268 / 7,391); `run.sh` repeats it |
| `logs/canon/autgroup.cpp`, `autgroup.log` | Computed Fact S1.1 (Remark 2.9 of the main part): the full automorphism group has order 1,152 |
| `logs/proof/e23min.cpp`, `e23min.log` | Computed Fact S2.6 on the numbers of stones of the patterns E2 and E3 |
| `logs/proof/fuzz_cprime.py`, `smallgeo.py`, `fuzz_cprime_run1.log` | the random test quoted at the end of Appendix S1.3 on the monotonicity of C |
| `logs/paper/secA_check1.py`, `secA_checks.log`, `logs/core/test_core.log`, `logs/minstones/run.log` | scripts and logs named in Computed Facts 3.10 and 4.23 on the sets T7 and T8 |
| `scripts/check_7cube_midplanes.py`, `logs/paper/check_7cube_midplanes.out` | the check of the mid-plane decomposition of the 7×7×7 board and of the printed pairing table of Appendix B (Remark 9.4) |
| `logs/final/*.log`, `logs/repro/*.log`, `logs/search/` | logs of the certified runs, of the validation run of `reproduce.sh` and of the search experiments (with `NOTES.md`); running times and counters quoted in the paper are taken from them |
| `logs/paper/artifact_test_final.log` | test of the final build of this archive in a fresh directory: checksums, build, the certifying run of `treecheck`, both runs of `spotcheck.py`, identity of the PDF; the reference running times of the paper (Section 8.3, Appendix S6.2) are taken from it |
| `logs/paper/artifact_test.log`, `logs/paper/artifact_test_revision.log` | tests of two earlier builds of the archive: that of the first version of the paper, and that of the revision before the second round (the latter also runs the verifier written from the paper alone and the LOST test vectors) |

Not included, because the scripts regenerate them: binaries (`bin/`), the per-θ instance files
`results/final/checks/leaves_theta*.txt` and `perturbed.txt` (`python3 scripts/leaf_instances.py
results/final/leaves.txt results/final/checks 20000`), and repeated or exploratory runs. The
simulator source used for the 10^6-game run is byte-identical to `sim/sim.cpp`.

In the archived copies of the referee material the absolute path of a temporary working directory has been
replaced by `/tmp/scratch` (a few files; `scripts/make_artifact.sh` does this while staging and names
them), and two lines of code were changed in `logs/paper/referee/b_sigma.py` and `b_expand.py` (and one
in `b_play6.py`), see the next paragraph. Nothing else differs from the files in the project tree.

**Running the programs of the referees.** The programs in `logs/paper/referee/` and `logs/paper/referee2/`
are archived as they were run. Many of them contain the absolute path `/home/user/ttt6/` of the project
directory (and some the scratch directory `/tmp/scratch/`); to run those, unpack or link the archive at
`/home/user/ttt6` and create `/tmp/scratch`. `b_play7.py` in addition needs a table that `b_checks.py`
writes. The exceptions are the programs listed in Table 7 of the paper: `logs/paper/replication_from_paper/verify.cpp`
and `logs/paper/referee/audit/ref_audit.cpp` take the two certificate files as arguments, and the archived
copies of `b_sigma.py` and `b_expand.py` were changed so that `python3 logs/paper/referee/b_expand.py`
runs from the root of the unpacked archive: the certificate is read relative to the script, and the
reachable set that the script writes for `b_play6.py` goes to the system's temporary directory.

**Words used in the project documents.** `SPEC.md`, `PROOF.md`, `REPORT.md`, `REVIEW.md`, `PROGRESS.md`
and the files under `logs/` were written during the work and are archived unchanged. In them "the task
statement" is the description of the problem and of the intended method with which the project was
started; it is not part of the archive, and where the project deviated from it, `REVIEW.md` and
`REPORT.md` say so. "The user" is the person who directed the work. Labels such as "author A", "author
B", "editor", "reviser M", "reviser C", "referee", "checker X" and "checker Y" denote separate working
sessions of the AI-based agents (see the Declarations of the paper), not persons, and "the harness" is
the software in which these sessions ran.

## File formats; writing your own verifier

The appendix "File formats, canonical form and test vectors" of the paper specifies everything an
independent implementation needs (cell ids, canonical form, the two file formats down to the byte level,
which defects of a file a verifier must reject, test vectors for the canonical form, for LEAF and for
LOST); `SPEC.md` §1–§4 and §6 contain the same definitions in the FP/SP terminology. In brief:

* cell (x,y,z), 0 ≤ x,y,z ≤ 5, has id 36x + 6y + z; a set of cells is written as its ids in ascending order
  separated by commas, the empty set as `-`;
* `strategy.tbl`: one line `t B F m` per Q-state (Maker has made t moves; B = Breaker's t−1 stones;
  F = the revealed stones of Maker; m = Breaker's move), in the canonical frame of (B,F);
* `leaves.txt`: one line `t B F` per leaf (a P-state after t complete rounds), in canonical form;
* both files are ASCII, every line ends with a single line feed, fields are separated by single spaces.

A verifier has to implement Algorithms 1 and 2 of the paper (Section 7: expansion of the tree; the LEAF
test by integer max-flow). It should reproduce: 246,299 reachable states = 153,135 P-states (46,412 expanded, 106,723
leaves) + 93,164 Q-states; 143,498 threat children and 28,275 danger children (non-VOID; a child is
counted once for every expanded state of which it is a child); 151 VOID children, all of type (ii); and
the per-round counts listed above. A verifier written from the PDF of the first version of the paper alone
did reproduce all of them (`logs/paper/referee_replication.md`; on its exit code 1 see the entry
`logs/paper/replication_from_paper/` in the table above).

Other entry points:

```
make bin/phicheck   && echo "8 0,35,46,92,123,169,180,215 -" | bin/phicheck      # LEAF(T8, {}, 8): true
echo "9 0,35,46,92,123,169,180,215 -" | bin/treecheck --leafcheck --verbose      # LEAF(T8, {}, 9): false
make bin/enumcheck  && echo "9 0,5,35,43,46,61,86,87,92 123,129,154,169,172,180,210,215" | bin/enumcheck
bin/treecheck --selftest --quick
bin/treecheck --lostcheck < logs/paper/lost_vectors/vectors.txt                  # LOST test vectors
```

## Known minor issues of the tools

The audits found no defect that affects a result, but the following properties should be known to anyone
who reuses the programs (paper, Section 7.7 and Appendix S4.4). `treecheck` accepts a table that contains two different
moves for the same key (it uses the smaller move and counts the conflict), accepts lines written in a
non-canonical frame (it re-canonicalizes them and counts them), leaves an older output file in place if it
aborts, parses numeric options leniently, and reports success for a VOID root given on the command line.
Therefore check, in addition to `"ok":true`, the fields listed under "Quick start" above; all these
counters are zero in the certified run. `results/final/stats.json` counts only VOID children of types (c)
and (d) (there are none), not the 151 VOID children of type (ii). The LOST code of all programs is not
exercised by a certificate with R_max = 10. The simulator does not count a missing table entry that is met
only while an adversary predicts Breaker's next move.

## Checksums

The archive `ttt6-artifact.tar.gz` has 355 files (`MANIFEST.sha256` included), about 6.3 MB compressed and
about 38 MB unpacked. `MANIFEST.sha256` lists every other file of the archive. The two files that matter for the theorem are the
certificate files in the table at the top.

## Availability

Persistent identifier of the archive: [DOI / repository URL to be added].

## License

[license to be chosen by the authors]

#!/bin/bash
# scripts/make_artifact.sh -- build the supplementary archive of the paper
#                             "The 6x6x6 tic-tac-toe game is a draw".
#
#   scripts/make_artifact.sh             creates  dist/ttt6-artifact.tar.gz  and  MANIFEST.sha256
#   scripts/make_artifact.sh --out DIR   creates  DIR/ttt6-artifact.tar.gz  and  DIR/MANIFEST.sha256  and
#                                        leaves dist/ and the MANIFEST.sha256 of the project root alone
#   scripts/make_artifact.sh --dry-run   only checks that every file of the list exists and prints the
#                                        list; nothing is written
#   (--allow-stale-pdf  packages paper/paper.pdf even if it is older than the sources of the paper; for
#                        tests of this script only)
#
# The archive unpacks into one directory, ttt6-artifact/, and contains what is needed to check or to
# reproduce the computation and to inspect every piece of evidence that the paper cites: documents,
# sources, the certificate, the verification outputs, the outputs of the cross-checks, the programs written
# during the internal review and during the internal refereeing of the paper, the referee reports of both
# rounds and the responses to them, the logs from which running times and counters are quoted, and the paper itself
# (sources and PDF).  RULE: every file that is named in the paper must be in this list.
# The archive contains no binaries and none of the large intermediate files that the scripts regenerate.
# MANIFEST.sha256 lists the SHA-256 checksum of every file in the archive (except itself); it can be
# checked with
#       sha256sum -c MANIFEST.sha256
# from the unpacked directory.  The script stops if paper/paper.pdf is older than the sources of the paper,
# or if the number of files printed in the paper (\ArchiveFiles in paper/macros.tex) and in README.md is
# not the number of files of the archive.  (From the project root the same command works for all files except the few
# that are rewritten while staging; see "neutral paths" and "runnable copies" below.)
#
# Nothing is uploaded anywhere.  Requirements: bash, GNU tar, gzip, sha256sum, find, sort, sed, grep, file.
set -euo pipefail
cd "$(dirname "$0")/.."

NAME=ttt6-artifact
OUT=dist
DRY=0
STALE_PDF_OK=0
WRITE_ROOT_MANIFEST=1
while [ $# -gt 0 ]; do
  case "$1" in
    --out)     [ $# -ge 2 ] || { echo "make_artifact: --out needs a directory" >&2; exit 2; }
               OUT=$2; WRITE_ROOT_MANIFEST=0; shift 2 ;;
    --dry-run) DRY=1; shift ;;
    --allow-stale-pdf) STALE_PDF_OK=1; shift ;;      # for tests of this script only
    *)         echo "usage: scripts/make_artifact.sh [--out DIR] [--dry-run] [--allow-stale-pdf]" >&2; exit 2 ;;
  esac
done
STAGE="$OUT/$NAME"
TARBALL="$OUT/$NAME.tar.gz"

# checksums of the certificate (paper, Section 1.7); the script refuses to package anything else
SHA_TABLE=13629393fcdcee87a03f2c9c73f640d9584d2134a46f90dad9393dc92c9ce3fc
SHA_LEAVES=3c14c7814e296e812b4fdf3cda2f446c6ce4865d15d351980e7fecc7690fd470

die() { echo "make_artifact: $*" >&2; exit 1; }

[ "$(sha256sum < results/final/strategy.tbl | cut -d' ' -f1)" = "$SHA_TABLE" ]  || die "results/final/strategy.tbl has an unexpected checksum"
[ "$(sha256sum < results/final/leaves.txt   | cut -d' ' -f1)" = "$SHA_LEAVES" ] || die "results/final/leaves.txt has an unexpected checksum"

# ---------------------------------------------------------------------------------------------- file list
# 1. files that must exist
required=(
  README.md SPEC.md PROOF.md REPORT.md REVIEW.md PROGRESS.md Makefile
  src/geom.hpp src/canon.hpp src/match.hpp src/phi.hpp
  src/search.cpp src/certify.cpp src/enumcheck.cpp src/phicheck.cpp src/leaftable.cpp src/minstones.cpp
  src/geom_selftest.cpp src/test_canon.cpp src/test_core.cpp
  verify/treecheck.cpp verify/spotcheck.py
  sim/sim.cpp
  py/geom.py py/ip_check.py py/regress.py py/crosscheck.py
  scripts/reproduce.sh scripts/leaf_instances.py scripts/compare_leafcheck.py scripts/make_artifact.sh
  results/final/strategy.tbl results/final/leaves.txt
  results/final/stats.json results/final/treecheck.json results/final/treecheck_crosscheck.json
  results/final/spotcheck_full.json
  results/final/checks/enum_theta0.jsonl results/final/checks/enum_theta1.jsonl
  results/final/checks/enum_theta2.jsonl results/final/checks/enum_theta3.jsonl
  results/final/checks/ip_leaf_sample.txt results/final/checks/ip_leaf_sample.jsonl
  results/final/checks/ip_leaf_theta_ge4_rest.txt
  results/final/sim/sim_mix_1M.json results/final/sim/sim_mix_1M_bugs.jsonl
  results/final/sim/sim_mix_1M_uncovered.txt
  results/final/sim/sim_paranoid_4k.json results/final/sim/sim_paranoid_4k_bugs.jsonl
  results/regression/minstones.json results/regression/minstones_pairings.json
  results/regression/ip_regression.json results/regression/ip_regression.cache.jsonl
  results/leaftable/phi_k8.jsonl results/leaftable/phi_k9.jsonl results/leaftable/phi_k10.jsonl
  results/ip/leaftable_k8.jsonl
  results/crosscheck/summary.json results/crosscheck/instances.txt
  results/crosscheck/instances_meta.jsonl results/crosscheck/certify.jsonl results/crosscheck/ip_reused.jsonl
  logs/spec_issues.md
  # --- programs and logs named in the Computed Facts that the main theorem does not use
  #     (paper: Computed Facts S1.1, 3.10, 4.23, S2.6; end of Appendix S1.3; Remark 9.4)
  logs/canon/autgroup.cpp logs/canon/autgroup.log
  logs/core/test_core.log
  logs/minstones/run.log
  logs/paper/secA_check1.py logs/paper/secA_checks.log
  logs/proof/e23min.cpp logs/proof/e23min.log
  logs/proof/fuzz_cprime.py logs/proof/smallgeo.py logs/proof/fuzz_cprime_run1.log
  scripts/check_7cube_midplanes.py logs/paper/check_7cube_midplanes.out
  # --- the two re-expansions written during the internal review (paper, Table 7), with their outputs,
  #     and the 800 LOST instances of the audit of the main verifier
  logs/review/treecheck_mut/aexpand.cpp logs/review/treecheck_mut/aleaf.cpp
  logs/review/treecheck_mut/aexpand_final.json logs/review/treecheck_mut/lost_inst.txt
  logs/review/lead/lead_expand.cpp logs/review/lead/lead_leaf.cpp logs/review/lead/lead_expand.json
  # --- the internal refereeing of the first version of the paper: the five reports, ...
  logs/paper/referee_math_a.md logs/paper/referee_math_b.md logs/paper/referee_audit.md
  logs/paper/referee_journal.md logs/paper/referee_replication.md
  # ... the two reports of the second round, on the revised version (their programs: pattern below), ...
  logs/paper/referee2_math.md logs/paper/referee2_quality.md
  # ... the literature report and the list of issues of the first version, which the reports and the
  #     responses cite, ...
  logs/paper/related_work.md logs/paper/issues.md
  # ... the three programs written from the text of the paper (Table 7), ...
  logs/paper/replication_from_paper/verify.cpp
  logs/paper/referee/audit/ref_audit.cpp logs/paper/referee/audit/expand.out
  logs/paper/referee/b_expand.py logs/paper/referee/b_sigma.py logs/paper/referee/b_geom.py
  logs/paper/referee/b_expand.log
  # ... and the LOST test vectors of Appendix A with their three-way comparison
  logs/paper/lost_vectors/vectors.txt logs/paper/lost_vectors/run.sh logs/paper/lost_vectors/run.log
  logs/paper/lost_vectors/lost_driver.cpp logs/paper/lost_vectors/spotcheck_lost.py
  logs/paper/lost_vectors/treecheck.out logs/paper/lost_vectors/replication.out
  logs/paper/lost_vectors/spotcheck.out
  # ... and the recount of the flow statistics of Appendix S4.1
  logs/paper/flow_counts/flowcount.cpp logs/paper/flow_counts/run.sh logs/paper/flow_counts/run.log
  # --- logs from which running times and counters are quoted in Sections 6 to 8 and Appendices S3, S4, S6
  logs/paper/artifact_test.log logs/repro/tc_all.log logs/search/NOTES.md
  # --- tests of the unpacked archive: after the revision of the paper, and of the final build (the
  #     reference running times of Section 8.3 and Appendix S6.2 are quoted from the latter)
  logs/paper/artifact_test_revision.log logs/paper/artifact_test_final.log
  # --- the script that checks the cross-references, labels and citations of the paper
  logs/paper/check_refs.py
  # --- corrected LaTeX that the response of the first phase of the revision cites
  logs/paper/revision_M_for_C.tex
  # --- the paper
  paper/paper.tex paper/macros.tex paper/refs.bib paper/paper.pdf
)
# 2. patterns (everything that matches is taken; at least one match is required where marked)
shopt -s nullglob
src_extra=( src/*.cpp src/*.hpp )                                   # any further source or header
ip_rest=( results/final/checks/ip_leaf_theta_ge4_rest.shard*.jsonl ) # HiGHS on the leaves with theta >= 4
ip_table9=( results/ip/leaftable_k9.shard*.jsonl )                  # HiGHS on the leaf table, k = 9
ip_cross=( results/crosscheck/ip.shard*of*.jsonl )                  # HiGHS on the cross-check instances
reviews=( logs/review/*.md )                                        # the review and audit reports
responses=( logs/paper/response*.md )                               # responses to the referee reports
revchecks=( logs/paper/revM_checks/* )                              # check scripts cited in the responses
paper=( paper/*.tex paper/*.bib paper/*.bbl paper/*.bst paper/*.cls paper/*.sty )   # sources of the paper
paper_v1=( logs/paper/paper_v1/*.tex logs/paper/paper_v1/*.bib logs/paper/paper_v1/*.pdf )  # the refereed version
replication=( logs/paper/replication_from_paper/* )                 # the verifier written from the PDF alone
referee=( logs/paper/referee/* logs/paper/referee/audit/* )         # programs and outputs of the referees
referee2=( logs/paper/referee2/* )                                  # programs and outputs of the second round
round2=( logs/paper/round2_checks/* )                               # checks of the changes of the second round
logs_final=( logs/final/*.log )                                     # logs of the certified runs
logs_repro=( logs/repro/*.log )                                     # logs of the validation run of reproduce.sh
logs_search=( logs/search/*.log )                                   # logs of the search experiments (Appendix S3.2)
shopt -u nullglob
[ ${#ip_rest[@]}   -ge 1 ] || die "no results/final/checks/ip_leaf_theta_ge4_rest.shard*.jsonl"
[ ${#ip_table9[@]} -ge 1 ] || die "no results/ip/leaftable_k9.shard*.jsonl"
[ ${#ip_cross[@]}  -ge 1 ] || die "no results/crosscheck/ip.shard*of*.jsonl"
[ ${#reviews[@]}   -ge 1 ] || die "no logs/review/*.md"
[ ${#responses[@]} -ge 1 ] || die "no logs/paper/response*.md (the responses to the referee reports)"
[ ${#paper_v1[@]}  -ge 1 ] || die "no logs/paper/paper_v1/ (the refereed version of the paper)"
[ -f paper/paper.pdf ]     || die "paper/paper.pdf not present (build the paper first)"
if [ -n "$(find paper -maxdepth 1 \( -name '*.tex' -o -name '*.bib' \) -newer paper/paper.pdf)" ]; then
  if [ $STALE_PDF_OK -eq 1 ]; then echo "make_artifact: WARNING: paper/paper.pdf is older than the sources of the paper" >&2
  else die "paper/paper.pdf is older than the sources of the paper (rebuild the paper first)"; fi
fi

# Deliberately NOT included (all of it can be regenerated, see README.md):
#   bin/                                   binaries (make bin/<name>)
#   results/final/checks/leaves_all.txt    byte-identical to results/final/leaves.txt
#   results/final/checks/leaves_theta*.txt, perturbed.txt     written by scripts/leaf_instances.py
#   results/final/sim/sim_snapshot_0806.*  the source is byte-identical to sim/sim.cpp (checked below)
#   results/search, results/repro, results/rehearsal, ...     earlier or repeated runs
#   logs/                                  except the files and patterns listed above
#   executables and pickled data in logs/paper/referee/ (filtered out below)
if [ -f results/final/sim/sim_snapshot_0806.cpp ]; then
  cmp -s results/final/sim/sim_snapshot_0806.cpp sim/sim.cpp \
    || die "sim/sim.cpp differs from the snapshot used for the 10^6-game run (results/final/sim/sim_snapshot_0806.cpp)"
fi
if [ -f results/final/checks/leaves_all.txt ]; then
  cmp -s results/final/checks/leaves_all.txt results/final/leaves.txt \
    || die "results/final/checks/leaves_all.txt differs from results/final/leaves.txt"
fi

files=( "${required[@]}" )
files+=( "${src_extra[@]}" "${ip_rest[@]}" "${ip_table9[@]}" "${ip_cross[@]}" "${reviews[@]}" "${responses[@]}"
         "${paper[@]}" "${paper_v1[@]}" "${logs_final[@]}" "${logs_repro[@]}" "${logs_search[@]}" )
# referee material: regular text files only (no executables, no pickled data, no directories)
for f in "${replication[@]}" "${referee[@]}" "${revchecks[@]}" "${referee2[@]}" "${round2[@]}"; do
  [ -f "$f" ] || continue
  case "$f" in *.pkl|*.pyc|*.o) continue ;; esac
  if file -b "$f" | grep -q -i -E 'executable|shared object|ELF'; then
    case "$f" in *.sh|*.py) ;; *) continue ;; esac
  fi
  files+=( "$f" )
done

LIST=$(printf '%s\n' "${files[@]}" | LC_ALL=C sort -u)
while IFS= read -r f; do
  [ -f "$f" ] || die "missing file: $f"
done <<< "$LIST"

# every file named in the paper with a directory must be in the list (a coarse automatic check of the RULE)
missing=0
for p in $(grep -h -o '\\fn{[^}]*}' paper/*.tex | sed 's/\\fn{//; s/}$//' | grep '/' | grep -v '[*<>$\\ ]' | sort -u); do
  case "$p" in
    */) continue ;;                                # a directory
    bin/*|results/repro/*) continue ;;             # regenerated by the scripts
  esac
  if ! grep -q -x -F "$p" <<< "$LIST"; then echo "make_artifact: named in the paper but not packaged: $p" >&2; missing=1; fi
done
[ $missing -eq 0 ] || die "the paper names files that are not in the archive"

# the number of files printed in Section 8.1 of the paper (\\ArchiveFiles in paper/macros.tex) and in
# README.md must be the number of files of the archive, MANIFEST.sha256 included
nlist=$(( $(wc -l <<< "$LIST") + 1 ))
nmacro=$(sed -n 's/^\\newcommand{\\ArchiveFiles}{\([0-9]*\)}.*/\1/p' paper/macros.tex)
if [ "$nmacro" != "$nlist" ] || ! grep -q "has $nlist files" README.md; then
  msg="the archive has $nlist files, paper/macros.tex says '${nmacro:-nothing}' (\\ArchiveFiles) and README.md must say 'has $nlist files'"
  if [ $STALE_PDF_OK -eq 1 ] || [ $DRY -eq 1 ]; then echo "make_artifact: WARNING: $msg" >&2
  else die "$msg; update both, rebuild the paper and run this script again"; fi
fi

if [ $DRY -eq 1 ]; then
  echo "$LIST"
  echo "make_artifact: dry run, $(wc -l <<< "$LIST") files, nothing written" >&2
  exit 0
fi

# ---------------------------------------------------------------------------------------------- staging
rm -rf "$STAGE" "$TARBALL"
mkdir -p "$STAGE"
while IFS= read -r f; do
  mkdir -p "$STAGE/$(dirname "$f")"
  cp -p "$f" "$STAGE/$f"
done <<< "$LIST"

# neutral paths: some referee scripts and reports contain the absolute path of a temporary working
# directory of the session that wrote them.  It carries no information; it is replaced by /tmp/scratch in
# the staged copies (the files in the project tree are not touched).
TMP_RE='/tmp/[a-z]+-[0-9]+/-home-[a-z]+/[0-9a-f-]+/scratchpad'
grep -r -l -I -E "$TMP_RE" "$STAGE" 2>/dev/null | while IFS= read -r f; do
  sed -i -E "s#$TMP_RE#/tmp/scratch#g" "$f"
  echo "make_artifact: temporary path replaced in ${f#"$STAGE"/}" >&2
done
if grep -r -l -I -E '/tmp/[a-z]+-[0-9]+/-home-' "$STAGE" >/dev/null 2>&1; then
  die "a temporary session path is left in the staging directory"
fi

# runnable copies: the transcription of Algorithm 1 that is listed in Table 7 of the paper
# (logs/paper/referee/b_expand.py with b_sigma.py) reads the certificate from the absolute path of the
# project directory and writes its reachable set into the temporary directory replaced above.  In the
# staged copies the certificate is read relative to the script, and the reachable set is written to (and
# read by b_play6.py from) the system's temporary directory, so that
#       python3 logs/paper/referee/b_expand.py
# runs from the root of the unpacked archive.  Two lines of code change; nothing else does.
sed -i "s#def load_certificate(root='/home/user/ttt6/results/final/'):#def load_certificate(root=__import__('os').path.join(__import__('os').path.dirname(__import__('os').path.abspath(__file__)), '..', '..', '..', 'results', 'final', '')):#" "$STAGE/logs/paper/referee/b_sigma.py"
sed -i "s#'/tmp/scratch/b_reachable.pkl'#__import__('os').path.join(__import__('tempfile').gettempdir(), 'b_reachable.pkl')#" "$STAGE/logs/paper/referee/b_expand.py" "$STAGE/logs/paper/referee/b_play6.py"
grep -q "abspath(__file__)), '..', '..', '..', 'results', 'final', '')):" "$STAGE/logs/paper/referee/b_sigma.py" || die "b_sigma.py: the path of the certificate was not rewritten"
grep -q "gettempdir(), 'b_reachable.pkl')" "$STAGE/logs/paper/referee/b_expand.py" || die "b_expand.py: the path of the reachable set was not rewritten"
grep -q "gettempdir(), 'b_reachable.pkl')" "$STAGE/logs/paper/referee/b_play6.py" || die "b_play6.py: the path of the reachable set was not rewritten"
if grep -q "/home/user\|/tmp/scratch" "$STAGE/logs/paper/referee/b_sigma.py" "$STAGE/logs/paper/referee/b_expand.py"; then
  die "an absolute path is left in the staged copies of b_sigma.py or b_expand.py"
fi

# no binaries, no caches
if find "$STAGE" -type f \( -name '*.o' -o -name '*.pyc' -o -name '*.bin' -o -name '*.pkl' \) | grep -q .; then
  die "binary or cache file in the staging directory"
fi
if find "$STAGE" -type f -exec file -b {} + | grep -q -E '^ELF'; then
  die "executable in the staging directory"
fi

# ---------------------------------------------------------------------------------------------- manifest
( cd "$STAGE" && find . -type f ! -name MANIFEST.sha256 -printf '%P\n' | LC_ALL=C sort | xargs -d '\n' sha256sum ) > "$STAGE/MANIFEST.sha256"
if [ $WRITE_ROOT_MANIFEST -eq 1 ]; then cp "$STAGE/MANIFEST.sha256" MANIFEST.sha256; else cp "$STAGE/MANIFEST.sha256" "$OUT/MANIFEST.sha256"; fi

# ---------------------------------------------------------------------------------------------- archive
unpacked_kb=$(du -sk "$STAGE" | cut -f1)
tar --sort=name --owner=0 --group=0 --numeric-owner -C "$OUT" -cf - "$NAME" | gzip -9 -n > "$TARBALL"
rm -rf "$STAGE"

if [ $WRITE_ROOT_MANIFEST -eq 1 ]; then nfiles=$(wc -l < MANIFEST.sha256); else nfiles=$(wc -l < "$OUT/MANIFEST.sha256"); fi
bytes=$(stat -c %s "$TARBALL")
echo "wrote $TARBALL: $nfiles files + MANIFEST.sha256, $bytes bytes, $unpacked_kb KB unpacked"
echo "sha256 $(sha256sum < "$TARBALL" | cut -d' ' -f1)  $TARBALL"
echo "wrote MANIFEST.sha256 ($nfiles entries)"
echo "remember: update \\ArchiveSize and \\ArchiveUnpacked in paper/macros.tex (Section 8.1 of the paper) and the sizes in README.md if they changed"

#!/bin/bash
# script_assimilator handler for Lyra: appends everything found by the canonical result to
#   lyra_results/11tuplets.txt        "X,"  one per central 11-tuplet
#   lyra_results/continuations.txt    "13-tuplet" ... "27-tuplet" sections as in the result files
#   lyra_results/progress.txt         one line per finished work unit (wu, k range, counters)
# A 27-tuplet also creates lyra_results/FOUND_27.txt.
# Never exits non-zero for a bad/missing file (that would crash-loop script_assimilator).
# Called as: lyra_assimilate.sh <result_id> <wu_name>   |   --error <error_mask> <wu_name>
set -u
cd /home/bitboinc/projects/bitboinc
DB="mysql --defaults-extra-file=/home/bitboinc/.my.cnf bitboinc -N -B"
if [ "${1:-}" = "--error" ]; then echo "lyra_assimilate: WU $3 failed (error $2)" >&2; exit 0; fi
result_id=$1; wu_name=$2
phys=$($DB -e "SELECT xml_doc_in FROM result WHERE id=$result_id" 2>/dev/null | grep -oP '(?<=<file_name>)[^<]+' | head -1)
outfile=""; [ -n "$phys" ] && outfile=$(bin/dir_hier_path "$phys" | sed 's#/download/#/upload/#')
[ -s "$outfile" ] || { echo "lyra_assimilate: output for result $result_id ($wu_name) is gone, skipping" >&2; exit 0; }
mkdir -p lyra_results
tr -d '\r' < "$outfile" | awk -v wu="$wu_name" '
    NR == 1 { print wu " " $0 >> "lyra_results/progress.txt"; next }
    /^[0-9]+,$/ { print >> "lyra_results/11tuplets.txt"; next }
    /^[0-9]+-tuplet$/ { sect = $0; print "# " wu >> "lyra_results/continuations.txt"; print >> "lyra_results/continuations.txt"; if (sect == "27-tuplet") big = 1; next }
    { print >> "lyra_results/continuations.txt"; if (big && sect == "27-tuplet") print wu ": " $0 >> "lyra_results/FOUND_27.txt" }
'
exit 0

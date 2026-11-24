#!/bin/bash
# script_validator compare_script for Lyra (min_quorum 2): the search is deterministic, so two results must be
# identical in everything except the diagnostic counters surv/fermat1 (CPU and GPU differ slightly there).
# Called as: lyra_compare.sh <result_id> <result_id2>.   Offline: lyra_compare.sh --file <out1> <out2>
set -u
DB="mysql --defaults-extra-file=/home/bitboinc/.my.cnf bitboinc -N -B"
resolve() {
    local phys
    phys=$($DB -e "SELECT xml_doc_out FROM result WHERE id=$1" 2>/dev/null | grep -oP '(?<=<name>)[^<]+' | head -1)
    [ -n "$phys" ] || return 1
    bin/dir_hier_path "$phys" | sed 's#/download/#/upload/#'
}
if [ "${1:-}" = "--file" ]; then
    f1=$2; f2=$3
else
    cd /home/bitboinc/projects/bitboinc
    f1=$(resolve "$1") && f2=$(resolve "$2") || { echo "lyra_compare: can't resolve files for $1 / $2" >&2; exit 1; }
fi
[ -s "$f1" ] && [ -s "$f2" ] || { echo "lyra_compare: missing output" >&2; exit 1; }
# surv= and fermat1= in the header are diagnostics that differ slightly between CPU and GPU implementations;
# k range, dobs, hits count and every tuplet line must match exactly.
norm() { tr -d '\r' < "$1" | sed '1s/ surv=[0-9]*//; 1s/ fermat1=[0-9]*//'; }
cmp -s <(norm "$f1") <(norm "$f2") || { echo "lyra_compare: results differ" >&2; exit 1; }
exit 0

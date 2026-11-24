#!/bin/bash
# script_validator init_script for Lyra: is a single result complete and sane?
#   - header: "symm27 v2 k=[a,b] dobs=286720 surv=N fermat1=N hits=N" with the k range of the work unit name;
#   - then exactly `hits` lines "X," (central 11-tuplets, strictly ascending), then optional sections
#     "13-tuplet" ... "27-tuplet" (odd lengths, ascending) each with lines "X: 0, d1, d2, ...".
# Called as: lyra_validate.sh <result_id>.   Offline: lyra_validate.sh --file <result> [wu_name]
set -u
DB="mysql --defaults-extra-file=/home/bitboinc/.my.cnf bitboinc -N -B"
wu=""
if [ "${1:-}" = "--file" ]; then
    outfile=$2; wu=${3:-}
else
    cd /home/bitboinc/projects/bitboinc
    phys=$($DB -e "SELECT xml_doc_out FROM result WHERE id=$1" 2>/dev/null | grep -oP '(?<=<name>)[^<]+' | head -1)
    [ -n "$phys" ] || { echo "lyra_validate: result $1: can't resolve the output file" >&2; exit 1; }
    outfile=$(bin/dir_hier_path "$phys" | sed 's#/download/#/upload/#')
    wu=$($DB -e "SELECT w.name FROM result r JOIN workunit w ON r.workunitid=w.id WHERE r.id=$1" 2>/dev/null)
fi
[ -s "$outfile" ] || { echo "lyra_validate: missing/empty output $outfile" >&2; exit 1; }
tr -d '\r' < "$outfile" | awk -v wu="$wu" '
    function bad(m) { print "lyra_validate: " m > "/dev/stderr"; failed = 1; exit 1 }
    NR == 1 {
        if (!match($0, /^symm27 v2 k=\[[0-9]+,[0-9]+\] dobs=286720 surv=[0-9]+ fermat1=[0-9]+ hits=[0-9]+$/)) bad("bad header: " $0)
        split($3, kk, /[\[\],=]/); a = kk[3]; b = kk[4]
        if (wu != "" && wu != "lyra_" a "_" b && wu != "lyra_" a "_" b "_r2" && wu != "lyra_gpu_" a "_" b) bad("k range " a "-" b " does not match work unit " wu)
        split($5, ss, "="); split($6, ff, "="); split($7, hh, "="); want = hh[2] + 0
        if (ff[2] + 0 > ss[2] + 0 || want > ff[2] + 0) bad("inconsistent counters")
        next
    }
    n < want { n++; if ($0 !~ /^[0-9]+,$/) bad("line " NR ": expected an 11-tuplet start with a comma"); x = substr($0, 1, length($0) - 1)
               if (n > 1 && !(length(x) > length(prev) || (length(x) == length(prev) && x > prev))) bad("11-tuplets not strictly ascending"); prev = x; next }
    /^[0-9]+-tuplet$/ { L = $0 + 0; if (L < 13 || L > 27 || L % 2 == 0 || L <= lastL) bad("bad section " $0); lastL = L; cnt = 0; next }
    { if (lastL == 0) bad("line " NR ": unexpected text"); if ($0 !~ /^[0-9]+: 0(, [0-9]+)+$/) bad("line " NR ": bad continuation line")
      m = split($0, t, ", "); if (m != lastL) bad("line " NR ": expected " lastL " offsets"); cnt++ }
    END { if (!failed && n < want) bad("only " n " of " want " 11-tuplets present") }
'

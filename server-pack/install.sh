#!/bin/bash
# Lyra server pack installer. Run as the BOINC project user, from inside this folder:
#   ./install.sh /path/to/project      (e.g. /home/boincadm/projects/termit)
# Does: copies binaries into apps/lyra/<ver>/<platform>[__planclass]/ with version.xml, copies templates and scripts,
# then runs bin/xadd and bin/update_versions (signs the files with YOUR code-signing key).
# Config edits (plan_class_spec.xml, config.xml, project.xml) are NOT done automatically: see README.md steps 2-3.
set -eu
P=${1:?usage: ./install.sh /path/to/project}
cd "$(dirname "$0")"; H=$PWD
[ -d "$P/bin" ] && [ -d "$P/apps" ] || { echo "not a BOINC project dir: $P" >&2; exit 1; }
mkdir -p "$P/apps/lyra" "$P/templates" "$P/lyra_results"
cp templates/lyra_in templates/lyra_out "$P/templates/"
cp bin/lyra_generator.py bin/lyra_validate.sh bin/lyra_compare.sh bin/lyra_assimilate.sh "$P/bin/"
chmod +x "$P"/bin/lyra_*
sed -i "s#^PROJECT_DIR = .*#PROJECT_DIR = \"$P\"#" "$P/bin/lyra_generator.py"
for d in "$H"/apps/lyra/*/*; do
  ver=$(basename "$(dirname "$d")"); name=$(basename "$d")
  dest="$P/apps/lyra/$ver/$name"; mkdir -p "$dest"
  for f in "$d"/*; do [ "$(basename "$f")" = version.xml ] || cp "$f" "$dest/"; done
  prog=$(ls "$d" | grep -v '^version.xml$' | head -1)
  case "$name" in *__*) pc=${name#*__} ;; *) pc= ;; esac
  { echo "<version>"; echo "  <file><physical_name>$prog</physical_name><main_program/></file>"
    [ -n "$pc" ] && echo "  <plan_class>$pc</plan_class>"; echo "</version>"; } > "$dest/version.xml"
done
cd "$P"
bin/xadd 2>&1 | tail -3 || true
bin/update_versions
echo "Done. Next: README.md steps 2-3, then 'bin/stop; bin/start'."

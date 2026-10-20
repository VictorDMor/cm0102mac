#!/bin/bash
# First build from a fresh checkout: export, oracle cases, generation, configure,
# build and test. Day-to-day work uses tools/dev.sh with the same paths.
set -euo pipefail
if [[ $# -ne 1 ]]; then
    echo "Usage: JAVA_HOME=/path/to/jdk $0 <Ghidra installation>" >&2
    exit 2
fi
task_root="$(cd "$(dirname "$0")/.." && pwd)"
export GHIDRA_INSTALL="$1"
project_dir="$task_root/.local/ghidra"
current="$task_root/.local/current"
input_dir="$current/export"
generated_dir="$current/generated"
build_dir="$current/build"
log_dir="$task_root/.local/logs"
if [[ ! -f "$project_dir/cm0102-ppc32.gpr" ]]; then
    echo "First analyze the supplied disc with tools/analyze_mac.sh" >&2
    exit 1
fi
mkdir -p "$current" "$log_dir"
# Export again when the cached export lacks any root in discovery_roots.txt,
# so newly listed code is never silently omitted.
if [[ ! -f "$input_dir/program.json" ]] || ! python3 - "$input_dir/program.json" "$task_root/tools/discovery_roots.txt" <<'PY'
import json, re, sys
roots = set(json.load(open(sys.argv[1])).get("explicitDiscoveryRoots", []))
wanted = {int(m, 16) for m in re.findall(r"^0x[0-9a-fA-F]+", open(sys.argv[2]).read(), re.M)}
sys.exit(not wanted <= roots)
PY
then
    "$task_root/tools/dev.sh" export
fi
python3 "$task_root/tools/make_aot_cases.py" "$input_dir/cases.json"
# Remove only the derived oracle, so a failed Ghidra script cannot silently use
# results from an earlier, different case specification.
rm -f "$input_dir/oracle.tsv"
"$GHIDRA_INSTALL/support/analyzeHeadless" "$project_dir" cm0102-ppc32 \
    -process 'CM 01-02' -readOnly -noanalysis -scriptPath "$task_root/tools" \
    -postScript ExportOracle.java "$input_dir/cases.json" "$input_dir/oracle.tsv" \
    -max-cpu 4 > "$log_dir/aot-oracle.log" 2>&1
test -f "$input_dir/oracle.tsv"
mkdir -p "$generated_dir"
python3 "$task_root/tools/check_jump_tables.py" "$input_dir"
python3 "$task_root/tools/compile_recomp.py" "$input_dir" "$generated_dir" \
    --root 0x10773710 --all-analyzed
cmake -S "$task_root/native" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
    -DCM_GENERATED_DIR="$generated_dir" -DCM_ORACLE_DIR="$input_dir"
cmake --build "$build_dir" -j6
ctest --test-dir "$build_dir" --output-on-failure
file "$build_dir/cm-aot-probe"

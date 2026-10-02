#!/bin/bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
    echo "Usage: JAVA_HOME=/path/to/jdk $0 <Ghidra installation>" >&2
    exit 2
fi
task_root="$(cd "$(dirname "$0")/.." && pwd)"
ghidra_install="$1"
project_dir="$task_root/.local/ghidra"
input_dir="$task_root/.local/recomp-input"
generated_dir="$task_root/.local/recompiled"
build_dir="$task_root/.local/aot-build"
log_dir="$task_root/.local/logs"
if [[ ! -f "$project_dir/cm0102-ppc32.gpr" ]]; then
    echo "First analyze the supplied disc with tools/analyze_mac.sh" >&2
    exit 1
fi
mkdir -p "$input_dir" "$log_dir"
if [[ ! -f "$input_dir/program.json" ]]; then
    # Audited event-loop jump table at 0x1091c330: automatic analysis skipped
    # these case destinations. Discover them offline before exporting the code.
    # The competition result table at 0x108cc9fc also contains indirect cases
    # reached during the 2002 World Cup final; discover all seven destinations.
    "$ghidra_install/support/analyzeHeadless" "$project_dir" cm0102-ppc32 \
        -process 'CM 01-02' -readOnly -noanalysis -scriptPath "$task_root/tools" \
        -postScript ExportRecomp.java "$input_dir" \
        0x1076ffa8 0x1076ffb4 0x1076ffcc 0x1076ffd8 0x1076ffe4 0x10770148 \
        0x101b8a4c 0x101b8908 0x101b87c0 0x101b8680 0x101b8adc 0x101b8548 0x101b84cc \
        -max-cpu 4 > "$log_dir/recomp-export.log" 2>&1
    test -f "$input_dir/program.json"
fi
python3 "$task_root/tools/index_recomp.py" "$input_dir"
python3 "$task_root/tools/make_aot_cases.py" "$input_dir/cases.json"
# Remove only the derived oracle, so a failed Ghidra script cannot silently use
# results from an earlier, different case specification.
rm -f "$input_dir/oracle.tsv"
"$ghidra_install/support/analyzeHeadless" "$project_dir" cm0102-ppc32 \
    -process 'CM 01-02' -readOnly -noanalysis -scriptPath "$task_root/tools" \
    -postScript ExportOracle.java "$input_dir/cases.json" "$input_dir/oracle.tsv" \
    -max-cpu 4 > "$log_dir/aot-oracle.log" 2>&1
test -f "$input_dir/oracle.tsv"
python3 "$task_root/tools/compile_recomp.py" "$input_dir" "$generated_dir" \
    --root 0x10773710 --all-analyzed
cmake -S "$task_root/native" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
    -DCM_GENERATED_DIR="$generated_dir" -DCM_ORACLE_DIR="$input_dir"
cmake --build "$build_dir" -j6
ctest --test-dir "$build_dir" --output-on-failure
file "$build_dir/cm-aot-probe"

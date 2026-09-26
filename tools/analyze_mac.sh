#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <Ghidra installation directory> <mounted CM disc directory>" >&2
    exit 2
fi
task_root="$(cd "$(dirname "$0")/.." && pwd)"
ghidra_install="$1"
media_root="$2"
project_dir="$task_root/.local/ghidra"
report_dir="$task_root/reports/generated/ghidra"
mkdir -p "$project_dir" "$report_dir"
if [[ -f "$project_dir/cm0102-ppc32.gpr" ]]; then
    operation=(-process 'CM 01-02' -readOnly -noanalysis)
else
    # The PEF loader's automatic language selection can choose PPC VLE/64-bit.
    # This retail binary uses standard big-endian 32-bit PowerPC instructions.
    operation=(-import "$media_root/Championship Manager/CM 01-02"
        -processor PowerPC:BE:32:default -cspec default -analysisTimeoutPerFile 180)
fi
exec "$ghidra_install/support/analyzeHeadless" "$project_dir" cm0102-ppc32 \
    "${operation[@]}" -scriptPath "$task_root/tools" \
    -postScript AuditProgram.java "$report_dir" -max-cpu 4

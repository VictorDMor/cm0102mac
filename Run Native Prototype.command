#!/bin/bash
# Finder launcher for the existing diagnostic build; it does not build the port.
set -u

project_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)" || exit 1
installation_dir="$HOME/Library/Application Support/CM 01-02 Native"
game_dir="$installation_dir/Game"
binary="$project_dir/.local/aot-build/cm-aot-probe"

finish() {
    local result="$1"
    if [[ -t 0 ]]; then
        printf '\nPress Return to close this launcher. '
        read -r answer
    fi
    exit "$result"
}

printf 'CM 01-02 — Native ARM64 prototype\n\n'
printf 'This development build is not playable yet.\n'
printf 'It starts the original opening sequence, then stops at an unfinished service or the diagnostic limit.\n'
printf 'The opening can take about 30 seconds. A log and window images will be saved.\n\n'

if [[ ! -x "$binary" || ! -f "$project_dir/.local/recomp-input/program.json" ]]; then
    printf 'The local native build or its original program data is missing.\n'
    printf 'Restore or build the prototype before running this launcher.\n'
    finish 2
fi
if [[ ! -f "$game_dir/CM 01-02" ]]; then
    printf 'Local game data is not installed.\n'
    printf 'Run Install Native Prototype.command once, then run this launcher again.\n'
    finish 2
fi
if ! mkdir -p "$project_dir/.local/prototype-runs"; then
    printf 'Cannot create the local run directory.\n'
    finish 2
fi
run_dir="$(mktemp -d "$project_dir/.local/prototype-runs/run.XXXXXX")" || finish 2
printf 'Starting the native prototype...\n'

CM_AOT_INSTRUCTION_BUDGET="${CM_AOT_INSTRUCTION_BUDGET:-10000000000}" \
CM_AOT_TRACE_AUDIO=1 \
CM_AOT_CAPTURE_DIRECTORY="$run_dir/images" \
    "$binary" "$project_dir/.local/recomp-input" 0x10773710 \
    "$game_dir" "$installation_dir/Runtime" > "$run_dir/startup.log" 2>&1
result=$?

if [[ "$result" -eq 0 ]]; then
    printf '\nThe native program returned. This does not establish a playable game.\n'
else
    printf '\nThe prototype stopped (exit status %s). This development build is not a playable game yet.\n' "$result"
fi
printf '\nLast diagnostic messages:\n'
tail -n 8 "$run_dir/startup.log"
printf '\nRun files: %s\n' "$run_dir"
printf 'Private game files: %s\n' "$installation_dir/Runtime"
finish "$result"

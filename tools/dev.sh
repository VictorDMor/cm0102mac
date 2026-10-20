#!/bin/bash
# Everyday build commands. Paths live under .local/current (see CLAUDE.md):
#   export     Ghidra export with tools/discovery_roots.txt, then index and check it
#   generate   regenerate C++ from the export; rewrites only pages that changed
#   build      incremental build of the app and every test binary already built
#   test       run every test whose binary exists, or pass ctest arguments
#   install    install the built app; game data and saves are preserved
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
current="$root/.local/current"
export_dir="$current/export"
generated="$current/generated"
build="$current/build"
# Machine settings; variables already set (e.g. by build_aot.sh) win.
if [[ -f "$current/env" ]]; then
    while IFS='=' read -r key value; do
        if [[ -z "${!key:-}" ]]; then export "$key=$value"; fi
    done < <(grep -E '^[A-Z_][A-Z0-9_]*=' "$current/env")
fi
export DEVELOPER_DIR="${DEVELOPER_DIR:-/Library/Developer/CommandLineTools}"
jobs="$(sysctl -n hw.ncpu)"

# The app plus test binaries that exist already. Building every test target
# links ~140 more 170 MB executables, so build others only when needed.
default_targets() {
    local known name
    known="$(cmake --build "$build" --target help | sed -n 's/^\.\.\. \([^ ]*\).*/\1/p')"
    echo cm-native-prototype
    for file in "$build"/cm-*; do
        name="$(basename "$file")"
        if [[ -f "$file" && -x "$file" ]] && grep -qxF -- "$name" <<<"$known"; then echo "$name"; fi
    done
}

# Tests whose binary and absolute-path inputs exist, and whose fixture setup
# tests are runnable too (ctest adds those to any selection), as a regex.
runnable_tests() {
    ctest --test-dir "$build" --show-only=json-v1 2>/dev/null | python3 -I -c '
import json, os, re, sys
tests = json.load(sys.stdin)["tests"]
def prop(test, name):
    return next((p["value"] for p in test.get("properties", []) if p["name"] == name), [])
setup = {}
for test in tests:
    for fixture in prop(test, "FIXTURES_SETUP"): setup.setdefault(fixture, []).append(test["name"])
runnable = {t["name"] for t in tests if t.get("command") and
            all(os.path.exists(arg) for arg in t["command"] if arg.startswith("/"))}
changed = True
while changed:
    changed = False
    for test in tests:
        if test["name"] in runnable and any(not set(setup.get(f, [])) <= runnable for f in prop(test, "FIXTURES_REQUIRED")):
            runnable.discard(test["name"]); changed = True
print(f"Running {len(runnable)} of {len(tests)} tests; the rest lack a built binary or input files.", file=sys.stderr)
print("^(" + "|".join(re.escape(n) for n in sorted(runnable)) + ")$")'
}

command="${1:-}"
[[ $# -gt 0 ]] && shift
case "$command" in
export)
    : "${GHIDRA_INSTALL:?Set GHIDRA_INSTALL in .local/current/env}"
    if [[ -e "$export_dir" && ! -L "$export_dir" ]]; then
        echo "$export_dir is a real directory; move it into .local/exports and link it instead" >&2
        exit 1
    fi
    out="$root/.local/exports/$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$out"
    roots=()
    while read -r address; do roots+=("$address"); done < <(grep -Eo '^0x[0-9a-fA-F]+' "$root/tools/discovery_roots.txt")
    echo "Exporting with ${#roots[@]} discovery roots into $out (a few minutes)"
    "$GHIDRA_INSTALL/support/analyzeHeadless" "$root/.local/ghidra" cm0102-ppc32 \
        -process 'CM 01-02' -readOnly -noanalysis -scriptPath "$root/tools" \
        -postScript ExportRecomp.java "$out" "${roots[@]}" -max-cpu 4 > "$out/export.log" 2>&1
    test -f "$out/program.json"
    python3 "$root/tools/index_recomp.py" "$out"
    python3 "$root/tools/check_jump_tables.py" "$out"
    ln -sfn "$out" "$export_dir"
    echo "Current export: $out"
    ;;
generate)
    python3 "$root/tools/check_jump_tables.py" "$export_dir"
    stamp="$(mktemp)"
    python3 "$root/tools/compile_recomp.py" "$export_dir" "$generated" --root 0x10773710 --all-analyzed
    echo "Changed pages:"
    find "$generated/" -name 'page_*.cpp' -newer "$stamp" | sort | sed 's|.*/|  |'
    rm -f "$stamp"
    ;;
build)
    if [[ $# -eq 0 ]]; then
        targets=()
        while read -r target; do targets+=("$target"); done < <(default_targets)
        set -- "${targets[@]}"
    fi
    cmake --build "$build" -j "$jobs" --target "$@"
    ;;
test)
    if [[ $# -eq 0 ]]; then set -- -R "$(runnable_tests)"; fi
    ctest --test-dir "$build" -j "$jobs" --output-on-failure "$@"
    ;;
install)
    python3 "$root/tools/install_native.py" "$@"
    ;;
*)
    sed -n '2,7p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
esac

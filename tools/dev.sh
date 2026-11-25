#!/bin/bash
# Everyday build commands. Paths live under .local/current (see CLAUDE.md):
#   export     Ghidra export with tools/discovery_roots.txt, then index and check it
#   generate   regenerate C++ from the export; rewrites only pages that changed
#   build      incremental build of the app and every test binary already built
#   test       run every test that can run headless here, or pass ctest arguments
#   install    install the built app; game data and saves are preserved
#   pgo        refresh the profile-guided optimisation profile (instrumented build, training runs, merge,
#              normal build); steps: select, instrument, train, merge, use, status, clear (see docs/native-build-storage.md)
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
pgo_dir="$current/pgo"
jobs="${CM_BUILD_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

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

# New targets and tests in CMakeLists.txt are unknown until CMake reconfigures.
# CMake leaves the Makefile untouched when its content is unchanged.
reconfigure() {
    if [[ "$root/native/CMakeLists.txt" -nt "$build/Makefile" ]]; then
        cmake "$build" >/dev/null && touch "$build/Makefile"
    fi
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
# Tests that start an app bundle show it on screen; run those with -R.
runnable = {t["name"] for t in tests if t.get("command") and
            all(os.path.exists(arg) for arg in t["command"] if arg.startswith("/")) and
            not any(".app/" in arg for arg in t["command"])}
changed = True
while changed:
    changed = False
    for test in tests:
        if test["name"] in runnable and any(not set(setup.get(f, [])) <= runnable for f in prop(test, "FIXTURES_REQUIRED")):
            runnable.discard(test["name"]); changed = True
print(f"Running {len(runnable)} of {len(tests)} tests; the rest lack a built binary or input files, or open an app.", file=sys.stderr)
print("^(" + "|".join(re.escape(n) for n in sorted(runnable)) + ")$")'
}

# Profile-guided optimisation. `build` uses $pgo_dir/cm.profdata when `pgo` finished writing it and the marker
# names the compiler in use (an indexed profile of another Clang may not read); CM_PGO=off builds without it.
# A profile older than the sources still builds: Clang ignores the functions whose control flow changed.
# An instrumented build is for training only, so `build` always switches the instrumentation back off.
compiler_id() { c++ --version | head -1; }
cache_value() { sed -n "s/^$1:[A-Z]*=//p" "$build/CMakeCache.txt" 2>/dev/null | head -1; }
sync_pgo_config() {
    local want=""
    if [[ "${CM_PGO:-}" != off && -f "$pgo_dir/cm.profdata" && -f "$pgo_dir/cm.profdata.ok" &&
          "$(sed -n 1p "$pgo_dir/cm.profdata.ok")" == "$(compiler_id)" ]]; then
        want="$pgo_dir/cm.profdata"
    fi
    if [[ "$(cache_value CM_PGO_PROFILE)" != "$want" || "$(cache_value CM_PGO_GENERATE)" == ON ]]; then
        cmake -DCM_PGO_GENERATE=OFF "-DCM_PGO_PROFILE=$want" "$build" >/dev/null
    fi
    if [[ -n "$want" ]]; then echo "PGO: building with $want ($(sed -n 2p "$pgo_dir/cm.profdata.ok"))" >&2; fi
}

# One build or generation at a time in the shared directories: an exclusive lock on fd 9, held until exit.
take_build_lock() {
    mkdir -p "$build"
    exec 9>"$build/.dev-build.lock"
    perl -e 'use Fcntl ":flock"; open(my $f, ">&=", 9) or die; flock($f, LOCK_EX) or die'
}

pgo_command() {
    local step="${1:-all}"
    case "$step" in
    all)
        for s in select instrument train merge; do "$0" pgo "$s"; done
        "$0" build
        ;;
    select)
        # The generated pages the training builds instrument (tools/pgo_pages.py says why not all): the busiest
        # in the game's own workload, sampled on the normal app (tools/pgo_train.py: holiday and season, every
        # thread, workers included). Rewritten only when the list changes.
        take_build_lock
        mkdir -p "$pgo_dir/log"
        reconfigure
        sync_pgo_config
        cmake --build "$build" -j "$jobs" --target cm-native-prototype
        rm -rf "$pgo_dir/log/samples"
        python3 "$root/tools/pgo_train.py" ${CM_PGO_SCENARIOS:-holiday season} --sample-dir "$pgo_dir/log/samples"
        python3 -I "$root/tools/pgo_pages.py" "$pgo_dir/pages.txt.new" "$generated" "$pgo_dir/log/samples"/*.sample.txt
        rm -rf "$pgo_dir/log/samples"
        if cmp -s "$pgo_dir/pages.txt" "$pgo_dir/pages.txt.new"; then rm "$pgo_dir/pages.txt.new"; else mv "$pgo_dir/pages.txt.new" "$pgo_dir/pages.txt"; fi
        ;;
    instrument)
        take_build_lock
        df -h / | tail -1
        reconfigure
        [[ -f "$pgo_dir/pages.txt" ]] || { echo "Run: tools/dev.sh pgo select" >&2; exit 1; }
        cmake -DCM_PGO_GENERATE=ON -DCM_PGO_PROFILE= "-DCM_PGO_PAGES=$pgo_dir/pages.txt" "$build" >/dev/null
        cmake --build "$build" -j "$jobs" --target cm-native-prototype cm-world-replay
        mkdir -p "$pgo_dir"; touch "$pgo_dir/instrumented"
        df -h / | tail -1
        ;;
    train)
        take_build_lock
        [[ -f "$pgo_dir/instrumented" && "$(cache_value CM_PGO_GENERATE)" == ON ]] || { echo "Run: tools/dev.sh pgo instrument" >&2; exit 1; }
        rm -rf "$pgo_dir/raw" "$pgo_dir/log"
        mkdir -p "$pgo_dir/raw" "$pgo_dir/log"
        # The game's own workload in driven copies of the instrumented app (tools/pgo_train.py): the counters
        # are written in LLVM's continuous mode, so every thread's work counts and the driver's stop loses
        # nothing. CM_PGO_SCENARIOS picks scenarios (default: holiday season).
        python3 "$root/tools/pgo_train.py" ${CM_PGO_SCENARIOS:-holiday season} --profile-dir "$pgo_dir/raw" \
            >"$pgo_dir/log/game.log" 2>&1 || { tail -c 600 "$pgo_dir/log/game.log" >&2; echo "game training failed" >&2; exit 1; }
        cat "$pgo_dir/log/game.log"
        ls "$pgo_dir/raw"/*.profraw >/dev/null
        du -sh "$pgo_dir/raw"
        ;;
    merge)
        take_build_lock
        ls "$pgo_dir/raw"/*.profraw >/dev/null
        xcrun llvm-profdata merge -sparse -output="$pgo_dir/cm.profdata.new" "$pgo_dir/raw"/*.profraw
        mv "$pgo_dir/cm.profdata.new" "$pgo_dir/cm.profdata"
        printf '%s\ntrained %s at %s on: %s\n' "$(compiler_id)" "$(date +%Y-%m-%d)" "$(git -C "$root" rev-parse --short HEAD)" \
            "$(cd "$pgo_dir/raw" && ls *.profraw | sed 's/\.profraw$//' | tr '\n' ' ')" > "$pgo_dir/cm.profdata.ok"
        rm -rf "$pgo_dir/raw" "$pgo_dir/instrumented"
        ls -l "$pgo_dir/cm.profdata"
        echo "Profile ready; tools/dev.sh build now builds with it (the instrumented build is still in $build until then)."
        ;;
    use)
        "$0" build
        ;;
    status)
        ls -l "$pgo_dir"/cm.profdata* 2>&1; cat "$pgo_dir/cm.profdata.ok" 2>/dev/null
        echo "build: generate=$(cache_value CM_PGO_GENERATE) profile=$(cache_value CM_PGO_PROFILE)"
        ;;
    clear)
        rm -rf "$pgo_dir/cm.profdata" "$pgo_dir/cm.profdata.ok" "$pgo_dir/raw" "$pgo_dir/instrumented"
        echo "Profile removed; the next tools/dev.sh build is a plain build."
        ;;
    *)
        echo "usage: tools/dev.sh pgo [all|select|instrument|train|merge|use|status|clear]" >&2; exit 2
        ;;
    esac
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
    # Pages must not change under a build (the same lock as build).
    mkdir -p "$build"
    exec 9>"$build/.dev-build.lock"
    perl -e 'use Fcntl ":flock"; open(my $f, ">&=", 9) or die; flock($f, LOCK_EX) or die'
    python3 "$root/tools/check_jump_tables.py" "$export_dir"
    stamp="$(mktemp)"
    python3 "$root/tools/compile_recomp.py" "$export_dir" "$generated" --root 0x10773710 --all-analyzed
    echo "Changed pages:"
    find "$generated/" -name 'page_*.cpp' -newer "$stamp" | sort | sed 's|.*/|  |'
    rm -f "$stamp"
    ;;
build)
    # One build or generation at a time in the shared directories (parallel
    # authors): an exclusive lock on fd 9, held until this script exits.
    exec 9>"$build/.dev-build.lock"
    perl -e 'use Fcntl ":flock"; open(my $f, ">&=", 9) or die; flock($f, LOCK_EX) or die'
    if [[ $# -eq 0 ]]; then
        targets=()
        while read -r target; do targets+=("$target"); done < <(default_targets)
        set -- "${targets[@]}"
    fi
    reconfigure
    sync_pgo_config
    cmake --build "$build" -j "$jobs" --target "$@"
    ;;
pgo)
    pgo_command "$@"
    ;;
test)
    reconfigure
    if [[ $# -eq 0 ]]; then set -- -R "$(runnable_tests)"; fi
    ctest --test-dir "$build" -j "$jobs" --output-on-failure "$@"
    ;;
install)
    if [[ "$(cache_value CM_PGO_GENERATE)" == ON ]]; then
        echo "The build tree is instrumented for PGO training; run tools/dev.sh build first." >&2
        exit 1
    fi
    python3 "$root/tools/install_native.py" "$@"
    ;;
*)
    sed -n '2,7p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
esac

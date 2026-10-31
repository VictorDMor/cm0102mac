#!/usr/bin/env python3
"""Run cm-world-replay over every existing capture, in parallel.

  replay_sweep.py census ENTRIES_FILE   which captures reach these entries
  replay_sweep.py pair CM_X_ORIGINAL    original/C++ pair per capture

A pair runs the original and the C++ with the replay's RNG and heap checks, then
the C++ on its production path (--timing skips those checks and their observer):
  MATCH     every run reaches the capture's recorded end state
  STALE     the original itself fails, so the capture proves nothing
  DIFFERS   the original passes and a C++ run does not
  SAME      all pass with the original's instruction count, so the C++ may not have run
A timeout is a TIMEOUT, never a pass. Exit status 1 on any DIFFERS or TIMEOUT, or
when no capture gives a MATCH. A census counts dispatches and only completed runs
print them: it misses same-page and in-place calls, so zero proves nothing, and a
capture that fails or times out says nothing about its entries.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
REPLAY = ROOT / ".local/current/build/cm-world-replay"
NATIVE = ROOT / "native"
CAPTURE_ROOTS = (".local/benchmarks", ".local/performance")


def captures(root=ROOT):
    return sorted(path.parent for directory in CAPTURE_ROOTS for path in (root / directory).rglob("capture.txt"))


def require_original_variable(variable, native=None):
    """A misspelled variable would compare the C++ with itself."""
    native = native or NATIVE
    if not any(f'"{variable}"' in path.read_text(errors="replace") for path in native.glob("*.[ch]pp")):
        raise SystemExit(f"{variable} is not read anywhere in {native}")


def replay(binary, capture, arguments=(), environment=None, timeout=900):
    """(status, output): status is the exit code, or None after the timeout."""
    command = [str(binary), str(capture), "1", *arguments]
    try:
        run = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=timeout,
                             env=environment)
    except subprocess.TimeoutExpired:
        return None, ""
    return run.returncode, run.stdout + run.stderr


def divergence(output):
    found = re.search(r"First divergence: (.{0,80})", output)
    return found[1] if found else None


def counts(output):
    """(instructions, native calls) from the replay's final line; diagnostics only."""
    for line in reversed(output.splitlines()):
        if line.startswith('{"run"'):
            run = json.loads(line)
            return run.get("instructions"), run.get("nativeCalls")
    return None, None


def census(binary, capture, entries, timeout):
    status, output = replay(binary, capture, ("--census", str(entries)), timeout=timeout)
    if status is None:
        return "TIMEOUT", {}
    if status != 0:
        return f"failed: {divergence(output) or f'rc={status}'}", {}
    return "complete", {entry: int(calls) for entry, calls in re.findall(r"^census (0x[0-9a-f]+) (\d+)$", output, re.M)
                        if int(calls)}


def pair_verdict(original, rewrite, production=None):
    """The verdict and a note, from (status, output) results. Without a
    production result the original had failed or timed out."""
    (original_status, original_output), (rewrite_status, rewrite_output) = original, rewrite
    results = {"original": original_status, "C++": rewrite_status, "C++ production": (production or (0,))[0]}
    timed_out = [name for name, status in results.items() if status is None]
    if timed_out:
        return "TIMEOUT", ", ".join(timed_out)
    if original_status != 0:
        return "STALE", divergence(original_output) or f"original rc={original_status}"
    if rewrite_status != 0:
        return "DIFFERS", divergence(rewrite_output) or f"C++ rc={rewrite_status}"
    if production[0] != 0:
        return "DIFFERS", f"production: {divergence(production[1]) or f'rc={production[0]}'}"
    (instructions, calls), (rewrite_instructions, rewrite_calls) = counts(original_output), counts(rewrite_output)
    if instructions is None or rewrite_instructions is None:
        return "DIFFERS", "a run printed no final line"
    if instructions == rewrite_instructions:
        return "SAME", f"{instructions} instructions on both sides"
    note = f"{instructions} and {rewrite_instructions} instructions"
    return "MATCH", note if calls == rewrite_calls else f"{note}; native calls {calls} and {rewrite_calls}"


def pair(binary, capture, variable, timeout):
    rewrite_environment = {name: value for name, value in os.environ.items() if name != variable}
    original = replay(binary, capture, environment={**rewrite_environment, variable: "1"}, timeout=timeout)
    rewrite = replay(binary, capture, environment=rewrite_environment, timeout=timeout)
    # A failed original settles the verdict, whatever the production run would do.
    if original[0] != 0 or rewrite[0] != 0:
        return pair_verdict(original, rewrite)
    return pair_verdict(original, rewrite, replay(binary, capture, ("--timing",), rewrite_environment, timeout))


def label(capture, root=ROOT):
    return str(capture.relative_to(root)) if capture.is_relative_to(root) else str(capture)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=("census", "pair"))
    parser.add_argument("argument", help="census: file of entries, one per line; pair: the module's CM_X_ORIGINAL")
    parser.add_argument("--captures", nargs="*", type=Path, help="these captures rather than every existing one")
    parser.add_argument("--binary", type=Path, default=REPLAY)
    parser.add_argument("--jobs", type=int, default=os.cpu_count())
    parser.add_argument("--timeout", type=int, default=900, help="seconds per replay")
    args = parser.parse_args(argv)
    selected = [path.resolve() for path in args.captures] if args.captures else captures()
    if not selected:
        parser.error("no captures found")
    for capture in selected:
        if not (capture / "capture.txt").exists():
            parser.error(f"{capture} is not a capture")
    if args.mode == "census":
        work = lambda capture: census(args.binary, capture, args.argument, args.timeout)
    else:
        require_original_variable(args.argument)
        work = lambda capture: pair(args.binary, capture, args.argument, args.timeout)
    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(work, selected))
    if args.mode == "census":
        for capture, (status, hits) in zip(selected, results):
            if hits or status != "complete":
                print(f"{label(capture)}: {status}; " + (" ".join(f"{e}={n}" for e, n in hits.items())
                                                        if status == "complete" else "entries unknown"))
        completed = sum(status == "complete" for status, _ in results)
        print(f"{len(selected)} captures: {completed} complete, {sum(bool(hits) for _, hits in results)} reach an entry. "
              "Zero calls don't prove a routine unreachable: same-page and in-place calls aren't counted.")
        return 1 if not completed or any(status == "TIMEOUT" for status, _ in results) else 0
    for capture, (verdict, note) in zip(selected, results):
        print(f"{verdict:8}{label(capture)}: {note}")
    tally = {verdict: sum(first == verdict for first, _ in results) for verdict in ("MATCH", "STALE", "SAME", "DIFFERS", "TIMEOUT")}
    print(" ".join(f"{verdict}={count}" for verdict, count in tally.items()))
    return 1 if tally["DIFFERS"] or tally["TIMEOUT"] or not tally["MATCH"] else 0


if __name__ == "__main__":
    sys.exit(main())

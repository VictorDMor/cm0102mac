#!/usr/bin/env python3
"""Alternating paired timing of two cm-world-replay builds on captured operations.

Usage: paired_replay.py BASE CANDIDATE PAIRS CAPTURE... [--runs N]

Each pair runs both binaries once, in alternating order. A process replays the
capture N times (default 12); the first two runs are warmups, the median of
the rest is the process's time. A pair's reduction is 1 - candidate / base.
Every run must reach the capture's recorded end state (cm-world-replay exits
non-zero otherwise), so a speedup is only reported for identical end states.
"""
import argparse
import json
import statistics
import subprocess
import sys
from pathlib import Path


def measure(binary, capture, runs):
    result = subprocess.run([binary, str(capture), str(runs)], capture_output=True, text=True)
    if result.returncode:
        sys.exit(f"{binary} failed on {capture}: {result.stderr.strip()}")
    samples = [json.loads(line) for line in result.stdout.splitlines()][2:]
    return statistics.median(s["wall"] for s in samples), statistics.median(s["cpu"] for s in samples)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("base")
    parser.add_argument("candidate")
    parser.add_argument("pairs", type=int)
    parser.add_argument("captures", nargs="+", type=Path)
    parser.add_argument("--runs", type=int, default=12)
    args = parser.parse_args()
    if args.runs < 3:
        parser.error("--runs must be at least 3 (two warmups and one measured run)")
    for capture in args.captures:
        if not (capture / "expected.snapshot").exists():
            parser.error(f"{capture} has no expected.snapshot to check the end state against")
        wall, cpu, base_walls = [], [], []
        for pair in range(args.pairs):
            order = [("base", args.base), ("candidate", args.candidate)]
            if pair % 2: order.reverse()
            results = {label: measure(binary, capture, args.runs) for label, binary in order}
            wall.append(1 - results["candidate"][0] / results["base"][0])
            cpu.append(1 - results["candidate"][1] / results["base"][1])
            base_walls.append(results["base"][0])
        print(f"{capture}: base {statistics.median(base_walls) * 1000:.2f} ms; "
              f"reduction wall {100 * statistics.median(wall):.2f}% cpu {100 * statistics.median(cpu):.2f}%; "
              f"faster pairs {sum(w > 0 for w in wall)}/{len(wall)}; end state identical", flush=True)


if __name__ == "__main__":
    main()

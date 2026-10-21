#!/usr/bin/env python3
"""Charge guest-PC samples from `cm-world-replay --profile` to original routines.

Usage: guest_profile.py SAMPLES... [--export DIR] [--generated DIR] [--top N]

Each sample holds the guest PC, LR and the return addresses on the guest
stack. A routine's self share counts samples whose PC lies in it; its
inclusive share counts samples with it anywhere on the stack. Routine starts
are the export's direct call targets; routines with a C++ owner are labelled
with the owner's family from the generated hooks.
"""
import argparse
import bisect
import collections
import gzip
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def routine_starts(export):
    starts = set()
    call = re.compile(rb'"flow":"UNCONDITIONAL_CALL","fallthrough":\d+,"flows":\[(\d+)\]')
    with gzip.open(export / "instructions.jsonl.gz", "rb") as stream:
        for line in stream:
            match = call.search(line)
            if match: starts.add(int(match.group(1)))
    return sorted(starts)


def owner_labels(generated):
    labels = {}
    hook = re.compile(r"try_(\w+)\(s, 0x([0-9a-f]+)\)")
    for page in generated.glob("page_*.cpp"):
        for name, address in hook.findall(page.read_text(errors="replace")):
            labels.setdefault(int(address, 16), name.removeprefix("modern_"))
    return labels


def samples(paths):
    for path in paths:
        data = Path(path).read_bytes()
        offset = 0
        while offset < len(data):
            (count,) = struct.unpack_from("<I", data, offset)
            yield struct.unpack_from(f"<{count}I", data, offset + 4)
            offset += 4 * (1 + count)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("samples", nargs="+")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--generated", type=Path, default=ROOT / ".local/current/generated")
    parser.add_argument("--top", type=int, default=40)
    args = parser.parse_args()
    starts = routine_starts(args.export)
    labels = owner_labels(args.generated)

    def routine(address):
        index = bisect.bisect_right(starts, address) - 1
        return starts[index] if index >= 0 else address

    self_counts, inclusive = collections.Counter(), collections.Counter()
    total = 0
    for chain in samples(args.samples):
        if not chain: continue
        total += 1
        self_counts[routine(chain[0])] += 1
        for start in {routine(address) for address in chain if address >= 0x10000000}:
            inclusive[start] += 1
    print(f"{total} samples")
    for title, counts in (("inclusive", inclusive), ("self", self_counts)):
        print(f"\n{title:>9}   self  routine")
        for start, count in counts.most_common(args.top):
            print(f"{100 * inclusive[start] / total:8.1f}% {100 * self_counts[start] / total:6.1f}%  "
                  f"0x{start:08x} {labels.get(start, '')}")


if __name__ == "__main__":
    main()

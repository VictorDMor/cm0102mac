#!/usr/bin/env python3
"""List the generated pages worth instrumenting for a PGO training build.

Usage: pgo_pages.py OUT GENERATED_DIR SAMPLES... [--coverage 0.98] [--max-pages 200]

SAMPLES are files written by `cm-world-replay --profile` (the guest PC and return addresses at each
millisecond of CPU time). A page is charged the samples whose guest PC lies in it; the busiest pages up
to the coverage share of all samples (and at most --max-pages) are written to OUT, one `page_XXXXX` per line.

Why not every page: instrumentation doubles the machine code of a page, and an executable whose code is
much more than 250 MB cannot be linked (arm64 branches reach 128 MB and the linker's islands add one hop).
The pages no sample reaches keep their normal code, which is also what a profile without them gives them.
"""
import argparse
import collections
import struct
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out", type=Path)
    parser.add_argument("generated", type=Path)
    parser.add_argument("samples", nargs="+", type=Path)
    parser.add_argument("--coverage", type=float, default=0.98)
    parser.add_argument("--max-pages", type=int, default=200)
    args = parser.parse_args()
    pages, total = collections.Counter(), 0
    for path in args.samples:
        data = path.read_bytes()
        offset = 0
        while offset < len(data):
            (count,) = struct.unpack_from("<I", data, offset)
            chain = struct.unpack_from(f"<{count}I", data, offset + 4)
            offset += 4 * (1 + count)
            if chain and (args.generated / f"page_{chain[0] >> 12:05x}.cpp").exists():
                pages[chain[0] >> 12] += 1
                total += 1
    chosen, covered = [], 0
    for page, count in pages.most_common(args.max_pages):
        if covered >= args.coverage * total: break
        chosen.append(f"page_{page:05x}")
        covered += count
    if not chosen: raise SystemExit("no samples reached a generated page")
    args.out.write_text("\n".join(sorted(chosen)) + "\n")
    print(f"{len(chosen)} of {len(pages)} sampled pages cover {100 * covered / total:.1f}% of {total} samples")


if __name__ == "__main__":
    main()

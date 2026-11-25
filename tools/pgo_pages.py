#!/usr/bin/env python3
"""List the generated pages worth instrumenting for a PGO training build.

Usage: pgo_pages.py OUT GENERATED_DIR SAMPLES... [--coverage 0.98] [--max-pages 200]

SAMPLES are files written by `cm-world-replay --profile` (the guest PC and return addresses at each
millisecond of CPU time), or reports of macOS `sample` on the game (tools/pgo_train.py --sample-dir), told
apart by their text header. From a replay profile a page is charged the samples whose guest PC lies in it;
from a `sample` report, on every thread, each sample is charged to the deepest generated page on its host
stack (the page running, or the page whose call into the runtime or a C++ owner is running). The busiest
pages up to the coverage share of all samples (and at most --max-pages) are written to OUT, one
`page_XXXXX` per line.

Why not every page: instrumentation doubles the machine code of a page, and an executable whose code is
much more than 250 MB cannot be linked (arm64 branches reach 128 MB and the linker's islands add one hop).
The pages no sample reaches keep their normal code, which is also what a profile without them gives them.
"""
import argparse
import collections
import re
import struct
from pathlib import Path


PAGE = re.compile(r"cm::aot::page_([0-9a-f]{5})(?:_impl)?\b")
LINE = re.compile(r"^([ +!:|]*?)(\d+) (.*)$")


def host_samples(text, generated):
    """Samples per page from a `sample` call graph: the self samples of each page function, on every thread.
    A page's calls into C++ owners are not charged to it: handwritten code is always instrumented."""
    charged = collections.Counter()
    nodes = []   # (depth, count, page or None) of the current thread's tree, in the report's order

    def flush():
        for i, (depth, count, page) in enumerate(nodes):
            if page is None: continue
            children, child_depth, j = 0, None, i + 1
            while j < len(nodes) and nodes[j][0] > depth:
                if child_depth is None: child_depth = nodes[j][0]
                if nodes[j][0] == child_depth: children += nodes[j][1]
                j += 1
            if count > children and (generated / f"page_{page:05x}.cpp").exists():
                charged[page] += count - children
        nodes.clear()

    in_graph = False
    for raw in text.splitlines():
        if raw.startswith("Call graph:"):
            in_graph = True; continue
        if not in_graph: continue
        if raw.startswith("Total number in stack") or raw.startswith("Sort by top of stack"):
            break
        if raw.startswith("    ") and "Thread_" in raw[:40]:
            flush(); continue
        m = LINE.match(raw[4:] if raw.startswith("    ") else raw)
        if not m: continue
        found = PAGE.search(m[3].split("  (in ")[0])
        nodes.append((len(m[1]), int(m[2]), int(found[1], 16) if found else None))
    flush()
    return charged


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
        if data.startswith(b"Analysis of sampling"):
            charged = host_samples(data.decode(errors="replace"), args.generated)
            pages.update(charged)
            total += sum(charged.values())
            continue
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

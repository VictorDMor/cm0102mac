#!/usr/bin/env python3
"""Fail when a resolvable jump table targets code missing from the export.

Ghidra leaves `bctr` flows empty, so the generator cannot see switch cases.
An unexported case compiles fine and stops the game with "Uncompiled original
address" only when play reaches it. This resolves the compiler's TOC-relative
tables (lwz table,d(r2) / cmplwi bound / lwzx / mtctr / bctr) and checks that
every case target was exported. Add missing targets to discovery_roots.txt.
"""
import argparse
import json
from pathlib import Path
import re
import sqlite3
import struct

AUDITED_EXECUTABLE = "91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b"
TOC = 0x108bba90  # r2 of the audited executable
TABLE_BASE = re.compile(r"lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)")
BOUND = re.compile(r"cmplwi (?:cr\d,)?r\d+,(0x[0-9a-f]+|\d+)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    args = parser.parse_args()
    metadata = json.loads((args.export / "program.json").read_text())
    if metadata["executableSHA256"] != AUDITED_EXECUTABLE:
        raise SystemExit("Jump-table check supports only the audited CM 01/02 Mac executable")
    segments = [(s["address"], (args.export / s["file"]).read_bytes()) for s in metadata["segments"]
                if s.get("initialized") and "file" in s and (args.export / s["file"]).exists()]

    def word(address):
        for start, data in segments:
            if start <= address and address + 4 <= start + len(data):
                return struct.unpack_from(">I", data, address - start)[0]
        return None

    with sqlite3.connect(f"file:{args.export / 'instructions.sqlite'}?mode=ro", uri=True) as db:
        assembly = dict(db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions"))
    resolved = unresolved = targets = 0
    missing = []
    for address, text in assembly.items():
        if text != "bctr":
            continue
        before = [assembly.get(address - 4 * k, "") for k in range(1, 12)]
        if not (before[0].startswith("mtspr CTR") and before[1].startswith("lwzx")):
            continue
        table = bound = None
        for line in before[2:]:
            if table is None and (match := TABLE_BASE.match(line)):
                table = word((TOC + int(match.group(1), 0)) & 0xFFFFFFFF)
            if bound is None and (match := BOUND.match(line)):
                bound = int(match.group(1), 0)
        if table is None or bound is None:
            unresolved += 1
            continue
        resolved += 1
        for case in range(bound + 1):
            target = word(table + 4 * case)
            targets += 1
            if target is None or target not in assembly:
                missing.append((address, case, target))
    print(f"Jump tables: {resolved} resolved, {unresolved} not resolvable by pattern, {targets} case targets")
    for address, case, target in missing:
        shown = "unreadable" if target is None else f"0x{target:x}"
        print(f"  bctr 0x{address:x} case {case}: {shown} is not in the export")
    if missing:
        raise SystemExit(f"{len(missing)} jump-table targets are missing; add them to tools/discovery_roots.txt and re-export")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Repair the audited English club-index mismatch in a separate Mac save copy."""
import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from language_compat import align_english, sha256

TABLE_START = 8
ENTRY_SIZE = 264
PAYLOAD_START = TABLE_START + 1000 * ENTRY_SIZE


@dataclass
class Entry:
    name: str
    size: int
    payload: bytes


def decompress(payload, expected):
    result = bytearray()
    cursor = 0
    while cursor < len(payload):
        value = payload[cursor]
        cursor += 1
        if value <= 0x80:
            result.append(value)
        else:
            if cursor == len(payload):
                raise ValueError("Truncated compressed run")
            result.extend(bytes([payload[cursor]]) * (value - 0x80))
            cursor += 1
        if len(result) > expected:
            raise ValueError("Compressed block exceeds its declared size")
    if len(result) != expected:
        raise ValueError("Compressed block is shorter than its declared size")
    return bytes(result)


def compress(data):
    result = bytearray()
    cursor = 0
    while cursor < len(data):
        value = data[cursor]
        end = cursor + 1
        while end < len(data) and end - cursor < 127 and data[end] == value:
            end += 1
        count = end - cursor
        if count == 1 and value <= 0x80:
            result.append(value)
        else:
            result.extend((0x80 + count, value))
        cursor = end
    return bytes(result)


def entries(data):
    # This tool deliberately supports only the observed compressed Mac format.
    if len(data) < PAYLOAD_START or data[0] != 4:
        raise ValueError("Expected a compressed Mac save with a 1000-entry directory")
    count = struct.unpack_from(">I", data, 4)[0]
    if not 1 <= count <= 1000:
        raise ValueError("Invalid save entry count")
    offsets = [struct.unpack_from(">I", data, TABLE_START + i * ENTRY_SIZE)[0]
               for i in range(count)] + [len(data)]
    if offsets[0] != PAYLOAD_START or any(a > b for a, b in zip(offsets, offsets[1:])):
        raise ValueError("Invalid save payload boundaries")
    result = []
    for i in range(count):
        start = TABLE_START + i * ENTRY_SIZE
        size = struct.unpack_from(">I", data, start + 4)[0]
        name = data[start + 8:start + ENTRY_SIZE].split(b"\0", 1)[0].decode("mac_roman")
        payload = data[offsets[i]:offsets[i + 1]]
        if size > 512 * 1024 * 1024:
            raise ValueError("Save block exceeds the diagnostic size limit")
        decompress(payload, size)
        result.append(Entry(name, size, payload))
    if len({entry.name for entry in result}) != len(result):
        raise ValueError("Duplicate save entry names")
    return result


def repair(data):
    original = entries(data)
    english = next((i for i, entry in enumerate(original) if entry.name == "eng.lng"), None)
    if english is None:
        raise ValueError("Save has no English language block")
    entry = original[english]
    before = decompress(entry.payload, entry.size)
    after = align_english(before)
    if before == after:
        raise ValueError("Save already contains the aligned English language block")
    repaired_payload = compress(after)
    output = bytearray(data[:PAYLOAD_START])
    for i, entry in enumerate(original):
        struct.pack_into(">I", output, TABLE_START + i * ENTRY_SIZE, len(output))
        output.extend(repaired_payload if i == english else entry.payload)
    checked = entries(output)
    for i, (old, new) in enumerate(zip(original, checked)):
        if old.name != new.name or old.size != new.size:
            raise ValueError("Repair changed a save entry's identity or size")
        if i != english and old.payload != new.payload:
            raise ValueError("Repair changed an unrelated save payload")
    if decompress(checked[english].payload, original[english].size) != after:
        raise ValueError("Repaired language block failed verification")
    return bytes(output), {
        "originalSHA256": sha256(data), "repairedSHA256": sha256(output),
        "entries": len(original), "changedEntry": "eng.lng",
        "unchangedPayloads": len(original) - 1,
        "originalEnglishSHA256": sha256(before), "alignedEnglishSHA256": sha256(after),
        "loadedInGame": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        if args.output.exists():
            raise ValueError("Output already exists; the existing file was preserved")
        repaired, report = repair(args.source.read_bytes())
        with tempfile.TemporaryDirectory(prefix=".save-repair-", dir=args.output.parent) as stage:
            staged = Path(stage) / args.output.name
            subprocess.run(["/usr/bin/ditto", "--rsrc", "--extattr", str(args.source), str(staged)], check=True)
            staged.write_bytes(repaired)
            if staged.read_bytes() != repaired:
                raise ValueError("Written save failed verification")
            # Publish without replacing any existing save, even if it appeared
            # while the repair was running. The resource fork stays on the inode.
            os.link(staged, args.output)
        report["output"] = str(args.output.resolve())
        print(json.dumps(report, indent=2))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Save repair stopped: {error}\n")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Read-only inventory of a mounted CM 01/02 Mac disc and its PEF dependencies.

PEF layout reference: Apple, Mac OS Runtime Architectures, chapter 8.
https://developer.apple.com/library/archive/documentation/mac/pdf/MacOS_RT_Architectures.pdf
This tool parses metadata. It does not execute, translate, or decompile the game.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def read_struct(data, fmt, offset):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"Truncated PEF structure at {offset}")
    return struct.unpack_from(fmt, data, offset)


def cstring(data, offset, limit):
    if not 0 <= offset < limit <= len(data):
        raise ValueError("String outside loader section")
    end = data.find(b"\0", offset, limit)
    if end < 0:
        raise ValueError("Unterminated loader string")
    return data[offset:end].decode("mac_roman")


def parse_pef(data):
    header = read_struct(data, ">4s4s4s5I2HI", 0)
    tag1, tag2, arch, version = header[:4]
    if (tag1, tag2, arch, version) != (b"Joy!", b"peff", b"pwpc", 1):
        raise ValueError("Expected version 1 PowerPC PEF executable")
    sections = []
    kinds = {0: "code", 1: "unpacked_data", 2: "pattern_data", 3: "constant", 4: "loader"}
    keys = ["name_offset", "default_address", "total_size", "unpacked_size", "packed_size",
            "container_offset", "kind_id", "share_kind", "alignment", "reserved"]
    for index in range(header[8]):
        section = dict(zip(keys, read_struct(data, ">i5I4B", 40 + 28 * index)))
        if section["container_offset"] + section["packed_size"] > len(data):
            raise ValueError("Section extends past the file")
        section["index"] = index
        section["kind"] = kinds.get(section["kind_id"], "other")
        sections.append(section)
    loaders = [s for s in sections if s["kind"] == "loader"]
    if len(loaders) != 1:
        raise ValueError("Expected exactly one loader")
    start = loaders[0]["container_offset"]
    end = start + loaders[0]["packed_size"]
    loader_keys = ["main_section", "main_offset", "init_section", "init_offset", "term_section",
                   "term_offset", "library_count", "import_count", "relocation_section_count",
                   "relocation_offset", "string_offset", "export_offset", "export_hash_power", "export_count"]
    loader = dict(zip(loader_keys, read_struct(data, ">iIiIiI8I", start)))
    strings = start + loader["string_offset"]
    symbol_start = start + 56 + 24 * loader["library_count"]
    if symbol_start + loader["import_count"] * 4 > end:
        raise ValueError("Import table exceeds loader section")
    imports = []
    for index in range(loader["import_count"]):
        encoded, = read_struct(data, ">I", symbol_start + index * 4)
        imports.append({"index": index, "name": cstring(data, strings + (encoded & 0xffffff), end),
                        "class": (encoded >> 24) & 0x0f, "weak": bool(encoded & 0x80000000)})
    libraries = []
    for index in range(loader["library_count"]):
        name, old, current, count, first, flags, _, _ = read_struct(data, ">5IBBH", start + 56 + index * 24)
        if first + count > len(imports):
            raise ValueError("Invalid library import range")
        library = {"name": cstring(data, strings + name, end), "weak": bool(flags & 0x40),
                   "old_version": old, "current_version": current, "imports": imports[first:first + count]}
        libraries.append(library)
    source_hints = sorted({x.decode("ascii").strip() for x in re.findall(
        rb"[A-Za-z0-9_./ -]{2,100}\.(?:cpp|c|h)(?=\x00)", data, re.IGNORECASE)})
    return {"format": "PEF", "architecture": "PowerPC 32-bit", "byte_order": "big-endian",
            "sections": sections, "loader": loader, "libraries": libraries,
            "embedded_source_filename_hints": source_hints,
            "note": "Filename strings are not recovered source code; export count is not a function count."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("volume", type=Path)
    parser.add_argument("--output", type=Path, default=Path("reports/generated"))
    args = parser.parse_args()
    root = args.volume.resolve()
    output = args.output.resolve()
    if output == root or root in output.parents:
        parser.error("Output must be outside the input volume")
    executable = root / "Championship Manager" / "CM 01-02"
    binary = executable.read_bytes()
    files = sorted(p for p in root.rglob("*") if p.is_file())
    source_suffixes = {".c", ".cpp", ".cc", ".h", ".hpp", ".m", ".mm", ".swift", ".s", ".asm"}
    source_files = [str(p.relative_to(root)) for p in files if p.suffix.lower() in source_suffixes]
    resource = executable / "..namedfork" / "rsrc"
    report = {"executable": {"relative_path": str(executable.relative_to(root)), "size": len(binary),
                              "sha256": hashlib.sha256(binary).hexdigest()},
              "file_count": len(files), "source_files": source_files,
              "extensions": dict(sorted(Counter(p.suffix.lower() or "<none>" for p in files).items())),
              "pef": parse_pef(binary), "game_execution_verified": False}
    if resource.exists():
        report["executable"]["resource_fork_size"] = resource.stat().st_size
        report["executable"]["resource_fork_sha256"] = digest(resource)
    output.mkdir(parents=True, exist_ok=True)
    (output / "media-audit.json").write_text(json.dumps(report, indent=2) + "\n")
    inventory = [{"path": str(p.relative_to(root)), "size": p.stat().st_size} for p in files]
    (output / "media-files.json").write_text(json.dumps(inventory, indent=2) + "\n")
    readme = root / "Championship Manager" / "Read Me" / "Read Me"
    (output / "original-readme.txt").write_text(readme.read_bytes().decode("mac_roman").replace("\r", "\n"))
    print(f"Audited {len(files)} files; {len(source_files)} source files; "
          f"{report['pef']['loader']['import_count']} imported symbols.")


if __name__ == "__main__":
    main()

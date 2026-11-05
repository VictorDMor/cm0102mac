"""POR_SECOND.CPP source pool, reachable entries and transition-vector ownership."""
import argparse
from bisect import bisect_right
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
MODULE = (0x105773c0, 0x10578be0)
ROUTINES = (
    (0x105773c0, 0x105776e8, "3854eaa46d7325c1"),
    (0x105776f0, 0x105777f4, "659ecfb576f9aac3"),
    (0x10577800, 0x10578680, "4d6c5cd3e9ce2b42"),
    (0x10578680, 0x1057883c, "19dcd5fbef020f66"),
    (0x10578840, 0x10578934, "8ce2e3b957ed6f03"),
    (0x10578940, 0x10578bd8, "3f036504f57f5c97"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
VECTORS = {0x108bb1f4: 0x10578940, 0x108bb1fc: 0x10578840,
           0x108bb204: 0x10577800, 0x108bb20c: 0x105776f0}
OWNED_VTABLE = {8: 0x108bb20c, 0x10: 0x108bb1f4, 0x44: 0x108bb204, 0x94: 0x108bb1fc}
DIRECT_ROOTS = frozenset({0x105773c0, 0x10578680})
SOURCE_REFERENCES = (0x105773d8, 0x10577814, 0x105786a0, 0x10578958)
# These five byte matches straddle unrelated instructions and have unaligned
# targets. They cannot describe PowerPC instruction entries or CFM vectors.
OPCODE_SPANS = {0x100cc6cf: 0x10578006, 0x100cccdf: 0x10578006,
               0x100ccfd3: 0x10578006, 0x1024dadf: 0x10578007, 0x102fd7bf: 0x1057800f}


def production_entries(source):
    entries = tuple(int(value, 16) for value in re.findall(r"^POR_SECOND_ROUTINE\((0x[0-9a-f]+),", source, re.M))
    if entries != tuple(sorted(set(entries))) or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid Portuguese Second Division production registry")
    return entries


POR_SECOND_ENTRIES = production_entries((ROOT / "native/por_second_routines.inc").read_text())


def owner(address):
    index = bisect_right(ENTRIES, address) - 1
    if index >= 0 and address < ROUTINES[index][1]:
        return ENTRIES[index]
    return None


def validate_flows(records, incoming):
    calls = set()
    for source, target, flow in incoming:
        if "CALL" in flow:
            calls.add(target)
        elif owner(source) is None or owner(source) != owner(target):
            raise ValueError(f"Cross-entry POR_SECOND branch {source:#x} -> {target:#x}")
    if calls != DIRECT_ROOTS or calls | set(VECTORS.values()) != set(ENTRIES):
        raise ValueError("POR_SECOND call and transition-vector roots changed")
    reached = set()
    for entry in ENTRIES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if address in reached:
                continue
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid POR_SECOND flow {entry:#x} -> {address:#x}")
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified POR_SECOND computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned POR_SECOND instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for POR_SECOND audit")
    segments = [(segment["address"], (export / segment["file"]).read_bytes())
                for segment in program["segments"] if segment.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address - base:address - base + size]
        raise ValueError(f"Missing POR_SECOND bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in ((0x44a4, 0x1083f93c, b"POR_PRM.CPP\0"),
                              (0x44a0, 0x1083f95c, b"POR_SECOND.CPP\0"),
                              (0x4498, 0x1083f980, b"SCO_FIRST.CPP\0")):
        if word(TOC - slot) != base or read(base, len(label)) != label:
            raise ValueError("Changed POR_SECOND source-pool boundary")
    if word(TOC - 0x5fb8) != 0x108db9f0:
        raise ValueError("Changed POR_SECOND vtable slot")
    for offset, vector in OWNED_VTABLE.items():
        if word(0x108db9f0 + offset) != vector or word(vector) != VECTORS[vector] or word(vector + 4) != TOC:
            raise ValueError("Changed POR_SECOND owned virtual method")
    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x57", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS | OPCODE_SPANS or any(target % 4 == 0 for target in OPCODE_SPANS.values()):
        raise ValueError("Unclassified POR_SECOND original pointer")
    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        for entry, end, expected in ROUTINES:
            if digest(routine_records(db, entry, end)) != expected:
                raise ValueError(f"Changed POR_SECOND body at {entry:#x}")
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x44a0(r2)%",)))
        if refs != SOURCE_REFERENCES:
            raise ValueError("Changed POR_SECOND source references")
        for address in (0x105773b0, 0x10578bd4):
            if routine_records(db, address, address + 4)[0]["assembly"] != "blr":
                raise ValueError("Changed POR_SECOND neighboring return")
        validate_flows(records, db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters POR_SECOND")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted POR_SECOND word at {address:#x}")
    return len(records)


def por_second_hook(entry):
    return f"if (try_modern_por_second(s, 0x{entry:x})) return true;"


def require_por_second_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in POR_SECOND_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", por_second_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing POR_SECOND hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"POR_SECOND.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

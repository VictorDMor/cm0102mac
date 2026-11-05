"""AFRICAN_NATIONS.CPP source pool, reachable entries and transition-vector ownership."""
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
MODULE = (0x104f4c80, 0x104f7110)
ROUTINES = (
    (0x104f4c80, 0x104f4ed4, "d4a464d3aa4376d4"),
    (0x104f4ee0, 0x104f4fe4, "2ad3ef70c1c82716"),
    (0x104f4ff0, 0x104f5908, "425f3fdce9807688"),
    (0x104f5910, 0x104f5c7c, "f15b82b010097a6c"),
    (0x104f5c80, 0x104f5d54, "634f9fc45bf286fd"),
    (0x104f5d60, 0x104f5d94, "7c9700e174372ca7"),
    (0x104f5da0, 0x104f5f34, "0e1b23c3caa1da5d"),
    (0x104f5f40, 0x104f607c, "74147f1ec2661d4d"),
    (0x104f6080, 0x104f613c, "10bc168bad49583b"),
    (0x104f6140, 0x104f639c, "715cb2b8345b3f91"),
    (0x104f63a0, 0x104f660c, "13530e774a222d2c"),
    (0x104f6610, 0x104f69c8, "f0e1c14e1ad8bb89"),
    (0x104f69d0, 0x104f6d44, "2f40e35b76ecf7d6"),
    (0x104f6d50, 0x104f6ff8, "4cdcfaca9618424b"),
    (0x104f7000, 0x104f7110, "acb95cc6f5dd7d68"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
VECTORS = {
    0x108ba684: 0x104f7000,
    0x108ba68c: 0x104f6d50,
    0x108ba694: 0x104f69d0,
    0x108ba69c: 0x104f6080,
    0x108ba6a4: 0x104f5f40,
    0x108ba6ac: 0x104f5da0,
    0x108ba6b4: 0x104f5d60,
    0x108ba6bc: 0x104f5c80,
    0x108ba6c4: 0x104f5910,
    0x108ba6cc: 0x104f4ff0,
    0x108ba6d4: 0x104f4ee0
}
OWNED_VTABLE = {
    0x8: 0x108ba6d4,
    0x10: 0x108ba6a4,
    0x1c: 0x108ba694,
    0x30: 0x108ba69c,
    0x38: 0x108ba6b4,
    0x44: 0x108ba6cc,
    0x4c: 0x108ba6ac,
    0x50: 0x108ba6c4,
    0x64: 0x108ba68c,
    0x70: 0x108ba684,
    0x94: 0x108ba6bc
}
DIRECT_ROOTS = frozenset({0x104f4c80, 0x104f6140, 0x104f63a0, 0x104f6610})
SOURCE_REFERENCES = (
    0x104f4c98,
    0x104f5004,
    0x104f5928,
    0x104f5f50,
    0x104f6088,
    0x104f6158,
    0x104f63b8,
    0x104f6648,
    0x104f69e8,
    0x104f6d64,
)
# Unaligned data bytes happen to begin with a module-range prefix. Neither
# this source position nor its odd target is an aligned code pointer.
NON_POINTER_SPANS = {0x108ef7b2: 0x104f5bc5}


def production_entries(source):
    entries = tuple(int(value, 16) for value in re.findall(r"^AFRICAN_NATIONS_ROUTINE\((0x[0-9a-f]+),", source, re.M))
    if entries != tuple(sorted(set(entries))) or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid African Nations production registry")
    return entries


AFRICAN_NATIONS_ENTRIES = production_entries((ROOT / "native/african_nations_routines.inc").read_text())


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
            raise ValueError(f"Cross-entry AFRICAN_NATIONS branch {source:#x} -> {target:#x}")
    if calls != DIRECT_ROOTS or calls | set(VECTORS.values()) != set(ENTRIES):
        raise ValueError("AFRICAN_NATIONS call and transition-vector roots changed")
    reached = set()
    for entry in ENTRIES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if address in reached:
                continue
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid AFRICAN_NATIONS flow {entry:#x} -> {address:#x}")
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified AFRICAN_NATIONS computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned AFRICAN_NATIONS instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for AFRICAN_NATIONS audit")
    segments = [(segment["address"], (export / segment["file"]).read_bytes())
                for segment in program["segments"] if segment.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address - base:address - base + size]
        raise ValueError(f"Missing AFRICAN_NATIONS bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in ((0x45b0, 0x1083ce6c, b"MINI_LEAGUE.CPP\0"),
                              (0x45ac, 0x1083ce90, b"AFRICAN_NATIONS.CPP\0"),
                              (0x45a8, 0x1083d130, b"EURO_CHAMP.CPP\0")):
        if word(TOC - slot) != base or read(base, len(label)) != label:
            raise ValueError("Changed AFRICAN_NATIONS source-pool boundary")
    if word(TOC - 0x6078) != 0x108d995c:
        raise ValueError("Changed AFRICAN_NATIONS vtable slot")
    for offset, vector in OWNED_VTABLE.items():
        if word(0x108d995c + offset) != vector or word(vector) != VECTORS[vector] or word(vector + 4) != TOC:
            raise ValueError("Changed AFRICAN_NATIONS owned virtual method")
    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x4f", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS | NON_POINTER_SPANS or any(target % 4 == 0 for target in NON_POINTER_SPANS.values()):
        raise ValueError("Unclassified AFRICAN_NATIONS original pointer")
    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        for entry, end, expected in ROUTINES:
            if digest(routine_records(db, entry, end)) != expected:
                raise ValueError(f"Changed AFRICAN_NATIONS body at {entry:#x}")
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x45ac(r2)%",)))
        if refs != SOURCE_REFERENCES:
            raise ValueError("Changed AFRICAN_NATIONS source references")
        for address in (0x104f4c78, 0x104f710c):
            if routine_records(db, address, address + 4)[0]["assembly"] != "blr":
                raise ValueError("Changed AFRICAN_NATIONS neighboring return")
        validate_flows(records, db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters AFRICAN_NATIONS")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted AFRICAN_NATIONS word at {address:#x}")
    return len(records)


def african_nations_hook(entry):
    return f"if (try_modern_african_nations(s, 0x{entry:x})) return true;"


def require_african_nations_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in AFRICAN_NATIONS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", african_nations_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing AFRICAN_NATIONS hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"AFRICAN_NATIONS.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

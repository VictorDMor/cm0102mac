"""GER_FIRST.CPP source pool, reachable entries and transition-vector ownership."""
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
MODULE = (0x10536d50, 0x1053aad0)
ROUTINES = (
    (0x10536d50, 0x10537078, "078f0e18dfc7600f"),
    (0x10537080, 0x10537114, "7b183707f0589395"),
    (0x10537120, 0x10539588, "64da56994dd0b579"),
    (0x10539590, 0x105397b0, "fd0ab81aabf88e31"),
    (0x105397b0, 0x10539874, "bbd0f0e9fbbfa08c"),
    (0x10539880, 0x10539bac, "5f773f214e393536"),
    (0x10539bb0, 0x10539d1c, "6a8a9c8af37e043f"),
    (0x10539d20, 0x10539f30, "d3de1d3fd3d83363"),
    (0x10539f30, 0x1053a200, "f00310470205caf3"),
    (0x1053a200, 0x1053a900, "3e10ba9eda722f8a"),
    (0x1053a900, 0x1053aac8, "304f53c2a3fe8b8c"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
VECTORS = {0x108bae1c: 0x10539bb0, 0x108bae24: 0x10539880,
           0x108bae2c: 0x105397b0, 0x108bae34: 0x10537120, 0x108bae3c: 0x10537080}
OWNED_VTABLE = {8: 0x108bae3c, 0x10: 0x108bae24, 0x44: 0x108bae34,
                0x94: 0x108bae2c, 0xb8: 0x108bae1c}
DIRECT_ROOTS = frozenset({0x10536d50, 0x10539590, 0x10539d20, 0x10539f30, 0x1053a200, 0x1053a900})
SOURCE_REFERENCES = (0x10536d68, 0x10537134, 0x105395d4, 0x105398a0,
                     0x10539d48, 0x10539f6c, 0x1053a27c, 0x1053a928)
OPCODE_SPANS = {}


def production_entries(source):
    entries = tuple(int(value, 16) for value in re.findall(r"^GER_FIRST_ROUTINE\((0x[0-9a-f]+),", source, re.M))
    if entries != tuple(sorted(set(entries))) or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid German First Division production registry")
    return entries


GER_FIRST_ENTRIES = production_entries((ROOT / "native/ger_first_routines.inc").read_text())


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
            raise ValueError(f"Cross-entry GER_FIRST branch {source:#x} -> {target:#x}")
    if calls != DIRECT_ROOTS or calls | set(VECTORS.values()) != set(ENTRIES):
        raise ValueError("GER_FIRST call and transition-vector roots changed")
    reached = set()
    for entry in ENTRIES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if address in reached:
                continue
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid GER_FIRST flow {entry:#x} -> {address:#x}")
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified GER_FIRST computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned GER_FIRST instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for GER_FIRST audit")
    segments = [(segment["address"], (export / segment["file"]).read_bytes())
                for segment in program["segments"] if segment.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address - base:address - base + size]
        raise ValueError(f"Missing GER_FIRST bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in ((0x4500, 0x1083eaa0, b"FRA_THIRD.CPP\0"),
                              (0x44f8, 0x1083eac4, b"GER_FIRST.CPP\0"),
                              (0x44f4, 0x1083eb08, b"GER_SECOND.CPP\0")):
        if word(TOC - slot) != base or read(base, len(label)) != label:
            raise ValueError("Changed GER_FIRST source-pool boundary")
    if word(TOC - 0x6000) != 0x108dad74:
        raise ValueError("Changed GER_FIRST vtable slot")
    for offset, vector in OWNED_VTABLE.items():
        if word(0x108dad74 + offset) != vector or word(vector) != VECTORS[vector] or word(vector + 4) != TOC:
            raise ValueError("Changed GER_FIRST owned virtual method")
    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x53", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS | OPCODE_SPANS or any(target % 4 == 0 for target in OPCODE_SPANS.values()):
        raise ValueError("Unclassified GER_FIRST original pointer")
    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        for entry, end, expected in ROUTINES:
            if digest(routine_records(db, entry, end)) != expected:
                raise ValueError(f"Changed GER_FIRST body at {entry:#x}")
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x44f8(r2)%",)))
        if refs != SOURCE_REFERENCES:
            raise ValueError("Changed GER_FIRST source references")
        for address in (0x10536d44, 0x1053aac4):
            if routine_records(db, address, address + 4)[0]["assembly"] != "blr":
                raise ValueError("Changed GER_FIRST neighboring return")
        validate_flows(records, db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters GER_FIRST")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted GER_FIRST word at {address:#x}")
    return len(records)


def ger_first_hook(entry):
    return f"if (try_modern_ger_first(s, 0x{entry:x})) return true;"


def require_ger_first_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in GER_FIRST_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", ger_first_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing GER_FIRST hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"GER_FIRST.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

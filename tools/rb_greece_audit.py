"""Pin the Greek disciplinary rulebook's source and executable ownership."""
import argparse
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
MODULE = (0x106ab7d0, 0x106abe20)
ROUTINES = (
    (0x106ab7d0, 0x106ab884, "1df288c077f54718"),
    (0x106ab890, 0x106ab8e8, "63d981062d647e25"),
    (0x106ab8f0, 0x106ab970, "742f2ce2ff8e976a"),
    (0x106ab970, 0x106abe1c, "5466323397cde282"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
VECTORS = {0x108bc4fc: 0x106ab890, 0x108bc504: 0x106ab7d0, 0x108bc524: 0x106ab970}
VTABLES = {
    -0x5de0: (0x108decc4, {0x18: 0x108bc504, 0x1c: 0x108bc4fc}),
    -0x5ddc: (0x108decec, {0x10: 0x108bc524}),
}
DIRECT_CALLS = {0x100e80bc: 0x106ab8f0}
PADDING = {0x106ab884: 0, 0x106ab888: 0, 0x106ab88c: 0, 0x106ab8e8: 0,
           0x106ab8ec: 0, 0x106abb6c: 0x60000000, 0x106abe1c: 0}
RB_GREECE_ENTRIES = ENTRIES[:3]


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for rb_greece audit")
    segments = [(segment["address"], (export / segment["file"]).read_bytes())
                for segment in program["segments"] if segment.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address <= base + len(data) - size:
                return data[address - base:address - base + size]
        raise ValueError(f"Missing original bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    source_pools = ((-0x41d8, 0x1088a2d0, b"rb_finland_league.cpp\0"),
                    (-0x41d4, 0x1088a308, b"rb_greece.cpp\0"),
                    (-0x41d0, 0x1088a32c, b"rb_ireland.cpp\0"))
    for slot, address, source in source_pools:
        if word(TOC + slot) != address or read(address, len(source)) != source:
            raise ValueError("Changed Greek rulebook source boundary")
    for slot, (address, slots) in VTABLES.items():
        if word(TOC + slot) != address:
            raise ValueError("Changed Greek rulebook table")
        for offset, vector in slots.items():
            if word(address + offset) != vector or word(vector) != VECTORS[vector] or word(vector + 4) != TOC:
                raise ValueError("Changed Greek rulebook transition vector")

    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x6a", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS:
        raise ValueError("Unclassified Greek rulebook code pointer")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {r["address"]: r for r in routine_records(db, *MODULE)}
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x41d4(r2)%",)))
        if refs != (0x106ab7dc,):
            raise ValueError("Changed Greek rulebook source reference")
        neighbors = [(0x106ab7c8, "blr"), (0x106abe20, "mfspr r0,LR")]
        for address, assembly in neighbors:
            if routine_records(db, address, address + 4)[0]["assembly"] != assembly:
                raise ValueError("Changed Greek rulebook neighboring function")
        incoming = db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<? "
            "AND (i.address<? OR i.address>=?)", (*MODULE, *MODULE))
        if set(incoming) != {(source, target, "UNCONDITIONAL_CALL") for source, target in DIRECT_CALLS.items()}:
            raise ValueError("Changed Greek rulebook external branch or call")
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters the Greek rulebook")
        validate_routines(records)
    if set(range(*MODULE, 4)) - records.keys() != PADDING.keys():
        raise ValueError("Unowned words in the Greek rulebook")
    if any(word(address) != expected for address, expected in PADDING.items()):
        raise ValueError("Changed Greek rulebook alignment padding")
    return len(records)


def validate_routines(records):
    if set(ENTRIES) != set(VECTORS.values()) | set(DIRECT_CALLS.values()):
        raise ValueError("Greek rulebook roots do not cover the inventory")
    all_reached = set()
    for entry, end, expected in ROUTINES:
        body = [r for address, r in records.items() if entry <= address < end]
        if digest(body) != expected:
            raise ValueError(f"Changed Greek rulebook body at {entry:#x}")
        pending, reached = [entry], set()
        while pending:
            address = pending.pop()
            if address in reached:
                continue
            if not entry <= address < end or address not in records:
                raise ValueError(f"Invalid Greek rulebook flow {entry:#x} -> {address:#x}")
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified Greek rulebook computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
        all_reached.update(reached)
    if all_reached != records.keys():
        raise ValueError("Unreachable instructions in the Greek rulebook")


def rb_greece_hook(entry):
    return f"if (try_modern_rb_greece(s, 0x{entry:x})) return true;"


def require_rb_greece_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in RB_GREECE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", rb_greece_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing rb_greece hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"rb_greece.cpp: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

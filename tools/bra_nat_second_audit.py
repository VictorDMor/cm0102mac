"""BRA_NAT_SECOND.CPP source pool, reachable entries and transition-vector ownership."""
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
MODULE = (0x105177b0, 0x105195f0)
ROUTINES = (
    (0x105177b0, 0x10517ae8, "7a8b03c680c07942"),
    (0x10517af0, 0x10517bf4, "10f329c2dde614f5"),
    (0x10517c00, 0x10518b10, "0b53adae38d13b48"),
    (0x10518b10, 0x10518b44, "8255bcd1acebbd32"),
    (0x10518b50, 0x10518c94, "a76811fecb11a054"),
    (0x10518ca0, 0x10518d70, "85877fbebf2baa74"),
    (0x10518d70, 0x10519008, "43ff57f4dc823f28"),
    (0x10519010, 0x105190cc, "aba93d148cbccd1d"),
    (0x105190d0, 0x105192b8, "a80dd3d7a6d40643"),
    (0x105192c0, 0x1051938c, "263fced02ac2b6a6"),
    (0x10519390, 0x10519540, "d03f7e89aa1d1545"),
    (0x10519540, 0x105195cc, "fcdaad1ec8e6ccbf"),
    (0x105195d0, 0x105195f0, "020c29bdc129bfd5"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
VECTORS = {
    0x108baa64: 0x105195d0, 0x108baa6c: 0x10519540,
    0x108baa74: 0x10519390, 0x108baa7c: 0x105192c0,
    0x108baa84: 0x10519010, 0x108baa8c: 0x10518d70,
    0x108baa94: 0x10518ca0, 0x108baa9c: 0x10518b10,
    0x108baaa4: 0x10517c00, 0x108baaac: 0x10517af0,
}
OWNED_VTABLE = {
    0x8: 0x108baaac, 0x10: 0x108baa8c, 0x30: 0x108baa84,
    0x38: 0x108baa9c, 0x44: 0x108baaa4, 0x4c: 0x108baa7c,
    0x50: 0x108baa74, 0x70: 0x108baa6c, 0x94: 0x108baa94,
    0x98: 0x108baa64,
}
DIRECT_CALLS = {
    0x10350844: 0x105177b0, 0x10517900: 0x10518b50,
    0x10518e9c: 0x10518b50, 0x105190b0: 0x105190d0,
}
SOURCE_REFERENCES = (0x105177c8, 0x10517c14, 0x10518b6c, 0x10518d88,
                     0x10519018, 0x105190e8, 0x105193a4)
# These unaligned code bytes straddle PowerPC words and cannot be CFM vectors.
OPCODE_SPANS = {
    0x101e4632: 0x10517c63, 0x103477c2: 0x10518082,
    0x103524d2: 0x10517c7c, 0x10436aa2: 0x1051807f,
    0x10528842: 0x10517f83, 0x10548842: 0x10517fc3,
    0x106c77c2: 0x10517c65,
}


def production_entries(source):
    entries = tuple(int(value, 16) for value in re.findall(r"^BRA_NAT_SECOND_ROUTINE\((0x[0-9a-f]+),", source, re.M))
    if entries != tuple(sorted(set(entries))) or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid Brazilian Second Division production registry")
    return entries


BRA_NAT_SECOND_ENTRIES = production_entries((ROOT / "native/bra_nat_second_routines.inc").read_text())


def owner(address):
    index = bisect_right(ENTRIES, address) - 1
    if index >= 0 and address < ROUTINES[index][1]:
        return ENTRIES[index]
    return None


def validate_flows(records, incoming):
    calls = {}
    for source, target, flow in incoming:
        if "CALL" in flow:
            calls[source] = target
        elif owner(source) is None or owner(source) != owner(target):
            raise ValueError(f"Cross-entry BRA_NAT_SECOND branch {source:#x} -> {target:#x}")
    if calls != DIRECT_CALLS or set(calls.values()) | set(VECTORS.values()) != set(ENTRIES):
        raise ValueError("BRA_NAT_SECOND call and transition-vector roots changed")
    reached = set()
    for entry in ENTRIES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if address in reached:
                continue
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid BRA_NAT_SECOND flow {entry:#x} -> {address:#x}")
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified BRA_NAT_SECOND computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned BRA_NAT_SECOND instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for BRA_NAT_SECOND audit")
    segments = [(segment["address"], (export / segment["file"]).read_bytes())
                for segment in program["segments"] if segment.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address - base:address - base + size]
        raise ValueError(f"Missing BRA_NAT_SECOND bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in ((0x4554, 0x1083e348, b"BRA_NAT_FIRST.CPP\0"),
                              (0x4550, 0x1083e464, b"BRA_NAT_SECOND.CPP\0"),
                              (0x454c, 0x1083e4f0, b"BRA_REG_NORTH.CPP\0")):
        if word(TOC - slot) != base or read(base, len(label)) != label:
            raise ValueError("Changed BRA_NAT_SECOND source-pool boundary")
    if word(TOC - 0x6044) != 0x108da0b4:
        raise ValueError("Changed BRA_NAT_SECOND vtable slot")
    for offset, vector in OWNED_VTABLE.items():
        if word(0x108da0b4 + offset) != vector or word(vector) != VECTORS[vector] or word(vector + 4) != TOC:
            raise ValueError("Changed BRA_NAT_SECOND owned virtual method")
    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x51", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS | OPCODE_SPANS or any(address % 4 != 2 for address in OPCODE_SPANS):
        raise ValueError("Unclassified BRA_NAT_SECOND original pointer")
    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        for entry, end, expected in ROUTINES:
            if digest(routine_records(db, entry, end)) != expected:
                raise ValueError(f"Changed BRA_NAT_SECOND body at {entry:#x}")
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x4550(r2)%",)))
        if refs != SOURCE_REFERENCES:
            raise ValueError("Changed BRA_NAT_SECOND source references")
        for address in (0x105177ac, 0x105195ec):
            if routine_records(db, address, address + 4)[0]["assembly"] != "blr":
                raise ValueError("Changed BRA_NAT_SECOND neighboring return")
        validate_flows(records, db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters BRA_NAT_SECOND")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted BRA_NAT_SECOND word at {address:#x}")
    return len(records)


def bra_nat_second_hook(entry):
    return f"if (try_modern_bra_nat_second(s, 0x{entry:x})) return true;"


def require_bra_nat_second_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in BRA_NAT_SECOND_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", bra_nat_second_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing BRA_NAT_SECOND hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"BRA_NAT_SECOND.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

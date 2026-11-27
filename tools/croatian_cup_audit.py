"""Check the six CRO_CUP.CPP entry points, source boundaries and indirect callers."""
import argparse
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

EXPORT = Path(__file__).resolve().parent.parent / ".local/current/export"
MODULE = (0x106bf0f0, 0x106bfe90)
ROUTINES = (
    (0x106bf0f0, 0x106bf3c4, "ea269d622654ebdc"),
    (0x106bf3d0, 0x106bf4bc, "f5201efb20f76f12"),
    (0x106bf4c0, 0x106bf9a8, "c23d7c9b294cd197"),
    (0x106bf9b0, 0x106bfbd4, "9ef0e7a5a9f5e80b"),
    (0x106bfbe0, 0x106bfe30, "cdd727f4e49b770b"),
    (0x106bfe30, 0x106bfe8c, "b5c3e4d3d9a9449a"),
)
CROATIAN_CUP_ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
DIRECT_CALLS = {(0x10350ddc, 0x106bf0f0), (0x106bf1c8, 0x106bf9b0), (0x106bfcc8, 0x106bf9b0)}
# Vtable slot, relocated transition vector, routine entry.
SLOTS = ((0x08, 0x108bc95c, 0x106bf3d0), (0x10, 0x108bc94c, 0x106bfbe0),
         (0x44, 0x108bc954, 0x106bf4c0), (0x94, 0x108bc944, 0x106bfe30))
POOLS = ((-0x4124, 0x10895630, b"wales_rules.cpp\0"), (-0x4120, 0x10895654, b"cro_cup.cpp\0"),
         (-0x411c, 0x10895674, b"fin_cup.cpp\0"))


def require_croatian_cup_index(db):
    """Cheap generator check over the six definitions, including their p-code."""
    for entry, end, expected in ROUTINES:
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"CRO_CUP.CPP routine {entry:#x} changed: {actual} (expected {expected})")


def validate_image(segments):
    def read(address, size):
        for base, data in segments:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        raise ValueError(f"Missing original bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, pointer, label in POOLS:
        if word(TOC + slot) != pointer or read(pointer, len(label)) != label:
            raise ValueError("CRO_CUP.CPP or neighboring source pool changed")

    table = word(TOC - 0x5d3c)
    if table != 0x108df314 or word(TOC - 0x5d38) != table + 0xa8:
        raise ValueError("CRO_CUP.CPP vtable boundaries changed")
    vectors = {vector: entry for _, vector, entry in SLOTS}
    owned_slots = {(slot, word(table + slot)) for slot in range(8, 0xa8, 4) if word(table + slot) in vectors}
    if owned_slots != {(slot, vector) for slot, vector, _ in SLOTS}:
        raise ValueError("CRO_CUP.CPP vtable slots changed")
    if any(word(vector + 4) != TOC for vector in vectors):
        raise ValueError("CRO_CUP.CPP transition vector TOC changed")

    # Include unaligned data pointers. An extra vector or a dispatch table
    # targeting the middle of a function would invalidate entry-only routing.
    pointers = {}
    for base, data in segments:
        for match in re.finditer(b"\x10\x6b", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset+4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != vectors:
        raise ValueError("CRO_CUP.CPP contains an unclassified entry pointer or a changed transition vector")

    for (_, end, _), following in zip(ROUTINES, (*CROATIAN_CUP_ENTRIES[1:], MODULE[1])):
        if any(word(address) not in (0, 0x60000000) for address in range(end, following, 4)):
            raise ValueError(f"Unexpected CRO_CUP.CPP alignment word at the end of {end:#x}")


def validate_edges(records, incoming):
    calls = set()
    for source, target, flow in incoming:
        if flow == "UNCONDITIONAL_CALL":
            calls.add((source, target))
        elif "CALL" in flow:
            raise ValueError(f"Unexpected CRO_CUP.CPP call at {source:#x}")
        elif source not in records:
            raise ValueError(f"External CRO_CUP.CPP interior branch {source:#x} -> {target:#x}")
    if calls != DIRECT_CALLS:
        raise ValueError("CRO_CUP.CPP direct callers changed")
    reached = set()
    for entry, end, _ in ROUTINES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if address not in records or not entry <= address < end:
                raise ValueError(f"Control flow escapes CRO_CUP.CPP routine {entry:#x} to {address:#x}")
            if address in reached:
                continue
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                raise ValueError(f"Unclassified computed flow at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned instructions inside CRO_CUP.CPP")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for CRO_CUP.CPP audit")
    validate_image([(s["address"], (export / s["file"]).read_bytes()) for s in program["segments"] if s.get("file")])
    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        require_croatian_cup_index(db)
        records = {r["address"]: r for r in routine_records(db, *MODULE)}
        edges = db.execute("SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
                           "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE)
        validate_edges(records, edges)
        if db.execute("SELECT 1 FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough into CRO_CUP.CPP")
        neighbors = [db.execute("SELECT json_extract(body,'$.assembly') FROM instructions WHERE address=?",
                                (addr,)).fetchone() for addr in (0x106bf0e0, MODULE[1], MODULE[1] + 0x18)]
        if neighbors != [("blr",), ("mfspr r0,LR",), ("lwz r31,-0x411c(r2)",)]:
            raise ValueError("CRO_CUP.CPP neighboring functions changed")
    return len(records)


def validate_production_page(code):
    labels = {int(value, 16) for value in re.findall(r"^i_([0-9a-f]+):", code, re.MULTILINE)}
    owned = {address for address in labels if MODULE[0] <= address < MODULE[1]}
    if owned != set(CROATIAN_CUP_ENTRIES):
        raise ValueError("production must contain exactly the six Croatian Cup ABI entries")
    for address in owned:
        body = code.split(f"i_{address:x}: //", 1)[1].split("\ni_", 1)[0]
        lines = [line.strip() for line in body.splitlines()[1:]]
        if lines != [f"s.step(0x{address:x});", f"return croatia_cup::dispatch(s, 0x{address:x});"]:
            raise ValueError(f"unexpected original instructions at Croatian Cup entry {address:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--production-page", type=Path)
    args = parser.parse_args()
    if args.production_page:
        validate_production_page(args.production_page.read_text())
    print(f"CRO_CUP.CPP: {len(ROUTINES)} routines, {validate_export(args.export)} reachable instructions")

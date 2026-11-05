"""Pin the Hall of Fame module's original entries, callbacks and jump table."""
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
MODULE = (0x10140730, 0x10143280)
ROUTINES = (
    (0x10140730, 0x1014079c, "06a370ca3e154870"),
    (0x101407a0, 0x101407d4, "12a2ceaa537a008c"),
    (0x101407e0, 0x101409b4, "80f7d057549850f4"),
    (0x101409c0, 0x10140a88, "3c25ee426c5cad97"),
    (0x10140a90, 0x10140b80, "5e47473e6ba3be8f"),
    (0x10140b80, 0x10140f54, "26383345487c68e0"),
    (0x10140f60, 0x10141190, "cbdda73423221865"),
    (0x10141190, 0x10141500, "be5419d6c98c8d31"),
    (0x10141500, 0x10141990, "ba47ce158723a1cd"),
    (0x10141990, 0x10141ca0, "29a4b49bba638355"),
    (0x10141ca0, 0x10141e78, "2a8fdced9ffa7058"),
    (0x10141e80, 0x101420f8, "e58a544e74a64a3b"),
    (0x10142100, 0x10142174, "00c25226a0f3bc74"),
    (0x10142180, 0x101426b8, "3ad2bf39444989c0"),
    (0x101426c0, 0x10142a0c, "b62cce7b00ef8e1a"),
    (0x10142a10, 0x10143048, "ac13a9234215405c"),
    (0x10143050, 0x1014309c, "224827ef4c7a77ea"),
    (0x101430a0, 0x10143174, "48954a1400177ede"),
    (0x10143180, 0x10143234, "95afcb17d13c9756"),
    (0x10143240, 0x10143274, "b8b2771e3536ea80"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
HALL_OF_FAME_ENTRIES = (0x101407a0, 0x10142100)
VECTORS = {0x108b8ac4: 0x10143180, 0x108b8acc: 0x10143050,
           0x108b8ad4: 0x10142100, 0x108b8adc: 0x101407a0}
DIRECT_CALLS = {
    0x1000001c: 0x10143240, 0x10118f68: 0x10140730, 0x10140770: 0x10141190,
    0x10140780: 0x10141990, 0x10140a6c: 0x10140b80, 0x10140b54: 0x10141e80,
    0x10140b68: 0x10140b80, 0x10141460: 0x101426c0, 0x10141890: 0x101426c0,
    0x10141944: 0x10140f60, 0x10141a9c: 0x10142180, 0x10141c14: 0x101426c0,
    0x10141d70: 0x10142a10, 0x10143074: 0x10141500, 0x101431f4: 0x10142a10,
    0x10143208: 0x10142a10, 0x1033fdb4: 0x10142a10, 0x1033fe70: 0x101430a0,
    0x1033fe88: 0x10142a10, 0x1034007c: 0x10142a10, 0x10340280: 0x10142a10,
    0x10340638: 0x101407e0, 0x1034094c: 0x10141ca0, 0x103409f0: 0x10141500,
    0x10340a58: 0x10141990, 0x1034dda0: 0x10141990, 0x103862bc: 0x10140a90,
    0x10386314: 0x10140a90, 0x1038696c: 0x101409c0,
}
SOURCE_REFS = (0x101407e8, 0x101409cc, 0x10140aa0, 0x10140b98, 0x10140f70, 0x10141198,
               0x10141520, 0x10141998, 0x10141cb0, 0x1014219c, 0x101426dc, 0x10142a18)
SWITCH = 0x10142ac8
TABLE = 0x108ca15c
LABELS = (0x10142acc, 0x10142af8, *range(0x10142b28, 0x10142fa9, 0x30))
# These values are unaligned sequences in code or packed data, so they
# cannot be aligned CFM transition vectors or table words.
UNALIGNED = {
    0x101bc2f2: 0x10142c03, 0x10259956: 0x10142816, 0x1040070a: 0x10142c12,
    0x108ace1e: 0x10142800, 0x108bfe9f: 0x10141001, 0x108c1046: 0x10141094,
    0x108c266a: 0x10141014,
}


def validate_flows(records, incoming):
    if {source: target for source, target, flow in incoming if "CALL" in flow} != DIRECT_CALLS:
        raise ValueError("Changed Hall of Fame call sites")
    if set(ENTRIES) != set(VECTORS.values()) | set(DIRECT_CALLS.values()):
        raise ValueError("Hall of Fame roots do not cover the inventory")

    def owner(address):
        return next((start for start, end, _ in ROUTINES if start <= address < end), None)

    if any(owner(source) is None or owner(source) != owner(target)
           for source, target, flow in incoming if "CALL" not in flow):
        raise ValueError("Unexpected Hall of Fame cross-entry flow")
    reached = set()
    for entry, end, expected in ROUTINES:
        body = [record for address, record in records.items() if entry <= address < end]
        if digest(body) != expected:
            raise ValueError(f"Changed Hall of Fame body at {entry:#x}")
        pending = [entry]
        while pending:
            address = pending.pop()
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid Hall of Fame flow {entry:#x} -> {address:#x}")
            if address in reached:
                continue
            reached.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED"):
                if address != SWITCH or record["flow"] != "COMPUTED_JUMP":
                    raise ValueError(f"Unclassified Hall of Fame indirect flow at {address:#x}")
                pending.extend(LABELS)
            elif "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unreachable exported Hall of Fame instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for Hall of Fame audit")
    segments = [(seg["address"], (export / seg["file"]).read_bytes())
                for seg in program["segments"] if seg.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address <= base + len(data) - size:
                return data[address - base:address - base + size]
        raise ValueError(f"Missing original bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    if word(TOC - 0x54cc) != 0x107a79c4 or read(0x107a79c4, 17) != b"HALL_OF_FAME.CPP\0":
        raise ValueError("Changed Hall of Fame source pool")
    if word(TOC - 0x54d0) != 0x108b8ad4 or word(TOC - 0x54dc) != TABLE:
        raise ValueError("Changed Hall of Fame TOC")
    table_words = {TABLE + 4 * index: target for index, target in enumerate(LABELS)}
    if any(word(slot) != target for slot, target in (VECTORS | table_words).items()):
        raise ValueError("Changed Hall of Fame transition vectors or jump table")
    if any(word(vector + 4) != TOC for vector in VECTORS):
        raise ValueError("Changed Hall of Fame vector TOC")

    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10\x14", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                pointers[base + offset] = target
    if pointers != VECTORS | table_words | UNALIGNED or any(address % 4 == 0 for address in UNALIGNED):
        raise ValueError("Unclassified Hall of Fame pointer")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        refs = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x54cc(r2)%",)))
        if refs != SOURCE_REFS:
            raise ValueError("Changed Hall of Fame source references")
        neighbors = ((0x10140720, "blr"), (0x10143270, "blr"), (0x10143280, "mfspr r0,LR"))
        for address, assembly in neighbors:
            if routine_records(db, address, address + 4)[0]["assembly"] != assembly:
                raise ValueError("Changed Hall of Fame neighbor")
        incoming = list(db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        validate_flows(records, incoming)
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters Hall of Fame")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unowned original Hall of Fame word at {address:#x}")
    return len(records)


def hall_of_fame_hook(entry):
    return f"if (try_modern_hall_of_fame(s, 0x{entry:x})) return true;"


def require_hall_of_fame_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in HALL_OF_FAME_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", hall_of_fame_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing Hall of Fame hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"HALL_OF_FAME.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

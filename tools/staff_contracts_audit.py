"""Original STAFF_CONTRACTS.CPP bounds and entry ownership.

The contract pool at TOC -0x4da4 follows the SIMULATED_STATS.CPP pool at
-0x4dcc and precedes STAFF_RECORDS.CPP at -0x4d94. The first contract
constructor follows the final simulated-stats return at 0x103721e8.
The date initializer immediately before the first staff-records routine
belongs to this unit. The five tables contain case labels, not entry points.
"""
import argparse
from bisect import bisect_right
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import struct

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
MODULE = (0x103721f0, 0x10381110)
ROUTINES = (
    (0x103721f0, 0x10372254, "a4d0b02bdb71137f"),
    (0x10372260, 0x103723b0, "69a91f0ee77118a3"),
    (0x103723b0, 0x1037258c, "58a741628d9ddd3f"),
    (0x10372590, 0x10372724, "988a7067cbe1eb42"),
    (0x10372730, 0x103727bc, "e3f9ed6c2dfc2a49"),
    (0x103727c0, 0x10374570, "3bef1934efc680c9"),
    (0x10374570, 0x10374ae8, "f5f98282b7ff9334"),
    (0x10374af0, 0x10374b34, "901cb36ede4092f0"),
    (0x10374b40, 0x10374b54, "59d7859af890c532"),
    (0x10374b60, 0x10374b74, "919b4a3b0e9ec83c"),
    (0x10374b80, 0x10374b98, "d32f8bbf2ba85cb6"),
    (0x10374ba0, 0x10374bd4, "e3af378c22fb23f5"),
    (0x10374be0, 0x10374ca4, "795521036d9565d6"),
    (0x10374cb0, 0x10374cd8, "b55a3b1cf4a90acb"),
    (0x10374ce0, 0x10374d04, "2b989c7b577bcd8e"),
    (0x10374d10, 0x10374d44, "a121fc73a8538045"),
    (0x10374d50, 0x10374d74, "661d303748efafb2"),
    (0x10374d80, 0x10374f44, "960fbc40db42216c"),
    (0x10374f50, 0x10378d28, "4f7dcd535c4449d3"),
    (0x10378d30, 0x10378d94, "03248ff3960216a6"),
    (0x10378da0, 0x1037b6e4, "161dc3d0ff80b94c"),
    (0x1037b6f0, 0x1037e3d8, "a44be63f1955de22"),
    (0x1037e3e0, 0x1037e658, "51f4a6f057d8cfcd"),
    (0x1037e660, 0x1037f050, "bab297ea7b8ee0b6"),
    (0x1037f050, 0x1037f110, "cfa7c50877691ea7"),
    (0x1037f110, 0x1037f588, "cb4d1dfab3d185d3"),
    (0x1037f590, 0x1037f7c4, "9e43db9db2058e74"),
    (0x1037f7d0, 0x1037f830, "d872210fe9240bd0"),
    (0x1037f830, 0x1037f898, "50c4d421a48c1114"),
    (0x1037f8a0, 0x1037fa48, "212ba1faf2467105"),
    (0x1037fa50, 0x1037fa60, "63fca03b06f08576"),
    (0x1037fa60, 0x1037fa6c, "a6d8d8e9b201955d"),
    (0x1037fa70, 0x1037fa78, "5a922e8ed4005a33"),
    (0x1037fa80, 0x10380264, "37c4ce006d609ad4"),
    (0x10380270, 0x103802f4, "331d6f597d4c1f62"),
    (0x10380300, 0x103803ac, "14ae1c2fe87f50b5"),
    (0x103803b0, 0x10380400, "ddca3843ead89d0a"),
    (0x10380400, 0x10380520, "4d5dcc7d41d07c10"),
    (0x10380520, 0x10380554, "ee8c87976acef7f5"),
    (0x10380560, 0x10380630, "cf8a01eb9fb68d71"),
    (0x10380630, 0x10380700, "39f9c5ddf9db2f0f"),
    (0x10380700, 0x10380708, "4862f7f921302204"),
    (0x10380710, 0x10380c04, "845eb121dd60a013"),
    (0x10380c10, 0x10380d4c, "3998a0779bb3257a"),
    (0x10380d50, 0x10380f1c, "e87d45ca541bab74"),
    (0x10380f20, 0x103810d0, "59d46a9def760da6"),
    (0x103810d0, 0x10381104, "6675092517b10489"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)


def production_entries():
    registry = ROOT / "native/staff_contracts_routines.inc"
    entries = tuple(int(entry, 16) for entry in re.findall(
        r"^STAFF_CONTRACTS_(?:ROUTINE|CONTINUATION)\((0x[0-9a-f]+),", registry.read_text(), re.MULTILINE))
    if tuple(sorted(set(entries))) != entries or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid STAFF_CONTRACTS.CPP production registry")
    return entries


STAFF_CONTRACTS_ENTRIES = production_entries()
SWITCHES = {
    0x10374df4: (-0x4db0, 0x108cf098, 12),
    0x103782bc: (-0x4db4, 0x108cf0c8, 8),
    0x10379020: (-0x4db8, 0x108cf0e8, 11),
    0x1037b970: (-0x4dbc, 0x108cf114, 11),
    0x10380cb0: (-0x4dc0, 0x108cf140, 8),
}
# These skipped branches sit behind other unconditional branches; none is a
# direct flow target or a transition-vector or switch target.
SKIPPED_BRANCHES = {
    0x1037569c: 0x480000b8,
    0x10379f3c: 0x4800001c,
    0x10379f40: 0x48000020,
    0x1037cafc: 0x4800001c,
    0x1037cb00: 0x48000020,
}


def owner(address):
    index = bisect_right(ENTRIES, address) - 1
    if index >= 0:
        start, end, _ = ROUTINES[index]
        if address < end:
            return start
    return None


def validate_ownership(records, edges, switches):
    calls = set()
    for source, target, flow in edges:
        if "CALL" in flow:
            calls.add(target)
        elif owner(source) is None or owner(source) != owner(target):
            raise ValueError(f"Cross-entry branch 0x{source:x} -> 0x{target:x}")
    if calls != set(ENTRIES):
        raise ValueError("Direct call roots do not match the staff-contract inventory")

    reached = set()
    for root in ENTRIES:
        pending = [root]
        while pending:
            address = pending.pop()
            if owner(address) != root or address not in records:
                raise ValueError(f"Invalid flow from 0x{root:x} to 0x{address:x}")
            if address in reached:
                continue
            reached.add(address)
            item = records[address]
            if item["flow"] == "COMPUTED_JUMP":
                if address not in switches:
                    raise ValueError(f"Unknown computed jump at 0x{address:x}")
                pending.extend(switches[address])
            elif "CALL" not in item["flow"]:
                pending.extend(item["flows"])
            if item.get("fallthrough") is not None:
                pending.append(item["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned staff-contract export records")
    if {a for a, r in records.items() if r["flow"] == "COMPUTED_JUMP"} != switches.keys():
        raise ValueError("Changed staff-contract switch sites")
    if any(r["flow"] == "COMPUTED_CALL" for r in records.values()):
        raise ValueError("Unclassified indirect staff-contract call")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for the STAFF_CONTRACTS.CPP audit")
    segments = [(s["address"], (export / s["file"]).read_bytes())
                for s in program["segments"] if s.get("file")]

    def read(address, width):
        for start, data in segments:
            if start <= address and address + width <= start + len(data):
                return data[address - start:address - start + width]
        raise ValueError(f"Missing original bytes at 0x{address:x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in (
        (-0x4dcc, 0x1080bd64, b"SIMULATED_STATS.CPP\0"),
        (-0x4da4, 0x1080bde8, b"STAFF_CONTRACTS.CPP\0"),
        (-0x4d94, 0x1080c7bc, b"STAFF_RECORDS.CPP\0"),
    ):
        if word(TOC + slot) != base or label not in read(base, 120):
            raise ValueError(f"Changed staff-contract source pool at TOC {slot:#x}")

    if hashlib.sha256(read(0x108cf098, 0xc8)).hexdigest() != "23ee8c7fc64acd79454821f1fe276d66490226d4b790b569b45931262db98366":
        raise ValueError("Changed staff-contract switch targets")
    switches = {}
    table_words = {}
    for pc, (slot, base, count) in SWITCHES.items():
        if word(TOC + slot) != base:
            raise ValueError(f"Changed staff-contract switch pointer at 0x{pc:x}")
        switches[pc] = struct.unpack(f">{count}I", read(base, count * 4))
        table_words.update((base + index * 4, target) for index, target in enumerate(switches[pc]))

    # Scan all initialized sections, including code, for aligned words that
    # could enter the module. Only the five jump tables contain such words.
    pointers = {}
    for base, data in segments:
        for match in re.finditer(rb"\x10[\x37\x38]", data):
            offset = match.start()
            address = base + offset
            if address % 4 == 0 and offset + 4 <= len(data):
                target = struct.unpack_from(">I", data, offset)[0]
                if MODULE[0] <= target < MODULE[1]:
                    pointers[address] = target
    if pointers != table_words:
        raise ValueError("Unclassified aligned pointer to STAFF_CONTRACTS.CPP")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro",
                                 uri=True)) as db:
        records = {r["address"]: r for r in routine_records(db, *MODULE)}
        for entry, end, fingerprint in ROUTINES:
            body = [records[a] for a in range(entry, end, 4) if a in records]
            if digest(body) != fingerprint:
                raise ValueError(f"Changed staff-contract body at 0x{entry:x}")

        refs = [address for (address,) in db.execute(
            "SELECT address FROM instructions WHERE body LIKE '%-0x4da4(r2)%' ORDER BY address")]
        if (len(refs) != 16 or refs[0] != 0x103723d0 or refs[-1] != 0x10380d5c
                or any(owner(address) is None for address in refs)):
            raise ValueError("Changed references to the STAFF_CONTRACTS.CPP source pool")
        for address, instruction in ((0x10372048, "lwz r31,-0x4dcc(r2)"),
                                     (0x103721e8, "blr"),
                                     (0x10381140, "lwz r28,-0x4d94(r2)")):
            body = routine_records(db, address, address + 4)
            if not body or body[0]["assembly"] != instruction:
                raise ValueError(f"Changed neighboring staff-contract boundary at 0x{address:x}")

        edges = db.execute(
            "SELECT i.address,e.value,json_extract(i.body,'$.flow') "
            "FROM instructions i,json_each(i.body,'$.flows') e WHERE e.value>=? AND e.value<?",
            MODULE)
        validate_ownership(records, edges, switches)
        external_fallthrough = db.execute(
            "SELECT address FROM instructions WHERE (address<? OR address>=?) "
            "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
            (*MODULE, *MODULE)).fetchone()
        if external_fallthrough:
            raise ValueError("External fallthrough enters STAFF_CONTRACTS.CPP")

    for address in range(*MODULE, 4):
        if address not in records and word(address) != SKIPPED_BRANCHES.get(address, 0):
            raise ValueError(f"Unaccounted original instruction at 0x{address:x}")
    return len(records)


def staff_contracts_hook(entry):
    return f"if (try_modern_staff_contracts(s, 0x{entry:x})) return true;"


def require_staff_contracts_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in STAFF_CONTRACTS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f"i_{entry:x}:"
        hook = staff_contracts_hook(entry)
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        step = f"s.step(0x{entry:x});"
        if (label not in code or code.count(hook) != 1 or hook not in fragment
                or step not in fragment or fragment.index(hook) > fragment.index(step)):
            raise ValueError(f"Missing or misplaced staff-contract hook at 0x{entry:x}")
        if fragment.count("try_modern_") != 1:
            raise ValueError(f"Competing staff-contract hook at 0x{entry:x}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true", help="print entry addresses for replay census")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"0x{entry:x}")
    else:
        count = validate_export(args.export)
        print(f"STAFF_CONTRACTS.CPP: {len(ENTRIES)} entries, {count} exported instructions")


if __name__ == "__main__":
    main()

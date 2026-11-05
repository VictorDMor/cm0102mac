"""Original SIMULATED_STATS.CPP source pool, entry boundaries and production hooks."""
import argparse
from bisect import bisect_right
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import struct

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
MODULE = (0x1036ff60, 0x103721f0)
ROUTINES = (
    (0x1036ff60, 0x1037082c, "7468de5a224746fa"),
    (0x10370830, 0x10370da8, "033bbb9bff6a7baf"),
    (0x10370db0, 0x10370e0c, "cefcd0443d0bb245"),
    (0x10370e10, 0x10371d90, "c999afa39ed6949f"),
    (0x10371d90, 0x10371e58, "5282399a65d5e5c2"),
    (0x10371e60, 0x1037203c, "033c19ae215a9eb3"),
    (0x10372040, 0x103721ec, "acd87d41fbb0775e"),
)
ENTRIES = tuple(entry for entry, _, _ in ROUTINES)
POOL_REFERENCES = {
    0x4dd0: (0x1036ff70, 0x1037083c, 0x10370e28),
    0x4dcc: (0x1036ffa4, 0x10370e40, 0x10371e6c, 0x10372048),
    0x4dc8: (0x1036ffa8,),
    0x4dc4: (0x1036ffac,),
}


def production_entries(source):
    entries = tuple(int(value, 16) for value in re.findall(r"^SIMULATED_STATS_ROUTINE\((0x[0-9a-f]+),", source, re.M))
    if entries != tuple(sorted(set(entries))) or not set(entries) <= set(ENTRIES):
        raise ValueError("Invalid simulated-stats production registry")
    return entries


SIMULATED_STATS_ENTRIES = production_entries((ROOT / "native/simulated_stats_routines.inc").read_text())


def owner(address):
    index = bisect_right(ENTRIES, address) - 1
    if index >= 0:
        start, end, _ = ROUTINES[index]
        if address < end:
            return start
    return None


def validate_ownership(records, incoming):
    calls = set()
    for source, target, flow in incoming:
        if "CALL" in flow:
            calls.add(target)
        elif owner(source) is None or owner(source) != owner(target):
            raise ValueError(f"Cross-entry simulated-stats branch {source:#x} -> {target:#x}")
    if calls != set(ENTRIES):
        raise ValueError("Direct call roots do not match the simulated-stats inventory")

    reached = set()
    for entry in ENTRIES:
        pending = [entry]
        while pending:
            address = pending.pop()
            if owner(address) != entry or address not in records:
                raise ValueError(f"Invalid simulated-stats flow {entry:#x} -> {address:#x}")
            if address in reached:
                continue
            reached.add(address)
            item = records[address]
            if item["flow"] in ("COMPUTED_CALL", "COMPUTED_JUMP"):
                raise ValueError(f"Unclassified computed flow at {address:#x}")
            if "CALL" not in item["flow"]:
                pending.extend(item["flows"])
            if item.get("fallthrough") is not None:
                pending.append(item["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned simulated-stats export records")


def validate_sampler_alias(db, read):
    """The shared sampler differs only in relative addresses and its pool slots."""
    def normalized(entry, constants_load, bias_load):
        end = entry + 0x578
        rows = routine_records(db, entry, end)
        return [
            (row["address"] - entry, re.sub(
                r"0x[0-9a-f]+", lambda m: f"ENTRY+{int(m[0], 16) - entry:#x}"
                if entry <= int(m[0], 16) < end else m[0],
                row["assembly"].replace(constants_load, "CONSTANTS").replace(bias_load, "BIAS")))
            for row in rows
        ]

    stats = normalized(0x10370830, "-0x4dd0(r2)", "0x8(r25)")
    contracts = normalized(0x10374570, "-0x4dac(r2)", "0xc0(r25)")
    if len(stats) != 350 or stats != contracts:
        raise ValueError("Simulated-stats and staff-contract sampler bodies differ")
    stats_constants = int.from_bytes(read(TOC - 0x4dd0, 4), "big")
    contract_constants = int.from_bytes(read(TOC - 0x4dac, 4), "big")
    if (read(stats_constants, 8) != read(contract_constants, 8)
            or read(stats_constants + 8, 8) != read(contract_constants + 0xc0, 8)):
        raise ValueError("Simulated-stats sampler constants differ")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for the simulated-stats audit")
    segments = [(s["address"], (export / s["file"]).read_bytes())
                for s in program["segments"] if s.get("file")]

    def read(address, width):
        for base, data in segments:
            if base <= address and address + width <= base + len(data):
                return data[address - base:address - base + width]
        raise ValueError(f"Missing simulated-stats original bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, label in (
        (0x4dd8, 0x1080b64c, b"SHORTLIST_MANAGER.CPP\0"),
        (0x4dcc, 0x1080bd64, b"SIMULATED_STATS.CPP\0"),
        (0x4da4, 0x1080bde8, b"STAFF_CONTRACTS.CPP\0"),
    ):
        if word(TOC - slot) != base or label not in read(base, 116):
            raise ValueError(f"Changed simulated-stats source boundary at TOC -{slot:#x}")
    for slot, base in ((0x4dd0, 0x1080bdd8), (0x4dc8, 0x1080bd54), (0x4dc4, 0x1080bd40)):
        if word(TOC - slot) != base:
            raise ValueError(f"Changed simulated-stats constants at TOC -{slot:#x}")
    for base, data in segments:
        for match in re.finditer(rb"\x10[\x36\x37]", data):
            offset = match.start()
            if (base + offset) % 4 == 0 and offset + 4 <= len(data):
                target = struct.unpack_from(">I", data, offset)[0]
                if MODULE[0] <= target < MODULE[1]:
                    raise ValueError(f"Unclassified simulated-stats pointer at {base + offset:#x}")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {r["address"]: r for r in routine_records(db, *MODULE)}
        for entry, end, fingerprint in ROUTINES:
            body = [records[a] for a in range(entry, end, 4) if a in records]
            if digest(body) != fingerprint:
                raise ValueError(f"Changed simulated-stats body at {entry:#x}")
        validate_sampler_alias(db, read)
        for slot, expected in POOL_REFERENCES.items():
            observed = tuple(a for a, in db.execute(
                "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", (f"%-{slot:#x}(r2)%",)))
            if observed != expected:
                raise ValueError(f"Changed simulated-stats references to TOC -{slot:#x}")
        for address in (MODULE[0] - 4, ROUTINES[-1][1] - 4):
            if routine_records(db, address, address + 4)[0]["assembly"] != "blr":
                raise ValueError(f"Changed simulated-stats boundary return at {address:#x}")
        validate_ownership(records, db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<?", MODULE))
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters simulated stats")

    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted simulated-stats instruction at {address:#x}")
    return len(records)


def simulated_stats_hook(entry):
    return f"if (try_modern_simulated_stats(s, 0x{entry:x})) return true;"


def require_simulated_stats_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in SIMULATED_STATS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f"i_{entry:x}:"
        hook = simulated_stats_hook(entry)
        step = f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing, competing or misplaced simulated-stats hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"{entry:#x}")
    else:
        print(f"SIMULATED_STATS.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

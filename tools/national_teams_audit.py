"""Closed NATIONAL_TEAMS.CPP inventory and default C++ owner routing."""
import argparse
from bisect import bisect_right
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
MODULE = (0x102aadb0, 0x102bc340)
ROUTINES = (
    (0x102aadb0, 0x102aaf64, 109, "7d9b399a6a95a7c5"),
    (0x102aaf70, 0x102ab0a8, 77, "3890113769cb530e"),
    (0x102ab0b0, 0x102ab528, 284, "2f844fefbd85d117"),
    (0x102ab530, 0x102abbbc, 417, "d4135e42b51cd702"),
    (0x102abbc0, 0x102ac130, 345, "c87ccd2f73ec0557"),
    (0x102ac130, 0x102ac698, 345, "9d86584be73335ee"),
    (0x102ac6a0, 0x102ace24, 481, "14fd9808639167d7"),
    (0x102ace30, 0x102acf10, 55, "be07fe0d3f3344de"),
    (0x102acf10, 0x102acf38, 10, "56df29b0a1d6d2f0"),
    (0x102acf40, 0x102ae62c, 1465, "6af9d1f6aff8927b"),
    (0x102ae630, 0x102ae6c0, 36, "2c04214f4607d9a2"),
    (0x102ae6c0, 0x102ae7bc, 63, "c417c46a7f39f0d2"),
    (0x102ae7c0, 0x102ae9b4, 125, "f4881ab468bee393"),
    (0x102ae9c0, 0x102aea1c, 23, "6f272b5275b6cda9"),
    (0x102aea20, 0x102aea44, 9, "58e69f6c18700cd4"),
    (0x102aea50, 0x102aea78, 10, "5f3d763ffddbcf17"),
    (0x102aea80, 0x102aeb34, 45, "8c6b221c886f1442"),
    (0x102aeb40, 0x102aec10, 52, "f53ef573fe8a1cac"),
    (0x102aec10, 0x102aed74, 89, "ee58aa312ac03a32"),
    (0x102aed80, 0x102aee2c, 43, "a66e4caddb0e36b9"),
    (0x102aee30, 0x102aefb0, 96, "6e6b6c5871b56bce"),
    (0x102aefb0, 0x102af084, 53, "b49d7366412a246d"),
    (0x102af090, 0x102af620, 356, "3dcace9a57eeb7d1"),
    (0x102af620, 0x102af644, 9, "f24b7ad9ce9cf007"),
    (0x102af650, 0x102af674, 9, "fe1a140ef79bbf7d"),
    (0x102af680, 0x102af6dc, 23, "b55bf526b3bdd45d"),
    (0x102af6e0, 0x102af7c8, 57, "35f887cc4de5ee32"),
    (0x102af7d0, 0x102afad0, 191, "8d7ea43bbf532972"),
    (0x102afad0, 0x102b09e0, 963, "805545b1464d1ef1"),
    (0x102b09e0, 0x102b0c18, 141, "0aaf919eaa9ce820"),
    (0x102b0c20, 0x102b1130, 324, "a8fcb5ed3f3a0e7e"),
    (0x102b1130, 0x102b13ec, 175, "4d2c14420ebd5f5b"),
    (0x102b13f0, 0x102b16cc, 183, "f4eb66b32053549d"),
    (0x102b16d0, 0x102b1934, 153, "38c0451d22ad38c4"),
    (0x102b1940, 0x102b1bd8, 166, "920f458ff2ae846f"),
    (0x102b1be0, 0x102b1e14, 141, "491f4bd199873328"),
    (0x102b1e20, 0x102b20fc, 183, "22c207277324c3af"),
    (0x102b2100, 0x102b2840, 462, "1b925db59762ef3f"),
    (0x102b2840, 0x102b6ffc, 4591, "bd9ca05a36c2b144"),
    (0x102b7000, 0x102b70a0, 40, "997a58465b3fd754"),
    (0x102b70a0, 0x102b7138, 38, "5ce45384b8af844d"),
    (0x102b7140, 0x102b71d4, 37, "ff1403b439360b1d"),
    (0x102b71e0, 0x102b727c, 39, "1a1e3e504fc2fd8b"),
    (0x102b7280, 0x102b7444, 113, "58439f19a3456d30"),
    (0x102b7450, 0x102b7490, 16, "c83e4351fe8aeab7"),
    (0x102b7490, 0x102b7748, 174, "d472299a25b86067"),
    (0x102b7750, 0x102b7830, 56, "09a3b487aa698caf"),
    (0x102b7830, 0x102b7900, 52, "03cc86e69b765fa3"),
    (0x102b7900, 0x102b79d0, 52, "ebf723ef3ea0bbf0"),
    (0x102b79d0, 0x102b7a9c, 50, "c5bcdfce303615e2"),
    (0x102b7aa0, 0x102b7bf4, 85, "348940c6ca389450"),
    (0x102b7c00, 0x102b7c70, 28, "41e828a138619bd9"),
    (0x102b7c70, 0x102b8178, 320, "147213ba65446338"),
    (0x102b8180, 0x102b8454, 180, "366e66a1532819af"),
    (0x102b8460, 0x102b8844, 248, "86d907d72437f6e8"),
    (0x102b8850, 0x102b8c48, 254, "89d915ef1aba721b"),
    (0x102b8c50, 0x102b8d54, 64, "bbd2b5e659deabd5"),
    (0x102b8d60, 0x102b8e60, 62, "1c0b64f0f22585e5"),
    (0x102b8e60, 0x102b8eb4, 21, "7c6bfe259eeb6386"),
    (0x102b8ec0, 0x102b96dc, 519, "2e22ca6638eceeb4"),
    (0x102b96e0, 0x102b9e48, 468, "b0315b1360cff091"),
    (0x102b9e50, 0x102ba1b8, 217, "3d90746a46659ee2"),
    (0x102ba1c0, 0x102ba284, 49, "901c6c73ac06f977"),
    (0x102ba290, 0x102ba60c, 223, "4876538ea575fbac"),
    (0x102ba610, 0x102ba7b0, 104, "bd2cf839ffd5d622"),
    (0x102ba7b0, 0x102baa3c, 162, "9aff6a672744e547"),
    (0x102baa40, 0x102bacb4, 157, "ed7e9c1c0d332113"),
    (0x102bacc0, 0x102bad08, 18, "271e5c5627aac779"),
    (0x102bad10, 0x102bb050, 206, "d6f2253c3381d224"),
    (0x102bb050, 0x102bc178, 1098, "7a4b021a3c84cb53"),
    (0x102bc180, 0x102bc250, 52, "2cc9aceed318b555"),
    (0x102bc250, 0x102bc270, 8, "5dbbc811d96a3f6a"),
    (0x102bc270, 0x102bc340, 52, "7aca1c4fa79c75cd"),
)
ENTRIES = tuple(entry for entry, _, _, _ in ROUTINES)
NATIONAL_TEAMS_ENTRIES = (0x102ae9c0, 0x102aea20, 0x102aea50, 0x102af620, 0x102af650, 0x102b7450,
                          0x102b7c00, 0x102b7c70)
VECTORS = {
    0x108b8d14: 0x102bc180,
    0x108b8d1c: 0x102bc270,
    0x108b8d24: 0x102bc250,
}
SOURCE_REFS = (
    0x102aadb8, 0x102ab0e0, 0x102ae2f4, 0x102ae314, 0x102ae334, 0x102ae454,
    0x102ae474, 0x102ae494, 0x102ae6d0, 0x102af0b0, 0x102afaf0, 0x102b0c3c,
    0x102b1410, 0x102b1960, 0x102b1e40, 0x102b2858, 0x102b74d0, 0x102b7aac,
    0x102b7c98, 0x102b847c, 0x102b8ee4, 0x102b96fc, 0x102ba1e8, 0x102ba2b8,
    0x102ba7d8, 0x102bb078,
)
INCOMING_DIGEST = "3109c927be5a0e1ac0709eb4eff25296f4f03b970d828885045ca01eceb350c1"
UNALIGNED_DIGEST = "53e75f61040b6be160bebd5a333d9eb19bb1c2e84305ae541355b2c03714a392"


def fingerprint(rows):
    return hashlib.sha256(json.dumps(rows, separators=(",", ":")).encode()).hexdigest()


def owner(address):
    index = bisect_right(ENTRIES, address) - 1
    if index >= 0 and address < ROUTINES[index][1]:
        return ENTRIES[index]
    return None


def validate_flows(records, incoming):
    roots = set(VECTORS.values())
    for site, target, flow in incoming:
        if "CALL" in flow:
            roots.add(target)
        elif owner(site) is None or owner(site) != owner(target):
            raise ValueError(f"NATIONAL_TEAMS cross-entry branch {site:#x} -> {target:#x}")
    if roots != set(ENTRIES) or fingerprint(incoming) != INCOMING_DIGEST:
        raise ValueError("Changed NATIONAL_TEAMS roots or call edges")
    claimed = set()
    for entry, end, count, expected in ROUTINES:
        body = [records[a] for a in range(entry, end, 4) if a in records]
        if len(body) != count or digest(body) != expected:
            raise ValueError(f"Changed NATIONAL_TEAMS body at {entry:#x}")
        pending = [entry]
        while pending:
            address = pending.pop()
            if owner(address) != entry or address not in records:
                raise ValueError(f"NATIONAL_TEAMS flow leaves its entry at {address:#x}")
            if address in claimed:
                continue
            claimed.add(address)
            record = records[address]
            if record["flow"].startswith("COMPUTED") and "CALL" not in record["flow"]:
                raise ValueError(f"NATIONAL_TEAMS unclassified computed jump at {address:#x}")
            if "CALL" not in record["flow"]:
                pending.extend(record["flows"])
            if record.get("fallthrough") is not None:
                pending.append(record["fallthrough"])
    if claimed != records.keys():
        raise ValueError("Unowned NATIONAL_TEAMS instructions")


def validate_export(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for NATIONAL_TEAMS audit")
    segments = [(seg["address"], (export / seg["file"]).read_bytes())
                for seg in program["segments"] if seg.get("file")]

    def read(address, size):
        for base, data in segments:
            if base <= address <= base + len(data) - size:
                return data[address - base:address - base + size]
        raise ValueError(f"Missing NATIONAL_TEAMS bytes at {address:#x}")

    def word(address):
        return int.from_bytes(read(address, 4), "big")

    for slot, base, offset, label in (
            (-0x5150, 0x107e7530, 0x8ac, b"MENUBAR.CPP\0"),
            (-0x513c, 0x107e7e80, 0x37, b"NATIONAL_TEAMS.CPP\0"),
            (-0x5120, 0x107f0efc, 0, b"NATIONAL_TEAMS_SCREENS.CPP\0")):
        if word(TOC + slot) != base or read(base + offset, len(label)) != label:
            raise ValueError("Changed NATIONAL_TEAMS source pool boundary")
    # MENUBAR's adjacent 0xfc-byte array retains its constructor and destructor.
    for slot, target in ((-0x63e8, 0x102aad50), (-0x63ec, 0x102aad10)):
        vector = word(TOC + slot)
        if word(vector) != target or word(vector + 4) != TOC:
            raise ValueError("Changed adjacent MENUBAR array")
    if any(word(slot) != target or word(slot + 4) != TOC for slot, target in VECTORS.items()):
        raise ValueError("Changed NATIONAL_TEAMS comparator transition vectors")

    aligned, unaligned = {}, []
    for base, data in segments:
        for match in re.finditer(rb"\x10[\x2a\x2b]", data):
            offset = match.start()
            target = int.from_bytes(data[offset:offset + 4], "big")
            if MODULE[0] <= target < MODULE[1]:
                address = base + offset
                if address % 4:
                    unaligned.append((address, target))
                else:
                    aligned[address] = target
    if aligned != VECTORS or fingerprint(sorted(unaligned)) != UNALIGNED_DIGEST:
        raise ValueError("Unclassified NATIONAL_TEAMS pointer span")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        records = {record["address"]: record for record in routine_records(db, *MODULE)}
        source = tuple(address for address, in db.execute(
            "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x513c(r2)%",)))
        if source != SOURCE_REFS:
            raise ValueError("Changed NATIONAL_TEAMS assertion references")
        for address, assembly in ((0x102aada0, "blr"), (0x102bc33c, "blr"), (0x102bc340, "mfspr r0,LR")):
            if routine_records(db, address, address + 4)[0]["assembly"] != assembly:
                raise ValueError("Changed NATIONAL_TEAMS boundary code")
        incoming = list(db.execute(
            "SELECT i.address,f.value,json_extract(i.body,'$.flow') FROM instructions i,"
            "json_each(i.body,'$.flows') f WHERE f.value>=? AND f.value<? ORDER BY i.address", MODULE))
        validate_flows(records, incoming)
        if db.execute("SELECT address FROM instructions WHERE (address<? OR address>=?) "
                      "AND json_extract(body,'$.fallthrough')>=? AND json_extract(body,'$.fallthrough')<? LIMIT 1",
                      (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough into NATIONAL_TEAMS")
    for address in range(*MODULE, 4):
        if address not in records and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unowned NATIONAL_TEAMS executable word at {address:#x}")
    if len(set(NATIONAL_TEAMS_ENTRIES)) != len(NATIONAL_TEAMS_ENTRIES) or not set(NATIONAL_TEAMS_ENTRIES) <= set(ENTRIES):
        raise ValueError("Invalid NATIONAL_TEAMS production registry")
    return len(records)


def national_teams_hook(entry):
    return f"if (try_modern_national_teams(s, 0x{entry:x})) return true;"


def require_national_teams_hooks(page, instructions, code):
    addresses = {record["address"] for record in instructions}
    for entry in NATIONAL_TEAMS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f"i_{entry:x}:", national_teams_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(label, 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if (label not in code or code.count(hook) != 1 or hook not in fragment or step not in fragment
                or fragment.index(hook) > fragment.index(step) or fragment.count("try_modern_") != 1):
            raise ValueError(f"Missing or competing NATIONAL_TEAMS hook at {entry:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    args = parser.parse_args()
    print(f"NATIONAL_TEAMS.CPP: {len(ENTRIES)} entries, {validate_export(args.export)} reachable instructions")

"""AWARD_MANAGER.CPP original instruction inventory and C++ routing."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10436080, 0x10437cc0)
ROUTINES = (
    (0x10436080, 0x104362d0, 147, "0e6a44b272a63dca"),
    (0x104362d0, 0x104363e0, 65, "1e67a3fe33b61dc1"),
    (0x104363e0, 0x104364e0, 62, "6e700edd0d5ccb20"),
    (0x104364e0, 0x104365b0, 52, "8e814026165d14d4"),
    (0x104365b0, 0x10436dc0, 516, "ff89d8208b7d96b4"),
    (0x10436dc0, 0x10436f60, 103, "cbfc71bf0d920381"),
    (0x10436f60, 0x104370f0, 97, "355828d4cef8bc32"),
    (0x104370f0, 0x10437130, 13, "b1db0886007798c2"),
    (0x10437130, 0x10437160, 9, "1e3c016f24a41f09"),
    (0x10437160, 0x104371a0, 13, "967bc3fae62acb69"),
    (0x104371a0, 0x104371d0, 9, "d92fcf6d6f21b38f"),
    (0x104371d0, 0x104372c0, 59, "5e2c5201bc5e66e2"),
    (0x104372c0, 0x10437310, 20, "cb833fa30df34436"),
    (0x10437310, 0x10437730, 264, "0e14c2916ec56c9b"),
    (0x10437730, 0x10437850, 72, "6f7da0156d19bcd5"),
    (0x10437850, 0x10437910, 48, "a64ea8f46a53f68f"),
    (0x10437910, 0x10437a00, 58, "6caa8316ccf9e3eb"),
    (0x10437a00, 0x10437a90, 32, "e8027ab02519cbfc"),
    (0x10437a90, 0x10437b20, 32, "9b567509020e9841"),
    (0x10437b20, 0x10437b50, 12, "2b2d930ae5796c02"),
    (0x10437b50, 0x10437cc0, 92, "9eec63b7873bdc0e"),
)
INCLUDES = ROOT / "native/award_manager_routines.inc"
ROUTINE = re.compile(r'AWARD_MANAGER_ROUTINE\(\s*(0x[0-9a-f]+),\s*(0x[0-9a-f]+),\s*\w+,\s*"([0-9a-f]+)"')


def rewritten(path=INCLUDES):
    inventory = {a: (b, fingerprint) for a, b, _, fingerprint in ROUTINES}
    entries = set()
    for entry, end, fingerprint in ROUTINE.findall(path.read_text()):
        entry, end = int(entry, 16), int(end, 16)
        if inventory.get(entry) != (end, fingerprint):
            raise ValueError(f"Invalid AWARD_MANAGER routine contract at {entry:x}")
        if entry in entries:
            raise ValueError(f"Duplicate AWARD_MANAGER routine at {entry:x}")
        entries.add(entry)
    return tuple(sorted(entries))


AWARD_MANAGER_ENTRIES = rewritten()


def require_award_manager_index(db):
    if AWARD_MANAGER_ENTRIES != tuple(entry for entry, _, _, _ in ROUTINES):
        raise ValueError("AWARD_MANAGER must route its complete inventory")
    for entry, end, count, expected in ROUTINES:
        records = routine_records(db, entry, end)
        if len(records) != count or digest(records) != expected:
            raise ValueError(f"AWARD_MANAGER original changed at {entry:x}")


def award_manager_hook(address):
    return f"if (try_modern_award_manager(s, 0x{address:x})) return true;"


if __name__ == "__main__":
    import argparse
    import sqlite3

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--entries", action="store_true")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    args = parser.parse_args()
    with sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_award_manager_index(db)
    for entry, end, count, expected in ROUTINES:
        if args.entries:
            print(f"0x{entry:x}")
        else:
            print(f"0x{entry:x} 0x{end:x} {count:4} {expected}")

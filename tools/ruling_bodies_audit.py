"""The disciplinary rulebooks rb_*.cpp, one class per ruling body, as one family.

Each unit holds one class in this order: constructor, the report of a match's cards, the nation text
(whose assertion names the unit's source file) and the "next card bans" query. The class's virtual
table ties the last three together; the constructor that stores that table is called directly from
the factory at 0x100e7xxx-0x100e8xxx. Units are added here as their C++ lands.

rb_ireland.cpp is 0x106ab8f0, 0x106ab970, 0x106abe20, 0x106abee0. Its first two routines were
delivered on 2026-11-05 as part of "rb_greece" (tools/rb_greece_audit.py took the Greek text and
query that precede them for the same class); they stay with that owner.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
# Unit -> its routines in this family (entry, end).
UNITS = {
    "rb_ireland.cpp": ((0x106abe20, 0x106abed4), (0x106abee0, 0x106abf58)),
    "rb_northern_ireland.cpp": ((0x106abf60, 0x106abfe0), (0x106abfe0, 0x106ac21c), (0x106ac220, 0x106ac2d4),
                                (0x106ac2e0, 0x106ac39c), (0x106ac3a0, 0x106ac3cc)),
}
ROUTINES = tuple(sorted(routine for routines in UNITS.values() for routine in routines))
ROUTINE = re.compile(r'RULING_BODIES_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "ruling_bodies_routines.inc"


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (RULING_BODIES_ROUTINE), with drafts also the RULING_BODIES_DRAFT ones check tools run."""
    text = path.read_text() if path.exists() else ""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(text) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"ruling_bodies_routines.inc: {entry:x}-{end:x} is not a routine of the family")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A ruling-body routine is listed twice")
    return found


RULING_BODIES_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_ruling_bodies_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"Ruling-body routine {entry:x} changed in the export (digest {actual}, "
                             f"ruling_bodies_routines.inc pins {expected})")


def ruling_bodies_hook(address):
    return f"if (try_modern_ruling_bodies(s, 0x{address:x})) return true;"


def require_ruling_bodies_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in RULING_BODIES_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = ruling_bodies_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced ruling-body hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of ruling_bodies_routines.inc."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", action="store_true", help="print every entry alone (for a census)")
    args = parser.parse_args()
    if args.entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    db = sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
    done = {entry for entry, _, _ in rewritten()}
    drafted = {entry for entry, _, _ in rewritten(drafts=True)} - done
    for unit, routines in UNITS.items():
        for entry, end in routines:
            records = routine_records(db, entry, end)
            stage = " (C++)" if entry in done else " (draft)" if entry in drafted else ""
            print(f"{unit:26} 0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

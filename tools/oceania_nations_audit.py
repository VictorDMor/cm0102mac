"""OCEANIA_NATIONS.CPP: 15 entries between FIFA_CONFED and INTER_AMER_CUP.

Source pool -0x4094, vtable -0x5cc4 and eleven transition vectors close the
indirect entry set. Whole-export incoming flows identify only the constructor;
internal calls identify three private routines. Every indexed instruction and
alignment gap is accounted for. Detailed game-derived evidence stays private.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106e9940, 0x106ebdd0)
ROUTINES = (
    (0x106e9940, 0x106e9ba0),
    (0x106e9ba0, 0x106e9cb0),
    (0x106e9cb0, 0x106ea780),
    (0x106ea780, 0x106eaaf0),
    (0x106eaaf0, 0x106eabc0),
    (0x106eabc0, 0x106eac00),
    (0x106eac00, 0x106eada0),
    (0x106eada0, 0x106eaee0),
    (0x106eaee0, 0x106eafa0),
    (0x106eafa0, 0x106eb1d0),
    (0x106eb1d0, 0x106eb400),
    (0x106eb400, 0x106eb7a0),
    (0x106eb7a0, 0x106ebb10),
    (0x106ebb10, 0x106ebcd0),
    (0x106ebcd0, 0x106ebdd0),
)
ROUTINE = re.compile(r'OCEANIA_NATIONS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(oceania_nations_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "oceania_nations_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of oceania_nations_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (OCEANIA_NATIONS_ROUTINE), with drafts also the OCEANIA_NATIONS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"oceania_nations_routines.inc: {entry:x}-{end:x} is not a routine of OCEANIA_NATIONS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An OCEANIA_NATIONS.CPP routine is listed twice")
    return found


OCEANIA_NATIONS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_oceania_nations_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"OCEANIA_NATIONS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"oceania_nations_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def oceania_nations_hook(address):
    return f"if (try_modern_oceania_nations(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of oceania_nations_routines.inc."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routines to print (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true",
                        help="print every entry alone (for cm-world-replay --census)")
    parser.add_argument("--volatiles", action="store_true",
                        help="list reads of the registers the C++ does not reproduce (empty when none)")
    args = parser.parse_args()
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    db = sqlite3.connect(args.export / "instructions.sqlite")
    if args.volatiles:
        for line in volatile_reads(db):
            print(line)
        return
    wanted = {int(e, 16) for e in args.entries}
    done = {entry for entry, _, _ in rewritten()}
    drafted = {entry for entry, _, _ in rewritten(drafts=True)} - done
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        stage = " (C++)" if entry in done else " (draft)" if entry in drafted else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

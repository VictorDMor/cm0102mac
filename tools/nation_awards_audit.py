"""NATION_AWARDS.CPP: 20 entries between MONTH_RATINGS and NORWAY_AWARDS.

Source/string pool -0x4a18, the base vtable and floating constants close the module
boundaries. Whole-export direct flows identify every entry; initialized data has seven transition vectors into this module. Detailed evidence stays private.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10444390, 0x10446d90)
ROUTINES = (
    (0x10444390, 0x104443b0),
    (0x104443b0, 0x10444490),
    (0x10444490, 0x10444550),
    (0x10444550, 0x10444580),
    (0x10444580, 0x104449c0),
    (0x104449c0, 0x10444c40),
    (0x10444c40, 0x10444e80),
    (0x10444e80, 0x104450c0),
    (0x104450c0, 0x104455b0),
    (0x104455b0, 0x104459a0),
    (0x104459a0, 0x10446040),
    (0x10446040, 0x104462b0),
    (0x104462b0, 0x10446510),
    (0x10446510, 0x10446b00),
    (0x10446b00, 0x10446c00),
    (0x10446c00, 0x10446c10),
    (0x10446c10, 0x10446d20),
    (0x10446d20, 0x10446d30),
    (0x10446d30, 0x10446d60),
    (0x10446d60, 0x10446d90),
)
ROUTINE = re.compile(r'NATION_AWARDS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(nation_awards_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "nation_awards_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of nation_awards_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (NATION_AWARDS_ROUTINE), with drafts also the NATION_AWARDS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"nation_awards_routines.inc: {entry:x}-{end:x} is not a routine of NATION_AWARDS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A NATION_AWARDS.CPP routine is listed twice")
    return found


NATION_AWARDS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_nation_awards_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"NATION_AWARDS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"nation_awards_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def nation_awards_hook(address):
    return f"if (try_modern_nation_awards(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of nation_awards_routines.inc."""
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

"""POL_CUP.CPP: six entries between NIR_LGE_CUP and POL_LGE_CUP.

Pool -0x40ec is read only by four owned routines; the adjacent pools are
-0x40f0 / -0x40e8. The -0x5d0c vtable owns destructor, reset, calendar and
configuration TVs. The sole external direct call reaches the constructor.
Whole-export flow and bytewise initialized-segment pointer scans found no
additional entries or interior resumes. Detailed evidence remains private.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106cd810, 0x106ce820)
ROUTINES = ((275568656, 275569392), (275569392, 275569632), (275569632, 275571072), (275571072, 275572080), (275572080, 275572672), (275572672, 275572768))
ROUTINE = re.compile(r'POL_CUP_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(pol_cup_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "pol_cup_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of pol_cup_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (POL_CUP_ROUTINE), with drafts also the POL_CUP_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"pol_cup_routines.inc: {entry:x}-{end:x} is not a routine of POL_CUP.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An POL_CUP.CPP routine is listed twice")
    return found


POL_CUP_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_pol_cup_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"POL_CUP.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"pol_cup_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "NONE" if a == 0x106ce570 else "R3" for a, _ in ROUTINES}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def pol_cup_hook(address):
    return f"if (try_modern_pol_cup(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of pol_cup_routines.inc."""
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

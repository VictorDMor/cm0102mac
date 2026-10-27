"""SCRMAN.CPP screen manager and its adjacent sound support.

The screen constructor starts at 0x1073c1b0; the registered global
initializer ends at 0x10744670. The adjacent eleven sound routines are
called directly by SCRMAN. Include them conservatively up to TCPIP.CPP
at 0x10744fc0: source assertions alone cannot prove the sound source name.
The monitor switch labels are internal branches, not callable entries.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1073c1b0, 0x10744fc0)
ROUTINES = ((0x1073c1b0,0x1073c460), (0x1073c460,0x1073c4e0), (0x1073c4e0,0x1073c5a0), (0x1073c5a0,0x1073c700), (0x1073c700,0x1073c730), (0x1073c730,0x1073da60), (0x1073da60,0x1073e350), (0x1073e350,0x1073e4b0), (0x1073e4b0,0x1073e990), (0x1073e990,0x1073e9b0), (0x1073e9b0,0x1073ea20), (0x1073ea20,0x1073eb00), (0x1073eb00,0x1073ec00), (0x1073ec00,0x1073ecd0), (0x1073ecd0,0x1073edb0), (0x1073edb0,0x1073ede0), (0x1073ede0,0x1073ee30), (0x1073ee30,0x1073ef70), (0x1073ef70,0x1073f0a0), (0x1073f0a0,0x1073f1e0), (0x1073f1e0,0x1073f450), (0x1073f450,0x1073f4e0), (0x1073f4e0,0x1073f530), (0x1073f530,0x1073f700), (0x1073f700,0x1073f7a0), (0x1073f7a0,0x1073f860), (0x1073f860,0x1073f900), (0x1073f900,0x1073fc20), (0x1073fc20,0x1073fd00), (0x1073fd00,0x1073fd60), (0x1073fd60,0x1073fdf0), (0x1073fdf0,0x1073fe80), (0x1073fe80,0x1073ff10), (0x1073ff10,0x1073ff80), (0x1073ff80,0x1073ffe0), (0x1073ffe0,0x10740070), (0x10740070,0x107401e0), (0x107401e0,0x107403c0), (0x107403c0,0x10740620), (0x10740620,0x10740940), (0x10740940,0x10740e10), (0x10740e10,0x10741130), (0x10741130,0x10741450), (0x10741450,0x10741770), (0x10741770,0x10741a80), (0x10741a80,0x10741da0), (0x10741da0,0x10742100), (0x10742100,0x10742420), (0x10742420,0x10742740), (0x10742740,0x10742b00), (0x10742b00,0x10742b80), (0x10742b80,0x10742e60), (0x10742e60,0x10742e90), (0x10742e90,0x10742ec0), (0x10742ec0,0x10743120), (0x10743120,0x10743510), (0x10743510,0x10743590), (0x10743590,0x107435c0), (0x107435c0,0x10743660), (0x10743660,0x10743840), (0x10743840,0x107439b0), (0x107439b0,0x10743bd0), (0x10743bd0,0x10743d40), (0x10743d40,0x10743eb0), (0x10743eb0,0x10744020), (0x10744020,0x107440c0), (0x107440c0,0x10744250), (0x10744250,0x107442f0), (0x107442f0,0x10744390), (0x10744390,0x10744430), (0x10744430,0x107444e0), (0x107444e0,0x107445a0), (0x107445a0,0x10744640), (0x10744640,0x10744670), (0x10744670,0x107446b0), (0x107446b0,0x10744700), (0x10744700,0x10744770), (0x10744770,0x10744860), (0x10744860,0x10744a60), (0x10744a60,0x10744c50), (0x10744c50,0x10744ca0), (0x10744ca0,0x10744d40), (0x10744d40,0x10744e50), (0x10744e50,0x10744ee0), (0x10744ee0,0x10744fc0))
ROUTINE = re.compile(r'SCREEN_MANAGER_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(screen_manager_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "screen_manager_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of screen_manager_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (SCREEN_MANAGER_ROUTINE), with drafts also the SCREEN_MANAGER_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"screen_manager_routines.inc: {entry:x}-{end:x} is not a routine of SCREEN_MANAGER.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An SCREEN_MANAGER.CPP routine is listed twice")
    return found


SCREEN_MANAGER_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_screen_manager_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"SCREEN_MANAGER.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"screen_manager_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def screen_manager_hook(address):
    return f"if (try_modern_screen_manager(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of screen_manager_routines.inc."""
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

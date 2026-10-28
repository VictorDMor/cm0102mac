"""CLUB_RECORDS.CPP: club record storage, updates and presentation.

The source pool at TOC -0x599c follows CLUB_HISTORY and precedes
CLUB_SCREENS. Two transition vectors and direct calls establish entries;
branch-table labels remain within their containing functions.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1000e6b0, 0x1001dae0)
ROUTINES = (
    (0x1000e6b0, 0x1000ea50),
    (0x1000ea50, 0x1000eb40),
    (0x1000eb40, 0x10011750),
    (0x10011750, 0x10011bd0),
    (0x10011bd0, 0x100121c0),
    (0x100121c0, 0x10012210),
    (0x10012210, 0x10012230),
    (0x10012230, 0x10012250),
    (0x10012250, 0x100122c0),
    (0x100122c0, 0x10012330),
    (0x10012330, 0x10012380),
    (0x10012380, 0x10012440),
    (0x10012440, 0x10012500),
    (0x10012500, 0x10012550),
    (0x10012550, 0x100125a0),
    (0x100125a0, 0x10012600),
    (0x10012600, 0x10012650),
    (0x10012650, 0x100126b0),
    (0x100126b0, 0x10012700),
    (0x10012700, 0x100127b0),
    (0x100127b0, 0x10012800),
    (0x10012800, 0x10012850),
    (0x10012850, 0x10012890),
    (0x10012890, 0x100128d0),
    (0x100128d0, 0x10012910),
    (0x10012910, 0x10012950),
    (0x10012950, 0x100129c0),
    (0x100129c0, 0x10012a30),
    (0x10012a30, 0x10012aa0),
    (0x10012aa0, 0x10012b10),
    (0x10012b10, 0x10012b80),
    (0x10012b80, 0x10012ba0),
    (0x10012ba0, 0x10012c10),
    (0x10012c10, 0x10012c80),
    (0x10012c80, 0x10012cf0),
    (0x10012cf0, 0x10012d10),
    (0x10012d10, 0x10012d30),
    (0x10012d30, 0x10012e90),
    (0x10012e90, 0x10012fb0),
    (0x10012fb0, 0x100132f0),
    (0x100132f0, 0x100134b0),
    (0x100134b0, 0x10013650),
    (0x10013650, 0x100137c0),
    (0x100137c0, 0x10013910),
    (0x10013910, 0x10013cc0),
    (0x10013cc0, 0x10013ee0),
    (0x10013ee0, 0x10014080),
    (0x10014080, 0x100163b0),
    (0x100163b0, 0x100163f0),
    (0x100163f0, 0x100190b0),
    (0x100190b0, 0x100190f0),
    (0x100190f0, 0x10019130),
    (0x10019130, 0x10019170),
    (0x10019170, 0x100191b0),
    (0x100191b0, 0x100191f0),
    (0x100191f0, 0x10019230),
    (0x10019230, 0x10019270),
    (0x10019270, 0x100192b0),
    (0x100192b0, 0x100192f0),
    (0x100192f0, 0x10019330),
    (0x10019330, 0x10019370),
    (0x10019370, 0x100193b0),
    (0x100193b0, 0x100193f0),
    (0x100193f0, 0x10019430),
    (0x10019430, 0x10019470),
    (0x10019470, 0x100194b0),
    (0x100194b0, 0x100194f0),
    (0x100194f0, 0x10019530),
    (0x10019530, 0x10019570),
    (0x10019570, 0x100195a0),
    (0x100195a0, 0x100195d0),
    (0x100195d0, 0x10019600),
    (0x10019600, 0x10019630),
    (0x10019630, 0x10019660),
    (0x10019660, 0x100196a0),
    (0x100196a0, 0x100196e0),
    (0x100196e0, 0x10019720),
    (0x10019720, 0x10019ca0),
    (0x10019ca0, 0x1001a690),
    (0x1001a690, 0x1001b120),
    (0x1001b120, 0x1001b830),
    (0x1001b830, 0x1001b9c0),
    (0x1001b9c0, 0x1001bb50),
    (0x1001bb50, 0x1001bd60),
    (0x1001bd60, 0x1001be60),
    (0x1001be60, 0x1001bec0),
    (0x1001bec0, 0x1001c160),
    (0x1001c160, 0x1001c290),
    (0x1001c290, 0x1001cb30),
    (0x1001cb30, 0x1001d410),
    (0x1001d410, 0x1001daa0),
    (0x1001daa0, 0x1001dae0),
)
ROUTINE = re.compile(r'CLUB_RECORDS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(club_records_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "club_records_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of club_records_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (CLUB_RECORDS_ROUTINE), with drafts also the CLUB_RECORDS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"club_records_routines.inc: {entry:x}-{end:x} is not a routine of CLUB_RECORDS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An CLUB_RECORDS.CPP routine is listed twice")
    return found


CLUB_RECORDS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_club_records_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"CLUB_RECORDS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"club_records_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def club_records_hook(address):
    return f"if (try_modern_club_records(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of club_records_routines.inc."""
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

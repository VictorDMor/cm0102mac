"""The original's MAIN.CPP and the routines of it that run as C++.

ROUTINES lists every routine of MAIN.CPP as (entry, end). The module runs
from main() (0x1076f2b0, called by the runtime's start at 0x1077375c) to the
runtime library's byte copy (0x10771110): its routines read MAIN.CPP's
string pool (TOC -0x3bac and the slots next to it, -0x3bcc..-0x3ba8) and its
globals (TOC -0x354e..-0x352b), and the two Apple-event callbacks at the end
are reached through its own TOC slots. The MoreFiles helpers before main()
and the runtime after 0x10771110 use neither. The entries are the analysed
functions plus every direct call or function-pointer target; each routine
runs to the next entry.

native/main_program_routines.inc lists the routines rewritten so far, each
with a digest of its original instructions (tools/tactics_audit.py digest):
MAIN_ROUTINE lines are verified and generation hooks them
(try_modern_main_program); MAIN_DRAFT lines run only in check tools
(cm-world-replay --check-routines). Generation fails when the export no
longer matches a digest.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1076f2b0, 0x10771110)
ROUTINES = (
    (0x1076f2b0, 0x1076f2e0), (0x1076f2e0, 0x1076f440), (0x1076f440, 0x1076f4f0), (0x1076f4f0, 0x1076f600),
    (0x1076f600, 0x1076f730), (0x1076f730, 0x1076f780), (0x1076f780, 0x1076f7d0), (0x1076f7d0, 0x1076f880),
    (0x1076f880, 0x1076f890), (0x1076f890, 0x1076f960), (0x1076f960, 0x1076fa20), (0x1076fa20, 0x1076faf0),
    (0x1076faf0, 0x1076fd90), (0x1076fd90, 0x1076fe80), (0x1076fe80, 0x10770170), (0x10770170, 0x107701d0),
    (0x107701d0, 0x10770240), (0x10770240, 0x10770380), (0x10770380, 0x107704e0), (0x107704e0, 0x10770570),
    (0x10770570, 0x10770650), (0x10770650, 0x10770670), (0x10770670, 0x10770760), (0x10770760, 0x107707c0),
    (0x107707c0, 0x10770880), (0x10770880, 0x107708c0), (0x107708c0, 0x10770950), (0x10770950, 0x10770970),
    (0x10770970, 0x107709e0), (0x107709e0, 0x10770a10), (0x10770a10, 0x10770a20), (0x10770a20, 0x10770a30),
    (0x10770a30, 0x10770a80), (0x10770a80, 0x10770ab0), (0x10770ab0, 0x10770ac0), (0x10770ac0, 0x10770c90),
    (0x10770c90, 0x10770ca0), (0x10770ca0, 0x10770cc0), (0x10770cc0, 0x10770d20), (0x10770d20, 0x10770e40),
    (0x10770e40, 0x10770ee0), (0x10770ee0, 0x10770f60), (0x10770f60, 0x10771010), (0x10771010, 0x10771110),
)
ROUTINE = re.compile(r'MAIN_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*(\w+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "main_program_routines.inc"


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (MAIN_ROUTINE), with drafts also the MAIN_DRAFT ones check tools run."""
    if not path.exists():
        return []
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(path.read_text()) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"main_program_routines.inc: {entry:x}-{end:x} is not a routine of MAIN.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A MAIN.CPP routine is listed twice")
    return found


MAIN_PROGRAM_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_main_program_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"MAIN.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"main_program_routines.inc pins {expected})")


def main_program_hook(address):
    return f"if (try_modern_main_program(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of main_program_routines.inc."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routines to print (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true",
                        help="print every entry alone (for cm-world-replay --census)")
    args = parser.parse_args()
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    db = sqlite3.connect(args.export / "instructions.sqlite")
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

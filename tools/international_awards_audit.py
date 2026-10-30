"""INTERNATIONAL_AWARDS.CPP: international award construction and publication."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10440620, 0x10440c70)
ROUTINES = ((0x10440620, 0x104406d0), (0x104406d0, 0x104409c0),
            (0x104409c0, 0x10440c70))
ROUTINE = re.compile(r'INTERNATIONAL_AWARDS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(international_awards_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "international_awards_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of international_awards_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (INTERNATIONAL_AWARDS_ROUTINE), with drafts also the INTERNATIONAL_AWARDS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"international_awards_routines.inc: {entry:x}-{end:x} is not a routine of INTERNATIONAL_AWARDS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A INTERNATIONAL_AWARDS.CPP routine is listed twice")
    return found


INTERNATIONAL_AWARDS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_international_awards_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"INTERNATIONAL_AWARDS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"international_awards_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def international_awards_hook(address):
    return f"if (try_modern_international_awards(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of international_awards_routines.inc."""
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

"""DATE.CPP: the game's dates (construction, arithmetic, differences, text).

31 routines at 0x107156b0..0x10718af0, named by the unit's assertion file name
(the pool at TOC -0x3fa0 starts "DATE.CPP") and bounded by what uses its TOC
slots -0x3fe8..-0x3fa0 and its static dates' guard bytes. One more routine is
delivered with it: 0x10718af0, an 18-instruction leaf between this unit and
DISPLAY.CPP with no data of its own (a language and a month in, a flag out).
The evidence is checked in tools/test_date_audit.py.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x107156b0, 0x10718b38)
ROUTINES = (
    (0x107156b0, 0x107156f0), (0x107156f0, 0x1071587c), (0x10715880, 0x1071589c), (0x107158a0, 0x10715a28),
    (0x10715a30, 0x10715b1c), (0x10715b20, 0x10715bc0), (0x10715bc0, 0x10715df4), (0x10715e00, 0x1071612c),
    (0x10716130, 0x107164ec), (0x107164f0, 0x1071689c), (0x107168a0, 0x10716afc), (0x10716b00, 0x10716ea4),
    (0x10716eb0, 0x10717268), (0x10717270, 0x10717620), (0x10717620, 0x10717888), (0x10717890, 0x10717a0c),
    (0x10717a10, 0x10717b1c), (0x10717b20, 0x10717ca8), (0x10717cb0, 0x10717dc4), (0x10717dd0, 0x10718130),
    (0x10718130, 0x107182f0), (0x107182f0, 0x10718354), (0x10718360, 0x10718438), (0x10718440, 0x107184cc),
    (0x107184d0, 0x10718570), (0x10718570, 0x1071864c), (0x10718650, 0x107186f8), (0x10718700, 0x107187cc),
    (0x107187d0, 0x1071889c), (0x107188a0, 0x107189c4), (0x107189d0, 0x10718af0), (0x10718af0, 0x10718b38))
# The leaf delivered with the unit (see the module docstring).
NEIGHBOUR = 0x10718af0
ROUTINE = re.compile(r'DATE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(date_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "date_routines.inc"
# Alignment before a loop in the day count and the day of the month; a branch jumps over each.
UNREACHABLE_NOPS = frozenset({0x1071820c, 0x1071833c})


def routine_lines(path=INCLUDES):
    """The text of date_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (DATE_ROUTINE), with drafts also the DATE_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"date_routines.inc: {entry:x}-{end:x} is not a routine of DATE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A DATE.CPP routine is listed twice")
    return found


DATE_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_date_index(db, path=INCLUDES, complete=True):
    """Require complete production registration and the exact original bodies."""
    registered = rewritten(path)
    if complete and (registered != rewritten(path, drafts=True) or
                     tuple((entry, end) for entry, end, _ in registered) != ROUTINES):
        raise ValueError("DATE.CPP requires all 32 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4)
                                           if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing DATE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"DATE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"date_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def date_hook(address):
    return f"if (try_modern_date(s, 0x{address:x})) return true;"


def require_date_hooks(page, instructions, code):
    """Every owned entry must route before its original first instruction."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = date_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if (code.count(hook) != 1 or hook not in fragment or step not in fragment or
                fragment.index(hook) > fragment.index(step)):
            raise ValueError(f'Missing or misplaced DATE.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of date_routines.inc."""
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
    db = sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
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

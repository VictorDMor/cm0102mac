"""fog_of_war.cpp: what each human manager knows of every person in the world.

24 routines at 0x106fd120..0x106ff550, 2,292 instructions. The unit is named by
its assertion file name (the string pool at TOC -0x4058, 0x10897274, starts with
"fog_of_war.cpp", then "v%s %s.%s %d", "Error", "fog.dat" and "Initialising game
data") and owns the double pool after it (TOC -0x405c, 0x108972b8: 8.5, the two
int-to-double constants and 65535). Only this unit loads those two slots, first at
0x106fd2dc and last at 0x106fef2c. The slot before them (-0x4060, the "Please
Confirm" pool) is loaded by the unit before at 0x106fcbcc, whose two routines at
0x106fcbb0 and 0x106fcee0 are reached through its own vectors (TOC -0x5c8c, -0x5c90,
loaded at 0x106fc82c/0x106fc834); the slot after them (-0x4054) is
new_transfer_rule_screens.cpp's pool, first loaded at 0x106ff560 by its routine at
0x106ff550. Every entry is reached by a direct call (13 from outside the unit, the
rest from inside); no transition vector, jump table or symbol points into the unit.

The whole-game inventory of 2026-11-15 gave the unit 23 routines: the knowledge query
0x106fe280, between two of the unit's routines, was left at the module boundary.
The evidence is checked in tools/test_fog_of_war_audit.py.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106fd120, 0x106ff550)
ROUTINES = (
    (0x106fd120, 0x106fd230), (0x106fd230, 0x106fd2b8), (0x106fd2c0, 0x106fd6c8), (0x106fd6d0, 0x106fd98c),
    (0x106fd990, 0x106fd9e8), (0x106fd9f0, 0x106fda80), (0x106fda80, 0x106fdafc), (0x106fdb00, 0x106fdb30),
    (0x106fdb30, 0x106fdc8c), (0x106fdc90, 0x106fdddc), (0x106fdde0, 0x106fde44), (0x106fde50, 0x106fe000),
    (0x106fe000, 0x106fe128), (0x106fe130, 0x106fe218), (0x106fe220, 0x106fe27c), (0x106fe280, 0x106fe3f0),
    (0x106fe3f0, 0x106fe700), (0x106fe700, 0x106fe7d0), (0x106fe7d0, 0x106fe818), (0x106fe820, 0x106fec6c),
    (0x106fec70, 0x106fef10), (0x106fef10, 0x106ff488), (0x106ff490, 0x106ff4f0), (0x106ff4f0, 0x106ff550))
ROUTINE = re.compile(r'FOG_OF_WAR_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(fog_of_war_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "fog_of_war_routines.inc"
# A word inside a routine that the export leaves out because nothing reaches it: the compiler's
# alignment no-op after the unconditional branch at 0x106fd690, before the loop at 0x106fd698.
UNREACHABLE = frozenset({0x106fd694})


def routine_lines(path=INCLUDES):
    """The text of fog_of_war_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (FOG_OF_WAR_ROUTINE), with drafts also the FOG_OF_WAR_DRAFT ones, which
    only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"fog_of_war_routines.inc: {entry:x}-{end:x} is not a routine of fog_of_war.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A fog_of_war.cpp routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter).
FOG_OF_WAR_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (FOG_OF_WAR_ROUTINE).
COMPLETE = True


def require_fog_of_war_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("fog_of_war.cpp requires all 24 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing fog_of_war.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"fog_of_war.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"fog_of_war_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def fog_of_war_hook(address):
    return f"if (try_modern_fog_of_war(s, 0x{address:x})) return true;"


def require_fog_of_war_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = fog_of_war_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced FOG_OF_WAR hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of fog_of_war_routines.inc."""
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

"""INJURY.CPP: injuries and illnesses, their records, daily course, news and file form.

47 routines at 0x101840e0..0x10190180, 12,259 instructions. The unit is named by
its assertion file name ("INJURY.CPP" in the string pool at TOC -0x5438) and
bounded by its private TOC slots: the linker gave each unit a run of slots in
unit order, and -0x5478..-0x5430 (pools, eight jump tables, three comparator
vectors) and -0x64d4, -0x64d0 (the injury tables) are loaded by this unit alone,
first at 0x101840f0 and last at 0x1018fe08. The slot before the run (-0x547c)
is loaded up to 0x10168a0c only, in the routine that ends where this unit
starts; the slots after it (-0x542c on) from 0x1019019c on. The three routines
after the last slot user are the comparators behind the vectors at TOC -0x5460,
-0x5458 and -0x5454, which only this unit loads. The evidence is checked in
tools/test_injury_unit_audit.py.

The files are named injury_unit because native/injury_* holds the nine older
register-level owners (injury_bootstrap ... injury_reporting, and 16 entries of
actor_attributes), which this unit's C++ replaces.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x101840e0, 0x10190180)
ROUTINES = (
    (0x101840e0, 0x10184308), (0x10184310, 0x101843c4), (0x101843d0, 0x10184528), (0x10184530, 0x10184e58),
    (0x10184e60, 0x10185404), (0x10185410, 0x101864a0), (0x101864a0, 0x10186a18), (0x10186a20, 0x10186cd0),
    (0x10186cd0, 0x10186e0c), (0x10186e10, 0x10186e94), (0x10186ea0, 0x10186fb4), (0x10186fc0, 0x10187450),
    (0x10187450, 0x10187644), (0x10187650, 0x10187680), (0x10187680, 0x101877dc), (0x101877e0, 0x101878d0),
    (0x101878d0, 0x10187a1c), (0x10187a20, 0x10187a38), (0x10187a40, 0x10188488), (0x10188490, 0x101885f0),
    (0x101885f0, 0x10188924), (0x10188930, 0x10188cbc), (0x10188cc0, 0x1018aec4), (0x1018aed0, 0x1018b2b0),
    (0x1018b2b0, 0x1018b6e0), (0x1018b6e0, 0x1018b9ec), (0x1018b9f0, 0x1018bb4c), (0x1018bb50, 0x1018bdd0),
    (0x1018bdd0, 0x1018c180), (0x1018c180, 0x1018c198), (0x1018c1a0, 0x1018c26c), (0x1018c270, 0x1018c2e4),
    (0x1018c2f0, 0x1018c33c), (0x1018c340, 0x1018c448), (0x1018c450, 0x1018c560), (0x1018c560, 0x1018d140),
    (0x1018d140, 0x1018d94c), (0x1018d950, 0x1018daa8), (0x1018dab0, 0x1018dcb0), (0x1018dcb0, 0x1018f33c),
    (0x1018f340, 0x1018f468), (0x1018f470, 0x1018fbd8), (0x1018fbe0, 0x1018fdf0), (0x1018fdf0, 0x1018ff04),
    (0x1018ff10, 0x1018ffe0), (0x1018ffe0, 0x101900b0), (0x101900b0, 0x10190180))
ROUTINE = re.compile(r'INJURY_UNIT_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(injury_unit_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "injury_unit_routines.inc"
# Alignment before a loop; a branch jumps over each, and the export leaves them out.
UNREACHABLE_NOPS = frozenset({0x10184164, 0x1018446c, 0x1018484c, 0x10184e10, 0x10186d24, 0x10186e4c, 0x10188a64,
                              0x10189304, 0x10189324, 0x1018b7c8})


def routine_lines(path=INCLUDES):
    """The text of injury_unit_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (INJURY_UNIT_ROUTINE), with drafts also the INJURY_UNIT_DRAFT ones, which
    only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"injury_unit_routines.inc: {entry:x}-{end:x} is not a routine of INJURY.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An INJURY.CPP routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter), and so
# does an entry with no line yet.
INJURY_UNIT_ENTRIES = tuple(entry for entry, _ in ROUTINES)
# True since delivery: the generator requires all 47 entries as complete production routines.
COMPLETE = True


def require_injury_unit_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("INJURY.CPP requires all 47 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing INJURY.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"INJURY.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"injury_unit_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def injury_unit_hook(address):
    return f"if (try_modern_injury_unit(s, 0x{address:x})) return true;"


def require_injury_unit_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = injury_unit_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced INJURY hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of injury_unit_routines.inc."""
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

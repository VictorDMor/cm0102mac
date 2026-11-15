"""HUMAN_MANAGER.CPP: the human managers' own records.

62 routines at 0x10145af0..0x1014d560, 7,746 instructions. The unit is named by
its assertion file name (the string pool at TOC -0x54a0, 0x107a8328, holds
"HUMAN_MANAGER.CPP" at +0x37) and bounded by its private TOC slots: -0x54a4..-0x5498
(a data word, the pool, two scrambled names for the CD check) are loaded by this unit
alone, first at 0x10145b0c and last at 0x1014d324. The slot before the run (-0x54a8,
a HISTORY.CPP vector) and the one after it (-0x5494, an INDEX.CPP jump table) are
loaded outside the unit only. The two routines before 0x10145af0 (0x10145a30,
0x10145aa0) are HISTORY.CPP's: their vectors sit in HISTORY.CPP's slots -0x54c4 and
-0x54c8. Five entries have no direct call: the record's constructor
0x10145e10 and destructor 0x10145d70, which the array allocation (0x10145af0, 0x1014a300)
passes to the runtime; the constructor 0x10145e70 and destructor 0x10145dd0 of the
record's five tactic slots, which the record's own pair passes on; and the comparator
0x1014d520 of the records' sorted 0x20-byte lists. Each is reached through a transition
vector whose TOC slot (-0x73dc, -0x73e0, -0x73e8, -0x73e4, -0x73f8) only this unit loads.
The evidence is checked in tools/test_human_manager_audit.py.

The whole-game inventory of 2026-11-13 gave the unit 61 routines; the comparator
0x1014d520 was left at the module boundary.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10145af0, 0x1014d560)
ROUTINES = (
    (0x10145af0, 0x10145d64), (0x10145d70, 0x10145dcc), (0x10145dd0, 0x10145e0c), (0x10145e10, 0x10145e6c),
    (0x10145e70, 0x10145ea0), (0x10145ea0, 0x10145fd0), (0x10145fd0, 0x101466b8), (0x101466c0, 0x10146828),
    (0x10146830, 0x10146a20), (0x10146a20, 0x10146b8c), (0x10146b90, 0x10146cfc), (0x10146d00, 0x10146e6c),
    (0x10146e70, 0x10146fdc), (0x10146fe0, 0x1014714c), (0x10147150, 0x1014730c), (0x10147310, 0x101476e0),
    (0x101476e0, 0x10147bbc), (0x10147bc0, 0x10147d18), (0x10147d20, 0x10147e70), (0x10147e70, 0x10147fc8),
    (0x10147fd0, 0x101481d0), (0x101481d0, 0x1014833c), (0x10148340, 0x101484a4), (0x101484b0, 0x10148660),
    (0x10148660, 0x101487c4), (0x101487d0, 0x10148928), (0x10148930, 0x10148a88), (0x10148a90, 0x10148be8),
    (0x10148bf0, 0x10148d48), (0x10148d50, 0x10148eac), (0x10148eb0, 0x10149008), (0x10149010, 0x10149168),
    (0x10149170, 0x101492c8), (0x101492d0, 0x101494a4), (0x101494b0, 0x10149690), (0x10149690, 0x10149790),
    (0x10149790, 0x101499e4), (0x101499f0, 0x10149bd0), (0x10149bd0, 0x10149c8c), (0x10149c90, 0x10149de8),
    (0x10149df0, 0x10149f50), (0x10149f50, 0x1014a1a0), (0x1014a1a0, 0x1014a2f8), (0x1014a300, 0x1014b094),
    (0x1014b0a0, 0x1014b68c), (0x1014b690, 0x1014b808), (0x1014b810, 0x1014bb4c), (0x1014bb50, 0x1014bdbc),
    (0x1014bdc0, 0x1014bf74), (0x1014bf80, 0x1014c0d8), (0x1014c0e0, 0x1014c2dc), (0x1014c2e0, 0x1014c48c),
    (0x1014c490, 0x1014c5e4), (0x1014c5f0, 0x1014c678), (0x1014c680, 0x1014c88c), (0x1014c890, 0x1014caa0),
    (0x1014caa0, 0x1014cce4), (0x1014ccf0, 0x1014cef8), (0x1014cf00, 0x1014d0e8), (0x1014d0f0, 0x1014d2fc),
    (0x1014d300, 0x1014d514), (0x1014d520, 0x1014d560))
ROUTINE = re.compile(r'HUMAN_MANAGER_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(human_manager_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "human_manager_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch, before a loop.
UNREACHABLE = frozenset({0x10145b7c, 0x10149764, 0x1014ae04, 0x1014d4c4})


def routine_lines(path=INCLUDES):
    """The text of human_manager_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (HUMAN_MANAGER_ROUTINE), with drafts also the HUMAN_MANAGER_DRAFT
    ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"human_manager_routines.inc: {entry:x}-{end:x} is not a routine of HUMAN_MANAGER.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A HUMAN_MANAGER.CPP routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter).
HUMAN_MANAGER_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (HUMAN_MANAGER_ROUTINE).
COMPLETE = True


def require_human_manager_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("HUMAN_MANAGER.CPP requires all 62 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing HUMAN_MANAGER.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"HUMAN_MANAGER.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"human_manager_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def human_manager_hook(address):
    return f"if (try_modern_human_manager(s, 0x{address:x})) return true;"


def require_human_manager_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = human_manager_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced HUMAN_MANAGER hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of human_manager_routines.inc."""
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

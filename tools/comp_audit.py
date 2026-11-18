"""COMP.CPP: the base competition class (every league, cup and stage object derives from it).

53 routines at 0x1044fed0..0x10453394, 3,269 instructions. The unit is named by its
assertion file name: its string pool (TOC -0x49c0, 0x1082de80) holds "COMP.CPP" at +0x380.
Thirteen routines load that pool, the first 0x10450030 and the last 0x10452bb0; the 38
routines 0x10450030..0x10452ec4 are the span the 2026-11-18 inventory gave the unit. The
previous unit's last pool user (TOC -0x49e0) ends at 0x1044fec4; COMP_STATS.CPP's first
(TOC -0x4984) starts at 0x104533a0.

The other fifteen are decided on this evidence (tools/test_comp_audit.py checks it):

- The unit's transition vectors. CodeWarrior emits each object file's descriptors as one
  contiguous run. The run 0x108b98ec..0x108b99e4 holds 32 descriptors, between the previous
  unit's last at 0x108b98e4 (0x1044f2b0) and COMP_STATS.CPP's first at 0x108b99ec (0x10460f50);
  they are the only data words that name this span, every one names an entry of it, and they
  include twelve of the fifteen: 0x1044ff60,
  0x1044ffe0, 0x1044fff0, 0x10450000, 0x10450010, 0x10450020 before the pool span and
  0x104530f0, 0x104532c0, 0x10453300, 0x10453370, 0x10453380, 0x10453390 after it.
- The class's vtable 0x108cf608 (TOC -0x619c): 0x1044ff60 (slot 0, the destructor),
  0x1044ffe0, 0x1044fff0, 0x10450000, 0x10450010, 0x10450020, 0x104530f0, 0x10453370,
  0x10453380 and 0x10453390 are virtuals of the base class, beside the pool span's own.
  (0x10450a00 is named only by the table after it at 0x108cf690, which nothing loads; the
  sort comparator 0x10451470 only by TOC -0x61a0, which 0x10450ee0 loads.)
- 0x1044fed0 is the constructor: it stores that vtable (TOC -0x619c) at +0 and the base
  fields. Its TOC slot sits in this unit's run of slots (-0x61a4..-0x619c, between the
  previous unit's -0x61a8 and COMP_STATS.CPP's -0x6198). The destructor 0x1044ff60 loads
  it too. (The third load, 0x1049e838, is a copy constructor the compiler emitted in
  LEAGUE.CPP's object; it stays there.)
- The comparators 0x104532c0 (a float at +4, then the word at +0) and 0x10453300 (the
  halfword column chosen by the byte at TOC -0x61a4, then the word at +0) have no direct
  caller: ARG_PRM.CPP and ARG_SECOND.CPP pass their vectors (TOC -0x6060, -0x6058) to a
  sort. Their descriptors are in this unit's run, and the global -0x61a4 that 0x10453300
  reads has its TOC slot in this unit's run of slots (first loaded by 0x10453300; the
  Argentine units load it later).
- 0x10452ed0 and 0x10453190 have no descriptor and no private TOC slot. They lie between
  routines this unit owns (0x10452bb0 and 0x104530f0; 0x104530f0 and 0x104532c0), and an
  object file's code is contiguous, so they are this unit's. Both take the competition
  object and read its base fields (+4, +8, +0xc, +0x30, +0x40, +0x43, +0x44). The inventory
  had guessed COMP_STATS.CPP for 0x10452ed0 from its callers alone.

Every external edge into the unit is a `bl` of an entry (222 sites); inside, 0x10450a50
calls 0x10450b60 and 0x10450c20, and 0x10451800 calls 0x10451990. 0x10450ee0 passes
0x10451470's vector (TOC -0x61a0) to the sort. No jump table, computed branch or symbol.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1044fed0, 0x10453394)
ROUTINES = (
    (0x1044fed0, 0x1044ff58), (0x1044ff60, 0x1044ffdc), (0x1044ffe0, 0x1044ffe8), (0x1044fff0, 0x1044fff8),
    (0x10450000, 0x10450004), (0x10450010, 0x10450018), (0x10450020, 0x10450028), (0x10450030, 0x10450578),
    (0x10450580, 0x104506fc), (0x10450700, 0x10450704), (0x10450710, 0x104508f4), (0x10450900, 0x104509c0),
    (0x104509c0, 0x104509c8), (0x104509d0, 0x104509d8), (0x104509e0, 0x104509e4), (0x104509f0, 0x104509f8),
    (0x10450a00, 0x10450a08), (0x10450a10, 0x10450a14), (0x10450a20, 0x10450a28), (0x10450a30, 0x10450a34),
    (0x10450a40, 0x10450a48), (0x10450a50, 0x10450b60), (0x10450b60, 0x10450c1c), (0x10450c20, 0x10450da4),
    (0x10450db0, 0x10450ed8), (0x10450ee0, 0x10451470), (0x10451470, 0x10451570), (0x10451570, 0x1045171c),
    (0x10451720, 0x104517f4), (0x10451800, 0x10451990), (0x10451990, 0x10451a2c), (0x10451a30, 0x10451ab4),
    (0x10451ac0, 0x10451c90), (0x10451c90, 0x10451c98), (0x10451ca0, 0x10451cfc), (0x10451d00, 0x10451d08),
    (0x10451d10, 0x10451f48), (0x10451f50, 0x10451f80), (0x10451f80, 0x10452010), (0x10452010, 0x10452284),
    (0x10452290, 0x104527c0), (0x104527c0, 0x10452880), (0x10452880, 0x10452a58), (0x10452a60, 0x10452bac),
    (0x10452bb0, 0x10452ec4), (0x10452ed0, 0x104530e4), (0x104530f0, 0x10453184), (0x10453190, 0x104532b8),
    (0x104532c0, 0x104532f4), (0x10453300, 0x10453364), (0x10453370, 0x10453374), (0x10453380, 0x10453388),
    (0x10453390, 0x10453394))
ROUTINE = re.compile(r'COMP_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(comp_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "comp_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch (or, at 0x10451a94, after an early `blr`).
UNREACHABLE = frozenset({0x1045077c, 0x1045085c, 0x1045089c, 0x10450a7c, 0x10450ab8, 0x10450ce0, 0x1045176c,
                         0x104518ec, 0x10451924, 0x104519ec, 0x10451a94, 0x10451bd0, 0x10451e20, 0x10451ec0,
                         0x10452fd0})


def routine_lines(path=INCLUDES):
    """The text of comp_routines.inc with the block files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (COMP_ROUTINE), with drafts also the COMP_DRAFT ones, which only check
    tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"comp_routines.inc: {entry:x}-{end:x} is not a routine of COMP.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A COMP.CPP routine is listed twice")
    return found


# Every entry carries the hook; an unlisted entry or a draft declines outside a checker (the adapter).
COMP_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (COMP_ROUTINE); False while an entry is still a draft.
COMPLETE = True


def require_comp_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("COMP.CPP requires all 53 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing COMP.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"COMP.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"comp_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def comp_hook(address):
    return f"if (try_modern_comp(s, 0x{address:x})) return true;"


def require_comp_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = comp_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced COMP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of comp_routines*.inc."""
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

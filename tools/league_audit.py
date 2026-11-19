"""LEAGUE.CPP: the base league class (every league table derives from it; it derives from COMP.CPP).

67 routines at 0x1049e4f0..0x104aac14, 12,587 instructions. The unit is named by its assertion
file name: its string pool (TOC -0x470c, 0x1083a804) holds "LEAGUE.CPP" at +0. Twenty-one
instructions load that pool, the first at 0x1049e978 (routine 0x1049e970) and the last at
0x104aa640 (0x104aa620). The 2026-11-18 inventory gave the unit 60 routines, 0x1049e680..0x104aaae4
(graph 2, evidence 21, filled 37); it removed none of them. The previous unit, HOST_COUNTRY.CPP
(pool TOC -0x4738), last loads its pool at 0x1049e3d8; the next unit's first instruction is its
constructor 0x104aac20, which loads that unit's slots -0x4708, -0x4704, -0x4700 and its vtable
slot -0x614c, all first loaded there (that unit names no source file: its pool holds "Insert CD").

The seven entries the inventory left unattributed are decided on this evidence
(tools/test_league_audit.py checks it):

- The unit's transition vectors. CodeWarrior emits each object file's descriptors as one
  contiguous run. The run 0x108b9b94..0x108b9c9c holds 33 descriptors, between HOST_COUNTRY.CPP's
  0x108b9b8c (0x1049e4c0, its comparator: the descriptor's TOC slot -0x6164 is loaded only by
  HOST_COUNTRY's 0x1049d43c and 0x1049d7d0) and the next unit's first, 0x108b9c9c (0x104ac9f0).
  Every one names an entry of this span, and they include 0x1049e600, 0x1049e960, 0x104aaaf0,
  0x104aab00 and 0x104aab20.
- The class's vtable 0x108d796c (TOC -0x6158): 0x1049e600 is slot +0x08 (the destructor) and
  0x1049e960 slot +0x30 (`blr`). The constructor 0x1049e4f0 stores it at +0 (0x1049e508), the
  destructor 0x1049e600 (0x1049e620) and the copy 0x1049e680 (0x1049e70c) too; nothing else
  loads -0x6158.
- The comparators 0x104aab20 (halfword +0xa), 0x104aab00 (halfword +0x1ec of the words at +0)
  and 0x104aaaf0 (the words at +0) have no direct caller: their descriptors are reached through
  TOC slots -0x472c, -0x4728 and -0x4718, which lie in this unit's run of data slots
  (-0x472c..-0x470c, between HOST_COUNTRY's -0x4730 and the next unit's -0x4708) and are loaded
  only by this unit's 0x104aa620 (-0x472c, -0x4728) and 0x104a28d0 (-0x4718), which pass them to
  the sort 0x10768300.
- 0x1049e4f0 is the constructor the derived leagues call (118 sites): it calls the COMP.CPP
  constructor, stores the vtable above and the league's fields. It has no descriptor (it is not
  virtual) and lies between HOST_COUNTRY's last routine 0x1049e4c0 and the destructor 0x1049e600;
  an object file's code is contiguous and the vtable slot -0x6158 is in this unit's run of vtable
  slots (-0x6160..-0x6158, between HOST_COUNTRY's -0x6164 and the next unit's -0x6154).
- 0x104aab30 has no descriptor and loads no TOC slot. It lies after 0x104aab20 and before the next
  unit's constructor 0x104aac20. It builds a fixture of a league from two club names: the date
  (0x107158a0), then the clubs found among the league's 0x44-byte rows at +0xb0 (+0x3e of them):
  the rows 0x1049ea00 indexes and the constructor clears. Its callers (the ITA_SER_* units, 2,216
  sites) pass r3 = a 16-byte row of the league's array at +0xe8, the array the league destructor
  frees. The next unit reads neither field of an object (its +0xb0 uses are r1-relative), and
  this pool holds the fixture's assertion text "home != -1 && away != -1".

Every external edge into the unit is a `bl` of an entry (13,164 sites); inside, 21 entries are
called by other entries of the unit. No jump table, computed branch, tail call or symbol. The
words inside routines that the export leaves out are the compiler's alignment no-ops after an
unconditional branch (UNREACHABLE). 0x1049e820 holds an inline copy of COMP.CPP's copy
constructor (its load of COMP's vtable slot -0x619c at 0x1049e838): it is this unit's code.
No entry of the span belongs to another C++ owner (OWNER_REPLACEMENTS and SEMANTIC_REPLACEMENTS
name none of them on 2026-11-18).
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1049e4f0, 0x104aac14)
ROUTINES = (
    (0x1049e4f0, 0x1049e600), (0x1049e600, 0x1049e678), (0x1049e680, 0x1049e81c), (0x1049e820, 0x1049e958),
    (0x1049e960, 0x1049e964), (0x1049e970, 0x1049e9fc), (0x1049ea00, 0x1049ea10), (0x1049ea10, 0x1049ea9c),
    (0x1049eaa0, 0x1049eb28), (0x1049eb30, 0x1049f030), (0x1049f030, 0x1049f318), (0x1049f320, 0x1049f53c),
    (0x1049f540, 0x1049f6d0), (0x1049f6d0, 0x1049f77c), (0x1049f780, 0x1049f8d8), (0x1049f8e0, 0x1049f998),
    (0x1049f9a0, 0x1049fc74), (0x1049fc80, 0x1049fdb4), (0x1049fdc0, 0x1049ff88), (0x1049ff90, 0x104a00a0),
    (0x104a00a0, 0x104a0aa8), (0x104a0ab0, 0x104a0e34), (0x104a0e40, 0x104a10d0), (0x104a10d0, 0x104a1348),
    (0x104a1350, 0x104a14e8), (0x104a14f0, 0x104a1518), (0x104a1520, 0x104a153c), (0x104a1540, 0x104a1c30),
    (0x104a1c30, 0x104a26a8), (0x104a26b0, 0x104a28c4), (0x104a28d0, 0x104a3344), (0x104a3350, 0x104a38c8),
    (0x104a38d0, 0x104a3924), (0x104a3930, 0x104a3b88), (0x104a3b90, 0x104a44ec), (0x104a44f0, 0x104a47f0),
    (0x104a47f0, 0x104a4864), (0x104a4870, 0x104a4968), (0x104a4970, 0x104a60b4), (0x104a60c0, 0x104a60c4),
    (0x104a60d0, 0x104a60d8), (0x104a60e0, 0x104a6440), (0x104a6440, 0x104a6654), (0x104a6660, 0x104a6754),
    (0x104a6760, 0x104a6850), (0x104a6850, 0x104a8530), (0x104a8530, 0x104a8f34), (0x104a8f40, 0x104a9160),
    (0x104a9160, 0x104a9208), (0x104a9210, 0x104a95f8), (0x104a9600, 0x104a9808), (0x104a9810, 0x104a988c),
    (0x104a9890, 0x104a9a94), (0x104a9aa0, 0x104a9bc0), (0x104a9bc0, 0x104aa0bc), (0x104aa0c0, 0x104aa23c),
    (0x104aa240, 0x104aa33c), (0x104aa340, 0x104aa398), (0x104aa3a0, 0x104aa42c), (0x104aa430, 0x104aa55c),
    (0x104aa560, 0x104aa5b8), (0x104aa5c0, 0x104aa620), (0x104aa620, 0x104aaae4), (0x104aaaf0, 0x104aab00),
    (0x104aab00, 0x104aab18), (0x104aab20, 0x104aab30), (0x104aab30, 0x104aac14))
ROUTINE = re.compile(r'LEAGUE_BASE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(league_base_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "league_base_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch.
UNREACHABLE = frozenset({0x1049ebe4, 0x1049ec40, 0x1049efc0, 0x1049f118, 0x1049f1f0, 0x1049f26c, 0x1049f294,
                         0x1049fb5c, 0x1049fb9c, 0x1049fbbc, 0x1049fee4, 0x104a003c, 0x104a0174, 0x104a03a0,
                         0x104a0a3c, 0x104a0ccc, 0x104a0ddc, 0x104a0f04, 0x104a1f54, 0x104a2490, 0x104a2878,
                         0x104a3c1c, 0x104a4a64, 0x104a4e60, 0x104a5a3c, 0x104a5f34, 0x104a6394, 0x104a63e4,
                         0x104a65fc, 0x104a66f8, 0x104a68bc, 0x104a7d90, 0x104a7f74, 0x104a8064, 0x104a8e74,
                         0x104a8ed8, 0x104a9024, 0x104a926c, 0x104a93bc, 0x104a94c0, 0x104a9afc, 0x104a9d5c,
                         0x104a9d9c, 0x104a9e5c, 0x104a9fc4, 0x104aa004, 0x104aa1ec, 0x104aa254, 0x104aa34c,
                         0x104aa3dc, 0x104aa594, 0x104aa5ec, 0x104aa674, 0x104aa8f4, 0x104aa9b0, 0x104aaa6c})


def routine_lines(path=INCLUDES):
    """The text of league_base_routines.inc with the block files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (LEAGUE_BASE_ROUTINE), with drafts also the LEAGUE_BASE_DRAFT ones, which
    only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"league_base_routines.inc: {entry:x}-{end:x} is not a routine of LEAGUE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A LEAGUE.CPP routine is listed twice")
    return found


# Every entry carries the hook; an unlisted entry or a draft declines outside a checker (the adapter).
LEAGUE_BASE_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (LEAGUE_BASE_ROUTINE); False while an entry is still
# unwritten or a draft.
COMPLETE = False


def require_league_base_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("LEAGUE.CPP requires all 67 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing LEAGUE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"LEAGUE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"league_base_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def league_base_hook(address):
    return f"if (try_modern_league_base(s, 0x{address:x})) return true;"


def require_league_base_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = league_base_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced LEAGUE.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of league_base_routines*.inc."""
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

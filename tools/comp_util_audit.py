"""COMP_UTIL.CPP: competition utility routines (random draws, ids, round codes and names, result texts).

35 routines at 0x10462cd0..0x10477d54, 21,480 instructions. The unit is named by its assertion file
name: its string pool (TOC -0x47e4, 0x1083109c) holds "COMP_UTIL.CPP" at +0 and, after the version
format, the result-news texts ("{}<%s - Club Name 1>{} are through to the ..."). Fourteen routines
load that pool, the first at 0x104635c0 (routine 0x104635b0) and the last at 0x10477b54 (0x10477b40);
their assertions print the source lines 241..10539 (read from the decompiled code) in address
order, so the file's functions follow the code. The 2026-11-18 inventory gave the unit 33 routines,
0x10462cd0..0x10477cc4 (graph 2, evidence 14, filled 17); the audit removes none and adds two,
decided on the evidence below (tools/test_comp_util_audit.py checks it). The previous unit,
COMP_STATS.CPP (pool TOC -0x4984), is proven up to the routine 0x10462a40 (TOC -0x498c, load at
0x10462aa4); the next unit, CONMEBOL_SEEDING.CPP (slots TOC -0x47dc..-0x47d0), starts with 0x10477d60.

- The unit's transition vectors. CodeWarrior emits each object file's descriptors as one contiguous
  run. The run 0x108b9a04..0x108b9a44 holds 8 descriptors (code, TOC), every one naming an entry of
  this span: 0x10477cd0, 0x104777c0, 0x10477270, 0x10464830, 0x104647c0, 0x104647a0, 0x104647e0 and
  0x10463580. The run before it ends with 0x108b99fc (0x10462a30, a four-instruction comparator): its
  TOC slot -0x4990 is loaded only by COMP_STATS code (0x104571c8, 0x10457220), inside that unit's
  pool span, and the routine lies directly before 0x10462a40, so it is COMP_STATS's, not ours. The
  run after it, 0x108b9a44, is CONMEBOL_SEEDING's (slot -0x47dc, loaded only by that unit). Two of the
  eight have no caller and no TOC slot of this unit and are added to the inventory: 0x10463580 (a
  nine-instruction comparator of doubles at +4, between 0x10463000 and 0x104635b0, reached by the
  slot -0x7464 at 0x10131798) and 0x10477cd0 (a 33-instruction comparator, loaded by slot -0x5cfc at
  0x106d1f48, after the last routine 0x10477b40 and before CONMEBOL_SEEDING's 0x10477d60). The others
  are called through slots -0x6190, -0x4980, -0x6188, -0x6134, -0x60c4, -0x6074.
- The unit's TOC slots form one block -0x4980..-0x47e0 (105 slots, every load inside the unit): the
  descriptor slot of 0x10477270, the 101 jump-table slots, a global (-0x47e8, loaded by 0x10464d10),
  the pool (-0x47e4) and the constants (-0x47e0, "65535.0 and the int-to-double bias" at 0x10839d00). Three
  more slots are private (-0x6190, the descriptor of 0x104777c0; the globals -0x376c, -0x3768). The
  neighbours' blocks end at -0x4984 (COMP_STATS) and start at -0x47dc (CONMEBOL_SEEDING).
- Every external edge into the span is a `bl` of an entry (723 sites); inside, 7 entries are called by
  other entries of the unit (0x10463000 also calls itself). No tail call, no branch out of a routine,
  no fallthrough from outside, no symbol. 0x10469bb0 and 0x1046f1f0 hold the unit's 101 jump tables
  (27 and 74 `bctr`, one TOC slot each: -0x4854..-0x47ec and -0x497c..-0x4858): 7,999 data words,
  1,551 and 6,448, name code inside these two routines only, so they are labels, not functions. The words
  0x108e5918 and 0x108e63d4 that also fall in the span are entries of a table of arbitrary constants.
  The words inside routines the export leaves out (0x10463690, 0x10463b04, 0x1047710c) are the
  compiler's alignment no-ops after an unconditional branch (UNREACHABLE).

Not decided, so not an entry: 0x10462b80, 84 instructions, lies between COMP_STATS's last proven
routine and our first, 0x10462cd0. It loads no TOC slot, asserts nothing, has no descriptor and its
seven callers are CLUB_SCREENS, PLAYER_SEARCH, PRINTOUTS and RECORD_UTILS (none in COMP_STATS or
here). It takes a club and a fixture and returns 1..4, calling DATABASE.CPP's 0x100c7d00. The unit
that ends up owning it (COMP_STATS's audit or this one) must pin it; AMBIGUOUS_NEIGHBOUR records it.

Production: the routed blocks are the pilots and the blocks after them, the rest is not. native/comp_util_routines.inc
(with its block files) lists each C++ routine with the digest of the original body it replaces (COMP_UTIL_ROUTINE
routes it, COMP_UTIL_DRAFT only runs it in the check tools); the generator hooks exactly the listed entries
(COMP_UTIL_HOOKED), so every other entry runs its generated body. Routed today: numeric_ids (0x10464010, 0x10464880,
0x104648c0), club_queries (0x104646d0, 0x10464160, 0x10464900), comparators (0x10463580, 0x104647a0, 0x104647c0,
0x104647e0, 0x10464830, 0x10477cd0, 0x104777c0), interpolation (0x10476780), rating_sort (0x10477870, 0x10477b40)
and comp_screens_list (0x10477490), 17 of the 35. A block lands by adding its lines to the .inc; ROUTED in
tools/test_comp_util_audit.py pins the set.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10462cd0, 0x10477d54)
ROUTINES = (
    (0x10462cd0, 0x10462ffc), (0x10463000, 0x10463578), (0x10463580, 0x104635a4), (0x104635b0, 0x10463988),
    (0x10463990, 0x10463a90), (0x10463a90, 0x10463bd8), (0x10463be0, 0x10463dbc), (0x10463dc0, 0x10464008),
    (0x10464010, 0x10464154), (0x10464160, 0x104646c4), (0x104646d0, 0x104647a0), (0x104647a0, 0x104647b8),
    (0x104647c0, 0x104647d8), (0x104647e0, 0x10464830), (0x10464830, 0x10464880), (0x10464880, 0x104648b8),
    (0x104648c0, 0x104648fc), (0x10464900, 0x10464d00), (0x10464d00, 0x10469704), (0x10469710, 0x104698b4),
    (0x104698c0, 0x10469bac), (0x10469bb0, 0x1046f1ec), (0x1046f1f0, 0x10476774), (0x10476780, 0x10476898),
    (0x104768a0, 0x104769d0), (0x104769d0, 0x10476ebc), (0x10476ec0, 0x10476f3c), (0x10476f40, 0x104770bc),
    (0x104770c0, 0x1047726c), (0x10477270, 0x10477484), (0x10477490, 0x104777b8), (0x104777c0, 0x10477870),
    (0x10477870, 0x10477b34), (0x10477b40, 0x10477cc4), (0x10477cd0, 0x10477d54))
# The routine between COMP_STATS.CPP's last proven one and this unit's first, attributed to neither
# (entry, end, instructions, digest; see the docstring).
AMBIGUOUS_NEIGHBOUR = (0x10462b80, 0x10462cd0, 84, "4cb0450decf7fa1e")
ROUTINE = re.compile(r'COMP_UTIL_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(comp_util_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "comp_util_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch.
UNREACHABLE = frozenset({0x10463690, 0x10463b04, 0x1047710c})
# The adapter (compile_recomp.OWNER_REPLACEMENTS names it for each hooked entry).
OWNER = "comp_util_adapter"


def routine_lines(path=INCLUDES):
    """The text of comp_util_routines.inc with the block files it includes (empty while the file
    does not exist: nothing is routed)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (COMP_UTIL_ROUTINE), with
    drafts also the COMP_UTIL_DRAFT ones, which only check tools run. Empty today."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"comp_util_routines.inc: {entry:x}-{end:x} is not a routine of COMP_UTIL.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A COMP_UTIL.CPP routine is listed twice")
    return found


# The entries of the unit's inventory, and those the generator hooks: the ones comp_util_routines*.inc lists (an
# unlisted entry keeps its generated body and carries no hook; a draft declines outside a checker).
COMP_UTIL_ENTRIES = tuple(entry for entry, _ in ROUTINES)
COMP_UTIL_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))


# Whether the generator requires every entry routed (COMP_UTIL_ROUTINE); False while an entry is still
# unwritten or a draft.
COMPLETE = False


def require_comp_util_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("COMP_UTIL.CPP requires all 35 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing COMP_UTIL.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"COMP_UTIL.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"comp_util_routines.inc pins {expected})")


def comp_util_hook(address):
    return f"if (try_modern_comp_util(s, 0x{address:x})) return true;"


def require_comp_util_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = comp_util_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in COMP_UTIL_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected COMP_UTIL.CPP hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced COMP_UTIL.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of comp_util_routines*.inc."""
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
    db = sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
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

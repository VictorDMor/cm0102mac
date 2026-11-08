"""MATCH_MAN.CPP: the match manager (lineups, player ratings, team selection).

Bounds (closed 2026-11-07, tools/test_match_man_audit.py re-derives them):
- Start 0x102065e0, where MATCH_EVENTS.CPP's last routine 0x102064e0 ends. The
  routine there is a lone `blr` whose only reference is a transition vector,
  and the data segment groups each unit's vectors in unit order:
  MATCH_ENG.CPP's, then 0x1020ee00, 0x1020ea00, 0x10206cb0, 0x102065e0, then
  MATCH_PL.CPP's. MATCH_EVENTS.CPP has none, so the empty callback is
  MATCH_MAN.CPP's.
- End 0x1021b010, where MATCH_OFFICIAL.CPP begins with its referee object's
  two constructors (0x1021b010 clears the two words, 0x1021b020 stores the
  match and the referee: the match debugger builds match+0xe030 with it), as
  MATCH_ENG.CPP opens with its constructor 0x101d16a0.
- Every routine that loads a unit TOC slot loads MATCH_MAN.CPP's: the compiler
  allots a unit's slots together, and MATCH_MAN.CPP's are -0x5294..-0x5274
  (-0x5274 is its source-name pool) between MATCH_EVENTS.CPP's pool -0x52a0
  and MATCH_OFFICIAL.CPP's slots -0x5270..-0x5264. Routines with no unit slot
  sit between routines with one, and nothing places another unit there.

ROUTINES are (entry, end, instructions): entries are direct-call and
function-pointer targets, each body is what control flow reaches from its entry
(through the switch table below), and end is the word after its last
instruction. UNREACHABLE are words after an unconditional `b` that no flow
reaches and the export doesn't hold (31 alignment nops and one branch). Three
of the four switch tables follow check_jump_tables.py's pattern; the fourth is
pinned below.

0x1020e8f0 (prepare an actor for a squad member) was an actor_construction
builder; the MATCH_MAN.CPP owner supersedes it (the generator drops the older
hook at that entry).

native/match_man_routines.inc lists the routines rewritten so far, each with a
digest of its original instructions: MATCH_MAN_ROUTINE lines are verified and
generation hooks them (try_modern_match_man); MATCH_MAN_DRAFT lines run only in
check tools.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x102065e0, 0x1021b010)
CONTRACTS = (
    (0x102065e0, 0x102065e4, 1, "f0b063ea75240311"),
    (0x102065f0, 0x10206cac, 430, "9102ccc2e5365ab9"),
    (0x10206cb0, 0x10206d78, 50, "6f520ae915e0b5c9"),
    (0x10206d80, 0x10206e9c, 70, "ab150310b3c7e425"),
    (0x10206ea0, 0x10209d10, 2966, "d540232449263e00"),
    (0x10209d10, 0x1020a288, 350, "349442f2f4704f79"),
    (0x1020a290, 0x1020c5c4, 2253, "6fac37ca578dfb6b"),
    (0x1020c5d0, 0x1020d7cc, 1151, "af676d66e9743887"),
    (0x1020d7d0, 0x1020e8e4, 1093, "768ced4efc9cc64a"),
    (0x1020e8f0, 0x1020e9f8, 66, "192173d5171d1e85"),
    (0x1020ea00, 0x1020edf4, 253, "0b8e004fcde0f14f"),
    (0x1020ee00, 0x1020ef64, 89, "efeffa4cab9c31b9"),
    (0x1020ef70, 0x102102c8, 1233, "375f8643313749e9"),
    (0x102102d0, 0x1021042c, 87, "44823dc24eb7d860"),
    (0x10210430, 0x102104c4, 37, "535de3db3b91526e"),
    (0x102104d0, 0x102107c0, 186, "9f7f74d65c871ba5"),
    (0x102107c0, 0x102107fc, 14, "fcd5116f70b33228"),
    (0x10210800, 0x10210c44, 273, "059a4f97c678bfa7"),
    (0x10210c50, 0x10211e64, 1151, "5d2671f2b2fdedf3"),
    (0x10211e70, 0x1021682c, 4718, "e22320ced486b98a"),
    (0x10216830, 0x10219e34, 3457, "0526cdfb651427b9"),
    (0x10219e40, 0x1021a234, 253, "246c6eead869ae37"),
    (0x1021a240, 0x1021b010, 875, "aae2a23dc38ec0be"),
)
ROUTINES = tuple((entry, end) for entry, end, _, _ in CONTRACTS)
UNREACHABLE = frozenset({
    0x10206bdc, 0x10206db4, 0x10208478, 0x10208490, 0x10208bb8, 0x102091cc, 0x10209490, 0x102095a8,
    0x1020f0c4, 0x1020f1e0, 0x1020f6b8, 0x1020fa24, 0x1020fa3c, 0x102104fc, 0x10210750, 0x102107cc,
    0x10210e5c, 0x102111fc, 0x1021132c, 0x1021142c, 0x10211694, 0x10211a3c, 0x10216674, 0x1021a728,
    0x1021a7f0, 0x1021a978, 0x1021a9d8, 0x1021aa08, 0x1021aae8, 0x1021ad04, 0x1021ad2c, 0x1021afb4,
})
# bctr, TOC slot, table, number of words, SHA256 of the table bytes. The bound
# (cmplwi r0,8 at 0x10210910) is further back than check_jump_tables.py looks.
SWITCH_TABLES = (
    (0x10210944, -0x5284, 0x108cd0fc, 9, "6a7f6cf95e5e435db8238434253561100157ba362b2a6cc65a47aa044385aa45"),
)
# Owned before this module by actor_construction's builder.
SUPERSEDED = {0x1020e8f0: "actor_construction"}
ROUTINE = re.compile(r'MATCH_MAN_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(match_man_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "match_man_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of match_man_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (MATCH_MAN_ROUTINE), with drafts also the MATCH_MAN_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"match_man_routines.inc: {entry:x}-{end:x} is not a routine of MATCH_MAN.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A MATCH_MAN.CPP routine is listed twice")
    return found


MATCH_MAN_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_match_man_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        if [r["address"] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing MATCH_MAN.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"MATCH_MAN.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"match_man_routines.inc pins {expected})")


def match_man_hook(address):
    return f"if (try_modern_match_man(s, 0x{address:x})) return true;"


def require_match_man_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in MATCH_MAN_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = match_man_hook(entry), f"s.step(0x{entry:x});"
        # Up to the next instruction's label (the next word may be padding).
        fragment = re.split(r"\ni_[0-9a-f]+:", code.split(f"i_{entry:x}:", 1)[-1], maxsplit=1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced MATCH_MAN hook at {entry:x}")
        # The owner must be the first hook at its entry and the only owner: an
        # older one (the actor_construction builder at 0x1020e8f0) would run
        # instead of the C++, or instead of the original when the C++ declines.
        if fragment.find("if (try_") != fragment.index(hook) or fragment.count("if (try_modern_") != 1:
            raise ValueError(f"Another hook shares the MATCH_MAN hook's entry {entry:x}")


def main():
    """Print each routine's digest, for new lines of match_man_routines.inc."""
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

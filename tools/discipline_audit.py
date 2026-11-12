"""DISCIPLINE.CPP: bans, suspensions and their history (the discipline manager
at TOC -0x7aa4, its discipline.dat/ban_history.tmp files and its news texts).

Bounds (closed 2026-11-12, tools/test_discipline_audit.py re-derives them):
- Start 0x100de410, where DATABASE.CPP ends (tools/database_audit.py): the
  first routine that loads DISCIPLINE.CPP's source-name pool (TOC -0x5608).
- End 0x100e92d0. The unit's TOC slots are -0x5610..-0x5600: the conversion
  constants at -0x5610 (0x1079fcfc, the end of this unit's literal data, just
  before the next unit's texts at 0x1079fd0c), the comparator vector -0x560c,
  and the pool words -0x5608..-0x5600. 0x100e8780 and 0x100e8d10 load
  -0x5610 and 0x100e8780 calls this unit's 0x100dfb30/0x100e0090/0x100e0700;
  0x100e9290 is the comparator whose only transition vector (0x108b8a0c,
  this unit's vector group between DATABASE.CPP's 0x100cde70 and
  FILE_SCREENS.CPP's 0x100f0ba0) is TOC -0x560c, loaded by 0x100e05c0.
  From 0x100e92d0 on, the code belongs to an unnamed unit (the scout and
  transfer-rumour news: its slots -0x55fc, -0x55f8 point at those texts and
  at the constants before FILE_SCREENS.CPP's name); 0x100e92d0 loads no unit
  slot, is called only by SHORTLIST_MANAGER.CPP and posts a news item, and no
  DISCIPLINE.CPP routine calls it or is called by it.

ROUTINES are (entry, end, instructions): entries are direct-call and
function-pointer targets, each body is what control flow reaches from its
entry and end is the word after its last instruction. There are no switch
tables. UNREACHABLE are words after an unconditional `b` that no flow reaches
(alignment nops).

native/discipline_routines.inc lists the routines rewritten so far, each
with a digest of its original instructions: DISCIPLINE_ROUTINE lines are
verified and generation hooks them (try_modern_discipline);
DISCIPLINE_DRAFT lines run only in check tools.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x100de410, 0x100e92d0)
CONTRACTS = (
    (0x100de410, 0x100de658, 145, "b430ce5f240e237a"),
    (0x100de660, 0x100de754, 61, "dfd237296a306227"),
    (0x100de760, 0x100ded98, 398, "28c7050822e192d1"),
    (0x100deda0, 0x100df25c, 303, "186a133c0bde67f5"),
    (0x100df260, 0x100df2cc, 27, "ad6cf03367fc6df7"),
    (0x100df2d0, 0x100df358, 34, "1c128ce4fe676327"),
    (0x100df360, 0x100df4fc, 102, "31c4402d1a5627c3"),
    (0x100df500, 0x100df6c4, 112, "50d54e4ab0302a31"),
    (0x100df6d0, 0x100df950, 159, "3e7da3469fafa486"),
    (0x100df950, 0x100df9ec, 38, "98c6ae38f6812c27"),
    (0x100df9f0, 0x100dfb28, 77, "404172b9268560cb"),
    (0x100dfb30, 0x100e0090, 339, "afaa28853a1ad78d"),
    (0x100e0090, 0x100e024c, 110, "f12ce4347c816737"),
    (0x100e0250, 0x100e0404, 109, "eae8517c6b7a510a"),
    (0x100e0410, 0x100e05b4, 105, "ecf4db43befdec8b"),
    (0x100e05c0, 0x100e06f8, 76, "7ccc515344391d84"),
    (0x100e0700, 0x100e0b6c, 283, "89ba96a03b175419"),
    (0x100e0b70, 0x100e59b0, 5008, "35d67936db7bcfc1"),
    (0x100e59b0, 0x100e6e8c, 1335, "18bf19256b5978b3"),
    (0x100e6e90, 0x100e703c, 107, "241d98f47f230080"),
    (0x100e7040, 0x100e7080, 16, "bd9502182d83ecdf"),
    (0x100e7080, 0x100e70ac, 11, "76646f10d46f5032"),
    (0x100e70b0, 0x100e7158, 41, "6484d2631bb5fc4f"),
    (0x100e7160, 0x100e719c, 15, "6021f8d0f19e3fe7"),
    (0x100e71a0, 0x100e7208, 26, "1f047cb767f4efb7"),
    (0x100e7210, 0x100e7490, 160, "3f1a5f417f077100"),
    (0x100e7490, 0x100e7780, 188, "6d4ed60ed63310c0"),
    (0x100e7780, 0x100e7918, 102, "aca2d84ab1b1f67f"),
    (0x100e7920, 0x100e7a90, 91, "b6a8958d27fe1225"),
    (0x100e7a90, 0x100e7b3c, 43, "0fc52aa109fac47c"),
    (0x100e7b40, 0x100e7bc8, 34, "7d2f44c1011b095f"),
    (0x100e7bd0, 0x100e7cd0, 64, "b37e059cc4569805"),
    (0x100e7cd0, 0x100e8660, 612, "ae62cbe43db2bed3"),
    (0x100e8660, 0x100e8778, 70, "dfd8bb2aa9e58e2f"),
    (0x100e8780, 0x100e8d10, 355, "74dec42773fa8075"),
    (0x100e8d10, 0x100e9288, 350, "638f68895ae47e62"),
    (0x100e9290, 0x100e92d0, 16, "657a3c0cce24f3d7"),
)
ROUTINES = tuple((entry, end) for entry, end, _, _ in CONTRACTS)
UNREACHABLE = frozenset({
    0x100de4a0, 0x100df3a4, 0x100df5cc, 0x100df83c, 0x100df98c, 0x100dfabc, 0x100dfb64, 0x100dfc38,
    0x100dfc68, 0x100dfce8, 0x100dff18, 0x100e00d4, 0x100e0628, 0x100e06ac, 0x100e70ec, 0x100e7a38,
    0x100e8814,
})
# Other units' routines a rewrite follows inline (entry, end, digest).
INLINED = ()
ROUTINE = re.compile(r'DISCIPLINE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(discipline_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "discipline_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of discipline_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (DISCIPLINE_ROUTINE), with drafts also the DISCIPLINE_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"discipline_routines.inc: {entry:x}-{end:x} is not a routine of DISCIPLINE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A DISCIPLINE.CPP routine is listed twice")
    return found


DISCIPLINE_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_discipline_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    require_inlined(db)
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        if [r["address"] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing DISCIPLINE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"DISCIPLINE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"discipline_routines.inc pins {expected})")


def require_inlined(db):
    """The callees followed inline are still the instructions their copies follow."""
    for entry, end, expected in INLINED:
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"Inlined routine {entry:x} changed in the export (digest {actual}, pinned {expected})")


def discipline_hook(address):
    return f"if (try_modern_discipline(s, 0x{address:x})) return true;"


def require_discipline_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in DISCIPLINE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = discipline_hook(entry), f"s.step(0x{entry:x});"
        # Up to the next instruction's label (the next word may be padding).
        fragment = re.split(r"\ni_[0-9a-f]+:", code.split(f"i_{entry:x}:", 1)[-1], maxsplit=1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced DISCIPLINE hook at {entry:x}")
        # The owner must be the first hook at its entry and the only owner: an
        # older hook would run instead of the C++, or instead of the original
        # when the C++ declines.
        if fragment.find("if (try_") != fragment.index(hook) or fragment.count("if (try_modern_") != 1:
            raise ValueError(f"Another hook shares the DISCIPLINE hook's entry {entry:x}")


def main():
    """Print each routine's digest, for new lines of discipline_routines.inc."""
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

"""CUP.CPP: the base cup class (every knockout cup derives from it; it derives from COMP.CPP).

47 routines at 0x10478f20..0x104866f0, 13,700 instructions. The unit is named by its assertion
file name: its string pool (TOC -0x47b8, 0x10839e0c) holds "CUP.CPP" at +0 and the method names
of its texts ("CUP::map_fixture_tree", "CUP::process_fixture", "CUP::sort_round_teams",
"CUP::check_round_teams_for_nation", "CUP::randomize_round_teams", "CUP::load_from_disk",
"CUP::save_cup_to_disk", "CUP::rank_teams"). Nineteen instructions load that pool, the first at
0x10478fd8 (routine 0x10478fd0) and the last at 0x10485b38 (0x10485b10). The 2026-11-18 inventory
gave the unit 42 routines, 0x10478fd0..0x10486474 (evidence 19, filled 21, graph 2); it removed
none of them. The previous unit, CONMEBOL_SEEDING.CPP (TOC slots -0x47dc..-0x47d0), last loads
its slots at 0x1047868c and ends with its comparator 0x10478ea0; the next unit, FIFA_RANKINGS.CPP
(pool TOC -0x47b4, tools/fifa_rankings_audit.py, already C++), starts at 0x104866f0.

The five entries the inventory left unattributed are decided on this evidence
(tools/test_cup_audit.py checks it):

- The unit's transition vectors. CodeWarrior emits each object file's descriptors as one
  contiguous run. The run 0x108b9a4c..0x108b9b04 holds 23 descriptors, between
  CONMEBOL_SEEDING's 0x108b9a44 (0x10478ea0, its comparator: the descriptor's TOC slot -0x47dc
  is loaded only by CONMEBOL_SEEDING's 0x104782e8 and 0x1047868c) and FIFA_RANKINGS's first,
  0x108b9b04 (0x10487fb0). Every one names an entry of this span, and they include 0x10478f70,
  0x10486050 and 0x10486480.
- The class's vtable 0x108d7724 (TOC -0x6184, the unit's only vtable slot, between the
  previous run's -0x6188 and FIFA_RANKINGS's -0x6180): 0x10478f70 is slot +0x08 (the
  destructor), 0x10486050 slot +0x88 and 0x10486480 slot +0x8c. The constructor 0x10478f20
  stores it at +0 (0x10478f48 after the load at 0x10478f38) and the destructor 0x10478f70
  (0x10478f90) too; nothing else loads -0x6184.
- 0x10478f20 is the constructor the derived cups call (62 sites): it calls the COMP.CPP
  constructor (0x10478f34), stores the vtable above, the cup's kind 1 at +0x43 and clears +0xa8,
  +0xa9. It has no descriptor (it is not virtual) and lies between CONMEBOL_SEEDING's last
  routine 0x10478ea0 and the destructor 0x10478f70; an object file's code is contiguous.
- 0x104864d0 has no descriptor and loads no TOC slot of the unit (only the global -0x7a60). It
  lies after the last vtable body 0x10486480 and before FIFA_RANKINGS's constructor 0x104866f0.
  Its one caller, FIX_MAN.CPP's 0x104895c4, passes r3 = a competition whose kind (+0x43) it has
  just tested to be 1, the value this unit's constructor stores; the routine reads the cup's
  0x78-byte rounds at +0xa0 and the year +0x40 (the fields the derived cups name:
  european_cup_detail.hpp's `rounds`, `year`). No instruction of FIFA_RANKINGS loads or stores
  a word at +0xa0, +0xa4 or +0xac of an object (only of its own stack frame).

Every external edge into the unit is a `bl` of an entry (1,248 sites); inside, 17 entries are
called by other entries of the unit. No jump table, computed branch, tail call or symbol. The
words inside routines that the export leaves out are the compiler's alignment no-ops after an
unconditional branch (UNREACHABLE). The vtable slots of the base class the cup does not override
hold COMP.CPP's bodies. The comparators 0x10485830 and 0x10485870 have no caller here: their
descriptors are in other units' TOC slots (-0x60ec, -0x6128) passed to qsort; 0x10481e70's
slot -0x47c0 is this unit's (0x1047ff7c, 0x10480a24).

No entry of the span belongs to another C++ owner (OWNER_REPLACEMENTS and SEMANTIC_REPLACEMENTS
name none of them on 2026-11-19). One generator hook lies inside the span: the competition
retirement hook `inter_amer_cup_skip_daily` at the entry of the daily routine 0x10482ca0 (vt
+0x1c, compile_recomp.py `competition_retirement_hooks`; the retired Inter-American Cup returns 0
without running). It is emitted before this module's hook at that entry, so it keeps running
first in production; the C++ of 0x10482ca0 must still test it itself, since a checker or a
direct call reaches the C++ without the generated entry (.local/rewrites/cup/brief.md).
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10478f20, 0x104866f0)
ROUTINES = (
    (0x10478f20, 0x10478f6c), (0x10478f70, 0x10478fcc), (0x10478fd0, 0x1047905c), (0x10479060, 0x104790ec),
    (0x104790f0, 0x104790fc), (0x10479100, 0x1047a9d4), (0x1047a9e0, 0x1047b354), (0x1047b360, 0x1047b5a4),
    (0x1047b5b0, 0x1047b7d0), (0x1047b7d0, 0x1047ba48), (0x1047ba50, 0x1047bba8), (0x1047bbb0, 0x1047bdd8),
    (0x1047bde0, 0x1047bfa4), (0x1047bfb0, 0x1047e534), (0x1047e540, 0x1047e978), (0x1047e980, 0x1047ef48),
    (0x1047ef50, 0x1047f39c), (0x1047f3a0, 0x1047fcf8), (0x1047fd00, 0x10480620), (0x10480620, 0x104810ac),
    (0x104810b0, 0x10481ad0), (0x10481ad0, 0x10481e70), (0x10481e70, 0x10481e88), (0x10481e90, 0x10482640),
    (0x10482640, 0x10482c98), (0x10482ca0, 0x10483194), (0x104831a0, 0x10484418), (0x10484420, 0x10484b8c),
    (0x10484b90, 0x10484e08), (0x10484e10, 0x10484fcc), (0x10484fd0, 0x10485334), (0x10485340, 0x10485404),
    (0x10485410, 0x104854e4), (0x104854f0, 0x10485570), (0x10485570, 0x104855fc), (0x10485600, 0x10485830),
    (0x10485830, 0x10485868), (0x10485870, 0x104858a8), (0x104858b0, 0x10485930), (0x10485930, 0x10485ab0),
    (0x10485ab0, 0x10485b0c), (0x10485b10, 0x10485d70), (0x10485d70, 0x10486050), (0x10486050, 0x10486094),
    (0x104860a0, 0x10486474), (0x10486480, 0x104864c4), (0x104864d0, 0x104866f0))
ROUTINE = re.compile(r'CUP_BASE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(cup_base_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "cup_base_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch.
UNREACHABLE = frozenset({0x104791e4, 0x10479254, 0x1047932c, 0x10479700, 0x10479c7c, 0x1047a504, 0x1047a658,
                         0x1047a6a0, 0x1047b8e8, 0x1047badc, 0x1047bb64, 0x1047be04, 0x1047bf64, 0x1047c180,
                         0x1047c79c, 0x1047c7c4, 0x1047cce4, 0x1047cd0c, 0x1047d1fc, 0x1047d224, 0x1047d6f4,
                         0x1047d71c, 0x1047e8e0, 0x1047e9ec, 0x1047ebb8, 0x1047ec7c, 0x1047f208, 0x1047f2b8,
                         0x1047f43c, 0x104807b4, 0x104809cc, 0x10481a0c, 0x10481b44, 0x10482050, 0x10482d34,
                         0x10483214, 0x10484858, 0x10484b30, 0x10485494, 0x10485c44, 0x10485da4})
# The entry whose generated page also carries the competition retirement hook (see the docstring).
RETIREMENT_ENTRY = 0x10482ca0


def routine_lines(path=INCLUDES):
    """The text of cup_base_routines.inc with the block files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (CUP_BASE_ROUTINE), with drafts also the CUP_BASE_DRAFT ones, which
    only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"cup_base_routines.inc: {entry:x}-{end:x} is not a routine of CUP.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A CUP.CPP routine is listed twice")
    return found


# Every entry carries the hook; an unlisted entry or a draft declines outside a checker (the adapter).
CUP_BASE_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (CUP_BASE_ROUTINE); False while an entry is still
# unwritten or a draft.
COMPLETE = True


def require_cup_base_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("CUP.CPP requires all 47 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing CUP.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"CUP.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"cup_base_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def cup_base_hook(address):
    return f"if (try_modern_cup_base(s, 0x{address:x})) return true;"


def require_cup_base_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = cup_base_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced CUP.CPP hook at {entry:x}')
        # The retirement hook, where the generator emits it, runs before the C++ (the docstring).
        retirement = 'if (inter_amer_cup_skip_daily(s)) return true;'
        if entry == RETIREMENT_ENTRY and retirement in fragment and fragment.index(retirement) > fragment.index(hook):
            raise ValueError(f'The retirement hook at {entry:x} must precede the CUP.CPP hook')


def main():
    """Print each routine's digest, for new lines of cup_base_routines*.inc."""
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

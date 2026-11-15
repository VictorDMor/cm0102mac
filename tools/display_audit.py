"""DISPLAY.CPP: the display list, its controls, input and drawing, and its network form.

32 routines at 0x10718b40..0x10725a00, 13,178 instructions. The unit is named by its
assertion file name (the string pool at TOC -0x3f8c, 0x1089a218, starts "DISPLAY.CPP") and
bounded by its TOC slots. In the code run, DATE.CPP's pool (-0x3fa0) comes before and GUIO.CPP's
(-0x3f88) after; this unit's -0x3f9c..-0x3f8c (two words its static initializer 0x107259a0 passes
on, the two jump tables of 0x1071de80, the pool) and -0x35b8..-0x35b0 are loaded by it alone. In the
data run, AREA.CPP's words (-0x5c34, -0x5c30) come before and GUIO.CPP's (-0x5be8 on) after; of this
unit's -0x5c28..-0x5bec, five are loaded by it alone (-0x5c28, -0x5c08, -0x5bfc, and the transition
vectors -0x5bf8/-0x5bf4 of 0x10718d80/0x10718dd0, the elements of the table 0x10718ba0 destroys)
and the rest, the unit's globals, by other units too. DATE.CPP's audit ends at 0x10718b38, so
0x10718b40 (the table's field reset, no TOC load) is this unit's first routine; GUIO.CPP's first
routine is 0x10725a00. 0x1071de80 interprets the screen commands through two jump tables (27 and
11 cases, all inside it). Three entries had older C++ owners in builder style: 0x10718b40 and
0x10718ba0 (aggregate_object) and 0x107259a0 (engine_initialization); this unit takes them over.
The evidence is checked in tools/test_display_audit.py.

The whole-game inventory of 2026-11-15 gave the unit 27 routines and left the five entries
0x10718b40, 0x10718ba0, 0x10718d80, 0x10718dd0 and 0x107259a0 at the module boundary.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from turbo_pages import page_hook
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10718b40, 0x10725a00)
ROUTINES = (
    (0x10718b40, 0x10718ba0), (0x10718ba0, 0x10718d74), (0x10718d80, 0x10718dd0), (0x10718dd0, 0x10718e20),
    (0x10718e20, 0x10718ee4), (0x10718ef0, 0x10718f30), (0x10718f30, 0x10718f60), (0x10718f60, 0x10718fbc),
    (0x10718fc0, 0x10719348), (0x10719350, 0x10719554), (0x10719560, 0x10719740), (0x10719740, 0x107198c0),
    (0x107198c0, 0x1071a0f4), (0x1071a100, 0x1071a1bc), (0x1071a1c0, 0x1071a27c), (0x1071a280, 0x1071aa10),
    (0x1071aa10, 0x1071add0), (0x1071add0, 0x1071afb8), (0x1071afc0, 0x1071b514), (0x1071b520, 0x1071bcf8),
    (0x1071bd00, 0x1071c070), (0x1071c070, 0x1071c188), (0x1071c190, 0x1071c1d0), (0x1071c1d0, 0x1071c2cc),
    (0x1071c2d0, 0x1071de80), (0x1071de80, 0x10723650), (0x10723650, 0x10723844), (0x10723850, 0x10723a24),
    (0x10723a30, 0x10723ca4), (0x10723cb0, 0x10725770), (0x10725770, 0x10725998), (0x107259a0, 0x107259fc))
ROUTINE = re.compile(r'DISPLAY_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(display_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "display_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch, before a loop or a case.
UNREACHABLE = frozenset({0x1071a4a4, 0x1071a504, 0x1071a634, 0x1071b384, 0x1071db00, 0x1071dd2c, 0x1071e458,
                         0x1071ed3c, 0x107236a8, 0x107236dc, 0x1072372c, 0x10723760, 0x107237b0, 0x107237e4,
                         0x10725868})
# Entries an older owner had: TAKEOVERS leave aggregate_object (the display table's constructor and
# destructor) and engine_initialization (the unit's static initializer) for this unit's adapter.
TAKEOVERS = (0x10718b40, 0x10718ba0, 0x107259a0)


def routine_lines(path=INCLUDES):
    """The text of display_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (DISPLAY_ROUTINE), with drafts also the DISPLAY_DRAFT
    ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"display_routines.inc: {entry:x}-{end:x} is not a routine of DISPLAY.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A DISPLAY.CPP routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter).
DISPLAY_ENTRIES = tuple(entry for entry, _ in ROUTINES)


# Whether the generator requires every entry routed (DISPLAY_ROUTINE).
COMPLETE = True


def require_display_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("DISPLAY.CPP requires all 32 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing DISPLAY.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"DISPLAY.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"display_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def display_hook(address):
    return f"if (try_modern_display(s, 0x{address:x})) return true;"


def require_display_hooks(page, instructions, code):
    """Every entry on the page carries its hook once, before the entry's step. Page 0x1071a is a turbo
    page: its hook is wrapped in the counter's semantic boundary and its step is the counter's."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook = page_hook(page, entry, f'try_modern_display(s, 0x{entry:x})')
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced DISPLAY hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of display_routines.inc."""
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

"""ENG_FIRST.CPP: English First Division entry and source contracts."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x105267c0, 0x10529840)
ROUTINES = ((0x105267c0, 0x10526aec), (0x10526af0, 0x10526bf4),
            (0x10526c00, 0x10528c60), (0x10528c60, 0x10528e4c),
            (0x10528e50, 0x10528f1c), (0x10528f20, 0x105291b8),
            (0x105291c0, 0x1052927c), (0x10529280, 0x10529464),
            (0x10529470, 0x105295ec), (0x105295f0, 0x105296c8),
            (0x105296d0, 0x10529834))
ROUTINE = re.compile(r'ENG_FIRST_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(eng_first_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "eng_first_routines.inc"
UNREACHABLE_NOPS = frozenset({0x10526b4c, 0x10528d18, 0x10528f88, 0x10529570})


def routine_lines(path=INCLUDES):
    """The text of eng_first_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (ENG_FIRST_ROUTINE), with drafts also the ENG_FIRST_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"eng_first_routines.inc: {entry:x}-{end:x} is not a routine of ENG_FIRST.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An ENG_FIRST.CPP routine is listed twice")
    return found


ENG_FIRST_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_eng_first_index(db, path=INCLUDES):
    """Require complete production registration and the exact original bodies."""
    registered = rewritten(path)
    if (registered != rewritten(path, drafts=True) or
            tuple((entry, end) for entry, end, _ in registered) != ROUTINES):
        raise ValueError("ENG_FIRST.CPP requires all eleven complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4)
                                           if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing ENG_FIRST.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"ENG_FIRST.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"eng_first_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def eng_first_hook(address):
    return f"if (try_modern_eng_first(s, 0x{address:x})) return true;"


def require_eng_first_hooks(page, instructions, code):
    """Every owned entry must route before its original first instruction."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = eng_first_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if (code.count(hook) != 1 or hook not in fragment or step not in fragment or
                fragment.index(hook) > fragment.index(step)):
            raise ValueError(f'Missing or misplaced ENG_FIRST hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of eng_first_routines.inc."""
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

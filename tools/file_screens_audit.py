"""FILE_SCREENS.CPP: the file browser, the save information screen, the message box.

The readers of the source pool at TOC -0x55d4 bound the module: the first at
0x100edf38, the last at 0x100f0da0. The scouts code before it reads -0x55fc, and
FINANCE.CPP's pools are first read at 0x100f1078. 0x100f1000 and 0x100f1010, right
after the module's last routine, are an empty constructor and a deleting destructor
whose vectors only FINANCE.CPP reads (array constructor at 0x100f8618, destructor at
0x100f874c), so they are FINANCE's. Direct calls and the six vectors at
0x108b8a14-0x108b8a3c close the entries; the raw pointer census finds no other
reference. Two unindexed NOPs after unconditional jumps are alignment.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x100edf20, 0x100f1000)
ROUTINES = ((0x100edf20, 0x100ee214), (0x100ee220, 0x100ee9c8), (0x100ee9d0, 0x100ef7c8), (0x100ef7d0, 0x100ef83c),
            (0x100ef840, 0x100f052c), (0x100f0530, 0x100f0598), (0x100f05a0, 0x100f071c), (0x100f0720, 0x100f08b4),
            (0x100f08c0, 0x100f0ba0), (0x100f0ba0, 0x100f0d70), (0x100f0d70, 0x100f0ff8))
ROUTINE = re.compile(r'FILE_SCREENS_ROUTINE\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "file_screens_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x100f0474, 0x100f0dbc})


def routine_lines(path=INCLUDES):
    return path.read_text() if path.exists() else ""


def rewritten(path=INCLUDES):
    """(entry, end, digest) of each routine whose C++ is hooked."""
    found = sorted((int(entry, 16), int(end, 16), digest) for entry, end, _, digest in ROUTINE.findall(routine_lines(path)))
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"file_screens_routines.inc: {entry:x}-{end:x} is not a routine of FILE_SCREENS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A FILE_SCREENS.CPP routine is listed twice")
    return found


FILE_SCREENS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_file_screens_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("FILE_SCREENS.CPP requires all eleven complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing FILE_SCREENS.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"FILE_SCREENS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"file_screens_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # The filter's pointer glue reads r12, which file_screens_browser.cpp sets as the original does.
    return [line for line in found if line != "r12 read at 10781320 in 10781320, called by the module at 100ee554"]


def file_screens_hook(address):
    return f"if (try_modern_file_screens(s, 0x{address:x})) return true;"


def require_file_screens_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in FILE_SCREENS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = file_screens_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced FILE_SCREENS hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of file_screens_routines.inc."""
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
    db = sqlite3.connect(args.export / "instructions.sqlite")
    if args.volatiles:
        for line in volatile_reads(db):
            print(line)
        return
    wanted = {int(e, 16) for e in args.entries}
    done = {entry for entry, _, _ in rewritten()}
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        stage = " (C++)" if entry in done else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

"""The original's sort family: the entry every caller uses and its three heapsorts.

Three source units by the assertion texts in the pool: the entry 0x10768300 and
the byte sort 0x10768340 carry no name, 0x107684c0 is qsort4x.c ("(member_size
& 3) == 0") and 0x10768660 is qsort8x.c ("(member_size & 7) == 0"). The entry
is the only caller of the three sorts and no data word points at any of them
(tools/test_qsort4x_audit.py), so the four routines are delivered as one unit.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10768300, 0x10768808)
ROUTINES = ((0x10768300, 0x10768340), (0x10768340, 0x107684b8),
            (0x107684c0, 0x10768658), (0x10768660, 0x10768808))
ROUTINE = re.compile(r'QSORT4X_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(qsort4x_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "qsort4x_routines.inc"
# Alignment before each swap loop of the byte and word sorts; a branch jumps over it.
UNREACHABLE_NOPS = frozenset({0x107683ac, 0x1076846c, 0x10768550, 0x10768610})


def routine_lines(path=INCLUDES):
    """The text of qsort4x_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (QSORT4X_ROUTINE), with drafts also the QSORT4X_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"qsort4x_routines.inc: {entry:x}-{end:x} is not a routine of the sort family")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A sort family routine is listed twice")
    return found


QSORT4X_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_qsort4x_index(db, path=INCLUDES):
    """Require complete production registration and the exact original bodies."""
    registered = rewritten(path)
    if (registered != rewritten(path, drafts=True) or
            tuple((entry, end) for entry, end, _ in registered) != ROUTINES):
        raise ValueError("The sort family requires all four complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4)
                                           if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing sort family instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"Sort family routine {entry:x} changed in the export (digest {actual}, "
                             f"qsort4x_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # The comparator call supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def qsort4x_hook(address):
    return f"if (try_modern_qsort4x(s, 0x{address:x})) return true;"


def require_qsort4x_hooks(page, instructions, code):
    """Every owned entry must route before its original first instruction."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = qsort4x_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if (code.count(hook) != 1 or hook not in fragment or step not in fragment or
                fragment.index(hook) > fragment.index(step)):
            raise ValueError(f'Missing or misplaced sort family hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of qsort4x_routines.inc."""
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

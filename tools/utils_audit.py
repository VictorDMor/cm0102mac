"""UTILS.CPP: text, path, file and system helpers.

The first and last readers of the source pool at TOC -0x3e34 bound the module,
with ZIPDIR.CPP's pool reader right after it, in the order of their string pools.
Direct calls, the three jump tables at -0x3e44, -0x3e48 and -0x3e4c and the raw
pointer census close its entries. Five unindexed NOPs after unconditional jumps
are alignment. 0x107474a0 stays routed through try_modern_rng_core, which calls
this module's C++; every other entry has its own hook.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10745e00, 0x107475b0)
ROUTINES = ((0x10745e00, 0x10745fc0), (0x10745fc0, 0x107460b4), (0x107460c0, 0x10746278),
            (0x10746280, 0x10746300), (0x10746300, 0x1074633c), (0x10746340, 0x1074646c),
            (0x10746470, 0x107464d0), (0x107464d0, 0x107465a0), (0x107465a0, 0x107465ec),
            (0x107465f0, 0x10746760), (0x10746760, 0x10746780), (0x10746780, 0x107467a0),
            (0x107467a0, 0x10746a2c), (0x10746a30, 0x10746a70), (0x10746a70, 0x10746e80),
            (0x10746e80, 0x10746f78), (0x10746f80, 0x10747074), (0x10747080, 0x107470ac),
            (0x107470b0, 0x107471cc), (0x107471d0, 0x1074725c), (0x10747260, 0x107473b8),
            (0x107473c0, 0x10747494), (0x107474a0, 0x107475ac))
RNG_INITIALIZER = 0x107474a0
ROUTINE = re.compile(r'UTILS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(utils_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "utils_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x107466f8, 0x10746f04, 0x10746f34, 0x10746fd4, 0x1074740c})


def routine_lines(path=INCLUDES):
    """The text of utils_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (UTILS_ROUTINE), with drafts also the UTILS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"utils_routines.inc: {entry:x}-{end:x} is not a routine of UTILS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A UTILS.CPP routine is listed twice")
    return found


UTILS_ENTRIES = tuple(entry for entry, _, _ in rewritten())
# The entries generated pages hook directly.
UTILS_HOOKED_ENTRIES = tuple(entry for entry in UTILS_ENTRIES if entry != RNG_INITIALIZER)


def require_utils_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("UTILS.CPP requires all twenty-three complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing UTILS.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"UTILS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"utils_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def utils_hook(address):
    return f"if (try_modern_utils(s, 0x{address:x})) return true;"


def require_utils_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in UTILS_HOOKED_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = utils_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced UTILS hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of utils_routines.inc."""
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
    drafted = {entry for entry, _, _ in rewritten(drafts=True)} - done
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        stage = " (C++)" if entry in done else " (draft)" if entry in drafted else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

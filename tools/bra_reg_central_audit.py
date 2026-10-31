"""bra_reg_central.cpp: the Brazilian Central regional championship.

The source pool at -0x40c4, the eleven transition vectors of the -0x5ce8 vtable,
the constructor's one direct caller and the two direct owned calls close the
inventory. Four unindexed NOPs after unconditional jumps are alignment.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106d73f0, 0x106d9170)
ROUTINES = ((0x106d73f0, 0x106d7728), (0x106d7730, 0x106d7824), (0x106d7830, 0x106d835c),
            (0x106d8360, 0x106d84a4), (0x106d84b0, 0x106d8568), (0x106d8570, 0x106d8800),
            (0x106d8800, 0x106d88bc), (0x106d88c0, 0x106d8ae8), (0x106d8af0, 0x106d8c48),
            (0x106d8c50, 0x106d8e2c), (0x106d8e30, 0x106d8e68), (0x106d8e70, 0x106d90b8),
            (0x106d90c0, 0x106d914c), (0x106d9150, 0x106d9170))
ROUTINE = re.compile(r'BRA_REG_CENTRAL_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(bra_reg_central_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "bra_reg_central_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x106d778c, 0x106d85d8, 0x106d8b74, 0x106d8c18})


def routine_lines(path=INCLUDES):
    """The text of bra_reg_central_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (BRA_REG_CENTRAL_ROUTINE), with drafts also the BRA_REG_CENTRAL_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"bra_reg_central_routines.inc: {entry:x}-{end:x} is not a routine of bra_reg_central.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A bra_reg_central.cpp routine is listed twice")
    return found


BRA_REG_CENTRAL_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_bra_reg_central_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("bra_reg_central.cpp requires all fourteen complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing bra_reg_central.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"bra_reg_central.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"bra_reg_central_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def bra_reg_central_hook(address):
    return f"if (try_modern_bra_reg_central(s, 0x{address:x})) return true;"


def require_bra_reg_central_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = bra_reg_central_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced BRA_REG_CENTRAL hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of bra_reg_central_routines.inc."""
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

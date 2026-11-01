"""EUROPEAN_CUP.CPP: the Champions League, between CON_MERC_CUP and EUR_SUPER_CUP.

The source pool at -0x45c8 and floating constants at -0x45cc, the 0x108d94b0
vtable's fourteen transition vectors, the two comparators' vectors, the
constructor's one direct caller and the module's ten direct calls close the
inventory: 23 entries, 5,320 indexed instructions. The eighteen unindexed NOPs
after unconditional jumps are alignment.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x104e9fb0, 0x104ef380)
ROUTINES = ((0x104e9fb0, 0x104ea300), (0x104ea300, 0x104ea44c), (0x104ea450, 0x104ea618), (0x104ea620, 0x104eb288),
            (0x104eb290, 0x104eb764), (0x104eb770, 0x104eb900), (0x104eb900, 0x104eb934), (0x104eb940, 0x104ebc00),
            (0x104ebc00, 0x104ebf30), (0x104ebf30, 0x104ebf80), (0x104ebf80, 0x104ec20c), (0x104ec210, 0x104ec2f8),
            (0x104ec300, 0x104ed110), (0x104ed110, 0x104ed688), (0x104ed690, 0x104ee2d4), (0x104ee2e0, 0x104ee7f0),
            (0x104ee7f0, 0x104eeac4), (0x104eead0, 0x104eee20), (0x104eee20, 0x104ef0fc), (0x104ef100, 0x104ef130),
            (0x104ef130, 0x104ef30c), (0x104ef310, 0x104ef370), (0x104ef370, 0x104ef378))
ROUTINE = re.compile(r'EUROPEAN_CUP_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(european_cup_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "european_cup_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x104ea3a4, 0x104ebcd8, 0x104ec658, 0x104ecb64, 0x104ecbf8, 0x104eccb0, 0x104ed05c,
                              0x104ed810, 0x104ed868, 0x104eda70, 0x104edb20, 0x104edc90, 0x104edcf0, 0x104ede74,
                              0x104ee438, 0x104ee4ac, 0x104ee974, 0x104eebd0})


def routine_lines(path=INCLUDES):
    """The text of european_cup_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (EUROPEAN_CUP_ROUTINE), with drafts also the EUROPEAN_CUP_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"european_cup_routines.inc: {entry:x}-{end:x} is not a routine of european_cup.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A european_cup.cpp routine is listed twice")
    return found


EUROPEAN_CUP_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_european_cup_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("european_cup.cpp requires all twenty-three complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing european_cup.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"european_cup.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"european_cup_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def european_cup_hook(address):
    return f"if (try_modern_european_cup(s, 0x{address:x})) return true;"


def require_european_cup_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = european_cup_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced EUROPEAN_CUP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of european_cup_routines.inc."""
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

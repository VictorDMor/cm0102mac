"""bra_reg_parana.cpp: the Brazilian Paraná regional championship.

Source pool -0x406c, twelve transition vectors in the -0x5c9c vtable,
one incoming constructor call and four direct owned calls close this inventory.
The five omitted NOPs follow unconditional jumps and have no incoming flow.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106f6a80, 0x106f8d30)
ROUTINES = ((0x106f6a80, 0x106f6db8), (0x106f6dc0, 0x106f6eb4), (0x106f6ec0, 0x106f7ba8), (0x106f7bb0, 0x106f7cf4), (0x106f7d00, 0x106f7dc4), (0x106f7dd0, 0x106f8094), (0x106f80a0, 0x106f815c), (0x106f8160, 0x106f83d8), (0x106f83e0, 0x106f8540), (0x106f8540, 0x106f871c), (0x106f8720, 0x106f8814), (0x106f8820, 0x106f89cc), (0x106f89d0, 0x106f8a5c), (0x106f8a60, 0x106f8b50), (0x106f8b50, 0x106f8d08), (0x106f8d10, 0x106f8d30))
ROUTINE = re.compile(r'BRA_REG_PARANA_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(bra_reg_parana_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "bra_reg_parana_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x106f6e1c, 0x106f7e38, 0x106f8510, 0x106f8ac0, 0x106f8bf8})

# External shared competition, calendar, news/history, allocation, assertion and
# virtual-call boundaries. None belongs to the closed module interval.
EXTERNAL_DEPENDENCIES = (
    0x100f71f0, 0x10103510, 0x102d46c0, 0x10381b70, 0x10382e50, 0x10384500,
    0x103846e0, 0x10450030, 0x10452010, 0x104533a0, 0x104536c0, 0x104564a0,
    0x10457490, 0x10459b50, 0x10476780, 0x10477b40, 0x1047ef50, 0x104854f0,
    0x10485570, 0x1049e4f0, 0x1049e600, 0x1049eb30, 0x104a00a0, 0x104a1350,
    0x104a3930, 0x104a60e0, 0x104a6660, 0x104a9810, 0x104a9890, 0x104aa430,
    0x104bc3c0, 0x10517310, 0x1056dc60, 0x10727890, 0x10739890, 0x1074b830,
    0x1074bab0, 0x10768810, 0x10769b60, 0x10773520, 0x107735a0, 0x10775130,
    0x10775170, 0x1077b720, 0x10781320,
)


def routine_lines(path=INCLUDES):
    """The text of bra_reg_parana_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (BRA_REG_PARANA_ROUTINE), with drafts also the BRA_REG_PARANA_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"bra_reg_parana_routines.inc: {entry:x}-{end:x} is not a routine of bra_reg_parana.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A bra_reg_parana.cpp routine is listed twice")
    return found


BRA_REG_PARANA_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_bra_reg_parana_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("bra_reg_parana.cpp requires all sixteen complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing bra_reg_parana.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"bra_reg_parana.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"bra_reg_parana_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def bra_reg_parana_hook(address):
    return f"if (try_modern_bra_reg_parana(s, 0x{address:x})) return true;"


def require_bra_reg_parana_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = bra_reg_parana_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced BRA_REG_PARANA hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of bra_reg_parana_routines.inc."""
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

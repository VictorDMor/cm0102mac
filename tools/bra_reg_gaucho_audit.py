"""Entry and production contracts for the Brazilian Gaúcho championship."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x106f0dc0, 0x106f2ae0)
ROUTINES = (
    (0x106f0dc0, 0x106f10f8), (0x106f1100, 0x106f11f4), (0x106f1200, 0x106f1d54),
    (0x106f1d60, 0x106f1ea4), (0x106f1eb0, 0x106f1f7c), (0x106f1f80, 0x106f2210),
    (0x106f2210, 0x106f22cc), (0x106f22d0, 0x106f24c4), (0x106f24d0, 0x106f2628),
    (0x106f2630, 0x106f280c), (0x106f2810, 0x106f2848), (0x106f2850, 0x106f2a2c),
    (0x106f2a30, 0x106f2abc), (0x106f2ac0, 0x106f2ae0),
)
ROUTINE = re.compile(r'BRA_REG_GAUCHO_ROUTINE\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "bra_reg_gaucho_routines.inc"
# Each follows an unconditional jump; no export flow or transition vector reaches it.
UNREACHABLE_NOPS = frozenset({0x106f115c, 0x106f1fe8, 0x106f2554, 0x106f25f8})

# External shared competition, calendar, news/history, allocation, assertion and
# virtual-call boundaries. None belongs to the closed module interval.
EXTERNAL_DEPENDENCIES = (
    0x102d46c0, 0x10381b70, 0x10382e50, 0x10384500, 0x103846e0, 0x10450030,
    0x104533a0, 0x104536c0, 0x104564a0, 0x10457490, 0x10459b50, 0x10476780,
    0x10477b40, 0x1047ef50, 0x104854f0, 0x10485570, 0x1049e4f0, 0x1049e600,
    0x1049eb30, 0x104a00a0, 0x104a60e0, 0x104a6660, 0x104a9810, 0x104a9890,
    0x104aa430, 0x104bc3c0, 0x10517310, 0x10727890, 0x10739890, 0x1074b830,
    0x1074bab0, 0x10768810, 0x10769b60, 0x10773520, 0x107735a0, 0x10775130,
    0x10775170, 0x1077b720, 0x10781320,
)


def rewritten(path=INCLUDES):
    """Return the registered (entry, end, digest) triples."""
    found = sorted((int(entry, 16), int(end, 16), value) for entry, end, _, value in
                   ROUTINE.findall(path.read_text()))
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"bra_reg_gaucho_routines.inc: {entry:x}-{end:x} is not a routine of bra_reg_gaucho.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A bra_reg_gaucho.cpp routine is listed twice")
    return found


BRA_REG_GAUCHO_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_bra_reg_gaucho_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("bra_reg_gaucho.cpp requires all fourteen complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing bra_reg_gaucho.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"bra_reg_gaucho.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"bra_reg_gaucho_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for entry, _, flags, _ in
               ROUTINE.findall(INCLUDES.read_text())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def bra_reg_gaucho_hook(address):
    return f"if (try_modern_bra_reg_gaucho(s, 0x{address:x})) return true;"


def require_bra_reg_gaucho_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = bra_reg_gaucho_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced BRA_REG_GAUCHO hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of bra_reg_gaucho_routines.inc."""
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
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        stage = " (C++)" if entry in done else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

"""SIPRINT.CPP: printer lifetime, page-format resources and text pagination."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1074c960, 0x1074d37c)
ROUTINES = ((0x1074c960, 0x1074c99c), (0x1074c9a0, 0x1074ca50),
            (0x1074ca50, 0x1074ca54), (0x1074ca60, 0x1074cb44),
            (0x1074cb50, 0x1074cc18), (0x1074cc20, 0x1074cc2c),
            (0x1074cc30, 0x1074cd8c), (0x1074cd90, 0x1074d33c),
            (0x1074d340, 0x1074d37c))
ROUTINE = re.compile(r'SIPRINT_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(siprint_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "siprint_routines.inc"
# The unconditional jump at d16c skips this compiler alignment NOP. No export
# flow or transition vector reaches it; see the module boundary audit.
UNREACHABLE_NOPS = frozenset({0x1074d170})


def routine_lines(path=INCLUDES):
    """The text of siprint_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (SIPRINT_ROUTINE), with drafts also the SIPRINT_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"siprint_routines.inc: {entry:x}-{end:x} is not a routine of SIPRINT.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A SIPRINT.CPP routine is listed twice")
    return found


SIPRINT_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_siprint_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("SIPRINT.CPP requires all nine complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing SIPRINT.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"SIPRINT.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"siprint_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def siprint_hook(address):
    return f"if (try_modern_siprint(s, 0x{address:x})) return true;"


def require_siprint_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = siprint_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced SIPRINT hook at {entry:x}')
        if entry == 0x1074d340 and 'try_modern_engine_initialization' in fragment:
            raise ValueError('SIPRINT initializer still routes through its superseded builder')


def main():
    """Print each routine's digest, for new lines of siprint_routines.inc."""
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

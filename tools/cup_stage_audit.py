"""CUP_STAGE.CPP: cup-stage lifetime, persistence, entrant records and round draws."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x104bc3c0, 0x104bed58)
CONTRACTS = (
    (0x104bc3c0, 0x104bc748, "1cc2c1fc657c994e"),
    (0x104bc750, 0x104bc9a8, "2a73a24301a6726c"),
    (0x104bc9b0, 0x104bca88, "c6efef90686d646a"),
    (0x104bca90, 0x104bcb20, "307ffd7eddca4592"),
    (0x104bcb20, 0x104bd550, "4b7b80f93a8ee8f0"),
    (0x104bd550, 0x104bd8b8, "8d4fa297f4bfc618"),
    (0x104bd8c0, 0x104be7dc, "789abc96f9ee6717"),
    (0x104be7e0, 0x104bed58, "823d8b389f5c20e2"),
)
ROUTINES = tuple((entry, end) for entry, end, _ in CONTRACTS)
ROUTINE = re.compile(r'CUP_STAGE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(cup_stage_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "cup_stage_routines.inc"
# Each skipped word is a compiler alignment NOP after an unconditional branch.
# No indexed flow or transition vector targets these words.
UNREACHABLE_NOPS = frozenset({0x104bd7c4, 0x104bd9fc, 0x104bda34, 0x104bdfc8, 0x104be2c0, 0x104be48c})


def routine_lines(path=INCLUDES):
    """The text of cup_stage_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (CUP_STAGE_ROUTINE), with drafts also the CUP_STAGE_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"cup_stage_routines.inc: {entry:x}-{end:x} is not a routine of CUP_STAGE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A CUP_STAGE.CPP routine is listed twice")
    return found


CUP_STAGE_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_cup_stage_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    if tuple(rewritten(path)) != CONTRACTS:
        raise ValueError("CUP_STAGE.CPP requires all eight complete production entries")
    for entry, end, expected in CONTRACTS:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing CUP_STAGE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"CUP_STAGE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"cup_stage_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def cup_stage_hook(address):
    return f"if (try_modern_cup_stage(s, 0x{address:x})) return true;"


def require_cup_stage_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CUP_STAGE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = cup_stage_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced CUP_STAGE hook at {entry:x}')



def main():
    """Print each routine's digest, for new lines of cup_stage_routines.inc."""
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

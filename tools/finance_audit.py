"""FINANCE.CPP: club finances.

58 routines at 0x100f1000..0x10103600, 18,706 instructions. The unit is named by
its assertion file name (the string pool at TOC -0x5598 starts "FINANCE.CPP")
and bounded by its private TOC slots: the linker gave each unit a run of slots
in unit order, and -0x55d0..-0x5598 (three pools, two scrambled names, eight data
tables, two jump tables) are loaded by this unit alone, first at 0x100f1078 and last at
0x10103518, in the unit's last routine. The slot before the run (-0x55d4,
FILE_SCREENS.CPP's pool) is loaded up to 0x100f0da0 only, and FILE_SCREENS.CPP's
audit ends at 0x100f1000; the slot after it (-0x5594) at 0x10106560 only, past
the unit. The two routines before the first pool user, 0x100f1000 and
0x100f1010, are the constructor and destructor of the array element that
0x100f84e0 allocates and 0x100f8720 releases: only those two load their
transition vectors (TOC -0x74a0 and -0x74a4), and nothing calls them directly.
The evidence is checked in tools/test_finance_audit.py.

The whole-game inventory of 2026-11-13 gave the unit 56 routines; the two
vector routines were left at the module boundary.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x100f1000, 0x10103600)
ROUTINES = (
    (0x100f1000, 0x100f1004), (0x100f1010, 0x100f104c), (0x100f1050, 0x100f20fc), (0x100f2100, 0x100f2678),
    (0x100f2680, 0x100f478c), (0x100f4790, 0x100f4f14), (0x100f4f20, 0x100f4f80), (0x100f4f80, 0x100f55a8),
    (0x100f55b0, 0x100f5730), (0x100f5730, 0x100f5768), (0x100f5770, 0x100f7050), (0x100f7050, 0x100f709c),
    (0x100f70a0, 0x100f70d8), (0x100f70e0, 0x100f712c), (0x100f7130, 0x100f7168), (0x100f7170, 0x100f71a8),
    (0x100f71b0, 0x100f71e8), (0x100f71f0, 0x100f7228), (0x100f7230, 0x100f7268), (0x100f7270, 0x100f7ce4),
    (0x100f7cf0, 0x100f84d4), (0x100f84e0, 0x100f8714), (0x100f8720, 0x100f8780), (0x100f8780, 0x100f95d8),
    (0x100f95e0, 0x100f9afc), (0x100f9b00, 0x100f9c44), (0x100f9c50, 0x100f9d80), (0x100f9d80, 0x100f9f3c),
    (0x100f9f40, 0x100fa1e8), (0x100fa1f0, 0x100fa6d8), (0x100fa6e0, 0x100fcfc4), (0x100fcfd0, 0x100fd194),
    (0x100fd1a0, 0x100fd210), (0x100fd210, 0x100fe638), (0x100fe640, 0x100fefa0), (0x100fefa0, 0x100ffeb4),
    (0x100ffec0, 0x10100694), (0x101006a0, 0x101013f8), (0x10101400, 0x10101fe8), (0x10101ff0, 0x101020b8),
    (0x101020c0, 0x10102590), (0x10102590, 0x10102654), (0x10102660, 0x10102818), (0x10102820, 0x10102908),
    (0x10102910, 0x10102a6c), (0x10102a70, 0x10102b44), (0x10102b50, 0x10102c90), (0x10102c90, 0x10102d1c),
    (0x10102d20, 0x10102e00), (0x10102e00, 0x10102e94), (0x10102ea0, 0x101030c0), (0x101030c0, 0x10103178),
    (0x10103180, 0x10103268), (0x10103270, 0x101032d8), (0x101032e0, 0x10103374), (0x10103380, 0x10103420),
    (0x10103420, 0x1010350c), (0x10103510, 0x101035f4))
ROUTINE = re.compile(r'FINANCE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(finance_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "finance_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-ops before a loop (a branch jumps over each) and five branches that follow an
# unconditional branch.
UNREACHABLE_NOPS = frozenset({0x100f5900, 0x100f59ec, 0x100fa828, 0x100fa968, 0x100fa9d0, 0x100fabe8, 0x100fcd34,
                              0x101015f4})
UNREACHABLE_BRANCHES = frozenset({0x100f4d80, 0x100f7730, 0x100f9140, 0x100f98cc, 0x10102998})
UNREACHABLE = UNREACHABLE_NOPS | UNREACHABLE_BRANCHES


def routine_lines(path=INCLUDES):
    """The text of finance_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (FINANCE_ROUTINE), with drafts also the FINANCE_DRAFT
    ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"finance_routines.inc: {entry:x}-{end:x} is not a routine of FINANCE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A FINANCE.CPP routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter).
FINANCE_ENTRIES = tuple(entry for entry, _ in ROUTINES)


def require_finance_index(db, path=INCLUDES, complete=True):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("FINANCE.CPP requires all 58 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing FINANCE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"FINANCE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"finance_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def finance_hook(address):
    return f"if (try_modern_finance(s, 0x{address:x})) return true;"


def require_finance_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = finance_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced FINANCE hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of finance_routines.inc."""
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

"""NEWS_SCREENS.CPP: the news screen and the send-message dialog.

The source pool at -0x50f0 and the jump table at -0x50f4 are loaded only inside this range, and
the three callback vectors at -0x50e4..-0x50ec and the dialog's at -0x50f8/-0x50fc point into it.
NEWS.CPP ends at 0x102ce710 (its pool -0x5100 last loaded at 0x102ce32c); the next routine,
0x102d0a50, loads OFFICIALS_MANAGER.CPP's strings (-0x50cc "Insert CD").
Direct calls and six transition vectors close the inventory: 8 entries, 2,249 indexed instructions.
The word at 0x102cf4cc follows an unconditional jump and nothing reaches it (the compiler's nop).
The jump table's 57 labels are shared code of one routine, not entries.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x102ce710, 0x102d0a50)
ROUTINES = ((0x102ce710, 0x102cea2c), (0x102cea30, 0x102cfcd0), (0x102cfcd0, 0x102d022c), (0x102d0230, 0x102d02a4),
            (0x102d02b0, 0x102d036c), (0x102d0370, 0x102d0500), (0x102d0500, 0x102d07b0), (0x102d07b0, 0x102d0a50))
UNREACHABLE = frozenset({0x102cf4cc})
ROUTINE = re.compile(r'NEWS_SCREENS_ROUTINE\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "news_screens_routines.inc"
NEWS_SCREENS_ENTRIES = tuple(entry for entry, _ in ROUTINES)


def rewritten(path=INCLUDES):
    """(entry, end, digest) of each routine listed in news_screens_routines.inc."""
    if not path.exists():
        return []
    found = sorted((int(entry, 16), int(end, 16), fingerprint)
                   for entry, end, _, fingerprint in ROUTINE.findall(path.read_text()))
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"news_screens_routines.inc: {entry:x}-{end:x} is not a routine of NEWS_SCREENS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A NEWS_SCREENS.CPP routine is listed twice")
    return found


def require_news_screens_index(db, path=INCLUDES):
    """Every routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("NEWS_SCREENS.CPP requires all eight entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing NEWS_SCREENS.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"NEWS_SCREENS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"news_screens_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for entry, _, flags, _ in ROUTINE.findall(INCLUDES.read_text())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def news_screens_hook(address):
    return f"if (try_modern_news_screens(s, 0x{address:x})) return true;"


def require_news_screens_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = news_screens_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced NEWS_SCREENS.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for the routine list."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
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
    for entry, end in ROUTINES:
        records = routine_records(db, entry, end)
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}")


if __name__ == "__main__":
    main()

"""GAME_CONFIG.CPP: the preferences record, its file and what applying it switches."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1013f030, 0x10140724)
ROUTINES = ((0x1013f030, 0x1013f404), (0x1013f410, 0x1013f4c4), (0x1013f4d0, 0x1013f564),
            (0x1013f570, 0x1013f67c), (0x1013f680, 0x1013fb58), (0x1013fb60, 0x1013fc44),
            (0x1013fc50, 0x1013fcfc), (0x1013fd00, 0x1013fdf4), (0x1013fe00, 0x1013feac),
            (0x1013feb0, 0x101406d4), (0x101406e0, 0x101406e8), (0x101406f0, 0x10140724))
ROUTINE = re.compile(r'GAME_CONFIG_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(game_config_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "game_config_routines.inc"

def routine_lines(path=INCLUDES):
    """The text of game_config_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (GAME_CONFIG_ROUTINE), with drafts also the GAME_CONFIG_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"game_config_routines.inc: {entry:x}-{end:x} is not a routine of GAME_CONFIG.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A GAME_CONFIG.CPP routine is listed twice")
    return found


GAME_CONFIG_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_game_config_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("GAME_CONFIG.CPP requires all twelve complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != list(range(entry, end, 4)):
            raise ValueError(f"Missing GAME_CONFIG.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"GAME_CONFIG.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"game_config_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in
               ROUTINE.findall(routine_lines())}
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    return found


def game_config_hook(address):
    return f"if (try_modern_game_config(s, 0x{address:x})) return true;"


def require_game_config_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = game_config_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced GAME_CONFIG hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of game_config_routines.inc."""
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

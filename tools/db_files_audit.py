"""DB_FILES.CPP: the database tables on disk (loaders, savers, Windows and version 1 readers)."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x105d71b0, 0x105e0240)
ROUTINE = re.compile(r'DB_FILES_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "db_files_routines.inc"
ROUTINES = (
    (0x105d71b0, 0x105d7200), (0x105d7200, 0x105d7250), (0x105d7250, 0x105d7530), (0x105d7530, 0x105d76e0),
    (0x105d76e0, 0x105d7750), (0x105d7750, 0x105d7a30), (0x105d7a30, 0x105d8560), (0x105d8560, 0x105d85d0),
    (0x105d85d0, 0x105d88b0), (0x105d88b0, 0x105d8ad0), (0x105d8ad0, 0x105d8b40), (0x105d8b40, 0x105d8e20),
    (0x105d8e20, 0x105d8ef0), (0x105d8ef0, 0x105d8f60), (0x105d8f60, 0x105d9290), (0x105d9290, 0x105d9710),
    (0x105d9710, 0x105da670), (0x105da670, 0x105da6e0), (0x105da6e0, 0x105da9c0), (0x105da9c0, 0x105dac10),
    (0x105dac10, 0x105dac80), (0x105dac80, 0x105daf60), (0x105daf60, 0x105db230), (0x105db230, 0x105db530),
    (0x105db530, 0x105db6c0), (0x105db6c0, 0x105dc160), (0x105dc160, 0x105dc1d0), (0x105dc1d0, 0x105dc4d0),
    (0x105dc4d0, 0x105dc710), (0x105dc710, 0x105dd910), (0x105dd910, 0x105dd980), (0x105dd980, 0x105ddc60),
    (0x105ddc60, 0x105ddd20), (0x105ddd20, 0x105ddd90), (0x105ddd90, 0x105de070), (0x105de070, 0x105de0e0),
    (0x105de0e0, 0x105de150), (0x105de150, 0x105de450), (0x105de450, 0x105de550), (0x105de550, 0x105de9b0),
    (0x105de9b0, 0x105dea20), (0x105dea20, 0x105ded00), (0x105ded00, 0x105ded70), (0x105ded70, 0x105dede0),
    (0x105dede0, 0x105df0c0), (0x105df0c0, 0x105df1a0), (0x105df1a0, 0x105df210), (0x105df210, 0x105df4f0),
    (0x105df4f0, 0x105df5c0), (0x105df5c0, 0x105df630), (0x105df630, 0x105df910), (0x105df910, 0x105dfa10),
    (0x105dfa10, 0x105dfa80), (0x105dfa80, 0x105dfd60), (0x105dfd60, 0x105dfdf0), (0x105dfdf0, 0x105dfe60),
    (0x105dfe60, 0x105e0140), (0x105e0140, 0x105e01d0), (0x105e01d0, 0x105e0240),
)


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (DB_FILES_ROUTINE), with drafts also the DB_FILES_DRAFT ones check tools run."""
    if not path.exists():
        return []
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(path.read_text()) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"db_files_routines.inc: {entry:x}-{end:x} is not a routine of DB_FILES.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A DB_FILES.CPP routine is listed twice")
    return found


DB_FILES_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_db_files_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path)
    if tuple((entry, end) for entry, end, _ in registered) != ROUTINES:
        raise ValueError("DB_FILES.CPP requires all 59 complete production entries")
    for entry, end, expected in registered:
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"DB_FILES.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"db_files_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(INCLUDES.read_text())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def db_files_hook(address):
    return f"if (try_modern_db_files(s, 0x{address:x})) return true;"


def require_db_files_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = db_files_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced DB_FILES hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of db_files_routines.inc."""
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
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}" + (" (C++)" if entry in done else ""))


if __name__ == "__main__":
    main()

"""AWOL.CPP original entry contracts and production ownership."""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x100031b0, 0x10009ef0)
ROUTINES = (
    (0x100031b0, 0x100032f8),
    (0x10003300, 0x100033b8),
    (0x100033c0, 0x100036dc),
    (0x100036e0, 0x10003dc0),
    (0x10003dc0, 0x10003ec0),
    (0x10003ec0, 0x10003fa0),
    (0x10003fa0, 0x10004068),
    (0x10004070, 0x10006074),
    (0x10006080, 0x100065f8),
    (0x10006600, 0x100066d0),
    (0x100066d0, 0x100067d0),
    (0x100067d0, 0x10006828),
    (0x10006830, 0x10006900),
    (0x10006900, 0x10007100),
    (0x10007100, 0x100072e0),
    (0x100072e0, 0x10007a4c),
    (0x10007a50, 0x10008770),
    (0x10008770, 0x10008b58),
    (0x10008b60, 0x1000909c),
    (0x100090a0, 0x10009b54),
    (0x10009b60, 0x10009ee8),
)
ROUTINE = re.compile(r'AWOL_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "awol_routines.inc"


def routine_lines(path=INCLUDES):
    return path.read_text() if path.exists() else ""


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (AWOL_ROUTINE), with drafts also the AWOL_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"awol_routines.inc: {entry:x}-{end:x} is not a routine of AWOL.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An AWOL.CPP routine is listed twice")
    return found


AWOL_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_awol_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"AWOL.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"awol_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def awol_hook(address):
    return f"if (try_modern_awol(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of awol_routines.inc."""
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

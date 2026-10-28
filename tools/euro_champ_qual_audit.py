"""EURO_CHAMP_QUAL.CPP: 31 entries between EURO_CHAMP and GOLDCUP.

Source pool -0x4590, vtable -0x606c, twenty transition vectors and all direct
calls close the entry set. Two leaf routines have no LR prologue. Whole-export
flows and bytewise initialized-segment scans distinguish data from entries;
there are no computed jumps/calls or interior resumes. Private detailed evidence
lives under .local/rewrites/euro_champ_qual/.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x104f9fa0, 0x105026c0)
ROUTINES = (
    (0x104f9fa0, 0x104fa2a0),
    (0x104fa2a0, 0x104fa3d0),
    (0x104fa3d0, 0x104fa980),
    (0x104fa980, 0x104fab00),
    (0x104fab00, 0x104fab30),
    (0x104fab30, 0x104faf40),
    (0x104faf40, 0x104fb050),
    (0x104fb050, 0x104fbe40),
    (0x104fbe40, 0x104fc3c0),
    (0x104fc3c0, 0x104fc4b0),
    (0x104fc4b0, 0x104fc5d0),
    (0x104fc5d0, 0x104fc5e0),
    (0x104fc5e0, 0x104fc770),
    (0x104fc770, 0x104fc840),
    (0x104fc840, 0x104fcbc0),
    (0x104fcbc0, 0x104fd320),
    (0x104fd320, 0x104fd680),
    (0x104fd680, 0x104fdae0),
    (0x104fdae0, 0x104fdfc0),
    (0x104fdfc0, 0x104fe040),
    (0x104fe040, 0x104fe0b0),
    (0x104fe0b0, 0x104fe1d0),
    (0x104fe1d0, 0x104fe5a0),
    (0x104fe5a0, 0x104fe980),
    (0x104fe980, 0x104fed60),
    (0x104fed60, 0x104fef80),
    (0x104fef80, 0x104fefe0),
    (0x104fefe0, 0x104ff090),
    (0x104ff090, 0x105022a0),
    (0x105022a0, 0x105026a0),
    (0x105026a0, 0x105026c0),
)
ROUTINE = re.compile(r'EURO_CHAMP_QUAL_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(euro_champ_qual_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "euro_champ_qual_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of euro_champ_qual_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (EURO_CHAMP_QUAL_ROUTINE), with drafts also the EURO_CHAMP_QUAL_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"euro_champ_qual_routines.inc: {entry:x}-{end:x} is not a routine of EURO_CHAMP_QUAL.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An EURO_CHAMP_QUAL.CPP routine is listed twice")
    return found


EURO_CHAMP_QUAL_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_euro_champ_qual_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"EURO_CHAMP_QUAL.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"euro_champ_qual_routines.inc pins {expected})")


def volatile_reads(db):
    results = {a: "R3" for a, _ in ROUTINES}
    # Actual NONE/R3 declarations also cover callers of void routines once
    # implementations are registered; no declaration is inferred from a name.
    for entry, kind in re.findall(r'EURO_CHAMP_QUAL_(?:ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*0x[0-9a-fA-F]+,\s*(NONE|R3)', routine_lines()):
        results[int(entry, 16)] = kind
    found = module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)
    # Virtual dispatch supplies r12 explicitly; no other scratch inputs are used.
    return [line for line in found if not line.startswith("r12 read at 10781320 in 10781320,")]


def euro_champ_qual_hook(address):
    return f"if (try_modern_euro_champ_qual(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of euro_champ_qual_routines.inc."""
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

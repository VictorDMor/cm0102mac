"""The original's SHORTLIST_MANAGER.CPP and the routines of it that run as C++.

ROUTINES lists every routine of SHORTLIST_MANAGER.CPP as (entry, end). The
module runs from the shortlist manager's constructor (0x10356410, the first
routine to load the module's string pool, TOC -0x4dd8, which points at
"SHORTLIST_MANAGER.CPP") to SIMULATED_STATS.CPP's first routine (0x1036ff60,
the first to load that module's slots -0x4dd0..-0x4dc4). SETUP.CPP's pool
(-0x4e18) is last loaded at 0x10356250; the static initializer between
(0x103563e0) constructs SETUP.CPP's globals. The entries are every direct
call target and transition-vector target in those bounds (the seven vectors
at TOC -0x62a8..-0x6290 are the module's sort/search comparators and an
array element constructor); each routine runs to the next entry. Words
inside some routines are jump tables, not instructions.

native/shortlist_manager_routines.inc lists the routines rewritten so far,
each with a digest of its original instructions (tools/tactics_audit.py
digest): SHORTLIST_MANAGER_ROUTINE lines are verified and generation hooks
them (try_modern_shortlist_manager); SHORTLIST_MANAGER_DRAFT lines run only
in check tools (cm-world-replay --check-routines). Generation fails when the
export no longer matches a digest. Every entry is hooked (taking each over
from the shortlist families' builder owners); routines not listed as
SHORTLIST_MANAGER_ROUTINE run their generated bodies.
"""
import json
import re
from pathlib import Path

from main_program_audit import SCRATCH, module_volatile_reads, read_before_write
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10356410, 0x1036ff60)
ROUTINES = (
    (0x10356410, 0x103567b0), (0x103567b0, 0x103568e0), (0x103568e0, 0x10356e40), (0x10356e40, 0x103573c0),
    (0x103573c0, 0x10357cb0), (0x10357cb0, 0x10357d30), (0x10357d30, 0x10358020), (0x10358020, 0x10358af0),
    (0x10358af0, 0x10358b30), (0x10358b30, 0x10358f20), (0x10358f20, 0x10358f50), (0x10358f50, 0x10358f80),
    (0x10358f80, 0x10358ff0), (0x10358ff0, 0x10359020), (0x10359020, 0x10359980), (0x10359980, 0x10359ba0),
    (0x10359ba0, 0x10359dc0), (0x10359dc0, 0x1035a9f0), (0x1035a9f0, 0x1035e9f0), (0x1035e9f0, 0x10361490),
    (0x10361490, 0x10362d70), (0x10362d70, 0x10362e00), (0x10362e00, 0x10362ef0), (0x10362ef0, 0x10363130),
    (0x10363130, 0x103632f0), (0x103632f0, 0x103636b0), (0x103636b0, 0x10363b80), (0x10363b80, 0x10363ca0),
    (0x10363ca0, 0x10363f60), (0x10363f60, 0x10364d00), (0x10364d00, 0x10365590), (0x10365590, 0x10365c30),
    (0x10365c30, 0x10365c80), (0x10365c80, 0x10365cc0), (0x10365cc0, 0x10365d10), (0x10365d10, 0x103660b0),
    (0x103660b0, 0x10366250), (0x10366250, 0x103663b0), (0x103663b0, 0x10366410), (0x10366410, 0x10366470),
    (0x10366470, 0x10369080), (0x10369080, 0x1036d240), (0x1036d240, 0x1036d2f0), (0x1036d2f0, 0x1036d510),
    (0x1036d510, 0x1036d6e0), (0x1036d6e0, 0x1036ec30), (0x1036ec30, 0x1036f100), (0x1036f100, 0x1036f170),
    (0x1036f170, 0x1036f590), (0x1036f590, 0x1036f670), (0x1036f670, 0x1036f6f0), (0x1036f6f0, 0x1036f7e0),
    (0x1036f7e0, 0x1036f920), (0x1036f920, 0x1036fa00), (0x1036fa00, 0x1036fbf0), (0x1036fbf0, 0x1036ff00),
    (0x1036ff00, 0x1036ff60),
)
# The calls of the pointer glue (0x10781320) through which the module's
# callbacks return: qsort (0x10768300) and bsearch (0x10775600) for the
# comparators (0x10358f20, 0x10358f50, 0x10358f80, 0x10358ff0, 0x103663b0,
# 0x10366410), the array constructor (0x10773320) for 0x10358af0.
CALLBACK_SITES = (0x10768428, 0x1076844c, 0x107685c8, 0x107685ec, 0x10775658, 0x107756a4, 0x1077337c, 0x107733e0)
CALLBACK_CALLERS = ((0x10768300, 0x10768700), (0x10775600, 0x10775740), (0x10773320, 0x10773420))
# Besides the integer scratch registers, the volatile FPRs the C++ leaves as
# guest::Fpu left them: f0 and f2-f13 (f1 is a result; the checker compares
# f1 and f14-f31).
FLOAT_SCRATCH = {f"f{n}": (0x4000 + n * 16, 8) for n in (0, *range(2, 14))}
ROUTINE = re.compile(r'SHORTLIST_MANAGER_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(shortlist_manager_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "shortlist_manager_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of shortlist_manager_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (SHORTLIST_MANAGER_ROUTINE), with drafts also the SHORTLIST_MANAGER_DRAFT
    ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"shortlist_manager_routines.inc: {entry:x}-{end:x} is not a routine of SHORTLIST_MANAGER.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A SHORTLIST_MANAGER.CPP routine is listed twice")
    return found


# Generation hooks every routine of the module: the adapter runs the C++ of
# the SHORTLIST_MANAGER_ROUTINE lines and declines the others, whose
# generated bodies run (check tools also run the drafts).
SHORTLIST_MANAGER_ENTRIES = tuple(entry for entry, _ in ROUTINES)


def require_shortlist_manager_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"SHORTLIST_MANAGER.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"shortlist_manager_routines.inc pins {expected})")


def volatile_reads(db, routines_inc=INCLUDES):
    """Reads of the registers the C++ does not reproduce (r0, r11, r12, CTR,
    the carry, f0, f2-f13, and r3 after a routine that returns nothing)
    around SHORTLIST_MANAGER.CPP; empty when none
    (main_program_audit.module_volatile_reads)."""
    # r3 is dead after a routine that returns nothing or only f1.
    results = dict((int(entry, 16), "R3" if "R3" in kind else "NONE") for entry, kind in
                   re.findall(r'SHORTLIST_MANAGER_(?:ROUTINE|DRAFT)\((0x[0-9a-f]+), 0x[0-9a-f]+, ([\w|]+)',
                              routine_lines(routines_inc)))
    scratch = SCRATCH | FLOAT_SCRATCH
    found = module_volatile_reads(db, ROUTINES, MODULE, results, scratch)
    # The callbacks return through the pointer glue straight to qsort,
    # bsearch and the array constructor, after their glue calls.
    records = {}
    for low, high in CALLBACK_CALLERS:
        for address, body in db.execute("SELECT address, body FROM instructions WHERE address >= ? AND address < ?",
                                        (low, high)):
            records[address] = json.loads(body)
    for site in CALLBACK_SITES:
        for name, register in scratch.items():
            if (at := read_before_write(records, records[site]["fallthrough"], register)) is not None:
                found.append(f"{name} read at {at:x} after the callback call at {site:x}")
    return sorted(found)


def shortlist_manager_hook(address):
    return f"if (try_modern_shortlist_manager(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of shortlist_manager_routines.inc."""
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

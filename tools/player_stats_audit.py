"""The original's PLAYER_STATS.CPP and the routines of it that run as C++.

ROUTINES lists every routine of PLAYER_STATS.CPP as (entry, end). The module
runs from the player-stats object's constructor (0x10301540, the first
routine to load the module's string pool, TOC -0x5050, and its other slots
-0x5074..-0x5048) to PRINTOUTS.CPP's first routine (0x10309ee0, which loads
its own pool at -0x5034). The three leaves before 0x10301540 are called only
from the module before it. The entries are every direct call target and
transition-vector target in those bounds (the module's jump tables at TOC
-0x5068..-0x5074 hold case labels, not routines); each routine runs to the
next entry.

native/player_stats_routines.inc lists the routines rewritten so far, each
with a digest of its original instructions (tools/tactics_audit.py digest):
PLAYER_STATS_ROUTINE lines are verified and generation hooks them
(try_modern_player_stats); PLAYER_STATS_DRAFT lines run only in check tools
(cm-world-replay --check-routines). Generation fails when the export no
longer matches a digest.
"""
import json
import re
from pathlib import Path

from main_program_audit import SCRATCH, module_volatile_reads, read_before_write
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10301540, 0x10309ee0)
ROUTINES = (
    (0x10301540, 0x103023e0), (0x103023e0, 0x103024a0), (0x103024a0, 0x10302b20), (0x10302b20, 0x10302cb0),
    (0x10302cb0, 0x10302e70), (0x10302e70, 0x10303090), (0x10303090, 0x103031f0), (0x103031f0, 0x103032e0),
    (0x103032e0, 0x10303560), (0x10303560, 0x10303980), (0x10303980, 0x10303ff0), (0x10303ff0, 0x10304190),
    (0x10304190, 0x10304b10), (0x10304b10, 0x103054e0), (0x103054e0, 0x10305700), (0x10305700, 0x103058f0),
    (0x103058f0, 0x10305ae0), (0x10305ae0, 0x10305c60), (0x10305c60, 0x10306a80), (0x10306a80, 0x10306c00),
    (0x10306c00, 0x103078a0), (0x103078a0, 0x10308090), (0x10308090, 0x10308350), (0x10308350, 0x10308de0),
    (0x10308de0, 0x10309040), (0x10309040, 0x103092f0), (0x103092f0, 0x103094b0), (0x103094b0, 0x10309680),
    (0x10309680, 0x10309850), (0x10309850, 0x10309940), (0x10309940, 0x10309a00), (0x10309a00, 0x10309a70),
    (0x10309a70, 0x10309ae0), (0x10309ae0, 0x10309b60), (0x10309b60, 0x10309cc0), (0x10309cc0, 0x10309ee0),
)
# The calls of the pointer glue (0x10781320) in qsort (0x10768300) and
# bsearch (0x10775600): the comparators (0x10309a00, 0x10309a70, 0x10309ae0)
# return there.
# Besides the integer scratch registers, the volatile FPRs the C++ leaves as
# guest::Fpu left them: f0 and f2-f13 (f1 is a result; the checker compares
# f1 and f14-f31).
FLOAT_SCRATCH = {f"f{n}": (0x4000 + n * 16, 8) for n in (0, *range(2, 14))}
CALLBACK_SITES = (0x10768428, 0x1076844c, 0x107685c8, 0x107685ec, 0x10775658, 0x107756a4)
ROUTINE = re.compile(r'PLAYER_STATS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(player_stats_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "player_stats_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of player_stats_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (PLAYER_STATS_ROUTINE), with drafts also the PLAYER_STATS_DRAFT ones
    check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"player_stats_routines.inc: {entry:x}-{end:x} is not a routine of PLAYER_STATS.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A PLAYER_STATS.CPP routine is listed twice")
    return found


PLAYER_STATS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_player_stats_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"PLAYER_STATS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"player_stats_routines.inc pins {expected})")


def volatile_reads(db, routines_inc=INCLUDES):
    """Reads of the registers the C++ does not reproduce (r0, r11, r12, CTR,
    the carry, f0, f2-f13, and r3 after a routine that returns nothing) around
    PLAYER_STATS.CPP; empty when none (main_program_audit.module_volatile_reads)."""
    # r3 is dead after a routine that returns nothing or only f1.
    results = dict((int(entry, 16), "R3" if "R3" in kind else "NONE") for entry, kind in
                   re.findall(r'PLAYER_STATS_ROUTINE\((0x[0-9a-f]+), 0x[0-9a-f]+, ([\w|]+)', routine_lines(routines_inc)))
    scratch = SCRATCH | FLOAT_SCRATCH
    found = module_volatile_reads(db, ROUTINES, MODULE, results, scratch)
    # The comparators return through the pointer glue straight to qsort and
    # bsearch, after their glue calls.
    records = {}
    for low, high in ((0x10768300, 0x10768700), (0x10775600, 0x10775740)):
        for address, body in db.execute("SELECT address, body FROM instructions WHERE address >= ? AND address < ?",
                                        (low, high)):
            records[address] = json.loads(body)
    for site in CALLBACK_SITES:
        for name, register in scratch.items():
            if (at := read_before_write(records, records[site]["fallthrough"], register)) is not None:
                found.append(f"{name} read at {at:x} after the comparator call at {site:x}")
    return sorted(found)


def player_stats_hook(address):
    return f"if (try_modern_player_stats(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of player_stats_routines.inc."""
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

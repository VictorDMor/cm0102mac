"""The original's MAIN.CPP and the routines of it that run as C++.

ROUTINES lists every routine of MAIN.CPP as (entry, end). The module runs
from main() (0x1076f2b0, called by the runtime's start at 0x1077375c) to the
runtime library's byte copy (0x10771110): its routines read MAIN.CPP's
string pool (TOC -0x3bac and the slots next to it, -0x3bcc..-0x3ba8) and its
globals (TOC -0x354e..-0x352b), and the two Apple-event callbacks at the end
are reached through its own TOC slots. The MoreFiles helpers before main()
and the runtime after 0x10771110 use neither. The entries are the analysed
functions plus every direct call or function-pointer target; each routine
runs to the next entry.

native/main_program_routines.inc lists the routines rewritten so far, each
with a digest of its original instructions (tools/tactics_audit.py digest):
MAIN_ROUTINE lines are verified and generation hooks them
(try_modern_main_program); MAIN_DRAFT lines run only in check tools
(cm-world-replay --check-routines). Generation fails when the export no
longer matches a digest.
"""
import json
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1076f2b0, 0x10771110)
ROUTINES = (
    (0x1076f2b0, 0x1076f2e0), (0x1076f2e0, 0x1076f440), (0x1076f440, 0x1076f4f0), (0x1076f4f0, 0x1076f600),
    (0x1076f600, 0x1076f730), (0x1076f730, 0x1076f780), (0x1076f780, 0x1076f7d0), (0x1076f7d0, 0x1076f880),
    (0x1076f880, 0x1076f890), (0x1076f890, 0x1076f960), (0x1076f960, 0x1076fa20), (0x1076fa20, 0x1076faf0),
    (0x1076faf0, 0x1076fd90), (0x1076fd90, 0x1076fe80), (0x1076fe80, 0x10770170), (0x10770170, 0x107701d0),
    (0x107701d0, 0x10770240), (0x10770240, 0x10770380), (0x10770380, 0x107704e0), (0x107704e0, 0x10770570),
    (0x10770570, 0x10770650), (0x10770650, 0x10770670), (0x10770670, 0x10770760), (0x10770760, 0x107707c0),
    (0x107707c0, 0x10770880), (0x10770880, 0x107708c0), (0x107708c0, 0x10770950), (0x10770950, 0x10770970),
    (0x10770970, 0x107709e0), (0x107709e0, 0x10770a10), (0x10770a10, 0x10770a20), (0x10770a20, 0x10770a30),
    (0x10770a30, 0x10770a80), (0x10770a80, 0x10770ab0), (0x10770ab0, 0x10770ac0), (0x10770ac0, 0x10770c90),
    (0x10770c90, 0x10770ca0), (0x10770ca0, 0x10770cc0), (0x10770cc0, 0x10770d20), (0x10770d20, 0x10770e40),
    (0x10770e40, 0x10770ee0), (0x10770ee0, 0x10770f60), (0x10770f60, 0x10771010), (0x10771010, 0x10771110),
)
ROUTINE = re.compile(r'MAIN_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*(\w+),\s*"([0-9a-f]+)"')
INCLUDES = ROOT / "native" / "main_program_routines.inc"


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (MAIN_ROUTINE), with drafts also the MAIN_DRAFT ones check tools run."""
    if not path.exists():
        return []
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(path.read_text()) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"main_program_routines.inc: {entry:x}-{end:x} is not a routine of MAIN.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A MAIN.CPP routine is listed twice")
    return found


MAIN_PROGRAM_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_main_program_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"MAIN.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"main_program_routines.inc pins {expected})")


# Registers the C++ does not reproduce (native/main_program_detail.hpp): r0,
# r11, r12, CTR and XER's carry, which the code around the routines must
# write before reading, and r3 after a routine that returns nothing.
SCRATCH = {"r0": (0, 4), "r11": (44, 4), "r12": (48, 4), "CTR": (4132, 4), "CA": (1027, 1)}
RESULT = ("r3", (12, 4))
SCAN_LIMIT = 64


def _registers(varnode):
    if varnode and varnode[0] == "register":
        return int(varnode[1]), int(varnode[2])
    return None


def _touches(varnode, register):
    found = _registers(varnode)
    return found is not None and found[0] < register[0] + register[1] and register[0] < found[0] + found[1]


def _covers(varnode, register):
    found = _registers(varnode)
    return found is not None and found[0] <= register[0] and register[0] + register[1] <= found[0] + found[1]


def read_before_write(records, start, register, calls=2):
    """The first instruction from `start` (following branches, up to
    SCAN_LIMIT instructions a path) that reads `register` before any
    instruction writes it, or None. At a call the callee is scanned from its
    entry (`calls` levels deep) and the path ends: by the calling convention
    the volatile registers are dead after a call (r3 then holds the callee's
    result). A return ends the path too, for the same reason. The alignment
    no-op `ori r0,r0,0` reads nothing."""
    seen, paths = set(), [(start, 0)]
    while paths:
        address, depth = paths.pop()
        if address in seen or depth > SCAN_LIMIT or address not in records:
            continue
        seen.add(address)
        record = records[address]
        if record["assembly"] == "ori r0,r0,0x0":
            paths.append((record["fallthrough"], depth + 1))
            continue
        written = False
        for op in record["ops"]:
            if not written and any(_touches(input_, register) for input_ in op[2:]):
                return address
            if op[1] and _covers(op[1], register):
                written = True
        if written:
            continue
        flow = record.get("flow")
        if flow == "UNCONDITIONAL_CALL":
            target = (record.get("flows") or [None])[0]
            if target is not None and calls and (at := read_before_write(records, target, register, calls - 1)) is not None:
                return at
            continue
        if flow in ("TERMINATOR", "CALL_RETURN", "COMPUTED_CALL") or record["assembly"] in ("blr", "bctr"):
            continue
        for target in record.get("flows", []):
            paths.append((target, depth + 1))
        if record.get("fallthrough") is not None and flow != "UNCONDITIONAL_JUMP":
            paths.append((record["fallthrough"], depth + 1))
    return None


def volatile_reads(db, routines_inc=INCLUDES):
    """Where the code around MAIN.CPP reads a register its C++ leaves
    different: after each call of a MAIN.CPP routine (r0, r11, r12, CTR and
    the carry; r3 too for those that return nothing) and at the entry of
    each routine MAIN.CPP calls (the scratch registers). Empty when the C++
    may leave them as it likes."""
    records = {}
    for address, body in db.execute("SELECT address, body FROM instructions"):
        records[address] = json.loads(body)
    results = dict((int(entry, 16), kind) for entry, kind in
                   re.findall(r'MAIN_ROUTINE\((0x[0-9a-f]+), 0x[0-9a-f]+, (NONE|R3)', routines_inc.read_text()))
    entries = {entry for entry, _ in ROUTINES}
    low, high = MODULE
    found = []
    for address, record in records.items():
        if record.get("flow") != "UNCONDITIONAL_CALL" or not record.get("flows"):
            continue
        target = record["flows"][0]
        if target in entries and not low <= address < high:
            # A caller outside MAIN.CPP, after the call returns.
            checked = dict(SCRATCH)
            if results.get(target) == "NONE":
                checked[RESULT[0]] = RESULT[1]
            for name, register in checked.items():
                if (at := read_before_write(records, record["fallthrough"], register)) is not None:
                    found.append(f"{name} read at {at:x} after the call of {target:x} at {address:x}")
        elif low <= address < high and target not in entries:
            # A routine MAIN.CPP calls, from its entry.
            for name, register in SCRATCH.items():
                if (at := read_before_write(records, target, register)) is not None:
                    found.append(f"{name} read at {at:x} in {target:x}, called by MAIN.CPP at {address:x}")
    return sorted(set(found))


def main_program_hook(address):
    return f"if (try_modern_main_program(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of main_program_routines.inc."""
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
    if args.volatiles:
        db = sqlite3.connect(args.export / "instructions.sqlite")
        for line in volatile_reads(db):
            print(line)
        return
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    db = sqlite3.connect(args.export / "instructions.sqlite")
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

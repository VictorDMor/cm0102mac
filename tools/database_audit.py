"""DATABASE.CPP: the game database's records, lookups and queries.

Bounds (closed 2026-11-06 with tools/game_inventory.py): the module starts at
0x1009efc0, where CONTRACT_SCREENS.CPP's audited range ends, and ends before
0x100de410, the first routine that loads DISCIPLINE.CPP's source-name pool.
The last routine, 0x100de3b0, is the unit's static initializer (the initial
season date publication, already C++ under engine_initialization): the
compiler puts a unit's initializer at its end, as CONTRACT_MANAGER.CPP's
0x100939b0 and HALL_OF_FAME.CPP's 0x10143240 show. The 22 routines between
the last DATABASE.CPP source-name load (0x100db410) and that initializer have
no source-name evidence; their callers and callees (player searches with a
qsort comparator, contract and staff lookups, no DISCIPLINE.CPP caller) fit a
database unit, and nothing places another unit there.

ROUTINES are (entry, end): entries are direct-call and function-pointer
targets, each body is what control flow reaches from its entry (through jump
tables), and end is the word after its last instruction; the words between
routines are zero padding. test_database_audit.py re-derives all of this from
the export.

native/database_routines.inc lists the routines rewritten so far, each with a
digest of its original instructions: DATABASE_ROUTINE lines are verified and
generation hooks them (try_modern_database); DATABASE_DRAFT lines run only in
check tools (cm-world-replay --check-routines).
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x1009efc0, 0x100de410)
ROUTINES = (
            (0x1009efc0, 0x100a0d78), (0x100a0d80, 0x100a1944), (0x100a1950, 0x100a2d98),
            (0x100a2da0, 0x100a34e0), (0x100a34e0, 0x100a3da4), (0x100a3db0, 0x100a4bf8),
            (0x100a4c00, 0x100a4c68), (0x100a4c70, 0x100a4cd8), (0x100a4ce0, 0x100a4e50),
            (0x100a4e50, 0x100a55a0), (0x100a55a0, 0x100ab5cc), (0x100ab5d0, 0x100abfa8),
            (0x100abfb0, 0x100af89c), (0x100af8a0, 0x100b2b48), (0x100b2b50, 0x100b2b7c),
            (0x100b2b80, 0x100b3650), (0x100b3650, 0x100b3704), (0x100b3710, 0x100b3c64),
            (0x100b3c70, 0x100b44b8), (0x100b44c0, 0x100b604c), (0x100b6050, 0x100b8b70),
            (0x100b8b70, 0x100b8b7c), (0x100b8b80, 0x100b8cec), (0x100b8cf0, 0x100c2518),
            (0x100c2520, 0x100c2800), (0x100c2800, 0x100c2ca4), (0x100c2cb0, 0x100c3868),
            (0x100c3870, 0x100c3ca4), (0x100c3cb0, 0x100c4178), (0x100c4180, 0x100c42a8),
            (0x100c42b0, 0x100c43f8), (0x100c4400, 0x100c4b00), (0x100c4b00, 0x100c4ba0),
            (0x100c4ba0, 0x100c4cf0), (0x100c4cf0, 0x100c53dc), (0x100c53e0, 0x100c562c),
            (0x100c5630, 0x100c58b4), (0x100c58c0, 0x100c5aa4), (0x100c5ab0, 0x100c6150),
            (0x100c6150, 0x100c63b4), (0x100c63c0, 0x100c65c4), (0x100c65d0, 0x100c6a04),
            (0x100c6a10, 0x100c6ae4), (0x100c6af0, 0x100c6be8), (0x100c6bf0, 0x100c76bc),
            (0x100c76c0, 0x100c785c), (0x100c7860, 0x100c79f8), (0x100c7a00, 0x100c7a20),
            (0x100c7a20, 0x100c7a4c), (0x100c7a50, 0x100c7bc0), (0x100c7bc0, 0x100c7cf4),
            (0x100c7d00, 0x100c7dc8), (0x100c7dd0, 0x100c8298), (0x100c82a0, 0x100c8928),
            (0x100c8930, 0x100c9068), (0x100c9070, 0x100c943c), (0x100c9440, 0x100c9590),
            (0x100c9590, 0x100c96e0), (0x100c96e0, 0x100c9bac), (0x100c9bb0, 0x100c9c88),
            (0x100c9c90, 0x100c9e4c), (0x100c9e50, 0x100ca00c), (0x100ca010, 0x100ca140),
            (0x100ca140, 0x100ca6a0), (0x100ca6a0, 0x100ca7d0), (0x100ca7d0, 0x100ca7fc),
            (0x100ca800, 0x100caa10), (0x100caa10, 0x100caa84), (0x100caa90, 0x100cab04),
            (0x100cab10, 0x100cae30), (0x100cae30, 0x100caea4), (0x100caeb0, 0x100cb588),
            (0x100cb590, 0x100cb744), (0x100cb750, 0x100cbdd8), (0x100cbde0, 0x100cbf58),
            (0x100cbf60, 0x100cc49c), (0x100cc4a0, 0x100cd060), (0x100cd060, 0x100cd180),
            (0x100cd180, 0x100cd2a0), (0x100cd2a0, 0x100cd324), (0x100cd330, 0x100cd38c),
            (0x100cd390, 0x100cd408), (0x100cd410, 0x100cd498), (0x100cd4a0, 0x100cd528),
            (0x100cd530, 0x100cd6a8), (0x100cd6b0, 0x100cd734), (0x100cd740, 0x100cd7c0),
            (0x100cd7c0, 0x100cd974), (0x100cd980, 0x100cdae0), (0x100cdae0, 0x100cde64),
            (0x100cde70, 0x100cdeec), (0x100cdef0, 0x100ce09c), (0x100ce0a0, 0x100ce1fc),
            (0x100ce200, 0x100ce24c), (0x100ce250, 0x100ce314), (0x100ce320, 0x100d0538),
            (0x100d0540, 0x100d06e4), (0x100d06f0, 0x100d0940), (0x100d0940, 0x100d12d0),
            (0x100d12d0, 0x100d15e4), (0x100d15f0, 0x100d1ca8), (0x100d1cb0, 0x100d1d70),
            (0x100d1d70, 0x100d21e8), (0x100d21f0, 0x100d2324), (0x100d2330, 0x100d271c),
            (0x100d2720, 0x100d2a74), (0x100d2a80, 0x100d2b40), (0x100d2b40, 0x100d2d70),
            (0x100d2d70, 0x100d2e60), (0x100d2e60, 0x100d2f48), (0x100d2f50, 0x100d3038),
            (0x100d3040, 0x100d314c), (0x100d3150, 0x100d3228), (0x100d3230, 0x100d32fc),
            (0x100d3300, 0x100d33cc), (0x100d33d0, 0x100d34b8), (0x100d34c0, 0x100d356c),
            (0x100d3570, 0x100d3c40), (0x100d3c40, 0x100d3e60), (0x100d3e60, 0x100d3f24),
            (0x100d3f30, 0x100d4370), (0x100d4370, 0x100d487c), (0x100d4880, 0x100d496c),
            (0x100d4970, 0x100d4dec), (0x100d4df0, 0x100d4ed4), (0x100d4ee0, 0x100d4fc8),
            (0x100d4fd0, 0x100d50e0), (0x100d50e0, 0x100d51c4), (0x100d51d0, 0x100d530c),
            (0x100d5310, 0x100d5444), (0x100d5450, 0x100d5534), (0x100d5540, 0x100d5cdc),
            (0x100d5ce0, 0x100d5d64), (0x100d5d70, 0x100d5dd8), (0x100d5de0, 0x100d62f8),
            (0x100d6300, 0x100d6af0), (0x100d6af0, 0x100d7194), (0x100d71a0, 0x100d8254),
            (0x100d8260, 0x100d943c), (0x100d9440, 0x100d9478), (0x100d9480, 0x100d94c4),
            (0x100d94d0, 0x100d9514), (0x100d9520, 0x100d957c), (0x100d9580, 0x100d95dc),
            (0x100d95e0, 0x100d979c), (0x100d97a0, 0x100d995c), (0x100d9960, 0x100d9b7c),
            (0x100d9b80, 0x100d9da8), (0x100d9db0, 0x100d9e08), (0x100d9e10, 0x100d9e8c),
            (0x100d9e90, 0x100d9f0c), (0x100d9f10, 0x100d9fb0), (0x100d9fb0, 0x100da050),
            (0x100da050, 0x100da0ac), (0x100da0b0, 0x100da10c), (0x100da110, 0x100da314),
            (0x100da320, 0x100da524), (0x100da530, 0x100da5ac), (0x100da5b0, 0x100daf98),
            (0x100dafa0, 0x100db058), (0x100db060, 0x100db118), (0x100db120, 0x100db1b0),
            (0x100db1b0, 0x100db404), (0x100db410, 0x100dbb20), (0x100dbb20, 0x100dbb60),
            (0x100dbb60, 0x100dbbd4), (0x100dbbe0, 0x100dbc20), (0x100dbc20, 0x100dc4c8),
            (0x100dc4d0, 0x100dc9fc), (0x100dca00, 0x100dcc28), (0x100dcc30, 0x100dcd84),
            (0x100dcd90, 0x100dcfb8), (0x100dcfc0, 0x100dd0f8), (0x100dd100, 0x100dd1f4),
            (0x100dd200, 0x100dd378), (0x100dd380, 0x100dd4f8), (0x100dd500, 0x100dd678),
            (0x100dd680, 0x100dd7f0), (0x100dd7f0, 0x100dd9a8), (0x100dd9b0, 0x100ddb68),
            (0x100ddb70, 0x100dde94), (0x100ddea0, 0x100ddf2c), (0x100ddf30, 0x100ddfb4),
            (0x100ddfc0, 0x100de08c), (0x100de090, 0x100de15c), (0x100de160, 0x100de254),
            (0x100de260, 0x100de314), (0x100de320, 0x100de3b0), (0x100de3b0, 0x100de408),
)
# `b` words after an unconditional branch in 0x100d1d70: compiler leftovers no
# flow reaches, so the export doesn't hold them.
UNREACHABLE = frozenset({0x100d1e88, 0x100d2064})
INITIALIZER = 0x100de3b0
ROUTINE = re.compile(r'DATABASE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(database_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "database_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of database_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (DATABASE_ROUTINE), with drafts also the DATABASE_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"database_routines.inc: {entry:x}-{end:x} is not a routine of DATABASE.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A DATABASE.CPP routine is listed twice")
    return found


DATABASE_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_database_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        if [r["address"] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing DATABASE.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"DATABASE.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"database_routines.inc pins {expected})")


def database_hook(address):
    return f"if (try_modern_database(s, 0x{address:x})) return true;"


def require_database_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in DATABASE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = database_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(f"i_{entry:x}:", 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced DATABASE hook at {entry:x}")
        # The owner must be the first hook at its entry: an earlier one (an
        # older lowering, another family) would run instead of the C++.
        if fragment.find("if (try_") != fragment.index(hook):
            raise ValueError(f"Another hook precedes the DATABASE hook at {entry:x}")


def main():
    """Print each routine's digest, for new lines of database_routines.inc."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routines to print (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true",
                        help="print every entry alone (for cm-world-replay --census)")
    args = parser.parse_args()
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

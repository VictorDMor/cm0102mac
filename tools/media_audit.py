"""media.cpp: the press, between tur_second.cpp and the unnamed pundit unit.

The source pool at -0x41f8 ("media.dat", then "media.cpp") and the floating
constants at -0x4200 and -0x41fc are loaded only inside this range, and the next
unit's pool (-0x41e8, "CM3_DATA" and the pundit files) only after it. Direct calls,
six transition vectors and the switch tables close the inventory: 75 entries,
99,976 indexed instructions. The 63 unindexed words after unconditional jumps are
branches of the compiler's dead code; nothing reaches them.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10646d90, 0x106a8a50)
ROUTINES = ((0x10646d90, 0x10646dfc), (0x10646e00, 0x10646e84), (0x10646e90, 0x10647418), (0x10647420, 0x10647730), 
            (0x10647730, 0x10647bf4), (0x10647c00, 0x106487c4), (0x106487d0, 0x10649900), (0x10649900, 0x10649ac8), 
            (0x10649ad0, 0x10649fac), (0x10649fb0, 0x1064cf74), (0x1064cf80, 0x106548d8), (0x106548e0, 0x10655028), 
            (0x10655030, 0x10655d58), (0x10655d60, 0x10656448), (0x10656450, 0x10657b2c), (0x10657b30, 0x10658370), 
            (0x10658370, 0x106583cc), (0x106583d0, 0x10658480), (0x10658480, 0x106584c4), (0x106584d0, 0x10661a50), 
            (0x10661a50, 0x106620b0), (0x106620b0, 0x1066b5f4), (0x1066b600, 0x1066bc50), (0x1066bc50, 0x1066c670), 
            (0x1066c670, 0x106727c0), (0x106727c0, 0x106727f4), (0x10672800, 0x10677d58), (0x10677d60, 0x1067b914), 
            (0x1067b920, 0x1067be98), (0x1067bea0, 0x1067ca24), (0x1067ca30, 0x1067d02c), (0x1067d030, 0x1067d034), 
            (0x1067d040, 0x1067d2e8), (0x1067d2f0, 0x1067d82c), (0x1067d830, 0x1067dc0c), (0x1067dc10, 0x1067e210), 
            (0x1067e210, 0x1067fed0), (0x1067fed0, 0x10688e80), (0x10688e80, 0x10688fc4), (0x10688fd0, 0x106891ac), 
            (0x106891b0, 0x1068a468), (0x1068a470, 0x10698ba4), (0x10698bb0, 0x10698e80), (0x10698e80, 0x10698f44), 
            (0x10698f50, 0x10699d10), (0x10699d10, 0x1069acdc), (0x1069ace0, 0x1069d194), (0x1069d1a0, 0x1069d460), 
            (0x1069d460, 0x1069e18c), (0x1069e190, 0x1069e3d0), (0x1069e3d0, 0x1069e47c), (0x1069e480, 0x1069e6f4), 
            (0x1069e700, 0x1069e890), (0x1069e890, 0x1069e900), (0x1069e900, 0x1069e990), (0x1069e990, 0x1069eb04), 
            (0x1069eb10, 0x1069ec88), (0x1069ec90, 0x1069ece4), (0x1069ecf0, 0x1069ed10), (0x1069ed10, 0x1069f268), 
            (0x1069f270, 0x106a15f0), (0x106a15f0, 0x106a1858), (0x106a1860, 0x106a1ef8), (0x106a1f00, 0x106a2330), 
            (0x106a2330, 0x106a2e0c), (0x106a2e10, 0x106a4098), (0x106a40a0, 0x106a44c8), (0x106a44d0, 0x106a52c8), 
            (0x106a52d0, 0x106a5e88), (0x106a5e90, 0x106a6228), (0x106a6230, 0x106a6e5c), (0x106a6e60, 0x106a7120), 
            (0x106a7120, 0x106a7d68), (0x106a7d70, 0x106a810c), (0x106a8110, 0x106a8a50))
ROUTINE = re.compile(r'MEDIA_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(media_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "media_routines.inc"
# Each follows an unconditional jump; no export flow, table or vector reaches it.
UNREACHABLE = frozenset({0x10649cc0, 0x1064dddc, 0x10650b58, 0x10651808, 0x10651d2c, 0x10652f60, 0x106553e0, 0x106554ac, 
                              0x106555a8, 0x10655774, 0x10655878, 0x106575bc, 0x10657618, 0x10657b98, 0x10657bf4, 0x10657c50, 
                              0x10657cac, 0x10657d08, 0x10657d64, 0x10657dc0, 0x10657e1c, 0x10657e78, 0x10657ed4, 0x10657f30, 
                              0x10657f8c, 0x10657fe8, 0x10658044, 0x106580a0, 0x106580fc, 0x10658158, 0x106581b4, 0x10658210, 
                              0x1065826c, 0x106582c8, 0x10658324, 0x106728e8, 0x10677e70, 0x106797bc, 0x1067de5c, 0x1067f4c8, 
                              0x10680e20, 0x106813ac, 0x106818fc, 0x10681c48, 0x106823c4, 0x10682b58, 0x10685338, 0x10689ab0, 
                              0x1068bfb0, 0x1068c500, 0x1068ca68, 0x1068d780, 0x1068f490, 0x106911a8, 0x10692308, 0x10692848, 
                              0x10692b24, 0x10692e60, 0x106937a8, 0x106943e8, 0x1069ca5c, 0x106a4bfc, 0x106a894c})
# The generator hooks every entry from the start; the adapter declines drafts in
# production, so same-page calls reach the checker and unfinished routines stay generated.
MEDIA_ENTRIES = tuple(entry for entry, _ in ROUTINES)


def routine_lines(path=INCLUDES):
    """The text of media_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified (MEDIA_ROUTINE),
    with drafts also the MEDIA_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"media_routines.inc: {entry:x}-{end:x} is not a routine of media.cpp")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A media.cpp routine is listed twice")
    return found


def require_media_index(db, path=INCLUDES):
    """Every routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing media.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"media.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"media_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def media_hook(address):
    return f"if (try_modern_media(s, 0x{address:x})) return true;"


def require_media_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = media_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced media hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of the routine lists."""
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

"""Original CONTRACT_MANAGER.CPP census and staged semantic C++23 owners.

The source pool at TOC -0x5718 follows COMP_SCREENS. Its two comparator
vectors and its date-object initializer belong to this module; the next
screen callback begins at 0x10093a50. See private bounds evidence under
.local/contract-manager. The indexed reference omits an interior branch at
0x10080bf0 to 0x10080cf8; this is not another routine.
"""
import argparse
import json
from pathlib import Path
import re
import sqlite3

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10077ee0, 0x10093a50)
ROUTINES = ((268926688, 268927824), (268927824, 268928000), (268928000, 268929264), (268929264, 268930672), (268930672, 268930816), (268930816, 268931152), (268931152, 268942192), (268942192, 268942720), (268942720, 268943136), (268943136, 268943296), (268943296, 268943680), (268943680, 268944608), (268944608, 268948784), (268948784, 268949504), (268949504, 268952736), (268952736, 268952816), (268952816, 268953520), (268953520, 268953584), (268953584, 268953632), (268953632, 268954128), (268954128, 268955088), (268955088, 268955216), (268955216, 268955344), (268955344, 268955680), (268955680, 268955696), (268955696, 268955904), (268955904, 268956160), (268956160, 268957200), (268957200, 268957472), (268957472, 268957616), (268957616, 268957760), (268957760, 268958000), (268958000, 268958256), (268958256, 268958496), (268958496, 268958976), (268958976, 268959376), (268959376, 268959664), (268959664, 268960032), (268960032, 268961856), (268961856, 268962000), (268962000, 268962736), (268962736, 268963120), (268963120, 268966672), (268966672, 268966864), (268966864, 268970992), (268970992, 268971024), (268971024, 268971088), (268971088, 268971392), (268971392, 268971680), (268971680, 268971808), (268971808, 268971920), (268971920, 268972304), (268972304, 268972416), (268972416, 268975696), (268975696, 268977584), (268977584, 268978816), (268978816, 268978960), (268978960, 268979040), (268979040, 268979168), (268979168, 268982368), (268982368, 268999232), (268999232, 268999872), (268999872, 269000256), (269000256, 269000352), (269000352, 269001056), (269001056, 269001552), (269001552, 269003584), (269003584, 269008832), (269008832, 269009040), (269009040, 269009520), (269009520, 269010112), (269010112, 269011152), (269011152, 269011536), (269011536, 269012256), (269012256, 269014096), (269014096, 269017936), (269017936, 269020160), (269020160, 269022752), (269022752, 269023152), (269023152, 269023808), (269023808, 269024496), (269024496, 269026064), (269026064, 269026656), (269026656, 269027840), (269027840, 269029152), (269029152, 269030176), (269030176, 269030384), (269030384, 269030960), (269030960, 269031600), (269031600, 269032432), (269032432, 269037616), (269037616, 269039712), (269039712, 269039952), (269039952, 269040048), (269040048, 269040208))
ROUTINE = re.compile(r'CONTRACT_MANAGER_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(contract_manager_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "contract_manager_routines.inc"


def routine_lines(path=INCLUDES):
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text))


def rewritten(path=INCLUDES, drafts=False):
    found = sorted((int(entry, 16), int(end, 16), fingerprint) for stage, entry, end, _, fingerprint in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"contract_manager_routines.inc: {entry:x}-{end:x} is outside the original census")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A CONTRACT_MANAGER.CPP routine is listed twice")
    return found


CONTRACT_MANAGER_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_contract_manager_index(db):
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"CONTRACT_MANAGER.CPP reference changed at {entry:x}: {actual} != {expected}")


def contract_manager_hook(address):
    return f"if (try_modern_contract_manager(s, 0x{address:x})) return true;"


def require_contract_manager_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CONTRACT_MANAGER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook = contract_manager_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced contract-manager hook at {entry:x}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", action="store_true")
    parser.add_argument("--require-complete", action="store_true")
    args = parser.parse_args()
    if args.entries:
        print("\n".join(f"0x{entry:x}" for entry, _ in ROUTINES))
        return
    with sqlite3.connect(args.export / "instructions.sqlite") as db:
        require_contract_manager_index(db)
        completed = set(CONTRACT_MANAGER_ENTRIES)
        drafted = {entry for entry, _, _ in rewritten(drafts=True)} - completed
        for entry, end in ROUTINES:
            records = routine_records(db, entry, end)
            stage = "C++" if entry in completed else "draft" if entry in drafted else "original"
            print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5} {stage}")
    print(f"{len(completed)}/{len(ROUTINES)} production; {len(drafted)} drafts")
    if args.require_complete and len(completed) != len(ROUTINES):
        parser.exit(1, "CONTRACT_MANAGER.CPP still has routines outside semantic C++ production\n")


if __name__ == "__main__":
    main()

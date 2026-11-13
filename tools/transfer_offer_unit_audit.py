"""TRANSFER_OFFER.CPP: the transfer offer record, its terms, its value and its file form.

17 routines at 0x10411e60..0x10417590. The unit is named by its assertion file
name (the pool at TOC -0x4b88 starts "TRANSFER_OFFER.CPP") and bounded by what
loads its three pools (TOC -0x4b88, -0x4b8c, -0x4b90: 0x10411fa0 first,
0x104172f0 last). The three routines before the first pool load build and
destroy the record the others work on (0x10411ea0 calls 0x104126d0), and
TRANSFER_MANAGER.CPP's audit ends at 0x10411e60; TRANSFER_SCREENS.CPP's starts
at 0x10417590. The evidence is checked in tools/test_transfer_offer_unit_audit.py.

The files are named transfer_offer_unit because native/transfer_offer_* is an
older owner of 19 routines of TRANSFER_MANAGER.CPP.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records
from main_program_audit import SCRATCH, module_volatile_reads

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10411e60, 0x10417590)
ROUTINES = (
    (0x10411e60, 0x10411ea0), (0x10411ea0, 0x10411f08), (0x10411f10, 0x10411f9c), (0x10411fa0, 0x104123d4),
    (0x104123e0, 0x104126cc), (0x104126d0, 0x10415e4c), (0x10415e50, 0x104163c8), (0x104163d0, 0x104164e4),
    (0x104164f0, 0x10416710), (0x10416710, 0x10416834), (0x10416840, 0x10416d24), (0x10416d30, 0x10416d5c),
    (0x10416d60, 0x10416e04), (0x10416e10, 0x104170f0), (0x104170f0, 0x10417260), (0x10417260, 0x104172e4),
    (0x104172f0, 0x10417590))
ROUTINE = re.compile(r'TRANSFER_OFFER_UNIT_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(transfer_offer_unit_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "transfer_offer_unit_routines.inc"
# Alignment before a loop; a branch jumps over each, and the export leaves them out.
UNREACHABLE_NOPS = frozenset({0x10412134, 0x104121a8, 0x1041238c, 0x10412514, 0x10412638, 0x10416c78, 0x10416dbc,
                              0x1041708c, 0x1041729c})


def routine_lines(path=INCLUDES):
    """The text of transfer_offer_unit_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed
    (TRANSFER_OFFER_UNIT_ROUTINE), with drafts also the TRANSFER_OFFER_UNIT_DRAFT
    ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"transfer_offer_unit_routines.inc: {entry:x}-{end:x} is not a routine of TRANSFER_OFFER.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A TRANSFER_OFFER.CPP routine is listed twice")
    return found


# Every entry carries the hook; a draft's hook declines outside a checker (the adapter).
TRANSFER_OFFER_UNIT_ENTRIES = tuple(entry for entry, _ in ROUTINES)


def require_transfer_offer_unit_index(db, path=INCLUDES, complete=True):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("TRANSFER_OFFER.CPP requires all 17 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE_NOPS]:
            raise ValueError(f"Missing TRANSFER_OFFER.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"TRANSFER_OFFER.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"transfer_offer_unit_routines.inc pins {expected})")


def volatile_reads(db):
    results = {int(entry, 16): flags for _, entry, _, flags, _ in ROUTINE.findall(routine_lines())}
    return module_volatile_reads(db, ROUTINES, MODULE, results, SCRATCH)


def transfer_offer_unit_hook(address):
    return f"if (try_modern_transfer_offer_unit(s, 0x{address:x})) return true;"


def require_transfer_offer_unit_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = transfer_offer_unit_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced TRANSFER_OFFER hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of transfer_offer_unit_routines.inc."""
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

"""TRANSFER_SCREENS.CPP inventory, reference fingerprints and production owners.

The screen-state configuration helpers follow TRANSFER_OFFER; VIRTUAL_STAFF
starts at 0x1042fe10. Source pool TOC -0x4ad0, 51 transition vectors, every
incoming call, zero alignment padding and the absence of computed branches
close the 75-entry inventory. Raw listings and provenance stay private.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10417590, 0x1042fe10)
# (entry, end, indexed instruction count, original reference digest)
ORIGINAL = (
    (0x10417590, 0x10417880, 188, "5790b1a9950e67d3"),
    (0x10417880, 0x10417b60, 182, "0d061c796fbfebc4"),
    (0x10417b60, 0x1041b600, 3750, "93088ba994871231"),
    (0x1041b600, 0x1041e760, 3160, "a4f4d943bc8bd0c9"),
    (0x1041e760, 0x1041e770, 1, "9a6975b7e67b9b2b"),
    (0x1041e770, 0x1041ea60, 185, "ccfb77a79eecda10"),
    (0x1041ea60, 0x1041eb10, 43, "080984eac37961ed"),
    (0x1041eb10, 0x1041f3a0, 546, "81ebdac8578a8bfe"),
    (0x1041f3a0, 0x1041f830, 292, "ad5febab66d5f930"),
    (0x1041f830, 0x1041f8f0, 45, "4674b07581f95586"),
    (0x1041f8f0, 0x104204f0, 768, "041ea800687a693e"),
    (0x104204f0, 0x10420aa0, 361, "4eec3bd27064feeb"),
    (0x10420aa0, 0x10420b50, 43, "8172bddd4eb05439"),
    (0x10420b50, 0x10420e40, 188, "972c07e8b9fcd678"),
    (0x10420e40, 0x10420f00, 46, "725242f3f0225379"),
    (0x10420f00, 0x10421070, 91, "6655e47b03b28a48"),
    (0x10421070, 0x10421960, 571, "143f78a070f11e26"),
    (0x10421960, 0x10421a20, 46, "748c6b71de685a9f"),
    (0x10421a20, 0x10421af0, 50, "606f42f75da5b3f3"),
    (0x10421af0, 0x10421be0, 60, "9fd77a3a572fbd86"),
    (0x10421be0, 0x10421ce0, 61, "06f10262ece7023e"),
    (0x10421ce0, 0x10421dd0, 60, "73053d2a571f3a42"),
    (0x10421dd0, 0x10421ee0, 66, "8a9a6f4c727339af"),
    (0x10421ee0, 0x10421fb0, 50, "4bac73c6c67d563d"),
    (0x10421fb0, 0x10422310, 213, "11a0d255a68f6d73"),
    (0x10422310, 0x10422570, 150, "1739ec80d157ecfe"),
    (0x10422570, 0x10422600, 35, "9c693fdfae70c2d1"),
    (0x10422600, 0x10422a00, 253, "f2672faf502c0e73"),
    (0x10422a00, 0x10422ac0, 46, "7d83d0b75a20d472"),
    (0x10422ac0, 0x10422b50, 35, "4f1b003240fa87d9"),
    (0x10422b50, 0x10422e40, 188, "19c23104cd8944c0"),
    (0x10422e40, 0x10423040, 126, "49c892d7d5acc83f"),
    (0x10423040, 0x10423160, 72, "05ee457323665734"),
    (0x10423160, 0x10423270, 65, "9cffef8812bf9116"),
    (0x10423270, 0x10424220, 1001, "e13aca08cd56f79a"),
    (0x10424220, 0x104248a0, 414, "5051ea878955d74b"),
    (0x104248a0, 0x10424930, 35, "cf6bcca210bd55ea"),
    (0x10424930, 0x104249e0, 41, "ca0f767abf1edd80"),
    (0x104249e0, 0x10424d40, 214, "23a83762b4b4c733"),
    (0x10424d40, 0x10424f80, 142, "69c8c2e76feb24b4"),
    (0x10424f80, 0x10425240, 176, "3a39ad6107def867"),
    (0x10425240, 0x10425980, 463, "8998f7b9a4c25b6c"),
    (0x10425980, 0x10426550, 753, "e73ed0205e383f6a"),
    (0x10426550, 0x104265c0, 27, "8144af10f2086d1c"),
    (0x104265c0, 0x10426830, 156, "288afa0dfd1e2c46"),
    (0x10426830, 0x10426970, 79, "175f0042e724271a"),
    (0x10426970, 0x10426aa0, 75, "5ac9c2aba5529543"),
    (0x10426aa0, 0x10427df0, 1233, "8fcf8f728e835153"),
    (0x10427df0, 0x10428650, 535, "c5ef51853f7d06cf"),
    (0x10428650, 0x104286c0, 26, "db416c5c5091795b"),
    (0x104286c0, 0x10428750, 34, "9eb6ab10543b2a83"),
    (0x10428750, 0x10428ab0, 216, "0f04d38b475cb006"),
    (0x10428ab0, 0x10429a10, 982, "4d62bfb8cc807649"),
    (0x10429a10, 0x1042a780, 859, "666f2ffac44b8e9c"),
    (0x1042a780, 0x1042a810, 35, "41775ddb11e90bab"),
    (0x1042a810, 0x1042abb0, 232, "c8e5d6fd4e95eb0d"),
    (0x1042abb0, 0x1042ae20, 156, "9bfbb281923bb29e"),
    (0x1042ae20, 0x1042ae90, 27, "89b86adb4aacf2a3"),
    (0x1042ae90, 0x1042b3a0, 321, "153725be7571c9c4"),
    (0x1042b3a0, 0x1042b500, 86, "1db186ea124df71d"),
    (0x1042b500, 0x1042b580, 32, "fd3058eea73f71e6"),
    (0x1042b580, 0x1042bae0, 344, "7fd0244b8aff1b8d"),
    (0x1042bae0, 0x1042bf70, 289, "6369f4b6e5e08664"),
    (0x1042bf70, 0x1042c2c0, 210, "8e3158075c16f8a1"),
    (0x1042c2c0, 0x1042c6d0, 259, "55b9bfacc495ab1e"),
    (0x1042c6d0, 0x1042ca40, 217, "b8f335455071d15f"),
    (0x1042ca40, 0x1042e110, 1459, "1dce18c73143e172"),
    (0x1042e110, 0x1042ed80, 793, "79374496bdd02f30"),
    (0x1042ed80, 0x1042ee90, 65, "677d166a3b5667c4"),
    (0x1042ee90, 0x1042efa0, 66, "fea454cfb6634498"),
    (0x1042efa0, 0x1042f380, 246, "d6033ed48f55406d"),
    (0x1042f380, 0x1042f670, 188, "0b3da8c67c8c392f"),
    (0x1042f670, 0x1042f7c0, 82, "5698bde5b23d509e"),
    (0x1042f7c0, 0x1042fb30, 217, "9e2cb17f8dd4111f"),
    (0x1042fb30, 0x1042fe10, 182, "fd5112c155ef834d"),
)
ROUTINES = tuple((entry, end) for entry, end, _, _ in ORIGINAL)
INCLUDES = ROOT / "native/transfer_screens_routines.inc"
ROUTINE = re.compile(
    r'TRANSFER_SCREENS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),'
    r'\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(transfer_screens_routines_\w+\.inc)"')
CALLBACKS = (
    (0x108b942c, 0x1042fb30),
    (0x108b9434, 0x1042f7c0),
    (0x108b943c, 0x1042f380),
    (0x108b9444, 0x1042efa0),
    (0x108b944c, 0x1042e110),
    (0x108b9454, 0x1042ca40),
    (0x108b945c, 0x1042bf70),
    (0x108b9464, 0x1042bae0),
    (0x108b946c, 0x1042b580),
    (0x108b9474, 0x1042b3a0),
    (0x108b947c, 0x1042ae90),
    (0x108b9484, 0x1042abb0),
    (0x108b948c, 0x1042a810),
    (0x108b9494, 0x10429a10),
    (0x108b949c, 0x10428ab0),
    (0x108b94a4, 0x104286c0),
    (0x108b94ac, 0x10428650),
    (0x108b94b4, 0x10427df0),
    (0x108b94bc, 0x10426aa0),
    (0x108b94c4, 0x10426830),
    (0x108b94cc, 0x104265c0),
    (0x108b94d4, 0x10424d40),
    (0x108b94dc, 0x104249e0),
    (0x108b94e4, 0x10425240),
    (0x108b94ec, 0x10424f80),
    (0x108b94f4, 0x10424220),
    (0x108b94fc, 0x10423270),
    (0x108b9504, 0x10423040),
    (0x108b950c, 0x10422e40),
    (0x108b9514, 0x10422b50),
    (0x108b951c, 0x10422a00),
    (0x108b9524, 0x10422600),
    (0x108b952c, 0x10421ee0),
    (0x108b9534, 0x10421dd0),
    (0x108b953c, 0x10421ce0),
    (0x108b9544, 0x10422310),
    (0x108b954c, 0x10421fb0),
    (0x108b9554, 0x10421be0),
    (0x108b955c, 0x10421af0),
    (0x108b9564, 0x10421a20),
    (0x108b956c, 0x10421960),
    (0x108b9574, 0x10421070),
    (0x108b957c, 0x10420e40),
    (0x108b9584, 0x10420b50),
    (0x108b958c, 0x104204f0),
    (0x108b9594, 0x1041f8f0),
    (0x108b959c, 0x1041f3a0),
    (0x108b95a4, 0x1041eb10),
    (0x108b95ac, 0x1041e760),
    (0x108b95b4, 0x1041b600),
    (0x108b95bc, 0x10417b60),
)


def routine_lines(path=INCLUDES):
    """Missing root means no implementation yet; explicit includes must exist."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text()
                          for name in INCLUDE.findall(text))


def rewritten(path=INCLUDES, drafts=False):
    """Validate all registrations, returning exactly the requested stage."""
    records = [(stage, int(entry, 16), int(end, 16), fingerprint)
               for stage, entry, end, _, fingerprint in ROUTINE.findall(routine_lines(path))]
    bounds = dict(ROUTINES)
    for _, entry, end, _ in records:
        if bounds.get(entry) != end:
            raise ValueError(f"transfer_screens_routines.inc: {entry:x}-{end:x} is not a routine of TRANSFER_SCREENS.CPP")
    if len({entry for _, entry, _, _ in records}) != len(records):
        raise ValueError("A TRANSFER_SCREENS.CPP routine is listed twice")
    return sorted((entry, end, fingerprint) for stage, entry, end, fingerprint in records
                  if drafts or stage == "ROUTINE")


TRANSFER_SCREENS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_transfer_screens_index(db):
    """Pin the whole original inventory, including entries not implemented yet."""
    expected = {}
    for entry, end, count, fingerprint in ORIGINAL:
        records = routine_records(db, entry, end)
        actual = digest(records)
        if len(records) != count or actual != fingerprint:
            raise ValueError(f"TRANSFER_SCREENS.CPP routine {entry:x} changed in the export (digest {actual})")
        expected[entry] = fingerprint
    for entry, _, fingerprint in rewritten(drafts=True):
        if fingerprint != expected[entry]:
            raise ValueError(f"TRANSFER_SCREENS.CPP routine {entry:x} registration has a stale digest")


def require_transfer_screens_production(owners, routines=None):
    """Separate delivery gate: incremental implementation is not completeness."""
    if routines is None:
        routines = rewritten()
    if [(entry, end) for entry, end, _ in routines] != list(ROUTINES):
        raise ValueError("TRANSFER_SCREENS.CPP production implementation is incomplete")
    for entry, _ in ROUTINES:
        if [owner for at, owner in owners if at == entry] != ["transfer_screens_adapter"]:
            raise ValueError(f"TRANSFER_SCREENS.CPP production owner missing or conflicting at {entry:x}")


def transfer_screens_hook(address):
    return f"if (try_modern_transfer_screens(s, 0x{address:x})) return true;"


def main():
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routine addresses (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true")
    parser.add_argument("--require-production", action="store_true")
    args = parser.parse_args()
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    source = args.export / "instructions.sqlite"
    if not source.is_file():
        parser.error("Private TRANSFER_SCREENS export unavailable; inventory cannot be certified")
    with sqlite3.connect(f"file:{source}?mode=ro", uri=True) as db:
        require_transfer_screens_index(db)
        if args.require_production:
            from compile_recomp import OWNER_REPLACEMENTS
            require_transfer_screens_production(OWNER_REPLACEMENTS)
        wanted = {int(entry, 16) for entry in args.entries}
        done = {entry for entry, _, _ in rewritten()}
        drafts = {entry for entry, _, _ in rewritten(drafts=True)} - done
        for entry, end, count, fingerprint in ORIGINAL:
            if wanted and entry not in wanted:
                continue
            stage = " (C++)" if entry in done else " (draft)" if entry in drafts else ""
            print(f"0x{entry:x} 0x{end:x} {fingerprint} {count:5}{stage}")


if __name__ == "__main__":
    main()

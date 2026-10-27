"""NEWS.CPP's original entry inventory and registered C++23 replacements.

NEWS follows NATIONAL_TEAMS_SCREENS and precedes NEWS_SCREENS. Its assertion
pool is TOC -0x5100; the item constructor before the first assertion-bearing
routine initializes the same 0xe4-byte records used by the owned queue/storage
code. All 33 entries are direct-call targets and have ordinary LR prologues.
The sole indirect transfer is a local 56-label switch table, not callbacks.
Unimplemented entries remain in ROUTINES without being production owners.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x102bdf90, 0x102ce710)
ROUTINES = (
    (0x102bdf90, 0x102be040), (0x102be040, 0x102c6b10),
    (0x102c6b10, 0x102c8690), (0x102c8690, 0x102c9880),
    (0x102c9880, 0x102ca450), (0x102ca450, 0x102cb340),
    (0x102cb340, 0x102cb4b0), (0x102cb4b0, 0x102cb570),
    (0x102cb570, 0x102cb630), (0x102cb630, 0x102cb7f0),
    (0x102cb7f0, 0x102cb930), (0x102cb930, 0x102cbb70),
    (0x102cbb70, 0x102cbc10), (0x102cbc10, 0x102cc030),
    (0x102cc030, 0x102cc140), (0x102cc140, 0x102cc280),
    (0x102cc280, 0x102cc400), (0x102cc400, 0x102cc5b0),
    (0x102cc5b0, 0x102cc730), (0x102cc730, 0x102cc8e0),
    (0x102cc8e0, 0x102cc9e0), (0x102cc9e0, 0x102ccb20),
    (0x102ccb20, 0x102ccdd0), (0x102ccdd0, 0x102cd0c0),
    (0x102cd0c0, 0x102cd440), (0x102cd440, 0x102cd5f0),
    (0x102cd5f0, 0x102cd8b0), (0x102cd8b0, 0x102cda60),
    (0x102cda60, 0x102cdbf0), (0x102cdbf0, 0x102cdc80),
    (0x102cdc80, 0x102cde30), (0x102cde30, 0x102ce320),
    (0x102ce320, 0x102ce710),
)
ROUTINE = re.compile(
    r'NEWS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),'
    r'\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(news_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "news_routines.inc"


def routine_lines(path=INCLUDES):
    """Read registrations, failing for an absent explicitly included group."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text()
                          for name in INCLUDE.findall(text))


def rewritten(path=INCLUDES, drafts=False):
    """Return registered (entry, end, digest), without requiring completion.

    Validate draft registrations too, even when returning production only:
    malformed draft metadata must not disappear from a generation audit.
    """
    records = [(stage, int(entry, 16), int(end, 16), fingerprint)
               for stage, entry, end, _, fingerprint in ROUTINE.findall(routine_lines(path))]
    bounds = dict(ROUTINES)
    for _, entry, end, _ in records:
        if bounds.get(entry) != end:
            raise ValueError(f"news_routines.inc: {entry:x}-{end:x} is not a routine of NEWS.CPP")
    if len({entry for _, entry, _, _ in records}) != len(records):
        raise ValueError("A NEWS.CPP routine is listed twice")
    return sorted((entry, end, fingerprint) for stage, entry, end, fingerprint in records
                  if drafts or stage == "ROUTINE")


NEWS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_news_index(db):
    """Reject registered bodies whose original reference has changed."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"NEWS.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"news_routines.inc pins {expected})")


def news_hook(address):
    return f"if (try_modern_news(s, 0x{address:x})) return true;"


def main():
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routine addresses (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true")
    args = parser.parse_args()
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    wanted = {int(entry, 16) for entry in args.entries}
    done = {entry for entry, _, _ in rewritten()}
    drafts = {entry for entry, _, _ in rewritten(drafts=True)} - done
    with sqlite3.connect(args.export / "instructions.sqlite") as db:
        for entry, end in ROUTINES:
            if wanted and entry not in wanted:
                continue
            records = routine_records(db, entry, end)
            stage = " (C++)" if entry in done else " (draft)" if entry in drafts else ""
            print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

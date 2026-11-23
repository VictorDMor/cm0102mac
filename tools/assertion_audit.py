"""The assertion helpers: the file-name splitter 0x10739890 and the C assertion reporter 0x107755c0.

Two routines, 185 instructions, that the original's generated code reaches from thousands of sites and from nowhere else:
 * 0x10739890 (170 instructions; the inventory's "boundary" attribution) splits a Mac file path at its last ':' into volume, directory,
   name and extension (CodeWarrior's splitter). Every assertion site of the game calls it first, with its source file's name from the
   module's string pool, then reads the version text (0x10768810), formats the message (0x1077b720) and shows it (0x10769b60): 6,486
   `bl` sites. Its only callees are strlen and strcpy (0x1077dbd0, 0x1077dbf0). It reads four TOC slots (-0x3e78, -0x3e7c, -0x3e80,
   -0x3e84: the addresses of consecutive 256-byte statics its frame buffers start as copies of) that no other routine loads.
 * 0x107755c0 (15 instructions; MAIN.CPP by the call graph) is the C library's assertion reporter: fprintf(error stream, "Assertion (%s)
   failed in \"%s\" on line %d\\n", ...) then abort. 136 `bl` sites (the font engine's units, FONT.CPP, POL_CUP, QSORT4X, LEAGUE, DATABASE).
   It loads TOC slot -0x5a68 (the C library's FILE array, shared with the stream routines) and -0x3b6c (its format string, loaded by
   nothing else).

Which existing audited module each belongs to (tools/test_assertion_audit.py pins the evidence): none. 0x10739890 is not in UTILS.CPP
(0x10745e00..0x107475b0, whose pool slot is -0x3e34 and whose slots stop at -0x3e4c), not in graph_carbon.cpp (0x10768810.., which only
calls it) and not in string_core (whose bodies it calls); its neighbours 0x107390e0, 0x10739690, 0x107397d0 and 0x10739b40 are generated
and unowned, and its static arrays sit next to the ones 0x107390e0 and the engine initializer 0x10739f30 use (TOC -0x3e70). 0x107755c0
lies between stdlib_core's pinned bodies (0x10775560 and 0x10775600) but is none of them: that audit pins word utilities and the
comparator search by fingerprint and hooks them through try_modern_stdlib_core with an instruction budget, a different integration. Both
routines belong to the assertion path, so they form this small module with its own hook family (assertion_adapter).

Stack contract (tools/stack_contract.py module MAIN.CPP, the table of 2026-11-09): 0x10739890 is a leaver of its whole 0x460-byte frame.
The zero tests of MAIN's printf (0x10779594, 0x107795a0, 0x107795c8, 0x107795d8, 0x107795e0, 0x107795e8; "same"), the stream routines
0x10776940 / 0x10776a40 (reads at sp+56; the second "differs") and the import area's 0x10bf7bbc (17 offsets; "differs") read bytes its
buffer copies, parsing stores and callees' frames leave. Its C++ builds the whole frame (STACK_EXACT, compared on the dead stack too).
Nothing reads 0x107755c0's frame; it is compared exactly as well, for it hands its stack pointer to fprintf and abort.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
ROUTINES = ((0x10739890, 0x10739b3c), (0x107755c0, 0x107755fc))
ENTRIES = tuple(entry for entry, _ in ROUTINES)
# An alignment word inside 0x10739890 (after an unconditional jump) that the export does not hold.
UNREACHABLE = frozenset({0x10739a24})
# Both routines run on the dead stack exactly (native/assertion_stack_exact.inc).
STACK_EXACT = ENTRIES
ROUTINE = re.compile(r'ASSERTION_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(assertion_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "assertion_routines.inc"
# The adapter (compile_recomp.OWNER_REPLACEMENTS names it for each hooked entry).
OWNER = "assertion_adapter"


def routine_lines(path=INCLUDES):
    """The text of assertion_routines.inc with the block files it includes (empty while the file does not exist)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (ASSERTION_ROUTINE), with drafts also the ASSERTION_DRAFT
    ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"assertion_routines.inc: {entry:x}-{end:x} is not a routine of the assertion module")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("An assertion routine is listed twice")
    return found


ASSERTION_ENTRIES = ENTRIES
ASSERTION_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))

# The generator requires every owned entry routed (ASSERTION_ROUTINE): both are.
COMPLETE = True


def require_assertion_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple(entry for entry, _, _ in rewritten(path)) != ENTRIES:
        raise ValueError(f"the assertion module requires all {len(ENTRIES)} complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing assertion module instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"assertion routine {entry:x} changed in the export (digest {actual}, "
                             f"assertion_routines.inc pins {expected})")


def assertion_hook(address):
    return f"if (try_modern_assertion(s, 0x{address:x})) return true;"


def require_assertion_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = assertion_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in ASSERTION_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected assertion hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced assertion hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of assertion_routines.inc."""
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
    db = sqlite3.connect((args.export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
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

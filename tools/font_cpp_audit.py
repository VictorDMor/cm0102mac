"""FONT.CPP: the game's text drawing layer over the font engine (tools/font_engine_audit.py).

19 routines, 2,217 instructions, in two stretches of code: 0x1074f170..0x10751348 (17 routines, 2,140 instructions) and two helpers
0x1074d380..0x1074d4bc (27 + 50 instructions). The module is the game's wrapper around the T2K scaler: it loads a font file into a
font record, selects style and size, renders a glyph into a cache record, draws a string into a window's back buffer and keeps the
list of fonts the preferences screen offers. It is screen-side: no routine of it draws from the pooled random generator, reaches the
save files or the simulation (the callees outside the module are the C library, the allocator, the file layer, the localized text
formatter, the Toolbox resource manager through its glue, and the engine).

Bounds (the evidence the tests pin, tools/test_font_cpp_audit.py):
 * The string pool. The module's pool is 0x108a0164..0x108a0284, between SIPRINT.CPP's pool (before it) and AUTOGRID.C's, the font
   engine's first unit (after it): ".ttf", "FONT.CPP" at pool + 0x12 (the file name the assertions pass), the assertion texts, the
   fonts' file names, the names of the three data variables and the double constants of the skew (TOC slot -0x3e10). The slot -0x3e0c
   points at the pool's first string, and every load of it in the executable lies in the module (eleven loads, 0x1074f2c8 .. 0x10750d40, in eleven routines), as does
   every load of the float slot.
 * The code. The first routine that loads the pool is 0x1074f2c0; the routine before it, 0x1074f170, only calls it (and the two
   helpers) and is called by the module's own loader and font-list builder. The span ends where the engine's first routine
   (0x10751350, the audit of font_engine_audit.py) begins: the last routine, 0x10750f00 (the TrueType "name" table reader), has no
   pool user and is attributed by sequence (it is called by 0x10750d30 only and its end abuts the engine's first entry).
   Eleven of the 19 routines load a pool slot; the other eight are attributed by call graph.
 * The two helpers 0x1074d380 and 0x1074d450 (the resource file object: constructor, which saves the current resource file and opens
   a file's resource fork, and destructor) lie 0x1c00 bytes before the module, at the head of the block of compiler-generated
   inline and template bodies that the program's modules share (0x1074d4c0..0x1074f168: owners collection_lookup, tree_mutation,
   utility_object and routines of MAIN, FORMATION, BACKGROUND, graph_carbon, SCRMAN, TCPIP). Their only caller is 0x1074f170, which
   is FONT.CPP's; no other routine names them. They are part of the module's closure, attributed to it by that sole use
   (SIPRINT.CPP, which precedes them, uses the same Toolbox calls inline and never calls them).
 * The span is closed: 2,217 export instructions lie in the routines and every other word of the two stretches is an alignment
   no-op after an unconditional branch (six UNREACHABLE words) or padding between routines (at most 12 bytes); there is no `bctr`;
   no routine tail-calls another and no branch leaves its routine.

Callers from outside (EXTERNAL; the only code that names a routine of the module, by `bl`): graph_carbon.cpp's 0x1076bbf0 calls the
loader 0x1074f470 and the selection setter 0x10750a60, its 0x1076ca50 the string drawer 0x1074f800 (the ninth argument it passes on
the stack is 0 or the descriptor 0x108bdaac of its own callback 0x1076ca20), its 0x1076d1e0 the width/height routine 0x107500f0;
SETUP.CPP's 0x1033d030 and GAME_CONFIG.CPP's 0x1013f030 and 0x101406f0 call the font list's count 0x10750b00 and name 0x10750b60.
The record constructor 0x1074f390 and destructor 0x1074f410 are called through descriptors by the static array construction at
0x1076ea20 (TOC slots -0x5b18 and -0x5b1c); 0x1074d450 has a descriptor (0x108bd63c) that nothing loads.

The indirect call. 0x1074f800 calls its ninth argument (r29 = the stack word at 0xd8(r1), a descriptor) through the glue 0x10781320
at 0x1074f9d0 and 0x1074fa84 (r12 = r29), after setting r3-r6. The glue loads the code word from 0(r12) and the TOC word from 4(r12).
The only value the program passes is the descriptor at 0x108bdaac (TOC slot -0x3be4 of graph_carbon.cpp, loaded only at 0x1076cb1c)
or zero (the call is guarded by `cmplwi r29,0`); the recursive call at 0x1074f8fc passes zero. The descriptor's code is
0x1076ca20, an 11-instruction routine that reads r3 only (r4 is its first scratch register, nothing reads r5-r10): so the callback
reads at most one argument (tools/test_font_cpp_audit.py pins the descriptor, the single load and that no instruction of the callback
reads r4-r10 before writing it).

Production: font_cpp_routines*.inc lists each routed entry (FONT_CPP_ROUTINE) or draft (FONT_CPP_DRAFT) with the digest of its
original body; the generator hooks exactly the listed entries (FONT_CPP_HOOKED). ROUTED in tools/test_font_cpp_audit.py pins the
set; COMPLETE makes the generator refuse anything less than all 19 routed.
"""
import re
from pathlib import Path

from host_observed import require_calls_handled, require_handled
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
# The two stretches of code; MODULE spans from the first to the last for the tools that want one range.
SEGMENTS = ((0x1074d380, 0x1074d4bc), (0x1074f170, 0x10751348))
MODULE = (SEGMENTS[0][0], SEGMENTS[1][1])
# Groups (names without extension; the inventory tool appends .CPP to a key): the whole module is one source file.
MODULES = {"FONT": SEGMENTS}
ROUTINES = (
    # The resource file object (inline helpers at the head of the shared inline block).
    (0x1074d380, 0x1074d448), (0x1074d450, 0x1074d4bc),
    # The module proper.
    (0x1074f170, 0x1074f2c0), (0x1074f2c0, 0x1074f384), (0x1074f390, 0x1074f404), (0x1074f410, 0x1074f468),
    (0x1074f470, 0x1074f800), (0x1074f800, 0x107500ec), (0x107500f0, 0x10750200), (0x10750200, 0x10750320),
    (0x10750320, 0x10750580), (0x10750580, 0x107509f4), (0x10750a00, 0x10750a54), (0x10750a60, 0x10750afc),
    (0x10750b00, 0x10750b5c), (0x10750b60, 0x10750c9c), (0x10750ca0, 0x10750d28), (0x10750d30, 0x10750f00),
    (0x10750f00, 0x10751348),
)
# Words inside routines that the export leaves out because nothing reaches them: the compiler's alignment no-op after an
# unconditional branch.
UNREACHABLE = frozenset({0x1074ffc4, 0x10750178, 0x10750248, 0x10750618, 0x10750650, 0x10751304})
# The pool (its first string, TOC slot, the file name's offset) and the float slot.
POOL = 0x108a0164
POOL_SLOT = -0x3e0c
FLOAT_SLOT = -0x3e10
NAME_OFFSET = 0x12
# The callers outside the module (entry: sites of the `bl`s).
EXTERNAL = {
    0x1074f470: (0x1076bc3c, 0x1076bc58, 0x1076bc74, 0x1076bc90),
    0x1074f800: (0x1076cb7c, 0x1076cbb0, 0x1076cbe4, 0x1076cc18, 0x1076cc4c, 0x1076cc80, 0x1076ccb4, 0x1076cce8),
    0x107500f0: (0x1076d244, 0x1076d260, 0x1076d27c, 0x1076d298, 0x1076d2b4, 0x1076d2d0, 0x1076d2ec, 0x1076d308),
    0x10750a60: (0x1076bc30,),
    0x10750b00: (0x1033dbbc,),
    0x10750b60: (0x1013f280, 0x10140710, 0x1033da74, 0x1033dc64),
}
# Entries no `bl` names: called through descriptors (constructor and destructor of the font record, from the array construction
# 0x1076ea20). The resource file destructor 0x1074d450 is called by `bl` too and has a descriptor nothing loads.
POINTER_ONLY = frozenset({0x1074f390, 0x1074f410})
# The routines that leave their frame's bytes where the game's printf reads them (tools/stack_contract.py module FONT.CPP), with
# the first register their prologue saves: the stack contract's leavers, whose frames a rewrite must reproduce.
LEAVERS = {0x1074f800: 13, 0x107500f0: 26, 0x10750320: 24, 0x10750580: 23}
# The callback the string drawer's ninth argument can name, and the one load of the TOC slot that names it.
CALLBACK = {"descriptor": 0x108bdaac, "code": 0x1076ca20, "end": 0x1076ca4c, "slot": -0x3be4, "load": 0x1076cb1c}
ROUTINE = re.compile(r'FONT_CPP_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(font_cpp_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "font_cpp_routines.inc"
# The adapter (compile_recomp.OWNER_REPLACEMENTS names it for each hooked entry).
OWNER = "font_cpp_adapter"


def routine_lines(path=INCLUDES):
    """The text of font_cpp_routines.inc with the block files it includes (empty while the file does not exist)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


# The drawer 0x1074f800 and the glyph cache 0x10750580 hold host-observed instructions (tools/native_drawing_audit.py). The page runs each entry's
# own observation for every caller that reaches the entry through its page (the generator keeps both out of the in-place owner table); the C++
# passes the others itself: the drawer's 0x1074fb58 (a glyph inside the window), 0x107500b4 (the end of each character) and 0x107500e8 (the
# `blr`), the cache's 0x10750740 (the `bl` of the engine's render) and 0x107509f0 (the `blr`).
HANDLED_POINTS = {  # entry -> host-observed addresses its C++ (or the page, at the entry) passes; see tools/host_observed.py
    0x1074f800: frozenset({0x1074f800, 0x1074fb58, 0x107500b4, 0x107500e8}),
    0x10750580: frozenset({0x10750580, 0x10750740, 0x107509f0}),
}
# Host-observed entries a routed entry calls: the drawer's recursive call for the outline style and the string width's call of the cache are
# typed and run the callee's entry observation themselves; the drawer's call of the cache goes through the dispatcher (c.call: the page's).
HANDLED_CALLS = {0x1074f800: frozenset({0x1074f800, 0x10750580}), 0x107500f0: frozenset({0x10750580})}


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (FONT_CPP_ROUTINE), with drafts also the
    FONT_CPP_DRAFT ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest, stage) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _, stage in found:
        if bounds.get(entry) != end:
            raise ValueError(f"font_cpp_routines.inc: {entry:x}-{end:x} is not a routine of FONT.CPP")
        if stage == "ROUTINE":
            require_handled("font_cpp_routines.inc", entry, end, HANDLED_POINTS)
    if len({entry for entry, _, _, _ in found}) != len(found):
        raise ValueError("A FONT.CPP routine is listed twice")
    return [(entry, end, digest) for entry, end, digest, _ in found]


FONT_CPP_ENTRIES = tuple(entry for entry, _ in ROUTINES)
FONT_CPP_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))

# The generator requires every entry routed (FONT_CPP_ROUTINE): all 19 are, and a draft or a missing line stops the generation.
COMPLETE = True  # the three routines the host's HD drawing observes inside were drafts 2026-11-23 and are routed again (HANDLED_POINTS)


def require_font_cpp_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("FONT.CPP requires all 19 complete production entries")
    for entry, end, _ in rewritten(path):
        require_calls_handled("font_cpp_routines.inc", db, entry, end, HANDLED_CALLS)
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing FONT.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"FONT.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"font_cpp_routines.inc pins {expected})")


def font_cpp_hook(address):
    return f"if (try_modern_font_cpp(s, 0x{address:x})) return true;"


def require_font_cpp_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = font_cpp_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in FONT_CPP_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected FONT.CPP hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced FONT.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of font_cpp_routines*.inc."""
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

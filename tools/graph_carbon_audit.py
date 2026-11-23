"""graph_carbon.cpp: the game's drawing layer over the Mac (surfaces, rectangles, text entry points, the assertion box).

55 routines, 6,218 instructions, in one stretch 0x10768810..0x1076eaa0. Window and offscreen surface set-up over Carbon (CreateNewWindow,
NewGWorld, CopyBits, QDFlushPortBuffer), the 15-bit pixel primitives (rectangles, lines, ellipses, fills, fades), the bitmap font files
(.fnt, loaded through the C library) and the entry points that draw and measure text through FONT.CPP, the resource-held window
position, the cursors and the message box the assertions end in (0x10769b60). It is screen-side: no routine draws from the pooled
random generator or reaches the simulation; its persistence is the window-position resource and the .rgm region file (0x1076b240,
0x1076b2f0), which are host state, not the game's save. The inventory's earlier estimate (33 routines, 3,883 instructions) counted
only the routines the text-drawing closure of floor.json reaches.

Bounds (the evidence tools/test_graph_carbon_audit.py pins):
 * The string pool. TOC slot -0x3bd0 holds 0x108a0b08, whose first string is "graph_carbon.cpp" (the file name the assertions
   pass); the pool runs to MAIN.CPP's (0x108a0c60, "MAIN.CPP" at 0x108a0c80) and follows QSORT8X.C's ("qsort8x.c" at 0x108a0abf,
   then a byte table at 0x108a0acc that slot -0x3be0 gives to the font loader 0x1076c0b0). All 21 loads of the slot
   lie in the module, from 0x10768838 (0x10768820) to 0x1076e614 (0x1076e5f0), in eighteen routines.
 * The TOC. The module's own slots are -0x3bfc..-0x3bd0 (twelve: arrays and statics, the three jump tables, the callback
   descriptor, the font array) and some in -0x5b1c..-0x5ae4; every one is loaded only inside the module. Its small data (variables
   reached directly from r2, no slot) is -0x3580..-0x354f; MAIN.CPP's begins at -0x354e. No access to either lies outside.
 * The descriptors. Five code descriptors, 0x108bda9c..0x108bdabc, sit between the font engine's last (0x108bda94) and MAIN.CPP's
   first (0x108bdac4): the destructor of the font array 0x1076ea70 (registered by the static initializer 0x1076ea20), two cleanup
   destructors named only by the exception tables (0x1076d1a0, 0x1076c010), the text callback 0x1076ca20 (FONT.CPP's ninth
   argument, pinned by tools/test_font_cpp_audit.py) and the resource callback 0x1076e9d0 (loaded at 0x10768a60). They are the
   module's only routines no `bl` names.
 * The first routine, 0x10768810, returns the address of a global (TOC -0x5b20) and follows QSORT8X.C's last (0x10768804, end
   0x10768808 in tools/qsort4x_audit.py) after alignment; the assertion paths of every module call it. It is attributed by that
   sequence and by being the only entry between two units (no pool load: the module's first loader is 0x10768820).
 * The last routine, 0x1076ea70 (12 instructions), is the destructor of the font array the static initializer 0x1076ea20 builds
   (it loads the same slot -0x3bd8 as the module's loaders). 0x1076ea20 itself (17 instructions) is pinned by
   tools/engine_initialization_audit.py and stays there: it is listed here as part of the span, never routed. The next entry,
   0x1076eaa0, is a MoreFiles helper that MAIN.CPP's audit excludes ("the MoreFiles helpers before main()").
 * The span is closed: 6,218 export instructions lie in the routines (four alignment words inside routines are unreachable);
   every other word is alignment padding between routines (at most 12 bytes); no routine tail-calls another; three `bctr`s are the
   jump tables of the text drawers (0x1076ca50, 0x1076d1e0, 0x1076d470, eight entries each, inside their routines).

Host observation (tools/host_observed.py): the host's drawing view watches 22 instructions of this module (tools/native_drawing_audit.py: twelve
entries, which the generated page observes before its hook, and ten "exit" instructions inside the pixel primitives and the text drawer), its adaptive
layout patches 21 (tools/adaptive_gui_audit.py: the message panel's 17, the ellipse's two, the assertion box's one) and the profiler marks three of the
message panel. A routed routine must pass each interior one itself: rewritten() refuses a routed entry holding a host-observed address that HANDLED_POINTS
does not list for it (the entry's own address is the page's: the generator keeps such an entry out of the in-place owner table), and the index check
refuses a routed entry that calls a host-observed entry HANDLED_CALLS does not list. The checkers' boundary logs (native/native_drawing_log.hpp) compare
what the host reads at each point and what its adaptive hook is asked and answers, on the original canvas and a wider one (CM_GUI_CANVAS).

Stack contract (tools/stack_contract.py module graph_carbon.cpp): twelve routines are leavers (LEAVERS: entry -> first saved register);
0x1076b6e0 reads stale stack (its own zero tests), so a rewrite of a leaver keeps the leavers' frames and held registers.

Production: graph_carbon_routines*.inc lists each routed entry (GRAPH_CARBON_ROUTINE) or draft (GRAPH_CARBON_DRAFT) with the digest of
its original body; the generator hooks exactly the listed entries (GRAPH_CARBON_HOOKED). ROUTED in tools/test_graph_carbon_audit.py pins the
set; COMPLETE makes the generator refuse anything less than every owned routine routed.
"""
import re
from pathlib import Path

from host_observed import require_calls_handled, require_handled
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
# One stretch; MODULE is written as a literal pair because tools/game_inventory.py reads it from the source (and names the module from this
# file's name, matching the known name "graph_carbon.cpp").
MODULE = (0x10768810, 0x1076eaa0)
SEGMENTS = (MODULE,)
ROUTINES = (
    (0x10768810, 0x10768818), (0x10768820, 0x10768990), (0x10768990, 0x107689dc), (0x107689e0, 0x10768b38),
    (0x10768b40, 0x10768c4c), (0x10768c50, 0x10768c84), (0x10768c90, 0x10768dc0), (0x10768dc0, 0x10768e04),
    (0x10768e10, 0x107690e4), (0x107690f0, 0x107691a4), (0x107691b0, 0x107692e4), (0x107692f0, 0x10769460),
    (0x10769460, 0x107697b4), (0x107697c0, 0x107698c8), (0x107698d0, 0x1076991c), (0x10769920, 0x10769980),
    (0x10769980, 0x10769b4c), (0x10769b50, 0x10769b5c), (0x10769b60, 0x1076a140), (0x1076a140, 0x1076a574),
    (0x1076a580, 0x1076a788), (0x1076a790, 0x1076a8ec), (0x1076a8f0, 0x1076ad50), (0x1076ad50, 0x1076b018),
    (0x1076b020, 0x1076b13c), (0x1076b140, 0x1076b238), (0x1076b240, 0x1076b2e8), (0x1076b2f0, 0x1076b450),
    (0x1076b450, 0x1076b6d4), (0x1076b6e0, 0x1076b7dc), (0x1076b7e0, 0x1076b824), (0x1076b830, 0x1076bb14),
    (0x1076bb20, 0x1076bb34), (0x1076bb40, 0x1076bbe8), (0x1076bbf0, 0x1076c010), (0x1076c010, 0x1076c04c),
    (0x1076c050, 0x1076c0a4), (0x1076c0b0, 0x1076c974), (0x1076c980, 0x1076ca18), (0x1076ca20, 0x1076ca4c),
    (0x1076ca50, 0x1076d198), (0x1076d1a0, 0x1076d1e0), (0x1076d1e0, 0x1076d470), (0x1076d470, 0x1076d598),
    (0x1076d5a0, 0x1076d6e0), (0x1076d6e0, 0x1076e158), (0x1076e160, 0x1076e5e4), (0x1076e5f0, 0x1076e7c0),
    (0x1076e7c0, 0x1076e868), (0x1076e870, 0x1076e904), (0x1076e910, 0x1076e918), (0x1076e920, 0x1076e9d0),
    (0x1076e9d0, 0x1076ea20), (0x1076ea20, 0x1076ea64), (0x1076ea70, 0x1076eaa0),
)
# The static initializer of the module's file-scope objects (the array of eight font records): pinned and owned by
# tools/engine_initialization_audit.py, never routed here.
OTHER_AUDIT = {0x1076ea20: "engine_initialization_audit"}
# The routines this audit owns (54: 6,201 instructions).
OWNED = tuple(entry for entry, _ in ROUTINES if entry not in OTHER_AUDIT)
# Words inside routines that the export leaves out because nothing reaches them: the compiler's alignment no-op after an
# unconditional branch.
UNREACHABLE = frozenset({0x1076a0e4, 0x1076aea0, 0x1076afc0, 0x1076b0fc})
# The pool (its first string is the file name), the byte table before it, and the small data span (r2-relative, no slot).
POOL = 0x108a0b08
POOL_SLOT = -0x3bd0
TABLE_SLOT = -0x3be0
SMALL_DATA = (-0x3580, -0x354f)
OWN_SLOTS = tuple(range(-0x3bfc, -0x3bd0 + 4, 4))
# Entries no `bl` names: called through descriptors or exception tables. Descriptor address -> (entry, the TOC slot that loads it or None).
DESCRIPTORS = {
    0x108bda9c: (0x1076ea70, -0x3bf8), 0x108bdaa4: (0x1076d1a0, None), 0x108bdaac: (0x1076ca20, -0x3be4),
    0x108bdab4: (0x1076c010, None), 0x108bdabc: (0x1076e9d0, -0x5af0),
}
POINTER_ONLY = frozenset(entry for entry, _ in DESCRIPTORS.values())
# The three jump tables of the text drawers: slot -> (table, routine).
JUMP_TABLES = {-0x3be8: (0x1091c2c8, 0x1076ca50), -0x3bec: (0x1091c2e8, 0x1076d1e0), -0x3bf0: (0x1091c308, 0x1076d470)}
# The routines that leave their frame's bytes where a reader reads them (tools/stack_contract.py module graph_carbon.cpp), with the
# first register their prologue saves: the stack contract's leavers.
LEAVERS = {0x107692f0: 26, 0x10769980: 24, 0x1076a140: 19, 0x1076a790: 30, 0x1076b7e0: 31, 0x1076c0b0: 24, 0x1076ca50: 13,
           0x1076d1e0: 26, 0x1076d470: 30, 0x1076d6e0: 17, 0x1076e160: 18, 0x1076e5f0: 24}
# The callers outside the module (entry: number of `bl` sites, digest of the sorted site list). Every other entry is called from
# inside the module only, or through a descriptor.
EXTERNAL = {
    0x10768810: (6402, 'd4e3913bbd8fea01'), 0x10768820: (3, 'ae028ddb2db1c81c'), 0x10768990: (1, '5ddf293312530642'),
    0x107692f0: (19, '2cecc672e8351f02'), 0x10769460: (9, '0af80278ce170825'), 0x10769920: (1, 'e9bca63935aa9fda'),
    0x10769980: (2, '9014bba7f2a1b85f'), 0x10769b50: (3, '2278e69a0632d6b8'), 0x10769b60: (6479, '81c0cb22d3e78c2e'),
    0x1076a140: (2, 'd50bed87fb7d4460'), 0x1076a580: (2, 'd9961c3c06b03810'), 0x1076a790: (11, 'd53e67811d0d9c3b'),
    0x1076ad50: (1, 'b2f83bb7924c19ae'), 0x1076b020: (6, '1743453ea301fd66'), 0x1076b140: (25, 'e8f2d22a5be6c567'),
    0x1076b2f0: (8, '82a794988e5b1b18'), 0x1076b7e0: (31, 'c35c020eed60e240'), 0x1076bb20: (65, '6e7fd5bf7e0b75fe'),
    0x1076bb40: (1, '41a34ceb3499d5c0'), 0x1076d1e0: (62, '16b6de0a214816ef'), 0x1076d470: (168, '08cd4f4ee159aebe'),
    0x1076d6e0: (6, '977dc6e2368adf3f'), 0x1076e160: (5, 'debef9727583f098'), 0x1076e5f0: (1, 'e48af3153860382b'),
    0x1076e910: (1, 'ca21f0fc7b2f6fd5'), 0x1076e920: (7, 'd2e93a4a6cadcded'), 0x1076ea20: (1, '25c6acaa993aacf9'),
}
# The routines outside the module it calls with `bl` (the C library, the file layer, the allocator, FONT.CPP, MAIN.CPP, the shared
# inline block, the localized text and the assertion reporter); the Toolbox glue stubs (0x1077fad8..0x10781320) are in GLUE_NAMES.
CALLEES = (
    0x10718f60, 0x10727890, 0x10738c00, 0x10738c20, 0x10738d10, 0x10738e00, 0x10739890, 0x10739de0, 0x10739ef0, 0x10746280,
    0x10746470, 0x1074d5a0, 0x1074d8d0, 0x1074d910, 0x1074da90, 0x1074daa0, 0x1074dab0, 0x1074daf0, 0x1074f470, 0x1074f800,
    0x107500f0, 0x10750a60, 0x1076f7d0, 0x1076fd90, 0x1076fe80, 0x10770ee0, 0x10772df0, 0x10773320, 0x10773420, 0x10773520,
    0x107735a0, 0x107735f0, 0x10773610, 0x10775130, 0x10775170, 0x10775200, 0x10775550, 0x10775fe0, 0x10776360, 0x107766f0,
    0x10778300, 0x1077b590, 0x1077b720, 0x1077dbd0, 0x1077dbf0, 0x1077dc60,
)
GLUE_NAMES = (
    'AddResource', 'BlockMoveData', 'BringToFront', 'CFRelease', 'CFStringCreateWithCStringNoCopy', 'CFStringGetSystemEncoding',
    'ChangedResource', 'CopyBits', 'CreateNewWindow', 'DisposeCCursor', 'DisposeGWorld', 'DisposeWindow', 'ForeColor', 'GetCCursor',
    'GetCursor', 'GetGrayRgn', 'GetMainDevice', 'GetMaxDevice', 'GetPixBaseAddr', 'GetPort', 'GetPortBitMapForCopyBits',
    'GetPortPixMap', 'GetRegionBounds', 'GetResource', 'GetWindowBounds', 'GetWindowEventTarget', 'GetWindowPort', 'InitCursor',
    'InsetRect', 'InstallEventHandler', 'InvalMenuBar', 'LocalToGlobal', 'LockPixels', 'NewEventHandlerUPP', 'NewGWorld',
    'NewHandle', 'PaintRect', 'PtInRect', 'PtInRgn', 'QDFlushPortBuffer', 'ReleaseResource', 'SetCCursor', 'SetCursor', 'SetPort',
    'SetPt', 'SetRect', 'SetWindowTitleWithCFString', 'ShowWindow', 'WriteResource',
)
ROUTINE = re.compile(r'GRAPH_CARBON_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(graph_carbon_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "graph_carbon_routines.inc"
# The adapter (compile_recomp.OWNER_REPLACEMENTS names it for each hooked entry).
OWNER = "graph_carbon_adapter"


def routine_lines(path=INCLUDES):
    """The text of graph_carbon_routines.inc with the block files it includes (empty while the file does not exist)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


# Host-observed instructions a routed entry's body holds that its C++ (or every caller of the entry) passes (tools/host_observed.py):
# entry -> addresses. An entry's own address is handled by the generated page (it runs the host's code before the owner hook, and the
# generator keeps the entry out of the in-place owner table); the others are passed by the C++ itself. A routed entry holding any
# address missing here is refused (rewritten()); a draft is not checked: the game runs the original for it.
# drawing boundaries (tools/native_drawing_audit.py): text: draw_text runs native_drawing_boundary at 0x1076d194 itself; remap:
# fade_rectangle at 0x107696cc and 0x107697b0; colours: capture_rectangle at 0x1076b138, blit_bitmap at 0x1076b234; pixels: the exits
# 0x107698c4, 0x10769b48, 0x1076a570, 0x1076a784 and 0x1076b014 in graph_carbon_fills.cpp / graph_carbon_lines.cpp.
HANDLED_POINTS = {
    0x10769460: frozenset({0x10769460, 0x107696cc, 0x107697b0}), 0x107697c0: frozenset({0x107697c0, 0x107698c4}),
    0x10769980: frozenset({0x10769980, 0x10769b48}), 0x1076a140: frozenset({0x1076a140, 0x1076a570}),
    0x1076a580: frozenset({0x1076a580, 0x1076a784}), 0x1076b020: frozenset({0x1076b020, 0x1076b138}),
    0x1076b140: frozenset({0x1076b140, 0x1076b234}), 0x1076b7e0: frozenset({0x1076b7e0}), 0x1076c0b0: frozenset({0x1076c0b0}),
    0x1076c980: frozenset({0x1076c980}), 0x1076ca50: frozenset({0x1076ca50, 0x1076d194}),
    # The ellipse passes its drawing exit itself and the adaptive canvas width at the two `li` that load the table's unset left edge
    # (0x1076add4, 0x1076ae34: the host's hook, graph_carbon_lines.cpp `loaded`).
    0x1076ad50: frozenset({0x1076ad50, 0x1076add4, 0x1076ae34, 0x1076b014}),
    # The assertion box takes the room beside its panel from the adaptive hook at the `subfic` 0x10769c40 (graph_carbon_box.cpp).
    0x10769b60: frozenset({0x10769c40}),
    # The message panel passes the profiling markers inside (0x1076e7a4, 0x1076e7bc; the page emits the entry's) and calls the adaptive hook at
    # its 17 `li` (the bottom bar: graph_carbon_text_panel.cpp `loaded`).
    0x1076e5f0: frozenset({0x1076e5f0, 0x1076e6c0, 0x1076e6c4, 0x1076e6c8, 0x1076e6f8, 0x1076e704, 0x1076e71c, 0x1076e720, 0x1076e724, 0x1076e738,
                           0x1076e748, 0x1076e750, 0x1076e778, 0x1076e77c, 0x1076e784, 0x1076e798, 0x1076e79c, 0x1076e7a0, 0x1076e7a4, 0x1076e7bc}),
}
# Host-observed entries a routed entry calls, and how that stays right: through c.call the page's point runs; a typed call passes the
# point itself (the checker's boundary log compares the points seen on both sides, once per call).
HANDLED_CALLS = {
    0x10768820: frozenset({0x1076a580}), 0x107692f0: frozenset({0x107697c0}), 0x10769460: frozenset({0x1076b020, 0x1076b7e0}),
    0x1076a580: frozenset({0x1076a140}), 0x1076bbf0: frozenset({0x1076c0b0}), 0x1076c050: frozenset({0x1076c980}),
    0x1076c0b0: frozenset({0x1076c980}), 0x1076ca50: frozenset({0x1074f800, 0x1076a140}),
    0x1076d6e0: frozenset({0x10769980, 0x1076ad50, 0x1076a580, 0x1076a140}), 0x1076e160: frozenset({0x1076ca50}),
    0x10769b60: frozenset({0x1076b020, 0x1076b140, 0x1076b7e0}), 0x1076ad50: frozenset({0x1076a140}), 0x1076e5f0: frozenset({0x1076a580}),
}


def profiled_inside(entry, end):
    """The profiling markers strictly inside a routine (the entry's own is emitted by the generated page)."""
    from profiling_boundaries import PROFILE_BOUNDARIES
    return sorted(a for a in PROFILE_BOUNDARIES if entry < a < end)


def observed_inside(entry, end):
    """The native-drawing observation points strictly inside a routine (the entry's own is run by the generated page)."""
    from native_drawing_audit import NATIVE_DRAWING_BOUNDARIES
    return sorted(a for a in NATIVE_DRAWING_BOUNDARIES if entry < a < end)


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (GRAPH_CARBON_ROUTINE), with drafts also the
    GRAPH_CARBON_DRAFT ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest, stage) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _, stage in found:
        if bounds.get(entry) != end:
            raise ValueError(f"graph_carbon_routines.inc: {entry:x}-{end:x} is not a routine of graph_carbon.cpp")
        if entry in OTHER_AUDIT:
            raise ValueError(f"graph_carbon_routines.inc: {entry:x} belongs to {OTHER_AUDIT[entry]}")
        if stage == "ROUTINE":
            require_handled("graph_carbon_routines.inc", entry, end, HANDLED_POINTS)
    if len({entry for entry, _, _, _ in found}) != len(found):
        raise ValueError("A graph_carbon.cpp routine is listed twice")
    return [(entry, end, digest) for entry, end, digest, _ in found]


GRAPH_CARBON_ENTRIES = tuple(entry for entry, _ in ROUTINES)
GRAPH_CARBON_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))

# The generator requires every owned entry routed (GRAPH_CARBON_ROUTINE): all 54 are.
COMPLETE = True  # the three routines the host's HD drawing and adaptive layout observe inside were drafts 2026-11-23 and are routed again (HANDLED_POINTS)


def require_graph_carbon_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple(entry for entry, _, _ in rewritten(path)) != OWNED:
        raise ValueError(f"graph_carbon.cpp requires all {len(OWNED)} complete production entries")
    for entry, end, _ in rewritten(path):
        require_calls_handled("graph_carbon_routines.inc", db, entry, end, HANDLED_CALLS)
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing graph_carbon.cpp instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"graph_carbon.cpp routine {entry:x} changed in the export (digest {actual}, "
                             f"graph_carbon_routines.inc pins {expected})")


def graph_carbon_hook(address):
    return f"if (try_modern_graph_carbon(s, 0x{address:x})) return true;"


def require_graph_carbon_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = graph_carbon_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in GRAPH_CARBON_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected graph_carbon.cpp hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced graph_carbon.cpp hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of graph_carbon_routines*.inc."""
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
        stage = " (C++)" if entry in done else " (draft)" if entry in drafted else " (other audit)" if entry in OTHER_AUDIT else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage}")


if __name__ == "__main__":
    main()

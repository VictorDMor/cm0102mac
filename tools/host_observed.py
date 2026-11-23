"""Instructions where the host runs code while the generated pages execute the original (the guard for rewritten routines).

The host draws text and shapes in HD and resizes parts of the GUI by observing or patching specific original instructions as the
generated pages execute them. A hand-written replacement of a routine that CONTAINS such an instruction skips it unless its C++ does
what the page does there, so a module that routes such a routine loses the host feature (2026-11-23: HD text and the adaptive bottom bar).
Each rewritten module's audit refuses a routed (non-draft) entry whose body holds any of these addresses unless its table
HANDLED_POINTS lists the entry -> address pair, which says the C++ (or every caller of the entry) passes that point itself and
that the module's checker compares the host-observed state there (native/*_checks.cpp: the boundary log).

The sets (labels):
 * drawing:  tools/native_drawing_audit.py NATIVE_DRAWING_BOUNDARIES; the page calls native_drawing_boundary(s, pc) at each, and the HD
             layer reads registers, stack words and memory there (native/native_drawing.cpp observe_drawing).
 * adaptive: tools/adaptive_gui_audit.py ADAPTIVE_GUI_INSTRUCTIONS and ADAPTIVE_BACKGROUND_BOUNDARY; the page runs a host hook
             (adaptive_gui_instruction / draw_adaptive_background) in place of or around the instruction.
 * profile:  tools/profiling_boundaries.py PROFILE_BOUNDARIES; the page emits a profiling marker (CM_PROFILE_EVENT).
 * capture:  tools/register_context_audit.py CAPTURE_OBSERVATIONS; capture hooks of the register-context tool (diagnostic builds).
 * watched:  host code compares the PC with the address (native_drawing.cpp, adaptive_gui.cpp, platform*, Objective-C++ and the
             Match Debugger); only these host sources count: a rewrite's own `pc ==` tests are not host code.
 * special:  tools/compile_recomp.py SPECIAL_LOWERINGS (instructions generate_page lowers specially). The audits cannot import the
             generator (it imports them), so the audits refuse the sets above and tools/test_host_observed.py runs the full set,
             this one included, over every routed entry of the four modules.

The entry itself counts: the page runs the host's code before its owner hook, so an entry point is handled only when every caller reaches
the entry through its page (the generator keeps such an entry out of the in-place owner table); a typed C++ call into it skips the page
and must pass the point itself.
"""
import functools
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NATIVE = ROOT / "native"
# The same comparison compile_recomp.observed_instructions looks for.
PC_COMPARISON = re.compile(r"\bpc\s*[!=]=\s*(0x[0-9a-fA-F]{8})\b|\b(0x[0-9a-fA-F]{8})\s*[!=]=\s*[\w>.-]*\bpc\b")
HOST_SOURCE = re.compile(r"(native_drawing|adaptive_gui|platform).*\.(cpp|hpp|mm)$|.*debugger.*\.(cpp|hpp)$|.*\.mm$")


def watched_by_host(native=NATIVE):
    """Addresses the host's own sources compare the PC with."""
    found = set()
    for path in sorted(native.glob("*")):
        if HOST_SOURCE.fullmatch(path.name) and not re.search(r"checks|fixture|characterization|benchmark", path.name):
            for match in PC_COMPARISON.finditer(path.read_text(errors="replace")):
                found.add(int(match.group(1) or match.group(2), 16))
    return frozenset(found)


@functools.lru_cache(maxsize=8)
def host_points(special=frozenset(), native=NATIVE):
    """{address: sorted labels} of every instruction where the host runs code."""
    from native_drawing_audit import NATIVE_DRAWING_BOUNDARIES
    from adaptive_gui_audit import ADAPTIVE_GUI_INSTRUCTIONS, ADAPTIVE_BACKGROUND_BOUNDARY
    from profiling_boundaries import PROFILE_BOUNDARIES
    from register_context_audit import CAPTURE_OBSERVATIONS
    sets = {"drawing": NATIVE_DRAWING_BOUNDARIES, "adaptive": (*ADAPTIVE_GUI_INSTRUCTIONS, ADAPTIVE_BACKGROUND_BOUNDARY),
            "profile": PROFILE_BOUNDARIES, "capture": CAPTURE_OBSERVATIONS, "watched": watched_by_host(native), "special": special}
    points = {}
    for label, addresses in sets.items():
        for address in addresses:
            points.setdefault(address, set()).add(label)
    return {address: tuple(sorted(labels)) for address, labels in points.items()}


def points_inside(entry, end, special=frozenset()):
    """[(address, labels)] of the host points in the routine entry..end, the entry included."""
    return sorted((a, labels) for a, labels in host_points(special).items() if entry <= a < end)


def require_handled(module, entry, end, handled, special=frozenset()):
    """Refuse a routed entry holding a host point that `handled` (entry -> addresses its C++ or callers pass) does not list."""
    missing = [(a, labels) for a, labels in points_inside(entry, end, special) if a not in handled.get(entry, ())]
    if missing:
        named = ", ".join("%x %s" % (a, "+".join(labels)) for a, labels in missing)
        raise ValueError(f"{module}: {entry:x} holds host-observed instructions its C++ would skip or does not pass ({named}): "
                         f"list them in HANDLED_POINTS once the C++ passes them and the checker's boundary log compares them, "
                         f"or keep the routine a draft")


def calls_into_points(db, entry, end, special=frozenset()):
    """{call site: callee} of the direct calls (`bl`) in the routine entry..end whose target is a host point: the entries the host's
    code runs at when the page reaches them. A typed C++ call of such a callee skips the page and must pass the point itself."""
    from tactics_audit import routine_records
    points = host_points(special)
    return {r["address"]: r["flows"][0] for r in routine_records(db, entry, end)
            if r.get("flow") == "UNCONDITIONAL_CALL" and r.get("flows") and r["flows"][0] in points}


def require_calls_handled(module, db, entry, end, handled_calls, special=frozenset()):
    """Refuse a routed entry that calls a host-observed entry unless `handled_calls` (entry -> callee addresses) lists the callee: the
    listing says the C++ reaches it through the dispatcher (c.call: the page's point runs) or passes the point itself (a typed call),
    and that the checker's boundary log compares the points seen on both sides."""
    missing = sorted({callee for callee in calls_into_points(db, entry, end, special).values()
                      if callee not in handled_calls.get(entry, ())})
    if missing:
        raise ValueError(f"{module}: {entry:x} calls host-observed entries {', '.join(f'{a:x}' for a in missing)} that "
                         f"HANDLED_CALLS does not list (a typed C++ call would skip the point the page runs there)")


def scan(inventory, special=frozenset()):
    """Host points inside every owned routine of an inventory (tools/game_inventory.py's inventory.json), and the direct calls from owned routines
    into host-observed entries: [(owner, routine entry, address, labels)] and [(owner, routine entry, call site, callee, labels)]."""
    import bisect
    import json
    import sqlite3
    from tactics_audit import routine_records
    points = host_points(special)
    addresses = sorted(points)
    routines = [(int(r["entry"], 16), int(r["end"], 16), r["owner"]) for r in json.loads(Path(inventory).read_text())["routines"] if r["owner"]]
    inside, calls = [], []
    db = sqlite3.connect((ROOT / ".local/current/export/instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
    for entry, end, owner in routines:
        for i in range(bisect.bisect_left(addresses, entry), len(addresses)):
            if addresses[i] >= end:
                break
            inside.append((owner, entry, addresses[i], points[addresses[i]]))
        for site, callee in calls_into_points(db, entry, end, special).items():
            calls.append((owner, entry, site, callee, points[callee]))
    return inside, calls


def main():
    """python3 tools/host_observed.py scan <inventory.json>: list the host points in owned routines and the owned routines that call them."""
    import sys
    if len(sys.argv) != 3 or sys.argv[1] != "scan":
        raise SystemExit(main.__doc__)
    from compile_recomp import SPECIAL_LOWERINGS
    inside, calls = scan(sys.argv[2], SPECIAL_LOWERINGS)
    for owner, entry, address, labels in inside:
        print(f"{owner} {entry:x} holds {address:x} {'+'.join(labels)}")
    for owner, entry, site, callee, labels in calls:
        print(f"{owner} {entry:x} calls {callee:x} at {site:x} {'+'.join(labels)}")


if __name__ == "__main__":
    main()

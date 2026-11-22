"""PLAYER_REGEN.CPP: player regeneration (newgen players, youth-academy promotions, staff retirement).

47 routines at 0x102d8230..0x102eb6f8, 19,660 instructions. The unit is named by its assertion file
name: its string pool (TOC -0x50a0, 0x107f63a4) holds the assertion preamble, "PLAYER_REGEN.CPP" at +0x37 (0x107f63db)
and, after the version format and "Error", the unit's texts ("... Average Age ... Total Players", "Invalid staff_id
for regen", the youth-academy promotion news). Thirteen routines load the pool slot, the first at 0x102d824c (routine
0x102d8230) and the last at 0x102ead7c (0x102ead70); the 37 assertion sites that print a source line (sprintf
0x1077b720 with the version format, line in r8) read 127..6927 and rise with the address, so the file's functions follow
the code and no other unit's routine is interleaved. The previous unit, OFFICIALS_MANAGER.CPP (pool TOC -0x50cc), has its
last assertion (line 2657) at 0x102d5a50; the next, PLAYER_SEARCH.CPP (pool TOC -0x5078), its first (line 176) at 0x102eb9b0.
The 2026-11-21 inventory gave the unit 52 routines, 0x102d5b30..0x102eb6f8 (evidence 13, filled 30, graph 9); the audit
moves seven out (the player-config loader cluster, below) and two in (the comparators 0x102eb4e0, 0x102eb550) and keeps
the other 45 (tools/test_player_regen_audit.py checks it).

- The unit's TOC slots form one block -0x50bc..-0x5098 (10 slots, every load inside the unit): five jump-table slots
  (-0x50bc..-0x50ac; the tables are the 77 words 0x108cea10..0x108ceb43, one `bctr` each in 0x102e1f20 (2), 0x102e21c0 (2)
  and 0x102e27e0 (1)), two string slots (-0x50a8 0x107fa5f0 and -0x50a4 0x107fa5e8, the unit's last texts), the pool
  (-0x50a0) and the two assertion-preamble slots (-0x509c, -0x5098; loaded once, by 0x102d8230). The neighbours' blocks
  end at -0x50c0 (see below; its string 0x107f57c4 lies before this unit's preamble 0x107f6381) and start at -0x5094
  (0x108ceb44, the first word after this unit's jump tables, loaded only by PLAYER_SEARCH code at 0x10300114).
- Descriptors. The unit's code is reached through direct calls (56 sites from 17 entries, every one a `bl` of an entry)
  and two qsort callbacks, 0x102eb4e0 and 0x102eb550, whose descriptors 0x108b8db4 and 0x108b8dbc follow OFFICIALS_MANAGER's
  run (the last is 0x108b8dac -> 0x102d59e0) and precede the next unit's (0x108b8dc4 -> 0x102fe010). Their TOC slots are
  -0x632c (one load, in 0x102d8920) and -0x6328 (four, in 0x102d86c0, 0x102d8830, 0x102d8920 and 0x102df020): only this
  unit's routines take their address, and a table of descriptors (0x108b5764, 0x108b5768) also names them. They sit
  between 0x102ead70 and the helpers 0x102eb610 and 0x102eb670 that only this unit's routines call (0x102eb610 also
  MANAGER_MANAGER's 0x101b12f0; 0x102eb670 calls 0x102df020), so they are the unit's (the inventory's heuristic left them out).
- No tail call, no branch out of a routine, no fallthrough from outside, no symbol. The 42 distinct words that point into
  the span from data (0x108cea10..0x108ceb43) are jump-table labels inside the three routines above, not functions. 17
  words inside routines that the export leaves out are the compiler's alignment no-ops after an unconditional branch
  (UNREACHABLE); between routines only zero padding.

Not decided, so not entries (AMBIGUOUS_NEIGHBOURS):
- The player-config loader, seven routines 0x102d5b30..0x102d8224 (2,486 instructions): the entry 0x102d5b30 (nine callers in
  GAME.CPP, NATIONAL_TEAMS.CPP and SETUP.CPP) calls the parser 0x102d6770, which reads "player_setup.cfg" (INJURY,
  RETIREMENT, FUTURE_TRANSFER, LOAN... records, the texts "Player config (CFG_...)") and calls 0x102dee70 of this unit. The
  routines are a closed group (the helpers 0x102d5bb0, 0x102d5dc0, 0x102d7dc0, 0x102d7e80, 0x102d81a0 are called only from
  inside it). Its one private TOC slot, -0x50c0, points to "CM3_DATA" (0x107f57c4): a string of the literal region of
  OFFICIALS_MANAGER.CPP (its preamble 0x107f5711 .. this unit's preamble 0x107f6381), and in the 24 audited units whose pool is in
  the literal area no text string the code loads lies before the unit's own pool (the other three load binary tables there;
  tools/test_player_regen_audit.py checks it). The heuristic put the group
  here ("graph"); the evidence puts it with the previous unit or a unit of its own, and nothing proves it either way, so
  it is pinned and attributed to neither. It asserts nothing and prints no line number. Whoever audits OFFICIALS_MANAGER.CPP
  decides it.
- Four routines 0x102eb700, 0x102eb8b0, 0x102eb8f0, 0x102eb930 (169 instructions) between this unit's last routine and
  PLAYER_SEARCH.CPP's first assertion (0x102eb9b0): destructor-like callbacks (0x102eb930 calls PLAYER_SEARCH's 0x102fe360). Their
  descriptors 0x108b8e04..0x108b8e1c are taken by SCOUT_MANAGER.CPP code (slots -0x6304, -0x6300) and SEARCH_SCREENS.CPP calls
  0x102eb8f0; no routine of this unit calls or names them. They have no TOC slot of their own, so only that evidence
  attributes them (to PLAYER_SEARCH.CPP, SCOUT_MANAGER.CPP or a shared weak symbol); pinned, attributed to none.

Production: the 39 entries of the blocks regen_queries (0x102de3c0, 0x102de3f0, 0x102dbe90, 0x102eb4e0, 0x102eb550,
0x102eb610), candidate_score (0x102da630, 0x102dc410, 0x102dbf00), regen_lists (0x102deb70, 0x102ded90, 0x102dbeb0, 0x102dea50,
0x102dec50, 0x102dee70, 0x102df020, 0x102df1a0, 0x102df6d0, 0x102eb670), club_placement (0x102ea670, 0x102e9b90, 0x102e99f0,
0x102ea3c0, 0x102e98a0, 0x102dc000), detach (0x102e1f20, 0x102e21c0), retire_decision (0x102dcb20, 0x102dd910, 0x102de6e0,
0x102de470), persistence (0x102ea870, 0x102ead70), news_posting (0x102df780, 0x102e0090, 0x102e13c0) and news_texts (0x102dfbc0,
0x102e0510, 0x102e15b0) are C++ (native/player_regen*.cpp). native/player_regen_routines.inc
(with its block files) lists each C++ routine with the digest of the original body it replaces (PLAYER_REGEN_ROUTINE routes it,
PLAYER_REGEN_DRAFT only runs it in the check tools); the generator hooks exactly the listed entries (PLAYER_REGEN_HOOKED), so every
other entry runs its generated body. ROUTED in tools/test_player_regen_audit.py pins the set.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x102d8230, 0x102eb6f8)
ROUTINES = (
    (0x102d8230, 0x102d85c0), (0x102d85c0, 0x102d86b4), (0x102d86c0, 0x102d8830),
    (0x102d8830, 0x102d8918), (0x102d8920, 0x102da630), (0x102da630, 0x102daba8),
    (0x102dabb0, 0x102dbe84), (0x102dbe90, 0x102dbea8), (0x102dbeb0, 0x102dbef8),
    (0x102dbf00, 0x102dbff8), (0x102dc000, 0x102dc404), (0x102dc410, 0x102dcb20),
    (0x102dcb20, 0x102dd904), (0x102dd910, 0x102de3c0), (0x102de3c0, 0x102de3e4),
    (0x102de3f0, 0x102de464), (0x102de470, 0x102de6d4), (0x102de6e0, 0x102dea48),
    (0x102dea50, 0x102deb70), (0x102deb70, 0x102dec48), (0x102dec50, 0x102ded84),
    (0x102ded90, 0x102dee68), (0x102dee70, 0x102df020), (0x102df020, 0x102df198),
    (0x102df1a0, 0x102df6cc), (0x102df6d0, 0x102df778), (0x102df780, 0x102dfbb4),
    (0x102dfbc0, 0x102e0084), (0x102e0090, 0x102e0508), (0x102e0510, 0x102e13c0),
    (0x102e13c0, 0x102e15a8), (0x102e15b0, 0x102e1f18), (0x102e1f20, 0x102e21b4),
    (0x102e21c0, 0x102e27d4), (0x102e27e0, 0x102e4524), (0x102e4530, 0x102e9894),
    (0x102e98a0, 0x102e99e4), (0x102e99f0, 0x102e9b88), (0x102e9b90, 0x102ea3b8),
    (0x102ea3c0, 0x102ea664), (0x102ea670, 0x102ea870), (0x102ea870, 0x102ead68),
    (0x102ead70, 0x102eb4d4), (0x102eb4e0, 0x102eb54c), (0x102eb550, 0x102eb610),
    (0x102eb610, 0x102eb670), (0x102eb670, 0x102eb6f8),
)

# The routines between OFFICIALS_MANAGER.CPP and this unit and between this unit and PLAYER_SEARCH.CPP, attributed to
# none of the three (entry, end, instructions, digest; see the docstring).
AMBIGUOUS_NEIGHBOURS = (
    (0x102d5b30, 0x102d5bb0, 32, '90f05212d8bd6ef1'), (0x102d5bb0, 0x102d5dbc, 131, 'a6b231b91236ce08'),
    (0x102d5dc0, 0x102d6770, 620, '4c3c2e8e48cf3121'), (0x102d6770, 0x102d7dbc, 1427, 'b364798f2eaff8bb'),
    (0x102d7dc0, 0x102d7e78, 44, '1fc09e6ce69ab315'), (0x102d7e80, 0x102d819c, 199, '9017cdf8c6e50449'),
    (0x102d81a0, 0x102d8224, 33, '019c9a2629d3a0af'),
    (0x102eb700, 0x102eb8ac, 107, '3bb642684eb7ec78'), (0x102eb8b0, 0x102eb8ec, 15, '52ce23a2eb27967a'),
    (0x102eb8f0, 0x102eb92c, 15, 'ffbf0eb754ba97c0'), (0x102eb930, 0x102eb9b0, 32, 'fb03142a7b0ce6c6'),
)
ROUTINE = re.compile(r'PLAYER_REGEN_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(player_regen_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "player_regen_routines.inc"
# Words inside routines that the export leaves out because nothing reaches them: the compiler's
# alignment no-op after an unconditional branch.
UNREACHABLE = frozenset({0x102d82bc, 0x102dace8, 0x102daef0, 0x102dbf34, 0x102de3fc, 0x102deb7c, 0x102ded9c, 0x102e1fbc,
                         0x102e2044, 0x102e2234, 0x102e28f0, 0x102e4b08, 0x102e98e0, 0x102e9a44, 0x102ea4f0, 0x102ea55c,
                         0x102eb65c})
# The adapter (compile_recomp.OWNER_REPLACEMENTS will name it for each hooked entry).
OWNER = "player_regen_adapter"


def routine_lines(path=INCLUDES):
    """The text of player_regen_routines.inc with the block files it includes (empty while the file
    does not exist: nothing is routed)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (PLAYER_REGEN_ROUTINE), with
    drafts also the PLAYER_REGEN_DRAFT ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"player_regen_routines.inc: {entry:x}-{end:x} is not a routine of PLAYER_REGEN.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A PLAYER_REGEN.CPP routine is listed twice")
    return found


# The entries of the unit's inventory, and those the generator hooks: the ones player_regen_routines*.inc lists (an
# unlisted entry keeps its generated body and carries no hook; a draft declines outside a checker).
PLAYER_REGEN_ENTRIES = tuple(entry for entry, _ in ROUTINES)
PLAYER_REGEN_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))


# Whether the generator requires every entry routed (PLAYER_REGEN_ROUTINE); False while an entry is still
# unwritten or a draft. Nine of the 47 are routed (the pilot blocks).
COMPLETE = False


def require_player_regen_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("PLAYER_REGEN.CPP requires all 47 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing PLAYER_REGEN.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"PLAYER_REGEN.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"player_regen_routines.inc pins {expected})")


def player_regen_hook(address):
    return f"if (try_modern_player_regen(s, 0x{address:x})) return true;"


def require_player_regen_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = player_regen_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in PLAYER_REGEN_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected PLAYER_REGEN.CPP hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced PLAYER_REGEN.CPP hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of player_regen_routines*.inc."""
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

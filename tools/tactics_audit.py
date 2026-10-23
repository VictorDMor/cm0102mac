"""The original tactics system and the routines of it that run as C++.

ROUTINES lists every routine of the original's FORMATION.CPP, TACTICS.CPP and
TACTICS_SCREENS.CPP as (entry, end): the modules' bounds come from the source
file names their asserts print and the string pools their code reads, the
entries from the analysed functions plus every direct call or function-pointer
target; each routine runs to the next entry.

native/tactics_routines.inc lists the routines rewritten so far, each with a
digest of its original instructions: TACTICS_ROUTINE lines are verified and
generation hooks them (try_modern_tactics); TACTICS_DRAFT lines run only in
check tools (cm-world-replay --check-routines). Generation fails when the
export no longer matches a digest.
"""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULES = {"FORMATION": (0x10110c80, 0x10118ec0), "TACTICS": (0x103acf80, 0x103b3830),
           "TACTICS_SCREENS": (0x103b3830, 0x103cf5f0)}
ROUTINES = (
    # FORMATION.CPP: 47 routines
    (0x10110c80, 0x10110f40), (0x10110f40, 0x10111240), (0x10111240, 0x10111310),
    (0x10111310, 0x10111630), (0x10111630, 0x10111840), (0x10111840, 0x10111890), (0x10111890, 0x10111ba0),
    (0x10111ba0, 0x10111d30), (0x10111d30, 0x10111e60), (0x10111e60, 0x101124c0), (0x101124c0, 0x10112690),
    (0x10112690, 0x10112850), (0x10112850, 0x10112a00), (0x10112a00, 0x10112b90), (0x10112b90, 0x10112d20),
    (0x10112d20, 0x10112f00), (0x10112f00, 0x10113160), (0x10113160, 0x10113430), (0x10113430, 0x10113550),
    (0x10113550, 0x10113940), (0x10113940, 0x10113b60), (0x10113b60, 0x10113e10), (0x10113e10, 0x10113e30),
    (0x10113e30, 0x101149f0), (0x101149f0, 0x10114a00), (0x10114a00, 0x10114b10), (0x10114b10, 0x10114b50),
    (0x10114b50, 0x10114ba0), (0x10114ba0, 0x10114c00), (0x10114c00, 0x10114d40), (0x10114d40, 0x10115000),
    (0x10115000, 0x10115cd0), (0x10115cd0, 0x10116560), (0x10116560, 0x10116780), (0x10116780, 0x10116970),
    (0x10116970, 0x10116c90), (0x10116c90, 0x10116f80), (0x10116f80, 0x101171b0), (0x101171b0, 0x10117360),
    (0x10117360, 0x10117490), (0x10117490, 0x10117690), (0x10117690, 0x10117fa0), (0x10117fa0, 0x10118180),
    (0x10118180, 0x10118210), (0x10118210, 0x101182a0), (0x101182a0, 0x10118a40), (0x10118a40, 0x10118ec0),

    # TACTICS.CPP: 30 routines
    (0x103acf80, 0x103af060), (0x103af060, 0x103af130), (0x103af130, 0x103af480), (0x103af480, 0x103af570),
    (0x103af570, 0x103af6c0), (0x103af6c0, 0x103af800), (0x103af800, 0x103af9d0), (0x103af9d0, 0x103afb90),
    (0x103afb90, 0x103afe90), (0x103afe90, 0x103afff0), (0x103afff0, 0x103b04a0), (0x103b04a0, 0x103b0540),
    (0x103b0540, 0x103b05c0), (0x103b05c0, 0x103b07c0), (0x103b07c0, 0x103b0a00), (0x103b0a00, 0x103b0b20),
    (0x103b0b20, 0x103b0bf0), (0x103b0bf0, 0x103b0d40), (0x103b0d40, 0x103b0e70), (0x103b0e70, 0x103b10f0),
    (0x103b10f0, 0x103b1370), (0x103b1370, 0x103b14d0), (0x103b14d0, 0x103b1670), (0x103b1670, 0x103b17b0),
    (0x103b17b0, 0x103b1960), (0x103b1960, 0x103b1b10), (0x103b1b10, 0x103b1c00), (0x103b1c00, 0x103b1d00),
    (0x103b1d00, 0x103b3290), (0x103b3290, 0x103b3830),

    # TACTICS_SCREENS.CPP: 157 routines
    (0x103b3830, 0x103b3b20), (0x103b3b20, 0x103b3b50), (0x103b3b50, 0x103b3bd0), (0x103b3bd0, 0x103b3c40),
    (0x103b3c40, 0x103b3c50), (0x103b3c50, 0x103baa00), (0x103baa00, 0x103baa70), (0x103baa70, 0x103baae0),
    (0x103baae0, 0x103bab00), (0x103bab00, 0x103bab40), (0x103bab40, 0x103baba0), (0x103baba0, 0x103babe0),
    (0x103babe0, 0x103bac20), (0x103bac20, 0x103bac60), (0x103bac60, 0x103baca0), (0x103baca0, 0x103bacd0),
    (0x103bacd0, 0x103bace0), (0x103bace0, 0x103bacf0), (0x103bacf0, 0x103bad00), (0x103bad00, 0x103bad10),
    (0x103bad10, 0x103bad40), (0x103bad40, 0x103bada0), (0x103bada0, 0x103badd0), (0x103badd0, 0x103bade0),
    (0x103bade0, 0x103bae10), (0x103bae10, 0x103bae40), (0x103bae40, 0x103bae50), (0x103bae50, 0x103bae80),
    (0x103bae80, 0x103bae90), (0x103bae90, 0x103baec0), (0x103baec0, 0x103baee0), (0x103baee0, 0x103be410),
    (0x103be410, 0x103be420), (0x103be420, 0x103be490), (0x103be490, 0x103be4e0), (0x103be4e0, 0x103be550),
    (0x103be550, 0x103be560), (0x103be560, 0x103be5b0), (0x103be5b0, 0x103be620), (0x103be620, 0x103be630),
    (0x103be630, 0x103be680), (0x103be680, 0x103be6f0), (0x103be6f0, 0x103be700), (0x103be700, 0x103be750),
    (0x103be750, 0x103be7c0), (0x103be7c0, 0x103be7d0), (0x103be7d0, 0x103be820), (0x103be820, 0x103be880),
    (0x103be880, 0x103be8c0), (0x103be8c0, 0x103be910), (0x103be910, 0x103be980), (0x103be980, 0x103be990),
    (0x103be990, 0x103be9a0), (0x103be9a0, 0x103be9b0), (0x103be9b0, 0x103be9c0), (0x103be9c0, 0x103be9f0),
    (0x103be9f0, 0x103bea00), (0x103bea00, 0x103bea50), (0x103bea50, 0x103bea90), (0x103bea90, 0x103beae0),
    (0x103beae0, 0x103beb40), (0x103beb40, 0x103bebb0), (0x103bebb0, 0x103becb0), (0x103becb0, 0x103c0cc0),
    (0x103c0cc0, 0x103c0cf0), (0x103c0cf0, 0x103c0d10), (0x103c0d10, 0x103c11b0), (0x103c11b0, 0x103c1340),
    (0x103c1340, 0x103c14a0), (0x103c14a0, 0x103c1980), (0x103c1980, 0x103c1c00), (0x103c1c00, 0x103c1d40),
    (0x103c1d40, 0x103c1da0), (0x103c1da0, 0x103c1f20), (0x103c1f20, 0x103c1f30), (0x103c1f30, 0x103c2010),
    (0x103c2010, 0x103c4830), (0x103c4830, 0x103c4870), (0x103c4870, 0x103c4b10), (0x103c4b10, 0x103c5940),
    (0x103c5940, 0x103c5960), (0x103c5960, 0x103c5990), (0x103c5990, 0x103c59c0), (0x103c59c0, 0x103c59f0),
    (0x103c59f0, 0x103c5a40), (0x103c5a40, 0x103c5a90), (0x103c5a90, 0x103c5b70), (0x103c5b70, 0x103c5e60),
    (0x103c5e60, 0x103c5f00), (0x103c5f00, 0x103c60a0), (0x103c60a0, 0x103c6180), (0x103c6180, 0x103c6210),
    (0x103c6210, 0x103c6310), (0x103c6310, 0x103c6450), (0x103c6450, 0x103c64c0), (0x103c64c0, 0x103c6bd0),
    (0x103c6bd0, 0x103c6cb0), (0x103c6cb0, 0x103c6d80), (0x103c6d80, 0x103c6e80), (0x103c6e80, 0x103c7050),
    (0x103c7050, 0x103c7120), (0x103c7120, 0x103c7260), (0x103c7260, 0x103c72c0), (0x103c72c0, 0x103c7380),
    (0x103c7380, 0x103c7550), (0x103c7550, 0x103c7690), (0x103c7690, 0x103c78a0), (0x103c78a0, 0x103c79d0),
    (0x103c79d0, 0x103c7ad0), (0x103c7ad0, 0x103c7c10), (0x103c7c10, 0x103c7df0), (0x103c7df0, 0x103c80b0),
    (0x103c80b0, 0x103c8160), (0x103c8160, 0x103c81e0), (0x103c81e0, 0x103c82a0), (0x103c82a0, 0x103c8360),
    (0x103c8360, 0x103c83f0), (0x103c83f0, 0x103c8480), (0x103c8480, 0x103c85b0), (0x103c85b0, 0x103c8680),
    (0x103c8680, 0x103c87b0), (0x103c87b0, 0x103c88c0), (0x103c88c0, 0x103c8a10), (0x103c8a10, 0x103c8b60),
    (0x103c8b60, 0x103c8c70), (0x103c8c70, 0x103c8d80), (0x103c8d80, 0x103c8e70), (0x103c8e70, 0x103c9260),
    (0x103c9260, 0x103c95b0), (0x103c95b0, 0x103c9720), (0x103c9720, 0x103c9810), (0x103c9810, 0x103c9960),
    (0x103c9960, 0x103c9970), (0x103c9970, 0x103c9aa0), (0x103c9aa0, 0x103c9bd0), (0x103c9bd0, 0x103c9d00),
    (0x103c9d00, 0x103c9f40), (0x103c9f40, 0x103ca120), (0x103ca120, 0x103ca260), (0x103ca260, 0x103ca390),
    (0x103ca390, 0x103ca4e0), (0x103ca4e0, 0x103ca730), (0x103ca730, 0x103ca8d0), (0x103ca8d0, 0x103cac10),
    (0x103cac10, 0x103cad40), (0x103cad40, 0x103caf20), (0x103caf20, 0x103cb0b0), (0x103cb0b0, 0x103cb3c0),
    (0x103cb3c0, 0x103cb760), (0x103cb760, 0x103cba60), (0x103cba60, 0x103cba70), (0x103cba70, 0x103cbb30),
    (0x103cbb30, 0x103ce330), (0x103ce330, 0x103ce790), (0x103ce790, 0x103cea00),
    (0x103cea00, 0x103cf110), (0x103cf110, 0x103cf5f0),
)
ROUTINE = re.compile(r'TACTICS_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*(\w+),\s*"([0-9a-f]+)"')


INCLUDE = re.compile(r'#include "(tactics_routines_\w+\.inc)"')


def rewritten(path=ROOT / "native" / "tactics_routines.inc", drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (TACTICS_ROUTINE), with drafts also the TACTICS_DRAFT ones check tools run."""
    text = path.read_text()
    text += "".join((path.parent / name).read_text() for name in INCLUDE.findall(text))
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in ROUTINE.findall(text)
                   if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"tactics_routines.inc: {entry:x}-{end:x} is not a routine of the tactics system")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A tactics routine is listed twice")
    return found


TACTICS_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def digest(records):
    canonical = [[r["address"], r["assembly"], r["flow"], r.get("flows", []), r.get("fallthrough"), r["ops"]]
                 for r in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True).encode()).hexdigest()[:16]


def routine_records(db, entry, end):
    return [json.loads(row[0]) for row in db.execute(
        "SELECT body FROM instructions WHERE address >= ? AND address < ? ORDER BY address", (entry, end))]


def require_tactics_index(db):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    for entry, end, expected in rewritten(drafts=True):
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"Tactics routine {entry:x} changed in the export (digest {actual}, "
                             f"tactics_routines.inc pins {expected})")


def tactics_hook(address):
    return f"if (try_modern_tactics(s, 0x{address:x})) return true;"


def main():
    """Print each routine's digest, for new lines of tactics_routines.inc."""
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="*", help="routines to print (default: all)")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", action="store_true", help="print every entry alone (for cm-world-replay --census)")
    args = parser.parse_args()
    if args.entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    db = sqlite3.connect(args.export / "instructions.sqlite")
    wanted = {int(e, 16) for e in args.entries}
    done = {entry for entry, _, _ in rewritten()}
    drafted = {entry for entry, _, _ in rewritten(drafts=True)} - done
    for entry, end in ROUTINES:
        if wanted and entry not in wanted:
            continue
        records = routine_records(db, entry, end)
        module = next(name for name, (low, high) in MODULES.items() if low <= entry < high)
        stage = " (C++)" if entry in done else " (draft)" if entry in drafted else ""
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5} {module}{stage}")


if __name__ == "__main__":
    main()

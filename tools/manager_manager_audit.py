"""MANAGER_MANAGER.CPP: the manager manager (manager records, jobs, the
competition result tables, staff decisions).

Bounds (closed 2026-11-07, tools/test_manager_manager_audit.py re-derives them):
- Start 0x10193630. KEY_NATION.CPP ends before it: its static initializer
  0x101935b0 (reached from the start-up table) fills the 34 x 0x4c table at
  TOC -0x7b44 that KEY_NATION.CPP's 0x10193550 walks, and the destructor
  0x101935f0 (a function pointer only) sits in KEY_NATION.CPP's vector group:
  the data segment groups each unit's transition vectors in unit order, each
  group by descending address, and 0x108b8b5c holds 0x101935f0 next to
  KEY_NATION.CPP's 0x10193010, before MANAGER_MANAGER.CPP's 0x101b8af0,
  0x101b75c0, 0x101b6ff0.
- End 0x101c42f0, where MANAGER_SCREENS.CPP begins: 0x101c42f0 loads the
  unit slots -0x5304..-0x530c, contiguous with MANAGER_SCREENS.CPP's source
  pool -0x5310. 0x101c42c0 (no unit slot) belongs here: its only caller
  0x105d6ac0 passes it the same receiver (*r27) it passes to this unit's
  0x101bb900.
- Every unit slot the module loads is MANAGER_MANAGER.CPP's: -0x5360..-0x5420,
  with its source-name pool at -0x5368, between MANAGER_SCREENS.CPP's slots
  and KEY_NATION.CPP's pool -0x5424.

ROUTINES are (entry, end, instructions): entries are direct-call and
function-pointer targets, each body is what control flow reaches from its
entry (through its switch tables) and end is the word after its last
instruction. Nine jump-table cases of 0x101b3c90 (bctr 0x101b4b20) and
0x101b83f0 (bctr 0x101b84c8) are also discovery roots (tools/
discovery_roots.txt); they are folded into those routines, as
game_inventory.fold_case_roots does. UNREACHABLE are words after an
unconditional `b` that no flow reaches (alignment nops and leftover branches).

native/manager_manager_routines.inc lists the routines rewritten so far, each
with a digest of its original instructions: MANAGER_MANAGER_ROUTINE lines are
verified and generation hooks them (try_modern_manager_manager);
MANAGER_MANAGER_DRAFT lines run only in check tools.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10193630, 0x101c42f0)
CONTRACTS = (
    (0x10193630, 0x10193a18, 249, "26cc1625c7cff249"),
    (0x10193a20, 0x10193b10, 60, "fe9b652592bc5835"),
    (0x10193b10, 0x10194380, 532, "c809b36b5bcbf832"),
    (0x10194380, 0x10194db8, 644, "6c73f364d51d25f6"),
    (0x10194dc0, 0x10195e0c, 1043, "e6cfd32dbb35e56b"),
    (0x10195e10, 0x10196388, 350, "7419c39d48831a5f"),
    (0x10196390, 0x10197648, 1198, "6128ccdb2447b3cc"),
    (0x10197650, 0x101976a4, 20, "1ce385392ca42954"),
    (0x101976b0, 0x1019795c, 171, "5fe80e781e5055a1"),
    (0x10197960, 0x10197df4, 287, "80f1ecf7149d885e"),
    (0x10197e00, 0x10197e80, 32, "02fd04f217214bb0"),
    (0x10197e80, 0x10197f00, 32, "43688c9a5fd36ddc"),
    (0x10197f00, 0x101980a0, 104, "0135381648802850"),
    (0x101980a0, 0x1019c584, 4352, "84886375fb12e8b2"),
    (0x1019c590, 0x1019d320, 866, "00e83db3c5306d44"),
    (0x1019d320, 0x1019dcf4, 629, "8859175203eafda9"),
    (0x1019dd00, 0x1019e8e8, 762, "7993d4d694ad1e7c"),
    (0x1019e8f0, 0x1019e98c, 39, "ccf0343d988d9554"),
    (0x1019e990, 0x101a05c0, 1804, "98f124d3054d3e02"),
    (0x101a05c0, 0x101a0c1c, 407, "04286b4265577276"),
    (0x101a0c20, 0x101a0f50, 204, "a8ec4e01f4286710"),
    (0x101a0f50, 0x101a2478, 1351, "6c3d2981ce5699d9"),
    (0x101a2480, 0x101a27d0, 212, "ffd94102e3b7ee08"),
    (0x101a27d0, 0x101a2eb0, 440, "f5f6150a096e15a3"),
    (0x101a2eb0, 0x101a3350, 296, "85354286dbb10948"),
    (0x101a3350, 0x101a8ac4, 5597, "281636765e3dcde2"),
    (0x101a8ad0, 0x101a9df4, 1225, "5d365332df70de2e"),
    (0x101a9e00, 0x101aa50c, 451, "24cf757caed11fdd"),
    (0x101aa510, 0x101aa648, 78, "fb1d7cb144582ca1"),
    (0x101aa650, 0x101aa674, 9, "82fdb1999c8a64bd"),
    (0x101aa680, 0x101aaa90, 259, "203e0c7fbd6b4e2c"),
    (0x101aaa90, 0x101abda0, 1218, "03b62874a51bf3cd"),
    (0x101abda0, 0x101abf88, 121, "31e794982830ff9b"),
    (0x101abf90, 0x101ac124, 101, "26932153f85ec2ea"),
    (0x101ac130, 0x101ac280, 84, "d9baab7408226cd7"),
    (0x101ac280, 0x101acefc, 795, "9b87baa64b9fe00b"),
    (0x101acf00, 0x101ad080, 95, "d2bac2be7de4aecb"),
    (0x101ad080, 0x101b0020, 3048, "97d680c4309bcad1"),
    (0x101b0020, 0x101b00f8, 54, "c9cb9ca6f513c1db"),
    (0x101b0100, 0x101b0f2c, 907, "fdc168f7bd0f9a24"),
    (0x101b0f30, 0x101b0f84, 21, "03f064702a2902a4"),
    (0x101b0f90, 0x101b1618, 418, "2d48ee83f3cfe5e7"),
    (0x101b1620, 0x101b171c, 62, "8b3e2c4cdca68565"),
    (0x101b1720, 0x101b1978, 149, "4da3cd25b82366ed"),
    (0x101b1980, 0x101b2028, 426, "2b4c35256097c0e6"),
    (0x101b2030, 0x101b2188, 86, "7a6b27818bffd2ab"),
    (0x101b2190, 0x101b24d0, 208, "cbb43e0e0d6ae9b2"),
    (0x101b24d0, 0x101b2614, 81, "beec79488ebe815c"),
    (0x101b2620, 0x101b2a2c, 259, "18e9c0ac3819d4f0"),
    (0x101b2a30, 0x101b2f3c, 323, "8c27e3f67514954d"),
    (0x101b2f40, 0x101b39ec, 683, "91846afc6170e5aa"),
    (0x101b39f0, 0x101b3c88, 166, "6057e98cc9b104b0"),
    (0x101b3c90, 0x101b508c, 1278, "fca9761c666bf430"),
    (0x101b5090, 0x101b5368, 182, "eea1ce80c179a1b6"),
    (0x101b5370, 0x101b5b18, 490, "9b45ee1519f1569c"),
    (0x101b5b20, 0x101b6ff0, 1332, "0b44d8fa7ef10a78"),
    (0x101b6ff0, 0x101b6ff8, 2, "eba071f473fdc6ba"),
    (0x101b7000, 0x101b70a4, 41, "7519ba724696e49c"),
    (0x101b70b0, 0x101b75b8, 322, "3d52df16194f3db9"),
    (0x101b75c0, 0x101b75c8, 2, "69f5cacfba282c07"),
    (0x101b75d0, 0x101b76ac, 55, "708cc8eac75625c8"),
    (0x101b76b0, 0x101b782c, 95, "0ed7988a12492ad8"),
    (0x101b7830, 0x101b7954, 73, "a1bd790d2f40c5e0"),
    (0x101b7960, 0x101b7bc0, 152, "792be2ffa9444717"),
    (0x101b7bc0, 0x101b7dd4, 133, "725b67e19163a599"),
    (0x101b7de0, 0x101b83e8, 386, "edd137258d2aa529"),
    (0x101b83f0, 0x101b8af0, 448, "202a4dc12391dc82"),
    (0x101b8af0, 0x101b8af8, 2, "18fb86fd8f10f011"),
    (0x101b8b00, 0x101b8c08, 66, "48d9e3744e8d314e"),
    (0x101b8c10, 0x101b8e74, 153, "90aa5dfa92d3886d"),
    (0x101b8e80, 0x101b8fac, 75, "b9d594e18b46e204"),
    (0x101b8fb0, 0x101b912c, 95, "102ecaeb0df89769"),
    (0x101b9130, 0x101b94d0, 232, "748dc036a4ba37d7"),
    (0x101b94d0, 0x101b9b50, 416, "027dc25fdca835f7"),
    (0x101b9b50, 0x101bb8f8, 1898, "b151edab34d8c3ab"),
    (0x101bb900, 0x101bb9e4, 57, "961fa83e2200e8b8"),
    (0x101bb9f0, 0x101bd318, 1610, "232078f2f2b123d0"),
    (0x101bd320, 0x101bd45c, 79, "ca883829d269a8f8"),
    (0x101bd460, 0x101bde64, 641, "8f3e314fbd73402b"),
    (0x101bde70, 0x101bdf00, 35, "2f5f0f1f676f8e5c"),
    (0x101bdf00, 0x101bdf78, 30, "b3cd22ffe34983b9"),
    (0x101bdf80, 0x101bec74, 827, "b98d5c615303606f"),
    (0x101bec80, 0x101bedd8, 86, "cabdd58a6fa5b1aa"),
    (0x101bede0, 0x101bef88, 106, "a437c3de68c7d586"),
    (0x101bef90, 0x101bf064, 53, "c442f336f8b60031"),
    (0x101bf070, 0x101bf39c, 203, "c961d03eb851e798"),
    (0x101bf3a0, 0x101bf454, 45, "ee1e6df81b3d06d6"),
    (0x101bf460, 0x101bf5b8, 86, "9cc4212f807e789c"),
    (0x101bf5c0, 0x101bfd08, 466, "a20bec800cb94ac9"),
    (0x101bfd10, 0x101c00c8, 236, "8c682bdab246e330"),
    (0x101c00d0, 0x101c0224, 84, "ea1cf71858b86799"),
    (0x101c0230, 0x101c0fc0, 868, "a0f5ce825b5b8ffa"),
    (0x101c0fc0, 0x101c1174, 109, "5c2ff2dce7a2868f"),
    (0x101c1180, 0x101c12a0, 72, "2b0ef7c0d7fac036"),
    (0x101c12a0, 0x101c15e8, 210, "d20a35b7dfa754cf"),
    (0x101c15f0, 0x101c1688, 38, "60bc70bc2f4bc618"),
    (0x101c1690, 0x101c1824, 100, "fc73fab54ec0d9b4"),
    (0x101c1830, 0x101c1a0c, 118, "87a7b59af62e3be4"),
    (0x101c1a10, 0x101c1c14, 129, "5fc939129fc48589"),
    (0x101c1c20, 0x101c1e20, 128, "d3d72bf855107e4f"),
    (0x101c1e20, 0x101c3218, 1277, "fae445abd7126bf9"),
    (0x101c3220, 0x101c385c, 399, "85b8bce82636cad6"),
    (0x101c3860, 0x101c3b08, 170, "cc550c660aadf166"),
    (0x101c3b10, 0x101c42bc, 482, "c3ea6db715ed98f6"),
    (0x101c42c0, 0x101c42e8, 10, "377bda509f8b38df"),
)
ROUTINES = tuple((entry, end) for entry, end, _, _ in CONTRACTS)
UNREACHABLE = frozenset({
    0x101936bc, 0x10193fe8, 0x10194018, 0x1019409c, 0x10194120, 0x10194190, 0x101941c0, 0x1019423c,
    0x1019433c, 0x10194664, 0x10194744, 0x101947f4, 0x101948d4, 0x10194984, 0x10194a64, 0x10194b14,
    0x10194bf4, 0x10194ca4, 0x10194d84, 0x1019765c, 0x10197a98, 0x10197ad8, 0x10197be8, 0x10197cb8,
    0x10197cf8, 0x10197d38, 0x10198354, 0x1019841c, 0x1019858c, 0x10198cb4, 0x10198d48, 0x10198ddc,
    0x10198e70, 0x10198f04, 0x10198f98, 0x1019902c, 0x10199108, 0x101991c4, 0x10199278, 0x10199304,
    0x10199390, 0x10199780, 0x10199814, 0x101998a8, 0x1019993c, 0x101999d0, 0x10199a64, 0x10199af8,
    0x10199bd4, 0x10199c90, 0x10199d44, 0x10199dd0, 0x10199e60, 0x10199f0c, 0x1019a264, 0x1019a304,
    0x1019a3a4, 0x1019a444, 0x1019a4e4, 0x1019a584, 0x1019a624, 0x1019a710, 0x1019a7dc, 0x1019a8a0,
    0x1019a938, 0x1019a9d4, 0x1019aa90, 0x1019ac48, 0x1019acac, 0x1019ad10, 0x1019aed8, 0x1019af3c,
    0x1019afa0, 0x1019b778, 0x1019b7dc, 0x1019b840, 0x1019b8a4, 0x1019bf74, 0x1019bfd8, 0x1019c03c,
    0x1019c150, 0x1019c1b4, 0x1019c218, 0x1019c710, 0x1019c838, 0x101a14ec, 0x101a16e8, 0x101a1c64,
    0x101aa6e8, 0x101ab02c, 0x101ab0b0, 0x101abde4, 0x101acbc0, 0x101acbec, 0x101acc90, 0x101accbc,
    0x101acf98, 0x101b16c8, 0x101b193c, 0x101b45f8, 0x101bdeec, 0x101be460, 0x101be900, 0x101bfeec,
    0x101c0038, 0x101c01e4, 0x101c17e8, 0x101c1914, 0x101c213c, 0x101c3c40, 0x101c3c68, 0x101c3d58,
    0x101c3f14, 0x101c3fcc, 0x101c4084, 0x101c4184, 0x101c41dc, 0x101c424c,
})
# Other units' routines a rewrite follows inline (entry, end, digest): list_next,
# inside 0x101b1620's search (manager_manager_lists.cpp).
INLINED = ((0x10738e00, 0x10738e28, "e3c7915b2ce2c5ae"),)
# Discovery roots that are jump-table cases of these routines (folded in).
CASE_ROOTS = frozenset({0x101b4b24, 0x101b4b88, 0x101b84cc, 0x101b8548, 0x101b8680, 0x101b87c0, 0x101b8908,
                        0x101b8a4c, 0x101b8adc})
ROUTINE = re.compile(r'MANAGER_MANAGER_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(manager_manager_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "manager_manager_routines.inc"


def routine_lines(path=INCLUDES):
    """The text of manager_manager_routines.inc with the group files it includes."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and hooked
    (MANAGER_MANAGER_ROUTINE), with drafts also the MANAGER_MANAGER_DRAFT ones check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"manager_manager_routines.inc: {entry:x}-{end:x} is not a routine of MANAGER_MANAGER.CPP")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A MANAGER_MANAGER.CPP routine is listed twice")
    return found


MANAGER_MANAGER_ENTRIES = tuple(entry for entry, _, _ in rewritten())


def require_manager_manager_index(db, path=INCLUDES):
    """Every rewritten routine's original instructions are still the ones its C++ follows."""
    require_inlined(db)
    for entry, end, expected in rewritten(path, drafts=True):
        body = routine_records(db, entry, end)
        if [r["address"] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing MANAGER_MANAGER.CPP instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"MANAGER_MANAGER.CPP routine {entry:x} changed in the export (digest {actual}, "
                             f"manager_manager_routines.inc pins {expected})")


def require_inlined(db):
    """The callees followed inline are still the instructions their copies follow."""
    for entry, end, expected in INLINED:
        actual = digest(routine_records(db, entry, end))
        if actual != expected:
            raise ValueError(f"Inlined routine {entry:x} changed in the export (digest {actual}, pinned {expected})")


def manager_manager_hook(address):
    return f"if (try_modern_manager_manager(s, 0x{address:x})) return true;"


def require_manager_manager_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in MANAGER_MANAGER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = manager_manager_hook(entry), f"s.step(0x{entry:x});"
        # Up to the next instruction's label (the next word may be padding).
        fragment = re.split(r"\ni_[0-9a-f]+:", code.split(f"i_{entry:x}:", 1)[-1], maxsplit=1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced MANAGER_MANAGER hook at {entry:x}")
        # The owner must be the first hook at its entry and the only owner: an
        # older hook would run instead of the C++, or instead of the original
        # when the C++ declines.
        if fragment.find("if (try_") != fragment.index(hook) or fragment.count("if (try_modern_") != 1:
            raise ValueError(f"Another hook shares the MANAGER_MANAGER hook's entry {entry:x}")


def main():
    """Print each routine's digest, for new lines of manager_manager_routines.inc."""
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
    db = sqlite3.connect(args.export / "instructions.sqlite")
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

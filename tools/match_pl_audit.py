"""MATCH_PL.CPP original inventory and incremental plain-C++ ownership.

The zero-argument constructor starts before the first source-pool load. Its
transition vector closes that boundary; MATCH_SCREENS starts the next pool.
Thirty bounded switch tables contain local labels, not additional entries.
Original listings and pointer/flow evidence remain private under .local/.
"""
import re
from pathlib import Path
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x102238a0, 0x1028ad60)
# Entry, exclusive next-entry boundary, indexed instruction count, source digest.
CONTRACTS = (
    (0x102238a0, 0x10223ac0, 135, "f723a2754ee72021"),
    (0x10223ac0, 0x10226520, 2712, "c0db5a98924ccdd7"),
    (0x10226520, 0x102270f0, 753, "682a50b1abbedbb1"),
    (0x102270f0, 0x10227140, 18, "bfddc345d197acea"),
    (0x10227140, 0x10227460, 197, "b4cf921550e45315"),
    (0x10227460, 0x1022ba10, 4460, "36b5670e3bf1947a"),
    (0x1022ba10, 0x1022e3a0, 2660, "267b04268698447f"),
    (0x1022e3a0, 0x1022e710, 220, "ac962997d13f2944"),
    (0x1022e710, 0x1022e760, 18, "4b24fef090e818ac"),
    (0x1022e760, 0x1022e7e0, 31, "a29c182df5561795"),
    (0x1022e7e0, 0x10235db0, 7537, "1a4fccfecd07f022"),
    (0x10235db0, 0x10235ea0, 59, "df03e33747b7a155"),
    (0x10235ea0, 0x102373a0, 1344, "d0d1be29af04de46"),
    (0x102373a0, 0x102373c0, 5, "5559ca3a48a48fd4"),
    (0x102373c0, 0x10237420, 23, "75ee49061684f182"),
    (0x10237420, 0x102374f0, 52, "88dd6dee2d2edc13"),
    (0x102374f0, 0x10237830, 208, "2f5d2fee693f48f2"),
    (0x10237830, 0x10237b00, 177, "d30e61b3045d1c73"),
    (0x10237b00, 0x10237b10, 2, "23fcb7f81b151ea9"),
    (0x10237b10, 0x10237b50, 13, "f71ae0fa59ccd7ac"),
    (0x10237b50, 0x102393c0, 1563, "35946308526c1212"),
    (0x102393c0, 0x10239590, 115, "bca74f263a6e8fcd"),
    (0x10239590, 0x10239790, 127, "6db3598e8618a19e"),
    (0x10239790, 0x10239d10, 352, "91a71ce89d91068e"),
    (0x10239d10, 0x10239dc0, 42, "3a0cdcdb8808ff80"),
    (0x10239dc0, 0x10239e50, 34, "669a8ae62eb80937"),
    (0x10239e50, 0x10239f10, 48, "10751487df5b132b"),
    (0x10239f10, 0x1023cd20, 2948, "623db0dac28fc9de"),
    (0x1023cd20, 0x10244c60, 8142, "a42fccfeb44fe0eb"),
    (0x10244c60, 0x10245050, 252, "c4a854add6bd1dcd"),
    (0x10245050, 0x10245160, 67, "ef1f4ff8f6db8159"),
    (0x10245160, 0x10245220, 47, "e8656bbbde63591c"),
    (0x10245220, 0x10245870, 401, "56507dfea94f0ecc"),
    (0x10245870, 0x10247030, 1520, "798ce792d70bb868"),
    (0x10247030, 0x10251240, 10370, "acb43dfc3e3a0de1"),
    (0x10251240, 0x10251270, 10, "3c0b299daf9ce161"),
    (0x10251270, 0x10252080, 897, "70c79d24c07796fc"),
    (0x10252080, 0x102576e0, 5528, "0dd9e22723a2d360"),
    (0x102576e0, 0x1026a1b0, 19124, "bc1c48ae72e89268"),
    (0x1026a1b0, 0x1026d660, 3371, "f4ec46acd3336988"),
    (0x1026d660, 0x10270320, 2860, "f3bb112ea6fd059e"),
    (0x10270320, 0x10270ba0, 541, "2d1e5af461570df5"),
    (0x10270ba0, 0x10270c70, 50, "bc86e3ea793e5196"),
    (0x10270c70, 0x10272050, 1269, "fe00c9e7ef332838"),
    (0x10272050, 0x102720a0, 20, "682ab8d1fd5d1aef"),
    (0x102720a0, 0x102720d0, 10, "fa1a7866fe4b74ec"),
    (0x102720d0, 0x10272100, 12, "1e8f52c55bd68faa"),
    (0x10272100, 0x10272150, 18, "38cc74365987e3fb"),
    (0x10272150, 0x102721a0, 18, "89509a4e39327ef2"),
    (0x102721a0, 0x102721f0, 18, "2287300b48615787"),
    (0x102721f0, 0x10272240, 18, "d0b8d64e3d1be30e"),
    (0x10272240, 0x10274170, 1995, "75ec18c41067cd41"),
    (0x10274170, 0x102775c0, 3347, "f63ba66712ac1245"),
    (0x102775c0, 0x102775e0, 8, "731686a5bc14dc76"),
    (0x102775e0, 0x10278220, 783, "e0a5c9d8b2a87251"),
    (0x10278220, 0x10281780, 9558, "cdd40109703f18c1"),
    (0x10281780, 0x102818f0, 92, "6cd1c5a0fba9968f"),
    (0x102818f0, 0x10281af0, 127, "5f95d1d1cfbc11f8"),
    (0x10281af0, 0x10283540, 1683, "d215dfddb4d868d2"),
    (0x10283540, 0x10283590, 18, "32acc34d9366af03"),
    (0x10283590, 0x10283620, 33, "86411115cf6bd64f"),
    (0x10283620, 0x10283690, 26, "0853efa40dfb85bb"),
    (0x10283690, 0x102836a0, 2, "e3c4cade64c322c0"),
    (0x102836a0, 0x10285750, 2090, "fcaad8a1b3f4dd60"),
    (0x10285750, 0x10285790, 16, "0f2e8a18c6b6e758"),
    (0x10285790, 0x102857c0, 9, "6d76edf0ec8aa066"),
    (0x102857c0, 0x10285ed0, 449, "02d03e677ade5746"),
    (0x10285ed0, 0x10285ee0, 4, "5a304991323a0d32"),
    (0x10285ee0, 0x10285ef0, 2, "130df3dbd61ba572"),
    (0x10285ef0, 0x102860c0, 116, "58d18f4f935227ef"),
    (0x102860c0, 0x102861b0, 58, "fc4903e0a58bbfd4"),
    (0x102861b0, 0x102862b0, 62, "e1489c04752370a7"),
    (0x102862b0, 0x10286320, 28, "230339173acc73a3"),
    (0x10286320, 0x10286390, 28, "86402670701f8c0e"),
    (0x10286390, 0x10286400, 28, "96f4297b2e012314"),
    (0x10286400, 0x10286470, 28, "dd3a233c96dbf99d"),
    (0x10286470, 0x102864b0, 15, "e67fbee1a6bb7c13"),
    (0x102864b0, 0x102864f0, 15, "d669feb2d4d2278e"),
    (0x102864f0, 0x102865a0, 43, "54a8b3935b5cefd4"),
    (0x102865a0, 0x10286650, 44, "ca7cb93edc9abb87"),
    (0x10286650, 0x102875f0, 1000, "7816bfd72a14d5c1"),
    (0x102875f0, 0x102876c0, 50, "02510e3272e14414"),
    (0x102876c0, 0x10288230, 730, "38409c0bfd2350d5"),
    (0x10288230, 0x102884d0, 168, "b197398a8a58b3a1"),
    (0x102884d0, 0x10288f30, 662, "dc6f7691e60f89a9"),
    (0x10288f30, 0x10289550, 390, "7caa4433c52787bc"),
    (0x10289550, 0x10289560, 2, "37ba5ab0ecdd9b25"),
    (0x10289560, 0x102895a0, 15, "eef5843d04f65a71"),
    (0x102895a0, 0x102895e0, 15, "7c054bd4df2c4175"),
    (0x102895e0, 0x10289620, 15, "7bed26fd08fa5736"),
    (0x10289620, 0x10289d50, 458, "63a3f67171b2e132"),
    (0x10289d50, 0x1028abc0, 922, "3a3ce6965da9017f"),
    (0x1028abc0, 0x1028ac50, 35, "198352f7db2f1518"),
    (0x1028ac50, 0x1028acf0, 40, "c47ef5917085e55c"),
    (0x1028acf0, 0x1028ad60, 25, "8f865608b13eb5f9"),
 )
ROUTINES = tuple((a, b) for a, b, _, _ in CONTRACTS)
# bctr, TOC slot, table, number of words, SHA256 of relocated table bytes.
SWITCH_TABLES = (
    (0x102272cc, -0x51ec, 0x108cd550, 9, "8b11af22ab8f85273ad4ed6b0ca56db86aae9166b991e8cb15ee2466a8a68078"),
    (0x1022dfa4, -0x51f0, 0x108cd574, 63, "1fab3a5c6e81e3e037ee6f30bb4aef8772a58d2ca3758cd9b1badedcc961057e"),
    (0x1023041c, -0x51f4, 0x108cd758, 47, "24b723f58e88516ef8774a5e97524997d2eda9f24bf694b5773db8e3fcf188ad"),
    (0x10233b18, -0x51f8, 0x108cd69c, 47, "13c85580e354e62a8cf7edb5afd1bf053bfad719f9e64d0cac6ff528095592a0"),
    (0x10233d98, -0x51fc, 0x108cd670, 11, "e51435725d70bf007bac0eddcd572386f6ee09a51aa58725dfc4c548ad359d41"),
    (0x1023968c, -0x5200, 0x108cd814, 28, "4471df4ee72c2421688d5d0aac3fdfe2f63ee36d31fa0e391a8850b95fb22e8d"),
    (0x1023ff68, -0x5204, 0x108cdb08, 47, "c6e7d95bf3c4fde0a1bbaeb4c3f5d952dd446d36e7ee8ec9edc88611549071e1"),
    (0x10241470, -0x5208, 0x108cda24, 57, "27817a90852b942323aab442ada94565f222d7d386824846bb8c564a3593d564"),
    (0x1024368c, -0x520c, 0x108cd968, 47, "4abcb03783ff485a7531acf123308f7168a1e18bf2b2489f795fa387e6b5ccd3"),
    (0x10243918, -0x5210, 0x108cd940, 10, "c155273430f512a3b1e065f5ce61d96544bba8beb377a8f2b7d972d51babf44e"),
    (0x102439f0, -0x5214, 0x108cd884, 47, "0d01920167739f34d96c6441b2b63fd9cfe8cee353614901c77346fe6615bf42"),
    (0x10248b50, -0x5218, 0x108cdea0, 47, "d4a0a2198b6119894bff869c5e4786be932c361103c95a7a13c136edf94d7391"),
    (0x1024a328, -0x521c, 0x108cdde4, 47, "c0fac6f0a448ff66ee98119178af745118c4877e70493c9572ac944784d61811"),
    (0x1024a684, -0x5220, 0x108cdd28, 47, "650dc0a29e621613f196e1041b1f0a7dd77e0bfbe1eaf03923b3665c17965237"),
    (0x1024d420, -0x5224, 0x108cdbc4, 89, "e93892c2b802c1b241c399b11e7e69eb67ce83dca842da6c3becdcc9046a6dbd"),
    (0x102573d0, -0x5228, 0x108cdf5c, 56, "9f3ac119692ef137b613d06ca81e34c9de827deedff56bc591f5cbca09b98549"),
    (0x1025844c, -0x522c, 0x108ce274, 47, "f8ca75b9af61b1afd26e568b2a2791fcae07fe9258d0e6d32b62e0c09aede70f"),
    (0x10259c10, -0x5230, 0x108ce1b8, 47, "3a937133788baea581f775244388a281cff99373a36a97dda1e9a58673d0123b"),
    (0x10259f64, -0x5234, 0x108ce0fc, 47, "4e4187fa1971480ab8e6f327936998063a848536f3fea5ddf26eab4cf058135d"),
    (0x1025ac7c, -0x5238, 0x108ce0ac, 20, "59867f2528c8bdf54a031caa14a106127503c92e268e77de338f083caf1f15cb"),
    (0x1025c360, -0x523c, 0x108ce074, 14, "dbcc6bb113043d93999dbab2966b200403413d3132640a607e8e35ea04200707"),
    (0x1025c628, -0x5240, 0x108ce03c, 14, "8e111ddec4c5d8b59e274e01ab40627661535da1ceb00905919f54190b42f6bd"),
    (0x102719c0, -0x5244, 0x108ce3fc, 39, "4d01aa8146672d2d3af04ef0eb28319a99af546be2f83209316090febd364423"),
    (0x10271c40, -0x5248, 0x108ce330, 51, "8b13e076cdfb3b335a07b2a66d547ea1a9cc042d1bc8dbf55c1db22d852eb4b9"),
    (0x10274e34, -0x524c, 0x108ce63c, 47, "a2de2df7a3d3a1ce88b26d21250c0cb316959c788a120fcc0140c75d91130616"),
    (0x1027663c, -0x5250, 0x108ce580, 47, "dd3174c9336304daec9afd12a1fca608530083847b5b45a1c45cb94ee20d5be4"),
    (0x102768bc, -0x5254, 0x108ce554, 11, "09f6a2b1022108205264a7bd6828e179882109d3d62ff9a78596a63a66aa552b"),
    (0x10276994, -0x5258, 0x108ce498, 47, "52303649b912856c10307f02174c3aee2b58dc53beb1ac4f300b0dd35d3104ee"),
    (0x1027834c, -0x525c, 0x108ce6f8, 9, "3e6802b30d3d1390c4a4dc16641bebcd52b88720bed7374f12a840c32554caba"),
    (0x10289e74, -0x5260, 0x108ce71c, 27, "8c8e033fa69c8e1ec87a8a49addc2edf648fe1c5a6395a528de59b284d3a737f"),
 )
TRANSITION_VECTORS = ((0x108b8c5c, 0x102270f0), (0x108b8c64, 0x102238a0))
INCLUDES = ROOT / "native/match_pl_routines.inc"
ROUTINE = re.compile(r'MATCH_PL_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(match_pl_routines_\w+\.inc)"')


def routine_lines(path=INCLUDES):
    path = Path(path)
    if not path.exists():
        return ""
    text = path.read_text()
    for name in INCLUDE.findall(text):
        text += (path.parent / name).read_text()
    return text


def rewritten(path=INCLUDES, drafts=False):
    found = sorted((int(a, 16), int(b, 16), pinned)
                   for stage, a, b, _, pinned in ROUTINE.findall(routine_lines(path))
                   if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"MATCH_PL.CPP invalid entry/boundary {entry:x}-{end:x}")
    if len({a for a, _, _ in found}) != len(found):
        raise ValueError("MATCH_PL.CPP routine is listed twice")
    return found


def require_match_pl_index(db):
    # Every original entry is checked, including entries not implemented yet.
    # A partial promotion cannot hide missing or changed source instructions.
    for entry, end, count, pinned in CONTRACTS:
        records = routine_records(db, entry, end)
        if len(records) != count or digest(records) != pinned:
            raise ValueError(f"MATCH_PL.CPP original changed at {entry:x}")
    originals = {a: (b, pinned) for a, b, _, pinned in CONTRACTS}
    for entry, end, pinned in rewritten(drafts=True):
        if originals[entry] != (end, pinned):
            raise ValueError(f"MATCH_PL.CPP promoted digest changed at {entry:x}")


def require_match_pl_owners(routines, owners):
    """Promoted entries must have exactly one normal production owner."""
    for entry, _, _ in routines:
        actual = [owner for at, owner in owners if at == entry]
        if actual != ["match_pl_adapter"]:
            raise ValueError(f"MATCH_PL.CPP production ownership at {entry:x}: {actual}")


MATCH_PL_ENTRIES = tuple(a for a, _, _ in rewritten())


def match_pl_hook(address):
    return f"if (try_modern_match_pl(s, 0x{address:x})) return true;"


def main():
    import argparse
    import sqlite3
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("entries", nargs="*")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--entries", dest="list_entries", action="store_true")
    args = parser.parse_args()
    if args.list_entries:
        for entry, _ in ROUTINES:
            print(f"0x{entry:x}")
        return
    path = args.export / "instructions.sqlite"
    if not path.is_file():
        raise FileNotFoundError(f"Private MATCH_PL export unavailable: {path}")
    with sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_match_pl_index(db)
    wanted = {int(a, 16) for a in args.entries}
    promoted = set(MATCH_PL_ENTRIES)
    for entry, end, count, pinned in CONTRACTS:
        if not wanted or entry in wanted:
            print(f"0x{entry:x} 0x{end:x} {pinned} {count:5}" + (" (C++)" if entry in promoted else ""))


if __name__ == "__main__":
    main()

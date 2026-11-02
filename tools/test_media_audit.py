"""Certify media.cpp's entries, original boundary and production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import sys
import tempfile
import unittest
from unittest.mock import patch

import media_audit as audit
from tactics_audit import routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
CONTRACTS = (
    (0x10646d90, 0x10646dfc, 27, '3ab6bc87cd975181'),
    (0x10646e00, 0x10646e84, 33, '74a1ef524bf856ac'),
    (0x10646e90, 0x10647418, 354, 'bffd1e704c5167cc'),
    (0x10647420, 0x10647730, 196, 'b5adf370552833ff'),
    (0x10647730, 0x10647bf4, 305, 'ac9a219d25d73531'),
    (0x10647c00, 0x106487c4, 753, '8a4609d45a9e970f'),
    (0x106487d0, 0x10649900, 1100, '9f7459970d4782ca'),
    (0x10649900, 0x10649ac8, 114, '710c0e50fc67f9fd'),
    (0x10649ad0, 0x10649fac, 310, '3eeedebef5ab1e9b'),
    (0x10649fb0, 0x1064cf74, 3057, 'd4a75c6d97a52f7d'),
    (0x1064cf80, 0x106548d8, 7761, '9fbe646d194e5eb6'),
    (0x106548e0, 0x10655028, 466, '83a1049e39fa02b7'),
    (0x10655030, 0x10655d58, 837, 'a9d592ab7e85eaaf'),
    (0x10655d60, 0x10656448, 442, 'b244ed01d952be90'),
    (0x10656450, 0x10657b2c, 1461, '2ec1ff9cd6fa0e4c'),
    (0x10657b30, 0x10658370, 506, 'bc6ca91cb9b743a9'),
    (0x10658370, 0x106583cc, 23, '50f7ad99b4104fe0'),
    (0x106583d0, 0x10658480, 44, '4f70e93678b72139'),
    (0x10658480, 0x106584c4, 17, 'e16c567b1b7f12be'),
    (0x106584d0, 0x10661a50, 9568, 'b4d26f41b1153ba5'),
    (0x10661a50, 0x106620b0, 408, 'b9f98206bbfc6a99'),
    (0x106620b0, 0x1066b5f4, 9553, '5efeb0fe943e3b62'),
    (0x1066b600, 0x1066bc50, 404, 'f7bc2a4a15df2b45'),
    (0x1066bc50, 0x1066c670, 648, '1bcde318db0c295c'),
    (0x1066c670, 0x106727c0, 6228, 'e689d09f4abbfb7f'),
    (0x106727c0, 0x106727f4, 13, '7e3e83d2fbd9809d'),
    (0x10672800, 0x10677d58, 5461, 'dd1b65d837d64133'),
    (0x10677d60, 0x1067b914, 3819, 'be3dee35d912f9d6'),
    (0x1067b920, 0x1067be98, 350, '89a888399950d230'),
    (0x1067bea0, 0x1067ca24, 737, 'aa9682519904498a'),
    (0x1067ca30, 0x1067d02c, 383, 'a6907411e2fb272e'),
    (0x1067d030, 0x1067d034, 1, 'd1619834799a15b7'),
    (0x1067d040, 0x1067d2e8, 170, 'e9b43cd2f93c5739'),
    (0x1067d2f0, 0x1067d82c, 335, 'c46197f83c1331e8'),
    (0x1067d830, 0x1067dc0c, 247, '04ade4c33ca8a35c'),
    (0x1067dc10, 0x1067e210, 383, 'fc81f74a43f960d1'),
    (0x1067e210, 0x1067fed0, 1839, '3e879c498d6afc26'),
    (0x1067fed0, 0x10688e80, 9189, 'bb66914be1f9cfa7'),
    (0x10688e80, 0x10688fc4, 81, '2dd653e8b0c6fb9e'),
    (0x10688fd0, 0x106891ac, 119, 'ae40b6d662ae2a66'),
    (0x106891b0, 0x1068a468, 1197, '768efcb1811bad6e'),
    (0x1068a470, 0x10698ba4, 14785, 'ac0756f257302bb1'),
    (0x10698bb0, 0x10698e80, 180, '87d52796838f9cf2'),
    (0x10698e80, 0x10698f44, 49, 'd56e12eb39d4e525'),
    (0x10698f50, 0x10699d10, 880, '17e770f405624e64'),
    (0x10699d10, 0x1069acdc, 1011, 'cb180f2f6b670f96'),
    (0x1069ace0, 0x1069d194, 2348, '4706bafddb58e37a'),
    (0x1069d1a0, 0x1069d460, 176, 'd99aceb5e727dff7'),
    (0x1069d460, 0x1069e18c, 843, '6e99a08d75c4430e'),
    (0x1069e190, 0x1069e3d0, 144, '77108222254259e3'),
    (0x1069e3d0, 0x1069e47c, 43, 'a3b29f6700a8a477'),
    (0x1069e480, 0x1069e6f4, 157, '6446ec50a0582731'),
    (0x1069e700, 0x1069e890, 100, '656989d2e70d1cfd'),
    (0x1069e890, 0x1069e900, 28, 'd4d8e05bff901d28'),
    (0x1069e900, 0x1069e990, 36, 'cfea787620dbc191'),
    (0x1069e990, 0x1069eb04, 93, '1ccdc04c6240e7b3'),
    (0x1069eb10, 0x1069ec88, 94, 'dafb7dfe8b37827d'),
    (0x1069ec90, 0x1069ece4, 21, '95351192a52071a5'),
    (0x1069ecf0, 0x1069ed10, 8, 'e1abaf21335756d1'),
    (0x1069ed10, 0x1069f268, 342, 'f561663d7f4a8b6b'),
    (0x1069f270, 0x106a15f0, 2272, '1736eee7a5ea14d8'),
    (0x106a15f0, 0x106a1858, 154, 'ccb07b6475effe10'),
    (0x106a1860, 0x106a1ef8, 422, '7d1af8dd67a21e46'),
    (0x106a1f00, 0x106a2330, 268, '0e7f7d3a412a48ce'),
    (0x106a2330, 0x106a2e0c, 695, 'ac2b823539cfe110'),
    (0x106a2e10, 0x106a4098, 1186, 'd35144e582263923'),
    (0x106a40a0, 0x106a44c8, 266, '78939fa2224519ca'),
    (0x106a44d0, 0x106a52c8, 893, 'b05d4aa4f7f21588'),
    (0x106a52d0, 0x106a5e88, 750, 'ea279f992c1ce46a'),
    (0x106a5e90, 0x106a6228, 230, 'bdd95398beb3b4df'),
    (0x106a6230, 0x106a6e5c, 779, '74b94d580a5cfaf4'),
    (0x106a6e60, 0x106a7120, 176, '18cccbfa1b9bf80b'),
    (0x106a7120, 0x106a7d68, 786, 'e4ab1285089412bc'),
    (0x106a7d70, 0x106a810c, 231, 'c9db3b9df6d6bbfe'),
    (0x106a8110, 0x106a8a50, 591, '125934eb84886f20'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
PADDING = {0x10646dfc, 0x10646e84, 0x10646e88, 0x10646e8c, 0x10647418, 0x1064741c, 0x10647bf4, 0x10647bf8, 0x10647bfc, 0x106487c4, 0x106487c8, 0x106487cc, 0x10649ac8, 0x10649acc, 0x10649fac, 0x1064cf74, 0x1064cf78, 0x1064cf7c, 0x106548d8, 0x106548dc, 0x10655028, 0x1065502c, 0x10655d58, 0x10655d5c, 0x10656448, 0x1065644c, 0x10657b2c, 0x106583cc, 0x106584c4, 0x106584c8, 0x106584cc, 0x1066b5f4, 0x1066b5f8, 0x1066b5fc, 0x106727f4, 0x106727f8, 0x106727fc, 0x10677d58, 0x10677d5c, 0x1067b914, 0x1067b918, 0x1067b91c, 0x1067be98, 0x1067be9c, 0x1067ca24, 0x1067ca28, 0x1067ca2c, 0x1067d02c, 0x1067d034, 0x1067d038, 0x1067d03c, 0x1067d2e8, 0x1067d2ec, 0x1067d82c, 0x1067dc0c, 0x10688fc4, 0x10688fc8, 0x10688fcc, 0x106891ac, 0x1068a468, 0x1068a46c, 0x10698ba4, 0x10698ba8, 0x10698bac, 0x10698f44, 0x10698f48, 0x10698f4c, 0x1069acdc, 0x1069d194, 0x1069d198, 0x1069d19c, 0x1069e18c, 0x1069e47c, 0x1069e6f4, 0x1069e6f8, 0x1069e6fc, 0x1069eb04, 0x1069eb08, 0x1069eb0c, 0x1069ec88, 0x1069ec8c, 0x1069ece4, 0x1069ece8, 0x1069ecec, 0x1069f268, 0x1069f26c, 0x106a1858, 0x106a185c, 0x106a1ef8, 0x106a1efc, 0x106a2e0c, 0x106a4098, 0x106a409c, 0x106a44c8, 0x106a44cc, 0x106a52c8, 0x106a52cc, 0x106a5e88, 0x106a5e8c, 0x106a6228, 0x106a622c, 0x106a6e5c, 0x106a7d68, 0x106a7d6c, 0x106a810c}
UNREACHABLE = {0x10649cc0, 0x1064dddc, 0x10650b58, 0x10651808, 0x10651d2c, 0x10652f60, 0x106553e0, 0x106554ac, 0x106555a8, 0x10655774, 0x10655878, 0x106575bc, 0x10657618, 0x10657b98, 0x10657bf4, 0x10657c50, 0x10657cac, 0x10657d08, 0x10657d64, 0x10657dc0, 0x10657e1c, 0x10657e78, 0x10657ed4, 0x10657f30, 0x10657f8c, 0x10657fe8, 0x10658044, 0x106580a0, 0x106580fc, 0x10658158, 0x106581b4, 0x10658210, 0x1065826c, 0x106582c8, 0x10658324, 0x106728e8, 0x10677e70, 0x106797bc, 0x1067de5c, 0x1067f4c8, 0x10680e20, 0x106813ac, 0x106818fc, 0x10681c48, 0x106823c4, 0x10682b58, 0x10685338, 0x10689ab0, 0x1068bfb0, 0x1068c500, 0x1068ca68, 0x1068d780, 0x1068f490, 0x106911a8, 0x10692308, 0x10692848, 0x10692b24, 0x10692e60, 0x106937a8, 0x106943e8, 0x1069ca5c, 0x106a4bfc, 0x106a894c}
INCOMING = (
    (0x10005490, 0x10649fb0),
    (0x100823a4, 0x10698e80),
    (0x100e8a94, 0x106a15f0),
    (0x100e8c50, 0x106a15f0),
    (0x100e8cf4, 0x106a15f0),
    (0x100fb290, 0x10688e80),
    (0x1010e30c, 0x106583d0),
    (0x10119e30, 0x10698f50),
    (0x10119e3c, 0x1069e480),
    (0x1012687c, 0x10649fb0),
    (0x10127384, 0x1067e210),
    (0x10127a14, 0x1067e210),
    (0x10127c5c, 0x10658480),
    (0x10128228, 0x10649fb0),
    (0x101282cc, 0x10649fb0),
    (0x101284f0, 0x10649fb0),
    (0x10128514, 0x10649fb0),
    (0x10128fb8, 0x1067e210),
    (0x1012cf00, 0x106a40a0),
    (0x1012d0ac, 0x10658480),
    (0x1012d41c, 0x10649fb0),
    (0x1012dc3c, 0x10658480),
    (0x1012dfa8, 0x10649fb0),
    (0x1019fecc, 0x106891b0),
    (0x1019ffdc, 0x106891b0),
    (0x101a37e0, 0x106487d0),
    (0x101a380c, 0x10647730),
    (0x101a3830, 0x106891b0),
    (0x101a3a74, 0x106487d0),
    (0x101a3aa0, 0x10647730),
    (0x101a3ac4, 0x106891b0),
    (0x101a4474, 0x106891b0),
    (0x101a45a4, 0x106487d0),
    (0x101a45d0, 0x10647730),
    (0x101a45f4, 0x106891b0),
    (0x101a46e4, 0x106487d0),
    (0x101a4710, 0x10647730),
    (0x101a4734, 0x106891b0),
    (0x101a4db0, 0x106487d0),
    (0x101a4ddc, 0x10647730),
    (0x101a4e00, 0x106891b0),
    (0x101a5090, 0x106487d0),
    (0x101a50bc, 0x10647730),
    (0x101a50e0, 0x106891b0),
    (0x101a5440, 0x106487d0),
    (0x101a546c, 0x10647730),
    (0x101a5490, 0x106891b0),
    (0x101a5714, 0x106487d0),
    (0x101a5740, 0x10647730),
    (0x101a5764, 0x106891b0),
    (0x101a59e0, 0x106487d0),
    (0x101a5a0c, 0x10647730),
    (0x101a5a30, 0x106891b0),
    (0x101a5c38, 0x106487d0),
    (0x101a5c64, 0x10647730),
    (0x101a5c88, 0x106891b0),
    (0x101a5eb8, 0x106487d0),
    (0x101a5ee4, 0x10647730),
    (0x101a5f08, 0x106891b0),
    (0x101a6120, 0x106487d0),
    (0x101a614c, 0x10647730),
    (0x101a6170, 0x106891b0),
    (0x101a6348, 0x106891b0),
    (0x101a64fc, 0x106891b0),
    (0x101a6738, 0x106487d0),
    (0x101a6764, 0x10647730),
    (0x101a6788, 0x106891b0),
    (0x101a6940, 0x106891b0),
    (0x101be110, 0x106a15f0),
    (0x101be1f0, 0x106a15f0),
    (0x101c05bc, 0x106a15f0),
    (0x101c066c, 0x106a15f0),
    (0x101c0718, 0x106a15f0),
    (0x101c08d8, 0x106a15f0),
    (0x101cf224, 0x10649fb0),
    (0x101cf3fc, 0x1067e210),
    (0x101cff04, 0x106a1f00),
    (0x101d0118, 0x10649fb0),
    (0x101d0370, 0x10658480),
    (0x101d0410, 0x10649fb0),
    (0x101d064c, 0x10655d60),
    (0x101d0db0, 0x10649fb0),
    (0x101fde3c, 0x1069e700),
    (0x101fde88, 0x1069e700),
    (0x101fded4, 0x1069e700),
    (0x101fdfb0, 0x1069e700),
    (0x101fe070, 0x1069e700),
    (0x101fe150, 0x1069e700),
    (0x101fe230, 0x1069e700),
    (0x101fe264, 0x1069e700),
    (0x101fe298, 0x1069e700),
    (0x101fe2cc, 0x1069e700),
    (0x101fe3a8, 0x1069e700),
    (0x101fe488, 0x1069e700),
    (0x101fe4bc, 0x1069e700),
    (0x101fe59c, 0x1069e700),
    (0x101fe5d0, 0x1069e700),
    (0x10289f60, 0x1069e700),
    (0x10289fa4, 0x1069e700),
    (0x1028a0ac, 0x1069e700),
    (0x1028a1a0, 0x1069e700),
    (0x1028a358, 0x1069e700),
    (0x1028a414, 0x10658370),
    (0x1028a518, 0x1069e700),
    (0x1028a648, 0x1069e700),
    (0x1028a6dc, 0x1069e700),
    (0x1028a770, 0x1069e700),
    (0x1028a814, 0x1069e700),
    (0x102be3e0, 0x106a7120),
    (0x102c5a2c, 0x106a1860),
    (0x102c5aa4, 0x10688fd0),
    (0x102c5b58, 0x10649ad0),
    (0x102c5b80, 0x1067fed0),
    (0x102c5ba8, 0x1067dc10),
    (0x102c5bd0, 0x1064cf80),
    (0x102c5bf8, 0x1068a470),
    (0x102c5c20, 0x10655030),
    (0x102c5c6c, 0x1067d2f0),
    (0x102c5c94, 0x10656450),
    (0x102c5e4c, 0x106a8110),
    (0x102c5e74, 0x106a6230),
    (0x102c5ee4, 0x106a2330),
    (0x102c5f38, 0x106a44d0),
    (0x102c60cc, 0x1069d460),
    (0x102c6190, 0x1066c670),
    (0x102c621c, 0x1069ace0),
    (0x102f6d34, 0x10698bb0),
    (0x103464b0, 0x10646d90),
    (0x1034b1ec, 0x10646d90),
    (0x1034d864, 0x10647420),
    (0x1034df4c, 0x10646e00),
    (0x103607b4, 0x10698bb0),
    (0x10361124, 0x10698bb0),
    (0x103822a8, 0x106a6e60),
    (0x103822c8, 0x106a6e60),
    (0x10383b20, 0x106a6e60),
    (0x10383b4c, 0x106a6e60),
    (0x103a0c2c, 0x106583d0),
    (0x103a1ae4, 0x10658370),
    (0x103a2690, 0x10658370),
    (0x103f1b18, 0x1069f270),
    (0x1042591c, 0x10698bb0),
    (0x104543f0, 0x106891b0),
    (0x10454774, 0x106891b0),
    (0x105d0e6c, 0x1069d1a0),
)
# Direct calls between the module's routines: 553 sites, pinned by digest.
OWNED_CALLS = (553, 'e6b7081ad40dfc5e')
TRANSITION_VECTORS = {0x108bc46c: 0x1067b920, 0x108bc474: 0x1067d030, 0x108bc47c: 0x1067ca30, 0x108bc484: 0x1067bea0,
                      0x108bc48c: 0x1069e900, 0x108bc494: 0x1069e890}
VECTOR_USERS = {0x108b5718: 0x108bc46c, 0x108b5c8c: 0x108bc474, 0x108b5c90: 0x108bc47c, 0x108b5c94: 0x108bc484,
                0x108b5c98: 0x108bc48c, 0x108b5c9c: 0x108bc494}
# bctr site: (table, labels). The tables are contiguous in the data segment.
SWITCHES = {0x1064b310: (0x108de45c, 0x26), 0x1064efe0: (0x108de510, 8), 0x10650ea8: (0x108de4f4, 7),
            0x10657b78: (0x108de530, 0x80), 0x1067b2d0: (0x108de73c, 0x24), 0x1067c0dc: (0x108de7cc, 0x83),
            0x106808f4: (0x108deac8, 0xc), 0x10687ce8: (0x108dea98, 0xc), 0x10687e14: (0x108dea68, 0xc),
            0x10687fc8: (0x108dea38, 0xc), 0x10688120: (0x108dea08, 0xc), 0x106883d4: (0x108de9d8, 0xc),
            0x106893c4: (0x108deb38, 0x28), 0x10689d6c: (0x108deaf8, 0x10)}
# Aligned words in the data segment that happen to fall in the module's range: game data
# nothing loads as code (no flow or TOC slot reaches them).
STRAY_WORDS = {0x108e5774, 0x108e6f34, 0x108ecc3c, 0x108eee8c}


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private media.cpp export unavailable; inventory cannot be certified')


class MediaAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes())
                        for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.by_address = {r['address']: r for r in cls.records}

    def original_bytes(self, address, count):
        for start, contents in self.segments:
            if start <= address and address + count <= start + len(contents):
                return contents[address - start:address - start + count]
        self.fail(f'Original bytes unavailable at {address:x}')

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), 'big')

    def assert_production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry in ENTRIES:
            self.assertEqual([owner for at, owner in owners if at == entry], ['media_adapter'])

    def assert_hook(self, code, entry):
        hook = audit.media_hook(entry)
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        self.assertEqual(code.count(hook), 1)
        self.assertIn(hook, fragment)
        step = f's.step(0x{entry:x});'
        self.assertIn(step, fragment)
        self.assertLess(fragment.index(hook), fragment.index(step))

    def test_exact_entries_original_digests_and_exclusive_production_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
        self.assertEqual(audit.MODULE, (0x10646d90, 0x106a8a50))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.MEDIA_ENTRIES, ENTRIES)
        self.assertEqual(audit.UNREACHABLE, frozenset(UNREACHABLE))
        self.assertEqual(audit.rewritten(), [(a, b, digest) for a, b, _, digest in CONTRACTS])
        self.assertEqual(len(self.records), 99976)
        for start, end, count, _ in CONTRACTS:
            self.assertEqual(sum(start <= r['address'] < end for r in self.records), count)
        self.assert_production(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertFalse(set(ENTRIES) & {entry for entry, _ in SEMANTIC_REPLACEMENTS})
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_media_index(db)

    def test_source_pool_slots_and_neighbours_close_the_boundaries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x41f8), 0x10847ddc)
        self.assertEqual(self.original_bytes(0x10847ddc, 0x27), b'media.dat\0media.cpp\0v%s %s.%s %d\0Error\0\0'[:0x27])
        self.assertEqual(self.original_bytes(self.word(toc - 0x4260), 15), b'tur_second.cpp\0')
        self.assertEqual(self.original_bytes(self.word(toc - 0x41e8), 9), b'CM3_DATA\0')
        pools = {}
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for slot in ('-0x41f8', '-0x41fc', '-0x4200', '-0x4260', '-0x41e8'):
                pools[slot] = [row[0] for row in db.execute(
                    "SELECT address FROM instructions WHERE json_extract(body,'$.assembly') LIKE ? ORDER BY address",
                    (f'%,{slot}(r2)',))]
        low, high = audit.MODULE
        for slot in ('-0x41f8', '-0x41fc', '-0x4200'):
            self.assertTrue(pools[slot], slot)
            self.assertTrue(all(low <= at < high for at in pools[slot]), slot)
        self.assertLess(max(pools['-0x4260']), low)
        self.assertGreaterEqual(min(pools['-0x41e8']), high)
        self.assertEqual(self.by_address[low]['assembly'], 'mfspr r0,LR')
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            before = json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (low - 4,)).fetchone()[0])
            after = json.loads(db.execute('SELECT body FROM instructions WHERE address=?', (high,)).fetchone()[0])
        self.assertEqual(before['assembly'], 'blr')
        self.assertEqual(after['assembly'], 'mfspr r0,LR')
        self.assertEqual(self.by_address[high - 4]['assembly'], 'blr')

    def test_all_incoming_edges_have_named_entry_and_no_external_fallthrough(self):
        incoming = []
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            for address, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    'WHERE f.value>=? AND f.value<? ORDER BY i.address', audit.MODULE):
                if audit.MODULE[0] <= address < audit.MODULE[1]:
                    continue
                record = json.loads(body)
                for target in record['flows']:
                    if audit.MODULE[0] <= target < audit.MODULE[1]:
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        incoming.append((address, target))
            fallthroughs = list(db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?)",
                (*audit.MODULE, *audit.MODULE)))
        self.assertEqual(tuple(sorted(incoming)), INCOMING)
        self.assertEqual(fallthroughs, [])
        self.assertTrue(all(target in ENTRIES for _, target in incoming))

    def test_all_cross_entry_edges_are_direct_owned_calls_and_switches_stay_inside(self):
        cross = set()
        computed = set()
        for start, end in audit.ROUTINES:
            for record in self.records:
                if not start <= record['address'] < end:
                    continue
                if 'COMPUTED' in record['flow']:
                    computed.add(record['address'])
                    continue
                targets = record['flows'] + ([record['fallthrough']]
                                              if record.get('fallthrough') is not None else [])
                for target in targets:
                    self.assertNotIn(target, PADDING)
                    self.assertNotIn(target, UNREACHABLE)
                    if audit.MODULE[0] <= target < audit.MODULE[1] and not start <= target < end:
                        self.assertIn(target, ENTRIES)
                        self.assertEqual(record['flow'], 'UNCONDITIONAL_CALL')
                        cross.add((record['address'], target))
        pairs = ''.join(f'{a:x}>{t:x};' for a, t in sorted(cross))
        self.assertEqual((len(cross), hashlib.sha256(pairs.encode()).hexdigest()[:16]), OWNED_CALLS)
        self.assertEqual(computed, set(SWITCHES))
        self.assertEqual([s for s in self.program['symbols']
                          if audit.MODULE[0] <= s['address'] < audit.MODULE[1]], [])

    def test_switch_tables_hold_labels_of_their_own_routine(self):
        bounds = {a: b for a, b in audit.ROUTINES}
        for site, (table, labels) in SWITCHES.items():
            entry = max(a for a in bounds if a <= site)
            for n in range(labels):
                label = self.word(table + n * 4)
                self.assertTrue(entry <= label < bounds[entry], f'{site:x} label {n}')
                self.assertIn(label, self.by_address)
                self.assertNotIn(label, ENTRIES)

    def test_every_instruction_padding_and_unreachable_branch_accounted(self):
        addresses = set()
        for start, end in audit.ROUTINES:
            records = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual([r['address'] for r in records],
                             [a for a in range(start, end, 4) if a not in UNREACHABLE])
            self.assertEqual(records[-1]['assembly'], 'blr')
            addresses.update(range(start, end, 4))
        self.assertEqual(set(range(*audit.MODULE, 4)) - addresses, PADDING)
        for address in PADDING:
            self.assertEqual(self.word(address), 0)
            self.assertNotIn(address, self.by_address)
        for address in UNREACHABLE:
            self.assertEqual(self.word(address) >> 26, 18, f'{address:x} is not a branch')
            self.assertTrue(self.by_address[address - 4]['assembly'].startswith('b 0x'))
            self.assertNotIn(address, self.by_address)

    def test_raw_pointer_census_finds_only_vectors_switch_labels_and_stray_words(self):
        low, high = audit.MODULE
        labels = {table + n * 4 for table, count in SWITCHES.values() for n in range(count)}
        found = {}
        for start, contents in self.segments:
            for offset in range(0, len(contents) - 3, 4):
                target = int.from_bytes(contents[offset:offset + 4], 'big')
                if low <= target < high:
                    found[start + offset] = target
        self.assertEqual(set(found) - labels - set(TRANSITION_VECTORS), STRAY_WORDS)
        for vector, entry in TRANSITION_VECTORS.items():
            self.assertEqual((found[vector], self.word(vector + 4)), (entry, 0x108bba90))
            self.assertIn(entry, ENTRIES)
        for user, vector in VECTOR_USERS.items():
            self.assertEqual(self.word(user), vector)
        for stray in STRAY_WORDS:
            self.assertNotIn(found[stray], self.by_address.keys() & set(ENTRIES))

    def test_missing_export_and_stale_digest_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
            with patch.object(audit, 'rewritten', return_value=[(a, b, '0') for a, b in audit.ROUTINES]):
                with self.assertRaisesRegex(ValueError, 'Missing media.cpp instructions'):
                    audit.require_media_index(db)
        stale = [(a, b, '0000000000000000') if i == 0 else (a, b, digest)
                 for i, (a, b, digest) in enumerate(audit.rewritten(drafts=True))]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            with patch.object(audit, 'rewritten', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_media_index(db)

    def test_duplicate_bad_boundary_draft_and_missing_owner_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        contents = audit.routine_lines()
        first = next(line for line in contents.splitlines() if 'MEDIA_ROUTINE(' in line)
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / 'routines.inc'
            candidate.write_text(contents + '\n' + first)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate)
            entry, end = audit.ROUTINES[0]
            line = next(l for l in contents.splitlines() if f'0x{entry:x},' in l)
            candidate.write_text(contents.replace(line, line.replace(f'0x{end:x}', f'0x{end - 4:x}'), 1))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate)
            candidate.write_text(contents.replace('MEDIA_ROUTINE(', 'MEDIA_DRAFT(', 1))
            self.assertEqual(len(audit.rewritten(candidate, drafts=True)), 75)
            with self.assertRaises(AssertionError):
                self.assert_production(audit.rewritten(candidate), OWNER_REPLACEMENTS)
        missing = [(at, owner) for at, owner in OWNER_REPLACEMENTS if at != ENTRIES[0]]
        with self.assertRaises(AssertionError):
            self.assert_production(audit.rewritten(), missing)

    def test_normal_hooks_precede_original_steps_and_detect_disabled_or_duplicate_hook(self):
        from compile_recomp import generate_page
        selected = set(self.by_address)
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_media_hooks(page, part, normal)
            self.assertIn('#include "media_adapter.hpp"', normal)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                self.assert_hook(normal, entry)
                hook = audit.media_hook(entry)
                step = f's.step(0x{entry:x});'
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaises(AssertionError):
                        self.assert_hook(broken, entry)
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_media_hooks(page, part, broken)


if __name__ == '__main__':
    unittest.main()

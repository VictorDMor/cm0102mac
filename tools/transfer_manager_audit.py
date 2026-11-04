"""Inventory of the transfer manager in the original Mac executable."""
import argparse
from bisect import bisect_right
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3

from check_jump_tables import AUDITED_EXECUTABLE, TOC
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
REGISTRY = ROOT / "native/transfer_manager_routines.inc"
MODULE = (0x103df0d0, 0x10411e60)

# The preceding 16-byte record initializer at 0x103df0a0 has only the
# 0x103d5f28 caller in the training code. The manager constructor refers to
# its own filename pool at 0x103df0f0. Its final source-bearing routine
# begins at 0x104116e0. After that are its comparators and the club/national
# eligibility predicate at 0x10411e10, called from 0x103f8710. The next
# three routines construct/destroy an Offer (next unit), including a call
# from 0x10411ea0 to the source-bearing offer initializer at 0x104126d0.
# Bounds exclude inter-routine alignment.
ORIGINAL = (
    (0x103df0d0, 0x103df568, 293, "ce392bdb6363a230"),
    (0x103df570, 0x103df788, 132, "8e9fb50df9814114"),
    (0x103df790, 0x103e1218, 1693, "32830c50e96fb968"),
    (0x103e1220, 0x103e25c4, 1250, "7f7136fdb8bba9c0"),
    (0x103e25d0, 0x103e2da8, 500, "b6714d4dc62f0140"),
    (0x103e2db0, 0x103e316c, 238, "e5c466d0f44c1042"),
    (0x103e3170, 0x103e323c, 51, "f418802e8256667f"),
    (0x103e3240, 0x103e340c, 115, "d113a6956c17e198"),
    (0x103e3410, 0x103e3860, 276, "54a361240de173be"),
    (0x103e3860, 0x103e3dd8, 350, "92bd59c4fccb2004"),
    (0x103e3de0, 0x103e4130, 212, "e64e684b7fb09320"),
    (0x103e4130, 0x103e438c, 151, "c3c20c83e2750faa"),
    (0x103e4390, 0x103e44ac, 71, "61f960b3840ae038"),
    (0x103e44b0, 0x103e4740, 164, "7a47de4a4fbd5cbc"),
    (0x103e4740, 0x103e4c90, 340, "16c6a1cbc54dcedf"),
    (0x103e4c90, 0x103e4d68, 54, "d8bd09b6c2367c5a"),
    (0x103e4d70, 0x103e580c, 679, "86c25834705df715"),
    (0x103e5810, 0x103e5b6c, 215, "aa550ca3ab1648ce"),
    (0x103e5b70, 0x103e628c, 455, "0ad8eeea9dc4edac"),
    (0x103e6290, 0x103e6358, 50, "8a8b9d9d556430db"),
    (0x103e6360, 0x103e6464, 64, "41558b5eeb0c16c5"),
    (0x103e6470, 0x103e64dc, 27, "832437ae9c7a71f1"),
    (0x103e64e0, 0x103e68b8, 246, "73e6f204f259bb09"),
    (0x103e68c0, 0x103e6ca4, 249, "df2ae28badd471e3"),
    (0x103e6cb0, 0x103e6e24, 93, "3f875b2fe0288f9d"),
    (0x103e6e30, 0x103e6f6c, 79, "578084868a01aa37"),
    (0x103e6f70, 0x103e70e4, 93, "534efe0d07ed717c"),
    (0x103e70f0, 0x103e76a4, 364, "ef7193191cabb873"),
    (0x103e76b0, 0x103e76b8, 2, "ffdf2f0e562a4021"),
    (0x103e76c0, 0x103e7cec, 394, "a19eefe84ed78392"),
    (0x103e7cf0, 0x103e80d8, 250, "c572667af4c44fac"),
    (0x103e80e0, 0x103e8280, 104, "941ff405b13f45da"),
    (0x103e8280, 0x103e84fc, 159, "656748d7f63485e2"),
    (0x103e8500, 0x103e8bac, 427, "89a5ea40bb5d8c1c"),
    (0x103e8bb0, 0x103e8d24, 93, "77582f23a177d002"),
    (0x103e8d30, 0x103e921c, 314, "96d3c4fbaa5a9cd0"),
    (0x103e9220, 0x103e92b0, 36, "bfbfd94cd48f6e43"),
    (0x103e92b0, 0x103e959c, 187, "90c833df50cd0745"),
    (0x103e95a0, 0x103e9910, 219, "159fd71fd623f8a3"),
    (0x103e9910, 0x103e9ac0, 108, "ebb076a6bcd96864"),
    (0x103e9ac0, 0x103e9cd4, 132, "1a3be6c7cd079193"),
    (0x103e9ce0, 0x103ea244, 344, "c29f1178d43640c2"),
    (0x103ea250, 0x103ea26c, 7, "b315c6a959dbe49a"),
    (0x103ea270, 0x103ea28c, 7, "44d48fa841679ddf"),
    (0x103ea290, 0x103ea348, 46, "4172c0bd736f270c"),
    (0x103ea350, 0x103ea43c, 59, "9dcd074b3c1575e0"),
    (0x103ea440, 0x103ea5f8, 110, "12ebbdcf460e75f5"),
    (0x103ea600, 0x103ea75c, 87, "cb2b8e39d7764e5a"),
    (0x103ea760, 0x103ea83c, 55, "e04067d62d0d7cbe"),
    (0x103ea840, 0x103eae90, 402, "50f3a983d4757f06"),
    (0x103eae90, 0x103eae94, 1, "c79f58af9171dfda"),
    (0x103eaea0, 0x103eb014, 93, "916a73ee9808933f"),
    (0x103eb020, 0x103eb1e4, 113, "3097e5dba6441f5a"),
    (0x103eb1f0, 0x103eb358, 89, "1e2a3231b6cbbb22"),
    (0x103eb360, 0x103eb434, 53, "f0784a281e273c04"),
    (0x103eb440, 0x103eb464, 9, "e59af9b0e85651e4"),
    (0x103eb470, 0x103eb494, 9, "1651dd929bdfebd9"),
    (0x103eb4a0, 0x103eb648, 106, "b8dd1fc01198ab50"),
    (0x103eb650, 0x103ebf48, 574, "6e3916fdd127ea76"),
    (0x103ebf50, 0x103ec0e8, 101, "72e40146e2d86320"),
    (0x103ec0f0, 0x103ec440, 212, "f01829fbb5b1ebdc"),
    (0x103ec440, 0x103ec4ec, 43, "9de35833d90a595e"),
    (0x103ec4f0, 0x103ec5cc, 55, "682eaa784907b2bb"),
    (0x103ec5d0, 0x103ec6c4, 61, "05470dbc2df97fe4"),
    (0x103ec6d0, 0x103ec93c, 155, "335da2d692105ce6"),
    (0x103ec940, 0x103ecaf4, 109, "e67c5fc8fb8a0b3e"),
    (0x103ecb00, 0x103ecb80, 32, "ea5f4754c26a80bc"),
    (0x103ecb80, 0x103ecc18, 37, "f6799cb25790bb0b"),
    (0x103ecc20, 0x103ecd94, 93, "23240e5a59476748"),
    (0x103ecda0, 0x103eceec, 83, "e3ba95e98133d700"),
    (0x103ecef0, 0x103ed090, 104, "216f4d44780c8c4a"),
    (0x103ed090, 0x103ed164, 53, "0f47b2c82eb2b97e"),
    (0x103ed170, 0x103ed520, 235, "5f1b753da05a5218"),
    (0x103ed520, 0x103edef0, 628, "aa994cfe2fab3f98"),
    (0x103edef0, 0x103ee194, 169, "fa98ca504360f0f1"),
    (0x103ee1a0, 0x103ee1a8, 2, "0b8c42cde1de4b4a"),
    (0x103ee1b0, 0x103eebf8, 656, "d4af110143cda14e"),
    (0x103eec00, 0x103eecf4, 61, "7c06c30ac790473d"),
    (0x103eed00, 0x103ef580, 544, "33cc592b499f27a8"),
    (0x103ef580, 0x103ef898, 198, "e029f46aecb3a979"),
    (0x103ef8a0, 0x103efa98, 126, "7e5b310085bae94f"),
    (0x103efaa0, 0x103efec0, 262, "607ae630d504a784"),
    (0x103efec0, 0x103f1b30, 1819, "64951ae75b3c1566"),
    (0x103f1b30, 0x103f1e58, 202, "461d7a41d386fdec"),
    (0x103f1e60, 0x103f1fa8, 82, "432baed9e7526982"),
    (0x103f1fb0, 0x103f20f8, 82, "5e97781eed579ecf"),
    (0x103f2100, 0x103f2248, 82, "8a5a26257ab85b0b"),
    (0x103f2250, 0x103f255c, 194, "26e5c49093755dc5"),
    (0x103f2560, 0x103f27fc, 166, "91d39ff1cfcfe39b"),
    (0x103f2800, 0x103f29f0, 124, "c253cc305359947f"),
    (0x103f29f0, 0x103f2aa0, 43, "a2bfb7931367f8be"),
    (0x103f2aa0, 0x103f2b50, 43, "45bf5126c93b73f3"),
    (0x103f2b50, 0x103f2c00, 43, "6105355eff8a3e81"),
    (0x103f2c00, 0x103f2d4c, 83, "65e5611cfc4a4c73"),
    (0x103f2d50, 0x103f2ebc, 91, "bcb9ecd6d4eaef0c"),
    (0x103f2ec0, 0x103f30f8, 142, "d186a56edae83495"),
    (0x103f3100, 0x103f353c, 271, "142556e1e610ef4a"),
    (0x103f3540, 0x103f3668, 72, "341910b292e2f947"),
    (0x103f3670, 0x103f3bf0, 349, "a46dadd020a83596"),
    (0x103f3bf0, 0x103f4238, 402, "e03afb5d4a4d25f8"),
    (0x103f4240, 0x103f43a4, 88, "04e547feb1ccdff8"),
    (0x103f43b0, 0x103f4594, 119, "b3ad760fddd022e0"),
    (0x103f45a0, 0x103f4bf4, 403, "b271373fcd2fd549"),
    (0x103f4c00, 0x103f4d10, 67, "c587ac652045ad41"),
    (0x103f4d10, 0x103f4fc8, 174, "f411b0c88fd06c33"),
    (0x103f4fd0, 0x103f5a4c, 663, "6e928dbbfac390d7"),
    (0x103f5a50, 0x103f5bf8, 106, "8b684a8b6274bb3f"),
    (0x103f5c00, 0x103f5d1c, 71, "4b32db80ac543985"),
    (0x103f5d20, 0x103f5e50, 76, "8ff96068c9bea848"),
    (0x103f5e50, 0x103f6184, 203, "ff63d68512464e0b"),
    (0x103f6190, 0x103f623c, 42, "ef2175aef9a129c2"),
    (0x103f6240, 0x103f80e4, 1961, "73c6bc68eab14b1d"),
    (0x103f80f0, 0x103f8378, 162, "557d73846a59ad21"),
    (0x103f8380, 0x103f83bc, 15, "dc822f56d3b4ca34"),
    (0x103f83c0, 0x103f8810, 276, "1658885b1b0d42c4"),
    (0x103f8810, 0x103f8818, 2, "6bff732a90deed40"),
    (0x103f8820, 0x103f89d4, 108, "389948ad36727435"),
    (0x103f89e0, 0x103f8cc0, 184, "45df6eb00f5a8f82"),
    (0x103f8cc0, 0x103f9200, 330, "35f1873ddb6987bb"),
    (0x103f9200, 0x103f93dc, 118, "9c42c27fed7f34d9"),
    (0x103f93e0, 0x103f9544, 87, "2ad54309a711d272"),
    (0x103f9550, 0x103f9778, 136, "945a275e149e4f96"),
    (0x103f9780, 0x103f97ac, 11, "f300a56ed0b05b74"),
    (0x103f97b0, 0x103f98cc, 71, "33f1dce340167574"),
    (0x103f98d0, 0x103f9d60, 292, "e136c3c85ec30187"),
    (0x103f9d60, 0x103fa284, 328, "170d14cca1a60aff"),
    (0x103fa290, 0x103fa840, 361, "eea73a903414afaf"),
    (0x103fa840, 0x103fa848, 2, "c6091064e61ed31f"),
    (0x103fa850, 0x103fa8d4, 32, "f7dfdb41478094e2"),
    (0x103fa8e0, 0x103faba0, 175, "fb58156edf74cf63"),
    (0x103faba0, 0x103fac48, 41, "2bc9378d6677f6fa"),
    (0x103fac50, 0x103fac54, 1, "f7fb9270a9dbebcf"),
    (0x103fac60, 0x103fac64, 1, "03fcc045f5f092a1"),
    (0x103fac70, 0x103fafec, 223, "b968de428e050a5e"),
    (0x103faff0, 0x103fb12c, 79, "38dd078cdb5d3909"),
    (0x103fb130, 0x103fb384, 149, "1a192dbed5fc2e3b"),
    (0x103fb390, 0x103fb3f8, 26, "2ecacb1c1868537c"),
    (0x103fb400, 0x103fb504, 65, "1927aed1bd38f9a3"),
    (0x103fb510, 0x103fb5a0, 36, "71da5227ea4cb4e7"),
    (0x103fb5a0, 0x103fb630, 36, "22042b56cc5c0a54"),
    (0x103fb630, 0x103fb838, 129, "296242e01e216f0d"),
    (0x103fb840, 0x103fc704, 941, "f4b5f9b8e762e077"),
    (0x103fc710, 0x103fd414, 831, "be443cb8c8458095"),
    (0x103fd420, 0x103fdae0, 432, "7f866e84b190d22a"),
    (0x103fdae0, 0x103fdc14, 77, "b80a1970d255285f"),
    (0x103fdc20, 0x103fe194, 348, "43d2ac95a880a44d"),
    (0x103fe1a0, 0x103ffe74, 1844, "c0cd677800a4003c"),
    (0x103ffe80, 0x104001e0, 216, "620ec257c3c41d1f"),
    (0x104001e0, 0x104019cc, 1531, "0468182b5b1ed8a4"),
    (0x104019d0, 0x10401c90, 175, "0aa8c03bcd5a865a"),
    (0x10401c90, 0x104024d8, 529, "e033201a48fb7055"),
    (0x104024e0, 0x104026b0, 115, "ac32a4fe33ba2ad9"),
    (0x104026b0, 0x10402b88, 310, "c8a00d2d6c1cce4f"),
    (0x10402b90, 0x10402e68, 182, "b71a0528d3ec2d2f"),
    (0x10402e70, 0x10403250, 248, "8e266ff4d854b112"),
    (0x10403250, 0x10403458, 130, "edfcdc2ae89a2532"),
    (0x10403460, 0x10403884, 265, "b79c2206a15f8d4d"),
    (0x10403890, 0x10403a90, 128, "7ba48cf8ec960cff"),
    (0x10403a90, 0x10403e1c, 227, "d55390ddbd07e9a5"),
    (0x10403e20, 0x1040452c, 448, "1ba0ef0684317782"),
    (0x10404530, 0x104058e0, 1260, "ae864959dc97e2a5"),
    (0x104058e0, 0x10405c98, 238, "1178cc7560de6499"),
    (0x10405ca0, 0x10406910, 796, "763f286665daba1b"),
    (0x10406910, 0x10406aec, 118, "d5f98b74a1b195b3"),
    (0x10406af0, 0x10406dfc, 195, "730998ddce41f761"),
    (0x10406e00, 0x10406fc8, 114, "ef0c9cf4ac2f61f3"),
    (0x10406fd0, 0x1040745c, 291, "4b770b0823f357e6"),
    (0x10407460, 0x10407a50, 380, "5f3c9d58feaa136a"),
    (0x10407a50, 0x10408f74, 1353, "d7c3535fc21473c7"),
    (0x10408f80, 0x104091f8, 158, "af9f83ac116df184"),
    (0x10409200, 0x1040940c, 131, "513ff76bf43fa233"),
    (0x10409410, 0x104099e8, 374, "a8f7175508fbb06e"),
    (0x104099f0, 0x10409bd0, 120, "0969758d77d04695"),
    (0x10409bd0, 0x1040a04c, 287, "858b5940cf69e04c"),
    (0x1040a050, 0x1040a280, 139, "ae43ae5400ba7376"),
    (0x1040a280, 0x1040b46c, 1147, "a5bbcba704269146"),
    (0x1040b470, 0x1040bc04, 485, "3ecfd979d57a604f"),
    (0x1040bc10, 0x1040c1a0, 356, "b6e04548f8db74fc"),
    (0x1040c1a0, 0x1040c220, 32, "4743bc1c3a69d868"),
    (0x1040c220, 0x1040c578, 213, "8d103cf442cc26ff"),
    (0x1040c580, 0x1040ce38, 557, "d0ab3810c5a226a0"),
    (0x1040ce40, 0x1040d0c8, 162, "d4c65fb3adb4e1dc"),
    (0x1040d0d0, 0x1040d5b4, 313, "86e7ed9cb9a4a922"),
    (0x1040d5c0, 0x1040d820, 152, "782246b60203356c"),
    (0x1040d820, 0x1040ddb8, 358, "107344613cd054a7"),
    (0x1040ddc0, 0x1040df6c, 106, "c82598203c006d5f"),
    (0x1040df70, 0x1040e2d0, 216, "2b52310751a5fc1c"),
    (0x1040e2d0, 0x1040e490, 111, "fff3b539798f0eb7"),
    (0x1040e490, 0x1040ea40, 364, "2a72c9070184ffff"),
    (0x1040ea40, 0x1040ed98, 214, "44e9628399a0863a"),
    (0x1040eda0, 0x1040f270, 308, "49c91b29646de6b8"),
    (0x1040f270, 0x1040f444, 117, "0cf57761c668180a"),
    (0x1040f450, 0x1040f778, 202, "39fd3863da7a054d"),
    (0x1040f780, 0x1040f984, 129, "8087e5c75dd38e9a"),
    (0x1040f990, 0x1040fd94, 257, "59b9500797493ef3"),
    (0x1040fda0, 0x1040fff8, 150, "2f37d7e97ec90b47"),
    (0x10410000, 0x104101e8, 122, "d54f2564763003b3"),
    (0x104101f0, 0x104104fc, 195, "28e71c574021b8b4"),
    (0x10410500, 0x104106d0, 116, "fabe603e780e7605"),
    (0x104106d0, 0x104108bc, 123, "693f1daf5a988e9b"),
    (0x104108c0, 0x10410a58, 102, "9616e5c1d0260763"),
    (0x10410a60, 0x10410e4c, 251, "f13f7fa8c35e58f9"),
    (0x10410e50, 0x104113dc, 355, "d205217583e7e474"),
    (0x104113e0, 0x104116d8, 190, "0b219819bc90df7a"),
    (0x104116e0, 0x104119d4, 189, "5cb981bd6a1813df"),
    (0x104119e0, 0x10411a08, 10, "c3ba53bd9b7bdbc7"),
    (0x10411a10, 0x10411a20, 4, "766b8008b0536946"),
    (0x10411a20, 0x10411a88, 26, "af7bd155635901e9"),
    (0x10411a90, 0x10411b6c, 55, "d52b3dd69913d23a"),
    (0x10411b70, 0x10411c40, 52, "a390fe52ad1af62c"),
    (0x10411c40, 0x10411c80, 16, "962807457805c252"),
    (0x10411c80, 0x10411cd0, 20, "4488300b2211fa43"),
    (0x10411cd0, 0x10411e0c, 79, "a9ea2228a47cec1c"),
    (0x10411e10, 0x10411e60, 20, "56c72f65842f632a"),
)
ENTRIES = tuple(start for start, _, _, _ in ORIGINAL)
BOUNDS = {start: end for start, end, _, _ in ORIGINAL}

# PackedData descriptors store the entry followed by the program's TOC.
CALLBACKS = {
    0x108b93ac: 0x10411a90, 0x108b93b4: 0x10411a20,
    0x108b93bc: 0x10411c40, 0x108b93c4: 0x103fac60,
    0x108b93cc: 0x103fac50, 0x108b93d4: 0x103fa840,
    0x108b93dc: 0x103f9780, 0x108b93e4: 0x103f8810,
    0x108b93ec: 0x10411cd0, 0x108b93f4: 0x10411b70,
    0x108b93fc: 0x10411a10, 0x108b9404: 0x104119e0,
    0x108b940c: 0x103eae90, 0x108b9414: 0x103e76b0,
    0x108b941c: 0x10411c80,
}

# bctr -> (TOC slot, table address, case count, table digest).
SWITCHES = {
    0x103f2ca0: (-0x4bb8, 0x108cf27c, 10, "8a94da21d761562f3d3bff02ec286d04f35d5b2f394ce464e58c9a386fed4eda"),
    0x103f4800: (-0x4bc4, 0x108cf2a4, 20, "982b3f5322561cee2c5a5e3b53cb1e2e9f27a2d3c3e62174e884a9f4be121d41"),
    0x103f6434: (-0x4bc8, 0x108cf2f4, 8, "6baab15b564d6d72be0d00d545c6b0ae27403c44e1cf37525cd5cee2fe10d5d2"),
}
# The word at 0x10406a9c is skipped by the preceding unconditional jump.
SKIPPED_BRANCH = 0x10406a9c
# The only aligned data words in the range outside the descriptors and
# jump tables are part of a packed resource entry and a packed offset table.
NON_POINTER_WORDS = {0x108a9538: 0x10400000, 0x108bfe88: 0x10400100}

ROUTINE = re.compile(r"TRANSFER_MANAGER_ROUTINE\((0x[0-9a-f]+),")


def registered(path=REGISTRY):
    entries = tuple(int(entry, 16) for entry in ROUTINE.findall(path.read_text()))
    if entries != tuple(sorted(set(entries))) or set(entries) - BOUNDS.keys():
        raise ValueError("Invalid or duplicate transfer-manager replacement entry")
    return tuple((start, end, fingerprint) for start, end, _, fingerprint in ORIGINAL if start in entries)


TRANSFER_MANAGER_ENTRIES = tuple(start for start, _, _ in registered())


def require_transfer_manager_index(db):
    for start, end, count, expected in ORIGINAL:
        records = routine_records(db, start, end)
        if len(records) != count or digest(records) != expected:
            raise ValueError(f"TRANSFER_MANAGER.CPP original differs at 0x{start:x}")


def transfer_manager_hook(entry):
    return f"if (try_modern_transfer_manager(s, 0x{entry:x})) return true;"


def require_transfer_manager_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in TRANSFER_MANAGER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook = transfer_manager_hook(entry)
        label = f"i_{entry:x}:"
        if label not in code:
            raise ValueError(f"Missing transfer-manager label at 0x{entry:x}")
        fragment = code.split(label, 1)[1].split(f"i_{entry + 4:x}:", 1)[0]
        step = re.search(rf"(?:s|counter)\.step\(0x{entry:x}\);", fragment)
        if (code.count(hook) != 1 or hook not in fragment or not step or
                fragment.index(hook) > step.start() or "try_modern_transfer_offer" in fragment):
            raise ValueError(f"Missing or misplaced transfer-manager hook at 0x{entry:x}")


def containing_entry(address):
    at = bisect_right(ENTRIES, address) - 1
    if at < 0 or address >= BOUNDS[ENTRIES[at]]:
        return None
    return ENTRIES[at]


def original_segments(export):
    program = json.loads((export / "program.json").read_text())
    if program["executableSHA256"] != AUDITED_EXECUTABLE:
        raise ValueError("Wrong executable for the TRANSFER_MANAGER.CPP audit")
    return [(s["address"], (export / s["file"]).read_bytes())
            for s in program["segments"] if s.get("file")]


def read_original(segments, address, width):
    for start, contents in segments:
        if start <= address and address + width <= start + len(contents):
            return contents[address - start:address - start + width]
    raise ValueError(f"Missing original bytes at 0x{address:x}")


def validate_export(export):
    segments = original_segments(export)
    def word(address):
        return int.from_bytes(read_original(segments, address, 4), "big")

    if (word(TOC - 0x4b9c) != 0x10816290 or
            read_original(segments, 0x108162c7, 21) != b"TRANSFER_MANAGER.CPP\0" or
            word(TOC - 0x4b88) != 0x10827500 or
            read_original(segments, 0x10827500, 19) != b"TRANSFER_OFFER.CPP\0"):
        raise ValueError("Changed transfer source pools")

    targets = {}
    for site, (slot, table, count, fingerprint) in SWITCHES.items():
        if (word(TOC + slot) != table or
                hashlib.sha256(read_original(segments, table, count * 4)).hexdigest() != fingerprint):
            raise ValueError(f"Changed switch table at 0x{site:x}")
        targets[site] = [word(table + i * 4) for i in range(count)]

    vectors, words = {}, {}
    for base, contents in segments:
        for match in re.finditer(b"\x10[\x3d-\x41]", contents):
            offset = match.start()
            if offset + 4 > len(contents) or (base + offset) % 4:
                continue
            entry = int.from_bytes(contents[offset:offset + 4], "big")
            if MODULE[0] <= entry < MODULE[1]:
                address = base + offset
                words[address] = entry
                if int.from_bytes(contents[offset + 4:offset + 8], "big") == TOC:
                    vectors[address] = entry
    if vectors != CALLBACKS:
        raise ValueError("Changed transfer-manager transition vectors")
    table_words = {table + 4 * i: target
                   for site, (_, table, _, _) in SWITCHES.items()
                   for i, target in enumerate(targets[site])}
    if words != CALLBACKS | table_words | NON_POINTER_WORDS:
        raise ValueError("Unclassified aligned pointer to TRANSFER_MANAGER.CPP")

    with closing(sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)) as db:
        require_transfer_manager_index(db)
        refs = [address for (address,) in db.execute(
            "SELECT address FROM instructions WHERE body LIKE '%-0x4b9c(r2)%' ORDER BY address")]
        if (len(refs) != 78 or refs[0] != 0x103df0f0 or refs[-1] != 0x10411700
                or any(containing_entry(address) is None for address in refs)):
            raise ValueError("Changed references to the TRANSFER_MANAGER.CPP source pool")
        records = {r["address"]: r for r in routine_records(db, *MODULE)}
        calls = set(CALLBACKS.values())
        for address, body in db.execute(
                "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                "WHERE f.value>=? AND f.value<?", MODULE):
            item = json.loads(body)
            for target in item["flows"]:
                if not MODULE[0] <= target < MODULE[1]:
                    continue
                if item["flow"] == "UNCONDITIONAL_CALL":
                    calls.add(target)
                elif containing_entry(address) is None or containing_entry(address) != containing_entry(target):
                    raise ValueError(f"Cross-entry branch 0x{address:x} -> 0x{target:x}")
        if calls != set(ENTRIES):
            raise ValueError("Direct call and callback roots do not match the transfer-manager inventory")
        if db.execute(
                "SELECT address FROM instructions WHERE json_extract(body,'$.fallthrough')>=? "
                "AND json_extract(body,'$.fallthrough')<? AND (address<? OR address>=?) LIMIT 1",
                (*MODULE, *MODULE)).fetchone():
            raise ValueError("External fallthrough enters TRANSFER_MANAGER.CPP")
        if not db.execute("SELECT 1 FROM instructions WHERE address=0x10411fc0 "
                          "AND json_extract(body,'$.assembly')='lwz r31,-0x4b88(r2)'").fetchone():
            raise ValueError("Missing next unit's source reference")

    reached = set()
    for start in ENTRIES:
        pending = [start]
        while pending:
            address = pending.pop()
            if containing_entry(address) != start or address not in records:
                raise ValueError(f"Invalid flow from 0x{start:x} to 0x{address:x}")
            if address in reached:
                continue
            reached.add(address)
            item = records[address]
            if item["flow"] == "COMPUTED_JUMP":
                pending.extend(targets[address])
            elif "CALL" not in item["flow"]:
                pending.extend(item["flows"])
            if item.get("fallthrough") is not None:
                pending.append(item["fallthrough"])
    if reached != records.keys():
        raise ValueError("Unowned transfer-manager export records")
    if {a for a, r in records.items() if r["flow"] == "COMPUTED_JUMP"} != targets.keys():
        raise ValueError("Unaccounted indirect control flow")

    for address in range(*MODULE, 4):
        if address not in records and address != SKIPPED_BRANCH and word(address) not in (0, 0x60000000):
            raise ValueError(f"Unaccounted instruction at 0x{address:x}")
    if records[SKIPPED_BRANCH - 4]["flows"] != [SKIPPED_BRANCH + 4] or word(SKIPPED_BRANCH) != 0x4800003c:
        raise ValueError("Changed unreachable branch in transfer-manager news")
    return len(reached)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", type=Path, default=EXPORT)
    parser.add_argument("--entries", action="store_true")
    args = parser.parse_args()
    if args.entries:
        for entry in ENTRIES:
            print(f"0x{entry:x}")
    else:
        count = validate_export(args.export)
        print(f"TRANSFER_MANAGER.CPP: {len(ENTRIES)} entries, {count} instructions, "
              f"{len(CALLBACKS)} transition vectors, {len(SWITCHES)} switch tables")


if __name__ == "__main__":
    main()

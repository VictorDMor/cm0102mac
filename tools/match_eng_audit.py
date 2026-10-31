"""MATCH_ENG.CPP original inventory and incremental plain-C++ ownership.

The class constructor (0x101d16a0) precedes the first load of the source pool
(TOC -0x52cc, 0x101d1798); the last load of MATCH_DAY's pool (-0x52f0) lies in
the routine before it. MATCH_EVENTS' constructor (0x101fef60) loads the next
module's pool (-0x52a0). Every entry is a direct-call target or the one
transition vector; no flow enters or leaves a routine except by call or return.
Seven bounded switch tables hold local labels, not additional entries.
Original listings and pointer/flow evidence remain private under .local/.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x101d16a0, 0x101fef60)
TOC = 0x108bba90
DATA_BASE = 0x108b3a90
SOURCE_POOL_SLOT = -0x52cc
# Entry, exclusive next-entry boundary, indexed instruction count, source digest.
CONTRACTS = (
    (0x101d16a0, 0x101d1760, 45, "625664953717fe84"),
    (0x101d1760, 0x101d3ac0, 2264, "8c479d0ddeb590d8"),
    (0x101d3ac0, 0x101d3ad0, 3, "a3b01c2818a815d1"),
    (0x101d3ad0, 0x101d3b40, 27, "01eb537e4103b12c"),
    (0x101d3b40, 0x101d3b50, 2, "7530c6327914fb09"),
    (0x101d3b50, 0x101d3b60, 2, "fad8f7f3111e324b"),
    (0x101d3b60, 0x101d3bf0, 35, "de0278406902df8c"),
    (0x101d3bf0, 0x101d3c40, 18, "fd1f3be91adf29c7"),
    (0x101d3c40, 0x101d3c50, 2, "a7aefd961d173054"),
    (0x101d3c50, 0x101d3c80, 12, "e7f7f5e238756951"),
    (0x101d3c80, 0x101d3c90, 2, "39695edfe303546b"),
    (0x101d3c90, 0x101d3e50, 110, "3b626f1043a1c2e3"),
    (0x101d3e50, 0x101d3f60, 66, "81fa6f92594e8f8e"),
    (0x101d3f60, 0x101d5660, 1470, "bf3eaf008c6adf0b"),
    (0x101d5660, 0x101d5670, 2, "426229115462b664"),
    (0x101d5670, 0x101d5680, 2, "0b4dda9a4c529633"),
    (0x101d5680, 0x101d5690, 2, "a174177c6f2112bf"),
    (0x101d5690, 0x101d56a0, 2, "5abc34f74541bd3b"),
    (0x101d56a0, 0x101d56b0, 2, "5d62dd1353be9bea"),
    (0x101d56b0, 0x101d56c0, 2, "f58b90ed9e623511"),
    (0x101d56c0, 0x101d56d0, 2, "c2d2b98e4f0a970a"),
    (0x101d56d0, 0x101d56e0, 2, "94a123dece1e8a52"),
    (0x101d56e0, 0x101d56f0, 2, "7c9c5a0ec67f922b"),
    (0x101d56f0, 0x101d5700, 2, "51edf1a82dcf9de4"),
    (0x101d5700, 0x101d5730, 12, "719fcd255ba8f37b"),
    (0x101d5730, 0x101d5740, 2, "88d00e9340ba4420"),
    (0x101d5740, 0x101d5750, 3, "753f8213ce5cbb41"),
    (0x101d5750, 0x101d5760, 2, "1135fc320b30cb98"),
    (0x101d5760, 0x101d57c0, 22, "c201c3d534a0ddad"),
    (0x101d57c0, 0x101d57d0, 2, "853295095026e524"),
    (0x101d57d0, 0x101d5810, 13, "c4969076316ccdeb"),
    (0x101d5810, 0x101d5870, 21, "246b69370c7d661a"),
    (0x101d5870, 0x101d5880, 2, "2197d311b75ff4d0"),
    (0x101d5880, 0x101d5980, 64, "b7d7a8f11dd7594a"),
    (0x101d5980, 0x101d6690, 835, "cef4f43584f17c7f"),
    (0x101d6690, 0x101d66a0, 3, "c6bb7e9b40939e24"),
    (0x101d66a0, 0x101d66b0, 2, "b0cd72c860d4d30d"),
    (0x101d66b0, 0x101d66c0, 2, "5ed9a692acfda0b8"),
    (0x101d66c0, 0x101d66d0, 2, "7f2c1ea6dc90d18b"),
    (0x101d66d0, 0x101d66e0, 2, "3ccf0591502d6260"),
    (0x101d66e0, 0x101d66f0, 4, "46261d5ef06a3bbd"),
    (0x101d66f0, 0x101d6710, 8, "1d05a0d04081c746"),
    (0x101d6710, 0x101d6770, 22, "b7c2bb0d00025195"),
    (0x101d6770, 0x101d6780, 2, "c0d3401604c2c689"),
    (0x101d6780, 0x101d6860, 56, "cada48f5d7ff7783"),
    (0x101d6860, 0x101d68f0, 35, "da01060a3b6fbfab"),
    (0x101d68f0, 0x101d6930, 15, "a97bd788fead8568"),
    (0x101d6930, 0x101d6980, 19, "f009196156037874"),
    (0x101d6980, 0x101d69c0, 16, "5c23cd3a9cebf596"),
    (0x101d69c0, 0x101d6a90, 49, "605978f928d71424"),
    (0x101d6a90, 0x101d6da0, 195, "fbf4514ca3888bd3"),
    (0x101d6da0, 0x101d6f90, 121, "302f51e4d8a19278"),
    (0x101d6f90, 0x101d70d0, 79, "e35da828c1fcd940"),
    (0x101d70d0, 0x101d7d30, 790, "a251b1106737fd1b"),
    (0x101d7d30, 0x101d7d90, 22, "be0dc8a9fd92f19c"),
    (0x101d7d90, 0x101d7da0, 2, "53586d0a727bf2d0"),
    (0x101d7da0, 0x101d7db0, 2, "6f4c9e42a801054e"),
    (0x101d7db0, 0x101d7dc0, 2, "052391f88bdd1a75"),
    (0x101d7dc0, 0x101d8100, 208, "794066fbfad23677"),
    (0x101d8100, 0x101d8110, 2, "aa3f779737e1a648"),
    (0x101d8110, 0x101d8120, 2, "47dd07249a69797e"),
    (0x101d8120, 0x101d8260, 77, "4a746c73f3570cc9"),
    (0x101d8260, 0x101d8d60, 704, "9b430bade4c1e411"),
    (0x101d8d60, 0x101d8d70, 2, "6992432cdbb255ea"),
    (0x101d8d70, 0x101d8f20, 105, "5e90f5c74612d79f"),
    (0x101d8f20, 0x101d9080, 88, "2b92b4142d28d681"),
    (0x101d9080, 0x101d91b0, 73, "38aade3b06ae2ec4"),
    (0x101d91b0, 0x101d93a0, 123, "0d4c5a03401b5a8f"),
    (0x101d93a0, 0x101d93b0, 2, "793f53384b99694a"),
    (0x101d93b0, 0x101d9460, 42, "28f310740b174e9f"),
    (0x101d9460, 0x101d9510, 42, "016ce97b7cd3f1ad"),
    (0x101d9510, 0x101d95a0, 35, "4c206514b097ecda"),
    (0x101d95a0, 0x101da020, 671, "460372a0c5aa4151"),
    (0x101da020, 0x101da3c0, 230, "8456716aa6843d94"),
    (0x101da3c0, 0x101da5a0, 117, "5fe2a1491f13e7f2"),
    (0x101da5a0, 0x101da6d0, 75, "dd60101cbce0f630"),
    (0x101da6d0, 0x101da6f0, 7, "4a58d16ad1c852df"),
    (0x101da6f0, 0x101da750, 23, "5290e16706ba9fcd"),
    (0x101da750, 0x101da760, 2, "6005205c3a7bfa05"),
    (0x101da760, 0x101da820, 47, "9fa39535c42ae81c"),
    (0x101da820, 0x101db320, 701, "310736aaf122f9e4"),
    (0x101db320, 0x101db330, 2, "5b17ccb6875cc2f4"),
    (0x101db330, 0x101db340, 2, "ab8c7684685306b4"),
    (0x101db340, 0x101de040, 2879, "49374e3fcb439a37"),
    (0x101de040, 0x101de060, 8, "e1346ba24b8d9714"),
    (0x101de060, 0x101de080, 8, "66fc1ee38262d22e"),
    (0x101de080, 0x101de0a0, 8, "9dd48ee5e20fbd49"),
    (0x101de0a0, 0x101de340, 165, "6f6e33550a720cbf"),
    (0x101de340, 0x101de610, 179, "ba82580f98146ed5"),
    (0x101de610, 0x101de8b0, 165, "1e4fff7604f9998c"),
    (0x101de8b0, 0x101deb50, 165, "71c3243971846c14"),
    (0x101deb50, 0x101dee30, 181, "ff4dbe00c0b2b8bb"),
    (0x101dee30, 0x101df110, 181, "7b65f5cfc7085099"),
    (0x101df110, 0x101e15e0, 2354, "62cc031cc6fec57e"),
    (0x101e15e0, 0x101e1640, 22, "b1caffe9e097562d"),
    (0x101e1640, 0x101e1a50, 257, "08d090c63c70b099"),
    (0x101e1a50, 0x101e1bd0, 93, "718c6a63447a6015"),
    (0x101e1bd0, 0x101e2100, 329, "776edb93b2a4259e"),
    (0x101e2100, 0x101e22e0, 118, "d6943f06776bfe12"),
    (0x101e22e0, 0x101e25a0, 174, "01391f6e886fd300"),
    (0x101e25a0, 0x101e28b0, 194, "58a398ba9fe71c80"),
    (0x101e28b0, 0x101e3060, 491, "d3c110ca949c3c21"),
    (0x101e3060, 0x101e3150, 60, "c6b677b17be12316"),
    (0x101e3150, 0x101e3180, 9, "f4b3be954477ee5e"),
    (0x101e3180, 0x101e31a0, 8, "c9829eea316483f2"),
    (0x101e31a0, 0x101e32e0, 77, "e0d660f09930a586"),
    (0x101e32e0, 0x101e3d50, 668, "45db6409d713cfd6"),
    (0x101e3d50, 0x101e3de0, 35, "135b9f03deb8f70f"),
    (0x101e3de0, 0x101e3e70, 35, "9bf8bd6ac3b5d388"),
    (0x101e3e70, 0x101e3f00, 35, "b415ef08032c6f84"),
    (0x101e3f00, 0x101e3f10, 2, "e3a1d7b87c978445"),
    (0x101e3f10, 0x101e4020, 68, "a26d2a920a273267"),
    (0x101e4020, 0x101e4130, 66, "b4749159d15a0917"),
    (0x101e4130, 0x101e4e10, 822, "ef4878cb8f709e56"),
    (0x101e4e10, 0x101e4ea0, 36, "1e23b354520790c0"),
    (0x101e4ea0, 0x101e4ed0, 11, "62860c8fe795cfb7"),
    (0x101e4ed0, 0x101e5070, 101, "ab9d596d17131995"),
    (0x101e5070, 0x101e5080, 2, "2aebb30cdd0670f2"),
    (0x101e5080, 0x101e50b0, 11, "8ed3f07bcbef9ec2"),
    (0x101e50b0, 0x101e50c0, 1, "414b183d13233828"),
    (0x101e50c0, 0x101e53a0, 183, "0bd16ea907416530"),
    (0x101e53a0, 0x101e59b0, 387, "e6f65f48938357be"),
    (0x101e59b0, 0x101e6420, 667, "9c846ea2d7c3a7ec"),
    (0x101e6420, 0x101e66b0, 163, "faed2906b58d839b"),
    (0x101e66b0, 0x101e68b0, 128, "f1103f97e7162405"),
    (0x101e68b0, 0x101e6c50, 229, "769cd2efe8451eb4"),
    (0x101e6c50, 0x101e88f0, 1830, "f7c19f78b19458b2"),
    (0x101e88f0, 0x101e8900, 2, "de88be6a03fe143a"),
    (0x101e8900, 0x101e8910, 2, "2faa9a78543494b5"),
    (0x101e8910, 0x101e8920, 2, "5d27f727f415504c"),
    (0x101e8920, 0x101e8930, 2, "314ef81ac6d03e50"),
    (0x101e8930, 0x101e8940, 3, "4da3cd7cf0923bf6"),
    (0x101e8940, 0x101e8950, 3, "54e3c92b3514494f"),
    (0x101e8950, 0x101eaf10, 2413, "f38c158ea1703228"),
    (0x101eaf10, 0x101eaf50, 13, "3f59da294550fa81"),
    (0x101eaf50, 0x101eaf60, 2, "7576706a66d0bbb9"),
    (0x101eaf60, 0x101ec7a0, 1549, "991e4fa0274f9dc5"),
    (0x101ec7a0, 0x101ec7b0, 2, "8ca8ea08354116da"),
    (0x101ec7b0, 0x101ec890, 54, "49e22320681d9e4f"),
    (0x101ec890, 0x101ee510, 1821, "7ce3c82042edd99a"),
    (0x101ee510, 0x101ee520, 2, "ef06e8851f161355"),
    (0x101ee520, 0x101ee530, 2, "907ee24bbf5abfad"),
    (0x101ee530, 0x101ee610, 55, "a9a8750f482a5a28"),
    (0x101ee610, 0x101ee680, 26, "375629258f2f5f1b"),
    (0x101ee680, 0x101ee770, 59, "1f4f4f2bd2412973"),
    (0x101ee770, 0x101ee9c0, 145, "65d04fbd1636cd89"),
    (0x101ee9c0, 0x101ee9f0, 10, "5d7490ca0267212c"),
    (0x101ee9f0, 0x101eea30, 16, "9836c9fbb7280785"),
    (0x101eea30, 0x101eeac0, 33, "43e28141e01fbb7f"),
    (0x101eeac0, 0x101eecb0, 121, "2cdeab5844153a7e"),
    (0x101eecb0, 0x101eee20, 92, "c65d8d56567cd147"),
    (0x101eee20, 0x101eeeb0, 34, "90619c43798d5aa4"),
    (0x101eeeb0, 0x101ef3c0, 324, "3fea805c552fa9b8"),
    (0x101ef3c0, 0x101ef8d0, 323, "5e85cc3b4ff8dce1"),
    (0x101ef8d0, 0x101ef9a0, 51, "717178b09971009c"),
    (0x101ef9a0, 0x101f0090, 443, "20af07e9e5a8dfe4"),
    (0x101f0090, 0x101f09d0, 590, "3ca3129a89eab068"),
    (0x101f09d0, 0x101f0ad0, 61, "cb6f9c00fa2c3fb7"),
    (0x101f0ad0, 0x101f0bf0, 70, "14cd980cb22d3ea0"),
    (0x101f0bf0, 0x101f0c00, 4, "e4c88a6393f117a7"),
    (0x101f0c00, 0x101f0dd0, 116, "e9182a12e30c8ea3"),
    (0x101f0dd0, 0x101f0e80, 43, "8baeb8fb1440b42f"),
    (0x101f0e80, 0x101f0ed0, 19, "5810d21bb336b5ba"),
    (0x101f0ed0, 0x101f0f20, 19, "28e79e01d23f4a85"),
    (0x101f0f20, 0x101f1100, 120, "8659fde1675daca7"),
    (0x101f1100, 0x101f14b0, 233, "2a54bab3a3b4514f"),
    (0x101f14b0, 0x101f1520, 25, "5d1361b10b8c084e"),
    (0x101f1520, 0x101f1530, 2, "fa792f388f07bd11"),
    (0x101f1530, 0x101f1860, 203, "66df54b480f08982"),
    (0x101f1860, 0x101f1870, 2, "fb70604234656277"),
    (0x101f1870, 0x101f1880, 2, "8608deb0f9176766"),
    (0x101f1880, 0x101f1930, 43, "ad79d340b4dbae98"),
    (0x101f1930, 0x101f1940, 2, "40a3e8c53877150f"),
    (0x101f1940, 0x101f1950, 2, "b4d2e994dfff00d0"),
    (0x101f1950, 0x101f1dc0, 284, "1c8fc32053303d7a"),
    (0x101f1dc0, 0x101f1de0, 5, "aa35f58d453cc7be"),
    (0x101f1de0, 0x101f1e10, 11, "deb471d3b3f66123"),
    (0x101f1e10, 0x101f21c0, 236, "4b648fe99d152565"),
    (0x101f21c0, 0x101f21d0, 2, "f9cc170b52dbcabb"),
    (0x101f21d0, 0x101f2400, 137, "9e83bdfb2b110a9d"),
    (0x101f2400, 0x101f3480, 1055, "ecfa99dba0356baf"),
    (0x101f3480, 0x101f3a30, 362, "bb112d80589bbcd5"),
    (0x101f3a30, 0x101f4860, 905, "9a6593d20ae0d3c9"),
    (0x101f4860, 0x101f4900, 39, "99ac88ff60a1997b"),
    (0x101f4900, 0x101f5050, 467, "c0934395859e5e67"),
    (0x101f5050, 0x101f50c0, 26, "19baeffb3bb5f001"),
    (0x101f50c0, 0x101f5110, 20, "e8f46f0868d4b7da"),
    (0x101f5110, 0x101f5180, 28, "898a57da85b4e82b"),
    (0x101f5180, 0x101f51e0, 24, "d6db030528709fed"),
    (0x101f51e0, 0x101f5230, 20, "8b70ae718681f0a2"),
    (0x101f5230, 0x101f5280, 20, "f83f3109c0596a78"),
    (0x101f5280, 0x101f5350, 50, "4693e8c852acc409"),
    (0x101f5350, 0x101f6940, 1404, "39ec180a5112f251"),
    (0x101f6940, 0x101f6c60, 198, "b03ee673943c0261"),
    (0x101f6c60, 0x101f7100, 294, "5179c1dad7758994"),
    (0x101f7100, 0x101f7280, 94, "6faa6876ecfac40b"),
    (0x101f7280, 0x101f7500, 159, "671abcd5cc876d61"),
    (0x101f7500, 0x101f75a0, 39, "0b6718cb1f14c309"),
    (0x101f75a0, 0x101f7c10, 409, "d066c570ab561fe2"),
    (0x101f7c10, 0x101f84e0, 561, "ad386c9894d02603"),
    (0x101f84e0, 0x101f93d0, 955, "26bb30a9ca70df6e"),
    (0x101f93d0, 0x101f93f0, 5, "89146985e4f461bd"),
    (0x101f93f0, 0x101f9400, 2, "68c11a9398fcc80f"),
    (0x101f9400, 0x101f9410, 2, "136f2e815be772e0"),
    (0x101f9410, 0x101f9420, 2, "66879032f2288a08"),
    (0x101f9420, 0x101f9430, 2, "fb90be870add0f45"),
    (0x101f9430, 0x101f9440, 2, "05c68b1ae10f8fe5"),
    (0x101f9440, 0x101f9460, 8, "5812531ab9bb852d"),
    (0x101f9460, 0x101f9470, 3, "bb7eb64a3856b5c5"),
    (0x101f9470, 0x101fb570, 2112, "a0e5d2d6bf9a5562"),
    (0x101fb570, 0x101fb580, 4, "aa761349b7da80aa"),
    (0x101fb580, 0x101fc670, 1083, "957f392ad0f1f973"),
    (0x101fc670, 0x101fcde0, 476, "7bb2c2cbfc1cea6f"),
    (0x101fcde0, 0x101fce40, 23, "74155418f0290783"),
    (0x101fce40, 0x101fce60, 8, "5d3eb807e2f2a604"),
    (0x101fce60, 0x101fcf20, 48, "df16c178ad3cd859"),
    (0x101fcf20, 0x101fd000, 55, "aa943de2748d69ef"),
    (0x101fd000, 0x101fd100, 62, "42caa1ad2d03324d"),
    (0x101fd100, 0x101fd6b0, 361, "896c651c95392cca"),
    (0x101fd6b0, 0x101fd900, 148, "c4a170e2022c1831"),
    (0x101fd900, 0x101fdb20, 134, "4f16c77724d456c2"),
    (0x101fdb20, 0x101fdb50, 9, "a47a4138a52a3440"),
    (0x101fdb50, 0x101fdb70, 8, "a3f1e8f962011a74"),
    (0x101fdb70, 0x101fdc10, 40, "adde7bbfc61e495d"),
    (0x101fdc10, 0x101fe9f0, 887, "e163be8377e94def"),
    (0x101fe9f0, 0x101fea10, 7, "a03a12d08aa790b9"),
    (0x101fea10, 0x101fea90, 31, "7c9e33d041908bcd"),
    (0x101fea90, 0x101feae0, 19, "82516ffc89520394"),
    (0x101feae0, 0x101feba0, 45, "de9a9d2f7476aafd"),
    (0x101feba0, 0x101fec50, 41, "ff848f4e9eae7778"),
    (0x101fec50, 0x101fed00, 41, "7540d216a78cfd5d"),
    (0x101fed00, 0x101fedd0, 52, "dc126182f41a8459"),
    (0x101fedd0, 0x101fee30, 21, "5b860e248862dcef"),
    (0x101fee30, 0x101fee90, 21, "ea1ccc07a05bdc80"),
    (0x101fee90, 0x101fef60, 52, "c04a0852d36bbf12"),
)
ROUTINES = tuple((a, b) for a, b, _, _ in CONTRACTS)
# bctr, TOC slot, table, number of words, SHA256 of the table bytes.
SWITCH_TABLES = (
    (0x101d5fa4, -0x52d0, 0x108ccb00, 47, "f08f1eea1e5104e76a02406b82bf6cf607a93446d893894f4462c2df6e2d71ce"),
    (0x101e8820, -0x52d4, 0x108ccbbc, 9, "a20e83e431d31f5cea481da504fa70d139767f09cea6b9b9695edf5f7313e668"),
    (0x101f3c74, -0x52d8, 0x108ccbe0, 9, "adae87f95ce5f2d8026636bea773d120b686a8a01cda406482bb28bf470e0ad7"),
    (0x101f5ab4, -0x52e0, 0x108ccc50, 48, "0d907543483c964d8af0ac7952a7978d38a0b040ab624b95c87fc54e4e539a22"),
    (0x101f6428, -0x52e4, 0x108ccc28, 10, "0245272b346c9f9090928f904f748a868a2e47ed5423f833f11518b9ec77e0ef"),
    (0x101f66b0, -0x52e8, 0x108ccc04, 9, "d0ee8ff4d638016e7d4e28cd415013d2cc8695d644838f2389616d66e4bbf1b6"),
    (0x101fddac, -0x52ec, 0x108ccd10, 13, "d503c42223ab6a0a248c046953a4740f2c36ce9651b197617b32ed9250094d36"),
)
TRANSITION_VECTORS = ((0x108b8c34, 0x101d91b0),)
INCLUDES = ROOT / "native/match_eng_routines.inc"
LEAVES_HEADER = ROOT / "native/match_eng_leaves.hpp"
ROUTINE = re.compile(r'MATCH_ENG_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(match_eng_routines_\w+\.inc)"')
LEAF = re.compile(r"inline bool match_eng_leaf_([0-9a-f]+)\(State& s\)")
# The generator hooks every entry from the start: the adapter runs the
# MATCH_ENG_ROUTINE lines, drafts only under a checker, and declines the rest to
# their generated bodies (docs/cpp-rewrites.md, SHORTLIST_MANAGER.CPP).
MATCH_ENG_ENTRIES = tuple(a for a, _ in ROUTINES)


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
            raise ValueError(f"MATCH_ENG.CPP invalid entry/boundary {entry:x}-{end:x}")
    if len({a for a, _, _ in found}) != len(found):
        raise ValueError("MATCH_ENG.CPP routine is listed twice")
    return found


def production_leaves(path=LEAVES_HEADER, includes=INCLUDES):
    """Entries generated call sites run as inline C++ (match_eng_leaves.hpp)."""
    path = Path(path)
    if not path.exists():
        return ()
    done = {entry for entry, _, _ in rewritten(includes)}
    return tuple(sorted(entry for entry in (int(a, 16) for a in LEAF.findall(path.read_text())) if entry in done))


MATCH_ENG_LEAVES = production_leaves()


def require_match_eng_index(db):
    # Every original entry is checked, including entries not implemented yet.
    # A partial promotion cannot hide missing or changed source instructions.
    for entry, end, count, pinned in CONTRACTS:
        records = routine_records(db, entry, end)
        if len(records) != count or digest(records) != pinned:
            raise ValueError(f"MATCH_ENG.CPP original changed at {entry:x}")
    originals = {a: (b, pinned) for a, b, _, pinned in CONTRACTS}
    for entry, end, pinned in rewritten(drafts=True):
        if originals[entry] != (end, pinned):
            raise ValueError(f"MATCH_ENG.CPP promoted digest changed at {entry:x}")


def require_match_eng_owners(owners):
    """Every entry has exactly one normal production owner."""
    for entry in MATCH_ENG_ENTRIES:
        actual = [owner for at, owner in owners if at == entry]
        if actual != ["match_eng_adapter"]:
            raise ValueError(f"MATCH_ENG.CPP production ownership at {entry:x}: {actual}")


def require_match_eng_flows(db):
    """No flow enters a routine except at its entry, or leaves one except by a
    call or return; only the seven switch tables and one vector point inside."""
    low, high = MODULE
    entries = set(MATCH_ENG_ENTRIES)
    starts = sorted(entries)
    from bisect import bisect_right
    owner = lambda address: starts[bisect_right(starts, address) - 1]
    for address, flow, flows in db.execute(
            "SELECT address,json_extract(body,'$.flow'),json_extract(body,'$.flows') FROM instructions"):
        if not flows or flows == "[]":
            continue
        call = "CALL" in (flow or "")
        inside = low <= address < high
        for target in json.loads(flows):
            if low <= target < high:
                if call and target not in entries:
                    raise ValueError(f"Call into MATCH_ENG.CPP interior {target:x} from {address:x}")
                if not call and (not inside or owner(address) != owner(target)):
                    raise ValueError(f"Jump across MATCH_ENG.CPP routines {address:x} -> {target:x}")
            elif inside and not call:
                raise ValueError(f"Jump out of MATCH_ENG.CPP {address:x} -> {target:x}")


def require_match_eng_data(export):
    """Data pointing into the module is exactly the switch tables and the vector."""
    export = Path(export)
    data = (export / "segment-1.bin").read_bytes()
    low, high = MODULE
    allowed = {at for at, _ in TRANSITION_VECTORS}
    for _, slot, table, count, pinned in SWITCH_TABLES:
        raw = data[table - DATA_BASE:table - DATA_BASE + 4 * count]
        if hashlib.sha256(raw).hexdigest() != pinned:
            raise ValueError(f"MATCH_ENG.CPP switch table changed at {table:x}")
        if struct.unpack(">I", data[TOC + slot - DATA_BASE:TOC + slot - DATA_BASE + 4])[0] != table:
            raise ValueError(f"MATCH_ENG.CPP switch slot changed at {slot:x}")
        allowed |= set(range(table, table + 4 * count, 4))
    for offset in range(0, len(data) - 3, 4):
        value = struct.unpack(">I", data[offset:offset + 4])[0]
        if low <= value < high and DATA_BASE + offset not in allowed:
            raise ValueError(f"Unlisted data pointer {DATA_BASE + offset:x} -> {value:x}")
    for at, target in TRANSITION_VECTORS:
        if struct.unpack(">II", data[at - DATA_BASE:at - DATA_BASE + 8]) != (target, TOC):
            raise ValueError(f"MATCH_ENG.CPP transition vector changed at {at:x}")


def match_eng_hook(address):
    return f"if (try_modern_match_eng(s, 0x{address:x})) return true;"


def require_match_eng_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in MATCH_ENG_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = match_eng_hook(entry), f"s.step(0x{entry:x});"
        fragment = code.split(f"i_{entry:x}:", 1)[-1].split(f"i_{entry + 4:x}:", 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced MATCH_ENG hook at {entry:x}")


def main():
    """Print each routine's digest, for new lines of the routine lists."""
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
        raise FileNotFoundError(f"Private MATCH_ENG export unavailable: {path}")
    with sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True) as db:
        require_match_eng_index(db)
    wanted = {int(a, 16) for a in args.entries}
    done = {a for a, _, _ in rewritten()}
    drafted = {a for a, _, _ in rewritten(drafts=True)} - done
    for entry, end, count, pinned in CONTRACTS:
        if not wanted or entry in wanted:
            stage = " (C++)" if entry in done else " (draft)" if entry in drafted else ""
            print(f"0x{entry:x} 0x{end:x} {pinned} {count:5}{stage}")


if __name__ == "__main__":
    main()

"""The font engine: the T2K scaler (Bitstream T2K 2.x with its autogridder) the game links for its text.

263 routines at 0x10751350..0x10768080, 22,909 instructions, in fourteen compilation units. The engine's source files are named
by the assertion strings of its literal pool (0x108a0164..0x108a0a84, in link order): AUTOGRID.C, GHINTS.C, GLYPH.C, HSTRAT97.C,
T2K.C, T2KEXTRA.C, T2KSC.C, T2KSTRM.C, T2KTT.C, TRUETYPE.C and TSIMEM.C, after FONT.CPP (the game's wrapper, not part of this
audit) and before qsort4x.c. Each unit's pool slot (a TOC word) points at the unit's first assertion string, the name follows it, and the
unit's other assertions come next; the pool-using routines of one unit are contiguous and the units follow one another in the order of
the pool. The pool names eleven units; three more have no assertion and no string of their own, so no name is evidenced for them:
"FNT" (0x107559a0..0x1075ac40, between AUTOGRID.C and GHINTS.C), "STYLE" (0x1075e0c0..0x1075e960, between HSTRAT97.C and T2K.C) and
"UTIL" (0x10767ce0..0x10768080, after TSIMEM.C). The three names are guesses from T2K's source layout (fnt.c is its TrueType
bytecode interpreter, util.c its fixed-point helpers; STYLE holds the two functions FONT.CPP passes as `styling->StyleFunc`), not
evidence; the bounds are: the first pool user to the next unit's first pool user, with the pool-less routines between two units
given to the preceding unit (the audit pins the result, tools/test_font_engine_audit.py checks the pool slots and names).
Routines without a pool user (AUTOGRID.C's three tail routines 0x107555d0, 0x10755840, 0x10755930 test the hint handle's two
stamps like the string "ag_IsHinthandle(hintHandle)" and call AUTOGRID.C's 0x10751350; T2KTT.C's and TRUETYPE.C's small routines)
are attributed by sequence, call graph and shared structures, not by a string.

The span is closed: 22,909 export instructions lie in it and every one belongs to one of the 263 routines (78 further words inside
routines are the compiler's alignment no-ops after an unconditional branch: UNREACHABLE). No routine tail-calls another, no branch
leaves its routine, the only `bctr` are three jump tables inside the FNT unit (0x107570b0, 0x10759294, 0x10759474), and the 66 calls
through the indirect-call glue 0x10781320 are callbacks (below). Outside the span only FONT.CPP calls in (API, eleven entries,
below) and no other code names an engine routine; the engine calls out to six game routines: the allocator (0x10775130 malloc,
0x10775170 free, 0x107751b0 realloc), the assertion reporter 0x107755c0, setjmp 0x10773eb0 and longjmp 0x10773f30 (the engine's
error unwinding: every API entry runs setjmp on the memory object, error sites longjmp with a T2K error code, below), the
byte move 0x10739ef0 (NETWORK.CPP) and the indirect glue.

Function pointers. 127 entries (4,760 instructions) have no caller: their descriptors (code word, TOC word) are held in TOC slots
loaded only inside the engine (POINTER_ONLY): the TrueType interpreter's opcode handlers (114 of the FNT unit, installed into its
table by 0x1075a500), eight AUTOGRID.C hint-strategy callbacks (0x10753e70.., stored by HSTRAT97.C, called through the glue by
0x107536b0 and the two small callers 0x10754550, 0x107545b0), three TRUETYPE.C callbacks and the two STYLE functions FONT.CPP
passes to TRUETYPE.C's 0x10766ab0. They are reachable statically from the API (through those loads); whether the game runs them is
a question of the data (the .t2k fonts carry no fpgm/prep/cvt, so the TrueType bytecode is exercised only by symbol.ttf), which the
checker's end-to-end run answers by counting the dispatches (.local/rewrites/font_engine/brief.md).

Production: font_engine_routines*.inc lists each routed entry (FONT_ENGINE_ROUTINE) or draft (FONT_ENGINE_DRAFT) with the digest of
its original body; the generator hooks exactly the listed entries (FONT_ENGINE_HOOKED). ROUTED in tools/test_font_engine_audit.py pins
the set; COMPLETE makes the generator refuse anything less than all 263 routed.
"""
import re
from pathlib import Path

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10751350, 0x10768074)
# Per C file (names without extension: the inventory tool appends .CPP to a key); the units tile 0x10751350..0x10768080.
MODULES = {"AUTOGRID": (0x10751350, 0x107559a0), "FNT": (0x107559a0, 0x1075ac40), "GHINTS": (0x1075ac40, 0x1075b970),
           "GLYPH": (0x1075b970, 0x1075c980), "HSTRAT97": (0x1075c980, 0x1075e0c0), "STYLE": (0x1075e0c0, 0x1075e960),
           "T2K": (0x1075e960, 0x10761070), "T2KEXTRA": (0x10761070, 0x10761270), "T2KSC": (0x10761270, 0x10762f40),
           "T2KSTRM": (0x10762f40, 0x107630e0), "T2KTT": (0x107630e0, 0x10763820), "TRUETYPE": (0x10763820, 0x10767460),
           "TSIMEM": (0x10767460, 0x10767ce0), "UTIL": (0x10767ce0, 0x10768080)}
# The units with a pool: (source name, address of the name string, TOC slot of the pool, its word: the unit's first assertion string).
POOLS = {"AUTOGRID": ("AUTOGRID.C", 0x108a028f, -0x3e08, 0x108a0284), "GHINTS": ("GHINTS.C", 0x108a0475, -0x3c48, 0x108a0468),
         "GLYPH": ("GLYPH.C", 0x108a0512, -0x3c44, 0x108a04fc), "HSTRAT97": ("HSTRAT97.C", 0x108a057a, -0x3c40, 0x108a0564),
         "T2K": ("T2K.C", 0x108a0670, -0x3c3c, 0x108a0660), "T2KEXTRA": ("T2KEXTRA.C", 0x108a0764, -0x3c38, 0x108a075c),
         "T2KSC": ("T2KSC.C", 0x108a0806, -0x3c34, 0x108a07f0), "T2KSTRM": ("T2KSTRM.C", 0x108a0835, -0x3c30, 0x108a0824),
         "T2KTT": ("T2KTT.C", 0x108a0869, -0x3c20, 0x108a085c), "TRUETYPE": ("TRUETYPE.C", 0x108a088a, -0x3c1c, 0x108a0874),
         "TSIMEM": ("TSIMEM.C", 0x108a09e4, -0x3c18, 0x108a09d4)}
ROUTINES = (
    # AUTOGRID: 46 routines
    (0x10751350, 0x10751b0c), (0x10751b10, 0x10751bd0), (0x10751bd0, 0x10751eb0), (0x10751eb0, 0x10751fc8),
    (0x10751fd0, 0x107522b0), (0x107522b0, 0x107524a4), (0x107524b0, 0x10752984), (0x10752990, 0x10753104),
    (0x10753110, 0x10753560), (0x10753560, 0x107536b0), (0x107536b0, 0x107537bc), (0x107537c0, 0x1075397c),
    (0x10753980, 0x10753a80), (0x10753a80, 0x10753aa4), (0x10753ab0, 0x10753bec), (0x10753bf0, 0x10753c2c),
    (0x10753c30, 0x10753c6c), (0x10753c70, 0x10753ccc), (0x10753cd0, 0x10753d20), (0x10753d20, 0x10753d44),
    (0x10753d50, 0x10753d54), (0x10753d60, 0x10753d64), (0x10753d70, 0x10753e70), (0x10753e70, 0x10753f70),
    (0x10753f70, 0x10753f9c), (0x10753fa0, 0x10753fcc), (0x10753fd0, 0x1075404c), (0x10754050, 0x107540cc),
    (0x107540d0, 0x1075412c), (0x10754130, 0x1075418c), (0x10754190, 0x1075436c), (0x10754370, 0x1075454c),
    (0x10754550, 0x107545a4), (0x107545b0, 0x10754604), (0x10754610, 0x10754804), (0x10754810, 0x107548cc),
    (0x107548d0, 0x1075498c), (0x10754990, 0x10754cd0), (0x10754cd0, 0x10754f68), (0x10754f70, 0x10754fec),
    (0x10754ff0, 0x107551b4), (0x107551c0, 0x107552ac), (0x107552b0, 0x107555cc), (0x107555d0, 0x10755838),
    (0x10755840, 0x10755924), (0x10755930, 0x107559a0),
    # FNT: 125 routines
    (0x107559a0, 0x107559c8), (0x107559d0, 0x10755a14), (0x10755a20, 0x10755a5c), (0x10755a60, 0x10755aa4),
    (0x10755ab0, 0x10755af4), (0x10755b00, 0x10755b4c), (0x10755b50, 0x10755b58), (0x10755b60, 0x10755be0),
    (0x10755be0, 0x10755cb8), (0x10755cc0, 0x10755dfc), (0x10755e00, 0x10755e28), (0x10755e30, 0x10755e58),
    (0x10755e60, 0x10755eb4), (0x10755ec0, 0x10755f14), (0x10755f20, 0x10755f28), (0x10755f30, 0x10755f38),
    (0x10755f40, 0x10755f54), (0x10755f60, 0x1075601c), (0x10756020, 0x1075602c), (0x10756030, 0x107560d8),
    (0x107560e0, 0x107560fc), (0x10756100, 0x10756274), (0x10756280, 0x10756310), (0x10756310, 0x10756520),
    (0x10756520, 0x10756554), (0x10756560, 0x10756594), (0x107565a0, 0x1075662c), (0x10756630, 0x107566a4),
    (0x107566b0, 0x1075681c), (0x10756820, 0x107569fc), (0x10756a00, 0x10756b5c), (0x10756b60, 0x10756bfc),
    (0x10756c00, 0x10756c8c), (0x10756c90, 0x10756cbc), (0x10756cc0, 0x10756cec), (0x10756cf0, 0x10756d14),
    (0x10756d20, 0x10756fb4), (0x10756fc0, 0x10756fdc), (0x10756fe0, 0x10756ffc), (0x10757000, 0x1075701c),
    (0x10757020, 0x10757074), (0x10757080, 0x107570d8), (0x107570e0, 0x10757144), (0x10757150, 0x10757198),
    (0x107571a0, 0x107571e8), (0x107571f0, 0x10757288), (0x10757290, 0x10757420), (0x10757420, 0x10757468),
    (0x10757470, 0x10757488), (0x10757490, 0x107574a0), (0x107574a0, 0x107574b8), (0x107574c0, 0x107574e8),
    (0x107574f0, 0x1075750c), (0x10757510, 0x10757558), (0x10757560, 0x10757580), (0x10757580, 0x1075764c),
    (0x10757650, 0x1075778c), (0x10757790, 0x10757ab0), (0x10757ab0, 0x10757bcc), (0x10757bd0, 0x10757ca0),
    (0x10757ca0, 0x10757dc0), (0x10757dc0, 0x10757fac), (0x10757fb0, 0x107580cc), (0x107580d0, 0x10758268),
    (0x10758270, 0x107583b0), (0x107583b0, 0x10758474), (0x10758480, 0x1075854c), (0x10758550, 0x1075856c),
    (0x10758570, 0x107585b4), (0x107585c0, 0x1075860c), (0x10758610, 0x10758668), (0x10758670, 0x10758698),
    (0x107586a0, 0x10758704), (0x10758710, 0x10758790), (0x10758790, 0x10758ad8), (0x10758ae0, 0x10758b58),
    (0x10758b60, 0x10758b90), (0x10758b90, 0x10758bc4), (0x10758bd0, 0x10758c00), (0x10758c00, 0x10758c34),
    (0x10758c40, 0x10758c74), (0x10758c80, 0x10758cb4), (0x10758cc0, 0x10758d6c), (0x10758d70, 0x10758ddc),
    (0x10758de0, 0x10758e34), (0x10758e40, 0x10758edc), (0x10758ee0, 0x10758f94), (0x10758fa0, 0x107590a4),
    (0x107590b0, 0x10759184), (0x10759190, 0x107591ac), (0x107591b0, 0x10759218), (0x10759220, 0x10759230),
    (0x10759230, 0x10759240), (0x10759240, 0x10759250), (0x10759250, 0x10759430), (0x10759430, 0x10759514),
    (0x10759520, 0x107595a0), (0x107595a0, 0x10759654), (0x10759660, 0x107596e4), (0x107596f0, 0x107596f4),
    (0x10759700, 0x10759724), (0x10759730, 0x10759764), (0x10759770, 0x107597a4), (0x107597b0, 0x1075980c),
    (0x10759810, 0x10759834), (0x10759840, 0x107598d8), (0x107598e0, 0x10759ad4), (0x10759ae0, 0x10759dbc),
    (0x10759dc0, 0x10759e28), (0x10759e30, 0x10759ee8), (0x10759ef0, 0x10759fa0), (0x10759fa0, 0x1075a040),
    (0x1075a040, 0x1075a150), (0x1075a150, 0x1075a19c), (0x1075a1a0, 0x1075a1bc), (0x1075a1c0, 0x1075a1dc),
    (0x1075a1e0, 0x1075a3a0), (0x1075a3a0, 0x1075a3d0), (0x1075a3d0, 0x1075a408), (0x1075a410, 0x1075a448),
    (0x1075a450, 0x1075a480), (0x1075a480, 0x1075a4b8), (0x1075a4c0, 0x1075a4f8), (0x1075a500, 0x1075a964),
    (0x1075a970, 0x1075ac38),
    # GHINTS: 5 routines
    (0x1075ac40, 0x1075acc8), (0x1075acd0, 0x1075ad58), (0x1075ad60, 0x1075ae30), (0x1075ae30, 0x1075af60),
    (0x1075af60, 0x1075b970),
    # GLYPH: 6 routines
    (0x1075b970, 0x1075bb94), (0x1075bba0, 0x1075bc90), (0x1075bc90, 0x1075c0e8), (0x1075c0f0, 0x1075c824),
    (0x1075c830, 0x1075c8c0), (0x1075c8c0, 0x1075c97c),
    # HSTRAT97: 3 routines
    (0x1075c980, 0x1075ca50), (0x1075ca60, 0x1075cbbc), (0x1075cbc0, 0x1075e0bc),
    # STYLE: 3 routines
    (0x1075e0c0, 0x1075e1a0), (0x1075e1a0, 0x1075e31c), (0x1075e320, 0x1075e958),
    # T2K: 9 routines
    (0x1075e960, 0x1075eb0c), (0x1075eb10, 0x1075ecdc), (0x1075ece0, 0x1075eef0), (0x1075eef0, 0x1075f868),
    (0x1075f870, 0x1075f978), (0x1075f980, 0x1075fa08), (0x1075fa10, 0x10760e30), (0x10760e30, 0x10760f44),
    (0x10760f50, 0x10761064),
    # T2KEXTRA: 1 routines
    (0x10761070, 0x10761268),
    # T2KSC: 9 routines
    (0x10761270, 0x107614c0), (0x107614c0, 0x10761730), (0x10761730, 0x107617ac), (0x107617b0, 0x10761d44),
    (0x10761d50, 0x10761eb8), (0x10761ec0, 0x107620d0), (0x107620d0, 0x107623b8), (0x107623c0, 0x10762e2c),
    (0x10762e30, 0x10762f40),
    # T2KSTRM: 3 routines
    (0x10762f40, 0x10762fd0), (0x10762fd0, 0x1076304c), (0x10763050, 0x107630d8),
    # T2KTT: 8 routines
    (0x107630e0, 0x107630f0), (0x107630f0, 0x107631b8), (0x107631c0, 0x1076327c), (0x10763280, 0x1076328c),
    (0x10763290, 0x107632a0), (0x107632a0, 0x10763350), (0x10763350, 0x1076338c), (0x10763390, 0x1076381c),
    # TRUETYPE: 31 routines
    (0x10763820, 0x10763840), (0x10763840, 0x10763888), (0x10763890, 0x107638bc), (0x107638c0, 0x10763ea4),
    (0x10763eb0, 0x107642d4), (0x107642e0, 0x107643c8), (0x107643d0, 0x107644c0), (0x107644c0, 0x107645a0),
    (0x107645a0, 0x1076460c), (0x10764610, 0x107649a0), (0x107649a0, 0x10764b08), (0x10764b10, 0x10764be8),
    (0x10764bf0, 0x10764d10), (0x10764d10, 0x10764e48), (0x10764e50, 0x10764f58), (0x10764f60, 0x1076505c),
    (0x10765060, 0x10765728), (0x10765730, 0x10765780), (0x10765780, 0x10765810), (0x10765810, 0x1076642c),
    (0x10766430, 0x1076649c), (0x107664a0, 0x10766768), (0x10766770, 0x107668c4), (0x107668d0, 0x10766aa8),
    (0x10766ab0, 0x10766cbc), (0x10766cc0, 0x10766d24), (0x10766d30, 0x10766db4), (0x10766dc0, 0x107670d4),
    (0x107670e0, 0x10767188), (0x10767190, 0x1076742c), (0x10767430, 0x10767460),
    # TSIMEM: 8 routines
    (0x10767460, 0x107675c0), (0x107675c0, 0x10767684), (0x10767690, 0x1076771c), (0x10767720, 0x10767814),
    (0x10767820, 0x10767894), (0x107678a0, 0x107679f4), (0x10767a00, 0x10767bb0), (0x10767bb0, 0x10767ce0),
    # UTIL: 6 routines
    (0x10767ce0, 0x10767d34), (0x10767d40, 0x10767dbc), (0x10767dc0, 0x10767e40), (0x10767e40, 0x10767eb0),
    (0x10767eb0, 0x10767f20), (0x10767f20, 0x10768074),
)
# Words inside routines that the export leaves out because nothing reaches them: the compiler's alignment no-op after an
# unconditional branch.
UNREACHABLE = frozenset({
    0x10753224, 0x107541e4, 0x10754224, 0x107543c4, 0x10754404, 0x10754c44, 0x1075615c, 0x10757924, 0x10757a30,
    0x10757a34, 0x10757d34, 0x10758024, 0x10758584, 0x107585e4, 0x107588c0, 0x10759e94, 0x1075a084, 0x1075a310,
    0x1075a8c4, 0x1075a8e4, 0x1075a904, 0x1075a924, 0x1075a944, 0x1075bff4, 0x1075cde0, 0x1075d2cc, 0x1075d578,
    0x1075d6e8, 0x1075dd3c, 0x107600a4, 0x1076094c, 0x107614fc, 0x10761544, 0x107615c4, 0x1076165c, 0x107616e4,
    0x10761924, 0x107619dc, 0x10761a7c, 0x10761bcc, 0x10761c84, 0x10761d24, 0x1076208c, 0x10762678, 0x10762e88,
    0x10763c08, 0x107643f4, 0x107647dc, 0x107647fc, 0x10764ab0, 0x10765008, 0x107654c8, 0x1076573c, 0x1076586c,
    0x10765c08, 0x10765c30, 0x10765cc8, 0x10765d38, 0x10765d60, 0x10765dd0, 0x10765df8, 0x10765e60, 0x10765e88,
    0x10765f28, 0x1076605c, 0x107660f8, 0x1076622c, 0x10766274, 0x1076629c, 0x10766304, 0x10766390, 0x107663b8,
    0x107664dc, 0x10766504, 0x107667cc, 0x10766934, 0x10767510, 0x1076784c,
})
# The API: engine entries FONT.CPP calls (entry: FONT.CPP routines that call it).
API = {
    0x1075e960: (0x10750320,),
    0x1075eef0: (0x10750320,),
    0x1075f980: (0x10750580,),
    0x10760e30: (0x10750580,),
    0x10760f50: (0x10750200, 0x10750320),
    0x107631c0: (0x1074f470,),
    0x107632a0: (0x10750200,),
    0x10766ab0: (0x10750320,),
    0x10767190: (0x10750200, 0x10750320),
    0x10767460: (0x1074f470,),
    0x10767720: (0x10750200,),
}
# Entries no `bl` names: their descriptors are loaded from TOC slots inside the engine (or FONT.CPP's two style slots).
POINTER_ONLY = frozenset({
     0x10753e70, 0x10753fa0, 0x10753fd0, 0x10754050, 0x107540d0, 0x10754130, 0x10754550, 0x107545b0, 0x107559a0,
     0x107559d0, 0x10755a20, 0x10755a60, 0x10755b00, 0x10755b50, 0x10755b60, 0x10755be0, 0x10755e00, 0x10755e30,
     0x10755e60, 0x10755ec0, 0x10755f20, 0x10755f30, 0x10755f40, 0x10755f60, 0x10756020, 0x10756030, 0x107560e0,
     0x10756100, 0x10756280, 0x10756520, 0x10756560, 0x107565a0, 0x10756630, 0x107566b0, 0x10756820, 0x10756a00,
     0x10756b60, 0x10756c00, 0x10756c90, 0x10756cc0, 0x10756cf0, 0x10756d20, 0x10756fc0, 0x10756fe0, 0x10757000,
     0x10757020, 0x10757080, 0x107570e0, 0x10757150, 0x107571a0, 0x107571f0, 0x10757420, 0x10757470, 0x10757490,
     0x107574a0, 0x107574c0, 0x107574f0, 0x10757510, 0x10757560, 0x10757580, 0x10757650, 0x10757790, 0x10757bd0,
     0x10757ca0, 0x10757dc0, 0x10757fb0, 0x107580d0, 0x10758270, 0x107583b0, 0x10758480, 0x10758550, 0x10758570,
     0x107585c0, 0x10758610, 0x10758670, 0x107586a0, 0x10758710, 0x10758790, 0x10758b60, 0x10758b90, 0x10758bd0,
     0x10758c00, 0x10758c40, 0x10758c80, 0x10758cc0, 0x10758d70, 0x10758de0, 0x10758e40, 0x10758ee0, 0x10758fa0,
     0x107590b0, 0x10759190, 0x107591b0, 0x10759220, 0x10759230, 0x10759240, 0x10759250, 0x10759430, 0x107595a0,
     0x10759660, 0x107596f0, 0x10759700, 0x10759730, 0x10759770, 0x107597b0, 0x10759810, 0x107598e0, 0x10759ae0,
     0x10759dc0, 0x10759e30, 0x10759ef0, 0x10759fa0, 0x1075a040, 0x1075a150, 0x1075a1a0, 0x1075a1c0, 0x1075a3a0,
     0x1075a3d0, 0x1075a410, 0x1075a450, 0x1075a480, 0x1075a4c0, 0x1075e1a0, 0x1075e320, 0x10763820, 0x10763840,
     0x10763890,
})
ROUTINE = re.compile(r'FONT_ENGINE_(ROUTINE|DRAFT)\(\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*([\w|]+(?: \| [\w]+)*),\s*"([0-9a-f]+)"')
INCLUDE = re.compile(r'#include "(font_engine_routines_\w+\.inc)"')
INCLUDES = ROOT / "native" / "font_engine_routines.inc"
# The adapter (compile_recomp.OWNER_REPLACEMENTS names it for each hooked entry).
OWNER = "font_engine_adapter"


def routine_lines(path=INCLUDES):
    """The text of font_engine_routines.inc with the block files it includes (empty while the file does not exist)."""
    if not path.exists():
        return ""
    text = path.read_text()
    return text + "".join((path.parent / name).read_text() for name in INCLUDE.findall(text)
                          if (path.parent / name).exists())


def unit_of(entry):
    """The compilation unit's name of an entry."""
    for name, (start, end) in MODULES.items():
        if start <= entry < end:
            return name
    raise ValueError(f"{entry:x} is not in the font engine")


def rewritten(path=INCLUDES, drafts=False):
    """(entry, end, digest) of each routine whose C++ is verified and routed (FONT_ENGINE_ROUTINE), with drafts also the
    FONT_ENGINE_DRAFT ones, which only check tools run."""
    found = sorted((int(entry, 16), int(end, 16), digest) for stage, entry, end, _, digest in
                   ROUTINE.findall(routine_lines(path)) if drafts or stage == "ROUTINE")
    bounds = dict(ROUTINES)
    for entry, end, _ in found:
        if bounds.get(entry) != end:
            raise ValueError(f"font_engine_routines.inc: {entry:x}-{end:x} is not a routine of the font engine")
    if len({entry for entry, _, _ in found}) != len(found):
        raise ValueError("A font engine routine is listed twice")
    return found


FONT_ENGINE_ENTRIES = tuple(entry for entry, _ in ROUTINES)
FONT_ENGINE_HOOKED = tuple(entry for entry, _, _ in rewritten(drafts=True))

# The generator requires every entry routed (FONT_ENGINE_ROUTINE): all 263 are, and a draft or a missing line stops the generation.
COMPLETE = True


def require_font_engine_index(db, path=INCLUDES, complete=None):
    """Every listed routine's original instructions are still the ones its C++ follows."""
    complete = COMPLETE if complete is None else complete
    registered = rewritten(path, drafts=True)
    if complete and tuple((entry, end) for entry, end, _ in rewritten(path)) != ROUTINES:
        raise ValueError("The font engine requires all 263 complete production entries")
    for entry, end, expected in registered:
        body = routine_records(db, entry, end)
        if [r['address'] for r in body] != [a for a in range(entry, end, 4) if a not in UNREACHABLE]:
            raise ValueError(f"Missing font engine instructions at {entry:x}")
        actual = digest(body)
        if actual != expected:
            raise ValueError(f"Font engine routine {entry:x} changed in the export (digest {actual}, "
                             f"font_engine_routines.inc pins {expected})")


def font_engine_hook(address):
    return f"if (try_modern_font_engine(s, 0x{address:x})) return true;"


def require_font_engine_hooks(page, instructions, code):
    """Each hooked entry on the page carries the hook once, before its first step; an unhooked entry carries none."""
    addresses = {r['address'] for r in instructions}
    for entry, _ in ROUTINES:
        if entry >> 12 != page or entry not in addresses:
            continue
        hook, step = font_engine_hook(entry), f's.step(0x{entry:x});'
        fragment = code.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if entry not in FONT_ENGINE_HOOKED:
            if hook in code:
                raise ValueError(f'Unexpected font engine hook at {entry:x}')
            continue
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced font engine hook at {entry:x}')


def main():
    """Print each routine's digest, for new lines of font_engine_routines*.inc."""
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
        print(f"0x{entry:x} 0x{end:x} {digest(records)} {len(records):5}{stage} {unit_of(entry)}")


if __name__ == "__main__":
    main()

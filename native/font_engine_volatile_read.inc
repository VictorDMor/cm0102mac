// {entry, mask} of entries whose callers read only some of r4-r10: none yet.
// t2ktt_head: the callers (tools/register_contract.py --after ENTRY: direct `bl`s only, no descriptor names them) read r3 and r9/r10, which these routines do not write (r9/r10 reach the allocator, the reporter and the indirect glue through them untouched: bits 5 and 6 = 0x60); 0x10763eb0 and 0x107645a0 write r9 (and r10) and leave them as the original does.
{0x107630f0, 0x60}, {0x107631c0, 0x60}, {0x10763280, 0x60}, {0x10763290, 0x60}, {0x107632a0, 0x60}, {0x10763350, 0x60},
{0x10763eb0, 0x60}, {0x107643d0, 0x60}, {0x107644c0, 0x60}, {0x107645a0, 0x60},
// truetype_tables: 0x10765730 and 0x10765780 are read for r3 and r9/r10 (pass through: neither routine writes them) by their direct callers (--after).
{0x10765730, 0x60}, {0x10765780, 0x60},
// truetype_glyph: tools/register_contract.py --after over each entry's direct callers (no descriptor names them): r3 (the result) and r9/r10
// (which the entries pass through from their callees or the caller; 0x10766ab0 clears them) are read, nothing else; 0x10766d30's callers read r3 only.
{0x10765810, 0x60}, {0x10766430, 0x60}, {0x107664a0, 0x60}, {0x10766770, 0x60}, {0x107668d0, 0x60}, {0x10766ab0, 0x60}, {0x10766cc0, 0x60},
{0x10766d30, 0x00}, {0x10767190, 0x60}, {0x10767430, 0x60},
// t2ktt_head and truetype_tables, second look (the integration finding): a pass-through chain reaches callers' callers (0x10765810 -> 0x10766ab0's callers read r9/r10),
// which --after does not follow. These entries are compared on r9 and r10 as well; the routines whose original writes them (0x10763390 and 0x107638c0's unrolled loops,
// 0x10763eb0, 0x107645a0, 0x10764610's pass counter, 0x10765060's flag counter, 0x10765780) leave them as it does.
{0x107630e0, 0x60}, {0x10763390, 0x60}, {0x107638c0, 0x60}, {0x107642e0, 0x60}, {0x10764610, 0x60}, {0x107649a0, 0x60}, {0x10764b10, 0x60}, {0x10764bf0, 0x60}, {0x10764d10, 0x60}, {0x10764e50, 0x60}, {0x10764f60, 0x60}, {0x10765060, 0x60},
// t2ksc: tools/register_contract.py --after over the direct callers (0x107620d0 and 0x107623c0 from 0x1075fa10, the others inside the block) lists r3, r9 and r10 as the only registers read
// after a call (r4-r8 are written before any read); r9/r10 pass through the leaves, the allocator and the reporter, so every entry is compared on them (mask 0x60) and leaves them as the
// original does (the loops of 0x107614c0, 0x10761730, 0x107620d0 and 0x107623c0, the table copy of 0x10761270 and the blends of 0x107623c0 write them). 0x10762e30 is compared in full (no entry here).
{0x10761270, 0x60}, {0x107614c0, 0x60}, {0x10761730, 0x60}, {0x107617b0, 0x60}, {0x10761d50, 0x60}, {0x10761ec0, 0x60}, {0x107620d0, 0x60}, {0x107623c0, 0x60},
// t2k_core: tools/register_contract.py --after over the direct callers (FONT.CPP's for the five API entries, the block's own for the rest) lists r9 and r10 (and the constructor's r3) as the only
// registers read after a call; the callees pass r9/r10 through, and where the original writes them they are replicated (the cache fill of 0x1075eef0, the unrolled loop of 0x1075ece0, the caret routine's
// pass counter, the arguments of the hinter and the scan converter). Every entry is compared on r9 and r10.
{0x1075e960, 0x60}, {0x1075eb10, 0x60}, {0x1075ece0, 0x60}, {0x1075eef0, 0x60}, {0x1075f870, 0x60}, {0x1075f980, 0x60}, {0x1075fa10, 0x60}, {0x10760e30, 0x60}, {0x10760f50, 0x60},
// glyph and truetype_glyph, corrected after the t2k_core review: 0x1075c0f0 (append_glyph) and 0x10766dc0 (caret_slopes) were listed volatile-free (their direct callers read nothing after them) but
// they write r9 (and r10), which reach the API entries' returns through 0x1075fa10 / 0x1075eef0 and so the callers' reads: compared on r9 and r10, and the originals' loops reproduced
// (the first component's unrolled offset loop, the copy loops of a merge; the caret's doubling pass counter when the font has an hhea).
{0x1075c0f0, 0x60}, {0x10766dc0, 0x60},
// autogrid_a: tools/register_contract.py --after over the direct callers lists r3 (0x10751bd0) and r9/r10 (0x10751bd0's second caller, 0x10751eb0's from 0x10751fd0, where they are live);
// the others' callers read nothing after the call, but the chain (0x10755930, 0x10754ff0, 0x10753560 return to 0x10754f70 / 0x1075eb10, whose callers read r9, r10) passes r9 and r10 on,
// so every entry is compared on them and leaves them as the original does (r4-r8: read by no caller on the chain).
{0x10751350, 0x60}, {0x10751b10, 0x60}, {0x10751bd0, 0x60}, {0x10751eb0, 0x60}, {0x10751fd0, 0x60}, {0x107522b0, 0x60}, {0x107524b0, 0x60}, {0x10752990, 0x60}, {0x10753110, 0x60},
// ghints: 0x1075ae30 and 0x1075af60 call the glyph readers (truetype_glyph entries, compared on r9/r10 only: they leave r4-r8 their own way) and the generated analysis, and the callers
// (the median's caller 0x1075af60, prepare_hinting's call of the analysis) set r3-r8 before they read them; r9 and r10 pass through to the API entries' returns, so both are compared on
// them. The leaves 0x1075ac40/0x1075acd0 and the T2KG reader 0x1075ad60 leave r3-r10 as the original and are compared in full.
{0x1075ae30, 0x60}, {0x1075af60, 0x60},
// autogrid_b: the entries that call other blocks' routines (the first half's analyses, the strategy, the stem widths) leave r4-r8 as those callees leave them, which the callers on the
// chain do not read (tools/register_contract.py --after: 0x10755930's and 0x107552b0's callers read r3 only; 0x10754f70's, 0x10754ff0's chain, 0x10753560's and 0x10754990's read r3, r9 and r10):
// compared on r9 and r10 (they pass through every callee). The callbacks and the passes that call only the game's routines or this block's leaves are compared in full.
{0x10753560, 0x60}, {0x10754ff0, 0x60}, {0x10754f70, 0x60}, {0x107552b0, 0x60}, {0x10755930, 0x60},

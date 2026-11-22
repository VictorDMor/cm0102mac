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

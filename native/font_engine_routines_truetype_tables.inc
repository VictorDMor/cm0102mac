// The font engine, block `truetype_tables` (font_engine_truetype_tables.cpp): the readers of the sfnt's tables, the glyph reader and the
// directory lookups. The callers' reads of r4-r10 are in font_engine_volatile_{free,read}.inc.
FONT_ENGINE_ROUTINE(0x10764610, 0x107649a0, R3, "eedbfc0f69ae536a", [](Cpu& c) { c.set(3, font_engine::new_cmap(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
FONT_ENGINE_ROUTINE(0x107649a0, 0x10764b08, R3, "fa27078f24dc826e", [](Cpu& c) { c.set(3, font_engine::new_kern_subtable(c, c.r(3), c.r(4), c.r(5))); })
FONT_ENGINE_ROUTINE(0x10764b10, 0x10764be8, R3, "40664dad64441e24", [](Cpu& c) { c.set(3, font_engine::new_kern(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10764bf0, 0x10764d10, R3, "6f67f3ff4ea03274", [](Cpu& c) { c.set(3, font_engine::new_hhea(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10764d10, 0x10764e48, R3, "686df3fd4e9240c1", [](Cpu& c) { c.set(3, font_engine::new_head(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10764e50, 0x10764f58, R3, "43cb52b2be22e606", [](Cpu& c) { c.set(3, font_engine::new_maxp(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10764f60, 0x1076505c, R3, "17876294f3afea4e", [](Cpu& c) { c.set(3, font_engine::new_loca(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
FONT_ENGINE_ROUTINE(0x10765060, 0x10765728, R3, "2b38c5ff06b550f8", [](Cpu& c) { c.set(3, font_engine::new_glyph(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9))); })
FONT_ENGINE_ROUTINE(0x10765730, 0x10765780, R3, "59752b7b156cda73", [](Cpu& c) { c.set(3, font_engine::find_table_entry(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10765780, 0x10765810, R3, "d0d2eeb2c85e1978", [](Cpu& c) { c.set(3, font_engine::new_table_stream(c, c.r(3), c.r(4))); })

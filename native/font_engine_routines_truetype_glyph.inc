// The font engine, block `truetype_glyph` (font_engine_truetype_glyph.cpp): the sfnt, its creation and deletion, the cmap lookups, the glyph
// by index. 0x107670e0 is listed in font_engine_volatile_free.inc; 0x10766dc0 is compared on r9/r10 (font_engine_volatile_read.inc).
FONT_ENGINE_ROUTINE(0x10765810, 0x1076642c, NONE, "136cee12a92277a8", [](Cpu& c) { font_engine::read_sfnt(c, c.r(3), c.r(4), c.r(5)); })
FONT_ENGINE_ROUTINE(0x10766430, 0x1076649c, R3, "837354610a39de48", [](Cpu& c) { c.set(3, font_engine::glyph_limit(c, c.r(3))); })
FONT_ENGINE_ROUTINE(0x107664a0, 0x10766768, R3, "ee441d8ec63ecf4b", [](Cpu& c) { c.set(3, font_engine::load_glyph(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
FONT_ENGINE_ROUTINE(0x10766770, 0x107668c4, R3, "e8c3be1f044dc23d", [](Cpu& c) { c.set(3, font_engine::character_glyph(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x107668d0, 0x10766aa8, R3, "6568684c3fb79ee4", [](Cpu& c) { c.set(3, font_engine::is_digit_glyph(c, c.r(3), c.r(4))); })
FONT_ENGINE_ROUTINE(0x10766ab0, 0x10766cbc, R3, "7c2c5b7d332737a8", [](Cpu& c) { c.set(3, font_engine::new_sfnt(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })
FONT_ENGINE_ROUTINE(0x10766cc0, 0x10766d24, R3, "7ee3692f8fa7a3f6", [](Cpu& c) { c.set(3, font_engine::units_per_em(c, c.r(3))); })
FONT_ENGINE_ROUTINE(0x10766d30, 0x10766db4, R3, "00edbae96a5f776c", [](Cpu& c) { c.set(3, font_engine::max_outline_points(c, c.r(3))); })
FONT_ENGINE_ROUTINE(0x10766dc0, 0x107670d4, NONE, "a5b8cc91a4f57d4d", [](Cpu& c) { font_engine::caret_slopes(c, c.r(3), c.r(4), c.r(5)); })
FONT_ENGINE_ROUTINE(0x107670e0, 0x10767188, NONE, "a801e1f520e2e6c8", [](Cpu& c) { font_engine::delete_cmap(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10767190, 0x1076742c, NONE, "9e6501785d781457", [](Cpu& c) { font_engine::delete_sfnt(c, c.r(3), c.r(4)); })
FONT_ENGINE_ROUTINE(0x10767430, 0x10767460, NONE, "be903f73c34d95b7", [](Cpu& c) { font_engine::raise_error(c, c.r(3), c.r(4)); })

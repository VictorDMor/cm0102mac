// The font engine, block `glyph` (font_engine_glyph.cpp): the glyph record's routines. r4-r10 are left as the original does
// (0x1075c0f0 is compared on r9/r10: font_engine_volatile_read.inc).
FONT_ENGINE_ROUTINE(0x1075b970, 0x1075bb94, R3, "f95ed7d53e974a47", [](Cpu& c) { c.set(3, font_engine::read_compact_point(c, c.r(3), c.r(4), c.r(5))); })
FONT_ENGINE_ROUTINE(0x1075bba0, 0x1075bc90, R3, "ed2a8eeac1fa6cff", [](Cpu& c) { c.set(3, font_engine::new_empty_glyph(c, c.r(3), c.r(5), c.r(6), c.r(7))); })
FONT_ENGINE_ROUTINE(0x1075bc90, 0x1075c0e8, R3, "205ed96fc0fb0377", [](Cpu& c) { c.set(3, font_engine::new_compact_glyph(c, c.r(3), c.r(4), c.r(6), c.r(7), c.r(8), c.r(9))); })
FONT_ENGINE_ROUTINE(0x1075c0f0, 0x1075c824, NONE, "6f2f8241b858ae97", [](Cpu& c) { font_engine::append_glyph(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
FONT_ENGINE_ROUTINE(0x1075c830, 0x1075c8c0, NONE, "6b8f531a2b6c7b7c", [](Cpu& c) { font_engine::allocate_glyph_points(c, c.r(3), std::int32_t(c.r(4))); })
FONT_ENGINE_ROUTINE(0x1075c8c0, 0x1075c97c, NONE, "883415ed5fbc3939", [](Cpu& c) { font_engine::delete_glyph(c, c.r(3)); })

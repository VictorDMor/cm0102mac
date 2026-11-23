// The font engine, block `ghints` (font_engine_ghints.cpp): the global hint data. Every entry leaves r3-r10 as the original does.
FONT_ENGINE_ROUTINE(0x1075ac40, 0x1075acc8, R3, "7966f7c66edd3ff7", [](Cpu& c) { c.set(3, std::uint32_t(font_engine::glyph_highest(c, c.r(3)))); })
FONT_ENGINE_ROUTINE(0x1075acd0, 0x1075ad58, R3, "f1e295fd4f032256", [](Cpu& c) { c.set(3, std::uint32_t(font_engine::glyph_lowest(c, c.r(3)))); })
FONT_ENGINE_ROUTINE(0x1075ad60, 0x1075ae30, NONE, "1321dba11a8083b7", [](Cpu& c) { font_engine::read_hint_data(c, c.r(3), c.r(4)); })
FONT_ENGINE_ROUTINE(0x1075ae30, 0x1075af60, R3, "dd1e4ccc047ba397", [](Cpu& c) { c.set(3, std::uint32_t(font_engine::median_height(c, c.r(3), c.r(4), c.r(5)))); })
FONT_ENGINE_ROUTINE(0x1075af60, 0x1075b970, NONE, "1c768a0089d33798", [](Cpu& c) { font_engine::analyse_hint_data(c, c.r(3), c.r(4), c.r(5), c.r(6)); })

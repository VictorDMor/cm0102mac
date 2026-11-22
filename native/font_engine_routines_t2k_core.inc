// The font engine, block `t2k_core` (font_engine_t2k_core.cpp): the scaler object, its transformation, the renderer and the destruction.
// Every entry is compared on r9 and r10 at return (font_engine_volatile_read.inc: the callers read them, see the file), the constructor on r3 too.
FONT_ENGINE_ROUTINE(0x1075e960, 0x1075eb0c, R3, "f4ed1a99d282da48", [](Cpu& c) { c.set(3, font_engine::new_t2k(c, c.r(3), c.r(4), c.r(5))); })
FONT_ENGINE_ROUTINE(0x1075eb10, 0x1075ecdc, NONE, "dedbb811de1f5cca", [](Cpu& c) { font_engine::prepare_hinting(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x1075ece0, 0x1075eef0, NONE, "da9c5f2cd62a634b", [](Cpu& c) { font_engine::scale_coordinates(c, c.r(3), c.r(4), std::int32_t(c.r(5)), c.r(6)); })
FONT_ENGINE_ROUTINE(0x1075eef0, 0x1075f868, NONE, "7c415df271af60f8", [](Cpu& c) {
    font_engine::set_transformation(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
FONT_ENGINE_ROUTINE(0x1075f870, 0x1075f978, NONE, "43e3ec60d48522db", [](Cpu& c) { font_engine::release_glyph(c, c.r(3), std::int32_t(c.r(4))); })
FONT_ENGINE_ROUTINE(0x1075f980, 0x1075fa08, NONE, "addf5322d3f5b784", [](Cpu& c) { font_engine::purge(c, c.r(3), c.r(4), c.r(5)); })
FONT_ENGINE_ROUTINE(0x1075fa10, 0x10760e30, NONE, "646b65a907ca3697", [](Cpu& c) {
    font_engine::render_glyph(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
FONT_ENGINE_ROUTINE(0x10760e30, 0x10760f44, NONE, "88455a302878e289", [](Cpu& c) {
    font_engine::render_character(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
FONT_ENGINE_ROUTINE(0x10760f50, 0x10761064, NONE, "9722c3f1c4918411", [](Cpu& c) { font_engine::delete_t2k(c, c.r(3), c.r(4)); })

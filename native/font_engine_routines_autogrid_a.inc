// The font engine, block `autogrid_a` (font_engine_autogrid_a.cpp): AUTOGRID.C's height and stem analysis. Dormant for the game (hint = 0); the hinted
// end-to-end mode and the cases run it. The callers read r3 and r9/r10 (the volatile mask 0x60 in font_engine_volatile_read.inc); the three routines
// that call out build the original's frame.
FONT_ENGINE_ROUTINE(0x10751350, 0x10751b0c, R3, "7e52bf3e64006ba5", [](Cpu& c) { c.set(3, font_engine::scale_hint_heights(c, c.r(3), c.r(4), c.r(5))); })
FONT_ENGINE_ROUTINE(0x10751b10, 0x10751bd0, NONE, "9e13dde6efea098f", [](Cpu& c) { font_engine::band_coordinates(c, c.r(3), c.r(4)); })
FONT_ENGINE_ROUTINE(0x10751bd0, 0x10751eb0, R3, "5c855c5a227bea67", [](Cpu& c) { c.set(3, std::uint32_t(font_engine::nearest_height(c, c.r(3), c.r(4)))); })
FONT_ENGINE_ROUTINE(0x10751eb0, 0x10751fc8, NONE, "581fc1ea40f45f72", [](Cpu& c) { font_engine::normalize_vector(c, c.r(3), c.r(4)); })
FONT_ENGINE_ROUTINE(0x10751fd0, 0x107522b0, NONE, "c5cda22c6b59706e", [](Cpu& c) { font_engine::point_directions(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x107522b0, 0x107524a4, NONE, "70e03e510805f8b4", [](Cpu& c) { font_engine::mark_extrema(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x107524b0, 0x10752984, NONE, "d498fb21dd0ceb34", [](Cpu& c) { font_engine::mark_inflections(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10752990, 0x10753104, NONE, "e50d97adce586645", [](Cpu& c) { font_engine::classify_points(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10753110, 0x10753560, NONE, "8962071c898a0f1d", [](Cpu& c) { font_engine::link_edge_points(c, c.r(3)); })

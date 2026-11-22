// The font engine, block `util` (font_engine_util.cpp): the fixed-point helpers, the sort of halfwords and the vector length.
// The helpers leave r4-r8 as the original does (Leftovers: the hook starts from the registers it was entered with and the
// routine overwrites the ones the original writes).
#define FONT_ENGINE_LEFTOVERS(c) font_engine::Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)}
#define FONT_ENGINE_SET_LEFTOVERS(c) c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}})
FONT_ENGINE_ROUTINE(0x10767ce0, 0x10767d34, NONE, "35af70b3a6d96ab4", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); font_engine::sort_shorts(c, c.r(3), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); })
FONT_ENGINE_ROUTINE(0x10767d40, 0x10767dbc, R3, "e4215acc432d795e", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); const auto v = font_engine::fix_multiply(std::int32_t(c.r(3)), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); c.set(3, std::uint32_t(v)); })
FONT_ENGINE_ROUTINE(0x10767dc0, 0x10767e40, R3, "6bb91661067f86ad", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); const auto v = font_engine::frac_multiply(std::int32_t(c.r(3)), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); c.set(3, std::uint32_t(v)); })
FONT_ENGINE_ROUTINE(0x10767e40, 0x10767eb0, R3, "857fcee0ddc25f02", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); const auto v = font_engine::scale_multiply(std::int32_t(c.r(3)), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); c.set(3, std::uint32_t(v)); })
FONT_ENGINE_ROUTINE(0x10767eb0, 0x10767f20, R3, "47b0c8b07837ec75", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); const auto v = font_engine::fix_divide(std::int32_t(c.r(3)), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); c.set(3, std::uint32_t(v)); })
FONT_ENGINE_ROUTINE(0x10767f20, 0x10768074, R3, "6b3102e8b6929a18", [](Cpu& c) { FONT_ENGINE_LEFTOVERS(c); const auto v = font_engine::distance(std::int32_t(c.r(3)), std::int32_t(c.r(4)), left); FONT_ENGINE_SET_LEFTOVERS(c); c.set(3, std::uint32_t(v)); })
#undef FONT_ENGINE_SET_LEFTOVERS
#undef FONT_ENGINE_LEFTOVERS

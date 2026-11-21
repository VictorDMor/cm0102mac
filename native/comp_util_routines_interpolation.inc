// COMP_UTIL.CPP block `interpolation` (comp_util_interpolation.cpp): the 0..100 position value.
COMP_UTIL_ROUTINE(0x10476780, 0x10476898, R3, "2e52294b3d430d3b", [](Cpu& c) { c.set(3, std::uint32_t(std::int32_t(comp_util::interpolated_share(c, std::int8_t(c.r(3)), std::int8_t(c.r(4)), std::int16_t(c.r(5)), std::int16_t(c.r(6)))))); })

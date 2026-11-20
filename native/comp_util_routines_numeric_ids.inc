// COMP_UTIL.CPP block `numeric_ids` (comp_util_ids.cpp): pure functions of a number.
COMP_UTIL_ROUTINE(0x10464010, 0x10464154, R3, "2ec40f65e040580e", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::parent_series(std::int32_t(c.r(3))))); })
COMP_UTIL_ROUTINE(0x10464880, 0x104648b8, R3, "7e8980ea389b9fb8", [](Cpu& c) { c.set(3, std::uint32_t(std::int32_t(comp_util::last_digit(c, std::int32_t(c.r(3)))))); })
COMP_UTIL_ROUTINE(0x104648c0, 0x104648fc, R3, "0fea99944b73c6a3", [](Cpu& c) { c.set(3, std::uint32_t(std::int32_t(comp_util::round_down_to_ten(c, std::int16_t(c.r(3)))))); })

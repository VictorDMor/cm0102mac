// COMP_UTIL.CPP block `club_queries` (comp_util_clubs.cpp, comp_util_race.cpp): queries over clubs, fixtures and
// competitions.
COMP_UTIL_ROUTINE(0x104646d0, 0x104647a0, R3, "cee4821e44300eee", [](Cpu& c) { c.set(3, comp_util::competition_is_active(c, std::int32_t(c.r(3)))); })
COMP_UTIL_ROUTINE(0x10464900, 0x10464d00, R3, "6efd0d16e2a01a08", [](Cpu& c) { c.set(3, comp_util::fixture_class(c, c.r(3), c.r(4))); })
COMP_UTIL_ROUTINE(0x10464160, 0x104646c4, R3, "81b7a3a8a62d4dc0", [](Cpu& c) { c.set(3, comp_util::contenders_class(c, c.r(3), c.r(4))); })

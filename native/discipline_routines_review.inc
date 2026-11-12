// DISCIPLINE.CPP review block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100e8780, 0x100e8d10, R3, "74dec42773fa8075", [](Cpu& c) { c.set(3, discipline::review_player(c, c.r(3), c.r(4), c.r(5))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e8d10, 0x100e9288, R3, "638f68895ae47e62", [](Cpu& c) { c.set(3, std::uint32_t(discipline::sample_range(c, std::int32_t(c.r(3))))); c.finish(); })

// CUP.CPP, block `draws` (cup_base_draws*.cpp): the routines, all in production (CUP_BASE_ROUTINE). All four draw from the
// game's sample stream; 0x1047fd00, 0x10480620 and 0x104810b0 are read for r8-r10 by their caller 0x1047e980.
CUP_BASE_ROUTINE(0x1047a9e0, 0x1047b354, R3, "21cb0f405ebbf5d7", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::draws::draw_below(c, std::int32_t(c.r(3))))); })
CUP_BASE_ROUTINE(0x1047fd00, 0x10480620, NONE, "dc5c12d3fc2455c6", [](Cpu& c) { cup_base::draws::sort_round_teams(c, c.r(3), c.r(4)); })
CUP_BASE_ROUTINE(0x10480620, 0x104810ac, NONE, "3d46164fc59a54d3", [](Cpu& c) { cup_base::draws::check_round_teams_for_nation(c, c.r(3), c.r(4)); })
CUP_BASE_ROUTINE(0x104810b0, 0x10481ad0, NONE, "b7a444f47056cb5b", [](Cpu& c) { cup_base::draws::randomize_round_teams(c, c.r(3), c.r(4)); })

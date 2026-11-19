// CUP.CPP, block `tree` (cup_base_tree.cpp): the routines, all in production (CUP_BASE_ROUTINE). 0x10479100 draws from the
// game's sample stream (7 refill calls of rand) and is reached through vt +0x9c and from CUP_STAGE.CPP.
CUP_BASE_ROUTINE(0x10479100, 0x1047a9d4, R3, "b11039adefa29579", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::tree::map_fixture_tree(c, c.r(3)))); })  // +0x9c

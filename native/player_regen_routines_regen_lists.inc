// PLAYER_REGEN.CPP block `regen_lists` (player_regen_lists.cpp): the regen object's lists A-D, their appenders and removers, and the
// registration of a regenerated person.
PLAYER_REGEN_ROUTINE(0x102deb70, 0x102dec48, NONE, "15eb7bad40be6752", [](Cpu& c) { player_regen::remove_staff_pool_entry(c, c.r(3), std::int32_t(c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102ded90, 0x102dee68, NONE, "a1dfd6bc3b62bb8e", [](Cpu& c) { player_regen::remove_player_pool_entry(c, c.r(3), std::int32_t(c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102dbeb0, 0x102dbef8, NONE, "7f98dc43fcf99a5e", [](Cpu& c) { player_regen::pop_staff_pool_entry(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102dea50, 0x102deb70, NONE, "4eb907ddd90eb5b7", [](Cpu& c) { player_regen::append_staff_pool(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102dec50, 0x102ded84, NONE, "ec4d3ee3b7bcfb54", [](Cpu& c) { player_regen::append_player_pool(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102dee70, 0x102df020, NONE, "3279253b9def3e3b", [](Cpu& c) { player_regen::append_retirement(c, c.r(3), c.r(4)); })
PLAYER_REGEN_ROUTINE(0x102df020, 0x102df198, NONE, "bea88a303106bc09", [](Cpu& c) { player_regen::remove_retirement(c, c.r(3), c.r(4)); })
PLAYER_REGEN_ROUTINE(0x102df1a0, 0x102df6cc, NONE, "ab9966a89350fdff", [](Cpu& c) { player_regen::register_regen(c, c.r(3), c.r(4)); })
PLAYER_REGEN_ROUTINE(0x102df6d0, 0x102df778, NONE, "94791078758a5507", [](Cpu& c) { player_regen::pop_queue_entry(c, c.r(3)); })
PLAYER_REGEN_ROUTINE(0x102eb670, 0x102eb6f8, R3, "a76fe8bacf1ecd52", [](Cpu& c) { c.set(3, player_regen::remove_retirement_by_id(c, c.r(3), c.r(4))); })

// PLAYER_REGEN.CPP block `regen_queries` (player_regen_queries.cpp): queries over the regen object's lists and the people's
// kind table, and the two qsort comparators (the sort reads r3 only).
PLAYER_REGEN_ROUTINE(0x102de3c0, 0x102de3e4, R3, "b99f774168f53592", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::person_class(c, c.r(4)))); })
PLAYER_REGEN_ROUTINE(0x102de3f0, 0x102de464, R3, "708369b8707a38e1", [](Cpu& c) { c.set(3, player_regen::find_retirement_entry(c, c.r(3), c.r(4), std::int8_t(c.r(5)))); })
PLAYER_REGEN_ROUTINE(0x102dbe90, 0x102dbea8, R3, "ae1d60b15a642783", [](Cpu& c) { c.set(3, player_regen::staff_pool_has_entries(c, c.r(3))); })
PLAYER_REGEN_ROUTINE(0x102eb4e0, 0x102eb54c, R3, "17467a60446f9a8f", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::compare_queue_entries(c, c.r(3), c.r(4)))); })
PLAYER_REGEN_ROUTINE(0x102eb550, 0x102eb610, R3, "986c2dbfe793f876", [](Cpu& c) { c.set(3, std::uint32_t(player_regen::compare_retirement_entries(c, c.r(3), c.r(4)))); })
PLAYER_REGEN_ROUTINE(0x102eb610, 0x102eb670, R3, "fbc7a2c6eca95e81", [](Cpu& c) { c.set(3, player_regen::retirement_queued(c, c.r(3), c.r(4))); })

// PLAYER_STATS.CPP: the object's construction (the real-world history import), destruction, and "player stats.dat" (player_stats_files.hpp).
PLAYER_STATS_ROUTINE(0x10301540, 0x103023e0, R3, "2a16204d53e0f151", [](Cpu& c) { player_stats::create_stats(c, c.r(3), c.r(4)); })
PLAYER_STATS_ROUTINE(0x103023e0, 0x103024a0, R3, "8aa269dfbc49bfec", [](Cpu& c) { player_stats::destroy_stats(c, c.r(3), c.r(4)); })
PLAYER_STATS_ROUTINE(0x10306c00, 0x103078a0, R3, "d0597eddaef26409", [](Cpu& c) { player_stats::load_stats(c, c.r(3), c.r(4)); })
PLAYER_STATS_ROUTINE(0x103078a0, 0x10308090, R3, "7d08ac733ff4b96e", [](Cpu& c) { player_stats::save_stats(c, c.r(3), c.r(4)); })

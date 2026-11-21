// PLAYER_REGEN.CPP block `retire_decision` (player_regen_retire.cpp): who retires. The two scans' decisions read the caller's r19 (a staff
// member with no job byte): the hook hands it over as an argument.
PLAYER_REGEN_ROUTINE(0x102dcb20, 0x102dd904, R3, "1b1c7171357985f0", [](Cpu& c) { const auto r19 = c.r(19); c.set(3, player_regen::decide_retirement(c, c.r(4), c.r(5), r19)); })
PLAYER_REGEN_ROUTINE(0x102dd910, 0x102de3c0, R3, "46a515834be043f8", [](Cpu& c) { const auto r19 = c.r(19); c.set(3, player_regen::decide_retirement_with_club(c, c.r(4), c.r(5), r19)); })
PLAYER_REGEN_ROUTINE(0x102de6e0, 0x102dea48, R3, "603d1e9266164d03", [](Cpu& c) { c.set(3, player_regen::decide_unattached_retirement(c, c.r(4), c.r(5))); })
PLAYER_REGEN_ROUTINE(0x102de470, 0x102de6d4, NONE, "ac1beed6768e065b", [](Cpu& c) { player_regen::schedule_injury_retirement(c, c.r(3), c.r(4), c.r(5)); })

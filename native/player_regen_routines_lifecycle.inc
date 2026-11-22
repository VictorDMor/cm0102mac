// PLAYER_REGEN.CPP block `lifecycle_and_drivers` (player_regen_lifecycle.cpp): the regen object's constructor and destructor and the two scans of
// the retirement list. The scans read the callee-saved r19 of their caller (the job byte of a staff member with no job bytes): the hooks hand it over.
PLAYER_REGEN_ROUTINE(0x102d8230, 0x102d85c0, R3, "68785b5e452dd485", [](Cpu& c) { c.set(3, player_regen::construct_regen_object(c, c.r(3), c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102d85c0, 0x102d86b4, R3, "0ab10baefe69f0e7", [](Cpu& c) { c.set(3, player_regen::destroy_regen_object(c, c.r(3), c.r(4))); })
PLAYER_REGEN_ROUTINE(0x102d86c0, 0x102d8830, NONE, "eb82736b3324d019", [](Cpu& c) { const auto r19 = c.r(19); player_regen::scan_retirements(c, c.r(3), r19); })
PLAYER_REGEN_ROUTINE(0x102d8830, 0x102d8918, NONE, "c4ee97119de1d7d0", [](Cpu& c) { const auto r19 = c.r(19); player_regen::scan_initial_retirements(c, c.r(3), r19); })

// PLAYER_REGEN.CPP block `detach` (player_regen_detach.cpp): a retiring person leaves his club and national team.
PLAYER_REGEN_ROUTINE(0x102e1f20, 0x102e21b4, NONE, "4c884e4710c6abf7", [](Cpu& c) { player_regen::detach_player(c, c.r(3), c.r(4)); })
PLAYER_REGEN_ROUTINE(0x102e21c0, 0x102e27d4, NONE, "8697331f246f7112", [](Cpu& c) { player_regen::detach_staff(c, c.r(3), c.r(4)); })

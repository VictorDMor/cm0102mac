// PLAYER_REGEN.CPP block `persistence` (player_regen_file.cpp): "retire.dat" in the saved game.
PLAYER_REGEN_ROUTINE(0x102ea870, 0x102ead68, NONE, "b4fc2d5301f96b6c", [](Cpu& c) { player_regen::save_regen_file(c, c.r(3), c.r(4)); })
PLAYER_REGEN_ROUTINE(0x102ead70, 0x102eb4d4, R3, "76a66e9fe4a91d49", [](Cpu& c) { c.set(3, player_regen::load_regen_file(c, c.r(3), c.r(4))); })

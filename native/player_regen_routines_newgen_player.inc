// PLAYER_REGEN.CPP block `newgen_player` (player_regen_newgen_player.cpp): the creation of a new young player in place of a retired one. The caller
// 0x102dabb0 reads no register after the call; the routine takes the regen object (r3) and the person (r4) and reads no caller-saved register.
PLAYER_REGEN_ROUTINE(0x102e4530, 0x102e9894, NONE, "0f785dd74ccb94b6", [](Cpu& c) { player_regen::regenerate_player(c, c.r(3), c.r(4)); })

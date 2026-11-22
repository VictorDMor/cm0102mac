// PLAYER_REGEN.CPP block `regen_pending_processor` (player_regen_pending.cpp): the queue of regenerations and the spare staff's abilities. The
// caller reads no register after the call (the exemption is in player_regen_volatile_free.inc).
PLAYER_REGEN_ROUTINE(0x102dabb0, 0x102dbe84, NONE, "a1363a969717bb5c", [](Cpu& c) { player_regen::process_regen_queue(c, c.r(3)); })

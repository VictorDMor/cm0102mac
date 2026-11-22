// PLAYER_REGEN.CPP block `retirement_processor` (player_regen_retirement_processor.cpp): the daily walk of the retirement list. The caller
// 0x101198e8 reads r6-r10 after the call (what the last sort leaves), which the checker compares with every other register (r4-r10).
PLAYER_REGEN_ROUTINE(0x102d8920, 0x102da630, NONE, "f9140ee6e710b6cc", [](Cpu& c) { player_regen::process_retirements(c, c.r(3)); })

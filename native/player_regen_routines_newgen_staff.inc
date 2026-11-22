// PLAYER_REGEN.CPP block `newgen_staff` (player_regen_newgen_staff.cpp): the regeneration of a non-player staff person. The hook hands over the
// caller's callee-saved r20 (the job byte when the person has neither a club nor a national link) before the frame is built.
PLAYER_REGEN_ROUTINE(0x102e27e0, 0x102e4524, NONE, "235a4c3adba11962", [](Cpu& c) { const auto r20 = c.r(20); player_regen::regenerate_staff(c, c.r(3), c.r(4), r20); })

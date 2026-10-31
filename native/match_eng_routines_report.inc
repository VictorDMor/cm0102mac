// Original entries of the report block: the review of the match state after each step and
// the restarts for each play state.
MATCH_ENG_ROUTINE(0x101e50c0, 0x101e53a0, NONE, "0bd16ea907416530",
    [](Cpu& c) { match_eng::restart_state_2(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e53a0, 0x101e59b0, NONE, "e6f65f48938357be",
    [](Cpu& c) { match_eng::restart_state_3(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e59b0, 0x101e6420, NONE, "9c846ea2d7c3a7ec",
    [](Cpu& c) { match_eng::restart_state_6(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e6420, 0x101e66b0, NONE, "faed2906b58d839b",
    [](Cpu& c) { match_eng::restart_state_4(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e66b0, 0x101e68b0, NONE, "f1103f97e7162405",
    [](Cpu& c) { match_eng::restart_state_5(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e68b0, 0x101e6c50, NONE, "769cd2efe8451eb4",
    [](Cpu& c) { match_eng::restart_state_8(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e6c50, 0x101e88f0, NONE, "f7c19f78b19458b2",
    [](Cpu& c) { match_eng::review_play(c, c.r(3)); })

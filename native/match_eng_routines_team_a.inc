// Original entries of the team_a block.
MATCH_ENG_ROUTINE(0x101e8950, 0x101eaf10, R3, "f38c158ea1703228",
    [](Cpu& c) { match_eng::best_pass_receiver(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101eaf10, 0x101eaf50, F1, "3f59da294550fa81",
    [](Cpu& c) { match_eng::power(c, c.f(1), c.f(2)); })
MATCH_ENG_ROUTINE(0x101ec7b0, 0x101ec890, R3, "49e22320681d9e4f",
    [](Cpu& c) { match_eng::random_mark(c); })
MATCH_ENG_ROUTINE(0x101eaf60, 0x101ec7a0, R3, "991e4fa0274f9dc5",
    [](Cpu& c) { match_eng::best_player_in_ball_cell(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })

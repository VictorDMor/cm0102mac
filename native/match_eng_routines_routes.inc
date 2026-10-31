// Original entries of the routes block.
MATCH_ENG_ROUTINE(0x101eeeb0, 0x101ef3c0, R3, "3fea805c552fa9b8",
    [](Cpu& c) { match_eng::count_line_occupants(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101ef3c0, 0x101ef8d0, R3, "5e85cc3b4ff8dce1",
    [](Cpu& c) { match_eng::find_line_blocker(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101ef8d0, 0x101ef9a0, F1, "717178b09971009c",
    [](Cpu& c) { match_eng::cell_distance(c, c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101ef9a0, 0x101f0090, R3, "20af07e9e5a8dfe4",
    [](Cpu& c) { match_eng::first_blocker_to_goal(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
MATCH_ENG_ROUTINE(0x101f0090, 0x101f09d0, R3, "3ca3129a89eab068",
    [](Cpu& c) { match_eng::first_taker_to_cell(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f09d0, 0x101f0ad0, R3, "cb6f9c00fa2c3fb7",
    [](Cpu& c) { match_eng::cell_occupants(c, c.r(3), c.r(4), c.r(5), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f0ad0, 0x101f0bf0, R3, "14cd980cb22d3ea0",
    [](Cpu& c) { match_eng::cell_obstruction(c, c.r(3), c.r(4), c.r(5), c.r(7), c.r(8), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101f0c00, 0x101f0dd0, R3, "e9182a12e30c8ea3",
    [](Cpu& c) { match_eng::stop_for_ball_out(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f0dd0, 0x101f0e80, R3, "8baeb8fb1440b42f",
    [](Cpu& c) { match_eng::column_within_line(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101f0e80, 0x101f0ed0, R3, "5810d21bb336b5ba",
    [](Cpu& c) { match_eng::highest_column_of_line(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101f0ed0, 0x101f0f20, R3, "28e79e01d23f4a85",
    [](Cpu& c) { match_eng::lowest_column_of_line(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101f0f20, 0x101f1100, NONE, "8659fde1675daca7",
    [](Cpu& c) { match_eng::find_line_extremes(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101f1100, 0x101f14b0, NONE, "2a54bab3a3b4514f",
    [](Cpu& c) { match_eng::scatter_ball(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })

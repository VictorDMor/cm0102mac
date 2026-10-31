// Original entries of the space block.
MATCH_ENG_ROUTINE(0x101e28b0, 0x101e3060, NONE, "d3c110ca949c3c21",
    [](Cpu& c) { match_eng::home_cell(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101e3060, 0x101e3150, R3, "c6b677b17be12316",
    [](Cpu& c) { match_eng::side_has_ball(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e31a0, 0x101e32e0, R3, "e0d660f09930a586",
    [](Cpu& c) { match_eng::is_nearest_to_cell(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101e32e0, 0x101e3d50, R3, "45db6409d713cfd6",
    [](Cpu& c) { match_eng::nearest_player_to_cell(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101e3d50, 0x101e3de0, R3, "135b9f03deb8f70f",
    [](Cpu& c) { match_eng::position_value_a(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e3de0, 0x101e3e70, R3, "9bf8bd6ac3b5d388",
    [](Cpu& c) { match_eng::position_value_b(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e3e70, 0x101e3f00, R3, "b415ef08032c6f84",
    [](Cpu& c) { match_eng::previous_position_value_a(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e3f10, 0x101e4020, F1, "a26d2a920a273267",
    [](Cpu& c) { match_eng::nearest_distance(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101e4020, 0x101e4130, R3, "b4749159d15a0917",
    [](Cpu& c) { match_eng::count_nearby_players(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101e4130, 0x101e4e10, R3, "ef4878cb8f709e56",
    [](Cpu& c) { match_eng::update_line_limits(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101e4e10, 0x101e4ea0, R3, "1e23b354520790c0",
    [](Cpu& c) { match_eng::distance_behind_line(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101e4ed0, 0x101e5070, R3, "ab9d596d17131995",
    [](Cpu& c) { match_eng::lane_is_open(c, c.r(3), c.r(4), c.r(5), c.r(6)); })

// Original entries of the tactics_a block.
MATCH_ENG_ROUTINE(0x101d70d0, 0x101d7d30, R3, "a251b1106737fd1b",
    [](Cpu& c) { match_eng::choose_challenge_target(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d7d30, 0x101d7d90, R3, "be0dc8a9fd92f19c",
    [](Cpu& c) { match_eng::side_flag_400(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d7dc0, 0x101d8100, R3, "794066fbfad23677",
    [](Cpu& c) { match_eng::assess_cell(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101d8120, 0x101d8260, R3, "4a746c73f3570cc9",
    [](Cpu& c) { match_eng::marker_available(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MATCH_ENG_ROUTINE(0x101d8260, 0x101d8d60, R3, "9b430bade4c1e411",
    [](Cpu& c) { match_eng::choose_cell(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MATCH_ENG_ROUTINE(0x101d8d70, 0x101d8f20, R3, "5e90f5c74612d79f",
    [](Cpu& c) { match_eng::lane_clear(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101d8f20, 0x101d9080, NONE, "2b92b4142d28d681",
    [](Cpu& c) { match_eng::rate_goal_pressure(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d9080, 0x101d91b0, NONE, "38aade3b06ae2ec4",
    [](Cpu& c) { match_eng::build_actor_queue(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d91b0, 0x101d93a0, R3, "0d4c5a03401b5a8f",
    [](Cpu& c) { match_eng::compare_actors(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101d93b0, 0x101d9460, NONE, "28f310740b174e9f",
    [](Cpu& c) { match_eng::update_players_a(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d9460, 0x101d9510, NONE, "016ce97b7cd3f1ad",
    [](Cpu& c) { match_eng::update_players_b(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101d9510, 0x101d95a0, NONE, "4c206514b097ecda",
    [](Cpu& c) { match_eng::update_all_players(c, c.r(3), c.r(4)); })

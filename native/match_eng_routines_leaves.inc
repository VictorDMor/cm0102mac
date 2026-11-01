// Getters, setters and other routines without calls.
MATCH_ENG_ROUTINE(0x101d3ac0, 0x101d3ad0, R3, "a3b01c2818a815d1",
    [](Cpu& c) { match_eng_leaf_101d3ac0(c.s); })
MATCH_ENG_ROUTINE(0x101d3ad0, 0x101d3b40, F1, "01eb537e4103b12c",
    [](Cpu& c) { match_eng::side_rate(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101d3b40, 0x101d3b50, R3, "7530c6327914fb09",
    [](Cpu& c) { match_eng_leaf_101d3b40(c.s); })
MATCH_ENG_ROUTINE(0x101d3b50, 0x101d3b60, R3, "fad8f7f3111e324b",
    [](Cpu& c) { match_eng_leaf_101d3b50(c.s); })
MATCH_ENG_ROUTINE(0x101d3bf0, 0x101d3c40, R3, "fd1f3be91adf29c7",
    [](Cpu& c) { match_eng::fixture_flag_40(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101d3c40, 0x101d3c50, NONE, "a7aefd961d173054",
    [](Cpu& c) { match_eng_leaf_101d3c40(c.s); })
MATCH_ENG_ROUTINE(0x101d3c50, 0x101d3c80, R3, "e7f7f5e238756951",
    [](Cpu& c) { match_eng::fixture_team(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101d3c80, 0x101d3c90, F1, "39695edfe303546b",
    [](Cpu& c) { match_eng_leaf_101d3c80(c.s); })
MATCH_ENG_ROUTINE(0x101d5660, 0x101d5670, R3, "426229115462b664",
    [](Cpu& c) { match_eng_leaf_101d5660(c.s); })
MATCH_ENG_ROUTINE(0x101d5670, 0x101d5680, R3, "0b4dda9a4c529633",
    [](Cpu& c) { match_eng_leaf_101d5670(c.s); })
MATCH_ENG_ROUTINE(0x101d5680, 0x101d5690, R3, "a174177c6f2112bf",
    [](Cpu& c) { match_eng_leaf_101d5680(c.s); })
MATCH_ENG_ROUTINE(0x101d5690, 0x101d56a0, R3, "5abc34f74541bd3b",
    [](Cpu& c) { match_eng_leaf_101d5690(c.s); })
MATCH_ENG_ROUTINE(0x101d56a0, 0x101d56b0, R3, "5d62dd1353be9bea",
    [](Cpu& c) { match_eng_leaf_101d56a0(c.s); })
MATCH_ENG_ROUTINE(0x101d56b0, 0x101d56c0, R3, "f58b90ed9e623511",
    [](Cpu& c) { match_eng_leaf_101d56b0(c.s); })
MATCH_ENG_ROUTINE(0x101d56c0, 0x101d56d0, R3, "c2d2b98e4f0a970a",
    [](Cpu& c) { match_eng_leaf_101d56c0(c.s); })
MATCH_ENG_ROUTINE(0x101d56d0, 0x101d56e0, R3, "94a123dece1e8a52",
    [](Cpu& c) { match_eng_leaf_101d56d0(c.s); })
MATCH_ENG_ROUTINE(0x101d56e0, 0x101d56f0, R3, "7c9c5a0ec67f922b",
    [](Cpu& c) { match_eng_leaf_101d56e0(c.s); })
MATCH_ENG_ROUTINE(0x101d56f0, 0x101d5700, R3, "51edf1a82dcf9de4",
    [](Cpu& c) { match_eng_leaf_101d56f0(c.s); })
MATCH_ENG_ROUTINE(0x101d5700, 0x101d5730, NONE, "719fcd255ba8f37b",
    [](Cpu& c) { match_eng::append_marker(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101d5730, 0x101d5740, NONE, "88d00e9340ba4420",
    [](Cpu& c) { match_eng_leaf_101d5730(c.s); })
MATCH_ENG_ROUTINE(0x101d5740, 0x101d5750, NONE, "753f8213ce5cbb41",
    [](Cpu& c) { match_eng_leaf_101d5740(c.s); })
MATCH_ENG_ROUTINE(0x101d5750, 0x101d5760, NONE, "1135fc320b30cb98",
    [](Cpu& c) { match_eng_leaf_101d5750(c.s); })
MATCH_ENG_ROUTINE(0x101d57c0, 0x101d57d0, R3, "853295095026e524",
    [](Cpu& c) { match_eng_leaf_101d57c0(c.s); })
MATCH_ENG_ROUTINE(0x101d57d0, 0x101d5810, NONE, "c4969076316ccdeb",
    [](Cpu& c) { match_eng::set_pressure_adjustment(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101d5870, 0x101d5880, R3, "2197d311b75ff4d0",
    [](Cpu& c) { match_eng_leaf_101d5870(c.s); })
MATCH_ENG_ROUTINE(0x101d6690, 0x101d66a0, NONE, "c6bb7e9b40939e24",
    [](Cpu& c) { match_eng_leaf_101d6690(c.s); })
MATCH_ENG_ROUTINE(0x101d66a0, 0x101d66b0, NONE, "b0cd72c860d4d30d",
    [](Cpu& c) { match_eng_leaf_101d66a0(c.s); })
MATCH_ENG_ROUTINE(0x101d66b0, 0x101d66c0, NONE, "5ed9a692acfda0b8",
    [](Cpu& c) { match_eng_leaf_101d66b0(c.s); })
MATCH_ENG_ROUTINE(0x101d66c0, 0x101d66d0, NONE, "7f2c1ea6dc90d18b",
    [](Cpu& c) { match_eng_leaf_101d66c0(c.s); })
MATCH_ENG_ROUTINE(0x101d66d0, 0x101d66e0, NONE, "3ccf0591502d6260",
    [](Cpu& c) { match_eng_leaf_101d66d0(c.s); })
MATCH_ENG_ROUTINE(0x101d66e0, 0x101d66f0, NONE, "46261d5ef06a3bbd",
    [](Cpu& c) { match_eng_leaf_101d66e0(c.s); })
MATCH_ENG_ROUTINE(0x101d66f0, 0x101d6710, NONE, "1d05a0d04081c746",
    [](Cpu& c) { match_eng::count_side_event(c,c.r(3),c.r(4),0x1fb6); })
MATCH_ENG_ROUTINE(0x101d6770, 0x101d6780, R3, "c0d3401604c2c689",
    [](Cpu& c) { match_eng_leaf_101d6770(c.s); })
MATCH_ENG_ROUTINE(0x101d68f0, 0x101d6930, NONE, "a97bd788fead8568",
    [](Cpu& c) { match_eng::set_pending_restart(c,c.r(3),c.r(4),c.r(5),c.r(6),c.r(7),c.r(8),c.r(9),c.r(10)); })
MATCH_ENG_ROUTINE(0x101d6930, 0x101d6980, NONE, "f009196156037874",
    [](Cpu& c) { match_eng::clear_pending_restart(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101d6980, 0x101d69c0, R3, "5c23cd3a9cebf596",
    [](Cpu& c) { match_eng::pending_restart_side(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101d7d90, 0x101d7da0, R3, "53586d0a727bf2d0",
    [](Cpu& c) { match_eng_leaf_101d7d90(c.s); })
MATCH_ENG_ROUTINE(0x101d7da0, 0x101d7db0, R3, "6f4c9e42a801054e",
    [](Cpu& c) { match_eng_leaf_101d7da0(c.s); })
MATCH_ENG_ROUTINE(0x101d7db0, 0x101d7dc0, R3, "052391f88bdd1a75",
    [](Cpu& c) { match_eng_leaf_101d7db0(c.s); })
MATCH_ENG_ROUTINE(0x101d8100, 0x101d8110, R3, "aa3f779737e1a648",
    [](Cpu& c) { match_eng_leaf_101d8100(c.s); })
MATCH_ENG_ROUTINE(0x101d8110, 0x101d8120, R3, "47dd07249a69797e",
    [](Cpu& c) { match_eng_leaf_101d8110(c.s); })
MATCH_ENG_ROUTINE(0x101d8d60, 0x101d8d70, R3, "6992432cdbb255ea",
    [](Cpu& c) { match_eng_leaf_101d8d60(c.s); })
MATCH_ENG_ROUTINE(0x101d93a0, 0x101d93b0, F1, "793f53384b99694a",
    [](Cpu& c) { match_eng_leaf_101d93a0(c.s); })
MATCH_ENG_ROUTINE(0x101da6d0, 0x101da6f0, R3, "4a58d16ad1c852df",
    [](Cpu& c) { match_eng::nonzero_byte_1b(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101da6f0, 0x101da750, R3, "5290e16706ba9fcd",
    [](Cpu& c) { match_eng::valid_competition_record(c,c.r(4)); })
MATCH_ENG_ROUTINE(0x101da750, 0x101da760, R3, "6005205c3a7bfa05",
    [](Cpu& c) { match_eng_leaf_101da750(c.s); })
MATCH_ENG_ROUTINE(0x101db320, 0x101db330, NONE, "5b17ccb6875cc2f4",
    [](Cpu& c) { match_eng_leaf_101db320(c.s); })
MATCH_ENG_ROUTINE(0x101db330, 0x101db340, R3, "ab8c7684685306b4",
    [](Cpu& c) { match_eng_leaf_101db330(c.s); })
MATCH_ENG_ROUTINE(0x101de040, 0x101de060, NONE, "e1346ba24b8d9714",
    [](Cpu& c) { match_eng::count_side_event(c,c.r(3),c.r(4),0x1fbc); })
MATCH_ENG_ROUTINE(0x101de060, 0x101de080, NONE, "66fc1ee38262d22e",
    [](Cpu& c) { match_eng::count_side_event(c,c.r(3),c.r(4),0x1fbb); })
MATCH_ENG_ROUTINE(0x101de080, 0x101de0a0, NONE, "9dd48ee5e20fbd49",
    [](Cpu& c) { match_eng::count_side_event(c,c.r(3),c.r(4),0x1fbd); })
MATCH_ENG_ROUTINE(0x101e15e0, 0x101e1640, R3, "b1caffe9e097562d",
    [](Cpu& c) { match_eng::adjust_score_toward(c,c.r(4),c.f(1)); })
MATCH_ENG_ROUTINE(0x101e3150, 0x101e3180, R3, "f4b3be954477ee5e",
    [](Cpu& c) { match_eng::second_side_player(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101e3180, 0x101e31a0, R3, "c9829eea316483f2",
    [](Cpu& c) { match_eng::player_at(c,c.r(3),c.r(4),c.r(5)); })
MATCH_ENG_ROUTINE(0x101e3f00, 0x101e3f10, R3, "e3a1d7b87c978445",
    [](Cpu& c) { match_eng_leaf_101e3f00(c.s); })
MATCH_ENG_ROUTINE(0x101e4ea0, 0x101e4ed0, R3, "62860c8fe795cfb7",
    [](Cpu& c) { match_eng::column_excess(c,c.r(3),c.r(4),c.r(5)); })
MATCH_ENG_ROUTINE(0x101e5070, 0x101e5080, R3, "2aebb30cdd0670f2",
    [](Cpu& c) { match_eng_leaf_101e5070(c.s); })
MATCH_ENG_ROUTINE(0x101e5080, 0x101e50b0, R3, "8ed3f07bcbef9ec2",
    [](Cpu& c) { match_eng::player_text(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101e50b0, 0x101e50c0, NONE, "414b183d13233828",
    [](Cpu& c) { match_eng_leaf_101e50b0(c.s); })
MATCH_ENG_ROUTINE(0x101e88f0, 0x101e8900, R3, "de88be6a03fe143a",
    [](Cpu& c) { match_eng_leaf_101e88f0(c.s); })
MATCH_ENG_ROUTINE(0x101e8900, 0x101e8910, R3, "2faa9a78543494b5",
    [](Cpu& c) { match_eng_leaf_101e8900(c.s); })
MATCH_ENG_ROUTINE(0x101e8910, 0x101e8920, R3, "5d27f727f415504c",
    [](Cpu& c) { match_eng_leaf_101e8910(c.s); })
MATCH_ENG_ROUTINE(0x101e8920, 0x101e8930, R3, "314ef81ac6d03e50",
    [](Cpu& c) { match_eng_leaf_101e8920(c.s); })
MATCH_ENG_ROUTINE(0x101e8930, 0x101e8940, NONE, "4da3cd7cf0923bf6",
    [](Cpu& c) { match_eng_leaf_101e8930(c.s); })
MATCH_ENG_ROUTINE(0x101e8940, 0x101e8950, NONE, "54e3c92b3514494f",
    [](Cpu& c) { match_eng_leaf_101e8940(c.s); })
MATCH_ENG_ROUTINE(0x101eaf50, 0x101eaf60, R3, "7576706a66d0bbb9",
    [](Cpu& c) { match_eng_leaf_101eaf50(c.s); })
MATCH_ENG_ROUTINE(0x101ec7a0, 0x101ec7b0, R3, "8ca8ea08354116da",
    [](Cpu& c) { match_eng_leaf_101ec7a0(c.s); })
MATCH_ENG_ROUTINE(0x101ee510, 0x101ee520, R3, "ef06e8851f161355",
    [](Cpu& c) { match_eng_leaf_101ee510(c.s); })
MATCH_ENG_ROUTINE(0x101ee520, 0x101ee530, NONE, "907ee24bbf5abfad",
    [](Cpu& c) { match_eng_leaf_101ee520(c.s); })
MATCH_ENG_ROUTINE(0x101ee610, 0x101ee680, R3, "375629258f2f5f1b",
    [](Cpu& c) { match_eng::turn_cost(c,c.r(3),c.r(4),c.r(5),c.r(6),c.r(7)); })
MATCH_ENG_ROUTINE(0x101ee9c0, 0x101ee9f0, R3, "5d7490ca0267212c",
    [](Cpu& c) { match_eng::outside_central_rows(c,c.r(4)); })
MATCH_ENG_ROUTINE(0x101ee9f0, 0x101eea30, R3, "9836c9fbb7280785",
    [](Cpu& c) { match_eng::beyond_column(c,c.r(4),c.r(5)); })
MATCH_ENG_ROUTINE(0x101eea30, 0x101eeac0, R3, "43e28141e01fbb7f",
    [](Cpu& c) { match_eng::inside_end_area(c,c.r(4),c.r(5),c.r(6)); })
MATCH_ENG_ROUTINE(0x101f0bf0, 0x101f0c00, NONE, "e4c88a6393f117a7",
    [](Cpu& c) { match_eng_leaf_101f0bf0(c.s); })
MATCH_ENG_ROUTINE(0x101f14b0, 0x101f1520, NONE, "5d1361b10b8c084e",
    [](Cpu& c) { match_eng::set_last_touch_side(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101f1520, 0x101f1530, NONE, "fa792f388f07bd11",
    [](Cpu& c) { match_eng_leaf_101f1520(c.s); })
MATCH_ENG_ROUTINE(0x101f1860, 0x101f1870, R3, "fb70604234656277",
    [](Cpu& c) { match_eng_leaf_101f1860(c.s); })
MATCH_ENG_ROUTINE(0x101f1870, 0x101f1880, NONE, "8608deb0f9176766",
    [](Cpu& c) { match_eng_leaf_101f1870(c.s); })
MATCH_ENG_ROUTINE(0x101f1930, 0x101f1940, NONE, "40a3e8c53877150f",
    [](Cpu& c) { match_eng_leaf_101f1930(c.s); })
MATCH_ENG_ROUTINE(0x101f1940, 0x101f1950, NONE, "b4d2e994dfff00d0",
    [](Cpu& c) { match_eng_leaf_101f1940(c.s); })
MATCH_ENG_ROUTINE(0x101f1dc0, 0x101f1de0, R3, "aa35f58d453cc7be",
    [](Cpu& c) { match_eng_leaf_101f1dc0(c.s); })
MATCH_ENG_ROUTINE(0x101f1de0, 0x101f1e10, NONE, "deb471d3b3f66123",
    [](Cpu& c) { match_eng::clear_goalkeeper(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101f21c0, 0x101f21d0, NONE, "f9cc170b52dbcabb",
    [](Cpu& c) { match_eng_leaf_101f21c0(c.s); })
MATCH_ENG_ROUTINE(0x101f93d0, 0x101f93f0, R3, "89146985e4f461bd",
    [](Cpu& c) { match_eng_leaf_101f93d0(c.s); })
MATCH_ENG_ROUTINE(0x101f93f0, 0x101f9400, F1, "68c11a9398fcc80f",
    [](Cpu& c) { match_eng_leaf_101f93f0(c.s); })
MATCH_ENG_ROUTINE(0x101f9400, 0x101f9410, R3, "136f2e815be772e0",
    [](Cpu& c) { match_eng_leaf_101f9400(c.s); })
MATCH_ENG_ROUTINE(0x101f9410, 0x101f9420, R3, "66879032f2288a08",
    [](Cpu& c) { match_eng_leaf_101f9410(c.s); })
MATCH_ENG_ROUTINE(0x101f9420, 0x101f9430, R3, "fb90be870add0f45",
    [](Cpu& c) { match_eng_leaf_101f9420(c.s); })
MATCH_ENG_ROUTINE(0x101f9430, 0x101f9440, R3, "05c68b1ae10f8fe5",
    [](Cpu& c) { match_eng_leaf_101f9430(c.s); })
MATCH_ENG_ROUTINE(0x101f9440, 0x101f9460, R3, "5812531ab9bb852d",
    [](Cpu& c) { match_eng::player_at(c,c.r(3),c.r(4),c.r(5)); })
MATCH_ENG_ROUTINE(0x101f9460, 0x101f9470, NONE, "bb7eb64a3856b5c5",
    [](Cpu& c) { match_eng_leaf_101f9460(c.s); })
MATCH_ENG_ROUTINE(0x101fb570, 0x101fb580, R3, "aa761349b7da80aa",
    [](Cpu& c) { match_eng_leaf_101fb570(c.s); })
MATCH_ENG_ROUTINE(0x101fcde0, 0x101fce40, R3, "74155418f0290783",
    [](Cpu& c) { match_eng::scheduled_flag(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101fce40, 0x101fce60, R3, "5d3eb807e2f2a604",
    [](Cpu& c) { match_eng::is_statistics_fixture(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101fdb20, 0x101fdb50, R3, "a47a4138a52a3440",
    [](Cpu& c) { match_eng::thousands(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101fdb50, 0x101fdb70, R3, "a3f1e8f962011a74",
    [](Cpu& c) { match_eng::first_of_list(c,c.r(3)); })
MATCH_ENG_ROUTINE(0x101fe9f0, 0x101fea10, NONE, "a03a12d08aa790b9",
    [](Cpu& c) { match_eng::copy_cell_record(c,c.r(3),c.r(4)); })
MATCH_ENG_ROUTINE(0x101fea90, 0x101feae0, R3, "82516ffc89520394",
    [](Cpu& c) { match_eng::past_substitution_time(c,c.r(3)); })

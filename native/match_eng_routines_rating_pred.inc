// Original entries of the rating block: predicates (0x101fd6b0..0x101fdb70, 0x101fdc10).
MATCH_ENG_ROUTINE(0x101fd6b0, 0x101fd900, R3, "c4a170e2022c1831",
    [](Cpu& c) { match_eng::stage_number(c, c.r(3), c.r(4), c.r(5)); })
MATCH_ENG_ROUTINE(0x101fd900, 0x101fdb20, R3, "4f16c77724d456c2",
    [](Cpu& c) { match_eng::post_grade(c, c.r(3)); })
MATCH_ENG_ROUTINE(0x101fdb70, 0x101fdc10, R3, "adde7bbfc61e495d",
    [](Cpu& c) { match_eng::bench_average(c, c.r(3), c.r(4)); })
MATCH_ENG_ROUTINE(0x101fdc10, 0x101fe9f0, R3, "e163be8377e94def",
    [](Cpu& c) { match_eng::situation_met(c, c.r(3), c.r(4), c.r(5), c.r(6)); })

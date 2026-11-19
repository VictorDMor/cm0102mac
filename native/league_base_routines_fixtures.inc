// LEAGUE.CPP, block fixtures (league_base_fixtures.cpp): the fixture order of the standings.
LEAGUE_BASE_ROUTINE(0x104a4970, 0x104a60b4, NONE, "430a12119265ff7c", [](Cpu& c) { league_base::draw_fixture_order(c, c.r(3), std::int32_t(c.r(4))); })

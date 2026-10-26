// SHORTLIST_MANAGER.CPP: a candidate's assessed contribution and its helpers (shortlist_manager_assessment.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10359980, 0x10359ba0, R3, "6c18174264d7a9b4", [](Cpu& c) { c.set(3, std::uint32_t(shortlist_manager::position_group(c, std::int16_t(c.r(4))))); })
SHORTLIST_MANAGER_ROUTINE(0x10359ba0, 0x10359dc0, R3, "3976611f3361b692", [](Cpu& c) { c.set(3, std::uint32_t(shortlist_manager::club_target_reputation(c, c.r(3), c.r(4)))); })
SHORTLIST_MANAGER_ROUTINE(0x10361490, 0x10362d70, R3, "6510efcee70db355", [](Cpu& c) { c.set(3, shortlist_manager::assess_candidate_contribution(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9))); })

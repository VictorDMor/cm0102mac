// SHORTLIST_MANAGER.CPP: the club squad assessment (0x1035a9f0) and its helper (0x10359dc0) (shortlist_manager_squads.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10359dc0, 0x1035a9f0, NONE, "591558072263ae9a", [](Cpu& c) { shortlist_manager::lay_out_formation(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x1035a9f0, 0x1035e9f0, NONE, "b926e9a4e200a552", [](Cpu& c) { shortlist_manager::assess_squad(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })

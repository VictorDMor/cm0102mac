// SHORTLIST_MANAGER.CPP: the daily processing that drives the others: shortlist creation, the day's passes and their entry points (shortlist_manager_daily.hpp).
SHORTLIST_MANAGER_ROUTINE(0x103573c0, 0x10357cb0, NONE, "8b30615e8e95407d", [](Cpu& c) { shortlist_manager::create_shortlists(c, c.r(3)); })
SHORTLIST_MANAGER_ROUTINE(0x10357cb0, 0x10357d30, NONE, "e6c4cf6b66424e47", [](Cpu& c) { shortlist_manager::refresh_club_shortlist(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x10359020, 0x10359980, NONE, "b51312371a678fa6", [](Cpu& c) { shortlist_manager::share_out_budget(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x1036d6e0, 0x1036ec30, NONE, "87bf5b084391e40a", [](Cpu& c) { shortlist_manager::approach_targets(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x1036ec30, 0x1036f100, NONE, "e60c4ba97987ac51", [](Cpu& c) { shortlist_manager::rebuild_club_shortlist(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x1036f100, 0x1036f170, NONE, "562f211f4c38ecac", [](Cpu& c) { shortlist_manager::forget_approaches(c, c.r(3), c.r(4), c.r(5)); })

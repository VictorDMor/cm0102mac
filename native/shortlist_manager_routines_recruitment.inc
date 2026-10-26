// SHORTLIST_MANAGER.CPP: the daily interest distribution and each club's interest registration (shortlist_manager_recruitment.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10363ca0, 0x10363f60, NONE, "40e5a83e8f06d8d5", [](Cpu& c) { shortlist_manager::distribute_staff_interest(c, c.r(3), c.r(4), c.r(5)); })
SHORTLIST_MANAGER_ROUTINE(0x10363f60, 0x10364d00, R3, "d58e72772c08b1fd", [](Cpu& c) { c.set(3, shortlist_manager::register_club_interest(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
SHORTLIST_MANAGER_ROUTINE(0x10364d00, 0x10365590, R3, "0ce78c12ff2720eb", [](Cpu& c) { c.set(3, shortlist_manager::consider_requested_club_interest(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })

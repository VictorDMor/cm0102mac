// SHORTLIST_MANAGER.CPP: a club's interest evaluation (0x10369080) (shortlist_manager_interest.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10369080, 0x1036d240, F1, "85c77ec2bc7347d6", [](Cpu& c) {
    c.set_f(1, shortlist_manager::evaluate_club_interest(c, c.r(3), c.r(4), c.r(5), std::uint8_t(c.r(6)), c.r(7),
                                                         c.r(8), c.r(9)));
})

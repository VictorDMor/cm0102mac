// Original entries of the rating block.
#include "match_eng_routines_rating_pred.inc"
#include "match_eng_routines_rating_pick.inc"
MATCH_ENG_ROUTINE(0x101f9470, 0x101fb570, R3 | F1, "a0e5d2d6bf9a5562",
    [](Cpu& c) { match_eng::compute_ratings(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.f(1), c.r(9), c.r(10)); })
MATCH_ENG_ROUTINE(0x101fb580, 0x101fc670, R3 | F1, "957f392ad0f1f973",
    [](Cpu& c) { match_eng::compute_record_ratings(c, c.r(3), c.r(4), c.r(5), c.r(6), c.f(1), c.r(8), c.r(9)); })
MATCH_ENG_ROUTINE(0x101fc670, 0x101fcde0, R3 | F1, "7bb2c2cbfc1cea6f",
    [](Cpu& c) { match_eng::compute_side_tendency(c, c.r(3), c.r(4), c.f(1), c.r(6), c.r(7)); })

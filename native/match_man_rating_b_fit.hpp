#pragma once
// The staff fit's position groups of 0x10216830 (match_man_rating_b_fit.cpp): the judge's coaching
// bytes against the player's ratings, summed by the position the role asks for.
#include "match_man_rating_b_detail.hpp"

namespace cm::aot::match_man::appraisal {
// The records the groups read: the judge's staff data (r25), the player data (r28), the subject
// (r15), the judge (r14) and the role (r16, a signed 16-bit mask).
struct FitRecords {
    u32 staff, player_data, subject, judge, role;
};
// The sum (left in r4, with the role in r0) of the three "pre" terms and of the position group the
// role selects. Leaves the registers the original's arithmetic leaves for what follows.
void staff_fit_sum(Cpu& c, const FitRecords& records);
// The goalkeeper-trait bonus added to the sum in r4 (r0 on return holds the bonus).
void goalkeeper_trait_bonus(Cpu& c, u32 staff, u32 player_data);
}

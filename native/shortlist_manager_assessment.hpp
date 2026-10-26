#pragma once
// SHORTLIST_MANAGER.CPP: a candidate's assessed contribution and its helpers
// (shortlist_manager_assessment*.cpp). Each leaves what the original leaves
// (guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x10359980: the group (0-0x15) of a shortlist entry's position flags, 0xe
// for flags it does not list; leaves cr0 as the original's compares do.
std::int32_t position_group(Cpu& c, std::int16_t position);
// 0x10359ba0: recomputes a club's target reputation (inferred; the int16 at
// +6 of the club's 12-byte entry in the shortlist manager's +0xdc table) and
// returns it: a tenth of club+8 plus 4500, scaled by the manager's and the
// assistant's staff attributes, at least the club's reputation, at most
// 10000. Its one caller is 0x106ba770 (outside the module).
std::int32_t club_target_reputation(Cpu& c, std::uint32_t shortlists, std::uint32_t club);
// 0x10361490: fills in *valuation (the fee the club's judge estimates for
// the candidate on shortlist entry `entry`) and returns the candidate's
// contribution to the club, 0 when the valuation is out of reach. `budget`
// is signed; `capped` (int8) caps the result at the entry's limit.
std::uint32_t assess_candidate_contribution(Cpu& c, std::uint32_t shortlists, std::uint32_t club, std::uint32_t candidate,
                                            std::uint32_t entry, std::uint32_t valuation, std::uint32_t budget,
                                            std::uint32_t capped);
}

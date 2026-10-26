#pragma once
// SHORTLIST_MANAGER.CPP: the daily interest distribution and each club's interest registration.
// A club's recruitment entry (the manager's +0xdc table, one per club id)
// holds 40 shortlist rows; registering a person's interest at a club means
// finding a row of that club that wants the person (role, category, budget,
// the two assessments) and linking the person to it.
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x10363ca0: offers `staff` (a person available for recruitment) to the
// clubs in the game's club order, at most five registrations counted from
// the person's existing interest (the high nibble of byte +0xc of its
// 100-byte record, TOC -0x7b9c); `force` (low byte) skips the profile's
// threshold at +0xa (5,500).
void distribute_staff_interest(Cpu& c, std::uint32_t manager, std::uint32_t staff, std::uint32_t force);
// 0x10363f60: whether `club` registers its interest in `staff` on one of its
// shortlist rows whose roles fall within `roles` (low 16 bits); 1 also when
// the pair is already linked. Returns r3 as the original (0 or 1).
std::uint32_t register_club_interest(Cpu& c, std::uint32_t manager, std::uint32_t staff, std::uint32_t club,
                                     std::uint32_t roles);
// 0x10364d00: the same for a requested move, `valuation` the caller's
// figure for the person; with `apply` (low byte) clear the row is found but
// neither published nor linked.
std::uint32_t consider_requested_club_interest(Cpu& c, std::uint32_t manager, std::uint32_t staff, std::uint32_t club,
                                               std::uint32_t valuation, std::uint32_t apply);
}

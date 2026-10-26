#pragma once
// SHORTLIST_MANAGER.CPP: the club squad assessment (0x1035a9f0) and its helper (0x10359dc0).
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x10359dc0: lays out the 40 places of a club's squad plan: 25 player places
// (kind 0xb) and 15 staff places (kinds 6-10, role 1), and the positions each
// player place covers, from the plan's formation (int8 at plan+8, 0-9; any
// other value takes formation 0's layout). `manager` is not read.
void lay_out_formation(Cpu& c, std::uint32_t manager, std::uint32_t plan);
// 0x1035a9f0 ("Assessing Club Squads"): reviews `club`'s squad plan (`plan`,
// the manager's +0xdc entry for the club): frees places whose holders have
// gone, picks the players and staff for the open ones, marks contracts
// (status, happiness) and values each open place for the shortlists.
// `skip_human` skips a club in a human's hands; `refresh` lays the plan out
// anew (0x10359dc0) and lets the assessment change contracts.
void assess_squad(Cpu& c, std::uint32_t manager, std::uint32_t plan, std::uint32_t club, std::uint32_t skip_human,
                  std::uint32_t refresh);
}

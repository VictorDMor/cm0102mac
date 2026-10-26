#pragma once
// SHORTLIST_MANAGER.CPP: the candidate context evaluation (0x10366470).
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x10366470: how willing `person` is to join `club` for a job of
// `category` (its low byte, signed), as a factor (0: never; his own club
// starts from 3.5): filters, his likes, rivals and staff views, his
// contract, his job against the category, the club's value against the one
// he has, and with `refine` (low byte) and a score of 1.75 or more the
// reference quote (0x1037b6f0) against the club's budget. `object` is the
// shortlist manager (its per-club records at +0xdc); `reasons`, when not
// null, receives the reason bits (cleared first). Leaves r4-r10, cr0 and
// FPSCR as the original.
double evaluate_candidate_context(Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club,
                                  std::uint32_t refine, std::uint32_t category, std::uint32_t reasons);
}

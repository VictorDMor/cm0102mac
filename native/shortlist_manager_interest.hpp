#pragma once
// SHORTLIST_MANAGER.CPP: a club's interest evaluation (0x10369080),
// shortlist_manager_interest*.cpp.
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// evaluate_club_interest (0x10369080): how interested `club` is in `person`
// (a score, 0 when it would not approach him), from the transfer manager
// `manager`. `mode` (a byte) asks for the finance check, `loan` (r7, tested
// as a byte; inferred: a loan) and `role` (r8, the role the club has in
// mind, inferred) are kept whole as the original keeps them; `reasons`
// (optional) receives the reason bits.
double evaluate_club_interest(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
                              std::uint8_t mode, std::uint32_t loan, std::uint32_t role, std::uint32_t reasons);
}

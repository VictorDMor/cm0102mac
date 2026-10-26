#pragma once
// SHORTLIST_MANAGER.CPP: the candidate scan from the shortlist links (0x1035e9f0).
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x1035e9f0: fills one of `club`'s shortlist rows (`row`, 0x2c bytes) from
// the manager's candidate lists for the row's categories: scores every
// eligible person (other than `excluded`), keeps the best, links it to the
// club and may register the club's interest; with none found, retries once
// as a relaxed pass (row flag bit 7) or marks the row unfilled (+0x10 = -1).
// Recursive (the relaxed pass). shortlist_manager_candidates*.cpp.
void scan_candidates(Cpu& c, std::uint32_t manager, std::uint32_t row, std::uint32_t club, std::uint32_t excluded);
}

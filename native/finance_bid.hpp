#pragma once
// FINANCE.CPP, block bid: what a club can spend on a bid, and two random bookings on a club's account
// (see finance.hpp).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// 0x100f5770: how much the club in `request` (its funds as a signed 64-bit word at +0, the club's index
// at +8) can bid. The registers are the original's own words: `squad` (r5), `deals` (r8) and `checked`
// (r7) are flags read as bytes; `capped` (r6) is a byte the routine sets to 1 when it cut the bid, or 0.
// `other` (r4) is a record whose word +0 names a deal that does not count against the club. Returns the
// bid as a signed 64-bit word (the original's r3:r4); 0 when the club has no wage plan.
std::int64_t bid_limit(Cpu& c, std::uint32_t request, std::uint32_t other, std::uint32_t squad,
                       std::uint32_t capped, std::uint32_t checked, std::uint32_t deals);
// 0x100f9f40 (manager, club): the club moves up its facilities standing (byte +0x240) and pays for it
// (names inferred).
void upgrade_facilities(Cpu& c, std::uint32_t manager, std::uint32_t club);
// 0x100fa1f0 (manager, club): books a randomly sized sum, scaled by the club's reputation, as income
// on the ledger line at +0xbc (names inferred).
void book_sampled_income(Cpu& c, std::uint32_t manager, std::uint32_t club);
}

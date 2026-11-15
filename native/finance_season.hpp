#pragma once
// FINANCE.CPP, block season: the club reviews of the season (see finance.hpp).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// A donation to a club in need (names inferred): `object` is the first argument the news routine gets,
// `account` the club's account. Returns 1 when it booked one (and announced it), 0 when not.
std::uint32_t donation(Cpu&, std::uint32_t object, std::uint32_t club, std::uint32_t account);  // 0x100fe640
// The review of one club's finances (names inferred): takeovers and owner help, the squad budget, debts.
void review_club(Cpu&, std::uint32_t manager, std::uint32_t club);  // 0x100fd210
}

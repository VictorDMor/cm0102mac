#pragma once
// MATCH_MAN.CPP rating_b block: a person's value as judged by a club for a role.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10216830 (530,000 calls a day): the worth of `person` (a person record whose
// player data is at +0x14) as judged by `judge` for `club`, left in r3 and returned.
// - role: the positions (a bit mask, read as a signed 16-bit value) the value is for;
// - flags: the components of the value to include (r5, tested bit by bit);
// - floor: a signed minimum for the running average, or -1 for none: a call that
//   gives one returns -10000 as soon as the average of the abilities judged so
//   far falls below it;
// - judge: the person judging (may be 0, then club+0x90 is used).
// Returns -10000 for a person without player data. Calls 0x10219e40 (judged_ability).
std::int32_t rating_b(Cpu& c, std::uint32_t person, std::uint32_t role, std::uint32_t flags, std::uint32_t club,
                      std::int32_t floor, std::uint32_t judge);
}

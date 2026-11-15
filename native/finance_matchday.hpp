#pragma once
// FINANCE.CPP, block matchday: what a fixture earns and costs, and a club's ground works (see finance.hpp).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// 0x100f95e0: the amount `base` for a fixture, scaled by the clubs' reputation, the competition and its level
// (the figure MATCH_STATS.CPP asks for). A null fixture reports the failed check and answers 0.
std::int32_t fixture_income(Cpu&, std::uint32_t manager, std::uint32_t fixture, std::int32_t base);
// 0x100f7cf0: the home club adds `added_first` + `added_second` seats towards `wanted` / `planned`; books the cost on
// the account and tells the board (0x10103380 with `news`). `with_owner` (a byte) takes the capped path where the
// club's owner may cover the cost. True when it went ahead (or there was nothing to build).
bool expand_ground(Cpu&, std::uint32_t account, std::int32_t wanted, std::int32_t planned, std::uint32_t news,
                   std::uint32_t with_owner);
// 0x100f7270: a home match's takings for the club of `account` (gate, then the extras); `credit_gate` books the gate
// income too.
void matchday_takings(Cpu&, std::uint32_t account, std::uint32_t credit_gate);
// 0x100f8780: splits a played fixture's income between the two clubs' accounts (`record` is the match's attendance
// record: +0x34 the crowd, +0x38 a figure carried in). Null record or fixture reports the failed check.
void split_fixture_income(Cpu&, std::uint32_t manager, std::uint32_t fixture, std::uint32_t record);
}

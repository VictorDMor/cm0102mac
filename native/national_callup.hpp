#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Normal Lineup feature, independent of the debugging preferences.
bool is_national_squad(const State&, std::uint32_t club);
// Keep the best 23 eligible players by current ability (stable staff-id ties).
// A short national pool supplies everyone available. Retained players are untouched.
bool call_up_national_squad(State&, std::uint32_t club);
}

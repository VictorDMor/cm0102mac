#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Deliberate historical policy, separate from the original module's semantics.
bool inter_amer_cup_retired(State&);
std::uint32_t inter_amer_cup_creation_id(State&, std::uint32_t configured_id);
void inter_amer_cup_initialize_new_career(State&);
bool inter_amer_cup_retired_object(State&, std::uint32_t object);
bool inter_amer_cup_skip_daily(State&);
bool inter_amer_cup_cancel_pending(State&, std::uint32_t fixture);
bool inter_amer_cup_skip_host_announcement(State&, std::uint32_t row);
}

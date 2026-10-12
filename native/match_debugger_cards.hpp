#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_debugger {
// Debugger interventions use complete original discipline/availability methods.
// These bridges do not replace the game's foul decision policy.
void apply_original_card(State&, std::uint32_t match, std::uint32_t actor,
                         std::uint32_t opponent, bool yellow);
void apply_original_injury(State&, std::uint32_t match, std::uint32_t actor);
void return_dismissed_player(State&, std::uint32_t match, std::uint32_t actor, unsigned position);
}

#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_debugger {
// A debugger-only bridge into characterized original awarded-goal regions.
// This is intentionally not a reconstruction of the complete action routine.
void apply_original_awarded_goal(State&, std::uint32_t match, std::uint32_t actor, unsigned team);
}

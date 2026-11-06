#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_debugger {
// Deliberate debugger intervention through the live shot event frame.
void apply_awarded_goal(State&, std::uint32_t match, std::uint32_t actor, unsigned team);
}

#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Semantic bodies share the exact direct-call ABI. Destination hooks own
// original selection, counters and optional private differential diagnostics.
bool execute_semantic_calendar_boundary(State&, std::uint32_t entry);
bool execute_semantic_date(State& state, std::uint32_t entry);
}

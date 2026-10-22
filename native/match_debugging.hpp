#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Host-only preference; it is never stored in an original career or resource.
bool native_debugging_available();
bool native_debugging_enabled();
void initialize_native_debugging(bool enabled = false);
void select_native_debugging(bool enabled);

void append_debugging_option(State&);
void begin_debugging_options();
bool dispatch_debugging_option(State&);
void append_match_debugging_controls(State&);
bool dispatch_match_debugging_control(State&);
void reset_match_debugging();

// Shared validation for the live UI and focused original-engine checks.
bool eligible_debug_goal(const State&, std::uint32_t match, unsigned player);
void award_debug_goal(State&, std::uint32_t match, unsigned player);
}

#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in tactics_routines.inc: runs it as C++
// and continues at the return address. Always true, except while a checker
// runs the original body (routine_check.hpp).
bool try_modern_tactics(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool tactics_dispatch_probe(State& s);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void tactics_run_original(State& s, std::uint32_t entry);
void tactics_run_rewrite(State& s, std::uint32_t entry);
}

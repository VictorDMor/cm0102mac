#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in dispute_routines.inc (the
// original's DISPUTE.CPP): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_DISPUTE_ORIGINAL=1.
bool try_modern_dispute(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool dispute_dispatch_probe(State& s);
bool dispute_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void dispute_run_original(State& s, std::uint32_t entry);
void dispute_run_rewrite(State& s, std::uint32_t entry);
}

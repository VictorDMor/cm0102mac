#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in awol_routines.inc (the
// original's AWOL.CPP): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_AWOL_ORIGINAL=1.
bool try_modern_awol(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool awol_dispatch_probe(State& s);
bool awol_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void awol_run_original(State& s, std::uint32_t entry);
void awol_run_rewrite(State& s, std::uint32_t entry);
}

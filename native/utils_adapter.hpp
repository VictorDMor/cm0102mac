#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in utils_routines.inc (the original's
// UTILS.CPP): runs it as C++ and continues at the return address. Always true,
// except while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_UTILS_ORIGINAL=1.
// The generator hooks every entry here but 0x107474a0, where try_modern_rng_core
// keeps its diagnostics and calls utils::initialize_random.
bool try_modern_utils(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool utils_dispatch_probe(State& s);
bool utils_lists(std::uint32_t entry);
// Whether the original body must run now, for try_modern_rng_core.
bool utils_declining();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void utils_run_original(State& s, std::uint32_t entry);
void utils_run_rewrite(State& s, std::uint32_t entry);
}

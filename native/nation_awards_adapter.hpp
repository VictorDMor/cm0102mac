#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in nation_awards_routines.inc (the
// original's NATION_AWARDS.CPP): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_NATION_AWARDS_ORIGINAL=1.
bool try_modern_nation_awards(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool nation_awards_dispatch_probe(State& s);
bool nation_awards_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void nation_awards_run_original(State& s, std::uint32_t entry);
void nation_awards_run_rewrite(State& s, std::uint32_t entry);
}

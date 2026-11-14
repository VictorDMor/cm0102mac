#pragma once
#include "aot_runtime.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's rb_*.cpp units
// (tools/ruling_bodies_audit.py): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_RULING_BODIES_ORIGINAL=1.
bool try_modern_ruling_bodies(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool ruling_bodies_dispatch_probe(State& s);
bool ruling_bodies_lists(std::uint32_t entry);
std::vector<std::uint32_t> ruling_bodies_entries();
// The result registers the entry's line declares (RoutineChecker::result_r3).
unsigned ruling_bodies_results(std::uint32_t entry);
// For fixtures and synthetic calls (routine_check.hpp): run one routine's
// original body or its C++ from its entry state to its return.
void ruling_bodies_run_original(State& s, std::uint32_t entry);
void ruling_bodies_run_rewrite(State& s, std::uint32_t entry);
}

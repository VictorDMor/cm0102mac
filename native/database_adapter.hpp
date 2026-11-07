#pragma once
#include "aot_runtime.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine in database_routines.inc (the
// original's DATABASE.CPP): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_DATABASE_ORIGINAL=1.
bool try_modern_database(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool database_dispatch_probe(State& s);
bool database_lists(std::uint32_t entry);
// Listed as DATABASE_ROUTINE: the generator hooks its entry in production.
bool database_hooked(std::uint32_t entry);
// Every routine database_routines.inc lists (drafts too), for check tools.
std::vector<std::uint32_t> database_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void database_run_original(State& s, std::uint32_t entry);
void database_run_rewrite(State& s, std::uint32_t entry);
}

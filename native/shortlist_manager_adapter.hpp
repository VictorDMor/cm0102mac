#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of every routine of the original's SHORTLIST_MANAGER.CPP
// (tools/shortlist_manager_audit.py): runs a routine listed as
// SHORTLIST_MANAGER_ROUTINE in shortlist_manager_routines.inc as C++ and
// continues at the return address. Declines (the generated body runs) for
// routines not listed yet, for drafts outside check tools, while a checker
// runs the original body (routine_check.hpp), for routines a check run leaves
// out (--only), and with CM_SHORTLIST_MANAGER_ORIGINAL=1.
bool try_modern_shortlist_manager(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// (drafts included) reached through the dispatcher.
bool shortlist_manager_dispatch_probe(State& s);
bool shortlist_manager_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void shortlist_manager_run_original(State& s, std::uint32_t entry);
void shortlist_manager_run_rewrite(State& s, std::uint32_t entry);
}

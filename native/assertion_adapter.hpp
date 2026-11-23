#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the assertion module (tools/assertion_audit.py): runs it as C++ and continues at the return
// address. Drafts (ASSERTION_DRAFT) keep their generated body in production; false also while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with CM_ASSERTION_ORIGINAL=1.
bool try_modern_assertion(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine reached through the dispatcher.
bool assertion_dispatch_probe(State& s);
bool assertion_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool assertion_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned assertion_results(std::uint32_t entry);
// Listed as ASSERTION_ROUTINE: the hook runs it in production.
std::vector<std::uint32_t> assertion_production_entries();
// Every routine assertion_routines.inc lists (drafts too).
std::vector<std::uint32_t> assertion_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or its C++ from its entry state to its return.
void assertion_run_original(State& s, std::uint32_t entry);
void assertion_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool assertion_declining;
}

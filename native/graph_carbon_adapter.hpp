#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of graph_carbon.cpp
// (tools/graph_carbon_audit.py): runs it as C++ and continues at the return
// address. Drafts (GRAPH_CARBON_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_GRAPH_CARBON_ORIGINAL=1.
bool try_modern_graph_carbon(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool graph_carbon_dispatch_probe(State& s);
bool graph_carbon_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool graph_carbon_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned graph_carbon_results(std::uint32_t entry);
// Listed as GRAPH_CARBON_ROUTINE: the hook runs it in production.
std::vector<std::uint32_t> graph_carbon_production_entries();
// Every routine graph_carbon_routines.inc lists (drafts too).
std::vector<std::uint32_t> graph_carbon_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void graph_carbon_run_original(State& s, std::uint32_t entry);
void graph_carbon_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool graph_carbon_declining;
// The checker's call trace (graph_carbon_checks.cpp) sees the generated callers' calls as dispatches; a typed call from this module's C++
// into another module's rewritten routine is announced here instead (null in the app): the target, with the link register and the
// argument registers set as at the original's `bl`.
extern void (*graph_carbon_direct_call_probe)(State&, std::uint32_t target);
}

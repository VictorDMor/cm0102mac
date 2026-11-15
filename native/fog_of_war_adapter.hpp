#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's fog_of_war.cpp
// (tools/fog_of_war_audit.py): runs it as C++ and continues at the return
// address. Drafts (FOG_OF_WAR_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_FOG_OF_WAR_ORIGINAL=1.
bool try_modern_fog_of_war(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool fog_of_war_dispatch_probe(State& s);
bool fog_of_war_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool fog_of_war_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned fog_of_war_results(std::uint32_t entry);
// Listed as FOG_OF_WAR_ROUTINE: the hook runs it in production.
std::vector<std::uint32_t> fog_of_war_production_entries();
// Every routine fog_of_war_routines.inc lists (drafts too).
std::vector<std::uint32_t> fog_of_war_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void fog_of_war_run_original(State& s, std::uint32_t entry);
void fog_of_war_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool fog_of_war_declining;
}

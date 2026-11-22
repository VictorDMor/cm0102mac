#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's font engine
// (tools/font_engine_audit.py): runs it as C++ and continues at the return
// address. Drafts (FONT_ENGINE_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_FONT_ENGINE_ORIGINAL=1.
bool try_modern_font_engine(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool font_engine_dispatch_probe(State& s);
bool font_engine_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool font_engine_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned font_engine_results(std::uint32_t entry);
// Listed as FONT_ENGINE_ROUTINE: the hook runs it in production.
std::vector<std::uint32_t> font_engine_production_entries();
// Every routine font_engine_routines.inc lists (drafts too).
std::vector<std::uint32_t> font_engine_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void font_engine_run_original(State& s, std::uint32_t entry);
void font_engine_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool font_engine_declining;
}

#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's FINANCE.CPP
// (tools/finance_audit.py): runs it as C++ and continues at the return
// address. Drafts (FINANCE_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_FINANCE_ORIGINAL=1.
bool try_modern_finance(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool finance_dispatch_probe(State& s);
bool finance_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool finance_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned finance_results(std::uint32_t entry);
// Listed as FINANCE_ROUTINE: the hook runs it in production.
std::vector<std::uint32_t> finance_production_entries();
// Every routine finance_routines.inc lists (drafts too).
std::vector<std::uint32_t> finance_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void finance_run_original(State& s, std::uint32_t entry);
void finance_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool finance_declining;
namespace finance {
// A call from one rewritten routine of the unit to another: runs the listed routine's C++ in
// place, at the original return address `back`. A checker's parent goes through the hook, so
// nested calls are checked too, and routines a check run leaves out run their originals.
void call(guest::Cpu& c, std::uint32_t entry, std::uint32_t back);
}
}

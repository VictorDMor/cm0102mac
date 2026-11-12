#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's DISCIPLINE.CPP
// (tools/discipline_audit.py): runs it as C++ and continues at the return
// address. Drafts (DISCIPLINE_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_DISCIPLINE_ORIGINAL=1.
bool try_modern_discipline(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool discipline_dispatch_probe(State& s);
bool discipline_lists(std::uint32_t entry);
// Whether the entry is written native-first and compared under the native contract
// (discipline_native.inc; routine_check.hpp).
bool discipline_native(std::uint32_t entry);
// Whether the checker leaves the entry's dead stack out (every entry but the ones the stack audit lists as
// leaving bytes for a later reader).
bool discipline_dead_stack_free(std::uint32_t entry);
// Whether the address lies in the body of a routine written native-first (a call site's return address).
bool discipline_native_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned discipline_results(std::uint32_t entry);
// Listed as DISCIPLINE_ROUTINE: the generator hooks its entry in production.
std::vector<std::uint32_t> discipline_production_entries();
// Every routine discipline_routines.inc lists (drafts too).
std::vector<std::uint32_t> discipline_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void discipline_run_original(State& s, std::uint32_t entry);
void discipline_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool discipline_declining;
// Checker only (null in the app): told of each call of a listed routine before
// it runs, from the entry state.
extern void (*discipline_call_observer)(State&, std::uint32_t entry, unsigned results);
namespace discipline {
// A call from one rewritten DISCIPLINE.CPP routine to another (or to any
// routine): runs a listed routine's C++ in place, at the original return
// address `back`; a checker's parent goes through the hook, so nested calls
// are checked too, and routines a check run leaves out run their originals.
void call(guest::Cpu& c, std::uint32_t entry, std::uint32_t back);
}
}

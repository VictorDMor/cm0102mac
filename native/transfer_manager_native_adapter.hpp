#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's TRANSFER_MANAGER.CPP
// (tools/transfer_manager_audit.py) written native-first: runs it as C++ and continues at the return
// address. Drafts (TRANSFER_MANAGER_NATIVE_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_TRANSFER_MANAGER_ORIGINAL=1.
bool try_modern_transfer_manager_native(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool transfer_manager_native_dispatch_probe(State& s);
bool transfer_manager_native_lists(std::uint32_t entry);
// Whether the address lies in the body of a listed routine (a call site's return address).
bool transfer_manager_native_code(std::uint32_t address);
// The result registers the entry's line declares (RoutineChecker::result_r3 / result_f1).
unsigned transfer_manager_native_results(std::uint32_t entry);
// Listed as TRANSFER_MANAGER_NATIVE: the hook runs it in production.
std::vector<std::uint32_t> transfer_manager_native_production_entries();
// Every routine transfer_manager_native_routines.inc lists (drafts too).
std::vector<std::uint32_t> transfer_manager_native_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void transfer_manager_native_run_original(State& s, std::uint32_t entry);
void transfer_manager_native_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool transfer_manager_native_declining;
namespace transfer_manager_native {
// A call from one rewritten routine of the unit to another: runs the listed routine's C++ in
// place, at the original return address `back`. A checker's parent goes through the hook, so
// nested calls are checked too, and routines a check run leaves out run their originals.
void call(guest::Cpu& c, std::uint32_t entry, std::uint32_t back);
}
}

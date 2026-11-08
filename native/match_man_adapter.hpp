#pragma once
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's MATCH_MAN.CPP
// (tools/match_man_audit.py): runs it as C++ and continues at the return
// address. Drafts (MATCH_MAN_DRAFT) keep their generated body in production;
// false also while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_MATCH_MAN_ORIGINAL=1.
bool try_modern_match_man(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool match_man_dispatch_probe(State& s);
bool match_man_lists(std::uint32_t entry);
// Listed as MATCH_MAN_ROUTINE: the generator hooks its entry in production.
std::vector<std::uint32_t> match_man_production_entries();
// Every routine match_man_routines.inc lists (drafts too).
std::vector<std::uint32_t> match_man_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void match_man_run_original(State& s, std::uint32_t entry);
void match_man_run_rewrite(State& s, std::uint32_t entry);
// Set while every routine runs its original body.
extern bool match_man_declining;
// Checker only (null in the app): told of each call of a listed routine before
// it runs, from the entry state.
extern void (*match_man_call_observer)(State&, std::uint32_t entry, unsigned results);
namespace match_man {
// A call from one rewritten MATCH_MAN.CPP routine to another (or to any
// routine): runs a listed routine's C++ in place, at the original return
// address `back`; a checker's parent goes through the hook, so nested calls
// are checked too, and routines a check run leaves out run their originals.
void call(guest::Cpu& c, std::uint32_t entry, std::uint32_t back);
}
}

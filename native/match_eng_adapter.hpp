#pragma once
#include "aot_runtime.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of every routine of the original's MATCH_ENG.CPP: runs the
// routines listed in match_eng_routines.inc as C++ and continues at the return
// address. False while a checker runs the original body (routine_check.hpp),
// for routines a check run leaves out (--only), for routines not reconstructed
// yet, or with CM_MATCH_ENG_ORIGINAL=1.
bool try_modern_match_eng(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe).
bool match_eng_dispatch_probe(State& s);
bool match_eng_lists(std::uint32_t entry);
// The entries that run as C++ in the normal app.
std::vector<std::uint32_t> match_eng_production_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void match_eng_run_original(State& s, std::uint32_t entry);
void match_eng_run_rewrite(State& s, std::uint32_t entry);
// An older match family's original route (its fixture checks):
// the entry's generated body with MATCH_PL's and this module's hooks declining, since
// those hooks now sit on the pages in place of the family's.
void match_family_run_original(State& s, std::uint32_t entry);
// Check tools only: this state always runs the original bodies, so a host-level check
// can step one match as C++ and a control match as the original in one process.
extern const State* match_eng_original_state;
// Set while every routine runs its original body (see match_eng_leaves.hpp).
extern bool match_eng_declining;
// Checker only (null in the app): told of each call of a listed routine before
// it runs, from the entry state.
extern void (*match_eng_call_observer)(State&, std::uint32_t entry, unsigned results);
}

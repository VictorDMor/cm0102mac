#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Runs the C++ owner of `entry` (s.pc) as the entry's page would before its
// first instruction. The table (generated direct_owners.cpp, from
// tools/compile_recomp.py direct_owner_table) holds the owners production
// pages call in place plus CPP_IN_PLACE_WATCHED, which only checks watch by
// PC, restricted to entries whose page holds exactly that one standard hook
// behind a plain dispatch case. False, with nothing run, when `entry` has no
// such owner or it declined. Leaves may be listed, but run_owner_until tries
// run_leaf first, as run_until does: a leaf run_leaf takes still runs its
// generated body, and a C++ leaf it leaves alone reaches its hook, as on the
// page.
bool run_owner_in_place(State& s, std::uint32_t entry);

// run_until, except that the first instruction's owner runs in place of its
// page: the same leaf shortcut and dispatcher scope (context transfers resume
// where they would). With a dispatch probe or a routine checker it is
// run_until. Because of CPP_IN_PLACE_WATCHED, a caller with its own check
// hooks installed makes ordinary guest calls instead
// (guest::Cpu::call_in_place).
void run_owner_until(State& s, std::uint32_t return_pc);
}

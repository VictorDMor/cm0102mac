#pragma once
#include "aot_runtime.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's GAME_CONFIG.CPP
// (tools/game_config_audit.py): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_GAME_CONFIG_ORIGINAL=1.
bool try_modern_game_config(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool game_config_dispatch_probe(State& s);
bool game_config_lists(std::uint32_t entry);
std::vector<std::uint32_t> game_config_entries();
// The result registers the entry's line declares (RoutineChecker::result_r3).
unsigned game_config_results(std::uint32_t entry);
// For fixtures and synthetic calls (routine_check.hpp): run one routine's
// original body or its C++ from its entry state to its return.
void game_config_run_original(State& s, std::uint32_t entry);
void game_config_run_rewrite(State& s, std::uint32_t entry);
}

#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Private, unsaved evidence for the initializer transferred from the legacy
// engine-initialization owner. Returns count only completed frame epilogues.
struct ScreenManagerDiagnostics {
    std::uint64_t initialization_calls = 0;
    std::uint64_t initialization_returns = 0;
};
ScreenManagerDiagnostics& screen_manager_diagnostics(State&);
const ScreenManagerDiagnostics& screen_manager_diagnostics(const State&);

// Hook at the entry of each routine in screen_manager_routines.inc (the
// original's SCREEN_MANAGER.CPP): runs it as C++ and continues at the return
// address. Always true, except while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_SCREEN_MANAGER_ORIGINAL=1.
bool try_modern_screen_manager(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool screen_manager_dispatch_probe(State& s);
bool screen_manager_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void screen_manager_run_original(State& s, std::uint32_t entry);
void screen_manager_run_rewrite(State& s, std::uint32_t entry);
}

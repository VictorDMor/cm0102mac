#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in file_screens_routines.inc (the original's
// FILE_SCREENS.CPP): runs it as C++ and continues at the return address. Always
// true, except while a checker runs the original body (routine_check.hpp), for
// routines a check run leaves out (--only), or with CM_FILE_SCREENS_ORIGINAL=1.
bool try_modern_file_screens(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool file_screens_dispatch_probe(State& s);
bool file_screens_lists(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void file_screens_run_original(State& s, std::uint32_t entry);
void file_screens_run_rewrite(State& s, std::uint32_t entry);
}

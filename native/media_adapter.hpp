#pragma once
#include "aot_runtime.hpp"
#include <vector>

namespace cm::aot {
// Hook at the entry of each routine of the original's media.cpp: runs it as C++
// and continues at the return address. Drafts (media_routines_*.inc) stay with
// their generated body in production; false also while a checker runs the original
// body (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_MEDIA_ORIGINAL=1.
bool try_modern_media(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher.
bool media_dispatch_probe(State& s);
bool media_lists(std::uint32_t entry);
// Whether the routine has C++ a checker may run, draft or verified.
std::vector<std::uint32_t> media_production_entries();
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void media_run_original(State& s, std::uint32_t entry);
void media_run_rewrite(State& s, std::uint32_t entry);
extern bool media_declining;
}

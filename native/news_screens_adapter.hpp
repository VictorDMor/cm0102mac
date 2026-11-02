#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Hook at the entry of each routine in news_screens_routines.inc (the original's
// NEWS_SCREENS.CPP): runs it as C++ and continues at the return address. Always true,
// except while a checker runs the original body (routine_check.hpp), for routines a check
// run leaves out (--only), or with CM_NEWS_SCREENS_ORIGINAL=1.
bool try_modern_news_screens(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine reached
// through the dispatcher, hooked by the generator or not yet.
bool news_screens_dispatch_probe(State& s);
bool news_screens_lists(std::uint32_t entry);
std::vector<std::uint32_t> news_screens_production_entries();
// For synthetic calls and the checker: one routine's original body or its C++, entry to return.
void news_screens_run_original(State& s, std::uint32_t entry);
void news_screens_run_rewrite(State& s, std::uint32_t entry);
extern bool news_screens_declining;
}

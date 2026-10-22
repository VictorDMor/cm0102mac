#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// 0x101b1620: does the list at container+8 hold an element whose word at +8
// equals *key? The recruitment candidate scan asks this for every candidate.
// Runs as plain C++ unless reads are observed or a write observer watches
// the list cursor; then, or when the original would take its error report
// (a null node or element) or fault, returns false without any effect and
// the original runs.
bool try_native_list_search(State& state);
// How many searches the C++ version has completed (for checks).
std::uint64_t native_list_search_completions();
}

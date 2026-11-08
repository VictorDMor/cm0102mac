#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// 0x101b1620: does the list at container+8 hold an element whose word at +8
// equals *key? The recruitment candidate scan asks this for every candidate.
// The fast path of MANAGER_MANAGER.CPP's owner (manager_manager_lists.cpp):
// runs as plain C++ unless reads are observed, a write observer watches the
// list cursor, a write would alias a later read, the list holds a null node or
// element, or the instruction budget is short; then it returns false without
// any effect and the owner's own search runs.
bool try_native_list_search(State& state);
// How many searches the C++ version has completed (for checks).
std::uint64_t native_list_search_completions();
}

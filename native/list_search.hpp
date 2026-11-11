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
// How many of those the index answered (for checks and timing).
std::uint64_t native_list_search_index_answers();

// The search answers from a derived index of the list (list_search_index.hpp) where the process turned it on: the
// app and cm-world-replay do; check harnesses, which write guest memory directly, keep the walk unless they turn it
// on for a case. CM_LIST_SEARCH_INDEX=0 turns it off, CM_LIST_SEARCH_VERIFY=1 checks every answer against the walk.
void set_native_list_search_index(bool enabled);
struct ListSearchIndex;
ListSearchIndex& list_search_index(State& state);
// Called by the utility-object owners for every node and list routine they run (original and C++ bodies alike).
void note_list_mutation(State& state);
}

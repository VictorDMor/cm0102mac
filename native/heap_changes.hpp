#pragma once
#include <cstdint>
#include <vector>

namespace cm::aot {
// One change to State::allocated (table 0) or State::available (table 1):
// erase the entry at `address`, or set it to `size` (insert or resize).
struct HeapChange {
    std::uint8_t table, erase;
    std::uint32_t address, size;
};
// allocate/deallocate append every heap-table change of `heap_changes_owner`
// here while a season recorder is inside one of its native imports; null
// otherwise. Other States in the process are never logged.
struct State;
extern std::vector<HeapChange>* heap_changes;
extern const State* heap_changes_owner;
}

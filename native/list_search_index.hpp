#pragma once
// The index behind the list search 0x101b1620 (list_search.cpp); private to it and runtime_diagnostics.cpp.
#include <cstdint>
#include <unordered_map>

namespace cm::aot {
// For each element key, the outcome of the original's walk (the first node holding it and its position), and the
// last node for a key that is absent. Guest memory stays the truth. The index is rebuilt when the container, its
// list header, the header's count or head differ from when it was built, or when a node or list routine of the
// utility-object owners ran on this State since (0x10738b20 node constructor, 0x10738ba0 node destructor,
// 0x10738c20 list destructor, 0x10738ca0 clear: every append, removal, clear and free goes through them; the list
// constructor 0x10738c00 is a leaf the generated code inlines, so it is not seen, but it rewrites count and head).
// The only list the search is asked about holds MANAGER_MANAGER's 0x28-byte offer records keyed by person id, and
// no code writes a linked record's key (docs/cpp-rewrites.md lists that list's writers).
struct ListSearchIndex {
    struct Hit { std::uint32_t position, node, data; };
    std::uint64_t mutations = 0;  // node and list routines run on this State
    bool built = false, usable = false;
    std::uint64_t built_at = 0;
    std::uint32_t container = 0, list = 0, head = 0;
    std::int16_t count = 0;
    std::uint64_t low = 0, high = 0;  // every address the walk reads, [low, high)
    std::uint32_t last_node = 0, last_data = 0;
    std::unordered_map<std::int32_t, Hit> first;
};
}

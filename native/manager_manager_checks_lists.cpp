// MANAGER_MANAGER.CPP checker cases for the lists block (manager_manager_lists.hpp):
// 0x101b1620. Lists are built in the scratch area (the checker undoes the writes)
// in every shape the search distinguishes; the captured days add the real calls.
#include "manager_manager_checks.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t entry = 0x101b1620;
// Layout in the scratch area: the container, its list record, the key word, then
// nodes (12 bytes) and elements (16 bytes).
constexpr std::uint32_t container = scratch_base + 0x2000, list = scratch_base + 0x2100, key = scratch_base + 0x2200,
                        nodes = scratch_base + 0x3000, elements = scratch_base + 0x4000;

struct Shape {
    std::string label;
    unsigned chain;               // nodes linked from the head
    std::int32_t count;           // the list's 16-bit count
    std::uint32_t wanted;         // *key
    int null_data = -1;           // a node whose data is 0
    std::uint32_t cursor = 0x55;  // the cursor before the call (restored on every exit)
};

void add(std::vector<Case>& cases, const Shape& shape) {
    Case item{.entry = entry, .registers = {{3, container}, {4, key}}, .label = shape.label};
    auto& m = item.memory;
    m.push_back({4, container + 8, list});
    m.push_back({2, list + 0, std::uint32_t(shape.count) & 0xffff});
    m.push_back({4, list + 4, shape.chain ? nodes : 0});
    m.push_back({4, list + 0xc, shape.cursor});
    m.push_back({4, key, shape.wanted});
    for (unsigned n = 0; n < shape.chain; ++n) {
        const auto node = nodes + n * 12, element = elements + n * 16;
        m.push_back({4, node + 0, int(n) == shape.null_data ? 0 : element});
        m.push_back({4, node + 8, n + 1 < shape.chain ? node + 12 : 0});
        m.push_back({4, element + 8, 100 + n});  // keys 100, 101, ...
    }
    cases.push_back(std::move(item));
}
}

MANAGER_MANAGER_CASES("lists") {
    add(cases, {"empty list", 0, 0, 100});
    add(cases, {"negative count", 3, -1, 100});
    for (unsigned size : {1u, 2u, 5u, 40u}) {
        const auto name = std::to_string(size) + " elements: ";
        add(cases, {name + "found first", size, std::int32_t(size), 100});
        add(cases, {name + "found last", size, std::int32_t(size), 100 + size - 1});
        if (size > 2) add(cases, {name + "found in the middle", size, std::int32_t(size), 100 + size / 2});
        add(cases, {name + "absent", size, std::int32_t(size), 7});
        add(cases, {name + "absent, negative key", size, std::int32_t(size), 0xffffff9c});
        // A count shorter than the chain stops early; a longer one reaches the end (list_next
        // answers 0) and takes the assert.
        add(cases, {name + "count short by one, last absent", size, std::int32_t(size) - 1, 100 + size - 1});
        add(cases, {name + "count past the chain", size, std::int32_t(size) + 2, 7});
        add(cases, {name + "null data first", size, std::int32_t(size), 7, 0});
        add(cases, {name + "null data last, key before it", size, std::int32_t(size), 100, int(size) - 1});
        add(cases, {name + "null data last, key absent", size, std::int32_t(size), 7, int(size) - 1});
        add(cases, {name + "cursor already null", size, std::int32_t(size), 100 + size - 1, -1, 0});
    }
    // The key word aliasing the list's cursor: the search compares with the cursor list_next moved.
    {
        Case item{.entry = entry, .registers = {{3, container}, {4, list + 0xc}}, .label = "key aliases the cursor"};
        item.memory = {{4, container + 8, list}, {2, list, 3}, {4, list + 4, nodes}, {4, list + 0xc, 0}};
        for (unsigned n = 0; n < 3; ++n) {
            item.memory.push_back({4, nodes + n * 12, elements + n * 16});
            item.memory.push_back({4, nodes + n * 12 + 8, n + 1 < 3 ? nodes + (n + 1) * 12 : 0});
            item.memory.push_back({4, elements + n * 16 + 8, n == 1 ? nodes + 2 * 12 : 1});
        }
        cases.push_back(std::move(item));
    }
}
}

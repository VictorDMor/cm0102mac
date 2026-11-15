// DISPLAY.CPP checker cases for the table block (display_table.cpp, display_table_controls.cpp): the
// display table's lifecycle, its setup buffer, the two add-control routines and their lists.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include "guest_abi.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::display_checks {
namespace {
using u32 = std::uint32_t;
constexpr u32 high = 0x130000;
constexpr u32 field(u32 table, std::int32_t displacement) { return table + high + u32(displacement); }
std::string hex(u32 value) { char text[16]; std::snprintf(text, sizeof text, "%x", value); return text; }
}

DISPLAY_CASES("table") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const u32 tables[] = {game + 0x2c68, game + 0x12d6b0};
    const auto scratch = scratch_base;
    const auto label = [](const std::string& what, unsigned index) { return what + (index ? ", second table" : ", first table"); };
    const auto freed = std::vector<std::pair<u32, u32>>{{0x107735a0, 0}, {0x10775170, 0}, {0x1076b7e0, 0}};

    // Two real heap blocks of the snapshot: the pending pointer the game keeps outside the table (TOC
    // -0x5bec) and the first table's second buffer; the game's allocator runs for real on both sides.
    const auto count_variable = scenario.toc(-0x5bf0), pointer_variable = scenario.toc(-0x5bec);
    const u32 block_a = scenario.global(-0x5bec), block_b = scenario.r32(field(tables[0], -0x56d8));
    // The element destructors' second call (the object's own delete) is answered: the objects live in the table.
    const std::vector<std::pair<u32, u32>> deleted = {{0x107735a0, 0}};

    for (unsigned t = 0; t < 2; ++t) {
        const auto table = tables[t];
        // 0x10718b40: the reset.
        for (const u32 flag : {0u, 1u, 0x1ffu})
            cases.push_back({.entry = 0x10718b40, .registers = {{3, table}, {4, flag}}, .label = label("reset " + hex(flag), t)});
        // 0x10718ba0: the destroy of the real (empty) table; the pending pointer is freed as the game has it.
        for (const u32 flag : {0u, 1u, 0xffffu, 0x10000u, 0x8000u})
            cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, flag}}, .answers = deleted,
                             .label = label("destroy flag " + hex(flag), t)});
        // Areas and GUI objects present: their destructors and the element vectors run on the (zeroed) records.
        for (const u32 counts : {0x00010000u, 0x00020003u})
            cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, 0}},
                             .memory = {{2, field(table, -0x625c), counts >> 16}, {2, field(table, -0x625a), counts & 0xffff}},
                             .answers = deleted, .label = label("destroy with children " + hex(counts), t)});
        // Its owned blocks list: {pointer array, count}, each block freed, then the array.
        for (const u32 blocks : {0u, 1u}) {
            std::vector<Write> memory = {{4, table + display::table::string_list, scratch + 0x10}, {4, scratch + 0x10, block_b}, {4, scratch + 0x14, blocks},
                                         {4, block_b, block_a}, {4, field(table, -0x56d8), 0}, {2, count_variable, 1}};
            cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, 0}}, .memory = memory, .answers = deleted,
                             .label = label("destroy with " + std::to_string(blocks) + " blocks", t)});
        }
        // Its buffers, and the pending pointer the game keeps outside the table.
        cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, 0}},
                         .memory = {{4, field(table, -0x56dc), block_a}, {4, field(table, -0x56d8), block_b}, {2, count_variable, 1}},
                         .answers = deleted, .label = label("destroy with buffers", t)});
        for (const u32 count : {0u, 1u, 0xffffu})
            cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, 1}}, .memory = {{2, count_variable, count}},
                             .answers = deleted, .label = label("destroy with pending pointer, count " + hex(count), t)});
        cases.push_back({.entry = 0x10718ba0, .registers = {{3, table}, {4, 0}}, .memory = {{4, pointer_variable, 0}},
                         .answers = deleted, .label = label("destroy without pending pointer", t)});

        // 0x10718d80 / 0x10718dd0: the element destructors, on the table's first area and first GUI object.
        for (const u32 flag : {0u, 1u, 0xffffu, 0x8000u}) {
            cases.push_back({.entry = 0x10718d80, .registers = {{3, table + 4}, {4, flag}}, .answers = deleted,
                             .label = label("area destructor " + hex(flag), t)});
            cases.push_back({.entry = 0x10718dd0, .registers = {{3, table + 0xba864}, {4, flag}}, .answers = deleted,
                             .label = label("GUI object destructor " + hex(flag), t)});
        }
        cases.push_back({.entry = 0x10718d80, .registers = {{3, 0}, {4, 1}}, .label = label("area destructor, null", t)});
        cases.push_back({.entry = 0x10718dd0, .registers = {{3, 0}, {4, 1}}, .label = label("GUI object destructor, null", t)});

        // 0x10719560: clearing the table in use (a real stream: buffer, position 1, capacity 0xc00).
        for (const u32 children : {0u, 1u, 0x100u})
            for (const u32 busy : {0u, 1u})
                cases.push_back({.entry = 0x10719560, .registers = {{3, table}, {4, children}},
                                 .memory = {{1, table + display::table::screen_set, busy}, {2, field(table, -0x625c), 2}, {2, field(table, -0x625a), 2}},
                                 .label = label("clear, children " + hex(children) + ", -0x56bc " + std::to_string(busy), t)});
    }
    cases.push_back({.entry = 0x10718ba0, .registers = {{3, 0}, {4, 1}}, .label = "destroy no table"});

    // The stream the table writes its commands to (TOC -0x5c08: buffer, position, capacity).
    const auto stream = scenario.toc(-0x5c08);
    for (const u32 capacity : {0u, 1u, 0x1000u}) {
        cases.push_back({.entry = 0x10719560, .registers = {{3, tables[0]}, {4, 0}},
                         .memory = {{4, stream, scratch + 0x2000}, {4, stream + 8, capacity}},
                         .label = "clear, stream capacity " + hex(capacity)});
    }
    cases.push_back({.entry = 0x10719560, .registers = {{3, tables[0]}, {4, 0}},
                     .memory = {{4, stream, scratch + 0x2000}, {4, stream + 8, 0x1000}, {4, field(tables[0], -0x56dc), block_a}},
                     .label = "clear with a buffer"});

    // 0x10718e20 / 0x10718ef0 / 0x10718f30: the shared setup buffer (null in the snapshot: the allocator runs).
    const auto variable = scenario.toc(-0x5bfc);
    cases.push_back({.entry = 0x10718e20, .registers = {{4, 5}}, .label = "setup buffer allocated"});
    for (const u32 count : {1u, 0x10003u, 0xffffu})
        cases.push_back({.entry = 0x10718e20, .registers = {{4, count}}, .label = "setup buffer allocated, count " + hex(count)});
    cases.push_back({.entry = 0x10718e20, .registers = {{4, 5}}, .memory = {{4, variable, scratch + 0x800}},
                     .label = "setup buffer already there"});
    // A count the heap cannot satisfy: the allocation fails, reports and exits.
    cases.push_back({.entry = 0x10718e20, .registers = {{4, 0x7fff}}, .label = "setup buffer allocation fails"});
    cases.push_back({.entry = 0x10718ef0, .label = "free setup buffer, none"});
    cases.push_back({.entry = 0x10718ef0, .memory = {{4, variable, block_a}}, .label = "free setup buffer, set"});
    cases.push_back({.entry = 0x10718f30, .registers = {{4, 3}}, .label = "clear record, no buffer"});
    for (const u32 index : {0u, 1u})
        cases.push_back({.entry = 0x10718f30, .registers = {{4, index}}, .memory = {{4, variable, scratch + 0x100}},
                         .label = "clear record " + std::to_string(index)});

    // 0x1071a100 / 0x1071a1c0: the control / area index lists (1200 and 250 entries).
    for (unsigned t = 0; t < 2; ++t) {
        const auto table = tables[t];
        for (const u32 index : {0u, 5u, 0xffffu, 0x1ffffu})
            for (const u32 count : {0u, 7u, 0x4afu, 0x4b0u, 0xffffu}) {
                cases.push_back({.entry = 0x1071a100, .registers = {{3, table}, {4, index}}, .memory = {{2, table + display::table::guio_limit, count}},
                                 .label = label("control index " + hex(index) + ", list count " + hex(count), t)});
            }
        for (const u32 index : {0u, 5u, 0xffffu, 0x1ffffu})
            for (const u32 count : {0u, 7u, 0xf9u, 0xfau, 0xffffu}) {
                cases.push_back({.entry = 0x1071a1c0, .registers = {{3, table}, {4, index}}, .memory = {{2, field(table, -0x5704), count}},
                                 .label = label("area index " + hex(index) + ", list count " + hex(count), t)});
            }
    }

    // The caller's parameter area (entry sp + 0x38...) holds the arguments past the eighth.
    const u32 sp = u32(scenario.s.r<4>(4));
    const auto words = [&](std::initializer_list<u32> values) {
        std::vector<Write> memory;
        u32 offset = 0x38;
        for (const auto value : values) memory.push_back({4, sp + offset, value}), offset += 4;
        return memory;
    };
    const auto text = scratch + 0x100, other_text = scratch + 0x180;
    const std::vector<Write> strings = {{4, text, 0x41424344}, {4, text + 4, 0x45464748}, {4, other_text, 0x31323334}};
    const auto join = [](std::vector<Write> first, const std::vector<Write>& second) {
        first.insert(first.end(), second.begin(), second.end());
        return first;
    };

    // 0x10719740: add a control (the words: 0x38 .. 0x60 as the game's callers store them).
    for (unsigned t = 0; t < 2; ++t) {
        const auto table = tables[t];
        // Area 0 is a real record on the first table only (the second one's areas are unset: a null deref).
        for (const u32 area : {0xffffffffu, 0u})
            for (const u32 guios : {0u, 0x4aeu, 0x4afu, 0x4b0u, 0xffffu}) {
                if (area == 0 && t == 1) continue;
                auto memory = join(words({0x10, 5, 6, 1, 1, 7, text, 0, 0x22, 2, area}), strings);
                memory.push_back({2, field(table, -0x625a), guios});
                cases.push_back({.entry = 0x10719740, .registers = {{3, table}, {4, 2}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}},
                                 .memory = memory, .label = label("add control, area " + hex(area) + ", GUI count " + hex(guios), t)});
            }
        // The control index list full (area -1): the index list's assertion.
        auto full = join(words({0x10, 5, 6, 1, 1, 7, text, 0, 0x22, 2, 0xffffffffu}), strings);
        full.push_back({2, table + display::table::guio_limit, 0x4b0});
        cases.push_back({.entry = 0x10719740, .registers = {{3, table}, {4, 2}, {5, 3}, {6, 4}, {7, 5}, {8, 6}, {9, 7}, {10, 8}},
                         .memory = full, .label = label("add control, index list full", t)});
        // A count of -1: the new index is -1, which the list skips.
        auto negative = join(words({0x10, 5, 6, 1, 1, 7, text, 0, 0x22, 2, 0xffffffffu}), strings);
        negative.push_back({2, field(table, -0x625a), 0xffff});
        cases.push_back({.entry = 0x10719740, .registers = {{3, table}, {4, 2}, {5, 3}, {6, 4}, {7, 5}, {8, 6}, {9, 7}, {10, 8}},
                         .memory = negative, .label = label("add control, count -1", t)});
    }

    // 0x107198c0: add a grid control. The stream's capacity decides which of the thirteen checked
    // writes report: with the buffer in the scratch area, capacities 0..32 reach every site.
    for (unsigned t = 0; t < 2; ++t) {
        const auto table = tables[t];
        struct Variant { const char* name; u32 text, other; };
        for (const Variant variant : {Variant{"both texts", text, other_text}, Variant{"no first text", 0, other_text},
                                      Variant{"no second text", text, 0}, Variant{"no texts", 0, 0}}) {
            for (u32 capacity = 0; capacity <= 34; ++capacity) {
                if (t == 1 && capacity % 3 != 0) continue;
                if (variant.text == 0 && variant.other == 0 && capacity > 24) continue;
                auto memory = join(words({variant.other, 0x11223344, 0x55667788, 0x99aabbcc, 0}), strings);
                memory.insert(memory.end(), {{4, stream, scratch + 0x2000}, {4, stream + 4, 0}, {4, stream + 8, capacity}});
                cases.push_back({.entry = 0x107198c0,
                                 .registers = {{3, table}, {4, 0x1234}, {5, 0x2345}, {6, 0x3456}, {7, 0x4567}, {8, 0x2004}, {9, variant.text}, {10, 0x1103}},
                                 .memory = memory,
                                 .label = label(std::string("add grid control, ") + variant.name + ", capacity " + std::to_string(capacity), t)});
            }
        }
        for (const u32 areas : {0xf8u, 0xf9u, 0xfau, 0xffffu}) {
            auto memory = join(words({other_text, 1, 2, 3, 0}), strings);
            memory.insert(memory.end(), {{4, stream, scratch + 0x2000}, {4, stream + 4, 0}, {4, stream + 8, 0x1000}, {2, field(table, -0x625c), areas}});
            cases.push_back({.entry = 0x107198c0, .registers = {{3, table}, {4, 1}, {5, 2}, {6, 3}, {7, 4}, {8, 0x4}, {9, text}, {10, 3}},
                             .memory = memory, .label = label("add grid control, area count " + hex(areas), t)});
        }
        for (const u32 list : {0xf9u, 0xfau}) {
            auto memory = join(words({other_text, 1, 2, 3, 0}), strings);
            memory.insert(memory.end(), {{4, stream, scratch + 0x2000}, {4, stream + 4, 0}, {4, stream + 8, 0x1000}, {2, field(table, -0x5704), list}});
            cases.push_back({.entry = 0x107198c0, .registers = {{3, table}, {4, 1}, {5, 2}, {6, 3}, {7, 4}, {8, 0x4}, {9, text}, {10, 3}},
                             .memory = memory, .label = label("add grid control, area list count " + hex(list), t)});
        }
    }

    // 0x1071c070: the corner anchor of the rectangle at the pointer in TOC -0x5c2c.
    const auto bounds_variable = scenario.toc(-0x5c2c);
    const auto bounds = scratch + 0x4000;
    const std::vector<Write> bounds_memory = {{4, bounds_variable, bounds}, {4, bounds, 0xfffffff9u}, {4, bounds + 4, 10}, {4, bounds + 8, 2},
                                              {4, bounds + 0x10, 20}, {4, bounds + 0x14, 30}, {4, bounds + 0x28, 40}};
    for (const u32 corner : {0u, 1u, 2u, 3u, 4u, 0xffu, 0x100u, 0x102u}) {
        cases.push_back({.entry = 0x1071c070, .registers = {{4, corner}, {5, 0x1234}}, .memory = bounds_memory,
                         .label = "anchor corner " + hex(corner)});
    }
    auto positive = bounds_memory;
    positive[1] = {4, bounds, 5};
    cases.push_back({.entry = 0x1071c070, .registers = {{4, 1}, {5, 7}}, .memory = positive, .label = "anchor positive sum"});
    cases.push_back({.entry = 0x1071c070, .registers = {{4, 1}, {5, 7}}, .memory = {{4, bounds_variable, 0}}, .label = "anchor no bounds"});

    // 0x107259a0: the static initializer.
    cases.push_back({.entry = 0x107259a0, .label = "static initializer"});
}
}

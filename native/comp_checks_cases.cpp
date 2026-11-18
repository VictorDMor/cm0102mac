// COMP.CPP's pilot cases, from the real career (--world): the game's own competition objects (one per
// competition id, toc -0x7b5c), their stages and the nation descriptors. Objects a case needs that the career
// has no instance of (a fresh object, rows for the comparators, a holder naming another nation) are built in
// the scratch area from real values with one field changed.
#include "comp_checks.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::comp_checks {
namespace {
constexpr std::uint32_t construct = 0x1044fed0, destroy = 0x1044ff60, stage = 0x10450b60, descriptor_code = 0x10451a30,
                        compare_by_value = 0x104532c0, compare_by_column = 0x10453300;
// The base class's one-instruction virtuals.
constexpr std::uint32_t defaults[] = {0x1044ffe0, 0x1044fff0, 0x10450000, 0x10450010, 0x10450020, 0x10450700,
                                      0x104509c0, 0x104509d0, 0x104509e0, 0x104509f0, 0x10450a00, 0x10450a10,
                                      0x10450a20, 0x10450a30, 0x10450a40, 0x10451c90, 0x10451d00, 0x10453370,
                                      0x10453380, 0x10453390};
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, descriptors_slot = -0x7b44,
                       sort_column_slot = -0x61a4;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
struct World {
    const Scenario& s;
    std::vector<std::uint32_t> objects;  // every competition object the career has
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id)
            if (const auto object = s.r32(table + id * 4)) objects.push_back(object);
    }
    std::int32_t last_stage(std::uint32_t object) const { return std::int32_t(s.r32(object + 0x30)); }
};
// A copy of a real object's first 0x50 bytes in the scratch area.
std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < 0x50; offset += 4) writes.push_back({4, at + offset, s.r32(object + offset)});
    return writes;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

COMP_CASES("object") {
    if (scenario.match) return;
    const World w(scenario);
    const auto fresh = scratch_base + 0x100;
    // A fresh object: the scratch bytes it does not write keep a pattern, so a stray store shows.
    std::vector<Write> pattern;
    for (std::uint32_t offset = 0; offset < 0x60; offset += 4) pattern.push_back({4, fresh + offset, 0xa5a5a5a5u + offset});
    cases.push_back({.entry = construct, .registers = {{3, fresh}}, .memory = pattern, .label = "object: construct fresh"});
    for (std::size_t n = 0; n < w.objects.size() && n < 6; ++n)
        cases.push_back({.entry = construct, .registers = {{3, w.objects[n]}}, .label = "object: construct over " + hex(w.objects[n])});
    // The destructor: null; a copy without blocks under each flag word; real objects with their blocks.
    cases.push_back({.entry = destroy, .registers = {{3, 0}, {4, 1}}, .label = "object: destroy null"});
    if (!w.objects.empty()) {
        auto empty = object_copy(scenario, fresh, w.objects.front());
        empty.push_back({4, fresh + 0x14, 0});
        empty.push_back({4, fresh + 0x18, 0});
        for (const auto flag : {0u, 0x10000u, 0xffffu, 0x8000u})
            cases.push_back({.entry = destroy, .registers = {{3, fresh}, {4, flag}}, .memory = empty,
                             .label = "object: destroy without blocks, flag " + hex(flag)});
    }
    std::size_t with_blocks = 0;
    for (const auto object : w.objects) {
        const bool first = scenario.r32(object + 0x14) != 0, second = scenario.r32(object + 0x18) != 0;
        if (!first && !second && with_blocks > 2) continue;
        for (const auto flag : {0u, 1u})
            cases.push_back({.entry = destroy, .registers = {{3, object}, {4, flag}},
                             .label = "object: destroy " + hex(object) + (first ? " +14" : "") + (second ? " +18" : "") + " flag " + hex(flag)});
        if (++with_blocks > 8) break;
    }
    // The virtual defaults, on real objects.
    for (const auto entry : defaults)
        for (std::size_t n = 0; n < w.objects.size() && n < 2; ++n)
            cases.push_back({.entry = entry, .registers = {{3, w.objects[n]}, {4, 1}}, .label = "object: default " + hex(entry)});
}

COMP_CASES("stage") {
    if (scenario.match) return;
    const World w(scenario);
    std::size_t staged = 0;
    for (std::size_t n = 0; n < w.objects.size(); ++n) {
        const auto object = w.objects[n];
        const auto last = w.last_stage(object);
        // Every object at -1 and past its end; those with stages at each stage and below -1.
        cases.push_back({.entry = stage, .registers = {{3, object}, {4, 0xffffffffu}}, .label = "stage: self of " + hex(object)});
        if (n < 12) cases.push_back({.entry = stage, .registers = {{3, object}, {4, std::uint32_t(last + 1)}}, .label = "stage: past the last of " + hex(object)});
        if (last < 0 || staged++ > 10) continue;
        for (std::int32_t index = 0; index <= last && index < 8; ++index)
            cases.push_back({.entry = stage, .registers = {{3, object}, {4, std::uint32_t(index)}}, .label = "stage: " + std::to_string(index) + " of " + hex(object)});
        cases.push_back({.entry = stage, .registers = {{3, object}, {4, std::uint32_t(last)}}, .label = "stage: last of " + hex(object)});
        cases.push_back({.entry = stage, .registers = {{3, object}, {4, 0xfffffffeu}}, .label = "stage: -2 of " + hex(object)});
        cases.push_back({.entry = stage, .registers = {{3, object}, {4, 0x80000000u}}, .label = "stage: most negative of " + hex(object)});
        cases.push_back({.entry = stage, .registers = {{3, object}, {4, 0x7fffffffu}}, .label = "stage: largest of " + hex(object)});
    }
}

COMP_CASES("regions") {
    if (scenario.match) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.objects.size(); ++n)
        if (n < 40 || scenario.r32(w.objects[n] + 0x10)) cases.push_back({.entry = descriptor_code, .registers = {{3, w.objects[n]}}, .label = "regions: object " + hex(w.objects[n])});
    cases.push_back({.entry = descriptor_code, .registers = {{3, 0}}, .label = "regions: null"});
    if (w.objects.empty()) return;
    // A copy of a real object whose holder is a scratch row naming each descriptor's nation in turn,
    // then the same nation with its loaded byte cleared, then no nation and no holder.
    const auto object = scratch_base + 0x200, holder = scratch_base + 0x300;
    const auto base = joined(object_copy(scenario, object, w.objects.front()), {{4, object + 0x10, holder}});
    const auto rows = scenario.toc(descriptors_slot);
    for (std::uint32_t n = 0; n < 34; ++n) {
        const auto nation = scenario.r32(rows + n * 0x4c);
        cases.push_back({.entry = descriptor_code, .registers = {{3, object}}, .memory = joined(base, {{4, holder + 0x18, nation}}),
                         .label = "regions: descriptor " + std::to_string(n) + " nation " + hex(nation)});
        if (nation && n < 6)
            cases.push_back({.entry = descriptor_code, .registers = {{3, object}},
                             .memory = joined(base, {{4, holder + 0x18, nation}, {1, nation + 0x120, 0}}),
                             .label = "regions: descriptor " + std::to_string(n) + " not loaded"});
    }
    cases.push_back({.entry = descriptor_code, .registers = {{3, object}}, .memory = joined(base, {{4, holder + 0x18, 0}}), .label = "regions: holder without nation"});
    cases.push_back({.entry = descriptor_code, .registers = {{3, object}}, .memory = joined(base, {{4, holder + 0x18, object}}), .label = "regions: unknown nation"});
    cases.push_back({.entry = descriptor_code, .registers = {{3, object}}, .memory = joined(base, {{4, object + 0x10, 0}}), .label = "regions: no holder"});
}

COMP_CASES("order") {
    if (scenario.match) return;
    const auto left = scratch_base + 0x400, right = scratch_base + 0x440;
    const auto column_cell = scenario.toc(sort_column_slot);
    // Values: larger, smaller, equal, both signs of zero, NaN on either side, infinities.
    constexpr std::uint32_t nan = 0x7fc00000u, snan = 0x7fa00000u, infinity = 0x7f800000u, minus_zero = 0x80000000u;
    const std::pair<std::uint32_t, std::uint32_t> values[] = {
        {0x40400000u, 0x40000000u}, {0x40000000u, 0x40400000u}, {0x3f800000u, 0x3f800000u}, {0, minus_zero},
        {nan, 0x3f800000u}, {0x3f800000u, nan}, {snan, snan}, {infinity, 0xff800000u}, {0xbf800000u, 0x3f000000u}};
    const std::pair<std::uint32_t, std::uint32_t> ids[] = {{7, 3}, {3, 7}, {0x80000000u, 1}, {5, 5}};
    for (const auto& [a, b] : values)
        for (const auto& [id_a, id_b] : ids)
            cases.push_back({.entry = compare_by_value, .registers = {{3, left}, {4, right}},
                             .memory = {{4, left, id_a}, {4, left + 4, a}, {4, right, id_b}, {4, right + 4, b}},
                             .label = "order: value " + hex(a) + " " + hex(b) + " ids " + hex(id_a) + " " + hex(id_b)});
    const std::pair<std::uint32_t, std::uint32_t> halves[] = {{30, 20}, {20, 30}, {12, 12}, {0xffff, 1}, {0x8000, 0x7fff}};
    for (const auto column : {0u, 1u, 2u, 3u, 4u, 0xffu, 0x80u, 0x7fu})
        for (const auto& [a, b] : halves)
            for (const auto& [id_a, id_b] : ids) {
                std::vector<Write> memory = {{1, column_cell, column}, {4, left, id_a}, {4, right, id_b}};
                // Every column holds a different pair, so reading the wrong one shows.
                for (std::uint32_t k = 0; k < 4; ++k) {
                    memory.push_back({2, left + 0xa + k * 4, k == (column & 3) ? a : 100 + k});
                    memory.push_back({2, right + 0xa + k * 4, k == (column & 3) ? b : 200 - k});
                }
                cases.push_back({.entry = compare_by_column, .registers = {{3, left}, {4, right}}, .memory = memory,
                                 .label = "order: column " + hex(column) + " " + hex(a) + " " + hex(b) + " ids " + hex(id_a) + " " + hex(id_b)});
            }
}
}
}

// COMP_UTIL.CPP, block `comp_screens_list`: cases for 0x10477490, the sorted list of a COMP_SCREENS record.
//
// The list is a linked-list utility object of NETWORK.CPP whose nodes and row copies live on the game's heap,
// and clearing it frees them, so a case cannot fabricate one in the scratch area: the lists are built here with
// the game's own routines (0x10775130 for the object, 0x10738d10 to append a row, as the game does when it fills
// the list) on the live state before the cases run. The rows themselves are staged in the scratch area (this
// block's area is scratch_base + 0x80000..0x82000): clubs of the career at +0x1c (the names the last key
// compares), the keys at +0x30, +0x32, +0x34 and +0x42, and a pattern in every other byte so that the 0x50-byte
// copy is checked whole. Key ranges are narrow in most lists so that ties reach the name comparison and the sort's
// tie behaviour. The assertion paths: no list (r3 = 0), a list with no rows, and a failed allocation of the copy
// (answered on both sides; the allocator entries run their originals for that case).
// Labels start with "comp_screens_list:" (CM_COMP_UTIL_BLOCK=comp_screens_list).
#include "comp_util_checks.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t sort_list = 0x10477490, allocate = 0x10775130, append_row = 0x10738d10;
constexpr std::int32_t clubs_slot = -0x7be8, club_count_slot = -0x7b50;
constexpr std::uint32_t stage = scratch_base + 0x80000;

std::uint32_t guest_call(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    guest::Cpu c(s);
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    s.budget = s.executed + 400'000'000;
    c.call(entry, return_sentinel);
    return c.r(3);
}
}

COMP_UTIL_CASES("comp_screens_list") {
    State& s = scenario.s;
    const auto base = scenario.global(clubs_slot);
    const auto total = scenario.global(club_count_slot);
    std::mt19937 random(20261122);
    const auto saved_observer = s.memory.observer;
    s.memory.observer = nullptr;
    const auto saved_registers = s.registers;
    const auto new_list = [&] {
        const auto object = guest_call(s, allocate, {0x10});
        if (object == 0) throw std::runtime_error("comp_screens_list: the heap gave no list object");
        for (std::uint32_t at = 0; at < 0x10; at += 4) s.memory.write<4>(object + at, 0);
        return object;
    };
    // `rows`: how many; `range`: the width of each key's value range (1: all equal); `names`: how many distinct
    // clubs the rows draw from (1: the same club every time).
    const auto build = [&](std::uint32_t rows, std::uint32_t range, std::uint32_t names) {
        const auto object = new_list();
        std::vector<std::uint32_t> clubs;
        for (std::uint32_t n = 0; n < names; ++n) clubs.push_back(base + (random() % total) * 0x248);
        for (std::uint32_t row = 0; row < rows; ++row) {
            for (std::uint32_t at = 0; at < 0x50; ++at) s.memory.write<1>(stage + at, (row * 7 + at * 13 + 0x21) & 0xff);
            s.memory.write<4>(stage + 0x1c, clubs[random() % names]);
            const auto key = [&](std::uint32_t wide) { return range == 1 ? 0u : std::uint32_t(random() % range) * wide; };
            s.memory.write<1>(stage + 0x42, range > 8 ? (random() % 3 == 0 ? 0xffu : key(1)) : key(1));
            s.memory.write<2>(stage + 0x32, range > 8 && random() % 4 == 0 ? 0x8000 + key(1) : key(1));
            s.memory.write<2>(stage + 0x34, key(1));
            s.memory.write<2>(stage + 0x30, range > 8 && random() % 4 == 0 ? 0xfff0 + key(1) : key(1));
            guest_call(s, append_row, {object, stage, 0x50});
        }
        return object;
    };
    struct Spec { std::uint32_t rows, range, names; const char* what; };
    const Spec specs[] = {
        {1, 1, 1, "one row"}, {2, 1, 1, "two equal rows"}, {2, 3, 2, "two rows"}, {3, 1, 3, "three rows, names decide"}, {3, 3, 3, "three rows"},
        {4, 2, 4, "four rows"}, {5, 1, 5, "five rows, names decide"}, {5, 3, 2, "five rows, two clubs"}, {6, 2, 6, "six rows"}, {8, 1, 1, "eight identical rows"},
        {8, 2, 3, "eight rows"}, {9, 4, 9, "nine rows"}, {12, 2, 12, "twelve rows"}, {12, 20, 12, "twelve rows, wide keys"}, {16, 3, 16, "sixteen rows"},
        {20, 2, 20, "twenty rows, narrow keys"}, {20, 20, 20, "twenty rows, wide keys"}, {27, 1, 27, "twenty-seven rows, names decide"}, {33, 6, 8, "thirty-three rows"},
    };
    std::vector<std::pair<std::uint32_t, std::string>> lists;
    for (const auto& spec : specs)
        for (unsigned round = 0; round < (spec.rows <= 12 ? 3u : 2u); ++round)
            lists.emplace_back(build(spec.rows, spec.range, spec.names), std::string(spec.what) + " #" + std::to_string(round));
    const auto empty = new_list();
    const auto failing = build(3, 3, 3);
    s.registers = saved_registers;
    s.memory.observer = saved_observer;
    for (const auto& [object, what] : lists)
        cases.push_back({.entry = sort_list, .registers = {{3, object}}, .label = "comp_screens_list: " + what});
    cases.push_back({.entry = sort_list, .registers = {{3, 0}}, .label = "comp_screens_list: no list"});
    cases.push_back({.entry = sort_list, .registers = {{3, empty}}, .label = "comp_screens_list: a list with no rows"});
    cases.push_back({.entry = sort_list, .registers = {{3, failing}}, .answers = {{allocate, 0}}, .label = "comp_screens_list: allocation fails",
                     .original_owners = {allocate}});
}
}

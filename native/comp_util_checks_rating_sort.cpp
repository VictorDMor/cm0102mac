// COMP_UTIL.CPP, block `rating_sort`: cases for the club rating (0x10477870) and the sort by it (0x10477b40).
//
// The career's real clubs, members and finance manager are the starting point (a captured state: season-day).
// Each case edits the real state in the case's own memory (undone afterwards): the finance manager cell
// (TOC -0x7ab8) zeroed to take the "no manager" path, the club's word +4 and reputation at the edges of the
// divisions and the halfword, and the club's account balance (the account the real lookup 0x10103510 returns:
// manager +0 holds the accounts, one 0x168-byte row per club index at club +0) at the edges of the signed
// 64-bit division by 300. The sort's arrays are built in the scratch area (this block's area is
// scratch_base + 0x70000..0x78000) from real club pointers: none, one, two, several, empty slots, the same
// club twice, sorted and reverse-sorted by rating, a long one, and a failed allocation answered on both sides.
// Labels start with "rating_sort:" (CM_COMP_UTIL_BLOCK=rating_sort).
#include "comp_util_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t strength = 0x10477870, sort = 0x10477b40;
constexpr std::int32_t clubs_slot = -0x7be8, club_count_slot = -0x7b50, manager_slot = -0x7ab8;
constexpr std::uint32_t allocate = 0x10775130;
constexpr std::uint32_t area = scratch_base + 0x70000, array_at = area, members_at = area + 0x1000;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
}

COMP_UTIL_CASES("rating_sort") {
    const auto base = scenario.global(clubs_slot);
    const auto total = scenario.global(club_count_slot);
    const auto manager_cell = scenario.toc(manager_slot);
    const auto manager = scenario.r32(manager_cell);
    std::fprintf(stderr, "rating_sort: %u clubs, finance manager %x\n", total, manager);
    if (total < 40 || manager == 0) throw std::runtime_error("rating_sort: the career needs clubs and a finance manager");
    std::vector<std::uint32_t> real;  // a spread of the career's clubs
    for (std::uint32_t n = 0; n < 60; ++n) real.push_back(base + (n * (total / 60)) * 0x248);
    // The rating of no club, and of the real clubs with the finance manager as it is.
    cases.push_back({.entry = strength, .registers = {{3, 0}}, .label = "rating_sort: strength of no club"});
    cases.push_back({.entry = strength, .registers = {{3, 0}}, .memory = {w32(manager_cell, 0)}, .label = "rating_sort: strength of no club, no manager"});
    for (const auto club : real)
        cases.push_back({.entry = strength, .registers = {{3, club}}, .label = "rating_sort: strength real club " + hex(club)});
    // Without a finance manager: the club's word +4 / 400 at the edges of the division (truncated towards zero)
    // and the extremes, with the reputation (halfword, times 16) beside it.
    const auto club = real[7];
    const std::int32_t words[] = {0, 1, 199, 200, 399, 400, 401, 799, 800, 801, 1200, 123456, 400 * 5000, 400 * 5000 - 1, 0x7fffffff, 0x7ffffe70, 0x7ffffe6f,
                                  -1, -199, -200, -399, -400, -401, -799, -800, -801, -1200, -123456, -400 * 5000, int(0x80000000), int(0x80000001), int(0x8000018f), int(0x80000190)};
    for (const auto word : words)
        for (const std::uint32_t reputation : {0u, 0x7fffu, 0x8000u})
            cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = {w32(manager_cell, 0), w32(club + 4, std::uint32_t(word)), w16(club + 0x1ec, reputation)},
                             .label = "rating_sort: no manager, word " + hex(std::uint32_t(word)) + " reputation " + hex(reputation)});
    // The account's balance (a signed 64-bit word pair) divided by 300 through the compiler's quotient, at the
    // edges (the quotient's low word is what counts), on a real club with its real members.
    // The account the real lookup returns for this club (0x10103510: the manager's accounts + the club's index * 0x168).
    const auto account = scenario.r32(manager) + scenario.r32(club) * 0x168;
    const std::uint64_t balances[] = {0, 1, 299, 300, 301, 599, 600, 5'000'000, 5'000'001, 0xffffffffull, 0x100000000ull, 0x12345678abcdefull, 0x7fffffffffffffffull,
                                      0x7ffffffffffffe00ull, std::uint64_t(-1), std::uint64_t(-299), std::uint64_t(-300), std::uint64_t(-301), std::uint64_t(-5'000'000),
                                      std::uint64_t(-0x100000000ll), 0x8000000000000000ull, 0x8000000000000001ull, 0xffffffff00000000ull, 0xfffffffeffffffffull};
    for (const auto balance : balances)
        cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = {w32(account, std::uint32_t(balance >> 32)), w32(account + 4, std::uint32_t(balance))},
                         .label = "rating_sort: balance " + hex(std::uint32_t(balance >> 32)) + hex(std::uint32_t(balance))});
    // The lookup finds no account: the manager's accounts base is moved so that base + index * 0x168 wraps to 0
    // (0x10103510 is a C++ owner that runs in place, so a case cannot answer it; the memory is what it computes from).
    cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = {w32(manager, 0u - scenario.r32(club) * 0x168)},
                     .label = "rating_sort: no account"});
    // The club has no members: the 50-word list at +0x98 (what 0x100d06f0 copies out) is empty, so the place
    // chooser gets none and picks no one.
    std::vector<Write> no_members;
    for (std::uint32_t n = 0; n < 50; ++n) no_members.push_back(w32(club + 0x98 + n * 4, 0));
    cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = no_members, .label = "rating_sort: no members"});
    {
        auto both = no_members;
        both.push_back(w32(manager, 0u - scenario.r32(club) * 0x168));
        cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = both, .label = "rating_sort: no account, no members"});
    }
    // Members without player data (+0x14 of the person record zero: staff, not players) are left out of the places'
    // pool: the club's member list is read with the game's own routine (0x100d06f0, run on the live state into the
    // scratch area), then the first member's, and every member's, +0x14 is zeroed in a case's memory.
    {
        State& s = scenario.s;
        const auto saved_observer = s.memory.observer;
        const auto saved_registers = s.registers;
        s.memory.observer = nullptr;
        guest::Cpu c(s);
        std::vector<Write> staff_first, staff_all;
        std::uint32_t staffed = 0;  // a real club that lists members
        for (const auto candidate : real) {
            c.set({{3, candidate}, {4, members_at}, {5, 0}});
            s.budget = s.executed + 400'000'000;
            c.call(0x100d06f0, return_sentinel);
            for (std::uint32_t n = 0; n < 50; ++n)
                if (const auto member = scenario.r32(members_at + n * 4); member != 0) {
                    if (staff_first.empty()) staff_first.push_back(w32(member + 0x14, 0));
                    staff_all.push_back(w32(member + 0x14, 0));
                }
            if (!staff_all.empty()) { staffed = candidate; break; }
        }
        s.registers = saved_registers;
        s.memory.observer = saved_observer;
        if (staff_all.empty()) throw std::runtime_error("rating_sort: the club has no members to edit");
        cases.push_back({.entry = strength, .registers = {{3, staffed}}, .memory = staff_first, .label = "rating_sort: first member without player data"});
        cases.push_back({.entry = strength, .registers = {{3, staffed}}, .memory = staff_all, .label = "rating_sort: no member has player data"});
    }
    // Reputation at the halfword edges with the real manager.
    for (const std::uint32_t reputation : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 5000u})
        cases.push_back({.entry = strength, .registers = {{3, club}}, .memory = {w16(club + 0x1ec, reputation)}, .label = "rating_sort: real manager, reputation " + hex(reputation)});
}

COMP_UTIL_CASES("rating_sort") {
    // 0x10477b40: arrays of real club pointers in the scratch area. Ratings are the real ones (a club's money, its
    // reputation and its squad), so the order comes from the career; the arrays differ in length, in empty slots,
    // in repeated clubs (equal ratings keep their order) and in how they start (the order of the career's table,
    // its reverse).
    const auto base = scenario.global(clubs_slot);
    const auto total = scenario.global(club_count_slot);
    std::mt19937 random(20261121);
    const auto club_at = [&](std::uint32_t n) { return base + (n % total) * 0x248; };
    const auto array = [&](const std::vector<std::uint32_t>& clubs) {
        std::vector<Write> memory;
        for (std::size_t n = 0; n < clubs.size(); ++n) memory.push_back(w32(array_at + std::uint32_t(n) * 4, clubs[n]));
        return memory;
    };
    const auto add = [&](const std::vector<std::uint32_t>& clubs, const std::string& what, std::uint32_t count) {
        cases.push_back({.entry = sort, .registers = {{3, array_at}, {4, count}}, .memory = array(clubs), .label = "rating_sort: sort " + what});
    };
    add({}, "no clubs", 0);
    add({club_at(3)}, "one club", 1);
    add({0}, "one empty slot", 1);
    add({club_at(3), club_at(9)}, "two clubs", 2);
    add({club_at(9), club_at(3)}, "two clubs reversed", 2);
    add({club_at(3), club_at(3)}, "one club twice", 2);
    add({0, club_at(5)}, "empty then club", 2);
    add({club_at(5), 0}, "club then empty", 2);
    add({0, 0, 0}, "empty slots", 3);
    for (std::uint32_t size : {3u, 4u, 5u, 6u, 8u, 12u}) {
        std::vector<std::uint32_t> clubs;
        for (std::uint32_t n = 0; n < size; ++n) clubs.push_back(club_at(n * (total / 13) + 1));
        add(clubs, "ordered " + std::to_string(size), size);
        std::reverse(clubs.begin(), clubs.end());
        add(clubs, "reversed " + std::to_string(size), size);
        for (unsigned round = 0; round < 3; ++round) {
            std::shuffle(clubs.begin(), clubs.end(), random);
            add(clubs, "shuffled " + std::to_string(size) + "/" + std::to_string(round), size);
            auto with_gaps = clubs;
            with_gaps[random() % size] = 0;
            if (size > 4) with_gaps[random() % size] = clubs[random() % size];
            add(with_gaps, "shuffled with gaps " + std::to_string(size) + "/" + std::to_string(round), size);
        }
    }
    std::vector<std::uint32_t> long_row;
    for (std::uint32_t n = 0; n < 40; ++n) long_row.push_back(club_at(n * (total / 41) + 2));
    std::shuffle(long_row.begin(), long_row.end(), random);
    add(long_row, "forty clubs", 40);
    // A count shorter than the array leaves the tail alone.
    add({club_at(1), club_at(8), club_at(4), club_at(20)}, "count 3 of 4", 3);
    // The rating buffer's allocation fails: the game's assertion (line 0x292b), the array untouched. The allocator
    // entries run their originals for this case, so the answer applies on both sides.
    cases.push_back({.entry = sort, .registers = {{3, array_at}, {4, 3}}, .memory = array({club_at(2), club_at(7), club_at(5)}),
                     .answers = {{allocate, 0}}, .label = "rating_sort: sort allocation fails", .original_owners = {allocate}});
}
}

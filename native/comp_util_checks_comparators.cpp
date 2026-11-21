// COMP_UTIL.CPP, block `comparators`: cases for the seven qsort comparators (0x10463580 doubles, 0x104647a0 /
// 0x104647e0 / 0x10464830 club reputations, 0x104647c0 bytes, 0x10477cd0 division sort, 0x104777c0 screen rows).
//
// Each case is one call with two element addresses in r3 and r4, compared as the checker compares every entry:
// r3, r4-r10 at return, the non-volatile registers, FPSCR, memory (the frame of 0x104777c0 reaches the caller's
// +8). Elements are real clubs of the career where the comparator takes clubs, and rows built in the scratch
// area (this block's area is scratch_base + 0x60000..0x68000) with chosen values where the career has no
// instance of a branch: reputations at the halfword extremes, bytes either side of 0x80, doubles of every class
// (zeros of both signs, infinities, quiet and signalling NaNs, denormals), ids whose difference wraps. Cases
// that need a null pointer test every combination of null and non-null slots. Labels start with "comparators:"
// (CM_COMP_UTIL_BLOCK=comparators).
#include "comp_util_checks.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t score = 0x10463580, by_reputation = 0x104647a0, by_tag = 0x104647c0, nulls_last = 0x104647e0,
                        ascending = 0x10464830, by_division = 0x10477cd0, screen_rows = 0x104777c0;
constexpr std::int32_t clubs_slot = -0x7be8, club_count_slot = -0x7b50;
constexpr std::uint32_t area = scratch_base + 0x60000;
constexpr std::uint32_t slot_a = area, slot_b = area + 4;
constexpr std::uint32_t record_a = area + 0x100, record_b = area + 0x200;       // doubles / bytes / screen rows
constexpr std::uint32_t club_a = area + 0x1000, club_b = area + 0x1400;         // scratch club rows (0x400 each)
constexpr std::uint32_t division_a = area + 0x2000, division_b = area + 0x2100; // competition records

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
void append(std::vector<Write>& to, std::initializer_list<Write> more) { to.insert(to.end(), more.begin(), more.end()); }
void double_at(std::vector<Write>& to, std::uint32_t at, std::uint64_t bits) {
    append(to, {w32(at, std::uint32_t(bits >> 32)), w32(at + 4, std::uint32_t(bits))});
}
// The two slots, holding the given pointers.
std::vector<Write> slots(std::uint32_t first, std::uint32_t second) { return {w32(slot_a, first), w32(slot_b, second)}; }
Case slot_case(std::uint32_t entry, std::vector<Write> memory, const std::string& label) {
    return {.entry = entry, .registers = {{3, slot_a}, {4, slot_b}}, .memory = std::move(memory), .label = "comparators: " + label};
}
Case element_case(std::uint32_t entry, std::uint32_t first, std::uint32_t second, std::vector<Write> memory, const std::string& label) {
    return {.entry = entry, .registers = {{3, first}, {4, second}}, .memory = std::move(memory), .label = "comparators: " + label};
}
}

COMP_UTIL_CASES("comparators") {
    // 0x10463580: a double at +4 of two records, in every class and both orders (13 x 13 pairs, plus the same
    // record twice).
    const std::uint64_t doubles[] = {
        0x0000000000000000, 0x8000000000000000,  // +0, -0
        0x3ff0000000000000, 0xbff0000000000000,  // 1, -1
        0x3ff0000000000001, 0x3fefffffffffffff,  // one ulp above and below 1
        0x7ff0000000000000, 0xfff0000000000000,  // infinities
        0x7ff8000000000000, 0xfff8000000000001,  // quiet NaNs
        0x7ff0000000000001, 0xfff4000000000000,  // signalling NaNs
        0x0000000000000001, 0x800fffffffffffff,  // denormals
        0x7fefffffffffffff, 0x4059000000000000,  // largest, 100
    };
    for (const auto a : doubles)
        for (const auto b : doubles) {
            std::vector<Write> memory;
            double_at(memory, record_a + 4, a);
            double_at(memory, record_b + 4, b);
            cases.push_back(element_case(score, record_a, record_b, std::move(memory), "score " + hex(std::uint32_t(a >> 32)) + hex(std::uint32_t(a)) + " vs " + hex(std::uint32_t(b >> 32)) + hex(std::uint32_t(b))));
        }
    std::vector<Write> memory;
    double_at(memory, record_a + 4, 0x4000000000000000);
    cases.push_back(element_case(score, record_a, record_a, std::move(memory), "score the same record twice"));
}

COMP_UTIL_CASES("comparators") {
    // 0x104647c0: the signed byte at +4: every pair of bytes around the sign and the extremes.
    const std::uint32_t bytes[] = {0, 1, 2, 5, 0x7e, 0x7f, 0x80, 0x81, 0xfe, 0xff, 0x40, 0xc0};
    for (const auto a : bytes)
        for (const auto b : bytes)
            cases.push_back(element_case(by_tag, record_a, record_b, {w8(record_a + 4, a), w8(record_b + 4, b)}, "tag " + hex(a) + " vs " + hex(b)));
    cases.push_back(element_case(by_tag, record_a, record_a, {w8(record_a + 4, 0x90)}, "tag the same record twice"));
}

COMP_UTIL_CASES("comparators") {
    // The three club-reputation comparators on the career's real clubs: every pair among the first 60 clubs, both
    // orders, the club with itself, and the reputations of the real table as they stand.
    const auto base = scenario.global(clubs_slot);
    const auto total = std::min<std::uint32_t>(scenario.global(club_count_slot), 60);
    for (const auto entry : {by_reputation, nulls_last, ascending})
        for (std::uint32_t i = 0; i < total; ++i)
            for (std::uint32_t j = 0; j < total; ++j)
                cases.push_back(slot_case(entry, slots(base + i * 0x248, base + j * 0x248), "reputation " + hex(entry) + " real clubs " + std::to_string(i) + "/" + std::to_string(j)));
}

COMP_UTIL_CASES("comparators") {
    // The club-reputation comparators on scratch clubs: halfword extremes, equal values, and (for the two that
    // sort empty slots last) every combination of null and non-null slots.
    const std::uint32_t reputations[] = {0x8000, 0x8001, 0xfc18, 0xffff, 0, 1, 0x1f4, 0x7ffe, 0x7fff, 0x00ff, 0xff00};
    for (const auto entry : {by_reputation, nulls_last, ascending})
        for (const auto a : reputations)
            for (const auto b : reputations)
                cases.push_back(slot_case(entry, [&] { auto m = slots(club_a, club_b); append(m, {w16(club_a + 0x1ec, a), w16(club_b + 0x1ec, b)}); return m; }(),
                                          "reputation " + hex(entry) + " halfwords " + hex(a) + "/" + hex(b)));
    for (const auto entry : {nulls_last, ascending}) {
        cases.push_back(slot_case(entry, slots(0, 0), "reputation " + hex(entry) + " both slots empty"));
        for (const auto a : {0u, 0x8000u, 0x7fffu})
            cases.push_back(slot_case(entry, [&] { auto m = slots(0, club_b); append(m, {w16(club_b + 0x1ec, a)}); return m; }(), "reputation " + hex(entry) + " first slot empty " + hex(a)));
        for (const auto b : {0u, 0x8000u, 0x7fffu})
            cases.push_back(slot_case(entry, [&] { auto m = slots(club_a, 0); append(m, {w16(club_a + 0x1ec, b)}); return m; }(), "reputation " + hex(entry) + " second slot empty " + hex(b)));
    }
    // The same club in both slots, and two slots holding the same club.
    for (const auto entry : {by_reputation, nulls_last, ascending})
        cases.push_back(slot_case(entry, [&] { auto m = slots(club_a, club_a); append(m, {w16(club_a + 0x1ec, 0x1234)}); return m; }(), "reputation " + hex(entry) + " one club twice"));
}

COMP_UTIL_CASES("comparators") {
    // 0x10477cd0, the division sort. Real clubs first: pairs of the career's clubs (their real division records
    // and tiebreak bytes), then scratch clubs for every branch: a missing record on either or both sides,
    // reputations that differ (extremes of the halfword), equal reputations with ids that differ (the wrapping
    // difference: ids 0x7fffffff, 0x80000000, 0, 0xffffffff), equal ids with tiebreak bytes either side of 0x80.
    const auto base = scenario.global(clubs_slot);
    const auto total = std::min<std::uint32_t>(scenario.global(club_count_slot), 70);
    for (std::uint32_t i = 0; i < total; ++i)
        for (std::uint32_t j = 0; j < total; ++j)
            cases.push_back(slot_case(by_division, slots(base + i * 0x248, base + j * 0x248), "division real clubs " + std::to_string(i) + "/" + std::to_string(j)));
    const auto build = [&](std::uint32_t first_record, std::uint32_t second_record, std::uint32_t reputation_a, std::uint32_t reputation_b,
                           std::uint32_t id_a, std::uint32_t id_b, std::uint32_t tie_a, std::uint32_t tie_b) {
        std::vector<Write> memory = slots(club_a, club_b);
        append(memory, {w32(club_a + 0x34, first_record), w32(club_b + 0x34, second_record), w8(club_a + 0x23c, tie_a), w8(club_b + 0x23c, tie_b),
                        w16(division_a + 0x18, reputation_a), w16(division_b + 0x18, reputation_b), w32(division_a, id_a), w32(division_b, id_b)});
        return memory;
    };
    cases.push_back(slot_case(by_division, build(0, 0, 5, 5, 1, 1, 1, 1), "division neither club has a record"));
    cases.push_back(slot_case(by_division, build(0, division_b, 5, 5, 1, 1, 1, 1), "division first club without a record"));
    cases.push_back(slot_case(by_division, build(division_a, 0, 5, 5, 1, 1, 1, 1), "division second club without a record"));
    for (const auto a : {0x8000u, 0xffffu, 0u, 1u, 0x7fffu, 0x00ffu})
        for (const auto b : {0x8000u, 0xffffu, 0u, 1u, 0x7fffu, 0x00ffu})
            cases.push_back(slot_case(by_division, build(division_a, division_b, a, b, 7, 3, 0, 0), "division reputations " + hex(a) + "/" + hex(b)));
    for (const auto a : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 5u})
        for (const auto b : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 5u})
            cases.push_back(slot_case(by_division, build(division_a, division_b, 100, 100, a, b, 9, 4), "division ids " + hex(a) + "/" + hex(b)));
    for (const auto a : {0u, 1u, 0x7fu, 0x80u, 0xffu})
        for (const auto b : {0u, 1u, 0x7fu, 0x80u, 0xffu})
            cases.push_back(slot_case(by_division, build(division_a, division_b, 100, 100, 9, 9, a, b), "division tiebreaks " + hex(a) + "/" + hex(b)));
    cases.push_back(slot_case(by_division, [&] { auto m = build(division_a, division_a, 100, 100, 9, 9, 3, 3); m[0] = w32(slot_a, club_a); m[1] = w32(slot_b, club_a); return m; }(), "division one club twice"));
}

COMP_UTIL_CASES("comparators") {
    // 0x104777c0, the screen rows: each key deciding (both signs, extremes of the byte and the halfwords), a key
    // that ties and the next deciding, and the names of real clubs of the career when every key ties (pairs of
    // the first 40 clubs), plus scratch clubs with names that differ in case, length, accents and emptiness.
    const auto row = [&](std::uint32_t at, std::uint32_t club, std::uint32_t group, std::uint32_t k32, std::uint32_t k34, std::uint32_t k30) {
        return std::vector<Write>{w32(at + 0x1c, club), w8(at + 0x42, group), w16(at + 0x32, k32), w16(at + 0x34, k34), w16(at + 0x30, k30)};
    };
    const auto both = [&](std::uint32_t club_one, std::uint32_t club_two, std::array<std::uint32_t, 4> a, std::array<std::uint32_t, 4> b) {
        auto memory = row(record_a, club_one, a[0], a[1], a[2], a[3]);
        auto second = row(record_b, club_two, b[0], b[1], b[2], b[3]);
        memory.insert(memory.end(), second.begin(), second.end());
        return memory;
    };
    const auto base = scenario.global(clubs_slot);
    const std::uint32_t values[] = {0, 1, 0x7f, 0x80, 0xff};
    for (unsigned key = 0; key < 4; ++key)
        for (const auto a : values)
            for (const auto b : values) {
                std::array<std::uint32_t, 4> first{3, 0x1234, 0x8001, 7}, second = first;
                first[key] = a;
                second[key] = b;
                if (key > 0) { first[key - 1] = 5; second[key - 1] = 5; }
                // Extend the byte pattern to the halfword keys (sign on bit 15).
                if (key > 0) { first[key] = a * 0x101; second[key] = b * 0x101; }
                cases.push_back(element_case(screen_rows, record_a, record_b, both(base, base + 0x248, first, second),
                                             "screen rows key " + std::to_string(key) + " " + hex(first[key]) + "/" + hex(second[key])));
            }
    // Earlier keys deciding although later ones disagree the other way.
    cases.push_back(element_case(screen_rows, record_a, record_b, both(base, base + 0x248, {1, 9, 9, 9}, {2, 0, 0, 0}), "screen rows group decides over the rest"));
    cases.push_back(element_case(screen_rows, record_a, record_b, both(base, base + 0x248, {1, 1, 9, 9}, {1, 2, 0, 0}), "screen rows key 32 decides over the rest"));
    cases.push_back(element_case(screen_rows, record_a, record_b, both(base, base + 0x248, {1, 1, 1, 9}, {1, 1, 2, 0}), "screen rows key 34 decides over the rest"));
    const auto total = std::min<std::uint32_t>(scenario.global(club_count_slot), 40);
    for (std::uint32_t i = 0; i < total; ++i)
        for (std::uint32_t j = 0; j < total; ++j)
            cases.push_back(element_case(screen_rows, record_a, record_b, both(base + i * 0x248, base + j * 0x248, {1, 2, 3, 4}, {1, 2, 3, 4}),
                                         "screen rows real names " + std::to_string(i) + "/" + std::to_string(j)));
    // Scratch clubs with chosen names (club +0x221).
    const auto name_at = [&](std::uint32_t club, const std::string& name) {
        std::vector<Write> memory;
        for (std::size_t n = 0; n <= name.size(); ++n) memory.push_back(w8(club + 0x221 + std::uint32_t(n), n < name.size() ? std::uint8_t(name[n]) : 0));
        return memory;
    };
    const std::string names[] = {"", "A", "a", "AB", "Ab", "ab", "Abc", "abc", "ABC", "Zz", "Real Madrid", "real madrid", "Real Madrid B", "\xc9" "ire", "Eire", "\xe9" "ire"};
    for (const auto& a : names)
        for (const auto& b : names) {
            auto memory = both(club_a, club_b, {1, 2, 3, 4}, {1, 2, 3, 4});
            for (const auto& write : name_at(club_a, a)) memory.push_back(write);
            for (const auto& write : name_at(club_b, b)) memory.push_back(write);
            cases.push_back(element_case(screen_rows, record_a, record_b, std::move(memory), "screen rows names \"" + a + "\" / \"" + b + "\""));
        }
}
}

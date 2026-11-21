// PLAYER_REGEN.CPP, block `candidate_score`: cases for the pooled Random(n) (0x102da630), the candidate score
// (0x102dc410) and the pick of the best candidate (0x102dbf00).
//
// The career's real people, clubs and player records are the starting point (a captured state: season-day). Cases:
//  * 0x102da630: bounds on each threshold (0, +-1, the halfword limits, 0x10000, the wrapping tenfold that makes the
//    second bound 0, +-32768 or small), each with the generator at the middle of the pool, at its last sample and one or two
//    samples before it (so the first or the second draw refills), and samples and salts on the edges of the remainders; plus
//    scaled draws where the product of the two doubles lands on, just under and just over an integer (the truncation
//    is the edge: a draw computed with another rounding or an integer product differs there).
//  * 0x102dc410: real people (every kind of person: staff, retired, young, attached or not, with and without a player
//    record) and one real eligible person with each field written over a range of values (undone afterwards): the age byte
//    around 31, the class bits, the staff word, the club's reputation and the player's value around zero and one, the
//    byte +0x2f over all 256 values (the cubed term is scaled from 41 on), the two halfwords around 7500, each rating byte,
//    the club's word +0x90, the unattached branch's role byte and its modulo threshold, and the final score on each side of
//    +-65535 (the scaled final draw) with the age draw and the cubed term off so the score is known.
//  * 0x102dbf00: keys of real nations over windows of the real people table (the table pointer and the count are written
//    for the case), with and without a positive score in the first pass, empty and one-person windows, and the
//    whole table.
// Labels start with "candidate_score:" (CM_PLAYER_REGEN_BLOCK=candidate_score).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t draw_below = 0x102da630, candidate_score = 0x102dc410, best_candidate = 0x102dbf00;
constexpr std::int32_t cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90, start_slot = -0x7b8c,
                       people_slot = -0x7be0, count_slot = -0x7be4, kind_slot = -0x7b9c;

struct Pool {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, cursor, end, salt;
    explicit Pool(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)),
          cursor(s.r32(cursor_cell)), end(s.r32(end_cell)), salt(s.r32(salt_cell)) {}
    // Writes that put the generator at `at` with a salt; the samples the next draws read (after each advance) follow it.
    std::vector<Write> at(std::uint32_t position, std::uint32_t new_salt, const std::vector<std::uint32_t>& samples = {}) const {
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, new_salt)};
        std::uint32_t next = position;
        for (const auto sample : samples) {
            next += 2;
            memory.push_back(w16(next > end ? start : next, sample));
        }
        return memory;
    }
};

// The record fields of a person as the score reads them.
namespace field {
constexpr std::uint32_t id = 0x00, nation = 0x0c, player = 0x14, staff = 0x1c, club = 0x24, age = 0x5c, role = 0x5e,
                        r61 = 0x61, r63 = 0x63, r64 = 0x64, r65 = 0x65, r66 = 0x66, r67 = 0x67, r69 = 0x69;
constexpr std::uint32_t value_a = 0x0a, value_c = 0x0c, potential = 0x2f;  // the player record
constexpr std::uint32_t reputation = 0x1ec, word_90 = 0x90;                // the club row
}

struct World {
    const Scenario& s;
    std::uint32_t kind_table;
    explicit World(const Scenario& scenario) : s(scenario), kind_table(scenario.global(kind_slot)) {}
    std::uint32_t kind_byte(std::uint32_t person) const { return kind_table + s.r32(person + field::id) * 100 + 0xc; }
    std::int32_t age(std::uint32_t person) const { return std::int8_t(s.u16(person + field::age) & 0xff); }
    bool eligible(std::uint32_t person) const {
        return s.r32(person + field::staff) == 0 && ((s.u8(kind_byte(person)) >> 2) & 3) == 0 && age(person) >= 31;
    }
};

std::string label(const std::string& what, std::uint32_t value) { return "candidate_score: " + what + " " + hex(value); }
}

PLAYER_REGEN_CASES("candidate_score") {
    // First in the list (the call trace pairs only a routine's first calls and every 64th): the draws that refill the
    // pool, one for each of the original's refill sites, so the trace compares the calls' return addresses and registers.
    const World w(scenario);
    const Pool pool(scenario);
    // 0x102da630: the first draw refills (0x102da684); the second of a scaled draw refills (0x102da770).
    for (const std::uint32_t position : {pool.end, pool.end - 2})
        cases.push_back({.entry = draw_below, .registers = {{3, 65536}}, .memory = pool.at(position, 5, {7, 11}),
                         .label = label("traced refill of a scaled draw at " + std::to_string(pool.end - position), 0)});
    for (const std::uint32_t position : {pool.end, pool.end - 2})
        cases.push_back({.entry = draw_below, .registers = {{3, 100}}, .memory = pool.at(position, 5, {7, 11}), .label = label("traced refill of a small draw at " + std::to_string(pool.end - position), 0)});
    // 0x102dc410: an unattached person (four draws: age, cubed, modulo, final) and an attached one (three), the pool ending
    // before each draw; and a scaled final draw whose second draw (the call of 0x102da630) refills.
    std::uint32_t unattached = 0, attached = 0;
    for (std::uint32_t n = 0; n + 16 < scenario.people() && (!unattached || !attached); ++n) {
        const auto p = scenario.person(n);
        if (!w.eligible(p) || scenario.r32(p + field::player) == 0) continue;
        if (scenario.r32(p + field::club) == 0) { if (!unattached) unattached = p; }
        else if (!attached) attached = p;
    }
    for (const auto p : {unattached, attached})
        for (std::uint32_t before = 0; p && before <= 4; ++before) {
            auto memory = pool.at(pool.end - 2 * before, 7, {1234, 4321, 11111, 22222});
            append(memory, {w16(p + field::age, 40), w8(scenario.r32(p + field::player) + field::potential, 50)});
            cases.push_back({.entry = candidate_score, .registers = {{3, 0}, {4, p}}, .memory = std::move(memory),
                             .label = label(std::string("traced refill, ") + (p == unattached ? "unattached" : "attached") + " person, samples before the end " + std::to_string(before), 0)});
        }
    if (attached) {
        const auto club = scenario.r32(attached + field::club), player = scenario.r32(attached + field::player);
        auto memory = pool.at(pool.end - 2, 0, {0xfffe, 5});
        append(memory, {w16(attached + field::age, 31), w8(player + field::potential, 0), w16(club + field::reputation, 1), w16(player + field::value_a, 7000), w16(player + field::value_c, 0)});
        cases.push_back({.entry = candidate_score, .registers = {{3, 0}, {4, attached}}, .memory = std::move(memory),
                         .label = label("traced refill of the scaled final draw's second draw", 0)});
    }
}

PLAYER_REGEN_CASES("candidate_score") {
    // 0x102da630: Random(bound).
    const Pool pool(scenario);
    const auto middle = pool.cursor + 0x1000 < pool.end && pool.cursor > pool.start + 0x100 ? pool.cursor : pool.start + 0x400;
    const std::array<std::int32_t, 15> small{0, 1, -1, 2, -2, 3, 7, 10, 100, 9999, 10000, 65534, 65535, -65534, -65535};
    const std::array<std::int32_t, 27> wide{65536, -65536, 65537, -65537, 131070, 131071, 196605, 262144, -262144, 68921, -68921,
                                            1000000, -1000000, 8191750, 0x19999999, 0x1999999a, 0x10000000, 0x3fffffff, 0x40000000,
                                            -0x40000000, 0x7ffffff6, 0x7ffffffe, 0x7fffffff, std::int32_t(0x80000000), std::int32_t(0x80000001),
                                            std::int32_t(0xe6666667), 0x33333334};
    struct Setting { const char* name; std::uint32_t position; };
    const std::array<Setting, 4> settings{{{"middle", middle}, {"at the end", pool.end}, {"one before the end", pool.end - 2}, {"two before the end", pool.end - 4}}};
    const std::array<std::array<std::uint32_t, 4>, 6> samples{{{0, 0, 0, 0}, {1, 0, 1, 0}, {0xffff, 0xffff, 0xffff, 0xffff}, {0xffff, 0, 9, 0xffff},
                                                                {0xfffe, 1, 4, 5}, {12345, 54321, 54321, 4}}};
    for (const auto bound : small) for (const auto& setting : settings) for (const auto& s : samples) {
        auto memory = pool.at(setting.position, s[1], {s[0], s[2]});
        cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}}, .memory = memory,
                         .label = label("draw below " + std::to_string(bound) + " " + setting.name + " sample " + hex(s[0]), s[1])});
    }
    for (const auto bound : wide) for (const auto& setting : settings) for (const auto& s : samples) {
        auto memory = pool.at(setting.position, s[1], {s[0], s[2]});
        cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}}, .memory = memory,
                         .label = label("draw below " + std::to_string(bound) + " " + setting.name + " sample " + hex(s[0]), s[1])});
    }
    // The cap: a bound of 0x10000 and the largest remainder, with a second draw whose tenth rounds up.
    for (const std::int32_t bound : {0x10000, -0x10000, 0x10001, 0x20000})
        for (const std::uint32_t second : {0u, 4u, 5u, 9u, 14u, 15u, 100u}) {
            auto memory = pool.at(middle, 0, {0xfffe, second});
            cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}}, .memory = memory,
                             .label = label("draw below " + std::to_string(bound) + " cap, second sample " + hex(second), 0)});
        }
    // The truncation edge: bounds and remainders whose exact product over 65535 is an integer, and those where the
    // doubles (n / 65535, times the remainder) land on the other side of an integer than the exact quotient.
    unsigned exact = 0, rounded_down = 0, rounded_up = 0;
    for (std::int64_t n = 65536; n < 70000000 && (rounded_down < 150 || exact < 60); n += n < 80000 ? 1 : 3517) {
        for (const std::int64_t rest : {1, 2, 3, 255, 257, 21845, 32767, 32768, 43690, 65532, 65534}) {
            const double scale = double(n) / 65535.0;
            const auto computed = std::int64_t(scale * double(rest));
            const auto true_floor = (n * rest) / 65535;
            const bool integral = (n * rest) % 65535 == 0;
            // The exact integers where the doubles land on them, under them (the truncation then loses one) or, where
            // the quotient is not an integer, on the other side of the exact floor.
            unsigned* counter = computed < true_floor ? &rounded_down : computed > true_floor ? &rounded_up : integral ? &exact : nullptr;
            if (!counter || *counter >= 150) continue;
            ++*counter;
            for (const std::int64_t sign : {1, -1}) {
                // The sum is the remainder itself (sample + salt below 65535), the second draw's sample 3.
                auto memory = pool.at(middle, 0, {std::uint32_t(rest), 3});
                cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(sign * n)}}, .memory = memory,
                                 .label = label("draw below " + std::to_string(sign * n) + " truncation edge rest " + std::to_string(rest) + (computed < true_floor ? " under" : computed > true_floor ? " over" : " exact"), 0)});
            }
        }
    }
    std::fprintf(stderr, "candidate_score: truncation-edge bounds: %u exact integers, %u computed under, %u computed over\n", exact, rounded_down, rounded_up);
}

PLAYER_REGEN_CASES("candidate_score") {
    // 0x102dc410: real people of every kind, then one real eligible person with fields written over their ranges.
    const World w(scenario);
    const Pool pool(scenario);
    const auto people = scenario.people();
    std::vector<std::uint32_t> attached_player, unattached_player, no_player, staff, classed, young;
    for (std::uint32_t n = 0; n < people; ++n) {
        const auto p = scenario.person(n);
        if (scenario.r32(p + field::staff) != 0) staff.push_back(p);
        else if (((scenario.u8(w.kind_byte(p)) >> 2) & 3) != 0) classed.push_back(p);
        else if (w.age(p) < 31) young.push_back(p);
        else if (scenario.r32(p + field::player) == 0) no_player.push_back(p);
        else if (scenario.r32(p + field::club) != 0) attached_player.push_back(p);
        else unattached_player.push_back(p);
    }
    std::fprintf(stderr, "candidate_score: %u people: %zu staff, %zu classed, %zu under 31, %zu without a player record, %zu attached players, %zu unattached players\n",
                 people, staff.size(), classed.size(), young.size(), no_player.size(), attached_player.size(), unattached_player.size());
    if (attached_player.size() < 8) throw std::runtime_error("candidate_score: the career has too few eligible attached players");
    const auto sample_of = [&](const std::vector<std::uint32_t>& list, std::size_t wanted, const char* what) {
        for (std::size_t n = 0; n < wanted && n < list.size(); ++n) {
            const auto p = list[n * list.size() / std::min(wanted, list.size())];
            cases.push_back({.entry = candidate_score, .registers = {{3, 0}, {4, p}}, .label = label(std::string("score of real ") + what, p)});
        }
    };
    sample_of(attached_player, 600, "attached player");
    sample_of(unattached_player, 150, "unattached player");
    sample_of(staff, 20, "staff");
    sample_of(classed, 20, "classed person");
    sample_of(young, 20, "young person");
    sample_of(no_player, 20, "person without a player record");

    // The edits of one real eligible attached player.
    const auto P = attached_player[attached_player.size() / 2];
    const auto club = scenario.r32(P + field::club), player = scenario.r32(P + field::player);
    const auto add = [&](std::vector<Write> memory, const std::string& what) {
        cases.push_back({.entry = candidate_score, .registers = {{3, 0}, {4, P}}, .memory = std::move(memory), .label = "candidate_score: " + what});
    };
    for (const std::uint32_t value : {1u, 0x80000000u, 0xffffffffu}) add({w32(P + field::staff, value)}, "staff word " + hex(value));
    for (const std::uint32_t value : {0x00u, 0x04u, 0x08u, 0x0cu, 0x03u, 0xf3u, 0xffu, 0xf0u}) add({w8(w.kind_byte(P), value)}, "class byte " + hex(value));
    for (const std::uint32_t low : {0u, 1u, 30u, 31u, 32u, 33u, 100u, 127u, 0x80u, 0xffu})
        for (const std::uint32_t high : {0u, 0x12u, 0xffu}) add({w16(P + field::age, high << 8 | low)}, "age halfword " + hex(high << 8 | low));
    // The club's reputation against the player's value (the quotient times 250), the edges of the division.
    for (const std::uint32_t reputation : {0u, 1u, 0xffffu, 2u, 100u, 5000u, 0x7fffu, 0x8000u})
        for (const std::uint32_t value : {0u, 1u, 0xffffu, 7499u, 7500u, 7501u, 0x7fffu, 0x8000u, 1000u, 30000u})
            add({w16(club + field::reputation, reputation), w16(player + field::value_a, value)}, "reputation " + hex(reputation) + " value " + hex(value));
    add({w32(P + field::club, 0)}, "no club");
    add({w32(P + field::player, 0)}, "no player record");
    add({w32(P + field::player, 0), w32(P + field::club, 0)}, "neither club nor player record");
    // The cubed term: the byte over every value (the bound is scaled from |value / 2| >= 41).
    for (std::uint32_t value = 0; value < 256; ++value) add({w8(player + field::potential, value)}, "potential byte " + hex(value));
    // The thresholds on the player's halfwords.
    for (const std::uint32_t c : {0u, 7499u, 7500u, 7501u, 0x7fffu, 0x8000u})
        for (const std::uint32_t a : {0u, 7499u, 7500u, 7501u, 0x7fffu, 0x8000u})
            add({w16(player + field::value_c, c), w16(player + field::value_a, a)}, "values " + hex(c) + "/" + hex(a));
    for (const auto offset : {field::r61, field::r63, field::r64, field::r65, field::r66, field::r67, field::r69})
        for (const std::uint32_t value : {0x80u, 0xffu, 0u, 1u, 0x7fu, 2u, 20u}) add({w8(P + offset, value)}, "rating +" + hex(offset) + " = " + hex(value));
    for (const std::uint32_t value : {0u, 1u, 0x80000000u, 0xffffffffu}) add({w32(club + field::word_90, value)}, "club word 90 = " + hex(value));

    // The unattached branch: the role byte squared, and a doubling when (sample + salt) % 10000 is below the player's +0xc.
    // The age draw and the cubed term are off (age 31, the potential byte 0), so this is the first draw.
    for (const std::uint32_t salt : {0u, 1u, 9999u, 0xffffu})
        for (const std::uint32_t sample : {0u, 1u, 9999u, 10000u, 0x7fffu, 0xffffu}) {
            const auto rest = (sample + salt) % 10000;
            for (const std::int32_t delta : {-1, 0, 1})
                for (const std::uint32_t role : {0u, 1u, 100u, 255u}) {
                    auto memory = pool.at(pool.start + 0x400, salt, {sample, 7});
                    append(memory, {w32(P + field::club, 0), w16(P + field::age, 31), w8(P + field::role, role), w8(player + field::potential, 0),
                                    w16(player + field::value_c, std::uint32_t(std::int32_t(rest) + delta))});
                    add(std::move(memory), "unattached salt " + hex(salt) + " sample " + hex(sample) + " delta " + std::to_string(delta) + " role " + hex(role));
                }
        }
    for (const std::uint32_t value : {0x7fffu, 0x8000u, 0u})
        add({w32(P + field::club, 0), w16(player + field::value_c, value)}, "unattached with value_c " + hex(value));

    // The final draw on each side of 65535 (scaled from there): the score is known with the age draw and the cubed term off.
    // score = (value / 1) * 250 + 5 * r61, with the square terms picking the residue; the club has a reputation of 1 and a
    // nonzero word +0x90.
    const auto target = [&](std::int64_t score) {
        const auto k = std::int32_t(std::clamp<std::int64_t>((score + (score < 0 ? -125 : 125)) / 250, -32768, 32767));
        const auto rest = std::int32_t(score - std::int64_t(k) * 250);
        const auto square = [](std::int32_t r) { return r == 0 ? std::pair{0, 0} : r == 1 ? std::pair{1, 0} : r == 2 ? std::pair{1, 1} : r == 3 ? std::pair{2, 2} : std::pair{2, 0}; };
        const auto residue = ((rest % 5) + 5) % 5;
        const auto [a, b] = square(residue);
        const auto r61 = std::int32_t((rest - a * a - b * b) / 5);
        return std::vector<Write>{w32(P + field::staff, 0), w8(w.kind_byte(P), 0), w16(P + field::age, 31), w16(club + field::reputation, 1),
                                  w32(club + field::word_90, 1), w16(player + field::value_a, std::uint32_t(k)), w16(player + field::value_c, 0),
                                  w8(player + field::potential, 0), w8(P + field::r61, std::uint32_t(r61)), w8(P + field::r63, std::uint32_t(a)),
                                  w8(P + field::r67, std::uint32_t(b)), w8(P + field::r64, 0), w8(P + field::r65, 0), w8(P + field::r66, 0),
                                  w8(P + field::r69, 0)};
    };
    for (const std::int64_t score : {0, 1, 2, 5, 250, 32767, 32768, 65533, 65534, 65535, 65536, 65537, 65538, 131070, 1000000, 8191750,
                                     -1, -5, -32768, -65533, -65534, -65535, -65536, -65537, -65538, -131070, -1000000, -8191750})
        for (const std::uint32_t second : {0u, 3u, 0xffffu}) {
            auto memory = target(score);
            append(memory, pool.at(pool.start + 0x400, 0, {0xfffe, second}));
            add(std::move(memory), "final score " + std::to_string(score) + " second sample " + hex(second));
        }

    // The pool's end at each draw: an unattached person with an age draw and a cubed term draws four times (age, cubed,
    // modulo, final); an attached one three times.
    for (const auto p : {unattached_player.empty() ? P : unattached_player.front(), P})
        for (std::uint32_t before = 0; before <= 5; ++before) {
            auto memory = pool.at(pool.end - 2 * before, 7, {1234, 4321, 11111, 22222, 33333});
            append(memory, {w16(p + field::age, 40), w8(scenario.r32(p + field::player) + field::potential, 50)});
            cases.push_back({.entry = candidate_score, .registers = {{3, 0}, {4, p}}, .memory = std::move(memory),
                             .label = "candidate_score: pool " + std::to_string(before) + " samples before the end, " + (p == P ? "attached" : "unattached")});
        }
}

PLAYER_REGEN_CASES("candidate_score") {
    // 0x102dbf00: windows of the real people table. The table's base cell and the count are written for the case.
    const World w(scenario);
    const auto base_cell = scenario.toc(people_slot), count_cell = scenario.toc(count_slot);
    const auto people = scenario.people();
    std::vector<std::uint32_t> eligible;
    for (std::uint32_t n = 0; n + 16 < people; ++n)
        if (w.eligible(scenario.person(n)) && scenario.r32(scenario.person(n) + field::player) != 0) eligible.push_back(n);
    if (eligible.size() < 8) throw std::runtime_error("candidate_score: the career has too few eligible people");
    const auto nation_key = [&](std::uint32_t n) {
        const auto nation = scenario.r32(scenario.person(n) + field::nation);
        return nation ? scenario.r32(nation + 0x68) : 0xdeadbeefu;
    };
    const std::array<std::uint32_t, 3> starts{eligible.front(), eligible[eligible.size() / 3], eligible[eligible.size() * 2 / 3]};
    for (const auto start : starts)
        for (const std::uint32_t window : {0u, 1u, 2u, 5u, 40u, 200u}) {
            std::set<std::uint32_t> keys{0x7fffffffu, 0u};
            for (std::uint32_t t = 0; t < window && t < 40; ++t) keys.insert(nation_key(start + t));
            for (const auto key : keys)
                cases.push_back({.entry = best_candidate, .registers = {{3, 0x1234}, {4, key}},
                                 .memory = {w32(base_cell, scenario.person(start)), w32(count_cell, 16 + window)},
                                 .label = "candidate_score: best, window " + std::to_string(window) + " at " + hex(start) + " key " + hex(key)});
        }
    for (const std::uint32_t count : {0u, 15u, 16u, 17u, 0xfffffff0u, 0x80000000u})
        cases.push_back({.entry = best_candidate, .registers = {{3, 0}, {4, nation_key(eligible.front())}},
                         .memory = {w32(count_cell, count)}, .label = "candidate_score: best, count " + hex(count)});
    // The whole table, a key present and a key absent (the second pass runs): last, being the longest.
    cases.push_back({.entry = best_candidate, .registers = {{3, 0}, {4, nation_key(eligible.front())}}, .label = "candidate_score: best, whole table, a nation of the first"});
    cases.push_back({.entry = best_candidate, .registers = {{3, 0}, {4, 0x7fffffff}}, .label = "candidate_score: best, whole table, absent key"});
}
}

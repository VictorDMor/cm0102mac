// LEAGUE.CPP, block season: cases for the nine entries (0x104a00a0 season setup, 0x104a10d0 round-robin matrix,
// 0x104a28d0 fixture list, 0x104a3350 random draw, 0x104a38d0 round order, 0x104a9600 next round, 0x104a9810
// round date, 0x104a9890 match slot, 0x104aab30 preset fixture). Real league objects of the career (their
// schedules, standings, clubs and the fixture manager), with the values each branch reads written over them;
// scratch rows for the leaves; the sample stream's cursor, limit and salt set per case so the draws take every
// outcome; allocation failures through `answers`/`nth_answers`. Labels start with "season:".
#include "league_base_checks.hpp"
#include <array>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t setup = 0x104a00a0, pairing = 0x104a10d0, schedule = 0x104a28d0, random_below = 0x104a3350,
                        compare_rounds = 0x104a38d0, next_round = 0x104a9600, round_date = 0x104a9810,
                        match_slot = 0x104a9890, preset = 0x104aab30;
[[maybe_unused]] constexpr std::uint32_t allocate = 0x107735f0;
[[maybe_unused]] constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, cursor_slot = -0x7b84,
                       limit_slot = -0x7b88, seed_slot = -0x7b90, start_slot = -0x7b8c, special_slot = -0x7a30,
                       first_special_slot = -0x7bc0, second_special_slot = -0x6948;
constexpr std::uint32_t area = scratch_base + 0x80000;
constexpr std::uint32_t object_size = 0xec;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
std::string label(const std::string& text) { return "season: " + text; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;  // league objects with standings and a schedule
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (object && seen.insert(object).second && is_league(object) && s.r32(object + 0xb0) && s.r32(object + 0xbc) && clubs(object) >= 2)
                leagues.push_back(object);
        };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 2) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::int32_t clubs(std::uint32_t object) const { return std::int16_t(s.r32(object + 0x3c) & 0xffff); }
    std::int32_t rounds(std::uint32_t object) const { return std::int16_t(s.r32(object + 0xa4) & 0xffff); }
    std::int32_t year(std::uint32_t object) const { return std::int16_t(s.r32(object + 0x40) >> 16); }
    std::uint32_t schedule(std::uint32_t object) const { return s.r32(object + 0xbc); }
    std::uint32_t round(std::uint32_t object, std::int32_t index) const { return schedule(object) + std::uint32_t(index) * 0x4c; }
    std::uint32_t row(std::uint32_t object, std::uint32_t index) const { return s.r32(object + 0xb0) + index * 0x44; }
    std::uint32_t club(std::uint32_t object, std::uint32_t index) const { return s.r32(row(object, index)); }
    // Leagues of different club counts first (so the first picks cover the sizes), then the rest.
    std::vector<std::uint32_t> by_size(std::size_t limit) const {
        std::vector<std::uint32_t> picks;
        std::set<std::int32_t> sizes;
        for (const auto object : leagues)
            if (sizes.insert(clubs(object)).second && picks.size() < limit) picks.push_back(object);
        return picks;
    }
};
[[maybe_unused]] std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) writes.push_back({4, at + offset, s.r32(object + offset)});
    return writes;
}
// The sample stream: `at` the cursor's value (the sample drawn is the halfword at `at + 2`), `top` the limit, `salt`.
[[maybe_unused]] std::vector<Write> stream(const Scenario& scenario, std::uint32_t table, std::uint32_t at, std::uint32_t top, std::uint32_t salt,
                          std::uint32_t first, std::uint32_t second) {
    return {{4, scenario.toc(cursor_slot), at}, {4, scenario.toc(limit_slot), top}, {4, scenario.toc(seed_slot), salt},
            {2, table + 2, first}, {2, table + 4, second}, {2, table + 6, 3}};
}
std::vector<Write> pattern(std::uint32_t at, std::uint32_t bytes, std::uint32_t seed) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < bytes; offset += 4) writes.push_back({4, at + offset, seed + offset * 0x01010101u});
    return writes;
}

// 0x104a38d0.
LEAGUE_CASES("season-order") {
    if (scenario.match) return;
    const World w(scenario);
    const auto left = area + 0x100, right = area + 0x140;
    const std::uint32_t years[] = {2000, 2001, 2002, 0, 0x7fff, 0x8000, 0xffff};
    const std::uint32_t days[] = {0, 1, 2, 100, 0x7fff, 0x8000, 0xffff, 0xfffe, 0x4000};
    for (const auto year_a : years)
        for (const auto year_b : years) {
            if (year_a != year_b && year_a != 2001 && year_b != 2001) continue;
            for (const auto day_a : days)
                for (const auto day_b : days) {
                    if (year_a != year_b && (day_a != 100 || day_b != 100)) continue;
                    cases.push_back({.entry = compare_rounds, .registers = {{3, left}, {4, right}},
                                     .memory = {{2, left, day_a}, {2, left + 2, year_a}, {2, right, day_b}, {2, right + 2, year_b}},
                                     .label = label("order years " + hex(year_a) + " " + hex(year_b) + " days " + hex(day_a) + " " + hex(day_b))});
                }
        }
    // Real schedule rounds (the career's): adjacent and distant ones, both ways, and the round with itself.
    for (const auto object : w.by_size(4)) {
        const auto count = w.rounds(object);
        for (const auto [a, b] : {std::pair<int, int>{0, 1}, {1, 0}, {0, count - 1}, {count - 1, 0}, {2, 2}, {count / 2, count / 2 + 1}})
            if (a < count && b < count)
                cases.push_back({.entry = compare_rounds, .registers = {{3, w.round(object, a)}, {4, w.round(object, b)}},
                                 .label = label("order rounds " + std::to_string(a) + " " + std::to_string(b) + " of " + hex(object))});
    }
}

// 0x104a9810 and 0x104a9890: the rounds a derived league's calendar fills.
LEAGUE_CASES("season-rounds") {
    if (scenario.match) return;
    const auto rows = area + 0x200;
    const auto base = pattern(rows, 0x4c * 5, 0x11223344u);
    const auto stack_word = scenario.sp() + 0x38;
    std::uint32_t n = 0;
    const auto date_case = [&](std::uint32_t index, std::uint32_t day, std::uint32_t month, std::uint32_t year, std::uint32_t weekday,
                               std::uint32_t kind, std::uint32_t shift, std::uint32_t tail, const std::string& what) {
        cases.push_back({.entry = round_date,
                         .registers = {{3, rows + 0x4c}, {4, index}, {5, day}, {6, month}, {7, year}, {8, weekday}, {9, kind}, {10, shift}},
                         .memory = joined(base, {{4, stack_word, tail}}), .label = label("round date " + what + " #" + std::to_string(n++))});
    };
    // One argument at a time around a base, then a spread of combinations.
    for (const auto index : {0u, 1u, 3u, 0xffffffffu, 0x10001u, 0xffffu})
        date_case(index, 15, 5, 2001, 0xffffffffu, 2, 0, 0x01020304, "index " + hex(index));
    for (const auto day : {1u, 15u, 28u, 29u, 30u, 31u, 0u, 32u, 0xffffu, 0x10005u, 0x7f000003u})
        date_case(1, day, 5, 2001, 0xffffffffu, 2, 0, 7, "day " + hex(day));
    for (const auto month : {0u, 1u, 2u, 5u, 11u, 12u, 0xffu, 0x1000003u})
        date_case(1, 15, month, 2001, 0xffffffffu, 2, 0, 7, "month " + hex(month));
    for (const auto year : {0u, 1u, 1999u, 2000u, 2001u, 2004u, 0x7fffu, 0x8000u, 0xffffu, 0x12345u})
        date_case(1, 15, 5, year, 0xffffffffu, 2, 0, 7, "year " + hex(year));
    for (const auto weekday : {0xffffffffu, 0u, 1u, 3u, 6u, 7u, 0xfeu, 0x100u})
        date_case(1, 15, 5, 2001, weekday, 2, 0, 7, "weekday " + hex(weekday));
    for (const auto kind : {0u, 1u, 2u, 3u, 4u, 0xffu, 0x1ffu})
        date_case(1, 15, 5, 2001, 0xffffffffu, kind, 0, 7, "kind " + hex(kind));
    for (const auto shift : {0u, 1u, 0xffffu, 0x7fffu, 0x8000u, 0x12345u, 0xffff0003u, 2u})
        date_case(1, 15, 5, 2001, 0xffffffffu, 2, shift, 7, "shift " + hex(shift));
    for (const auto tail : {0u, 1u, 0x12345678u, 0xffffffffu})
        date_case(2, 10, 2, 2002, 3, 1, 1, tail, "tail " + hex(tail));
    for (const auto [day, month, year, weekday] : {std::array<std::uint32_t, 4>{1, 0, 2001, 0}, {31, 11, 2001, 6}, {29, 1, 2000, 2}, {29, 1, 2001, 2},
                                                   {31, 3, 2001, 0xffffffffu}, {30, 1, 2001, 4}})
        date_case(4, day, month, year, weekday, 0, 0xffffu, 0xcafe0000u + day, "calendar " + std::to_string(day) + "/" + std::to_string(month));

    // 0x104a9890: the slots; every range failure alone, together, and the valid corners.
    std::uint32_t k = 0;
    const auto slot_case = [&](std::uint32_t index, std::uint32_t slot, std::uint32_t first, std::uint32_t second, std::uint32_t third,
                               std::uint32_t tail, const std::string& what) {
        cases.push_back({.entry = match_slot, .registers = {{3, rows + 0x4c}, {4, index}, {5, slot}, {6, first}, {7, second}, {8, third}, {9, tail}},
                         .memory = base, .label = label("match slot " + what + " #" + std::to_string(k++))});
    };
    for (const auto slot : {0u, 1u, 7u, 8u, 0xffffffffu, 0x7fffu, 0x8000u, 0x10003u, 0xffffu})
        slot_case(1, slot, 2, 1, 0, 0x5555aaaau, "slot " + hex(slot));
    for (const auto first : {0xffffffffu, 0u, 6u, 7u, 0xfffeu, 0x8000u, 0x10005u, 0x7fffu})
        slot_case(1, 3, first, 1, 0, 0x5555aaaau, "first " + hex(first));
    for (const auto second : {0xffffffffu, 0u, 2u, 3u, 0xfeu, 0x80u, 0x7fu, 0x1ffu, 0x100u})
        slot_case(1, 3, 2, second, 0, 0x5555aaaau, "second " + hex(second));
    for (const auto third : {0xffffffffu, 0u, 4u, 5u, 0xfeu, 0x80u, 0x7fu, 0x104u})
        slot_case(1, 3, 2, 1, third, 0x5555aaaau, "third " + hex(third));
    for (const auto index : {0u, 2u, 3u, 0xffffffffu, 0x10002u})
        slot_case(index, 7, 6, 2, 4, 0x80000001u, "index " + hex(index));
    slot_case(1, 9, 9, 9, 9, 0x12345678u, "all four fail");
    slot_case(1, 0xffff8000u, 0xfffffff0u, 0xfffffff0u, 0xfffffff0u, 0, "all four fail negative");
    slot_case(1, 8, 0, 0, 0, 1, "slot only");
    slot_case(1, 0, 7, 3, 5, 1, "three fail");
}

// 0x104a9600 (the next round of a drawn league) and 0x104a3350 (the general random draw).
LEAGUE_CASES("season-draw") {
    if (scenario.match) return;
    const auto table = area + 0x1000, phase = area + 0x900;
    const auto special = scenario.global(special_slot);
    // The sample the draw takes is the halfword after the cursor's: `first` with the salt.
    const auto fed = [&](std::uint32_t salt, std::uint32_t first) { return stream(scenario, table, table, table + 0x100, salt, first, 0); };
    struct Args { std::uint32_t previous, competition, clubs, legs, rounds, flags; };
    const auto draw_case = [&](const Args& a, std::uint32_t state, std::vector<Write> memory, const std::string& what) {
        memory.push_back({1, phase, state});
        cases.push_back({.entry = next_round,
                         .registers = {{3, a.previous}, {4, phase}, {5, a.competition}, {6, a.clubs}, {7, a.legs}, {8, a.rounds}, {9, a.flags}},
                         .memory = std::move(memory), .label = label("next round " + what)});
    };
    // The competition with its own order: 2 and 5 are special, the rest step on.
    for (const auto previous : {0xffffffffu, 0u, 1u, 2u, 3u, 4u, 5u, 6u, 0x7fffffffu})
        draw_case({previous, special, 20, 2, 38, 0}, 0, fed(0, 0), "special previous " + hex(previous));
    // The sweep: every state byte, both samples' low bits, previous around the draw's cutoffs.
    for (const auto rounds : {10u, 38u, 46u})
        for (const auto previous : {0xffffffffu, 0u, 1u, 2u, rounds - 7, rounds - 6, rounds - 5, rounds - 4, rounds - 1, rounds, rounds + 1})
            for (const auto state : {0u, 1u, 2u, 3u, 4u, 0xffu, 0x80u, 0x7fu})
                for (const auto sample : {0u, 1u, 2u, 3u, 0xffffu})
                    draw_case({previous, special + 1, 20, 2, rounds, 0}, state, fed(sample == 0xffff ? 0xffff : 0, sample),
                              "rounds " + std::to_string(rounds) + " previous " + hex(previous) + " state " + hex(state) + " sample " + hex(sample));
    // The flags: bit 0x40 steps on, bit 0x80 wraps the two halves, neither draws; other bits are ignored.
    for (const auto flags : {0u, 0x40u, 0x80u, 0xc0u, 0x3u, 0x100u, 0x8000u, 0xffffu, 0x10040u, 0xffff0000u})
        for (const auto rounds : {2u, 3u, 9u, 10u, 11u, 33u, 0x7fffu, 0x8000u, 0xffffu})
            for (const auto previous : {0xffffffffu, 0u, 1u, 4u, 5u, 6u, 8u, 9u, 10u, 16u, 17u, 18u, 32u, 33u})
                draw_case({previous, special + 1, 20, 2, rounds, flags}, 0, fed(0, 1), "flags " + hex(flags) + " rounds " + hex(rounds) + " previous " + hex(previous));
    // The league's size gates the draw: fewer than 2 legs or 8 clubs steps on.
    for (const auto clubs : {0u, 7u, 8u, 9u, 0x8000u, 0xffffu, 0x10008u})
        for (const auto legs : {0u, 1u, 2u, 3u, 0xffffu, 0x10001u})
            draw_case({3, special + 1, clubs, legs, 38, 0}, 0, fed(0, 1), "clubs " + hex(clubs) + " legs " + hex(legs));
    // The sample stream: a refill at the draw's advance (the cursor on the limit), salts, the cursor wrapping.
    for (const auto top : {0u, 1u, 2u, 3u})
        draw_case({3, special + 1, 20, 2, 38, 0}, 0, stream(scenario, table, table + 0x100 - top, table + 0x100 - (top ? 0 : 0), 7, 1, 0),
                  "refill " + hex(top));
    draw_case({3, special + 1, 20, 2, 38, 0}, 3, stream(scenario, table, table + 0xfe, table + 0x100, 0, 1, 0), "state 3 stream near the end");
    draw_case({3, special + 1, 20, 2, 38, 0}, 0, joined(stream(scenario, table, table + 0x100, table + 0x100, 1, 0, 0), {{2, table + 0x102, 3}}), "cursor on the limit");
    for (const auto salt : {0u, 1u, 2u, 3u, 0xffffu, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        for (const auto sample : {0u, 1u, 2u, 3u})
            draw_case({3, special + 1, 20, 2, 38, 0}, 0, fed(salt, sample), "salt " + hex(salt) + " sample " + hex(sample));
    // The state byte updates: the original leaves it through the three steps and the draw's two outcomes (above).

    // 0x104a3350.
    const auto draw_below = [&](std::uint32_t bound, std::vector<Write> memory, const std::string& what) {
        cases.push_back({.entry = random_below, .registers = {{3, bound}}, .memory = std::move(memory), .label = label("random below " + what)});
    };
    const std::uint32_t bounds[] = {0u, 1u, 2u, 3u, 7u, 10u, 100u, 1000u, 32767u, 32768u, 65534u, 65535u, 65536u, 65537u, 100000u, 655350u,
                                    1000000u, 0x7ffffffu, 0x40000000u, 0x7fffffffu, 0xffffffffu, 0xfffffffeu, 0xffffff9cu, 0xffff0001u,
                                    0xffff0000u, 0xfffeffffu, 0xfff0bdc0u, 0xc0000000u, 0x80000001u, 0x80000000u, 0xfffe0000u};
    const std::pair<std::uint32_t, std::uint32_t> samples[] = {{0, 0}, {1, 2}, {0xffff, 0xffff}, {12345, 54321}, {0x7fff, 3}, {65534, 65535}};
    for (const auto bound : bounds)
        for (const auto& [first, second] : samples)
            draw_below(bound, stream(scenario, table, table, table + 0x100, first == 0xffff ? 7 : 0, first, second), "bound " + hex(bound) + " samples " + hex(first) + " " + hex(second));
    for (const auto bound : bounds) {
        // Refills: at the first advance, at the second (the cursor one step from the limit), and salts.
        draw_below(bound, stream(scenario, table, table + 0x100, table + 0x100, 9, 4, 5), "bound " + hex(bound) + " refill at the first advance");
        draw_below(bound, joined(stream(scenario, table, table + 0xfe, table + 0x100, 3, 4, 5), {{2, table + 0x100, 77}}),
                   "bound " + hex(bound) + " refill at the second advance");
        draw_below(bound, stream(scenario, table, table, table + 0x100, 0xffffffffu, 0xffff, 1), "bound " + hex(bound) + " salt -1");
    }
}

// 0x104a10d0: the round-robin matrix over scratch rows (a table of n + 1 row pointers, n words each): every
// size from 0 to 26 (odd ones too: the original has no parity test), the rows prefilled so the cells it does not
// write (column 0, the cells past a short loop) show, and the entry registers r6-r10 distinct (n <= 2 leaves them).
LEAGUE_CASES("season-matrix") {
    if (scenario.match) return;
    const auto table = area + 0x2000, rows = area + 0x3000;
    for (std::int32_t n = -2; n <= 26; ++n) {
        const auto teams = std::uint32_t(n > 0 ? n : 0);
        std::vector<Write> memory;
        for (std::uint32_t index = 0; index <= teams; ++index) memory.push_back({4, table + index * 4, rows + index * (teams * 4 + 16)});
        for (std::uint32_t index = 0; index <= teams; ++index) {
            const auto row = rows + index * (teams * 4 + 16);
            for (std::uint32_t column = 0; column < teams + 2; ++column) memory.push_back({4, row + column * 4, 0x70000000u + index * 0x100 + column});
        }
        for (const auto fill : {0u, 1u}) {
            auto filled = memory;
            // Cells that already hold a nonzero value are skipped by the circle method's loop: prefill with zero or not.
            if (fill) for (std::uint32_t index = 1; index <= teams; ++index)
                for (std::uint32_t column = 0; column < teams; ++column) filled.push_back({4, rows + index * (teams * 4 + 16) + column * 4, 0});
            cases.push_back({.entry = pairing,
                             .registers = {{3, 0x12345678}, {4, table}, {5, std::uint32_t(n)}, {6, 0x6060}, {7, 0x7070}, {8, 0x8080}, {9, 0x9090}, {10, 0xa0a0}},
                             .memory = std::move(filled), .label = label("matrix n " + std::to_string(n) + (fill ? " zeroed rows" : ""))});
        }
    }
}

// 0x104aab30: a preset fixture from a date and two club names, over the career's real leagues. The club name routine
// 0x100d51d0 reads a language file the snapshot has no host for, so each call is answered with one of three scratch
// strings: the two wanted names and a third; which row answers which is the case.
LEAGUE_CASES("season-preset") {
    if (scenario.match) return;
    const World w(scenario);
    constexpr std::uint32_t club_name = 0x100d51d0;
    const auto fixture = area + 0x4000, home_text = area + 0x4100, away_text = area + 0x4120, other_text = area + 0x4140;
    const auto texts = joined(joined(pattern(fixture, 0x20, 0x33445566u), {{4, home_text, 0x484f4d45u}, {4, home_text + 4, 0}, {4, away_text, 0x41574159u}, {4, away_text + 4, 0},
                                                                         {4, other_text, 0x4e4f4e45u}, {4, other_text + 4, 0}}), {});
    // Row r answers its first call (the home name's) with `home[r]` and its second with `away[r]` (0 home, 1 away, 2 other).
    const auto answered = [&](const std::vector<int>& home_rows, const std::vector<int>& away_rows, std::uint32_t rows) {
        std::vector<Case::NthAnswer> answers;
        const std::uint32_t pointers[3] = {home_text, away_text, other_text};
        for (std::uint32_t row = 0; row < rows; ++row) {
            const auto first = row < home_rows.size() ? home_rows[row] : 2, second = row < away_rows.size() ? away_rows[row] : 2;
            answers.push_back({club_name, 2 * row + 1, pointers[first]});
            answers.push_back({club_name, 2 * row + 2, pointers[second]});
        }
        return answers;
    };
    struct Hit { const char* what; std::vector<int> home, away; };
    const std::vector<Hit> hits = {
        {"both at row 0", {0}, {0}},
        {"home 1 away 2", {2, 0}, {2, 2, 1}},
        {"away 1 home 2", {2, 2, 0}, {2, 1}},
        {"home 0 away 3", {0}, {2, 2, 2, 1}},
        {"home twice", {0, 2, 0}, {2, 2, 2, 2, 2, 2, 2, 1}},
        {"away only", {}, {2, 1}},
        {"home only", {2, 0}, {}},
        {"neither", {}, {}},
        {"home hits the away text", {1, 1}, {0, 0}},
        {"last rows", {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0}, {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1}},
    };
    for (const auto object : w.by_size(5)) {
        const auto n = std::uint32_t(w.clubs(object));
        for (const auto& hit : hits)
            cases.push_back({.entry = preset, .registers = {{3, fixture}, {4, object}, {5, 14}, {6, 8}, {7, 2001}, {8, home_text}, {9, away_text}},
                             .memory = texts, .answers = {{club_name, other_text}}, .nth_answers = answered(hit.home, hit.away, n),
                             .label = label("preset " + hex(object) + " " + hit.what)});
        // An empty league, and clubs of one and of a negative count.
        for (const auto count : {0u})
            cases.push_back({.entry = preset, .registers = {{3, fixture}, {4, object}, {5, 31}, {6, 11}, {7, 2001}, {8, home_text}, {9, away_text}},
                             .memory = joined(texts, {{2, object + 0x3e, count}}), .answers = {{club_name, home_text}},
                             .label = label("preset " + hex(object) + " clubs " + hex(count))});
    }
    // Dates: a leap day, the first day, and arguments DATE asserts on.
    if (const auto object = w.leagues.front(); object) {
        for (const auto [day, month, year] : {std::array<std::uint32_t, 3>{29, 1, 2000}, {29, 1, 2001}, {1, 0, 1972}, {0, 5, 2001}, {32, 5, 2001}, {10, 12, 2001}, {10, 0xff, 2001}})
            cases.push_back({.entry = preset, .registers = {{3, fixture}, {4, object}, {5, day}, {6, month}, {7, year}, {8, home_text}, {9, away_text}},
                             .memory = texts, .answers = {{club_name, home_text}}, .nth_answers = answered({0}, {0, 1}, 3),
                             .label = label("preset date " + std::to_string(day) + "/" + std::to_string(month) + "/" + std::to_string(year))});
    }
}

// 0x104a28d0: the fixture list of a round's date, over the career's real leagues and their schedules. The fixture
// manager's routines read files the snapshot has no host for (1048a2a0 faults on HGetVol), so they are answered
// (their r3 only) and the list of candidates the manager would give is built by a poke at the dispatch of 1048a2a0:
// a list object in the frame (the iterator at sp - 0x2b0 + 0x90: count, head, tail, cursor), its nodes (data
// pointer, size, next, previous) and 0x50-byte records in the scratch area, with real clubs for the home and away
// pointers. The call trace then compares every call the routine makes and the registers each callee reads.
constexpr std::uint32_t find_fixture = 0x1048a2a0, list_end = 0x10738c20, list_remove = 0x10738e60, file_fixture = 0x104aff20,
                        add_fixture = 0x104888f0, remove_fixture = 0x1048a140, news_start = 0x102bdf90, news_field = 0x102cb4b0,
                        news_file = 0x102cc140, date_blocked = 0x1048b180;
struct Slot { std::uint32_t word; std::uint32_t weekday, place, kind; };
struct Candidate { std::uint32_t home, away, kind; int nation; };  // nation: 0 home's, 1 away's, 2 neither, 3 no competition record, 4 no holder
std::vector<std::pair<std::uint32_t, std::uint32_t>> manager_answers(std::uint32_t found, bool node_free = true) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{find_fixture, found}, {list_end, 0}, {list_remove, 0}, {file_fixture, 0}, {add_fixture, 0}, {remove_fixture, 0},
            {news_start, 0}, {news_field, 0}, {news_file, 0}, {date_blocked, 0}};
    // The original runs the real list_end on the poked list (it frees through the allocator in place, which an answer
    // does not reach on its side), so the node free is answered: 0x10738ba0 only.
    if (node_free) answers.push_back({0x10738ba0, 0});
    return answers;
}
void schedule_slots(std::vector<Write>& memory, std::uint32_t round, const std::vector<Slot>& slots) {
    for (std::uint32_t n = 0; n < slots.size(); ++n) {
        const auto at = round + 8 + 8 * n;
        memory.push_back({4, at, slots[n].word});
        memory.push_back({1, at + 4, slots[n].weekday});
        memory.push_back({1, at + 5, slots[n].place});
        memory.push_back({1, at + 6, slots[n].kind});
    }
}

LEAGUE_CASES("season-schedule") {
    if (scenario.match) return;
    const World w(scenario);
    const auto date = area + 0x5000, nodes = area + 0x6000, records = area + 0x6200, competition = area + 0x7000, holder = area + 0x7400;
    const auto iterator = scenario.sp() - 0x2b0 + 0x90;
    const auto table = area + 0x1000;
    struct Config {
        std::string what;
        std::vector<Slot> slots;
        std::vector<Candidate> candidates;
        std::uint32_t found = 1;
        std::vector<std::pair<std::int32_t, std::int32_t>> ratings = {};  // club index, rating halfword
        std::vector<std::pair<unsigned, std::uint32_t>> blocked = {};      // nth 1048b180 call, answer
        std::uint32_t salt = 0, first = 1, second = 2;
        std::int32_t owner_delta = 0;  // candidate kind bytes differ from the league's owner byte by this
        std::uint32_t owner_word = 0xffffffffu;
        std::uint32_t date_shift = 0;  // days added to the round's own date
    };
    const auto add_case = [&](const Config& config, std::uint32_t object, std::int32_t index) {
        const auto clubs = std::uint32_t(w.clubs(object));
        const auto round = w.round(object, index);
        std::vector<Write> memory = {{2, date, (scenario.r32(round) >> 16) + config.date_shift},
                                     {2, date + 2, std::uint32_t(w.year(object)) + (scenario.r32(round) & 0xffff)}};
        schedule_slots(memory, round, config.slots);
        for (const auto& [club, rating] : config.ratings) memory.push_back({2, w.club(object, std::uint32_t(club) % clubs) + 0x1ec, std::uint32_t(rating) & 0xffff});
        memory.push_back({4, scenario.toc(cursor_slot), table});
        memory.push_back({4, scenario.toc(limit_slot), table + 0x100});
        memory.push_back({4, scenario.toc(seed_slot), config.salt});
        memory.push_back({2, table + 2, config.first});
        memory.push_back({2, table + 4, config.second});
        for (std::uint32_t at = 6; at < 40; at += 2) memory.push_back({2, table + at, at});
        if (config.owner_word != 0xffffffffu) memory.push_back({4, object + 0x24, config.owner_word});
        // The poke: the list the manager gives, written once the iterator has been constructed.
        const auto count = config.candidates.size();
        std::vector<Write> list = {{2, iterator, std::uint32_t(count)}, {4, iterator + 4, count ? nodes : 0},
                                   {4, iterator + 8, count ? nodes + std::uint32_t(count - 1) * 0x10 : 0}, {4, iterator + 0xc, count ? nodes : 0}};
        const auto owner = std::int32_t(std::int8_t(scenario.r32(object + 0x44) >> 24));
        for (std::uint32_t k = 0; k < count; ++k) {
            const auto node = nodes + k * 0x10, record = records + k * 0x50;
            list.push_back({4, node, record});
            list.push_back({2, node + 4, 0x50});
            list.push_back({4, node + 8, k + 1 < count ? node + 0x10 : 0});
            list.push_back({4, node + 0xc, k ? node - 0x10 : 0});
            for (std::uint32_t at = 0; at < 0x50; at += 4) list.push_back({4, record + at, 0x01020304u * (k + 1) + at});
            const auto& candidate = config.candidates[k];
            const auto home = w.club(object, candidate.home % clubs), away = w.club(object, candidate.away % clubs);
            list.push_back({4, record + 0x1c, home});
            list.push_back({4, record + 0x20, away});
            list.push_back({1, record + 0x42, std::uint32_t(owner + candidate.kind + config.owner_delta) & 0xff});
            // The fixed-date test: +0x18 the competition, whose +0x10 holder has the nation at +0x18.
            const auto nation = candidate.nation == 0 ? scenario.r32(home + 0x2c) : candidate.nation == 1 ? scenario.r32(away + 0x2c) : 0x0badf00du;
            list.push_back({4, record + 0x18, candidate.nation == 3 ? 0 : competition + k * 0x20});
            list.push_back({4, competition + k * 0x20 + 0x10, candidate.nation == 4 ? 0 : holder + k * 0x20});
            list.push_back({4, holder + k * 0x20 + 0x18, nation});
        }
        std::vector<Case::NthAnswer> blocked;
        for (const auto& [nth, answer] : config.blocked) blocked.push_back({date_blocked, nth, answer});
        cases.push_back({.entry = schedule, .registers = {{3, object}, {4, date}}, .memory = std::move(memory),
                         .label = label("schedule " + hex(object) + " round " + std::to_string(index) + " " + config.what),
                         .answers = manager_answers(config.found), .pokes = {{find_fixture, std::move(list)}}, .nth_answers = std::move(blocked)});
    };
    const Candidate plain{0, 1, 0, 2}, second{2, 3, 0, 2}, third{4, 5, 0, 0}, fourth{1, 6, 0, 1};
    std::vector<Config> configs;
    // One slot: the weekday (the nearest day is before or after the date), the place, the kind, with one candidate.
    for (const auto weekday : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffu})
        for (const auto kind : {0u, 1u, 2u, 3u})
            configs.push_back({"weekday " + hex(weekday) + " kind " + std::to_string(kind), {{0, weekday, 1, kind}}, {plain, second}});
    for (const auto place : {0u, 1u, 2u, 3u, 0xffu, 0x7fu, 0x80u})
        configs.push_back({"place " + hex(place), {{0, 2, place, 1}}, {plain}});
    // Slot lists: stop at the first unused, eight used, flagged words (sorted), mixed kinds.
    configs.push_back({"stops at unused", {{0, 2, 0, 1}, {0, 3, 1, 1}, {0, 3, 1, 0xff}, {0, 4, 2, 1}}, {plain, second}});
    configs.push_back({"eight slots", {{0, 0, 0, 1}, {0, 1, 1, 2}, {0, 2, 2, 0}, {0, 3, 0, 3}, {0, 4, 1, 1}, {0, 5, 2, 2}, {0, 6, 0, 0}, {0, 2, 1, 3}}, {plain, second, third, fourth}});
    configs.push_back({"flagged words sorted", {{0x8000, 2, 0, 1}, {0x40, 3, 1, 2}, {0x7fc0, 4, 2, 0}, {0x1, 5, 0, 1}}, {plain, second}});
    configs.push_back({"flag low bit only", {{0x3f, 2, 0, 1}, {0x20, 3, 1, 2}}, {plain}});
    configs.push_back({"flag high bits only", {{0x8000, 2, 0, 1}, {0x10000, 3, 1, 2}}, {plain}});
    configs.push_back({"flag at the top of the mask", {{0x4000, 2, 0, 1}, {0x7fc1, 3, 1, 2}}, {plain}});
    // The list: none, one, many; a record of another owner is skipped.
    configs.push_back({"no candidates", {{0, 2, 1, 1}}, {}});
    configs.push_back({"found none", {{0, 2, 1, 1}}, {plain}, 0});
    configs.push_back({"five candidates", {{0, 2, 1, 2}}, {plain, second, third, fourth, {3, 4, 0, 2}}});
    configs.push_back({"candidates of another owner", {{0, 2, 1, 2}}, {plain, second}, 1, {}, {}, 0, 1, 2, 1});
    configs.push_back({"one of another owner", {{0, 2, 1, 2}}, {plain, {2, 3, 1, 2}, second}});
    // Ratings (the first draw's bound), the second draw's bound (rating + 1500: zero at -1500), and the stream.
    for (const auto home : {0, 1, 2, 100, 32767, -1, -32768})
        for (const auto away : {-1500, -1499, -1501, 0, 500, 32767, -32768})
            configs.push_back({"ratings " + std::to_string(home) + " " + std::to_string(away), {{0, 2, 1, 2}}, {plain, second},
                               1, {{0, home}, {1, away}, {2, home}, {3, away}}, {}, 0, 7, 9});
    for (const auto salt : {0u, 1u, 0xffffu, 0x7fffffffu})
        for (const auto first : {0u, 1u, 0xffffu, 12345u})
            configs.push_back({"stream salt " + hex(salt) + " sample " + hex(first), {{0, 2, 1, 2}}, {plain, second, third},
                               1, {{0, 90}, {1, 30}, {2, 300}, {3, 200}, {4, 20}, {5, 700}}, {}, salt, first, 1});
    // The fixed-date kind: the competition's nation against the clubs' nations (+0x2c), absent records.
    for (const auto nation : {0, 1, 2, 3, 4})
        configs.push_back({"kind 3 nation " + std::to_string(nation), {{0, 2, 1, 3}}, {{0, 1, 0, nation}, {2, 3, 0, 2}}});
    configs.push_back({"kind 3 two clubs nation", {{0, 2, 1, 3}}, {{0, 1, 0, 0}, {2, 3, 0, 1}, {4, 5, 0, 2}}});
    // Fixtures the manager blocks (1048b180 answers the home club, then the away club).
    for (const auto& [nth, answer] : std::vector<std::pair<unsigned, std::uint32_t>>{{1, 1}, {2, 1}, {1, 0x10000}, {2, 0x10000}, {3, 1}, {4, 1}})
        configs.push_back({"blocked call " + std::to_string(nth) + " answer " + hex(answer), {{0, 2, 1, 2}}, {plain, second, third}, 1, {{0, 90}, {1, 30}, {2, 300}, {3, 200}},
                           {{nth, answer}}});
    // The league's second argument (the owner word) and date shifts around the round (before, after, far away).
    configs.push_back({"owner word", {{0, 2, 1, 2}}, {plain}, 1, {}, {}, 0, 1, 2, 0, 5});
    for (const auto shift : {1u, 2u, 7u, 0xffffu, 0x8000u})
        configs.push_back({"date shifted " + hex(shift), {{0, 2, 1, 2}}, {plain}, 1, {}, {}, 0, 1, 2, 0, 0xffffffffu, shift});
    // Which league and round: the first of each size, rounds at the start, middle and end.
    const auto picks = w.by_size(5);
    for (const auto object : picks) {
        const auto count = w.rounds(object);
        for (const auto index : {0, count / 2, count - 1})
            for (std::size_t n = 0; n < configs.size(); ++n)
                if (n % 7 == std::size_t(index % 7) || object == picks.front()) add_case(configs[n], object, index);
    }
}

// 0x104a00a0: the season setup over real leagues. The fixture manager's routines are answered (as in the schedule
// cases; the list of 0x104a28d0 stays empty), the fixtures block's 0x104a4970 and the league's virtual +0x84 run.
LEAGUE_CASES("season-setup") {
    if (scenario.match) return;
    const World w(scenario);
    const auto answers = manager_answers(1, false);
    const auto round_kind = [&](std::uint32_t object, std::int32_t index) { return w.round(object, index) + 0xe; };
    const auto picks = w.by_size(9);
    const auto special = scenario.global(first_special_slot), second_special = scenario.global(second_special_slot);
    const auto table = area + 0x1000;
    const auto add = [&](std::uint32_t object, std::vector<Write> memory, const std::string& what, std::vector<Case::NthAnswer> nth = {},
                         std::vector<std::pair<std::uint32_t, std::uint32_t>> extra = {}) {
        auto all = answers;
        all.insert(all.end(), extra.begin(), extra.end());
        cases.push_back({.entry = setup, .registers = {{3, object}}, .memory = std::move(memory),
                         .label = label("setup " + hex(object) + " clubs " + std::to_string(w.clubs(object)) + " " + what),
                         .answers = std::move(all), .nth_answers = std::move(nth)});
    };
    for (const auto object : picks) {
        add(object, {}, "plain");
        add(object, {{1, round_kind(object, 0), 3}, {1, round_kind(object, w.rounds(object) - 1), 3}}, "kind 3 rounds");
        add(object, {{4, scenario.r32(object + 4), special}}, "first special competition");
        add(object, {{4, scenario.r32(object + 4), second_special}}, "second special competition");
        add(object, {{4, object + 0x24, scenario.r32(w.club(object, 0) + 0x3c) ? scenario.r32(scenario.r32(w.club(object, 0) + 0x3c)) : 7}}, "owner word of a club's region");
        add(object, {{4, object + 0x24, 5}}, "owner word 5");
        add(object, {{1, object + 0x44, 0xff}}, "owner byte -1");
        add(object, {{2, object + 0xdc, 0x4003}, {2, object + 0x3c, 1}}, "one leg");
        add(object, {{2, object + 0xdc, 0x80}}, "flag 0x80");
        add(object, {{2, object + 0xdc, 0x01c0}, {1, object + 0xe0, 7}}, "flags 0x1c0");
        for (const auto state : {0u, 1u, 2u, 3u, 4u})
            add(object, {{2, object + 0xdc, 0}, {4, scenario.toc(cursor_slot), table}, {4, scenario.toc(limit_slot), table + 0x100},
                         {4, scenario.toc(seed_slot), state}, {2, table + 2, state}, {2, table + 4, state + 1}}, "stream state " + std::to_string(state));
        add(object, {{2, object + 0x3c, 0}}, "no legs");
        add(object, {}, "no allocation", {}, {{allocate, 0}});
        for (unsigned nth = 2; nth <= 4; ++nth) add(object, {}, "allocation " + std::to_string(nth) + " fails", {{allocate, nth, 0}});
        add(object, {{2, object + 0xa6, 0}}, "no rounds");
    }
    // Odd and small club counts on a copy of a real league.
    if (!w.leagues.empty()) {
        const auto copy = area + 0x8000;
        const auto base = object_copy(scenario, copy, picks.front());
        for (const auto n : {2, 3, 4, 5, 6, 7, 8})
            cases.push_back({.entry = setup, .registers = {{3, copy}}, .memory = joined(base, {{2, copy + 0x3e, std::uint32_t(n)}}),
                             .label = label("setup copy clubs " + std::to_string(n)), .answers = answers});
    }
    // The league's own preset fixtures: a stand-in array (a real allocation of `new`: another league object, whose
    // release 0x10773610 frees it) holding the pairs of a 4-club league's 6 rounds; and the assertion for an id that no
    // standings row has.
    for (const auto object : w.leagues) {
        if (w.clubs(object) != 4 || w.rounds(object) != 6) continue;
        const auto standin = w.leagues.back() == object ? w.leagues.front() : w.leagues.back();
        const auto array = standin + 0x10;
        const auto id_of = [&](std::uint32_t index) { return scenario.r32(w.club(object, index % 4)); };
        std::vector<Write> rows = {{2, array + 2, std::uint32_t(w.year(object))}};
        for (std::uint32_t k = 0; k < 12; ++k) {
            rows.push_back({4, array + k * 16 + 8, id_of(k)});
            rows.push_back({4, array + k * 16 + 12, id_of(k + 1 + k / 4)});
        }
        add(object, joined({{4, object + 0xe8, array}}, rows), "preset fixtures");
        auto missing = rows;
        missing.push_back({4, array + 8, 0x7ffffff0u});
        add(object, joined({{4, object + 0xe8, array}}, missing), "preset fixtures with a missing id", {}, {{0x107755c0, 0}});
        add(object, joined({{4, object + 0xe8, array}}, joined(rows, {{2, array + 2, std::uint32_t(w.year(object) + 1)}})), "preset fixtures of another year");
        break;
    }
}
}
}

// INJURY.CPP checker cases for the status block's coverage gaps: the manager's entry list (built in the scratch
// area), the injury choice's minor types, the update's statics, windows and caches, and the draws' redraws.
//
// The entry list (manager +0x28) is the game's list object: header halfword count at +0, head node +4, tail node
// +8, cursor +0xc; a node holds the row pointer at +0, the row's size (halfword) at +4, next at +8, previous at
// +0xc (0x10738e00 steps the cursor, 0x10738e60 unlinks). A row is 8 bytes: the person index, the kind byte at
// +4 and the strength byte at +5. Paths that append (0x10738d10 allocates a node) run on the game's real heap;
// removal frees the node through 0x10738ba0, whose delete (0x10773610) is answered on both sides.
#include "injury_unit_checks.hpp"
#include <algorithm>
#include <string>
#include <tuple>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t random_below = 0x101864a0, note_entry = 0x10186cd0, reset_status = 0x10186ea0,
                        match_injuries = 0x10186fc0, match_fitness_of = 0x101878d0, find_entry = 0x10186e10, update_status = 0x10187a40,
                        pick_injury_type = 0x10188930;
constexpr std::uint32_t list_at = scratch_base + 0x8000;
using Row = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;  // person index, kind, strength

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void append(const Memory& other) { writes.insert(writes.end(), other.writes.begin(), other.writes.end()); }
};
// The entry list holding `rows`, and the manager pointing at it.
Memory entry_list(std::uint32_t manager, const std::vector<Row>& rows) {
    Memory memory;
    const auto node = [&](std::size_t n) { return list_at + 0x40 + std::uint32_t(n) * 0x10; };
    const auto row_at = [&](std::size_t n) { return list_at + 0x400 + std::uint32_t(n) * 8; };
    memory.put(list_at, std::uint32_t(rows.size()), 2);
    memory.put(list_at + 4, rows.empty() ? 0 : node(0));
    memory.put(list_at + 8, rows.empty() ? 0 : node(rows.size() - 1));
    memory.put(list_at + 0xc, 0);
    for (std::size_t n = 0; n < rows.size(); ++n) {
        memory.put(node(n), row_at(n));
        memory.put(node(n) + 4, 8, 2);
        memory.put(node(n) + 8, n + 1 < rows.size() ? node(n + 1) : 0);
        memory.put(node(n) + 0xc, n > 0 ? node(n - 1) : 0);
        memory.put(row_at(n), std::get<0>(rows[n]));
        memory.put(row_at(n) + 4, std::get<1>(rows[n]), 1);
        memory.put(row_at(n) + 5, std::get<2>(rows[n]), 1);
    }
    memory.put(manager + 0x28, list_at);
    return memory;
}
std::string number(std::uint32_t value) { return std::to_string(value); }
void answer(std::vector<Case>& cases, std::size_t from, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers) {
    for (auto at = from; at < cases.size(); ++at)
        cases[at].answers.insert(cases[at].answers.end(), answers.begin(), answers.end());
}
// People with a record, spread over the table: injured and not.
struct People { std::vector<std::uint32_t> injured, healthy; };
People some_people(const Scenario& scenario, std::uint32_t manager) {
    const auto rows = scenario.r32(manager);
    People found;
    for (std::uint32_t index = 3; index < scenario.people() && (found.injured.size() < 6 || found.healthy.size() < 6); index += 13) {
        if (scenario.r32(scenario.person(index) + 0x14) == 0) continue;
        auto& list = scenario.r32(rows + index * 0x24 + 0x10) == 0xffffffff ? found.healthy : found.injured;
        if (list.size() < 6) list.push_back(index);
    }
    if (found.healthy.empty()) found.healthy.push_back(1);
    if (found.injured.empty()) found.injured.push_back(found.healthy[0]);
    return found;
}
}

INJURY_UNIT_CASES("status lists") {
    const auto manager = scenario.global(-0x7ba0);
    const auto people = some_people(scenario, manager);
    const auto key = scratch_base + 0x200, kind_cell = scratch_base + 0x210, strength_cell = scratch_base + 0x211;
    const auto history = std::pair<std::uint32_t, std::uint32_t>{0x10748ce0, 0};
    const auto free_node = std::pair<std::uint32_t, std::uint32_t>{0x10773610, 0};
    const auto free_object = std::pair<std::uint32_t, std::uint32_t>{0x107735a0, 0};

    // 0x10186cd0 on a list that has rows: the same kind raises the strength (capped at 100), a different kind
    // replaces a weaker one; a key that is not there appends.
    {
        const auto first = cases.size();
        const std::vector<Row> rows{{5, 2, 40}, {7, 3, 90}, {9, 0xf0, 0xf0}};
        for (const std::uint32_t k : {5u, 7u, 9u, 11u})
            for (const std::uint32_t kind : {2u, 3u, 0xf0u, 1u})
                for (const std::uint32_t strength : {10u, 60u, 95u, 0x7fu, 0xf0u, 0xffu}) {
                    auto memory = entry_list(manager, rows);
                    memory.put(key, k);
                    memory.put(kind_cell, kind, 1);
                    memory.put(strength_cell, strength, 1);
                    cases.push_back({.entry = note_entry, .registers = {{3, manager}, {4, key}, {5, kind_cell}, {6, strength_cell}},
                                     .memory = memory.writes,
                                     .label = "status list note key " + number(k) + " kind " + number(kind) + " strength " + number(strength)});
                }
        // The allocator fails: the new list is null and the original reads through it (a fault on both sides).
        cases.push_back({.entry = note_entry, .registers = {{3, manager}, {4, key}, {5, kind_cell}, {6, strength_cell}},
                         .memory = {{4, manager + 0x28, 0}, {4, key, 5}, {1, kind_cell, 1}, {1, strength_cell, 5}},
                         .label = "status list note, no list, allocator fails", .answers = {{0x10773520, 0}}});
        (void)first;
    }
    // 0x10186e10 on a list that holds the key (first, last) and one that does not.
    for (const std::uint32_t k : {5u, 7u, 9u, 11u}) {
        auto memory = entry_list(manager, {{5, 2, 40}, {7, 3, 90}, {9, 1, 10}});
        memory.put(key, k);
        cases.push_back({.entry = find_entry, .registers = {{3, manager}, {4, key}}, .memory = memory.writes,
                         .label = "status list find " + number(k)});
    }
    // 0x10186ea0: the person's rows leave the list (first, middle, last, twice).
    for (const auto index : {people.healthy[0], people.injured[0]}) {
        const auto first = cases.size();
        const std::uint32_t other = index + 1, third = index + 2;
        const std::vector<std::vector<Row>> lists{{{index, 2, 40}, {other, 3, 30}, {third, 1, 20}},
                                                  {{other, 3, 30}, {index, 2, 40}, {third, 1, 20}},
                                                  {{other, 3, 30}, {third, 1, 20}, {index, 2, 40}},
                                                  {{index, 2, 40}, {other, 3, 30}, {index, 4, 50}},
                                                  {{other, 3, 30}}};
        for (std::size_t n = 0; n < lists.size(); ++n)
            cases.push_back({.entry = reset_status, .registers = {{3, manager}, {4, index}},
                             .memory = entry_list(manager, lists[n]).writes,
                             .label = "status list reset " + number(index) + " list " + number(std::uint32_t(n))});
        answer(cases, first, {free_node, free_object, history});
    }

    // 0x10186fc0 with rows for the players: each kind and strength threshold, the kind-7 profile byte, both
    // option words.
    {
        const auto first = cases.size();
        const auto match = scratch_base + 0x1000;
        // The reporting person: the first with a contract list pointer (+0x1c and +0x20, not -1), which 0x10188cc0 reads through.
        std::uint32_t manager_person = scenario.person(people.healthy.back());
        for (std::uint32_t index = 0; index < scenario.people(); ++index) {
            const auto list = scenario.r32(scenario.person(index) + 0x1c), second = scenario.r32(scenario.person(index) + 0x20);
            if (list != 0 && list != 0xffffffff && second != 0 && second != 0xffffffff &&
                scenario.r32(scenario.person(index) + 0x14) != 0) {
                manager_person = scenario.person(index);
                break;
            }
        }
        const std::uint32_t kinds[] = {1, 7, 3, 0, 4, 2, 0x20, 0x40, 0x7f, 0xff};
        const std::uint32_t strengths[] = {0x70, 0x50, 0x40, 0x20};
        for (const std::uint32_t first_word : {0xfffffffeu, 0u})
            for (const auto kind : kinds)
                for (const auto strength : strengths)
                    for (const std::uint32_t profile_byte : {10u, 20u}) {
                        if (kind != 7 && profile_byte == 20) continue;
                        const auto person_a = people.healthy[0], person_b = people.injured[0];
                        Memory memory = entry_list(manager, {{person_a, kind, strength}, {person_b, kind, strength}});
                        // The -2 form is a scratch object; the other is a real person record (0x10188cc0 reads through it).
                        const auto options = first_word == 0xfffffffeu ? scratch_base + 0x800 : manager_person;
                        if (first_word == 0xfffffffeu) memory.put(options, first_word);
                        memory.put(options + 0x40, 1, 1);
                        memory.put(match + 0x4c, 0, 1);
                        for (unsigned side = 0; side < 2; ++side)
                            for (unsigned slot = 0; slot < 20; ++slot) {
                                const auto entry = match + side * 0x664 + slot * 0x4c;
                                const bool used = side == 0 && slot < 2;
                                memory.put(entry + 0x194, used ? (slot == 0 ? person_a : person_b) : 0xffffffffu);
                                memory.put(entry + 0x1b3, 1, 1);
                                memory.put(entry + 0x1c4, 30, 1);
                                memory.put(entry + 0x1c6, 5, 1);
                                memory.put(entry + 0x1c8, 40, 1);
                                memory.put(entry + 0x1ca, 3000, 2);
                            }
                        for (const auto person : {person_a, person_b})
                            memory.put(scenario.r32(scenario.person(person) + 0x14) + 0xf, profile_byte, 1);
                        cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}},
                                         .memory = memory.writes,
                                         .label = "status list match kind " + number(kind) + " strength " + number(strength) +
                                                  " option " + number(first_word) + " byte " + number(profile_byte)});
                    }
        answer(cases, first, {free_node, free_object, history});
    }
    // 0x10186fc0 on a match played after today (the rating counts for nothing), without the people table (a null
    // person), and a first-eleven slot whose two rating bytes are both negative.
    {
        const auto first = cases.size();
        const auto options = scratch_base + 0x800, match = scratch_base + 0x1000;
        const auto match_date = scenario.toc(-0x7a9c), today = scenario.toc(-0x7b80);
        const auto year = std::uint32_t(scenario.s.memory.read<2>(today + 2));
        const auto person_a = people.healthy[0];
        const auto base_memory = [&](std::uint32_t person, std::uint32_t full, unsigned used_slot = 4, std::uint32_t z = 0xf1) {
            Memory memory;
            memory.put(options, 0);
            memory.put(options + 0x40, full, 1);
            memory.put(match + 0x4c, 0, 1);
            memory.put(match + 0x4e, 0x0a37, 2);
            for (unsigned side = 0; side < 2; ++side)
                for (unsigned slot = 0; slot < 20; ++slot) {
                    const auto entry = match + side * 0x664 + slot * 0x4c;
                    memory.put(entry + 0x194, side == 0 && slot == used_slot ? person : 0xffffffffu);
                    memory.put(entry + 0x1b3, 0, 1);
                    memory.put(entry + 0x1c4, 0xf0, 1);
                    memory.put(entry + 0x1c6, z, 1);
                    memory.put(entry + 0x1c8, 0xf0, 1);
                    memory.put(entry + 0x1ca, 4000, 2);
                }
            return memory;
        };
        for (const std::uint32_t full : {0u, 1u}) {
            auto memory = base_memory(person_a, full);
            memory.put(match_date + 2, year + 1, 2);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match before the match day, full " + number(full)});
            memory = base_memory(person_a, full);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match both rating bytes negative, full " + number(full)});
            // Today after the match day (the year part below today's); a reserve (slot 12) whose first rating byte is -1;
            // a person whose own index is past the people count.
            memory = base_memory(person_a, full);
            memory.put(match_date + 2, year - 1, 2);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match after the match day, full " + number(full)});
            memory = base_memory(person_a, full, 12, 0xff);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match reserve without a rating, full " + number(full)});
            memory = base_memory(person_a, full, 12, 0x05);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match reserve, full " + number(full)});
            memory = base_memory(person_a, full);
            memory.put(scenario.person(person_a), scenario.people());
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match person past the table, full " + number(full)});
            // The person table pointer is null and the slot names person 0: a null person (set_sharpness returns).
            memory = base_memory(0, 0);
            memory.put(scenario.toc(-0x7be0), 0);
            cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory.writes,
                             .label = "status list match null person"});
        }
        answer(cases, first, {free_node, free_object, history});
    }

    // 0x10188930: a current injury of each minor type is chosen again; a range of 0 in the type table.
    {
        const auto first = cases.size();
        const auto rows = scenario.r32(manager);
        const auto injuries = scenario.r32(manager + 4);
        const auto types = scenario.toc(-0x64d4), ranges = scenario.toc(-0x64d0);
        for (const auto index : people.injured) {
            const auto row = rows + index * 0x24;
            if (scenario.r32(row + 0x10) == 0xffffffff) continue;
            const auto record = injuries + scenario.r32(row + 0x10) * 0x38;
            for (const std::uint32_t type : {0u, 1u, 2u, 3u, 4u, 0x4cu, 0x4du, 0x4eu, 0x4fu, 0x50u, 0x51u})
                cases.push_back({.entry = pick_injury_type,
                                 .registers = {{3, manager}, {4, 1}, {5, 0xffffffff}, {6, scenario.person(index)}},
                                 .memory = {{1, record, type}}, .label = "status list pick current type " + number(type)});
        }
        for (const std::uint32_t kind : {1u, 2u, 3u}) {
            const auto begin = std::uint32_t(scenario.s.memory.read<1>(ranges + kind * 3 + 1));
            cases.push_back({.entry = pick_injury_type,
                             .registers = {{3, manager}, {4, kind}, {5, 0xffffffff}, {6, scenario.person(people.healthy[0])}},
                             .memory = {{1, types + std::uint32_t(std::int32_t(std::int8_t(begin))) * 12 + 2, 0}},
                             .label = "status list pick kind " + number(kind) + " first type with range 0"});
        }
        answer(cases, first, {history});
    }

    // 0x101878d0 as a percentage with a result over 100.
    for (const auto index : {people.healthy[0], people.healthy.back()}) {
        const auto row = scenario.r32(manager) + index * 0x24;
        cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, scenario.person(index)}, {5, 1}},
                         .memory = {{2, row + 8, 15000}, {2, row + 0xa, 15000}},
                         .label = "status list match fitness over 100 percent " + number(index)});
    }

    // 0x101864a0: the second level's draw redraws.
    {
        const auto cursor_cell = scenario.toc(-0x7b84);
        const auto last = scenario.global(-0x7b88);
        for (const std::uint32_t n : {0x12345u, 0x10000u, 0x7fffffffu, 0xfffffffeu, 0x80000001u})
            for (const std::uint32_t back : {2u, 4u})
                cases.push_back({.entry = random_below, .registers = {{3, n}}, .memory = {{4, cursor_cell, last - back}},
                                 .label = "status list random below " + number(n) + " redraw at draw " + number(back / 2)});
    }

    // 0x10187a40: the first-use guards of its statics, the window dates, the caches, a row dated in the future.
    {
        const auto first = cases.size();
        const std::uint32_t r2 = 0x108bba90;
        const auto today = scenario.toc(-0x7b80);
        const auto today_day = std::uint32_t(scenario.s.memory.read<2>(today));
        const auto today_year = std::uint32_t(scenario.s.memory.read<2>(today + 2));
        const auto rows = scenario.r32(manager);
        const auto window_a = scenario.toc(-0x5450), window_b = scenario.toc(-0x544c), cache = scenario.toc(-0x5448);
        const auto dated = [&](std::uint32_t row, std::uint32_t days) {
            return std::vector<Write>{{2, row, today_day - std::min(days, today_day)}, {2, row + 2, today_year}};
        };
        const auto join = [](std::vector<Write> a, const std::vector<Write>& b) {
            a.insert(a.end(), b.begin(), b.end());
            return a;
        };
        const auto person_index = people.healthy[0];
        const auto person = scenario.person(person_index);
        const auto row = rows + person_index * 0x24;
        // First use of each static (the guard bytes at r2 - 0x3947, -0x3948, -0x3949, -0x394a, -0x394c).
        for (const std::int32_t guard : {-0x3947, -0x3948, -0x3949, -0x394a, -0x394c})
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}},
                             .memory = join(dated(row, 3), {{1, r2 + std::uint32_t(guard), 0}}),
                             .label = "status list update first use " + number(std::uint32_t(-guard))});
        {
            auto memory = dated(row, 3);
            for (const std::int32_t guard : {-0x3947, -0x3948, -0x3949, -0x394a, -0x394c}) memory.push_back({1, r2 + std::uint32_t(guard), 0});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                             .label = "status list update first use of all"});
        }
        // The statics of 0x10187a40 are all unset in the snapshot, so every call constructs the window dates afresh
        // (0x10715880: 28 January 1972) and resets the cached club. The cases below mark the statics as set, so
        // the dates they write stay.
        std::vector<Write> set_statics;
        for (const std::int32_t guard : {-0x3947, -0x3948, -0x3949, -0x394a, -0x394c}) set_statics.push_back({1, r2 + std::uint32_t(guard), 1});
        // A date `back` days before today (a day of this year), for both windows: the days from the first window
        // to today and from today to the second are those numbers (0x10718130 is a span).
        const auto window = [&](std::uint32_t at, std::uint32_t back) {
            return std::vector<Write>{{2, at, today_day - std::min(back, today_day)}, {2, at + 2, today_year}};
        };
        const std::uint32_t spans[] = {0, 3, 10, 12, 14, 15, 16, 20, 100, 300};
        for (const auto a : spans)
            for (const auto b : spans)
                for (const std::uint32_t age : {3u, 25u}) {
                    auto memory = join(join(join(dated(row, age), set_statics), join(window(window_a, a), window(window_b, b))),
                                       {{2, row + 8, 5000}, {2, row + 0xa, 5000}});
                    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                                     .label = "status list update windows " + number(a) + " " + number(b) + " age " + number(age),
                                     .answers = {{0x101929b0, 0}}});
                    // The club check cached for today: the flag stays set and the call is not made.
                    memory.push_back({1, r2 - 0x394b, 1});
                    memory.push_back({4, r2 - 0x3944, scenario.r32(person + 0x24)});
                    memory.push_back({2, cache, today_day});
                    memory.push_back({2, cache + 2, today_year});
                    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                                     .label = "status list update windows cached " + number(a) + " " + number(b) + " age " + number(age)});
                }
        // The cached club on another date: another year; the same year and another day.
        for (const bool other_year : {true, false}) {
            auto memory = join(join(dated(row, 5), set_statics), {{4, r2 - 0x3944, scenario.r32(person + 0x24)},
                                                                  {2, cache, today_day + (other_year ? 0 : 1)},
                                                                  {2, cache + 2, today_year + (other_year ? 1 : 0)}});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                             .label = std::string("status list update cached club, other ") + (other_year ? "year" : "day"),
                             .answers = {{0x101929b0, 0}}});
        }
        // A row whose day part is past the year's end, in the year before today: the span is negative (0x10717dd0
        // adds a year's days back to a negative day difference), so the drain count is negative and the fitness
        // overflows a halfword. Dates in the game are within 0..365, so only this fabricated one reaches it.
        for (const std::uint32_t day : {30000u, 3000u})
            for (const std::uint32_t after : {10u, 100u}) {
                auto memory = join(join(set_statics, window(window_a, 10)), window(window_b, after));
                memory.push_back({2, row, day});
                memory.push_back({2, row + 2, today_year - 1});
                cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                                 .label = "status list update row day " + number(day) + " year before, window " + number(after),
                                 .answers = {{0x101929b0, 0}}});
            }
        answer(cases, first, {history});
    }
}
}

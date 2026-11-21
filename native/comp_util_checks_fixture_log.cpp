// COMP_UTIL.CPP, block `fixture_log`: cases for the history log (the global table of 0x1c-byte rows: the competition's
// record at +4, the winner, runner-up, third and host clubs, the year) and its four routines: the appender
// (0x10463990), the lookup (0x10463a90), the club collector (0x104770c0) and the collector's comparator (0x10477270).
//
// The log is the career's real one (about 6,600 rows, sorted by the lookups' earlier sorts, with no empty row). What a
// branch needs that the career lacks is written over the real table in a case's own memory (undone afterwards): empty
// trailing rows for the appender's reuse, a table that is all empty (the original grows it anyway), a shorter count for
// the lookup's sort. The comparator's rows and the competition records they name are staged in the scratch area (this
// block's area is scratch_base + 0xa0000..0xb0000), the records copied from a real one with the fields the comparator
// reads patched (continent +4, nation +8, reputation +0x18, id +0: the ids of competitions the game counts as current,
// one that it does not, and a shared id), the clubs real. The context clubs the comparator reads are two words
// below the TOC (r2 - 0x3768, r2 - 0x376c), written per case. Allocation failures are answered on both sides at the
// allocator's realloc (0x107751b0, whose owner runs its original for those cases), for the appender's growth and for
// the collector's first and third blocks. The assertion paths (an unknown place, the collector's failed
// allocation) are answered by the checker's scripted dialog.
// Labels start with "fixture_log:" (CM_COMP_UTIL_BLOCK=fixture_log).
#include "comp_util_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t append_row = 0x10463990, club_of_place = 0x10463a90, collect = 0x104770c0, compare_rows = 0x10477270;
constexpr std::uint32_t resize = 0x107751b0;
constexpr std::int32_t rows_slot = -0x7a34, count_slot = -0x7a38, club_count_slot = -0x7b50,
                       records_slot = -0x7bbc, objects_slot = -0x7b5c, object_count_slot = -0x7b60,
                       special_a = -0x6888, special_b = -0x67bc;
constexpr std::uint32_t real_toc = 0x108bba90, row_size = 0x1c;
constexpr std::uint32_t area = scratch_base + 0xa0000;
constexpr std::uint32_t record_at = area, rows_at = area + 0x1000, out_at = area + 0x8000;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
}

COMP_UTIL_CASES("fixture_log") {
    // The comparator: ordered pairs of staged rows, with the context clubs both set and with the second unset.
    State& s = scenario.s;
    const auto saved_observer = s.memory.observer;
    s.memory.observer = nullptr;
    const auto record_table = scenario.global(records_slot);
    const auto competition_count = std::int32_t(scenario.global(object_count_slot));
    // Ids: the two the game always counts as current, and one whose record has no nation (never current).
    const std::uint32_t current_a = scenario.global(special_a), current_b = scenario.global(special_b);
    std::uint32_t other = 0;
    for (std::int32_t id = 1; id < competition_count; ++id)
        if (scenario.r32(record_table + std::uint32_t(id) * 0x6c + 8) == 0) { other = std::uint32_t(id); break; }
    struct Profile { std::uint32_t id, continent, nation, reputation; const char* what; };
    const Profile profiles[] = {
        {current_a, 1, 1, 100, "current a"},      {current_b, 1, 1, 100, "current b"},     {current_b, 1, 1, 100, "current b again"},
        {other, 1, 1, 100, "not current"},        {current_a, 1, 1, 50, "current a, lower"}, {current_a, 1, 1, 100, "current a, equal"},
        {current_b, 0, 1, 100, "no continent"},   {current_b, 1, 0, 100, "no nation"},      {current_b, 0, 0, 100, "neither"},
        {other, 1, 0, 7, "not current, no nation"},
    };
    std::vector<std::uint32_t> records;
    for (std::size_t n = 0; n < std::size(profiles); ++n) {
        const auto at = record_at + std::uint32_t(n) * 0x80;
        for (std::uint32_t offset = 0; offset < 0x6c; offset += 4) s.memory.write<4>(at + offset, scenario.r32(record_table + offset));
        s.memory.write<4>(at + 0, profiles[n].id);
        s.memory.write<4>(at + 4, profiles[n].continent);
        s.memory.write<4>(at + 8, profiles[n].nation);
        s.memory.write<2>(at + 0x18, profiles[n].reputation);
        records.push_back(at);
    }
    // The clubs of the rows: real club rows (the context's first and second, a third and an empty column).
    const std::uint32_t club_x = scenario.club(3), club_y = scenario.club(11), club_z = scenario.club(40);
    struct Placement { std::uint32_t winner, runner_up, third; const char* what; };
    const Placement placements[] = {
        {club_x, club_z, 0, "x won"}, {club_z, club_x, 0, "x second"}, {club_z, 0, club_x, "x third"},
        {club_z, club_z, club_z, "neither"}, {club_y, club_z, 0, "y won"}, {club_z, club_y, 0, "y second"},
    };
    std::vector<std::pair<std::uint32_t, std::string>> rows;
    for (std::size_t r = 0; r < records.size(); ++r)
        for (const auto& placement : placements)
            for (const std::uint32_t year : {2001u, 2003u}) {
                const auto at = rows_at + std::uint32_t(rows.size()) * 0x20;
                s.memory.write<4>(at, std::uint32_t(rows.size()));
                s.memory.write<4>(at + 4, records[r]);
                s.memory.write<4>(at + 8, placement.winner);
                s.memory.write<4>(at + 0xc, placement.runner_up);
                s.memory.write<4>(at + 0x10, placement.third);
                s.memory.write<4>(at + 0x14, club_z);
                s.memory.write<2>(at + 0x18, year);
                rows.emplace_back(at, std::string(profiles[r].what) + ", " + placement.what + ", " + std::to_string(year));
            }
    // Rows with no competition record at all (+4 of 0) are never compared (the collector skips them), but the
    // comparator's first loads would follow the null: they are left out.
    s.memory.observer = saved_observer;
    const auto cell = [&](std::int32_t offset) { return real_toc + std::uint32_t(offset); };
    // All pairs over a thinned set (every third row) with the second context club unset, and a quarter of the rows
    // with it set (a club that placed second in some rows).
    for (std::size_t a = 0; a < rows.size(); a += 1)
        for (std::size_t b = 0; b < rows.size(); b += 1) {
            if ((a * 7 + b * 3) % 5 != 0 && (a / 12 != b / 12)) continue;  // the pairs of different records are thinned
            cases.push_back({.entry = compare_rows, .registers = {{3, rows[a].first}, {4, rows[b].first}},
                             .memory = {w32(cell(-0x3768), club_x), w32(cell(-0x376c), 0)},
                             .label = "fixture_log: compare " + rows[a].second + " with " + rows[b].second + " (context x)"});
            if ((a + b) % 3 == 0)
                cases.push_back({.entry = compare_rows, .registers = {{3, rows[a].first}, {4, rows[b].first}},
                                 .memory = {w32(cell(-0x3768), club_x), w32(cell(-0x376c), club_y)},
                                 .label = "fixture_log: compare " + rows[a].second + " with " + rows[b].second + " (context x and y)"});
        }
    // The context club unset (0) too, and both equal.
    for (std::size_t a = 0; a < rows.size(); a += 7)
        for (std::size_t b = 0; b < rows.size(); b += 5)
            cases.push_back({.entry = compare_rows, .registers = {{3, rows[a].first}, {4, rows[b].first}},
                             .memory = {w32(cell(-0x3768), 0), w32(cell(-0x376c), 0)},
                             .label = "fixture_log: compare " + rows[a].second + " with " + rows[b].second + " (no context)"});
}

COMP_UTIL_CASES("fixture_log") {
    // The lookup: keys (competitions the log holds, the first, a middle one and the last; competitions it does
    // not hold; no record) against the places the callers ask for and the ones that fail, with the full table
    // (which the lookup sorts again) and with shorter counts.
    const auto rows = scenario.global(rows_slot);
    const auto total = std::int32_t(scenario.global(count_slot));
    const auto record_table = scenario.global(records_slot);
    const auto competition_count = std::int32_t(scenario.global(object_count_slot));
    std::vector<std::uint32_t> held;  // the distinct records the rows name, in table order
    std::set<std::uint32_t> seen;
    for (std::int32_t n = 0; n < total; ++n) {
        const auto record = scenario.r32(rows + std::uint32_t(n) * row_size + 4);
        if (record && seen.insert(record).second) held.push_back(record);
    }
    std::vector<std::pair<std::uint32_t, std::string>> keys;
    if (!held.empty()) {
        keys.emplace_back(held.front(), "the first competition");
        keys.emplace_back(held[held.size() / 2], "a middle competition");
        keys.emplace_back(held.back(), "the last competition");
    }
    for (std::int32_t id = 0, found = 0; id < competition_count && found < 2; ++id) {
        const auto record = record_table + std::uint32_t(id) * 0x6c;
        if (seen.count(record) == 0) { keys.emplace_back(record, "an unlisted competition " + std::to_string(id)); ++found; }
    }
    keys.emplace_back(0, "no record");
    const std::uint32_t places[] = {0, 1, 2, 0xffffffffu, 3, 0x80, 0x7f, 0x102, 0xffffff01u, 0x100};
    for (const auto& [record, what] : keys)
        for (const auto place : places)
            cases.push_back({.entry = club_of_place, .registers = {{3, record}, {4, place}}, .label = "fixture_log: place " + hex(place) + " of " + what});
    // Shorter tables: the lookup sorts only the first rows. A count of 1 or 2 puts the search at the table's edges.
    for (const std::int32_t count : {0, 1, 2, 3, 7, 60, 500})
        for (const auto& [record, what] : keys)
            for (const std::uint32_t place : {0u, 1u, 2u, 5u})
                cases.push_back({.entry = club_of_place, .registers = {{3, record}, {4, place}},
                                 .memory = {w32(scenario.toc(count_slot), std::uint32_t(count))},
                                 .label = "fixture_log: place " + hex(place) + " of " + what + " in the first " + std::to_string(count) + " rows"});
}

COMP_UTIL_CASES("fixture_log") {
    // The collector: clubs that placed often, rarely and never, a second club, equal clubs, and club 0 (which
    // matches every empty column).
    const auto rows = scenario.global(rows_slot);
    const auto total = std::int32_t(scenario.global(count_slot));
    std::map<std::uint32_t, unsigned> placed;
    for (std::int32_t n = 0; n < total; ++n) {
        const auto row = rows + std::uint32_t(n) * row_size;
        if (!scenario.r32(row + 4)) continue;
        for (const std::uint32_t at : {8u, 0xcu, 0x10u}) if (const auto club = scenario.r32(row + at)) ++placed[club];
    }
    std::vector<std::pair<unsigned, std::uint32_t>> by_count;
    for (const auto& [club, count] : placed) by_count.emplace_back(count, club);
    std::sort(by_count.begin(), by_count.end());
    std::uint32_t never = 0;
    const auto club_total = scenario.global(club_count_slot);
    for (std::uint32_t n = 0; n < club_total; ++n)
        if (placed.count(scenario.club(n)) == 0) { never = scenario.club(n); break; }
    struct Pick { std::uint32_t club; std::string what; };
    std::vector<Pick> picks;
    if (!by_count.empty()) {
        picks.push_back({by_count.back().second, "the club with the most places (" + std::to_string(by_count.back().first) + ")"});
        picks.push_back({by_count[by_count.size() * 3 / 4].second, "a club with several places"});
        picks.push_back({by_count[by_count.size() / 2].second, "a club with some places"});
        picks.push_back({by_count.front().second, "a club with one place"});
    }
    if (never) picks.push_back({never, "a club that never placed"});
    picks.push_back({0, "club 0"});
    const std::uint32_t out = out_at;
    for (const auto& first : picks)
        for (const auto& second : picks) {
            cases.push_back({.entry = collect, .registers = {{3, first.club}, {4, second.club}, {5, out}},
                             .label = "fixture_log: collect " + first.what + " and " + second.what});
        }
    for (const auto& first : picks)
        cases.push_back({.entry = collect, .registers = {{3, first.club}, {4, first.club}, {5, out}}, .label = "fixture_log: collect " + first.what + " twice"});
    // The collected block is grown in steps of ten rows: a failure of the first, the second and the third growth.
    if (!by_count.empty()) {
        const auto busy = by_count.back().second, medium = by_count[by_count.size() * 3 / 4].second;
        for (const unsigned nth : {1u, 2u, 3u})
            cases.push_back({.entry = collect, .registers = {{3, busy}, {4, medium}, {5, out}},
                             .label = "fixture_log: collect, growth " + std::to_string(nth) + " fails", .nth_answers = {{resize, nth, 0}},
                             .original_owners = {resize}});
        // The club that has a few places and a shorter table: the first growth's size, then none.
        cases.push_back({.entry = collect, .registers = {{3, medium}, {4, 0}, {5, out}}, .memory = {w32(scenario.toc(count_slot), 40)},
                         .label = "fixture_log: collect from the first 40 rows"});
    }
    // Empty rows in the middle of the table are skipped (the real table has none): the busiest club's rows among
    // them, and a second club that placed third only (a row where the first club is not in the row at all).
    if (!by_count.empty()) {
        std::vector<Write> emptied;
        for (std::int32_t n = 0; n < 300 && n < total; ++n) emptied.push_back(w32(rows + std::uint32_t(n) * row_size + 4, 0));
        std::uint32_t third_only = 0;
        for (std::int32_t n = 0; n < total && !third_only; ++n)
            if (const auto club = scenario.r32(rows + std::uint32_t(n) * row_size + 0x10); club && club != never) third_only = club;
        cases.push_back({.entry = collect, .registers = {{3, by_count.back().second}, {4, 0}, {5, out}}, .memory = emptied,
                         .label = "fixture_log: collect with the first 300 rows empty"});
        if (never && third_only)
            cases.push_back({.entry = collect, .registers = {{3, never}, {4, third_only}, {5, out}},
                             .label = "fixture_log: collect a club that never placed with a club that came third"});
    }
    cases.push_back({.entry = collect, .registers = {{3, never ? never : 0}, {4, 0}, {5, out}}, .memory = {w32(scenario.toc(count_slot), 0)},
                     .label = "fixture_log: collect from an empty table"});
    cases.push_back({.entry = collect, .registers = {{3, never ? never : 0}, {4, 0}, {5, out}}, .memory = {w32(scenario.toc(count_slot), 0xffffffffu)},
                     .label = "fixture_log: collect from a table of negative count"});
}

COMP_UTIL_CASES("fixture_log") {
    // The appender: the real table (last row used: the table grows), trailing empty rows (the first of them is
    // reused), an empty run that is not trailing (grows), a table that is all empty (grows: the original finds no
    // used row to reuse after), a failing growth.
    const auto rows = scenario.global(rows_slot);
    const auto total = std::int32_t(scenario.global(count_slot));
    const auto object_table = scenario.global(objects_slot);
    const auto object_count = std::int32_t(scenario.global(object_count_slot));
    std::vector<std::uint32_t> objects;
    for (std::int32_t id = 0; id < object_count && objects.size() < 3; ++id)
        if (const auto object = scenario.r32(object_table + std::uint32_t(id) * 4)) objects.push_back(object);
    if (objects.empty() || total < 30) return;
    const std::uint32_t club_x = scenario.club(5), club_y = scenario.club(6), club_z = scenario.club(7), club_w = scenario.club(8);
    const auto emptied = [&](std::int32_t first, std::int32_t last) {
        std::vector<Write> memory;
        for (std::int32_t n = first; n < last; ++n) memory.push_back(w32(rows + std::uint32_t(n) * row_size + 4, 0));
        return memory;
    };
    for (const auto object : objects) {
        const std::string what = " for the object at " + hex(object);
        const std::vector<std::pair<std::string, std::vector<Write>>> states = {
            {"the real table", {}},
            {"one trailing empty row", emptied(total - 1, total)},
            {"two trailing empty rows", emptied(total - 2, total)},
            {"twenty trailing empty rows", emptied(total - 20, total)},
            {"an empty run before the last row", emptied(total - 10, total - 3)},
            {"every row empty", emptied(0, total)},
            {"the last row only used", emptied(0, total - 1)},
            {"a row after the first empty", emptied(1, 2)},
        };
        for (const auto& [state, memory] : states)
            cases.push_back({.entry = append_row, .registers = {{3, object}, {4, club_x}, {5, club_y}, {6, club_z}, {7, club_w}}, .memory = memory,
                             .label = "fixture_log: append " + state + what});
    }
    // The row's other words are stored whatever they are (clubs unset or all ones), and the empty table cases.
    cases.push_back({.entry = append_row, .registers = {{3, objects[0]}, {4, 0}, {5, 0xffffffffu}, {6, 0x80000000u}, {7, 1}}, .memory = emptied(total - 3, total),
                     .label = "fixture_log: append with odd club words, reusing"});
    cases.push_back({.entry = append_row, .registers = {{3, objects[0]}, {4, club_x}, {5, club_y}, {6, club_z}, {7, club_w}},
                     .memory = {w32(scenario.toc(count_slot), 0)}, .label = "fixture_log: append to a table of no rows"});
    cases.push_back({.entry = append_row, .registers = {{3, objects[0]}, {4, club_x}, {5, club_y}, {6, club_z}, {7, club_w}},
                     .memory = {w32(scenario.toc(count_slot), 0xffffffffu)}, .label = "fixture_log: append to a table of negative count"});
    // The growth fails: nothing is written, whatever the table held.
    cases.push_back({.entry = append_row, .registers = {{3, objects[0]}, {4, club_x}, {5, club_y}, {6, club_z}, {7, club_w}},
                     .label = "fixture_log: append, the growth fails", .answers = {{resize, 0}}, .original_owners = {resize}});
    cases.push_back({.entry = append_row, .registers = {{3, objects[0]}, {4, club_x}, {5, club_y}, {6, club_z}, {7, club_w}}, .memory = emptied(0, total),
                     .label = "fixture_log: append to an all-empty table, the growth fails", .answers = {{resize, 0}}, .original_owners = {resize}});
}
}

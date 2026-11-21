// PLAYER_REGEN.CPP, block `regen_queries`: cases for the person class (0x102de3c0), the retirement-entry search
// (0x102de3f0), the staff-pool word (0x102dbe90), the two qsort comparators (0x102eb4e0, 0x102eb550) and the
// queued test (0x102eb610).
//
// The career's real people and kind table are the starting point (a captured state: season-day): the class query runs
// on real people and on one real person with the class byte written to every value; the list routines run on lists the
// case builds in the scratch area from real person ids (a list is {+4 the entries, +8 the count}, a retirement entry 0x1c
// bytes, a queue entry 0xc bytes), with the edges the career's lists do not take: empty, negative and past-the-end
// counts, keys first, last, absent and duplicated, both selectors of the search with every byte value that matters, null
// elements of the comparators, every tie of their keys and the current date's edges (halfword days of either sign,
// years around the current one). Labels start with "regen_queries:" (CM_PLAYER_REGEN_BLOCK=regen_queries).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t person_class = 0x102de3c0, find_entry = 0x102de3f0, pool_word = 0x102dbe90, compare_queue = 0x102eb4e0,
                        compare_retirement = 0x102eb550, queued = 0x102eb610;
constexpr std::int32_t kind_table_slot = -0x7b9c, date_slot = -0x7b80;
constexpr std::uint32_t area = scratch_base + 0x10000;
constexpr std::uint32_t list_at = area, entries_at = area + 0x100, key_at = area + 0x4000, first_at = area + 0x4100,
                        second_at = area + 0x4200, object_at = area + 0x4300;
constexpr std::uint32_t entry_size = 0x1c;

// The ids of real people spread over the table (a list's entries and keys).
std::vector<std::uint32_t> real_ids(const Scenario& s, std::size_t wanted) {
    std::vector<std::uint32_t> ids;
    const auto people = s.people();
    for (std::size_t n = 0; n < wanted && people > 16; ++n) ids.push_back(s.r32(s.person(std::uint32_t((n * 7919 + 13) % (people - 16)))));
    return ids;
}

struct Row { std::uint32_t id, kind, announced; };
// A list object over the rows at the scratch area, `count` as the list's count word (it may differ from the rows).
std::vector<Write> list_of(const std::vector<Row>& rows, std::int32_t count) {
    std::vector<Write> memory{w32(list_at + 4, entries_at), w32(list_at + 8, std::uint32_t(count))};
    for (std::size_t n = 0; n < rows.size(); ++n) {
        const auto at = entries_at + std::uint32_t(n) * entry_size;
        memory.push_back(w32(at, rows[n].id));
        memory.push_back(w8(at + 0x18, rows[n].kind));
        memory.push_back(w8(at + 0x19, rows[n].announced));
    }
    return memory;
}
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102de3c0 on the career's people: the first and last few, and every 97th; r3 is ignored by the routine.
    const auto table = scenario.global(kind_table_slot);
    const auto people = scenario.people();
    std::set<std::uint32_t> classes;
    std::vector<std::uint32_t> picks;
    for (std::uint32_t n = 0; n < people; ++n)
        if (n < 40 || n + 40 >= people || n % 97 == 0) picks.push_back(n);
    for (const auto n : picks) {
        const auto record = scenario.person(n);
        classes.insert((scenario.u8(table + scenario.r32(record) * 100 + 0xc) >> 2) & 3);
        cases.push_back({.entry = person_class, .registers = {{3, record}, {4, record}}, .label = "regen_queries: class of person " + hex(n)});
    }
    std::fprintf(stderr, "regen_queries: %zu real people, classes seen:", picks.size());
    for (const auto value : classes) std::fprintf(stderr, " %u", value);
    std::fprintf(stderr, "\n");
    // The class byte written to every value on one real person: bits 2..3 are the class, the others must not matter.
    const auto person = scenario.person(people / 2);
    const auto byte_at = table + scenario.r32(person) * 100 + 0xc;
    for (std::uint32_t value = 0; value < 256; ++value)
        cases.push_back({.entry = person_class, .registers = {{3, 0}, {4, person}}, .memory = {w8(byte_at, value)},
                         .label = "regen_queries: class byte " + hex(value)});
    // Ids from the edges of the real ones: a word the case builds holds the id.
    for (const std::uint32_t id : {0u, 1u, scenario.r32(scenario.person(0)), scenario.r32(scenario.person(people - 1))})
        cases.push_back({.entry = person_class, .registers = {{3, 1}, {4, key_at}}, .memory = {w32(key_at, id)},
                         .label = "regen_queries: class of id " + hex(id)});
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102de3f0: the first entry with the key's id, then the selector. Rows are ids of real people.
    const auto ids = real_ids(scenario, 6);
    if (ids.size() < 6) throw std::runtime_error("regen_queries: the career has too few people");
    constexpr std::array<std::uint32_t, 5> kinds{0, 7, 8, 9, 0x88}, flags{0, 1, 2, 0x80, 0xff};
    // The register r5 is a char: 0x100 and 0xff80 have a low byte of 0 and 0x80.
    constexpr std::array<std::uint32_t, 6> selectors{0, 1, 8, 0x100, 0xff80, 0xffffffff};
    struct Where { const char* name; int row; };
    for (const std::int32_t count : {0, 1, 3, 5, 6, -2})
        for (const Where where : {Where{"first", 0}, Where{"last", 4}, Where{"absent", -1}})
            for (const auto selector : selectors)
                for (const auto kind : kinds)
                    for (const auto flag : flags) {
                        std::vector<Row> rows;
                        for (std::uint32_t n = 0; n < 5; ++n) rows.push_back({ids[n], 7, 0});
                        // The wanted row carries the combination; the others fail both tests.
                        auto key = ids[5];
                        if (where.row >= 0) { rows[std::size_t(where.row)].kind = kind; rows[std::size_t(where.row)].announced = flag; key = ids[std::size_t(where.row)]; }
                        auto memory = list_of(rows, count);
                        memory.push_back(w32(key_at, key));
                        cases.push_back({.entry = find_entry, .registers = {{3, list_at}, {4, key_at}, {5, selector}}, .memory = memory,
                                         .label = "regen_queries: find " + std::string(where.name) + " count " + std::to_string(count) + " selector " + hex(selector) +
                                                  " kind " + hex(kind) + " flag " + hex(flag)});
                    }
    // Past the end: the count names a row after the five the case wrote, whose id word is 0 (the key 0 finds it).
    {
        auto memory = list_of({{ids[0], 8, 1}, {ids[1], 8, 1}}, 4);
        memory.push_back(w32(key_at, 0));
        memory.push_back(w32(entries_at + 2 * entry_size, 0));
        memory.push_back(w8(entries_at + 2 * entry_size + 0x18, 8));
        memory.push_back(w8(entries_at + 2 * entry_size + 0x19, 1));
        for (const std::uint32_t selector : {0u, 1u})
            cases.push_back({.entry = find_entry, .registers = {{3, list_at}, {4, key_at}, {5, selector}}, .memory = memory,
                             .label = "regen_queries: find a zero id past the written rows, selector " + hex(selector)});
    }
    // A duplicated id: the first match decides even when a later one would pass.
    for (const std::uint32_t selector : {0u, 1u}) {
        auto memory = list_of({{ids[0], 7, 0}, {ids[0], 8, 1}, {ids[1], 8, 1}}, 3);
        memory.push_back(w32(key_at, ids[0]));
        cases.push_back({.entry = find_entry, .registers = {{3, list_at}, {4, key_at}, {5, selector}}, .memory = memory,
                         .label = "regen_queries: find a duplicated id, first fails, selector " + hex(selector)});
        auto passing = list_of({{ids[0], 8, 1}, {ids[0], 7, 0}}, 2);
        passing.push_back(w32(key_at, ids[0]));
        cases.push_back({.entry = find_entry, .registers = {{3, list_at}, {4, key_at}, {5, selector}}, .memory = passing,
                         .label = "regen_queries: find a duplicated id, first passes, selector " + hex(selector)});
    }
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102dbe90: a word of the object, zero or not (the sign and each byte).
    for (const std::uint32_t word : {0u, 1u, 0xffu, 0x100u, 0x10000u, 0x01000000u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        cases.push_back({.entry = pool_word, .registers = {{3, object_at}}, .memory = {w32(object_at + 0x20, word)},
                         .label = "regen_queries: staff pool word " + hex(word)});
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102eb4e0, the queue comparator: null elements, then the grid of the two halfwords and the id ties.
    cases.push_back({.entry = compare_queue, .registers = {{3, 0}, {4, 0}}, .label = "regen_queries: queue both null"});
    cases.push_back({.entry = compare_queue, .registers = {{3, 0}, {4, first_at}}, .memory = {w16(first_at + 6, 3)}, .label = "regen_queries: queue first null"});
    cases.push_back({.entry = compare_queue, .registers = {{3, first_at}, {4, 0}}, .memory = {w16(first_at + 6, 3)}, .label = "regen_queries: queue second null"});
    constexpr std::array<std::uint32_t, 5> highs{0, 1, 0x7fff, 0x8000, 0xffff}, lows{0x8000, 0xffff, 0, 1, 0x7fff};
    const auto entry = [](std::uint32_t at, std::uint32_t id, std::uint32_t low, std::uint32_t high) {
        return std::vector<Write>{w32(at, id), w16(at + 4, low), w16(at + 6, high)};
    };
    const auto pair = [&](std::uint32_t id_first, std::uint32_t id_second, std::uint32_t low_first, std::uint32_t low_second,
                          std::uint32_t high_first, std::uint32_t high_second, const std::string& label) {
        auto memory = entry(first_at, id_first, low_first, high_first);
        append(memory, entry(second_at, id_second, low_second, high_second));
        cases.push_back({.entry = compare_queue, .registers = {{3, first_at}, {4, second_at}}, .memory = memory, .label = "regen_queries: queue " + label});
    };
    for (const auto hf : highs) for (const auto hs : highs) for (const auto lf : lows) for (const auto ls : lows)
        pair(7, 9, lf, ls, hf, hs, "high " + hex(hf) + "/" + hex(hs) + " low " + hex(lf) + "/" + hex(ls));
    for (const auto hf : highs) for (const auto lf : lows)
        for (const auto& ids : std::array<std::pair<std::uint32_t, std::uint32_t>, 6>{{{5, 5}, {5, 6}, {6, 5}, {0x80000000, 1}, {1, 0x80000000}, {0xffffffff, 0}}})
            pair(ids.first, ids.second, lf, lf, hf, hf, "tie high " + hex(hf) + " low " + hex(lf) + " ids " + hex(ids.first) + "/" + hex(ids.second));
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102eb550, the retirement comparator: the due date of each entry is its first date unless that is earlier than
    // today, then its second. Today is the real date and four others (a halfword day of either sign, edges of the year).
    const auto date = scenario.toc(date_slot);
    const auto real_day = std::int16_t(scenario.u16(date)), real_year = std::int16_t(scenario.u16(date + 2));
    cases.push_back({.entry = compare_retirement, .registers = {{3, 0}, {4, 0}}, .label = "regen_queries: retirement both null"});
    cases.push_back({.entry = compare_retirement, .registers = {{3, 0}, {4, first_at}}, .label = "regen_queries: retirement first null"});
    cases.push_back({.entry = compare_retirement, .registers = {{3, first_at}, {4, 0}}, .label = "regen_queries: retirement second null"});
    struct Today { std::int32_t day, year; };
    struct Dates { std::int32_t first_day, first_year, second_day, second_year; };
    for (const Today today : {Today{real_day, real_year}, Today{-1, 0}, Today{0x7fff, 0xffff}, Today{-32768, 0x8000}, Today{100, 2001}}) {
        const auto d = today.day, y = today.year;
        // Five patterns of the first date against today (earlier year, earlier day, the same, later day, later year), and
        // three of the second against the first-date patterns of the other entry.
        std::vector<Dates> dates;
        const std::array<std::pair<std::int32_t, std::int32_t>, 5> firsts{{{d, y - 1}, {d - 1, y}, {d, y}, {d + 1, y}, {d, y + 1}}};
        const std::array<std::pair<std::int32_t, std::int32_t>, 3> seconds{{{d - 1, y}, {d, y + 1}, {d + 2, y + 1}}};
        for (const auto& f : firsts) for (const auto& sc : seconds) dates.push_back({f.first, f.second, sc.first, sc.second});
        // The edges of the halfwords as dates.
        dates.push_back({-32768, 0, 32767, 0xffff});
        dates.push_back({32767, 0xffff, -32768, 0});
        for (const auto& a : dates) for (const auto& b : dates)
            for (const std::uint32_t ids : {0u, 1u}) {
                std::vector<Write> memory{w16(date, std::uint32_t(d)), w16(date + 2, std::uint32_t(y))};
                const auto put = [&](std::uint32_t at, const Dates& v, std::uint32_t id) {
                    append(memory, {w32(at, id), w16(at + 8, std::uint32_t(v.first_day)), w16(at + 0xa, std::uint32_t(v.first_year)),
                                    w16(at + 0x10, std::uint32_t(v.second_day)), w16(at + 0x12, std::uint32_t(v.second_year))});
                };
                put(first_at, a, 5);
                put(second_at, b, ids ? 6 : 5);
                cases.push_back({.entry = compare_retirement, .registers = {{3, first_at}, {4, second_at}}, .memory = memory,
                                 .label = "regen_queries: retirement today " + hex(std::uint32_t(d) & 0xffff) + "/" + hex(std::uint32_t(y) & 0xffff) + " first " +
                                          hex(std::uint32_t(a.first_day) & 0xffff) + "/" + hex(std::uint32_t(a.first_year) & 0xffff) + " " + hex(std::uint32_t(a.second_day) & 0xffff) + "/" +
                                          hex(std::uint32_t(a.second_year) & 0xffff) + " second " + hex(std::uint32_t(b.first_day) & 0xffff) + "/" + hex(std::uint32_t(b.first_year) & 0xffff) +
                                          " " + hex(std::uint32_t(b.second_day) & 0xffff) + "/" + hex(std::uint32_t(b.second_year) & 0xffff) + (ids ? " ids differ" : " ids equal")});
            }
    }
    // The ids decide ties: the wrapping difference.
    for (const auto& ids : std::array<std::pair<std::uint32_t, std::uint32_t>, 4>{{{0x80000000, 1}, {1, 0x80000000}, {0, 0xffffffff}, {7, 7}}}) {
        std::vector<Write> memory{w16(date, std::uint32_t(real_day)), w16(date + 2, std::uint32_t(real_year))};
        for (const auto at : {first_at, second_at})
            append(memory, {w16(at + 8, std::uint32_t(real_day) + 5), w16(at + 0xa, std::uint32_t(real_year) + 1), w16(at + 0x10, 3), w16(at + 0x12, 3)});
        append(memory, {w32(first_at, ids.first), w32(second_at, ids.second)});
        cases.push_back({.entry = compare_retirement, .registers = {{3, first_at}, {4, second_at}}, .memory = memory,
                         .label = "regen_queries: retirement tie ids " + hex(ids.first) + "/" + hex(ids.second)});
    }
}

PLAYER_REGEN_CASES("regen_queries") {
    // 0x102eb610: whether a list holds the key's id; the key is a real person's record (its first word the id).
    const auto ids = real_ids(scenario, 6);
    if (ids.size() < 6) throw std::runtime_error("regen_queries: the career has too few people");
    const auto person = [&](std::size_t n) { return scenario.person(std::uint32_t((n * 7919 + 13) % (scenario.people() - 16))); };
    cases.push_back({.entry = queued, .registers = {{3, list_at}, {4, 0}}, .memory = list_of({{ids[0], 0, 0}}, 1), .label = "regen_queries: queued null key"});
    for (const std::int32_t count : {0, 1, 3, 5, 6, -1, 0x7fffffff})
        for (const int where : {0, 2, 4, -1}) {
            // `count` past the five rows reads rows the case did not write (a zero id: no real key matches it).
            const auto rows = std::vector<Row>{{ids[0], 0, 0}, {ids[1], 0, 0}, {ids[2], 0, 0}, {ids[3], 0, 0}, {ids[4], 0, 0}};
            const auto key = where >= 0 ? person(std::size_t(where)) : person(5);
            cases.push_back({.entry = queued, .registers = {{3, list_at}, {4, key}}, .memory = list_of(rows, count > 0x1000 ? 5 : count),
                             .label = "regen_queries: queued count " + std::to_string(count) + " key at " + std::to_string(where)});
        }
    // A key id 0 against a list whose count runs past the rows written (zero ids).
    cases.push_back({.entry = queued, .registers = {{3, list_at}, {4, key_at}}, .memory = [&] { auto m = list_of({{ids[0], 0, 0}}, 3); m.push_back(w32(key_at, 0)); return m; }(),
                     .label = "regen_queries: queued id 0 past the rows"});
    // A duplicated id: found at the first.
    cases.push_back({.entry = queued, .registers = {{3, list_at}, {4, person(0)}}, .memory = list_of({{ids[0], 0, 0}, {ids[0], 0, 0}}, 2), .label = "regen_queries: queued duplicated id"});
}
}

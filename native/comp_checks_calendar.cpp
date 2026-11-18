// COMP.CPP calendar block's cases (comp_calendar.cpp, comp_regions.cpp), from the real career (--world): the
// game's competition objects and records, its clubs and the nation descriptors. Rows a case needs that the
// career lacks (a pair of fixture rows, a match object naming two clubs) are built in the scratch area from
// real values with one field changed.
#include "comp_checks.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::comp_checks {
namespace {
constexpr std::uint32_t fixture_dates = 0x10450ee0, compare_dates = 0x10451470, first_date = 0x10453190,
                        descriptor_index = 0x10451720, fixture_code = 0x10451800, nation_code = 0x10451990;
constexpr std::uint32_t allocate = 0x10775130, allocate_object = 0x10773520, rules_check = 0x10494d20,
                        is_special = 0x100c7d00, query_a = 0x10192740, query_b = 0x10192860, query_c = 0x10192130;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, descriptors_slot = -0x7b44,
                       rules_slot = -0x7b54, none_slot = -0x7aa8, records_slot = -0x7bbc,
                       clubs_slot = -0x7be8, club_count_slot = -0x7b50;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
std::vector<std::uint32_t> objects_of(const Scenario& s) {
    std::vector<std::uint32_t> objects;
    const auto table = s.global(competitions_slot);
    const auto count = s.global(competition_count_slot);
    for (std::uint32_t id = 0; table && id < count && id < 4096; ++id)
        if (const auto object = s.r32(table + id * 4)) objects.push_back(object);
    return objects;
}
}

COMP_CASES("calendar: dates") {
    if (scenario.match) return;
    const auto objects = objects_of(scenario);
    const auto count_out = scratch_base + 0x800;
    for (const auto object : objects) {
        cases.push_back({.entry = fixture_dates, .registers = {{3, object}, {4, count_out}}, .label = "calendar: dates of " + hex(object)});
        cases.push_back({.entry = first_date, .registers = {{3, scratch_base + 0x820}, {4, object}}, .label = "calendar: first date of " + hex(object)});
    }
    if (objects.empty()) return;
    const auto object = objects.front();
    // The allocator fails: the rows block, then a date's `new` (the date rows read from address 0 on both sides).
    // The realloc's failure (0x495) cannot be answered: the original runs it in place, never through the dispatcher.
    cases.push_back({.entry = fixture_dates, .registers = {{3, object}, {4, count_out}}, .answers = {{allocate, 0}}, .label = "calendar: dates, no rows block"});
    for (const auto other : objects)
        cases.push_back({.entry = fixture_dates, .registers = {{3, other}, {4, count_out}}, .answers = {{allocate_object, 0}}, .label = "calendar: dates, no date " + hex(other)});
}

COMP_CASES("calendar: order") {
    if (scenario.match) return;
    const auto left = scratch_base + 0x900, right = scratch_base + 0x920;
    const auto records = scenario.global(records_slot);
    const auto count = scenario.global(competition_count_slot);
    // Two records with different reputations (word +8 of a row is the record's index), and two with the same.
    std::uint32_t low = 0, high = 0;
    std::int32_t best = -0x10000, worst = 0x10000;
    for (std::uint32_t id = 0; id < count && id < 4096; ++id) {
        const auto raw = scenario.r32(records + id * 0x6c + 0x18) >> 16;
        const std::int32_t value = std::int16_t(raw);
        if (value > best) { best = value; high = id; }
        if (value < worst) { worst = value; low = id; }
    }
    const auto row = [&](std::uint32_t at, std::uint16_t e, std::uint16_t c, std::uint32_t record, std::uint16_t number, std::uint16_t kind) {
        return std::vector<Write>{{2, at + 0xe, e}, {2, at + 0xc, c}, {4, at + 8, record}, {2, at + 0x10, number}, {2, at + 0x12, kind}};
    };
    const std::pair<std::uint16_t, std::uint16_t> halves[] = {{5, 3}, {3, 5}, {4, 4}, {0xffff, 1}, {0x8000, 0x7fff}, {1, 0xffff}};
    for (const auto& [a, b] : halves)
        for (const auto& [c_a, c_b] : halves) {
            cases.push_back({.entry = compare_dates, .registers = {{3, left}, {4, right}},
                             .memory = joined(row(left, a, c_a, high, 0, 0), row(right, b, c_b, low, 0, 0)),
                             .label = "calendar: order e " + hex(a) + " " + hex(b) + " c " + hex(c_a) + " " + hex(c_b)});
        }
    // Equal dates: by reputation both ways, then equal reputation with texts of different and equal rows.
    cases.push_back({.entry = compare_dates, .registers = {{3, left}, {4, right}}, .memory = joined(row(left, 7, 7, high, 0, 0), row(right, 7, 7, low, 0, 0)), .label = "calendar: order reputation high first"});
    cases.push_back({.entry = compare_dates, .registers = {{3, left}, {4, right}}, .memory = joined(row(left, 7, 7, low, 0, 0), row(right, 7, 7, high, 0, 0)), .label = "calendar: order reputation low first"});
    const auto objects = objects_of(scenario);
    std::vector<std::uint32_t> ids;
    for (const auto object : objects) {
        const auto record = scenario.r32(object + 4);
        if (record) ids.push_back((record - records) / 0x6c);
    }
    for (std::size_t a = 0; a + 1 < ids.size() && a < 12; ++a)
        for (std::size_t b = a; b < ids.size() && b < a + 3; ++b)
            for (const std::uint16_t kind : {0, 1, 3})
                cases.push_back({.entry = compare_dates, .registers = {{3, left}, {4, right}},
                                 .memory = joined(row(left, 7, 7, ids[a], std::uint16_t(ids[a]), kind), row(right, 7, 7, ids[a], std::uint16_t(ids[b]), 0)),
                                 .label = "calendar: order texts " + std::to_string(a) + " " + std::to_string(b) + " kind " + std::to_string(kind)});
    cases.push_back({.entry = compare_dates, .registers = {{3, left}, {4, right}}, .memory = joined(row(left, 7, 7, high, 5, 5), row(right, 7, 7, high, 5, 5)), .label = "calendar: order equal rows"});
}

COMP_CASES("calendar: descriptors") {
    if (scenario.match) return;
    const auto rows = scenario.toc(descriptors_slot);
    // 0x10451720: every row's code, and codes no row has.
    for (std::uint32_t n = 0; n < 34; ++n) {
        const auto code = scenario.r32(rows + n * 0x4c + 0x14) >> 24;
        cases.push_back({.entry = descriptor_index, .registers = {{3, code}}, .label = "calendar: descriptor code of row " + std::to_string(n)});
    }
    for (const std::uint32_t code : {0x7fu, 0x80u, 0xffu, 0x03u, 0x1fu, 0xfffffff0u, 0x12345600u})
        cases.push_back({.entry = descriptor_index, .registers = {{3, code}}, .label = "calendar: descriptor code " + hex(code)});
    // 0x10451990: clubs of the career, a scratch club naming each descriptor's nation, then without one.
    const auto clubs = scenario.global(clubs_slot);
    const auto club_count = scenario.global(club_count_slot);
    for (std::uint32_t n = 0; n < club_count && n < 60; ++n)
        cases.push_back({.entry = nation_code, .registers = {{3, clubs + n * 0x248}}, .label = "calendar: nation code of club " + std::to_string(n)});
    const auto club = scratch_base + 0xa00;
    for (std::uint32_t n = 0; n < 34; ++n) {
        const auto nation = scenario.r32(rows + n * 0x4c);
        cases.push_back({.entry = nation_code, .registers = {{3, club}}, .memory = {{4, club + 0x2c, nation}}, .label = "calendar: nation code row " + std::to_string(n)});
        if (nation && n < 8)
            cases.push_back({.entry = nation_code, .registers = {{3, club}}, .memory = {{4, club + 0x2c, nation}, {1, nation + 0x120, 0}}, .label = "calendar: nation code row " + std::to_string(n) + " not loaded"});
    }
    for (const std::uint32_t answer : {0u, 1u, 0x100u, 0x7fu})
        cases.push_back({.entry = nation_code, .registers = {{3, club}}, .memory = {{4, club + 0x2c, 0}}, .answers = {{is_special, answer}}, .label = "calendar: nation code none, special " + hex(answer)});
    cases.push_back({.entry = nation_code, .registers = {{3, club}}, .memory = {{4, club + 0x2c, club}}, .answers = {{is_special, 1}}, .label = "calendar: nation code unknown nation, special"});
    cases.push_back({.entry = nation_code, .registers = {{3, club}}, .memory = {{4, club + 0x2c, club}}, .answers = {{is_special, 0}}, .label = "calendar: nation code unknown nation, ordinary"});
}

COMP_CASES("calendar: fixture code") {
    if (scenario.match) return;
    const auto rows = scenario.toc(descriptors_slot);
    const auto records = scenario.global(records_slot);
    const auto count = scenario.global(competition_count_slot);
    const auto clubs = scenario.global(clubs_slot);
    const auto rules_object = scenario.global(rules_slot);
    const auto none_word = scenario.global(none_slot);
    // Without a match: every record of the career, then answers for the three queries of a record without a nation.
    std::size_t listed = 0;
    for (std::uint32_t id = 0; id < count && id < 4096; ++id) {
        const auto record = records + id * 0x6c;
        if (listed++ < 160 || scenario.r32(record + 8) != 0)
            cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, 0}}, .label = "calendar: fixture code record " + std::to_string(id)});
    }
    const auto record = scratch_base + 0xb00, context = scratch_base + 0xc00, holder = scratch_base + 0xd00;
    std::vector<Write> base;
    for (std::uint32_t offset = 0; offset < 0x6c; offset += 4) base.push_back({4, record + offset, scenario.r32(records + offset)});
    const auto with_nation = [&](std::uint32_t nation, std::uint32_t id_word) {
        auto writes = base;
        writes.push_back({4, record + 8, nation});
        writes.push_back({4, record + 0, id_word});
        return writes;
    };
    const auto nation0 = scenario.r32(rows), nation1 = scenario.r32(rows + 0x4c * 3);
    for (const std::uint32_t nation : {0u, nation0, nation1, 0x1234u}) {
        for (const std::uint32_t answers_a : {0u, 1u})
            for (const std::uint32_t answers_b : {0u, 1u}) {
                std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{query_a, answers_a}, {query_b, answers_b}, {query_c, 0x55}};
                cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, 0}}, .memory = with_nation(nation, 5), .answers = answers,
                                 .label = "calendar: fixture code nation " + hex(nation) + " queries " + std::to_string(answers_a) + std::to_string(answers_b)});
            }
        const auto not_loaded = nation && nation != 0x1234 ? std::vector<Write>{{1, nation + 0x120, 0}} : std::vector<Write>{};
        cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, 0}}, .memory = joined(with_nation(nation, 5), not_loaded), .label = "calendar: fixture code nation " + hex(nation) + " not loaded"});
    }
    // With a match: the rules answer (forced), the word +0 of the record, the holder, the clubs' queries.
    const auto club_a = clubs, club_b = clubs + 0x248 * 2;
    const auto match_with = [&](std::uint32_t holder_word_at, std::uint32_t home, std::uint32_t away) {
        return std::vector<Write>{{4, context + 0x14, holder_word_at}, {4, context + 0x1c, home}, {4, context + 0x20, away}};
    };
    // The context's word +0x14 equals the rules object's word +4 (a value, not its address) to pass the first test.
    const auto rules_plus = rules_object ? scenario.r32(rules_object + 4) : 0;
    const std::uint32_t holders[] = {rules_plus, holder, holder + 0x10, 0};
    const std::pair<std::uint32_t, std::uint32_t> clubs_pairs[] = {{club_a, club_b}, {club_a, club_a}};
    for (const std::uint32_t ruled : {0u, 1u})
        for (const auto holder_pointer : holders)
            for (const std::uint32_t id_word : {5u, none_word})
                for (const auto& [home, away] : clubs_pairs)
                    cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, context}},
                                     .memory = joined(joined(with_nation(nation0, id_word), match_with(holder_pointer, home, away)),
                                                      {{4, holder, id_word == none_word ? none_word : 0u}, {4, holder + 0x10, 0u}}),
                                     .answers = {{rules_check, ruled}, {is_special, 0}},
                                     .label = "calendar: fixture code match ruled " + std::to_string(ruled) + " holder " + hex(holder_pointer) + " id " + hex(id_word) + (home == away ? " same club" : "")});
    // The special-club answers of the two clubs, one at a time and together; the nations of the clubs.
    for (const std::uint32_t first : {0u, 1u})
        for (const std::uint32_t second : {0u, 1u})
            cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, context}},
                             .memory = joined(with_nation(nation0, 5), match_with(rules_plus, club_a, club_b)),
                             .answers = {{rules_check, 1}}, .nth_answers = {{is_special, 1, first}, {is_special, 2, second}},
                             .label = "calendar: fixture code specials " + std::to_string(first) + std::to_string(second)});
    for (std::uint32_t n = 0; n < 34; n += 3) {
        const auto nation = scenario.r32(rows + n * 0x4c);
        if (!nation) continue;
        // Both clubs of one nation (a scratch pair), the nation loaded or not; the clubs of two nations.
        const auto home = scratch_base + 0xe00, away = scratch_base + 0xf00;
        const std::vector<Write> pair = {{4, home + 0x2c, nation}, {4, away + 0x2c, nation}};
        cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, context}},
                         .memory = joined(joined(with_nation(nation0, 5), match_with(rules_plus, home, away)), pair),
                         .answers = {{rules_check, 1}, {is_special, 0}}, .label = "calendar: fixture code both clubs of row " + std::to_string(n)});
        cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, context}},
                         .memory = joined(joined(joined(with_nation(nation0, 5), match_with(rules_plus, home, away)), pair), {{1, nation + 0x120, 0}}),
                         .answers = {{rules_check, 1}, {is_special, 0}}, .label = "calendar: fixture code both clubs of row " + std::to_string(n) + " not loaded"});
        cases.push_back({.entry = fixture_code, .registers = {{3, record}, {4, context}},
                         .memory = joined(joined(with_nation(nation0, 5), match_with(rules_plus, home, away)), {{4, home + 0x2c, nation}, {4, away + 0x2c, nation + 4}}),
                         .answers = {{rules_check, 1}, {is_special, 0}}, .label = "calendar: fixture code clubs of two nations " + std::to_string(n)});
    }
}
}

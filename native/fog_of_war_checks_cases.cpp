// fog_of_war.cpp's cases, from the real career (--world): the game's own table of sixteen slots, its
// knowledge objects, managers and people. Rows a case needs that the career has no instance of (a person
// past the table's end, a match sheet, a knowledge object without a manager) are copies of real rows in the
// scratch area with one field changed. The file routines' ZIPDIR.CPP transfers are answered on both sides
// (a snapshot has no files): `pokes` give the words a read would have read, `nth_answers` the failures.
#include "fog_of_war_checks.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::fog_of_war_checks {
namespace {
constexpr std::uint32_t construct_table = 0x106fd120, destroy_table = 0x106fd230, read_table = 0x106fd2c0,
                        write_table = 0x106fd6d0, age_table = 0x106fd990, forget_in_table = 0x106fd9f0,
                        add_manager = 0x106fda80, knowledge_of_manager = 0x106fdb00, record_match = 0x106fdb30,
                        construct_knowledge = 0x106fdc90, destroy_knowledge = 0x106fdde0, read_knowledge = 0x106fde50,
                        write_knowledge = 0x106fe000, age_knowledge = 0x106fe130, forget = 0x106fe220, level_of = 0x106fe280,
                        initial_level = 0x106fe3f0, famous = 0x106fe700, well_known = 0x106fe7d0, level_floor = 0x106fe820,
                        sight = 0x106fec70, random_below = 0x106fef10, within_a_season = 0x106ff490,
                        within_two_seasons = 0x106ff4f0;
// seasons_since_club (PLAYER_STATS.CPP) reads the history file, and the progress gauge draws: a snapshot has no
// volume or port for either, so both are answered. The seasons answers cycle through each outcome of the checks.
constexpr std::uint32_t seasons_since_club = 0x10309cc0, progress = 0x10743120;
constexpr std::uint32_t season_answers[] = {0xffffffffu, 0, 1, 2, 3, 0xfffffffeu};
constexpr std::uint32_t open = 0x107475b0, read = 0x10747a80, write = 0x10747cc0, close = 0x10747900,
                        allocate_object = 0x10773520;
// The saved game's version (labs of it is inlined in the generated reader, so it is set in memory, not answered).
constexpr std::int32_t version_cell = -0x7b74;
constexpr std::uint32_t handle = 0x00c0ffee, file_spec = scratch_base + 0x80000;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}

struct World {
    const Scenario& s;
    std::uint32_t table = 0, enabled_at = 0, today = 0;
    std::int32_t count = 0;
    std::vector<std::uint32_t> knowledge, managers;  // the present slots and their managers
    explicit World(const Scenario& scenario) : s(scenario) {
        table = s.global(-0x7978);
        enabled_at = s.toc(-0x797c);
        today = s.toc(-0x7b80);
        count = std::int32_t(s.people());
        if (table) for (std::uint32_t n = 0; n < 16; ++n)
            if (const auto k = s.r32(table + n * 4)) { knowledge.push_back(k); managers.push_back(s.r32(k + 4)); }
    }
    std::uint32_t row(std::int32_t index) const { return s.person(std::uint32_t(index)); }
    std::uint32_t club_of(std::uint32_t person) const { return s.r32(person + 0x24); }
    std::uint8_t level(std::uint32_t k, std::uint32_t person) const {
        return std::uint8_t(s.s.memory.read<1>(s.r32(k) + s.r32(person) * 12));
    }
    // A spread of people over the whole table plus everyone who shares a club, nation, national side or
    // division with a manager: the rules' branches turn on those.
    std::vector<std::uint32_t> people(std::uint32_t manager, std::size_t spread, std::size_t related) const {
        std::vector<std::uint32_t> found;
        for (std::size_t n = 0; n < spread && count > 0; ++n) found.push_back(row(std::int32_t(n * std::size_t(count) / spread)));
        const auto manager_club = club_of(manager), side = s.r32(manager + 0x20), nation = s.r32(manager + 0xc);
        const auto club_nation = manager_club ? s.r32(manager_club + 0x2c) : 0, division = manager_club ? s.r32(manager_club + 0x30) : 0;
        std::size_t same_club = 0, same_side = 0, same_nation = 0, nation_clubs = 0, same_division = 0;
        for (std::int32_t n = 0; n < count && found.size() < spread + related; ++n) {
            const auto person = row(n), club = club_of(person);
            bool take = false;
            if (club && club == manager_club && same_club < related / 5) { take = true; ++same_club; }
            else if (side && s.r32(person + 0x20) == side && same_side < related / 5) { take = true; ++same_side; }
            else if (s.r32(person + 0xc) == nation && same_nation < related / 5) { take = true; ++same_nation; }
            else if (club && s.r32(club + 0x2c) == club_nation && nation_clubs < related / 5) { take = true; ++nation_clubs; }
            else if (club && s.r32(club + 0x30) == division && same_division < related / 5) { take = true; ++same_division; }
            if (take) found.push_back(person);
        }
        return found;
    }
    // A copy of a real person row in the scratch area with its id set to `id`.
    std::vector<Write> person_copy(std::uint32_t at, std::uint32_t person, std::uint32_t id) const {
        std::vector<Write> writes;
        for (std::uint32_t offset = 0; offset < 0x70; offset += 4) writes.push_back({4, at + offset, s.r32(person + offset)});
        writes.push_back({4, at, id});
        return writes;
    }
};
// Whether the career keeps no knowledge object (the fog off, or no human): the blocks that need one skip.
bool w_empty(const Scenario& scenario) {
    const auto table = scenario.global(-0x7978);
    if (!table) return true;
    for (std::uint32_t n = 0; n < 16; ++n) if (scenario.r32(table + n * 4)) return false;
    return true;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

FOG_OF_WAR_CASES("queries") {
    if (scenario.match) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.managers.size(); ++n)
        cases.push_back({.entry = knowledge_of_manager, .registers = {{3, w.table}, {4, w.managers[n]}}, .label = "slot of manager " + std::to_string(n)});
    cases.push_back({.entry = knowledge_of_manager, .registers = {{3, w.table}, {4, 0}}, .label = "slot of no manager"});
    const auto past = scratch_base + 0x100;
    for (std::size_t n = 0; n < w.knowledge.size(); ++n) {
        const auto k = w.knowledge[n], manager = w.managers[n];
        for (const auto person : w.people(manager, 120, 100))
            cases.push_back({.entry = level_of, .registers = {{3, k}, {4, person}}, .label = "level of " + hex(person)});
        const auto someone = w.row(w.count / 2);
        cases.push_back({.entry = level_of, .registers = {{3, k}, {4, 0}}, .label = "level of nobody"});
        cases.push_back({.entry = level_of, .registers = {{3, k}, {4, someone}}, .memory = {{1, w.enabled_at, 0}}, .label = "level with the fog off"});
        cases.push_back({.entry = level_of, .registers = {{3, k}, {4, past}}, .memory = w.person_copy(past, someone, std::uint32_t(w.count)),
                         .label = "level of a person past the table"});
        const auto orphan = scratch_base + 0x200;
        cases.push_back({.entry = level_of, .registers = {{3, orphan}, {4, someone}},
                         .memory = {{4, orphan, scenario.r32(k)}, {4, orphan + 4, 0}}, .label = "level without a manager"});
        // A manager without a club, and one whose club a contract lookup would name.
        cases.push_back({.entry = level_of, .registers = {{3, k}, {4, someone}}, .memory = {{4, manager + 0x24, 0}}, .label = "level, manager without a club"});
    }
}

FOG_OF_WAR_CASES("rules") {
    if (scenario.match) return;
    const World w(scenario);
    const auto first = cases.size();
    const auto past = scratch_base + 0x100;
    for (std::size_t n = 0; n < w.knowledge.size() && n < 3; ++n) {
        const auto k = w.knowledge[n], manager = w.managers[n];
        for (const auto person : w.people(manager, 150, 150)) {
            cases.push_back({.entry = initial_level, .registers = {{3, k}, {4, manager}, {5, person}}, .label = "initial " + hex(person)});
            cases.push_back({.entry = level_floor, .registers = {{3, k}, {4, manager}, {5, person}}, .label = "floor " + hex(person)});
            if (n == 0) {
                cases.push_back({.entry = famous, .registers = {{3, k}, {4, person}}, .label = "famous " + hex(person)});
                cases.push_back({.entry = well_known, .registers = {{3, k}, {4, person}}, .label = "well known " + hex(person)});
                cases.push_back({.entry = within_a_season, .registers = {{3, k}, {4, person}, {5, w.club_of(manager)}}, .label = "a season at the manager's club"});
                cases.push_back({.entry = within_two_seasons, .registers = {{3, k}, {4, person}, {5, w.club_of(manager)}}, .label = "two seasons at the manager's club"});
                if (const auto club = w.club_of(person)) {
                    cases.push_back({.entry = within_a_season, .registers = {{3, k}, {4, person}, {5, club}}, .label = "a season at his club"});
                    cases.push_back({.entry = within_two_seasons, .registers = {{3, k}, {4, person}, {5, club}}, .label = "two seasons at his club"});
                }
            }
        }
        // A manager with no club, a person with no player record, nation or club.
        const auto someone = w.row(w.count / 3);
        cases.push_back({.entry = initial_level, .registers = {{3, k}, {4, manager}, {5, someone}}, .memory = {{4, manager + 0x24, 0}}, .label = "initial, manager without a club"});
        cases.push_back({.entry = level_floor, .registers = {{3, k}, {4, manager}, {5, someone}}, .memory = {{4, manager + 0x24, 0}}, .label = "floor, manager without a club"});
        cases.push_back({.entry = level_floor, .registers = {{3, k}, {4, manager}, {5, past}},
                         .memory = joined(w.person_copy(past, someone, scenario.r32(someone)), {{4, past + 0xc, 0}}), .label = "floor, person without a nation"});
        cases.push_back({.entry = initial_level, .registers = {{3, k}, {4, manager}, {5, past}},
                         .memory = joined(w.person_copy(past, someone, scenario.r32(someone)), {{4, past + 0x14, 0}}), .label = "initial, no player record"});
    }
    cases.push_back({.entry = famous, .registers = {{3, 0}, {4, 0}}, .label = "famous nobody"});
    const auto someone = w.row(w.count / 4);
    cases.push_back({.entry = famous, .registers = {{3, 0}, {4, past}}, .memory = joined(w.person_copy(past, someone, 0), {{4, past + 0xc, 0}}),
                     .label = "famous without a nation"});
    cases.push_back({.entry = within_a_season, .registers = {{3, 0}, {4, someone}, {5, 0}}, .label = "a season at no club"});
    cases.push_back({.entry = within_two_seasons, .registers = {{3, 0}, {4, 0}, {5, w.club_of(someone)}}, .label = "two seasons for nobody"});
    for (std::size_t n = first; n < cases.size(); ++n) cases[n].answers.push_back({seasons_since_club, season_answers[n % std::size(season_answers)]});
}

FOG_OF_WAR_CASES("updates") {
    if (scenario.match || w_empty(scenario)) return;
    const World w(scenario);
    const auto k = w.knowledge.front(), manager = w.managers.front();
    const auto past = scratch_base + 0x100;
    const auto someone = w.row(w.count / 2);
    cases.push_back({.entry = forget, .registers = {{3, k}, {4, someone}}, .label = "forget"});
    cases.push_back({.entry = forget, .registers = {{3, k}, {4, 0}}, .label = "forget nobody"});
    cases.push_back({.entry = forget, .registers = {{3, k}, {4, someone}}, .memory = {{1, w.enabled_at, 0}}, .label = "forget with the fog off"});
    cases.push_back({.entry = forget, .registers = {{3, k}, {4, past}}, .memory = w.person_copy(past, someone, std::uint32_t(w.count)), .label = "forget past the table"});
    const auto orphan = scratch_base + 0x200;
    cases.push_back({.entry = forget, .registers = {{3, orphan}, {4, someone}}, .memory = {{4, orphan, scenario.r32(k)}, {4, orphan + 4, 0}}, .label = "forget without a manager"});
    cases.push_back({.entry = forget_in_table, .registers = {{3, w.table}, {4, someone}}, .label = "forget everywhere"});
    cases.push_back({.entry = forget_in_table, .registers = {{3, w.table}, {4, 0}}, .label = "forget nobody everywhere"});
    cases.push_back({.entry = forget_in_table, .registers = {{3, w.table}, {4, someone}}, .memory = {{1, w.enabled_at, 0}}, .label = "forget everywhere with the fog off"});
    cases.push_back({.entry = forget_in_table, .registers = {{3, w.table}, {4, past}}, .memory = w.person_copy(past, someone, std::uint32_t(w.count)), .label = "forget everywhere past the table"});
    // Sightings: no observer, an observer without a staff record, and judging observers on people at each level.
    std::vector<std::uint32_t> judges, bystanders;
    for (std::int32_t n = 0; n < w.count && (judges.size() < 24 || bystanders.empty()); ++n) {
        const auto person = w.row(n);
        if (scenario.r32(person + 0x1c) && judges.size() < 24) judges.push_back(person);
        else if (!scenario.r32(person + 0x1c) && bystanders.empty()) bystanders.push_back(person);
    }
    std::vector<std::uint32_t> by_level[4];
    for (std::int32_t n = 0; n < w.count; ++n) {
        const auto person = w.row(n);
        const auto level = w.level(k, person);
        if (level < 4 && by_level[level].size() < 12) by_level[level].push_back(person);
    }
    for (unsigned level = 0; level < 4; ++level)
        for (std::size_t n = 0; n < by_level[level].size(); ++n) {
            const auto person = by_level[level][n];
            cases.push_back({.entry = sight, .registers = {{3, k}, {4, person}, {5, 0}}, .label = "seen at a match, level " + std::to_string(level)});
            if (!bystanders.empty()) cases.push_back({.entry = sight, .registers = {{3, k}, {4, person}, {5, bystanders.front()}}, .label = "seen by a bystander"});
            for (std::size_t j = n; j < judges.size(); j += 6)
                cases.push_back({.entry = sight, .registers = {{3, k}, {4, person}, {5, judges[j]}}, .label = "judged, level " + std::to_string(level)});
        }
    cases.push_back({.entry = sight, .registers = {{3, k}, {4, 0}, {5, 0}}, .label = "seen nobody"});
    cases.push_back({.entry = sight, .registers = {{3, k}, {4, past}, {5, 0}}, .memory = w.person_copy(past, someone, std::uint32_t(w.count)), .label = "seen past the table"});
    // The sample table at its end: the base is drawn again.
    const auto cursor = scenario.toc(-0x7b84), last = scenario.r32(scenario.toc(-0x7b88));
    if (!judges.empty() && !by_level[0].empty())
        cases.push_back({.entry = sight, .registers = {{3, k}, {4, by_level[0].front()}, {5, judges.front()}}, .memory = {{4, cursor, last}}, .label = "judged at the sample table's end"});
    // New objects: an added manager (the whole initial pass) and the bare constructor of the reader.
    const auto table_copy = scratch_base + 0x400;
    std::vector<Write> empty_table;
    for (std::uint32_t n = 0; n < 16; ++n) empty_table.push_back({4, table_copy + n * 4, 0});
    cases.push_back({.entry = add_manager, .registers = {{3, table_copy}, {4, manager}}, .memory = empty_table, .label = "add a manager",
                     .answers = {{progress, 0}, {seasons_since_club, 0xffffffffu}}});
    cases.push_back({.entry = construct_knowledge, .registers = {{3, scratch_base + 0x540}, {4, manager}}, .label = "an object for a manager",
                     .answers = {{progress, 0}, {seasons_since_club, 1}}});
    cases.push_back({.entry = add_manager, .registers = {{3, table_copy}, {4, 0}}, .memory = empty_table, .label = "add no manager"});
    cases.push_back({.entry = add_manager, .registers = {{3, table_copy}, {4, manager}}, .memory = empty_table, .label = "add a manager, no memory",
                     .answers = {{allocate_object, 0}}});
    const auto object = scratch_base + 0x500;
    cases.push_back({.entry = construct_knowledge, .registers = {{3, object}, {4, 0}}, .label = "an empty object"});
    cases.push_back({.entry = construct_knowledge, .registers = {{3, object}, {4, manager}}, .label = "an object, no memory for its entries",
                     .answers = {{0x10775130, 0}}});
    cases.push_back({.entry = destroy_knowledge, .registers = {{3, k}, {4, 1}}, .label = "destroy and free an object"});
    cases.push_back({.entry = destroy_knowledge, .registers = {{3, k}, {4, 0}}, .label = "destroy an object"});
    cases.push_back({.entry = destroy_knowledge, .registers = {{3, 0}, {4, 1}}, .label = "destroy no object"});
    cases.push_back({.entry = destroy_knowledge, .registers = {{3, object}, {4, 0xffff}}, .memory = {{4, object, 0}}, .label = "destroy an object without entries"});
    cases.push_back({.entry = destroy_table, .registers = {{3, w.table}, {4, 0}}, .label = "destroy the table"});
    cases.push_back({.entry = destroy_table, .registers = {{3, w.table}, {4, 1}}, .label = "destroy and free the table"});
    cases.push_back({.entry = destroy_table, .registers = {{3, 0}, {4, 1}}, .label = "destroy no table"});
}

FOG_OF_WAR_CASES("daily") {
    if (scenario.match || w_empty(scenario)) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.knowledge.size(); ++n)
        cases.push_back({.entry = age_knowledge, .registers = {{3, w.knowledge[n]}}, .label = "a day for manager " + std::to_string(n)});
    // Two years on, every unrefreshed level falls.
    const auto year = std::uint32_t(scenario.s.memory.read<2>(w.today + 2));
    cases.push_back({.entry = age_knowledge, .registers = {{3, w.knowledge.front()}}, .memory = {{2, w.today + 2, year + 2}}, .label = "a day two years on"});
    cases.push_back({.entry = age_table, .registers = {{3, w.table}}, .label = "a day for everyone"});
    cases.push_back({.entry = age_table, .registers = {{3, w.table}}, .memory = {{2, w.today + 2, year + 2}}, .label = "a day for everyone, two years on"});
}

FOG_OF_WAR_CASES("match") {
    if (scenario.match || w_empty(scenario)) return;
    const World w(scenario);
    const auto match = scratch_base + 0x1000, sheets = scratch_base + 0x2000;
    // The first manager's club at home against the second's (or any club); the sheets list real people.
    const auto home = w.club_of(w.managers.front());
    const auto away = w.managers.size() > 1 ? w.club_of(w.managers[1]) : scenario.club(1);
    std::vector<Write> memory = {{4, match + 0x20, home}, {4, match + 0x1c, away}};
    for (std::uint32_t side = 0; side < 2; ++side)
        for (std::uint32_t n = 0; n < 20; ++n) {
            const auto line = sheets + side * 0x664 + 0x194 + n * 0x4c;
            const auto index = (n * 977 + side * 131) % std::uint32_t(w.count);
            memory.push_back({4, line, n == 3 ? 0xffffffffu : index});
            memory.push_back({4, line + 4, n == 5 ? 0xffffffffu : 1});
            memory.push_back({1, line + 0x32, n % 3 == 0 ? 0xffu : 0});
        }
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = memory, .label = "a match"});
    auto swapped = memory;
    swapped[0].value = away; swapped[1].value = home;
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = swapped, .label = "a match away"});
    // A sheet naming a person past the table, and a club without a manager.
    auto beyond = memory;
    beyond.push_back({4, sheets + 0x194, std::uint32_t(w.count) + 5});
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = beyond, .label = "a sheet past the table"});
    auto unmanaged = memory;
    unmanaged.push_back({4, home + 0x90, 0});
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = unmanaged, .label = "a club without its manager"});
}

FOG_OF_WAR_CASES("random") {
    if (scenario.match) return;
    const auto cursor = scenario.toc(-0x7b84), last = scenario.r32(scenario.toc(-0x7b88));
    for (const std::uint32_t limit : {0u, 1u, 2u, 7u, 100u, 0xffffu, 0x10000u, 0x10001u, 123456u, 0x7fffffffu, 0x80000000u,
                                      0xffffffffu, 0xfffffff6u, 0xffff0001u, 0xffff0000u, 0xfff00000u, 0x0ccccccdu, 0x19999999u})
        for (const bool at_end : {false, true})
            cases.push_back({.entry = random_below, .registers = {{3, limit}}, .memory = at_end ? std::vector<Write>{{4, cursor, last}} : std::vector<Write>{},
                             .label = "random below " + hex(limit) + (at_end ? " at the table's end" : "")});
}

FOG_OF_WAR_CASES("file") {
    if (scenario.match || w_empty(scenario)) return;
    const World w(scenario);
    const auto k = w.knowledge.front(), manager = w.managers.front();
    const auto count = std::uint32_t(w.count);
    // Frames below the case's stack pointer: the reader's present word and handle, its object reader's index.
    const auto table_frame = scenario.sp() - 0x150, object_frame = table_frame - 0xd0, lone_object_frame = scenario.sp() - 0xd0;
    const auto present = table_frame + 0x38, index = object_frame + 0x38;
    // The object's own file routines.
    cases.push_back({.entry = write_knowledge, .registers = {{3, k}, {4, handle}}, .label = "write an object",
                     .nth_answers = {{write, 1, 1}, {write, 2, count}}});
    cases.push_back({.entry = write_knowledge, .registers = {{3, k}, {4, handle}}, .label = "write an object, its index fails",
                     .nth_answers = {{write, 1, 0}, {write, 2, count}}});
    cases.push_back({.entry = write_knowledge, .registers = {{3, k}, {4, handle}}, .label = "write an object, its entries fail",
                     .nth_answers = {{write, 1, 1}, {write, 2, count - 1}}});
    const auto object = scratch_base + 0x600;
    const auto object_case = [&](std::uint32_t value, std::uint32_t entries_read, std::string label) {
        return Case{.entry = read_knowledge, .registers = {{3, object}, {4, handle}}, .memory = {{4, object, scenario.r32(k)}, {4, object + 4, 0}},
                    .label = std::move(label), .pokes = {{read, {{4, lone_object_frame + 0x38, value}}}},
                    .nth_answers = {{read, 1, 1}, {read, 2, entries_read}}};
    };
    cases.push_back(object_case(scenario.r32(manager), count, "read an object"));
    cases.push_back(object_case(count, count, "read an object naming the table's end"));
    cases.push_back(object_case(count + 1, count, "read an object naming past the table"));
    cases.push_back(object_case(0xffffffffu, count, "read an object naming -1"));
    cases.push_back(object_case(scenario.r32(manager), count - 1, "read an object, its entries short"));
    auto failed_index = object_case(scenario.r32(manager), count, "read an object, its index fails");
    failed_index.nth_answers = {{read, 1, 0}};
    cases.push_back(failed_index);
    // The table's writer: the real table, a table with an empty first slot, the fog off, and each failure.
    const auto spec = std::vector<Write>{{4, file_spec, 0x12345678}};
    std::vector<Case::NthAnswer> writes = {{write, 1, 1}};
    unsigned n = 2;
    for (std::uint32_t slot = 0; slot < 16; ++slot) {
        writes.push_back({write, n++, 1});
        if (scenario.r32(w.table + slot * 4)) { writes.push_back({write, n++, 1}); writes.push_back({write, n++, count}); }
    }
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, file_spec}}, .memory = spec, .label = "write the table",
                     .answers = {{open, handle}, {close, 0}}, .nth_answers = writes});
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, 0}}, .label = "write the table, no file"});
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, file_spec}}, .memory = spec, .label = "write the table, not opened",
                     .answers = {{open, 0}, {close, 0}}});
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, file_spec}}, .memory = spec, .label = "write the table, the flag fails",
                     .answers = {{open, handle}, {close, 0}}, .nth_answers = {{write, 1, 0}}});
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, file_spec}}, .memory = joined(spec, {{1, w.enabled_at, 0}}),
                     .label = "write the table with the fog off", .answers = {{open, handle}, {close, 0}, {write, 1}}});
    cases.push_back({.entry = write_table, .registers = {{3, w.table}, {4, file_spec}}, .memory = spec, .label = "write the table, a present word fails",
                     .answers = {{open, handle}, {close, 0}}, .nth_answers = {{write, 1, 1}, {write, 2, 0}}});
    const auto sparse = scratch_base + 0x700;
    std::vector<Write> sparse_table = spec;
    for (std::uint32_t slot = 0; slot < 16; ++slot) sparse_table.push_back({4, sparse + slot * 4, slot == 1 ? k : 0});
    cases.push_back({.entry = write_table, .registers = {{3, sparse}, {4, file_spec}}, .memory = sparse_table, .label = "write a table, an absent word fails",
                     .answers = {{open, handle}, {close, 0}}, .nth_answers = {{write, 1, 1}, {write, 2, 0}}});
    cases.push_back({.entry = write_table, .registers = {{3, sparse}, {4, file_spec}}, .memory = sparse_table, .label = "write a sparse table",
                     .answers = {{open, handle}, {close, 0}, {write, 1}}});
    // The table's reader. Old saved games start every human afresh; newer ones read each slot.
    std::vector<Write> empty = spec;
    const auto fresh = scratch_base + 0x800;
    for (std::uint32_t slot = 0; slot < 16; ++slot) empty.push_back({4, fresh + slot * 4, 0});
    const auto reader = [&](std::string label, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers,
                            std::vector<Case::NthAnswer> nth, std::uint32_t present_word, std::uint32_t index_word) {
        memory.push_back({4, scenario.toc(version_cell), 0x3e040});
        answers.push_back({open, handle});
        answers.push_back({close, 0});
        return Case{.entry = read_table, .registers = {{3, fresh}, {4, file_spec}}, .memory = std::move(memory), .label = std::move(label),
                    .answers = std::move(answers), .pokes = {{read, {{4, present, present_word}, {4, index, index_word}}}},
                    .nth_answers = std::move(nth)};
    };
    cases.push_back({.entry = read_table, .registers = {{3, fresh}, {4, file_spec}}, .memory = joined(empty, {{4, scenario.toc(version_cell), 0x3e035}}),
                     .label = "read an old saved game", .answers = {{progress, 0}, {seasons_since_club, 2}}});
    cases.push_back({.entry = read_table, .registers = {{3, fresh}, {4, 0}}, .memory = joined(empty, {{4, scenario.toc(version_cell), 0xfffc1fca}}),
                     .label = "read the table, no file (the version's magnitude)"});
    cases.push_back(reader("read the table, not opened", empty, {{open, 0}}, {}, 0, 0));
    cases.push_back(reader("read the table, the flag fails", empty, {}, {{read, 1, 0}}, 0, 0));
    cases.push_back(reader("read the table with the fog off", joined(empty, {{1, w.enabled_at, 0}}), {{read, 1}}, {}, 0, 0));
    cases.push_back(reader("read an empty table", empty, {{read, 1}}, {}, 0, 0));
    cases.push_back(reader("read the table, a present word fails", empty, {}, {{read, 1, 1}, {read, 2, 0}}, 1, 0));
    std::vector<Case::NthAnswer> full = {{read, 1, 1}};
    for (unsigned slot = 0, at = 2; slot < 16; ++slot) { full.push_back({read, at++, 1}); full.push_back({read, at++, 1}); full.push_back({read, at++, count}); }
    cases.push_back(reader("read a full table", empty, {}, full, 1, scenario.r32(manager)));
    cases.push_back(reader("read the table, an object's index is bad", empty, {}, {{read, 1, 1}, {read, 2, 1}, {read, 3, 1}}, 1, count + 1));
    cases.push_back(reader("read the table, no memory for an object", empty, {{allocate_object, 0}}, {{read, 1, 1}, {read, 2, 1}}, 1, 0));
    // The table's constructor: the fog off, no file, a file read, and a failed read that starts everyone afresh.
    cases.push_back({.entry = construct_table, .registers = {{3, fresh}, {4, file_spec}}, .memory = joined(empty, {{1, w.enabled_at, 0}}), .label = "a table with the fog off"});
    cases.push_back({.entry = construct_table, .registers = {{3, fresh}, {4, 0}}, .memory = empty, .label = "a table without a file"});
    cases.push_back({.entry = construct_table, .registers = {{3, fresh}, {4, file_spec}}, .memory = joined(empty, {{4, scenario.toc(version_cell), 0x3e040}}),
                     .label = "a table from an empty file", .answers = {{open, handle}, {close, 0}, {read, 1}},
                     .pokes = {{read, {{4, scenario.sp() - 0x50 - 0x150 + 0x38, 0}}}}});
    cases.push_back({.entry = construct_table, .registers = {{3, fresh}, {4, file_spec}}, .memory = joined(empty, {{4, scenario.toc(version_cell), 0x3e040}}),
                     .label = "a table whose file fails", .answers = {{open, 0}, {progress, 0}, {seasons_since_club, 0}}});
}
}
}

// Branches the career's own managers and people don't show: the manager's position (club, national side,
// nation) against a person's, with the person a copy of a real player row and the contract and club-code
// lookups answered where a branch turns on them. Each base runs over ages, reputations and caps either side
// of the rules' thresholds.
namespace cm::aot::fog_of_war_checks {
namespace {
constexpr std::uint32_t primary_contract = 0x10082f20, club_of_code = 0x100ce320, attribute_rating = 0x100d2f50;

FOG_OF_WAR_CASES("branches") {
    if (scenario.match || w_empty(scenario)) return;
    const World w(scenario);
    const auto k = w.knowledge.front(), manager = w.managers.front();
    const auto manager_nation = scenario.r32(manager + 0xc), manager_club = w.club_of(manager);
    const auto clubs = scenario.global(-0x7be8);
    const auto nation_of = [&](std::uint32_t club) { return scenario.r32(club + 0x2c); };
    // A plain player (not famous), a club abroad (another nation, ranked below 1000) and a club in the manager's nation.
    std::uint32_t player = 0, abroad = 0, home = 0, third = 0;
    for (std::int32_t n = 0; n < w.count && !(player && abroad && home && third); ++n) {
        const auto person = w.row(n), club = w.club_of(person), record = scenario.r32(person + 0x14);
        if (!player && record && club && club != manager_club && scenario.r32(person + 0x20) == 0 &&
            std::int16_t(scenario.s.memory.read<2>(record + 0xc)) < 3000) player = person;
        if (club && !abroad && nation_of(club) && nation_of(club) != manager_nation &&
            (!manager_club || nation_of(club) != nation_of(manager_club)) && scenario.r32(nation_of(club) + 0x6c) < 1000) abroad = club;
        if (club && !home && nation_of(club) == manager_nation && club != manager_club) home = club;
        if (club && abroad && !third && nation_of(club) && nation_of(club) != manager_nation && nation_of(club) != nation_of(abroad)) third = club;
    }
    if (!player || !abroad || !home || !third) throw std::runtime_error("fog_of_war branches: the career lacks a plain player or clubs");
    const auto record = scenario.r32(player + 0x14);
    const auto copy = scratch_base + 0x3100, post = scratch_base + 0x3000;
    const auto copy_with = [&](std::vector<Write> fields) { return joined(w.person_copy(copy, player, scenario.r32(player)), fields); };
    const auto post_naming = [&](std::uint32_t club) { return std::vector<Write>{{4, post + 4, (club - clubs) / 0x248}}; };
    struct Base { const char* name; std::vector<Write> memory; std::vector<std::pair<std::uint32_t, std::uint32_t>> answers; };
    const std::vector<Base> bases = {
        {"contracted to the manager's club", joined(copy_with({{4, copy + 0x24, abroad}}), post_naming(manager_club)), {{primary_contract, post}}},
        {"the manager's national side", copy_with({{4, copy + 0x24, 0}, {4, copy + 0x20, abroad}, {4, manager + 0x20, abroad}}), {}},
        {"a club coded as the manager's", copy_with({{4, copy + 0x24, abroad}}), {{club_of_code, manager_club}}},
        {"a contract coded as the manager's club", joined(copy_with({{4, copy + 0x24, 0}}), post_naming(abroad)),
         {{primary_contract, post}, {club_of_code, manager_club}}},
        {"a side's nation, the manager clubless", copy_with({{4, copy + 0x24, abroad}, {4, manager + 0x24, 0}, {4, manager + 0x20, abroad}}),
         {{primary_contract, post}, {club_of_code, abroad}}},
        {"a club in the manager's nation, the manager clubless", joined(copy_with({{4, copy + 0x24, home}, {4, manager + 0x24, 0}, {4, manager + 0x20, 0}}),
                                                                        post_naming(abroad)), {{primary_contract, post}, {club_of_code, abroad}}},
        {"a clubless compatriot, the manager clubless", joined(copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, manager_nation}, {4, manager + 0x24, 0}, {4, manager + 0x20, 0}}),
                                                               post_naming(abroad)), {{primary_contract, post}, {club_of_code, abroad}}},
        {"a clubless compatriot", copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, manager_nation}}), {}},
        {"a foreigner at a club in the manager's nation", copy_with({{4, copy + 0x24, home}, {4, copy + 0xc, nation_of(abroad)}}), {}},
        {"a national side's compatriot", joined(copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, nation_of(abroad)}, {4, manager + 0x24, 0}, {4, manager + 0x20, abroad}}),
                                                post_naming(abroad)), {{primary_contract, post}, {club_of_code, abroad}}},
        {"a national side's compatriot, its nation ranked 1000", joined(copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, nation_of(abroad)}, {4, manager + 0x24, 0},
                                                                                   {4, manager + 0x20, abroad}, {4, nation_of(abroad) + 0x6c, 1000}}),
                                                                         post_naming(abroad)), {{primary_contract, post}, {club_of_code, abroad}}},
        {"a person with neither record", copy_with({{4, copy + 0x14, 0}, {4, copy + 0x1c, 0}}), {}},
        {"a side abroad, a person at a club at home, the manager clubless",
         joined(copy_with({{4, copy + 0x24, home}, {4, manager + 0x24, 0}, {4, manager + 0x20, abroad}}), post_naming(third)),
         {{primary_contract, post}, {club_of_code, third}}},
        {"the manager abroad, a clubless person of that nation",
         joined(copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, nation_of(abroad)}, {4, manager + 0x24, abroad}, {4, manager + 0x20, 0}}), post_naming(home)),
         {{primary_contract, post}, {club_of_code, home}}},
        {"a national side's foreigner", joined(copy_with({{4, copy + 0x24, 0}, {4, copy + 0xc, nation_of(third)}, {4, manager + 0x24, 0}, {4, manager + 0x20, abroad}}),
                                               post_naming(home)), {{primary_contract, post}, {club_of_code, home}}},
    };
    for (const auto& base : bases)
        for (const std::uint32_t age : {18u, 30u})
            for (const std::uint32_t reputation : {0u, 4500u, 6000u})
                for (const std::uint32_t caps : {0u, 200u}) {
                    auto memory = base.memory;
                    memory.push_back({2, copy + 0x5c, age});
                    memory.push_back({1, copy + 0x5e, caps});
                    memory.push_back({2, record + 0xa, reputation});
                    memory.push_back({2, record + 0xc, reputation});
                    const auto label = std::string(base.name) + ", age " + std::to_string(age) + ", reputation " + std::to_string(reputation) + ", caps " + std::to_string(caps);
                    auto answers = base.answers;
                    answers.push_back({0x10309cc0, 0xffffffffu});
                    cases.push_back({.entry = initial_level, .registers = {{3, k}, {4, manager}, {5, copy}}, .memory = memory, .label = "initial: " + label, .answers = answers});
                    cases.push_back({.entry = level_floor, .registers = {{3, k}, {4, manager}, {5, copy}}, .memory = memory, .label = "floor: " + label, .answers = answers});
                    cases.push_back({.entry = level_of, .registers = {{3, k}, {4, copy}}, .memory = memory, .label = "level: " + label, .answers = answers});
                    cases.push_back({.entry = famous, .registers = {{3, k}, {4, copy}}, .memory = memory, .label = "famous: " + label});
                    cases.push_back({.entry = well_known, .registers = {{3, k}, {4, copy}}, .memory = memory, .label = "well known: " + label});
                }
    // Judgements across the odds: none (no draw), low, the threshold of 15, high and negative bytes.
    std::uint32_t judge = 0;
    for (std::int32_t n = 0; n < w.count && !judge; ++n) if (scenario.r32(w.row(n) + 0x1c)) judge = w.row(n);
    std::vector<std::uint32_t> by_level[3];
    for (std::int32_t n = 0; n < w.count; ++n) {
        const auto level = w.level(k, w.row(n));
        if (level < 3 && by_level[level].size() < 3) by_level[level].push_back(w.row(n));
    }
    for (const std::uint32_t judgement : {0u, 1u, 5u, 14u, 15u, 20u, 0x7fu, 0x80u, 0xffu, 0x1f0u})
        for (unsigned level = 0; level < 3; ++level)
            for (const auto person : by_level[level])
                cases.push_back({.entry = sight, .registers = {{3, k}, {4, person}, {5, judge}},
                                 .label = "judged " + hex(judgement) + ", level " + std::to_string(level), .answers = {{attribute_rating, judgement}}});
    // A match with the manager's slot empty, a sheet line whose row is address 0, and one whose row's id is past the table.
    const auto match = scratch_base + 0x1000, sheets = scratch_base + 0x2000;
    const auto people = scenario.global(-0x7be0);
    std::vector<Write> lines = {{4, match + 0x20, manager_club}, {4, match + 0x1c, abroad}};
    for (std::uint32_t side = 0; side < 2; ++side)
        for (std::uint32_t n = 0; n < 20; ++n) {
            const auto line = sheets + side * 0x664 + 0x194 + n * 0x4c;
            lines.push_back({4, line, n});
            lines.push_back({4, line + 4, 1});
            lines.push_back({1, line + 0x32, 0});
        }
    auto empty_slot = lines;
    for (std::uint32_t n = 0; n < 16; ++n) if (scenario.r32(w.table + n * 4) == k) empty_slot.push_back({4, w.table + n * 4, 0});
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = empty_slot, .label = "a match, the manager's slot empty"});
    auto no_manager = lines;
    no_manager.push_back({4, manager_club + 0x90, 0});
    no_manager.push_back({4, abroad + 0x90, 0});
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = no_manager,
                     .label = "a match, a human-run club without its manager row", .answers = {{0x1014c2e0, 1}}});
    auto odd_rows = lines;
    if (people % 16 == 0) {
        // index * 0x70 == -people (mod 2^32): 0x70 = 16 * 7, and 7's inverse mod 2^28 is 0x6db6db7.
        const auto index = std::uint32_t((std::uint64_t((0u - people) / 16) * 0x6db6db7u) & 0x0fffffffu);
        odd_rows.push_back({4, sheets + 0x194, index});
    }
    const auto past_row = people + ((scratch_base + 0x4000 - people) / 0x70 + 1) * 0x70;
    odd_rows.push_back({4, sheets + 0x194 + 0x4c, (past_row - people) / 0x70});
    odd_rows.push_back({4, past_row, std::uint32_t(w.count) + 1});
    cases.push_back({.entry = record_match, .registers = {{3, 0}, {4, match}, {5, sheets}}, .memory = odd_rows, .label = "a match naming row 0 and a row past the table"});
}
}
}

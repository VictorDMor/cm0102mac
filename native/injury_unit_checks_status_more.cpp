// INJURY.CPP checker cases for the rest of the status block: the daily update, the stage and fitness
// questions, the entry list, the injury choice and the draws.
#include "injury_unit_checks.hpp"
#include <algorithm>
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t random_below = 0x101864a0, set_sharpness = 0x10186a20, note_entry = 0x10186cd0,
                        find_entry = 0x10186e10, reset_status = 0x10186ea0, match_injuries = 0x10186fc0,
                        wear_condition = 0x10187450, injury_stage = 0x10187680, sharpness_of = 0x101877e0,
                        match_fitness_of = 0x101878d0, update_status = 0x10187a40, pick_injury_type = 0x10188930,
                        close_injury = 0x1018b9f0;

struct People {
    std::vector<std::uint32_t> injured, healthy;
};
People sample_people(const Scenario& scenario, std::uint32_t manager) {
    const auto rows = scenario.r32(manager);
    const auto people = scenario.people();
    People found;
    // Only people with a record (+0x14): the routines read through it.
    const auto has_profile = [&](std::uint32_t index) { return scenario.r32(scenario.person(index) + 0x14) != 0; };
    for (std::uint32_t index = 0; index < people && (found.injured.size() < 10 || found.healthy.size() < 10); index += 11) {
        if (!has_profile(index)) continue;
        auto& list = scenario.r32(rows + index * 0x24 + 0x10) == 0xffffffff ? found.healthy : found.injured;
        if (list.size() < 10) list.push_back(index);
    }
    for (std::uint32_t index = 0; index < people && found.injured.size() < 10; ++index)
        if (has_profile(index) && scenario.r32(rows + index * 0x24 + 0x10) != 0xffffffff && std::ranges::find(found.injured, index) == found.injured.end())
            found.injured.push_back(index);
    if (found.healthy.empty()) found.healthy.push_back(0);
    if (found.injured.empty()) found.injured.push_back(found.healthy[0]);
    return found;
}
std::string number(std::uint32_t value) { return std::to_string(value); }
// Closing an injury writes the injury history file (0x10748ce0: FSMakeFSSpec / HGetVol, which the native bridge does
// not implement): it is answered (r3 = 0) on both sides so the paths that reach it return and are compared.
void answer_history_file(std::vector<Case>& cases, std::size_t from) {
    for (auto at = from; at < cases.size(); ++at) cases[at].answers.push_back({0x10748ce0, 0});
}
}

INJURY_UNIT_CASES("status update") {
    const auto first_case = cases.size();
    const auto manager = scenario.global(-0x7ba0);
    const auto rows = scenario.r32(manager);
    const auto injuries = scenario.r32(manager + 4);
    const auto today = scenario.toc(-0x7b80);
    const auto today_day = std::uint32_t(scenario.s.memory.read<2>(today));
    const auto today_year = std::uint32_t(scenario.s.memory.read<2>(today + 2));
    const auto people = sample_people(scenario, manager);
    const auto back = [&](std::uint32_t days) { return std::min(days, today_day); };
    // The row dated `days` before today.
    const auto dated = [&](std::uint32_t row, std::uint32_t days) {
        return std::vector<Write>{{2, row, today_day - back(days)}, {2, row + 2, today_year}};
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) {
        a.insert(a.end(), b.begin(), b.end());
        return a;
    };
    struct Course { std::uint32_t elapsed, since, unfit, out; const char* name; };
    const Course courses[] = {{5, 2, 10, 30, "before unfit"}, {5, 12, 10, 30, "inside, short"}, {25, 11, 10, 30, "inside, long"},
                              {30, 35, 10, 30, "past, long"}, {20, 35, 10, 30, "past, short"}, {1, 35, 10, 30, "past, one day"}};
    for (const auto index : people.injured) {
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        const auto record = injuries + scenario.r32(row + 0x10) * 0x38;
        if (scenario.r32(row + 0x10) == 0xffffffff) continue;
        for (const auto& course : courses) {
            auto memory = dated(row, course.elapsed);
            memory.push_back({2, record + 0x28, today_day - back(course.since)});
            memory.push_back({2, record + 0x2a, today_year});
            memory.push_back({2, record + 0x32, course.unfit});
            memory.push_back({2, record + 0x30, course.out});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                             .label = std::string("update injured ") + number(index) + " " + course.name});
        }
        cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = dated(row, 3),
                         .label = "update injured " + number(index) + " as is, 3 days"});
    }
    const auto bad_index = [&](std::uint32_t row, std::uint32_t value, const char* name) {
        cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, scenario.person(people.healthy[0])}},
                         .memory = join(dated(row, 4), {{4, row + 0x10, value}}), .label = std::string("update ") + name});
    };
    bad_index(rows + people.healthy[0] * 0x24, 0x7fffffff, "injury index past the records");
    bad_index(rows + people.healthy[0] * 0x24, 0xfffffffe, "injury index below -1");
    bad_index(rows + people.healthy[0] * 0x24, scenario.r32(manager + 8), "injury index at the count");
    for (const auto index : people.healthy) {
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        for (const std::uint32_t days : {1u, 4u, 20u, 60u, 200u}) {
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = dated(row, days),
                             .label = "update healthy " + number(index) + " after " + number(days) + " days"});
            // No club entry; no club; neither.
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}},
                             .memory = join(dated(row, days), {{4, person + 0x20, 0}}),
                             .label = "update healthy " + number(index) + " without club entry, " + number(days)});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}},
                             .memory = join(dated(row, days), {{4, person + 0x24, 0}}),
                             .label = "update healthy " + number(index) + " without club, " + number(days)});
        }
        // The sample table redraws, with fitness and sharpness at several levels.
        const auto cursor_cell = scenario.toc(-0x7b84);
        const auto last = scenario.global(-0x7b88);
        for (const std::uint32_t fitness : {3000u, 9000u, 9900u, 7500u}) {
            auto memory = join(dated(row, 30), {{2, row + 8, fitness}, {2, row + 0xa, fitness}, {2, row + 0xc, 1500}});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                             .label = "update healthy " + number(index) + " fitness " + number(fitness)});
            memory.push_back({4, cursor_cell, last});
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}}, .memory = memory,
                             .label = "update healthy " + number(index) + " fitness " + number(fitness) + " redraw"});
        }
        for (const std::uint32_t fitness : {500u, 3000u, 8000u, 9500u, 9900u, 10000u})
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, person}},
                             .memory = join(dated(row, 2), {{2, row + 8, fitness}, {2, row + 0xa, 6000}, {2, row + 0xc, 0xfc00}}),
                             .label = "update healthy " + number(index) + " 2 days fitness " + number(fitness) + " low condition"});
    }
    // A row already dated today, a null person, one past the table, one without a record.
    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, scenario.person(people.healthy[0])}},
                     .memory = dated(rows + people.healthy[0] * 0x24, 0), .label = "update dated today"});
    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, 0}}, .label = "update null person"});
    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, scratch_base + 0x100}},
                     .memory = {{4, scratch_base + 0x100, scenario.people()}}, .label = "update past the table"});
    cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, scenario.person(people.healthy[0])}},
                     .memory = {{4, scenario.person(people.healthy[0]) + 0x14, 0}}, .label = "update without record"});
    // The final formula at each level of fitness, with the person's trait byte and the row's flag byte varied.
    for (const std::uint32_t fitness : {0x2000u, 0x2400u, 0x2700u, 0x2800u, 0x1000u, 0x1c00u, 0x2380u}) {
        const auto index = people.healthy[0];
        const auto row = rows + index * 0x24;
        const auto profile = scenario.r32(scenario.person(index) + 0x14);
        for (const std::uint32_t trait : {0u, 20u, 0xf0u, 0x7fu})
            cases.push_back({.entry = update_status, .registers = {{3, manager}, {4, scenario.person(index)}},
                             .memory = join(dated(row, 90), {{2, row + 8, fitness}, {2, row + 0xa, 9000}, {2, row + 0xc, 1200},
                                                            {1, row + 0x22, 70}, {1, profile + 0x34, trait}}),
                             .label = "update formula fitness " + number(fitness) + " trait " + number(trait)});
    }
    answer_history_file(cases, first_case);
}

INJURY_UNIT_CASES("status questions") {
    const auto first_case = cases.size();
    const auto manager = scenario.global(-0x7ba0);
    const auto rows = scenario.r32(manager);
    const auto people = sample_people(scenario, manager);
    const auto count = scenario.people();
    const auto today = scenario.toc(-0x7b80);
    const auto today_day = std::uint32_t(scenario.s.memory.read<2>(today));
    const auto injuries = scenario.r32(manager + 4);
    std::vector<std::uint32_t> all;
    for (const auto index : people.injured) all.push_back(index);
    for (const auto index : people.healthy) all.push_back(index);
    for (const auto index : all) {
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        for (const std::uint32_t percent : {0u, 1u}) {
            cases.push_back({.entry = sharpness_of, .registers = {{3, manager}, {4, person}, {5, percent}},
                             .label = "sharpness " + number(index) + " percent " + number(percent)});
            cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, person}, {5, percent}},
                             .label = "match fitness " + number(index) + " percent " + number(percent)});
            cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, person}, {5, percent}},
                             .memory = {{2, row + 8, 9800}, {2, row + 0xa, 9900}}, .label = "match fitness high " + number(index) + " " + number(percent)});
            cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, person}, {5, percent}},
                             .memory = {{2, row + 8, 100}, {2, row + 0xa, 100}}, .label = "match fitness low " + number(index) + " " + number(percent)});
        }
        cases.push_back({.entry = injury_stage, .registers = {{3, manager}, {4, person}}, .label = "stage " + number(index)});
        cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, person}, {5, 0}},
                         .memory = {{2, row + 8, 9800}, {2, row + 0xa, 9900}}, .label = "match fitness high " + number(index)});
    }
    // Injury stages: an injury that has not lasted its first length, lasted past it, past the second.
    for (const auto index : people.injured) {
        if (scenario.r32(rows + index * 0x24 + 0x10) == 0xffffffff) continue;
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        const auto record = injuries + scenario.r32(row + 0x10) * 0x38;
        for (const std::uint32_t since : {0u, 8u, 40u})
            for (const std::uint32_t percent : {0u, 1u}) {
                std::vector<Write> memory{{2, row, today_day}, {2, record + 0x28, today_day - std::min(since, today_day)},
                                          {2, record + 0x32, 5}, {2, record + 0x30, 20}};
                cases.push_back({.entry = injury_stage, .registers = {{3, manager}, {4, person}}, .memory = memory,
                                 .label = "stage " + number(index) + " since " + number(since)});
                cases.push_back({.entry = match_fitness_of, .registers = {{3, manager}, {4, person}, {5, percent}}, .memory = memory,
                                 .label = "match fitness " + number(index) + " since " + number(since)});
            }
    }
    // Null and out-of-range people.
    for (const auto entry : {injury_stage, sharpness_of, match_fitness_of, close_injury}) {
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, 0}, {5, 0}}, .label = "null person"});
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, 0}, {5, 1}}, .label = "null person, flag"});
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, scratch_base + 0x100}, {5, 0}},
                         .memory = {{4, scratch_base + 0x100, count}}, .label = "person past the table"});
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, scratch_base + 0x100}, {5, 1}},
                         .memory = {{4, scratch_base + 0x100, count}}, .label = "person past the table, flag"});
    }
    // Wear and sharpness set.
    for (const auto index : all) {
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        const auto profile = scenario.r32(person + 0x14);
        for (const std::uint32_t condition : {0xf000u, 0xfc18u, 0xfd00u, 0xfe0cu, 0xffffu, 0u, 300u, 1100u, 1200u, 1250u, 1500u, 2000u}) {
            for (const std::uint32_t form : {0x0005u, 0x0a14u, 0x0023u}) {
                std::vector<Write> memory{{2, row + 0xc, condition}, {2, person + 0x5c, form}};
                cases.push_back({.entry = wear_condition, .registers = {{3, manager}, {4, index}}, .memory = memory,
                                 .label = "wear " + number(index) + " condition " + number(condition) + " form " + number(form)});
                memory.push_back({1, profile + 0xf, 0x14});
                cases.push_back({.entry = wear_condition, .registers = {{3, manager}, {4, index}}, .memory = memory,
                                 .label = "wear " + number(index) + " condition " + number(condition) + " form " + number(form) + " keeper"});
                const auto cursor_cell = scenario.toc(-0x7b84);
                memory.push_back({4, cursor_cell, scenario.global(-0x7b88)});
                cases.push_back({.entry = wear_condition, .registers = {{3, manager}, {4, index}}, .memory = memory,
                                 .label = "wear " + number(index) + " condition " + number(condition) + " redraw"});
            }
            for (const std::uint32_t change : {0u, 3u, 0xfff9u, 20u})
                cases.push_back({.entry = set_sharpness, .registers = {{3, manager}, {4, person}, {5, 7777}, {6, change}},
                                 .memory = {{2, row + 0xc, condition}, {1, profile + 0x44, 3}, {1, profile + 0x3c, 4}, {1, person + 0x67, 2}},
                                 .label = "set sharpness " + number(index) + " condition " + number(condition) + " change " + number(change)});
        }
        cases.push_back({.entry = set_sharpness, .registers = {{3, manager}, {4, person}, {5, 1234}, {6, 4}},
                         .memory = {{2, row + 0xc, 1230}, {4, scenario.toc(-0x7b84), scenario.global(-0x7b88)}},
                         .label = "set sharpness " + number(index) + " redraw"});
    }
    for (const auto index : {0u, count / 2, count - 1, count, count + 3})
        cases.push_back({.entry = wear_condition, .registers = {{3, manager}, {4, index}}, .label = "wear index " + number(index)});
    cases.push_back({.entry = set_sharpness, .registers = {{3, manager}, {4, 0}, {5, 1}, {6, 1}}, .label = "set sharpness null person"});
    cases.push_back({.entry = set_sharpness, .registers = {{3, manager}, {4, scratch_base + 0x100}, {5, 1}, {6, 1}},
                     .memory = {{4, scratch_base + 0x100, count}}, .label = "set sharpness past the table"});
    answer_history_file(cases, first_case);
}

INJURY_UNIT_CASES("status injury") {
    const auto first_case = cases.size();
    const auto manager = scenario.global(-0x7ba0);
    const auto rows = scenario.r32(manager);
    const auto people = sample_people(scenario, manager);
    std::vector<std::uint32_t> all(people.injured);
    all.insert(all.end(), people.healthy.begin(), people.healthy.end());
    for (const auto index : all) {
        const auto person = scenario.person(index);
        const auto row = rows + index * 0x24;
        for (const std::uint32_t report : {0u, 1u}) {
            cases.push_back({.entry = close_injury, .registers = {{3, manager}, {4, person}, {5, report}},
                             .label = "close " + number(index) + " report " + number(report)});
            cases.push_back({.entry = close_injury, .registers = {{3, manager}, {4, person}, {5, report}},
                             .memory = {{1, scenario.r32(manager + 4) + scenario.r32(row + 0x10) * 0x38, 0x51}},
                             .label = "close " + number(index) + " report " + number(report) + " kind 0x51"});
        }
        cases.push_back({.entry = reset_status, .registers = {{3, manager}, {4, index}}, .label = "reset " + number(index)});
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffu})
            for (const std::uint32_t severity : {0xffffffffu, 0u, 1u, 2u, 3u})
                cases.push_back({.entry = pick_injury_type, .registers = {{3, manager}, {4, kind}, {5, severity}, {6, person}},
                                 .label = "pick " + number(index) + " kind " + number(kind) + " severity " + number(severity)});
        const auto cursor_cell = scenario.toc(-0x7b84);
        cases.push_back({.entry = pick_injury_type, .registers = {{3, manager}, {4, 1}, {5, 0xffffffff}, {6, person}},
                         .memory = {{4, cursor_cell, scenario.global(-0x7b88)}}, .label = "pick " + number(index) + " redraw"});
        // A current injury of a major type is kept; of a minor type it is chosen again.
        if (scenario.r32(row + 0x10) != 0xffffffff) {
            const auto record = scenario.r32(manager + 4) + scenario.r32(row + 0x10) * 0x38;
            for (const std::uint32_t type : {0u, 2u, 0x4cu, 0x51u, 0x20u})
                for (const std::uint32_t kind : {1u, 3u})
                    cases.push_back({.entry = pick_injury_type, .registers = {{3, manager}, {4, kind}, {5, 0xffffffff}, {6, person}},
                                     .memory = {{1, record, type}, {1, row + 0x14, 5}},
                                     .label = "pick " + number(index) + " current type " + number(type) + " kind " + number(kind)});
        }
    }
    cases.push_back({.entry = pick_injury_type, .registers = {{3, manager}, {4, 1}, {5, 0xffffffff}, {6, 0}}, .label = "pick null person"});
    // The entry list: keys in scratch memory, kind and strength bytes.
    const auto key = scratch_base + 0x200, kind_cell = scratch_base + 0x210, strength_cell = scratch_base + 0x211;
    for (const std::uint32_t k : {1u, 7u, 100u})
        for (const std::uint32_t kind : {1u, 3u})
            for (const std::uint32_t strength : {10u, 60u, 95u, 120u, 0xf0u}) {
                cases.push_back({.entry = note_entry, .registers = {{3, manager}, {4, key}, {5, kind_cell}, {6, strength_cell}},
                                 .memory = {{4, key, k}, {1, kind_cell, kind}, {1, strength_cell, strength}},
                                 .label = "note " + number(k) + " kind " + number(kind) + " strength " + number(strength)});
            }
    for (const std::uint32_t k : {1u, 7u, 100u, 5000u})
        cases.push_back({.entry = find_entry, .registers = {{3, manager}, {4, key}}, .memory = {{4, key, k}}, .label = "find " + number(k)});
    // The draw.
    const auto cursor_cell = scenario.toc(-0x7b84);
    const std::int32_t ranges[] = {0, 1, 2, 7, 100, 0xffff, 0x10000, 0x12345, 0x7fffffff, -1, -5, -0xffff, -0x10000, -0x23456,
                                   int(0x80000000), 654321, 100000000, 5000000};
    for (const auto n : ranges) {
        cases.push_back({.entry = random_below, .registers = {{3, std::uint32_t(n)}}, .label = "random below " + std::to_string(n)});
        cases.push_back({.entry = random_below, .registers = {{3, std::uint32_t(n)}}, .memory = {{4, cursor_cell, scenario.global(-0x7b88)}},
                         .label = "random below " + std::to_string(n) + " redraw"});
    }
    // The sides' players: 2 x 20 entries of 0x4c bytes at +0x194 of a 0x664-byte side.
    const auto match = scratch_base + 0x1000, options = scratch_base + 0x800;
    for (const std::uint32_t full : {0u, 1u})
        for (const std::uint32_t flag : {0u, 1u})
            for (const std::uint32_t first : {0xfffffffeu, 0u}) {
                std::vector<Write> memory{{4, options, first}, {1, options + 0x40, full}, {1, match + 0x4c, flag}, {2, match + 0x4e, 0x0a37}};
                for (unsigned side = 0; side < 2; ++side)
                    for (unsigned slot = 0; slot < 20; ++slot) {
                        const auto entry = match + side * 0x664 + slot * 0x4c;
                        const auto who = slot % 5 == 4 ? 0xffffffffu : all[(side * 20 + slot) % all.size()];
                        memory.push_back({4, entry + 0x194, who});
                        memory.push_back({1, entry + 0x1b3, slot % 3 == 0 ? 1u : 0u});
                        memory.push_back({1, entry + 0x1c4, slot % 4 == 0 ? 0xf0u : 30u + slot});
                        memory.push_back({1, entry + 0x1c6, slot % 7 == 0 ? 0xffu : slot});
                        memory.push_back({1, entry + 0x1c8, slot % 3 == 1 ? 0xf0u : 40u + slot});
                        memory.push_back({2, entry + 0x1ca, slot * 500});
                    }
                cases.push_back({.entry = match_injuries, .registers = {{3, manager}, {4, options}, {5, match}}, .memory = memory,
                                 .label = "match injuries full " + number(full) + " flag " + number(flag) + " first " + number(first)});
            }
    answer_history_file(cases, first_case);
}
}

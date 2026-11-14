// INJURY.CPP checker cases for the course block (injury_unit_course.hpp): the comparators, the event queue,
// the injury course of one person and the daily pass. Records are the career's own (injured people, their
// clubs and squads); Case::memory moves the dates, the sample table's base and the attribute bytes that
// the rules read, so each branch runs on a real record. Rows the queues need are built in the scratch area.
#include "injury_unit_checks.hpp"
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t daily_pass = 0x1018b2b0, queue_event = 0x1018bb50, run_events = 0x1018bdd0,
                        injury_course = 0x1018dcb0, compare_b = 0x1018ff10, compare_a = 0x1018ffe0, compare_event = 0x101900b0;
constexpr std::uint32_t key_area = scratch_base + 0x100, row_area = scratch_base + 0x200, date_area = scratch_base + 0x300,
                        queue_area = scratch_base + 0x1000, due_area = scratch_base + 0x2000;

struct Date { std::uint16_t day, year; };
void put_date(std::vector<Write>& memory, std::uint32_t at, Date date) {
    memory.push_back({2, at, date.day});
    memory.push_back({2, at + 2, date.year});
    memory.push_back({1, at + 4, 0});
}
}

INJURY_UNIT_CASES("course comparators") {
    if (scenario.match) return;
    const Date dates[] = {{10, 2001}, {11, 2001}, {10, 2002}, {0xfff0, 2001}, {5, 2001}, {0x7fff, 2000}, {0x8000, 2000}, {10, 2001}};
    for (const auto entry : {compare_a, compare_event, compare_b}) {
        const std::uint32_t offset = entry == compare_b ? 8 : 0;
        for (const auto& key : dates)
            for (const auto& row : dates) {
                Case c{.entry = entry, .registers = {{3, key_area}, {4, row_area}},
                       .label = "compare " + std::to_string(entry) + " " + std::to_string(key.day) + "/" + std::to_string(key.year) +
                                " vs " + std::to_string(row.day) + "/" + std::to_string(row.year)};
                put_date(c.memory, key_area + offset, key);
                put_date(c.memory, row_area + offset, row);
                cases.push_back(std::move(c));
            }
    }
}

INJURY_UNIT_CASES("course queue") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto base_cell = scenario.toc(-0x7b90);
    const auto today = scenario.toc(-0x7b80);
    const std::uint16_t day = std::uint16_t(scenario.r32(today) >> 16), year = std::uint16_t(scenario.r32(today));
    const auto injured = [&]() -> std::uint32_t {
        const auto rows = scenario.r32(manager);
        for (std::uint32_t i = 0; i < scenario.people(); ++i)
            if (scenario.r32(rows + i * 0x24 + 0x10) != 0xffffffff) return scenario.person(i);
        return 0;
    }();
    // queue_club_event: every kind, a spread of sample bases, with and without a person, full and empty queues.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 0x103u})
        for (const std::uint32_t base : {0u, 1u, 2u, 3u, 4u, 7u, 15u, 30u})
            for (const std::uint32_t club : {0u, 3u})
                for (const int shape : {0, 1, 2}) {
                    Case c{.entry = queue_event, .registers = {{3, manager}, {4, date_area}, {5, kind}, {6, club}, {7, shape == 2 ? 0 : injured}},
                           .label = "queue event kind " + std::to_string(kind) + " base " + std::to_string(base) + " club " +
                                    std::to_string(club) + " shape " + std::to_string(shape)};
                    c.memory.push_back({4, base_cell, base});
                    put_date(c.memory, date_area, {day, year});
                    if (shape == 1) {  // queue full: the buffer grows (real heap block, or an allocation if none)
                        c.memory.push_back({4, manager + 0x24, scenario.r32(manager + 0x20)});
                    } else if (shape == 2) {  // no buffer yet: allocated
                        c.memory.push_back({4, manager + 0x1c, 0});
                        c.memory.push_back({4, manager + 0x20, 0});
                        c.memory.push_back({4, manager + 0x24, 0});
                    }
                    cases.push_back(std::move(c));
                }

    // run_club_events: rows around today (future, today, expired, within the window), kinds 3 and others.
    struct Row { std::int32_t days; std::uint8_t kind; std::uint32_t club; };
    const std::vector<std::vector<Row>> queues = {
        {{-9, 1, 0}, {-3, 1, 1}, {0, 3, 2}, {0, 1, 3}, {2, 2, 4}, {9, 1, 5}},
        {{-2, 3, 0}, {-1, 2, 1}, {0, 3, 3}, {0, 3, 4}},
        {{0, 1, 0}, {0, 2, 1}, {0, 0, 2}, {1, 1, 3}},
        {{-20, 1, 0}, {-30, 3, 1}},
        {}};
    for (std::size_t q = 0; q < queues.size(); ++q)
        for (const std::uint32_t base : {0u, 1u, 2u, 5u, 11u, 40u, 77u}) {
            Case c{.entry = run_events, .registers = {{3, manager}},
                   .label = "run events queue " + std::to_string(q) + " base " + std::to_string(base)};
            c.memory.push_back({4, base_cell, base});
            c.memory.push_back({4, manager + 0x1c, queue_area});
            c.memory.push_back({4, manager + 0x24, std::uint32_t(queues[q].size())});
            std::uint32_t at = queue_area;
            for (const auto& row : queues[q]) {
                put_date(c.memory, at, {std::uint16_t(day + row.days), year});
                c.memory.push_back({1, at + 8, row.kind});
                c.memory.push_back({4, at + 0xc, row.club});
                c.memory.push_back({4, at + 0x10, 0});
                at += 0x14;
            }
            cases.push_back(std::move(c));
        }
}

INJURY_UNIT_CASES("course progress") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto base_cell = scenario.toc(-0x7b90);
    const auto rows = scenario.r32(manager);
    std::vector<std::uint32_t> people;  // injured people first, then others with a club
    for (std::uint32_t i = 0; i < scenario.people() && people.size() < 4; ++i)
        if (scenario.r32(rows + i * 0x24 + 0x10) != 0xffffffff && scenario.r32(scenario.person(i) + 0x24) != 0) people.push_back(scenario.person(i));
    for (std::uint32_t i = 0; i < scenario.people() && people.size() < 6; i += 997)
        if (scenario.r32(scenario.person(i) + 0x24) != 0) people.push_back(scenario.person(i));
    const std::uint32_t bases[] = {0, 1, 3, 4, 5, 8, 100, 29499, 29999 , 30001};
    for (const auto person : people)
        for (const std::uint32_t type : {0x4du, 0x4eu, 0x4fu, 0x51u, 0x10u, 0x1u})
            for (const std::uint32_t stage : {0u, 1u})
                for (const std::uint32_t queued : {0u, 1u})
                    for (const std::uint32_t base : bases)
                        for (const int shape : {0, 1}) {
                            Case c{.entry = injury_course, .registers = {{3, manager}, {4, person}, {5, stage}, {6, type}, {7, queued}, {8, 5}},
                                   .label = "course person " + std::to_string(person) + " type " + std::to_string(type) + " stage " +
                                            std::to_string(stage) + " queued " + std::to_string(queued) + " base " + std::to_string(base) +
                                            " shape " + std::to_string(shape)};
                            c.memory.push_back({4, base_cell, base});
                            // shape 1: attributes that reach the notices (high B and A, low D, nonzero C).
                            const std::uint8_t a = shape ? 18 : 3, b = shape ? 18 : 4, cc = shape ? 9 : 0, d = shape ? 2 : 9;
                            c.memory.push_back({1, person + 0x63, a});
                            c.memory.push_back({1, person + 0x65, b});
                            c.memory.push_back({1, person + 0x67, cc});
                            c.memory.push_back({1, person + 0x69, d});
                            cases.push_back(std::move(c));
                        }
    if (!people.empty()) {
        Case c{.entry = injury_course, .registers = {{3, manager}, {4, 0}, {5, 0}, {6, 0x4d}, {7, 0}, {8, 0}}, .label = "course no person"};
        cases.push_back(std::move(c));
    }
}

INJURY_UNIT_CASES("course daily pass") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto base_cell = scenario.toc(-0x7b90);
    const auto today = scenario.toc(-0x7b80);
    const std::uint16_t day = std::uint16_t(scenario.r32(today) >> 16), year = std::uint16_t(scenario.r32(today));
    const auto statuses = scenario.r32(manager), injuries = scenario.r32(manager + 4);
    // Injury records of people in the squads the pass visits: make them end today after ten days.
    std::vector<Write> ending;
    unsigned made = 0;
    const auto count = scenario.people();
    for (std::uint32_t group = 0; group < 16 && made < 4; ++group) {
        const auto club = scenario.r32(scenario.person(count - group - 1) + 0x24);
        if (!club) continue;
        for (std::uint32_t k = 0; k < 50 && made < 4; ++k) {
            const auto person = scenario.r32(club + 0x98 + 4 * k);
            if (!person) continue;
            const auto index = scenario.r32(statuses + scenario.r32(person) * 0x24 + 0x10);
            if (index == 0xffffffff) continue;
            const auto record = injuries + index * 0x38;
            put_date(ending, record + 0x28, {std::uint16_t(day >= 11 ? day - 10 : day), year});
            ending.push_back({2, record + 0x32, day >= 11 ? 10u : 0u});
            ending.push_back({2, record + 0x30, 14});
            ending.push_back({1, statuses + scenario.r32(person) * 0x24 + 0x20, 0});
            ending.push_back({1, record, made % 2 ? 0x4eu : 0x4du});
            ++made;
        }
    }
    for (const std::uint32_t base : {0u, 2u, 7u, 40u}) {
        Case c{.entry = daily_pass, .registers = {{3, manager}}, .label = "daily pass real state base " + std::to_string(base)};
        c.memory = ending;
        c.memory.push_back({4, base_cell, base});
        cases.push_back(std::move(c));
    }
    // The due queue: rows with a first date of today and rows with a second date of today, plus older rows
    // the pass drops, built over real people.
    const std::uint32_t person_index[] = {1, 2, 3, 4, 5, 6};
    struct Due { std::int32_t first, second; };
    const std::vector<std::vector<Due>> queues = {
        {{-2, -4}, {0, -1}, {0, 0}, {0, 1}, {1, 0}, {1, 3}},
        {{-3, -3}, {-1, 0}, {-1, 0}, {2, 0}},
        {{0, 2}, {0, 3}, {3, 5}},
        {{-5, -5}}};
    for (std::size_t q = 0; q < queues.size(); ++q)
        for (const std::uint32_t base : {0u, 5u}) {
            Case c{.entry = daily_pass, .registers = {{3, manager}}, .label = "daily pass due queue " + std::to_string(q) + " base " + std::to_string(base)};
            c.memory = ending;
            c.memory.push_back({4, base_cell, base});
            c.memory.push_back({4, manager + 0x10, due_area});
            c.memory.push_back({4, manager + 0x18, std::uint32_t(queues[q].size())});
            std::uint32_t at = due_area;
            for (std::size_t i = 0; i < queues[q].size(); ++i) {
                put_date(c.memory, at, {std::uint16_t(day + queues[q][i].first), year});
                c.memory.push_back({4, at + 4, 0});
                put_date(c.memory, at + 8, {std::uint16_t(day + queues[q][i].second), year});
                c.memory.push_back({4, at + 0x10, person_index[i]});
                c.memory.push_back({1, at + 0x14, 0});
                c.memory.push_back({1, at + 0x15, i % 2 ? 0x4eu : 0x4du});
                at += 0x18;
            }
            cases.push_back(std::move(c));
        }
}
}

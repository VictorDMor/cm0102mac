// INJURY.CPP checker cases for the text block (injury_unit_text.hpp): the questions about a status row
// (0x1018c270, 0x1018c2f0), the injury names and descriptions (0x1018c450, 0x1018c560), the language
// flag (0x1018d140), the time left (0x1018d950), the forms (0x1018dab0), the injury-81 question
// (0x1018f340), the news of a recovery and of a match (0x1018f470, 0x1018fbe0) and the two routines on
// the status row's date (0x1018c1a0, 0x1018fdf0).
//
// The injury manager, people, clubs and competitions are the game's own; the news records (fields from
// +8 for a recovery) and the text sink (a zero object) are built in the scratch area. Memory writes are
// undone after each case. The game's language (0x10727880) and the answers of the person and match tests
// (other modules' routines) are set with `answers` so every branch runs. The match scenario runs only the
// two routines the match engine calls.
#include "injury_unit_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t row_date = 0x1018c1a0, degree = 0x1018c270, kind = 0x1018c2f0, name_of_person = 0x1018c450,
                        name_text = 0x1018c560, language_flag = 0x1018d140, time_left = 0x1018d950, form_text = 0x1018dab0,
                        has_81 = 0x1018f340, recovery = 0x1018f470, match_result = 0x1018fbe0, record_date = 0x1018fdf0;
constexpr std::uint32_t person_allowed = 0x100cbde0, match_test = 0x104abf40, match_link = 0x1029eb20,
                        exit_game = 0x1076f7d0, history_write = 0x10748ce0, clock_reading = 0x10749fd0, clock_copy = 0x10749fc0, days_apart = 0x10717dd0;
constexpr std::uint32_t buffer_area = scratch_base + 0x8000, news_area = scratch_base + 0x9000, sink_area = scratch_base + 0xa000,
                        person_area = scratch_base + 0xb000;

struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

struct World {
    const Scenario& sc;
    std::uint32_t manager, rows, records, people;
    std::vector<std::uint32_t> injured, healthy;
    std::uint32_t row(std::uint32_t index) const { return rows + index * 0x24; }
    std::uint32_t record_of(std::uint32_t index) const { return records + sc.r32(row(index) + 0x10) * 0x38; }
};

World world(const Scenario& scenario) {
    World w{scenario, scenario.global(-0x7ba0), 0, 0, scenario.people(), {}, {}};
    w.rows = scenario.r32(w.manager);
    w.records = scenario.r32(w.manager + 4);
    for (std::uint32_t index = 0; index < w.people && (w.injured.size() < 14 || w.healthy.size() < 10); index += 5) {
        auto& list = scenario.r32(w.row(index) + 0x10) == 0xffffffff ? w.healthy : w.injured;
        if (list.size() < 14) list.push_back(index);
    }
    for (std::uint32_t index = 0; index < w.people && w.injured.size() < 14; ++index)
        if (scenario.r32(w.row(index) + 0x10) != 0xffffffff) w.injured.push_back(index);
    return w;
}

std::string text(const char* what, std::uint32_t value) { return std::string(what) + " " + std::to_string(value); }

void put(Case& item, unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
// The game's language is a byte of the table of contents area read by 0x10727880 (a leaf, so it cannot be answered).
constexpr std::uint32_t language_byte = 0x108bba90 - 0x35ac;
void say_language(Case& item, std::uint32_t value) { item.memory.push_back({1, language_byte, value & 0xff}); }
}

INJURY_UNIT_CASES("text status") {
    const auto w = world(scenario);
    // 0x1018c270 and 0x1018c2f0 also run on the match: the engine calls the first.
    std::vector<std::uint32_t> persons;
    for (const auto index : w.injured) persons.push_back(index);
    for (const auto index : w.healthy) persons.push_back(index);
    for (const std::uint32_t entry : {degree, kind}) {
        cases.push_back({.entry = entry, .registers = {{3, w.manager}, {4, 0}}, .label = "status of no person"});
        for (const auto index : persons)
            cases.push_back({.entry = entry, .registers = {{3, w.manager}, {4, scenario.person(index)}},
                             .label = text("status of person", index)});
        // A person record that is past the people count, one without a detail record, and rows with every
        // kind and degree byte the tests tell apart.
        for (const std::uint32_t index : {w.people, w.people + 1, 0x7fffffffu, 0x80000000u}) {
            Case item{.entry = entry, .registers = {{3, w.manager}, {4, person_area}}, .label = text("status past the table", index)};
            put(item, 4, person_area, index);
            put(item, 4, person_area + 0x14, 1);
            cases.push_back(item);
        }
        {
            Case item{.entry = entry, .registers = {{3, w.manager}, {4, scenario.person(persons[0])}}, .label = "status without detail record"};
            put(item, 4, scenario.person(persons[0]) + 0x14, 0);
            cases.push_back(item);
        }
        for (const std::uint32_t kind_byte : {0u, 1u, 0x7fu, 0x80u, 0xffu})
            for (const std::uint32_t degree_byte : {0u, 1u, 2u, 5u, 0x7fu, 0x80u, 0xffu}) {
                const auto index = persons[(kind_byte + degree_byte) % persons.size()];
                Case item{.entry = entry, .registers = {{3, w.manager}, {4, scenario.person(index)}},
                          .label = "status kind " + std::to_string(kind_byte) + " degree " + std::to_string(degree_byte)};
                put(item, 1, w.row(index) + 0x14, kind_byte);
                put(item, 1, w.row(index) + 0x15, degree_byte);
                cases.push_back(item);
            }
    }
}

INJURY_UNIT_CASES("text names") {
    if (scenario.match) return;
    const auto w = world(scenario);
    Rng rng{0x1ee5};
    // The names and descriptions of every type, in every language the routine tells apart.
    const std::uint32_t languages[] = {0, 1, 3, 5, 9};
    for (std::uint32_t style : {0u, 1u, 2u, 0x100u, 0x101u, 0xffu}) {
        for (std::uint32_t type = 0; type < 0x58; ++type) {
            Case item{.entry = name_text, .registers = {{3, w.manager}, {4, type}, {5, buffer_area}, {6, 40}, {7, style}},
                      .label = "name of type " + std::to_string(type) + " style " + std::to_string(style)};
            if (style == 1 || style == 2 || style == 0xff) say_language(item, languages[(type + style) % 5]);
            cases.push_back(item);
        }
    }
    for (const std::uint32_t type : {0x80u, 0xffu, 0x100u + 5u, 0xfffffff0u, 0x7fu, 0x51u, 0x52u}) {
        for (const std::uint32_t style : {0u, 1u}) {
            Case item{.entry = name_text, .registers = {{3, w.manager}, {4, type}, {5, buffer_area}, {6, 40}, {7, style}},
                      .label = "name of high type " + std::to_string(type) + " style " + std::to_string(style)};
            if (style == 1) say_language(item, rng.pick<std::uint32_t>({3, 0, 9}));
            cases.push_back(item);
        }
    }
    for (const std::int32_t size : {0, 29, 30, 31, 100, -1, std::numeric_limits<std::int32_t>::min()})
        for (const std::uint32_t style : {0u, 1u})
            for (const std::uint32_t type : {0u, 40u, 0x60u}) {
                Case item{.entry = name_text, .registers = {{3, w.manager}, {4, type}, {5, buffer_area}, {6, std::uint32_t(size)}, {7, style}},
                          .label = "name size " + std::to_string(size) + " type " + std::to_string(type)};
                cases.push_back(item);
            }

    // The injury name of a person: no person, past the table, healthy, injured (with every type written
    // into the record).
    cases.push_back({.entry = name_of_person, .registers = {{3, w.manager}, {4, 0}, {5, buffer_area}, {6, 40}, {7, 0}},
                     .label = "person name of no person"});
    for (const auto index : w.healthy)
        cases.push_back({.entry = name_of_person, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 40}, {7, 0}},
                         .label = text("injury name of healthy", index)});
    {
        Case item{.entry = name_of_person, .registers = {{3, w.manager}, {4, person_area}, {5, buffer_area}, {6, 40}, {7, 0}},
                  .label = "injury name past the table"};
        put(item, 4, person_area, w.people + 3);
        cases.push_back(item);
    }
    unsigned n = 0;
    for (std::uint32_t type = 0; type < 0x56; ++type) {
        const auto index = w.injured[type % w.injured.size()];
        const std::uint32_t style = type % 3 == 0 ? 0 : type % 3 == 1 ? 1 : 2;
        Case item{.entry = name_of_person, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 30 + type % 3}, {7, style}},
                  .label = "injury name of type " + std::to_string(type) + " style " + std::to_string(style)};
        put(item, 1, w.record_of(index), type);
        item.answers.push_back({history_write, 0});
        if (style != 0) say_language(item, languages[n++ % 5]);
        cases.push_back(item);
    }
    for (const auto index : w.injured) {
        cases.push_back({.entry = name_of_person, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 8}, {7, 0}},
                         .label = text("injury name into a small buffer of", index)});
        // The pending update some of these people have writes an injury history (0x10748ce0): answered.
        Case item{.entry = name_of_person, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 40}, {7, 1}},
                  .label = text("injury name, history answered, of", index)};
        item.answers.push_back({history_write, 0});
        cases.push_back(item);
    }
}

INJURY_UNIT_CASES("text language and forms") {
    if (scenario.match) return;
    const auto w = world(scenario);
    const std::uint32_t languages[] = {0, 1, 2, 3, 5, 9, 4, 0x103};
    for (const auto lang : languages)
        for (std::uint32_t type = 0; type < 0x56; ++type) {
            Case item{.entry = language_flag, .registers = {{3, w.manager}, {4, type}},
                      .label = "flag of type " + std::to_string(type) + " language " + std::to_string(lang)};
            say_language(item, lang);
            cases.push_back(item);
        }
    for (const std::uint32_t type : {0x80u, 0xffu, 0x100u, 0x14cu, 0x14du, 0xffffff00u, 0xffffffb3u})
        for (const auto lang : {9u, 3u, 5u, 0u}) {
            Case item{.entry = language_flag, .registers = {{3, w.manager}, {4, type}}, .label = "flag of high type " + std::to_string(type)};
            say_language(item, lang);
            cases.push_back(item);
        }
    for (const std::uint32_t form : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 0x80u, 0xffu, 0x100u, 0x10cu, 0x10du})
        for (const auto lang : {3u, 1u, 9u})
            for (const std::int32_t size : {20, 50, 19, 0, -5}) {
                Case item{.entry = form_text, .registers = {{3, w.manager}, {4, form}, {5, buffer_area}, {6, std::uint32_t(size)}},
                          .label = "form " + std::to_string(form) + " language " + std::to_string(lang) + " size " + std::to_string(size)};
                say_language(item, lang);
                cases.push_back(item);
            }
}

INJURY_UNIT_CASES("text time left and row dates") {
    if (scenario.match) return;
    const auto w = world(scenario);
    Rng rng{0x71ae};
    cases.push_back({.entry = time_left, .registers = {{3, w.manager}, {4, 0}, {5, buffer_area}, {6, 2}, {7, 1}, {8, 3}},
                     .label = "time left of no person"});
    {
        Case item{.entry = time_left, .registers = {{3, w.manager}, {4, person_area}, {5, buffer_area}, {6, 2}, {7, 1}, {8, 3}},
                  .label = "time left past the table"};
        put(item, 4, person_area, w.people);
        cases.push_back(item);
    }
    for (const auto index : w.healthy)
        cases.push_back({.entry = time_left, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 2}, {7, 1}, {8, 3}},
                         .label = text("time left of healthy", index)});
    for (const auto index : w.injured) {
        Case item{.entry = time_left, .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, 2}, {7, 1}, {8, 3}},
                  .label = text("time left, history answered, of", index)};
        item.answers.push_back({history_write, 0});
        cases.push_back(item);
    }
    for (const auto index : w.injured)
        for (const std::uint32_t which : {0u, 1u, 0x100u, 0x1ffu}) {
            Case item{.entry = time_left,
                      .registers = {{3, w.manager}, {4, scenario.person(index)}, {5, buffer_area}, {6, rng.below(4)}, {7, which}, {8, rng.below(3)}},
                      .label = "time left of " + std::to_string(index) + " which " + std::to_string(which)};
            if (rng.chance(50)) item.answers.push_back({days_apart, rng.next()});
            if (rng.chance(40)) {
                put(item, 2, w.record_of(index) + 0x30, rng.pick<std::uint32_t>({0, 1, 7, 40, 400, 0x7fff, 0x8000, 0xffff}));
                put(item, 2, w.record_of(index) + 0x32, rng.pick<std::uint32_t>({0, 3, 12, 90, 1000, 0x7fff, 0x8000, 0xfffe}));
            }
            cases.push_back(item);
        }

    // The two row-date routines.
    cases.push_back({.entry = row_date, .registers = {{3, w.manager}, {4, 0}}, .label = "row date of no person"});
    for (const auto index : w.injured) {
        cases.push_back({.entry = row_date, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("row date of", index)});
        Case item{.entry = row_date, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("row date answered of", index)};
        item.answers.push_back({clock_reading, rng.next()});
        cases.push_back(item);
    }
    {
        Case item{.entry = row_date, .registers = {{3, w.manager}, {4, scenario.person(w.healthy[0])}}, .label = "row date without a clock"};
        put(item, 4, w.manager + 0xc, 0);
        cases.push_back(item);
    }
    {
        Case item{.entry = record_date, .registers = {{3, w.manager}, {4, 0}}, .label = "record date of no person"};
        item.answers.push_back({exit_game, 0});
        cases.push_back(item);
    }
    for (const auto index : w.injured) {
        cases.push_back({.entry = record_date, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("record date of", index)});
        Case item{.entry = record_date, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("record date answered of", index)};
        item.answers.push_back({clock_copy, rng.next()});
        cases.push_back(item);
    }
    {
        Case item{.entry = record_date, .registers = {{3, w.manager}, {4, scenario.person(w.healthy[0])}}, .label = "record date without a clock"};
        put(item, 4, w.manager + 0xc, 0);
        cases.push_back(item);
    }

    // The injury-81 question: every person kind, and an injured one with type 81.
    cases.push_back({.entry = has_81, .registers = {{3, w.manager}, {4, 0}}, .label = "injury 81 of no person"});
    for (const auto index : w.healthy)
        cases.push_back({.entry = has_81, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("injury 81 of healthy", index)});
    for (const auto index : w.injured) {
        cases.push_back({.entry = has_81, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("injury 81 of", index)});
        Case item{.entry = has_81, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("injury 81 forced for", index)};
        put(item, 1, w.record_of(index), 0x51);
        item.answers.push_back({history_write, 0});
        cases.push_back(item);
        Case answered{.entry = has_81, .registers = {{3, w.manager}, {4, scenario.person(index)}}, .label = text("injury 81, history answered, of", index)};
        answered.answers.push_back({history_write, 0});
        cases.push_back(answered);
    }
    {
        Case item{.entry = has_81, .registers = {{3, w.manager}, {4, person_area}}, .label = "injury 81 past the table"};
        put(item, 4, person_area, w.people + 2);
        cases.push_back(item);
    }
}

INJURY_UNIT_CASES("text news") {
    if (scenario.match) return;
    const auto w = world(scenario);
    Rng rng{0x5eed};
    // A recovery news record: fields from +8. The sink is a zero object.
    const auto recovery_case = [&](unsigned n, std::uint32_t outcome, std::uint32_t type, std::uint32_t style, std::uint32_t detail,
                                   std::uint32_t result) {
        Case item{.entry = recovery, .registers = {{3, w.manager}, {4, news_area}, {5, style}, {6, sink_area}},
                  .label = "recovery news " + std::to_string(n) + " type " + std::to_string(type)};
        const std::uint32_t fields[] = {outcome, type, rng.below(w.people), rng.below(100), rng.below(100), rng.below(100),
                                        rng.below(0x10000), rng.below(0x10000), rng.pick<std::uint32_t>({0, 1, 5, 30, 400, 0x7fff, 0x8000, 0xffff}),
                                        detail, result};
        unsigned k = 0;
        for (const auto value : fields) put(item, 4, news_area + 8 + 4 * k++, value);
        put(item, 4, sink_area, 0);
        if (rng.chance(50)) item.answers.push_back({person_allowed, rng.below(2)});
        if (rng.chance(30)) say_language(item, rng.pick<std::uint32_t>({3, 9, 5, 1}));
        return item;
    };
    unsigned n = 0;
    for (std::uint32_t type = 0; type < 0x52; ++type)
        for (const std::uint32_t outcome : {0u, 1u})
            for (const std::uint32_t style : {0u, 1u})
                cases.push_back(recovery_case(n++, outcome, type, style, rng.below(3), rng.pick<std::uint32_t>({0, 1, 2, 3, 0xffffffffu})));
    for (std::uint32_t type = 0x4c; type <= 0x51; ++type)
        for (const std::uint32_t result : {0xffffffffu, 0u, 1u, 2u, 3u, 5u})
            for (const std::uint32_t detail : {0u, 1u, 2u, 5u})
                for (const std::uint32_t outcome : {0u, 1u})
                    cases.push_back(recovery_case(n++, outcome, type, 1, detail, result));
    // Type 0x51 with result 3 without the full text; and class 12 forced onto types the table gives another class
    // (the class byte is at +1 of the type's row), for both outcomes and several detail/result values.
    for (const std::uint32_t result : {3u, 2u})
        cases.push_back(recovery_case(n++, 1, 0x51, 0, 1, result));
    const auto type_table = scenario.toc(-0x64d4);
    for (const std::uint32_t type : {3u, 20u, 0x20u})
        for (const std::uint32_t outcome : {0u, 1u})
            for (const std::uint32_t detail : {0u, 2u}) {
                auto item = recovery_case(n++, outcome, type, 1, detail, rng.below(4));
                item.label += " forced class 12";
                put(item, 1, type_table + type * 12 + 1, 12);
                cases.push_back(item);
            }
    for (const std::uint32_t type : {0x80u, 0xffu, 0x1ffu, 0xfffffffeu})
        for (const std::uint32_t style : {0u, 1u, 2u, 0x100u})
            cases.push_back(recovery_case(n++, rng.below(2), type, style, rng.below(3), rng.below(4)));
    for (unsigned k = 0; k < 120; ++k)
        cases.push_back(recovery_case(n++, rng.pick<std::uint32_t>({0, 1, 2, 0x100, 0xff}), rng.below(0x56), rng.pick<std::uint32_t>({0, 1, 2, 0x100}),
                                      rng.below(4), rng.pick<std::uint32_t>({0, 1, 2, 3, 0xffffffffu, 9})));

    // A match news record: halfwords at +4/+6, the competition at +0xc, the two clubs at +0x18/+0x1c, the
    // scores (bytes) at +0x20/+0x21 and the link at +0x24.
    const auto club_total = scenario.global(-0x7b50);
    unsigned m = 0;
    for (const std::uint32_t first : {0u, 1u, 3u, 0x7fu, 0x80u, 0xffu})
        for (const std::uint32_t second : {0u, 1u, 3u, 0x7fu, 0x80u, 0xffu}) {
            Case item{.entry = match_result, .registers = {{3, w.manager}, {4, news_area}, {5, sink_area}},
                      .label = "match news " + std::to_string(m++) + " score " + std::to_string(first) + ":" + std::to_string(second)};
            put(item, 2, news_area + 4, rng.below(0x10000));
            put(item, 2, news_area + 6, rng.below(0x10000));
            put(item, 4, news_area + 0xc, rng.below(30));
            put(item, 4, news_area + 0x18, rng.below(club_total));
            put(item, 4, news_area + 0x1c, rng.below(club_total));
            put(item, 1, news_area + 0x20, first);
            put(item, 1, news_area + 0x21, second);
            put(item, 4, news_area + 0x24, rng.below(1000));
            put(item, 4, sink_area, 0);
            if (m % 3 != 0) item.answers.push_back({match_test, rng.below(2)});
            if (m % 4 != 0) item.answers.push_back({match_link, rng.below(2)});
            cases.push_back(item);
        }
}
}

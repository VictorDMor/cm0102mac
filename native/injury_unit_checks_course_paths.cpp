// INJURY.CPP checker cases for the course block, second part: the paths the first part's cases never took.
// The sample table is scripted (cursor cell pointed at scratch halfwords, base 0), so each draw of a run is
// a chosen value; the outside routines that decide a branch (the deal lookup, the staff checks, the game's
// search, the allocator) are answered where a real record does not reach it.
#include "injury_unit_checks.hpp"
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t daily_pass = 0x1018b2b0, queue_event = 0x1018bb50, run_events = 0x1018bdd0, injury_course = 0x1018dcb0;
constexpr std::uint32_t script_area = scratch_base + 0x6000, deal_area = scratch_base + 0x4000, queue_area = scratch_base + 0x1000,
                        due_area = scratch_base + 0x2000, date_area = scratch_base + 0x300;
constexpr std::uint32_t find_deal = 0x10082f20, staff_a_check = 0x100d9580, staff_a_act = 0x100d9e90, staff_b_check = 0x100d9520,
                        staff_b_act = 0x100d9e10, set_deal_flag = 0x1037f7d0, search = 0x10775600, allocate = 0x10775130;

struct Date { std::uint16_t day, year; };
void put_date(std::vector<Write>& memory, std::uint32_t at, Date date) {
    memory.push_back({2, at, date.day});
    memory.push_back({2, at + 2, date.year});
    memory.push_back({1, at + 4, 0});
}
// The draws of the run, in order: the k-th draw's sample is values[k] (base 0); more draws read 1.
void script(const Scenario& scenario, Case& c, std::vector<std::uint16_t> values) {
    values.resize(40, 1);
    c.memory.push_back({4, scenario.toc(-0x7b84), script_area});
    c.memory.push_back({4, scenario.toc(-0x7b88), script_area + 0x100});
    c.memory.push_back({4, scenario.toc(-0x7b90), 0});
    for (std::size_t k = 0; k < values.size(); ++k) c.memory.push_back({2, script_area + 2 * std::uint32_t(k + 1), values[k]});
}
}

INJURY_UNIT_CASES("course paths progress") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    // A person with a club, a profile and a club staff; and a club without staff.
    std::uint32_t person = 0, plain_club = 0;
    for (std::uint32_t i = 0; i < scenario.people() && !(person && plain_club); ++i) {
        const auto p = scenario.person(i), club = scenario.r32(p + 0x24);
        if (!club || !scenario.r32(p + 0x14)) continue;
        if (scenario.r32(club + 0x90)) { if (!person) person = p; } else if (!plain_club) plain_club = p;
    }
    if (!person) return;
    const auto club = scenario.r32(person + 0x24), profile = scenario.r32(person + 0x14);
    const auto world = scenario.toc(-0x7b24);
    int serial = 0;
    struct Opt {
        std::uint8_t a = 3, b = 4, c = 0, d = 9;       // person bytes 0x63, 0x65, 0x67, 0x69
        std::uint32_t flags = 0;                       // the deal's flags
        std::uint8_t rating = 0;
        bool deal = true, check = true, no_staff = false, no_club = false, no_profile = false, no_world = false, old = false;
        std::uint8_t form = 5;
        std::uint32_t queued = 1;
        std::uint32_t home = 1;  // 0: person +0xc zeroed
    };
    const auto add = [&](const char* what, std::uint32_t type, std::uint32_t stage, std::vector<std::uint16_t> samples, Opt o) {
        Case c{.entry = injury_course, .registers = {{3, manager}, {4, person}, {5, stage}, {6, type}, {7, o.queued}, {8, 5}},
               .label = std::string("course paths progress ") + what + " #" + std::to_string(serial++)};
        script(scenario, c, std::move(samples));
        c.memory.push_back({1, person + 0x63, o.a});
        c.memory.push_back({1, person + 0x65, o.b});
        c.memory.push_back({1, person + 0x67, o.c});
        c.memory.push_back({1, person + 0x69, o.d});
        c.memory.push_back({1, profile + 0x44, o.form});
        c.memory.push_back({2, profile + 0xa, o.old ? 8000u : 100u});
        if (o.no_club) c.memory.push_back({4, person + 0x24, 0});
        if (o.no_profile) c.memory.push_back({4, person + 0x14, 0});
        if (o.no_staff) c.memory.push_back({4, club + 0x90, 0});
        if (o.no_world) c.memory.push_back({4, world, 0});
        if (!o.home) c.memory.push_back({4, person + 0xc, 0});
        c.memory.push_back({4, deal_area + 0x50, o.flags});
        c.memory.push_back({1, deal_area + 0x4c, o.rating});
        c.answers = {{find_deal, o.deal ? deal_area : 0}, {staff_a_check, o.check}, {staff_a_act, 0}, {staff_b_check, o.check},
                     {staff_b_act, 0}, {set_deal_flag, 0}};
        cases.push_back(std::move(c));
    };
    // The deal path (type 0x51): lookup failures, flags, rating clamps, the staff, the world, the stat moves.
    add("deal missing", 0x51, 0, {0}, {.deal = false});
    add("profile missing", 0x51, 0, {0}, {.no_profile = true});
    add("club missing", 0x51, 0, {0}, {.no_club = true});
    add("club missing home missing", 0x51, 0, {0}, {.no_club = true, .home = 0});
    for (const std::uint32_t flags : {0x20000u, 0x20u, 0x40u, 0x20060u, 0x20u | 0x40u})
        for (const std::uint8_t cc : {std::uint8_t(9), std::uint8_t(0xfb)})
            for (const bool set : {true, false})
                for (const std::uint8_t rating : {std::uint8_t(0), std::uint8_t(100), std::uint8_t(0x88)})
                    for (const bool check : {true, false})
                        add("flags", 0x51, 0, {0, 4, std::uint16_t(set ? 0 : 24), 2, std::uint16_t(set ? 1 : 24), 8, std::uint16_t(set ? 3 : 25), 7, 3, 19, 1},
                            {.b = 18, .c = cc, .flags = flags, .rating = rating, .check = check});
    for (const std::uint16_t reportdraw : {std::uint16_t(7), std::uint16_t(2)})
        add("flagged report", 0x51, 0, {0, 4, 0, reportdraw, 3, 19, 1}, {.b = 18, .c = 9, .flags = 0x20000});
    add("flagged no staff", 0x51, 0, {0, 4, 0, 7, 3, 19, 1}, {.b = 18, .c = 9, .flags = 0x20000, .no_staff = true});
    add("flagged no club", 0x51, 0, {0, 4, 0, 7, 3, 19, 1}, {.b = 18, .c = 9, .flags = 0x20000, .no_club = true});
    // No flag: the rating may still move (a draw against attribute C), then the world or the stat moves.
    for (const std::uint16_t moves : {std::uint16_t(0), std::uint16_t(1)})
        for (const std::uint8_t rating : {std::uint8_t(0), std::uint8_t(100), std::uint8_t(0x88)})
            for (const std::uint16_t world_draw : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(4)})
                for (const bool no_world : {false, true})
                    for (const std::uint8_t b : {std::uint8_t(4), std::uint8_t(12), std::uint8_t(0xf0)})
                        add("no flag", 0x51, 0, {0, moves, world_draw, 3, 3, 19, 1}, {.b = b, .c = 9, .rating = rating, .no_world = no_world});
    // The stat moves after the deal: age, which of the four arms, attributes at their limits.
    for (const std::uint16_t which : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(2), std::uint16_t(3)})
        for (const bool old : {false, true})
            for (const std::uint8_t a : {std::uint8_t(3), std::uint8_t(20)})
                for (const std::uint8_t d : {std::uint8_t(1), std::uint8_t(9)})
                    for (const std::uint8_t form : {std::uint8_t(1), std::uint8_t(5)})
                        add("stat arm", 0x51, 0, {0, 1, 1, 19, which}, {.a = a, .b = std::uint8_t(a == 20 ? 1 : 4), .c = 9, .d = d, .no_world = true, .old = old, .form = form});
    add("stat no draw", 0x51, 0, {0, 1, 1, 1, 19, 1}, {.b = 0, .c = 0, .no_world = true});
    add("stat bad roll", 0x51, 0, {0, 1, 1, 1, 0, 1}, {.b = 4, .c = 9, .no_world = true});
    // The other types: the roll decides the outcome.
    for (const std::uint32_t stage : {0u, 1u}) {
        for (const std::uint16_t roll : {std::uint16_t(0), std::uint16_t(3), std::uint16_t(4), std::uint16_t(5), std::uint16_t(29499), std::uint16_t(29500),
                                         std::uint16_t(29999), std::uint16_t(30000), std::uint16_t(31999)})
            for (const std::uint32_t type : {0x4du, 0x4eu, 0x4fu})
                for (const std::uint16_t second : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(5), std::uint16_t(7)})
                    for (const int shape : {0, 1, 2, 3, 4, 5}) {
                        Opt o;
                        if (shape == 1) { o.b = 18; o.a = 12; }
                        if (shape == 2) { o.b = 11; o.a = 12; }
                        if (shape == 3) { o.b = 18; o.a = 12; o.no_staff = true; }
                        if (shape == 4) { o.b = 18; o.a = 12; o.no_club = true; }
                        if (shape == 5) { o.b = 18; o.a = 5; }
                        add("roll", type, stage, {roll, second, 7, 7, 0, 0}, o);
                    }
    }
    // Notices with the staff: acting, reporting or not.
    for (const bool check : {true, false})
        for (const std::uint16_t report : {std::uint16_t(0), std::uint16_t(7)}) {
            add("notice good", 0x4d, 0, {0, report, 5}, {.b = 18, .check = check});
            add("notice bad", 0x4d, 0, {20, report, 5}, {.d = 2, .check = check});
        }
    add("notice bad no staff", 0x4d, 0, {20, 0, 5}, {.d = 2, .no_staff = true});
    add("class-less type", 0x10, 0, {0}, {});
    add("no club, queued 0", 0x4d, 0, {0}, {.no_club = true, .queued = 0});
}

INJURY_UNIT_CASES("course paths queues") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto today = scenario.toc(-0x7b80);
    const std::uint16_t day = std::uint16_t(scenario.r32(today) >> 16), year = std::uint16_t(scenario.r32(today));
    // queue_club_event: the allocation fails (no buffer, or the buffer grown), the event is dropped.
    for (const std::uint32_t kind : {1u, 2u, 3u})
        for (const std::uint32_t club : {0u, 3u})
            for (const int shape : {0}) {  // (a realloc is a same-page call of the original: it cannot be answered)
                Case c{.entry = queue_event, .registers = {{3, manager}, {4, date_area}, {5, kind}, {6, club}, {7, 0}},
                       .label = "course paths queue alloc fails kind " + std::to_string(kind) + " club " + std::to_string(club) + " shape " + std::to_string(shape)};
                script(scenario, c, {kind == 3 ? std::uint16_t(15) : std::uint16_t(1)});
                put_date(c.memory, date_area, {day, year});
                if (shape == 0) { c.memory.push_back({4, manager + 0x1c, 0}); c.memory.push_back({4, manager + 0x20, 0}); c.memory.push_back({4, manager + 0x24, 0}); }
                else c.memory.push_back({4, manager + 0x24, scenario.r32(manager + 0x20)});
                c.answers = {{allocate, 0}};
                cases.push_back(std::move(c));
            }
    // run_events with today moved near the start of a year: kind-3 rows of the previous year (not today).
    for (const std::uint16_t today_day : {std::uint16_t(1), std::uint16_t(3)})
        for (const std::uint16_t row_day : {std::uint16_t(360), std::uint16_t(363), std::uint16_t(364), std::uint16_t(365), std::uint16_t(1)})
            for (const std::uint8_t kind : {std::uint8_t(3), std::uint8_t(1)}) {
                Case c{.entry = run_events, .registers = {{3, manager}},
                       .label = "course paths events today " + std::to_string(today_day) + " row " + std::to_string(row_day) + " kind " + std::to_string(kind)};
                script(scenario, c, {1, 1, 3, 5, 9, 12, 1, 2, 3});
                put_date(c.memory, today, {today_day, year});
                c.memory.push_back({4, manager + 0x1c, queue_area});
                c.memory.push_back({4, manager + 0x24, 2});
                put_date(c.memory, queue_area, {row_day, std::uint16_t(row_day > 300 ? year - 1 : year)});
                c.memory.push_back({1, queue_area + 8, kind});
                c.memory.push_back({4, queue_area + 0xc, 1});
                c.memory.push_back({4, queue_area + 0x10, 0});
                put_date(c.memory, queue_area + 0x14, {today_day, year});
                c.memory.push_back({1, queue_area + 0x14 + 8, 1});
                c.memory.push_back({4, queue_area + 0x14 + 0xc, 2});
                c.memory.push_back({4, queue_area + 0x14 + 0x10, 0});
                cases.push_back(std::move(c));
            }
}

INJURY_UNIT_CASES("course paths daily") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto today = scenario.toc(-0x7b80);
    const std::uint16_t day = std::uint16_t(scenario.r32(today) >> 16), year = std::uint16_t(scenario.r32(today));
    const auto statuses = scenario.r32(manager), injuries = scenario.r32(manager + 4);
    const auto count = scenario.people();
    // Injury records in the visited squads that end today, with the given length.
    const auto ending = [&](std::uint32_t length, std::uint8_t flag) {
        std::vector<Write> w;
        unsigned made = 0;
        for (std::uint32_t group = 0; group < 16 && made < 3; ++group) {
            const auto club = scenario.r32(scenario.person(count - group - 1) + 0x24);
            if (!club) continue;
            for (std::uint32_t k = 0; k < 50 && made < 3; ++k) {
                const auto person = scenario.r32(club + 0x98 + 4 * k);
                if (!person) continue;
                const auto index = scenario.r32(statuses + scenario.r32(person) * 0x24 + 0x10);
                if (index == 0xffffffff) continue;
                const auto record = injuries + index * 0x38;
                put_date(w, record + 0x28, {std::uint16_t(day >= 11 ? day - length : day), year});
                w.push_back({2, record + 0x32, day >= 11 ? length : 0u});
                w.push_back({2, record + 0x30, 14});
                w.push_back({1, statuses + scenario.r32(person) * 0x24 + 0x20, flag});
                w.push_back({1, record, 0x4du});
                ++made;
            }
        }
        return w;
    };
    // Short injuries (not past five days) and ones whose course was already run.
    for (const std::uint32_t length : {3u, 5u, 6u})
        for (const std::uint8_t flag : {std::uint8_t(0), std::uint8_t(1)}) {
            Case c{.entry = daily_pass, .registers = {{3, manager}},
                   .label = "course paths daily length " + std::to_string(length) + " flag " + std::to_string(flag)};
            c.memory = ending(length, flag);
            cases.push_back(std::move(c));
        }
    // Due queues, with the game's search answered with the first row (so a row not equal to today is "found").
    struct Due { std::int32_t first, second; };
    const std::vector<std::vector<Due>> queues = {
        {{0, 0}, {0, 0}, {0, 0}}, {{0, 0}}, {{-5, -5}, {-4, -4}}, {{-5, -5}, {-1, -1}}, {{2, 2}, {3, 3}}, {{0, 2}}, {{-2, 0}, {0, 0}, {0, 0}}};
    for (std::size_t q = 0; q < queues.size(); ++q)
        for (const bool answered : {false}) {  // (the game's search is a same-page call of the original: it cannot be answered)
            Case c{.entry = daily_pass, .registers = {{3, manager}},
                   .label = "course paths daily due queue " + std::to_string(q) + (answered ? " answered" : " searched")};
            script(scenario, c, {0, 1, 2, 3, 4, 5, 6, 7, 8});
            c.memory.push_back({4, manager + 0x10, due_area});
            c.memory.push_back({4, manager + 0x18, std::uint32_t(queues[q].size())});
            std::uint32_t at = due_area;
            for (std::size_t i = 0; i < queues[q].size(); ++i) {
                put_date(c.memory, at, {std::uint16_t(day + queues[q][i].first), year});
                c.memory.push_back({4, at + 4, 0});
                put_date(c.memory, at + 8, {std::uint16_t(day + queues[q][i].second), year});
                c.memory.push_back({4, at + 0x10, std::uint32_t(i + 1)});
                c.memory.push_back({1, at + 0x14, 0});
                c.memory.push_back({1, at + 0x15, 0x4du});
                at += 0x18;
            }
            if (answered) c.answers = {{search, due_area}};
            cases.push_back(std::move(c));
        }
}
}

// HUMAN_MANAGER.CPP checker cases for the schedule block (human_manager_schedule.hpp): the options a
// human sets, the tactic slot for a club, the booking rows and the staff a club answers to.
//
// The table, the current human and the people are the career's own. Rows a case builds (a booking row,
// a fixture list with its fixtures) live in the scratch area; the clubs, humans and staff in them are real.
#include "human_manager_checks.hpp"
#include <array>
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t record_size = 0xa65c, booking = 0x9a68;
constexpr std::uint32_t choose_tactic = 0x10146830, set_away = 0x10147fd0, set_runs_club = 0x10148340,
                        set_runs_nation = 0x10148660, set_holiday = 0x101492d0, booking_matches = 0x101494b0,
                        any_booking_matches = 0x10149690, set_booking = 0x10149790, booking_is_free = 0x101499f0,
                        clear_taken = 0x10149bd0, club_head_staff = 0x1014c5f0, drop_unfit = 0x1014c680;
constexpr std::uint32_t match_day = 0x101cc810;  // answered
constexpr std::uint32_t fixture_list = 0x1048d420;  // FIX_MAN.CPP: answered with a list a case builds
// Fields of a booking row (0x50 bytes).
struct Field { unsigned size; std::uint32_t offset; };
constexpr Field booking_fields[] = {{4, 0x00}, {4, 0x0c}, {4, 0x10}, {2, 0x28}, {2, 0x2a}, {2, 0x34}, {1, 0x3f}};
constexpr std::uint32_t row_a = scratch_base + 0x1000, list_a = scratch_base + 0x2000, node_a = scratch_base + 0x2100,
                        node_b = scratch_base + 0x2110, fixture_a = scratch_base + 0x3000, fixture_b = scratch_base + 0x3100;
}

HUMAN_MANAGER_CASES("schedule") {
    if (scenario.match) return;
    const auto first_case = cases.size();
    const auto humans = scenario.global(-0x7a54);
    const auto records = scenario.r32(humans), count = scenario.r32(humans + 4);
    const auto staff_total = scenario.people();
    const auto game = scenario.toc(-0x7bb0);
    const auto current = std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(game + 0x2c58))));
    const auto current_staff_cell = game + current * 0x2c0 + 4;
    const auto current_staff = scenario.r32(current_staff_cell);
    const auto human_record = [&](std::uint32_t staff) {
        return records + (scenario.r32(staff) - staff_total + 16) * record_size;
    };
    const auto in_table = [&](std::uint32_t staff) { return scenario.r32(staff) - staff_total + 16 < count; };
    const auto notify_cell = scenario.toc(-0x7420), skipped_cell = scenario.toc(-0x73ec), season_cell = scenario.toc(-0x79ec),
               today_cell = scenario.toc(-0x7b80), lists_cell = scenario.toc(-0x73f0);
    std::vector<std::pair<std::uint32_t, std::string>> staff{{0, "current"}, {current_staff, "current given"}};
    for (std::uint32_t slot = 0; slot < 16 && staff_total >= 16 - slot; ++slot)
        staff.emplace_back(scenario.person(staff_total - 16 + slot), "slot " + std::to_string(slot));
    if (staff_total > 17) staff.emplace_back(scenario.person(staff_total - 17), "below the table");
    staff.emplace_back(scenario.person(0), "ordinary staff");
    const std::vector<std::pair<std::vector<Write>, std::string>> settings{
        {{}, ""},
        {{{4, current_staff_cell, 0}}, ", no current human"},
        {{{4, humans + 4, 0}}, ", empty table"},
        {{{4, humans + 4, 1}}, ", table of one"},
        {{{4, humans + 4, 0x80000000u}}, ", table count INT_MIN"},
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) {
        a.insert(a.end(), b.begin(), b.end());
        return a;
    };
    const std::vector<std::pair<std::vector<Write>, std::string>> notify{
        {{{4, notify_cell, 0}}, ", match day off"},
        {{{4, notify_cell, 1}}, ", match day on"},
    };

    // The clubs: a sample, the national sides at the end, and the ones humans manage.
    const auto clubs = scenario.global(-0x7b50), range_low = scenario.global(-0x7b04);
    std::vector<std::uint32_t> sample;
    for (std::uint32_t index = 0; index < clubs; index += clubs / 6 + 1) sample.push_back(scenario.club(index));
    if (clubs) sample.push_back(scenario.club(clubs - 1));
    std::vector<std::uint32_t> managed, unmanaged;
    for (std::uint32_t index = 0; index < clubs; ++index) {
        const auto club = scenario.club(index);
        const auto manager = scenario.r32(club + 0x90);
        if (manager && std::int32_t(scenario.r32(manager)) >= std::int32_t(staff_total - 16)) managed.push_back(club);
        else if (unmanaged.size() < 4 && index + 4 < clubs) unmanaged.push_back(club);
    }

    // ---- 0x10146830: the tactic slot for a club. A club's first word against the ranges decides each
    // of the two tests: 0 answers no to both, the word at the doubled range's edge answers the doubled
    // test only, a large word answers both.
    const auto range_high = clubs;
    const std::uint32_t words[] = {0u, range_high - 2 * range_low, 0x7fffffffu, 0x80000000u, 0xffffffffu};
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff)
            for (const std::uint32_t flag : {0u, 1u})
                for (const auto club : {0u, sample.empty() ? 0u : sample[0]})
                    cases.push_back({.entry = choose_tactic, .registers = {{3, humans}, {4, flag}, {5, club}, {6, staff_record}},
                                     .memory = setting_memory, .label = "tactic, " + staff_label + setting_label});
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
        if (!in_table(staff_record)) continue;
        for (const auto club : sample)
            for (const std::uint32_t word : words)
                for (const std::uint32_t flag : {0u, 1u, 0x100u, 0x80u, 0xffu}) {
                    cases.push_back({.entry = choose_tactic, .registers = {{3, humans}, {4, flag}, {5, club}, {6, staff_record}},
                                     .memory = {{4, club, word}}, .label = "tactic, club word " + std::to_string(word)});
                    // The club taken from the staff's +0x24.
                    cases.push_back({.entry = choose_tactic, .registers = {{3, humans}, {4, flag}, {5, 0}, {6, staff_record}},
                                     .memory = {{4, club, word}, {4, staff_record + 0x24, club}},
                                     .label = "tactic, club from the staff " + std::to_string(word)});
                }
        cases.push_back({.entry = choose_tactic, .registers = {{3, humans}, {4, 1}, {5, 0}, {6, staff_record}},
                         .memory = {{4, staff_record + 0x24, 0}}, .label = "tactic, staff without a club"});
    }

    // ---- The option setters (0x10147fd0, 0x10148340, 0x10148660, 0x101492d0): every staff, every table
    // state, and the values around the signed byte tests, with the match day off and on.
    for (const auto entry : {set_away, set_runs_club, set_runs_nation, set_holiday}) {
        for (const auto& [setting_memory, setting_label] : settings)
            for (const auto& [staff_record, staff_label] : staff)
                for (const std::uint32_t value : {0u, 1u, 0x100u, 0xffu})
                    cases.push_back({.entry = entry, .registers = {{3, humans}, {4, value}, {5, staff_record}},
                                     .memory = join(setting_memory, {{4, notify_cell, 0}}),
                                     .label = "option " + staff_label + setting_label});
        for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
            if (!in_table(staff_record)) continue;
            const auto human = human_record(staff_record);
            const std::uint32_t offset = entry == set_away ? 0x9984 : entry == set_runs_club ? 0x9985
                                       : entry == set_runs_nation ? 0x9986 : 0x9a65;
            for (const auto& [notify_memory, notify_label] : notify)
                for (const std::uint32_t old : {0u, 1u, 0x80u, 0xffu})
                    for (const std::uint32_t value : {0u, 1u, 0x80u, 0xffu, 0x100u}) {
                        cases.push_back({.entry = entry, .registers = {{3, humans}, {4, value}, {5, staff_record}},
                                         .memory = join(notify_memory, {{1, human + offset, old}}),
                                         .label = "option value " + std::to_string(value) + " over " + std::to_string(old) + notify_label});
                        cases.push_back({.entry = entry, .registers = {{3, humans}, {4, value}, {5, 0}},
                                         .memory = join(join(notify_memory, {{1, human + offset, old}}), {{4, current_staff_cell, staff_record}}),
                                         .label = "option value, as current " + std::to_string(value) + notify_label});
                    }
            if (entry == set_away) {
                // The news: each of the staff's two club words set or not; a human away or not.
                for (const std::uint32_t first : {0u, managed.empty() ? 0u : managed[0]})
                    for (const std::uint32_t second : {0u, sample.empty() ? 0u : sample[0]})
                        for (const std::uint32_t value : {0u, 1u})
                            cases.push_back({.entry = entry, .registers = {{3, humans}, {4, value}, {5, staff_record}},
                                             .memory = {{4, staff_record + 0x24, first}, {4, staff_record + 0x20, second},
                                                        {4, notify_cell, 1}},
                                             .label = "away news"});
            }
        }
    }

    // ---- Booking rows. The human's own row is at record +0x9a68; a case's row is built in the scratch
    // area from the fields of the record's, with one field changed at a time.
    const auto own_row = [&](std::uint32_t staff_record) { return human_record(staff_record) + booking; };
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff) {
            cases.push_back({.entry = booking_matches, .registers = {{3, humans}, {4, 0}, {5, staff_record}},
                             .memory = setting_memory, .label = "matches, no row, " + staff_label + setting_label});
            cases.push_back({.entry = booking_matches, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                             .memory = setting_memory, .label = "matches, " + staff_label + setting_label});
            cases.push_back({.entry = any_booking_matches, .registers = {{3, humans}, {4, row_a}},
                             .memory = setting_memory, .label = "any matches" + setting_label});
        }
    cases.push_back({.entry = any_booking_matches, .registers = {{3, humans}, {4, 0}}, .label = "any matches, no row"});
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1), scenario.person(staff_total - 16)}) {
        if (!in_table(staff_record)) continue;
        const auto own = own_row(staff_record);
        for (const std::uint32_t value : {0x5au, 0x12345678u}) {
            // The booking set to a value, the row the same, and each field of the row changed in turn.
            std::vector<Write> own_values;
            for (const auto [size, offset] : booking_fields)
                own_values.push_back({size, own + offset, value & (size == 4 ? 0xffffffffu : size == 2 ? 0xffffu : 0xffu)});
            for (int changed = -1; changed < int(std::size(booking_fields)); ++changed) {
                std::vector<Write> row_values;
                for (const auto& write : own_values) row_values.push_back({write.size, row_a + (write.address - own), write.value});
                auto writes = join(own_values, row_values);
                if (changed >= 0) writes.push_back({booking_fields[changed].size, row_a + booking_fields[changed].offset, 0x77});
                cases.push_back({.entry = booking_matches, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                                 .memory = writes, .label = "matches, field " + std::to_string(changed)});
                cases.push_back({.entry = booking_matches, .registers = {{3, humans}, {4, row_a}, {5, 0}},
                                 .memory = join(writes, {{4, current_staff_cell, staff_record}}), .label = "matches as current, field " + std::to_string(changed)});
                // The scan of the 16 humans: the person flagged or not, the row equal to this human's.
                for (const std::uint32_t flagged : {0u, 1u, 0x80u, 0x100u}) {
                    const auto person = staff_record;
                    auto scan = join(writes, {{1, person + 0x6b, flagged}});
                    cases.push_back({.entry = any_booking_matches, .registers = {{3, humans}, {4, row_a}}, .memory = scan,
                                     .label = "any matches, field " + std::to_string(changed) + ", person flag " + std::to_string(flagged)});
                }
            }
        }
        cases.push_back({.entry = booking_matches, .registers = {{3, humans}, {4, own}, {5, staff_record}}, .label = "matches, its own row"});
        cases.push_back({.entry = any_booking_matches, .registers = {{3, humans}, {4, own}}, .label = "any matches, an own row"});
    }

    // A row that is free: the kind byte negative, no club of the row run by the human. Real clubs in
    // the row; the human's own club (a managed one) with and without the flag byte that assumes it.
    const std::uint32_t today_day = std::uint32_t(scenario.s.memory.read<2>(today_cell)),
                        today_year = std::uint32_t(scenario.s.memory.read<2>(today_cell + 2));
    const std::uint32_t season_byte = std::uint32_t(scenario.s.memory.read<1>(season_cell));
    const std::uint32_t free_club = unmanaged.empty() ? (sample.empty() ? 0 : sample[0]) : unmanaged[0];
    const std::uint32_t human_club = managed.empty() ? free_club : managed[0];
    const std::uint32_t human_club_manager = scenario.r32(human_club + 0x90);
    const auto build_row = [&](std::uint32_t kind, std::uint32_t first_club, std::uint32_t first_flag, std::uint32_t second_club,
                               std::uint32_t second_flag, std::uint32_t day, std::uint32_t year, std::uint32_t season) {
        return std::vector<Write>{{1, row_a + 0x43, kind}, {4, row_a + 0x1c, first_club}, {1, row_a + 0x40, first_flag},
                                  {4, row_a + 0x20, second_club}, {1, row_a + 0x41, second_flag}, {2, row_a + 0x2a, day},
                                  {2, row_a + 0x28, year}, {1, row_a + 0x3f, season}};
    };
    // Two fixtures a fabricated list returns (FIX_MAN answered, the list walk real).
    const auto fixtures = [&](std::uint32_t club_a, std::uint32_t flag_a, std::uint32_t club_b, std::uint32_t flag_b, std::uint32_t second) {
        return std::vector<Write>{{2, list_a, 2}, {4, list_a + 4, node_a}, {4, list_a + 0xc, 0}, {4, node_a, fixture_a},
                                  {4, node_a + 8, node_b}, {4, node_b, fixture_b}, {4, node_b + 8, 0},
                                  {4, fixture_a + 0x1c, club_a}, {1, fixture_a + 0x40, flag_a},
                                  {4, fixture_a + 0x20, second}, {1, fixture_a + 0x41, 0},
                                  {4, fixture_b + 0x1c, club_b}, {1, fixture_b + 0x40, flag_b},
                                  {4, fixture_b + 0x20, second}, {1, fixture_b + 0x41, 0}};
    };
    struct Variant { std::vector<Write> memory; std::string label; };
    std::vector<Variant> variants;
    for (const std::uint32_t kind : {0x80u, 0xffu, 0u, 0x7fu})
        for (const auto [club_a, flag_a, club_b, flag_b] :
             {std::array<std::uint32_t, 4>{free_club, 0, free_club, 0}, {human_club, 1, free_club, 0}, {free_club, 0, human_club, 1},
              {human_club, 0, free_club, 0}, {free_club, 0, human_club, 0}, {0, 0, free_club, 0}, {free_club, 0, 0, 0}})
            variants.push_back({build_row(kind, club_a, flag_a, club_b, flag_b, today_day, today_year, 3),
                                "row kind " + std::to_string(kind) + " clubs " + (club_a == human_club ? "H" : club_a ? "f" : "0") +
                                (club_b == human_club ? "H" : club_b ? "f" : "0") + std::to_string(flag_a) + std::to_string(flag_b)});
    for (const auto staff_record : {0u, current_staff}) {
        for (const auto& [setting_memory, setting_label] : settings)
            cases.push_back({.entry = booking_is_free, .registers = {{3, humans}, {4, 0}, {5, staff_record}}, .memory = setting_memory,
                             .label = "free, no row" + setting_label});
        for (const auto& variant : variants) {
            // The human of the managed club, as the staff asked about, with his option bytes in each state.
            for (const std::uint32_t option : {0u, 1u}) {
                std::vector<Write> memory = variant.memory;
                if (human_club_manager && in_table(human_club_manager)) {
                    memory.push_back({1, human_record(human_club_manager) + 0x9985, option});
                    memory.push_back({4, current_staff_cell, human_club_manager});
                }
                for (const std::uint32_t list_count : {0u, 1u, 2u, 0x80u}) {
                    auto with_count = join(memory, {{1, lists_cell, list_count}});
                    // The real fixture lists of the day.
                    cases.push_back({.entry = booking_is_free, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                                     .memory = with_count, .label = variant.label + ", lists " + std::to_string(list_count)});
                    // A list the case builds, with each fixture's clubs.
                    for (const auto [club_a, flag_a, club_b, flag_b, second] :
                         {std::array<std::uint32_t, 5>{free_club, 0, free_club, 0, free_club}, {human_club, 1, free_club, 0, free_club},
                          {free_club, 0, human_club, 1, free_club}, {free_club, 0, free_club, 0, human_club},
                          {0, 0, free_club, 0, free_club}})
                        cases.push_back({.entry = booking_is_free, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                                         .memory = join(with_count, fixtures(club_a, flag_a, club_b, flag_b, second)),
                                         .label = variant.label + ", built list " + std::to_string(list_count),
                                         .answers = {{fixture_list, list_a}}});
                }
            }
        }
    }
    // The empty list a built answer can give.
    cases.push_back({.entry = booking_is_free, .registers = {{3, humans}, {4, row_a}, {5, 0}},
                     .memory = join(variants[0].memory, {{1, lists_cell, 1}, {2, list_a, 0}, {4, list_a + 4, 0}, {4, list_a + 0xc, 0}}),
                     .label = "free, empty list", .answers = {{fixture_list, list_a}}});
    cases.push_back({.entry = booking_is_free, .registers = {{3, humans}, {4, row_a}, {5, 0}},
                     .memory = join(variants[0].memory, {{1, lists_cell, 1}}),
                     .label = "free, no list", .answers = {{fixture_list, 0}}});

    // ---- 0x10149790: set a human's row. The row free or not, the date equal to today or not, the game
    // listening or not; row 0 clears the record's row.
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff)
            for (const auto row : {0u, row_a})
                cases.push_back({.entry = set_booking, .registers = {{3, humans}, {4, row}, {5, staff_record}},
                                 .memory = setting_memory, .label = "set booking, " + staff_label + setting_label});
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
        if (!in_table(staff_record)) continue;
        const auto own = own_row(staff_record);
        for (const std::uint32_t day : {today_day, (today_day + 1) & 0xffffu, 0xffffu})
            for (const std::uint32_t year : {today_year, (today_year + 1) & 0xffffu})
                for (const std::uint32_t season : {3u, 4u})
                    for (const std::uint32_t listening : {0u, 1u})
                        for (const std::uint32_t skipped : {0u, 1u, 0x80u}) {
                            std::vector<Write> state{{1, season_cell, 3},
                                                     {4, notify_cell, listening}, {1, skipped_cell, skipped}};
                            // Row 0: the record's own row equal to today or not.
                            cases.push_back({.entry = set_booking, .registers = {{3, humans}, {4, 0}, {5, staff_record}},
                                             .memory = join(state, {{2, own + 0x2a, day}, {2, own + 0x28, year}, {1, own + 0x3f, season}}),
                                             .label = "set booking 0, own row"});
                            // A row the human is free for, with the date and season.
                            cases.push_back({.entry = set_booking, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                                             .memory = join(state, build_row(0x80, free_club, 0, free_club, 0, day, year, season)),
                                             .label = "set booking, row free"});
                            cases.push_back({.entry = set_booking, .registers = {{3, humans}, {4, row_a}, {5, staff_record}},
                                             .memory = join(state, build_row(0, free_club, 0, free_club, 0, day, year, season)),
                                             .label = "set booking, row not free"});
                        }
    }
    {
        // Today's day and year with the signed halfword: a negative day never equals an unsigned one.
        const auto staff_record = current_staff;
        if (in_table(staff_record)) {
            const auto own = own_row(staff_record);
            for (const std::uint32_t day : {0xffffu, 0x8000u, 0x7fffu})
                for (const std::uint32_t row : {0u, row_a}) {
                    std::vector<Write> memory{{2, today_cell, day}, {4, notify_cell, 1}, {1, skipped_cell, 0},
                                              {2, own + 0x2a, day}, {2, own + 0x28, today_year}, {1, own + 0x3f, season_byte},
                                              {2, row_a + 0x2a, day}, {2, row_a + 0x28, today_year}, {1, row_a + 0x3f, season_byte},
                                              {1, row_a + 0x43, 0x80}, {4, row_a + 0x1c, free_club}, {4, row_a + 0x20, free_club}};
                    cases.push_back({.entry = set_booking, .registers = {{3, humans}, {4, row}, {5, staff_record}}, .memory = memory,
                                     .label = "set booking, negative day " + std::to_string(day)});
                }
        }
    }

    // ---- 0x10149bd0: clear the rows that no longer fit. Each human's person flagged or not, the row
    // free or not.
    for (const std::uint32_t flagged : {0u, 1u, 0xffu}) {
        std::vector<Write> memory;
        for (std::uint32_t slot = 0; slot < 16 && staff_total >= 16 - slot; ++slot) {
            const auto person = scenario.person(staff_total - 16 + slot);
            memory.push_back({1, person + 0x6b, (slot & 1) ? flagged : (flagged ? 0u : 1u)});
            if (slot >= count) continue;
            const auto row = records + slot * record_size + booking;
            memory.push_back({4, row, 0x11111111u * (slot + 1)});
            memory.push_back({1, row + 0x43, slot % 3 == 0 ? 0x80u : slot % 3 == 1 ? 0x7fu : 0xffu});
            memory.push_back({4, row + 0x1c, (slot & 4) ? human_club : free_club});
            memory.push_back({4, row + 0x20, free_club});
            memory.push_back({1, row + 0x40, (slot & 4) ? 1u : 0u});
        }
        cases.push_back({.entry = clear_taken, .registers = {{3, humans}}, .memory = memory,
                         .label = "clear bookings, flag " + std::to_string(flagged)});
    }
    cases.push_back({.entry = clear_taken, .registers = {{3, humans}}, .label = "clear bookings, as it is"});

    // ---- 0x1014c5f0: the staff a club answers to. A club's +0x94 or the best of the +0x160.. candidates
    // by the +0x2a of their +0x1c record.
    for (const auto club : sample) {
        cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .label = "head staff, club as it is"});
        cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = {{4, club + 0x94, 0}}, .label = "head staff, none of its own"});
        std::vector<Write> candidates{{4, club + 0x94, 0}};
        for (unsigned slot = 0; slot < 5; ++slot) {
            const auto candidate = scenario.r32(club + 0x160 + slot * 4);
            if (!candidate) continue;
            if (const auto profile = scenario.r32(candidate + 0x1c)) {
                const std::uint32_t values[] = {5, 0x7fff, 0x8000, 5, 9};
                candidates.push_back({2, profile + 0x2a, values[slot]});
                cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = {{4, club + 0x94, 0}, {2, profile + 0x2a, 0xffff}},
                                 .label = "head staff, one candidate negative"});
                cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = {{4, club + 0x94, 0}, {4, candidate + 0x1c, 0}},
                                 .label = "head staff, candidate without a record"});
            }
            cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = {{4, club + 0x94, 0}, {4, club + 0x160 + slot * 4, 0}},
                             .label = "head staff, candidate missing"});
        }
        cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = candidates, .label = "head staff, candidates ranked"});
    }
    // Candidates the case builds: five staff of the table with their +0x1c record in the scratch area, the
    // +0x2a values (signed halfwords) ranking them, with and without the club's own staff.
    if (!sample.empty()) {
        const auto club = sample[0];
        std::vector<Write> staff_words;
        for (unsigned slot = 0; slot < 5; ++slot) staff_words.push_back({4, club + 0x160 + slot * 4, scenario.person(slot)});
        const std::uint32_t rankings[][5] = {{5, 0x7fff, 0x8000, 5, 9}, {0x8000, 0xffff, 0, 1, 0x7fff}, {7, 7, 7, 7, 7}, {1, 2, 3, 4, 5}, {5, 4, 3, 2, 1}};
        for (const auto& ranking : rankings)
            for (const std::uint32_t own : {0u, scenario.person(7)})
                for (const std::uint32_t mask : {0x1fu, 0x1eu, 0x15u, 0x0au}) {
                    auto memory = staff_words;
                    memory.push_back({4, club + 0x94, own});
                    for (unsigned slot = 0; slot < 5; ++slot) {
                        memory.push_back({4, scenario.person(slot) + 0x1c, (mask >> slot & 1) ? scratch_base + 0x4000 + slot * 0x100 : 0u});
                        memory.push_back({2, scratch_base + 0x4000 + slot * 0x100 + 0x2a, ranking[slot]});
                    }
                    cases.push_back({.entry = club_head_staff, .registers = {{3, club}}, .memory = memory, .label = "head staff, built candidates"});
                }
    }

    // ---- 0x1014c680: the current human's 50 chosen ids. The tactic slot in use, the staff's club words,
    // the people's club words and bits, and ids of -1 and of people that fit or not.
    cases.push_back({.entry = drop_unfit, .registers = {{3, humans}}, .memory = {{4, current_staff_cell, 0}}, .label = "drop, no human"});
    cases.push_back({.entry = drop_unfit, .registers = {{3, humans}}, .label = "drop, as it is"});
    if (in_table(current_staff)) {
        const auto human = human_record(current_staff);
        const auto people = scenario.global(-0x7be0);
        std::uint32_t real_job = 0;
        for (std::uint32_t index = 0; index < staff_total && !real_job; ++index) {
            const auto job = scenario.r32(scenario.person(index) + 0x20);
            if (job && scenario.r32(job + 0x2c) && scenario.r32(scenario.r32(job + 0x2c) + 0x68) < 0x10000) real_job = job;
        }
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u})
            for (const std::uint32_t job_state : {0u, 1u})
                for (const std::uint32_t bits : {0u, 4u, 8u, 12u}) {
                    const auto slot_of = kind == 1 ? 0u : kind;
                    const auto list = human + slot_of * 0x1eb4 + 0x1dec;
                    std::vector<Write> memory{{1, human + 0x9a00, kind}};
                    const std::uint32_t entries[] = {0xffffffffu, 0, 1, 2, 3, 0xffffffffu, 4};
                    for (unsigned i = 0; i < std::size(entries); ++i) memory.push_back({4, list + i * 4, entries[i]});
                    // Person 0 shares the staff's club, person 1 differs; person 2 and 3 with the bits in
                    // each state and the staff's job, or the table itself, as the other side.
                    // The current human given a real job (taken from the first person who has one), or none.
                    const auto staff_club = scenario.r32(current_staff + 0x24), job = job_state ? real_job : 0u;
                    if (job_state) memory.push_back({4, current_staff + 0x20, real_job});
                    memory.push_back({4, people + 0x24, staff_club});
                    memory.push_back({4, people + 0x70 + 0x24, staff_club + 1});
                    if (!job_state) memory.push_back({4, current_staff + 0x20, 0});
                    // What 0x100c7860 answers for the job's competition: the club table's entry at the record's +0x68
                    // plus the ranges' difference (database_records.cpp).
                    const auto competition = job ? scenario.r32(job + 0x2c) : 0u;
                    const std::uint32_t job_club = competition ? scenario.club(0) + (scenario.r32(competition + 0x68) + range_high - range_low) * 0x248 : 0u;
                    const auto other = job_state ? job_club : humans;
                    for (unsigned person = 0; person < 4; ++person) {
                        memory.push_back({4, people + person * 0x70 + 0x20, person & 1 ? other : (job_state ? job + 1 : humans + 1)});
                        memory.push_back({1, people + person * 0x70 + 0x6a, (person & 2) ? bits : (bits ^ 12)});
                    }
                    // Person 4 shares the staff's own job word and carries the bits: kinds 2 and 3 keep it when its bit is set.
                    memory.push_back({4, people + 4 * 0x70 + 0x20, job});
                    memory.push_back({1, people + 4 * 0x70 + 0x6a, bits});
                    cases.push_back({.entry = drop_unfit, .registers = {{3, humans}}, .memory = memory,
                                     .label = "drop, kind " + std::to_string(kind) + (job_state ? ", staff with a job" : ", no job") +
                                              ", bits " + std::to_string(bits)});
                }
    }
    // MATCH_DAY.CPP's update (file paths and a screen refresh a snapshot has no host for) cannot run: both
    // sides answer it, so its argument is compared instead.
    for (auto i = first_case; i < cases.size(); ++i) {
        cases[i].answers.emplace_back(match_day, 0);
        // The fixture list of a date the game has not built yet is allocated (the heap is the game's): a row
        // of another date than today's gets no list here (the walk over a real list is covered by the rows
        // 0x101499f0 is asked about directly).
        if (cases[i].entry == set_booking) cases[i].answers.emplace_back(fixture_list, 0);
    }
}
}

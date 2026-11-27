#include "croatia_second_calendar.hpp"
#include "league_base_table_detail.hpp"
#include <array>

namespace cm::aot::croatia_second {
namespace {
using guest::Cpu;
using guest::Frame;
namespace competition = comp::competition;
namespace league = league_base::league;
namespace season = league_base::season;
namespace tie_break = league_base::table::tie_break;

struct RoundDate {
    unsigned day, month;
    bool tuesday = false;
};

// Windows 3.89, CRO_A2.CPP. DATE.CPP takes a zero-based month and finds
// the requested weekday nearest to the anchor in each competition season.
constexpr std::array<RoundDate, 34> rounds{{
    {26, 7}, {2, 8}, {9, 8}, {12, 8, true}, {16, 8}, {23, 8}, {30, 8},
    {7, 9}, {14, 9}, {21, 9}, {24, 9, true}, {28, 9}, {4, 10}, {11, 10},
    {18, 10}, {25, 10}, {2, 11},
    {19, 1}, {22, 1, true}, {26, 1}, {4, 2}, {7, 2, true}, {11, 2},
    {14, 2, true}, {18, 2}, {25, 2}, {28, 2, true}, {1, 3}, {4, 3, true},
    {8, 3}, {15, 3}, {22, 3}, {29, 3}, {6, 4},
}};
constexpr int first_division_slot = -0x6a40;
}

std::uint32_t calendar(Cpu& c, std::uint32_t object, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t code_out, std::uint32_t enabled_out) {
    const Frame frame(c, 0x106dd4b0, 0xb0, {.first_gpr = 27});
    if (std::int8_t(stage) != -1) return 0;
    if (enabled_out) c.put8(enabled_out, 1);
    c.put16(count_out, rounds.size());
    c.put16(code_out, 0);
    const auto pool = c.toc(-0x40b8);
    c.set({{27, c.toc(comp::toc::message)}, {28, pool}, {29, object}, {31, c.toc(comp::toc::success)}});
    const auto schedule = comp::invoke(c, comp::callee::allocate_block, 0x106dd504,
                                       {rounds.size() * league::round_row});
    c.set(30, schedule);
    if (!schedule) {
        guest::report_assertion(c, {0x106dd520, pool, pool + 0xc, pool + 0x19,
                                    frame.at(0x70), frame.at(0x48), 0xad});
        return 0;
    }
    for (unsigned index = 0; index < rounds.size(); ++index) {
        const auto& date = rounds[index];
        const auto year = comp::sext16(c.u16(object + competition::year));
        c.put32(frame.at(0x38), 0);
        c.link(0x106dd58c);
        season::set_round_date(c, schedule, index, date.day, date.month, date.month < 6,
                               date.tuesday ? 1 : 5, date.tuesday ? 2 : 1, year, 0);
        c.link(0x106dd5ac);
        season::set_match_slot(c, schedule, index, 0, UINT32_MAX, UINT32_MAX, UINT32_MAX, 0);
    }
    return schedule;
}

std::uint32_t configure(Cpu& c, std::uint32_t object) {
    const Frame frame(c, 0x106de140, 0x50, {.first_gpr = 29});
    c.set({{29, object}, {30, 2}, {31, 3}});
    c.put16(object + competition::rounds, 2);
    c.put8(object + competition::type, 1);
    c.put8(object + league::automatic_promotion, 2);
    c.put8(object + league::playoff_clubs, 0);
    c.put8(object + league::byte_c2, 0);
    c.put8(object + league::relegation, 5);
    c.put8(object + league::win_points, 3);
    c.put8(object + league::draw_points, 1);
    c.put8(object + league::byte_c6, 2);
    c.put8(object + league::tie_breaks, tie_break::goal_difference);
    c.put8(object + league::tie_breaks + 1, tie_break::goals_scored);
    c.put8(object + league::tie_breaks + 2, tie_break::wins);
    c.put32(object + competition::parent, c.u32(c.toc(first_division_slot)));
    c.put32(object + competition::next, UINT32_MAX);
    c.put8(object + competition::bench, 7);
    c.put8(object + competition::substitutes, 3);

    const auto count = object + season::detail::field::schedule_count;
    c.set({{3, object}, {4, UINT32_MAX}, {5, count}, {6, object + competition::code}, {7, 0}});
    c.link(0x106de200);
    c.put32(object + league::schedule, calendar(c, object, UINT32_MAX, count, object + competition::code, 0));
    return 1;
}
}

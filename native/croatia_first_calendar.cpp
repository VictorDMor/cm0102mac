#include "croatia_first_calendar.hpp"
#include "croatia_second_rules.hpp"
#include "league_base_table_detail.hpp"
#include <array>
#include <span>

namespace cm::aot::croatia_first {
namespace {
using guest::Cpu;
using guest::Frame;
namespace competition = comp::competition;
namespace league = league_base::league;
namespace season = league_base::season;
namespace table = league_base::table;

struct RoundDate {
    unsigned day, month;
    unsigned weekday = 5, kind = 1;
};

// 3.89 uses the same anchors every season. Month and weekday are zero-based
// in DATE.CPP. The ten group dates are also the final ten root calendar rows.
constexpr std::array<RoundDate, 32> rounds{{
    {29, 6}, {5, 7}, {12, 7}, {19, 7}, {26, 7}, {9, 8}, {16, 8}, {23, 8},
    {30, 8}, {7, 9}, {14, 9}, {21, 9}, {28, 9}, {4, 10}, {11, 10}, {18, 10},
    {25, 10}, {17, 1}, {24, 1}, {3, 2}, {10, 2}, {17, 2}, {31, 2}, {3, 3, 1, 2},
    {7, 3}, {14, 3}, {21, 3}, {28, 3}, {5, 4}, {12, 4}, {19, 4}, {26, 4},
}};
constexpr unsigned opening_rounds = 22;
constexpr unsigned championship_group = 0x44b;
}

std::uint32_t calendar(Cpu& c, std::uint32_t object, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t code_out, std::uint32_t enabled_out) {
    const Frame frame(c, 0x106db190, 0xb0, {.first_gpr = 27});
    const int group = std::int8_t(stage);
    if (group < -1 || group > 1) return 0;
    const bool root = group == -1;
    const auto dates = std::span<const RoundDate>(rounds).subspan(root ? 0 : opening_rounds);
    if (enabled_out) c.put8(enabled_out, 1);
    c.put16(count_out, dates.size());
    c.put16(code_out, root ? 0 : championship_group + group);

    const auto pool = c.toc(-0x40bc);
    c.set({{27, c.toc(comp::toc::message)}, {28, pool}, {29, object}, {31, c.toc(comp::toc::success)}});
    const auto schedule = comp::invoke(c, comp::callee::allocate_block, root ? 0x106db1e4 : 0x106dbb60,
                                       {std::uint32_t(dates.size()) * league::round_row});
    c.set(30, schedule);
    if (!schedule) {
        guest::report_assertion(c, {root ? 0x106db200u : 0x106dbb7cu, pool, pool + 0xb, pool + 0x18,
                                    frame.at(root ? 0x70 : 0x50), frame.at(root ? 0x48 : 0x40),
                                    root ? 0xa8u : 0x121u});
        return 0;
    }
    for (unsigned index = 0; index < dates.size(); ++index) {
        const auto& date = dates[index];
        const unsigned next_year = date.month < 6;
        const auto year = comp::sext16(c.u16(object + competition::year));
        c.put32(frame.at(0x38), 0);
        c.link(0x106db26c);
        season::set_round_date(c, schedule, index, date.day, date.month, next_year, date.weekday, date.kind, year, 0);
        c.link(0x106db28c);
        season::set_match_slot(c, schedule, index, 0, UINT32_MAX, UINT32_MAX, UINT32_MAX, 0);
    }
    return schedule;
}

std::uint32_t configure(Cpu& c, std::uint32_t object) {
    const Frame frame(c, 0x106dbe20, 0x50, {.first_gpr = 29});
    c.set({{29, object}, {30, 2}, {31, 3}});
    c.put16(object + competition::rounds, 2);
    c.put16(object + table::field::played_limit, rounds.size());
    c.put32(object + table::field::compare, 0);
    c.put8(object + league::automatic_promotion, 0);
    c.put8(object + league::playoff_clubs, 0);
    c.put8(object + league::byte_c2, 0);
    c.put8(object + league::relegation, 2);
    c.put8(object + league::win_points, 3);
    c.put8(object + league::draw_points, 1);
    c.put8(object + league::byte_c6, 2);
    c.put16(object + league::half_a8, 0);
    c.put16(object + league::half_de, 0);
    c.put8(object + competition::type, 1);
    c.put8(object + league::tie_breaks, table::tie_break::goal_difference);
    c.put8(object + league::tie_breaks + 1, table::tie_break::goals_scored);
    c.put8(object + league::tie_breaks + 2, table::tie_break::wins);
    c.put32(object + competition::parent, UINT32_MAX);
    c.put32(object + competition::next, c.u32(c.toc(croatia_second::national_division_slot)));
    c.put8(object + competition::bench, 7);
    c.put8(object + competition::substitutes, 3);
    c.put8(object + competition::byte_4c, 2);

    const auto count = object + season::detail::field::schedule_count;
    c.set({{3, object}, {4, UINT32_MAX}, {5, count}, {6, object + competition::code}, {7, 0}});
    c.link(0x106dbedc);
    const auto schedule = calendar(c, object, UINT32_MAX, count, object + competition::code, 0);
    c.put16(count, opening_rounds);
    c.put32(object + league::schedule, schedule);
    return 1;
}
}

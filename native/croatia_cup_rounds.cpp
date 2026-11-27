#include "croatia_cup_rounds.hpp"
#include "croatia_cup_profile.hpp"
#include "cup_base_detail.hpp"

#include <array>

namespace cm::aot::croatia_cup {
namespace {
using guest::Cpu;
using cup_base::Round;

void set_format(Cpu& c, std::uint32_t row, unsigned index, const RoundRules& format) {
    c.put16(row + Round::label, format.label);
    c.put16(row + Round::index, index);
    c.put16(row + 0x0c, 0);
    c.put16(row + Round::flags, format.legs == 1);
    c.put16(row + Round::field_10, format.match_flags);
    c.put8(row + 0x18, format.fixture_rule);
    c.put16(row + Round::entrants, format.clubs);
    c.put16(row + Round::ties, format.clubs / 2);
    c.put16(row + 0x1e, format.new_entrants);
    c.put16(row + 0x20, format.first_entrant);
    c.put8(row + 0x22, 0);
    c.put8(row + Round::leg_rule, format.legs);
    c.put8(row + 0x24, format.leg_gap);
    c.put32(row + Round::prizes, 0);
    c.put32(row + Round::prizes + 4, UINT32_MAX);
    c.put32(row + Round::prizes + 8, 0);
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t cup, std::uint32_t stage, std::uint32_t count_out,
                       std::uint32_t code_out, std::uint32_t enabled_out, const Profile& profile) {
    const guest::Frame frame(c, 0x106bf4c0, 0x80, {.first_gpr = 28});
    const auto pool = c.toc(-0x4120);
    c.set({{28, c.toc(comp::toc::message)}, {29, pool}, {30, cup}});
    c.compare(0, std::int8_t(stage), -1);
    if (std::int8_t(stage) != -1) return 0;
    c.compare_logical(0, enabled_out, 0);
    if (enabled_out) c.put8(enabled_out, 0);
    constexpr unsigned round_count = std::tuple_size_v<decltype(Profile::rounds)>;
    c.put16(count_out, round_count);
    c.put16(code_out, 0);
    const auto schedule = comp::invoke(c, comp::callee::allocate_block, 0x106bf51c,
                                       {round_count * cup_base::cup::round_row});
    c.set(31, schedule);
    c.compare(0, std::int32_t(schedule), 0);
    if (!schedule) {
        guest::report_assertion(c, {0x106bf538, pool, pool + 0xc, pool + 0x19, frame.at(0x48), frame.at(0x40), 0x99});
        c.set(4, c.toc(comp::toc::success));
        return 0;
    }

    // The in-place date helpers retain the original return sites and frame.
    constexpr std::array<unsigned, 6> draw_back{
        0x106bf59c, 0x106bf644, 0x106bf6ec, 0x106bf798, 0x106bf848, 0x106bf8f0};
    constexpr std::array<unsigned, 5> saved_r29{0, 1, 0, 0, 4};
    static_assert(draw_back.size() == round_count);
    for (unsigned i = 0; i < profile.rounds.size(); ++i) {
        const auto& round = profile.rounds[i];
        const auto year = comp::sext16(c.u16(cup + comp::competition::year));
        const auto& d = round.draw;
        c.set({{3, schedule}, {4, i}, {5, d.day}, {6, d.month}, {7, d.next_year}, {8, d.weekday}, {9, year}});
        c.link(draw_back[i]);
        cup_base::daily::set_draw_date(c, schedule, i, d.day, d.month, d.next_year, d.weekday, year);
        c.put32(frame.at(0x38), 0);
        const auto& f = round.fixture;
        const auto fixture_year = comp::sext16(c.u16(cup + comp::competition::year));
        const unsigned kind = i == 0 ? 1 : 2;
        c.set({{3, schedule}, {4, i}, {5, f.day}, {6, f.month}, {7, f.next_year},
               {8, f.weekday}, {9, kind}, {10, fixture_year}});
        c.link(draw_back[i] + 0x2c);
        cup_base::daily::set_fixture_date(c, schedule, i, f.day, f.month, f.next_year, f.weekday, kind, fixture_year, 0);
        set_format(c, schedule + i * cup_base::cup::round_row, i, round);
        if (i < saved_r29.size()) c.set(29, saved_r29[i]);
    }
    c.set({{3, schedule}, {4, profile.rounds.back().leg_gap}, {5, 1}, {6, 2}, {7, 6}, {8, 0}, {11, 3}, {12, 0}});
    return schedule;
}
}

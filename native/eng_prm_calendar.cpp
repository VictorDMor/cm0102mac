#include "eng_prm_detail.hpp"
#include <span>

namespace cm::aot::eng_prm {
using namespace detail;
namespace {
struct TimeOption {
    std::int32_t index, weekday, date_rule, time_rule, flags;
    std::uint32_t back;
};
struct Round {
    std::int32_t index, day, month, next_year, weekday, flags;
    std::uint32_t back;
    unsigned option_count;
    std::array<TimeOption, 4> options;
};
#include "eng_prm_calendar_rules.inc"

void append_rounds(Cpu& c, const Frame& frame, const Competition& league,
                   std::uint32_t schedule, std::span<const Round> rounds) {
    Program p{c};
    for (const auto& round : rounds) {
        c.put32(frame.at(0x38), 0);
        p.call(0x104a9810, round.back,
               {schedule, std::uint32_t(round.index), std::uint32_t(round.day), std::uint32_t(round.month),
                std::uint32_t(round.next_year), std::uint32_t(round.weekday), std::uint32_t(round.flags),
                extended(league.year())});
        for (unsigned i = 0; i < round.option_count; ++i) {
            const auto& option = round.options[i];
            p.call(0x104a9890, option.back,
                   {schedule, std::uint32_t(round.index), std::uint32_t(option.index), std::uint32_t(option.weekday),
                    std::uint32_t(option.date_rule), std::uint32_t(option.time_rule), std::uint32_t(option.flags)});
        }
    }
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t extra_out, std::uint32_t enabled_out) {
    Frame f(c, 0x10529c20, 0xb0, {.first_gpr = 27});
    keep(c, {{29, c.toc(-0x7bac)}, {27, address}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    if (compare(c, std::int8_t(stage), -1) != 0) return 0;
    if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 1);
    c.put16(count_out, 38); c.put16(extra_out, 0);
    Program p{c}; Competition league{c, address};
    Kept<28> schedule(c, p.call(0x10775130, 0x10529c74, {0xb48}));
    if (compare(c, schedule) == 0) {
        assertion(c, f, 0x10529c90, 0x78, 0x50, 0xa7); return 0;
    }
    append_rounds(c, f, league, schedule, opening_rounds);
    p.call(0x107156f0, 0x1052a7e4, {f.at(0x40), 1, 11, std::uint16_t(league.year()), missing});
    const auto relative_day = std::int16_t(std::uint32_t(c.i16(std::uint32_t(schedule) + 0x50c)) -
                                         std::uint32_t(c.i16(c.r(3))) + 1);
    const auto branch = std::uint32_t(std::int32_t(relative_day)) - 13;
    compare_logical(c, branch, 6);
    switch (branch) {
        case 0: append_rounds(c, f, league, schedule, december_0); break;
        case 1: append_rounds(c, f, league, schedule, december_1); break;
        case 2: append_rounds(c, f, league, schedule, december_2); break;
        case 3: append_rounds(c, f, league, schedule, december_3); break;
        case 4: append_rounds(c, f, league, schedule, december_4); break;
        case 5: append_rounds(c, f, league, schedule, december_5); break;
        case 6: append_rounds(c, f, league, schedule, december_6); break;
        default:
            assertion(c, f, 0x1052b098, 0x58, 0x48, 0x173);
            append_rounds(c, f, league, schedule, december_fallback);
            break;
    }
    append_rounds(c, f, league, schedule, closing_rounds);
    return schedule;
}
}

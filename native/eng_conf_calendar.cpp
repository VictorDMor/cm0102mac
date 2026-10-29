#include "eng_conf_detail.hpp"
#include <span>

namespace cm::aot::eng_conf {
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
    TimeOption options[2];
};
#include "eng_conf_calendar_rules.inc"

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

// Each knockout round is a 0x78-byte guest record. Date helpers own its
// date fields; these are the competition's format/ranking configuration.
void configure_knockout_round(Cpu& c, std::uint32_t record, unsigned label,
                              unsigned index, unsigned flags, unsigned entrants,
                              unsigned ties, unsigned return_legs, unsigned advance,
                              unsigned seeding, unsigned kind, unsigned venues) {
    c.put16(record + 8, label); c.put16(record + 0xa, index);
    c.put16(record + 0xc, 0); c.put16(record + 0xe, flags);
    c.put8(record + 0x18, 5); c.put16(record + 0x1a, entrants);
    c.put16(record + 0x1c, ties); c.put16(record + 0x1e, return_legs);
    c.put16(record + 0x10, advance); c.put16(record + 0x20, seeding);
    c.put8(record + 0x22, 0); c.put8(record + 0x23, kind); c.put8(record + 0x24, venues);
    c.put32(record + 0x6c, 0); c.put32(record + 0x70, 0); c.put32(record + 0x74, 0);
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t extra_out, std::uint32_t enabled_out) {
    Frame f(c, 0x10524a50, 0xe0, {.first_gpr = 27});
    keep(c, {{29, c.toc(-0x7bac)}, {27, address}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 1);
        c.put16(count_out, 42); c.put16(extra_out, 0);
        Kept<28> schedule(c, p.call(0x10775130, 0x10524aa4, {0xc78}));
        if (compare(c, schedule) == 0) {
            assertion(c, f, 0x10524ac0, 0xa0, 0x58, 0xb1); return 0;
        }
        append_rounds(c, f, league, schedule, opening_rounds);
        p.call(0x107156f0, 0x10525358, {f.at(0x40), 1, 11, std::uint16_t(league.year()), missing});
        const auto relative_day = std::int16_t(std::uint32_t(c.i16(std::uint32_t(schedule) + 0x5f0)) -
                                             std::uint32_t(c.i16(c.r(3))) + 1);
        c.set(3, extended(relative_day));
        const auto branch = std::uint32_t(std::int32_t(relative_day)) - 16;
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
                assertion(c, f, 0x105257e4, 0x80, 0x50, 0x13d);
                append_rounds(c, f, league, schedule, december_fallback);
                break;
        }
        append_rounds(c, f, league, schedule, closing_rounds);
        return schedule;
    }
    if (compare(c, std::int8_t(stage)) != 0) return 0;
    if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 0);
    c.put16(count_out, 3); c.put16(extra_out, 160);
    Kept<28> schedule(c, p.call(0x10775130, 0x10526070, {0x168}));
    if (compare(c, schedule) == 0) {
        assertion(c, f, 0x1052608c, 0x60, 0x48, 0x19f); return 0;
    }
    p.call(0x104854f0, 0x105260ec, {schedule, 0, 7, 4, 1, 0, extended(league.year())});
    c.put32(f.at(0x38), 0);
    p.call(0x10485570, 0x10526118, {schedule, 0, 12, 4, 1, 5, 1, extended(league.year())});
    configure_knockout_round(c, schedule, 130, 0, 0x204, 4, 2, 4, 3, 0, 2, 4);
    c.set(10, 4);
    p.call(0x104854f0, 0x10526194, {schedule, 1, 17, 4, 1, 3, extended(league.year())});
    c.put32(f.at(0x38), 0);
    p.call(0x10485570, 0x105261c0, {schedule, 1, 19, 4, 1, 5, 1, extended(league.year())});
    keep(c, {{29, 1}});
    configure_knockout_round(c, std::uint32_t(schedule) + 0x78, 150, 1, 0x204, 2, 1, 0, 3, 0, 2, 4);
    c.set(10, 2);
    p.call(0x104854f0, 0x10526240, {schedule, 2, 24, 4, 1, 3, extended(league.year())});
    c.put32(f.at(0x38), 4);
    p.call(0x10485570, 0x1052626c, {schedule, 2, 26, 4, 1, 5, 1, extended(league.year())});
    configure_knockout_round(c, std::uint32_t(schedule) + 0xf0, 360, 2, 3, 2, 1, 1, 0, 4, 1, 0);
    leave(c, {{4, 1}, {5, 5}, {6, 3}, {7, 0}, {8, 2}});
    return schedule;
}
}

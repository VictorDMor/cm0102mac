#include "bra_reg_central_detail.hpp"

namespace cm::aot::bra_reg_central {
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
    TimeOption options[3];
};
#include "bra_reg_central_calendar_rules.inc"

// Each knockout round is a 0x78-byte guest record. Date helpers own its date
// fields; these are the competition's format values. No reader was traced for
// the last three, so they keep their offsets as names.
struct KnockoutRound {
    unsigned label, index, flags, entrants, ties, field_1e;
    std::uint32_t amount_70, amount_74;
};
void configure_knockout_round(Cpu& c, std::uint32_t record, const KnockoutRound& round) {
    c.put16(record + 8, round.label); c.put16(record + 0xa, round.index);
    c.put16(record + 0xc, 0); c.put16(record + 0xe, round.flags);
    c.put8(record + 0x18, 8); c.put16(record + 0x1a, round.entrants);
    c.put16(record + 0x1c, round.ties); c.put16(record + 0x1e, round.field_1e);
    c.put16(record + 0x10, 0x804); c.put16(record + 0x20, 0);
    c.put8(record + 0x22, 0); c.put8(record + 0x23, 2); c.put8(record + 0x24, 7);
    c.put32(record + 0x6c, 0); c.put32(record + 0x70, round.amount_70); c.put32(record + 0x74, round.amount_74);
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t extra_out, std::uint32_t enabled_out) {
    Frame f(c, 0x106d7830, 0xb0, {.first_gpr = 27});
    keep(c, {{31, c.toc(slot::flag)}, {29, address}, {27, c.toc(slot::message)}, {28, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 1);
        c.put16(count_out, 15); c.put16(extra_out, 0);
        Kept<30> schedule(c, p.call(0x10775130, 0x106d7884, {0x474}));
        if (compare(c, schedule) == 0) {
            assertion(c, f, 0x106d78a0, 0x70, 0x48, 0xa4); return 0;
        }
        for (const auto& round : league_rounds) {
            c.put32(f.at(0x38), 0);
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
        return schedule;
    }
    if (compare(c, std::int8_t(stage)) != 0) return 0;
    if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 0);
    c.put16(count_out, 3); c.put16(extra_out, 0);
    Kept<30> schedule(c, p.call(0x10775130, 0x106d80cc, {0x168}));
    if (compare(c, schedule) == 0) {
        assertion(c, f, 0x106d80e8, 0x50, 0x40, 0x114); return 0;
    }
    const auto first_leg = [&](std::uint32_t index, std::uint32_t day, std::uint32_t month, std::uint32_t back) {
        p.call(0x104854f0, back, {schedule, index, day, month, 0, 0, extended(league.year())});
    };
    const auto second_leg = [&](std::uint32_t index, std::uint32_t day, std::uint32_t month, std::uint32_t back) {
        c.put32(f.at(0x38), 0);
        p.call(0x10485570, back, {schedule, index, day, month, 0, 6, 1, extended(league.year())});
    };
    first_leg(0, 7, 4, 0x106d8148);
    second_leg(0, 13, 4, 0x106d8174);
    keep(c, {{31, 0}});
    configure_knockout_round(c, schedule, {120, 0, 0x200, 8, 4, 8, 0, 0});
    c.set(10, 0x804);
    first_leg(1, 21, 4, 0x106d81f4);
    second_leg(1, 27, 4, 0x106d8220);
    configure_knockout_round(c, std::uint32_t(schedule) + 0x78, {130, 1, 0x200, 4, 2, 0, 0, 0});
    c.set(10, 2);
    first_leg(2, 4, 5, 0x106d82a4);
    second_leg(2, 10, 5, 0x106d82d0);
    configure_knockout_round(c, std::uint32_t(schedule) + 0xf0, {150, 2, 0, 2, 1, 0, 250000, 100000});
    leave(c, {{4, 250000}, {5, 7}, {6, 0x804}, {7, 1}, {8, 8}, {9, 0}, {10, 2}});
    return schedule;
}
}

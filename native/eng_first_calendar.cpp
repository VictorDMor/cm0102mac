#include "eng_first_detail.hpp"
#include "english_competition_rules.hpp"
#include <array>
#include <span>

namespace cm::aot::eng_first {
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
    std::array<TimeOption, 3> options;
};
#include "eng_first_calendar_rules.inc"

void append_rounds(Cpu& c, const Frame& frame, const Competition& league,
                   std::uint32_t schedule, std::span<const Round> rounds) {
    Program p{c};
    for (const auto& round : rounds) {
        c.put32(frame.at(0x38), 0);
        p.call(0x104a9810, round.back,
               {schedule, std::uint32_t(round.index), std::uint32_t(round.day), std::uint32_t(round.month),
                std::uint32_t(round.next_year), std::uint32_t(round.weekday), std::uint32_t(round.flags), extended(league.year())});
        for (unsigned i = 0; i < round.option_count; ++i) {
            const auto& option = round.options[i];
            p.call(0x104a9890, option.back,
                   {schedule, std::uint32_t(round.index), std::uint32_t(option.index), std::uint32_t(option.weekday),
                    std::uint32_t(option.date_rule), std::uint32_t(option.time_rule), std::uint32_t(option.flags)});
        }
    }
}
void knockout_round(Cpu& c, std::uint32_t record, unsigned label, unsigned index,
                    unsigned flags, unsigned entrants, unsigned ties, unsigned new_entrants,
                    unsigned replay_flags, unsigned entrant_offset, unsigned legs, unsigned leg_gap) {
    c.put16(record + 8, label); c.put16(record + 0xa, index);
    c.put16(record + 0xc, 0); c.put16(record + 0xe, flags);
    c.put8(record + 0x18, 5); c.put16(record + 0x1a, entrants);
    c.put16(record + 0x1c, ties); c.put16(record + 0x1e, new_entrants);
    c.put16(record + 0x10, replay_flags); c.put16(record + 0x20, entrant_offset);
    c.put8(record + 0x22, 0); c.put8(record + 0x23, legs); c.put8(record + 0x24, leg_gap);
    c.put32(record + 0x6c, 0); c.put32(record + 0x70, 0); c.put32(record + 0x74, 0);
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t extra_out, std::uint32_t enabled_out) {
    Frame f(c, 0x10526c00, 0xe0, {.first_gpr = 27});
    keep(c, {{29, c.toc(-0x7bac)}, {27, address}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 1);
        c.put16(count_out, 46); c.put16(extra_out, 0);
        Kept<28> schedule(c, p.call(0x10775130, 0x10526c54, {0xda8}));
        if (compare(c, schedule) == 0) { assertion(c, f, 0x10526c70, 0xa0, 0x58, 0xae); return 0; }
        append_rounds(c, f, league, schedule, opening_rounds);
        p.call(0x107156f0, 0x105277a0, {f.at(0x40), 1, 11, std::uint16_t(league.year()), missing});
        const auto relative_day = std::int16_t(std::uint32_t(c.i16(std::uint32_t(schedule) + 0x688)) -
                                             std::uint32_t(c.i16(c.r(3))) + 1);
        c.set(3, extended(relative_day));
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
                assertion(c, f, 0x10528054, 0x80, 0x50, 0x17c);
                append_rounds(c, f, league, schedule, december_fallback);
                break;
        }
        append_rounds(c, f, league, schedule, closing_rounds);
        return schedule;
    }
    if (compare(c, std::int8_t(stage)) != 0) return 0;
    if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, 0);
    if (english_rules::expanded_season(c, address)) {
        using namespace english_rules;
        c.put16(count_out, playoff_rounds); c.put16(extra_out, 160);
        Kept<28> schedule(c, p.call(0x10775130, 0x10528a98, {playoff_rounds * round_bytes}));
        if (!schedule) { assertion(c, f, 0x10528ab4, 0x60, 0x48, 0x1ed); return 0; }
        for (unsigned i = 0; i < playoff_rounds * round_bytes; ++i) c.put8(std::uint32_t(schedule) + i, 0);
        for (unsigned round = 0; round < playoff_rounds; ++round) {
            const bool final = round == playoff_rounds - 1;
            p.call(0x104854f0, 0x10528b14,
                   {schedule, round, 7 + round * 7, 4, 1, 0, extended(league.year())});
            c.put32(f.at(0x38), final ? neutral_venue : home_venue);
            p.call(0x10485570, 0x10528b40,
                   {schedule, round, final ? 28u : 12 + round * 7, 4, 1,
                    final ? 0u : 5u, final ? 2u : 1u, extended(league.year())});
            const auto record = std::uint32_t(schedule) + round * round_bytes;
            knockout_round(c, record, final ? 150 : round == 0 ? 120 : 130, round,
                           fixed_draw | extra_time_and_penalties, 8 >> round, 4 >> round,
                           round == 0 ? 8 : 0, 0, 0, 1, 0);
            c.put16(record + 0xc, final ? 0 : higher_seed_home);
        }
        return schedule;
    }
    c.put16(count_out, 2); c.put16(extra_out, 160);
    Kept<28> schedule(c, p.call(0x10775130, 0x10528a98, {0xf0}));
    if (compare(c, schedule) == 0) { assertion(c, f, 0x10528ab4, 0x60, 0x48, 0x1ed); return 0; }
    p.call(0x104854f0, 0x10528b14, {schedule, 0, 7, 4, 1, 0, extended(league.year())});
    c.put32(f.at(0x38), 0);
    p.call(0x10485570, 0x10528b40, {schedule, 0, 12, 4, 1, 5, 1, extended(league.year())});
    knockout_round(c, schedule, 130, 0, 0x204, 4, 2, 4, 3, 0, 2, 4);
    c.set(10, 4);
    p.call(0x104854f0, 0x10528bbc, {schedule, 1, 17, 4, 1, 3, extended(league.year())});
    c.put32(f.at(0x38), 4);
    p.call(0x10485570, 0x10528be8, {schedule, 1, 28, 4, 1, 0, 2, extended(league.year())});
    knockout_round(c, std::uint32_t(schedule) + 0x78, 150, 1, 3, 2, 1, 0, 0, 0, 1, 0);
    leave(c, {{4, 5}, {5, 3}, {6, 0}, {7, 1}});
    return schedule;
}
}

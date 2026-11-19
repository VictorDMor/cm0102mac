// CUP.CPP, block `daily`: the cup's daily update (vt +0x1c) and the question whether it is finished (vt +0x18).
// Registers follow the original where a callee can see or save them: the non-volatile ones (r25-r31) are set where the
// original defines them, and r3-r6 where a later call or a return leaves them behind.
#include "cup_base_daily.hpp"
#include "cup_base_detail.hpp"
#include "date.hpp"
#include "inter_amer_cup_retirement.hpp"

namespace cm::aot::cup_base::daily {
namespace {
namespace field {
inline constexpr std::uint32_t
    stage_count_word = 0x30,   // competition::last_stage: the last stage index (-1 none)
    stage_total = 0x2c,        // competition::word_2c: the number of stages
    stats = 0x08,              // competition::stats: the statistics object (0 none)
    owner = 0x44,              // competition::owner: -1 for a competition that is its own
    active = 0x45,             // competition::byte_45
    closed = 0x49,             // byte: the stages are finished and the statistics were closed (inferred)
    round_year = 0x40;         // competition::year
}
namespace cell {
inline constexpr std::int32_t
    today = -0x7b80,           // cell: the current date (day halfword, year halfword) (comp_detail's reader of the season)
    league_table_owner = -0x7ad4,  // cell: a word, the object 0x1000cfc0 files a finished competition in (inferred)
    phase_byte = -0x79ec;    // cell: a global byte; 1 lets the morning pass draw a final round (inferred: the day's phase)
}
// The fixture-end rounds carry this label (a round's +8 halfword): the final (0x8c, as the tree block's `final_label`).
constexpr std::int32_t final_label = 0x8c;
constexpr std::uint32_t round_row = cup::round_row;

// The date of a round (day halfword at the row's start, year byte at +2 added to the season's year) into `buffer`.
// `with_row` is whether the original loads the row table into r6 first (only the loops do).
void round_date(Cpu& c, std::uint32_t cup_object, std::uint32_t rounds, std::uint32_t row, std::uint32_t buffer,
                std::uint32_t back, bool with_row) {
    const auto day = sext16(c.u16(rounds + row));
    const auto year = (sext16(c.u16(cup_object + field::round_year)) + sext8(c.u8(rounds + row + 2))) & 0xffffu;
    c.set({{3, buffer}, {4, day}, {5, year}});
    if (with_row) c.set(6, rounds);
    c.link(back);
    date::from_day_and_year(c, buffer, day, year);
}
}

std::uint32_t finished(Cpu& c, std::uint32_t cup_object, std::uint32_t mode) {
    const Frame frame(c, 0x10485410, 0x50, {.first_gpr = 29});
    c.set(29, cup_object);
    if (sext8(c.u8(cup_object + cup::byte_b0)) == 0) return 0;
    if (std::int32_t(mode) == 1 && std::int8_t(c.u8(cup_object + field::owner)) == -1) {
        c.set({{30, 0}, {31, 0}});
        std::uint32_t index = 0, offset = 0;
        for (;; ++index, offset += 4) {
            const auto last = c.i32(cup_object + field::stage_count_word);
            c.set(4, std::uint32_t(last));
            if (std::int32_t(index) > last) break;
            const auto stage = c.u32(c.u32(cup_object + competition::stages) + offset);
            if (std::int8_t(virtual_call(c, 0x18, 0x10485480, {stage, 0, 1})) == 0) return 0;
            c.set({{30, index + 1}, {31, offset + 4}});
        }
        const auto total = c.u32(cup_object + field::stage_total);
        c.set(3, total);
        if (c.i32(cup_object + field::stage_count_word) < std::int32_t(total) - 1) return 0;
    }
    return 1;
}

std::uint32_t daily_update(Cpu& c, std::uint32_t cup_object, std::uint32_t when, std::uint32_t phase) {
    // The generator's hook (the competition retirement policy) runs ahead of this routine: a retired Inter-American
    // Cup returns 0 without touching anything. A checker's dispatch and a direct call reach this body without it.
    if (inter_amer_cup_retired_object(c.s, cup_object)) return 0;
    const Frame frame(c, 0x10482ca0, 0xa0, {.first_gpr = 25});
    const auto today = c.toc(cell::today);
    // r27 the cup, r28 the date word's address, r29 the phase, r30 "every stage finished" (then a stage offset, then
    // the round's number), r31 the date cell; r25/r26 count the stages and their offsets.
    c.set({{27, cup_object}, {28, when}, {29, phase}, {31, today}, {30, 1}});
    // The date the day arrives with, copied to the frame (day, year, leap byte at sp+0x78).
    c.set({{5, sext16(c.u16(when))}, {4, c.u16(when + 2)}, {3, c.u8(when + 4)}});
    c.put16(frame.at(0x78), c.u16(when));
    c.put16(frame.at(0x7a), c.u16(when + 2));
    c.put8(frame.at(0x7c), c.u8(when + 4));
    const auto stage = [&](std::uint32_t offset) { return c.u32(c.u32(cup_object + competition::stages) + offset); };
    const auto last_stage = [&] { return c.i32(cup_object + field::stage_count_word); };
    const auto count = [&] { return sext16(c.u16(cup_object + competition::rounds)); };
    const auto is_own = [&] { return std::int8_t(c.u8(cup_object + field::owner)) == -1; };

    if (is_own()) {
        if (sext8(c.u8(cup_object + cup::byte_b0)) != 0) {
            bool every_stage_done = true;
            std::uint32_t index = 0, offset = 0;
            c.set({{25, 0}, {26, 0}});
            while (std::int32_t(index) <= last_stage()) {
                if (std::int8_t(virtual_call(c, 0x18, 0x10482d20, {stage(offset), 0, 1})) == 0) {
                    every_stage_done = false;
                    c.set(30, 0);
                    break;
                }
                ++index;
                offset += 4;
                c.set({{25, index}, {26, offset}});
            }
            if (every_stage_done) c.set({{3, c.u32(cup_object + field::stage_total)}, {4, std::uint32_t(last_stage())}});
            if (every_stage_done && last_stage() < c.i32(cup_object + field::stage_total) - 1) {
                virtual_call(c, 0x30, 0x10482d78, {cup_object});
            } else if (every_stage_done && sext8(c.u8(cup_object + field::closed)) == 0) {
                invoke(c, 0x1000cfc0, 0x10482da4, {c.u32(c.toc(cell::league_table_owner)), cup_object});
                virtual_call(c, 0x38, 0x10482db4, {cup_object});
                const auto statistics = c.u32(cup_object + field::stats);
                c.set(3, statistics);
                if (statistics) invoke(c, 0x10456290, 0x10482dc8, {statistics});
                c.put8(cup_object + field::closed, 1);
            }
        }
        const auto statistics = c.u32(cup_object + field::stats);
        c.set(3, statistics);
        if (statistics) invoke(c, 0x104573c0, 0x10482de0, {statistics});
    }
    if (is_own()) {
        std::uint32_t index = 0, offset = 0;
        c.set({{25, 0}, {30, 0}});
        while (std::int32_t(index) <= last_stage()) {
            virtual_call(c, 0x1c, 0x10482e18, {stage(offset), today, phase});
            ++index;
            offset += 4;
            c.set({{25, index}, {30, offset}});
        }
    }

    // The last round, when it is the final (label 0x8c): its date against today's (day and year halfwords); on its day
    // the round is drawn (`step` 0x1047e980, morning) or advanced (0x1047ef50, evening). The date word is copied to
    // `copy` (two words, the second read whole: it carries the leap byte and three bytes beyond).
    const auto final_round_today = [&](std::uint32_t buffer, std::uint32_t copy, std::uint32_t date_back,
                                       std::uint32_t step, std::uint32_t step_back) {
        c.set(3, std::uint32_t(count()));
        if (std::int32_t(count()) <= 0) return;
        const auto rounds = c.u32(cup_object + cup::rounds);
        const auto row = (count() - 1) * round_row;
        c.set({{3, row}, {4, rounds}});
        if (sext16(c.u16(rounds + row + Round::label)) != final_label) return;
        round_date(c, cup_object, rounds, row, buffer, date_back, false);
        const auto first = c.u32(when), second = c.u32(when + 4);
        c.set({{4, 0}, {5, second}});
        c.put32(copy, first);
        c.put32(copy + 4, second);
        c.set(3, c.u16(copy + 2));
        if (c.u16(copy + 2) != c.u16(buffer + 2)) return;
        c.set(3, sext16(c.u16(copy)));
        if (c.i16(copy) != c.i16(buffer)) return;
        c.set(4, 1);
        invoke(c, step, step_back, {cup_object, count() - 1});
    };

    if (std::int8_t(phase) == 0 && c.u8(c.toc(cell::phase_byte)) == 1) {
        final_round_today(frame.at(0x70), frame.at(0x50), 0x10482e9c, 0x1047e980, 0x10482eec);
        if (sext8(c.u8(cup_object + cup::byte_b0)) == 0) {
            c.set(3, sext8(c.u8(cup_object + cup::byte_a9)));
            while (sext8(c.u8(cup_object + cup::byte_a9)) != count()) {
                const auto raw = c.u8(cup_object + cup::byte_a9);
                const auto rounds = c.u32(cup_object + cup::rounds);
                c.set(30, sext8(raw));
                const auto buffer = frame.at(0x68);
                round_date(c, cup_object, rounds, sext8(raw) * round_row, buffer, 0x10482f40, true);
                const auto first = c.u32(buffer), second = c.u32(buffer + 4);
                c.set(4, second);
                c.put32(frame.at(0x48), first);
                c.put32(frame.at(0x4c), second);
                const auto year_of_row = c.u16(frame.at(0x4a)), year_today = c.u16(when + 2);
                c.set(3, year_of_row);
                std::uint32_t before_today;
                if (year_of_row > year_today) before_today = 1;
                else if (year_of_row < year_today) before_today = 0;
                else {
                    // Equal years: whether the row's day (signed halfword) is before today's, through the original's
                    // xor/shift expression (`srawi` of the xor less the and, then the sign bit).
                    const auto row_day = sext16(c.u16(frame.at(0x48))), today_day = sext16(c.u16(when));
                    const auto difference = row_day ^ today_day;
                    const auto shifted = std::uint32_t(std::int32_t(difference) >> 1);
                    c.set({{4, row_day}, {3, shifted}});
                    before_today = ((shifted - (difference & row_day)) >> 31) & 1;
                }
                if (std::int8_t(before_today) != 0) break;
                const auto number = sext8(c.u8(cup_object + cup::byte_a9));
                invoke(c, 0x1047e980, 0x10482fac, {cup_object, number});
                if (sext8(raw) == sext8(c.u8(cup_object + cup::byte_a9))) break;
                c.set(3, sext8(c.u8(cup_object + cup::byte_a9)));
            }
        }
    }

    if (std::int8_t(phase) != 0 && sext8(c.u8(cup_object + field::active)) != 0) {
        final_round_today(frame.at(0x60), frame.at(0x40), 0x10483038, 0x1047ef50, 0x10483088);
        if (sext8(c.u8(cup_object + cup::byte_b0)) == 0 && sext8(c.u8(cup_object + field::active)) != 0) {
            c.set(3, sext8(c.u8(cup_object + cup::current_round)));
            while (sext8(c.u8(cup_object + cup::current_round)) != count()) {
                const auto raw = c.u8(cup_object + cup::current_round);
                const auto rounds = c.u32(cup_object + cup::rounds);
                c.set(30, sext8(raw));
                const auto buffer = frame.at(0x58);
                round_date(c, cup_object, rounds, sext8(raw) * round_row, buffer, 0x104830e8, true);
                const auto first = c.u32(buffer), second = c.u32(buffer + 4);
                c.set({{4, 0}, {5, second}});
                c.put32(frame.at(0x38), first);
                c.put32(frame.at(0x3c), second);
                c.set(3, c.u16(frame.at(0x3a)));
                bool its_day = false;
                if (c.u16(frame.at(0x3a)) == c.u16(when + 2)) {
                    c.set(3, sext16(c.u16(frame.at(0x38))));
                    its_day = c.i16(frame.at(0x38)) == c.i16(when);
                }
                c.set(4, its_day ? 1 : 0);
                if (!its_day) {
                    // Not its day: only a round whose matches are all played (byte +0x34 of the round's rows) goes on.
                    const auto matches = c.u32(c.u32(cup_object + cup::round_matches) +
                                               sext8(c.u8(cup_object + cup::current_round)) * 4);
                    c.set(3, matches);
                    if (sext8(c.u8(matches + 0x34)) == 0) return 0;
                }
                const auto number = sext8(c.u8(cup_object + cup::current_round));
                invoke(c, 0x1047ef50, 0x10483158, {cup_object, number});
                if (sext8(raw) == sext8(c.u8(cup_object + cup::current_round))) break;
                c.set(3, sext8(c.u8(cup_object + cup::current_round)));
            }
        }
    }
    return 0;
}

}

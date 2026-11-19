// CUP.CPP, block `daily`: the decisions on a played tie (vt +0xa4) and on a fixture (vt +0xa0). A tie row's figures
// are signed bytes at +0x14..+0x25 (two sides each: points, goal difference, goals ...) (inferred); a fixture's flags
// are the halfword at +0x38 and its two outcome bytes +0x3d and +0x3e (0, 1 or 8: a side's mark; -3 and -1 adjust it).
// Registers: the original's r4 and r6-r9 survive to the return where it writes them last; the helpers below set them
// at the same points.
#include "cup_base_daily.hpp"
#include "cup_base_detail.hpp"

namespace cm::aot::cup_base::daily {
namespace {
namespace flag {
inline constexpr std::uint32_t one = 1, two = 2, four = 4, decided = 0x10, either = 0x21, replay = 0x2000,
                               first_side = 0x800, second_side = 0x1000, away_rule = 0x4000;
}
namespace fixture {
inline constexpr std::uint32_t flags = 0x38, first_mark = 0x3d, second_mark = 0x3e;
}
}

std::uint32_t tie_outcome(Cpu& c, std::uint32_t, std::uint32_t tie, std::uint32_t fixture_row) {
    const Frame frame(c, 0x10482640, 0x70, {.first_gpr = 30});
    c.set({{31, c.toc(toc::pool)}, {30, c.toc(toc::message)}});
    const auto flags = sext16(c.u16(fixture_row + fixture::flags));
    const auto has = [&](std::uint32_t bit) { return (flags & bit) != 0; };
    const auto tie_byte = [&](std::uint32_t at) { return std::int32_t(sext8(c.u8(tie + at))); };
    const auto mark_first = [&](std::uint32_t value) { c.put8(fixture_row + fixture::first_mark, value); };
    const auto mark_second = [&](std::uint32_t value) { c.put8(fixture_row + fixture::second_mark, value); };
    const auto adjust_first = [&](std::uint32_t by) { c.put8(fixture_row + fixture::first_mark, c.u8(fixture_row + fixture::first_mark) - by); };
    const auto adjust_second = [&](std::uint32_t by) { c.put8(fixture_row + fixture::second_mark, c.u8(fixture_row + fixture::second_mark) - by); };
    const auto fixture_byte = [&](std::uint32_t at) { return std::int32_t(sext8(c.u8(fixture_row + at))); };

    if (std::int8_t(c.u8(tie + 0x33)) != 1) {
        // Not a two-sided tie: marks only, and the answer is 3.
        if (has(flag::decided)) {
            if (tie_byte(0x14) > tie_byte(0x15)) mark_first(1);
            else if (tie_byte(0x14) < tie_byte(0x15)) mark_second(1);
            else if (tie_byte(0x18) > tie_byte(0x19)) mark_first(1);
            else if (tie_byte(0x18) < tie_byte(0x19)) mark_second(1);
            else if (tie_byte(0x1a) > tie_byte(0x1b)) mark_first(1);
            else mark_second(1);
        } else if (has(flag::replay)) {
            if (tie_byte(0x14) > tie_byte(0x15)) adjust_first(3);
            else if (tie_byte(0x14) < tie_byte(0x15)) adjust_second(3);
            else if (tie_byte(0x18) > tie_byte(0x19)) adjust_first(3);
            else if (tie_byte(0x18) < tie_byte(0x19)) adjust_second(3);
            else if (tie_byte(0x1a) > tie_byte(0x1b)) adjust_first(3);
            else if (tie_byte(0x1a) < tie_byte(0x1b)) adjust_second(3);
            else { adjust_first(1); adjust_second(1); }
        }
        return 3;
    }

    // The sums of the two legs' figures: r6 the first side's (+0x14 and +0x1f), r7 the second's (+0x15 and +0x1e),
    // r8 and r9 the two single figures.
    const auto figure_14 = tie_byte(0x14), figure_15 = tie_byte(0x15);
    const auto sum_first = std::int32_t(std::int16_t(figure_14 + tie_byte(0x1f)));
    const auto sum_second = std::int32_t(std::int16_t(figure_15 + tie_byte(0x1e)));
    c.set({{9, std::uint32_t(figure_14)}, {8, std::uint32_t(figure_15)}, {6, std::uint32_t(sum_first)},
           {7, std::uint32_t(sum_second)}});

    if (has(flag::decided)) {
        // Who is ahead on the two ladders (this leg's figures, then the other's): 1 the first side.
        std::uint32_t ahead;
        if (figure_14 > figure_15) ahead = 1;
        else if (figure_14 < figure_15) ahead = 0;
        else if (tie_byte(0x18) > tie_byte(0x19)) ahead = 1;
        else if (tie_byte(0x18) < tie_byte(0x19)) ahead = 0;
        else ahead = tie_byte(0x1a) > tie_byte(0x1b) ? 1 : 0;
        c.set(6, ahead);
        std::uint32_t other;
        if (tie_byte(0x1e) > tie_byte(0x1f)) other = 1;
        else if (tie_byte(0x1e) < tie_byte(0x1f)) other = 0;
        else if (tie_byte(0x22) > tie_byte(0x23)) other = 1;
        else if (tie_byte(0x22) < tie_byte(0x23)) other = 0;
        else other = tie_byte(0x24) > tie_byte(0x25) ? 1 : 0;
        if (ahead == other) { mark_first(1); mark_second(1); return 4; }
        if (ahead != 0) { mark_second(2); return 2; }
        mark_first(2);
        return 1;
    }
    if (has(flag::replay)) {
        if (fixture_byte(0x43) > fixture_byte(0x44)) adjust_first(3);
        else if (fixture_byte(0x43) < fixture_byte(0x44)) adjust_second(3);
        else if (fixture_byte(0x47) > fixture_byte(0x48)) adjust_first(3);
        else if (fixture_byte(0x47) < fixture_byte(0x48)) adjust_second(3);
        else if (fixture_byte(0x49) > fixture_byte(0x4a)) adjust_first(3);
        else if (fixture_byte(0x49) < fixture_byte(0x4a)) adjust_second(3);
        else { adjust_first(1); adjust_second(1); }
        if (fixture_byte(fixture::first_mark) < -5) return 1;
        if (fixture_byte(fixture::second_mark) < -5) return 2;
        return 4;
    }
    if (sum_first > sum_second) return 2;
    if (sum_first < sum_second) return 1;
    if (has(flag::first_side)) { mark_first(8); return 1; }
    if (has(flag::second_side)) { mark_second(8); return 2; }
    if (has(flag::two)) {
        const auto first_leg = tie_byte(0x22);
        c.set(6, std::uint32_t(first_leg));
        if (first_leg != -1) {
            if (has(flag::away_rule)) {
                const auto second_leg = tie_byte(0x23);
                c.set(7, std::uint32_t(first_leg + figure_15));
                c.set(6, std::uint32_t(second_leg));
                if (first_leg + figure_15 == second_leg + figure_14) {
                    if (second_leg > figure_15) { mark_second(6); return 2; }
                    if (second_leg < figure_15) { mark_first(6); return 1; }
                }
            }
            const auto away_first = tie_byte(0x1e), away_second = tie_byte(0x1f);
            c.set({{8, std::uint32_t(away_first)}, {6, std::uint32_t(away_second)}});
            const auto difference_first = tie_byte(0x22) - away_first, difference_second = tie_byte(0x23) - away_second;
            c.set(7, std::uint32_t(difference_first));
            if (difference_first > difference_second) { mark_first(3); return 1; }
            if (difference_first < difference_second) { mark_second(3); return 2; }
        }
    }
    if (!has(flag::four) && !has(flag::away_rule)) {
        const auto second_figure = tie_byte(0x1f), first_figure = tie_byte(0x15);
        c.set(6, std::uint32_t(first_figure));
        if (second_figure > first_figure) { mark_second(6); return 2; }
        if (first_figure > second_figure) { mark_first(6); return 1; }
    }
    if (has(flag::either)) {
        const auto raw = c.u8(tie + 0x24);
        c.set(6, raw);
        const auto first_figure = std::int32_t(sext8(raw)), second_figure = tie_byte(0x25);
        c.set(4, std::uint32_t(first_figure));
        if (first_figure > second_figure) { mark_first(has(flag::one) ? 4 : 5); return 1; }
        if (second_figure > first_figure) { mark_second(has(flag::one) ? 4 : 5); return 2; }
        // Equal after everything the flags allow: the assertion (source line 0x11d1), then 4.
        report(c, frame, {0x10482c3c, 0x40, 0x38, 0x11d1});
    }
    return 4;
}

std::uint32_t fixture_winner(Cpu& c, std::uint32_t cup_object, std::uint32_t fixture_row) {
    const Frame frame(c, 0x1047e540, 0x490, {.first_gpr = 27});
    c.set({{27, cup_object}, {28, fixture_row}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    if (fixture_row == 0) {
        report(c, frame, {0x1047e584, 0x68, 0x40, 0x8df});
        return 0xffffffffu;
    }
    const auto flags = [&] { return sext16(c.u16(fixture_row + fixture::flags)); };
    const auto byte = [&](std::uint32_t at) { return std::int32_t(sext8(c.u8(fixture_row + at))); };
    const auto raw = [&](std::uint32_t at) { return c.u8(fixture_row + at); };
    const auto put = [&](std::uint32_t at, std::uint32_t value) { c.put8(fixture_row + at, value); };
    constexpr auto first = fixture::first_mark, second = fixture::second_mark;
    if (flags() & flag::decided) { put(first, 1); put(second, 1); }
    if (byte(0x43) > byte(0x44)) {
        if (raw(first) == 1) put(first, 2);
        if (flags() & flag::replay) put(first, raw(first) - 3);
        return 1;
    }
    if (byte(0x43) < byte(0x44)) {
        if (raw(second) == 1) put(second, 2);
        if (flags() & flag::replay) put(second, raw(second) - 3);
        return 2;
    }
    // A side that wins on a later ladder: its mark 1 becomes 2, else minus 3 on a replay, else `plain`.
    const auto win = [&](std::uint32_t mark, std::uint32_t plain) {
        const auto current = raw(mark);
        if (current == 1) put(mark, 2);
        else if (flags() & flag::replay) put(mark, current - 3);
        else put(mark, plain);
    };
    c.set(4, flags());
    if (flags() & flag::two) {
        if (byte(0x47) > byte(0x48)) { win(first, 3); return 1; }
        if (byte(0x47) < byte(0x48)) { win(second, 3); return 2; }
    }
    c.set(4, flags());
    if (flags() & flag::either) {
        const auto first_raw = raw(0x49), second_raw = raw(0x4a);
        c.set({{9, first_raw}, {10, second_raw}});
        const auto first_figure = std::int32_t(sext8(first_raw)), second_figure = std::int32_t(sext8(second_raw));
        const auto penalty_mark = (flags() & flag::one) ? 4u : 5u;
        if (first_figure > second_figure) { win(first, penalty_mark); return 1; }
        if (second_figure > first_figure) { win(second, penalty_mark); return 2; }
        // The score text of the assertion: the six figures (sign-extended) through the shared formatter.
        const auto pool = c.toc(toc::pool);
        c.set({{5, std::uint32_t(byte(0x43))}, {6, std::uint32_t(byte(0x44))}, {7, std::uint32_t(byte(0x47))},
               {8, std::uint32_t(byte(0x48))}, {9, std::uint32_t(first_figure)}, {10, std::uint32_t(second_figure)}});
        invoke(c, comp::callee::sprintf, 0x1047e800, {frame.at(0x88), pool + text::score});
        report(c, frame, {0x1047e814, 0x48, 0x38, 0x97e});
    }
    c.set(3, flags());
    if (flags() & flag::first_side) { put(first, 8); return 1; }
    if (flags() & flag::second_side) { put(second, 8); return 2; }
    if (flags() & flag::replay) {
        c.set({{6, 0}, {7, 0}});
        put(first, raw(first) - 1);
        put(second, raw(second) - 1);
        if (std::int8_t(raw(first)) < std::int8_t(raw(second))) return 1;
        if (std::int8_t(raw(first)) > std::int8_t(raw(second))) return 2;
        std::uint32_t index = 0;
        c.set(8, 0);
        while (std::int32_t(std::int16_t(index)) < c.i16(cup_object + competition::club_count)) {
            const auto table = c.u32(cup_object + cup::entrants);
            const auto row = sext16(index) * cup::entrant_row;
            c.set({{5, table}, {4, row}});
            if (c.u32(table + row) == c.u32(fixture_row + 0x1c)) c.set(6, c.u8(table + row + 4));
            const auto again = c.u32(cup_object + cup::entrants);
            c.set({{5, again}, {4, row}});
            if (c.u32(again + row) == c.u32(fixture_row + 0x20)) c.set(7, c.u8(again + row + 4));
            ++index;
            c.set(8, index);
        }
        return std::int8_t(c.r(6)) < std::int8_t(c.r(7)) ? 1u : 2u;
    }
    return 0;
}

}

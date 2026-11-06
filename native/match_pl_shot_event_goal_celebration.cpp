#include "match_pl_shot_event_detail.hpp"

#include <bit>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr std::uint32_t finish_celebration = 0x10264bb0;

int compare_celebration(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return value < bound ? -1 : value > bound ? 1 : 0;
}

void describe_team_celebration(Cpu& c, std::uint32_t actor, std::uint32_t team_return,
                               std::uint32_t refill_return, std::uint32_t commentary_return) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(actor + 0x27));
    c.call(0x101feae0, team_return);
    if (compare_celebration(c, std::int8_t(c.r(3))) == 0) return;

    advance_shot_stream(c, refill_return);
    c.set(5, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(3, c.u32(actor + 0x6c));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(28)));
    c.set(0, c.u8(c.r(3) + 0x64));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.s.wr<1>(reg::xer_ca, high < 0 && (high & 3));
    const auto shifted = std::uint32_t(high >> 2);
    c.set(4, sample - (shifted + (shifted >> 31)) * 10);
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + 14)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) report_shot_event(c, actor, 0x21be, commentary_return);
}

void report_goal_provider(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(6, c.u32(actor + 0x1a4));
    c.set(8, UINT32_MAX);
    c.set(5, c.r(3) + 0x10000);
    c.set(7, c.u32(actor + 0x1a4));
    c.set(3, c.r(6) + 0x10000);
    c.set(5, c.u32(c.r(5) - 0x1fd4));
    c.set(6, c.u32(c.r(3) - 0x1fd4));
    c.set(9, UINT32_MAX);
    c.set(3, c.u32(c.r(7) + 0x79f4));
    c.set(10, 0);
    c.set(7, c.u8(c.r(5) + 0x27));
    c.set(6, c.u8(c.r(6) + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}
}

void describe_direct_goal_celebration(Cpu& c) {
    describe_team_celebration(c, c.r(15), 0x1026370c, 0x10263734, 0x102637b0);
    c.s.pc = finish_celebration;
}

void describe_goal_celebration(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(3, c.u32(actor + 0x1a4));
    if (compare_celebration(c, c.i32(c.r(3) + 0x310c), 1400) > 0) {
        if (has_shot_pressure(c, actor, stack)) report_shot_event(c, actor, 0x2013, 0x1026381c);
        else if (has_shot_pressure(c, actor, stack, 0x370)) report_shot_event(c, actor, 0x2015, 0x10263878);
        else report_shot_event(c, actor, 0x201b, 0x102638a4);
        apply_shot_morale(c, actor, 8, 0x102638c4, 0x102638fc, 0x10263910);
        describe_team_celebration(c, actor, 0x1026391c, 0x10263944, 0x102639c0);
    } else if (compare_celebration(c, c.i8(stack + 0x124)) != 0 &&
               compare_celebration(c, c.i8(stack + 0x128)) != 0) {
        report_shot_event(c, actor, 0x201c, 0x10263a04);
        if (compare_celebration(c, std::int8_t(c.r(17)), 1) == 0) report_goal_provider(c, actor, 0x1fcd, 0x10263a50);
        else report_goal_provider(c, actor, 0x20fa, 0x10263a94);
        apply_shot_morale(c, actor, 6, 0x10263ab4, 0x10263af4, 0x10263b08);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        if (compare_celebration(c, c.i32(c.r(3) + 0x310c), 900) > 0) {
            if (has_shot_pressure(c, actor, stack, 0x370)) report_shot_event(c, actor, 0x2016, 0x10263b74);
            else report_shot_event(c, actor, 0x2018, 0x10263ba0);
            apply_shot_morale(c, actor, 6, 0x10263bc0, 0x10263c00, 0x10263c14);
        } else if (has_shot_pressure(c, actor, stack)) {
            report_shot_event(c, actor, 0x2019, 0x10263c70);
            apply_shot_morale(c, actor, 4, 0x10263c90, 0x10263cc8, 0x10263cdc);
        } else {
            if (compare_celebration(c, std::int8_t(c.r(17)), 1) != 0) {
                c.s.pc = 0x10264a94;
                return;
            }
            if (compare_celebration(c, c.i8(stack + 0x128)) != 0) {
                c.s.pc = 0x10264a50;
                return;
            }
            report_shot_event(c, actor, 0x1fcc, 0x10263d20);
            c.s.pc = 0x10263d20;
            return;
        }
    }
    c.s.pc = finish_celebration;
}

static void report_special_scorer(Cpu& c, std::uint32_t actor, std::uint32_t back) {
    c.set(3, c.toc(-0x51e4));
    c.set(5, c.u32(actor + 0x6c));
    c.set(4, c.r(3) + 0x11e);
    c.set(3, c.u32(c.r(5) + 0x2c));
    c.call(0x1077dc90, back);
    c.compare(0, std::int32_t(c.r(3)), 0);
    if (!c.r(3)) report_shot_event(c, actor, 0x216c, back + 0x30);
}

static bool is_special_goal_context(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    return compare_celebration(c, c.i8(c.r(3) + 0x7931)) != 0;
}

void describe_provider_goal(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (c.s.pc == 0x10264a50) {
        report_goal_provider(c, actor, 0x1fcd, 0x10264a90);
    } else if (compare_celebration(c, c.i8(stack + 0x128)) != 0) {
        report_goal_provider(c, actor, 0x20fa, 0x10264bb0);
    } else {
        report_shot_event(c, actor, 0x20f9, 0x10264ac8);
        if (is_special_goal_context(c, actor)) {
            advance_shot_stream(c, 0x10264af8);
            c.set(3, c.u32(c.r(31)));
            c.set(0, c.u32(c.r(28)));
            const auto sample = c.u16(c.r(3)) + c.r(0);
            const auto sign = sample >> 31;
            c.set(3, sign);
            const auto residue = std::rotl((std::rotr(sample, 3) & 0xe0000000u) - sign, 3) + sign;
            if (compare_celebration(c, std::int32_t(residue)) == 0)
                report_special_scorer(c, actor, 0x10264b3c);
        }
    }
    c.s.pc = finish_celebration;
}

static void report_scoring_pair(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(8, c.u8(c.r(9) + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(9, c.u8(c.r(9) + 0x27));
    c.call(0x10201180, back);
}

static std::int8_t goal_match_minute(Cpu& c, std::uint32_t actor) {
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, 0x2e8ba2e9);
    const auto clock = c.i16(c.r(4) + 0x7948);
    const auto high = std::int32_t((std::int64_t(clock) * 0x2e8ba2e9) >> 32);
    const auto shifted = high >> 1;
    c.s.wr<1>(reg::xer_ca, high < 0 && (high & 1));
    c.set(3, shifted < 0);
    return std::int8_t(shifted + std::int32_t(c.r(3)));
}

void choose_goal_provider_context(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (is_special_goal_context(c, actor)) {
        const auto remainder = draw_shot_modulo_three(c, 0x10263d50);
        if (compare_celebration(c, std::int32_t(remainder)) != 0)
            report_special_scorer(c, actor, 0x10263d9c);
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, actor);
    c.call(0x101feba0, 0x10263dd8);
    c.set(0, c.u8(stack + 0x1cf));
    c.set(14, c.r(3));
    if (compare_celebration(c, std::int32_t(c.r(0)), 0x35) == 0 ||
        compare_celebration(c, std::int8_t(c.r(14))) == 0) {
        c.s.pc = 0x10264180;
        return;
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, actor);
    c.call(0x101fec50, 0x10263dfc);
    const auto recipient = c.r(3);
    c.set(9, recipient);
    c.compare(0, std::int32_t(recipient), 0);
    if (!recipient) {
        c.s.pc = finish_celebration;
        return;
    }
    if (compare_celebration(c, std::int8_t(c.r(18))) != 0) {
        c.s.pc = 0x10263ed8;
        return;
    }
    c.compare_logical(0, c.r(22), 0);
    if (c.r(22)) {
        c.s.pc = 0x10263ed8;
        return;
    }
    c.s.pc = finish_celebration;
    if (compare_celebration(c, std::int8_t(c.r(14)), 1) != 0) {
        // The original sign-extends a byte before comparing with 7250.
        compare_celebration(c, c.i8(actor + 0x1b), 7250);
        return;
    }
    if (compare_celebration(c, c.i16(recipient + 0x19c), 110) == 0 ||
        compare_celebration(c, c.i16(recipient + 0x36), 6750) >= 0 ||
        compare_celebration(c, c.i16(actor + 0x36), 6750) <= 0) return;
    if (compare_celebration(c, goal_match_minute(c, actor), 15) > 0)
        report_scoring_pair(c, actor, 0x21c2, 0x10263e98);
    c.s.pc = finish_celebration;
}

static bool rates_provider_above_opponent(Cpu& c, std::uint32_t actor, std::uint32_t opponent, unsigned offset) {
    c.set(3, c.u32(actor + 0x70));
    if (compare_celebration(c, c.i8(c.r(3) + offset), 10) <= 0) return false;
    c.set(4, c.u32(opponent + 0x70));
    c.set(3, c.u32(actor + 0x70));
    c.set(4, c.u8(c.r(4) + offset));
    const auto actor_attribute = c.i8(c.r(3) + offset);
    c.set(3, std::uint32_t(std::int8_t(c.r(4))));
    return compare_celebration(c, actor_attribute - std::int32_t(c.r(3)), 3) > 0;
}

static bool is_set_piece_delivery(Cpu& c, std::uint32_t stack) {
    if (compare_celebration(c, c.i16(stack + 0x11e), 112) == 0) return true;
    c.set(3, std::uint32_t(c.i16(stack + 0x11e)));
    c.set(0, std::uint16_t(c.r(3) - 105));
    c.compare_logical(0, c.r(0), 1);
    return c.r(0) <= 1 || compare_celebration(c, c.i16(stack + 0x11e), 18) == 0;
}

static int goal_score_byte(Cpu& c, std::uint32_t actor, bool scoring_side) {
    const auto side = c.i8(actor + 0x27);
    compare_celebration(c, side);
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    const unsigned displacement = (side != 0) != scoring_side ? 0x1fc3 : 0x1f8d;
    return c.i8(c.r(3) - displacement);
}

static bool reaches_goal_side_threshold(Cpu& c, std::uint32_t actor, unsigned offset, unsigned limit) {
    const auto side = c.i8(actor + 0x27);
    const auto match = c.u32(actor + 0x1a4);
    c.set(3, match + 0xe03c + std::uint32_t(side * 0x36));
    c.set(0, c.u8(c.r(3) + offset));
    c.compare_logical(0, c.r(0), limit);
    return c.r(0) > limit;
}

static void describe_goal_score_context(Cpu& c, std::uint32_t actor) {
    if (compare_celebration(c, goal_match_minute(c, actor), 60) <= 0 ||
        compare_celebration(c, goal_score_byte(c, actor, false)) != 0 ||
        compare_celebration(c, goal_score_byte(c, actor, true), 1) != 0) return;

    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.s.wr<1>(reg::xer_ca, c.r(3) >= 0x1fc4);
    if (compare_celebration(c, std::int32_t(c.r(3) - 0x1fc4)) == 0) return;
    if (!reaches_goal_side_threshold(c, actor, 2, 4) ||
        !reaches_goal_side_threshold(c, actor, 0x16, 65) ||
        !reaches_goal_side_threshold(c, actor, 0x15, 55)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x21d7);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(8, c.u8(actor + 0x2f));
    c.set(9, c.u8(actor + 0x27));
    c.call(0x10201180, 0x1026417c);
}

void describe_goal_provider_pair(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp(), opponent = c.r(9);
    c.s.pc = finish_celebration;
    if (compare_celebration(c, std::int8_t(c.r(18))) == 0) return;
    c.compare_logical(0, c.r(22), 0);
    if (!c.r(22)) return;
    c.set(0, c.u32(actor + 0x70));
    c.compare_logical(0, c.r(0), 0);
    if (!c.r(0)) return;
    c.set(0, c.u32(opponent + 0x70));
    c.compare_logical(0, c.r(0), 0);
    if (!c.r(0)) return;
    if (compare_celebration(c, c.i16(actor + 0x36), 6750) <= 0 ||
        compare_celebration(c, c.i16(opponent + 0x36), 7250) >= 0) return;
    c.compare_logical(0, opponent, c.r(22));
    if (opponent != c.r(22)) return;
    if ((compare_celebration(c, c.u8(stack + 0x1cf), 3) == 0 ||
         compare_celebration(c, c.u8(stack + 0x1cf), 0x39) == 0) &&
        rates_provider_above_opponent(c, actor, opponent, 0x2e)) {
        report_scoring_pair(c, actor, 0x21c4, 0x10263f98);
    } else if (is_set_piece_delivery(c, stack) && rates_provider_above_opponent(c, actor, opponent, 0x36)) {
        report_scoring_pair(c, actor, 0x21c5, 0x10264028);
    } else {
        describe_goal_score_context(c, actor);
    }
    c.s.pc = finish_celebration;
}

void describe_goal_celebration_style(Cpu& c) {
    const auto actor = c.r(15);
    if (compare_celebration(c, std::int8_t(c.r(13)), 14) == 0) report_shot_event(c, actor, 0x2175, 0x10264be4);
    else if (compare_celebration(c, std::int8_t(c.r(13)), 15) == 0) report_shot_event(c, actor, 0x2176, 0x10264c1c);
    c.s.pc = 0x10264c1c;
}
}

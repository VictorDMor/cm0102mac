#include "match_pl_goal_line.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool GoalLineAction::actor_halfword_is(unsigned field, int value) {
    c.set(0, std::uint32_t(c.i16(actor.address + field)));
    return compare(c, std::int32_t(c.r(0)), value) == 0;
}

bool GoalLineAction::milestone_multiple(unsigned field, int minimum, unsigned divisor, std::int32_t magic, unsigned shift) {
    c.set(0, std::uint32_t(c.i16(actor.address + field)));
    if (compare(c, std::int32_t(c.r(0)), minimum) < 0) return false;
    const auto value = c.i16(actor.address + field);
    c.set(4, std::uint32_t(value));
    const auto high = std::int32_t((std::int64_t(value) * magic) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
    const auto quotient = high >> shift;
    c.set(3, unsigned(quotient < 0));
    c.set(0, c.r(4) - divisor * std::uint32_t(quotient + (quotient < 0)));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

bool GoalLineAction::distinct_counts(unsigned first, unsigned second) {
    c.set(3, std::uint32_t(c.i16(actor.address + first)));
    c.set(0, std::uint32_t(c.i16(actor.address + second)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0;
}

void GoalLineAction::announce_milestone(unsigned message, unsigned field, std::uint32_t back) {
    announce(message, actor.address, false, back, true, std::uint32_t(c.i16(actor.address + field)));
}

void GoalLineAction::announce_scorer_milestone() {
    c.set(0, std::uint32_t(c.i8(actor.address + 0x31)));
    bool debut_goal = false;
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        c.set(0, c.u8(actor.address + 0xc));
        debut_goal = compare_logical(c, c.r(0), 1) == 0;
    }
    if (debut_goal) {
        c.set({{3, actor.address}, {4, 0x21b6}});
        c.call(0x10289d50, 0x1026be94);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        if (compare(c, std::int32_t(c.r(0))) != 0) announce_milestone(0x21b6, 0x3e, 0x1026bec4);
        else announce_milestone(0x2010, 0x3e, 0x1026bef0);
    } else if (actor_halfword_is(0x3e, 1)) announce_milestone(0x200f, 0x3e, 0x1026bf28);
    else if (actor_halfword_is(0x40, 1)) announce_milestone(0x200b, 0x40, 0x1026bf60);
    else if (actor_halfword_is(0x44, 1)) announce_milestone(0x2012, 0x44, 0x1026bf98);
    else if (milestone_multiple(0x40, 10, 10, 0x66666667, 2)) announce_milestone(0x200b, 0x40, 0x1026bff8);
    else if (actor_halfword_is(0x3e, 50) || milestone_multiple(0x3e, 100, 100, 0x51eb851f, 5))
        announce_milestone(0x200a, 0x3e, 0x1026c064);
    else if (distinct_counts(0x42, 0x3e) &&
             (actor_halfword_is(0x42, 50) || milestone_multiple(0x42, 100, 100, 0x51eb851f, 5)))
        announce_milestone(0x200c, 0x42, 0x1026c0e0);
    else if (distinct_counts(0x44, 0x42) && milestone_multiple(0x44, 100, 100, 0x51eb851f, 5))
        announce_milestone(0x200d, 0x44, 0x1026c150);
    else if (actor_halfword_is(0x46, 1) || milestone_multiple(0x46, 5, 5, 0x66666667, 1))
        announce_milestone(0x200e, 0x46, 0x1026c1bc);
}

std::int32_t GoalLineAction::match_minute(std::int16_t ticks) {
    const auto high = std::int32_t((std::int64_t(ticks) * 0x2e8ba2e9) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
    const auto quotient = high >> 1;
    c.set(3, unsigned(quotient < 0));
    c.set(0, std::uint32_t(quotient + (quotient < 0)));
    return std::int32_t(c.r(0));
}

std::int32_t GoalLineAction::read_match_minute() {
    c.set(4, actor.match());
    return match_minute(c.i16(c.r(4) + 0x7948));
}

std::int32_t GoalLineAction::score_margin() {
    c.set(3, std::uint32_t(c.i8(frame.at(0x3d))));
    c.set(0, std::uint32_t(c.i8(frame.at(0x3e))));
    c.set(0, c.r(0) - c.r(3));
    return std::int32_t(c.r(0));
}

bool GoalLineAction::late_match(int normal, int extended) {
    if (compare(c, read_match_minute(), normal) >= 0) {
        c.set(3, actor.match());
        c.set(0, std::uint32_t(c.i16(c.r(3) + 0x794c)));
        if (compare(c, std::int32_t(c.r(0)), 0x3de) == 0) return true;
    }
    return compare(c, read_match_minute(), extended) >= 0;
}

void GoalLineAction::adjust_team(unsigned coefficient, std::uint32_t back) {
    if (opposite) c.set(0, actor.side());
    c.set(3, actor.match());
    if (!opposite) c.set(4, actor.side());
    c.set_f(1, c.f32(c.r(floats_register()) + coefficient));
    if (opposite) {
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
    }
    c.call(0x101d6860, back);
}

bool GoalLineAction::terminal_minutes(std::int32_t minute) {
    c.set(0, std::uint32_t(minute));
    if (compare(c, minute, 85) >= 0 && compare(c, minute, 90) < 0) return true;
    return compare(c, minute, 115) >= 0;
}

void GoalLineAction::finish_level_score() {
    const auto detail_register = opposite ? 20u : 17u;
    const auto message_register = opposite ? 18u : 19u;
    if (opposite) c.set(0, actor.side());
    c.set(detail_register, UINT32_MAX);
    c.set(3, actor.match());
    if (opposite) {
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
    } else c.set(4, actor.side());
    c.call(0x101eee20, resume(0x1026c488, 0x1026f0f8));
    if (opposite) c.set(0, c.r(3));
    else {
        c.set(0, actor.side());
        c.set(4, c.r(3));
    }
    c.set(3, actor.match());
    if (opposite) c.set(4, actor.side());
    c.set(message_register, std::uint32_t(std::int8_t(c.r(opposite ? 0 : 4))));
    if (!opposite) {
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
    }
    c.call(0x101eee20, resume(0x1026c49c, 0x1026f11c));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(message_register))) < 0) {
        c.set(0, actor.side());
        c.set(3, actor.match());
        c.set(0, opposite ? std::uint32_t(std::int8_t(c.r(0))) : unsigned(std::int8_t(c.r(0)) == 0));
        c.set(3, c.r(3) + c.r(0));
        c.set(detail_register, c.u8(c.r(3) + 4));
    }
    c.set(4, actor.match());
    const auto ticks = c.i16(c.r(4) + 0x7948);
    c.set(4, std::uint32_t(ticks));
    c.set(0, std::uint32_t(std::int8_t(c.r(detail_register))));
    const bool default_detail = compare(c, std::int32_t(c.r(0)), -1) == 0;
    const auto minute = std::int32_t(std::int16_t(match_minute(ticks)));
    c.set(3, std::uint32_t(minute));
    const bool late = terminal_minutes(minute);
    const auto message = default_detail ? (late ? 0x1fd8u : 0x1fd6u) : (late ? 0x1fd9u : 0x1fd7u);
    c.set(message_register, message);
    if (default_detail) {
        if (late) adjust_team(0xa0, resume(0x1026c538, 0x1026f1b4));
        else adjust_team(0x24, resume(0x1026c560, 0x1026f1cc));
    } else if (late) adjust_team(0x12c, resume(0x1026c5ac, 0x1026f208));
    else adjust_team(0x98, resume(0x1026c5d4, 0x1026f220));
    announce(c.r(message_register), actor.address, !opposite, resume(0x1026c600, 0x1026f258), false,
             std::uint32_t(std::int8_t(c.r(detail_register))));
}

bool GoalLineAction::trailing_comment() {
    if (stack_byte(0x3a) || compare(c, score_margin()) >= 0 || compare(c, score_margin(), -3) <= 0)
        return false;
    if (compare(c, read_match_minute(), 80) < 0) {
        bool qualifies = compare(c, read_match_minute(), 70) >= 0;
        if (qualifies) {
            c.set(0, c.u8(frame.at(0x3e)));
            qualifies = compare(c, std::int32_t(c.r(0)), 1) == 0;
        }
        if (!qualifies) {
            if (compare(c, read_match_minute(), 60) < 0) return false;
            c.set(0, c.u8(frame.at(0x3e)));
            if (compare(c, std::int32_t(c.r(0)), 1) != 0 || compare(c, score_margin(), -2) != 0)
                return false;
        }
    }
    const auto minute = std::int32_t(std::int16_t(read_match_minute()));
    c.set(3, std::uint32_t(minute));
    const bool late = terminal_minutes(minute);
    announce(late ? 0x1fdb : 0x1fda, actor.address, !opposite, resume(0x1026c768, 0x1026f3cc), false);
    return true;
}

bool GoalLineAction::confirm_goal() {
    bool comment_on_keeper = false;
    if (opposite) {
        announce_scorer_milestone();
        comment_on_keeper = stack_byte(0x39) && keeper_role(0x1026c1d0);
    } else {
        comment_on_keeper = keeper_role(0x1026ee38) &&
            compare_logical(c, c.r(30), actor.address) != 0 && stack_byte(0x39);
    }
    if (comment_on_keeper)
        announce(0x1fe1, c.r(30), false, resume(0x1026c208, 0x1026ee84), true,
                 std::uint32_t(c.i8(frame.at(0x39))));
    c.set(3, c.u8(frame.at(0x3e)));
    c.set(0, c.r(3) + 1);
    c.put8(frame.at(0x3e), c.r(0));
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x1c1)));
    if (compare(c, std::int32_t(c.r(0))) == 0) return false;
    announce(0x1fd2, actor.address, !opposite, resume(0x1026c250, 0x1026eed8), false);
    return true;
}

void GoalLineAction::apply_confirmed_goal_to_team() {
    if (!stack_byte(0x3a) && compare(c, score_margin(), 2) == 0 && late_match(70, 106)) {
        announce(0x1fd0, actor.address, !opposite, resume(0x1026c308, 0x1026ef9c), false);
        adjust_team(0x128, resume(0x1026c328, 0x1026efac));
        return;
    }
    if (!stack_byte(0x3a) && (compare(c, score_margin(), 1) == 0 || stack_byte(0x3c)) && late_match(80, 110)) {
        announce(0x1fd1, actor.address, !opposite, resume(0x1026c3ec, 0x1026f07c), false);
        adjust_team(0xa0, resume(0x1026c40c, 0x1026f08c));
        return;
    }
    if (!stack_byte(0x3a) && score_matches(0) && !stack_byte(0x3c) && compare(c, read_match_minute(), 70) >= 0) {
        finish_level_score();
        return;
    }
    if (trailing_comment()) return;
    if (stack_byte(0x3c) && compare(c, read_match_minute(), 80) < 0) {
        announce(0x1fdc, actor.address, !opposite, resume(0x1026c7cc, 0x1026f43c), false);
        adjust_team(0xa0, resume(0x1026c7ec, 0x1026f44c));
    } else adjust_team(0x130, resume(0x1026c810, 0x1026f460));
}
}

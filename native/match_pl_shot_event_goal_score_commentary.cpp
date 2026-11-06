#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr auto incident = 0x10266d68;

int comparison(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
    return value < limit ? -1 : value > limit ? 1 : 0;
}

int convert_minute(Cpu& c, int clock) {
    c.set(3, 0x2e8ba2e9);
    const auto high = std::int32_t((std::int64_t(clock) * 0x2e8ba2e9) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (high & 1));
    const auto shifted = high >> 1;
    c.set(3, shifted < 0);
    c.set(0, std::uint32_t(shifted + std::int32_t(c.r(3))));
    return std::int32_t(c.r(0));
}

int match_minute(Cpu& c, std::uint32_t actor) {
    c.set(4, c.u32(actor + 0x1a4));
    return convert_minute(c, c.i16(c.r(4) + 0x7948));
}

int score_delta(Cpu& c, std::uint32_t stack) {
    c.set(3, c.u8(stack + 0x4a));
    c.set(0, c.u8(stack + 0x4b));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    const auto value = std::int8_t(c.r(0)) - std::int32_t(c.r(3));
    c.set(0, std::uint32_t(value));
    return value;
}

bool is_late_goal(Cpu& c, std::uint32_t actor, int regular, int extra) {
    if (comparison(c, match_minute(c, actor), regular) >= 0) {
        c.set(3, c.u32(actor + 0x1a4));
        if (comparison(c, c.i16(c.r(3) + 0x794c), 990) == 0) return true;
    }
    return comparison(c, match_minute(c, actor), extra) >= 0;
}

void report_score_event(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(0, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{7, std::uint32_t(std::int8_t(c.r(0)))}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void change_other_side_rating(Cpu& c, std::uint32_t actor, unsigned offset, std::uint32_t back,
                              unsigned event = 0) {
    c.set(0, c.u8(actor + 0x27));
    if (event) c.set(13, event);
    c.set(4, c.toc(-0x51e0));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set_f(1, c.f32(c.r(4) + offset));
    c.set(0, c.r(0) >> 5);
    c.set(4, c.r(0));
    c.call(0x101d6860, back);
}

bool is_final_period(Cpu& c, int minute, bool compare_register_first) {
    if (compare_register_first) c.compare(0, minute, 85);
    else comparison(c, minute, 85);
    if (minute >= 85 && comparison(c, minute, 90) < 0) return true;
    return comparison(c, minute, 115) >= 0;
}

bool report_equaliser(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (comparison(c, c.i8(stack + 0x47)) != 0) return false;
    c.set(3, std::uint32_t(c.i8(stack + 0x4b)));
    if (comparison(c, c.i8(stack + 0x4a), std::int32_t(c.r(3))) != 0) {
        c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
        return false;
    }
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (comparison(c, c.i8(stack + 0x49)) != 0 ||
        comparison(c, match_minute(c, actor), 70) < 0 ||
        comparison(c, c.i8(stack + 0x48)) != 0) return false;

    c.set(0, c.u8(actor + 0x27));
    c.set(14, UINT32_MAX);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::int8_t(c.r(0)) == 0 ? 1 : 0);
    c.set(4, c.r(0));
    c.call(0x101eee20, 0x102669c8);
    c.set(0, c.r(3));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(actor + 0x27));
    c.set(13, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x101eee20, 0x102669dc);
    if (comparison(c, std::int8_t(c.r(3)), std::int32_t(c.r(13))) < 0) {
        c.set(0, c.u8(actor + 0x27));
        c.set(3, c.u32(actor + 0x1a4) + std::uint32_t(std::int8_t(c.r(0))));
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.set(14, c.u8(c.r(3) + 4));
    }
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, 0x2e8c0000);
    c.set(0, std::uint32_t(std::int8_t(c.r(14))));
    c.set(4, std::uint32_t(c.i16(c.r(4) + 0x7948)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    const bool has_team_status = std::int8_t(c.r(14)) != -1;
    const auto minute = std::int16_t(convert_minute(c, std::int16_t(c.r(4))));
    c.set(3, std::uint32_t(minute));
    if (is_final_period(c, minute, false)) {
        if (has_team_status) change_other_side_rating(c, actor, 0x12c, 0x10266af8, 0x1fd9);
        else change_other_side_rating(c, actor, 0xa0, 0x10266a7c, 0x1fd8);
    } else {
        if (has_team_status) change_other_side_rating(c, actor, 0x98, 0x10266b24, 0x1fd7);
        else change_other_side_rating(c, actor, 0x24, 0x10266aa8, 0x1fd6);
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.r(13));
    c.set(0, c.u8(actor + 0x27));
    c.set(10, std::uint32_t(std::int8_t(c.r(14))));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{7, std::uint32_t(std::int8_t(c.r(0)))}, {8, UINT32_MAX}, {9, UINT32_MAX}});
    c.call(0x10201180, 0x10266b50);
    return true;
}

bool describe_narrow_deficit(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (comparison(c, c.i8(stack + 0x47)) != 0 || comparison(c, score_delta(c, stack)) >= 0 ||
        comparison(c, score_delta(c, stack), -3) <= 0) return false;
    if (comparison(c, match_minute(c, actor), 80) < 0 &&
        (comparison(c, match_minute(c, actor), 70) < 0 || comparison(c, c.u8(stack + 0x4b), 1) != 0) &&
        (comparison(c, match_minute(c, actor), 60) < 0 || comparison(c, c.u8(stack + 0x4b), 1) != 0 ||
         comparison(c, score_delta(c, stack), -2) != 0)) return false;
    const auto minute = std::int16_t(match_minute(c, actor));
    c.set(3, std::uint32_t(minute));
    const unsigned event = is_final_period(c, minute, true) ? 0x1fdb : 0x1fda;
    c.set(4, event);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, UINT32_MAX);
    c.set(0, c.u8(actor + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{7, std::uint32_t(std::int8_t(c.r(0)))}, {10, 0}});
    c.call(0x10201180, 0x10266cb8);
    return true;
}
}

void describe_goal_score_change(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (c.s.pc == 0x10266d44) {
        change_other_side_rating(c, actor, 0x130, 0x10266d68);
        c.s.pc = incident;
        return;
    }

    c.set(3, c.u8(stack + 0x4b));
    c.set(0, c.r(3) + 1);
    c.put8(stack + 0x4b, c.r(0));
    c.set(3, c.u32(actor + 0x1a4));
    if (comparison(c, c.i8(c.r(3) + 0x1c1)) != 0) {
        report_score_event(c, actor, 0x1fd2, 0x1026677c);
    } else if (comparison(c, c.i8(stack + 0x47)) == 0 && comparison(c, score_delta(c, stack), 2) == 0 &&
               is_late_goal(c, actor, 70, 106)) {
        report_score_event(c, actor, 0x1fd0, 0x10266834);
        change_other_side_rating(c, actor, 0x128, 0x10266858);
    } else if (comparison(c, c.i8(stack + 0x47)) == 0 &&
               (comparison(c, score_delta(c, stack), 1) == 0 || comparison(c, c.i8(stack + 0x49)) != 0) &&
               is_late_goal(c, actor, 80, 110)) {
        report_score_event(c, actor, 0x1fd1, 0x1026691c);
        change_other_side_rating(c, actor, 0xa0, 0x10266940);
    } else if (!report_equaliser(c, actor, stack) && !describe_narrow_deficit(c, actor, stack) &&
               comparison(c, c.i8(stack + 0x49)) != 0 && comparison(c, match_minute(c, actor), 80) < 0) {
        report_score_event(c, actor, 0x1fdc, 0x10266d1c);
        change_other_side_rating(c, actor, 0xa0, 0x10266d40);
    }
    c.s.pc = incident;
}
}

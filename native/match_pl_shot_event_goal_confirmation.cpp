#include "match_pl_shot_event_detail.hpp"

#include <array>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

struct ConfirmationMessage {
    unsigned code;
    std::uint32_t query_return, comment_return;
    bool opponent, detail;
};

static constexpr std::array<ConfirmationMessage, 13> confirmations{{
    {0x21a3, 0x102671d0, 0x10267200, false, false},
    {0x21a4, 0x10267218, 0x10267248, false, false},
    {0x21a5, 0x10267260, 0x10267290, false, false},
    {0x21a6, 0x102672b8, 0x102672f8, true, false},
    {0x21a7, 0x10267320, 0x10267360, true, false},
    {0x21a8, 0x10267388, 0x102673c8, true, false},
    {0x21a9, 0x102673f0, 0x10267430, true, false},
    {0x21aa, 0x10267448, 0x10267478, false, false},
    {0x21ab, 0x10267490, 0x102674c0, false, true},
    {0x21ac, 0x102674e8, 0x10267528, true, true},
    {0x21ad, 0x10267540, 0x10267570, false, true},
    {0x21ae, 0x10267598, 0x102675d8, true, true},
    {0x21af, 0x10267600, 0x10267640, true, true},
}};

static bool test_confirmation_message(Cpu& c, std::uint32_t actor, ConfirmationMessage msg) {
    const auto detail = c.sp() + 0x54;
    if (msg.opponent) {
        c.set(0, c.u8(actor + 0x27));
        c.set(msg.detail ? 6 : 5, msg.detail ? detail : msg.code);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(msg.detail ? 5 : 6, msg.detail ? msg.code : 0);
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        if (msg.detail) c.set(6, detail);
        else if (msg.code != 0x21a3) c.set(5, msg.code);
        c.set(4, c.u8(actor + 0x27));
        if (msg.detail) c.set(5, msg.code);
        else if (msg.code != 0x21a3) c.set(6, 0);
    }
    c.call(0x101fdc10, msg.query_return);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static void emit_confirmation_message(Cpu& c, std::uint32_t actor, ConfirmationMessage msg) {
    if (msg.opponent) {
        c.set(0, c.u8(actor + 0x27));
        c.set(4, msg.code);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(6, UINT32_MAX);
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.set(msg.detail ? 10 : 5, c.u32(msg.detail ? c.sp() + 0x54 : actor + 0x1a4));
        const bool away = c.r(0) == 0;
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(0, away);
        if (msg.detail) c.set(5, c.u32(actor + 0x1a4));
        c.set({{8, UINT32_MAX}, {7, c.r(0)}, {9, UINT32_MAX}});
        if (!msg.detail) c.set(10, 0);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, msg.code);
        if (msg.detail) c.set(10, c.u32(c.sp() + 0x54));
        else c.set(7, c.u8(actor + 0x27));
        c.set(6, UINT32_MAX);
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(8, UINT32_MAX);
        if (msg.detail) c.set(7, c.u8(actor + 0x27));
        if (!msg.detail) c.set(5, c.u32(actor + 0x1a4));
        c.set(9, UINT32_MAX);
        if (msg.detail) c.set(5, c.u32(actor + 0x1a4));
        else c.set(10, 0);
    }
    c.call(0x10201180, msg.comment_return);
}

void report_goal_confirmation(Cpu& c) {
    const auto actor = c.r(15);
    for (const auto msg : confirmations) {
        if (!test_confirmation_message(c, actor, msg)) continue;
        emit_confirmation_message(c, actor, msg);
        break;
    }
    c.s.pc = 0x10267640;
}

static std::uint32_t goal_team_record(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u8(actor + 0x27));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + ((c.cr(0) & 2) ? 0x1c : 0x20)));
    return c.r(3);
}

static bool team_has_goal_reaction(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u32(goal_team_record(c, actor) + 0x2c));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return false;
    c.set(3, c.u32(goal_team_record(c, actor) + 0x2c));
    c.set(0, c.u8(c.r(3) + 0x119));
    c.compare(0, std::int32_t(c.r(0)), 8);
    return (c.cr(0) & 2) != 0;
}

static void report_goal_reaction(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

static std::int32_t shot_high_product(std::uint32_t sample, std::int32_t multiplier) {
    return std::int32_t((std::int64_t(std::int32_t(sample)) * multiplier) >> 32);
}

static std::uint32_t shift_shot_half(Cpu& c, std::int32_t value) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & 1));
    return std::uint32_t(value >> 1);
}

static bool goal_margin_reaction(Cpu& c, std::uint32_t stack) {
    advance_shot_stream(c, 0x10267660);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(stack + 0x4a));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    const auto high = shot_high_product(sample, 0x66666667);
    c.set(4, std::uint32_t(high));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(c.i8(stack + 0x4b)));
    c.set(3, c.r(0) - c.r(3));
    const auto shifted = shift_shot_half(c, high);
    c.set(4, sample - (shifted + (shifted >> 31)) * 5);
    c.set(0, c.r(4) + 2);
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 4)) return false;

    advance_shot_stream(c, 0x102676d8);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto next = c.u16(c.r(3)) + c.r(0);
    const auto sign = next >> 31;
    c.set(3, sign);
    c.set(0, ((next & 1) ^ sign) - sign);
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) != 0;
}

void react_to_goal_margin(Cpu& c) {
    const auto actor = c.r(15);
    if (goal_margin_reaction(c, c.sp()) && team_has_goal_reaction(c, actor)) {
        report_goal_reaction(c, actor, 0x2117, 0x10267790);
        c.s.pc = 0x10267b28;
    } else {
        c.s.pc = 0x10267794;
    }
}

static bool can_report_close_goal(Cpu& c, std::uint32_t stack) {
    c.set(3, c.u8(stack + 0x4b));
    c.set(0, c.u8(stack + 0x4a));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 2) return true;
    c.set(3, c.u8(stack + 0x4b));
    c.set(0, c.u8(stack + 0x4a));
    c.set(3, std::uint32_t(std::int8_t(c.r(3)) - 1));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 2) return true;
    c.set(0, std::uint32_t(c.i8(stack + 0x49)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static std::uint32_t shot_clock_bound(Cpu& c) {
    const auto shifted = shift_shot_half(c, shot_high_product(std::uint32_t(std::int16_t(c.r(13))), 0x2e8ba2e9));
    return shifted + (shifted >> 31);
}

static void sample_close_goal_time(Cpu& c, std::uint32_t actor) {
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, 0x2e8ba2e9);
    c.set(13, std::uint32_t(c.i16(c.r(4) + 0x7948)));
    const auto scaled = shift_shot_half(c, shot_high_product(c.r(13), 0x2e8ba2e9));
    c.set(3, scaled >> 31);
    c.set(0, scaled + c.r(3));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        c.set(14, 0);
        return;
    }
    advance_shot_stream(c, 0x10267824);
    // The source clock is a signed halfword and the conversion divides
    // its magnitude by approximately eleven. Both wide guards pass.
    for (const auto limit : {65535, -65535}) {
        c.set(4, 0x2e8ba2e9);
        c.set(3, std::uint32_t(limit));
        c.set(0, shot_clock_bound(c));
        c.compare(0, std::int32_t(c.r(0)), limit);
    }
    c.set(4, 0x2e8c0000);
    c.set(5, c.u32(c.r(31)));
    c.set(0, std::uint32_t(std::int16_t(c.r(13))));
    c.set(3, 0x2e8c0000);
    c.set(4, 0x2e8ba2e9);
    c.set(6, c.u16(c.r(5)));
    c.set(4, std::uint32_t(shot_high_product(c.r(0), 0x2e8ba2e9)));
    c.set(5, c.u32(c.r(28)));
    c.set(0, std::uint32_t(std::int16_t(c.r(13))));
    c.set(3, 0x2e8ba2e9);
    c.set(6, c.r(6) + c.r(5));
    const auto first_shifted = shift_shot_half(c, std::int32_t(c.r(4)));
    const auto divisor = first_shifted + (first_shifted >> 31);
    c.set(5, divisor);
    const auto divided = std::uint32_t(divide_signed<4>(c.r(6), divisor));
    c.set(4, divided * divisor);
    const auto second_shifted = shift_shot_half(c, shot_high_product(c.r(0), 0x2e8ba2e9));
    c.set(3, second_shifted >> 31);
    c.set(0, second_shifted + c.r(3));
    c.compare(0, std::int32_t(c.r(0)), 0);
    const auto remainder = c.r(6) - c.r(4);
    c.set(14, remainder);
    c.set(0, (c.cr(0) & 4) ? remainder : 0u - remainder);
    c.set(14, c.r(0));
}

void react_to_close_goal(Cpu& c) {
    const auto actor = c.r(15);
    if (can_report_close_goal(c, c.sp())) {
        sample_close_goal_time(c, actor);
        draw_shot_modulo_fifty(c, 0x10267a5c);
        c.compare(0, std::int32_t(c.r(14)), std::int32_t(c.r(0)));
        if ((c.cr(0) & 4) && team_has_goal_reaction(c, actor))
            report_goal_reaction(c, actor, 0x2116, 0x10267b28);
    }
    c.s.pc = 0x10267b28;
}

void reset_after_goal(Cpu& c) {
    const auto actor = c.r(15);
    c.set(0, c.u8(actor + 0x27));
    c.set(5, 0);
    c.set(6, c.u32(actor + 0x1a4));
    c.set(3, 8);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, 0);
    c.set(4, c.r(4) == 0);
    c.put8(c.r(6) + 0x7924, c.r(4));
    c.set(4, c.u32(actor + 0x1a4) + 0x10000);
    c.put32(c.r(4) - 0x2000, c.r(5));
    c.set(4, c.u32(actor + 0x1a4));
    c.put8(c.r(4) + 0x7928, c.r(3));
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x1e3, c.r(0));
    c.s.pc = 0x1026a190;
}
}

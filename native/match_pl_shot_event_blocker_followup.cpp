#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static void record_shot_value(Cpu& c, std::int32_t value, std::int32_t against = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
}

static bool shot_value_equals(Cpu& c, std::int32_t value, std::int32_t against = 0) {
    record_shot_value(c, value, against);
    return (c.cr(0) & 2) != 0;
}

static void emit_restart_message(Cpu& c, std::uint32_t actor, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.r(17));
    c.set(0, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

static void emit_blocker_message(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(7, c.u8(c.r(22) + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(c.r(22) + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

static void draw_followup_quarter(Cpu& c) {
    advance_shot_stream(c, 0x10258bfc);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    record_shot_value(c, std::int32_t(((sample + sign * 3) & 3) - sign * 3));
}

static void draw_followup_twentieth(Cpu& c) {
    advance_shot_stream(c, 0x10258dbc);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x66660000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x66666667);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 7));
    const auto quotient = std::uint32_t(high >> 3);
    c.set(3, quotient >> 31);
    c.set(0, sample - (quotient + c.r(3)) * 20);
}

static std::uint32_t opposing_team_record(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u8(c.r(22) + 0x27));
    c.set(3, c.u32(actor + 0x1a4));
    record_shot_value(c, std::int8_t(c.r(0)) == 0);
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + ((c.cr(0) & 2) ? 0x1c : 0x20)));
    return c.r(3);
}

static void maybe_emit_opposition_comment(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u32(opposing_team_record(c, actor) + 0x2c));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return;
    c.set(3, c.u32(opposing_team_record(c, actor) + 0x2c));
    if (!shot_value_equals(c, c.u8(c.r(3) + 0x119), 8)) return;

    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x20f5);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(6, 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{7, 0}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, 0x10258ea0);
}

static void report_blocker_severity(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    draw_followup_quarter(c);
    if (c.cr(0) & 2) {
        const auto bound = c.i8(stack + 0x4d);
        c.set(14, std::uint32_t(bound));
        draw_narrow_shot_sample(c, bound, 13, 0x10258c60);
        draw_followup_twentieth(c);
        c.compare(0, std::int32_t(c.r(13)), std::int32_t(c.r(0)));
        if (c.cr(0) & 4) maybe_emit_opposition_comment(c, actor);
        emit_blocker_message(c, actor, 0x2109, 0x10258ec8);
    }
    emit_blocker_message(c, actor, c.r(16), 0x10258ef0);

    bool record = shot_value_equals(c, c.u8(stack + 0x40), 6);
    if (!record) {
        c.set(3, c.u8(stack + 0x40));
        c.set(0, std::uint8_t(c.r(3) - 3));
        c.compare_logical(0, c.r(0), 1);
        record = !(c.cr(0) & 4) || shot_value_equals(c, c.u8(stack + 0x40), 5);
    }
    if (record) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set({{4, c.r(22)}, {5, 0}});
        c.call(0x101f1530, 0x10258f2c);
    }
}

void publish_blocker_followup(Cpu& c) {
    const auto actor = c.r(15);
    c.set(4, c.u8(actor + 0x105));
    c.set(0, std::uint32_t(std::int16_t(c.r(17))));
    c.set(3, c.u32(actor + 0x1a4));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    c.set(0, 0);
    c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(actor + 0x106));
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x791e, c.r(4));
    c.set(4, c.u8(actor + 0x27));
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x7924, c.r(4));
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
    if (c.cr(0) & 2) emit_restart_message(c, actor, 0x10258bd0);
    record_shot_value(c, std::int16_t(c.r(16)), -1);
    if (c.cr(0) & 4) report_blocker_severity(c, actor, c.sp());
    c.s.pc = 0x10258f2c;
}

static void emit_recipient_message(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(0, c.u8(actor + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(c.r(13) + 0x2f));
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.call(0x10201180, back);
}

static bool can_describe_restart(Cpu& c, std::uint32_t actor, std::uint32_t back) {
    c.set(3, actor);
    call_shot_owner(c, 0x10272150, back);
    return !shot_value_equals(c, std::int8_t(c.r(3)));
}

static void describe_restart_position(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (!shot_value_equals(c, std::int16_t(c.r(17)), 0x1f79) ||
        !shot_value_equals(c, c.i8(stack + 0x41))) return;
    if (shot_value_equals(c, c.u8(stack + 0x40), 7)) {
        if (can_describe_restart(c, actor, 0x1025901c) && !in_shot_goal_area(c, actor))
            emit_recipient_message(c, actor, 0x21e1, 0x1025910c);
        return;
    }
    c.set(4, c.u8(actor + 0x105));
    c.set(3, actor);
    call_shot_owner(c, 0x102720a0, 0x1025911c);
    if (!shot_value_equals(c, std::int8_t(c.r(3))) && can_describe_restart(c, actor, 0x1025912c))
        emit_recipient_message(c, actor, 0x21df, 0x10259160);
    else if (can_describe_restart(c, actor, 0x1025916c))
        emit_recipient_message(c, actor, 0x21e0, 0x102591a0);
}

void describe_blocker_restart(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (!shot_value_equals(c, std::int16_t(c.r(17)), 0x1f7a)) {
        c.set(4, c.u32(actor + 0x1a4));
        c.set(6, 0);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(5, c.u8(c.r(4) + 0x7928));
        c.set(4, c.u8(actor + 0x27));
        c.call(0x101f3a30, 0x10258f50);
        c.set(13, c.r(3));
        c.compare(0, std::int32_t(c.r(13)), 0);
        if (c.cr(0) & 2) {
            emit_restart_message(c, actor, 0x10258f84);
        } else {
            if (shot_value_equals(c, c.u8(stack + 0x40), 7))
                emit_recipient_message(c, actor, 0x21e2, 0x10258fc0);
            else emit_restart_message(c, actor, 0x10258ff0);
            describe_restart_position(c, actor, stack);
        }
    }
    c.s.pc = 0x102591a0;
}

static void emit_contact_warning(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x20dd);
    c.set(7, c.u8(actor + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(actor + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x102591e8);
}

static void emit_opposing_side_comment(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back) {
    c.set(0, c.u8(actor + 0x27));
    c.set(4, code);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, UINT32_MAX);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) == 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{8, UINT32_MAX}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

static bool draw_zero_parity(Cpu& c, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    return shot_value_equals(c, std::int32_t(((sample & 1) ^ sign) - sign));
}

static bool draw_zero_ninth(Cpu& c) {
    advance_shot_stream(c, 0x102592d0);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x38e40000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x38e38e39);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x38e38e39) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
    const auto quotient = std::uint32_t(high >> 1);
    c.set(3, quotient >> 31);
    return shot_value_equals(c, std::int32_t(sample - (quotient + c.r(3)) * 9));
}

static bool can_comment_on_severity(Cpu& c, std::uint32_t stack) {
    c.set(3, c.u8(stack + 0x40));
    c.set(0, std::uint8_t(c.r(3) - 3));
    c.compare_logical(0, c.r(0), 3);
    return !(c.cr(0) & 4) || shot_value_equals(c, std::int16_t(c.r(17)), 0x1f7a);
}

static void describe_blocker_contact(Cpu& c, std::uint32_t actor, std::uint32_t blocker, std::uint32_t stack) {
    if (!shot_value_equals(c, c.i8(stack + 0x41))) {
        emit_contact_warning(c, actor);
        if (shot_value_equals(c, c.u8(stack + 0x40), 4) || shot_value_equals(c, c.u8(stack + 0x40), 5)) {
            c.set(0, 1);
            c.put8(blocker + 0x16, 1);
        }
        if (can_comment_on_severity(c, stack) && draw_zero_parity(c, 0x10259248))
            emit_opposing_side_comment(c, actor, 0x20d9, 0x102592ac);
    } else if (draw_zero_ninth(c)) {
        emit_opposing_side_comment(c, actor, 0x20df, 0x10259344);
        if (shot_value_equals(c, std::int32_t(draw_shot_modulo_three(c, 0x10259364))))
            emit_opposing_side_comment(c, actor, 0x20e0, 0x102593d4);
        if (can_comment_on_severity(c, stack) && draw_zero_parity(c, 0x10259414))
            emit_opposing_side_comment(c, actor, 0x20d9, 0x10259478);
    }
}

void conclude_blocker_followup(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22), stack = c.sp();
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(stack + 0x42));
    c.call(0x101ee530, 0x102591ac);
    if (shot_value_equals(c, std::int8_t(c.r(3)))) {
        describe_blocker_contact(c, actor, blocker, stack);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, blocker);
        c.set_f(1, c.f32(stack + 0x58));
        c.call(0x101d6780, 0x10259488);
        c.set({{3, blocker}, {4, 0}});
        call_shot_owner(c, 0x102884d0, 0x10259494);
    }
    c.set(3, std::uint32_t(c.i16(blocker + 0x36)));
    c.set(0, c.r(3) - 25);
    c.put16(blocker + 0x36, c.r(0));
    c.s.pc = 0x1026a190;
}
}

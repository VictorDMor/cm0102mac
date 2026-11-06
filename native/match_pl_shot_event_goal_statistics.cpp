#include "match_pl_shot_event_detail.hpp"
#include "match_pl_adapter.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool equal_signed(Cpu& c, std::int32_t value, std::int32_t limit) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
    return (c.cr(0) & 2) != 0;
}

static void notify_goal_assistant(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (!equal_signed(c, c.u8(stack + 0x1cf), 0x33)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u32(c.r(3) + 0x3110));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u32(c.r(3) + 0x3110));
    c.compare_logical(0, c.r(0), actor);
    if (c.cr(0) & 2) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u32(c.r(3) + 0x3110));
    c.set(3, c.u8(c.r(4) + 0x10));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(4) + 0x10, c.r(0));
}

static void record_goal_category(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    const auto category = c.i8(c.r(3) + 0x7927);
    unsigned field = 0;
    if (equal_signed(c, category, 3)) {
        field = 0x1fbd;
    } else if (c.cr(0) & 8) {
        if (equal_signed(c, category, -1)) field = 0x1fbc;
    } else if (equal_signed(c, category, 7)) {
        field = 0x1fbc;
    } else if ((c.cr(0) & 8) && equal_signed(c, category, 6)) {
        field = 0x1fbb;
    }
    if (!field) return;
    c.set(0, c.u8(actor + 0x27));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::uint32_t(std::int8_t(c.r(0)) * 0x36));
    c.set(3, c.r(3) + 0x10000);
    c.set(4, c.r(3) + c.r(0));
    c.set(3, c.u8(c.r(4) - field));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(4) - field, c.r(0));
}

static void record_credited_goal(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101d6980, 0x102626d0);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if (c.cr(0) & 4) {
        c.set(3, c.u32(actor + 0x1a4));
        c.call(0x101d6930, 0x102626e4);
    }
    notify_goal_assistant(c, actor, stack);
    record_goal_category(c, actor);
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101fcde0, 0x102627dc);
    if (!equal_signed(c, std::int8_t(c.r(3)), 0)) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, 1);
        c.put8(c.r(3) + 0x1c1, 1);
    }
    if (equal_signed(c, c.u8(stack + 0x1cf), 0x33)) {
        c.set(3, c.u8(actor + 0xe));
        c.set(0, c.r(3) + 1);
        c.put8(actor + 0xe, c.r(0));
    }
}

static void describe_non_scoring_event(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u8(actor + 0x27));
    c.set(4, 0x2000);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(8, UINT32_MAX);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(9, UINT32_MAX);
    c.set(0, c.r(0) * 0x36);
    c.set(3, c.r(3) + 0x10000);
    c.set(10, 0);
    c.set(5, c.r(3) + c.r(0));
    c.set(3, c.u8(c.r(5) - 0x1fc4));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(5) - 0x1fc4, c.r(0));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, c.u8(actor + 0x27));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x1026286c);
    if (equal_signed(c, std::int8_t(c.r(17)), 3)) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, 0x212e);
        c.set(7, c.u8(actor + 0x27));
        c.set(8, UINT32_MAX);
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(9, UINT32_MAX);
        c.set(6, c.u8(actor + 0x2f));
        c.set(10, 0);
        c.set(5, c.u32(actor + 0x1a4));
        c.call(0x10201180, 0x102628a0);
    }
    c.s.pc = 0x10264c1c;
}

void begin_goal_statistics(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (equal_signed(c, std::int8_t(c.r(17)), 1)) record_credited_goal(c, actor, stack);
    if (equal_signed(c, std::int8_t(c.r(17)), 3) || equal_signed(c, std::int8_t(c.r(17)), 4)) {
        describe_non_scoring_event(c, actor);
        return;
    }
    if (equal_signed(c, c.i8(stack + 0x128), 0)) {
        c.set(3, std::uint32_t(c.i16(actor + 0x3e)));
        c.set(0, 1);
        c.put8(stack + 0x125, 1);
        c.set(0, c.r(3) + 1);
        c.put16(actor + 0x3e, c.r(0));
        for (unsigned offset = 0x40; offset <= 0x46; offset += 2) {
            c.set(3, std::uint32_t(c.i16(actor + offset)));
            c.set(0, c.r(3) + 1);
            c.put16(actor + offset, c.r(0));
        }
        c.set(3, c.u8(actor + 0xc));
        c.set(0, c.r(3) + 1);
        c.put8(actor + 0xc, c.r(0));
    }
    c.s.pc = 0x10262900;
}

static bool draw_goal_comment(Cpu& c, std::uint32_t stack) {
    if (equal_signed(c, c.u8(stack + 0x1cf), 0x33) ||
        equal_signed(c, c.u8(stack + 0x1cf), 0x3a) ||
        !equal_signed(c, c.i8(stack + 0x45), 0)) return false;
    draw_shot_modulo_fifty(c, 0x102634a8);
    if (!(c.cr(0) & 2) || !equal_signed(c, c.i8(stack + 0x46), 0)) return false;
    c.set(3, c.r(24));
    c.link(0x102634f8);
    c.s.pc = 0x10289560;
    if (!try_modern_match_pl(c.s, 0x10289560)) run_until(c.s, 0x102634f8);
    return equal_signed(c, std::int16_t(c.r(3)), 1);
}

static void report_statistics_comment(Cpu& c, std::uint32_t actor, std::uint32_t subject,
                                      unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(7, c.u8(subject + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(subject + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

void settle_goal_statistics(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (draw_goal_comment(c, stack)) {
        const bool special = equal_signed(c, c.u8(stack + 0x1cf), 0x1a);
        report_statistics_comment(c, actor, c.r(24), special ? 0x21d2 : 0x20d6,
                                  special ? 0x10263538 : 0x10263564);
    }
    report_statistics_comment(c, actor, actor, 0x1fff, 0x1026358c);
    if (equal_signed(c, c.u8(stack + 0x1cf), 0x33)) {
        const bool credited = equal_signed(c, std::int8_t(c.r(17)), 1);
        report_statistics_comment(c, actor, actor, credited ? 0x1fcf : 0x20f8,
                                  credited ? 0x102635cc : 0x102635f8);
        c.s.pc = 0x10264bb0;
        return;
    }

    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u32(c.r(4) + 0x310c));
    c.set(0, c.u32(c.r(3) + 0x3108));
    c.set(4, 0u - c.r(4));
    c.set(0, c.r(0) + c.r(4));
    c.s.pc = 0x10263614;
    c.put32(c.r(3) + 0x3108, c.r(0));
    c.s.pc = 0x10263618;
}

static void describe_disallowed_goal(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (equal_signed(c, c.i8(stack + 0x128), 0)) {
        report_statistics_comment(c, actor, actor, 0x20f9, 0x10263678);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, 0x20fa);
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
        c.call(0x10201180, 0x102636bc);
    }
    c.s.pc = 0x10264bb0;
}

void select_goal_announcement(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101d6980, 0x10263620);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if ((c.cr(0) & 4) || equal_signed(c, std::int8_t(c.r(17)), 6) ||
        equal_signed(c, std::int8_t(c.r(17)), 5)) {
        describe_disallowed_goal(c, actor, stack);
        return;
    }
    if (equal_signed(c, c.i8(stack + 0x124), 0) || !equal_signed(c, c.i8(stack + 0x128), 0)) {
        c.s.pc = 0x102637b4;
        return;
    }
    c.s.pc = 0x102636d8;
}

static void maybe_react_to_goal(Cpu& c, std::uint32_t actor) {
    c.set(0, std::uint32_t(c.i8(actor + 0x17)));
    c.set(3, c.r(0) * 20);
    const auto bound = c.r(3) + 20;
    c.s.wr<1>(reg::xer_ca, bound < c.r(3));
    c.set(16, bound);
    // The signed tendency gives a bound of [-2540, 2560].
    draw_narrow_shot_sample(c, std::int16_t(bound), 14, 0x1026297c);
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_shot_stream(c, 0x10262ae0);
    c.set(4, c.u32(c.r(31)));
    c.set(3, actor);
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(4)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(0, ((sample + (sign * 7)) & 7) - (sign * 7));
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10288f30, 0x10262b18);
    if (!equal_signed(c, std::int8_t(c.r(3)), 0)) {
        c.set({{3, actor}, {4, 0}});
        call_shot_owner(c, 0x1022ba10, 0x10262b2c);
    }
}

void record_direct_goal(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (!equal_signed(c, std::int8_t(c.r(17)), 1)) {
        c.s.pc = 0x10263464;
        return;
    }
    if (!equal_signed(c, c.i8(stack + 0x128), 0)) {
        c.s.pc = 0x10262b60;
        return;
    }
    const bool set_piece = equal_signed(c, c.u8(stack + 0x1cf), 0x33);
    c.set(3, std::uint32_t(c.i16(actor + 0x36)));
    c.set(0, c.r(3) + (set_piece ? 700 : 650));
    c.put16(actor + 0x36, c.r(0));
    if (!set_piece) maybe_react_to_goal(c, actor);
    c.set(3, c.u8(actor + 0xc));
    c.set(4, std::uint32_t(c.i16(actor + 0x36)));
    c.set(0, (c.r(3) + 5) * 1000);
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) c.set(0, std::uint32_t(c.i16(actor + 0x36)));
    else {
        c.set(3, c.u8(actor + 0xc));
        c.set(0, (c.r(3) + 5) * 1000);
    }
    c.put16(actor + 0x36, c.r(0));
    c.s.pc = 0x10262e08;
}

static void react_to_alternate_goal(Cpu& c, std::uint32_t actor) {
    c.set(0, std::uint32_t(c.i8(actor + 0x17)));
    c.set(3, c.r(0) * 20);
    const auto bound = c.r(3) + 50;
    c.s.wr<1>(reg::xer_ca, bound < c.r(3));
    c.set(16, bound);
    // The signed tendency bounds this sample to [-2510, 2590].
    draw_narrow_shot_sample(c, std::int16_t(bound), 14, 0x10262bb4);
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_shot_stream(c, 0x10262d18);
    c.set(4, c.u32(c.r(31)));
    c.set(3, actor);
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(4)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(0, ((sample + sign * 3) & 3) - sign * 3);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10288f30, 0x10262d50);
    if (!equal_signed(c, std::int8_t(c.r(3)), 0)) {
        c.set({{3, actor}, {4, 0}});
        call_shot_owner(c, 0x1022ba10, 0x10262d64);
    }
}

static void alternate_goal_rating_floor(Cpu& c, std::uint32_t actor, unsigned offset, bool compare) {
    const auto stack = c.sp();
    Fpu fp(c);
    c.set(3, c.u8(actor + 0x10));
    c.set(0, 0x43300000);
    c.put32(stack + offset, c.r(0));
    c.set_f(2, c.f64(c.r(27) + 0xf0));
    c.put32(stack + offset + 4, c.r(3));
    c.set(3, c.toc(-0x51e0));
    c.set_f(1, c.f64(stack + offset));
    c.set_f(0, c.f32(c.r(3) + 8));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(3, c.f64(c.r(27) + 0x358));
    c.set_f(2, c.f64(c.r(27) + 0xe0));
    if (compare) c.set(4, std::uint32_t(c.i16(actor + 0x36)));
    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
    c.set_f(0, fp.fadd(c.f(3), c.f(0)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(stack + offset + 8, fp.fctiwz(0, c.f(0)));
    c.set(0, c.u32(stack + offset + 12));
}

void record_alternate_goal(Cpu& c) {
    const auto actor = c.r(15);
    c.set(3, c.u8(actor + 0x10));
    c.set(0, c.r(3) + 1);
    c.put8(actor + 0x10, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor + 0x36)));
    c.set(0, c.r(3) + 275);
    c.put16(actor + 0x36, c.r(0));
    react_to_alternate_goal(c, actor);
    alternate_goal_rating_floor(c, actor, 0xfc, true);
    c.set(0, std::uint32_t(std::int16_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) c.set(0, std::uint32_t(c.i16(actor + 0x36)));
    else alternate_goal_rating_floor(c, actor, 0x10c, false);
    c.put16(actor + 0x36, c.r(0));
    c.s.pc = 0x10262e08;
}
}

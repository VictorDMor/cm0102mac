#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool blocker_reaction_needed(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(c.sp() + 0x42));
    c.call(0x101ee530, 0x1025996c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return true;
    c.set(0, std::uint32_t(c.i8(c.sp() + 0x41)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return true;
    const auto bound = c.i16(blocker + 0x32);
    c.set(14, std::uint32_t(bound));
    draw_narrow_shot_sample(c, bound, 13, 0x102599b4);
    c.compare(0, std::int32_t(c.r(13)), -10);
    return (c.cr(0) & 8) != 0;
}

static bool draw_deflection_comment(Cpu& c, std::uint32_t blocker) {
    const auto side = c.u8(blocker + 0x27);
    c.set(0, side);
    c.compare(0, side, 1);
    if (!in_goal_area_for_side(c, blocker, side == 1)) return false;
    advance_shot_stream(c, 0x10259bc4);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    c.set(0, ((sample & 1) ^ sign) - sign);
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) != 0;
}

static void report_deflection_side(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    draw_shot_modulo_fifty(c, 0x10259e40);
    const auto draw = std::int32_t(c.r(0));
    c.compare(0, draw, 1);
    if (c.cr(0) & 8) c.compare(0, draw, 0);
    const unsigned code = draw == 0 ? 0x20eb : draw == 1 ? 0x20ea : 0x20e8;
    const auto back = draw == 0 ? 0x10259ec8 : draw == 1 ? 0x10259f04 : 0x10259f40;
    c.set(0, c.u8(blocker + 0x27));
    c.set(4, code);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, UINT32_MAX);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) == 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{8, UINT32_MAX}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
    c.s.pc = 0x1025a11c;
}

static void compare_blocker_balance(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    const auto stack = c.sp();
    Fpu fp(c);
    advance_shot_stream(c, 0x1025a058);
    c.set(5, c.u32(c.r(31)));
    c.set({{4, 0x2aab0000}, {0, 0x43300000}});
    c.set(3, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(5)));
    c.set(4, 0x2aaaaaab);
    c.put32(stack + 0x104, c.r(0));
    const auto sample = c.r(5) + c.r(3);
    c.set(5, sample);
    c.set(3, c.toc(-0x51e0));
    c.set_f(0, c.f32(blocker + 0xb0));
    c.set_f(1, c.f32(c.r(3) + 0xe4));
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x2aaaaaab) >> 32);
    c.set(3, high);
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.set_f(3, fp.fadds(c.f(1), c.f(0)));
    c.set_f(0, c.f32(actor + 0xb0));
    c.set(0, (sample - (high + (high >> 31)) * 6) ^ 0x80000000u);
    c.put32(stack + 0x108, c.r(0));
    c.set_f(1, fp.fsubs(c.f64(stack + 0x104), c.f(2)));
    c.set_f(1, fp.fadds(c.f(3), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) report_blocked_shot(c, actor, 0x2164, 0x1025a0f0);
    else report_blocked_shot(c, actor, 0x1f5d, 0x1025a11c);
}

static void resolve_deflection_message(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    const auto action = std::uint32_t(c.i8(c.sp() + 0x42));
    c.set(0, action);
    c.compare_logical(0, action, 46);
    c.s.pc = 0x1025a11c;
    if (c.cr(0) & 4) return;
    c.set(3, c.toc(-0x5234));
    c.set(0, action * 4);
    c.s.pc = 0x10259f5c;
    const auto target = c.u32(c.r(3) + c.r(0));
    c.set(3, target);
    c.s.wr<4>(reg::ctr, target);
    switch (target) {
    case 0x10259f68:
        c.set(0, std::uint32_t(c.i16(c.sp() + 0x52)));
        c.set(3, blocker);
        c.set(0, c.r(0) - c.r(16));
        c.set(4, std::uint32_t(std::int16_t(c.r(0))));
        call_shot_owner(c, 0x10270320, 0x10259f7c);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        c.compare(0, std::int32_t(c.r(0)), 0);
        c.s.pc = (c.cr(0) & 2) ? 0x1026a190 : 0x1025a11c;
        return;
    case 0x10259f88: report_blocked_shot(c, actor, 0x1f61, 0x10259fb0); break;
    case 0x10259fb4: report_blocked_shot(c, actor, 0x1f62, 0x10259fdc); break;
    case 0x10259fe0: report_blocked_shot(c, actor, 0x2150, 0x1025a008); break;
    case 0x1025a00c: report_blocked_shot(c, actor, 0x1f64, 0x1025a034); break;
    case 0x1025a038: compare_blocker_balance(c, actor, blocker); break;
    case 0x1025a0f4: report_blocked_shot(c, actor, 0x1f5d, 0x1025a11c); break;
    case 0x1025a11c: break;
    default: c.s.pc = target; return;
    }
    c.s.pc = 0x1025a11c;
}

void describe_shot_deflection(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22);
    c.compare_logical(0, blocker, 0);
    if (c.cr(0) & 2) {
        c.s.pc = 0x1025a908;
        return;
    }
    if (blocker_reaction_needed(c, actor, blocker) && draw_deflection_comment(c, blocker)) {
        report_blocking_action(c, actor, c.sp(), {-0x5230, 0x10259c08, 0x10259c14, 0x10259e20});
        if (c.s.pc == 0x10259e20) report_deflection_side(c, actor, blocker);
    } else {
        resolve_deflection_message(c, actor, blocker);
    }
}
}

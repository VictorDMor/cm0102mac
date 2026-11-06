#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool pass_goal_tendency(Cpu& c, std::uint32_t keeper, unsigned field, std::uint32_t back) {
    c.set(3, c.u32(keeper + 0x6c));
    c.set(0, c.u8(c.r(3) + field));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    const auto bound = c.r(3) + 1;
    c.s.wr<1>(reg::xer_ca, bound < c.r(3));
    c.set(16, bound);
    draw_narrow_shot_sample(c, std::int32_t(bound), 14, back);
    c.compare(0, std::int32_t(c.r(14)), 0);
    return (c.cr(0) & 2) == 0;
}

static void adjust_keeper_reaction(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    // Both profile bytes constrain their bounds to [-127, 128].
    if (!pass_goal_tendency(c, keeper, 0x64, 0x10262e64) ||
        !pass_goal_tendency(c, keeper, 0x66, 0x10262fe4)) return;
    c.set(3, c.u8(keeper + 0xf));
    c.set(0, c.u8(keeper + 0xf));
    const auto bound = c.r(3) * c.r(0);
    c.set(16, bound);
    // A product of two unsigned bytes is at most 65,025.
    draw_narrow_shot_sample(c, std::int32_t(bound), 14, 0x10263160);
    c.set(0, c.u8(actor + 0xf));
    c.set(3, std::uint32_t(std::int8_t(c.r(14))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 8) {
        c.set(0, c.u8(actor + 0xf));
        c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    }
    c.set(5, c.u8(keeper + 0x103));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.set({{3, keeper}, {4, 0}});
    c.set(0, c.r(5) + c.r(0));
    c.put8(keeper + 0x103, c.r(0));
    call_shot_owner(c, 0x1022ba10, 0x102632d0);
}

static void reduce_keeper_rating(Cpu& c, std::uint32_t keeper) {
    const auto stack = c.sp();
    c.set(4, c.u8(stack + 0x1cf));
    c.set({{3, 0x33}, {0, std::uint32_t(-500)}});
    c.set(5, std::uint32_t(std::int8_t(c.r(4))));
    c.set(6, std::uint32_t(c.i16(keeper + 0x36)));
    c.set(4, c.r(5) - c.r(3));
    const auto difference = std::int32_t(~(c.r(4) | (c.r(3) - c.r(5))));
    c.s.wr<1>(reg::xer_ca, difference < 0 && (std::uint32_t(difference) & 0x7fffffff));
    c.set(3, std::uint32_t(difference >> 31));
    c.set(0, c.r(6) + (c.r(0) & ~c.r(3)));
    c.put16(keeper + 0x36, c.r(0));
    c.set(0, std::uint32_t(c.i8(stack + 0x45)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return;

    Fpu fp(c);
    c.set_f(0, c.f64(c.r(27) + 0x110));
    c.set_f(3, c.f64(c.r(27) + 0x360));
    c.set_f(2, fp.fadd(c.f(0), c.f(30)));
    c.set_f(1, c.f64(c.r(27) + 0x28));
    c.set_f(0, c.f64(c.r(27) + 0x368));
    c.set(3, std::uint32_t(c.i16(keeper + 0x36)));
    c.set_f(2, fp.fmul(c.f(3), c.f(2)));
    c.set_f(1, fp.fmul(c.f(2), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) {
        c.set_f(0, c.f64(c.r(27) + 0x110));
        c.set_f(2, c.f64(c.r(27) + 0x360));
        c.set_f(1, fp.fadd(c.f(0), c.f(30)));
        c.set_f(0, c.f64(c.r(27) + 0x28));
        c.set_f(1, fp.fmul(c.f(2), c.f(1)));
        c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    } else {
        c.set_f(0, c.f64(c.r(27) + 0x368));
    }
    c.put64(stack + 0xfc, fp.fctiwz(0, c.f(0)));
    c.set(0, c.u32(stack + 0x100) + c.r(3));
    c.put16(keeper + 0x36, c.r(0));
}

static void award_goal_assistant(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x1fd8));
    c.compare_logical(0, actor, c.r(0));
    if (!(c.cr(0) & 2)) return;
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x1fdc));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return;

    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u8(c.sp() + 0x120));
    c.set(3, c.r(3) + 0x10000);
    c.set(4, c.u32(c.r(3) - 0x1fdc));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.set(3, c.u8(c.r(4) + 0x10));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(4) + 0x10, c.r(0));
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(4, c.u32(c.r(3) - 0x1fdc));
    c.set(3, std::uint32_t(c.i16(c.r(4) + 0x36)));
    c.set(0, c.r(3) + 275);
    c.put16(c.r(4) + 0x36, c.r(0));
    if (c.cr(0) & 2) credit_shot_assistant(c, actor);
}

void account_goal_keeper(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24);
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, 0x10262e10);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 1);
    if (c.cr(0) & 2) {
        c.set(3, c.u8(keeper + 0xf));
        c.set(0, c.r(3) + 1);
        c.put8(keeper + 0xf, c.r(0));
        adjust_keeper_reaction(c, actor, keeper);
        reduce_keeper_rating(c, keeper);
    }
    award_goal_assistant(c, actor);
    c.s.pc = 0x10263464;
}
}

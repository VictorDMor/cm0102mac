#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static void draw_blocking_comment(Cpu& c, std::uint32_t stack) {
    advance_shot_stream(c, 0x10258740);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x92490000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x92492493);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(stack + 0x4d));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * std::int32_t(0x92492493)) >> 32);
    const auto corrected = std::int32_t(high + sample);
    c.s.wr<1>(reg::xer_ca, corrected < 0 && (std::uint32_t(corrected) & 3));
    const auto shifted = std::uint32_t(corrected >> 2);
    c.set(4, shifted >> 31);
    c.set(4, sample - (shifted + c.r(4)) * 7);
    c.set(0, c.r(4) + 2);
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) c.set(16, 0x210a);
}

static void classify_blocking_comment(Cpu& c, std::uint32_t stack) {
    const auto result = c.i8(stack + 0x40);
    c.set(16, UINT32_MAX);
    c.set(0, std::uint32_t(result));
    c.compare(0, result, 4);
    if (c.cr(0) & 2) {
        c.set(16, 0x1ff8);
    } else if (c.cr(0) & 4) {
        c.compare(0, result, 6);
        if (c.cr(0) & 8) c.set(16, 0x1ff6);
        else draw_blocking_comment(c, stack);
    } else {
        c.compare(0, result, 2);
        if (c.cr(0) & 2) c.set(16, 0x1ff9);
        else if (c.cr(0) & 4) c.set(16, 0x1ff7);
        else draw_blocking_comment(c, stack);
    }
}

static void report_penalty_chance(Cpu& c, std::uint32_t actor) {
    if (std::int16_t(c.r(16)) != -1) {
        c.set(0, std::uint32_t(std::int16_t(c.r(16))));
        c.compare(0, std::int32_t(c.r(0)), -1);
        return;
    }
    c.set(0, UINT32_MAX);
    c.compare(0, -1, -1);
    advance_shot_stream(c, 0x1025889c);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    const auto remainder = std::int32_t(((sample + sign * 3) & 3) - sign * 3);
    c.set(0, std::uint32_t(remainder));
    c.compare(0, remainder, 1);
    if (c.cr(0) & 4) return;
    if (c.cr(0) & 8) {
        c.compare(0, remainder, 0);
        if (c.cr(0) & 8) return;
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, remainder ? 0x20ec : 0x20e9);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, remainder ? 0x10258934 : 0x10258908);
}

static void penalize_blocking_player(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set({{0, 5}, {17, 0x1f7a}});
    c.put8(c.r(3) + 0x7928, c.r(0));
    c.set(3, c.u32(actor + 0x1a4));
    c.put32(c.r(3) + 0x3110, actor);
    c.set(3, std::uint32_t(c.i16(blocker + 0x36)));
    c.set(0, c.r(3) - 500);
    c.put16(blocker + 0x36, c.r(0));
    c.set(3, c.u32(blocker + 0x6c));
    const auto tendency = c.i8(c.r(3) + 0x66);
    c.set(14, std::uint32_t(tendency));
    draw_narrow_shot_sample(c, tendency, 13, 0x10258990);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_shot_stream(c, 0x10258af4);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(0, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(5, c.u16(c.r(3)));
    c.set(3, blocker);
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 3));
    const auto shifted = std::uint32_t(high >> 2);
    c.set(4, shifted >> 31);
    c.set(0, (shifted + c.r(4)) * 10 - sample);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10288f30, 0x10258b3c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        c.set({{3, blocker}, {4, 0}});
        call_shot_owner(c, 0x1022ba10, 0x10258b50);
    }
}

void react_to_shot_blocker(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    classify_blocking_comment(c, stack);
    c.set(0, c.u8(stack + 0x40));
    c.compare(0, std::int32_t(c.r(0)), 7);
    if (c.cr(0) & 2) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set({{0, 7}, {17, 0x1f79}});
        c.put8(c.r(3) + 0x7928, c.r(0));
    } else if (in_shot_goal_area(c, actor)) {
        report_penalty_chance(c, actor);
        penalize_blocking_player(c, actor, c.r(22));
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set({{0, 6}, {17, 0x1f79}});
        c.put8(c.r(3) + 0x7928, c.r(0));
    }
    c.s.pc = 0x10258b64;
}
}

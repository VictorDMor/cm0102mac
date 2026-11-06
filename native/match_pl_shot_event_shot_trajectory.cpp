#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool matches_shot_value(Cpu& c, std::int32_t value, std::int32_t against) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
    return (c.cr(0) & 2) != 0;
}

static bool can_comment_on_shot(Cpu& c, std::uint32_t stack) {
    return !matches_shot_value(c, c.u8(stack + 0x1cf), 0x33) &&
           !matches_shot_value(c, c.u8(stack + 0x1cf), 0x35) &&
           !matches_shot_value(c, std::int8_t(c.r(17)), 4) &&
           !matches_shot_value(c, c.u8(stack + 0x1cf), 3) &&
           !matches_shot_value(c, c.u8(stack + 0x1cf), 0x39);
}

static bool previously_attacked(Cpu& c, std::uint32_t actor) {
    c.compare_logical(0, c.r(22), 0);
    if (!(c.cr(0) & 2) || !in_shot_goal_area(c, actor) ||
        !matches_shot_value(c, std::int8_t(c.r(18)), 0)) return false;
    const auto last = c.i16(actor + 0x19c);
    c.set(3, std::uint32_t(last));
    bool active = false;
    for (const auto action : {0xd, 0xc, 0x15, 0x2e, 0x3b, 0x25}) {
        if (matches_shot_value(c, last, action)) {
            active = true;
            break;
        }
    }
    return !matches_shot_value(c, active, 0);
}

static void report_attacking_shot(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x1f43);
    c.set(7, c.u8(actor + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(actor + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x102597cc);
}

static bool choose_shot_range_comment(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    advance_shot_stream(c, 0x102597f0);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x66660000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x66666667);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 3));
    const auto shifted = std::uint32_t(high >> 2);
    c.set(3, shifted >> 31);
    if (!matches_shot_value(c, std::int32_t(sample - (shifted + c.r(3)) * 10), 0)) return false;

    const auto distance = c.i8(stack + 0x127);
    c.set(0, std::uint32_t(distance));
    c.compare(0, distance, 20);
    if (c.cr(0) & 8) return false;
    const auto live_distance = c.i8(stack + 0x127);
    c.set(0, std::uint32_t(live_distance));
    c.compare(0, live_distance, 35);
    if ((c.cr(0) & 4) || in_shot_goal_area(c, actor)) return false;
    const auto column = c.i8(actor + 0x105);
    c.set(0, std::uint32_t(column));
    c.compare(0, column, 6);
    if (!(c.cr(0) & 8)) return false;
    const auto live_column = c.i8(actor + 0x105);
    c.set(0, std::uint32_t(live_column));
    c.compare(0, live_column, 2);
    return (c.cr(0) & 4) != 0;
}

static void report_shot_range(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x2161);
    c.set(0, c.u8(stack + 0x127));
    c.set(8, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(9, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(10, std::uint32_t(std::int8_t(c.r(0))));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x10259954);
}

void route_shot_after_block(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22), stack = c.sp();
    c.compare_logical(0, blocker, 0);
    if (!(c.cr(0) & 2)) {
        c.set(0, std::uint32_t(c.i16(stack + 0x52)));
        c.set(3, std::uint32_t(std::int16_t(c.r(16))));
        c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
        if (!(c.cr(0) & 4)) {
            c.s.pc = 0x10259958;
            return;
        }
    }

    c.compare_logical(0, blocker, 0);
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u8(blocker + 0x2c));
        c.set(0, c.u8(stack + 0x43));
        c.set(0, c.r(3) + c.r(0));
        c.put8(blocker + 0x2c, c.r(0));
    } else if (can_comment_on_shot(c, stack)) {
        if (previously_attacked(c, actor)) report_attacking_shot(c, actor);
        else if (choose_shot_range_comment(c, actor, stack)) report_shot_range(c, actor, stack);
    }
    c.s.pc = 0x1025a968;
}
}

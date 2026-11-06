#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool out_of_play_equals(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
    return (c.cr(0) & 2) != 0;
}

static void describe_out_of_play_shot(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (!out_of_play_equals(c, c.i8(stack + 0x124)) && out_of_play_equals(c, c.i8(stack + 0x128))) {
        report_shot_event(c, actor, 0x2020, 0x1025d2ac);
        return;
    }
    if (!out_of_play_equals(c, c.i8(stack + 0x128))) return;
    out_of_play_equals(c, c.i8(stack + 0x127), 25);
    if (c.cr(0) & 8) return;
    out_of_play_equals(c, c.i16(actor + 0x3a), 50);
    if (!(c.cr(0) & 8)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x1f9e);
    c.set(0, c.u8(stack + 0x127));
    c.set(8, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(9, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(10, std::uint32_t(std::int8_t(c.r(0))));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x1025d304);
}

static bool keeper_is_available(Cpu& c, std::uint32_t keeper, std::uint32_t back) {
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, back);
    return out_of_play_equals(c, std::int16_t(c.r(3)), 1);
}

static void describe_keeper_delay(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack) {
    if (out_of_play_equals(c, c.i8(stack + 0x45)) || !out_of_play_equals(c, c.i8(stack + 0x46)) ||
        !keeper_is_available(c, keeper, 0x1025d324)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x20d5);
    c.set(7, c.u8(keeper + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(keeper + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x1025d358);
}

static void replace_ball(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    const bool low = out_of_play_equals(c, std::int8_t(c.r(13)), 12);
    c.set(0, 0x33);
    c.set(3, c.u8(stack + 0x1cf));
    c.set(4, 0x33);
    const bool other_side = !out_of_play_equals(c, c.i8(stack + 0x128));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.set(3, c.r(4) - c.r(0));
    c.set(0, (c.r(0) - c.r(4)) | c.r(3));
    c.set(5, c.r(0) >> 31);
    if (other_side) {
        c.set(0, c.u8(actor + 0x27));
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(7, c.r(0));
    } else c.set(7, c.u8(actor + 0x27));
    c.set(4, c.u32(actor + 0x1a4));
    c.set(9, c.r(5));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, 2);
    c.set(5, c.u8(c.r(4) + 0x791e));
    c.set(8, 1);
    c.set(4, c.u8(c.r(3) + 0x791d));
    c.set(10, 0);
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101f1100, low ? 0x1025d3d4 : 0x1025d448);
}

static void reset_keeper_after_miss(Cpu& c, std::uint32_t keeper) {
    c.compare_logical(0, keeper, 0);
    if ((c.cr(0) & 2) || !keeper_is_available(c, keeper, 0x1025d458)) return;
    advance_shot_stream(c, 0x1025d484);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x55550000);
    c.set(0, c.u32(c.r(28)));
    c.set(4, 0x55555556);
    c.set(5, c.u16(c.r(3)));
    c.set(3, keeper);
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x55555556) >> 32);
    c.set(0, sample - (high + (high >> 31)) * 3);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10285ed0, 0x1025d4c4);
}

void finish_shot_out_of_play(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    describe_out_of_play_shot(c, actor, stack);
    describe_keeper_delay(c, actor, keeper, stack);
    replace_ball(c, actor, stack);
    reset_keeper_after_miss(c, keeper);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, 1);
    c.put8(c.r(3) + 0x1e6, 1);
    c.s.pc = 0x1026a190;
}
}

#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool shot_restart_value(Cpu& c, std::int32_t value, std::int32_t against) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
    return (c.cr(0) & 2) != 0;
}

static void choose_restart_actor(Cpu& c, std::uint32_t actor) {
    c.set(4, c.u8(actor + 0x27));
    c.set(0, 11);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, actor);
    c.set(5, std::uint32_t(std::int8_t(c.r(4))));
    c.set(4, c.u8(actor + 0x27));
    c.set(6, c.r(5) == 0 ? 32 : 0);
    c.set(5, c.u8(actor + 0x106));
    c.set(6, 0u - (c.r(6) >> 5));
    c.set(0, c.r(0) & c.r(6));
    c.set(6, c.r(0));
    c.call(0x101f5230, 0x1025ada4);
    c.set(23, c.r(3));
}

static bool draw_restart_rejection(Cpu& c) {
    advance_shot_stream(c, 0x1025add0);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x10620000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x10624dd3);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x10624dd3) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 31));
    const auto shifted = std::uint32_t(high >> 5);
    c.set(3, shifted >> 31);
    return !shot_restart_value(c, std::int32_t(sample - (shifted + c.r(3)) * 500), 0);
}

static void restart_without_goal(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    const auto stack = c.sp();
    c.set({{3, UINT32_MAX}, {0, UINT32_MAX}});
    c.put32(stack + 0x38, c.r(3));
    c.set({{5, recipient}, {4, UINT32_MAX}, {6, 0}});
    c.put32(stack + 0x3c, c.r(0));
    c.set({{7, 0}, {8, 8}});
    c.set(3, c.u32(actor + 0x1a4));
    c.set(9, c.u8(recipient + 0x105));
    c.set(10, c.u8(recipient + 0x106));
    c.call(0x101d68f0, 0x1025ae40);
    c.set(17, 2);
}

static void announce_restart_actor(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x1f78);
    c.set(6, c.u8(recipient + 0x2f));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x1025ae78);
}

static void publish_shot_restart(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(4, c.u8(actor + 0x27));
    c.set(3, 7);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, 0);
    c.set(4, std::uint32_t(std::int8_t(c.r(4)) * 0x36));
    c.set(5, c.r(5) + 0x10000 + c.r(4));
    c.set(4, c.u8(c.r(5) - 0x1fb6) + 1);
    c.put8(c.r(5) - 0x1fb6, c.r(4));
    c.set(4, c.u8(recipient + 0x13) + 1);
    c.put8(recipient + 0x13, c.r(4));
    c.set(5, c.u8(recipient + 0x105));
    c.set(4, c.u32(actor + 0x1a4));
    c.put8(c.r(4) + 0x791d, c.r(5));
    c.set(5, c.u8(recipient + 0x106));
    c.set(4, c.u32(actor + 0x1a4));
    c.put8(c.r(4) + 0x791e, c.r(5));
    c.set(4, c.u32(actor + 0x1a4));
    c.put8(c.r(4) + 0x7928, c.r(3));
    c.set(3, c.u8(actor + 0x27));
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, std::int8_t(c.r(3)) == 0);
    c.put8(c.r(4) + 0x7924, c.r(3));
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
}

void route_shot_restart(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(0, 0x6d);
    c.put16(actor + 0x19c, c.r(0));
    if (!shot_restart_value(c, c.u8(stack + 0x1cf), 0x33) &&
        !shot_restart_value(c, c.u8(stack + 0x1cf), 0x35) &&
        shot_restart_value(c, std::int8_t(c.r(17)), 1))
        choose_restart_actor(c, actor);

    c.compare_logical(0, c.r(23), 0);
    if (!(c.cr(0) & 2) && draw_restart_rejection(c)) {
        restart_without_goal(c, actor, c.r(23));
        c.s.pc = 0x1025aef8;
        return;
    }
    c.compare_logical(0, c.r(23), 0);
    if (!(c.cr(0) & 2)) {
        announce_restart_actor(c, actor, c.r(23));
        publish_shot_restart(c, actor, c.r(23));
        c.s.pc = 0x1026a190;
    } else c.s.pc = 0x1025aef8;
}
}

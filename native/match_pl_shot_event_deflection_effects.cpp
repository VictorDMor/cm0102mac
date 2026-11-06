#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static void compare_deflection(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
}

static void clamp_blocker_power(Cpu& c, std::uint32_t blocker) {
    c.set(0, std::uint32_t(c.i16(c.sp() + 0x52)));
    c.set(3, c.r(0) - c.r(16));
    c.set(0, c.r(3) - 200);
    c.put16(blocker + 0x1a0, c.r(0));
    const auto first = c.i16(blocker + 0x1a0);
    compare_deflection(c, first, 200);
    if (c.cr(0) & 4) {
        c.set(0, 200);
        c.put16(blocker + 0x1a0, 200);
        return;
    }
    const auto live = c.i16(blocker + 0x1a0);
    compare_deflection(c, live, -200);
    if (c.cr(0) & 8) {
        c.set(0, -200);
        c.put16(blocker + 0x1a0, -200);
    }
}

static bool should_move_blocker(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    const auto action = c.i8(c.sp() + 0x42);
    c.set({{13, 0}, {4, 100}});
    compare_deflection(c, action, 0x21);
    if (c.cr(0) & 2) c.set(4, 250);
    else {
        const int boundary = (c.cr(0) & 8) ? 0x13 : 0x2d;
        c.compare(0, action, boundary);
        if (c.cr(0) & 2) c.set(4, boundary == 0x13 ? 400 : 250);
    }
    c.set(3, std::uint32_t(c.i16(blocker + 0x1a0)));
    c.set(0, c.r(4));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8)) return true;
    c.set(3, c.u32(actor + 0x1a4));
    compare_deflection(c, c.u8(c.r(3) + 0x791f), 4);
    if (c.cr(0) & 2) return true;
    c.set(3, c.u32(actor + 0x1a4));
    compare_deflection(c, c.i8(c.r(3) + 0x791f), -1);
    return (c.cr(0) & 2) != 0;
}

void begin_deflection_effects(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22);
    c.set(3, blocker);
    call_shot_owner(c, 0x1022e3a0, 0x1025a124);
    clamp_blocker_power(c, blocker);
    if (should_move_blocker(c, actor, blocker)) {
        c.s.pc = 0x1025a650;
        return;
    }
    c.set(0, c.u8(actor + 0x27));
    c.set(6, 1);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(8, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, c.u8(actor + 0x105));
    c.set(0, c.r(0) == 0);
    c.set(5, c.u8(actor + 0x106));
    c.set({{9, 1}, {7, c.r(0)}, {10, 0}});
    c.call(0x101f1100, 0x1025a208);
    c.set(3, c.u32(actor + 0x1a4));
    compare_deflection(c, c.i8(c.r(3) + 0x7928));
    c.s.pc = (c.cr(0) & 2) ? 0x1025a218 : 0x1025a638;
}

static void move_blocker(Cpu& c, std::uint32_t actor, std::uint32_t blocker) {
    c.set(3, c.u32(actor + 0x1a4));
    compare_deflection(c, c.i8(c.r(3) + 0x791f), 4);
    if (c.cr(0) & 8) {
        c.set(3, c.u32(actor + 0x1a4));
        compare_deflection(c, c.i8(c.r(3) + 0x791f), -1);
        if (c.cr(0) & 4) {
            c.set(3, c.u32(actor + 0x1a4));
            c.set(0, 0);
            c.put8(c.r(3) + 0x791f, 0);
        }
    }
    compare_deflection(c, c.i8(blocker + 0x27));
    bool move_up = (c.cr(0) & 2) != 0;
    if (move_up) {
        c.set(3, c.u32(actor + 0x1a4));
        compare_deflection(c, c.i8(c.r(3) + 0x791f), -1);
        move_up = (c.cr(0) & 2) == 0;
    }
    c.set(0, c.u8(actor + 0x106));
    c.set(3, blocker);
    c.set(4, c.u8(actor + 0x105));
    c.set(6, 4);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u8(actor + 0x106));
    c.compare(0, std::int32_t(c.r(0)), move_up ? 9 : 3);
    const bool shift = move_up ? (c.cr(0) & 8) : (c.cr(0) & 4);
    if (shift) {
        c.set(7, std::uint32_t(c.i8(actor + 0x106)));
        c.set(0, c.r(7) + (move_up ? 3u : -3u));
    } else c.set(0, move_up ? 11 : 0);
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10237830, move_up ? 0x1025a6e0 : 0x1025a720);
}

void apply_deflection_position(Cpu& c) {
    const auto actor = c.r(15);
    if (c.s.pc == 0x1025a638) {
        compare_deflection(c, c.i8(c.sp() + 0x121));
        if (!(c.cr(0) & 2)) {
            c.set(3, actor);
            call_shot_owner(c, 0x10288230, 0x1025a64c);
        }
    } else {
        move_blocker(c, actor, c.r(22));
    }
    c.s.pc = 0x1025a720;
}

static void add_blocker_rating(Cpu& c, std::uint32_t blocker) {
    c.set(4, c.u8(blocker + 5));
    c.set(3, blocker);
    c.set(0, c.r(4) + 1);
    c.put8(blocker + 5, c.r(0));
    call_shot_owner(c, 0x10289560, 0x1025a734);
    compare_deflection(c, std::int32_t(c.r(3) & 4));
    bool doubled = (c.cr(0) & 2) != 0;
    if (doubled) {
        c.set(3, blocker);
        call_shot_owner(c, 0x10289560, 0x1025a744);
        compare_deflection(c, std::int32_t(c.r(3) & 2));
        doubled = (c.cr(0) & 2) != 0;
    }
    compare_deflection(c, std::int8_t(c.r(13)));
    c.set(3, std::uint32_t(c.i16(blocker + 0x36)));
    const unsigned delta = (c.cr(0) & 2) ? 200 : 120;
    c.set(0, c.r(3) + delta * (doubled ? 2 : 1));
    c.put16(blocker + 0x36, c.r(0));
}

static bool draw_deflection_end(Cpu& c) {
    advance_shot_stream(c, 0x1025a898);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x92490000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x92492493);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * std::int32_t(0x92492493)) >> 32);
    const auto corrected = std::int32_t(high + sample);
    c.s.wr<1>(reg::xer_ca, corrected < 0 && (std::uint32_t(corrected) & 3));
    const auto shifted = std::uint32_t(corrected >> 2);
    c.set(3, shifted >> 31);
    compare_deflection(c, std::int32_t(sample - (shifted + c.r(3)) * 7));
    return (c.cr(0) & 2) != 0;
}

void finish_deflected_shot(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22);
    add_blocker_rating(c, blocker);
    c.set(3, c.u8(actor + 0x2c));
    c.set(0, c.u8(c.sp() + 0x44));
    c.set(0, c.r(3) + c.r(0));
    c.put8(actor + 0x2c, c.r(0));
    if (in_shot_goal_area(c, actor)) {
        c.set(3, c.u8(blocker + 6));
        c.set(0, c.r(3) + 1);
        c.put8(blocker + 6, c.r(0));
    }
    compare_deflection(c, std::int8_t(c.r(13)));
    if (!(c.cr(0) & 2) && draw_deflection_end(c)) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, 0x1f41);
        c.set(7, c.u8(blocker + 0x27));
        c.set(8, UINT32_MAX);
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(9, UINT32_MAX);
        c.set(6, c.u8(blocker + 0x2f));
        c.set(10, 0);
        c.set(5, c.u32(actor + 0x1a4));
        c.call(0x10201180, 0x1025a904);
    }
    c.s.pc = 0x1026a190;
}

void diagnose_deflection_without_blocker(Cpu& c) {
    const auto stack = c.sp();
    c.set(3, c.toc(-0x51e4));
    c.set({{6, stack + 0xbc}, {7, stack + 0x6c}, {4, 0}, {3, c.r(3) + 0x8f}, {5, 0}});
    c.call(0x10739890, 0x1025a924);
    c.call(0x10768810, 0x1025a928);
    c.set(4, c.toc(-0x51e4));
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set({{6, stack + 0xbc}, {4, c.r(4) + 0x9c}, {7, stack + 0x6c}, {8, 0x3038}});
    c.call(0x1077b720, 0x1025a948);
    c.set(3, c.toc(-0x51e4));
    c.set(5, 0);
    c.set(4, c.toc(-0x7ba8));
    c.set(3, c.r(3) + 0xa9);
    c.call(0x10769b60, 0x1025a95c);
    c.set(3, c.toc(-0x7bac));
    c.set(0, 0);
    c.put8(c.r(3), 0);
    c.s.pc = 0x1025a968;
}

void sample_deflected_shot(Cpu&);

void apply_deflection_effects(Cpu& c) {
    if (c.s.pc == 0x1025a908) {
        diagnose_deflection_without_blocker(c);
        return;
    }
    begin_deflection_effects(c);
    if (c.s.pc == 0x1025a218) sample_deflected_shot(c);
    else apply_deflection_position(c);
    finish_deflected_shot(c);
}
}

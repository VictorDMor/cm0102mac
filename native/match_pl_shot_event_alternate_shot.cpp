#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool compare_alternate_shot(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
    return (c.cr(0) & 2) != 0;
}

void report_shot_event(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back,
                                   bool launch_parameter) {
    c.set(3, c.u32(actor + 0x1a4));
    if (launch_parameter) c.set(10, std::uint32_t(std::int8_t(c.r(20))));
    else c.set(4, code);
    c.set(7, c.u8(actor + 0x27));
    if (launch_parameter) c.set(4, code);
    else c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    if (launch_parameter) c.set(8, UINT32_MAX);
    else c.set(9, UINT32_MAX);
    c.set(6, c.u8(actor + 0x2f));
    if (launch_parameter) c.set(9, UINT32_MAX);
    else c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

static void diagnose_invalid_side(Cpu& c, std::uint32_t stack, std::uint32_t back, unsigned context) {
    const auto output = stack + (context == 0x3250 ? 0x9c : 0x7c);
    const auto detail = stack + (context == 0x3250 ? 0x64 : 0x5c);
    c.set(3, c.toc(-0x51e4));
    c.set({{6, output}, {7, detail}, {4, 0}, {3, c.r(3) + 0x8f}, {5, 0}});
    c.call(0x10739890, back);
    c.call(0x10768810, back + 0x4);
    c.set(4, c.toc(-0x51e4));
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set({{6, output}, {4, c.r(4) + 0x9c}, {7, detail}, {8, context}});
    c.call(0x1077b720, back + 0x24);
    c.set(3, c.toc(-0x51e4));
    c.set(5, 0);
    c.set(4, c.toc(-0x7ba8));
    c.set(3, c.r(3) + 0xa9);
    c.call(0x10769b60, back + 0x38);
    c.set(4, c.toc(-0x7bac));
    c.set({{0, 0}, {3, UINT32_MAX}});
    c.put8(c.r(4), 0);
    c.call(0x1076f7d0, back + 0x4c);
}

void place_alternate_shot_outside_pitch(Cpu& c, std::uint32_t actor, std::uint32_t stack,
                                        std::uint32_t diagnostic_return, unsigned diagnostic_context) {
    int row = -1;
    if (!compare_alternate_shot(c, c.u8(actor + 0x27), 1)) {
        if (!compare_alternate_shot(c, c.i8(actor + 0x27))) {
            diagnose_invalid_side(c, stack, diagnostic_return, diagnostic_context);
            return;
        }
        row = 12;
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::uint32_t(row));
    c.put8(c.r(3) + 0x791e, c.r(0));
}

static void restore_shot_counters(Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t back) {
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{3, 0}, {0, 0}, {4, 0x20f7}});
    c.put32(c.r(5) + 0x310c, 0);
    c.set({{8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.set(3, c.u32(actor + 0x1a4));
    c.put32(c.r(3) + 0x3108, 0);
    for (const auto& [slot, offset] : {std::pair{0x138u, 0x310cu}, std::pair{0x134u, 0x3108u}}) {
        c.set(0, c.u32(stack + slot));
        c.set(5, c.u32(actor + 0x1a4));
        c.set(3, c.u32(c.r(5) + offset));
        c.set(0, c.r(3) + c.r(0));
        c.put32(c.r(5) + offset, c.r(0));
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, c.u8(actor + 0x27));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

void continue_shot_retry(Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t refresh_return) {
    c.set(3, actor);
    call_shot_owner(c, 0x1022e3a0, refresh_return);
    c.set(4, c.u8(stack + 0x1cf));
    c.set(3, actor);
    c.set(6, std::uint32_t(c.i16(stack + 0x1d6)));
    c.set(7, 1);
    c.set(5, std::uint32_t(c.i16(stack + 0x130)));
    c.set({{8, 0}, {9, 0}});
    c.link(refresh_return + 0x20);
    c.s.pc = 0x102576e0;
}

void restore_and_retry_shot(Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t back) {
    restore_shot_counters(c, actor, stack, back);
    c.set(0, std::uint32_t(c.i16(stack + 0x12e)));
    c.set(3, actor);
    c.set(4, c.u8(stack + 0x12a));
    c.put16(actor + 0x3a, c.r(0));
    c.set(5, c.u8(stack + 0x129));
    call_shot_owner(c, 0x102374f0, back + 0x18);
    continue_shot_retry(c, actor, stack, back + 0x20);
}

void close_alternate_shot(Cpu& c, std::uint32_t actor, std::uint32_t back) {
    c.set(7, c.u8(actor + 0x27));
    c.set(5, 0);
    c.set(3, c.u32(actor + 0x1a4));
    c.set({{0, 3}, {4, 0x1f75}, {6, UINT32_MAX}});
    c.put8(c.r(3) + 0x7924, c.r(7));
    c.set({{8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(5));
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x7928, c.r(0));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, c.u8(actor + 0x27));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
    c.s.pc = 0x1026a190;
}

void resolve_alternate_shot(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    const bool corner = compare_alternate_shot(c, std::int8_t(c.r(20)), 5);
    report_shot_event(c, actor, corner ? 0x1f83 : 0x1f82, corner ? 0x1025c024 : 0x1025c050, true);
    if (compare_alternate_shot(c, std::int8_t(c.r(17)), 4) ||
        compare_alternate_shot(c, std::int8_t(c.r(17)), 3))
        report_shot_event(c, actor, 0x2008, 0x1025c090);
    else if (compare_alternate_shot(c, c.u8(stack + 0x1cf), 0x33) &&
             compare_alternate_shot(c, std::int8_t(c.r(17)), 1))
        report_shot_event(c, actor, 0x1fce, 0x1025c0d4);
    place_alternate_shot_outside_pitch(c, actor, stack, 0x1025c128, 0x3250);
    if (compare_alternate_shot(c, std::int8_t(c.r(17)), 5) ||
        compare_alternate_shot(c, std::int8_t(c.r(17)), 6))
        restore_and_retry_shot(c, actor, stack, 0x1025c1f4);
    else close_alternate_shot(c, actor, 0x1025c288);
}

void route_missed_shot_outcome(Cpu& c) {
    if (compare_alternate_shot(c, std::int8_t(c.r(17)), 1) ||
        compare_alternate_shot(c, std::int8_t(c.r(17)), 2)) {
        const auto flight = std::int8_t(c.r(13));
        c.set(0, std::uint32_t(flight));
        c.compare(0, flight, 12);
        if (c.cr(0) & 8) {
            c.compare(0, flight, 5);
            if (c.cr(0) & 8) {
                c.compare(0, flight, 2);
                if (!(c.cr(0) & 8)) {
                    c.s.pc = 0x1025d4d4;
                    return;
                }
            }
        } else {
            c.compare(0, flight, 14);
            if (c.cr(0) & 8) {
                c.s.pc = 0x1025d26c;
                return;
            }
        }
        c.s.pc = 0x1025f914;
    } else if (compare_alternate_shot(c, std::int8_t(c.r(17)), 5) ||
               compare_alternate_shot(c, std::int8_t(c.r(17)), 6)) {
        restore_and_retry_shot(c, c.r(15), c.sp(), 0x1025d200);
    } else {
        c.s.pc = 0x1026a190;
    }
}
}

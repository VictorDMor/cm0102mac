#include "match_pl_shot_event_detail.hpp"
#include <array>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool compare_shot_value(Cpu& c, std::int32_t value, std::int32_t against) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
    return (c.cr(0) & 2) != 0;
}

static bool convert_to_restart(Cpu& c, std::uint32_t stack) {
    if (compare_shot_value(c, c.i8(stack + 0x40), 0)) return false;
    draw_shot_modulo_fifty(c, 0x10258360);
    return (c.cr(0) & 2) && !compare_shot_value(c, std::int8_t(c.r(17)), 4) &&
           compare_shot_value(c, c.i8(stack + 0x4c), 0);
}

static void award_blocked_restart(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(0, c.u8(stack + 0x4e));
    c.set(5, c.r(22));
    c.set(6, actor);
    c.put32(stack + 0x38, c.r(0));
    c.set(0, c.u8(stack + 0x4d));
    c.put32(stack + 0x3c, c.r(0));
    c.set(4, c.u8(stack + 0x42));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, c.u8(stack + 0x4c));
    c.set(4, std::uint32_t(std::int8_t(c.r(4))));
    c.set(8, c.u8(stack + 0x40));
    c.set(9, c.u8(actor + 0x105));
    c.set(10, c.u8(actor + 0x106));
    c.call(0x101d68f0, 0x102583ec);
    c.set(17, 2);
    c.s.pc = 0x10259580;
}

void report_blocked_shot(Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(c.r(22) + 0x2f));
    c.set(7, c.u8(c.r(22) + 0x27));
    c.set(8, c.u8(actor + 0x2f));
    c.set(9, c.u8(actor + 0x27));
    c.call(0x10201180, back);
}

void report_blocking_action(Cpu& c, std::uint32_t actor, std::uint32_t stack, BlockMessageTable table) {
    const auto kind = std::uint32_t(c.i8(stack + 0x42));
    c.set(0, kind);
    c.compare_logical(0, kind, 46);
    constexpr std::uint32_t stride = 0x2c;
    const auto first = table.first, fallback = first + 11 * stride;
    std::uint32_t target = fallback;
    if (!(c.cr(0) & 4)) {
        c.set(3, c.toc(table.toc_offset));
        c.set(0, kind * 4);
        c.s.pc = table.read_pc;
        target = c.u32(c.r(3) + c.r(0));
        c.set(3, target);
        c.s.wr<4>(reg::ctr, target);
    }
    constexpr std::array<unsigned, 12> messages{
        0x1fea, 0x1feb, 0x1fec, 0x1fed, 0x1fee, 0x1ff2, 0x1fef, 0x1ff1, 0x1ff0, 0x1ff3, 0x1ff4, 0x1fe9};
    if (target < first || target > fallback || (target - first) % stride != 0) {
        // An observer may retarget the live jump table. The continuation
        // dispatcher owns the selected PC and the original LR is still live.
        c.s.pc = target;
        return;
    }
    report_blocked_shot(c, actor, messages[(target - first) / stride], target + 0x28);
    c.s.pc = table.continuation;
}

static void restart_shot_after_block(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x1f7a);
    c.set(0, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, 0x10258694);
    c.set({{0, 0x33}, {3, actor}});
    c.put8(stack + 0x1cf, c.r(0));
    c.set({{4, stack + 0x1cf}, {5, stack + 0x1d6}});
    call_shot_owner(c, 0x10252080, 0x102586ac);
    c.set(4, c.u8(stack + 0x1cf));
    c.set(5, std::uint32_t(std::int16_t(c.r(3))));
    c.set(6, std::uint32_t(c.i16(stack + 0x1d6)));
    c.set({{3, actor}, {7, 4}, {8, 0}, {9, 0}});
    c.link(0x102586cc);
    c.s.pc = 0x102576e0;
}

void resolve_blocked_shot(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (convert_to_restart(c, stack)) {
        award_blocked_restart(c, actor, stack);
        return;
    }
    if (compare_shot_value(c, c.i8(stack + 0x40), 0)) {
        c.s.pc = 0x102594a4;
        return;
    }
    c.set(0, c.u8(stack + 0x41));
    c.set({{4, c.r(22)}, {5, actor}, {6, stack + 0x40}});
    c.put32(stack + 0x38, c.r(0));
    c.set(10, stack + 0x58);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(7, c.u8(stack + 0x4e));
    c.set(8, c.u8(stack + 0x4d));
    c.set(9, c.u8(stack + 0x4c));
    c.call(0x101f5110, 0x1025842c);
    report_blocking_action(c, actor, stack, {-0x522c, 0x10258444, 0x10258450, 0x1025865c});
    if (c.s.pc != 0x1025865c) return;
    if (compare_shot_value(c, std::int8_t(c.r(17)), 4)) restart_shot_after_block(c, actor, stack);
    else c.s.pc = 0x102586d0;
}
}

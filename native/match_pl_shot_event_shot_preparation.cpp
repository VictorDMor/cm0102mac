#include "match_pl_shot_event_detail.hpp"
#include "match_pl_adapter.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

void advance_shot_stream(Cpu& c, std::uint32_t back) {
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(30)));
    c.set(3, c.r(3) + 2);
    c.compare_logical(0, c.r(3), c.r(0));
    c.put32(c.r(31), c.r(3));
    if (!(c.cr(0) & 4)) return;
    c.put32(c.r(31), c.r(29));
    c.call(0x1077b770, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(c.r(28), c.r(0));
}

// Callers establish bounds in [-65535, 65535]. Preserve both original
// bound comparisons and the narrow sampler's guest register effects.
void draw_narrow_shot_sample(Cpu& c, std::int32_t bound, unsigned destination, std::uint32_t back) {
    c.compare(0, bound, 0);
    if (bound == 0) {
        c.set(destination, 0);
        return;
    }
    advance_shot_stream(c, back);
    c.set(3, 0xffff);
    c.compare(0, bound, 65535);
    c.set(3, 0xffff0001u);
    c.compare(0, bound, -65535);
    c.set(3, c.u32(c.r(31)));
    c.compare(0, bound, 0);
    c.set(0, c.u32(c.r(28)));
    const auto numerator = c.u16(c.r(3)) + c.r(0);
    c.set(3, numerator);
    const auto quotient = std::uint32_t(divide_signed<4>(numerator, std::uint32_t(bound)));
    const auto remainder = numerator - quotient * std::uint32_t(bound);
    c.set(destination, remainder);
    c.set(0, bound > 0 ? remainder : 0u - remainder);
    c.set(destination, c.r(0));
}

std::uint32_t draw_shot_modulo_three(Cpu& c, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x55550000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x55555556);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x55555556) >> 32);
    c.set(3, high);
    return sample - (high + (high >> 31)) * 3;
}

void draw_shot_modulo_fifty(Cpu& c, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x51ec0000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x51eb851f);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x51eb851f) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
    const auto shifted = std::uint32_t(high >> 4);
    c.set(3, shifted >> 31);
    c.set(0, sample - (shifted + c.r(3)) * 50);
    c.compare(0, std::int32_t(c.r(0)), 0);
}

void call_shot_owner(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    c.link(back);
    c.s.pc = entry;
    if (!try_modern_match_pl(c.s, entry)) run_until(c.s, back);
}

static void apply_opening_sample(Cpu& c) {
    const auto actor = c.r(15), slot = c.sp() + 0x50;
    const auto initial_bound = c.i16(slot);
    c.set(4, std::uint32_t(initial_bound));
    c.set(14, c.r(3));
    c.set(0, std::uint32_t(c.i16(actor + 0x3a)));
    const auto corrected = initial_bound + (initial_bound < 0);
    c.s.wr<1>(reg::xer_ca, corrected < 0 && (std::uint32_t(corrected) & 1));
    c.set(3, std::uint32_t(corrected >> 1));
    c.set(0, c.r(0) - c.r(3));
    c.put16(actor + 0x3a, c.r(0));
    // The actor write can cause the bound to change before the second read.
    const auto bound = c.i16(slot);
    c.set(22, std::uint32_t(bound));
    draw_narrow_shot_sample(c, bound, 21, 0x10257ad0);
    c.put16(slot, c.r(21));
}

static std::uint32_t load_match_local(Cpu& c, std::uint32_t actor, unsigned reg) {
    c.set(reg, c.u32(actor + 0x1a4) + 0x10000);
    return c.r(reg);
}

static bool eligible_for_shot_bonus(Cpu& c, std::uint32_t actor) {
    c.compare_logical(0, actor, c.r(3));
    if (!(c.cr(0) & 2)) return false;
    c.set(0, c.u32(load_match_local(c, actor, 3) - 0x1fdc));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return false;
    for (const auto excluded : {0x12, 0x11}) {
        c.set(0, std::uint32_t(c.i16(actor + 0x19c)));
        c.compare(0, std::int32_t(c.r(0)), excluded);
        if (c.cr(0) & 2) return false;
    }
    c.set(0, std::uint32_t(c.i16(actor + 0x2a)));
    c.compare(0, std::int32_t(c.r(0)), -100);
    if (!(c.cr(0) & 4)) {
        c.set(0, std::uint32_t(std::int16_t(c.r(16))));
        c.compare(0, std::int32_t(c.r(0)), 100);
        if (!(c.cr(0) & 4)) return false;
    }
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    c.compare(0, std::int32_t(c.r(0)), 1);
    return c.cr(0) & 2;
}

void credit_shot_assistant(Cpu& c, std::uint32_t actor) {
    for (const auto excluded : {0x34, 0x37}) {
        c.set(3, c.u32(load_match_local(c, actor, 3) - 0x1fdc));
        c.set(0, std::uint32_t(c.i16(c.r(3) + 0x19c)));
        c.compare(0, std::int32_t(c.r(0)), excluded);
        if (c.cr(0) & 2) return;
    }
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, 0x55555556);
    c.set(4, c.r(4) + 0x10000);
    c.set(5, c.u32(c.r(4) - 0x1fdc));
    c.set(4, c.u8(c.r(5) + 3));
    c.set(0, c.r(4) + 1);
    c.put8(c.r(5) + 3, c.r(0));
    c.set(0, std::uint32_t(c.i16(actor + 0x2a)));
    c.set(4, c.u32(actor + 0x1a4));
    const auto high = std::uint32_t((std::int64_t(std::int32_t(c.r(0))) * 0x55555556) >> 32);
    c.set(3, high);
    c.set(4, c.r(4) + 0x10000);
    c.set(5, c.u32(c.r(4) - 0x1fdc));
    c.set(4, std::uint32_t(c.i16(c.r(5) + 0x36)));
    c.set(3, high + (high >> 31));
    c.set(0, c.r(4) + std::uint32_t(std::int16_t(c.r(3) + 0xe1)));
    c.put16(c.r(5) + 0x36, c.r(0));
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, c.u32(c.r(4) + 0x3108));
    c.set(0, c.r(3) + 0x113);
    c.put32(c.r(4) + 0x3108, c.r(0));
}

void prepare_shot(Cpu& c) {
    apply_opening_sample(c);
    const auto actor = c.r(15);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::uint32_t(c.i16(c.sp() + 0x50)));
    c.set(3, c.u32(c.r(3) + 0xe028));
    c.set(16, c.r(16) - c.r(0));
    if (eligible_for_shot_bonus(c, actor)) {
        credit_shot_assistant(c, actor);
        c.set(0, 1);
        c.put8(c.sp() + 0x120, 1);
    }

    c.set(0, c.u8(actor + 0x27));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::int8_t(c.r(0)) == 0);
    c.set(4, c.r(0));
    c.call(0x101f1e10, 0x10257d30);
}

static void diagnose_missing_keeper(Cpu& c) {
    const auto stack = c.sp();
    c.set(3, c.toc(-0x51e4));
    c.set({{6, stack + 0xdc}, {7, stack + 0x74}, {4, 0}, {3, c.r(3) + 0x8f}, {5, 0}});
    c.call(0x10739890, 0x10257d54);
    c.call(0x10768810, 0x10257d58);
    c.set(4, c.toc(-0x51e4));
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set({{6, stack + 0xdc}, {4, c.r(4) + 0x9c}, {7, stack + 0x74}, {8, 0x2dff}});
    c.call(0x1077b720, 0x10257d78);
    c.set(3, c.toc(-0x51e4));
    c.set(5, 0);
    c.set(4, c.toc(-0x7ba8));
    c.set(3, c.r(3) + 0xa9);
    c.call(0x10769b60, 0x10257d8c);
    c.set(4, c.toc(-0x7bac));
    c.set({{0, 0}, {3, UINT32_MAX}});
    c.put8(c.r(4), 0);
    c.call(0x1076f7d0, 0x10257da0);
}

static void select_keeper_option(Cpu& c) {
    const auto action = c.i8(c.sp() + 0x1cf);
    c.set(0, std::uint32_t(action));
    switch (action) {
    case 0x16: case 0x39:
        c.compare(0, action, action);
        c.set(21, 0);
        break;
    case 0x17: case 0x1a: case 0x1b:
        c.compare(0, action, action == 0x1b ? 0x1a : action);
        c.set(21, 1);
        break;
    case 0x18: case 0x19: case 0x1c: {
        c.set(3, draw_shot_modulo_three(c, 0x10257e20));
        c.set(0, (c.r(3) | (0u - c.r(3))) >> 31);
        c.set(21, c.r(0));
        break;
    }
    case 0x3a: {
        advance_shot_stream(c, 0x10257e88);
        c.set(3, c.u32(c.r(31)));
        c.set(0, c.u32(c.r(28)));
        const auto sample = c.u16(c.r(3)) + c.r(0);
        c.set(3, sample >> 31);
        c.set(0, (sample & 3) == 0);
        c.set(21, c.r(0));
        break;
    }
    default: {
        advance_shot_stream(c, 0x10257ee4);
        c.set(3, c.u32(c.r(31)));
        c.set(0, c.u32(c.r(28)));
        const auto sample = c.u16(c.r(3)) + c.r(0);
        c.set(3, sample >> 31);
        c.set(0, ((sample & 1) ^ c.r(3)) - c.r(3));
        c.set(21, std::uint32_t(std::int8_t(c.r(0))));
        break;
    }
    }
}

static void sample_keeper_response(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(4, c.u8(actor + 0x105));
    c.set(3, c.r(24));
    c.set(5, c.u8(actor + 0x106));
    c.set(6, c.r(21));
    c.set(7, c.u8(stack + 0x1cf));
    c.set({{8, stack + 0x46}, {9, stack + 0x45}});
    c.link(0x10257f30);
    c.s.pc = 0x10251270;
    if (!try_modern_match_pl(c.s, 0x10251270)) run_until(c.s, 0x10257f30);
    c.set(4, c.u8(stack + 0x45));
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    c.put16(stack + 0x132, c.r(0));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return;
    c.set(0, std::uint32_t(c.i16(stack + 0x132)));
    c.compare(0, std::int32_t(c.r(0)), 375);
    if (c.cr(0) & 4) {
        c.set(0, 1);
        c.put8(stack + 0x123, 1);
        return;
    }
    c.set(0, std::uint32_t(c.i16(stack + 0x132)));
    c.compare(0, std::int32_t(c.r(0)), 260);
    if (c.cr(0) & 4) {
        c.set(0, 1);
        c.put8(stack + 0x122, 1);
    }
}

static bool search_for_blocker(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u8(c.r(3) + 0x791f));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (c.cr(0) & 2) return false;
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (c.cr(0) & 2) return false;
    for (const auto action : {0x33, 0x35}) {
        c.set(0, c.u8(stack + 0x1cf));
        c.compare(0, std::int32_t(c.r(0)), action);
        if (c.cr(0) & 2) return false;
    }
    return true;
}

void choose_shot_opposition(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(24, c.r(3));
    c.compare(0, std::int32_t(c.r(24)), 0);
    if (c.cr(0) & 2) diagnose_missing_keeper(c);
    select_keeper_option(c);
    sample_keeper_response(c, actor, stack);
    if (search_for_blocker(c, actor, stack)) {
        c.set(0, stack + 0x4c);
        c.set(4, actor);
        c.put32(stack + 0x38, c.r(0));
        c.set({{7, stack + 0x44}, {8, stack + 0x43}, {9, stack + 0x52}});
        c.set(0, c.u8(stack + 0x46));
        c.set({{10, stack + 0x42}, {6, 0x6d}});
        c.put32(stack + 0x3c, c.r(0));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(5, c.u8(actor + 0x27));
        c.call(0x101eaf60, 0x10257fd8);
        c.set(22, c.r(3));
    } else {
        c.set(22, 0);
    }
    c.set(7, c.u8(stack + 0x42));
    c.set(4, actor);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, c.r(22));
    c.set(7, std::uint32_t(std::int8_t(c.r(7))));
    c.set({{5, 0x6d}, {8, 0}});
    c.call(0x101f5350, 0x10258004);
}
}

#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool compare_shot_comment(Cpu& c, std::int32_t value, std::int32_t target) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, target);
    return (c.cr(0) & 2) != 0;
}

static void maybe_clear_keeper_mark(Cpu& c, std::uint32_t actor) {
    c.set(0, c.u8(actor + 0x2d));
    c.set(3, std::uint32_t(std::int8_t(c.r(14))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 2)) return;

    advance_shot_stream(c, 0x1025a99c);
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
    if (!compare_shot_comment(c, std::int32_t(sample - (shifted + c.r(3)) * 10), 0))
        c.set(14, UINT32_MAX);
}

static void emit_shot_message(Cpu& c, std::uint32_t actor, unsigned qualifier, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(8, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(9, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(10, qualifier);
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(4, std::uint32_t(c.i16(c.sp() + 0x1d6)));
    c.call(0x10201180, back);
}

static bool goalkeeper_far_from_shot(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    c.set(3, c.u8(keeper + 0x105));
    c.set(0, c.u8(actor + 0x105));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set(14, c.u8(keeper + 0x106));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(3, c.r(0) - c.r(3));
    c.call(0x10775540, 0x1025aaac);
    c.set(0, c.u8(actor + 0x106));
    c.set(4, std::uint32_t(std::int8_t(c.r(14))));
    c.set(14, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(3, c.r(0) - c.r(4));
    c.call(0x10775540, 0x1025aac4);
    c.set(3, std::uint32_t(std::int8_t(c.r(3)) * 0x24));
    c.set(0, std::uint32_t(std::int8_t(c.r(14))));
    c.set_f(0, c.f64(c.r(27) + 0x90));
    c.set(0, c.r(3) + c.r(0) * 4);
    c.set(3, c.toc(-0x6424));
    c.set_f(1, c.f32(c.r(3) + c.r(0)));
    Fpu(c).fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

static void emit_far_keeper_message(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    c.set(0, c.u8(actor + 0x27));
    c.set(4, 0x1fe0);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    const bool opposing_side = c.r(0) == 0;
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, opposing_side);
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(9, c.r(0));
    c.set(8, c.u8(keeper + 0x2f));
    c.call(0x10201180, 0x1025ab24);
}

static unsigned update_keeper_event_code(Cpu& c, std::int32_t code) {
    c.compare(0, code, 0x1fa4);
    if (c.cr(0) & 2) return 0;
    if (c.cr(0) & 8) {
        c.compare(0, code, 0x1f8b);
        if (c.cr(0) & 2) return 0x1f8c;
        if (c.cr(0) & 4) {
            c.compare(0, code, 0x1fa3);
            return (c.cr(0) & 8) ? 0 : 0x1fa4;
        }
        c.compare(0, code, 0x1f89);
        return (c.cr(0) & 2) ? 0x1f8a : 0;
    }
    c.compare(0, code, 0x1fb2);
    if (c.cr(0) & 2) return 0;
    if (c.cr(0) & 4) {
        c.compare(0, code, 0x1fb4);
        return (c.cr(0) & 8) ? 0x1fb4 : 0;
    }
    c.compare(0, code, 0x1fb1);
    if (!(c.cr(0) & 8)) return 0x1fb2;
    c.compare(0, code, 0x1fa6);
    return (c.cr(0) & 8) ? 0x1fa6 : 0;
}

static void emit_keeper_shot_message(Cpu& c, std::uint32_t actor, std::uint32_t keeper, bool far) {
    if (!far) c.set(10, 0);
    c.set(3, c.u32(actor + 0x1a4));
    if (far) c.set(10, 0);
    c.set(4, std::uint32_t(c.i16(c.sp() + 0x1d6)));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(8, c.u8(keeper + 0x2f));
    c.set(9, c.u8(keeper + 0x27));
    c.call(0x10201180, far ? 0x1025ab4c : 0x1025ac2c);
}

static void describe_keeper_shot(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, 0x1025aa4c);
    if (!compare_shot_comment(c, std::int16_t(c.r(3)), 1)) {
        emit_shot_message(c, actor, 0, 0x1025aa80);
        return;
    }
    const auto stack = c.sp();
    const bool far = compare_shot_comment(c, c.u8(stack + 0x46), 4) && goalkeeper_far_from_shot(c, actor, keeper);
    if (far) {
        emit_far_keeper_message(c, actor, keeper);
        emit_keeper_shot_message(c, actor, keeper, true);
        c.set(0, 0);
        c.put8(stack + 0x46, 0);
        return;
    }
    const auto old = c.i16(stack + 0x1d6);
    c.set(0, std::uint32_t(old));
    if (const auto code = update_keeper_event_code(c, old)) {
        c.set(0, code);
        c.put16(stack + 0x1d6, code);
    }
    c.set({{0, 0}, {10, 0}});
    c.put8(stack + 0x46, 0);
    emit_keeper_shot_message(c, actor, keeper, false);
}

static bool aimed_keeper_action(Cpu& c, std::uint32_t stack) {
    return compare_shot_comment(c, c.u8(stack + 0x1cf), 0x1b) ||
           compare_shot_comment(c, c.u8(stack + 0x1cf), 0x1a);
}

static void describe_unmarked_shot(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (aimed_keeper_action(c, stack)) {
        c.set(0, 0x1f8e);
        c.put16(stack + 0x1d6, c.r(0));
    }
    if (!compare_shot_comment(c, std::int8_t(c.r(14)), -1)) {
        const auto index = std::uint32_t(c.i16(stack + 0x1d6));
        c.set(3, index);
        c.set(0, index - 0x1f86);
        c.compare_logical(0, c.r(0), 19);
        if (!(c.cr(0) & 4)) {
            c.set(3, c.toc(-0x5238));
            c.set(0, c.r(0) * 4);
            c.s.pc = 0x1025ac74;
            const auto target = c.u32(c.r(3) + c.r(0));
            c.set(3, target);
            c.s.wr<4>(reg::ctr, target);
            unsigned base = 0;
            switch (target) {
            case 0x1025ac80: base = 0x1f88; break;
            case 0x1025aca0: base = 0x1f90; break;
            case 0x1025acc0: base = 0x1f95; break;
            case 0x1025ace0: base = 0x1f98; break;
            case 0x1025ad00: base = 0x1f9b; break;
            case 0x1025ad1c: break;
            default: c.s.pc = target; return;
            }
            if (base) {
                c.set(3, std::int8_t(c.r(14)) == 0 ? UINT32_MAX : 0);
                c.set(0, c.r(3) + base);
                c.put16(stack + 0x1d6, c.r(0));
            }
        }
    }
    emit_shot_message(c, actor, 0, 0x1025ad44);
    c.s.pc = 0x1025ad44;
}

void describe_shot_attempt(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    maybe_clear_keeper_mark(c, actor);
    if (compare_shot_comment(c, c.u8(stack + 0x1cf), 3) &&
        compare_shot_comment(c, std::int8_t(c.r(18)), 0)) {
        c.compare_logical(0, c.r(22), 0);
        if (c.cr(0) & 2) {
            emit_shot_message(c, actor, 7, 0x1025aa20);
            c.s.pc = 0x1025ad44;
            return;
        }
    }
    if (aimed_keeper_action(c, stack)) {
        c.compare_logical(0, c.r(24), 0);
        if (!(c.cr(0) & 2)) {
            describe_keeper_shot(c, actor, c.r(24));
            c.s.pc = 0x1025ad44;
            return;
        }
    }
    describe_unmarked_shot(c, actor, stack);
}
}

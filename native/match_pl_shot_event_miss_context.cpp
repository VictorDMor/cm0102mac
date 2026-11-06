#include "match_pl_shot_event_detail.hpp"

#include <array>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool record_context_value(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return (c.cr(0) & 2) != 0;
}

static bool eligible_for_miss_recipient(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (record_context_value(c, c.u8(stack + 0x1cf), 0x33) ||
        record_context_value(c, c.u8(stack + 0x1cf), 0x35) ||
        !record_context_value(c, std::int8_t(c.r(17)), 1)) return false;
    for (const auto offset : {0xc, 0x10, 3}) {
        c.set(0, c.u8(actor + offset));
        c.compare_logical(0, c.r(0), 0);
        if (!(c.cr(0) & 2)) return false;
    }
    c.set(0, c.u8(actor + 0xa));
    c.compare_logical(0, c.r(0), 5);
    if (!(c.cr(0) & 8)) return false;
    c.set(0, c.u8(actor + 0xb));
    c.compare_logical(0, c.r(0), 2);
    return (c.cr(0) & 8) != 0;
}

static bool passes_recipient_clock(Cpu& c, std::uint32_t stack, std::uint32_t refill_back) {
    advance_shot_stream(c, refill_back);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(stack + 0x11c));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 7));
    const auto shifted = std::uint32_t(high >> 3);
    c.set(4, sample - (shifted + (shifted >> 31)) * 20);
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + 65)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 4) != 0;
}

static bool passes_miss_recipient_rating(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    record_context_value(c, c.i16(actor + 0x36), 7250);
    if (!(c.cr(0) & 8)) return false;
    record_context_value(c, c.i16(recipient + 0x36), 6750);
    return (c.cr(0) & 4) != 0;
}

static bool sample_miss_recipient(Cpu& c, std::uint32_t actor, std::uint32_t refill_back, unsigned left, unsigned right) {
    record_context_value(c, c.i8(actor + 0x103), 1);
    if (!(c.cr(0) & 8)) {
        const auto bound = c.i8(actor + 0x103);
        c.set(13, std::uint32_t(bound));
        draw_narrow_shot_sample(c, bound, 17, refill_back);
        c.compare(0, std::int32_t(c.r(17)), 0);
        if (!(c.cr(0) & 2)) return false;
    }
    c.compare_logical(0, c.r(left), c.r(right));
    return (c.cr(0) & 2) != 0;
}

static void report_miss_pair(Cpu& c, std::uint32_t actor, std::uint32_t recipient,
                             unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(8, c.u8(recipient + 0x2f));
    c.set(9, c.u8(recipient + 0x27));
    c.call(0x10201180, back);
}

static void report_miss_pair_on_other_side(Cpu& c, std::uint32_t actor, std::uint32_t recipient,
                                           unsigned code, std::uint32_t back) {
    c.set(0, c.u8(actor + 0x27));
    c.set(4, code);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) >> 5);
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(9, c.r(0));
    c.set(8, c.u8(recipient + 0x2f));
    c.call(0x10201180, back);
}

static void report_miss_recipient_event(Cpu& c, std::uint32_t actor, std::uint32_t recipient,
                                        unsigned code, std::uint32_t back) {
    c.set(0, c.u8(actor + 0x27));
    c.set(4, code);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(8, UINT32_MAX);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(6, c.u8(recipient + 0x2f));
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) >> 5);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

static bool select_miss_recipient_event(Cpu& c, std::uint32_t actor, std::uint32_t recipient,
                                        std::uint32_t first_back) {
    constexpr std::array<unsigned, 6> events{0x219f, 0x21a0, 0x21b0, 0x21b3, 0x21b4, 0x21b5};
    auto back = first_back;
    for (const auto code : events) {
        c.set({{3, recipient}, {4, code}});
        call_shot_owner(c, 0x10289d50, back);
        if (!record_context_value(c, std::int8_t(c.r(3)))) {
            report_miss_recipient_event(c, actor, recipient, code, back + 0x40);
            return true;
        }
        back += 0x50;
    }
    return false;
}

void describe_single_context_miss(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp(), context = c.r(19);
    c.set(0, c.u32(context + 4));
    c.set(3, context);
    c.put32(context + 0xc, c.r(0));
    c.call(0x10738e00, 0x1025d9e0);
    c.set(0, c.u8(stack + 0x11d));
    c.set(14, c.r(3));
    if (record_context_value(c, std::int8_t(c.r(0)))) {
        report_miss_pair_on_other_side(c, actor, c.r(14), 0x2154, 0x1025dce0);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, actor);
        c.call(0x101fec50, 0x1025d9fc);
        c.set(16, c.r(3));
        c.compare(0, std::int32_t(c.r(16)), 0);
        if (!(c.cr(0) & 2) && eligible_for_miss_recipient(c, actor, stack) &&
            passes_recipient_clock(c, stack, 0x1025da84) && passes_miss_recipient_rating(c, actor, c.r(16)) &&
            sample_miss_recipient(c, actor, 0x1025db30, 14, 16))
            report_miss_pair(c, actor, c.r(14), 0x21c6, 0x1025dca4);
    }
    select_miss_recipient_event(c, actor, c.r(14), 0x1025dcec);
    c.s.pc = 0x1025e6a4;
}

static bool draw_second_context(Cpu& c, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    return !record_context_value(c, std::int32_t(((sample & 1) ^ sign) - sign));
}

static void report_both_miss_recipients(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u8(actor + 0x27));
    c.set(4, 0x2155);
    c.set(0, c.u8(actor + 0x27));
    c.set(10, 0);
    c.set(6, std::uint32_t(std::int8_t(c.r(3))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(7, c.r(6) == 0 ? 32 : 0);
    c.set(0, std::int8_t(c.r(0)) == 0 ? 32 : 0);
    c.set(3, c.u32(c.r(5) + 0x79f4));
    c.set(7, c.r(7) >> 5);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) >> 5);
    c.set(6, c.u8(c.r(14) + 0x2f));
    c.set(8, c.u8(c.r(16) + 0x2f));
    c.set(9, c.r(0));
    c.call(0x10201180, 0x1025e244);
}

void describe_double_context_miss(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp(), context = c.r(19);
    if (!draw_second_context(c, 0x1025dee0)) {
        report_shot_event(c, actor, 0x2132, 0x1025e62c);
        c.s.pc = 0x1025e6a4;
        return;
    }
    c.set(0, c.u32(context + 4));
    c.set(3, context);
    c.put32(context + 0xc, c.r(0));
    c.call(0x10738e00, 0x1025df1c);
    c.set(0, c.r(3));
    c.set(3, context);
    c.set(14, c.r(0));
    c.call(0x10738e00, 0x1025df2c);
    c.set(0, c.u8(stack + 0x11d));
    c.set(16, c.r(3));
    if (record_context_value(c, std::int8_t(c.r(0)))) {
        report_both_miss_recipients(c, actor);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, actor);
        c.call(0x101fec50, 0x1025df48);
        c.set(18, c.r(3));
        c.compare(0, std::int32_t(c.r(18)), 0);
        if (!(c.cr(0) & 2)) {
            c.compare_logical(0, c.r(16), 0);
            if (!(c.cr(0) & 2) && eligible_for_miss_recipient(c, actor, stack) &&
                passes_recipient_clock(c, stack, 0x1025dfd8) && passes_miss_recipient_rating(c, actor, c.r(18)) &&
                sample_miss_recipient(c, actor, 0x1025e084, 16, 18))
                report_miss_pair(c, actor, c.r(16), 0x21c6, 0x1025e1f8);
        }
    }
    if (!select_miss_recipient_event(c, actor, c.r(14), 0x1025e250))
        select_miss_recipient_event(c, actor, c.r(16), 0x1025e430);
    c.s.pc = 0x1025e6a4;
}

void describe_power_miss_recipient(Cpu& c) {
    constexpr std::uint32_t closure = 0x1025f858;
    const auto actor = c.r(15), stack = c.sp(), context = c.r(19);
    if (record_context_value(c, c.i8(stack + 0x11d))) {
        c.s.pc = closure;
        return;
    }
    c.set(0, c.u32(context + 4));
    c.set(3, context);
    c.put32(context + 0xc, c.r(0));
    c.call(0x10738e00, 0x1025e9b4);
    c.set(0, c.r(3));
    c.set(3, c.u32(actor + 0x1a4));
    c.set({{14, c.r(0)}, {4, actor}});
    c.call(0x101fec50, 0x1025e9c8);
    c.set(16, c.r(3));
    c.compare(0, std::int32_t(c.r(16)), 0);
    if (!(c.cr(0) & 2) && eligible_for_miss_recipient(c, actor, stack) &&
        passes_recipient_clock(c, stack, 0x1025ea50) && passes_miss_recipient_rating(c, actor, c.r(16)) &&
        sample_miss_recipient(c, actor, 0x1025eafc, 14, 16)) {
        report_miss_pair(c, actor, c.r(14), 0x21c6, 0x1025ec70);
    }
    c.s.pc = closure;
}

bool report_single_shot_context(Cpu& c, std::uint32_t iterator_return) {
    const auto actor = c.r(15), context = c.r(19);
    c.set(0, c.u32(context + 4));
    c.set(3, context);
    c.put32(context + 0xc, c.r(0));
    c.call(0x10738e00, iterator_return);
    c.set(0, c.u8(actor + 0x27));
    c.set(14, c.r(3));
    c.set(4, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set(3, c.u32(c.r(4) + 0x79f4));
    c.set(0, c.r(0) >> 5);
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(4, 0x2154);
    c.set(8, c.u8(c.r(14) + 0x2f));
    c.set(9, c.r(0));
    c.call(0x10201180, iterator_return + 0x3c);
    return select_miss_recipient_event(c, actor, c.r(14), iterator_return + 0x48);
}

void describe_low_power_single_miss(Cpu& c) {
    const bool on_target = c.s.pc == 0x1025fb88;
    const bool reported = report_single_shot_context(c, on_target ? 0x1025fb98 : 0x1025eeac);
    c.s.pc = on_target ? 0x10262154 : reported ? 0x1025f858 : 0x1025f0c8;
}

void describe_low_power_fallback(Cpu& c) {
    const bool on_target = c.s.pc == 0x10260258;
    report_shot_event(c, c.r(15), 0x2132, on_target ? 0x10260280 : 0x1025f858);
    c.s.pc = on_target ? 0x10262154 : 0x1025f858;
}

void describe_low_power_miss_recipient(Cpu& c) {
    constexpr std::uint32_t closure = 0x1025f858;
    const auto actor = c.r(15), stack = c.sp();
    if (record_context_value(c, c.i8(stack + 0x11d))) {
        c.s.pc = closure;
        return;
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, actor);
    c.call(0x101fec50, 0x1025f0e0);
    c.set(16, c.r(3));
    c.compare(0, std::int32_t(c.r(16)), 0);
    if (!(c.cr(0) & 2) && eligible_for_miss_recipient(c, actor, stack) &&
        passes_recipient_clock(c, stack, 0x1025f168) && passes_miss_recipient_rating(c, actor, c.r(16)) &&
        sample_miss_recipient(c, actor, 0x1025f214, 14, 16)) {
        report_miss_pair(c, actor, c.r(14), 0x21c6, 0x1025f388);
    }
    c.s.pc = closure;
}

void report_double_shot_context(Cpu& c, std::uint32_t refill_return) {
    const auto actor = c.r(15), context = c.r(19);
    if (!draw_second_context(c, refill_return)) {
        report_shot_event(c, actor, 0x2132, refill_return + 0x480);
        return;
    }
    c.set(0, c.u32(context + 4));
    c.set(3, context);
    c.put32(context + 0xc, c.r(0));
    c.call(0x10738e00, refill_return + 0x3c);
    c.set(0, c.r(3));
    c.set(3, context);
    c.set(14, c.r(0));
    c.call(0x10738e00, refill_return + 0x4c);
    c.set(5, c.u8(actor + 0x27));
    c.set(16, c.r(3));
    c.set(0, c.u8(actor + 0x27));
    c.set(4, 0x2155);
    c.set(7, c.u32(actor + 0x1a4));
    c.set(6, std::uint32_t(std::int8_t(c.r(5))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(3, c.u32(c.r(7) + 0x79f4));
    c.set(7, c.r(6) == 0 ? 32 : 0);
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set(6, c.u8(c.r(14) + 0x2f));
    c.set(7, c.r(7) >> 5);
    c.set(8, c.u8(c.r(16) + 0x2f));
    c.set(0, c.r(0) >> 5);
    c.set({{10, 0}, {9, c.r(0)}});
    c.call(0x10201180, refill_return + 0x98);
    if (!select_miss_recipient_event(c, actor, c.r(14), refill_return + 0xa4))
        select_miss_recipient_event(c, actor, c.r(16), refill_return + 0x284);
}

void describe_low_power_double_miss(Cpu& c) {
    const bool on_target = c.s.pc == 0x1025fdb4;
    report_double_shot_context(c, on_target ? 0x1025fdd4 : 0x1025f3ac);
    c.s.pc = on_target ? 0x10262154 : 0x1025f858;
}
}

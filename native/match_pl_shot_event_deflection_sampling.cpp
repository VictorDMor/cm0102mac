#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static void refill_deflection_sample(Cpu& c, std::uint32_t back) {
    c.put32(c.r(31), c.r(29));
    c.call(0x1077b770, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(c.r(28), c.r(0));
}

static std::int32_t deflection_high(std::uint32_t sample) {
    return std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
}

static std::uint32_t deflection_shift(Cpu& c, std::int32_t value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
    return std::uint32_t(value >> bits);
}

static void draw_deflection_traits(Cpu& c, std::uint32_t blocker) {
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(30)));
    c.set(3, c.r(3) + 2);
    c.set(13, c.u8(blocker + 0x10f));
    c.set(14, c.u8(blocker + 0x110));
    c.compare_logical(0, c.r(3), c.r(0));
    c.put32(c.r(31), c.r(3));
    if (c.cr(0) & 4) refill_deflection_sample(c, 0x1025a240);

    c.set(5, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(28)));
    c.set(3, c.r(3) + 2);
    c.set(0, c.u32(c.r(30)));
    c.set(5, c.r(6) + c.r(5));
    c.put32(c.r(31), c.r(3));
    const auto first_high = deflection_high(c.r(5));
    c.set(4, std::uint32_t(first_high));
    c.compare_logical(0, c.r(3), c.r(0));
    const auto first_quotient = deflection_shift(c, first_high, 2);
    c.set(3, first_quotient >> 31);
    c.set(0, (first_quotient + c.r(3)) * 10);
    c.set(16, c.r(5) - c.r(0));
    if (c.cr(0) & 4) refill_deflection_sample(c, 0x1025a298);

    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(7, c.u16(c.r(3)));
    c.set(0, std::uint32_t(std::int8_t(c.r(13))));
    c.set(3, c.r(0) ^ c.r(16));
    c.set(6, std::uint32_t(std::int8_t(c.r(14))));
    c.set(7, c.r(7) + c.r(5));
    const auto second_high = deflection_high(c.r(7));
    c.set(0, c.r(3) & c.r(0));
    const auto second_quotient = deflection_shift(c, second_high, 2);
    c.set(5, second_quotient >> 31);
    c.set(17, c.r(7) - (second_quotient + c.r(5)) * 10);
    c.set(4, c.r(6) ^ c.r(17));
    c.set(5, deflection_shift(c, std::int32_t(c.r(4)), 1));
    c.set(3, deflection_shift(c, std::int32_t(c.r(3)), 1));
    c.set(4, c.r(4) & c.r(6));
    const auto first_sign = (c.r(3) - c.r(0)) >> 31;
    c.set(3, (c.r(5) - c.r(4)) >> 31);
    c.s.wr<1>(reg::xer_ca, 1);
    const auto bound = 3 - first_sign - c.r(3);
    c.set(0, bound);
    c.compare(0, std::int32_t(bound), 0);
}

static std::uint32_t deflection_bound(Cpu& c, std::int32_t against) {
    const auto second = std::uint32_t(std::int8_t(c.r(14)));
    const auto first = std::uint32_t(std::int8_t(c.r(13)));
    c.set(6, second);
    c.set(5, first);
    const auto second_xor = second ^ c.r(17), first_xor = first ^ c.r(16);
    c.set(7, deflection_shift(c, std::int32_t(second_xor), 1));
    c.set(6, second_xor & second);
    const auto first_half = deflection_shift(c, std::int32_t(first_xor), 1) - (first_xor & first);
    const auto second_half = c.r(7) - c.r(6);
    c.set(3, std::uint32_t(against));
    c.set(4, second_half >> 31);
    c.s.wr<1>(reg::xer_ca, 1);
    const auto bound = 3 - (first_half >> 31) - c.r(4);
    c.set(0, bound);
    c.compare(0, std::int32_t(bound), against);
    return bound;
}

static void finish_deflection_sample(Cpu& c) {
    const auto second = std::uint32_t(std::int8_t(c.r(14)));
    const auto first = std::uint32_t(std::int8_t(c.r(13)));
    c.set({{6, second}, {4, first}, {3, second ^ c.r(17)}});
    c.set(8, c.u32(c.r(31)));
    const auto first_xor = first ^ c.r(16), second_xor = c.r(3);
    c.set(5, second);
    c.set(7, deflection_shift(c, std::int32_t(second_xor), 1));
    c.set(6, second_xor & second);
    const auto first_half = deflection_shift(c, std::int32_t(first_xor), 1) - (first_xor & first);
    c.set(3, c.r(7) - c.r(6));
    c.set(0, first_half >> 31);
    c.set(7, c.u16(c.r(8)));
    c.set(4, c.u32(c.r(28)));
    c.set(6, c.r(3) >> 31);
    c.s.wr<1>(reg::xer_ca, 1);
    c.set(0, 3 - c.r(0));
    c.set(3, second_xor);
    c.set(9, c.r(7) + c.r(4));
    c.set(4, first);
    const auto bound = c.r(0) - c.r(6);
    c.set(8, bound);
    c.set(6, deflection_shift(c, std::int32_t(second_xor), 1));
    c.set(7, std::uint32_t(divide_signed<4>(c.r(9), bound)));
    c.set(5, second_xor & second);
    c.set(3, deflection_shift(c, std::int32_t(first_xor), 1));
    c.set(4, c.r(6) - c.r(5));
    c.set(5, c.r(7) * bound);
    c.set(3, c.r(4) >> 31);
    c.s.wr<1>(reg::xer_ca, 1);
    c.set(18, c.r(9) - c.r(5));
    c.compare(0, std::int32_t(bound), 0);
    // This bound is positive, so the original remainder is kept as-is.
    c.set(0, c.r(18));
}

void sample_deflected_shot(Cpu& c) {
    const auto blocker = c.r(22);
    draw_deflection_traits(c, blocker);
    // The bound is 3 minus two sign bits, so it is always in [1, 3].
    // The original zero and wide-sample paths are therefore unreachable.
    advance_shot_stream(c, 0x1025a334);
    deflection_bound(c, 65535);
    deflection_bound(c, -65535);
    finish_deflection_sample(c);
    c.put8(blocker + 0x2c, c.r(18));
    c.set(13, 1);
    c.s.pc = 0x1025a720;
}
}

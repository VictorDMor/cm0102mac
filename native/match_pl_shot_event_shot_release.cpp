#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool release_value_equals(Cpu& c, std::int32_t value, std::int32_t target = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, target);
    return (c.cr(0) & 2) != 0;
}

static void release_shot_ball(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    const bool alternate = release_value_equals(c, c.u8(stack + 0x1cf), 0x35);
    const auto power = std::uint32_t(std::int16_t(c.r(16)));
    if (alternate) {
        c.set(4, power);
        c.set(0, c.u8(actor + 0x27));
        c.set(3, 0u - c.r(4));
        c.set(5, power);
        const auto mask = std::int32_t(c.r(3) & ~c.r(4));
        c.set(3, std::uint32_t(mask));
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.s.wr<1>(reg::xer_ca, mask < 0 && (std::uint32_t(mask) & 0x7fffffff));
        c.set(6, std::uint32_t(mask >> 31));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, c.r(0) == 0 ? 32 : 0);
        c.set(4, c.u8(actor + 0x105));
        c.set(6, c.r(5) & c.r(6));
        c.set(5, c.u8(actor + 0x106));
        c.set(0, c.r(0) >> 5);
        c.set(8, c.r(20));
        c.set(6, std::uint32_t(std::int16_t(c.r(6))));
        c.set({{9, 1}, {7, c.r(0)}});
    } else {
        c.set(0, c.u8(actor + 0x27));
        c.set({{3, 0}, {6, 0}, {4, power}});
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.set(5, power);
        c.set(0, c.r(0) == 0);
        c.set(3, power);
        c.s.wr<1>(reg::xer_ca, 1);
        c.set(4, power >> 31);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(6, 0u - c.r(4));
        c.s.wr<1>(reg::xer_ca, c.r(4) == 0);
        c.set(4, c.u8(actor + 0x105));
        c.set(6, c.r(5) & ~c.r(6));
        c.set(5, c.u8(actor + 0x106));
        c.set({{8, c.r(20)}, {7, c.r(0)}, {6, std::uint32_t(std::int16_t(c.r(6)))}, {9, 0}});
    }
    c.call(0x101ef9a0, alternate ? 0x1025af94 : 0x1025aff0);
    c.put8(stack + 0x128, c.r(3));
}

static void announce_alternate_release(Cpu& c, std::uint32_t actor) {
    const bool corner = release_value_equals(c, std::int8_t(c.r(20)), 5);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(10, std::uint32_t(std::int8_t(c.r(20))));
    c.set(6, c.u32(actor + 0x1a4));
    c.set(4, corner ? 0x1fd4 : 0x1fd3);
    c.set(5, c.r(3) + 0x10000);
    c.set(7, c.u32(actor + 0x1a4));
    c.set(3, c.r(6) + 0x10000);
    c.set(5, c.u32(c.r(5) - 0x1fd4));
    c.set(6, c.u32(c.r(3) - 0x1fd4));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(7) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(7, c.u8(c.r(5) + 0x27));
    c.set(6, c.u8(c.r(6) + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, corner ? 0x1025b084 : 0x1025b0c8);
}

static std::uint32_t release_high_product(std::uint32_t sample) {
    return std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x10624dd3) >> 32);
}

static std::uint32_t release_quotient(Cpu& c, std::uint32_t high, unsigned shift) {
    c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    return std::uint32_t(std::int32_t(high) >> shift);
}

static void sample_alternate_release(Cpu& c, std::uint32_t actor) {
    advance_shot_stream(c, 0x1025b0e8);
    c.set(5, c.u32(c.r(31)));
    c.set(4, 0x10620000);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x10624dd3);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(28)));
    c.set(3, c.r(3) + 2);
    c.set(0, c.u32(c.r(30)));
    c.set(5, c.r(6) + c.r(5));
    c.put32(c.r(31), c.r(3));
    c.set(4, release_high_product(c.r(5)));
    c.compare_logical(0, c.r(3), c.r(0));
    const auto shifted = release_quotient(c, c.r(4), 6);
    c.set(3, shifted >> 31);
    c.set(0, (shifted + c.r(3)) * 1000);
    c.set(14, c.r(5) - c.r(0));
    if (c.cr(0) & 4) {
        c.put32(c.r(31), c.r(29));
        c.call(0x1077b770, 0x1025b140);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(28), c.r(0));
    }

    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x10620000);
    c.set(5, c.u32(c.r(28)));
    c.set(0, 0);
    c.set(6, c.u16(c.r(3)));
    c.set(4, 0x10624dd3);
    c.set(3, c.r(16) + c.r(14));
    c.set(5, c.r(6) + c.r(5));
    c.put16(actor + 0x3a, 0);
    const auto second = release_quotient(c, release_high_product(c.r(5)), 7);
    c.set(4, second >> 31);
    const auto remainder = c.r(5) - (second + c.r(4)) * 2000;
    c.set(0, c.r(3) - remainder);
    c.set(16, std::uint32_t(std::int16_t(c.r(0))));
}

void prepare_shot_release(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{3, UINT32_MAX}, {0, 0}, {4, 0}});
    c.put8(c.r(5) + 0x7924, c.r(3));
    c.set(3, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.put8(c.r(5) + 0x1c0, 0);
    c.set(0, std::uint32_t(c.i16(actor + 0x2a)));
    c.put16(actor + 0x2a, 0);
    c.put16(actor + 0x1a0, 0);
    c.put16(stack + 0x12c, c.r(0));

    if (!release_value_equals(c, c.u8(stack + 0x1cf), 0x33) &&
        !release_value_equals(c, std::int8_t(c.r(17)), 4)) {
        release_shot_ball(c, actor, stack);
        c.set(3, c.u32(actor + 0x1a4));
        if (!release_value_equals(c, c.i8(c.r(3) + 0x7928))) {
            c.s.pc = 0x1026a190;
            return;
        }
    }

    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x2000));
    c.compare_logical(0, c.r(0), actor);
    if (c.cr(0) & 2) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, 0);
        c.set(3, c.r(3) + 0x10000);
        c.put32(c.r(3) - 0x2000, 0);
    }
    if (release_value_equals(c, c.u8(stack + 0x128), 2)) {
        announce_alternate_release(c, actor);
        sample_alternate_release(c, actor);
    }
    c.s.pc = 0x1025b18c;
}
}

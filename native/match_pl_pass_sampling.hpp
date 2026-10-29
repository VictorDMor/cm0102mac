#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
// The two pass completion paths share the same ordered distance-weighted draw.
// Reuse arithmetic without adding a guest frame or changing caller stack slots.
inline void sample_interception_distance(guest::Cpu& c, guest::Frame& frame,
    unsigned draw, unsigned slot, std::uint32_t singles, std::uint32_t doubles,
    std::uint32_t seed, std::uint32_t begin, std::uint32_t end, std::uint32_t cursor,
    std::uint32_t random_back, std::uint32_t correction_back) {
    using namespace guest; Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance = [&] {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, random_back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto bound = [&](unsigned destination, unsigned coefficient) {
        c.set_f(destination, fp.fmuls(c.f(31), c.f(30))); c.set_f(coefficient, c.f32(singles + 0x2c));
        c.set_f(destination, fp.fmuls(c.f(coefficient), c.f(destination))); return c.f(destination);
    };
    bound(1, 2); c.set_f(0, c.f32(singles)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) c.set(draw, 0);
    else {
        advance(); bound(0, 1); c.set(3, 65535); c.put64(frame.at(slot + 0x30), fp.fctiwz(0, c.f(0)));
        bool narrow = compare(c, c.i32(frame.at(slot + 0x34)), 65535) <= 0;
        if (narrow) {
            bound(0, 1); c.set(3, std::uint32_t(-65535)); c.put64(frame.at(slot + 0x30), fp.fctiwz(0, c.f(0)));
            narrow = compare(c, c.i32(frame.at(slot + 0x34)), -65535) >= 0;
        }
        if (narrow) {
            c.set_f(2, fp.fmuls(c.f(31), c.f(30))); c.set_f(3, c.f32(singles + 0x2c)); c.set(3, c.u32(cursor));
            c.set_f(0, fp.fmuls(c.f(31), c.f(30))); c.set_f(1, c.f32(singles + 0x2c));
            c.set_f(2, fp.fmuls(c.f(3), c.f(2))); c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
            c.set(3, c.u16(c.r(3))); c.set(5, c.r(3) + c.u32(seed));
            const auto first_tag = fp.fctiwz(1, c.f(2)), second_tag = fp.fctiwz(0, c.f(0));
            c.put64(frame.at(slot + 0x0), first_tag); c.set(4, c.u32(frame.at(slot + 0x4))); c.put64(frame.at(slot + 0x8), second_tag);
            const auto quotient = c.r(4) == 0 ? 0u : std::uint32_t(divide_signed<4>(c.r(5), c.r(4)));
            c.set(3, quotient); const auto sign_bound = c.i32(frame.at(slot + 0xc)); compare(c, sign_bound);
            const auto residual = c.r(5) - quotient * c.r(4); c.set(draw, sign_bound > 0 ? residual : 0u - residual);
        } else {
            bound(0, 1); c.set(7, c.u32(cursor)); c.set(4, 0x43300000); c.put32(frame.at(slot + 0x28), c.r(4));
            c.set(6, 0x80008001); c.set(5, 65535); c.set(3, 0x80008001); c.set(7, c.u16(c.r(7)));
            const auto first_tag = fp.fctiwz(1, c.f(0)); c.set(4, c.u32(seed)); c.put32(frame.at(slot + 0x20), 0x43300000); c.set(7, c.r(7) + c.r(4));
            c.set_f(4, c.f64(doubles + 0x80)); c.put64(frame.at(slot + 0x30), first_tag); c.set(6, std::uint32_t(high_product(c.r(7), std::int32_t(0x80008001))));
            c.set_f(2, c.f64(doubles)); bound(0, 1); c.set(4, c.u32(frame.at(slot + 0x34))); c.put32(frame.at(slot + 0x2c), c.r(4) ^ 0x80000000);
            const auto draw_high = std::int32_t(c.r(6) + c.r(7)); const auto draw_sign = shifted_sign(draw_high, 15);
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(3, c.f64(frame.at(slot + 0x28))); const auto second_tag = fp.fctiwz(0, c.f(0));
            c.set_f(3, fp.fsub(c.f(3), c.f(4))); c.set(4, draw_sign); c.put64(frame.at(slot + 0x10), second_tag);
            c.set_f(2, fp.fdiv(c.f(3), c.f(2))); c.set(draw, c.r(7) - std::uint32_t(std::int32_t(c.r(7)) / 65535) * 65535u);
            c.set(4, c.r(draw) ^ 0x80000000); c.put32(frame.at(slot + 0x24), c.r(4));
            const auto reduced_word = c.u32(frame.at(slot + 0x14)) * 10; c.set_f(0, fp.fsub(c.f64(frame.at(slot + 0x20)), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
            const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
            c.set(3, shifted_sign(reduced_high, 15)); c.set(3, std::uint32_t(std::int32_t(reduced_word) / 65535));
            c.put64(frame.at(slot + 0x18), fp.fctiwz(0, c.f(0))); c.set(draw, c.u32(frame.at(slot + 0x1c)));
            enter(c, correction_back, {c.r(3)}); sample_range(c, c.r(3));
            const auto numerator = c.r(3) + 5; bound(0, 1); c.set(4, 0x66666667);
            const auto correction_high = high_product(numerator, 0x66666667); c.put64(frame.at(slot + 0x8), fp.fctiwz(0, c.f(0)));
            c.set(4, shifted_sign(correction_high, 2)); c.set(3, c.u32(frame.at(slot + 0xc))); c.set(draw, c.r(draw) + std::uint32_t(std::int32_t(numerator) / 10));
            if (compare(c, std::int32_t(c.r(draw)), std::int32_t(c.r(3) - 1)) > 0) {
                c.put64(frame.at(slot + 0x0), fp.fctiwz(0, bound(0, 1))); c.set(3, c.u32(frame.at(slot + 0x4))); c.set(draw, c.r(3) - 1);
            }
        }
    }
}
}

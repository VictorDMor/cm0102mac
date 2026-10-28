#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t sample_decision(Cpu& c, std::uint32_t player, std::uint32_t trait_word) {
    Frame frame(c, 0x10289620, 0xb0, {.first_gpr = 24});
    const auto doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c),
               end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    const auto trait = std::int16_t(trait_word), readiness = c.i16(player + 0x3c);
    keep(c, {{24, extended(readiness)}, {26, doubles}, {27, seed}, {28, begin}, {29, end}, {30, cursor}, {31, trait_word}});
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto residual = [](std::uint32_t value, std::int32_t bound) {
        return value - std::uint32_t(divide_signed<4>(value, std::uint32_t(bound))) * std::uint32_t(bound);
    };
    // Both unscaled bounds come from signed16 fields, making their original
    // expanded wide-sampling arms unreachable.
    std::uint32_t readiness_draw = 0;
    if (compare(c, readiness) != 0) {
        advance(0x1028967c); compare(c, readiness, 65535); compare(c, readiness, -65535);
        const auto value = std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed);
        readiness_draw = residual(value, readiness);
        if (compare(c, readiness) <= 0) readiness_draw = 0u - readiness_draw;
    }
    keep(c, {{25, readiness_draw}});
    advance(0x102897d8);
    const auto gate_value = std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed);
    const auto gate_high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(gate_value)) * std::int32_t(0xe5f36cb1)) >> 32);
    const auto gate_partial = std::int32_t(std::uint32_t(gate_high) + gate_value);
    c.s.wr<1>(reg::xer_ca, gate_partial < 0 && (std::uint32_t(gate_partial) & 255));
    leave(c, {{3, std::uint32_t(gate_partial < 0)}, {4, gate_value}});
    if (compare(c, std::int32_t(readiness_draw), std::int32_t(residual(gate_value, 285))) <= 0) {
        std::uint32_t draw = 0;
        if (compare(c, trait) != 0) {
            advance(0x10289bd8); compare(c, trait, 65535); compare(c, trait, -65535);
            const auto at = c.u32(cursor), value = std::uint32_t(c.u16(at)) + c.u32(seed);
            leave(c, {{4, value}, {5, at}});
            draw = residual(value, trait);
            if (compare(c, trait) <= 0) draw = 0u - draw;
        }
        keep(c, {{24, draw}});
        return result(c, extended(std::int16_t(draw)));
    }

    Fpu fp(c);
    const auto conversion = [&](std::uint32_t value, unsigned at) {
        c.put32(frame.at(at), 0x43300000); c.put32(frame.at(at + 4), value ^ 0x80000000);
        return fp.fsub(c.f64(frame.at(at)), c.f64(doubles + 0x80));
    };
    const auto scaled = [&](unsigned at) { return fp.fmul(c.f64(doubles + 0x3e8), conversion(extended(trait), at)); };
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f64(doubles + 0x3e8));
    c.set_f(1, scaled(0x78)); c.set_f(0, c.f64(doubles + 0x88)); fp.fcmp(0, c.f(1), c.f(0));
    std::uint32_t draw = 0;
    if (!(c.cr(0) & 2)) {
        advance(0x10289878);
        const auto bound_at = [&](unsigned input, unsigned output) {
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x3e8));
            c.put64(frame.at(output), fp.fctiwz(0, scaled(input)));
            return c.u32(frame.at(output + 4));
        };
        auto bound = bound_at(0x78, 0x80);
        bool wide = compare(c, std::int32_t(bound), 65535) > 0;
        if (!wide) { bound = bound_at(0x78, 0x80); wide = compare(c, std::int32_t(bound), -65535) < 0; }
        if (!wide) {
            const auto first = scaled(0x38), second_input = conversion(extended(trait), 0x48);
            const auto value = std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed);
            c.put64(frame.at(0x40), fp.fctiwz(2, first)); c.set_f(1, c.f64(doubles + 0x3e8));
            c.put64(frame.at(0x50), fp.fctiwz(0, fp.fmul(c.f(1), second_input)));
            leave(c, {{4, c.u32(frame.at(0x44))}, {5, value}});
            // At 0x10289b28 the quotient only feeds a multiply by its own
            // divisor. The reference's proven zero-divisor lowering therefore
            // leaves the dividend as the residual, even when 0.7*trait truncates
            // to zero after the earlier nonzero floating-point comparison.
            const auto divisor = c.i32(frame.at(0x44));
            draw = divisor == 0 ? value : residual(value, divisor);
            if (compare(c, c.i32(frame.at(0x54))) <= 0) draw = 0u - draw;
        } else {
            c.put64(frame.at(0x80), fp.fctiwz(0, scaled(0x78)));
            const auto at = c.u32(cursor), value = std::uint32_t(c.u16(at)) + c.u32(seed);
            const auto second_input = conversion(extended(trait), 0x60);
            const auto bound_value = conversion(c.u32(frame.at(0x84)), 0x88);
            const auto scale = fp.fdiv(bound_value, c.f64(doubles));
            c.put64(frame.at(0x58), fp.fctiwz(0, fp.fmul(c.f64(doubles + 0x3e8), second_input)));
            const auto converted_residual = conversion(residual(value, 65535), 0x70);
            const auto scaled_residual = fp.fmul(scale, converted_residual);
            c.put64(frame.at(0x68), fp.fctiwz(0, scaled_residual));
            const auto whole = c.u32(frame.at(0x6c)); keep(c, {{24, whole}});
            const auto multiplied = c.u32(frame.at(0x5c)) * 10u;
            const auto reduced = std::int32_t(multiplied) / 65535;
            const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(multiplied)) * std::int32_t(0x80008001)) >> 32);
            const auto partial = std::int32_t(std::uint32_t(high) + multiplied);
            c.s.wr<1>(reg::xer_ca, partial < 0 && (std::uint32_t(partial) & 0x7fff));
            c.set_f(1, converted_residual); c.set_f(2, c.f64(doubles + 0x80));
            c.set_f(3, scale); c.set_f(4, bound_value);
            leave(c, {{3, std::uint32_t(reduced)}, {4, 0x80008001}, {5, 0x43300000},
                      {6, value}, {7, 65535}, {8, 0x80008001}, {9, value - c.u32(seed)}});
            enter(c, 0x10289a20, {std::uint32_t(reduced)}); sample_range(c, std::uint32_t(reduced));
            const auto numerator = std::int32_t(c.r(3) + 5);
            const auto product = std::int32_t(std::uint64_t(std::int64_t(numerator) * 0x66666667) >> 32);
            c.s.wr<1>(reg::xer_ca, product < 0 && (std::uint32_t(product) & 3));
            c.set(4, 0x66666667); draw = whole + std::uint32_t(numerator / 10);
            bound = bound_at(0x50, 0x48);
            if (compare(c, std::int32_t(draw), std::int32_t(bound - 1)) > 0) draw = bound_at(0x40, 0x38) - 1;
        }
    }
    keep(c, {{24, draw}});
    c.set_f(2, conversion(extended(trait), 0x38)); c.set_f(1, c.f64(doubles + 0x80));
    const auto converted_draw = conversion(draw, 0x40); c.set_f(3, c.f64(doubles + 0x20));
    c.put64(frame.at(0x48), fp.fctiwz(0, fp.multiply_add(0x10289b94, c.f(3), c.f(2), converted_draw)));
    leave(c, {{4, 0x43300000}, {5, extended(trait) ^ 0x80000000}});
    return result(c, c.u32(frame.at(0x4c)));
}
}

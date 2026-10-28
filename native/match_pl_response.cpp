#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t accepts_response(Cpu& c, std::uint32_t player, std::uint32_t kind_word, std::uint32_t opponent) {
    Frame frame(c, 0x10286650, 0xc0, {.first_gpr = 24, .gpr_offset = -0x30, .first_fpr = 31});
    const auto doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{24, player}, {25, doubles}, {30, kind_word}, {26, seed}, {27, begin}, {28, end}, {29, cursor}});
    bool same = false;
    if (compare_logical(c, opponent) != 0) same = compare_logical(c, opponent, c.u32(player + 0x1ac)) == 0;
    c.set(3, unsigned(same)); compare(c, c.r(3));
    const auto threshold = same ? 8 : 1; keep(c, {{31, unsigned(threshold)}});
    c.set_f(1, c.f32(player + 0x1a8)); c.set_f(2, c.f64(doubles + 0xc8));
    c.call(0x10780fc0, 0x102866bc); c.set(2, c.u32(frame.at(0x14)));
    Fpu fp(c);
    c.put64(frame.at(0x88), fp.fctiwz(0, c.f(1)));
    const auto power_word = c.u32(frame.at(0x8c)); const auto kind = std::int8_t(kind_word);
    keep(c, {{30, power_word}});
    compare(c, kind, 3);
    if (kind != 3) {
        if (kind > 3) { if (compare(c, kind, 5) > 0) return result(c, 0); }
        else if (compare(c, kind, 1) < 0) return result(c, 0);
    }
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto ordering = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto residual = [](std::uint32_t value, std::int32_t bound) {
        const auto quotient = std::uint32_t(divide_signed<4>(value, std::uint32_t(bound)));
        return value - quotient * std::uint32_t(bound);
    };
    if (kind == 1) {
        // The trait narrows to a signed byte, so this bound is [-17,238].
        // The original's expanded wide-sampling arm cannot be reached.
        c.set_f(31, c.f32(player + 0x1a8));
        c.put64(frame.at(0x88), fp.fctiwz(0, c.f(31)));
        const auto intensity_byte = std::int8_t(c.u32(frame.at(0x8c)));
        auto bound = 110 - intensity_byte;
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(intensity_byte)) <= 110);
        std::uint32_t draw = 0;
        if (compare(c, bound) != 0) {
            advance(0x10286744);
            c.put64(frame.at(0x88), fp.fctiwz(0, c.f(31)));
            compare(c, bound, 65535); compare(c, bound, -65535);
            const auto word = c.u16(c.u32(cursor)); const auto bias = c.u32(seed);
            const auto value = std::uint32_t(word) + bias;
            c.put64(frame.at(0x58), fp.fctiwz(1, c.f(31)));
            c.put64(frame.at(0x60), fp.fctiwz(0, c.f(31)));
            bound = 110 - std::int8_t(c.u32(frame.at(0x5c)));
            const auto compared_intensity = std::int8_t(c.u32(frame.at(0x64)));
            c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(compared_intensity)) <= 110);
            compare(c, bound); draw = residual(value, bound);
            if (bound < 0) draw = 0u - draw;
            leave(c, {{4, std::uint32_t(bound)}, {5, value}});
        }
        keep(c, {{25, draw}});
        return result(c, compare(c, std::int32_t(draw), threshold) < 0);
    }

    // All four policies use the same draw algorithm. Their original conversion
    // slots and return PCs differ; keep that compatibility image explicit.
    struct Layout { unsigned initial, wide_bias, post_input, post_integer, clamp_input, clamp_integer; std::uint32_t reseed, correction; };
    constexpr Layout layouts[]{
        {0x58, 0x20, 0x50, 0x48, 0x40, 0x38, 0x1028698c, 0x10286b24},
        {0x38, 0, 0x70, 0x78, 0x80, 0x88, 0x10286cb4, 0x10286e4c},
        {0x38, 0, 0x70, 0x78, 0x80, 0x88, 0x10286fdc, 0x10287174},
        {0x38, 0, 0x70, 0x78, 0x80, 0x88, 0x10287304, 0x1028749c}};
    const auto layout = layouts[unsigned(kind - 2)];
    const auto coefficient = 0x3b0u + unsigned(kind - 2) * 8;
    const auto conversion = [&](std::uint32_t value, unsigned at) {
        c.put32(frame.at(at), 0x43300000); c.put32(frame.at(at + 4), value ^ 0x80000000);
        return fp.fsub(c.f64(frame.at(at)), c.f64(doubles + 0x80));
    };
    const auto range = [&](unsigned at) {
        return fp.fdiv(c.f64(doubles + coefficient), conversion(power_word, at));
    };
    c.set(3, power_word ^ 0x80000000); c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(3, c.f64(doubles + coefficient)); c.set_f(1, range(layout.initial));
    c.set_f(0, c.f64(doubles + 0x88)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) { keep(c, {{26, 0}}); return result(c, compare(c, 0, threshold) < 0); }
    advance(layout.reseed);
    const auto integer_range = [&](unsigned input, unsigned output) {
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + coefficient));
        c.put64(frame.at(output), fp.fctiwz(0, range(input)));
        return c.u32(frame.at(output + 4));
    };
    auto bound_word = integer_range(layout.initial, layout.initial + 8);
    const auto bound = std::int32_t(bound_word);
    bool wide = compare(c, bound, 65535) > 0;
    if (!wide) {
        bound_word = integer_range(layout.initial, layout.initial + 8);
        wide = compare(c, std::int32_t(bound_word), -65535) < 0;
    }
    std::uint32_t draw;
    if (!wide) {
        const auto at = c.u32(cursor); c.set(5, at);
        const auto first_range = range(0x38);
        c.set_f(3, first_range); c.set_f(2, c.f64(doubles + coefficient));
        const auto second_denominator = conversion(power_word, 0x48);
        const auto value = std::uint32_t(c.u16(at)) + c.u32(seed); c.set(4, value);
        c.put64(frame.at(0x40), fp.fctiwz(1, first_range));
        c.put64(frame.at(0x50), fp.fctiwz(0, fp.fdiv(c.f(2), second_denominator)));
        const auto divisor = c.i32(frame.at(0x44));
        // All four source divisions are proven quotient-free in the generator:
        // a zero integer range leaves the dividend after the multiply/subtract.
        draw = divisor == 0 ? value : residual(value, divisor);
        if (compare(c, c.i32(frame.at(0x54))) <= 0) draw = 0u - draw;
    } else {
        const auto b = layout.wide_bias;
        const auto first_range = range(0x38 + b);
        const auto second_denominator = conversion(power_word, 0x60 + b);
        const auto at = c.u32(cursor); const auto word = c.u16(at); const auto value = std::uint32_t(word) + c.u32(seed);
        c.put64(frame.at(0x40 + b), fp.fctiwz(5, first_range));
        c.put64(frame.at(0x68 + b), fp.fctiwz(0, fp.fdiv(c.f64(doubles + coefficient), second_denominator)));
        const auto scale = fp.fdiv(conversion(c.u32(frame.at(0x44 + b)), 0x48 + b), c.f64(doubles));
        const auto raw = residual(value, 65535);
        const auto scaled_residual = fp.fmul(scale, conversion(raw, 0x50 + b));
        c.put64(frame.at(0x58 + b), fp.fctiwz(0, scaled_residual));
        const auto whole = c.u32(frame.at(0x5c + b)); keep(c, {{26, whole}});
        const auto multiplied = c.u32(frame.at(0x6c + b)) * 10u;
        const auto reduced = std::int32_t(multiplied) / 65535;
        const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(multiplied)) * std::int32_t(0x80008001)) >> 32);
        const auto partial = std::int32_t(std::uint32_t(high) + multiplied);
        c.s.wr<1>(reg::xer_ca, partial < 0 && (std::uint32_t(partial) & 0x7fff));
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, scale); c.set_f(3, c.f64(doubles)); c.set_f(4, c.f64(doubles + 0x80));
        leave(c, {{3, std::uint32_t(reduced)}, {4, 0x80008001}, {5, std::uint32_t(std::int32_t(value) < 0)},
                  {6, value}, {7, 65535}, {8, 0x80008001}, {9, at}});
        enter(c, layout.correction, {std::uint32_t(reduced)}); sample_range(c, std::uint32_t(reduced));
        const auto numerator = std::int32_t(c.r(3) + 5);
        const auto product = std::int32_t(std::uint64_t(std::int64_t(numerator) * 0x66666667) >> 32);
        c.s.wr<1>(reg::xer_ca, product < 0 && (std::uint32_t(product) & 3)); c.set(4, 0x66666667);
        draw = whole + std::uint32_t(numerator / 10);
        bound_word = integer_range(layout.post_input, layout.post_integer);
        if (compare(c, std::int32_t(draw), std::int32_t(bound_word - 1)) > 0)
            draw = integer_range(layout.clamp_input, layout.clamp_integer) - 1;
    }
    keep(c, {{26, draw}});
    return result(c, compare(c, std::int32_t(draw), threshold) < 0);
}
}

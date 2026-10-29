#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

// The temporary candidate list is a guest linked list of owned player copies.
// Keep its explicit normal-exit destruction and its untouched padding bytes.
std::uint32_t choose_primary_action(Cpu& c, std::uint32_t player, std::uint32_t mode) {
    Frame frame(c, 0x10278220, 0x180, {.first_gpr = 13, .gpr_offset = -0x6c, .first_fpr = 28});
    const auto doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c),
               end = c.toc(-0x7b88), cursor = c.toc(-0x7b84), singles = c.toc(-0x51e0);
    keep(c, {{15, player}, {16, mode}, {19, 0}, {20, 0}, {26, doubles}, {27, seed},
             {28, begin}, {29, end}, {30, cursor}, {31, singles}});
    const Player actor{c, player}; Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto uncertainty_draw = [&](std::uint32_t back) {
        c.set(13, c.u8(player + 0x109));
        const auto limit = [&] { return 1000u - std::uint32_t(std::int32_t(std::int8_t(c.r(13))) * 50); };
        const auto carry = [&] { c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(std::int8_t(c.r(13))) * 50) <= 1000); };
        carry(); if (compare(c, std::int32_t(limit())) == 0) { c.set(14, 0); return; }
        advance(back); c.set(3, 65535); carry(); compare(c, std::int32_t(limit()), 65535);
        c.set(3, std::uint32_t(-65535)); carry(); compare(c, std::int32_t(limit()), -65535);
        // 1000-signedbyte*50 lies in[-5350,7400]; only the narrow draw exists.
        c.set(4, c.u32(cursor)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13))) * 50)); c.set(6, c.u16(c.r(4))); c.set(5, c.u32(seed));
        c.set(4, limit()); c.set(5, c.r(6) + c.r(5)); c.set(3, std::uint32_t(divide_signed<4>(c.r(5), c.r(4))) * c.r(4));
        c.set(14, c.r(5) - c.r(3)); carry(); c.set(3, 0u - c.r(14));
        if (compare(c, std::int32_t(limit())) > 0) c.set(3, c.r(14)); c.set(14, c.r(3));
    };
    const auto bounded_draw = [&](unsigned bound_register, unsigned draw_register, unsigned bound_slot,
                                  unsigned residual_slot, unsigned coarse_slot, std::uint32_t advance_back,
                                  std::uint32_t correction_back) {
        const auto limit = [&] { return c.r(bound_register); };
        if (compare(c, std::int32_t(limit())) == 0) { c.set(draw_register, 0); return; }
        advance(advance_back); c.set(3, 65535);
        bool narrow = compare(c, std::int32_t(limit()), 65535) <= 0;
        if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, std::int32_t(limit()), -65535) >= 0; }
        if (narrow) {
            c.set(3, c.u32(cursor)); compare(c, std::int32_t(limit())); c.set(3, sample());
            const auto residual = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), limit())) * limit();
            c.set(draw_register, std::int32_t(limit()) > 0 ? residual : 0u - residual); return;
        }
        c.set(3, limit() ^ 0x80000000); c.put32(frame.at(bound_slot + 4), c.r(3));
        c.set(6, 0x80008001); c.set(7, c.u32(cursor)); c.set(4, 0x43300000); c.put32(frame.at(bound_slot), 0x43300000);
        c.set(5, 65535); c.set(3, 0x80008001); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(frame.at(bound_slot)));
        c.set_f(0, c.f64(doubles)); c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set(8, c.u16(c.r(7))); c.set(7, c.u32(seed));
        c.put32(frame.at(residual_slot), c.r(4)); c.set_f(2, fp.fdiv(c.f(1), c.f(0))); c.set(7, c.r(8) + c.r(7));
        c.set_f(1, c.f64(doubles + 0x80)); const auto value = c.r(7);
        const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value);
        c.set(6, shifted_sign(high, 15)); c.set(draw_register, value - std::uint32_t(std::int32_t(value) / 65535) * 65535u);
        c.set(4, c.r(draw_register) ^ 0x80000000); c.put32(frame.at(residual_slot + 4), c.r(4));
        c.set_f(0, fp.fsub(c.f64(frame.at(residual_slot)), c.f(1))); const auto reduced_word = limit() * 10;
        c.set_f(0, fp.fmul(c.f(2), c.f(0))); const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
        c.set(3, shifted_sign(reduced_high, 15)); const auto reduced = std::uint32_t(std::int32_t(reduced_word) / 65535);
        c.set(3, reduced); c.put64(frame.at(coarse_slot), fp.fctiwz(0, c.f(0))); c.set(draw_register, c.u32(frame.at(coarse_slot + 4)));
        enter(c, correction_back, {reduced}); sample_range(c, c.r(3)); const auto numerator = c.r(3) + 5;
        c.set(4, 0x66666667); const auto correction_high = high_product(numerator, 0x66666667);
        c.set(4, shifted_sign(correction_high, 2)); c.set(3, std::uint32_t(std::int32_t(numerator) / 10)); c.set(draw_register, c.r(draw_register) + c.r(3));
        if (compare(c, std::int32_t(c.r(draw_register)), std::int32_t(limit() - 1)) > 0) c.set(draw_register, limit() - 1);
    };
    const auto finish = [&](std::uint32_t status, std::uint32_t back) {
        leave(c, {{3, frame.at(0x74)}, {4, UINT32_MAX}}); c.set(13, status);
        c.call(0x10738c20, back); return result(c, c.r(13));
    };
    const auto query_action = [&](unsigned action, unsigned kind_offset, std::uint32_t back) {
        c.put32(frame.at(0x38), action);
        leave(c, {{3, actor.match()}, {4, c.u8(actor.match() + 0x791d)}, {5, c.u8(actor.match() + 0x791e)},
                  {6, frame.at(kind_offset)}, {7, frame.at(0x56)}, {8, frame.at(0x55)},
                  {9, frame.at(0x54)}, {10, frame.at(0x68)}}); c.call(0x101e8950, back);
    };
    const auto perform_action = [&](unsigned action, std::uint32_t target, std::uint32_t back) {
        c.put16(player + 0x19c, action);
        leave(c, {{3, player}, {4, action}, {5, target}, {6, c.u8(frame.at(0x56))},
                  {7, c.u8(frame.at(0x55))}, {8, c.u8(frame.at(0x54))}}); c.link(back); complete_targeted_action(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8));
    };
    const auto choose_shot = [&](unsigned kind_offset, std::uint32_t back) {
        leave(c, {{3, player}, {4, frame.at(kind_offset)}, {5, frame.at(0x5c)}}); c.call(0x10252080, back);
        c.set(21, extended(std::int16_t(c.r(3))));
    };
    const auto shoot = [&](unsigned action, unsigned shot_mode, std::uint32_t back) {
        leave(c, {{3, player}, {4, action}, {5, extended(std::int16_t(c.r(21)))},
                  {6, extended(c.i16(frame.at(0x5c)))}, {7, shot_mode}, {8, 0}, {9, 0}});
        c.call(0x102576e0, back);
    };
    const auto shot_bound = [&](unsigned trait_register, unsigned value_slot, unsigned integer_slot,
                                std::uint32_t site, unsigned integer_fpr = 0) {
        const auto trait = std::uint32_t(std::int32_t(std::int8_t(c.r(trait_register)))) * 4;
        c.put32(frame.at(value_slot), 0x43300000); c.put32(frame.at(value_slot + 4), trait ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(value_slot))); c.set_f(2, c.f32(singles + 0x90));
        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.multiply_add(site, c.f(2), c.f(31), c.f(0)));
        c.put64(frame.at(integer_slot), fp.fctiwz(integer_fpr, c.f(0))); return c.u32(frame.at(integer_slot + 4));
    };
    enum class KeeperDraw { running, outside_area, off_line, blocked_line, movement_opening };
    const auto keeper_distance_draw = [&](KeeperDraw kind, std::uint32_t advance_back, std::uint32_t correction_back, unsigned draw_register = 14) {
        const auto value = [&](unsigned destination) {
            if (kind == KeeperDraw::outside_area) c.set_f(destination, c.f(31));
            else {
                const auto offset = kind == KeeperDraw::movement_opening ? 0xd0u : kind == KeeperDraw::running ? 0xf0u : kind == KeeperDraw::off_line ? 0xe4u : 0x5cu;
                c.set_f(0, c.f32(singles + offset)); c.set_f(destination, fp.fmuls(c.f(0), c.f(31)));
                if (kind == KeeperDraw::off_line) c.set_f(destination, fp.fmuls(c.f(destination), c.f(31)));
            }
        };
        advance(advance_back); c.set(3, 65535); value(0); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0)));
        bool narrow = compare(c, c.i32(frame.at(0xa8)), 65535) <= 0;
        if (narrow) {
            c.set(3, std::uint32_t(-65535)); value(0); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0)));
            narrow = compare(c, c.i32(frame.at(0xa8)), -65535) >= 0;
        }
        if (narrow) {
            value(1); value(0); c.set(3, c.u32(cursor)); const auto draw = sample();
            const auto first_tag = fp.fctiwz(1, c.f(1)), second_tag = fp.fctiwz(0, c.f(0)); c.set(5, draw);
            c.put64(frame.at(0xa4), first_tag); c.set(4, c.u32(frame.at(0xa8))); c.put64(frame.at(0xac), second_tag);
            const auto divisor = c.r(4), quotient = divisor == 0 ? 0u : std::uint32_t(divide_signed<4>(draw, divisor)); c.set(3, quotient);
            const auto sign_bound = c.i32(frame.at(0xb0)); compare(c, sign_bound); const auto residual = draw - quotient * divisor;
            c.set(draw_register, sign_bound > 0 ? residual : 0u - residual); return;
        }
        value(1); value(0); const auto first_tag = fp.fctiwz(1, c.f(1)), second_tag = fp.fctiwz(0, c.f(0));
        const auto word = c.u16(c.u32(cursor)); const auto draw = std::uint32_t(word) + c.u32(seed);
        c.put64(frame.at(0xa4), first_tag); c.put64(frame.at(0xc4), second_tag);
        c.put32(frame.at(0xac), 0x43300000); c.put32(frame.at(0xb0), c.u32(frame.at(0xa8)) ^ 0x80000000);
        c.put32(frame.at(0xb4), 0x43300000);
        if (kind == KeeperDraw::outside_area) {
            c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, c.f64(doubles)); c.set_f(1, c.f64(frame.at(0xac)));
            c.set_f(2, fp.fsub(c.f(1), c.f(2))); c.set_f(2, fp.fdiv(c.f(2), c.f(0)));
        } else {
            c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles)); c.set_f(0, c.f64(frame.at(0xac)));
            c.set_f(0, fp.fsub(c.f(0), c.f(3))); c.set_f(2, fp.fdiv(c.f(0), c.f(2)));
        }
        c.set_f(1, c.f64(doubles + 0x80)); const auto quotient = std::int32_t(draw) / 65535;
        const auto residual = draw - std::uint32_t(quotient) * 65535u; c.set(draw_register, residual);
        c.put32(frame.at(0xb8), residual ^ 0x80000000); c.set_f(0, fp.fsub(c.f64(frame.at(0xb4)), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0))); const auto reduced_word = c.u32(frame.at(0xc8)) * 10;
        const auto draw_high = std::int32_t(std::uint32_t(high_product(draw, std::int32_t(0x80008001))) + draw);
        const auto draw_sign = shifted_sign(draw_high, 15);
        const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
        const auto reduced_sign = shifted_sign(reduced_high, 15), reduced = std::uint32_t(std::int32_t(reduced_word) / 65535);
        if (kind == KeeperDraw::running || kind == KeeperDraw::movement_opening) leave(c, {{4, 0x80008001}, {5, std::uint32_t(quotient) * 65535}, {6, 65535}, {7, draw_sign}, {8, word}});
        else if (kind == KeeperDraw::outside_area) leave(c, {{4, reduced_sign}, {5, std::uint32_t(quotient) * 65535}, {6, 65535}, {7, 0x80008001}, {8, word}});
        else {
            leave(c, {{4, residual ^ 0x80000000}, {5, 65535}, {6, draw_sign}, {7, draw}});
            if (kind == KeeperDraw::blocked_line) c.set(8, word);
        }
        c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); c.set(draw_register, c.u32(frame.at(0xc0)));
        enter(c, correction_back, {reduced}); sample_range(c, c.r(3));
        const auto numerator = c.r(3) + 5; const auto high = high_product(numerator, 0x66666667);
        const auto sign = shifted_sign(high, 2); c.set(4, kind == KeeperDraw::outside_area ? std::uint32_t(high >> 2) : kind == KeeperDraw::off_line ? 0x66666667u : sign);
        value(0); c.put64(frame.at(0xcc), fp.fctiwz(0, c.f(0))); const auto bound = c.u32(frame.at(0xd0));
        c.set(draw_register, c.r(draw_register) + std::uint32_t(std::int32_t(numerator) / 10));
        c.set(3, kind == KeeperDraw::outside_area ? std::uint32_t(std::int32_t(numerator) / 10) : bound);
        if (compare(c, std::int32_t(c.r(draw_register)), std::int32_t(bound - 1)) > 0) {
            value(0); c.put64(frame.at(0xd4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(frame.at(0xd8))); c.set(draw_register, c.r(3) - 1);
        }
    };
    const auto pressure_draw = [&] {
        const auto bound = [&](std::uint32_t site) {
            c.set_f(1, c.f32(singles + 0xb0)); c.set_f(0, c.f32(singles + 0x14c));
            c.set_f(0, fp.multiply_add(site, c.f(1), c.f(29), c.f(0))); return c.f(0);
        };
        advance(0x1027bc00); c.set(3, 65535); c.put64(frame.at(0xa4), fp.fctiwz(0, bound(0x1027bc18)));
        bool narrow = compare(c, c.i32(frame.at(0xa8)), 65535) <= 0;
        if (narrow) { c.set(3, std::uint32_t(-65535)); c.put64(frame.at(0xa4), fp.fctiwz(0, bound(0x1027bc40))); narrow = compare(c, c.i32(frame.at(0xa8)), -65535) >= 0; }
        if (narrow) {
            bound(0x1027bda8); c.set(3, c.u32(cursor)); const auto value = sample(); c.set_f(1, c.f32(singles + 0xb0));
            const auto first_tag = fp.fctiwz(2, c.f(0)); c.set_f(0, c.f32(singles + 0x14c)); c.set(5, value);
            c.set_f(0, fp.multiply_add(0x1027bdc4, c.f(1), c.f(29), c.f(0))); c.put64(frame.at(0xa4), first_tag); c.set(4, c.u32(frame.at(0xa8)));
            const auto second_tag = fp.fctiwz(0, c.f(0)); const auto divisor = c.r(4);
            c.set(3, divisor == 0 ? 0u : std::uint32_t(divide_signed<4>(value, divisor))); c.put64(frame.at(0xac), second_tag);
            const auto sign_bound = c.i32(frame.at(0xb0)); compare(c, sign_bound); const auto residual = value - c.r(3) * divisor;
            c.set(13, sign_bound > 0 ? residual : 0u - residual); return;
        }
        const auto value = sample(); c.put32(frame.at(0xac), 0x43300000);
        const auto first_tag = fp.fctiwz(0, bound(0x1027bc70)); c.set_f(4, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles)); c.put64(frame.at(0xa4), first_tag);
        const auto first_integer = c.u32(frame.at(0xa8)); c.set(5, 65535); c.set(7, value);
        const auto raw_high = high_product(value, std::int32_t(0x80008001)); c.set(6, std::uint32_t(raw_high));
        c.put32(frame.at(0xb4), 0x43300000); c.put32(frame.at(0xb0), first_integer ^ 0x80000000);
        const auto value_high = std::int32_t(std::uint32_t(raw_high) + value); shifted_sign(value_high, 15);
        c.set_f(3, c.f64(frame.at(0xac))); const auto second_value = bound(0x1027bcd4);
        c.set_f(3, fp.fsub(c.f(3), c.f(4))); c.set_f(1, c.f64(doubles + 0x80)); const auto second_tag = fp.fctiwz(0, second_value);
        c.set_f(2, fp.fdiv(c.f(3), c.f(2))); c.set(13, value - std::uint32_t(std::int32_t(value) / 65535) * 65535u);
        c.put64(frame.at(0xc4), second_tag); c.set(4, c.r(13) ^ 0x80000000); c.put32(frame.at(0xb8), c.r(4));
        const auto reduced_word = c.u32(frame.at(0xc8)) * 10; c.set_f(0, fp.fsub(c.f64(frame.at(0xb4)), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); c.set(13, c.u32(frame.at(0xc0)));
        const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word); shifted_sign(reduced_high, 15);
        enter(c, 0x1027bd38, {std::uint32_t(std::int32_t(reduced_word) / 65535)}); sample_range(c, c.r(3));
        const auto numerator = c.r(3) + 5; const auto correction_high = high_product(numerator, 0x66666667);
        bound(0x1027bd4c); c.set(4, shifted_sign(correction_high, 2)); c.put64(frame.at(0xcc), fp.fctiwz(0, c.f(0)));
        c.set(3, c.u32(frame.at(0xd0))); c.set(13, c.r(13) + std::uint32_t(std::int32_t(numerator) / 10));
        if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(3) - 1)) > 0) {
            c.put64(frame.at(0xd4), fp.fctiwz(0, bound(0x1027bd84))); c.set(3, c.u32(frame.at(0xd8))); c.set(13, c.r(3) - 1);
        }
    };
    // Both source role arms use the same ordered, single-precision polynomial;
    // only their call continuations and final trait reward differ.
    const auto polynomial_value = [&](bool first_layout, unsigned slot) {
        const auto support = std::uint32_t(std::int32_t(std::int8_t(c.r(17))) + 1);
        c.set_f(0, c.f32(singles + 0x34)); c.set_f(0, fp.fadds(c.f(0), c.f(31)));
        c.put32(frame.at(slot + 4), support ^ 0x80000000);
        if (first_layout) c.set_f(3, c.f64(doubles + 0x80)); else c.set_f(2, c.f64(doubles + 0x80));
        c.set_f(0, fp.fmuls(c.f(0), c.f(31))); c.put32(frame.at(slot), 0x43300000);
        if (first_layout) {
            c.set_f(4, c.f32(singles + 0x164)); c.set_f(2, c.f64(frame.at(slot))); c.set_f(1, fp.fmuls(c.f(31), c.f(0)));
            c.set_f(2, fp.fsubs(c.f(2), c.f(3))); c.set_f(1, fp.fmuls(c.f(31), c.f(1)));
            c.set_f(1, fp.fmuls(c.f(2), c.f(1))); c.set_f(1, fp.fmuls(c.f(4), c.f(1))); return c.f(1);
        }
        c.set_f(3, c.f32(singles + 0x164)); c.set_f(1, c.f64(frame.at(slot))); c.set_f(0, fp.fmuls(c.f(31), c.f(0)));
        c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.fmuls(c.f(31), c.f(0)));
        c.set_f(0, fp.fmuls(c.f(1), c.f(0))); c.set_f(0, fp.fmuls(c.f(3), c.f(0))); return c.f(0);
    };
    const auto polynomial_draw = [&](bool special_role) {
        c.set(3, (std::uint32_t(std::int32_t(std::int8_t(c.r(17))) + 1)) ^ 0x80000000);
        polynomial_value(true, 0xa4); c.set_f(0, c.f32(singles)); c.set(13, c.u8(player + 0x10c)); fp.fcmp(0, c.f(1), c.f(0));
        if (c.cr(0) & 2) { c.set(14, 0); return; }
        advance(special_role ? 0x1027cc20 : 0x1027d06c); c.set(3, 65535);
        c.set(4, (std::uint32_t(std::int32_t(std::int8_t(c.r(17))) + 1)) ^ 0x80000000);
        c.put64(frame.at(0xac), fp.fctiwz(0, polynomial_value(false, 0xa4)));
        bool narrow = compare(c, c.i32(frame.at(0xb0)), 65535) <= 0;
        if (narrow) { c.set(3, std::uint32_t(-65535)); c.put64(frame.at(0xac), fp.fctiwz(0, polynomial_value(false, 0xa4))); narrow = compare(c, c.i32(frame.at(0xb0)), -65535) >= 0; }
        const auto support_bits = (std::uint32_t(std::int32_t(std::int8_t(c.r(17))) + 1)) ^ 0x80000000;
        if (narrow) {
            const auto first_value = polynomial_value(true, 0xa4); c.set(6, c.u32(cursor)); const auto value = sample();
            c.set(5, value); const auto first_tag = fp.fctiwz(4, first_value); c.put64(frame.at(0xac), first_tag);
            const auto first_integer = c.u32(frame.at(0xb0)); polynomial_value(false, 0xb4);
            c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); c.set(4, first_integer);
            c.set(3, first_integer == 0 ? 0u : std::uint32_t(divide_signed<4>(value, first_integer)));
            const auto sign_bound = c.i32(frame.at(0xc0)); compare(c, sign_bound); const auto residual = value - c.r(3) * first_integer;
            c.set(14, sign_bound > 0 ? residual : 0u - residual); return;
        }
        const auto first_value = polynomial_value(true, 0xa4), second_value = polynomial_value(false, 0xcc);
        const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
        c.put32(frame.at(0xb4), 0x43300000); c.put64(frame.at(0xac), fp.fctiwz(1, first_value));
        const auto first_integer = c.u32(frame.at(0xb0)); c.put32(frame.at(0xb8), first_integer ^ 0x80000000);
        c.put32(frame.at(0xbc), 0x43300000); c.set_f(7, c.f64(doubles + 0x80)); c.set_f(5, c.f64(doubles)); c.set_f(6, c.f64(frame.at(0xb4)));
        c.set_f(2, fp.fsub(c.f(6), c.f(7))); c.set_f(3, c.f32(singles + 0x164)); c.set_f(4, c.f64(doubles + 0x80));
        c.set_f(2, fp.fdiv(c.f(2), c.f(5))); const auto value_high_raw = high_product(value, std::int32_t(0x80008001));
        const auto value_high = std::int32_t(std::uint32_t(value_high_raw) + value), value_quotient = std::int32_t(value) / 65535;
        c.set(14, value - std::uint32_t(value_quotient) * 65535u); c.put32(frame.at(0xc0), c.r(14) ^ 0x80000000);
        const auto second_tag = fp.fctiwz(0, second_value); c.set_f(1, fp.fsub(c.f64(frame.at(0xbc)), c.f(4))); c.put64(frame.at(0xd4), second_tag);
        c.set_f(1, fp.fmul(c.f(2), c.f(1))); const auto reduced_word = c.u32(frame.at(0xd8)) * 10;
        c.put64(frame.at(0xc4), fp.fctiwz(0, c.f(1))); c.set(14, c.u32(frame.at(0xc8)));
        leave(c, {{4, 0x80008001}, {5, 0x43300000}, {6, shifted_sign(value_high, 15)}, {7, 65535}, {8, std::uint32_t(value_high_raw)}, {9, word}});
        const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word); shifted_sign(reduced_high, 15);
        enter(c, special_role ? 0x1027ce38 : 0x1027d284, {std::uint32_t(std::int32_t(reduced_word) / 65535)}); sample_range(c, c.r(3));
        const auto numerator = c.r(3) + 5; const auto high = high_product(numerator, 0x66666667);
        c.set(4, support_bits); c.set(5, 0x66666667); polynomial_value(false, 0xdc); c.set(3, shifted_sign(high, 2));
        c.set(14, c.r(14) + std::uint32_t(std::int32_t(numerator) / 10)); c.put64(frame.at(0xe4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(frame.at(0xe8)));
        if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(3) - 1)) > 0) {
            c.set(3, support_bits); c.put64(frame.at(0xf4), fp.fctiwz(0, polynomial_value(false, 0xec)));
            c.set(3, c.u32(frame.at(0xf8))); c.set(14, c.r(3) - 1);
        }
    };

    const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
    c.set(3, std::uint32_t(std::int32_t(actor.column()) - 4)); c.call(0x10775540, first_side ? 0x1027827c : 0x102782c0);
    c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    c.set(3, std::uint32_t(std::int32_t(actor.row()) - (first_side ? 11 : 0))); c.call(0x10775540, first_side ? 0x10278290 : 0x102782d4);
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    const auto distance_offset = c.r(3) * 36 + c.r(13) * 4;
    c.set(3, c.toc(-0x6424)); c.set_f(0, c.f32(c.r(3) + distance_offset));
    c.put16(frame.at(0x58), 0); c.set(5, 0); c.set(4, UINT32_MAX); c.set_f(31, c.f(0));
    c.put8(frame.at(0x102), 0); c.set_f(30, c.f32(singles)); c.put8(frame.at(0x56), 0);
    c.put8(frame.at(0x101), 0); c.put8(frame.at(0x51), UINT32_MAX); c.set(3, frame.at(0x74));
    c.call(0x10738c00, 0x10278328);
    c.set(3, actor.match()); const auto restart = std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x7928)));
    compare_logical(c, restart, 8);
    switch (restart) {
    case 8: c.set(3, actor.match()); c.put8(c.r(3) + 0x7928, 0); break;
    case 3: {
        c.set(3, 0); c.put8(actor.match() + 0x7928, 0); query_action(0x34, 0x46, 0x102783a0);
        c.set(22, c.r(3));
        if (compare(c, c.r(22)) == 0) {
            advance(0x102783c8); const auto value = sample();
            leave(c, {{3, std::uint32_t(high_product(value, 0x55555556))}, {4, value}});
            c.put8(frame.at(0x55), 5u - std::uint32_t(std::int32_t(value) % 3));
            if (compare(c, actor.side(), 1) == 0) c.put8(frame.at(0x54), 0);
            else if (compare(c, std::int8_t(actor.side())) == 0) c.put8(frame.at(0x54), 11);
        }
        perform_action(0x34, c.r(22), 0x10278454); return finish(0, 0x10278464);
    }
    case 2: {
        c.set(3, 0); c.put8(actor.match() + 0x7928, 0); query_action(0x36, 0x45, 0x102784ac);
        c.set(13, c.r(3));
        if (compare(c, c.r(13)) == 0) {
            c.set(3, 1); c.set(4, c.u8(actor.match() + 0x791e));
            if (compare(c, actor.side(), 1) == 0) c.set(3, UINT32_MAX);
            c.put8(frame.at(0x54), c.r(3) + c.r(4));
            if (compare(c, c.i8(frame.at(0x54))) < 0) c.put8(frame.at(0x54), 0);
            if (compare(c, c.i8(frame.at(0x54)), 11) > 0) c.put8(frame.at(0x54), 11);
            c.set(3, actor.match()); const bool left_edge = compare(c, c.i8(c.r(3) + 0x791d)) == 0;
            advance(left_edge ? 0x10278534 : 0x10278584); const auto value = sample(); c.set(3, value >> 31);
            const auto residual = std::uint32_t(std::int32_t(value) % 4);
            c.put8(frame.at(0x55), left_edge ? residual : 8u - residual);
        }
        perform_action(0x36, c.r(13), 0x102785dc); return finish(0, 0x102785ec);
    }
    case 4: {
        c.set(3, 0); c.put8(actor.match() + 0x7928, 0); query_action(0x5a, 0x44, 0x10278634);
        c.set(5, c.r(3));
        bool fallback = compare(c, c.r(5)) == 0;
        if (!fallback) fallback = compare(c, c.i32(frame.at(0x68)), 2000) < 0;
        if (fallback) {
            advance(0x10278668); const auto value = sample();
            const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x92492493))) + value);
            c.set(3, shifted_sign(high, 2)); c.set(4, value);
            c.put8(frame.at(0x55), 7u - std::uint32_t(std::int32_t(value) % 7));
            const bool first = compare(c, std::int8_t(actor.side())) == 0;
            advance(first ? 0x102786d8 : 0x10278738); const auto row_draw = sample();
            leave(c, {{3, first ? std::uint32_t(std::int32_t(row_draw) % 3) : std::uint32_t(high_product(row_draw, 0x55555556))}, {4, row_draw}});
            c.put8(frame.at(0x54), first ? std::uint32_t(std::int32_t(row_draw) % 3) + 6u : 5u - std::uint32_t(std::int32_t(row_draw) % 3));
            c.set(5, 0);
        }
        perform_action(0x5a, c.r(5), 0x10278798); return finish(0, 0x102787a8);
    }
    case 5: {
        c.put8(actor.match() + 0x7928, 0); c.put8(frame.at(0x43), 0x33); choose_shot(0x43, 0x102787d4);
        c.set(4, c.u32(cursor)); advance(0x102787f8); const auto value = sample();
        c.set(3, shifted_sign(high_product(value, 0x51eb851f), 5)); c.set(4, value);
        const bool varied = compare(c, std::int32_t(value) % 100) == 0;
        shoot(0x33, varied ? 5 : 1, varied ? 0x10278854 : 0x10278878); return finish(0, 0x10278888);
    }
    case 7: {
        c.set(3, 0); c.put8(actor.match() + 0x7928, 0); query_action(0x37, 0x42, 0x102788d0);
        c.set(22, c.r(3));
        bool fallback = compare(c, c.r(22)) == 0;
        if (!fallback) fallback = compare(c, c.i32(frame.at(0x68)), 2000) < 0;
        if (fallback) {
            c.put8(frame.at(0x55), 4); const bool first = compare(c, std::int8_t(actor.side())) == 0;
            c.set(3, c.u8(actor.match() + (first ? 0x7925 : 0x7926)));
            c.put8(frame.at(0x54), c.r(3) + (first ? -2u : 2u)); c.set(22, 0);
        }
        perform_action(0x37, c.r(22), 0x10278944); return finish(0, 0x10278954);
    }
    case 6: {
        c.set(3, actor.match()); c.put8(c.r(3) + 0x7928, 0);
        const bool wide = compare(c, actor.column(), 2) < 0 || compare(c, actor.column(), 6) > 0;
        compare(c, wide); bool shoot_directly = false;
        if (!wide) {
            c.set_f(0, c.f64(doubles + 0x380)); fp.fcmp(0, c.f(31), c.f(0));
            if (c.cr(0) & 8) {
                c.set_f(1, c.f32(singles + 0xf0)); c.set(13, c.u8(player + 0x10c));
                c.set_f(0, fp.fadds(c.f(1), c.f32(player + 0xa4)));
                c.put64(frame.at(0xf4), fp.fctiwz(0, c.f(0))); c.set(16, c.u32(frame.at(0xf8)));
                if (compare(c, std::int32_t(c.r(16))) == 0) c.set(14, 0);
                else {
                    advance(0x102789f4); c.set(3, 65535);
                    bool narrow = compare(c, std::int32_t(c.r(16)), 65535) <= 0;
                    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, std::int32_t(c.r(16)), -65535) >= 0; }
                    if (narrow) {
                        c.set(3, c.u32(cursor)); compare(c, std::int32_t(c.r(16))); const auto value = sample(); c.set(3, value);
                        const auto residual = value - std::uint32_t(divide_signed<4>(value, c.r(16))) * c.r(16);
                        c.set(14, std::int32_t(c.r(16)) > 0 ? residual : 0u - residual);
                    } else {
                        c.put32(frame.at(0xf8), c.r(16) ^ 0x80000000); c.set(6, 0x80008001); c.set(7, c.u32(cursor));
                        c.put32(frame.at(0xf4), 0x43300000); c.set(5, 65535); c.set(3, 0x80008001);
                        c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(frame.at(0xf4))); c.set_f(0, c.f64(doubles));
                        c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set(8, c.u16(c.r(7))); c.set(7, c.u32(seed));
                        c.put32(frame.at(0xec), 0x43300000); c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
                        c.set(7, c.r(8) + c.r(7)); c.set_f(1, c.f64(doubles + 0x80));
                        const auto value = c.r(7); const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value);
                        c.set(6, shifted_sign(high, 15)); c.set(14, value - std::uint32_t(std::int32_t(value) / 65535) * 65535u);
                        c.set(4, c.r(14) ^ 0x80000000); c.put32(frame.at(0xf0), c.r(4));
                        c.set_f(0, fp.fsub(c.f64(frame.at(0xec)), c.f(1)));
                        const auto reduced = std::int32_t(c.r(16) * 10) / 65535;
                        c.set_f(0, fp.fmul(c.f(2), c.f(0))); const auto reduced_high = std::int32_t(std::uint32_t(high_product(c.r(16) * 10, std::int32_t(0x80008001))) + c.r(16) * 10);
                        c.set(3, shifted_sign(reduced_high, 15)); c.set(3, std::uint32_t(reduced));
                        c.put64(frame.at(0xe4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xe8)));
                        enter(c, 0x10278acc, {std::uint32_t(reduced)}); sample_range(c, c.r(3));
                        const auto numerator = c.r(3) + 5; const auto quotient_high = high_product(numerator, 0x66666667);
                        c.set(4, shifted_sign(quotient_high, 2)); c.set(3, std::uint32_t(std::int32_t(numerator) / 10));
                        c.set(14, c.r(14) + c.r(3));
                        if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(16) - 1)) > 0) c.set(14, c.r(16) - 1);
                    }
                }
                const auto numerator = c.r(14) + std::uint32_t(std::int32_t(std::int8_t(c.r(13))));
                c.set(4, 0x55555556); c.set(3, std::uint32_t(high_product(numerator, 0x55555556))); c.set_f(1, c.f64(doubles + 0x80));
                c.put32(frame.at(0xe4), 0x43300000); c.put32(frame.at(0xe8), std::uint32_t(std::int32_t(numerator) / 3) ^ 0x80000000);
                c.set_f(0, fp.fsubs(c.f64(frame.at(0xe4)), c.f(1))); fp.fcmp(0, c.f(31), c.f(0)); shoot_directly = (c.cr(0) & 8) != 0;
            }
        }
        if (shoot_directly) {
            c.put8(actor.match() + 0x7927, 6); c.put8(frame.at(0x40), 0x35); choose_shot(0x40, 0x10278b94);
            c.set(4, c.u32(cursor)); advance(0x10278bb8); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x51eb851f), 5)); c.set(4, value);
            const bool varied = compare(c, std::int32_t(value) % 100) == 0;
            shoot(0x35, varied ? 6 : 1, varied ? 0x10278c14 : 0x10278c38);
        } else {
            query_action(0x37, 0x41, 0x10278c70); c.set(22, c.r(3));
            bool fallback = compare(c, c.r(22)) == 0;
            if (!fallback) fallback = compare(c, c.i32(frame.at(0x68)), 2000) < 0;
            if (fallback) {
                c.put8(frame.at(0x55), 4); const bool first = compare(c, std::int8_t(actor.side())) == 0;
                c.set(3, c.u8(actor.match() + (first ? 0x7925 : 0x7926)));
                c.put8(frame.at(0x54), c.r(3) + (first ? -2u : 2u)); c.set(22, 0);
            }
            perform_action(0x37, c.r(22), 0x10278ce4);
        }
        return finish(0, 0x10278cf4);
    }
    default: break;
    }

    if (compare(c, std::int8_t(c.r(16)), 4) == 0) {
        c.put8(frame.at(0x53), 4); const bool first = compare(c, std::int8_t(actor.side())) == 0;
        c.put8(frame.at(0x52), first ? 11 : 0);
    } else {
        leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x53)}, {6, frame.at(0x52)}, {7, 1}}); c.call(0x101e28b0, 0x10278d4c);
        enter(c, 0x10278d58, {player, c.u8(frame.at(0x53))}); adjust_action_bias(c, player, c.r(4)); c.put8(frame.at(0x53), c.r(3));
        bool target_behind = compare(c, std::int8_t(actor.side())) == 0;
        std::uint32_t first_back = 0x10278d9c, second_back = 0x10278db4;
        if (target_behind) { c.set(3, std::uint32_t(std::int32_t(actor.row()))); target_behind = compare(c, actor.row(), c.i8(frame.at(0x52))) > 0; }
        if (!target_behind) {
            if (compare(c, actor.side(), 1) == 0) {
                c.set(3, std::uint32_t(std::int32_t(actor.row()))); target_behind = compare(c, actor.row(), c.i8(frame.at(0x52))) < 0;
                first_back = 0x10278e14; second_back = 0x10278e2c;
            }
        }
        if (target_behind) {
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - c.i8(frame.at(0x53)))); c.set(13, c.u8(frame.at(0x52)));
            c.call(0x10775540, first_back); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))));
            c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))); c.set(3, std::uint32_t(std::int32_t(actor.row()) - c.r(4)));
            c.call(0x10775540, second_back); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            const auto offset = c.r(3) * 36 + c.r(13) * 4; c.set(3, c.toc(-0x6424)); c.set_f(30, c.f32(c.r(3) + offset));
        }
    }
    leave(c, {{3, actor.match()}, {4, c.u8(player + 0x106)}, {5, actor.side()}, {6, 0}}); c.call(0x101ee680, 0x10278e5c);
    c.put8(frame.at(0x100), c.r(3)); c.set(17, 0); leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101ee770, 0x10278e74);
    c.put8(frame.at(0xff), c.r(3));
    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.u8(player + 0x105)}, {6, c.u8(player + 0x106)}, {7, 1}}); c.call(0x101e3f10, 0x10278ea4); c.set_f(29, c.f(1));
    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.u8(player + 0x105)}, {6, c.u8(player + 0x106)}, {7, 1}}); c.call(0x101e4020, 0x10278ed0);
    c.set(4, c.r(3)); c.put8(frame.at(0xfe), c.r(4)); c.put8(frame.at(0xfd), 0); c.put8(frame.at(0xfc), 0);
    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}}); c.call(0x101f1e10, 0x10278f04); c.set(23, c.r(3));
    leave(c, {{3, actor.match()}, {4, actor.side()}, {5, frame.at(0x4b)}, {6, frame.at(0x4a)}, {7, frame.at(0x49)}, {8, frame.at(0x48)}, {9, frame.at(0x47)}, {10, 0}}); c.call(0x101f21d0, 0x10278f30);
    if (compare(c, std::int8_t(c.r(16)), 4) == 0) c.put16(player + 0x2a, 350);
    else {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x791f), -1) == 0) c.put16(player + 0x2a, 200);
        else {
            leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x58)}, {6, frame.at(0x74)}}); c.call(0x101d7dc0, 0x10278f64);
            const auto populated = std::int8_t(c.r(3)); compare(c, populated); c.put8(frame.at(0xfd), populated);
            if (populated == 0) {
                c.set_f(0, c.f64(doubles + 0x108)); fp.fcmp(0, c.f(29), c.f(0));
                if (c.cr(0) & 4) c.set_f(29, c.f32(singles + 0x140));
                c.set_f(1, c.f32(singles + 0xe8)); c.set_f(0, c.f32(singles + 0x144));
                c.set_f(0, fp.multiply_add(0x10278f88, c.f(1), c.f(29), c.f(0)));
                c.put64(frame.at(0xe4), fp.fctiwz(0, c.f(0))); c.put16(player + 0x2a, c.u32(frame.at(0xe8)));
            } else {
                if (compare(c, c.i16(frame.at(0x58))) < 0) c.put16(frame.at(0x58), 0);
                c.set(4, std::uint32_t(std::int32_t(c.i8(frame.at(0xfd)))));
                c.set(3, c.r(4) * std::uint32_t(std::int32_t(c.i16(frame.at(0x58))))); c.set(5, extended(c.i16(frame.at(0x58))));
                const auto penalty = c.r(5) * c.r(3); c.put32(frame.at(0xe4), 0x43300000);
                c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x30));
                c.put32(frame.at(0xe8), penalty ^ 0x80000000); c.set_f(0, fp.fsub(c.f64(frame.at(0xe4)), c.f(1)));
                c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set_f(0, -c.f(0));
                c.put64(frame.at(0xec), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xf0)));
                if (compare(c, std::int32_t(c.r(14)), -500) < 0) {
                    const auto bound = [&] { return std::int32_t(c.r(14) + 500) / 3; };
                    const auto quotient = high_product(c.r(14) + 500, 0x55555556);
                    c.set(3, std::uint32_t(quotient));
                    if (compare(c, bound()) == 0) c.set(13, 0);
                    else {
                        advance(0x1027904c); c.set(4, std::uint32_t(high_product(c.r(14) + 500, 0x55555556))); c.set(3, 65535);
                        bool narrow = compare(c, bound(), 65535) <= 0;
                        if (narrow) { c.set(3, std::uint32_t(-65535)); c.set(4, std::uint32_t(high_product(c.r(14) + 500, 0x55555556))); narrow = compare(c, bound(), -65535) >= 0; }
                        if (narrow) {
                            c.set(5, c.u32(cursor)); c.set(6, c.u16(c.r(5))); c.set(4, c.u32(seed)); c.set(6, c.r(6) + c.r(4));
                            c.set(5, std::uint32_t(bound())); c.set(3, std::uint32_t(high_product(c.r(14) + 500, 0x55555556))); c.set(4, c.r(5) >> 31);
                            const auto residual = c.r(6) - std::uint32_t(divide_signed<4>(c.r(6), c.r(5))) * c.r(5);
                            compare(c, bound()); c.set(13, bound() > 0 ? residual : 0u - residual);
                        } else {
                            c.set(6, std::uint32_t(high_product(c.r(14) + 500, 0x55555556))); c.set(9, c.u32(cursor)); c.put32(frame.at(0xe4), 0x43300000);
                            c.set(8, 0x80008001); c.set_f(2, c.f64(doubles + 0x80)); c.set(7, 65535); c.set_f(0, c.f64(doubles));
                            c.put32(frame.at(0xe8), std::uint32_t(bound()) ^ 0x80000000); c.set(9, c.u16(c.r(9))); c.set(6, c.u32(seed));
                            c.set_f(1, c.f64(frame.at(0xe4))); c.set(9, c.r(9) + c.r(6)); c.set_f(2, fp.fsub(c.f(1), c.f(2)));
                            c.set(3, std::uint32_t(high_product(c.r(14) + 500, 0x55555556))); c.put32(frame.at(0xec), 0x43300000);
                            c.set(4, 0x80008001); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, fp.fdiv(c.f(2), c.f(0)));
                            const auto high = std::int32_t(std::uint32_t(high_product(c.r(9), std::int32_t(0x80008001))) + c.r(9));
                            c.set(6, shifted_sign(high, 15)); c.set(5, std::uint32_t(std::int32_t(c.r(9)) / 65535) * 65535);
                            c.set(13, c.r(9) - c.r(5)); c.set(5, c.r(13) ^ 0x80000000); c.put32(frame.at(0xf0), c.r(5));
                            c.set_f(0, fp.fsub(c.f64(frame.at(0xec)), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
                            const auto reduced_word = std::uint32_t(bound()) * 10; const auto reduced = std::int32_t(reduced_word) / 65535;
                            const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
                            shifted_sign(reduced_high, 15); c.set(3, std::uint32_t(reduced));
                            c.put64(frame.at(0xf4), fp.fctiwz(0, c.f(0))); c.set(13, c.u32(frame.at(0xf8)));
                            enter(c, 0x10279184, {std::uint32_t(reduced)}); sample_range(c, c.r(3));
                            const auto numerator = c.r(3) + 5; const auto correction_high = high_product(numerator, 0x66666667);
                            c.set(5, shifted_sign(correction_high, 2)); c.set(4, std::uint32_t(std::int32_t(numerator) / 10)); c.set(13, c.r(13) + c.r(4));
                            c.set(3, std::uint32_t(bound()));
                            if (compare(c, std::int32_t(c.r(13)), bound() - 1) > 0) { c.set(3, std::uint32_t(bound())); c.set(13, c.r(3) - 1); }
                        }
                    }
                    c.set(14, c.r(13) - 500);
                    if (compare(c, std::int32_t(c.r(14)), -2000) < 0) c.set(14, std::uint32_t(-2000));
                }
                if (compare(c, std::int32_t(c.r(14)), 32000) > 0) c.set(14, 32000);
                c.set(3, c.r(14) ^ 0x80000000); c.put32(frame.at(0xe8), c.r(3)); c.set_f(1, c.f64(doubles + 0x80));
                c.put32(frame.at(0xe4), 0x43300000); c.set_f(2, c.f64(doubles + 0x30)); c.set_f(0, fp.fsub(c.f64(frame.at(0xe4)), c.f(1)));
                c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.put64(frame.at(0xec), fp.fctiwz(0, c.f(0))); c.put16(player + 0x2a, c.u32(frame.at(0xf0)));
            }
        }
        bool pursuit = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 6) >= 0;
        if (!pursuit) pursuit = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 6) < 0;
        compare(c, pursuit);
        if (!pursuit) {
            leave(c, {{3, actor.match()}, {4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)}, {6, 4}, {7, std::int8_t(actor.side()) == 0 ? 11u : 0u}});
            c.call(0x101ef3c0, 0x10279320); c.set(17, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        }
        const auto opposing_goal_area = [&] {
            const auto opposite = unsigned(std::int8_t(actor.side()) == 0);
            const bool second = compare(c, opposite, 1) == 0; c.set(3, 0);
            bool inside = compare(c, actor.column(), 2) >= 0 && compare(c, actor.column(), 6) <= 0;
            if (inside) inside = second ? compare(c, actor.row(), 10) >= 0 && compare(c, actor.row(), 11) <= 0 : compare(c, actor.row()) >= 0 && compare(c, actor.row(), 1) <= 0;
            c.set(3, inside); compare(c, inside); return inside;
        };
        const bool inside = opposing_goal_area();
        c.put32(frame.at(0x38), UINT32_MAX);
        leave(c, {{3, actor.match()}, {4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)}, {6, frame.at(0x50)},
                  {7, frame.at(0x56)}, {8, frame.at(0x4f)}, {9, frame.at(0x4e)}, {10, frame.at(inside ? 0x64 : 0x60)}});
        c.call(0x101e8950, inside ? 0x1027940c : 0x10279478);
        c.set(22, c.r(3)); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(17))) + c.i8(frame.at(0xfd))));
        c.set(5, 0x66666667); c.set(6, c.u32(frame.at(inside ? 0x64 : 0x60)));
        const auto numerator = (c.r(4) + (inside ? 2u : 4u)) * c.r(6); const auto high = high_product(numerator, 0x66666667);
        c.set(3, shifted_sign(high, 1)); c.put32(frame.at(0x68), std::uint32_t(std::int32_t(numerator) / 5));
    }
    c.set(3, extended(c.i16(player + 0x2a))); c.put16(player + 0x2a, c.r(3) + 70);

    const auto goal_area = [&](bool opposing) {
        const auto side = opposing ? unsigned(std::int8_t(actor.side()) == 0) : unsigned(actor.side());
        const bool second = compare(c, side, 1) == 0; c.set(3, 0);
        bool inside = compare(c, actor.column(), 2) >= 0 && compare(c, actor.column(), 6) <= 0;
        if (inside) inside = second ? compare(c, actor.row(), 10) >= 0 && compare(c, actor.row(), 11) <= 0 : compare(c, actor.row()) >= 0 && compare(c, actor.row(), 1) <= 0;
        c.set(3, inside); compare(c, inside); return inside;
    };
    const auto pursuit_half = [&] {
        bool selected = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 6) >= 0;
        if (!selected) selected = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 6) < 0;
        compare(c, selected); return selected;
    };
    const auto target_open = [&](std::uint32_t first_back, std::uint32_t second_back) {
        const bool first = compare(c, std::int8_t(actor.side())) == 0;
        bool at_target = first ? compare(c, c.u8(player + 0x106), 11) == 0 : compare(c, actor.row()) == 0;
        if (at_target) at_target = compare(c, actor.column(), 3) >= 0 && compare(c, actor.column(), 5) <= 0;
        if (at_target) c.set(3, 1);
        else {
            c.set_f(1, c.f32(singles)); c.set_f(2, c.f32(singles)); c.set(9, 0);
            enter(c, first ? first_back : second_back, {player, 4, first ? 11u : 0u, 0});
            accepts_target(c, player, c.r(4), c.r(5), 0, 0, c.f(1), c.f(2));
        }
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    c.set(3, actor.match() + 0x10000 + unsigned(std::int8_t(actor.side()) == 0) * 0x18e0);
    if (compare(c, c.u32(c.r(3) - 0x7e0c) & 0x400) != 0) {
        c.set(13, c.u8(player + 0x109)); advance(0x10279520); const auto value = sample();
        const auto high = high_product(value, 0x66666667); c.set(4, shifted_sign(high, 1));
        leave(c, {{3, std::uint32_t(std::int32_t(value) % 5)}, {5, value}});
        if (compare(c, std::int8_t(c.r(13)), std::int32_t(c.r(3))) > 0 && !goal_area(true) && !goal_area(false) && !pursuit_half()) {
            c.set(3, c.u32(frame.at(0x68))); c.set(4, 4000u - c.u32(frame.at(0x68)));
            const auto sum = c.r(3) + (c.r(3) >> 31); const auto half = std::int32_t(sum) >> 1;
            c.s.wr<1>(reg::xer_ca, std::int32_t(sum) < 0 && (sum & 1));
            c.set(19, c.r(19) + c.r(4)); c.set(20, c.r(20) + 2000u - std::uint32_t(half));
        }
    }
    c.set(3, actor.match());
    if (compare(c, c.u8(c.r(3) + 0x791f), 4) == 0) {
        c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(31), c.f(0)); bool close = (c.cr(0) & 8) != 0;
        if (!close) {
            bool end_row = compare(c, actor.row()) == 0;
            if (!end_row) end_row = compare(c, c.u8(player + 0x106), 11) == 0;
            if (end_row) {
                c.set(3, std::uint32_t(std::int32_t(actor.column()) - 4)); c.call(0x10775540, 0x10279780); close = compare(c, std::int32_t(c.r(3)), 1) == 0;
            }
        }
        if (close) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(17)))) * 400); c.put32(frame.at(0xe4), 0x43300000);
            c.set_f(3, c.f32(singles + 0x90)); c.set_f(2, c.f32(player + 0xac)); c.set_f(0, c.f32(singles + 0x148));
            c.put32(frame.at(0xe8), c.r(3) ^ 0x80000000); c.set_f(1, c.f64(doubles + 0x80));
            c.set_f(2, fp.multiply_add(0x102797b0, c.f(3), c.f(2), c.f(0)));
            c.set_f(0, fp.fsubs(c.f64(frame.at(0xe4)), c.f(1))); c.set_f(0, fp.fsubs(c.f(2), c.f(0)));
            c.put64(frame.at(0xec), fp.fctiwz(0, c.f(0))); c.set(21, c.u32(frame.at(0xf0)));
        } else c.set(21, std::uint32_t(-5000));
    } else {
        c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(31), c.f(0)); const auto bits = c.cr(0);
        c.set_cr(0, (bits & ~2u) | ((bits & 10) ? 2u : 0u));
        if (c.cr(0) & 2) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(17)))) * 400); c.set(5, extended(c.i16(player + 0x2a)) ^ 0x80000000);
            c.set(4, 0x43300000); c.put32(frame.at(0xec), 0x43300000);
            c.set_f(4, c.f32(singles + 0x1c)); c.set_f(3, c.f32(player + 0x9c)); c.set_f(2, c.f32(singles + 0x14c));
            c.put32(frame.at(0xf0), c.r(3) ^ 0x80000000); c.set_f(1, c.f64(doubles + 0x80));
            c.set_f(0, c.f64(frame.at(0xec))); c.set_f(2, fp.multiply_add(0x10279820, c.f(4), c.f(3), c.f(2)));
            c.put32(frame.at(0xe8), c.r(5)); c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(3, c.f64(doubles + 0x80));
            c.put32(frame.at(0xe4), c.r(4)); c.set_f(4, c.f64(doubles + 0x190)); c.set_f(1, c.f64(frame.at(0xe4)));
            c.set_f(0, fp.fsubs(c.f(2), c.f(0))); c.set_f(1, fp.fsub(c.f(1), c.f(3)));
            c.set_f(0, fp.multiply_add(0x10279844, c.f(4), c.f(1), c.f(0)));
            c.put64(frame.at(0xf4), fp.fctiwz(0, c.f(0))); c.set(21, c.u32(frame.at(0xf8)));
        } else {
            c.set(13, c.u8(player + 0x10c)); advance(0x1027987c); const auto value = sample();
            const auto trait = std::int32_t(std::int8_t(c.r(13)));
            const auto trait_high = std::int32_t(std::uint32_t(high_product(std::uint32_t(trait), std::int32_t(0x92492493))) + std::uint32_t(trait));
            const auto trait_sign = shifted_sign(trait_high, 2);
            c.set(3, trait_sign); c.set(4, std::uint32_t(std::int32_t(value) % 3)); c.set(5, std::uint32_t(high_product(value, 0x55555556)));
            c.set(6, value); c.set(7, value - c.u32(seed));
            c.put32(frame.at(0xe4), 0x43300000); c.put32(frame.at(0xe8), std::uint32_t(trait / 7 + std::int32_t(c.r(4)) + 3) ^ 0x80000000);
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, fp.fsubs(c.f64(frame.at(0xe4)), c.f(1))); fp.fcmp(0, c.f(31), c.f(0));
            bool choose = (c.cr(0) & 8) != 0;
            if (choose && compare(c, std::int8_t(c.r(16)), 4) == 0) {
                c.set(13, c.u8(player + 0x10c)); advance(0x1027992c); const auto value = sample();
                c.set(4, shifted_sign(high_product(value, 0x51eb851f), 4));
                leave(c, {{3, std::uint32_t(std::int32_t(value) % 50)}, {5, value}});
                choose = compare(c, std::int8_t(c.r(13)), std::int32_t(c.r(3))) > 0;
            }
            if (!choose) c.set(21, std::uint32_t(-500));
            else {
                choose_shot(0x51, 0x10279980); c.set(14, c.u8(player + 0x109));
                c.put32(frame.at(0xe4), 0x43300000);
                c.put32(frame.at(0xe8), std::uint32_t(std::int32_t(std::int8_t(c.r(14))) * 4) ^ 0x80000000);
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f32(singles + 0x90)); c.set_f(0, c.f32(singles)); c.set(13, c.u8(player + 0x109));
                c.set_f(1, fp.fsubs(c.f64(frame.at(0xe4)), c.f(2)));
                c.set_f(1, fp.multiply_add(0x102799b8, c.f(3), c.f(31), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
                if (c.cr(0) & 2) c.set(18, 0);
                else {
                    advance(0x102799ec); c.set(3, 65535);
                    auto bound = shot_bound(14, 0xe4, 0xec, 0x10279a24);
                    bool narrow = compare(c, std::int32_t(bound), 65535) <= 0;
                    if (narrow) { c.set(3, std::uint32_t(-65535)); bound = shot_bound(14, 0xe4, 0xec, 0x10279a6c); narrow = compare(c, std::int32_t(bound), -65535) >= 0; }
                    if (narrow) {
                        c.set(4, 0x43300000); c.set(5, c.u32(cursor));
                        const auto trait_bits = std::uint32_t(std::int32_t(std::int8_t(c.r(14))) * 4) ^ 0x80000000;
                        c.set(3, trait_bits); c.put32(frame.at(0xa8), trait_bits); c.put32(frame.at(0xa4), 0x43300000);
                        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(2, c.f32(singles + 0x90));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set(5, c.u16(c.r(5))); c.set(4, c.u32(seed));
                        c.put32(frame.at(0xb8), trait_bits); c.set_f(0, fp.multiply_add(0x10279c90, c.f(2), c.f(31), c.f(0)));
                        c.set(5, c.r(5) + c.r(4)); c.put32(frame.at(0xb4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
                        const auto first_tag = fp.fctiwz(3, c.f(0)); c.set_f(0, c.f64(frame.at(0xb4))); c.set_f(2, c.f32(singles + 0x90));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.put64(frame.at(0xac), first_tag); c.set(4, c.u32(frame.at(0xb0)));
                        c.set_f(0, fp.multiply_add(0x10279cb8, c.f(2), c.f(31), c.f(0)));
                        const auto quotient = c.r(4) == 0 ? 0u : std::uint32_t(divide_signed<4>(c.r(5), c.r(4)));
                        c.set(3, quotient); c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); const auto sign_bound = c.u32(frame.at(0xc0));
                        compare(c, std::int32_t(sign_bound)); const auto residual = c.r(5) - quotient * c.r(4);
                        c.set(18, std::int32_t(sign_bound) > 0 ? residual : 0u - residual);
                    } else {
                        const auto bound_word = shot_bound(14, 0xe4, 0xec, 0x10279ae4);
                        c.set(4, c.u16(c.u32(cursor))); const auto value = c.r(4) + c.u32(seed);
                        const auto trait_bits = std::uint32_t(std::int32_t(std::int8_t(c.r(14))) * 4) ^ 0x80000000;
                        c.set(3, 0x80008001); c.set(5, 0x43300000); c.set(6, trait_bits); c.set(7, 0x80008001);
                        c.set(8, 0x43300000); c.set(9, 0x43300000); c.set(10, 65535);
                        c.put32(frame.at(0xf4), 0x43300000); c.set_f(5, c.f64(doubles + 0x80));
                        c.put32(frame.at(0xd0), trait_bits);
                        c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xcc), 0x43300000); c.set_f(4, c.f64(doubles));
                        c.set(3, bound_word ^ 0x80000000); c.put32(frame.at(0xf8), c.r(3));
                        const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value);
                        c.set(4, shifted_sign(high, 15)); c.set_f(0, c.f64(frame.at(0xcc))); c.set_f(3, c.f64(frame.at(0xf4)));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(2, c.f32(singles + 0x90)); c.set_f(1, fp.fsub(c.f(3), c.f(5)));
                        const auto residual = value - std::uint32_t(std::int32_t(value) / 65535) * 65535u;
                        c.put32(frame.at(0xdc), 0x43300000); c.set_f(0, fp.multiply_add(0x10279b4c, c.f(2), c.f(31), c.f(0)));
                        c.set_f(2, fp.fdiv(c.f(1), c.f(4))); c.set_f(3, c.f64(doubles + 0x80)); c.set(18, residual);
                        c.put32(frame.at(0xe0), residual ^ 0x80000000); c.set_f(1, c.f64(frame.at(0xdc)));
                        const auto second_bound_tag = fp.fctiwz(0, c.f(0)); c.set_f(1, fp.fsub(c.f(1), c.f(3))); c.put64(frame.at(0xc4), second_bound_tag);
                        c.set_f(0, fp.fmul(c.f(2), c.f(1))); const auto reduced_word = c.u32(frame.at(0xc8)) * 10;
                        c.put64(frame.at(0xd4), fp.fctiwz(0, c.f(0))); c.set(18, c.u32(frame.at(0xd8)));
                        const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
                        shifted_sign(reduced_high, 15); enter(c, 0x10279ba4, {std::uint32_t(std::int32_t(reduced_word) / 65535)}); sample_range(c, c.r(3));
                        const auto numerator = c.r(3) + 5; c.set(4, trait_bits); c.set(5, 0x66666667);
                        c.put32(frame.at(0xbc), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xc0), trait_bits);
                        const auto high_correction = high_product(numerator, 0x66666667); c.set_f(2, c.f32(singles + 0x90)); c.set_f(0, c.f64(frame.at(0xbc)));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.multiply_add(0x10279be0, c.f(2), c.f(31), c.f(0)));
                        c.set(3, shifted_sign(high_correction, 2)); c.set(18, c.r(18) + std::uint32_t(std::int32_t(numerator) / 10));
                        c.put64(frame.at(0xb4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(frame.at(0xb8)));
                        if (compare(c, std::int32_t(c.r(18)), std::int32_t(c.r(3) - 1)) > 0) {
                            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(14)))) * 4);
                            c.set(3, shot_bound(14, 0xac, 0xa4, 0x10279c30)); c.set(18, c.r(3) - 1);
                        }
                    }
                }
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * 4);
                const auto initial_bound = shot_bound(13, 0xa4, 0xac, 0x10279d10) - c.r(18);
                if (compare(c, std::int32_t(initial_bound)) == 0) c.set(14, 0);
                else {
                    advance(0x10279d50); c.set(3, 65535);
                    auto bound = shot_bound(13, 0xa4, 0xac, 0x10279d88) - c.r(18);
                    bool narrow = compare(c, std::int32_t(bound), 65535) <= 0;
                    if (narrow) { c.set(3, std::uint32_t(-65535)); bound = shot_bound(13, 0xa4, 0xac, 0x10279dd4) - c.r(18); narrow = compare(c, std::int32_t(bound), -65535) >= 0; }
                    if (narrow) {
                        c.set(4, 0x43300000); c.set(5, c.u32(cursor));
                        const auto trait_bits = std::uint32_t(std::int32_t(std::int8_t(c.r(13))) * 4) ^ 0x80000000;
                        c.set(3, trait_bits); c.put32(frame.at(0xa8), trait_bits); c.put32(frame.at(0xa4), 0x43300000);
                        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(2, c.f32(singles + 0x90));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set(5, c.u16(c.r(5))); c.set(4, c.u32(seed));
                        c.put32(frame.at(0xb8), trait_bits); c.set_f(0, fp.multiply_add(0x1027a00c, c.f(2), c.f(31), c.f(0)));
                        c.set(5, c.r(5) + c.r(4)); c.put32(frame.at(0xb4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
                        const auto first_tag = fp.fctiwz(3, c.f(0)); c.set_f(0, c.f64(frame.at(0xb4))); c.set_f(2, c.f32(singles + 0x90));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.put64(frame.at(0xac), first_tag); const auto first_integer = c.u32(frame.at(0xb0));
                        c.set_f(0, fp.multiply_add(0x1027a034, c.f(2), c.f(31), c.f(0))); c.set(4, first_integer - c.r(18));
                        const auto quotient = c.r(4) == 0 ? 0u : std::uint32_t(divide_signed<4>(c.r(5), c.r(4)));
                        c.set(3, quotient); c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); const auto sign_bound = c.u32(frame.at(0xc0)) - c.r(18);
                        compare(c, std::int32_t(sign_bound)); const auto residual = c.r(5) - quotient * c.r(4);
                        c.set(14, std::int32_t(sign_bound) > 0 ? residual : 0u - residual);
                    } else {
                        const auto first_bound = shot_bound(13, 0xa4, 0xac, 0x10279e48);
                        c.set(5, c.u16(c.u32(cursor))); const auto value = c.r(5) + c.u32(seed);
                        const auto trait_bits = std::uint32_t(std::int32_t(std::int8_t(c.r(13))) * 4) ^ 0x80000000;
                        c.set(4, 0x80008001); c.set(6, 0x43300000); c.set(7, trait_bits); c.set(8, 0x80008001);
                        c.set(9, 0x43300000); c.set(10, 0x43300000); c.put32(frame.at(0xd0), trait_bits);
                        c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xcc), 0x43300000); c.set_f(4, c.f64(doubles + 0x80));
                        c.set_f(0, c.f64(frame.at(0xcc))); c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
                        c.put32(frame.at(0xb4), 0x43300000); const auto bound_word = first_bound - c.r(18);
                        c.set(4, bound_word ^ 0x80000000); c.put32(frame.at(0xb8), c.r(4)); c.set_f(2, c.f32(singles + 0x90));
                        c.set_f(0, fp.multiply_add(0x10279e9c, c.f(2), c.f(31), c.f(0))); c.set_f(3, c.f64(doubles));
                        const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value);
                        c.set_f(1, c.f64(frame.at(0xb4))); c.set(5, shifted_sign(high, 15)); c.set(4, std::uint32_t(std::int32_t(value) / 65535));
                        c.set_f(1, fp.fsub(c.f(1), c.f(4))); const auto second_bound_tag = fp.fctiwz(0, c.f(0));
                        c.put32(frame.at(0xbc), 0x43300000); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, fp.fdiv(c.f(1), c.f(3)));
                        c.put64(frame.at(0xd4), second_bound_tag); c.set(14, value - c.r(4) * 65535u); c.put32(frame.at(0xc0), c.r(14) ^ 0x80000000);
                        const auto reduced_word = (c.u32(frame.at(0xd8)) - c.r(18)) * 10;
                        c.set_f(1, fp.fsub(c.f64(frame.at(0xbc)), c.f(2))); c.set_f(0, fp.fmul(c.f(3), c.f(1)));
                        c.put64(frame.at(0xc4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xc8)));
                        const auto reduced_high = std::int32_t(std::uint32_t(high_product(reduced_word, std::int32_t(0x80008001))) + reduced_word);
                        shifted_sign(reduced_high, 15); enter(c, 0x10279f18, {std::uint32_t(std::int32_t(reduced_word) / 65535)}); sample_range(c, c.r(3));
                        const auto numerator = c.r(3) + 5; c.set(4, trait_bits); c.set(5, 0x66666667);
                        c.put32(frame.at(0xdc), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xe0), trait_bits);
                        const auto correction_high = high_product(numerator, 0x66666667); c.set_f(2, c.f32(singles + 0x90)); c.set_f(0, c.f64(frame.at(0xdc)));
                        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.multiply_add(0x10279f54, c.f(2), c.f(31), c.f(0)));
                        c.set(3, shifted_sign(correction_high, 2)); c.set(14, c.r(14) + std::uint32_t(std::int32_t(numerator) / 10));
                        c.put64(frame.at(0xe4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(frame.at(0xe8)) - c.r(18));
                        if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(3) - 1)) > 0) {
                            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * 4);
                            c.set(3, shot_bound(13, 0xec, 0xf4, 0x10279fa8) - c.r(18)); c.set(14, c.r(3) - 1);
                        }
                    }
                }
                c.put32(frame.at(0x104), c.r(21) + c.r(14)); const auto positive = compare(c, std::int32_t(c.r(21))) > 0; c.put8(frame.at(0x101), 1);
                if (positive) {
                    c.set(4, compare(c, c.i8(player + 0x10c), 1) < 0 ? 1u : std::uint32_t(std::int32_t(c.i8(player + 0x10c))));
                    c.set(3, c.r(21) * c.r(21)); c.set(5, c.r(4) ^ 0x80000000); c.put32(frame.at(0xa4), 0x43300000); c.set(4, 0x43300000);
                    c.set_f(3, c.f64(doubles + 0x80)); c.put32(frame.at(0xa8), c.r(3) ^ 0x80000000); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(17)))) * 700);
                    c.set_f(4, c.f64(doubles + 8)); c.set_f(2, c.f64(frame.at(0xa4))); c.set_f(0, fp.fmuls(c.f(31), c.f(31)));
                    c.set_f(1, c.f32(singles + 0x150)); c.set_f(5, fp.fsub(c.f(2), c.f(3))); c.set(3, c.r(3) ^ 0x80000000);
                    c.put32(frame.at(0xb0), c.r(5)); c.set_f(3, fp.fmuls(c.f(1), c.f(0))); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(4, fp.fdiv(c.f(5), c.f(4)));
                    c.put32(frame.at(0xac), c.r(4)); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xac)));
                    c.put32(frame.at(0xb8), c.r(3)); c.put32(frame.at(0xb4), 0x43300000); c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
                    c.set_f(0, fp.fsub(c.f64(frame.at(0xb4)), c.f(1))); c.set_f(1, fp.fdivs(c.f(3), c.f(2)));
                    c.set_f(1, fp.fsub(c.f(4), c.f(1))); c.set_f(0, fp.fsub(c.f(1), c.f(0)));
                    c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); c.set(21, c.u32(frame.at(0xc0)));
                } else {
                    c.set(3, std::uint32_t(std::int32_t(c.i8(player + 0x109)) + c.i8(player + 0x10e)) * 4);
                    c.set(21, c.r(21) * (c.r(3) + 2));
                }
            }
        }
    }

    c.set(3, actor.match() + std::uint32_t(std::int32_t(std::int8_t(actor.side()))));
    if (compare(c, c.u8(c.r(3) + 0x1c4), 5) == 0) {
        c.set(5, extended(c.i8(frame.at(0x4a))));
        c.set(3, player); c.set(4, std::uint32_t(c.i8(frame.at(0x4b)) - c.i8(frame.at(0x4a))) * 3000);
        c.put32(frame.at(0x68), c.u32(frame.at(0x68)) * 2); c.set(21, c.r(21) - c.r(4));
        enter(c, 0x1027a1a4, {player}); current_role(c, player);
        if (compare(c, std::int16_t(c.r(3)), 1) != 0) {
            c.set(4, std::uint32_t(c.i8(frame.at(0x4b)) - c.i8(frame.at(0x4a)))); c.set(3, c.u32(player + 0x6c));
            c.set(14, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x67))) * 2 - c.r(4) * 6);
            c.set(3, c.r(4) * 6); bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027a208, 0x1027a2e0);
            if (compare(c, std::int32_t(c.r(13)), 1) < 0) { c.set(3, c.u8(player + 0x103)); c.put8(player + 0x103, c.r(3) - 1); }
        }
    } else {
        c.set(3, std::uint32_t(std::int32_t(c.i8(frame.at(0x4b))) - 2));
        if (compare(c, std::int32_t(c.r(3)), c.i8(frame.at(0x4a))) > 0) {
            c.set(3, c.r(3) - std::uint32_t(std::int32_t(c.i8(frame.at(0x4a))))); c.set(4, extended(c.i8(frame.at(0x4a))));
            c.put32(frame.at(0x68), c.u32(frame.at(0x68)) * 2); c.set(21, c.r(21) - c.r(3) * 3000);
        }
    }
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x791f), 4) < 0) {
        c.set_f(0, c.f32(singles)); fp.fcmp(0, c.f(31), c.f(0));
        if (c.cr(0) & 4) {
            if (compare(c, c.u8(frame.at(0xff)), 2) == 0) {
                c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(31))); c.put32(frame.at(0xac), 0x43300000);
                c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 8));
                c.put32(frame.at(0xb0), c.u32(frame.at(0xa8)) ^ 0x80000000); c.set_f(1, fp.fsub(c.f64(frame.at(0xac)), c.f(1)));
                c.call(0x10780fc0, 0x1027a400); c.set(2, c.u32(frame.at(0x14))); c.set_f(28, c.f(1)); advance(0x1027a428);
                c.set_f(1, c.f32(player + 0x98)); c.set_f(0, c.f32(player + 0x98)); c.set(3, 0x43300000); c.set(4, c.u32(cursor));
                c.set_f(2, fp.fmuls(c.f(1), c.f(0))); c.set_f(1, c.f64(doubles + 0x28)); c.set_f(0, c.f64(doubles + 0x28)); c.set(5, c.u16(c.r(4)));
                c.set_f(0, fp.multiply_add(0x1027a454, c.f(2), c.f(1), c.f(0))); c.set(4, c.u32(seed)); c.put32(frame.at(0xac), c.r(3));
                c.set(4, c.r(5) + c.r(4)); c.set_f(3, c.f64(doubles + 0x80)); const auto tag = fp.fctiwz(0, c.f(0));
                const auto residue = std::uint32_t(std::int32_t(c.r(4)) % 4); c.set(4, residue); c.put32(frame.at(0xb4), 0x43300000);
                c.set_f(1, c.f64(doubles + 0x80)); c.put64(frame.at(0xa4), tag); c.set(4, residue);
                c.set(3, extended(std::int16_t(c.u32(frame.at(0xa8)))) ^ 0x80000000);
                c.put32(frame.at(0xb8), (residue + 1) ^ 0x80000000); c.set_f(0, c.f64(frame.at(0xb4))); c.put32(frame.at(0xb0), c.r(3));
                c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(2, c.f64(frame.at(0xac))); c.set_f(1, fp.fsub(c.f(2), c.f(3)));
                c.set_f(1, fp.fmul(c.f(1), c.f(28))); c.set_f(0, fp.fmul(c.f(1), c.f(0)));
                c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xc0)));
            }
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - c.i8(c.r(23) + 0x105))); c.set(13, c.u8(c.r(23) + 0x106)); c.call(0x10775540, 0x1027a4ec);
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(13))))); c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(3, std::uint32_t(std::int32_t(actor.row()) - c.r(4))); c.call(0x10775540, 0x1027a504); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set_f(0, c.f64(doubles + 0x88)); const auto offset = c.r(3) * 36 + c.r(13) * 4; c.set(3, c.toc(-0x6424)); c.set_f(1, c.f32(c.r(3) + offset));
            fp.fcmp(0, c.f(0), c.f(1));
            if (c.cr(0) & 2) {
                const auto cell = actor.match() + std::uint32_t(std::int32_t(actor.column()) * 0x450 + actor.row() * 0x5c);
                c.set(4, actor.match() + std::uint32_t(std::int32_t(actor.column()) * 0x450)); c.set(5, actor.match());
                c.set(3, cell + unsigned(std::int8_t(actor.side()) == 0));
                if (compare(c, c.u8(c.r(3) + 0xa60), 1) == 0) {
                    c.set_f(1, c.f32(singles + 0x5c)); c.set_f(0, fp.fmuls(c.f(1), c.f32(player + 0x98)));
                    c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xa8)));
                    bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027a5b8, 0x1027a690);
                    c.set_f(1, c.f32(singles + 0x5c)); c.set_f(0, fp.fmuls(c.f(1), c.f32(player + 0x9c)));
                    c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(18, c.u32(frame.at(0xa8)));
                    bounded_draw(18, 14, 0xa4, 0xac, 0xb4, 0x1027a73c, 0x1027a814);
                    c.set(3, (c.r(13) - c.r(14)) ^ 0x80000000); c.put32(frame.at(0xa4), 0x43300000);
                    c.set_f(3, c.f32(singles + 0xec)); c.put32(frame.at(0xa8), c.r(3)); c.set_f(2, c.f32(singles + 0x154)); c.set_f(1, c.f64(doubles + 0x80));
                    c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(2, fp.multiply_add(0x1027a8a0, c.f(3), c.f(31), c.f(2))); c.put8(frame.at(0xfc), 1);
                    c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.fmuls(c.f(2), c.f(0)));
                    c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
                }
            }
        }
    }

    if (compare(c, c.i8(frame.at(0xff))) == 0 && compare(c, c.i32(frame.at(0x68)), 3000) < 0 && compare(c, std::int8_t(c.r(16)), 4) != 0) {
        c.set(4, actor.match()); c.set(13, extended(c.i16(c.r(4) + 0x7948))); advance(0x1027a90c); const auto value = sample();
        const auto draw_sign = shifted_sign(high_product(value, 0x66666667), 3), residual = std::uint32_t(std::int32_t(value) % 20);
        const auto rating = std::int32_t(std::int16_t(c.r(13))); const auto rating_high = high_product(std::uint32_t(rating), 0x2e8ba2e9);
        c.set(5, draw_sign); c.set(6, value); c.set(4, residual); c.set(3, shifted_sign(rating_high, 1)); c.set(3, std::uint32_t(rating / 11));
        if (compare(c, rating / 11, std::int32_t(residual) + 70) > 0 && !goal_area(true) && compare(c, c.i8(player + 0x109)) != 0) {
            c.set(3, actor.match());
            if (compare(c, c.i8(c.r(3) + 0x791f), 3) < 0) {
                c.set(3, c.u32(player + 0x6c)); c.set(14, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x68))));
                bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027aa80, 0x1027ab58);
                advance(0x1027abdc); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x66666667), 2)); c.set(4, value);
                if (compare(c, std::int32_t(c.r(13)), std::int32_t(value) % 10) < 0) {
                    leave(c, {{3, actor.match()}, {4, actor.side()}}); c.call(0x101fd000, 0x1027ac28);
                    if (compare(c, std::int8_t(c.r(3))) != 0) {
                        leave(c, {{3, actor.match()}, {4, actor.side()}, {5, frame.at(0x4b)}, {6, frame.at(0x4a)},
                                  {7, frame.at(0x49)}, {8, frame.at(0x48)}, {9, frame.at(0x47)}, {10, 1}}); c.call(0x101f21d0, 0x1027ac54);
                        if (compare(c, std::int8_t(c.r(3))) > 0 && compare(c, std::int8_t(c.r(3)), 3) < 0) {
                            const bool wide = compare(c, actor.column(), 2) < 0 || compare(c, actor.column(), 6) > 0; compare(c, wide);
                            bool near = false;
                            if (wide) {
                                near = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 3) < 0;
                                if (!near) near = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 8) > 0;
                                compare(c, near);
                            }
                            if (near) {
                                leave(c, {{3, player}, {4, 1}}); c.link(0x1027acf4); resolve_direct_action(c, c.r(3), c.r(4)); c.put16(player + 0x19c, 16); return finish(0, 0x1027ad0c);
                            }
                            if (!pursuit_half() && compare(c, c.i16(player + 0x2a), -20) > 0 && compare(c, std::int32_t(c.r(21))) < 0) {
                                c.set(19, c.r(19) + 4000); c.put8(frame.at(0x102), 1);
                            }
                        }
                    }
                }
            }
        }
    }
    c.set_f(0, c.f64(doubles + 8)); fp.fcmp(0, c.f(31), c.f(0));
    if ((c.cr(0) & 8) && compare(c, std::int32_t(c.r(21))) > 0) {
        const auto high = high_product(c.r(21), 0x66666667); c.set(3, shifted_sign(high, 1)); c.set(20, c.r(20) - std::uint32_t(std::int32_t(c.r(21)) / 5));
    }
    enter(c, 0x1027adb8, {player}); current_role(c, player);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        if (compare(c, c.i8(player + 0x10d)) != 0) {
            c.set_f(2, c.f32(singles + 0xf8)); c.set_f(1, c.f32(player + 0xa0)); c.set_f(0, c.f32(singles + 0x158));
            c.set_f(0, fp.multiply_add(0x1027addc, c.f(2), c.f(1), c.f(0))); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0)));
            c.set(19, c.r(19) - c.u32(frame.at(0xa8)));
        }
        if (pursuit_half()) c.set(21, c.r(21) - 25000);
    }
    if (compare_logical(c, c.r(23)) != 0 && !pursuit_half()) {
        enter(c, 0x1027ae9c, {c.r(23)}); current_role(c, c.r(23));
        if (compare(c, c.r(3) & 1) == 0) c.set(21, c.r(21) + 5000);
        else if (compare(c, c.i8(c.r(23) + 0x2c)) != 0) {
            c.set_f(1, c.f32(singles + 0xf0)); c.set_f(0, c.f32(singles)); c.set_f(1, fp.fmuls(c.f(1), c.f(31))); c.set(13, c.u8(player + 0x10e));
            fp.fcmp(0, c.f(1), c.f(0));
            if (c.cr(0) & 2) c.set(14, 0); else keeper_distance_draw(KeeperDraw::running, 0x1027aef8, 0x1027b020);
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))));
            if (compare(c, std::int32_t(c.r(3) + 5), std::int32_t(c.r(14))) > 0) c.set(21, c.r(21) + 1500);
        }
        const Player keeper{c, c.r(23)}; const bool second_goal = compare(c, unsigned(std::int8_t(actor.side()) == 0), 1) == 0; c.set(3, 0);
        bool inside = compare(c, keeper.column(), 2) >= 0 && compare(c, keeper.column(), 6) <= 0;
        if (inside) inside = second_goal ? compare(c, keeper.row(), 10) >= 0 && compare(c, keeper.row(), 11) <= 0 : compare(c, keeper.row()) >= 0 && compare(c, keeper.row(), 1) <= 0;
        c.set(3, inside); compare(c, inside);
        if (!inside) {
            c.set_f(0, c.f32(singles)); c.set(13, c.u8(player + 0x10e)); fp.fcmp(0, c.f(31), c.f(0));
            if (c.cr(0) & 2) c.set(14, 0); else keeper_distance_draw(KeeperDraw::outside_area, 0x1027b1dc, 0x1027b2e4);
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))));
            if (compare(c, std::int32_t(c.r(3) + 5), std::int32_t(c.r(14))) > 0) {
                c.set_f(2, c.f32(singles + 0x1c)); c.set_f(1, c.f32(player + 0x98)); c.set_f(0, c.f32(singles + 0x148));
                c.set_f(0, fp.multiply_add(0x1027b394, c.f(2), c.f(1), c.f(0))); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) + c.u32(frame.at(0xa8)));
            }
        } else {
            const bool second = compare(c, keeper.side(), 1) == 0; c.set(3, 0);
            bool on_line = compare(c, c.u8(keeper.address + 0x105), 4) == 0;
            if (on_line) on_line = second ? compare(c, c.u8(keeper.address + 0x106), 11) == 0 : compare(c, keeper.row()) == 0;
            c.set(3, on_line); compare(c, on_line);
            if (!on_line) {
                c.set_f(1, c.f32(singles + 0xe4)); c.set_f(0, c.f32(singles)); c.set_f(1, fp.fmuls(c.f(1), c.f(31))); c.set(13, c.u8(player + 0x10e));
                c.set_f(1, fp.fmuls(c.f(1), c.f(31))); fp.fcmp(0, c.f(1), c.f(0));
                if (c.cr(0) & 2) c.set(14, 0); else keeper_distance_draw(KeeperDraw::off_line, 0x1027b450, 0x1027b588);
                c.set(4, c.u8(player + 0x17)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(4)))));
                if (compare(c, std::int8_t(c.r(13)) - 10 + std::int8_t(c.r(4)), std::int32_t(c.r(14))) > 0) {
                    c.set_f(2, c.f32(singles + 0x80)); c.set_f(1, c.f32(player + 0x98)); c.set_f(0, c.f32(singles + 0xdc));
                    c.set_f(0, fp.multiply_add(0x1027b674, c.f(2), c.f(1), c.f(0))); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) + c.u32(frame.at(0xa8)));
                }
                c.set(3, std::uint32_t(std::int32_t(actor.column())));
                if (compare(c, actor.column(), keeper.column()) == 0) {
                    c.set(3, std::uint32_t(std::int32_t(actor.row())));
                    if (compare(c, actor.row(), keeper.row()) == 0) {
                        c.set(3, c.u8(player + 0x17)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(2, c.f64(doubles + 0x80));
                        c.set_f(3, c.f32(player + 0x98)); c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000);
                        c.set_f(4, c.f32(singles + 0x1c)); c.set_f(1, c.f64(frame.at(0xa4))); c.set_f(0, c.f32(singles + 0x14c));
                        c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(1, fp.fadds(c.f(3), c.f(1)));
                        c.set_f(0, fp.multiply_add(0x1027b6ec, c.f(4), c.f(1), c.f(0))); c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
                    }
                }
            } else {
                enter(c, 0x1027b714, {keeper.address, c.u8(player + 0x105), c.u8(player + 0x106)}); can_cover_goal(c, keeper.address, c.r(4), c.r(5));
                if (compare(c, std::int8_t(c.r(3))) == 0) {
                    c.set_f(1, c.f32(singles + 0x5c)); c.set_f(0, c.f32(singles)); c.set_f(1, fp.fmuls(c.f(1), c.f(31))); c.set(13, c.u8(player + 0x10e));
                    fp.fcmp(0, c.f(1), c.f(0));
                    if (c.cr(0) & 2) c.set(14, 0); else keeper_distance_draw(KeeperDraw::blocked_line, 0x1027b75c, 0x1027b884);
                    if (compare(c, std::int8_t(c.r(13)), std::int32_t(c.r(14))) > 0) {
                        c.set_f(2, c.f32(singles + 0x80)); c.set_f(1, c.f32(player + 0x98)); c.set_f(0, c.f32(singles + 0xb0));
                        c.set_f(0, fp.multiply_add(0x1027b950, c.f(2), c.f(1), c.f(0))); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) + c.u32(frame.at(0xa8)));
                    }
                }
            }
        }
    }

    c.set(3, actor.match()); bool special_distance = compare(c, c.u8(c.r(3) + 0x791f), 4) == 0;
    if (special_distance) { c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(31), c.f(0)); special_distance = (c.cr(0) & 4) != 0; }
    if (special_distance) {
        c.set_f(0, fp.fmuls(c.f(31), c.f(31))); c.set_f(1, c.f64(doubles + 0x388)); c.set_f(0, fp.fmul(c.f(1), c.f(0)));
        c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) - c.u32(frame.at(0xa8)));
    } else if (compare(c, actor.column(), 3) > 0 && compare(c, actor.column(), 7) < 0) {
        bool end_row = compare(c, actor.side(), 1) == 0 && compare(c, actor.row()) == 0;
        if (!end_row) end_row = compare(c, std::int8_t(actor.side())) == 0 && compare(c, c.u8(player + 0x106), 11) == 0;
        if (end_row) {
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - 4)); c.call(0x10775540, 0x1027ba00); c.set(21, c.r(21) + 5000u - c.r(3) * 2000);
        }
    }
    if (compare_logical(c, c.r(22)) == 0 && compare(c, c.i8(frame.at(0x100))) == 0) {
        c.set(3, c.u32(frame.at(0x68))); c.put32(frame.at(0x68), c.r(3) - 2000);
    }
    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.u8(player + 0x106)}}); c.call(0x101ee9f0, 0x1027ba4c);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x791f), -1) != 0 && compare(c, c.i8(player + 0x109)) != 0) {
            c.set(4, actor.match()); c.set(3, c.u32(frame.at(0x68)));
            if (compare(c, std::int32_t(c.r(3)), 4000 - c.i16(c.r(4) + 0x7948)) < 0) {
                leave(c, {{3, actor.match()}, {4, actor.side()}}); c.call(0x101fd000, 0x1027ba98);
                if (compare(c, std::int8_t(c.r(3))) != 0) {
                    enter(c, 0x1027baa8, {player}); match_context(c, player);
                    leave(c, {{3, player}, {4, c.r(3)}, {5, 1}}); c.link(0x1027bab8); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x1027bac8);
                }
            }
        }
    }
    bool pressure = goal_area(false);
    if (pressure) {
        c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(29), c.f(0)); const auto bits = c.cr(0);
        c.set_cr(0, (bits & ~2u) | ((bits & 10) ? 2u : 0u)); pressure = (c.cr(0) & 2) != 0;
    }
    if (!pressure) {
        leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.u8(player + 0x106)}}); c.call(0x101ee9f0, 0x1027bbac);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(29), c.f(0)); pressure = (c.cr(0) & 8) != 0;
        }
    }
    if (pressure) {
        c.set_f(2, c.f32(singles + 0xb0)); c.set_f(1, c.f32(singles + 0x14c)); c.set_f(0, c.f32(singles));
        c.set_f(1, fp.multiply_add(0x1027bbcc, c.f(2), c.f(29), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
        if (c.cr(0) & 2) c.set(13, 0); else pressure_draw();
        c.set(3, c.u32(frame.at(0x68))); const auto score = c.r(3) + 2000; c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 2000);
        c.set(18, score); bounded_draw(18, 14, 0xa4, 0xac, 0xb4, 0x1027be30, 0x1027bf08);
        if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(14))) > 0) {
            c.set(3, actor.match());
            if (compare(c, c.i8(c.r(3) + 0x791f), -1) != 0 && compare(c, std::int8_t(c.r(16)), 4) != 0) {
                enter(c, 0x1027bf9c, {player}); match_context(c, player);
                leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x1027bfac); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x1027bfbc);
            }
        }
    }

    leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(player + 0x106)}}); c.call(0x101e4ea0, 0x1027bfd4);
    bool force_movement = compare(c, std::int8_t(c.r(3))) >= 0;
    if (!force_movement) {
        leave(c, {{3, actor.match()}, {4, player}, {5, 0}}); c.call(0x101d8d70, 0x1027bfec);
        if (compare(c, std::int8_t(c.r(3))) != 0 && compare(c, c.i8(frame.at(0x100))) == 0) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027c00c); force_movement = compare(c, std::int16_t(c.r(3)), 1) != 0;
        }
    }
    if (force_movement) {
        c.set_f(1, c.f32(singles + 0xdc)); c.set_f(0, c.f32(singles + 0xdc)); const auto mode_order = compare(c, std::int8_t(c.r(16)), 4);
        c.set(20, c.r(20) - 5000); c.set_f(0, fp.multiply_add(0x1027c02c, c.f(1), c.f(31), c.f(0)));
        c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xa8)));
        if (mode_order == 0) {
            c.set_f(1, c.f32(singles + 0xd8)); c.set_f(0, c.f32(singles + 0xdc)); c.set_f(0, fp.multiply_add(0x1027c04c, c.f(1), c.f(31), c.f(0)));
            c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
        }
        if (pursuit_half()) c.set(21, c.r(21) - 3000);
        if (goal_area(true)) { c.set(20, c.r(20) - 1000); c.set(21, c.r(21) + 1000); }
    } else {
        if (goal_area(true)) c.set(20, c.r(20) - 1000);
        if (goal_area(false)) { c.set(20, c.r(20) - 1000); c.set(19, c.r(19) - 1200); }
        else if (pursuit_half() && !target_open(0x1027c39c, 0x1027c3f4)) {
            c.set(3, std::uint32_t(std::int32_t(c.i16(frame.at(0x58)))) * 100); c.set(19, c.r(19) - c.r(3) - 500);
        }
    }
    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027c418);
    if (compare(c, std::int16_t(c.r(3)), 1) != 0 && !goal_area(false)) {
        c.set_f(1, c.f32(player + 0x98)); c.set_f(0, c.f32(singles + 0xf0)); c.set_f(2, c.f64(doubles + 0x228));
        c.set_f(0, fp.fsubs(c.f(1), c.f(0))); c.set_f(1, c.f64(doubles + 0x390)); c.set_f(0, fp.multiply_add(0x1027c4e4, c.f(2), c.f(29), c.f(0)));
        c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xa8)));
    } else {
        c.set_f(1, c.f32(singles + 0x160)); c.set_f(0, c.f32(singles + 0x15c)); c.set_f(0, fp.multiply_add(0x1027c508, c.f(1), c.f(29), c.f(0)));
        c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(20, c.r(20) - c.u32(frame.at(0xa8)));
    }
    if (!target_open(0x1027c57c, 0x1027c5d4) && !pursuit_half()) c.set(19, c.r(19) + 2000u - c.u32(frame.at(0x68)));
    c.set(4, c.u8(frame.at(0x53))); c.set(5, c.u8(frame.at(0x52))); c.put8(frame.at(0x4d), c.r(4)); c.put8(frame.at(0x4c), c.r(5));
    const auto offer_mode = unsigned(c.i8(frame.at(0xff)) != 0) + 2;
    leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x4d)}, {6, frame.at(0x4c)}, {7, offer_mode}}); c.call(0x101d8260, 0x1027c674); c.set(18, c.r(3));
    const auto wide_column = [&] { const bool wide = compare(c, actor.column(), 2) < 0 || compare(c, actor.column(), 6) > 0; compare(c, wide); return wide; };
    const auto near_attacking_end = [&] {
        bool near = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 3) < 0;
        if (!near) near = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 8) > 0;
        compare(c, near); return near;
    };
    if (compare(c, std::int8_t(c.r(18))) != 0) {
        if (compare(c, std::int8_t(c.r(16)), 4) == 0) {
            auto column = c.i8(frame.at(0x4d));
            if (compare(c, column, 3) < 0) column = 3;
            else if (compare(c, column, 5) > 0) column = 5;
            c.put8(frame.at(0x4d), column);
        }
        c.set(20, c.r(20) - 3000);
        if (wide_column() && !near_attacking_end() && !pursuit_half()) { c.set(19, c.r(19) + 1250); c.set(21, c.r(21) - 500); }
        else if (near_attacking_end() && !goal_area(true)) { c.set(19, c.r(19) + 1000); c.set(21, c.r(21) - 2000u + std::uint32_t(std::int32_t(c.i8(player + 0x10c))) * 100); }
    } else {
        if (wide_column() && !near_attacking_end()) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027c954); bool primary_wide = false;
            if (compare(c, c.r(3) & 0x38) != 0) {
                enter(c, 0x1027c964, {player}); both_roles_masked(c, player);
                if (compare(c, std::int8_t(c.r(3))) != 0) {
                    bool near = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 3) < 0;
                    if (!near) near = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 8) > 0;
                    compare(c, near); primary_wide = !near;
                }
            }
            if (primary_wide) {
                c.set(3, c.u8(player + 0x10c)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f32(player + 0x98));
                c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000); c.set_f(3, c.f32(singles + 0x5c));
                c.set_f(0, fp.fsubs(c.f64(frame.at(0xa4)), c.f(1))); c.set_f(0, fp.fadds(c.f(2), c.f(0))); c.set_f(0, fp.fmuls(c.f(3), c.f(0)));
                c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
            } else {
                leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027ca14); bool alternative_wide = compare(c, c.r(3) & 0x38) != 0;
                if (alternative_wide) { leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027ca28); alternative_wide = compare(c, std::uint32_t(std::int32_t(std::int16_t(c.r(3)))) & 0x880) != 0; }
                if (alternative_wide) {
                    c.set(3, c.u8(player + 0x10c)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f32(player + 0x98));
                    c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000); c.set_f(4, c.f32(singles + 0x5c));
                    c.set_f(1, c.f64(frame.at(0xa4))); c.set_f(0, c.f32(singles + 0xf8)); c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(1, fp.fadds(c.f(3), c.f(1)));
                    c.set_f(0, fp.multiply_add(0x1027ca68, c.f(4), c.f(1), c.f(0))); c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
                } else c.set(19, c.r(19) - 3000);
            }
            c.set(21, c.r(21) - 650);
        } else c.set(19, c.r(19) - 3000);
    }

    bool direct_shot = !goal_area(true) && !wide_column();
    if (direct_shot) { c.set(3, actor.match()); direct_shot = compare(c, c.u8(c.r(3) + 0x791f), 4) != 0; }
    if (direct_shot) {
        leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027cb9c); const bool special = compare(c, c.r(3) & 0x40) != 0;
        polynomial_draw(special); c.set(21, c.r(21) + std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * (special ? 500u : 325u) - c.r(14));
    } else if (goal_area(true) && !wide_column()) {
        c.set(3, c.u8(player + 0x10c)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f32(player + 0x9c));
        c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000); c.set_f(4, c.f32(singles + 0x84));
        c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(3, c.f32(singles + 0xf0)); c.set_f(1, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, c.f32(player + 0xe4));
        c.set_f(3, fp.multiply_add(0x1027d560, c.f(4), c.f(31), c.f(3))); c.set_f(1, fp.fadds(c.f(2), c.f(1))); c.set_f(0, fp.fsubs(c.f(1), c.f(0)));
        c.set_f(0, fp.fmuls(c.f(3), c.f(0))); c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) + c.u32(frame.at(0xb0)));
    } else if (!pursuit_half()) {
        c.set(3, c.u8(player + 0x10c)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f32(singles + 0x38));
        c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3))) * 75) ^ 0x80000000); c.set_f(0, fp.fsubs(c.f64(frame.at(0xa4)), c.f(1)));
        c.set_f(0, fp.multiply_add(0x1027d5fc, c.f(2), c.f(31), c.f(0))); c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) - c.u32(frame.at(0xb0)));
    }
    c.set_f(0, c.f32(player + 0xe4)); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xa8)));
    bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027d650, 0x1027d728);

    bool direct_boost = compare(c, std::int32_t(c.r(13))) == 0;
    if (direct_boost) {
        leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1027d7a0);
        if (compare(c, c.r(3) & 0x40) == 0) {
            advance(0x1027d7c8); const auto value = sample(); c.set(3, value >> 31); direct_boost = compare(c, std::int32_t(value) % 4) == 0;
        }
    }
    const auto middle_distance = [&] {
        c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, c.f(31), c.f(0)); const auto bits = c.cr(0);
        c.set_cr(0, (bits & ~2u) | ((bits & 6) ? 2u : 0u));
        if (!(c.cr(0) & 2)) return false;
        c.set_f(0, c.f64(doubles + 0x358)); fp.fcmp(0, c.f(31), c.f(0)); return (c.cr(0) & 8) != 0;
    };
    if (direct_boost) {
        bool active_traits = pursuit_half();
        if (active_traits) active_traits = compare(c, c.i8(player + 0x109)) != 0 || compare(c, c.i8(player + 0x10e)) != 0;
        if (!active_traits) {
            c.set(13, actor.side()); c.set_f(1, c.f32(player + 0x98)); c.set_f(2, c.f64(doubles + 8)); c.call(0x10780fc0, 0x1027d86c); c.set(2, c.u32(frame.at(0x14)));
            const bool second = compare(c, std::int8_t(c.r(13)), 1) == 0; c.set(3, 0);
            bool inside = compare(c, actor.column(), 2) >= 0 && compare(c, actor.column(), 6) <= 0;
            if (inside) inside = second ? compare(c, actor.row(), 10) >= 0 && compare(c, actor.row(), 11) <= 0 : compare(c, actor.row()) >= 0 && compare(c, actor.row(), 1) <= 0;
            c.set(3, inside ? 1000 : 0); c.put32(frame.at(0xa4), 0x43300000); c.set_f(2, c.f64(doubles + 0x80)); c.put32(frame.at(0xa8), (1000u - c.r(3)) ^ 0x80000000);
            c.set_f(0, fp.fsub(c.f64(frame.at(0xa4)), c.f(2))); c.set_f(0, fp.fadd(c.f(1), c.f(0)));
            c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb0)));
        }
        if (middle_distance() && !wide_column()) {
            if (compare(c, c.i8(player + 0x10c)) != 0) {
                c.set_f(0, c.f32(player + 0xb4)); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xa8)));
                bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027d9ec, 0x1027dac4);
                c.set(3, extended(c.i16(player + 0x2a)) * 2); c.set(21, c.r(21) + c.r(3) + c.r(13) * 400 + 2500);
            }
        } else if (goal_area(true)) {
            c.set(3, c.u8(player + 0x10c)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
            c.set_f(4, c.f32(singles + 0x84)); c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000);
            c.set_f(3, c.f32(singles + 0xf0)); c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(2, c.f32(player + 0x9c));
            c.set_f(3, fp.multiply_add(0x1027dc2c, c.f(4), c.f(31), c.f(3))); c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
            c.set_f(0, fp.fadds(c.f(2), c.f(0))); c.set_f(0, fp.fmuls(c.f(3), c.f(0)));
            c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) + c.u32(frame.at(0xb0)));
        }
    } else if (compare(c, std::int8_t(c.r(16)), 4) != 0) {
        enter(c, 0x1027dc64, {player}); strategy_bit25(c, player);
        bool carry = compare(c, std::int8_t(c.r(3))) != 0 && !goal_area(false) && compare(c, c.i8(frame.at(0xfd)), 2) < 0;
        if (carry) {
            c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, c.f(31), c.f(0)); const auto bits = c.cr(0);
            c.set_cr(0, (bits & ~2u) | ((bits & 6) ? 2u : 0u)); carry = (c.cr(0) & 2) != 0;
        }
        if (carry && compare(c, c.i16(frame.at(0x58))) != 0 && pursuit_half()) {
            c.set(13, c.u8(player + 0x109)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * 2);
            const auto limit = [&] { return std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * 2 + 20; };
            c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 20);
            if (compare(c, std::int32_t(limit())) == 0) c.set(14, 0);
            else {
                advance(0x1027ddcc); c.set(3, 65535); c.set(4, limit() - 20); compare(c, std::int32_t(limit()), 65535);
                c.set(3, std::uint32_t(-65535)); c.set(4, limit() - 20); compare(c, std::int32_t(limit()), -65535);
                // The signed-byte trait makes this bound lie in[-236,274].
                c.set(3, c.u32(cursor)); c.set(6, c.u16(c.r(3))); c.set(5, c.u32(seed)); c.set(4, limit());
                c.s.wr<1>(reg::xer_ca, limit() - 20 > UINT32_MAX - 20); compare(c, std::int32_t(limit())); c.set(3, c.r(6) + c.r(5));
                const auto residual = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(4))) * c.r(4);
                c.set(14, std::int32_t(limit()) > 0 ? residual : 0u - residual);
            }
            c.set(3, c.r(14) ^ 0x80000000); c.put32(frame.at(0xa8), c.r(3)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xa4), 0x43300000);
            c.set_f(0, fp.fsubs(c.f64(frame.at(0xa4)), c.f(1))); fp.fcmp(0, c.f(0), c.f(31)); carry = (c.cr(0) & 8) != 0;
        }
        if (carry) {
            c.set(7, 0x43300000); c.set(3, 0x43300000); c.set(5, extended(c.i16(player + 0x2a)) * 10);
            c.set(6, 4000u - c.u32(frame.at(0x68))); c.set(8, std::uint32_t(std::int32_t(c.i8(player + 0x10c))));
            c.set(4, c.r(5) + c.u8(player + 0x11) * 300 + c.r(6)); c.put32(frame.at(0xa4), c.r(7));
            c.set_f(1, c.f32(player + 0xe4)); c.set(5, c.r(8) ^ 0x80000000); c.set(4, c.r(4) ^ 0x80000000); c.put32(frame.at(0xa8), c.r(5));
            c.set_f(0, c.f32(player + 0x98)); const auto first_column = compare(c, actor.column(), 2);
            c.put32(frame.at(0xb0), c.r(4)); c.set_f(3, c.f64(doubles + 0x80)); c.set_f(4, fp.fmuls(c.f(1), c.f(0))); c.put32(frame.at(0xac), c.r(3));
            c.set_f(2, c.f64(frame.at(0xa4))); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xac)));
            c.set_f(2, fp.fsubs(c.f(2), c.f(3))); c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.multiply_add(0x1027dff8, c.f(4), c.f(2), c.f(0)));
            c.put64(frame.at(0xb4), fp.fctiwz(0, c.f(0))); c.set(19, c.r(19) + c.u32(frame.at(0xb8)));
            const bool wide = first_column < 0 || compare(c, actor.column(), 6) > 0; compare(c, wide);
            if (wide) {
                enter(c, 0x1027e03c, {player}); both_roles_masked(c, player);
                if (compare(c, std::int8_t(c.r(3))) != 0 && !near_attacking_end()) {
                    enter(c, 0x1027e098, {player}); alternate_strategy_flags(c, player);
                    c.set(6, 0); c.set(5, c.r(3) & 4); c.set(4, 0); c.set(3, 0);
                    if (compare(c, c.r(5)) != 0) { c.set(3, c.u32(frame.at(0x68))); c.set(19, c.r(19) + 500); c.put32(frame.at(0x68), c.r(3) - 250); }
                }
            }
        }
        enter(c, 0x1027e0d8, {player}); strategy_bit27(c, player);
        if (compare(c, std::int8_t(c.r(3))) != 0 && !pursuit_half() && !target_open(0x1027e18c, 0x1027e1e4)) c.set(20, c.r(20) + 3500);
        enter(c, 0x1027e1f8, {player}); strategy_bit29(c, player);
        if (compare(c, std::int8_t(c.r(3))) != 0 && middle_distance()) {
            c.set_f(0, c.f32(player + 0xb4)); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xa8)));
            bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027e25c, 0x1027e334);
            c.set(3, extended(c.i16(player + 0x2a)) * 2); c.set(21, c.r(21) + c.r(13) * 400 + c.r(3));
        }
    }

    c.set(3, actor.match()); bool turning = compare(c, c.i8(c.r(3) + 0x791f), 2) < 0;
    if (!turning) {
        c.set_f(0, c.f64(doubles + 0x328)); fp.fcmp(0, c.f(31), c.f(0));
        if (!(c.cr(0) & 4)) turning = false;
        else { c.set(3, actor.match()); turning = compare(c, c.i8(c.r(3) + 0x791f), 4) < 0; }
    }
    if (turning) {
        c.set_f(0, c.f64(doubles + 0x10)); c.set_f(1, fp.fadd(c.f(0), c.f(31))); c.set_f(1, fp.frsp(c.f(1)));
        enter(c, 0x1027e434, {player, 22, c.u8(player + 0x105), c.u8(player + 0x106), 4, std::int8_t(actor.side()) == 1 ? 0u : 11u}); c.set(10, frame.at(0x5a));
        turn_direction(c, player, 22, c.f(1), c.r(10));
        if (compare(c, std::int8_t(c.r(3)), 2) != 0) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            if (compare(c, std::int32_t(c.r(3)), c.i8(player + 0x2d)) != 0 && compare(c, c.i8(frame.at(0xff))) == 0) {
                c.set(14, std::uint32_t(std::int32_t(c.i8(player + 0x2e))) * 300);
                // A signed-byte multiplier keeps this range within[-38400,38100].
                bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027e49c, 0x1027e574); c.set(21, c.r(21) - c.r(13));
            }
        }
    }
    c.set_f(0, c.f64(doubles + 0x18)); fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) {
        c.set(4, extended(c.i16(frame.at(0x58)))); c.set(3, extended(c.i16(frame.at(0x58)))); c.set_f(0, c.f64(doubles + 0xb8));
        c.set(3, c.r(4) * c.r(3)); c.set(4, extended(c.i16(frame.at(0x58)))); c.set_f(0, fp.fsub(c.f(0), c.f(31))); c.put32(frame.at(0xa4), 0x43300000);
        c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, fp.frsp(c.f(0))); c.put32(frame.at(0xa8), (c.r(4) * c.r(3)) ^ 0x80000000);
        c.set_f(1, fp.fsubs(c.f64(frame.at(0xa4)), c.f(2))); c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
        c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(21, c.r(21) - c.u32(frame.at(0xb0)));
    }
    c.set(3, actor.match()); if (compare(c, c.u8(c.r(3) + 0x791f), 3) == 0) c.set(21, c.r(21) - 2000);
    c.set_f(4, c.f32(singles + 0x94)); c.set_f(1, fp.fmuls(c.f(30), c.f(30))); c.set_f(3, c.f32(player + 0xa0));
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(18))))); c.set_f(2, c.f32(singles + 0xa8)); c.set_f(0, c.f64(doubles + 0x10));
    c.set_f(2, fp.multiply_add(0x1027e668, c.f(4), c.f(3), c.f(2))); c.set(4, std::uint32_t(std::int32_t(c.i8(frame.at(0xfe))) - c.r(3)));
    fp.fcmp(0, c.f(31), c.f(0)); const auto distance_bits = c.cr(0); c.set_f(0, fp.fmuls(c.f(2), c.f(1)));
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(18))))); const auto movement_penalty = std::uint32_t(std::int32_t(c.i8(frame.at(0xfe))) - c.r(3)) * 100;
    c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0))); c.set(20, c.r(20) - movement_penalty); c.set(3, c.r(4) * 100); c.set(19, c.r(19) - c.u32(frame.at(0xa8)) - c.r(3));
    c.set_cr(0, (distance_bits & ~2u) | ((distance_bits & 6) ? 2u : 0u));

    bool opening = (c.cr(0) & 2) != 0;
    if (opening) {
        c.set_f(1, c.f32(singles + 0xd0)); c.set_f(0, c.f32(singles)); c.set_f(1, fp.fmuls(c.f(1), c.f(31)));
        keep(c, {{13, c.u8(player + 0x10c)}, {14, c.u8(player + 0x10c)}, {25, c.u8(player + 0x10c)}, {24, c.u8(player + 0x10c)}});
        fp.fcmp(0, c.f(1), c.f(0));
        if (c.cr(0) & 2) c.set(23, 0); else keeper_distance_draw(KeeperDraw::movement_opening, 0x1027e700, 0x1027e828, 23);
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(25))))); auto product = c.r(3) * std::uint32_t(std::int32_t(std::int8_t(c.r(24))));
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(14))))); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))));
        product = c.r(4) * (c.r(3) * product); opening = compare(c, std::int32_t(product), std::int32_t(c.r(23))) > 0;
    }
    if (opening) {
        bool add_movement = !goal_area(false);
        if (add_movement) {
            c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, c.f(31), c.f(0)); const auto bits = c.cr(0);
            c.set_cr(0, (bits & ~2u) | ((bits & 6) ? 2u : 0u)); add_movement = (c.cr(0) & 2) != 0;
        }
        if (add_movement) add_movement = compare(c, c.i8(frame.at(0xfd)), 2) < 0;
        if (add_movement) {
            bool active_traits = pursuit_half();
            if (active_traits) active_traits = compare(c, c.i8(player + 0x109)) != 0 || compare(c, c.i8(player + 0x10e)) != 0;
            if (!active_traits) {
                c.set_f(1, c.f32(player + 0x98)); c.set_f(2, c.f64(doubles + 8)); c.call(0x10780fc0, 0x1027ea3c); c.set(2, c.u32(frame.at(0x14)));
                c.set(5, 10000u - c.u32(frame.at(0x68))); c.set(3, extended(c.i16(player + 0x2a))); c.put32(frame.at(0xa4), 0x43300000);
                c.set(4, c.r(5) ^ 0x80000000); c.set_f(3, c.f64(doubles + 0x80)); c.put32(frame.at(0xa8), c.r(4));
                c.set(3, c.r(3) * 3); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xa4))); c.set(3, c.r(3) ^ 0x80000000);
                c.put32(frame.at(0xac), 0x43300000); c.set_f(0, fp.fsub(c.f(0), c.f(3))); c.put32(frame.at(0xb0), c.r(3)); c.set_f(1, fp.fadd(c.f(0), c.f(1)));
                c.set_f(0, fp.fsub(c.f64(frame.at(0xac)), c.f(2))); c.set_f(0, fp.fadd(c.f(0), c.f(1))); c.put64(frame.at(0xb4), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xb8)));
                bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027eacc, 0x1027eba4); c.set(19, c.r(19) + c.r(13));
            }
        }
        c.set(3, actor.match()); bool shot_bonus = compare(c, c.i8(c.r(3) + 0x791f), 4) < 0 && compare(c, std::int8_t(c.r(17)), 2) < 0;
        if (shot_bonus) {
            c.set_f(0, c.f64(doubles + 0x18)); fp.fcmp(0, c.f(31), c.f(0)); shot_bonus = (c.cr(0) & 8) != 0;
            if (!shot_bonus) {
                c.set_f(0, c.f64(doubles + 0x70)); fp.fcmp(0, c.f(31), c.f(0));
                if (c.cr(0) & 8) {
                    advance(0x1027ec64); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x10624dd3), 5)); c.set(4, value);
                    shot_bonus = compare(c, std::int32_t(value) % 500) == 0;
                }
            }
        }
        if (shot_bonus) {
            c.set_f(1, c.f32(singles + 0x5c)); c.set_f(0, c.f32(singles)); c.set_f(1, fp.fmuls(c.f(1), c.f(31))); fp.fcmp(0, c.f(1), c.f(0));
            if (c.cr(0) & 2) c.set(13, 0); else keeper_distance_draw(KeeperDraw::blocked_line, 0x1027ecdc, 0x1027ee04, 13);
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(13))))); const auto candidate = compare(c, std::int32_t(c.r(3)), 10) < 0 ? 10 : std::int32_t(c.r(3));
            c.set(14, std::uint32_t(std::int32_t(std::int8_t(candidate)))); c.set(3, 400000);
            const auto limit = [&] { return 400000 / std::int8_t(c.r(14)); };
            // max(10,signedbyte(draw)) lies in[10,127], so this second range
            // lies in[3149,40000]: nonzero, positive and never wide.
            compare(c, limit()); advance(0x1027ef0c);
            c.set(4, 400000); c.set(3, 65535); compare(c, limit(), 65535);
            c.set(4, 400000); c.set(3, std::uint32_t(-65535)); compare(c, limit(), -65535);
            c.set(3, 400000); c.set(4, c.u32(cursor)); c.set(6, c.u16(c.r(4))); c.set(5, std::uint32_t(limit())); c.set(4, c.u32(seed)); c.set(6, c.r(6) + c.r(4));
            c.set(4, std::uint32_t(divide_signed<4>(c.r(6), c.r(5))) * c.r(5)); compare(c, limit()); c.set(13, c.r(6) - c.r(4)); c.set(21, c.r(21) + c.r(13));
        }
        c.set(20, c.r(20) - 10000);
    } else {
        if (!goal_area(false)) {
            c.set(13, c.u8(player + 0x109)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f64(doubles + 0x120));
            c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) ^ 0x80000000); c.set_f(0, c.f64(doubles + 0x88));
            c.set_f(1, fp.fsub(c.f64(frame.at(0xa4)), c.f(2))); c.set_f(1, fp.fmul(c.f(3), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
            if (c.cr(0) & 2) c.set(14, 0);
            else {
                advance(0x1027f1dc);
                const auto range = [&] {
                    c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) ^ 0x80000000); c.put32(frame.at(0xa4), 0x43300000);
                    c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0xa8), c.r(4)); c.set_f(2, c.f64(doubles + 0x120));
                    c.set_f(0, fp.fsub(c.f64(frame.at(0xa4)), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
                    c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); return c.i32(frame.at(0xb0));
                };
                c.set(3, 65535); compare(c, range(), 65535); c.set(3, std::uint32_t(-65535)); compare(c, range(), -65535);
                // The pinned double coefficient is30, so signedbyte*30 lies
                // in[-3840,3810]. The source's wide expansion is unreachable.
                c.set(4, 0x43300000); c.set(5, c.u32(cursor)); const auto trait_bits = std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) ^ 0x80000000;
                c.put32(frame.at(0xa8), trait_bits); c.set(3, trait_bits); c.put32(frame.at(0xa4), 0x43300000);
                c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xa4))); c.set_f(2, c.f64(doubles + 0x120)); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
                c.set(5, c.u16(c.r(5))); c.set(4, c.u32(seed)); c.put32(frame.at(0xb8), trait_bits); c.set_f(2, fp.fmul(c.f(2), c.f(0)));
                c.set(5, c.r(5) + c.r(4)); c.put32(frame.at(0xb4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0xb4)));
                const auto first_tag = fp.fctiwz(2, c.f(2)); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(1, c.f64(doubles + 0x120)); c.put64(frame.at(0xac), first_tag);
                c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set(4, c.u32(frame.at(0xb0))); c.set(3, std::uint32_t(divide_signed<4>(c.r(5), c.r(4))));
                c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0))); const auto sign_bound = c.i32(frame.at(0xc0)); compare(c, sign_bound);
                const auto residual = c.r(5) - c.r(3) * c.r(4); c.set(14, sign_bound > 0 ? residual : 0u - residual);
            }
            advance(0x1027f4d8); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x51eb851f), 6)); c.set(4, value);
            if (compare(c, std::int32_t(c.r(14)), std::int32_t(value) % 200) > 0) {
                c.set(23, 0); c.set(4, actor.match()); c.set(17, c.r(4) + 0xa08 + std::uint32_t(actor.column() * 0x450 + actor.row() * 0x5c));
                for (;;) {
                    const auto opposing = unsigned(std::int8_t(actor.side()) == 0); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(23))))); c.set(3, c.r(17) + opposing);
                    if (compare(c, std::int32_t(c.r(4)), c.i8(c.r(3) + 0x58)) >= 0) break;
                    const auto offset = unsigned(std::int8_t(actor.side()) == 0) * 0x2c + std::uint32_t(std::int32_t(std::int8_t(c.r(23)))) * 4;
                    c.set(3, c.u32(c.r(17) + offset));
                    if (compare(c, c.i8(c.r(3) + 0x24)) != 0) {
                        c.set(3, c.u8(player + 0x17)); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
                        c.set_f(3, c.f32(singles + 0xf4)); c.set_f(2, c.f32(player + 0x98)); c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(std::int8_t(c.r(3))) * 100) ^ 0x80000000);
                        c.set_f(0, fp.fsubs(c.f64(frame.at(0xa4)), c.f(1))); c.set_f(0, fp.multiply_add(0x1027f5ac, c.f(3), c.f(2), c.f(0)));
                        c.put64(frame.at(0xac), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(frame.at(0xb0)));
                        bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027f5ec, 0x1027f6c4); c.set(19, c.r(19) + c.r(13));
                    }
                    c.set(23, c.r(23) + 1);
                }
            }
        }
    }

    c.set(3, actor.match()); c.set(14, 0); c.put32(frame.at(0xa4), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 8));
    c.put32(frame.at(0xa8), std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x791f))) ^ 0x80000000);
    c.set_f(1, fp.fsub(c.f64(frame.at(0xa4)), c.f(1))); c.call(0x10780fc0, 0x1027f794); c.set(2, c.u32(frame.at(0x14)));
    c.set_f(2, c.f32(singles + 0xd0)); const bool target_exists = compare_logical(c, c.r(22)) != 0; c.set_f(0, c.f32(player + 0x50)); c.set_f(3, c.f64(doubles + 0x38));
    c.set_f(2, fp.fsubs(c.f(2), c.f(0))); c.set_f(0, c.f32(singles + 0x3c)); c.set(3, extended(c.i16(player + 0x2a)));
    c.set_f(1, fp.fmul(c.f(3), c.f(1))); const auto condition_penalty = std::uint32_t(std::int32_t(c.i16(player + 0x1a0))) * 100; c.set(4, c.r(3) * 10);
    c.set_f(0, fp.fdivs(c.f(2), c.f(0))); c.put64(frame.at(0xb4), fp.fctiwz(0, c.f(0))); const auto context_tag = fp.fctiwz(0, c.f(1));
    c.set(3, c.u32(frame.at(0xb8))); c.put64(frame.at(0xac), context_tag); c.set(5, c.u32(frame.at(0xb0))); c.set(14, c.r(5) - c.r(4) + c.r(3) - condition_penalty);
    const auto pass_kind = [&](bool include_long) {
        if (include_long && compare(c, c.u8(frame.at(0x50)), 94) == 0) return true;
        c.set(3, c.u8(frame.at(0x50))); if (compare_logical(c, (c.r(3) - 7) & 0xff, 1) <= 0) return true;
        return compare(c, c.u8(frame.at(0x50)), 6) == 0;
    };
    const auto alternate_bit4 = [&](std::uint32_t back) {
        enter(c, back, {player}); alternate_strategy_flags(c, player); c.set(6, 0); c.set(5, c.r(3) & 4); c.set(4, 0); c.set(3, 0); return compare(c, c.r(5)) != 0;
    };
    if (target_exists && compare(c, c.i32(frame.at(0x68))) > 0 && pass_kind(true)) c.set(14, c.r(14) + c.u32(frame.at(0x68)));
    else if (alternate_bit4(0x1027f840)) {
        enter(c, 0x1027f870, {player}); both_roles_masked(c, player);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            bool end_row = compare(c, actor.row()) == 0;
            if (!end_row) end_row = compare(c, c.u8(player + 0x106), 11) == 0;
            compare(c, end_row);
            if (end_row) c.set(14, c.r(14) + 500);
            else if (wide_column()) {
                if (near_attacking_end()) c.set(19, c.r(19) + 2000);
                else { c.set_f(0, c.f64(doubles + 0x18)); fp.fcmp(0, c.f(31), c.f(0)); if (c.cr(0) & 8) c.set(19, c.r(19) + 6000); }
            }
        }
    }
    if (compare(c, c.i8(frame.at(0xfc))) == 0 && compare(c, std::int32_t(c.r(21)), 2000) > 0 && compare(c, c.i16(player + 0x2a), -50) > 0) {
        bounded_draw(21, 13, 0xa4, 0xac, 0xb4, 0x1027f994, 0x1027fa6c); c.set(14, c.r(14) + c.r(13));
    }
    if (compare(c, c.i8(player + 0x109)) == 0) { bounded_draw(14, 13, 0xa4, 0xac, 0xb4, 0x1027fb10, 0x1027fbe8); c.set(14, c.r(13)); }
    if (pursuit_half()) {
        const bool nonnegative = std::int32_t(c.r(14)) >= 0; c.set(4, 0); c.set(3, 0); c.s.wr<1>(reg::xer_ca, nonnegative);
        if (!nonnegative) c.set(14, 0);
    }
    c.set(19, c.r(19) - c.r(14)); leave(c, {{3, actor.match()}, {4, c.u8(player + 0x105)}}); c.call(0x101ee9c0, 0x1027fcc4);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x1027fcd4, {player}); match_context(c, player);
        if (compare(c, std::int8_t(c.r(3)), 3) < 0) {
            bool end_row = compare(c, std::int8_t(actor.side())) == 0 && compare(c, c.u8(player + 0x106), 11) == 0;
            if (!end_row) end_row = compare(c, actor.side(), 1) == 0 && compare(c, actor.row()) == 0;
            if (end_row) {
                if (compare_logical(c, c.r(22)) != 0 && pass_kind(false)) {
                    c.set(4, c.u32(frame.at(0x68))); c.put32(frame.at(0x68), c.r(4) + 1500);
                    if (alternate_bit4(0x1027fd4c)) { c.set(3, c.u32(frame.at(0x68))); c.put32(frame.at(0x68), c.r(3) + 1500); }
                } else {
                    advance(0x1027fda4); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x66666667), 2)); c.set(4, value);
                    if (compare(c, std::int32_t(value) % 10) != 0) {
                        enter(c, 0x1027fde8, {player}); match_context(c, player); leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x1027fdf8); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x1027fe08);
                    }
                }
            }
        }
    }
    uncertainty_draw(0x1027fe50); c.put32(frame.at(0x68), c.u32(frame.at(0x68)) + c.r(14));
    uncertainty_draw(0x10280038); c.set(19, c.r(19) + c.r(14));
    uncertainty_draw(0x10280218); c.set(20, c.r(20) + c.r(14));
    uncertainty_draw(0x102803f8);

    c.set(21, c.r(21) + c.r(14)); c.set(14, 3); c.set(13, c.r(21));
    if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(19))) < 0) {
        c.set(3, actor.match()); bool select = compare(c, c.u8(c.r(3) + 0x791f), 4) != 0;
        if (select) { c.set(3, actor.match()); select = compare(c, c.i8(c.r(3) + 0x791f), -1) != 0; }
        if (select) {
            bool on_point = compare(c, std::int8_t(actor.side())) == 0 && compare(c, c.u8(player + 0x105), 4) == 0 && compare(c, c.u8(player + 0x106), 11) == 0;
            if (!on_point) on_point = compare(c, actor.side(), 1) == 0 && compare(c, c.u8(player + 0x105), 4) == 0 && compare(c, actor.row()) == 0;
            if (!on_point) { c.set(13, c.r(19)); c.set(14, 4); }
        }
    }
    if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(20))) < 0) {
        c.set(3, actor.match()); bool select = compare(c, c.u8(c.r(3) + 0x791f), 4) != 0;
        if (select) { c.set(3, actor.match()); select = compare(c, c.i8(c.r(3) + 0x791f), -1) != 0; }
        if (select) {
            leave(c, {{3, actor.match()}, {4, player}, {5, 1}}); c.call(0x101d8d70, 0x10280658); select = compare(c, std::int8_t(c.r(3))) == 0;
            if (select && compare(c, c.i16(player + 0x19c), 16) == 0) {
                c.set(3, actor.match() + std::uint32_t(std::int32_t(std::int8_t(actor.side())))); select = compare(c, c.i8(c.r(3) + 0x77bc), 12) < 0;
            }
            if (select) { c.set(13, c.r(20)); c.set(14, 2); }
        }
    }
    bool forced_loose = false;
    if (compare(c, c.i16(player + 0x19c), 16) != 0) { c.set(3, actor.match()); forced_loose = compare(c, c.i8(c.r(3) + 0x791f), -1) == 0; }
    if (forced_loose) c.set(14, 2);
    else if (compare(c, std::int32_t(c.r(13)), c.i32(frame.at(0x68))) < 0 && compare(c, std::int8_t(c.r(16)), 4) != 0) {
        if (compare_logical(c, c.r(22)) != 0) c.set(14, 1);
        else {
            bool randomized = compare(c, c.i8(frame.at(0x100))) == 0;
            if (!randomized) randomized = near_attacking_end();
            if (randomized) { c.set(3, actor.match()); randomized = compare(c, c.u8(c.r(3) + 0x791f), 4) != 0; }
            if (randomized) {
                c.set(13, 2); if (compare(c, c.i16(player + 0x19c), 16) == 0) c.set(13, 10);
                bounded_draw(13, 14, 0xa4, 0xac, 0xb4, 0x10280790, 0x10280868); c.set(14, compare(c, std::int32_t(c.r(14))) == 0 ? 2 : 4);
            } else {
                enter(c, 0x102808ec, {player}); match_context(c, player); leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x102808fc); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x1028090c);
            }
        }
    }
    if (compare(c, std::int8_t(c.r(14)), 3) == 0 && !target_open(0x10280980, 0x102809d8)) {
        c.set(3, actor.match());
        if (compare(c, c.u8(c.r(3) + 0x791f), 3) == 0) {
            c.set(13, c.u8(player + 0x10c)); advance(0x10280a14); const auto value = sample();
            const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x88888889))) + value); c.set(4, shifted_sign(high, 3));
            leave(c, {{3, std::uint32_t(std::int32_t(value) % 15)}, {5, value}}); bool retain = compare(c, std::int8_t(c.r(13)), std::int32_t(c.r(3))) > 0;
            if (retain) { c.set_f(0, c.f64(doubles + 0x278)); fp.fcmp(0, c.f(31), c.f(0)); retain = (c.cr(0) & 8) != 0; }
            c.set(14, retain ? 3 : 2);
        } else { c.set(3, actor.match()); c.set(14, compare(c, c.u8(c.r(3) + 0x791f), 4) == 0 ? 1 : 4); }
    }
    if (compare(c, std::int8_t(c.r(14)), 3) == 0) {
        c.set(3, actor.match()); bool choose_pass = compare(c, c.u8(c.r(3) + 0x791f), 4) == 0;
        if (choose_pass) {
            c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, c.f(31), c.f(0)); const auto bits = c.cr(0);
            c.set_cr(0, (bits & ~2u) | ((bits & 6) ? 2u : 0u)); choose_pass = (c.cr(0) & 2) != 0;
        }
        if (!choose_pass) { c.set(3, actor.match()); choose_pass = compare(c, c.i8(c.r(3) + 0x791f), -1) == 0; }
        if (choose_pass) c.set(14, 1);
    }
    if (compare(c, std::int8_t(c.r(14)), 4) == 0) {
        c.set(3, actor.match()); bool choose_pass = compare(c, c.u8(c.r(3) + 0x791f), 4) == 0;
        if (!choose_pass) { c.set(3, actor.match()); choose_pass = compare(c, c.i8(c.r(3) + 0x791f), -1) == 0; }
        if (choose_pass) c.set(14, 1);
    }

    if (compare(c, std::int8_t(c.r(14)), 3) == 0) {
        bool replace = pursuit_half();
        if (!replace && wide_column()) {
            c.set(13, c.u8(player + 0x10c)); advance(0x10280bbc); const auto value = sample();
            const auto high = std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x88888889))) + value); c.set(4, shifted_sign(high, 6));
            leave(c, {{3, std::uint32_t(std::int32_t(value) % 120)}, {5, value}}); replace = compare(c, std::int8_t(c.r(13)), std::int32_t(c.r(3))) < 0;
        }
        if (replace && compare(c, std::int8_t(c.r(16)), 4) != 0) {
            if (compare_logical(c, c.r(22)) != 0) c.set(14, 1);
            else {
                enter(c, 0x10280c28, {player}); match_context(c, player); bool select = compare(c, std::int8_t(c.r(3)), 3) < 0;
                if (select) { c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(29), c.f(0)); select = (c.cr(0) & 4) != 0; }
                if (select) {
                    if (compare(c, c.i16(player + 0x19c), 2) != 0) {
                        c.set_f(0, c.f32(player + 0x98)); c.set(13, c.u8(player + 0x10c)); c.put64(frame.at(0xa4), fp.fctiwz(0, c.f(0)));
                        c.set(17, extended(std::int16_t(c.u32(frame.at(0xa8)))));
                        bounded_draw(17, 14, 0xa4, 0xac, 0xb4, 0x10280c90, 0x10280d68);
                        advance(0x10280dec); const auto value = sample(); c.set(4, shifted_sign(high_product(value, 0x51eb851f), 4));
                        leave(c, {{3, std::uint32_t(std::int32_t(value) % 50)}, {5, value}});
                        c.set(14, compare(c, std::int32_t(c.r(14) + std::uint32_t(std::int32_t(std::int8_t(c.r(13))))), std::int32_t(c.r(3))) < 0 ? 2 : 4);
                    } else c.set(14, 4);
                } else {
                    enter(c, 0x10280e4c, {player}); match_context(c, player); leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x10280e5c); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x10280e6c);
                }
            }
        }
    }
    if (compare(c, c.i16(player + 0x19c), 59) == 0) c.set(14, 3);
    if (compare(c, std::int8_t(c.r(16)), 4) == 0) {
        bool force_shot = compare_logical(c, (c.r(14) - 1) & 0xff, 1) <= 0;
        if (!force_shot) force_shot = compare(c, std::int8_t(c.r(18))) == 0;
        if (force_shot) c.set(14, 3);
    }
    const auto publish_running = [&](std::uint32_t back) {
        const auto match = actor.match(); leave(c, {{3, c.u32(match + 0x79f4)}, {4, 0x1f44}, {5, actor.match()},
            {6, c.u8(player + 0x2f)}, {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
        c.set(3, extended(c.i16(player + 0x36))); c.put16(player + 0x36, c.r(3) + 25);
    };
    const auto move = [&](std::uint32_t column, std::uint32_t row, unsigned kind, std::uint32_t flag, std::uint32_t context, std::uint32_t back) {
        leave(c, {{3, player}, {4, column}, {5, row}, {6, kind}, {7, flag}, {8, context}}); c.call(0x1022e7e0, back);
    };
    const auto dispatch_order = compare(c, std::int8_t(c.r(14)), 3);
    if (dispatch_order == 0) {
        if (compare(c, std::int8_t(c.r(16)), 4) == 0) return finish(1, 0x102815e8);
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x1c0)) != 0 && compare(c, c.i16(player + 0x19c), 59) != 0) {
            c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x791f)) == 0) publish_running(0x10281644);
        }
        if (compare(c, c.i8(frame.at(0x101))) != 0) {
            leave(c, {{3, player}, {4, frame.at(0x51)}, {5, frame.at(0x5c)}}); c.call(0x10252080, 0x1028166c);
            c.set(5, extended(std::int16_t(c.r(3)))); const auto prior = c.u32(frame.at(0x104));
            c.set(4, prior >> 31); const auto first_sum = c.r(5) + (c.r(5) >> 31), second_sum = prior + c.r(4);
            c.set(3, std::uint32_t(std::int32_t(first_sum) >> 1)); c.s.wr<1>(reg::xer_ca, std::int32_t(second_sum) < 0 && (second_sum & 1));
            c.set(21, c.r(3) + std::uint32_t(std::int32_t(second_sum) >> 1));
        } else {
            leave(c, {{3, player}, {4, frame.at(0x51)}, {5, frame.at(0x5c)}}); c.call(0x10252080, 0x102816a8); c.set(21, extended(std::int16_t(c.r(3))));
        }
        c.set(13, extended(c.i16(frame.at(0x74)))); c.set(3, actor.match()); c.call(0x101d69c0, 0x102816bc);
        leave(c, {{3, player}, {4, c.u8(frame.at(0x51))}, {5, extended(std::int16_t(c.r(21)))}, {6, extended(c.i16(frame.at(0x5c)))},
                  {7, c.r(3)}, {8, std::uint32_t(std::int32_t(std::int8_t(c.r(13))))}, {9, frame.at(0x74)}}); c.call(0x102576e0, 0x102816dc);
        return finish(0, 0x10281758);
    }
    if (dispatch_order < 0) {
        const auto order = compare(c, std::int8_t(c.r(14)), 1);
        if (order == 0) {
            if (compare_logical(c, c.r(22)) == 0) {
                c.put16(player + 0x19c, 9); enter(c, 0x10280ef0, {player}); match_context(c, player); leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x10280f00); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x10280f10);
            }
            c.put16(player + 0x19c, 108); leave(c, {{3, player}, {4, c.u8(frame.at(0x50))}, {5, c.r(22)}, {6, c.u8(frame.at(0x56))}, {7, c.u8(frame.at(0x4f))}, {8, c.u8(frame.at(0x4e))}}); c.link(0x10280f3c); complete_targeted_action(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8));
            return finish(0, 0x10281758);
        }
        if (order > 0) {
            leave(c, {{3, player}, {4, 0}}); c.link(0x102815c0); resolve_direct_action(c, c.r(3), c.r(4)); c.put16(player + 0x19c, 16); return finish(0, 0x10281758);
        }
    } else if (compare(c, std::int8_t(c.r(14)), 5) < 0) {
        if (compare(c, c.i8(frame.at(0x102))) != 0) {
            const bool first = compare(c, std::int8_t(actor.side())) == 0;
            bool right = compare(c, c.i8(frame.at(0x53)), 4) > 0;
            if (!right && compare(c, c.u8(frame.at(0x53)), 4) == 0) {
                advance(first ? 0x10280f94 : 0x10281024); const auto value = sample(); c.set(3, value >> 31);
                right = compare(c, std::int32_t(value) % 2) == 0;
            }
            c.set(3, right ? 8 : 0); c.put8(frame.at(0x4d), c.r(3)); c.put8(frame.at(0x4c), first ? 11 : 0); c.set(18, 1);
        }
        if (!target_open(0x102810d8, 0x10281130)) {
            bool immediate = compare(c, std::int8_t(c.r(18))) == 0;
            if (!immediate) immediate = compare(c, std::int8_t(c.r(16)), 4) == 0;
            if (immediate) {
                const bool first = compare(c, std::int8_t(actor.side())) == 0; const auto row = actor.row();
                const auto destination = first ? compare(c, row, 9) < 0 ? row + 3 : 11 : compare(c, row, 3) > 0 ? row - 3 : 0;
                enter(c, first ? 0x10281194 : 0x102811d4, {player, c.u8(player + 0x105), c.u8(player + 0x106), 4, std::uint32_t(std::int32_t(std::int8_t(destination)))});
                set_heading(c, player, c.r(4), c.r(5), c.r(6), c.r(7)); move(c.u8(player + 0x105), c.u8(player + 0x106), 15, 0, 0, 0x102811f0); c.put16(player + 0x19c, 15); return finish(0, 0x10281758);
            }
        }
        if (compare(c, std::int8_t(c.r(18))) == 0 && compare(c, std::int8_t(c.r(16)), 4) != 0 && compare(c, c.i8(player + 0x10c)) == 0) {
            advance(0x1028123c); const auto value = sample(); c.set(3, shifted_sign(high_product(value, 0x66666667), 2)); c.set(4, value);
            if (compare(c, std::int32_t(value) % 10) != 0) {
                leave(c, {{3, actor.match()}, {4, player}, {5, 4}, {6, std::int8_t(actor.side()) == 0 ? 11u : 0u}, {7, std::uint32_t(std::int32_t(std::int8_t(actor.side())))}}); c.call(0x101eeac0, 0x102812a8);
                if (compare(c, std::int8_t(c.r(3))) == 0) { leave(c, {{3, player}, {4, 0}}); c.link(0x102812bc); resolve_direct_action(c, c.r(3), c.r(4)); c.put16(player + 0x19c, 16); return finish(0, 0x10281758); }
            }
            c.put16(player + 0x19c, 9); enter(c, 0x102812d8, {player}); match_context(c, player); leave(c, {{3, player}, {4, c.r(3)}, {5, 0}}); c.link(0x102812e8); choose_pass_action(c, c.r(3), c.r(4), c.r(5)); return finish(0, 0x102812f8);
        }
        auto column = c.i8(frame.at(0x4d));
        if (compare(c, column) < 0) column = 0; else if (compare(c, column, 8) > 0) column = 8;
        c.set(3, std::uint32_t(column)); c.put8(frame.at(0x55), c.r(3)); auto row = c.i8(frame.at(0x4c));
        if (compare(c, row) < 0) row = 0; else if (compare(c, row, 11) > 0) row = 11;
        c.put8(frame.at(0x54), row);
        bool intermediate = compare(c, std::int8_t(actor.side())) == 0;
        if (intermediate) { c.set(3, c.u8(frame.at(0x55))); intermediate = compare(c, std::int8_t(c.r(3)), actor.column()) >= 0; }
        if (!intermediate && compare(c, actor.side(), 1) == 0) { c.set(3, c.u8(frame.at(0x55))); intermediate = compare(c, std::int8_t(c.r(3)), actor.column()) <= 0; }
        if (intermediate) {
            const auto target_column = c.i8(frame.at(0x55)), target_row = c.i8(frame.at(0x54));
            c.set(9, std::uint32_t((target_column + 4) / 2)); c.set(8, std::uint32_t(target_row / 2));
            const auto heading_row = c.r(8) + (std::int8_t(actor.side()) == 0 ? 6u : 0u); c.s.wr<1>(reg::xer_ca, target_row < 0 && (unsigned(target_row) & 1));
            enter(c, 0x1028141c, {player, c.u8(player + 0x105), c.u8(player + 0x106), std::uint32_t(std::int32_t(std::int8_t(c.r(9)))), std::uint32_t(std::int32_t(std::int8_t(heading_row)))});
            set_heading(c, player, c.r(4), c.r(5), c.r(6), c.r(7));
        } else {
            enter(c, 0x10281438, {player, c.u8(player + 0x105), c.u8(player + 0x106), c.u8(frame.at(0x55)), c.u8(frame.at(0x54))}); set_heading(c, player, c.r(4), c.r(5), c.r(6), c.r(7));
        }
        c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x1c0)) != 0) publish_running(0x10281470);
        bool repeat = compare(c, c.i16(player + 0x19c), 18) == 0;
        if (!repeat) repeat = compare(c, c.i16(player + 0x19c), 17) == 0;
        if (repeat) {
            move(c.u8(frame.at(0x55)), c.u8(frame.at(0x54)), 18, c.u8(frame.at(0xff)), c.r(16), 0x102815a8); c.put16(player + 0x19c, 18); return finish(0, 0x10281758);
        }
        if (compare(c, c.i8(frame.at(0xff))) != 0) {
            c.set(3, actor.match() + 0x10000);
            if (compare_logical(c, player, c.u32(c.r(3) - 0x1fd8)) == 0) {
                c.set(3, actor.match() + 0x10000);
                if (compare_logical(c, c.u32(c.r(3) - 0x1fdc)) != 0 && compare(c, c.i16(player + 0x2a), -100) > 0) {
                    c.set(3, c.u32(actor.match() + 0xe024));
                    if (compare(c, c.i16(c.r(3) + 0x19c), 52) != 0) {
                        c.set(3, c.u32(actor.match() + 0xe024));
                        if (compare(c, c.i16(c.r(3) + 0x19c), 55) != 0) {
                            c.set(5, c.u32(actor.match() + 0xe024)); c.set(4, c.u8(c.r(5) + 3)); c.put8(c.r(5) + 3, c.r(4) + 1);
                            const auto rating = c.i16(player + 0x2a); c.set(5, c.u32(actor.match() + 0xe024)); c.set(4, extended(c.i16(c.r(5) + 0x36)));
                            c.set(3, std::uint32_t(std::int32_t(rating) / 3)); c.put16(c.r(5) + 0x36, c.r(4) + std::uint32_t(std::int32_t(std::int16_t(c.r(3) + 275))));
                            c.set(4, actor.match()); c.set(3, c.u32(c.r(4) + 0x3108)); c.put32(c.r(4) + 0x3108, c.r(3) + 275);
                        }
                    }
                }
            }
        }
        move(c.u8(frame.at(0x55)), c.u8(frame.at(0x54)), 17, c.u8(frame.at(0xff)), c.r(16), 0x10281580); c.put16(player + 0x19c, 17); return finish(0, 0x10281758);
    }
    const auto strings = c.toc(-0x51e4); leave(c, {{3, strings + 0x8f}, {4, 0}, {5, 0}, {6, frame.at(0x84)}, {7, frame.at(0x6c)}}); c.call(0x10739890, 0x102816fc); c.call(0x10768810, 0x10281700);
    leave(c, {{3, c.toc(-0x7ba8)}, {4, c.toc(-0x51e4) + 0x9c}, {5, c.r(3)}, {6, frame.at(0x84)}, {7, frame.at(0x6c)}, {8, 0x50af}}); c.call(0x1077b720, 0x10281720);
    leave(c, {{3, c.toc(-0x51e4) + 0xa9}, {4, c.toc(-0x7ba8)}, {5, 0}}); c.call(0x10769b60, 0x10281734);
    c.set(4, c.toc(-0x7bac)); c.set(3, UINT32_MAX); c.put8(c.r(4), 0); c.call(0x1076f7d0, 0x10281748); return finish(0, 0x10281758);
}
}

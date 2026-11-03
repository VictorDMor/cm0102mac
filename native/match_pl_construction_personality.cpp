#include "match_pl_construction.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
int high_product(std::uint32_t word, std::int32_t multiplier) {
    return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(word)) * multiplier) >> 32);
}
int shift(Cpu& c, int value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
    return value >> bits;
}
}

std::uint32_t PlayerConstruction::narrow_preference(std::int32_t bound, std::uint32_t refill_back) {
    if (compare(c, bound) == 0) return 0;
    advance(refill_back);
    c.set(3, 65535); compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
    c.set(3, c.u32(cursor)); const bool positive = compare(c, bound) > 0;
    const auto bias = c.u32(seed); const auto draw = std::uint32_t(c.u16(c.r(3))) + bias;
    c.set(3, draw);
    const auto quotient = std::uint32_t(divide_signed<4>(draw, std::uint32_t(bound)));
    const auto residual = draw - quotient * std::uint32_t(bound);
    return positive ? residual : 0u - residual;
}

void PlayerConstruction::initialize_random_preferences() {
    advance(0x10224528);
    c.set(4, c.u32(cursor)); c.set(3, 0x66660000);
    const auto bias = c.u32(seed); c.set(3, 0x66666667);
    const auto draw = std::uint32_t(c.u16(c.r(4))) + bias; c.set(4, draw);
    const auto quotient = shift(c, high_product(draw, 0x66666667), 3);
    c.set(3, quotient < 0);
    c.put8(actor.address + 0x11b, draw - (std::uint32_t(quotient) + c.r(3)) * 20);
    c.set(3, c.u32(actor.address + 0x70));
    const bool exceptional = compare(c, c.u8(c.r(3) + 0xf), 20) == 0;
    c.set(3, c.u32(actor.address + 0x70));
    const auto trait = std::int32_t(c.i8(c.r(3) + 0x21));
    const auto subtrahend = std::uint32_t(trait) * (exceptional ? 1u : 2u);
    const auto base = exceptional ? 35u : 60u;
    c.s.wr<1>(reg::xer_ca, subtrahend <= base);
    Kept<14> bound(c, base - subtrahend);
    // The original wide arms cannot execute: the signed-byte formulas give
    // [-92,163] or [-194,316]. Zero bounds consume no second draw.
    Kept<13> selected(c, narrow_preference(std::int32_t(std::uint32_t(bound)), exceptional ? 0x102245b4 : 0x1022473c));
    c.put8(actor.address + 0x11c, selected);
}

void PlayerConstruction::affiliation_factor() {
    const auto side = actor.side(); c.set(3, actor.match());
    const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.call(0x100c7d00, 0x102248dc);
    if (compare(c, std::int8_t(c.r(3))) != 0) { c.set_f(30, c.f64(doubles + 0x18)); return; }
    if (compare_logical(c, c.r(20)) == 0) { c.set_f(30, c.f64(doubles + 0x10)); return; }
    const auto band = c.u8(c.r(20) + 0x60) & 15;
    unsigned coefficient;
    const auto order = compare(c, band, 2);
    if (order == 0) coefficient = 0x28;
    else if (order < 0) coefficient = compare(c, band, 1) >= 0 ? 0x20 : 8;
    else coefficient = compare(c, band, 4) >= 0 ? 8 : 0x30;
    c.set_f(30, c.f64(doubles + coefficient));
    c.set(3, c.u8(c.r(20) + 0x4c)); c.put32(at(0x4c), 0x43300000);
    const auto influence = std::uint32_t(std::int32_t(std::int8_t(c.r(3))));
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, c.f64(doubles + 0x38));
    c.put32(at(0x50), influence ^ 0x80000000); c.set_f(3, c.f64(doubles + 0x10));
    c.set_f(1, c.f64(at(0x4c))); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.fadd(c.f(3), c.f(0)));
    c.set_f(30, fp.fmul(c.f(30), c.f(0)));
}

void PlayerConstruction::fixture_reputation() {
    c.set(3, c.u32(actor.match() + 0x3118));
    if (compare(c, c.i8(c.r(3) + 0x40)) != 0) {
        const auto side = actor.side(); c.set(3, actor.match());
        const bool opponent_away = compare(c, std::int8_t(side) == 0) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(4, c.u32(c.r(3) + (opponent_away ? 0x20 : 0x1c)));
        c.set(3, 0x10620000); const auto reputation = c.i16(c.r(4) + 0x1ec);
        c.set(3, 0x10624dd3); c.set(4, actor.match());
        const auto product = high_product(extended(reputation), 0x10624dd3);
        c.set(3, c.u32(c.r(4) + 0x3118)); c.set(4, c.u32(c.r(3) + 0x14));
        const auto shifted = shift(c, product, 5); c.set(4, extended(c.i16(c.r(4) + 0x18)));
        c.set(3, shifted < 0);
        const auto scaled = std::uint32_t(shifted) + c.r(3);
        const auto sum = c.r(4) + scaled; c.set(3, sum);
        const auto adjusted = sum + unsigned(std::int32_t(sum) < 0);
        const auto average = shift(c, std::int32_t(adjusted), 1);
        keep(c, {{13, std::uint32_t(std::int32_t(std::int8_t(average)))}});
    } else {
        c.set(3, c.u32(actor.match() + 0x3118)); c.set(3, c.u32(c.r(3) + 0x14));
        keep(c, {{13, std::uint32_t(std::int32_t(std::int8_t(c.i16(c.r(3) + 0x18))))}});
    }
    for (const auto field : {0x74u, 0x78u, 0x7cu}) {
        const auto side = actor.side(); c.set(3, actor.match());
        const bool opponent_away = compare(c, std::int8_t(side) == 0) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(4, c.u32(c.r(3) + (opponent_away ? 0x20 : 0x1c)));
        const auto reread_side = actor.side(); c.set(3, actor.match());
        const bool away = compare(c, std::int8_t(reread_side)) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        if (compare_logical(c, c.u32(c.r(3) + field), c.r(4)) == 0) {
            keep(c, {{13, c.r(13) + 10}}); break;
        }
    }
}

void PlayerConstruction::repair_competition_descriptor() {
    c.set(3, c.u32(actor.match() + 0x3118));
    if (compare(c, c.i32(c.r(3)), -1) <= 0) return;
    c.set(4, actor.match()); c.set(3, c.u32(c.r(24))); c.set(4, c.u32(c.r(4) + 0x3118));
    const auto index = c.u32(c.r(4));
    if (compare_logical(c, c.u32(c.r(3) + index * 4)) != 0) return;
    c.set(3, c.toc(-0x7a50)); c.set(4, actor.match()); const auto replacement = c.u32(c.r(3));
    c.set(3, c.u32(c.r(4) + 0x3118)); c.set(4, c.toc(-0x7bbc)); c.put32(c.r(3), replacement);
    c.set(3, c.toc(-0x7a50)); c.set(5, actor.match()); const auto fresh_index = c.u32(c.r(3));
    c.set(3, c.u32(c.r(4))); c.set(4, c.u32(c.r(5) + 0x3118));
    c.put32(c.r(4) + 0x14, c.r(3) + fresh_index * 0x6c);
}

bool PlayerConstruction::prepare_personality_factors() {
    c.set(3, actor.match()); c.call(0x101fce40, 0x10224884);
    bool skip = compare(c, std::int8_t(c.r(3))) != 0;
    if (!skip && compare_logical(c, staff) != 0) {
        c.set(3, c.u32(staff)); skip = compare(c, std::int32_t(c.r(3)), c.i32(staff_count)) >= 0;
    }
    if (skip) { c.set_f(31, c.f64(doubles + 0x10)); c.put8(actor.address + 0x103, 0); return false; }
    affiliation_factor(); fixture_reputation(); repair_competition_descriptor();
    c.set(3, c.u32(actor.match() + 0x3118));
    if (compare(c, c.i32(c.r(3)), -1) > 0) {
        const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(4, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(5, actor.match()); c.set(3, c.u32(c.r(24))); c.set(5, c.u32(c.r(5) + 0x3118));
        c.set(3, c.u32(c.r(3) + c.u32(c.r(5)) * 4)); c.call(0x10452ed0, 0x10224bfc);
        if (compare(c, std::int8_t(c.r(3))) == 0) keep(c, {{13, c.r(13) - 10}});
    }
    const auto adjusted = std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) - 10; c.set(3, adjusted);
    c.put32(at(0x4c), 0x43300000); c.set_f(2, c.f64(doubles + 0x80));
    c.put32(at(0x50), adjusted ^ 0x80000000); c.set_f(0, c.f64(doubles + 0x40));
    c.set_f(1, c.f64(at(0x4c))); c.set_f(3, c.f64(doubles + 0x10));
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(2, c.f64(doubles + 0x48));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(31, fp.fadd(c.f(3), c.f(0))); c.set_f(1, c.f(31));
    c.call(0x10780fc0, 0x10224c48);
    c.set(2, c.u32(at(0x14))); const auto side = actor.side(); c.set_f(31, c.f(1)); compare(c, std::int8_t(side));
    return true;
}
}

namespace cm::aot::match_pl {
std::uint32_t PlayerConstruction::variable_draw(std::uint32_t bound, std::uint32_t refill_back,
                                               std::uint32_t rounding_back) {
    if (compare(c, std::int32_t(bound)) == 0) return 0;
    advance(refill_back); c.set(3, 65535);
    bool wide = compare(c, std::int32_t(bound), 65535) > 0;
    if (!wide) { c.set(3, std::uint32_t(-65535)); wide = compare(c, std::int32_t(bound), -65535) < 0; }
    if (!wide) {
        c.set(3, c.u32(cursor)); const bool positive = compare(c, std::int32_t(bound)) > 0;
        const auto bias = c.u32(seed), draw = std::uint32_t(c.u16(c.r(3))) + bias; c.set(3, draw);
        const auto quotient = std::uint32_t(divide_signed<4>(draw, bound));
        const auto residual = draw - quotient * bound;
        return positive ? residual : 0u - residual;
    }
    c.set(3, bound ^ 0x80000000); c.put32(at(0x40), c.r(3)); c.set(6, 0x80010000);
    c.set(7, c.u32(cursor)); c.set(4, 0x43300000); c.put32(at(0x3c), 0x43300000);
    c.set(5, 0x10000); c.set_f(2, c.f64(doubles + 0x80)); c.set(3, 0x80010000);
    c.set_f(1, c.f64(at(0x3c))); c.set(6, 0x80008001); c.set_f(0, c.f64(doubles)); c.set(5, 65535);
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    const auto word = c.u16(c.r(7)); c.set(8, word); const auto bias = c.u32(seed); c.set(7, bias);
    const auto scaled_bound = bound * 10; c.set(3, 0x80008001); c.put32(at(0x44), c.r(4));
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    const auto draw = std::uint32_t(word) + bias; c.set(7, draw);
    c.set(4, high_product(draw, std::int32_t(0x80008001))); c.set_f(1, c.f64(doubles + 0x80));
    const auto product = c.r(4) + draw; c.set(4, product);
    const auto quotient = shift(c, std::int32_t(product), 15); c.set(4, quotient); c.set(6, quotient < 0);
    c.set(4, std::uint32_t(quotient) + c.r(6)); c.set(4, c.r(4) * c.r(5));
    Kept<13> residual(c, draw - c.r(4)); c.set(4, std::uint32_t(residual) ^ 0x80000000); c.put32(at(0x48), c.r(4));
    const auto reduction = high_product(scaled_bound, std::int32_t(0x80008001)); c.set(3, reduction);
    c.set_f(0, c.f64(at(0x44))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    const auto reduction_shift = shift(c, std::int32_t(std::uint32_t(reduction) + scaled_bound), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set(3, reduction_shift < 0);
    const auto correction_bound = std::uint32_t(reduction_shift) + c.r(3); c.set(3, correction_bound);
    c.put64(at(0x4c), fp.fctiwz(0, c.f(0))); Kept<13> coarse(c, c.u32(at(0x50)));
    enter(c, rounding_back); sample_range(c, correction_bound);
    c.set(4, 0x66660000); const auto rounded = c.r(3) + 5; c.set(3, rounded); c.set(4, 0x66666667);
    const auto rounding_shift = shift(c, high_product(rounded, 0x66666667), 2); c.set(3, rounding_shift);
    c.set(4, rounding_shift < 0); const auto correction = std::uint32_t(rounding_shift) + c.r(4); c.set(3, correction);
    const auto candidate = std::uint32_t(coarse) + correction;
    return compare(c, std::int32_t(candidate), std::int32_t(bound - 1)) > 0 ? bound - 1 : candidate;
}

std::uint32_t PlayerConstruction::constant_draw(std::uint32_t refill_back, std::int32_t multiplier,
                                                unsigned shift_bits, std::int32_t divisor, bool negate) {
    advance(refill_back); c.set(4, c.u32(cursor)); c.set(3, std::uint32_t(multiplier) & 0xffff0000);
    const auto bias = c.u32(seed); c.set(3, multiplier);
    const auto draw = std::uint32_t(c.u16(c.r(4))) + bias; c.set(4, draw);
    const auto quotient = shift(c, high_product(draw, multiplier), shift_bits); c.set(3, quotient < 0);
    const auto residual = draw - (std::uint32_t(quotient) + c.r(3)) * std::uint32_t(divisor);
    return negate ? 0u - residual : residual;
}

void PlayerConstruction::probability_threshold(std::uint32_t refill_back, std::uint32_t rounding_back,
                                               unsigned coefficient, unsigned rounding_bound, int limit) {
    advance(refill_back); c.set(3, c.u32(cursor)); c.set(5, 0x80010000); c.set(4, 0x10000);
    const auto word = c.u16(c.r(3)); c.set(7, word); c.set(5, 0x80008001);
    const auto bias = c.u32(seed); c.set(6, bias); c.set(4, 65535); c.put32(at(0x3c), 0x43300000);
    c.set(3, rounding_bound); const auto draw = std::uint32_t(word) + bias; c.set(6, draw);
    c.set_f(1, c.f64(doubles + 0x80)); const auto product = std::uint32_t(high_product(draw, std::int32_t(0x80008001))) + draw;
    c.set_f(2, c.f64(doubles + coefficient)); const auto quotient = shift(c, std::int32_t(product), 15);
    c.set(5, quotient < 0); const auto residual = draw - (std::uint32_t(quotient) + c.r(5)) * c.r(4);
    keep(c, {{13, residual}}); c.put32(at(0x40), residual ^ 0x80000000);
    c.set_f(0, c.f64(at(0x3c))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(at(0x44), fp.fctiwz(0, c.f(0))); Kept<13> coarse(c, c.u32(at(0x48)));
    enter(c, rounding_back); sample_range(c, rounding_bound);
    c.set(4, 0x66660000); const auto rounded = c.r(3) + 5; c.set(4, 0x66666667); c.set(3, std::uint32_t(limit) & 0xffff0000);
    const auto correction_product = high_product(rounded, 0x66666667); c.set(3, limit);
    const auto correction_shift = shift(c, correction_product, 2); c.set(4, correction_shift < 0);
    const auto threshold = std::uint32_t(coarse) + std::uint32_t(correction_shift) + c.r(4);
    keep(c, {{13, compare(c, std::int32_t(threshold), limit) > 0 ? std::uint32_t(limit) : threshold}});
}

void PlayerConstruction::choose_personality_band() {
    keep(c, {{14, c.r(13)}});
    probability_threshold(0x10224ff4, 0x10225070, 0x50, 0x131, 1999999);
    if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(13))) > 0) {
        c.put8(actor.address + 0x103, constant_draw(0x102250d0, 0x66666667, 2, 10)); return;
    }
    probability_threshold(0x10225130, 0x102251ac, 0x58, 0x98, 999999);
    if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(13))) > 0) {
        c.put8(actor.address + 0x103, constant_draw(0x1022520c, 0x66666667, 1, 5)); return;
    }
    enter(c, 0x10225254, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) { c.put8(actor.address + 0x103, 0); return; }
    probability_threshold(0x1022528c, 0x10225308, 0x60, 15, 99999);
    if (compare(c, std::int32_t(c.r(14)), std::int32_t(c.r(13))) < 0) {
        c.put8(actor.address + 0x103, constant_draw(0x10225368, std::int32_t(0x99999999), 1, -5, true)); return;
    }
    const auto threshold = constant_draw(0x102253cc, 0x68db8bad, 12, 10000);
    // The original deliberately preserves the previous personality byte if
    // this final gate fails; no default zero is written on that path.
    if (compare(c, std::int32_t(c.r(14)), std::int32_t(threshold)) > 0)
        c.put8(actor.address + 0x103, constant_draw(0x1022542c, std::int32_t(0x99999999), 2, -10, true));
}

void PlayerConstruction::apply_personality_attributes() {
    c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 4) {
        c.set_f(0, c.f64(doubles + 0x70)); c.set_f(1, c.f64(doubles + 0x68));
        c.set_f(0, fp.fdiv(c.f(31), c.f(0))); c.set_f(31, fp.fadd(c.f(1), c.f(0)));
    }
    c.set(3, c.u32(actor.address + 0x70)); c.put32(at(0x3c), 0x43300000);
    const auto trait = c.i8(c.r(3) + 0x2c); c.set_f(1, c.f64(doubles + 0x80)); c.set(3, std::int32_t(trait));
    c.put32(at(0x40), (std::uint32_t(std::int32_t(trait)) - 10) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x3c))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(0), c.f(31)));
    c.put64(at(0x44), fp.fctiwz(0, c.f(0))); Kept<14> bound(c, c.u32(at(0x48)));
    Kept<13> selected(c, variable_draw(bound, 0x102254f4, 0x102255cc));
    c.set(5, c.u8(actor.address + 0x103)); c.set(3, std::int32_t(std::int8_t(std::uint32_t(selected)))); c.set(4, 0x43300000);
    c.set(3, c.r(5) + c.r(3)); c.put32(at(0x44), 0x43300000); c.set_f(2, c.f64(doubles + 0x80));
    c.put8(actor.address + 0x103, c.r(3)); c.set_f(0, c.f64(doubles + 0x78));
    c.set(3, c.u32(actor.address + 0x70)); c.set(5, c.u8(actor.address + 0x103));
    const auto rating = extended(c.i16(c.r(3) + 4)); c.set(3, std::int32_t(std::int8_t(c.r(5))));
    c.put32(at(0x3c), c.r(4)); c.set_f(4, c.f64(doubles + 0x80)); c.put32(at(0x48), rating ^ 0x80000000);
    c.set_f(3, c.f64(doubles + 0x10)); c.set_f(1, c.f64(at(0x44))); c.put32(at(0x40), c.r(3) ^ 0x80000000);
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(2, c.f64(at(0x3c))); c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(1, fp.fsub(c.f(2), c.f(4))); c.set_f(0, fp.fsub(c.f(3), c.f(0))); c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.put64(at(0x4c), fp.fctiwz(0, c.f(0))); c.put8(actor.address + 0x103, c.u32(at(0x50)));
    c.set(3, c.u32(actor.match() + 0x3118));
    if (compare(c, c.i8(c.r(3) + 0x40)) != 0) {
        const auto side = actor.side(); c.set(3, actor.match()); const bool opponent_away = compare(c, std::int8_t(side) == 0) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (opponent_away ? 0x20 : 0x1c)));
        c.set(4, c.u32(actor.address + 0x6c)); c.set(3, c.u32(c.r(3) + 0x90)); c.call(0x100d9580, 0x102256fc);
        if (compare(c, std::int8_t(c.r(3))) != 0) { c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) + 10); }
    }
}
}

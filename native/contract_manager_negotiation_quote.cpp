#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
void combine_factors(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    n.relation_basis = fp.fmul(n.tenure_factor, n.relationship_factor);
    c.set_f(24, n.relation_basis);
    auto lower_limit = c.f64(n.constants + 0x2f8);
    c.set_f(0, lower_limit);
    const auto relationship_limit = fp.fmul(n.relationship_factor, n.relation_basis);
    c.set_f(1, relationship_limit);
    fp.fcmp(0, lower_limit, relationship_limit);
    if (!(lower_limit > relationship_limit)) { lower_limit = relationship_limit; c.set_f(0, lower_limit); }
    auto cap = c.f64(n.constants + 0xc8);
    c.set_f(1, cap);
    fp.fcmp(0, cap, lower_limit);
    if (!(cap < lower_limit)) { cap = lower_limit; c.set_f(1, cap); }
    auto combined = fp.fmul(n.tenure_factor, n.quote_factor);
    c.set_f(0, combined);
    combined = fp.fmul(n.finance_factor, combined);
    c.set_f(0, combined);
    combined = fp.fmul(n.relationship_factor, combined);
    c.set_f(0, combined);
    combined = fp.fmul(n.potential_factor, combined);
    c.set_f(0, combined);
    combined = fp.fmul(n.promise_factor, combined);
    c.set_f(0, combined);
    fp.fcmp(0, cap, combined);
    if (!(cap < combined)) { cap = combined; c.set_f(1, cap); }
    const auto finance_coefficient = c.f64(n.constants + 0x300);
    c.set_f(0, finance_coefficient);
    n.quote_factor = cap;
    c.set_f(27, cap);
    const auto finance_limit = fp.fmul(finance_coefficient, n.finance_factor);
    c.set_f(0, finance_limit);
    fp.fcmp(0, cap, finance_limit);
    if (cap > finance_limit) { n.quote_factor = finance_limit; c.set_f(27, finance_limit); }
}

struct QualityInputs { std::int16_t score, club_reputation; };
QualityInputs quality_floor(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto attributes = c.u32(n.person + 0x14);
    c.set(5, attributes);
    c.set(4, 0x43300000);
    c.set(3, 0x20000);
    c.put32(n.frame.at(0x9c), 0x43300000);
    const auto first_score = c.i16(attributes + 8);
    c.set(0, extended(first_score));
    n.special_club_or_floor = 150000;
    const auto second_score = c.i16(attributes + 10);
    c.set(3, extended(second_score));
    c.set(0, extended(first_score) ^ 0x80000000u);
    const auto potential_score = c.i16(attributes + 12);
    c.set(5, extended(potential_score));
    c.put32(n.frame.at(0xa0), c.r(0));
    c.set(0, extended(second_score) ^ 0x80000000u);
    c.set(3, extended(potential_score) ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(4, bias);
    const auto first_encoded = c.f64(n.frame.at(0x9c));
    c.set_f(0, first_encoded);
    c.put32(n.frame.at(0x98), c.r(0));
    const auto first = fp.fsub(first_encoded, bias);
    c.set_f(0, first);
    const auto score_weight = c.f64(n.constants + 0x268);
    c.set_f(2, score_weight);
    c.put32(n.frame.at(0x94), 0x43300000);
    const auto potential_weight = c.f64(n.constants + 0x308);
    c.set_f(5, potential_weight);
    const auto second_encoded = c.f64(n.frame.at(0x94));
    c.set_f(1, second_encoded);
    const auto weighted_first = fp.fmul(score_weight, first);
    c.set_f(0, weighted_first);
    c.put32(n.frame.at(0x90), c.r(3));
    const auto second = fp.fsub(second_encoded, bias);
    c.set_f(1, second);
    const auto club_reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(club_reputation));
    c.put32(n.frame.at(0x8c), 0x43300000);
    const auto potential_encoded = c.f64(n.frame.at(0x8c));
    c.set_f(3, potential_encoded);
    const auto weighted_scores = fp.multiply_add(0x100867c8, score_weight, second, weighted_first);
    c.set_f(0, weighted_scores);
    const auto potential = fp.fsub(potential_encoded, bias);
    c.set_f(1, potential);
    const auto weighted_quality = fp.multiply_add(0x100867d0, potential_weight, potential, weighted_scores);
    c.set_f(0, weighted_quality);
    c.put64(n.frame.at(0xa4), fp.fctiwz(0, weighted_quality));
    const auto raw_quality = c.u32(n.frame.at(0xa8));
    c.set(5, raw_quality);
    const auto quality = std::int16_t(raw_quality);
    c.set(3, extended(quality));
    const auto combined_reputation = extended(club_reputation) + extended(quality);
    c.set(3, combined_reputation);
    const auto scaled_reputation = combined_reputation * 225u;
    c.set(3, scaled_reputation);
    const auto candidate = std::int32_t(scaled_reputation - 900000u);
    c.set(4, std::uint32_t(candidate));
    if (compare(c, std::int32_t(std::uint32_t(n.special_club_or_floor)), candidate) <= 0) {
        c.set(3, 0x1f0000);
        n.special_club_or_floor = 2000000;
        if (compare(c, 2000000, candidate) >= 0) n.special_club_or_floor = std::uint32_t(candidate);
    }
    return {quality, club_reputation};
}

void quality_premium(Context& n, QualityInputs inputs) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(3, extended(inputs.score));
    c.set(4, 0x43300000);
    c.set(5, extended(inputs.score) ^ 0x80000000u);
    c.set(3, extended(inputs.club_reputation) ^ 0x80000000u);
    c.put32(n.frame.at(0x88), c.r(5));
    const auto mood = std::int16_t(std::uint32_t(n.mood));
    c.set(0, extended(mood));
    c.set(0, extended(mood) ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(8, bias);
    c.put32(n.frame.at(0x84), 0x43300000);
    const auto score_divisor = c.f64(n.constants + 0x310);
    c.set_f(0, score_divisor);
    const auto score_encoded = c.f64(n.frame.at(0x84));
    c.set_f(1, score_encoded);
    c.put32(n.frame.at(0x78), c.r(3));
    const auto quality = fp.fsub(score_encoded, bias);
    c.set_f(2, quality);
    const auto club_scale = c.f64(n.constants + 0x20);
    c.set_f(5, club_scale);
    c.put32(n.frame.at(0x74), 0x43300000);
    const auto club_intercept = c.f64(n.constants + 0x318);
    c.set_f(1, club_intercept);
    const auto quality_share = fp.fdiv(quality, score_divisor);
    c.set_f(7, quality_share);
    const auto club_encoded = c.f64(n.frame.at(0x74));
    c.set_f(0, club_encoded);
    c.put32(n.frame.at(0x80), c.r(5));
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(4, one);
    c.put32(n.frame.at(0x7c), 0x43300000);
    const auto half = c.f64(n.constants + 0xb8);
    c.set_f(2, half);
    const auto club_reputation = fp.fsub(club_encoded, bias);
    c.set_f(3, club_reputation);
    const auto repeated_score_encoded = c.f64(n.frame.at(0x7c));
    c.set_f(0, repeated_score_encoded);
    c.put32(n.frame.at(0x70), c.r(0));
    const auto repeated_quality = fp.fsub(repeated_score_encoded, bias);
    c.set_f(6, repeated_quality);
    const auto mood_intercept = c.f64(n.constants + 0x240);
    c.set_f(0, mood_intercept);
    c.put32(n.frame.at(0x6c), 0x43300000);
    const auto club_baseline = fp.multiply_add(0x10086884, club_scale, club_reputation, club_intercept);
    c.set_f(5, club_baseline);
    const auto divisor = c.f64(n.constants + 0x228);
    c.set_f(1, divisor);
    const auto mood_encoded = c.f64(n.frame.at(0x6c));
    c.set_f(3, mood_encoded);
    const auto signed_mood = fp.fsub(mood_encoded, bias);
    c.set_f(3, signed_mood);
    const auto quality_gap = fp.fsub(repeated_quality, club_baseline);
    c.set_f(5, quality_gap);
    const auto mood_scale = fp.multiply_add(0x10086898, signed_mood, half, mood_intercept);
    c.set_f(2, mood_scale);
    const auto relative_quality = fp.multiply_add(0x1008689c, quality_share, quality_gap, one);
    c.set_f(0, relative_quality);
    const auto scaled_quality = fp.multiply_add(0x100868a0, mood_scale, relative_quality, divisor);
    c.set_f(0, scaled_quality);
    const auto numerator = fp.fsub(scaled_quality, mood_scale);
    c.set_f(0, numerator);
    auto premium = fp.fdiv(numerator, divisor);
    c.set_f(1, premium);
    fp.fcmp(0, premium, one);
    if (premium > one) {
        c.set_f(2, c.f64(n.constants + 0x188));
        n.call(0x10780fc0, 0x100868bc);
        c.set(2, c.u32(n.frame.at(0x14)));
        premium = c.f(1); // ABI result of pow, with retained factors restored.
    }
    n.finance_factor = c.f64(n.constants + 0x18);
    c.set_f(23, n.finance_factor);
    fp.fcmp(0, n.finance_factor, premium);
    if (!(n.finance_factor > premium)) {
        n.finance_factor = c.f64(n.constants + 0x100);
        c.set_f(23, n.finance_factor);
        fp.fcmp(0, n.finance_factor, premium);
        if (!(n.finance_factor < premium)) { n.finance_factor = premium; c.set_f(23, premium); }
    }
}

void price_adjustment(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(4, 0x43300000);
    const auto price_encoded_word = std::uint32_t(n.price) ^ 0x80000000u;
    c.set(0, price_encoded_word);
    c.put32(n.frame.at(0x58), price_encoded_word);
    c.set(3, std::uint32_t(n.special_club_or_floor) ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x54), 0x43300000);
    const auto price_encoded = c.f64(n.frame.at(0x54));
    c.set_f(0, price_encoded);
    c.put32(n.frame.at(0x50), price_encoded_word);
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(1, price);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto repeated_price_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, repeated_price_encoded);
    c.put32(n.frame.at(0x68), c.r(3));
    const auto repeated_price = fp.fsub(repeated_price_encoded, bias);
    c.set_f(0, repeated_price);
    c.put32(n.frame.at(0x64), 0x43300000);
    const auto floor_encoded = c.f64(n.frame.at(0x64));
    c.set_f(2, floor_encoded);
    const auto difference = fp.multiply_add(0x10086920, price, n.quote_factor, repeated_price);
    c.set_f(0, difference);
    const auto floor = fp.fsub(floor_encoded, bias);
    c.set_f(1, floor);
    const auto difference_bits = fp.fctiwz(0, difference);
    const auto weighted_floor = fp.fmul(floor, n.quote_factor);
    c.set_f(1, weighted_floor);
    c.put64(n.frame.at(0x44), difference_bits);
    const auto floor_bits = fp.fctiwz(0, weighted_floor);
    const auto integer_difference = c.i32(n.frame.at(0x48));
    c.set(3, std::uint32_t(integer_difference));
    const auto negative = compare(c, integer_difference) < 0;
    c.put64(n.frame.at(0x5c), floor_bits);
    const auto integer_floor = c.u32(n.frame.at(0x60));
    c.set(5, integer_floor);
    if (negative) {
        c.put32(n.frame.at(0x50), price_encoded_word);
        c.set(0, std::uint32_t(integer_difference) ^ 0x80000000u);
        const auto reduction_divisor = c.f64(n.constants + 0x320);
        c.set_f(0, reduction_divisor);
        c.put32(n.frame.at(0x4c), 0x43300000);
        const auto reduction_price_encoded = c.f64(n.frame.at(0x4c));
        c.set_f(1, reduction_price_encoded);
        c.put32(n.frame.at(0x48), c.r(0));
        const auto reduction_price = fp.fsub(reduction_price_encoded, bias);
        c.set_f(1, reduction_price);
        c.put32(n.frame.at(0x44), 0x43300000);
        auto reduction = fp.fdiv(reduction_price, reduction_divisor);
        c.set_f(1, reduction);
        const auto difference_encoded = c.f64(n.frame.at(0x44));
        c.set_f(0, difference_encoded);
        const auto signed_difference = fp.fsub(difference_encoded, bias);
        c.set_f(0, signed_difference);
        fp.fcmp(0, signed_difference, reduction);
        if (signed_difference > reduction) {
            c.put32(n.frame.at(0x58), c.r(0));
            c.put32(n.frame.at(0x54), 0x43300000);
            const auto chosen_encoded = c.f64(n.frame.at(0x54));
            c.set_f(0, chosen_encoded);
            reduction = fp.fsub(chosen_encoded, bias);
            c.set_f(1, reduction);
        }
        const auto reduction_bits = fp.fctiwz(0, reduction);
        c.set(3, 0xffd20000);
        n.special_club_or_floor = std::uint32_t(-3000000);
        c.put64(n.frame.at(0x5c), reduction_bits);
        const auto bounded_reduction = c.i32(n.frame.at(0x60));
        c.set(0, std::uint32_t(bounded_reduction));
        if (compare(c, -3000000, bounded_reduction) <= 0) n.special_club_or_floor = std::uint32_t(bounded_reduction);
        return;
    }
    c.set(3, std::uint32_t(integer_difference) ^ 0x80000000u);
    c.set(0, integer_floor ^ 0x80000000u);
    c.put32(n.frame.at(0x48), c.r(3));
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto difference_encoded = c.f64(n.frame.at(0x44));
    c.set_f(0, difference_encoded);
    c.put32(n.frame.at(0x50), c.r(0));
    auto adjustment = fp.fsub(difference_encoded, bias);
    c.set_f(1, adjustment);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto converted_floor_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, converted_floor_encoded);
    adjustment = fp.fmul(adjustment, n.tenure_factor);
    c.set_f(1, adjustment);
    const auto converted_floor = fp.fsub(converted_floor_encoded, bias);
    c.set_f(0, converted_floor);
    fp.fcmp(0, adjustment, converted_floor);
    if (!(adjustment < converted_floor)) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen_encoded = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen_encoded);
        adjustment = fp.fsub(chosen_encoded, bias);
        c.set_f(1, adjustment);
    }
    const auto adjustment_bits = fp.fctiwz(0, adjustment);
    auto tenure_limit = c.f64(n.constants + 0x2e8);
    c.set_f(1, tenure_limit);
    const auto tenure_square = fp.fmul(n.tenure_factor, n.tenure_factor);
    c.set_f(2, tenure_square);
    c.put64(n.frame.at(0x5c), adjustment_bits);
    fp.fcmp(0, tenure_limit, tenure_square);
    const auto integer_adjustment = c.u32(n.frame.at(0x60));
    c.set(0, integer_adjustment);
    if (!(tenure_limit < tenure_square)) { tenure_limit = tenure_square; c.set_f(1, tenure_limit); }
    c.set(3, integer_adjustment ^ 0x80000000u);
    auto scale = fp.fmul(n.age_factor, n.market_factor);
    c.set_f(0, scale);
    c.set(0, 0x43300000);
    c.put32(n.frame.at(0x68), c.r(3));
    const auto adjustment_bias = c.f64(n.constants + 8);
    c.set_f(2, adjustment_bias);
    c.put32(n.frame.at(0x64), 0x43300000);
    scale = fp.fmul(scale, tenure_limit);
    c.set_f(0, scale);
    const auto final_encoded = c.f64(n.frame.at(0x64));
    c.set_f(1, final_encoded);
    const auto final_adjustment = fp.fsub(final_encoded, adjustment_bias);
    c.set_f(1, final_adjustment);
    const auto scaled_adjustment = fp.fmul(final_adjustment, scale);
    c.set_f(0, scaled_adjustment);
    c.put64(n.frame.at(0x6c), fp.fctiwz(0, scaled_adjustment));
    n.special_club_or_floor = c.u32(n.frame.at(0x70));
}
}

void initial_quote_adjustment(Context& context) {
    combine_factors(context);
    const auto inputs = quality_floor(context);
    quality_premium(context, inputs);
    price_adjustment(context);
}
}

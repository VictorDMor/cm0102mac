#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
bool same_region(Context& n) {
    auto& c = n.c;
    const auto buyer_location = c.u32(n.buyer + 0x3c);
    c.set(3, buyer_location);
    if (!nonzero_logical(c, buyer_location)) return false;
    const auto employer_location = c.u32(n.employer + 0x3c);
    c.set(4, employer_location);
    if (!nonzero_logical(c, employer_location)) return false;
    const auto buyer_region = c.u32(buyer_location + 0x10);
    c.set(3, buyer_region);
    const auto employer_region = c.u32(employer_location + 0x10);
    c.set(0, employer_region);
    return compare_logical(c, buyer_region, employer_region) == 0;
}

void relationship_weight(Context& n, unsigned minimum_offset, unsigned divisor_offset,
                         unsigned additive_offset = 0) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto score = std::int16_t(std::uint32_t(n.national_context_or_role_score));
    c.set(3, extended(score));
    c.set(0, 0x43300000);
    c.set(3, extended(score) ^ 0x80000000u);
    c.put32(n.frame.at(0x94), 0x43300000);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    c.put32(n.frame.at(0x98), c.r(3));
    const auto divisor = c.f64(n.constants + divisor_offset);
    c.set_f(0, divisor);
    const auto encoded = c.f64(n.frame.at(0x94));
    c.set_f(1, encoded);
    double additive = 0;
    if (additive_offset) {
        additive = c.f64(n.constants + additive_offset);
        c.set_f(3, additive);
    } else {
        n.relationship_factor = c.f64(n.constants + minimum_offset);
        c.set_f(31, n.relationship_factor);
    }
    const auto signed_score = fp.fsub(encoded, bias);
    c.set_f(1, signed_score);
    if (additive_offset) {
        n.relationship_factor = c.f64(n.constants + minimum_offset);
        c.set_f(31, n.relationship_factor);
    } else additive = n.relationship_factor;
    const auto contribution = fp.fdiv(signed_score, divisor);
    c.set_f(0, contribution);
    const auto candidate = fp.fadd(additive, contribution);
    c.set_f(0, candidate);
    fp.fcmp(0, n.relationship_factor, candidate);
    if (!(n.relationship_factor > candidate)) {
        n.relationship_factor = candidate;
        c.set_f(31, candidate);
    }
}

void relationship_diagnostic(Context& n, unsigned flag, bool inclusive) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.diagnostics)) return;
    const auto threshold = c.f64(n.constants + 0x30);
    c.set_f(0, threshold);
    n.fp.fcmp(0, n.relationship_factor, threshold);
    if (inclusive) {
        // The original cror merges ordered GT into EQ without discarding
        // the other bits of the floating comparison.
        c.set_cr(0, c.cr(0) | ((c.cr(0) & 4) >> 1));
    }
    const bool report = inclusive ? n.relationship_factor >= threshold : n.relationship_factor > threshold;
    if (report) {
        const auto flags = extended(c.i16(n.diagnostics)) | flag;
        c.set(0, flags);
        c.put16(n.diagnostics, flags);
    }
}

void club_relationship(Context& n) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto mood = std::int16_t(std::uint32_t(n.mood));
    c.set(3, extended(mood));
    bool reduced = false;
    if (compare(c, mood, 20) > 0) {
        const auto terms = c.i32(n.contract + 0x50);
        c.set(0, std::uint32_t(terms));
        reduced = compare(c, terms) == 0;
    }
    if (reduced) {
        c.set(3, extended(mood) ^ 0x80000000u);
        c.set(0, 0x43300000);
        c.put32(n.frame.at(0x98), c.r(3));
        const auto bias = c.f64(n.constants + 8);
        c.set_f(1, bias);
        c.put32(n.frame.at(0x94), 0x43300000);
        const auto scale = c.f64(n.constants + 0xe0);
        c.set_f(2, scale);
        const auto encoded = c.f64(n.frame.at(0x94));
        c.set_f(0, encoded);
        const auto signed_mood = n.fp.fsub(encoded, bias);
        c.set_f(0, signed_mood);
        const auto scaled_mood = n.fp.fmul(scale, signed_mood);
        c.set_f(0, scaled_mood);
        c.put64(n.frame.at(0x9c), n.fp.fctiwz(0, scaled_mood));
        n.national_context_or_role_score = c.u32(n.frame.at(0xa0));
    } else n.national_context_or_role_score = n.mood;

    const auto first_special_holder = c.toc(-0x7a04);
    c.set(3, first_special_holder);
    const auto buyer_id = c.u32(n.buyer);
    c.set(4, buyer_id);
    const auto first_special = c.u32(first_special_holder);
    c.set(3, first_special);
    bool special_buyer = compare(c, std::int32_t(buyer_id), std::int32_t(first_special)) == 0;
    if (!special_buyer) {
        const auto second_special = c.u32(n.special_club_or_floor);
        c.set(0, second_special);
        special_buyer = compare(c, std::int32_t(buyer_id), std::int32_t(second_special)) == 0;
    }
    bool special_pair = false;
    if (special_buyer) {
        const auto employer_id = c.u32(n.employer);
        c.set(4, employer_id);
        special_pair = compare(c, std::int32_t(employer_id), std::int32_t(first_special)) == 0;
        if (!special_pair) {
            const auto second_special = c.u32(n.special_club_or_floor);
            c.set(0, second_special);
            special_pair = compare(c, std::int32_t(employer_id), std::int32_t(second_special)) == 0;
        }
    }
    if (special_pair) {
        relationship_weight(n, 0x2e0, 0x2e8);
        relationship_diagnostic(n, 0x800, true);
        return;
    }
    const auto raw_relationship = n.call(0x10464160, 0x10086324, {n.buyer, n.employer});
    const auto related = std::int8_t(raw_relationship);
    c.set(0, std::uint32_t(std::int32_t(related)));
    if (nonzero(c, related)) {
        if (same_region(n)) relationship_weight(n, 0x98, 0x128);
        else relationship_weight(n, 0x30, 0x240, 0x2f0);
        relationship_diagnostic(n, 0x800, true);
        return;
    }
    bool rival = false;
    for (unsigned offset : {0x74u, 0x78u, 0x7cu}) {
        const auto other = c.u32(n.employer + offset);
        c.set(0, other);
        if (compare_logical(c, n.buyer, other) == 0) { rival = true; break; }
    }
    if (rival) {
        if (same_region(n)) relationship_weight(n, 0x30, 0x240, 0x2f0);
        else relationship_weight(n, 0x30, 0x228);
        relationship_diagnostic(n, 0x800, true);
        return;
    }
    const auto buyer_competition = c.u32(n.buyer + 0x30);
    c.set(5, buyer_competition);
    if (!nonzero_logical(c, buyer_competition)) return;
    const auto excluded = c.toc(-0x7a0c);
    c.set(3, excluded);
    const auto competition_id = c.u32(buyer_competition);
    c.set(4, competition_id);
    const auto excluded_id = c.u32(excluded);
    c.set(0, excluded_id);
    if (compare(c, std::int32_t(competition_id), std::int32_t(excluded_id)) == 0) return;
    const auto employer_competition = c.u32(n.employer + 0x30);
    c.set(0, employer_competition);
    if (compare_logical(c, buyer_competition, employer_competition) != 0) return;
    relationship_weight(n, 0x18, 0x1d8);
    relationship_diagnostic(n, 0x1000, false);
}

void player_potential(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(3, 0x51ec0000);
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    c.set(4, 0x51eb851f);
    c.set(3, 0x66660000);
    const auto high = multiply_high(0x51eb851f, reputation);
    c.set(0, std::uint32_t(high));
    const auto report = nonzero_logical(c, n.diagnostics);
    c.set(4, 0x66666667);
    const auto first_floor = high >> 4;
    c.set(0, std::uint32_t(first_floor));
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
    c.set(3, std::uint32_t(first_floor) >> 31);
    const auto bracket = std::uint32_t(first_floor) + (std::uint32_t(first_floor) >> 31);
    c.set(3, bracket);
    const auto step = std::int16_t(bracket + 1);
    c.set(0, extended(step));
    const auto scaled_step = extended(step) * 17;
    c.set(3, scaled_step);
    const auto second_high = multiply_high(0x66666667, std::int32_t(scaled_step));
    c.set(3, std::uint32_t(second_high));
    const auto second_floor = second_high >> 3;
    c.set(3, std::uint32_t(second_floor));
    c.s.wr<1>(guest::reg::xer_ca, second_high < 0 && (std::uint32_t(second_high) & 7));
    c.set(4, std::uint32_t(second_floor) >> 31);
    const auto rounded_threshold = std::uint32_t(second_floor) + (std::uint32_t(second_floor) >> 31);
    c.set(3, rounded_threshold);
    const auto threshold = std::int16_t(rounded_threshold);
    c.set(5, extended(threshold));
    if (report) {
        const auto raw_squad_role = c.u8(n.contract + 0x60);
        c.set(3, raw_squad_role);
        const auto squad_role = raw_squad_role & 15u;
        c.set(3, squad_role);
        if (compare(c, std::int32_t(squad_role), 5) == 0) {
            const auto flags = extended(c.i16(n.diagnostics)) | 0x2000u;
            c.set(0, flags);
            c.put16(n.diagnostics, flags);
            return;
        }
    }
    if (compare(c, std::int32_t(std::uint32_t(n.suitability)), threshold) <= 0) return;
    const auto stored_age = c.i16(n.person + 0x5c);
    c.set(3, extended(stored_age));
    const auto age = std::int8_t(stored_age);
    c.set(3, std::uint32_t(std::int32_t(age)));
    if (compare(c, age, 25) >= 0) return;
    const auto raw_squad_role = c.u8(n.contract + 0x60);
    c.set(3, raw_squad_role);
    const auto squad_role = raw_squad_role & 15u;
    c.set(4, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) >= 0) return;
    if (nonzero_logical(c, n.diagnostics)) {
        c.set(3, squad_role);
        if (compare(c, std::int32_t(squad_role), 6) == 0) {
            const auto flags = extended(c.i16(n.diagnostics)) | 0x2000u;
            c.set(0, flags);
            c.put16(n.diagnostics, flags);
            return;
        }
    }
    c.set(3, 0x43300000);
    c.set(0, extended(step) ^ 0x80000000u);
    c.put32(n.frame.at(0x98), c.r(0));
    const auto excess = std::uint32_t(n.suitability) - extended(threshold);
    c.set(0, excess ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x94), 0x43300000);
    const auto reputation_divisor = c.f64(n.constants + 0x1d8);
    c.set_f(1, reputation_divisor);
    const auto encoded_step = c.f64(n.frame.at(0x94));
    c.set_f(0, encoded_step);
    c.put32(n.frame.at(0xa0), c.r(0));
    const auto step_value = fp.fsub(encoded_step, bias);
    c.set_f(2, step_value);
    const auto potential_divisor = c.f64(n.constants + 0x2e8);
    c.set_f(0, potential_divisor);
    c.put32(n.frame.at(0x9c), 0x43300000);
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(4, one);
    const auto step_scale = fp.fdiv(step_value, reputation_divisor);
    c.set_f(2, step_scale);
    const auto encoded_excess = c.f64(n.frame.at(0x9c));
    c.set_f(1, encoded_excess);
    c.put32(n.frame.at(0x8c), 0x43300000);
    const auto excess_value = fp.fsub(encoded_excess, bias);
    c.set_f(1, excess_value);
    const auto weighted_excess = fp.fmul(step_scale, excess_value);
    c.set_f(1, weighted_excess);
    c.put64(n.frame.at(0xa4), fp.fctiwz(1, weighted_excess));
    const auto integer_excess = c.u32(n.frame.at(0xa8));
    c.set(0, integer_excess);
    const auto narrowed_excess = std::int16_t(integer_excess);
    c.set(0, extended(narrowed_excess));
    c.set(0, extended(narrowed_excess) ^ 0x80000000u);
    c.put32(n.frame.at(0x90), c.r(0));
    const auto narrowed_encoded = c.f64(n.frame.at(0x8c));
    c.set_f(1, narrowed_encoded);
    const auto narrowed_value = fp.fsub(narrowed_encoded, bias);
    c.set_f(1, narrowed_value);
    const auto contribution = fp.fdiv(narrowed_value, potential_divisor);
    c.set_f(0, contribution);
    n.potential_factor = fp.fadd(one, contribution);
    c.set_f(29, n.potential_factor);
    fp.fcmp(0, n.potential_factor, one);
    if (n.potential_factor < one) { n.potential_factor = one; c.set_f(29, one); }
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 4) == 0) {
        n.quote_factor = fp.fmul(n.quote_factor, n.potential_factor);
        c.set_f(27, n.quote_factor);
    }
    if (!nonzero_logical(c, n.diagnostics)) return;
    const auto diagnostic_threshold = c.f64(n.constants + 0x2f0);
    c.set_f(0, diagnostic_threshold);
    fp.fcmp(0, n.potential_factor, diagnostic_threshold);
    if (n.potential_factor > diagnostic_threshold) {
        const auto flags = extended(c.i16(n.diagnostics)) | 0x2000u;
        c.set(0, flags);
        c.put16(n.diagnostics, flags);
    }
}
}

void relationships_and_potential(Context& context) {
    club_relationship(context);
    player_potential(context);
}
}

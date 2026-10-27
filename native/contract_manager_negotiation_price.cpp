#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
bool byte_answer(Cpu& c, std::uint32_t value) {
    const auto answer = std::int8_t(value);
    c.set(0, std::uint32_t(std::int32_t(answer)));
    return nonzero(c, answer);
}

void apply_adjustment_and_quality(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    if (compare(c, std::int32_t(std::uint32_t(n.special_club_or_floor))) > 0 &&
        byte_answer(c, n.call(0x100d3150, 0x10086a68, {n.person, 0}))) {
        c.set(3, std::uint32_t(n.special_club_or_floor) ^ 0x80000000u);
        c.set(0, 0x43300000);
        c.put32(n.frame.at(0x48), c.r(3));
        const auto bias = c.f64(n.constants + 8);
        c.set_f(1, bias);
        c.put32(n.frame.at(0x44), 0x43300000);
        const auto half = c.f64(n.constants + 0xb8);
        c.set_f(2, half);
        const auto encoded = c.f64(n.frame.at(0x44));
        c.set_f(0, encoded);
        const auto adjustment = fp.fsub(encoded, bias);
        c.set_f(0, adjustment);
        const auto reduced = fp.fmul(half, adjustment);
        c.set_f(0, reduced);
        c.put64(n.frame.at(0x4c), fp.fctiwz(0, reduced));
        n.special_club_or_floor = c.u32(n.frame.at(0x50));
    }
    c.set(3, 0x2fb0000);
    auto adjustment = std::uint32_t(50000000);
    c.set(0, adjustment);
    if (compare(c, 50000000, std::int32_t(std::uint32_t(n.special_club_or_floor))) >= 0) {
        adjustment = n.special_club_or_floor;
        c.set(0, adjustment);
    }
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(0, one);
    n.price = std::uint32_t(n.price) + adjustment;
    fp.fcmp(0, n.finance_factor, one);
    if (!(n.finance_factor > one)) return;
    c.set(5, std::uint32_t(n.price) ^ 0x80000000u);
    c.set(4, 0x43300000);
    c.put32(n.frame.at(0x48), c.r(5));
    c.set(3, 0x4c0000);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.set(0, 5000000);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto encoded = c.f64(n.frame.at(0x44));
    c.set_f(0, encoded);
    c.put32(n.frame.at(0x50), c.r(5));
    const auto price = fp.fsub(encoded, bias);
    c.set_f(1, price);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto repeated_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, repeated_encoded);
    const auto repeated_price = fp.fsub(repeated_encoded, bias);
    c.set_f(0, repeated_price);
    const auto premium = fp.multiply_add(0x10086af8, price, n.finance_factor, repeated_price);
    c.set_f(0, premium);
    c.put64(n.frame.at(0x54), fp.fctiwz(0, premium));
    auto monetary_premium = c.u32(n.frame.at(0x58));
    c.set(5, monetary_premium);
    if (compare(c, std::int32_t(monetary_premium), 5000000) > 0) {
        c.set(0, monetary_premium ^ 0x80000000u);
        c.put32(n.frame.at(0x5c), 0x43300000);
        const auto slope = c.f64(n.constants + 0x330);
        c.set_f(2, slope);
        c.put32(n.frame.at(0x60), c.r(0));
        const auto intercept = c.f64(n.constants + 0x328);
        c.set_f(0, intercept);
        const auto tier_encoded = c.f64(n.frame.at(0x5c));
        c.set_f(1, tier_encoded);
        const auto tier_value = fp.fsub(tier_encoded, bias);
        c.set_f(1, tier_value);
        const auto tier_premium = fp.multiply_add(0x10086b2c, slope, tier_value, intercept);
        c.set_f(0, tier_premium);
        c.put64(n.frame.at(0x64), fp.fctiwz(0, tier_premium));
        monetary_premium = c.u32(n.frame.at(0x68));
        c.set(5, monetary_premium);
    }
    c.set(3, 0x990000);
    c.set(0, 10000000);
    if (compare(c, std::int32_t(monetary_premium), 10000000) > 0) {
        c.set(3, monetary_premium ^ 0x80000000u);
        c.set(0, 0x43300000);
        c.put32(n.frame.at(0x70), c.r(3));
        const auto tier_bias = c.f64(n.constants + 8);
        c.set_f(2, tier_bias);
        c.put32(n.frame.at(0x6c), 0x43300000);
        const auto slope = c.f64(n.constants + 0x340);
        c.set_f(3, slope);
        const auto tier_encoded = c.f64(n.frame.at(0x6c));
        c.set_f(1, tier_encoded);
        const auto intercept = c.f64(n.constants + 0x338);
        c.set_f(0, intercept);
        const auto tier_value = fp.fsub(tier_encoded, tier_bias);
        c.set_f(1, tier_value);
        const auto tier_premium = fp.multiply_add(0x10086b70, slope, tier_value, intercept);
        c.set_f(0, tier_premium);
        c.put64(n.frame.at(0x74), fp.fctiwz(0, tier_premium));
        monetary_premium = c.u32(n.frame.at(0x78));
        c.set(5, monetary_premium);
    }
    c.set(3, monetary_premium ^ 0x80000000u);
    c.set(0, 0x43300000);
    c.put32(n.frame.at(0x80), c.r(3));
    const auto market_scale = fp.fmul(n.age_factor, n.market_factor);
    c.set_f(0, market_scale);
    const auto final_bias = c.f64(n.constants + 8);
    c.set_f(2, final_bias);
    c.put32(n.frame.at(0x7c), 0x43300000);
    const auto final_encoded = c.f64(n.frame.at(0x7c));
    c.set_f(1, final_encoded);
    const auto final_premium = fp.fsub(final_encoded, final_bias);
    c.set_f(1, final_premium);
    const auto scaled_premium = fp.fmul(final_premium, market_scale);
    c.set_f(0, scaled_premium);
    c.put64(n.frame.at(0x84), fp.fctiwz(0, scaled_premium));
    const auto increase = c.u32(n.frame.at(0x88));
    c.set(0, increase);
    n.price = std::uint32_t(n.price) + increase;
}

void retain_relationship_diagnostic(Context& n) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.diagnostics)) return;
    const auto flags = extended(c.i16(n.diagnostics));
    c.set(0, flags);
    const auto relationship = flags & 0x800u;
    c.set(0, relationship);
    nonzero(c, std::int32_t(relationship));
    const auto retained = relationship ? 0x800u : 0u;
    c.set(0, retained);
    c.put16(n.diagnostics, retained);
}

void cap_to_player_value(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto stored_value = c.u32(n.person + 8);
    c.set(4, stored_value);
    c.set(3, 0x43300000);
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    c.put32(n.frame.at(0x44), 0x43300000);
    c.set(4, stored_value ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    c.put32(n.frame.at(0x48), c.r(4));
    const auto value_encoded = c.f64(n.frame.at(0x44));
    c.set_f(0, value_encoded);
    c.put32(n.frame.at(0x50), c.r(0));
    auto bounded_price = fp.fsub(value_encoded, bias);
    c.set_f(1, bounded_price);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto price_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, price_encoded);
    bounded_price = fp.fmul(bounded_price, n.relationship_factor);
    c.set_f(1, bounded_price);
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(0, price);
    fp.fcmp(0, bounded_price, price);
    if (!(bounded_price < price)) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen_encoded = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen_encoded);
        bounded_price = fp.fsub(chosen_encoded, bias);
        c.set_f(1, bounded_price);
    }
    c.put64(n.frame.at(0x5c), fp.fctiwz(0, bounded_price));
    n.price = c.u32(n.frame.at(0x60));
}

void apply_player_status(Context& n) {
    auto& c = n.c;
    if (compare(c, std::int32_t(std::uint32_t(n.price)), 1000) < 0) n.price = 1000;
    if (byte_answer(c, n.call(0x103ecef0, 0x10086bd4, {c.u32(n.transfers_or_age_gap), n.person, n.employer, 0}))) {
        const auto lower = compare(c, std::int32_t(std::uint32_t(n.price)), std::int32_t(std::uint32_t(n.manager_or_base))) < 0;
        const auto selected = lower ? std::uint32_t(n.price) : std::uint32_t(n.manager_or_base);
        c.set(0, n.manager_or_base);
        if (lower) c.set(0, n.price);
        n.price = selected;
        return;
    }
    if (byte_answer(c, n.call(0x100d3150, 0x10086c00, {n.person, 0}))) {
        retain_relationship_diagnostic(n);
        cap_to_player_value(n);
        return;
    }
    const auto current_club = c.u32(n.person + 0x24);
    c.set(3, current_club);
    if (!nonzero_logical(c, current_club)) return;
    const auto secondary = n.call(0x100ce320, 0x10086cac, {current_club, n.frame.at(0x38), 1});
    if (!nonzero_logical(c, secondary)) return;
    const auto status = c.u8(n.frame.at(0x38));
    c.set(0, status);
    if (byte_answer(c, status)) return;
    const auto raw_squad_role = c.u8(n.contract + 0x60);
    c.set(0, raw_squad_role);
    const auto squad_role = raw_squad_role & 15u;
    c.set(3, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) return;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 5) == 0) return;
    const auto adjacent = (squad_role - 2) & 0xffu;
    c.set(0, adjacent);
    if (compare_logical(c, adjacent, 1) <= 0) return;
    retain_relationship_diagnostic(n);
    cap_to_player_value(n);
}

void relationship_price_floor(Context& n) {
    auto& c = n.c;
    if (byte_answer(c, n.call(0x1014c2e0, 0x10086d8c, {n.employer, 0, 1, 0, 0}))) return;
    if (!nonzero_logical(c, n.diagnostics)) return;
    const auto diagnostic = extended(c.i16(n.diagnostics));
    c.set(0, diagnostic);
    const auto related = diagnostic & 0x800u;
    c.set(0, related);
    if (!nonzero(c, std::int32_t(related))) return;
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    if (compare(c, reputation, 4250) <= 0) return;
    const auto stored_age = c.i16(n.person + 0x5c);
    c.set(0, extended(stored_age));
    const auto age = std::int8_t(stored_age);
    c.set(0, std::uint32_t(std::int32_t(age)));
    if (compare(c, age, 18) <= 0) return;
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return;
    const auto first = std::int8_t(n.call(0x100f55b0, 0x10086ddc, {n.singles_or_finance}));
    c.set(0, std::uint32_t(std::int32_t(first)));
    if (compare(c, first, -1) == 0) return;
    const auto second = std::int8_t(n.call(0x100f55b0, 0x10086df0, {n.singles_or_finance}));
    c.set(0, std::uint32_t(std::int32_t(second)));
    if (compare(c, second, -2) == 0) return;
    const auto current_reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(current_reputation));
    const auto scaled_reputation = extended(current_reputation) * 100;
    c.set(3, scaled_reputation + 0x260000);
    const auto employer_floor = scaled_reputation + 2500000u;
    c.set(0, employer_floor);
    if (compare(c, std::int32_t(std::uint32_t(n.price)), std::int32_t(employer_floor)) >= 0) return;
    // The original rereads the person's club and uses its reputation here.
    const auto current_club = c.u32(n.person + 0x24);
    c.set(3, current_club);
    const auto club_reputation = c.i16(current_club + 0x1ec);
    c.set(0, extended(club_reputation));
    const auto club_scale = extended(club_reputation) * 100;
    c.set(3, club_scale);
    n.price = club_scale + 0x260000;
    n.price = std::uint32_t(n.price) + 0x25a0;
}

void cap_total_price(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto base = std::uint32_t(n.manager_or_base);
    const auto triple = base * 3;
    c.set(4, triple);
    c.set(3, base + 0x4c0000);
    auto ceiling = base + 5000000u;
    c.set(0, ceiling);
    if (compare(c, std::int32_t(triple), std::int32_t(ceiling)) > 0) { ceiling = triple; c.set(0, ceiling); }
    c.set(3, 0x43300000);
    c.set(0, ceiling ^ 0x80000000u);
    c.put32(n.frame.at(0x50), c.r(0));
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto ceiling_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, ceiling_encoded);
    c.put32(n.frame.at(0x48), c.r(0));
    const auto ceiling_value = fp.fsub(ceiling_encoded, bias);
    c.set_f(0, ceiling_value);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto price_encoded = c.f64(n.frame.at(0x44));
    c.set_f(1, price_encoded);
    auto bounded_price = fp.fmul(n.relation_basis, ceiling_value);
    c.set_f(0, bounded_price);
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(1, price);
    fp.fcmp(0, price, bounded_price);
    if (price < bounded_price) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen_encoded = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen_encoded);
        bounded_price = fp.fsub(chosen_encoded, bias);
        c.set_f(0, bounded_price);
    } else {
        c.set(3, base + 0x4c0000);
        ceiling = base + 5000000u;
        c.set(0, ceiling);
        if (compare(c, std::int32_t(triple), std::int32_t(ceiling)) > 0) { ceiling = triple; c.set(0, ceiling); }
        c.set(3, ceiling ^ 0x80000000u);
        c.set(0, 0x43300000);
        c.put32(n.frame.at(0x60), c.r(3));
        const auto ceiling_bias = c.f64(n.constants + 8);
        c.set_f(1, ceiling_bias);
        c.put32(n.frame.at(0x5c), 0x43300000);
        const auto repeated_encoded = c.f64(n.frame.at(0x5c));
        c.set_f(0, repeated_encoded);
        const auto repeated_ceiling = fp.fsub(repeated_encoded, ceiling_bias);
        c.set_f(0, repeated_ceiling);
        bounded_price = fp.fmul(n.relation_basis, repeated_ceiling);
        c.set_f(0, bounded_price);
    }
    c.put64(n.frame.at(0x64), fp.fctiwz(0, bounded_price));
    n.price = c.u32(n.frame.at(0x68));
}

void blend_contract_expiry(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    if (compare(c, std::int32_t(std::uint32_t(n.price)), std::int32_t(std::uint32_t(n.manager_or_base))) <= 0) return;
    const auto terms = c.u32(n.contract + 0x50);
    c.set(3, terms);
    bool prorated = false;
    for (unsigned mask : {2u, 1u, 8u, 0x20u, 0x40u, 0x100u}) {
        const auto selected = terms & mask;
        c.set(0, selected);
        if (nonzero(c, std::int32_t(selected))) { prorated = true; break; }
    }
    if (!prorated) return;
    const auto current_year = c.u16(n.today + 2);
    c.set(4, current_year);
    const auto expiry_year = c.u16(n.contract + 0x32);
    c.set(0, expiry_year);
    const auto current_day = c.i16(n.today);
    c.set(3, extended(current_day));
    const auto years = std::uint32_t(expiry_year) - current_year;
    c.set(4, years);
    const auto expiry_day = c.i16(n.contract + 0x30);
    c.set(0, extended(expiry_day));
    c.set(4, years * 365u);
    c.set(0, extended(expiry_day) - extended(current_day));
    const auto days = years * 365u + extended(expiry_day) - extended(current_day);
    c.set(4, days);
    std::uint32_t expiry_base;
    if (compare(c, std::int32_t(days), 365) >= 0) {
        expiry_base = n.manager_or_base;
        c.set(5, expiry_base);
    } else {
        c.set(3, 0x43300000);
        c.set(0, days ^ 0x80000000u);
        c.put32(n.frame.at(0x50), c.r(0));
        c.set(0, std::uint32_t(n.manager_or_base) ^ 0x80000000u);
        const auto bias = c.f64(n.constants + 8);
        c.set_f(2, bias);
        c.put32(n.frame.at(0x4c), 0x43300000);
        const auto year = c.f64(n.constants + 0xf8);
        c.set_f(0, year);
        const auto days_encoded = c.f64(n.frame.at(0x4c));
        c.set_f(1, days_encoded);
        c.put32(n.frame.at(0x48), c.r(0));
        const auto signed_days = fp.fsub(days_encoded, bias);
        c.set_f(1, signed_days);
        c.put32(n.frame.at(0x44), 0x43300000);
        const auto fraction = fp.fdiv(signed_days, year);
        c.set_f(0, fraction);
        const auto base_encoded = c.f64(n.frame.at(0x44));
        c.set_f(1, base_encoded);
        const auto base = fp.fsub(base_encoded, bias);
        c.set_f(1, base);
        const auto scaled_base = fp.fmul(base, fraction);
        c.set_f(0, scaled_base);
        c.put64(n.frame.at(0x54), fp.fctiwz(0, scaled_base));
        expiry_base = c.u32(n.frame.at(0x58));
        c.set(5, expiry_base);
    }
    c.set(3, days ^ 0x80000000u);
    c.set(0, 0x43300000);
    c.put32(n.frame.at(0x60), c.r(3));
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    c.put32(n.frame.at(0x5c), 0x43300000);
    const auto expiry_slope = c.f64(n.constants + 0x348);
    c.set_f(3, expiry_slope);
    const auto days_encoded = c.f64(n.frame.at(0x5c));
    c.set_f(1, days_encoded);
    const auto expiry_intercept = c.f64(n.constants + 0x350);
    c.set_f(0, expiry_intercept);
    const auto signed_days = fp.fsub(days_encoded, bias);
    c.set_f(1, signed_days);
    auto expiry_weight = c.f64(n.constants + 0x60);
    c.set_f(5, expiry_weight);
    const auto expiry_candidate = fp.multiply_add(0x10086fac, expiry_slope, signed_days, expiry_intercept);
    c.set_f(0, expiry_candidate);
    fp.fcmp(0, expiry_weight, expiry_candidate);
    if (!(expiry_weight > expiry_candidate)) {
        expiry_weight = c.f64(n.constants + 0x18);
        c.set_f(5, expiry_weight);
        fp.fcmp(0, expiry_weight, expiry_candidate);
        if (!(expiry_weight < expiry_candidate)) { expiry_weight = expiry_candidate; c.set_f(5, expiry_weight); }
    }
    c.set(3, 0x43300000);
    c.set(0, expiry_base ^ 0x80000000u);
    c.put32(n.frame.at(0x70), c.r(0));
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    auto balance_floor = c.f64(n.constants + 0x18);
    c.set_f(6, balance_floor);
    c.put32(n.frame.at(0x6c), 0x43300000);
    const auto blend_bias = c.f64(n.constants + 8);
    c.set_f(4, blend_bias);
    const auto inverse_weight = fp.fsub(balance_floor, expiry_weight);
    c.set_f(1, inverse_weight);
    const auto expiry_encoded = c.f64(n.frame.at(0x6c));
    c.set_f(0, expiry_encoded);
    c.put32(n.frame.at(0x68), c.r(0));
    const auto base = fp.fsub(expiry_encoded, blend_bias);
    c.set_f(2, base);
    const auto balance_scale = c.f64(n.constants + 0x98);
    c.set_f(0, balance_scale);
    c.put32(n.frame.at(0x64), 0x43300000);
    const auto price_encoded = c.f64(n.frame.at(0x64));
    c.set_f(3, price_encoded);
    const auto relation_scale = fp.fmul(balance_scale, n.relationship_factor);
    c.set_f(0, relation_scale);
    const auto base_part = fp.fmul(base, inverse_weight);
    c.set_f(1, base_part);
    const auto price = fp.fsub(price_encoded, blend_bias);
    c.set_f(2, price);
    const auto balance_weight = fp.fmul(relation_scale, n.finance_balance);
    c.set_f(3, balance_weight);
    const auto interpolated = fp.multiply_add(0x10087018, price, expiry_weight, base_part);
    c.set_f(0, interpolated);
    fp.fcmp(0, balance_floor, balance_weight);
    c.put64(n.frame.at(0x74), fp.fctiwz(0, interpolated));
    const auto integer_blend = c.u32(n.frame.at(0x78));
    c.set(4, integer_blend);
    if (!(balance_floor > balance_weight)) { balance_floor = balance_weight; c.set_f(6, balance_floor); }
    const auto total = integer_blend + expiry_base;
    c.set(3, total);
    c.set(0, 0x43300000);
    c.set(3, total ^ 0x80000000u);
    c.put32(n.frame.at(0x7c), 0x43300000);
    const auto ratio_bias = c.f64(n.constants + 8);
    c.set_f(3, ratio_bias);
    c.put32(n.frame.at(0x80), c.r(3));
    auto ratio = c.f64(n.constants + 0x18);
    c.set_f(1, ratio);
    const auto total_encoded = c.f64(n.frame.at(0x7c));
    c.set_f(0, total_encoded);
    const auto ratio_slope = c.f64(n.constants + 0x360);
    c.set_f(2, ratio_slope);
    const auto total_value = fp.fsub(total_encoded, ratio_bias);
    c.set_f(0, total_value);
    const auto ratio_candidate = fp.fdiv(total_value, balance_floor);
    c.set_f(0, ratio_candidate);
    fp.fcmp(0, ratio, ratio_candidate);
    if (!(ratio > ratio_candidate)) {
        ratio = c.f64(n.constants + 0x100);
        c.set_f(1, ratio);
        fp.fcmp(0, ratio, ratio_candidate);
        if (!(ratio < ratio_candidate)) { ratio = ratio_candidate; c.set_f(1, ratio); }
    }
    const auto blend_intercept = c.f64(n.constants + 0x358);
    c.set_f(0, blend_intercept);
    c.set(3, 0x43300000);
    c.set(0, expiry_base ^ 0x80000000u);
    c.set(4, integer_blend ^ 0x80000000u);
    c.put32(n.frame.at(0x90), c.r(0));
    const auto blend = fp.multiply_add(0x10087098, ratio_slope, ratio, blend_intercept);
    c.set_f(4, blend);
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(0, one);
    c.put32(n.frame.at(0x8c), 0x43300000);
    const auto final_bias = c.f64(n.constants + 8);
    c.set_f(3, final_bias);
    const auto base_encoded = c.f64(n.frame.at(0x8c));
    c.set_f(1, base_encoded);
    const auto inverse_blend = fp.fsub(one, blend);
    c.set_f(0, inverse_blend);
    c.put32(n.frame.at(0x88), c.r(4));
    const auto final_base = fp.fsub(base_encoded, final_bias);
    c.set_f(1, final_base);
    c.put32(n.frame.at(0x84), 0x43300000);
    const auto blend_encoded = c.f64(n.frame.at(0x84));
    c.set_f(2, blend_encoded);
    const auto final_base_part = fp.fmul(final_base, inverse_blend);
    c.set_f(0, final_base_part);
    const auto final_blend = fp.fsub(blend_encoded, final_bias);
    c.set_f(1, final_blend);
    const auto final_price = fp.multiply_add(0x100870c8, final_blend, blend, final_base_part);
    c.set_f(0, final_price);
    c.put64(n.frame.at(0x94), fp.fctiwz(0, final_price));
    n.price = c.u32(n.frame.at(0x98));
}
}

void protect_and_bound_price(Context& context) {
    apply_adjustment_and_quality(context);
    apply_player_status(context);
    relationship_price_floor(context);
    cap_total_price(context);
    blend_contract_expiry(context);
}
}

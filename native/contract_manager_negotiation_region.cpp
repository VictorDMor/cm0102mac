#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
bool byte_answer(Cpu& c, std::uint32_t value) {
    const auto answer = std::int8_t(value);
    c.set(0, std::uint32_t(std::int32_t(answer)));
    return nonzero(c, answer);
}

bool eligible_region(Context& n) {
    auto& c = n.c;
    const auto manager = c.u32(n.transfers_or_age_gap);
    c.set(3, manager);
    c.set(4, n.employer);
    c.set(5, n.person);
    c.set(6, 0);
    const auto shortlist_manager = c.u32(manager + 0x988);
    c.set(3, shortlist_manager);
    c.set(7, 1);
    const auto shortlist = n.call(0x1036d240, 0x100870f4);
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(4, extended(reputation));
    n.tenure_or_shortlist = shortlist;
    if (compare(c, reputation, 4000) <= 0) return false;
    const auto division = c.u32(n.employer + 0x2c);
    c.set(3, division);
    if (!nonzero_logical(c, division)) return false;
    const auto first_holder = c.toc(-0x7a10);
    c.set(5, first_holder);
    const auto country = c.u32(division + 0x68);
    c.set(0, country);
    const auto first_country = c.u32(first_holder);
    c.set(5, first_country);
    bool eligible = compare(c, std::int32_t(country), std::int32_t(first_country)) == 0;
    if (!eligible) {
        for (int slot : {-0x7a14, -0x7a18, -0x7a00, -0x7af4, -0x7a1c, -0x7a20, -0x7a24, -0x7a28}) {
            const auto holder = c.toc(slot);
            c.set(5, holder);
            const auto candidate = c.u32(holder);
            c.set(5, candidate);
            if (compare(c, std::int32_t(country), std::int32_t(candidate)) == 0) { eligible = true; break; }
        }
        if (!eligible) {
            const auto last_country = c.u32(n.country);
            c.set(5, last_country);
            eligible = compare(c, std::int32_t(country), std::int32_t(last_country)) == 0;
        }
    }
    if (!eligible) return false;
    const auto raw_level = c.u8(division + 0x11f);
    c.set(0, raw_level);
    const auto level = std::int8_t(raw_level);
    c.set(0, std::uint32_t(std::int32_t(level)));
    if (compare(c, level, 12) <= 0 && compare(c, reputation, 7000) < 0) return false;
    if (!nonzero_logical(c, n.singles_or_finance)) return false;
    const auto first_class = std::int8_t(n.call(0x100f55b0, 0x100871d8, {n.singles_or_finance}));
    c.set(0, std::uint32_t(std::int32_t(first_class)));
    if (compare(c, first_class, -1) == 0) return false;
    const auto second_class = std::int8_t(n.call(0x100f55b0, 0x100871ec, {n.singles_or_finance}));
    c.set(0, std::uint32_t(std::int32_t(second_class)));
    if (compare(c, second_class, -2) == 0) return false;
    if (byte_answer(c, n.call(0x100d3040, 0x10087204, {n.person, 0}))) return false;
    return !byte_answer(c, n.call(0x100d3300, 0x10087218, {n.person, 0}));
}

bool shortlisted(Context& n) {
    auto& c = n.c;
    if (nonzero_logical(c, n.tenure_or_shortlist)) {
        const auto candidate = c.u32(n.tenure_or_shortlist + 0x10);
        c.set(3, candidate);
        const auto person_id = c.u32(n.person);
        c.set(0, person_id);
        if (compare(c, std::int32_t(candidate), std::int32_t(person_id)) == 0) return true;
    }
    const auto status = c.u8(n.person + 0x6d);
    c.set(0, status);
    if (compare(c, status, 2) != 0) return false;
    const auto club_status = c.i32(n.employer + 0x14);
    c.set(0, std::uint32_t(club_status));
    return compare(c, club_status, -1) > 0;
}

void young_player_price(Context& n, int age_limit = 25,
                        std::uint32_t first_return = 0x10087294, std::uint32_t second_return = 0x10087380) {
    auto& c = n.c;
    auto& fp = n.fp;
    if (compare(c, std::int32_t(std::uint32_t(n.age)), age_limit) >= 0) return;
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 6) == 0) return;
    const auto suitability_score = std::uint32_t(n.suitability) * 50;
    c.set(3, suitability_score);
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    if (compare(c, std::int32_t(suitability_score), reputation) <= 0) return;
    n.transfers_or_age_gap = std::uint32_t(n.age) - 20;
    n.special_club_or_floor = c.u8(n.contract + 0x4c);
    const auto first_gap = n.call(0x10775540, first_return, {n.transfers_or_age_gap});
    const auto promise = std::int8_t(std::uint32_t(n.special_club_or_floor));
    c.set(4, std::uint32_t(std::int32_t(promise)));
    c.set(0, 0x43300000);
    const auto promise_floor = std::int32_t(promise) >> 2;
    c.set(4, std::uint32_t(promise_floor));
    const bool rounded = promise < 0 && (std::uint32_t(std::int32_t(promise)) & 3);
    c.s.wr<1>(guest::reg::xer_ca, rounded);
    c.set(5, std::uint32_t(n.suitability) ^ 0x80000000u);
    const auto quarter = std::uint32_t(promise_floor) + rounded;
    c.set(4, quarter);
    c.s.wr<1>(guest::reg::xer_ca, rounded && quarter == 0);
    c.set(7, 0x140000);
    const auto denominator = 125u - quarter;
    c.set(4, denominator);
    c.s.wr<1>(guest::reg::xer_ca, quarter <= 125);
    c.set(6, std::uint32_t(n.price) ^ 0x80000000u);
    c.set(4, denominator ^ 0x80000000u);
    c.set(8, 0x145855);
    c.put32(n.frame.at(0x58), c.r(5));
    c.set(7, std::uint32_t(n.price) + 0x4c0000);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    const auto age_deduction = first_gap * 0x145855u;
    c.set(8, age_deduction);
    c.put32(n.frame.at(0x54), 0x43300000);
    const auto age_ceiling = std::uint32_t(n.price) + 5000000u - age_deduction;
    c.set(3, std::uint32_t(n.price) + 5000000u);
    const auto suitability_encoded = c.f64(n.frame.at(0x54));
    c.set_f(0, suitability_encoded);
    c.put32(n.frame.at(0x60), c.r(4));
    const auto suitability = fp.fsub(suitability_encoded, bias);
    c.set_f(1, suitability);
    c.put32(n.frame.at(0x5c), 0x43300000);
    c.set(3, age_ceiling ^ 0x80000000u);
    const auto denominator_encoded = c.f64(n.frame.at(0x5c));
    c.set_f(0, denominator_encoded);
    c.put32(n.frame.at(0x50), c.r(6));
    const auto divisor = fp.fsub(denominator_encoded, bias);
    c.set_f(0, divisor);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto ratio = fp.fdiv(suitability, divisor);
    c.set_f(0, ratio);
    const auto price_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(1, price_encoded);
    c.put32(n.frame.at(0x48), c.r(3));
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto ceiling_encoded = c.f64(n.frame.at(0x44));
    c.set_f(2, ceiling_encoded);
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(1, price);
    const auto ceiling = fp.fsub(ceiling_encoded, bias);
    c.set_f(2, ceiling);
    auto quote = fp.fmul(price, ratio);
    c.set_f(0, quote);
    fp.fcmp(0, quote, ceiling);
    if (quote < ceiling) {
        const auto current_promise = c.i8(n.contract + 0x4c);
        c.set(3, std::uint32_t(std::int32_t(current_promise)));
        c.put32(n.frame.at(0x70), c.r(5));
        const auto floor = std::int32_t(current_promise) >> 2;
        c.set(3, std::uint32_t(floor));
        const bool carry = current_promise < 0 && (std::uint32_t(std::int32_t(current_promise)) & 3);
        c.s.wr<1>(guest::reg::xer_ca, carry);
        c.put32(n.frame.at(0x6c), 0x43300000);
        const auto rounded_quarter = std::uint32_t(floor) + carry;
        c.set(3, rounded_quarter);
        c.s.wr<1>(guest::reg::xer_ca, carry && rounded_quarter == 0);
        const auto current_denominator = 125u - rounded_quarter;
        c.set(3, current_denominator);
        c.s.wr<1>(guest::reg::xer_ca, rounded_quarter <= 125);
        const auto repeated_suitability = c.f64(n.frame.at(0x6c));
        c.set_f(0, repeated_suitability);
        c.set(3, current_denominator ^ 0x80000000u);
        c.put32(n.frame.at(0x74), 0x43300000);
        const auto signed_suitability = fp.fsub(repeated_suitability, bias);
        c.set_f(1, signed_suitability);
        c.put32(n.frame.at(0x78), c.r(3));
        const auto repeated_denominator = c.f64(n.frame.at(0x74));
        c.set_f(0, repeated_denominator);
        c.put32(n.frame.at(0x68), c.r(6));
        const auto signed_denominator = fp.fsub(repeated_denominator, bias);
        c.set_f(0, signed_denominator);
        c.put32(n.frame.at(0x64), 0x43300000);
        const auto repeated_ratio = fp.fdiv(signed_suitability, signed_denominator);
        c.set_f(0, repeated_ratio);
        const auto repeated_price = c.f64(n.frame.at(0x64));
        c.set_f(1, repeated_price);
        const auto signed_price = fp.fsub(repeated_price, bias);
        c.set_f(1, signed_price);
        quote = fp.fmul(signed_price, repeated_ratio);
        c.set_f(0, quote);
    } else {
        const auto second_gap = n.call(0x10775540, second_return, {n.transfers_or_age_gap});
        c.set(5, 0x140000);
        c.set(4, std::uint32_t(n.price) + 0x4c0000);
        c.set(5, 0x145855);
        c.set(0, 0x43300000);
        const auto deduction = second_gap * 0x145855u;
        c.set(5, deduction);
        const auto base_ceiling = std::uint32_t(n.price) + 5000000u;
        c.set(3, base_ceiling);
        c.put32(n.frame.at(0x7c), 0x43300000);
        const auto ceiling_bias = c.f64(n.constants + 8);
        c.set_f(1, ceiling_bias);
        const auto selected = base_ceiling - deduction;
        c.set(0, selected ^ 0x80000000u);
        c.put32(n.frame.at(0x80), c.r(0));
        const auto selected_encoded = c.f64(n.frame.at(0x7c));
        c.set_f(0, selected_encoded);
        quote = fp.fsub(selected_encoded, ceiling_bias);
        c.set_f(0, quote);
    }
    c.put64(n.frame.at(0x84), fp.fctiwz(0, quote));
    n.price = c.u32(n.frame.at(0x88));
}

bool same_competition(Context& n) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.employer)) return false;
    if (!nonzero_logical(c, n.buyer)) return false;
    const auto employer = c.u32(n.employer + 0x30);
    c.set(3, employer);
    const auto buyer = c.u32(n.buyer + 0x30);
    c.set(0, buyer);
    // The original also treats two null competition links as equal.
    return compare_logical(c, employer, buyer) == 0;
}

// A percentage increase capped by a fixed monetary addition. The caller
// stores the converted result at the original point around its club test.
std::uint64_t markup(Context& n, unsigned factor_offset, std::uint32_t increment) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(3, 0x43300000);
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    c.put32(n.frame.at(0x50), c.r(0));
    c.set(4, std::uint32_t(n.price) + ((increment + 0x8000u) & 0xffff0000u));
    const auto limit = std::uint32_t(n.price) + increment;
    c.set(0, limit);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x4c), 0x43300000);
    c.set(0, limit ^ 0x80000000u);
    const auto factor = c.f64(n.constants + factor_offset);
    c.set_f(1, factor);
    const auto price_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, price_encoded);
    c.put32(n.frame.at(0x48), c.r(0));
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(0, price);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto limit_encoded = c.f64(n.frame.at(0x44));
    c.set_f(2, limit_encoded);
    auto quote = fp.fmul(factor, price);
    c.set_f(1, quote);
    const auto limit_value = fp.fsub(limit_encoded, bias);
    c.set_f(0, limit_value);
    fp.fcmp(0, limit_value, quote);
    if (limit_value < quote) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen);
        quote = fp.fsub(chosen, bias);
        c.set_f(1, quote);
    }
    return fp.fctiwz(0, quote);
}

std::uint32_t expiry_distance(Context& n, std::uint32_t back) {
    auto& c = n.c;
    c.set(4, c.u32(n.today));
    c.set(3, n.contract + 0x30);
    c.set(5, c.u32(n.today + 4));
    return n.call(0x10718130, back);
}

void profile_quote(Context& n) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.employer)) return;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto distance = expiry_distance(n, 0x10087898);
    if (compare(c, std::int32_t(distance), 225) <= 0) return;
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return;
    const auto raw_first = c.u8(n.person + 0x67);
    c.set(0, raw_first);
    const auto first_trait = std::int8_t(raw_first);
    c.set(0, std::uint32_t(std::int32_t(first_trait)));
    if (compare(c, first_trait, 5) <= 0) return;
    const auto raw_second = c.u8(n.person + 0x65);
    c.set(4, raw_second);
    const auto second_trait = std::int8_t(raw_second);
    c.set(0, std::uint32_t(std::int32_t(second_trait)));
    if (compare(c, second_trait, 5) <= 0) return;
    const auto attributes = c.u32(n.person + 0x14);
    c.set(3, attributes);
    if (!nonzero_logical(c, attributes)) return;
    const auto reputation = c.i16(attributes + 10);
    c.set(0, extended(reputation));
    if (compare(c, reputation, 6250) <= 0) return;
    const auto raw_mood = c.u8(n.contract + 0x44);
    c.set(3, raw_mood);
    c.set(0, 0u - raw_second);
    const auto opposed_trait = std::int8_t(0u - raw_second);
    c.set(0, std::uint32_t(std::int32_t(opposed_trait)));
    const auto mood = std::int8_t(raw_mood);
    c.set(3, std::uint32_t(std::int32_t(mood)));
    if (compare(c, mood, opposed_trait) <= 0) return;
    const auto age = std::int32_t(std::uint32_t(n.age));
    if (compare(c, age, 22) <= 0) return;
    if (compare(c, age, 31) >= 0) return;
    const auto result = markup(n, 0x40, 1000000);
    const auto employer_competition = c.u32(n.employer + 0x30);
    c.set(3, employer_competition);
    const auto buyer_competition = c.u32(n.buyer + 0x30);
    c.set(0, buyer_competition);
    c.put64(n.frame.at(0x5c), result);
    const auto shared = compare_logical(c, employer_competition, buyer_competition) == 0;
    n.price = c.u32(n.frame.at(0x60));
    if (!shared) return;
    if (!byte_answer(c, n.call(0x10464160, 0x10087990, {n.employer, n.buyer}))) return;
    c.put64(n.frame.at(0x5c), markup(n, 0x40, 400000));
    n.price = c.u32(n.frame.at(0x60));
    return;
}

void long_contract_quote(Context& n, std::uint32_t terms) {
    auto& c = n.c;
    const auto qualifying = terms & 0x101u;
    c.set(0, qualifying);
    if (!nonzero(c, std::int32_t(qualifying))) return profile_quote(n);
    const auto distance = expiry_distance(n, 0x10087620);
    if (compare(c, std::int32_t(distance), 425) <= 0) return profile_quote(n);
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) {
        const auto previous = std::uint32_t(n.price);
        c.set(3, previous + 0x260000);
        n.price = previous * 2;
        const auto limit = previous + 2500000u;
        c.set(0, limit);
        if (compare(c, std::int32_t(limit), std::int32_t(std::uint32_t(n.price))) < 0) n.price = limit;
        if (!same_competition(n)) return;
        const auto price = std::uint32_t(n.price);
        c.set(3, price + 0xf0000);
        n.price = price * 2;
        const auto shared_limit = price + 1000000u;
        c.set(0, shared_limit);
        if (compare(c, std::int32_t(shared_limit), std::int32_t(std::uint32_t(n.price))) < 0) n.price = shared_limit;
        return;
    }
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 2) == 0) {
        const auto result = markup(n, 0xe0, 1500000);
        const auto employer_present = nonzero_logical(c, n.employer);
        c.put64(n.frame.at(0x5c), result);
        n.price = c.u32(n.frame.at(0x60));
        if (!employer_present) return;
        if (!nonzero_logical(c, n.buyer)) return;
        const auto employer_competition = c.u32(n.employer + 0x30);
        c.set(3, employer_competition);
        const auto buyer_competition = c.u32(n.buyer + 0x30);
        c.set(0, buyer_competition);
        if (compare_logical(c, employer_competition, buyer_competition) != 0) return;
        c.put64(n.frame.at(0x5c), markup(n, 0x368, 750000));
        n.price = c.u32(n.frame.at(0x60));
        return;
    }
    if (compare(c, std::int32_t(squad_role), 3) != 0) return;
    const auto result = markup(n, 0x40, 1000000);
    const auto employer_present = nonzero_logical(c, n.employer);
    c.put64(n.frame.at(0x5c), result);
    n.price = c.u32(n.frame.at(0x60));
    if (!employer_present) return;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto employer_competition = c.u32(n.employer + 0x30);
    c.set(3, employer_competition);
    const auto buyer_competition = c.u32(n.buyer + 0x30);
    c.set(0, buyer_competition);
    if (compare_logical(c, employer_competition, buyer_competition) != 0) return;
    c.put64(n.frame.at(0x5c), markup(n, 0x40, 400000));
    n.price = c.u32(n.frame.at(0x60));
    return;
}

void premium_role_quote(Context& n) {
    auto& c = n.c;
    const auto terms = c.u32(n.contract + 0x50);
    c.set(3, terms);
    if (nonzero(c, std::int32_t(terms))) {
        const auto raw_promise = c.u8(n.contract + 0x4c);
        c.set(0, raw_promise);
        const auto promise = std::int8_t(raw_promise);
        c.set(0, std::uint32_t(std::int32_t(promise)));
        if (compare(c, promise, -50) <= 0) return long_contract_quote(n, terms);
    }
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) {
        const auto price = std::uint32_t(n.price);
        auto selected = price * 5;
        c.set(4, selected);
        c.set(3, price + 0x4c0000);
        const auto limit = price + 5000000u;
        c.set(0, limit);
        if (compare(c, std::int32_t(limit), std::int32_t(selected)) < 0) { selected = limit; c.set(4, selected); }
        const auto employer_present = nonzero_logical(c, n.employer);
        n.price = selected;
        if (!employer_present) return;
        if (!nonzero_logical(c, n.buyer)) return;
        const auto employer_competition = c.u32(n.employer + 0x30);
        c.set(3, employer_competition);
        const auto buyer_competition = c.u32(n.buyer + 0x30);
        c.set(0, buyer_competition);
        if (compare_logical(c, employer_competition, buyer_competition) != 0) return;
        c.set(3, selected + 0xf0000);
        n.price = selected * 2;
        const auto extra_limit = selected + 1000000u;
        c.set(0, extra_limit);
        if (compare(c, std::int32_t(extra_limit), std::int32_t(std::uint32_t(n.price))) < 0) n.price = extra_limit;
        return;
    }
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 2) == 0) {
        const auto result = markup(n, 0xf0, 2500000);
        const auto employer_present = nonzero_logical(c, n.employer);
        c.put64(n.frame.at(0x5c), result);
        n.price = c.u32(n.frame.at(0x60));
        if (!employer_present) return;
        if (!nonzero_logical(c, n.buyer)) return;
        const auto employer_competition = c.u32(n.employer + 0x30);
        c.set(3, employer_competition);
        const auto buyer_competition = c.u32(n.buyer + 0x30);
        c.set(0, buyer_competition);
        if (compare_logical(c, employer_competition, buyer_competition) != 0) return;
        c.put64(n.frame.at(0x5c), markup(n, 0x368, 750000));
        n.price = c.u32(n.frame.at(0x60));
        return;
    }
    if (compare(c, std::int32_t(squad_role), 3) != 0) return;
    const auto previous = std::uint32_t(n.price);
    c.set(3, previous + 0x170000);
    n.price = previous * 2;
    const auto limit = previous + 1500000u;
    c.set(0, limit);
    if (compare(c, std::int32_t(limit), std::int32_t(std::uint32_t(n.price))) < 0) n.price = limit;
    if (!same_competition(n)) return;
    c.put64(n.frame.at(0x5c), markup(n, 0x38, 400000));
    n.price = c.u32(n.frame.at(0x60));
    return;
}

// This policy keeps its competition pointer live across the first markup;
// its conversion scratch therefore occupies r4/r5 rather than r3/r4.
std::uint64_t retained_competition_markup(Context& n, unsigned factor_offset, std::uint32_t increment) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(4, 0x43300000);
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    c.put32(n.frame.at(0x50), c.r(0));
    c.set(5, std::uint32_t(n.price) + ((increment + 0x8000u) & 0xffff0000u));
    const auto limit = std::uint32_t(n.price) + increment;
    c.set(0, limit);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x4c), 0x43300000);
    c.set(0, limit ^ 0x80000000u);
    const auto factor = c.f64(n.constants + factor_offset);
    c.set_f(1, factor);
    const auto encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, encoded);
    c.put32(n.frame.at(0x48), c.r(0));
    const auto price = fp.fsub(encoded, bias);
    c.set_f(0, price);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto limit_encoded = c.f64(n.frame.at(0x44));
    c.set_f(2, limit_encoded);
    auto quote = fp.fmul(factor, price);
    c.set_f(1, quote);
    const auto limit_value = fp.fsub(limit_encoded, bias);
    c.set_f(0, limit_value);
    fp.fcmp(0, limit_value, quote);
    if (limit_value < quote) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen);
        quote = fp.fsub(chosen, bias);
        c.set_f(1, quote);
    }
    return fp.fctiwz(0, quote);
}

std::uint64_t small_markup(Context& n, unsigned factor_offset = 0x380, std::uint32_t increment = 25000) {
    auto& c = n.c;
    auto& fp = n.fp;
    c.set(3, 0x43300000);
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    c.put32(n.frame.at(0x50), c.r(0));
    const auto limit = std::uint32_t(n.price) + increment;
    c.set(0, limit ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto factor = c.f64(n.constants + factor_offset);
    c.set_f(1, factor);
    const auto encoded = c.f64(n.frame.at(0x4c));
    c.set_f(0, encoded);
    c.put32(n.frame.at(0x48), c.r(0));
    const auto price = fp.fsub(encoded, bias);
    c.set_f(0, price);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto limit_encoded = c.f64(n.frame.at(0x44));
    c.set_f(2, limit_encoded);
    auto quote = fp.fmul(factor, price);
    c.set_f(1, quote);
    const auto limit_value = fp.fsub(limit_encoded, bias);
    c.set_f(0, limit_value);
    fp.fcmp(0, limit_value, quote);
    if (limit_value < quote) {
        c.put32(n.frame.at(0x58), c.r(0));
        c.put32(n.frame.at(0x54), 0x43300000);
        const auto chosen = c.f64(n.frame.at(0x54));
        c.set_f(0, chosen);
        quote = fp.fsub(chosen, bias);
        c.set_f(1, quote);
    }
    return fp.fctiwz(0, quote);
}

void lower_reputation_role_quote(Context& n, std::uint32_t competition) {
    auto& c = n.c;
    const auto terms = c.u32(n.contract + 0x50);
    c.set(4, terms);
    bool long_contract = false;
    if (nonzero(c, std::int32_t(terms))) {
        const auto raw_promise = c.u8(n.contract + 0x4c);
        c.set(0, raw_promise);
        const auto promise = std::int8_t(raw_promise);
        c.set(0, std::uint32_t(std::int32_t(promise)));
        long_contract = compare(c, promise) <= 0;
    }
    if (long_contract) {
        const auto qualifying = terms & 0x101u;
        c.set(0, qualifying);
        if (!nonzero(c, std::int32_t(qualifying))) return;
        const auto distance = expiry_distance(n, 0x10087d08);
        if (compare(c, std::int32_t(distance), 425) <= 0) return;
        const auto squad_role = c.u8(n.contract + 0x60) & 15u;
        c.set(0, squad_role);
        unsigned factor, extra_factor;
        std::uint32_t increment, extra_increment;
        if (compare(c, std::int32_t(squad_role), 1) == 0) {
            factor = 0x40; increment = 375000; extra_factor = 0x388; extra_increment = 50000;
        } else {
            c.set(0, squad_role);
            if (compare(c, std::int32_t(squad_role), 2) != 0) return;
            factor = 0x20; increment = 200000; extra_factor = 0x380; extra_increment = 37500;
        }
        const auto result = markup(n, factor, increment);
        const auto employer_present = nonzero_logical(c, n.employer);
        c.put64(n.frame.at(0x5c), result);
        n.price = c.u32(n.frame.at(0x60));
        if (!employer_present) return;
        if (!nonzero_logical(c, n.buyer)) return;
        const auto employer = c.u32(n.employer + 0x30);
        c.set(3, employer);
        const auto buyer = c.u32(n.buyer + 0x30);
        c.set(0, buyer);
        if (compare_logical(c, employer, buyer) != 0) return;
        c.put64(n.frame.at(0x5c), markup(n, extra_factor, extra_increment));
        n.price = c.u32(n.frame.at(0x60));
        return;
    }
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    unsigned factor, extra_factor;
    std::uint32_t increment, extra_increment;
    if (compare(c, std::int32_t(squad_role), 1) == 0) {
        factor = 0x368; increment = 750000; extra_factor = 0x28; extra_increment = 100000;
    } else {
        c.set(0, squad_role);
        if (compare(c, std::int32_t(squad_role), 2) == 0) {
            factor = 0x38; increment = 400000; extra_factor = 0x370; extra_increment = 75000;
        } else {
            if (compare(c, std::int32_t(squad_role), 3) != 0) return;
            factor = 0x378; increment = 150000; extra_factor = 0x380; extra_increment = 25000;
        }
    }
    const auto result = retained_competition_markup(n, factor, increment);
    const auto employer_present = nonzero_logical(c, n.employer);
    c.put64(n.frame.at(0x5c), result);
    n.price = c.u32(n.frame.at(0x60));
    if (!employer_present) return;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto buyer_competition = c.u32(n.buyer + 0x30);
    c.set(0, buyer_competition);
    if (compare_logical(c, competition, buyer_competition) != 0) return;
    c.put64(n.frame.at(0x5c), squad_role == 3 ? small_markup(n) : markup(n, extra_factor, extra_increment));
    n.price = c.u32(n.frame.at(0x60));
    return;
}

void shared_markup(Context& n, unsigned factor, std::uint32_t increment,
                   unsigned extra_factor, std::uint32_t extra_increment, bool immediate_extra = false) {
    auto& c = n.c;
    const auto result = markup(n, factor, increment);
    const auto employer_present = nonzero_logical(c, n.employer);
    c.put64(n.frame.at(0x5c), result);
    n.price = c.u32(n.frame.at(0x60));
    if (!employer_present) return;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto employer = c.u32(n.employer + 0x30);
    c.set(3, employer);
    const auto buyer = c.u32(n.buyer + 0x30);
    c.set(0, buyer);
    if (compare_logical(c, employer, buyer) != 0) return;
    const auto extra = immediate_extra ? small_markup(n, extra_factor, extra_increment) : markup(n, extra_factor, extra_increment);
    c.put64(n.frame.at(0x5c), extra);
    n.price = c.u32(n.frame.at(0x60));
}

void ordinary_profile_quote(Context& n) {
    auto& c = n.c;
    const auto distance = expiry_distance(n, 0x10088674);
    if (compare(c, std::int32_t(distance), 225) <= 0) return;
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return;
    const auto raw_first = c.u8(n.person + 0x67);
    c.set(0, raw_first);
    const auto raw_second = c.u8(n.person + 0x65);
    c.set(4, raw_second);
    const auto first = std::int8_t(raw_first), second = std::int8_t(raw_second);
    c.set(0, std::uint32_t(std::int32_t(first)));
    c.set(4, std::uint32_t(std::int32_t(second)));
    const auto combined_traits = std::int32_t(first) + second;
    c.set(0, std::uint32_t(combined_traits));
    if (compare(c, combined_traits, 10) <= 0) return;
    const auto attributes = c.u32(n.person + 0x14);
    c.set(3, attributes);
    if (!nonzero_logical(c, attributes)) return;
    const auto reputation = c.i16(attributes + 10);
    c.set(0, extended(reputation));
    if (compare(c, reputation, 5250) <= 0) return;
    const auto raw_mood = c.u8(n.contract + 0x44);
    c.set(3, raw_mood);
    const auto threshold_word = 10u - std::uint32_t(std::int32_t(second));
    c.set(0, threshold_word);
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(second)) <= 10);
    const auto threshold = std::int8_t(threshold_word), mood = std::int8_t(raw_mood);
    c.set(0, std::uint32_t(std::int32_t(threshold)));
    c.set(3, std::uint32_t(std::int32_t(mood)));
    if (compare(c, mood, threshold) <= 0) return;
    const auto age = std::int32_t(std::uint32_t(n.age));
    if (compare(c, age, 22) <= 0) return;
    if (compare(c, age, 31) >= 0) return;
    shared_markup(n, 0x378, 750000, 0x390, 150000);
}

void ordinary_long_quote(Context& n, std::uint32_t terms) {
    auto& c = n.c;
    const auto selected = terms & 0x101u;
    c.set(0, selected);
    if (!nonzero(c, std::int32_t(selected))) return ordinary_profile_quote(n);
    const auto distance = expiry_distance(n, 0x10088374);
    if (compare(c, std::int32_t(distance), 425) <= 0) return ordinary_profile_quote(n);
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) return shared_markup(n, 0x40, 500000, 0x28, 250000);
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 2) == 0) return shared_markup(n, 0x20, 1000000, 0x388, 200000);
    if (compare(c, std::int32_t(squad_role), 3) == 0) shared_markup(n, 0x378, 750000, 0x390, 150000);
}

void ordinary_premium_quote(Context& n) {
    auto& c = n.c;
    const auto terms = c.u32(n.contract + 0x50);
    c.set(3, terms);
    if (nonzero(c, std::int32_t(terms))) {
        const auto raw_promise = c.u8(n.contract + 0x4c);
        c.set(0, raw_promise);
        const auto promise = std::int8_t(raw_promise);
        c.set(0, std::uint32_t(std::int32_t(promise)));
        if (compare(c, promise) <= 0) return ordinary_long_quote(n, terms);
    }
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) {
        const auto previous = std::uint32_t(n.price);
        c.set(3, previous + 0x1f0000);
        n.price = previous * 2;
        const auto limit = previous + 2000000u;
        c.set(0, limit);
        if (compare(c, std::int32_t(limit), std::int32_t(std::uint32_t(n.price))) < 0) n.price = limit;
        if (!same_competition(n)) return;
        c.put64(n.frame.at(0x5c), markup(n, 0x28, 100000));
        n.price = c.u32(n.frame.at(0x60));
        return;
    }
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 2) == 0) return shared_markup(n, 0xe0, 1500000, 0x370, 75000);
    if (compare(c, std::int32_t(squad_role), 3) == 0) shared_markup(n, 0xe0, 1000000, 0x390, 20000, true);
}

void ordinary_lower_quote(Context& n, std::uint32_t competition) {
    auto& c = n.c;
    const auto terms = c.u32(n.contract + 0x50);
    c.set(0, terms);
    if (nonzero(c, std::int32_t(terms))) {
        const auto raw_promise = c.u8(n.contract + 0x4c);
        c.set(0, raw_promise);
        const auto promise = std::int8_t(raw_promise);
        c.set(0, std::uint32_t(std::int32_t(promise)));
        if (compare(c, promise) <= 0) return;
    }
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) {
        const auto result = retained_competition_markup(n, 0x40, 250000);
        const auto employer_present = nonzero_logical(c, n.employer);
        c.put64(n.frame.at(0x5c), result);
        n.price = c.u32(n.frame.at(0x60));
        if (!employer_present) return;
        if (!nonzero_logical(c, n.buyer)) return;
        const auto buyer = c.u32(n.buyer + 0x30);
        c.set(0, buyer);
        if (compare_logical(c, competition, buyer) != 0) return;
        c.put64(n.frame.at(0x5c), markup(n, 0x388, 50000));
        n.price = c.u32(n.frame.at(0x60));
        return;
    }
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 2) != 0) {
        const auto raw_trait = c.u8(n.person + 0x65);
        c.set(0, raw_trait);
        const auto raw_mood = c.u8(n.contract + 0x44);
        c.set(4, raw_mood);
        const auto trait = std::int8_t(raw_trait), mood = std::int8_t(raw_mood);
        c.set(0, std::uint32_t(std::int32_t(trait)));
        c.set(4, std::uint32_t(std::int32_t(mood)));
        const auto threshold = 40u - std::uint32_t(std::int32_t(trait));
        c.set(0, threshold);
        c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(trait)) <= 40);
        if (compare(c, mood, std::int32_t(threshold)) <= 0) return;
    }
    const auto result = retained_competition_markup(n, 0x28, 100000);
    const auto employer_present = nonzero_logical(c, n.employer);
    c.put64(n.frame.at(0x5c), result);
    n.price = c.u32(n.frame.at(0x60));
    if (!employer_present) return;
    if (!nonzero_logical(c, n.buyer)) return;
    const auto buyer = c.u32(n.buyer + 0x30);
    c.set(0, buyer);
    if (compare_logical(c, competition, buyer) != 0) return;
    c.put64(n.frame.at(0x5c), small_markup(n));
    n.price = c.u32(n.frame.at(0x60));
}

void unlisted_price(Context& n, bool ineligible_region) {
    auto& c = n.c;
    auto& fp = n.fp;
    if (nonzero_logical(c, n.tenure_or_shortlist)) return;
    if (compare(c, std::int32_t(std::uint32_t(n.price)), std::int32_t(std::uint32_t(n.manager_or_base))) <= 0) return;
    const auto mood = std::int16_t(std::uint32_t(n.mood));
    c.set(0, extended(mood));
    if (compare(c, mood, 60) < 0) {
        c.set(3, 0x43300000);
        c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
        c.put32(n.frame.at(0x50), c.r(0));
        c.set(0, std::uint32_t(n.manager_or_base) ^ 0x80000000u);
        const auto bias = c.f64(n.constants + 8);
        c.set_f(3, bias);
        c.put32(n.frame.at(0x4c), 0x43300000);
        const auto price_weight = c.f64(n.constants + 0x3a0);
        c.set_f(1, price_weight);
        const auto price_encoded = c.f64(n.frame.at(0x4c));
        c.set_f(0, price_encoded);
        c.put32(n.frame.at(0x48), c.r(0));
        const auto price = fp.fsub(price_encoded, bias);
        c.set_f(0, price);
        const auto base_weight = c.f64(n.constants + 0x398);
        c.set_f(4, base_weight);
        c.put32(n.frame.at(0x44), 0x43300000);
        const auto base_encoded = c.f64(n.frame.at(0x44));
        c.set_f(2, base_encoded);
        const auto price_part = fp.fmul(price_weight, price);
        c.set_f(0, price_part);
        const auto base = fp.fsub(base_encoded, bias);
        c.set_f(1, base);
        const auto quote = fp.multiply_add(ineligible_region ? 0x10088b24 : 0x10088a48, base_weight, base, price_part);
        c.set_f(0, quote);
        c.put64(n.frame.at(0x54), fp.fctiwz(0, quote));
        n.price = c.u32(n.frame.at(0x58));
        return;
    }
    c.set(4, 0x43300000);
    c.set(0, extended(mood) ^ 0x80000000u);
    c.put32(n.frame.at(0x48), c.r(0));
    c.set(0, std::uint32_t(n.price) ^ 0x80000000u);
    c.set(3, std::uint32_t(n.manager_or_base) ^ 0x80000000u);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    c.put32(n.frame.at(0x44), 0x43300000);
    const auto slope = c.f64(n.constants + 0x90);
    c.set_f(4, slope);
    const auto mood_encoded = c.f64(n.frame.at(0x44));
    c.set_f(0, mood_encoded);
    const auto intercept = c.f64(n.constants + 0x3a8);
    c.set_f(1, intercept);
    const auto signed_mood = fp.fsub(mood_encoded, bias);
    c.set_f(2, signed_mood);
    c.put32(n.frame.at(0x58), c.r(0));
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(0, one);
    c.put32(n.frame.at(0x54), 0x43300000);
    const auto weight = fp.multiply_add(ineligible_region ? 0x10088b70 : 0x10088a94, slope, signed_mood, intercept);
    c.set_f(4, weight);
    const auto price_encoded = c.f64(n.frame.at(0x54));
    c.set_f(1, price_encoded);
    c.put32(n.frame.at(0x50), c.r(3));
    const auto price = fp.fsub(price_encoded, bias);
    c.set_f(1, price);
    c.put32(n.frame.at(0x4c), 0x43300000);
    const auto complement = fp.fsub(one, weight);
    c.set_f(0, complement);
    const auto base_encoded = c.f64(n.frame.at(0x4c));
    c.set_f(2, base_encoded);
    const auto price_part = fp.fmul(price, complement);
    c.set_f(0, price_part);
    const auto base = fp.fsub(base_encoded, bias);
    c.set_f(1, base);
    const auto quote = fp.multiply_add(ineligible_region ? 0x10088b94 : 0x10088ab8, base, weight, price_part);
    c.set_f(0, quote);
    c.put64(n.frame.at(0x5c), fp.fctiwz(0, quote));
    n.price = c.u32(n.frame.at(0x60));
}

void ordinary_quote(Context& n) {
    auto& c = n.c;
    if (!nonzero_logical(c, n.tenure_or_shortlist)) return unlisted_price(n, false);
    const auto candidate = c.i32(n.tenure_or_shortlist + 0x10);
    c.set(0, std::uint32_t(candidate));
    if (compare(c, candidate, -1) != 0) return unlisted_price(n, false);
    young_player_price(n, 22, 0x10087f5c, 0x10088048);
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    if (compare(c, reputation, 6750) <= 0) {
        const auto competition = c.u32(n.employer + 0x30);
        c.set(3, competition);
        if (!nonzero_logical(c, competition)) return ordinary_lower_quote(n, competition);
        const auto standing = c.i16(competition + 0x18);
        c.set(0, extended(standing));
        if (compare(c, standing, 15) <= 0) return ordinary_lower_quote(n, competition);
    }
    ordinary_premium_quote(n);
}
}

void regional_quote(Context& n) {
    if (!eligible_region(n)) return unlisted_price(n, true);
    if (!shortlisted(n)) return ordinary_quote(n);
    young_player_price(n);
    auto& c = n.c;
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    if (compare(c, reputation, 6750) <= 0) {
        const auto competition = c.u32(n.employer + 0x30);
        c.set(3, competition);
        if (!nonzero_logical(c, competition)) return lower_reputation_role_quote(n, competition);
        const auto standing = c.i16(competition + 0x18);
        c.set(0, extended(standing));
        if (compare(c, standing, 15) <= 0) return lower_reputation_role_quote(n, competition);
    }
    premium_role_quote(n);
}
}

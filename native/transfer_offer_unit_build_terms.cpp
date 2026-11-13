// TRANSFER_OFFER.CPP, building an offer (0x104126d0): the terms. When the seller asks well over what the
// club offers, part of the fee becomes instalments (+0x40, over the months at +0x44), an appearance extra
// (+0x48, due after the appearances at +0x4c) or a sell-on share (+0x50); a club short of the seller's
// price revises its fee toward its ceiling.
#include "transfer_offer_unit_build_detail.hpp"

namespace cm::aot::transfer_offer_unit::build_detail {
namespace {
constexpr std::uint8_t lt = 8, gt = 4, eq = 2;
}

// ---- pieces ----

void OfferBuild::months_from(std::uint32_t back, std::int32_t modulus, std::int32_t base) {
    const auto drawn = sample(back);
    c.put8(offer + field::instalment_months, std::uint32_t(wrap_mul(drawn % modulus + base, 10)));
}

// fee * share / (100 - months / 1.5), truncated.
std::int32_t OfferBuild::instalments_for(std::int32_t share) {
    const auto spread = fpu.fdiv(to_double(months()), constant(k::month_scale));
    const auto whole = to_double(fee());
    const auto denominator = fpu.fsub(constant(k::hundred), spread);
    const auto product = fpu.fmul(whole, to_double(share));
    return truncate(fpu.fdiv(product, denominator));
}

// fee * percent / 100, truncated.
std::int32_t OfferBuild::percent_of_fee(std::int32_t percent) {
    const auto whole = to_double(fee());
    const auto product = fpu.fmul(whole, to_double(percent));
    return truncate(fpu.fdiv(product, constant(k::hundred)));
}

std::int8_t OfferBuild::sell_on_share(std::uint32_t back, bool five_steps) {
    const auto drawn = sample(back);
    return std::int8_t(five_steps ? (drawn % 5 + 1) * 5 : (drawn % 6) * 5);
}

// The seller's two valuations differ by more than three quarters of his price.
bool OfferBuild::gap_is_large() {
    const auto share = fpu.fmul(constant(k::three_quarters), to_double(asking));
    const auto apart = to_double(gap);
    fpu.fcmp(0, apart, share);
    return c.cr(0) & gt;
}

// The instalments at most the price plus 30,000 a month, and at most their share of the gap
// (instalments * gap / the price, the price taken as at least 10,000).
void OfferBuild::cap_instalments() {
    const auto divisor = asking > 10000 ? asking : 10000;
    const auto product = wrap_mul(c.i32(offer + field::instalments), gap);
    const auto most = wrap_add(asking, wrap_mul(months(), 30000));
    const auto share = divide(product, divisor);
    // The months' bound always wins: the months are 10 to 50 (draws of 0..0x1fffe, the table's base being a
    // halfword), so `most` is over the price plus 300,000, while the quotient is at most 2^31 / divisor:
    // 214,748 for a price up to 10,000, under the price itself above it.
    c.put32(offer + field::instalments, std::uint32_t(most < share ? most : share));
}

void OfferBuild::split_fee(const Split& split, SellOn rule) {
    months_from(split.months_back, split.months_modulus, split.months_base);
    const auto drawn = sample(split.share_back);
    c.put32(offer + field::instalments, std::uint32_t(instalments_for(drawn % 11 + split.share_add)));
    const auto first = sell_on_share(split.sell_on_a, split.five_steps);
    keep_signed(13, first);
    const auto second = sell_on_share(split.sell_on_b, split.five_steps);
    if (rule != SellOn::lower && gap_is_large()) {
        c.put8(offer + field::sell_on_percent, std::uint8_t(first > second ? first : second));
        if (rule == SellOn::higher_and_capped_when_large) cap_instalments();
        return;
    }
    c.put8(offer + field::sell_on_percent, std::uint8_t(first < second ? first : second));
}

// ---- the seller asks much more (0x1041389c) ----

// Only for a player row that is keen or has a valuation gap; false gives up. A player reputed for a
// nation of reputation over 5000 (role flag clear) gets an appearance extra of the larger of three
// quarters of the fee and 0.85 of the gap.
bool OfferBuild::negotiate() {
    if (!wants_player()) return false;  // never: only player rows get here (OfferBuild::against_budget)
    if (!keen() && gap == 0) return false;
    terms_set = true;
    const auto nation_level = nation_reputation();
    if (nation_level <= 5000 || c.u8(player + person::role_flag) != 0) {
        split_fee({0x10413f34, 3, 3, 0x10413f94, 5, 0x10414068, 0x104140c8, true}, SellOn::higher_when_large);
    } else if (reputed_for_nation(nation_level, 0x1041395c)) {
        const auto of_fee = fpu.fmul(constant(k::three_quarters), to_double(fee()));
        const auto of_gap = fpu.fmul(constant(k::gap_share), to_double(gap));
        fpu.fcmp(0, of_fee, of_gap);
        c.put32(offer + field::extra, std::uint32_t(truncate((c.cr(0) & gt) ? of_fee : of_gap)));
        c.put8(offer + field::extra_count, 10);
        split_fee({0x104139f0, 3, 3, 0x10413a50, 5, 0x10413b24, 0x10413b7c, false}, SellOn::higher_and_capped_when_large);
    } else {
        split_fee({0x10413c90, 5, 1, 0x10413cf4, 10, 0x10413dc8, 0x10413e20, false}, SellOn::higher_and_capped_when_large);
    }
    return true;
}

// ---- the seller asks a little more (0x1041417c) ----

// A fee of 100,000 or more still 1.2 times short of the price: a young reputed player (under 25-28, a
// draw) gets an appearance extra, then the random terms.
void OfferBuild::appearance_extra() {
    if (fee() < 100000) return;
    const auto asked = to_double(asking);
    fpu.fcmp(0, more_than_fee, asked);
    if (!(c.cr(0) & lt)) return;
    if (!wants_player()) return;  // never: only player rows get here (OfferBuild::against_budget)
    const auto nation_at = c.u32(player + person::nation);
    if (nation_at == 0) return;
    const std::int32_t nation_level = c.i16(nation_at + nation::reputation);
    if (nation_level > 5000 && c.u8(player + person::role_flag) < 5 && reputed_for_nation(nation_level, 0x1041423c)) {
        const auto drawn = sample(0x10414264);
        if (age() < drawn % 4 + 25) {
            terms_set = true;
            if (c.u8(player + person::role_flag) == 0 && sample(0x104142d4) % 5 == 0) {
                const auto percent = sample(0x1041432c) % 6 + 5;
                c.put32(offer + field::extra, std::uint32_t(percent_of_fee(percent)));
                c.put8(offer + field::extra_count, 1);
            } else {
                const auto percent = sample(0x104143e0) % 11 + 5;
                c.put32(offer + field::extra, std::uint32_t(percent_of_fee(percent)));
                c.put8(offer + field::extra_count, 10);
            }
        }
    }
    random_terms();
}

// Draws below an eighth of the fee decide instalments (category 5, half the time), a sell-on share (the
// lower of two; outside category 5 only for a player under 28 less a draw) and, when nothing was chosen,
// instalments again when a draw below the row's weight beats one below 0.67 of its level.
// Reached only from appearance_extra, past its player-row test: `row != 0` holds at every test below.
void OfferBuild::random_terms() {
    auto against = draw_below(fee() / 8, {0x104144ac, 0x10414574});
    if (c.i32(offer + field::extra) < against && row != 0 && category() == 5 && sample(0x10414618) % 2 != 0) {
        terms_set = true;
        months_from(0x10414668, 5, 1);
        const auto drawn = sample(0x104146cc);
        c.put32(offer + field::instalments, std::uint32_t(instalments_for(drawn % 11 + 5)));
    }
    against = draw_below(fee() / 8, {0x104147b8, 0x10414880});
    const auto young = [&] {
        const auto drawn = sample(0x1041492c);
        return age() < 28 - drawn % 20;
    };
    if (wrap_add(c.i32(offer + field::extra), c.i32(offer + field::instalments)) < against && row != 0 &&
        (category() == 5 || young())) {
        terms_set = true;
        const auto first = sell_on_share(0x104149a0, true);
        keep_signed(13, first);
        const auto second = sell_on_share(0x10414a00, true);
        c.put8(offer + field::sell_on_percent, std::uint8_t(first < second ? first : second));
    }
    if (c.i32(offer + field::extra) != 0 || c.i32(offer + field::instalments) != 0 ||
        c.i8(offer + field::sell_on_percent) != 0 || row == 0)
        return;
    const auto by_weight = draw_below(c.i32(row + wanted::weight), {0x10414ab8, 0x10414b84}, false);
    const auto level_part = truncate(fpu.fmul(constant(k::level_share), to_double(c.i32(row + wanted::level))));
    if (by_weight <= draw_below(level_part, {0x10414c54, 0x10414d1c})) return;
    terms_set = true;
    months_from(0x10414db0, 5, 1);
    const auto drawn = sample(0x10414e14);
    c.put32(offer + field::instalments, std::uint32_t(instalments_for(drawn % 11 + 5)));
}

// ---- revising the fee (0x10414fb8) ----

// The ceiling at or under three quarters of the fee: a keen player row with no terms yet makes a counter
// offer, the rest goes on. A ceiling at or over the fee only raises the fee to 1.2 times an earlier total
// (giving up when that passes the ceiling). Under the fee, a fee under the budget may be stretched toward
// the budget by the row's weight against its level; the fee becomes the ceiling.
// Only player rows get here: outside them price, fee and ceiling are 0, so neither 0 > 0 nor 0 < 0.8 x 0 sends
// the fee here (0x10414f1c, 0x10414f60). And a fee with terms set is the budget itself (0x10413818,
// 0x104137fc), so `current < budget` fails before the terms are tested.
std::uint32_t OfferBuild::revise(std::int32_t prior, std::int32_t current) {
    {
        const auto share = fpu.fmul(constant(k::three_quarters), to_double(current));
        const auto most = to_double(ceiling);
        fpu.fcmp(0, most, share);
        if ((c.cr(0) & (lt | eq)) && !terms_set && wants_player()) return counter_offer(current);
    }
    if (ceiling >= current) {
        const auto earlier = fpu.fmul(constant(k::more), to_double(prior));
        const auto now = to_double(current);
        fpu.fcmp(0, now, earlier);
        if (c.cr(0) & lt) {
            c.put32(offer + field::fee, std::uint32_t(truncate(earlier)));
            if (fee() > ceiling) {
                free_deal(0x10415a24);
                return 0;
            }
        }
        return check_funds();
    }
    if (current < budget && row != 0 && c.i32(row + wanted::level) != 0 && !terms_set &&
        c.u8(row + wanted::kind) == wanted::player)
        stretch_to_budget();
    c.put32(offer + field::fee, std::uint32_t(ceiling));
    return check_funds();
}

// Under two fifths of the fee, or a row not keen: the club gives up (its deal freed). Otherwise it splits
// the fee as in the negotiation, always taking the lower sell-on share, and still answers 0.
std::uint32_t OfferBuild::counter_offer(std::int32_t current) {
    const auto share = fpu.fmul(constant(k::two_fifths), to_double(current));
    const auto most = to_double(ceiling);
    fpu.fcmp(0, most, share);
    if (!(c.cr(0) & (gt | eq)) || !keen()) {
        free_deal(0x10415758);
        return 0;
    }
    const auto nation_level = nation_reputation();
    if (nation_level <= 5000 || c.u8(player + person::role_flag) != 0) {
        split_fee({0x1041555c, 3, 3, 0x104155bc, 5, 0x10415690, 0x104156f0, true}, SellOn::lower);
    } else if (reputed_for_nation(nation_level, 0x104150f4)) {
        c.put32(offer + field::extra, std::uint32_t(truncate(fpu.fmul(constant(k::three_quarters), to_double(fee())))));
        c.put8(offer + field::extra_count, 10);
        split_fee({0x1041515c, 5, 3, 0x104151c0, 5, 0x10415294, 0x104152ec, false}, SellOn::lower);
    } else {
        split_fee({0x1041535c, 5, 1, 0x104153c0, 10, 0x10415494, 0x104154ec, false}, SellOn::lower);
    }
    return 0;
}

// The row's weight over its level (single precision, kept within 0.05..0.75) decides: on an odd draw the
// rest of the budget goes into instalments over 10-50 months (at most 30,000 a month), on an even one half
// that share of it raises the ceiling; the ceiling stays at most the price.
void OfferBuild::stretch_to_budget() {
    const auto level = c.i32(row + wanted::level);
    const auto weight = to_single(c.i32(row + wanted::weight));
    auto ratio = fpu.fdivs(weight, to_single(level));
    fpu.fcmp(0, ratio, constant(k::ratio_floor));
    if (c.cr(0) & lt) ratio = single_constant(kf::ratio_floor);
    fpu.fcmp(0, ratio, constant(k::three_quarters));
    if (c.cr(0) & gt) ratio = single_constant(kf::ratio_ceiling);
    keep_f31(ratio);
    if (sample(0x1041582c) % 2 != 0) {
        const auto ten = single_constant(kf::ten);
        const auto tens = wrap_mul(std::int8_t(truncate(fpu.fdivs(fpu.fdivs(ten, ratio), ten))), 10);
        c.put8(offer + field::instalment_months, std::uint32_t(tens < 20 ? 20 : tens > 100 ? 100 : tens));
        // At least 20 after the clamp above, so the `< 10` arm below (0x104158c8) never takes.
        const auto rounded = months() / 10 * 10;
        c.put8(offer + field::instalment_months, std::uint32_t(rounded < 10 ? 10 : rounded > 50 ? 50 : rounded));
        const auto share = fpu.fmuls(ratio, to_single(wrap_sub(budget, fee())));
        const auto most = to_single(wrap_mul(months(), 30000));
        fpu.fcmp(0, share, most);
        const auto chosen = (c.cr(0) & lt) ? share : to_single(wrap_mul(months(), 30000));
        c.put32(offer + field::instalments, std::uint32_t(truncate(chosen)));
    } else {
        const auto share = fpu.fmuls(ratio, to_single(wrap_sub(budget, fee())));
        set_ceiling(wrap_add(ceiling, truncate(fpu.fmuls(share, single_constant(kf::half)))));
    }
    if (ceiling > asking) set_ceiling(asking);
}
}

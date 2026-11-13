// TRANSFER_OFFER.CPP: building a club's offer for a player (0x104126d0), the price and the verdict.
// The terms (instalments, the appearance extra, the sell-on share) are in transfer_offer_unit_build_terms.cpp.
#include "transfer_offer_unit_build_detail.hpp"

namespace cm::aot::transfer_offer_unit {
namespace build_detail {
namespace {
constexpr std::uint8_t lt = 8, gt = 4, eq = 2;
}

// ---- shared pieces ----

std::uint32_t OfferBuild::contract_of(std::uint32_t person_record, std::uint32_t back) {
    const auto found = invoke(c, call::contract_of, back, {c.u32(c.toc(cell::contracts)), person_record});
    c.put32(c.toc(cell::last_contract), found);
    return found;
}

std::uint32_t OfferBuild::contracted_club(std::uint32_t back) {
    keep(13, 0);
    const auto found = contract_of(player, back);
    if (found == 0 || contract_kind(c.u8(found + contract::kind)) == 5) return 0;
    keep(13, 1);
    return c.u32(c.toc(toc::clubs)) + c.u32(c.u32(c.toc(cell::last_contract)) + contract::club) * club_size;
}

void OfferBuild::free_deal(std::uint32_t back) {
    if (deal == 0) return;
    invoke(c, callee::free_object, back, {c.u32(deal)});
    c.put32(deal, 0);
}

std::int32_t OfferBuild::draw_below(std::int32_t bound, const BelowSite& site, bool in_r13) {
    keep_signed(14, bound);
    const auto result = [&](std::int32_t value) {
        if (in_r13) keep_signed(13, value);
        return value;
    };
    if (bound == 0) return result(0);
    const auto drawn = sample(site.draw);
    if (bound >= -0xffff && bound <= 0xffff) {
        const auto rest = std::int32_t(std::uint32_t(drawn) - std::uint32_t(wrap_mul(divide(drawn, bound), bound)));
        return result(bound > 0 ? rest : std::int32_t(0u - std::uint32_t(rest)));
    }
    // A wide bound: the sample's place in 0xffff scaled by the bound, plus a tenth of a draw (0x10415e50)
    // below a tenth of the bound's share of 0xffff, kept below the bound.
    const auto scale = fpu.fdiv(to_double(bound), constant(k::sample_range));
    const auto tenth_bound = wrap_mul(bound, 10) / 0xffff;
    const auto place = to_double(drawn % 0xffff);
    const auto part = result(truncate(fpu.fmul(scale, place)));
    const auto tenth = std::int32_t(invoke_unit(c, call::draw_below, site.call, {std::uint32_t(tenth_bound)}));
    const auto sum = wrap_add(part, wrap_add(tenth, 5) / 10);
    const auto top = wrap_add(bound, -1);
    return result(sum > top ? top : sum);
}

bool OfferBuild::reputed_for_nation(std::int32_t nation_reputation, std::uint32_t back) {
    const auto own = to_double(c.i16(c.u32(player + person::playing) + playing::reputation));
    const auto scaled = fpu.fdiv(own, constant(k::reputation_ratio));
    const auto theirs = to_double(nation_reputation);
    fpu.fcmp(0, scaled, theirs);
    if (c.cr(0) & gt) return true;
    const auto birth = c.u32(player + person::birth_club);
    return birth != 0 && std::int8_t(invoke(c, call::foreign_rule, back, {birth})) != 0;
}

// ---- the start ----

// A draw that marks the offer (+0x35): a player of world reputation above 4000-6999 (a draw), from a club of
// the buyer's league, and then one draw in `one_in`; only for rows of category 0-3.
void OfferBuild::mark_chance(std::uint32_t first_draw, std::uint32_t contract_a, std::uint32_t contract_b,
                             std::uint32_t second_draw, std::int32_t one_in) {
    bool marked = false;
    if (c.u32(player + person::playing) != 0 && row != 0 && category() < 4) {
        const auto drawn = sample(first_draw);
        if (c.i16(c.u32(player + person::playing) + playing::world_reputation) > drawn % 3000 + 4000 &&
            club_of(contract_of(player, contract_a)) != 0) {
            // The club is looked up again; a missing one reads its league from address 0x30.
            const auto league = c.u32(club_of(contract_of(player, contract_b)) + club::league);
            if (league == c.u32(club + club::league) && sample(second_draw) % one_in == 0) marked = true;
        }
    }
    c.put8(offer + offer_more::chance, marked ? 1 : 0);
}

// The offer's parties and empty terms. The two nibbles at +0x38 and +0x3c are cleared in place: their low
// halves keep what the record held (often the caller's stack).
void OfferBuild::blank_terms(std::uint8_t fixed) {
    c.put32(offer + field::clubs, 0);
    c.put32(offer + field::exchange, 0);
    c.put32(offer + field::buyer, c.u32(club + club::id));
    c.put32(offer + field::player, c.u32(player + person::id));
    player_contract = invoke(c, call::contract_of, 0x10412954, {c.u32(c.toc(cell::contracts)), player});
    keep(21, player_contract);
    c.put8(offer + offer_more::fixed, fixed);
    c.put8(offer + field::kind, row ? c.u8(row + wanted::kind) : wanted::player);
    c.put8(offer + offer_more::byte_24, 0);
    c.put32(offer + field::instalments, 0);
    c.put8(offer + field::instalment_months, 10);
    c.put32(offer + field::extra, 0);
    c.put8(offer + field::extra_count, 10);
    c.put8(offer + field::sell_on_percent, 0);
    c.put8(offer + field::months, 0);
    c.put32(offer + offer_more::link_b, 0xffffffff);
    c.put32(offer + offer_more::link_a, 0xffffffff);
    c.put8(offer + offer_more::byte_34, 0);
    c.put8(offer + field::status, 0);
    c.put8(offer + offer_more::byte_36, 0);
    c.put8(offer + offer_more::clause, c.u8(offer + offer_more::clause) & 0x0f);
    c.put8(offer + offer_more::handling, c.u8(offer + offer_more::handling) & 0x0f);
    c.put32(offer + field::fee, 0);
    if (deal) c.put32(deal, 0);
}

// ---- a human club ----

// A human club repeats its earlier offer for the player (0x103f5d20) with fresh flags and only the players
// in exchange it still holds; without one (or with an earlier one at +0x24, or a fixed offer) it bids the
// player's value, at most its balance.
void OfferBuild::repeat_human_offer() {
    const auto earlier = invoke(c, call::earlier_offer, 0x10412a18, {transfers(), player, club, 1});
    keep(13, earlier);
    if (earlier == 0 || c.i8(earlier + offer_more::byte_24) != 0 || c.i8(offer + offer_more::fixed) != 0) {
        c.put32(offer + field::fee, c.u32(player + person::value));
        const auto books = invoke(c, call::finances_of, 0x10412ed8, {c.u32(c.toc(cell::finances)), club});
        invoke(c, call::balance, 0x10412ef0, {books, player, 0, 0, 1, 0});
        const auto low_word = std::int32_t(c.r(4));
        if (fee() > low_word) c.put32(offer + field::fee, std::uint32_t(low_word));
        return;
    }
    struct Copy { unsigned size; std::uint32_t at; };
    static constexpr Copy copied[] = {{4, 0x00}, {4, 0x04}, {4, 0x08}, {4, 0x0c}, {1, 0x10}, {1, 0x11}, {2, 0x16},
                                      {2, 0x14}, {1, 0x18}, {1, 0x1c}, {4, 0x20}, {1, 0x24}, {4, 0x28}, {4, 0x2c},
                                      {4, 0x30}, {1, 0x34}, {1, 0x35}, {1, 0x36}, {1, 0x37}, {1, 0x38}, {4, 0x3c},
                                      {4, 0x40}, {1, 0x44}, {4, 0x48}, {1, 0x4c}, {1, 0x50}, {1, 0x51}};
    for (const auto& [size, at] : copied) {
        if (size == 4) c.put32(offer + at, c.u32(earlier + at));
        else if (size == 2) c.put16(offer + at, c.u16(earlier + at));
        else c.put8(offer + at, c.u8(earlier + at));
    }
    mark_chance(0x10412b54, 0x10412bac, 0x10412bec, 0x10412c48, 3);
    c.put8(offer + field::status, 0);
    c.put8(offer + offer_more::byte_34, 0);
    c.put8(offer + offer_more::byte_36, 0);
    c.put8(offer + offer_more::clause, c.u8(offer + offer_more::clause) & 0x0f);
    c.put8(offer + offer_more::handling, c.u8(offer + offer_more::handling) & 0x0f);
    c.put32(offer + offer_more::link_b, 0xffffffff);
    c.put32(offer + offer_more::link_a, 0xffffffff);
    c.put8(offer + offer_more::fixed, 0);
    c.put32(offer + field::clubs, 0);
    if (c.u32(offer + field::exchange) == 0) return;

    // The players in exchange: a fresh list of those still at the buyer, with no contract pending, not on a
    // kind-5 contract, of role 11, and either free of transfer business (0x103eaea0, 0x103eb020) or with
    // exactly one piece of it, an offer for this player.
    const auto business = frame.at(0x5c);
    invoke(c, callee::list_construct, 0x10412ce4, {business});
    const auto list = invoke(c, callee::allocate_object, 0x10412cec, {0x10});
    keep(14, list);
    // A null list (the heap exhausted) is stored as it is; the original then reads its count at 0x10412e94
    // (or appends to it at 0x10412e7c) and faults on the null page.
    if (list) invoke(c, callee::list_construct, 0x10412cf8, {list});
    c.put32(offer + field::exchange, list);
    rewind(c, c.u32(earlier + field::exchange));
    for (;;) {
        const auto row_at = invoke(c, callee::list_next, 0x10412e88, {c.u32(earlier + field::exchange)});
        keep(18, row_at);
        if (row_at == 0) break;
        const auto person_at = c.u32(c.toc(toc::people)) + c.u32(row_at + exchange_row::person) * person_size;
        keep(14, person_at);
        if (club_of(contract_of(person_at, 0x10412d28)) != club) continue;
        if (invoke(c, call::contract_pending, 0x10412d68, {c.u32(c.toc(cell::contracts)), person_at}) != 0) continue;
        const auto held = contract_of(person_at, 0x10412d7c);
        if (held != 0 && contract_kind(c.u8(held + contract::kind)) == 5) continue;
        if (c.u8(person_at + person::role) != 11) continue;
        if (invoke(c, call::transfer_a, 0x10412dd8, {transfers(), person_at, 0, 0}) != 0 ||
            invoke(c, call::transfer_b, 0x10412df0, {transfers(), person_at, business}) != 0) {
            bool ours = false;
            keep(14, 0);
            if (c.i16(business) == 1) {
                rewind(c, business);
                const auto first = invoke(c, callee::list_next, 0x10412e18, {business});
                const auto entry_at = invoke(c, call::transfer_row, 0x10412e28, {transfers(), c.u32(first)});
                if (c.u32(player + person::id) == c.u32(c.u32(entry_at) + 0xc)) {
                    ours = true;
                    keep(14, 1);
                }
            }
            if (!ours) continue;
        }
        // The row is copied with its status cleared. Its last byte (sp+0x53) is never written: not by this
        // routine (it writes 0x4c-0x52 here, 0x44-0x48 and 0x5c-0x68 elsewhere) and not by its callees, whose
        // frames lie below sp and which write in this frame only the linkage words (sp+4, sp+8) and what they
        // are passed (the list at sp+0x5c: 0x10738c00, 0x103eb020). It is the stack as it was before the entry,
        // and is left so.
        const auto copy = frame.at(0x4c);
        c.put32(copy + exchange_row::person, c.u32(row_at + exchange_row::person));
        c.put8(copy + 4, c.u8(row_at + 4));
        c.put8(copy + 5, c.u8(row_at + 5));
        c.put8(copy + 6, c.u8(row_at + 6));
        c.put8(copy + exchange_row::status, 0);
        invoke(c, callee::list_append, 0x10412e80, {c.u32(offer + field::exchange), copy, 8});
    }
    const auto kept = c.u32(offer + field::exchange);
    if (c.i16(kept) == 0) {
        invoke(c, callee::list_destroy, 0x10412ea8, {kept, 1});
        c.put32(offer + field::exchange, 0);
    }
    invoke(c, callee::list_destroy, 0x10412ebc, {business, 0xffffffff});
}

// ---- a computer club: the price ----

// The club will not bid (0x1036fbf0, or 0x103faff0 for the player): with a deal word it signs the player
// without a fee instead (0x10080d30 fills a new contract record).
std::uint32_t OfferBuild::refuse_or_sign_free() {
    if (deal == 0) return 0;
    const auto record = invoke(c, callee::allocate_object, 0x10412fc0, {100});
    keep(13, record);
    if (record) invoke(c, call::deal_construct, 0x10412fcc, {record});
    c.put32(deal, record);
    c.set_f(1, constant(k::one));
    c.set(10, 2);
    const auto signed_up = invoke(c, call::sign_free, 0x10412ff0,
                                  {c.u32(c.toc(cell::contracts)), player, club, c.u32(deal), 0xffffffff});
    if (std::int8_t(signed_up) == 0) {
        invoke(c, callee::free_object, 0x10413000, {c.u32(deal)});
        return 0;
    }
    c.put8(offer + offer_more::fixed, 1);
    return finish();
}

// What the seller asks: nothing outside a player row; a human seller's price (0x10089a40), with a floor
// when he keeps the player (0x100d3040: the larger of a third and 1,500,000 less); otherwise the two
// valuations of 0x10085860, the gap between them kept. A release clause below the price caps it.
void OfferBuild::ask_price() {
    // For a player row the player record is never null here: 0x10413060 returned 0 for that (no call between,
    // the same word read).
    if (!wants_player() || c.u32(player + person::playing) == 0) {
        set_asking(0);
    } else if (seller != 0 && std::int8_t(invoke(c, call::club_human, 0x104130bc, {seller, 0, 1, 0, 0})) != 0) {
        set_asking(std::int32_t(invoke(c, call::human_asking, 0x104130d8, {c.u32(c.toc(cell::contracts)), seller, club, player})));
        if (std::int8_t(invoke(c, call::keeps_player, 0x104130ec, {player, 0})) != 0) {
            const auto less = wrap_add(asking, -1500000), third = asking / 3;
            human_floor = third > less ? third : less;
        }
    } else {
        set_asking(std::int32_t(invoke(c, call::value_to, 0x10413144,
                                       {c.u32(c.toc(cell::contracts)), seller, club, player, 0, 1, 1})));
        const auto low = std::int32_t(invoke(c, call::value_to, 0x1041316c,
                                             {c.u32(c.toc(cell::contracts)), seller, club, player, 0, 1, 0}));
        gap = wrap_sub(asking, low);
        keep_signed(14, gap);
    }
    if (player_contract != 0 && std::int8_t(invoke(c, call::has_release_clause, 0x10413180, {player_contract})) != 0) {
        const auto release = c.i32(player_contract + contract::release_fee);
        if (asking > release) {
            c.put8(offer + offer_more::clause, (c.u8(offer + offer_more::clause) & 0x0f) | 0x10);
            set_asking(release);
        }
    }
}

// The buyer's ceiling (0x10361490, player rows only). A seller whose finances bar him (+0x165 == 1) holds
// it to what he would take: his valuations (0x10084be0) when human, the player's value otherwise.
void OfferBuild::judge_ceiling() {
    // 0x104131c4 tests the row again (always set here): its 0x10084be0 arm (0x104131f8) cannot run.
    set_ceiling(wants_player() ? std::int32_t(invoke(c, call::ceiling, 0x104131f0,
                                                     {c.u32(transfers() + 0x988), club, player, row, frame.at(0x40), 0xffffffff, 1}))
                               : 0);
    if (!wants_player() || seller == 0) return;
    if (invoke(c, call::finances_of, 0x10413244, {c.u32(c.toc(cell::finances)), seller}) == 0) return;
    if (c.u8(invoke(c, call::finances_of, 0x1041325c, {c.u32(c.toc(cell::finances)), seller}) + finances::barred) != 1)
        return;
    if (std::int8_t(invoke(c, call::club_human, 0x10413280, {seller, 0, 1, 0, 0})) != 0) {
        const auto valuation = [&](std::uint32_t back) {
            return std::int32_t(invoke(c, call::valuation, back, {c.u32(c.toc(cell::contracts)), player, seller, 0, 0xffffffff, 0}));
        };
        const auto first = valuation(0x104132a4);
        const auto lower = ceiling < first ? ceiling : valuation(0x104132d0);
        if (asking < lower) {
            set_ceiling(asking);
        } else {
            const auto third = valuation(0x104132fc);
            set_ceiling(ceiling < third ? ceiling : valuation(0x10413328));
        }
    } else {
        const auto value = c.i32(player + person::value);
        const auto lower = ceiling < value ? ceiling : value;
        set_ceiling(asking < lower ? asking : (ceiling < value ? ceiling : value));
    }
    seller_limited = true;
}

// One club's finance state (0x100f55b0 of its 0x10103510 record, -2..2) scales the factor; the seller's
// scale runs the other way.
double OfferBuild::finance_factor(double factor, std::uint32_t of, std::uint32_t back_finances,
                                  std::uint32_t back_state, bool seller_side) {
    static constexpr std::uint32_t by_state[] = {k::factor_m2, k::factor_m1, k::factor_0, k::factor_1, k::factor_2};
    const auto books = invoke(c, call::finances_of, back_finances, {c.u32(c.toc(cell::finances)), of});
    const auto state = std::int8_t(invoke(c, call::finance_state, back_state, {books}));
    if (state < -2 || state > 2) return factor;
    const auto at = seller_side ? by_state[2 - state] : by_state[state + 2];
    const auto scaled = fpu.fmul(factor, constant(at));
    keep_f31(scaled);
    return scaled;
}

// A price far over the estimate (a draw around 1,000,000 plus a tenth of 0x10415e50(0x98), at most 999,999)
// is spread over months when the clubs' finances weigh enough: the fee, the ceiling and the budget are
// divided by the tier's divisor.
void OfferBuild::scale_by_finances() {
    const auto drawn = sample(0x10413398);
    const auto noise = truncate(fpu.fmul(constant(k::noise), to_double(drawn % 0xffff)));
    keep_signed(13, noise);
    const auto extra = std::int32_t(invoke_unit(c, call::draw_below, 0x10413414, {0x98}));
    auto estimate = wrap_add(noise, wrap_add(extra, 5) / 10);
    // Never over 999,999 with the game's constants: the place is at most 65534, 15.259021896696423 x 65534
    // truncates to 999,984, and the draw below 0x98 is at most 151, a tenth of it (rounded) 15.
    if (estimate > 999999) estimate = 999999;
    keep_signed(13, estimate);
    if (fee() <= estimate) return;
    auto factor = constant(k::base_factor);
    const auto nation_number = c.u32(c.u32(club + club::nation) + nation::number);
    if (nation_number == c.u32(c.toc(cell::nation_a)) || nation_number == c.u32(c.toc(cell::nation_b)))
        factor = constant(k::three_quarters);
    keep_f31(factor);
    factor = finance_factor(factor, club, 0x10413494, 0x10413498, false);
    factor = finance_factor(factor, seller, 0x1041350c, 0x10413510, true);
    struct Tier { std::uint32_t above; std::uint8_t months; std::uint32_t divisor; };
    static constexpr Tier tiers[] = {{k::tier_24, 24, k::divisor_24}, {k::tier_18, 18, k::divisor_18},
                                     {k::tier_12, 12, k::divisor_12}, {k::one, 6, k::divisor_6}};
    for (const auto& tier : tiers) {
        fpu.fcmp(0, factor, constant(tier.above));
        if (c.cr(0) & gt) return divide_prices(tier.months, tier.divisor);
    }
}

void OfferBuild::divide_prices(std::uint8_t months, std::uint32_t divisor) {
    c.put8(offer + field::months, months);
    const auto by = constant(divisor);
    const auto fee_part = fpu.fdiv(to_double(fee()), by);
    auto ceiling_part = to_double(ceiling);
    auto budget_part = to_double(budget);
    const auto new_fee = truncate(fee_part);
    ceiling_part = fpu.fdiv(ceiling_part, by);
    c.put32(offer + field::fee, std::uint32_t(new_fee));
    budget_part = fpu.fdiv(budget_part, by);
    set_ceiling(truncate(ceiling_part));
    set_budget(truncate(budget_part));
}

// Outside a player row the price is 0 (0x10413094), so is the fee (scaling 0 leaves 0), and any budget of 0
// or more settles here: the negotiation, the small gap and the revision of the fee only ever see player rows.
// The fee against the budget. Over it, a human seller's floor settles it; otherwise the fee becomes the
// budget and a seller asking much more (1.4 times, or 1.2 times with a valuation gap) is offered terms.
OfferBuild::Verdict OfferBuild::against_budget() {
    if (budget <= -1 || budget >= fee()) return Verdict::settle;
    const auto current = fee();
    if (human_floor > -1) {
        c.put32(offer + field::fee, std::uint32_t(human_floor < current ? human_floor : current));
        c.put8(offer + field::months, 0);
        if (fee() > budget) c.put32(offer + field::fee, std::uint32_t(budget));
        return Verdict::settle;
    }
    c.put32(offer + field::fee, std::uint32_t(budget));
    {
        const auto offered = fpu.fmul(constant(k::much_more), to_double(fee()));
        const auto asked = to_double(asking);
        fpu.fcmp(0, offered, asked);
        if (c.cr(0) & lt) return Verdict::negotiate;
    }
    const auto offered = fpu.fmul(constant(k::more), to_double(fee()));
    const auto asked = to_double(asking);
    fpu.fcmp(0, offered, asked);
    more_than_fee = offered;
    if (!(c.cr(0) & lt) || gap == 0) return Verdict::small_gap;
    return Verdict::negotiate;
}

// ---- the verdict ----

// A pending offer of the club for the player at stage 2 (0x103f4240) gives the earlier total (0x10416710)
// unless the ceiling came from the seller. A ceiling over the fee, or under four fifths of it, revises the
// fee; otherwise the club gives up when the fee is not 1.2 times the earlier total.
std::uint32_t OfferBuild::settle() {
    const auto pending = invoke(c, call::pending_offer, 0x10414ed8, {transfers(), player, club});
    std::int32_t prior = -1;
    if (pending != 0 && c.u8(pending + offer_more::byte_34) == 2 && !seller_limited)
        prior = std::int32_t(invoke_unit(c, call::total, 0x10414f0c, {pending, 0, 0, 0, 1}));
    const auto current = fee();
    if (ceiling > current) return revise(prior, current);
    {
        const auto share = fpu.fmul(constant(k::four_fifths), to_single(current));
        const auto most = to_double(ceiling);
        fpu.fcmp(0, most, share);
        if (!(c.cr(0) & (gt | eq))) return revise(prior, current);
    }
    const auto earlier = fpu.fmul(constant(k::more), to_double(prior));
    const auto now = to_double(current);
    fpu.fcmp(0, now, earlier);
    if (c.cr(0) & lt) {
        free_deal(0x10414fa8);
        return 0;
    }
    return check_funds();
}

// The fee against the club's balance (0x100f5770's 64-bit answer): short of it, no offer. A release clause
// under 1.05 times the fee becomes the whole fee; a plain player row lets 0x104172f0 reshape the fee.
std::uint32_t OfferBuild::check_funds() {
    const auto books = invoke(c, call::finances_of, 0x10415a44, {c.u32(c.toc(cell::finances)), club});
    invoke(c, call::balance, 0x10415a5c, {books, player, 0, 0, 1, 0});
    const auto balance = std::int64_t((std::uint64_t(c.r(3)) << 32) | c.r(4));
    if (balance < std::int64_t(fee())) return 0;
    if (player_contract != 0 && std::int8_t(invoke(c, call::has_release_clause, 0x10415a98, {player_contract})) != 0) {
        const auto margin = fpu.fmul(constant(k::clause_margin), to_double(fee()));
        const auto release = to_double(c.i32(player_contract + contract::release_fee));
        fpu.fcmp(0, margin, release);
        if (c.cr(0) & gt) {
            c.put32(offer + field::instalments, 0);
            c.put32(offer + field::extra, 0);
            c.put8(offer + field::sell_on_percent, 0);
            c.put32(offer + field::fee, c.u32(player_contract + contract::release_fee));
        }
    } else if (row == 0 || c.u8(row + wanted::kind) == wanted::player) {
        invoke_unit(c, call::reshape_fee, 0x10415b2c, {offer, player, seller, club});
    }
    return finish();
}

// The end: a fee over the budget (at least 5000 for a player with a value) gives up. A deal for a player of
// the role 0x100c7a20 accepts takes the contract's date and, when his clause cannot be paid
// (0x10380f20), the seller's own price (handling 5, else 3). The approach (0x103f8820) is made from today
// or that date; refused (other than reason 9), no offer. The handling nibble then records how the deal
// goes, and an offer with no club to buy from, or a fixed one, takes the seller's price.
std::uint32_t OfferBuild::finish() {
    if (budget != -1 && budget < 5000 && c.i32(player + person::value) > 0) set_budget(5000);
    if (budget > -1 && fee() > budget) {
        free_deal(0x10415b70);
        return 0;
    }
    const auto when = frame.at(0x44), today = c.toc(cell::today);
    c.put16(when, c.u16(today));
    c.put16(when + 2, c.u16(today + 2));
    c.put8(when + 4, c.u8(today + 4));
    if (deal != 0 && c.u32(deal) != 0 &&
        std::int8_t(invoke(c, call::role_rule, 0x10415bb8, {c.u8(player + person::role)})) != 0 && player_contract != 0) {
        set_handling(3);
        c.put16(when + 2, c.u16(player_contract + contract::date + 2));
        c.put16(when, c.u16(player_contract + contract::date));
        c.put8(when + 4, c.u8(player_contract + contract::date + 4));
        if (std::int8_t(invoke(c, call::clause_payable, 0x10415c04, {player_contract, club, 0, 0})) == 0) {
            c.put32(offer + field::fee, invoke(c, call::seller_price, 0x10415c20, {transfers(), player, club, 0}));
            set_handling(5);
        }
    }
    const auto from = contracted_club(0x10415c44);
    const auto reason = frame.at(0x38);
    const auto answer = invoke(c, call::approach, 0x10415cb8, {transfers(), club, when, player, from, 1, reason});
    keep(13, answer);
    const auto agreed = std::int8_t(answer);
    if (agreed == 0 && c.i16(reason) != 9) {
        free_deal(0x10415ce0);
        return 0;
    }
    if ((deal == 0 || c.u32(deal) == 0) &&
        std::int8_t(invoke(c, call::role_rule, 0x10415d0c, {c.u8(player + person::role)})) != 0 && agreed != 0) {
        set_handling(0);
    } else if (std::int8_t(invoke(c, call::role_rule, 0x10415d38, {c.u8(player + person::role)})) != 0) {
        if (handling() == 0) set_handling(1);
        else if (handling() == 3) set_handling(4);
    }
    if (contracted_club(0x10415d8c) == 0) c.put8(offer + offer_more::fixed, 1);
    if (c.i8(offer + offer_more::fixed) != 0 && handling() != 5)
        c.put32(offer + field::fee, invoke(c, call::seller_price, 0x10415e2c, {transfers(), player, club, 1}));
    return 1;
}

// ---- a computer club: the whole ----

std::uint32_t OfferBuild::computer_offer() {
    const auto refused = invoke(c, call::refuses, 0x10412f20,
                                {c.u32(transfers() + 0x988), row, player_contract, club, frame.at(0x3c)});
    keep(13, refused);
    const bool blocked = std::int8_t(refused) == 0 ||
                         std::int8_t(invoke(c, call::unwanted, 0x10412f38, {transfers(), player})) != 0;
    if (row != 0 && blocked) {
        // A refusal of a player under contract of kind other than 1, 3 or 5 still bids.
        bool bids = player_contract != 0 && c.u8(row + wanted::kind) == wanted::player;
        if (bids) {
            const auto kind = contract_kind(c.u8(player_contract + contract::kind));
            bids = kind != 1 && kind != 3 && kind != 5 && std::int8_t(refused) == 0;
        }
        if (!bids) return refuse_or_sign_free();
    }
    seller = club_of(contract_of(player, 0x10413020));
    keep(24, seller);
    if (wants_player() && c.u32(player + person::playing) == 0) return 0;
    ask_price();
    judge_ceiling();
    c.put32(offer + field::fee, std::uint32_t(asking));
    if (seller != 0) scale_by_finances();
    switch (against_budget()) {
        case Verdict::settle: break;
        case Verdict::negotiate:
            if (!negotiate()) return 0;
            break;
        case Verdict::small_gap: appearance_extra(); break;
    }
    return settle();
}
}

std::uint32_t build_offer(guest::Cpu& c, std::uint32_t offer, std::uint32_t player, std::uint32_t club,
                          std::uint32_t wanted, std::int32_t budget, std::uint32_t fixed, std::uint32_t deal) {
    using namespace build_detail;
    Frame frame(c, entry, 0x130, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    // The original's register image from its prologue on (see OfferBuild::keep).
    c.set({{13, c.toc(toc::pool)}, {14, 0}, {15, offer}, {16, player}, {17, club}, {18, wanted}, {19, std::uint32_t(budget)}, {20, deal}, {21, fixed},
           {22, c.toc(toc::people)}, {23, c.toc(toc::message)}, {25, c.toc(toc::doubles)}, {26, c.toc(cell::transfers)},
           {27, c.toc(cell::contracts)}, {28, c.toc(sample_table::base)}, {29, c.toc(sample_table::first)},
           {30, c.toc(sample_table::last)}, {31, c.toc(sample_table::cursor)}});
    if (player == 0 || club == 0) {
        report_check(c, frame, {0x1041275c, 0x6c, 0x54, 0x154});
        return 0;
    }
    OfferBuild build(c, frame, offer, player, club, wanted, budget, deal);
    build.mark_chance(0x104127e8, 0x10412840, 0x10412880, 0x104128dc, 20);
    build.blank_terms(std::uint8_t(fixed));
    if (std::int8_t(invoke(c, call::club_human, 0x104129fc, {club, 0, 1, 0, 0})) != 0) {
        build.repeat_human_offer();
        return build.finish();
    }
    return build.computer_offer();
}
}

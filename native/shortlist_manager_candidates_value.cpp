#include "shortlist_manager_candidates_scan.hpp"

// The candidate scan's valuation (0x1035fbb4-0x10360cfc): score an eligible
// person, price him, keep him when he beats the best so far, and the side
// effects on the way: interest registered at once for a strong first-pass
// player (approach), and now and then an unsettling approach for a player
// the club rates above its best (surprise). The registers the original
// leaves on the way (r4-r7) are left too.
namespace cm::aot::shortlist_manager::candidates {
namespace {
// The compiler's signed 64-bit compare (subfc/subfe/subfe/neg.): cr0 compares
// the neg.'s result, 0 when a >= b, with 0.
bool wide_at_least(Cpu& c, std::int64_t a, std::int64_t b) {
    const bool at_least = a >= b;
    compare(c, at_least ? 0 : 1);
    return at_least;
}
std::uint32_t high(std::int64_t value) { return std::uint32_t(std::uint64_t(value) >> 32); }
}

// The score (0x10361490 for a player row, with the row and the minimum;
// 0x101ad080 for staff) and the price (r21:r20: 0x10361490's answer, the
// person's value for staff); then the asking price and the bid, and the
// draw that may make him the pick.
void Scan::value() {
    const Person p = candidate();
    if (nonzero(c, std::int8_t(player_row))) {
        set_price(std::int32_t(program.call(0x10361490, 0x1035fbdc,
                                            {manager, club_address, person, row_address, at(local::score), best, 1})));
    } else {
        c.put32(at(local::score), program.call(0x101ad080, 0x1035fc04, {c.u32(c.toc(slot::listings)), person, club_address,
                                                                         club.manager(), row.kind()}));
        set_price(p.value());
    }
    const std::int32_t score = c.i32(at(local::score));
    if (compare(c, score, c.i32(at(local::minimum))) < 0) return;
    if (compare(c, score, std::int32_t(best - 10000)) < 0) return;
    if (nonzero(c, std::int32_t(Row::loan(row.flags())))) {
        // The relaxed pass asks 0x1036f170 about the person's own club with
        // no asking price (-1:-1), its ninth argument 1.
        c.put32(at(local::ninth_argument), 1);
        const auto verdict = program.call(0x1036f170, 0x10360154, {manager, p.club(), person, r21, r20, club_address,
                                                                    std::uint32_t(-1), std::uint32_t(-1)});
        if (compare(c, std::int8_t(verdict), 2) == 0) return;
        injury_penalty(0x10360170, 10, 200);
    } else {
        if (!negotiate()) return;
        const std::int32_t limit = c.i32(at(local::limit));
        if (compare(c, limit, -1) != 0 && compare(c, r18.value(), limit) > 0) return;
        if (nonzero(c, std::int8_t(player_row))) adjust_score();
    }
    if (nonzero(c, std::int8_t(player_row)) &&
        !nonzero(c, std::int8_t(program.call(0x103f83c0, 0x103601c0, {c.u32(c.toc(slot::transfers)), person, club_address, 0, 0}))))
        return;
    // The score x (92 + a draw below 17) / 100, in single precision.
    advance(0x103601e8);
    const std::int32_t value = draw();
    const std::int32_t scored = c.i32(at(local::score));
    const double score_value = integer_single(0xec, scored);
    const double percent = integer_single(0xf4, value % 17 + 92);
    c.set_f(1, percent);
    const double factor = fpu.frsp(fpu.fdiv(percent, k::hundred));
    const std::int32_t drawn = truncated(0xfc, fpu.fmuls(score_value, factor));
    leave(c, {{4, std::uint32_t(value)}, {5, 0x43300000}, {6, signed_word(scored)}});
    const bool better = compare(c, drawn, best.value()) > 0;
    c.put32(at(local::score), std::uint32_t(drawn));
    if (better && !evaluate()) return;
    surprise();
}

// The first pass's asking price (r18): the selling club's (0x10089a40 when
// it is human, 0x10085860 otherwise; the person's value for staff), capped
// by 0x1036fbf0's offer cap; a player's price r21:r20 is lowered to what a
// human seller with a class-1 finance record asks (0x10084be0), or to his
// value. Below the asking price, modes 5 and 4 (under 28) add up to the
// shortfall, capped at 2.5 million and (0.5 + reputation/13000) x price.
// 0x1036f170 answering 2 refuses.
bool Scan::negotiate() {
    const Person p = candidate();
    r13 = contract_club(person, 0x1035fc44);
    if (nonzero(c, std::int8_t(player_row)) && nonzero_logical(c, r13) &&
        nonzero_logical(c, program.call(0x10103510, 0x1035fc90, {c.u32(c.toc(slot::finances)), r13}))) {
        const auto record = program.call(0x10103510, 0x1035fca8, {c.u32(c.toc(slot::finances)), r13});
        if (compare(c, c.u8(record + 0x165), 1) == 0) {
            if (human(r13, 0, 0x1035fccc)) {
                const std::initializer_list<std::uint32_t> asking_arguments{c.u32(contracts_slot), person, r13, 0,
                                                                            std::uint32_t(-1), 0};
                const std::int32_t asking = std::int32_t(program.call(0x10084be0, 0x1035fcf0, asking_arguments));
                const std::uint32_t value_high = r21 ^ 0x80000000;
                const bool covered = wide_at_least(c, price(), asking);
                leave(c, {{4, std::uint32_t(!covered)}, {5, value_high}});
                // Covered: asked again (the second answer is the price).
                set_price(covered ? std::int32_t(program.call(0x10084be0, 0x1035fd34,
                                                              {c.u32(contracts_slot), person, r13, 0, std::uint32_t(-1), 0}))
                                  : r20.value());
            } else {
                const std::int32_t worth = p.value();
                const std::uint32_t value_high = r21 ^ 0x80000000;
                const std::int32_t lowered = wide_at_least(c, price(), worth) ? worth : r20.value();
                leave(c, {{4, value_high}, {5, std::uint32_t(lowered)}});
                set_price(lowered);
            }
        }
    }
    if (!nonzero_logical(c, r14)) r14 = program.call(0x10082f20, 0x1035fd88, {c.u32(contracts_slot), person});
    const bool player = nonzero(c, std::int8_t(player_row));
    r18 = 0;
    if (player && nonzero(c, std::int8_t(program.call(0x1036fbf0, 0x1035fdb0,
                                                      {manager, row_address, r14, club_address, at(local::offer_cap)}))))
        r18 = 1;
    const bool no_seller = compare_logical(c, r13) == 0;
    bid_capped = std::uint32_t(std::int32_t(std::int8_t(r18)));
    if (no_seller || nonzero(c, std::int8_t(bid_capped))) {
        r18 = 0;
    } else if (!nonzero(c, std::int8_t(player_row))) {
        r18 = std::uint32_t(p.value());
    } else if (human(r13, 0, 0x1035fe00)) {
        r18 = program.call(0x10089a40, 0x1035fe1c, {c.u32(contracts_slot), r13, club_address, person});
    } else {
        r18 = program.call(0x10085860, 0x1035fe44, {c.u32(contracts_slot), r13, club_address, person, 0, 1, 1});
    }
    const std::int32_t cap = c.i32(at(local::offer_cap));
    if (nonzero(c, cap) && nonzero(c, std::int8_t(bid_capped)) && compare(c, cap, r18.value()) < 0)
        r18 = std::uint32_t(cap);

    if (!nonzero(c, std::int8_t(player_row))) return true;
    if (!nonzero_logical(c, contract_club(person, 0x1035fe84))) return true;
    if (nonzero(c, std::int8_t(bid_capped))) return true;
    price_high = std::uint32_t(r18.value() >> 31);
    const std::int64_t asking = r18.value();
    leave(c, {{4, r21 ^ 0x80000000}});
    const bool covered = wide_at_least(c, price(), asking);
    r14 = 0;
    if (!covered) {
        const auto mode = Row::category(row.flags());
        if (compare_logical(c, mode, 5) == 0 || (compare_logical(c, mode, 4) == 0 && compare(c, p.age(), 28) < 0)) {
            program.call(0x10773c80, 0x1035ff18, {r21, r20});
            const double worth = c.f(1);
            const double factor = fpu.fadd(k::half, fpu.fdiv(integer(0xec, club.reputation()), k::thirteen_thousand));
            f30 = k::bid_cap;
            f29 = fpu.fmul(worth, factor);
            if (!(fcompare(f30, f29) & 8)) f30 = double(f29);
            const std::int64_t shortfall = asking - price();
            r14 = std::uint32_t(shortfall);
            r13 = high(shortfall);
            program.call(0x10773c80, 0x1035ff74, {r13, r14});
            double raise;
            if (fcompare(c.f(1), f30) & 8) {
                program.call(0x10773c80, 0x1035ff88, {r13, r14});
                raise = c.f(1);
            } else {
                raise = k::bid_cap;
                if (!(fcompare(raise, f29) & 8)) raise = f29;
            }
            c.set_f(1, raise);
            r14 = std::uint32_t(truncated(0xf4, raise));
        }
    }
    const auto seller = contract_club(person, 0x1035ffb8);
    const std::uint32_t offered = r20 + r14;
    c.put32(at(local::ninth_argument), 0);
    // Quirk: the offer's high word passed is the sign of its low word, not
    // the 64-bit sum's.
    const auto verdict = program.call(0x1036f170, 0x10360014, {manager, seller, person, std::uint32_t(std::int32_t(offered) >> 31),
                                                                offered, club_address, price_high, r18});
    return compare(c, std::int8_t(verdict), 2) != 0;
}

// A first-pass player's score scaled by what the price leaves of the
// budget: 1 - (asking - price) / (budget x scale), above 1 eased to 0.67 +
// a third, kept within 0.01-3; then the injury penalty.
void Scan::adjust_score() {
    const std::int64_t margin = std::int64_t(r18.value()) - price();
    program.call(0x10773c80, 0x1036004c, {high(margin), std::uint32_t(margin)});
    const double margin_value = c.f(1);
    const double scale = fpu.fmul(f31, integer(0xec, c.i32(at(local::fee_floor))));
    double share = fpu.fsub(k::one, fpu.fdiv(margin_value, scale));
    if (fcompare(share, k::one) & 4) share = fpu.fadd(k::two_thirds, fpu.fdiv(share, k::three));
    const std::int32_t score = c.i32(at(local::score));
    const auto floor = fcompare(k::hundredth, share);
    const double score_value = integer(0xf4, score);
    c.set_f(1, score_value);
    double factor = k::hundredth;
    if (!(floor & 4)) {
        factor = k::three;
        if (!(fcompare(k::three, share) & 8)) factor = share;
    }
    c.put32(at(local::score), std::uint32_t(truncated(0xfc, fpu.fmul(score_value, factor))));
    injury_penalty(0x103600fc, 15, 1);
}

// 0x1018c340: an injury of more than `threshold` days costs weight x the
// square of the excess.
void Scan::injury_penalty(std::uint32_t back, std::int32_t threshold, std::uint32_t weight) {
    const auto injury = program.call(0x1018c340, back, {c.u32(c.toc(slot::injuries)), person});
    if (!nonzero_logical(c, injury)) return;
    const std::int32_t days = c.i16(injury + 0x34);
    if (compare(c, days, threshold) <= 0) return;
    const std::uint32_t excess = std::uint32_t(days - threshold);
    c.put32(at(local::score), c.u32(at(local::score)) - excess * excess * weight);
}

// A score above the best: the club's interest weight (f30: 0x10369080 for a
// player row, 0x10366470 for staff); a first-pass player may be approached
// at once; then, unless the weight is below 3.15, a first-pass player must
// be affordable. False when he is not (the entry ends there).
bool Scan::evaluate() {
    const std::uint8_t flags = row.flags();
    if (nonzero(c, std::int8_t(player_row))) {
        program.call(0x10369080, 0x103602b4, {manager, person, club_address, 1,
                                              std::uint32_t(std::int32_t(std::int8_t(Row::loan(flags)))),
                                              std::uint32_t(std::int32_t(std::int8_t(Row::category(flags)))), 0});
    } else {
        program.call(0x10366470, 0x103602d4, {manager, person, club_address, 1, row.kind(), 0});
    }
    f30 = c.f(1);
    if (!nonzero(c, std::int32_t(Row::loan(row.flags()))) && nonzero(c, std::int8_t(player_row))) approach();
    if (!at_least(f30, k::three_fifteen)) return true;
    const std::uint8_t now = row.flags();
    c.set(7, now);
    if (!nonzero(c, std::int32_t(Row::loan(now))) && nonzero(c, std::int8_t(player_row)) && !affordable()) return false;
    c.put32(at(local::chosen), person);
    best = c.u32(at(local::score));
    return true;
}

// The immediate approach: when the weight reaches 3.15 - a draw below 20
// tenths, one draw in 79 - ability/150 hits; a player of ability above 5000
// plus a draw below 2000, or one at a human club, then gets the club's
// interest registered (0x10698bb0) with 0.75 x his price plus a draw below
// 1.25 x his price as the offer (none when the offer was capped).
void Scan::approach() {
    const Person p = candidate();
    advance(0x1036030c);
    const std::int32_t value = draw();
    const std::uint32_t entry_value = c.u16(cursor());
    const double tenths = integer(0xec, value % 20);
    c.set_f(1, tenths);
    const double bar = fpu.fsub(k::three_fifteen, fpu.fdiv(tenths, k::ten));
    leave(c, {{4, std::uint32_t(value < 0)}, {5, std::uint32_t(value)}, {6, entry_value}});
    if (!at_least(f30, bar)) return;
    const auto profile = p.player().get();
    const std::int32_t ability = Player{c, profile}.reputation();
    r13 = std::uint32_t(79 - ability / 150);
    leave(c, {{4, std::uint32_t(ability < 0)}, {5, profile}});
    random_below(0xec, Copy::a, 0x103603cc, 0x103604dc, 0x10360494);
    if (compare(c, r14.value(), 0) != 0) return;
    advance(0x1036051c);
    const std::int32_t second = draw();
    const std::int32_t strength = Player{c, p.player()}.reputation();
    leave(c, {{4, std::uint32_t(second)}, {5, std::uint32_t(strength)}});
    if (compare(c, strength, second % 2000 + 5000) <= 0) {
        const auto own = p.club().get();
        if (!nonzero_logical(c, own)) return;
        if (!human(own, 0, 0x10360588)) return;
    }
    if (nonzero(c, std::int8_t(bid_capped))) {
        r20 = 0;
        r21 = 0;
    } else {
        program.call(0x10773c80, 0x103605b0, {r21, r20});
        f29 = fpu.fmul(k::one_quarter_more, c.f(1));
        if (fcompare(f29, k::zero) & 2) r13 = 0;
        else r13 = std::uint32_t(random_below_share());
        program.call(0x10773c80, 0x10360764, {r21, r20});
        const double worth = c.f(1);
        const double extra = integer(0xdc, r13.value());
        set_price(truncated(0xe4, fpu.multiply_add(0x10360784, k::three_quarters, worth, extra)));
    }
    c.set_f(1, f30);
    program.call(0x10698bb0, 0x103607b8, {c.u32(c.toc(slot::interest)), person, club_address, r20});
}

// The inlined random_below for n = trunc(f29) (read back from the stack each
// time), into r13.
std::int32_t Scan::random_below_share() {
    advance(0x103605ec);
    const std::uint64_t bits = fpu.fctiwz(0, f29);
    c.put64(at(0xec), bits);
    bool big = compare(c, c.i32(at(0xf0)), 0xffff) > 0;
    if (!big) {
        c.put64(at(0xec), bits);
        big = compare(c, c.i32(at(0xf0)), -0xffff) < 0;
    }
    if (!big) {
        c.put64(at(0xdc), bits);
        const std::int32_t value = draw();
        const std::int32_t n = c.i32(at(0xe0));
        c.put64(at(0xe4), bits);
        compare(c, c.i32(at(0xe8)), 0);
        const std::uint32_t remainder = std::uint32_t(modulo(0x10360738, value, n));
        leave(c, {{4, std::uint32_t(n)}, {5, std::uint32_t(value)}});
        r13 = c.i32(at(0xe8)) > 0 ? remainder : 0u - remainder;
        return r13.value();
    }
    const std::uint64_t again = fpu.fctiwz(0, f29);
    c.put64(at(0xec), again);
    c.put64(at(0xe4), again);
    const auto d = wide_draw(c, fpu, random(), c.i32(at(0xf0)), at(0xf4), at(0xfc), at(0x104));
    r13 = std::uint32_t(d.part);
    leave(c, {{4, signed_word(std::int32_t(d.x - d.multiple))}, {5, 0xffff}, {6, d.whole.sign}, {7, d.x}});
    const auto sampled = std::int32_t(program.call(routine_address::sample_range, 0x103606dc, {std::uint32_t(d.coarse.value)}));
    c.put64(at(0xdc), fpu.fctiwz(0, f29));
    r13 = std::uint32_t(wide_total(c, r13.value(), sampled, c.i32(at(0xe0)), true));
    return r13.value();
}

// Whether the club can afford a first-pass player it rates at 3.15 or more:
// the quote (0x1037b6f0, priced as kind 1 above 1250 x reputation outside
// mode 1, as kind 2 above 750 x reputation in modes 0-1, as kind 3 for a
// mode-5 player of 21-23 a club below 4250 reputation would see 750 above
// it, else as the row's mode) x max(0.9, 3.5 / max(0.1, weight)) within 1.2
// x the finance record's budget for that kind (0x100f2680).
bool Scan::affordable() {
    const Person p = candidate();
    const std::int32_t reputation = club.reputation();
    std::int32_t kind;
    if (!wide_at_least(c, std::int32_t(std::uint32_t(reputation) * 1250), price()) &&
        compare_logical(c, Row::category(row.flags()), 1) != 0) {
        kind = 1;
    } else if (!wide_at_least(c, std::int32_t(std::uint32_t(reputation) * 750), price()) &&
               compare_logical(c, Row::category(row.flags()), 2) < 0) {
        kind = 2;
    } else {
        const std::uint32_t mode = Row::category(row.flags());
        const auto profile = p.player().get();
        if (compare_logical(c, mode, 5) == 0 && nonzero_logical(c, profile) && compare(c, p.age(), 20) > 0 &&
            compare(c, p.age(), 24) < 0 && compare(c, reputation, 0x109a) < 0 &&
            compare(c, std::int32_t(Player{c, profile}.reputation()) - 750, reputation) > 0)
            kind = 3;
        else
            kind = std::int8_t(mode);
    }
    r13 = std::uint32_t(std::int32_t(std::int8_t(kind)));
    program.call(0x1037b6f0, 0x103608d0, {at(local::quote), person, club_address, 0, r13, 11, 0});
    f30 = c.f(1);
    const auto weight = [&](std::uint32_t back) {
        program.call(0x10369080, back, {manager, person, club_address, 0, 0,
                                        std::uint32_t(std::int32_t(std::int8_t(Row::category(row.flags())))), 0});
        return c.f(1);
    };
    double interest = weight(0x103608fc);
    if (fcompare(k::tenth, interest) & 4) interest = k::tenth;
    else interest = weight(0x10360934);
    double ratio = fpu.frsp(fpu.fdiv(k::three_and_half, fpu.frsp(interest)));
    if (fcompare(ratio, k::nine_tenths) & 8) ratio = k::nine_tenths_single;
    f29 = fpu.fmuls(ratio, f30);
    c.set_f(1, ratio);
    const auto record = program.call(0x10103510, 0x10360970, {c.u32(c.toc(slot::finances)), club_address});
    const std::int32_t budget = std::int32_t(program.call(0x100f2680, 0x1036097c, {record, person, r13}));
    const double budget_value = integer_single(0xdc, budget);
    c.set_f(1, budget_value);
    const double available = fpu.fmul(k::one_point_two, budget_value);
    return !(fcompare(f29, available) & 4);
}

// When a draw below 1.5 x the score beats the best, a first-pass player
// under 28 - a draw below 6, contracted to a club and at one, whose club's
// reputation plus a draw below 1000 plus 2000 is below this club's, may be
// unsettled: 0x102cdbf0 about his contract's club, then 0x100e92d0.
void Scan::surprise() {
    const Person p = candidate();
    r13 = std::uint32_t(truncated(0xe4, fpu.fmul(k::one_and_half, integer(0xdc, c.i32(at(local::score))))));
    c.set_f(1, k::signed_bias);  // lfd f1: the bias itself
    random_below(0xdc, Copy::a, 0x10360a14, 0x10360b24, 0x10360adc);
    if (compare(c, r14.value(), best.value()) <= 0) return;
    if (!nonzero(c, std::int8_t(player_row))) return;
    advance(0x10360b6c);
    const std::int32_t value = draw();
    const std::int32_t age = p.age();
    leave(c, {{4, std::uint32_t(value)}, {5, std::uint32_t(age)}});
    if (compare(c, age, 28 - value % 6) >= 0) return;
    if (!nonzero_logical(c, contract_club(person, 0x10360bc0))) return;
    if (!nonzero_logical(c, p.club())) return;
    r13 = contract_club(person, 0x10360c0c);
    advance(0x10360c58);
    const std::int32_t second = draw();
    const std::int32_t theirs = c.i16(r13 + 0x1ec), ours = club.reputation();
    leave(c, {{4, std::uint32_t(second < 0)}, {5, std::uint32_t(second)}, {6, std::uint32_t(ours)}});
    if (compare(c, ours, theirs + second % 1000 + 2000) <= 0) return;
    const auto seller = contract_club(person, 0x10360cb4);
    c.set(4, seller);
    if (!nonzero(c, std::int8_t(program.call(0x102cdbf0, 0x10360cec, {c.u32(c.toc(slot::club_relations)), seller}))))
        return;
    program.call(0x100e92d0, 0x10360d00, {person, club_address});
}
}

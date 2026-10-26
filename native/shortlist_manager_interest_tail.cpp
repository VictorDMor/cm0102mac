#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080), the end: the club's value against
// his club's (0x1036c408-0x1036cde8, a move only), a contract he signed
// lately (0x1036cde8-0x1036cf48), and whether the club can pay for him
// (0x1036cf48-0x1036d218).
namespace cm::aot::shortlist_manager::interest {
void Evaluation::value_comparison() {
    const Person p = person();
    const auto division = club().division().get();
    if (!nonzero_logical(c, division) || compare(c, c.i32(division), c.i32(c.toc(toc::division_b))) == 0) {
        scale(0.1);
    } else if (compare_logical(c, p.club(), club_at) == 0) {
        // Back to his registered club: x(+0x45 / 10) when over 10.
        const auto player = p.player().get();
        if (nonzero_logical(c, player)) {
            const std::int32_t versatility = Player{c, player}.versatility();
            if (compare(c, versatility, 10) > 0) scale(fpu.fdiv(real(0xe4, versatility), 10.0));
        }
    }

    // f28 from here: 1 + (the club's reputation - his average) / 8000.
    const std::int32_t club_reputation = club().reputation();
    {
        const Player player = p.player_record();
        const std::int32_t world = player.world_reputation(), home = player.home_reputation(), now = player.reputation();
        leave(c, {{4, std::uint32_t(club_reputation)}, {5, std::uint32_t(home)}, {6, std::uint32_t(now)},
                  {7, std::uint32_t(world)}, {8, 0x55555556}});
        const std::int32_t average = std::int32_t(std::uint32_t(now) + std::uint32_t(home + world)) / 3;
        weight_squad = fpu.fadd(1.0, fpu.fdiv(real(0xe4, std::int32_t(std::uint32_t(club_reputation) - std::uint32_t(average))), 8000.0));
    }
    const bool on_loan = loan();
    const std::int32_t value = std::int16_t(std::uint32_t(r31)), current_value = std::int16_t(std::uint32_t(r15));
    const auto difference = [&](std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) - std::uint32_t(b)); };
    const auto keen = [&] { return has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_10)); };

    // The gap (f3, single precision) between the two values.
    std::optional<double> gap;
    if (on_loan && (keen() || test(routine::staff_test_3230, 0x1036c514))) {
        // A promising loan to a smaller club of a lower division (inferred): half the gap.
        const auto player_at = p.player().get();
        leave(c, {{4, player_at}});
        const Player player{c, player_at};
        const auto eligible = [&] {
            if (compare(c, std::int32_t(std::uint32_t(std::int32_t(player.ability())) * 50), current().reputation()) >= 0 ||
                nonzero_logical(c, p.byte_5e()))
                return false;
            if (compare(c, age(), 0x18) >= 0 && compare(c, age(), 0x20) <= 0) return false;
            if (compare(c, club().reputation(), 0xdac) <= 0 || compare(c, player.reputation(), 0x157c) >= 0) return false;
            const auto nation = club().nation().get();
            leave(c, {{6, nation}});
            if (!nonzero_logical(c, nation)) return false;
            const auto own_division = club().division().get();
            if (!nonzero_logical(c, own_division)) return false;
            const auto other_division = current().division().get();
            leave(c, {{4, other_division}});
            if (!nonzero_logical(c, other_division)) return false;
            const std::int32_t level = c.i16(own_division + 0x18);
            leave(c, {{5, std::uint32_t(level)}});
            if (compare(c, level, 6) <= 0 && compare(c, level, std::int32_t(c.i16(other_division + 0x18)) - 3) <= 0)
                return false;
            const auto other_nation = current().nation().get();
            leave(c, {{4, other_nation}});
            if (!nonzero_logical(c, other_nation)) return false;
            const std::int32_t ranking = Nation{c, nation}.ranking();
            return compare(c, ranking, 10) >= 0 || compare(c, ranking, Nation{c, other_nation}.ranking()) >= 0;
        };
        if (eligible()) gap = fpu.frsp(fpu.fmul(0.5, real(0xe4, difference(value, current_value))));
    }
    if (!gap && keen() && compare(c, current_value, value) > 0 &&
        compare(c, difference(current_value, value), 0x1388) < 0) {
        // Keen on a club worth less: -0.0002 x the difference, squared.
        const std::int32_t less = difference(current_value, value);
        const double first = fpu.fsubs(raw_signed(0xe4, less), k::signed_bias);
        const double second_raw = raw_signed(0xec, less);
        const double scaled = fpu.fmul(-0.0002, first);
        const double second = fpu.fsubs(second_raw, k::signed_bias);
        gap = fpu.fmuls(fpu.frsp(scaled), second);
    }
    if (!gap) {
        leave(c, {{4, std::uint32_t(value)}});
        if (compare(c, value, current_value) > 0) {
            if (has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_1))) {
                gap = fpu.frsp(fpu.fmul(2.5, real(0xe4, difference(value, current_value))));
            } else {
                // x(0.75 + determination / 11.4).
                const std::int32_t determination = p.ambition();
                leave(c, {{4, magic}, {5, signed_word(determination)}});
                const double factor = fpu.fadd(0.75, fpu.fdiv(real(0xe4, determination), 11.4));
                gap = fpu.frsp(fpu.fmul(factor, real(0xec, difference(value, current_value))));
            }
        } else if (compare(c, club_role(), 3) > 0) {
            // Quirk: the original's bgt before this compare is dead (the same cr0 took ble).
            gap = fpu.frsp(fpu.fmul(1.5, real(0xe4, difference(value, current_value))));
        }
    }
    if (!gap) {
        leave(c, {{4, std::uint32_t(value)}});
        if (compare(c, value, current_value) <= 0 && order(score, 10.0).gt) {
            // A step down for an interested club: x(1.1 - score / 100), 0.1-1.
            double factor;
            if (compare(c, current_value, 0x1b58) > 0 && compare(c, value, 0x7d0) < 0 && loan()) {
                const bool under_50 = order(score, 50.0).lt;
                const double down = fpu.fsubs(raw_signed(0xe4, difference(current_value, value)), k::signed_bias);
                const double slope = multiply_add(0x1036c7dc, double(c.f32(c.toc(slot::singles) + 0x28)), down,
                                                  double(c.f32(c.toc(slot::singles) + 0x24)));
                double weighed = fpu.fmul(slope, under_50 ? double(score) : 50.0);
                if (order(10.0, weighed).gt) weighed = 10.0;
                else weighed = fpu.fmul(slope, order(score, 50.0).lt ? double(score) : 50.0);
                factor = fpu.frsp(fpu.fsub(1.1, fpu.fdiv(fpu.frsp(weighed), 100.0)));
            } else {
                factor = fpu.frsp(fpu.fsub(1.1, fpu.fdiv(score, 100.0)));
            }
            const double bounded = order(0.1, factor).gt ? 0.1 : order(1.0, factor).lt ? 1.0 : factor;
            const double raw_gap = raw_signed(0xe4, difference(value, current_value));
            const double rounded = fpu.frsp(bounded);
            gap = fpu.fmuls(rounded, fpu.fsubs(raw_gap, k::signed_bias));
        } else {
            gap = fpu.fsubs(raw_signed(0xe4, difference(value, current_value)), k::signed_bias);
        }
    }

    // With a career factor under 1, a part of the gap counts at it.
    const double step = *gap;
    if (order(step, 0.0).gt && order(fit, 1.0).lt) {
        const double fiftieth = fpu.fdiv(step, 50.0);
        const double capped = order(100.0, fiftieth).lt ? 100.0 : fiftieth;
        const bool w = wanted();
        const double factor = fpu.fdiv(multiply_add(0x1036c8fc, fpu.fsub(100.0, capped), fit, capped), 100.0);
        scale(factor);
        if (w && order(factor, 0.6).lt) reason(0x100000);
    }
    // The value factor 1 + gap / 6200 (at least 0.001, x1.5 with flag 0x100).
    double factor = fpu.fadd(1.0, fpu.fdiv(step, 6200.0));
    if (order(factor, 0.001).lt) factor = 0.001;
    if (has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_100))) factor = fpu.fmul(factor, 1.5);
    const auto value_reasons = [&](double low, double high) {
        if (order(factor, low).lt) {
            const bool improving = order(weight_league, 1.2).gt && nonzero_logical(c, current_league) &&
                                   !nonzero_logical(c, candidate_league) && order(score, 1.25).gt &&
                                   compare(c, current_role(), 4) < 0 && compare(c, current_role(), 0) > 0 && !(has_contract() && nonzero(c, contract().listed()));
            reason(improving ? 0x400000 : 0x800000);
        } else if (order(factor, high).gt && order(weight_league, 1.2).gt && !nonzero_logical(c, current_league) &&
                   nonzero_logical(c, candidate_league) && (!has_club() || compare(c, current().reputation(), 0x1d4c) < 0) &&
                   compare(c, club_role(), 4) < 0 && compare(c, club_role(), 0) > 0 &&
                   !(has_contract() && nonzero(c, contract().listed()))) {
            reason(0x200000);
        }
    };
    scale(factor);
    if (wanted()) value_reasons(0.6, 1.5);
    if (!loan()) scale(factor);
    if (has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_1))) {
        scale(factor);
        if (wanted()) value_reasons(0.8, 2.0);
    }

    if (loan()) {
        // A loan from a club valued below his reputation (inferred).
        const auto player_at = p.player().get();
        leave(c, {{4, player_at}});
        if (compare(c, Player{c, player_at}.reputation(), current_value) > 0 && compare(c, current_value, value) > 0) {
            weight_squad = fpu.fmul(weight_squad, fpu.fadd(1.0, fpu.fdiv(real(0xe4, difference(value, current_value)), 5000.0)));
            if (order(weight_squad, 0.1).lt) weight_squad = 0.1;
        }
    }
    const std::int32_t current_tier = std::int8_t(std::uint32_t(r29)), club_tier = std::int8_t(std::uint32_t(candidate_tier));
    const auto different_tiers = [&] {
        leave(c, {{4, std::uint32_t(club_tier)}});
        return compare(c, current_tier, club_tier) != 0;
    };
    if (loan()) {
        // The nations' tiers: x(his club's / the club's).
        leave(c, {{4, signed_word(current_tier)}});
        const bool w = wanted();
        const double his = real(0xe4, current_tier);
        const double tiers = fpu.fdiv(his, real(0xec, club_tier));
        scale(tiers);
        if (w && order(tiers, 0.6).lt) reason(0x1000000);
    } else if (has_club() && different_tiers()) {
        // Different tiers: x(his tier x the club's division value) / (the
        // club's tier x his club's division value, at least 500), 0.01-10.
        leave(c, {{5, signed_word(halfword(0x130))}, {6, magic}});
        const double his = real(0xe4, current_tier);
        const double division_value = real(0xec, halfword(0x130));
        const std::int32_t his_division = halfword(0x12e);
        const bool small = compare(c, his_division, 0x1f4) <= 0;
        const double over = fpu.fmul(his, division_value);
        const double theirs = real(0xf4, club_tier);
        const double under = fpu.fmul(theirs, real(0xfc, small ? 0x1f4 : his_division));
        const double ratio = fpu.fdiv(over, under);
        const double bounded = order(0.01, ratio).gt ? 0.01 : order(10.0, ratio).lt ? 10.0 : ratio;
        scale(bounded);
        const auto federation = club().nation_record().continent().get();
        leave(c, {{4, federation}});
        if (compare_logical(c, federation, current().nation_record().continent()) != 0 && order(bounded, 1.0).lt)
            scale(0.2, 0x2000000);
    } else if (has_club()) {
        // The same tier: a move out of the federation (TOC id) the person and his club share.
        const auto federation = p.nation_record().continent().get();
        leave(c, {{4, federation}});
        if (nonzero_logical(c, federation)) {
            const std::int32_t id = c.i32(c.toc(toc::federation_id));
            leave(c, {{4, std::uint32_t(id)}});
            if (compare(c, c.i32(federation + 8), id) == 0) {
                const auto club_federation = club().nation_record().continent().get();
                leave(c, {{5, club_federation}});
                if (nonzero_logical(c, club_federation) && compare(c, c.i32(club_federation + 8), id) != 0 &&
                    compare_logical(c, club_federation, current().nation_record().continent()) != 0)
                    scale(0.4, 0x2000000);
            }
        }
    }
    const bool w = wanted();
    scale(weight_squad);
    if (w && order(weight_squad, 0.6).lt) reason(0x800000);
}

std::optional<double> Evaluation::recent_contract() {
    const Person p = person();
    if (!has_club() || compare_logical(c, current_at, club_at) == 0) return std::nullopt;
    const std::int32_t potential = p.player_record().potential();
    leave(c, {{4, std::uint32_t(potential)}});
    if (compare(c, potential, 0x82) <= 0 || compare(c, potential, std::int32_t(current().reputation()) / 60) <= 0 ||
        compare(c, age(), 0x1a) >= 0)
        return std::nullopt;
    if (!nonzero(c, std::int8_t(program.call(routine::contract_dates, 0x1036ce48, {current_at, frame.at(0x78), frame.at(0x70)}))))
        return std::nullopt;
    if (compare(c, std::int8_t(std::uint32_t(r25))) <= 0 || !has_contract()) return std::nullopt;
    // A young talent whose club date (sp+0x70) is before his contract ends:
    // signed lately, not to be approached within 150 days (inferred).
    c.put32(frame.at(0x38), c.u32(frame.at(0x70)));
    c.put32(frame.at(0x3c), c.u32(frame.at(0x74)));
    const Date copy{c, frame.at(0x38)};
    const Date ends = contract().ends();
    const std::uint16_t year = copy.year();
    leave(c, {{4, year}});
    const auto order_of_years = compare_logical(c, year, std::uint16_t(ends.year()));
    bool before = order_of_years < 0;
    if (order_of_years == 0) {
        const std::int32_t end_day = ends.day();
        leave(c, {{4, std::uint32_t(end_day)}});
        before = day_after(end_day, copy.day());
    }
    if (!nonzero(c, before ? 1 : 0)) return std::nullopt;
    if (compare(c, std::int32_t(contract().flags().get())) != 0 || nonzero(c, contract().listed())) return std::nullopt;
    if (nonzero(c, std::int8_t(program.call(routine::human_controlled, 0x1036ceec, {club_at, 1, 1, 0, 0}))) &&
        compare(c, p.professionalism(), 0xf) <= 0)
        return std::nullopt;
    const auto now = today();
    const std::int32_t days =
        std::int32_t(program.call(routine::days_between, 0x1036cf1c, {frame.at(0x70), c.u32(now), c.u32(now + 4)}));
    if (compare(c, days, 0x96) >= 0) return std::nullopt;
    if (wanted()) {
        const auto had = c.u32(reasons_at);
        leave(c, {{4, had}});
        c.put32(reasons_at, had | 0x80000000u);
    }
    return 0.0;
}

double Evaluation::finances() {
    if (!nonzero(c, std::int8_t(c.u8(frame.at(0x1f7)))) || loan() || order(score, 1.75).lt) return score;
    // The club's means against the asking price (f27) and its budget (f26).
    program.call(routine::offer_object, 0x1036cf78, {frame.at(0x80)});
    r13 = program.call(routine::club_finances, 0x1036cf88, {c.u32(c.toc(toc::finances)), club_at});
    if (!nonzero(c, std::int32_t(std::uint32_t(r13)))) return 0.0;
    program.call(routine::asking_price, 0x1036cfb8, {frame.at(0x80), person_at, club_at, 0, role_word, 0xb, 0});
    fit = c.f(1);
    const std::int32_t budget = std::int32_t(program.call(routine::transfer_budget, 0x1036cfcc, {r13, person_at, role_word}));
    const bool low_score = order(0.1, score).gt;
    weight_squad = 0.0;  // f28 from here: the reluctance, 0-20
    f26 = real(0xe4, budget);
    weight_wage = fpu.fdiv(3.5, low_score ? 0.1 : double(score));  // f29 from here: 3.5 / score, at least 0.9
    if (order(weight_wage, 0.9).lt) weight_wage = 0.9;
    fit = fpu.fmul(weight_wage, fit);
    if (has_contract()) {
        if (flag(flag::bit_100)) weight_squad = fpu.fadd(weight_squad, 10.0);
        if (test(routine::staff_test_3150, 0x1036d040, current_at)) weight_squad = fpu.fadd(weight_squad, 10.0);
        const std::int32_t happiness = contract().happiness();
        if (compare(c, happiness) < 0) weight_squad = fpu.fadd(weight_squad, fpu.fdiv(real(0xe4, happiness), -5.0));
        if (compare(c, std::int32_t(flags())) != 0)
            weight_squad = fpu.fadd(weight_squad, fpu.fdiv(real(0xec, contract().morale()), -5.0));
        if (order(0.0, weight_squad).gt) weight_squad = 0.0;
        else if (order(20.0, weight_squad).lt) weight_squad = 20.0;
    }
    if (order(fpu.fmul(0.65, fit), f26).gt) return 0.0;
    if (order(fit, f26).gt) {
        const double half = fpu.fmul(fit, 0.5);
        const double met = order(f26, half).gt ? double(f26) : half;
        const double part = fpu.fmul(weight_squad, met);
        const double rest = fpu.fsub(20.0, weight_squad);
        fit = fpu.frsp(fpu.fdiv(multiply_add(0x1036d124, rest, fit, part), 20.0));
    }
    // Quirk: unreachable, the score is at least 1.75 (or NaN) here.
    if (order(score, 1.75).lt) fit = fpu.fadd(fit, 5000.0);
    if (order(fit, f26).gt && order(fit, 0.0).gt) {
        const double ratio = fpu.fdiv(f26, fit);
        if (order(ratio, 0.7).lt) return 0.1;
        scale(ratio);
        fit = double(f26);
    }
    if (has_contract()) {
        // His wage (at least 10) against the price.
        double wage = real(0xe4, contract().wage());
        if (order(wage, 10.0).lt) wage = 10.0;
        const double ratio = compare_logical(c, current_at, club_at) == 0 ? fpu.fdiv(wage, fit) : fpu.fdiv(fit, wage);
        const double bounded = order(0.1, ratio).gt ? 0.1 : order(10.0, ratio).lt ? 10.0 : ratio;
        order(bounded, 1.0);
        fold_greater_into_equal();
        if (c.cr(0) & 2) {
            scale(bounded);
        } else {
            const double blend = multiply_add(0x1036d208, bounded, fpu.fsub(20.0, weight_squad), weight_squad);
            scale(fpu.fdiv(fpu.fdiv(blend, 20.0), weight_wage));
        }
    }
    return score;
}
}

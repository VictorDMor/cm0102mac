// SHORTLIST_MANAGER.CPP 0x10366470, the candidate context evaluation: a
// club's value to him (inferred): prestige, the staff's rating of him,
// division standing, stadium capacity and finances, weighed by the weights
// of weigh_factors(), then the club's status and his affinity for it. The
// original has two copies, for the considered club (0x103674e4) and for the
// reference club (0x10368070), which differ in a few constants, in where
// they keep their temporaries and in what the staff rating is; they share
// the steps below, given the copy's sites. A free agent's own value is his
// reputation (0x10367fe0).
#include "shortlist_manager_context_detail.hpp"

namespace cm::aot::shortlist_manager::context {
namespace {
constexpr ValuationSites considered_sites{
    .staff_blend = {0x1036781c, 0x1036786c, 0x10367888, 0x103678e0, 0x103678fc},
    .finance = {0x10367928, 0x10367940, 0x1036794c},
    .finance_factors = {1.5, 1.25, 0.85, 0.7},
    .finance_floors = {0x1676, 0x109a},
    .budget = {0x10367a6c, 0x10367a7c},
    .national_factor = 1.45,
    .budget_blend = 0x10367b14,
    .weighed = {0x10367b90, 0x10367ba4, 0x10367bb8, 0x10367bc0},
    .status = 0x10367c2c,
    .outlook = 0x10367cd8,
    .affinity = 0x10367d4c,
    .affinity_blend = 0x10367d80,
};
constexpr ValuationSites reference_sites{
    .staff_blend = {0x10368428, 0x10368474, 0x10368490, 0x103684e4, 0x10368500},
    .finance = {0x1036852c, 0x10368544, 0x10368550},
    .finance_factors = {1.5, 1.25, 0.9, 0.75},
    .finance_floors = {0x1676, 0x128e},
    .budget = {0x10368670, 0x10368680},
    .national_factor = 1.35,
    .budget_blend = 0x10368718,
    .weighed = {0x10368794, 0x103687a8, 0x103687bc, 0x103687c4},
    .status = 0x103687dc,
    .outlook = 0x10368888,
    .affinity = 0x10368904,
    .affinity_blend = 0x10368938,
};
}

void Evaluation::value_considered_club() {
    const Temporaries t{0xbc};
    nation_level = level_squared(club.nation());
    club_prestige = prestige(club, t);
    club_division = division_reputation(club);
    // 0x10367688: its stadium record, by its own index or the clubs' table.
    const std::int32_t index = club.stadium();
    if (compare(c, index, -1) > 0) {
        stadium_record = c.u32(c.toc(slot::stadiums)) + word(index) * 0x6c;
    } else {
        const std::int32_t listed = c.i32(c.u32(c.toc(slot::club_stadiums)) + word(club.id()) * 8);
        if (compare(c, listed, -1) > 0) stadium_record = c.u32(c.toc(slot::stadiums)) + word(listed) * 0x6c;
    }
    club_capacity = capacity_of(stadium_record, club_division, t);
    // 0x10367734: the staff's rating of him (0x100cdef0), adjusted by the
    // club's results but for category 5.
    const std::uint32_t staff = club.manager();
    if (!nonzero_logical(c, staff) || !nonzero_logical(c, c.u32(staff + 0x1c))) {
        club_value = 0;
    } else {
        club_value = program.call(call::staff_rating, 0x10367768, {staff, person.address, 1});
        const std::uint32_t stats = c.u32(c.toc(slot::club_stats));
        c.set(4, stats);
        if (nonzero_logical(c, stats) && compare(c, category8(), role::manager) != 0)
            club_value = blended_staff(club, stats, half_of(club_value), t, considered_sites);
    }
    finance_reputation(club_finance, club, t, considered_sites);
    const std::uint32_t budget = budget_reputation(club, club_finance, t, considered_sites);
    club_value = weighted({club_prestige, club_value, club_division, club_capacity, budget, 0},
                          {0xb4, 0xd4, 0xcc, 0xc4, 0xbc, 0xac}, considered_sites.weighed);
    // His own nation's national team: x1.45.
    if (nonzero(c, byte_of(program.call(call::is_national_team, 0x10367bd8, {club.address}))) &&
        compare_logical(c, club.nation(), person.nation()) == 0) {
        const double value = integer(0xac, half_of(club_value));
        club_value = truncated(0xb4, fpu.fmul(1.45, value));
    }
    club_value = status_adjusted(club, club_value, considered_sites.status);
    if (!outlook(club_value, club, considered_sites.outlook)) return;
    // x(0.2 + his attraction to it).
    const double attracted = attraction(club, club_value, considered_sites);
    const bool reasons_wanted = has_reasons();
    const double factor = fpu.fadd(0.2, attracted);
    score = fpu.fmul(score, factor);
    if (reasons_wanted && fcompare(factor, 0.6).less) add_reason(reason::affinity);
}

void Evaluation::value_reference_club() {
    // 0x10368070
    const Club ref{c, reference};
    const Temporaries t{0xac};
    const std::uint32_t nation = ref.nation();
    reference_level = nonzero_logical(c, nation) ? level_squared(nation) : 9;
    reference_prestige = prestige(ref, t);
    reference_division = division_reputation(ref);
    // 0x10368224: its own stadium unless that is the shared one (TOC) and
    // it has 5000 reputation or more.
    const std::int32_t index = ref.stadium();
    c.set(4, word(index));
    if (compare(c, index, -1) > 0 &&
        (compare(c, index, c.i32(c.toc(slot::shared_stadium))) != 0 || compare(c, ref.reputation(), 0x1388) < 0)) {
        reference_stadium = c.u32(c.toc(slot::stadiums)) + word(index) * 0x6c;
    } else {
        const std::int32_t listed = c.i32(c.u32(c.toc(slot::club_stadiums)) + word(ref.id()) * 8);
        if (compare(c, listed, -1) > 0) reference_stadium = c.u32(c.toc(slot::stadiums)) + word(listed) * 0x6c;
    }
    reference_capacity = capacity_of(reference_stadium, reference_division, t);
    // 0x103682ec: the current reputation of its staff's person, adjusted by
    // its results but for category 5; without one 0.75 x its division value.
    const std::uint32_t staff = ref.manager();
    bool rated = false;
    if (nonzero_logical(c, staff)) {
        const std::uint32_t member = c.u32(staff + 0x1c);
        c.set(4, member);
        if (nonzero_logical(c, member)) {
            rated = true;
            reference_staff = word(StaffRecord{c, member}.reputation());
            const std::uint32_t stats = c.u32(c.toc(slot::club_stats));
            if (nonzero_logical(c, stats) && compare(c, category8(), role::manager) != 0)
                reference_staff = blended_staff(ref, stats, std::int32_t(reference_staff), t, reference_sites);
        }
    }
    if (!rated) {
        const double value = integer(t.at(0), half_of(reference_division));
        reference_staff = truncated(t.at(1), fpu.fmul(0.75, value));
    }
    finance_reputation(reference_finance, ref, t, reference_sites);
    const std::uint32_t budget = budget_reputation(ref, reference_finance, t, reference_sites);
    other_value = weighted({reference_prestige, reference_staff, reference_division, reference_capacity, budget, 0},
                           {0xcc, 0xc4, 0xbc, 0xb4, 0xac, 0xd4}, reference_sites.weighed);
    other_value = status_adjusted(ref, other_value, reference_sites.status);
    if (!outlook(other_value, ref, reference_sites.outlook) || compare_logical(c, club.address, reference) == 0)
        return;
    // Divided by 0.2 + his attraction to it.
    const double attracted = attraction(ref, other_value, reference_sites);
    const double factor = fpu.fadd(0.2, attracted);
    score = fpu.fdiv(score, factor);
}

void Evaluation::value_free_agent() {
    // 0x10367fe0: 1.5 x his average reputation - 5000.
    const std::uint32_t squared = level_squared(person.nation());
    const StaffRecord staff{c, person.non_player()};
    const std::int32_t world = staff.world_reputation(), home = staff.home_reputation();
    const std::int32_t current = staff.reputation();
    const std::int32_t sum = std::int32_t(word(current) + word(home) + word(world));
    const std::int32_t high = multiply_high(0x55555556, sum);
    const std::int32_t average = half_of(word(high) + word(sign_bit(high)));
    reference_level = squared;
    leave(c, {{4, word(home)}, {5, word(current)}, {6, word(world)}, {7, 0x55555556}, {8, last_level_square}});
    const double value = integer(0xac, average);
    other_value = truncated(0xb4, fpu.multiply_add(0x1036805c, 1.5, value, 5000.0));
}

// The nation's level (1-3, else 3) squared, extsb.
std::uint32_t Evaluation::level_squared(std::uint32_t nation) {
    std::int32_t level = byte_of(Nation{c, nation}.level());
    if (compare(c, level, 1) < 0 || compare(c, level, 3) > 0) level = 3;
    last_level_square = word(level * level);
    return word(byte_of(last_level_square));
}

// The club's prestige (inferred): its reputation / 3.5; with a profile
// detail record, the larger of that and its byte +0x37 x reputation / 15,
// or reputation / 2 when that byte is 0.
std::uint32_t Evaluation::prestige(Club of, Temporaries t) {
    const std::int32_t reputation = of.reputation();
    const std::uint32_t profile = of.ground();
    std::uint32_t detail = 0;
    if (nonzero_logical(c, profile)) detail = c.u32(profile + 0x10);
    if (!nonzero_logical(c, profile) || !nonzero_logical(c, detail)) {
        const double value = integer(t.at(0), reputation);
        return truncated(t.at(1), fpu.fdiv(value, 3.5));
    }
    const std::int32_t factor = c.i8(detail + 0x37);
    if (!nonzero(c, factor)) return word(half_of(word(std::int32_t(word(reputation) + word(sign_bit(reputation))) >> 1)));
    const std::int32_t product = std::int32_t(word(factor) * word(reputation));
    std::int32_t fifteenth = std::int32_t(word(multiply_high(std::int32_t(0x88888889), product)) + word(product)) >> 3;
    fifteenth = std::int32_t(word(fifteenth) + word(sign_bit(fifteenth)));
    leave(c, {{4, 0x43300000}, {5, signed_word(reputation)}, {6, word(reputation)}});
    const double value = integer(t.at(0), reputation);
    const double third = fpu.fdiv(value, 3.5);
    double chosen = fpu.fsub(stacked(t.at(1), fifteenth), k::signed_bias);
    if (fcompare(third, chosen).greater) chosen = third;
    else chosen = integer(t.at(2), fifteenth);
    return truncated(t.at(3), chosen);
}

// 0x10367608: the reputation, or with a division the larger of 500 x the
// division's reputation and reputation / 2 + 250 x the division's; a club
// of 7500 and more with an attendance over 35000 keeps its reputation if
// larger. extsh.
std::uint32_t Evaluation::division_reputation(Club of) {
    const std::uint32_t division = of.division();
    c.set(5, division);
    if (!nonzero_logical(c, division)) return word(of.reputation());
    const std::int32_t reputation = of.reputation(), standing = Division{c, division}.reputation();
    c.set(6, word(reputation));
    const std::int32_t floor = std::int32_t(word(standing) * 500);
    if (compare(c, reputation, 0x1d4c) >= 0) {
        const std::int32_t attendance = of.attendance();
        c.set(4, word(attendance));
        if (compare(c, attendance, 35000) > 0) {
            const std::int32_t kept = compare(c, floor, reputation) > 0 ? floor : reputation;
            c.set(6, word(kept));
            return word(half_of(word(kept)));
        }
    }
    const std::int32_t half = std::int32_t(word(reputation) + word(sign_bit(reputation))) >> 1;
    const std::int32_t blend = std::int32_t(word(half) + word(standing) * 250);
    const std::int32_t chosen = compare(c, floor, blend) > 0 ? floor : blend;
    c.set(4, word(chosen));
    return word(half_of(word(chosen)));
}

// 0x103676d4: 600 x the stadium record's int16 +0x18, else min(0.8 x the
// division value, 7000).
std::uint32_t Evaluation::capacity_of(std::uint32_t stadium, std::uint32_t division_value, Temporaries t) {
    if (nonzero_logical(c, stadium)) return word(half_of(word(c.i16(stadium + 0x18)) * 600));
    const double value = integer(t.at(0), half_of(division_value));
    const double scaled = fpu.fmul(0.8, value);
    return truncated(t.at(1), fcompare(scaled, 7000.0).less ? scaled : 7000.0);
}

// 0x10367788: the rating weighed by the club's results (its stats record,
// inferred): r = 0.67 x +6 + 0.33 x +8 (truncated, extsh); under 5000 the
// larger of rating / (5 - 0.0008 r) and rating - 1000 (5 - 0.0008 r), from
// 5000 the smaller of rating x (0.0001 r + 0.5) and rating + 1000 x that.
std::uint32_t Evaluation::blended_staff(Club of, std::uint32_t stats, std::int32_t rating, Temporaries t,
                                        const ValuationSites& s) {
    const auto figure = [&](std::uint32_t offset) {
        if (!nonzero_logical(c, of.address)) return std::int32_t{0};
        return std::int32_t(c.i16(c.u32(stats) + word(of.id()) * 0x4c + offset));
    };
    const double first = integer(t.at(0), figure(6));
    const std::int32_t second_figure = figure(8);
    leave(c, {{4, 0x43300000}});
    const double second = integer(t.at(1), second_figure);
    const double second_part = fpu.fmul(0.33, second);
    const std::int32_t results = half_of(truncated(t.at(2), fpu.multiply_add(s.staff_blend[0], 0.67, first, second_part)));
    if (compare(c, results, 0x1388) < 0) {
        const double r = integer(t.at(0), results);
        const double divisor = fpu.multiply_add(s.staff_blend[1], 0.0008, r, 5.0);  // fnmsub: 5 - 0.0008 r
        const double value = integer(t.at(1), rating);
        double chosen = fpu.fdiv(value, divisor);
        const double again = integer(t.at(2), rating);
        const double lowered = fpu.multiply_add(s.staff_blend[2], 1000.0, divisor, again);  // rating - 1000 d
        if (!fcompare(chosen, lowered).greater) chosen = lowered;
        return truncated(t.at(3), chosen);
    }
    const double r = integer(t.at(0), results);
    const double factor = fpu.multiply_add(s.staff_blend[3], 0.0001, r, 0.5);
    const double value = integer(t.at(1), rating);
    double chosen = fpu.fmul(value, factor);
    const double again = integer(t.at(2), rating);
    const double raised = fpu.multiply_add(s.staff_blend[4], 1000.0, factor, again);
    if (!fcompare(chosen, raised).less) chosen = raised;
    return truncated(t.at(3), chosen);
}

// 0x1036791c: the reputation weighed by the finance state (0x100f55b0 of
// 0x10103510's record; none for a national team): 2 and 1 raise it with a
// floor, -1 and -2 lower it, others leave it.
template <unsigned R>
void Evaluation::finance_reputation(Held<R>& reputation, Club of, Temporaries t, const ValuationSites& s) {
    reputation = word(of.reputation());
    if (nonzero(c, byte_of(program.call(call::is_national_team, s.finance[0], {of.address})))) return;
    const std::uint32_t record =
        program.call(call::finance_record, s.finance[1], {c.u32(c.toc(slot::finances)), of.address});
    if (!nonzero_logical(c, record)) return;
    const std::int32_t state = byte_of(program.call(call::finance_state, s.finance[2], {record}));
    const std::int32_t sign = compare(c, state);
    if (sign == 0) return;
    int which;
    if (sign > 0) {
        const std::int32_t against_two = compare(c, state, 2);
        if (against_two > 0) return;
        which = against_two == 0 ? 0 : 1;
    } else {
        const std::int32_t against_minus_two = compare(c, state, -2);
        if (against_minus_two < 0) return;
        which = against_minus_two == 0 ? 3 : 2;
    }
    const double value = integer(t.at(0), std::int32_t(reputation));
    reputation = truncated(t.at(1), fpu.fmul(s.finance_factors[which], value));
    if (which < 2 && compare(c, half_of(reputation), s.finance_floors[which]) < 0)
        reputation = word(s.finance_floors[which]);
}

// 0x10367a64: left in r7: a national team not managed by a human
// (inferred) its finance reputation x1.45 (x1.35); others 0.6 x it + 0.4 x
// the shortlist manager's int16 +6 of its 12-byte record (object +0xdc).
std::uint32_t Evaluation::budget_reputation(Club of, std::uint32_t reputation, Temporaries t, const ValuationSites& s) {
    if (nonzero(c, byte_of(program.call(call::is_national_team, s.budget[0], {of.address}))) &&
        !nonzero(c, byte_of(program.call(call::is_human_managed, s.budget[1], {of.address})))) {
        const double value = integer(t.at(0), half_of(reputation));
        const std::uint32_t result = truncated(t.at(1), fpu.fmul(s.national_factor, value));
        c.set(7, result);
        return result;
    }
    const std::uint32_t object = c.u32(frame.at(0x188));
    const std::int32_t points = c.i16(c.u32(object + 0xdc) + word(of.id()) * 0xc + 6);
    leave(c, {{4, 0x43300000}, {5, signed_word(half_of(reputation))}});
    const double recorded = integer(t.at(1), points);
    const double recorded_part = fpu.fmul(0.4, recorded);
    const double own = integer(t.at(0), half_of(reputation));
    const std::uint32_t result = truncated(t.at(2), fpu.multiply_add(s.budget_blend, 0.6, own, recorded_part));
    c.set(7, result);
    return result;
}

// 0x10367b24: (prestige x w27 + staff x w28 + division x w26 + capacity x
// w30 + budget) / the weights' sum, in single precision.
std::uint32_t Evaluation::weighted(Components values, Components at, const std::uint32_t (&sites)[4]) {
    leave(c, {{4, signed_word(half_of(values.division))}, {5, signed_word(half_of(values.capacity))}, {6, 0x43300000}});
    const auto component = [&](std::uint32_t offset, std::uint32_t value) {
        return fpu.fsubs(stacked(offset, half_of(value)), k::signed_bias);
    };
    const double prestige_value = component(at.prestige, values.prestige);
    double sum = fpu.fmuls(prestige_weight, prestige_value);
    const double staff_value = component(at.staff, values.staff);
    sum = fpu.multiply_add(sites[0], staff_weight, staff_value, sum);
    const double division_value = component(at.division, values.division);
    sum = fpu.multiply_add(sites[1], division_weight, division_value, sum);
    const double capacity_value = component(at.capacity, values.capacity);
    sum = fpu.multiply_add(sites[2], capacity_weight, capacity_value, sum);
    const double budget_value = component(at.budget, values.budget);
    sum = fpu.multiply_add(sites[3], k::one_s, budget_value, sum);
    return truncated(at.result, fpu.fdivs(sum, weight_sum));
}

// 0x10367c24: a club that is no national team and not professional
// (status 1, inferred) is worth min(v / 2, v - 2000 - 5 x trait 0x5e^2);
// an amateur one (status 3) then min(v / 2, v - 2000) again. extsh.
std::uint32_t Evaluation::status_adjusted(Club of, std::uint32_t value_word, std::uint32_t back) {
    if (nonzero(c, byte_of(program.call(call::is_national_team, back, {of.address})))) return value_word;
    const std::uint32_t status = of.status();
    c.set(6, status);
    if (compare(c, std::int32_t(status), 1) == 0) return value_word;
    const std::uint32_t trait = person.byte_5e();
    const std::int32_t value = half_of(value_word);
    const std::int32_t half = std::int32_t(word(value) + word(sign_bit(value))) >> 1;
    const std::int32_t penalty = std::int32_t(word(value) - 2000 - trait * trait * 5);
    const std::int32_t lowered = compare(c, half, penalty) >= 0 ? penalty : half;
    leave(c, {{4, word(lowered)}, {5, word(value)}});
    std::int32_t result = half_of(word(lowered));
    if (compare(c, byte_of(status), 3) == 0) {
        const std::int32_t again = std::int32_t(word(result) + word(sign_bit(result))) >> 1;
        const std::int32_t less = std::int32_t(word(result) - 2000);
        result = half_of(word(compare(c, again, less) >= 0 ? less : again));
    }
    return word(result);
}

// 0x10367ca8: the value becomes 1.5 x value - 5000; whether the club has a
// nation (the attraction needs one).
template <unsigned R>
bool Evaluation::outlook(Held<R>& value_word, Club of, std::uint32_t site) {
    const std::uint32_t nation = of.nation();
    leave(c, {{4, 0x43300000}, {5, nation}});
    const double value = integer(0xac, half_of(value_word));
    const bool has_nation = nonzero_logical(c, nation);
    value_word = truncated(0xb4, fpu.multiply_add(site, 1.5, value, 5000.0));
    return has_nation;
}

// 0x10367cec: the larger of his affinity for the club (0x103092f0, players
// only; else 10) / 10 and (v + 5000)^2 / 15000 / (12000 - 200 x affinity).
double Evaluation::attraction(Club of, std::uint32_t value_word, const ValuationSites& s) {
    const std::int32_t shifted = std::int32_t(word(half_of(value_word)) + 5000);
    const bool player = nonzero_logical(c, person.player());
    const double first = integer(0xac, shifted);
    const double share = fpu.fdiv(first, 15000.0);
    const double second = integer(0xb4, shifted);
    pull = fpu.fmul(share, second);
    double affinity = 10.0;
    if (player) {
        const std::uint32_t group = Nation{c, of.nation()}.group();
        const std::uint32_t found = program.call(
            call::affinity, s.affinity, {c.u32(c.toc(slot::player_stats)), person.address, of.address, group, 1});
        affinity = integer(0xbc, byte_of(found));
    }
    const double base = fpu.multiply_add(s.affinity_blend, 200.0, affinity, 12000.0);  // fnmsub: 12000 - 200 a
    const double tenth = fpu.fdiv(affinity, 10.0);
    const double ratio = fpu.fdiv(pull, base);
    return fcompare(tenth, ratio).greater ? tenth : ratio;
}
}

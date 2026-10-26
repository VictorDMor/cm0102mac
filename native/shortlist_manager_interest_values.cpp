#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080), the club values: what the club offers
// (0x1036a37c-0x1036ad00) and what his club offers (0x1036b92c-0x1036c408),
// each from the club's reputation, its ground, division and league, the
// staff rating of its manager, its finances, weighted by the person's
// weights (f28-f30, single precision); then 1.5 x value - 5000, against
// his familiarity with the club's region.
namespace cm::aot::shortlist_manager::interest {
std::int32_t Evaluation::clamped_tier(std::uint32_t nation) {
    const std::int32_t tier = Nation{c, nation}.level();
    return compare(c, tier, 1) < 0 || compare(c, tier, 3) > 0 ? 3 : tier;
}

void Evaluation::ground_value(const Club& of, std::int32_t at, std::int32_t base) {
    const std::int32_t reputation = of.reputation();
    const auto ground = of.ground().get();
    const auto place = [&] { return c.u32(ground + 0x10); };
    if (nonzero_logical(c, ground) && nonzero_logical(c, place())) {
        const std::int32_t support = std::int8_t(c.u8(place() + 0x37));
        if (!nonzero(c, support)) {
            put_halfword(at, reputation / 2);
            return;
        }
        // max(reputation / 3.5, support x reputation / 15).
        double value = fpu.fdiv(real(base, reputation), 3.5);
        const std::int32_t share = std::int32_t(std::uint32_t(support) * std::uint32_t(reputation)) / 15;
        if (!order(value, real(base + 8, share)).gt) value = real(base + 0x10, share);
        put_halfword(at, truncated(base + 0x18, value));
        return;
    }
    put_halfword(at, truncated(base + 8, fpu.fdiv(real(base, reputation), 3.5)));
}

void Evaluation::division_value(const Club& of, std::int32_t at) {
    const auto division = of.division().get();
    if (!nonzero_logical(c, division)) {
        put_halfword(at, of.reputation());
        return;
    }
    const std::int32_t reputation = of.reputation();
    const std::int32_t level = c.i16(division + 0x18);
    if (compare(c, reputation, 0x1d4c) >= 0 && compare(c, of.attendance(), 0x88b8) > 0) {
        // A big, rich club: max(level x 500, reputation).
        const std::int32_t by_level = level * 500;
        put_halfword(at, compare(c, by_level, reputation) > 0 ? by_level : reputation);
        return;
    }
    // max(level x 500, reputation / 2 + level x 250).
    const std::int32_t blend = reputation / 2 + level * 250, by_level = level * 500;
    put_halfword(at, compare(c, by_level, blend) <= 0 ? blend : by_level);
}

template <unsigned R>
void Evaluation::find_league(const Club& of, Kept<R>& record) {
    const auto leagues = [&] { return c.u32(c.toc(toc::leagues)); };
    const std::int32_t league = of.stadium();
    if (compare(c, league, -1) > 0 &&
        (compare(c, league, c.i32(c.toc(toc::league_special))) != 0 || compare(c, of.reputation(), 0x1388) < 0)) {
        record = leagues() + std::uint32_t(league) * 0x6c;
        return;
    }
    const std::int32_t listed = c.i32(c.u32(c.toc(toc::club_leagues)) + std::uint32_t(of.id()) * 8);
    if (compare(c, listed, -1) > 0) record = leagues() + std::uint32_t(listed) * 0x6c;
}

template <class Store>
void Evaluation::league_value(const Club& of, std::uint32_t record, std::int32_t division_at, std::int32_t base,
                              std::uint32_t back, Store store) {
    if (!nonzero_logical(c, record)) {
        const double value = fpu.fmul(0.8, real(base, halfword(division_at)));
        store(truncated(base + 8, order(value, 7000.0).lt ? value : 7000.0));
        return;
    }
    const std::int32_t id = c.i32(record);
    std::int32_t value;
    if (compare(c, id, c.i32(c.toc(toc::top_league))) == 0) value = 0x3a98;
    else value = std::int16_t(c.i16(record + 0x18) * 600);
    store(value);
    const auto table = c.u32(c.u32(c.u32(c.toc(slot::competitions)) + std::uint32_t(id) * 4) + 8);
    const auto row = program.call(routine::league_row, back, {table, of.address});
    if (!nonzero_logical(c, row) || nonzero_logical(c, c.u8(row + 4))) return;
    const auto position = c.u8(row + 0x75);
    if (compare_logical(c, position, 1) < 0 || compare_logical(c, position, 0x10) > 0) return;
    // By the league position: x(1.32 - position / 50).
    const double place = fpu.fdiv(unsigned_real(base + 8, position), 50.0);
    const double worth = real(base, std::int16_t(value));
    store(truncated(base + 0x10, fpu.fmul(worth, fpu.fsub(1.32, place))));
}

std::int32_t Evaluation::blended_rating(std::int32_t rating, std::uint32_t of, std::uint32_t tables, std::int32_t base,
                                        const std::uint32_t (&sites)[6]) {
    const auto entry = [&](std::uint32_t offset) -> std::int32_t {
        if (!nonzero_logical(c, of)) return 0;
        return c.i16(c.u32(tables) + std::uint32_t(Club{c, of}.id()) * 0x4c + offset);
    };
    const std::int32_t a = entry(0xa), b = entry(6);
    const double first = real(base, a), second = real(base + 8, b);
    const std::int32_t third = entry(8);
    double blend = fpu.fmul(0.25, real(base + 0x10, third));
    blend = multiply_add(sites[0], 0.5, second, blend);
    blend = multiply_add(sites[1], 0.25, first, blend);
    const std::int32_t value = std::int16_t(truncated(base + 0x18, blend));
    const std::int32_t own = std::int16_t(rating);
    if (compare(c, value, 0x1388) < 0) {
        // Under 5000: max(rating / k, rating - 1000 k), k = 5 - 0.0008 value.
        const double scale = multiply_add(sites[2], 0.0008, real(base, value), 5.0);
        const double divided = fpu.fdiv(real(base + 8, own), scale);
        const double less = multiply_add(sites[3], 1000.0, scale, real(base + 0x10, own));
        return truncated(base + 0x18, order(divided, less).gt ? divided : less);
    }
    // Else min(rating x k, rating + 1000 k), k = 0.5 + 0.0001 value.
    const double scale = multiply_add(sites[4], 0.0001, real(base, value), 0.5);
    const double multiplied = fpu.fmul(real(base + 8, own), scale);
    const double more = multiply_add(sites[5], 1000.0, scale, real(base + 0x10, own));
    return truncated(base + 0x18, order(multiplied, more).lt ? multiplied : more);
}

std::int32_t Evaluation::raised_reputation(std::int32_t reputation, double factor, std::int32_t add, std::int32_t base) {
    const std::int32_t plus = reputation + add;
    double raised = fpu.fmul(factor, real(base, reputation));
    const double other = real(base + 8, plus);
    if (!order(raised, other).gt) raised = real(base + 0x10, plus);
    return truncated(base + 0x18, raised);
}

std::int32_t Evaluation::best_reputation(std::int32_t reputation, std::int32_t club_id) {
    const std::int32_t entry = c.i16(c.u32(manager() + 0xdc) + std::uint32_t(club_id) * 12 + 6);
    return std::int16_t(compare(c, std::int16_t(reputation), entry) > 0 ? reputation : entry);
}

std::int32_t Evaluation::halved(std::int32_t value) {
    const std::int32_t v = std::int16_t(value), half = v / 2, less = v - 2000;
    return std::int16_t(compare(c, half, less) >= 0 ? less : half);
}

double Evaluation::familiarity(std::uint32_t of, std::int32_t value, std::uint32_t back) {
    const std::int32_t plus = std::int16_t(value) + 5000;
    const double first = real(0xe4, plus);
    const double second = raw_signed(0xec, plus);
    const double share = fpu.fdiv(first, 15000.0);
    f25 = fpu.fmul(share, fpu.fsub(second, k::signed_bias));
    const auto zone = Club{c, of}.nation_record().group().get();
    const auto known = program.call(routine::regional_familiarity, back,
                                    {c.u32(c.toc(slot::player_stats)), person_at, of, zone, 1});
    return real(0xf4, std::int8_t(known));
}

double Evaluation::familiarity_share(double known, std::uint32_t site) {
    const double against = multiply_add(site, 200.0, known, 12000.0);
    const double tenth = fpu.fdiv(known, 10.0);
    const double value = fpu.fdiv(f25, against);
    return order(tenth, value).gt ? tenth : value;
}

std::optional<double> Evaluation::candidate_value() {
    const Club to = club();
    const Person p = person();
    const std::int32_t tier = clamped_tier(to.nation());
    candidate_tier = std::uint32_t(std::int32_t(std::int8_t(tier * tier)));
    ground_value(to, 0x134, 0x10c);
    division_value(to, 0x130);
    find_league(to, candidate_league);
    league_value(to, candidate_league, 0x130, 0x10c, 0x1036a5ec, [&](std::int32_t value) { r13 = std::uint32_t(value); });

    // The club manager's rating of him, blended with the club table.
    const auto boss = to.manager().get();
    if (!nonzero_logical(c, boss) || !nonzero_logical(c, c.u32(boss + 0x1c))) {
        r31 = 0;
    } else {
        r31 = program.call(routine::staff_rating, 0x1036a6e8, {boss, person_at, 1});
        const auto tables = c.u32(c.toc(toc::club_tables));
        if (nonzero_logical(c, tables))
            r31 = std::uint32_t(blended_rating(std::int32_t(std::uint32_t(r31)), club_at, tables, 0x10c,
                                               {0x1036a7cc, 0x1036a7d0, 0x1036a820, 0x1036a83c, 0x1036a894, 0x1036a8b0}));
    }

    // Its reputation, raised by a good finance mood (tier 1 nations only).
    r29 = extended(to.reputation());
    const auto finances_of = program.call(routine::club_finances, 0x1036a8e4, {c.u32(c.toc(toc::finances)), club_at});
    if (nonzero_logical(c, finances_of) && compare(c, std::int32_t(std::uint32_t(candidate_tier)), 1) == 0) {
        const std::int32_t mood = std::int8_t(program.call(routine::finance_mood, 0x1036a8f8, {finances_of}));
        const auto against_2 = compare(c, mood, 2);
        if (against_2 == 0)
            r29 = std::uint32_t(raised_reputation(std::int32_t(std::uint32_t(r29)), 1.5, 0x5dc, 0x10c));
        else if (against_2 < 0 && compare(c, mood, 1) >= 0)
            r29 = std::uint32_t(raised_reputation(std::int32_t(std::uint32_t(r29)), 1.25, 0x2ee, 0x10c));
    }
    const std::int32_t reputation = std::int16_t(std::uint32_t(r29));
    const std::int32_t best = best_reputation(std::int32_t(std::uint32_t(r29)), to.id());

    // The weight sum (f26, at most 0 taken as 0.5) and 0.65 rep + 0.35 best.
    const double one = double(c.f32(c.toc(slot::singles) + 0xc));
    const double best_real = real(0x114, best);
    double sum = fpu.fadds(one, weight_squad);
    const double best_share = fpu.fmul(0.35, best_real);
    sum = fpu.fadds(weight_wage, sum);
    const double reputation_real = real(0x10c, reputation);
    sum = fpu.fadds(weight_league, sum);
    const double blended_real = multiply_add(0x1036aa64, 0.65, reputation_real, best_share);
    f26 = fpu.fadds(one, sum);
    const std::int32_t blended = std::int16_t(truncated(0x11c, blended_real, 1));
    order(f26, 0.0);
    fold_less_into_equal();
    if (c.cr(0) & 2) f26 = double(c.f32(c.toc(slot::singles) + 4));

    // The club's value: (w_squad ground + rating + w_wage division +
    // w_league league + blended) / f26.
    const std::int32_t league = std::int16_t(std::uint32_t(r13));
    leave(c, {{7, signed_word(league)}});
    double value = fpu.fmuls(weight_squad, fpu.fsubs(raw_signed(0xec, halfword(0x134)), k::signed_bias));
    value = fpu.multiply_add(0x1036aaf8, one, real_single(0xf4, std::int16_t(std::uint32_t(r31))), value);
    const std::int32_t status = to.status();
    const bool professional = compare(c, status, 1) == 0;
    value = multiply_add(0x1036ab10, weight_wage, real_single(0xfc, halfword(0x130)), value);
    value = multiply_add(0x1036ab24, weight_league, real_single(0x104, league), value);
    value = multiply_add(0x1036ab2c, one, real_single(0x124, blended), value);
    r31 = std::uint32_t(truncated(0xe4, fpu.fdivs(value, f26)));
    if (!professional) {
        // Quirk: an unprofessional (inferred) club halves, unless he is
        // keen (flag 0x10) and +0x5e is clear; status 3 halves again.
        if (!(has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_10)) &&
              !nonzero_logical(c, p.byte_5e())))
            r31 = std::uint32_t(halved(std::int32_t(std::uint32_t(r31))));
        if (compare(c, std::int8_t(status), 3) == 0) r31 = std::uint32_t(halved(std::int32_t(std::uint32_t(r31))));
    }
    const auto nation = to.nation().get();
    leave(c, {{6, nation}});
    const double raw_value = real(0xe4, std::int16_t(std::uint32_t(r31)));
    const bool has_nation = nonzero_logical(c, nation);
    r31 = std::uint32_t(truncated(0xec, multiply_add(0x1036abf0, 1.5, raw_value, 5000.0)));
    if (!has_nation) return std::nullopt;

    // His familiarity with the club's region: a minor under 5 outside the
    // federation is rejected; else x(0.2 + the familiarity share).
    const double known = familiarity(club_at, std::int32_t(std::uint32_t(r31)), 0x1036ac58);
    if (compare(c, age(), 0x12) < 0 && order(known, 5.0).lt) {
        const auto federation = Nation{c, p.nation()}.continent().get();
        leave(c, {{4, federation}});
        if (compare_logical(c, federation, to.nation_record().continent()) != 0) return 0.0;
    }
    const double share = familiarity_share(known, 0x1036acb8);
    const bool w = wanted();
    const double factor = fpu.fadd(0.2, share);
    scale(factor);
    if (w && order(factor, 0.6).lt) reason(0x10000);
    return std::nullopt;
}

void Evaluation::current_value() {
    const Person p = person();
    if (!has_club()) {
        // Without a club: his nation's tier and 1.5 x his average reputation - 5000.
        const std::int32_t tier = clamped_tier(p.nation());
        const Player player = p.player_record();
        const std::int32_t home = player.home_reputation(), world = player.world_reputation(),
                           now = player.reputation();
        leave(c, {{4, std::uint32_t(home)}, {5, std::uint32_t(now)}, {6, std::uint32_t(world)}, {7, 0x55555556},
                  {8, std::uint32_t(tier * tier)}});
        r29 = std::uint32_t(std::int32_t(std::int8_t(tier * tier)));
        const std::int32_t average = std::int16_t(std::int32_t(std::uint32_t(now) + std::uint32_t(home + world)) / 3);
        r15 = std::uint32_t(truncated(0xec, multiply_add(0x1036b9b0, 1.5, real(0xe4, average), 5000.0)));
        return;
    }
    const Club from = current();
    const auto nation = from.nation().get();
    const std::int32_t tier = nonzero_logical(c, nation) ? clamped_tier(nation) : 3;
    r29 = std::uint32_t(std::int32_t(std::int8_t(tier * tier)));
    ground_value(from, 0x132, 0xe4);
    division_value(from, 0x12e);
    find_league(from, current_league);
    league_value(from, current_league, 0x12e, 0xe4, 0x1036bc4c, [&](std::int32_t value) { put_halfword(0x12c, value); });

    // His manager's rating (+0x2a of his staff record), blended.
    const auto boss = from.manager().get();
    const auto staff = [&] { return c.u32(boss + 0x1c); };
    if (!nonzero_logical(c, boss) || !nonzero_logical(c, staff())) {
        r13 = std::uint32_t(truncated(0xec, fpu.fmul(0.75, real(0xe4, halfword(0x12e)))));
    } else {
        const auto tables = c.u32(c.toc(toc::club_tables));
        r13 = extended(c.i16(staff() + 0x2a));
        if (nonzero_logical(c, tables))
            r13 = std::uint32_t(blended_rating(std::int32_t(std::uint32_t(r13)), current_at, tables, 0xe4,
                                               {0x1036be88, 0x1036be8c, 0x1036bed8, 0x1036bef4, 0x1036bf48, 0x1036bf64}));
    }

    // His club's reputation, raised by a good finance mood when he has a
    // contract without flags (tier 1 nations only); mood -2 is a reason.
    r15 = extended(from.reputation());
    const auto finances_of = program.call(routine::club_finances, 0x1036bf98, {c.u32(c.toc(toc::finances)), current_at});
    if (nonzero_logical(c, finances_of) && compare(c, std::int32_t(std::uint32_t(r29)), 1) == 0 && has_contract() &&
        compare(c, c.i32(contract_at + 0x50)) == 0) {
        const std::int32_t mood = std::int8_t(program.call(routine::finance_mood, 0x1036bfc0, {finances_of}));
        const auto against_1 = compare(c, mood, 1);
        if (against_1 == 0)
            r15 = std::uint32_t(raised_reputation(std::int32_t(std::uint32_t(r15)), 1.25, 0x2ee, 0xe4));
        else if (against_1 > 0 && compare(c, mood, 3) < 0)
            r15 = std::uint32_t(raised_reputation(std::int32_t(std::uint32_t(r15)), 1.5, 0x5dc, 0xe4));
        else if (against_1 < 0 && compare(c, mood, -2) == 0 && wanted())
            reason(0x40000000);
    }
    const std::int32_t best = best_reputation(std::int32_t(std::uint32_t(r15)), from.id());

    // His club's value, as the club's (the operations in this copy's order).
    const double one = double(c.f32(c.toc(slot::singles) + 0xc));
    const std::int32_t league = halfword(0x12c);
    leave(c, {{7, signed_word(league)}});
    const double best_share = fpu.fmul(0.35, real(0xec, best));
    const double blended_real = multiply_add(0x1036c154, 0.65, real(0xe4, std::int16_t(std::uint32_t(r15))), best_share);
    const bool on_loan = loan();
    const double ground = fpu.fsubs(raw_signed(0x11c, halfword(0x132)), k::signed_bias);
    const std::int32_t blended = std::int16_t(truncated(0xf4, blended_real, 2));
    double value = fpu.fmuls(weight_squad, ground);
    value = multiply_add(0x1036c19c, one, real_single(0x114, std::int16_t(std::uint32_t(r13))), value);
    value = multiply_add(0x1036c1bc, weight_wage, real_single(0x10c, halfword(0x12e)), value);
    value = multiply_add(0x1036c1d0, weight_league, real_single(0x104, league), value);
    value = multiply_add(0x1036c1d8, one, real_single(0xfc, blended), value);
    // Quirk: on his own club f26 was never set by this routine: the caller's.
    r15 = std::uint32_t(truncated(0x124, fpu.fdivs(value, f26)));
    if (!on_loan && test(routine::staff_test_3040, 0x1036c1fc))
        r15 = std::uint32_t(truncated(0xec, fpu.fmul(0.9, real(0xe4, std::int16_t(std::uint32_t(r15))))));
    const std::int32_t status = from.status();
    if (compare(c, status, 1) != 0) {
        if (!(has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_10)) &&
              !nonzero_logical(c, p.byte_5e())))
            r15 = std::uint32_t(halved(std::int32_t(std::uint32_t(r15))));
        if (compare(c, std::int8_t(status), 3) == 0) r15 = std::uint32_t(halved(std::int32_t(std::uint32_t(r15))));
    }
    if (compare(c, std::int8_t(program.call(routine::contract_relation, 0x1036c2d4, {contracts(), person_at, current_at})),
                3) == 0)
        r15 = std::uint32_t(truncated(0xec, fpu.fmul(0.7, real(0xe4, std::int16_t(std::uint32_t(r15))))));
    const auto current_nation = from.nation().get();
    leave(c, {{4, magic}, {6, current_nation}});
    const double raw_value = real(0xf4, std::int16_t(std::uint32_t(r15)));
    const bool has_nation = nonzero_logical(c, current_nation);
    r15 = std::uint32_t(truncated(0xfc, multiply_add(0x1036c344, 1.5, raw_value, 5000.0)));
    if (!has_nation || own_club()) return;
    const double known = familiarity(current_at, std::int32_t(std::uint32_t(r15)), 0x1036c3b4);
    divide(fpu.fadd(0.2, familiarity_share(known, 0x1036c3e0)));
}
}

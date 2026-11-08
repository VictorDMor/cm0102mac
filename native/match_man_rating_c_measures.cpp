// 0x10211e70: the world's measures of the person (trend, form), the judge's own estimates and the
// overall average that becomes the modifier (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man::placement {

// Flag 4: the world's measure (0x10303560, 5 / 0x12) of the player scaled by the pool.
void Placement::flag_4_measure() {
    const u32 flag_4 = u32(flags) & bit(0x1d);
    compare(c, i32(flag_4), 0);
    c.put32(at(at::flag_4), flag_4);
    if (!flag_4 || !in_people_table()) return;
    c.set_f(1, F(0x8));
    c.set({{3, c.u32(world())}, {4, data}, {5, 5}, {6, 0x12}});
    c.call(0x10303560, 0x10212d64);
    const double product = fpu.fmul(D(0xc0), c.f(1));
    const i32 value = i32(truncate_at(c, fpu, at(0x90), product));
    c.set(0, reg(value));
    if (compare(c, value, 0) != 0) {
        total = u32(total) + reg(value);
        count = u32(count) + 1;
    }
}

// Flags 0x80 and 0x100: the judge's own ability estimate (0x10219e40, as current, then as potential) joins
// the total. The second recomputes the running estimate first (a quarter of the person's own potential
// when the club is the person's), and the person's attributes lower it.
std::optional<i32> Placement::judged_estimates() {
    constexpr u32 rejected_result = reg(rejected);
    if (wants(bit(0x18))) {
        if (compare(c, counted(), 0) != 0) {
            c.set(3, reg(i32(reg(i32(u32(total))) * 50)));
            c.put16(at(at::estimate), reg(divw(i32(reg(i32(u32(total))) * 50), counted())));
        }
        c.set({{3, c.u32(at(at::judge))}, {4, person}, {7, reg(c.i16(at(at::estimate)))}, {5, club}, {6, 2}});
        match_man::call(c, appraisal::callee::judged_ability, 0x10212dc4);
        c.put16(at(at::judged), c.r(3));
        const bool checked = compare(c, s8(c.u8(at(at::has_floor))), 0) != 0;
        count = u32(count) + 1;
        total = u32(total) + reg(c.i16(at(at::judged)));
        if (checked && floor_fails(true)) return guest::result(c, rejected_result);
    }
    alt_role = u32(flags) & bit(0x17);
    compare(c, i32(u32(alt_role)), 0);
    if (!alt_role) return std::nullopt;
    if (compare(c, counted(), 0) != 0) {
        c.set({{3, c.u32(c.toc(-0x7b7c))}, {4, person}});
        c.call(0x10082f20, 0x10212e20);
        const u32 found = c.r(3);
        c.set(4, c.toc(-0x7b78));
        compare_logical(c, found, 0);
        c.put32(c.r(4), found);
        u32 found_club = 0;
        if (found) {
            const u32 clubs = c.u32(c.toc(-0x7be8));
            c.set(3, clubs);
            found_club = clubs + c.u32(found + 4) * 0x248;
        }
        compare_logical(c, found_club, club);
        if (found_club == club) {
            const i32 quotient = divw(i32(reg(i32(u32(total))) * 25), counted());
            const u32 own = c.u32(person + appraisal::person::player);
            c.put16(at(at::estimate), reg(quotient) + reg(c.i16(own + 6) * 25));
        } else {
            c.put16(at(at::estimate), reg(divw(i32(reg(i32(u32(total))) * 50), counted())));
        }
    }
    c.set({{3, c.u32(at(at::judge))}, {4, person}, {7, reg(c.i16(at(at::estimate)))}, {5, club}, {6, 1}});
    match_man::call(c, appraisal::callee::judged_ability, 0x10212ea8);
    i32 judged = i32(c.r(3));
    const i32 earlier = c.i16(at(at::judged));
    if (compare(c, earlier, -1) > 0 && compare(c, s16(c.r(3)), earlier) < 0) judged = earlier;
    // Attributes of the person's own player data lower the estimate (a low 0x1b, a low 0x36).
    const u32 own = c.u32(person + appraisal::person::player);
    c.set(4, own);
    if (nonzero_logical(c, own)) {
        const i32 first = s8(c.u8(own + 0x1b));
        if (compare(c, first, 6) < 0) judged -= 10;
        else if (compare(c, first, 0xa) < 0) judged -= 5;
        const i32 second_value = s8(c.u8(own + 0x36));
        if (compare(c, second_value, 6) < 0) judged -= 15;
        else if (compare(c, second_value, 0xa) < 0) judged -= 10;
    }
    c.set(3, reg(judged));
    const bool checked = compare(c, s8(c.u8(at(at::has_floor))), 0) != 0;
    count = u32(count) + 1;
    total = u32(total) + reg(s16(reg(judged)));
    if (checked && floor_fails(true)) return guest::result(c, rejected_result);
    return std::nullopt;
}

// total += (the count) * (a * x - b) truncated, the count's words at 0x90 and the result at 0x98;
// `single` for the single-precision forms, `mixed_in_f1` when f1 keeps the multiply-subtract.
void Placement::add_scaled(u32 site, double a, double x, double b, bool single, bool mixed_in_f1) {
    c.set(3, reg(counted()) ^ 0x80000000u);
    const double pair = store_pair(c, at(0x90), counted());
    const double mixed = fpu.multiply_add(site, a, x, b);
    const double count_value = single ? fpu.fsubs(pair, conversion()) : fpu.fsub(pair, conversion());
    const double product = single ? fpu.fmuls(mixed, count_value) : fpu.fmul(mixed, count_value);
    c.set_f(1, mixed_in_f1 ? mixed : conversion());
    total = u32(total) + reg(s8(truncate_at(c, fpu, at(0x98), product)));
}

// The noise mixed with the person's age in days: (0x2f8 * (0x300 - age) + noise * age) / 0x300.
double Placement::narrow(u32 site, double noise, i32 days) {
    const double first = fpu.fsubs(store_pair(c, at(0x90), days), conversion());
    const double second_value = fpu.fsubs(store_pair(c, at(0x98), days), conversion());
    const double flipped = fpu.fsub(D(0x300), first);
    c.set_f(1, flipped);
    const double mixed = fpu.fmuls(noise, second_value);
    return fpu.frsp(fpu.fdiv(fpu.multiply_add(site, D(0x2f8), flipped, mixed), D(0x300)));
}

// The world's measure of the player's development (0x10303090) added in proportion to the count.
void Placement::trends() {
    if (!wants(bit(0xd)) || !in_people_table()) return;
    c.set({{3, c.u32(world())}, {4, data}});
    c.call(0x10303090, 0x10212f84);
    const i32 trend = s8(c.r(3));
    c.set({{3, 0x43300000}, {4, reg(trend) ^ 0x80000000u}});
    const double trend_value = fpu.fsub(store_pair(c, at(0x90), trend), conversion());
    c.set_f(1, trend_value);
    double f31 = fpu.frsp(fpu.fdiv(trend_value, D(0x100)));
    c.set_f(31, f31);
    compare_logical(c, club, c.u32(person + appraisal::person::club));
    if (inside_window({0x50, 0x70, 0x48, 0x10213030})) {
        const i32 days = appraisal::age_in_days(c, person);
        const u32 today = c.toc(appraisal::slot::date);
        c.set({{3, reg(days)}, {4, 0x43300000}, {5, reg(c.i16(person + appraisal::person::birth_day))},
               {6, c.u16(today + appraisal::date_year) - c.u16(person + appraisal::person::birth_year)}});
        f31 = narrow(0x102130ec, f31, days);
    }
    c.set_f(31, f31);
    if (fcompare(fpu, f31, D(0x308)).greater) add_scaled(0x10213128, D(0x310), f31, D(0x318), false, false);
    else if (fcompare(fpu, f31, D(0x178)).greater) add_scaled(0x1021317c, D(0x320), f31, D(0x328), false, false);
}

// The judge's feel for the player's form (0x10303560, in three measures).
void Placement::form() {
    if (!wants(bit(0x1e)) || !in_people_table()) return;
    double f1 = F(0x20);
    c.set_f(1, f1);
    compare(c, counted(), 0);
    // The judge's own estimate over 150: 0x20 plus the excess over 0x24, capped by 0x330 (single).
    const auto bounded = [&](i32 excess) {
        const double scaled = fpu.fdivs(fpu.fsubs(store_pair(c, at(0x90), excess), conversion()), F(0x24));
        const double sum = fpu.fadds(F(0x20), scaled);
        double capped = D(0x330);
        if (!fcompare(fpu, capped, sum).less) capped = sum;
        f1 = fpu.frsp(capped);
        c.set_f(1, f1);
    };
    bool measured = false;
    if (counted() != 0) {
        const i32 average = divw(i32(u32(total)), counted());
        compare(c, average, 0x96);
        if (average > 0x96) {
            bounded(average - 0x96);
            measured = true;
        }
    }
    if (!measured) {
        const i32 estimate = c.i16(at(at::estimate));
        compare(c, estimate, 0x96);
        if (estimate > 0x96) bounded(estimate - 0x96);
    }
    c.set({{3, c.u32(world())}, {4, data}, {5, 6}, {6, 0x12}});
    c.call(0x10303560, 0x1021329c);
    double f31 = c.f(1);
    c.set_f(31, f31);
    if (compare(c, s8(age), 0x21) > 0) {
        // Past 33 the form can improve by the world's own ratio (+0x10 over +0x18, or 1 for none).
        const u32 object = c.u32(world());
        const double length = c.f64(object + 0x18);
        const auto ratio = [&] {
            return fcompare(fpu, length, D(0x178)).equal ? D(0x308) : fpu.fdiv(c.f64(object + 0x10), length);
        };
        const double shift = D(0x338);
        c.set_f(1, shift);
        if (fcompare(fpu, f31, fpu.fadd(shift, ratio())).greater) {
            f31 = fpu.frsp(fpu.fadd(shift, ratio()));
            c.set_f(31, f31);
        }
    }
    compare_logical(c, club, c.u32(person + appraisal::person::club));
    if (inside_window({0x40, 0x68, 0x38, 0x10213384})) {
        const i32 days = appraisal::age_in_days(c, person);
        const u32 today = c.toc(appraisal::slot::date);
        c.set({{3, reg(magic_div(s8(c.u8(c.u32(person + appraisal::person::player) + 0xf)), 0x66666667, 2))}, {4, reg(days) ^ 0x80000000u},
               {5, c.u32(person + appraisal::person::player)}, {6, 0x43300000}, {7, reg(c.i16(today + appraisal::date_day))},
               {8, reg(c.i16(person + appraisal::person::birth_day))}, {9, reg(days)}});
        f31 = narrow(0x1021346c, f31, days);
        c.set_f(31, f31);
        // Goalkeepers peak later: the threshold age is 34 plus a tenth of the goalkeeper rating (read through
        // the person's own player pointer here).
        const i32 threshold = magic_div(s8(c.u8(c.u32(person + appraisal::person::player) + 0xf)), 0x66666667, 2) + 0x22;  // / 10
        if (compare(c, s8(age), threshold) > 0) {
            const u32 object = c.u32(world());
            const double length = c.f64(object + 0x18);
            const auto ratio = [&] {
                return fcompare(fpu, length, D(0x178)).equal ? D(0x308) : fpu.fdiv(c.f64(object + 0x10), length);
            };
            c.set_f(1, length);
            if (fcompare(fpu, f31, ratio()).greater) {
                const double single_ratio = fpu.frsp(ratio());
                c.set_f(1, single_ratio);
                f31 = fpu.multiply_add(0x102134e0, f31, F(0x28), fpu.fmuls(single_ratio, F(0x28)));
                c.set_f(31, f31);
            }
        }
    }
    if (fcompare(fpu, f31, D(0x340)).greater) add_scaled(0x10213518, F(0x2c), f31, F(0x30), true, false);
    else if (fcompare(fpu, f31, D(0x308)).greater) add_scaled(0x1021356c, D(0x348), f31, D(0x248), false, false);
    else if (fcompare(fpu, f31, D(0x178)).greater) add_scaled(0x102135c4, F(0x2c), f31, F(0x30), true, false);
    // The club's own player (or a countryman of its club's) is judged again with the club's bias.
    if (!nonzero_logical(c, club)) return;
    const u32 person_club = c.u32(person + appraisal::person::club);
    c.set(4, person_club);
    compare_logical(c, club, person_club);
    if (club != person_club) {
        if (!nonzero_logical(c, person_club)) return;
        const u32 nation_a = c.u32(club + appraisal::club::nation), nation_b = c.u32(person_club + appraisal::club::nation);
        c.set(3, nation_a);
        compare_logical(c, nation_a, nation_b);
        if (nation_a != nation_b) return;
    }
    f31 = fpu.frsp(fpu.fmul(D(0x198), f31));
    c.set_f(31, f31);
    c.set_f(1, f31);
    c.set({{3, c.u32(world())}, {4, data}, {5, 1}, {6, 0x12}});
    c.call(0x10303560, 0x10213638);
    const double measure = c.f(1);
    if (fcompare(fpu, measure, D(0x308)).greater) {
        add_scaled(0x10213668, D(0x320), measure, D(0x350), false, true);
    } else if (fcompare(fpu, measure, f31).less) {
        c.set(3, reg(counted()) ^ 0x80000000u);
        const double count_pair = store_pair(c, at(0x90), counted());
        const double scaled = fpu.fdivs(fpu.fmuls(F(0x34), measure), f31);
        const double count_value = fpu.fsubs(count_pair, conversion());
        const double excess = fpu.fsubs(scaled, F(0x34));
        c.set_f(1, count_value);
        total = u32(total) + reg(s8(truncate_at(c, fpu, at(0x98), fpu.fmuls(count_value, excess))));
    }
    c.set_f(1, F(0x38));
    c.set({{3, c.u32(world())}, {4, data}, {5, 6}, {6, 0x14}});
    c.call(0x10303560, 0x10213700);
    c.set(3, reg(counted()) ^ 0x80000000u);
    const double mixed = fpu.multiply_add(0x10213718, D(0xe0), c.f(1), D(0x358));
    const double count_value = fpu.fsubs(store_pair(c, at(0x90), counted()), conversion());
    c.set_f(1, conversion());
    const double narrowed = fpu.frsp(mixed);
    total = u32(total) + truncate_at(c, fpu, at(0x98), fpu.fmuls(narrowed, count_value));
}

// The average of the judged abilities becomes the modifier: beyond 150 each point counts more (the
// pool's 0x20 per point over); then flag 8 adds the player's attribute 0x45 above ten.
void Placement::overall() {
    if (compare(c, counted(), 0) != 0) {
        const i32 average = divw(i32(u32(total)), counted());
        modifier = reg(average);
        if (compare(c, average, 0x96) > 0) {
            c.set(3, reg(average - 0x96) ^ 0x80000000u);
            const double excess = fpu.fsub(store_pair(c, at(0x90), average - 0x96), conversion());
            c.set_f(1, excess);
            modifier = truncate_at(c, fpu, at(0x98), fpu.multiply_add(0x10213784, D(0x20), excess, D(0x360)));
        }
    }
    if (wants(bit(0x1c)) && in_people_table()) {
        c.set(3, reg(stat(0x45)));
        const double rating = fpu.fsub(store_pair(c, at(0x90), stat(0x45) - 10), conversion());
        c.set_f(1, conversion());
        modifier = u32(modifier) + truncate_at(c, fpu, at(0x98), fpu.fmul(D(0x290), rating));
    }
}
}

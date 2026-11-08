// 0x10211e70: the player's worth on the market, and his fit with the judge's staff by role group
// (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace cm::aot::match_man::placement {

// The game's own opinion of the person's worth (0x10187650, the person's id): a high one lowers `worth`.
i32 Placement::ask_opinion(u32 back) {
    c.set({{3, c.u32(game())}, {4, c.u32(person)}});
    c.call(0x10187650, back);
    return s16(c.r(3));
}

// The market's view: `worth` from the match form (a person on the side) or the limit, lowered by the
// game's opinion, then the worth turns the modifier (or, when low, ends the routine with the position
// scaled); a person withdrawn from the side (condition -2) ends it with -1000000.
std::optional<i32> Placement::market() {
    constexpr u32 withdrawn = reg(-1000000);
    if (compare(c, c.i32(at(at::flag_800)), 0) == 0) {
        if (nonzero_logical(c, record)) {
            const i32 mark = s8(c.u8(record + 0x19));
            compare(c, mark, -2);
            if (mark == -2) return guest::result(c, withdrawn);
            const double per_minute = double(c.f32(record + 0x5c));
            c.set_f(1, per_minute);
            const double hours = fpu.multiply_add(0x102150fc, F(0x10), per_minute, double(c.f32(record + 0x50)));
            const u32 match = c.u32(record + 0x1a4);
            const i32 clock = c.i16(match + 0x794c);
            worth = reg(s16(truncate_at(c, fpu, at(0x80), hours)));
            if (compare(c, clock, 0x1ef) == 0) {
                const i32 raised = i32(worth) + 0x5dc;
                worth = reg(raised > 0x2710 ? 0x2710 : raised);
            }
        } else {
            worth = reg(c.i16(at(at::limit)));
        }
        if (!wants(bit(0xa))) {
            if (compare(c, ask_opinion(0x10215154), 0x3e8) > 0) worth = u32(worth) - reg(ask_opinion(0x10215170) - 0x1f4);
        }
        if (compare(c, i32(worth), 0x1676) < 0) {
            const i32 scaled = i32(reg(i32(u32(position))) * 100);
            const i32 result = scaled + i32(worth) - 0x61a8;
            compare(c, i32(worth), 0xdac);
            if (i32(worth) >= 0xdac) return guest::result(c, reg(result));
            return guest::result(c, reg(result - 0x80000 + 0x5ee0));
        }
        if (place_has(0x8000, Test::first_high) || (place_has(0x2000, Test::second_high) && extra_has(0x100, false))) {
            const i32 value = i32(worth);
            const double pair = store_pair(c, at(0x80), value);
            store_pair(c, at(0x88), value);
            const double ratio = fpu.fdiv(fpu.fsub(pair, conversion()), D(0x140));
            c.set_f(1, ratio);
            worth = truncate_at(c, fpu, at(0x90), fpu.fmul(ratio, fpu.fsub(c.f64(at(0x88)), conversion())));
        }
        if (compare(c, i32(worth), 0x2328) < 0) {
            modifier = u32(modifier) - reg(0x118 - magic_div(i32(worth) * 3, 0x51eb851f, 5));  // / 100
        } else {
            modifier = u32(modifier) - reg(magic_div(10000 - i32(worth), 0x51eb851f, 5));  // / 100
        }
        c.set({{3, c.u32(game())}, {4, person}});
        c.call(0x1018c340, 0x10215290);
        if (nonzero_logical(c, c.r(3))) {
            const i32 injury = c.i16(c.r(3) + 0x30);
            c.set(3, reg(injury));
            compare(c, injury, 0xa);
            modifier = u32(modifier) - reg(injury > 0xa ? injury + 0x14 : injury * 2);
        }
    }
    if (wants(bit(0xa))) {
        worth = reg(c.i16(at(at::ceiling)));
        bool lowered = false;
        if (compare(c, i32(worth), 0x63) >= 0 && compare(c, stat(0x3c), 0xe) > 0 && compare(c, stat(0x2d), 8) < 0) {
            if (compare(c, ask_opinion(0x102152fc), 0x1f4) < 0) {
                modifier = u32(modifier) - 200;
                lowered = true;
            }
        }
        if (!lowered) modifier = u32(modifier) + reg(100 - i32(worth));
    }
    return std::nullopt;
}

// The player's fit with the judge's own staff (flag 0x1000, a role value of at least 100): a few small
// adjustments by the judge's coaching bytes, 0x100da050 / 0x100da0b0 scaling the modifier, then
// 0x100d9580 / 0x100d9520 and the role groups.
void Placement::staff_fit() {
    if (!wants(bit(0x13)) || !nonzero_logical(c, staff) || !nonzero_logical(c, club) || compare(c, i32(u32(position)), 0x64) < 0) return;
    if (compare(c, s8(age), 0x17) <= 0) modifier = u32(modifier) + scaled_term(0x80, staff_stat(0x42) - 10, 0x20, 0x88);
    c.set({{3, person}, {4, club}});
    c.call(0x100da050, 0x1021538c);
    if (compare(c, s8(c.r(3)), 0) != 0) {
        // Held below 0x190 times itself or ten under (the smaller wins).
        const i32 value = i32(u32(modifier));
        const double first = fpu.fsub(store_pair(c, at(0x88), value), conversion());
        store_pair(c, at(0x80), value - 10);
        double chosen = fpu.fmul(D(0x190), first);
        const double shifted = fpu.fsub(c.f64(at(0x80)), conversion());
        c.set_f(1, chosen);
        if (fcompare(fpu, shifted, chosen).less) {
            store_pair(c, at(0x90), value - 10);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
            c.set_f(1, chosen);
        }
        modifier = truncate_at(c, fpu, at(0x98), chosen);
    } else {
        c.set({{3, person}, {4, club}});
        c.call(0x100da0b0, 0x10215404);
        if (compare(c, s8(c.r(3)), 0) != 0) {
            const i32 value = i32(u32(modifier));
            const double first = fpu.fsub(store_pair(c, at(0x88), value), conversion());
            store_pair(c, at(0x80), value + 10);
            double chosen = fpu.fmul(D(0x40), first);
            const double shifted = fpu.fsub(c.f64(at(0x80)), conversion());
            c.set_f(1, chosen);
            if (fcompare(fpu, shifted, chosen).greater) {
                store_pair(c, at(0x90), value + 10);
                chosen = fpu.fsub(c.f64(at(0x90)), conversion());
                c.set_f(1, chosen);
            }
            modifier = truncate_at(c, fpu, at(0x98), chosen);
        }
    }
    c.set({{3, person}, {4, judge_arg}});
    c.call(0x100d9580, 0x10215478);
    if (compare(c, s8(c.r(3)), 0) != 0) {
        modifier = u32(modifier) - 0x32;
    } else {
        c.set({{3, person}, {4, judge_arg}});
        c.call(0x100d9520, 0x10215494);
        if (compare(c, s8(c.r(3)), 0) != 0) {
            // 0x180 times the modifier, or twenty above it (the larger wins).
            const i32 value = i32(u32(modifier));
            const double first = fpu.fsub(store_pair(c, at(0x80), value), conversion());
            const double raised_pair = store_pair(c, at(0x88), value + 0x14);
            double chosen = fpu.fmul(D(0x180), first);
            const double raised = fpu.fsub(raised_pair, conversion());
            c.set_f(1, chosen);
            if (!fcompare(fpu, chosen, raised).greater) {
                store_pair(c, at(0x90), value + 0x14);
                chosen = fpu.fsub(c.f64(at(0x90)), conversion());
                c.set_f(1, chosen);
            }
            modifier = truncate_at(c, fpu, at(0x98), chosen);
        }
    }
    fit_groups();
    after_groups();
}

// The judge's coaching bytes (staff +0x2e..+0x3f) against the player's ratings, by the role's group
// (2 and 4, 8, 0x10, 0x20, 0x40; goalkeepers none): `age` (r23) becomes the sum of the group's terms,
// each a product over a constant (the compiler's multiply-high divisions, exact for the signed bytes).
// The compiler's register use leaves the later code r20 (the group's last working value), r21, r22 (the
// rating 0x44), r24 (1 unless the role has 0x80 or 0x800), r25, r27 and r14 (the role).
void Placement::fit_groups() {
    // mulhw by a magic constant, and the sign bit the compiler adds after the shift (rlwinm 1,31,31).
    const auto mh = [](u32 magic, i32 value) { return appraisal::mulhw(i32(magic), value); };
    const auto sgn = [](i32 value) -> u32 { return value < 0 ? 1u : 0u; };
    const i32 d44 = stat(0x44), x = d44 - 10;
    alt_role = reg(d44);
    judge_arg = reg(x);
    const i32 s34 = staff_stat(0x34);
    const auto capped = [](i32 value) { return value >= 10 ? 10 : value; };  // `cmpwi r3,0xa; bge`: keep the 10
    const i32 t1 = capped(s34 * x / 3);
    const i32 t2 = capped(s34 * (s8(c.u8(person + appraisal::person::byte_67)) - 10) / 3);
    const u32 judge = c.u32(at(at::judge));
    const i32 judge_byte = s8(c.u8(judge + 0x65)), product3 = judge_byte * (s8(c.u8(person + 0x65)) - 10);
    const i32 t3 = capped(product3 / 3);
    // The head's scratch: r5 the last capped term, r6 the judge's raw byte, r7 the multiplier, r8 the second term.
    c.set({{3, reg(appraisal::mulhw(0x55555556, product3))}, {4, reg(judge_byte)}, {5, reg(t3)}, {6, c.u8(judge + 0x65)}, {7, 0x55555556}, {8, reg(t2)}});
    const i32 role = c.i16(at(at::role));
    age = reg(t1 + t2 + t3);
    adjust = reg(role);
    if (role & 1) return;
    const i32 flag = role & (bit(0x14) | bit(0x18)) ? 0 : 1;
    total = reg(flag);
    const i32 s3a = staff_stat(0x3a), s3f = staff_stat(0x3f), s2e = staff_stat(0x2e);
    const i32 d32 = stat(0x32), d27 = stat(0x27), d28 = stat(0x28), d26 = stat(0x26), d43 = stat(0x43);
    i32 sum = i32(u32(age)) - 10;
    const auto add_sum = [&](i32 term) { sum += term; };
    if (role & (4 | 2)) {
        add_sum(s3a * d32 / 40);
        add_sum(s3f * x / 8);
        add_sum(s2e * d27 / 350);
        add_sum(s2e * (d28 - 10) / 10);
        add_sum(s2e * d26 / 500);
        add_sum(s2e * d43 / 250);
        if (flag) sum += stat(0x2e) - 0xf;
        c.set({{4, reg(mh(0x10624dd3, s2e * d26) >> 5)}, {5, reg(s2e * (d28 - 10) / 10)}, {6, reg(mh(0x66666667, s2e * (d28 - 10)) >> 2)},
               {7, reg(s2e * d27 / 350)}, {8, sgn(mh(0x66666667, s3a * d32) >> 4)}, {9, 0x66666667}, {10, reg(s2e)}});
    } else if (role & 8) {
        second_word = reg(s2e);
        count = reg(d43);
        second = 0x1b4f0000;
        add_sum(s3a * d32 / 40);
        add_sum(s3f * x / 6);
        add_sum(d27 / 15);
        add_sum(s2e * (d28 - 10) / 8);
        add_sum(s2e * d26 / 325);
        add_sum(s2e * d43 / 150);
        if (flag) sum += d44 - 0xf + stat(0x3e) / 15;
        c.set({{4, sgn(mh(0x64d319ff, s2e * d26) >> 7)}, {5, reg(s2e * (d28 - 10) / 8)}, {6, reg(s2e * d43)}, {7, 0x88888889},
               {8, reg(s2e * d26)}, {9, reg(s3f * x / 6)}, {10, reg(mh(0x2aaaaaab, s3f * x))}});
    } else if (role & 0x10) {
        second_word = reg(s3a * d32 / 60);
        judge_arg = reg(flag);
        add_sum(s3a * d32 / 60);
        add_sum(s3f * x / 6);
        add_sum(d27 / 10);
        add_sum(s2e * (d28 - 10) / 5);
        add_sum(s2e * d26 / 150);
        add_sum(s2e * d43 / 100);
        c.set({{4, sgn(mh(0x51eb851f, s2e * d43) >> 5)}, {5, reg(mh(0x1b4e81b5, s2e * d26) >> 4)}, {6, reg(s2e * (d28 - 10) / 5)}, {7, 0x1b4e81b5},
               {8, reg(mh(0x66666667, s2e * (d28 - 10)) >> 1)}, {9, reg(d27 / 10)}, {10, 0x66666667}});
        if (flag) {
            sum += d44 - 0xf + stat(0x3e) / 15 + stat(0x37) / 15;
            c.set({{4, reg(stat(0x3e) / 15)}, {5, reg(mh(0x88888889, stat(0x3e)))}});
        } else {
            sum += stat(0x23) / 15;
        }
    } else if (role & 0x20) {
        add_sum(s3a * d32 / 150);
        add_sum(s3f * x / 8);
        add_sum(d27 / 8);
        add_sum(s2e * (d28 - 10) / 4);
        add_sum(s2e * d26 / 150);
        add_sum(s2e * d43 / 75);
        c.set({{4, reg(d43)}, {5, sgn(mh(0x1b4e81b5, s2e * d26) >> 4)}, {6, reg(s2e * d26)}, {7, reg(s2e * (d28 - 10) / 4)}, {8, reg(d27 >> 3)},
               {9, reg(s3f * x / 8)}, {10, reg(mh(0x1b4e81b5, s3a * d32) >> 4)}});
        if (flag) {
            sum += stat(0x1a) - 0xf + d43 / 15;
            c.set(5, stat_raw(0x1a));
        } else {
            sum += stat(0x23) / 15;
        }
    } else if (role & 0x40) {
        const i32 quotient_before = appraisal::mulhw(0x51eb851f, s3a * d32) >> 7;
        second_word = u32(quotient_before < 0 ? 1 : 0);
        judge_arg = reg(s2e * (d28 - 10));
        add_sum(s3a * d32 / 400);
        add_sum(s3f * x / 10);
        add_sum(d27 / 5);
        add_sum(s2e * (d28 - 10) / 3);
        add_sum(s2e * d26 / 175);
        add_sum(s2e * d43 / 200);
        c.set({{4, sgn(mh(0x5d9f7391, s2e * d26) >> 6)}, {5, reg(s2e * d43)}, {6, reg(s2e * (d28 - 10) / 3)}, {7, reg(d27)}, {8, reg(d26)},
               {9, reg(d27 / 5)}, {10, 0x5d9f7391}});
        if (flag) {
            add_sum(d27 / 15);
            add_sum(stat(0x33) / 15);
            sum += stat(0x40) - 10 + stat(0x36) - 10;
            c.set({{4, reg(stat(0x33) / 15)}, {5, sgn((mh(0x88888889, stat(0x33)) + stat(0x33)) >> 3)}, {6, reg(d27 / 15)},
                   {7, sgn((mh(0x88888889, d27) + d27) >> 3)}, {9, c.u8(data + 0x36)}});
            if (d26 > 0x1e && stat(0x36) > 0xf) {
                add_sum(stat(0x1b) * ((d26 / 3) * stat(0x36)) / 1500);
                c.set({{4, reg(mh(0x55555556, d26))}, {5, reg(stat(0x1b))}, {6, 0x057619f1}});
            }
        }
    } else {
        return;
    }
    age = reg(sum);
}

// After the groups: the player's own flag 0x40 with a role of 0x40 adds the judge's byte 0x33 over the
// two ratings, and the sum scales the modifier, bounded to -80..80.
void Placement::after_groups() {
    const i32 role = c.i16(at(at::role));
    if (compare(c, i32(c.u32(at(at::player_flags)) & 0x40), 0) != 0 && compare(c, role & 0x40, 0) != 0) {
        const i32 product = stat(0x2e) * stat(0x36);
        const i32 scaled = staff_stat(0x33) * (product - 200);
        c.set({{3, u32(appraisal::mulhw(0x10624dd3, scaled) >> 5 < 0 ? 1 : 0)}, {4, reg(staff_stat(0x33))}, {5, 0x10624dd3}});
        age = u32(age) + reg(scaled / 500);
    }
    if (compare(c, i32(u32(age)), 0) == 0) return;
    const double modifier_value = fpu.fsub(store_pair(c, at(0x88), i32(u32(modifier))), conversion());
    const double sum_pair = store_pair(c, at(0x80), i32(u32(age)));
    const double scaled = fpu.multiply_add(0x10215d0c, D(0x400), modifier_value, D(0x0));
    const double sum_value = fpu.fsub(sum_pair, conversion());
    c.set_f(1, sum_value);
    const i32 limit = i32(truncate_at(c, fpu, at(0x90), fpu.fmul(sum_value, scaled)));
    c.set(3, reg(limit));
    compare(c, limit, -0x50);
    i32 clamped = -0x50;
    if (limit >= -0x50) {
        compare(c, limit, 0x50);
        clamped = limit > 0x50 ? 0x50 : limit;
    }
    c.set(0, reg(clamped));
    modifier = u32(modifier) + reg(clamped);
}
}

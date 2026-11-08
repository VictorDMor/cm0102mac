// 0x10211e70: the value of the place's role (and of the alternative role in the place's rating
// halfword), match_man_rating_c_detail.hpp.
#include "match_man_rating_c_detail.hpp"
#include <algorithm>

namespace cm::aot::match_man::placement {
// The table of values by rating (TOC -0x5288) at the signed byte `index`.
static u32 value_at(Cpu& c, i32 index) { return appraisal::value_of_rating(c, c.toc(-0x5288), index); }

// The first role's position value, added to `position`: goalkeepers by the square of their rating,
// outfield roles by a rating (or the best of a few), with the player's flags (0xd4) deciding a bad fit.
// Roles are bit masks: 0x800 and 0x80 (the original tests them as bits 0x14 and 0x18) choose the
// sweeper-like rating of a pair.
void Placement::position_value() {
    const i32 role = c.i16(at(at::role));
    const i32 flags_word = i32(c.u32(at(at::player_flags)));
    const u32 table = c.toc(-0x5288);
    // The scratch the original leaves (r3 reloads the table's address after each lookup, r4 holds the role from the
    // first of the later tests on; each case adds what its ratings were loaded into).
    c.set(3, u32(role) & 1);
    if (role & 1) {
        const i32 keeping = stat(0xf);
        position = reg(keeping * keeping) + u32(position) - 0x12c;
        return;
    }
    c.set(3, u32(flags_word) & 1);
    if (flags_word & 1) {
        position = u32(position) - 200;
        return;
    }
    const auto add = [&](i32 index) { position = u32(position) + value_at(c, index); };
    c.set(3, table);
    if (role & 2) { add(stat(0x10)); return; }
    if (role & 4) { add(stat(0x11)); return; }
    c.set(4, reg(role));
    if (role == 8) {
        // The best of four neighbouring ratings.
        u32 best = stat_raw(0x16), raw = 0;
        for (const u32 offset : {0x11u, 0x12u, 0x13u}) {
            raw = stat_raw(offset);
            if (s8(raw) > s8(best)) best = raw;
        }
        c.set({{4, reg(s8(raw))}, {5, raw}});
        add(s8(best));
    } else if (role & 8) {
        add(role & (bit(0x14) | bit(0x18)) ? stat(0x16) : stat(0x12));
    } else if (role & 0x10) {
        if (role & (bit(0x14) | bit(0x18))) {
            const u32 first = stat_raw(0x13), second_raw = stat_raw(0x14);
            const u32 chosen = s8(second_raw) > s8(first) ? second_raw : first;
            c.set({{4, second_raw}, {5, chosen}});
            add(s8(chosen));
        } else if (role == 0x10) {
            const u32 low = stat_raw(0x12), high = stat_raw(0x14), third = stat_raw(0x13);
            const u32 best = s8(high) > s8(low) ? high : low;
            const u32 result = s8(best) > s8(third) ? (s8(high) > s8(low) ? high : low) : third;
            c.set({{4, best}, {5, high}, {6, result}, {7, third}});
            add(s8(result));
        } else {
            // The wing-back case: the last two ratings count five less.
            const i32 a = stat(0x14) - 5, b = stat(0x13);
            const i32 best = a > b ? a : b;
            c.set({{4, reg(best)}, {5, reg(a)}, {6, reg(wing_back())}});
            add(s8(reg(wing_back())));
        }
    } else if (role & 0x20) {
        if (role & (bit(0x14) | bit(0x18))) {
            add(stat(0x14));
        } else {
            c.set(4, reg(stat(0x14)) ^ 0x80000000u);
            add(average_type(0xa0, 0xa8, 0x98, 0x90));
        }
    } else if (role & 0x40) {
        add(stat(0x15));
    } else {
        c.set(3, u32(role) & 0x20);
        position = u32(position) + 100;
    }
}

// The wing-back rating: the best of (rating 0x14 less 5), rating 0x13 and (rating 0x12 less 5), as a
// signed byte.
i32 Placement::wing_back() const {
    const i32 a = stat(0x14) - 5, b = stat(0x13), d = stat(0x12) - 5;
    i32 best = b;
    if (a > b) best = a;
    i32 result;
    if (best > d) result = a > b ? a : b;
    else result = d;
    return result;  // not sign-extended (the table lookup extends it)
}

// The average-type value: the greater of the rating 0x14 and the pool's factor times the sum of the
// ratings 0x14 and 0x1a, truncated. The int-to-double words go through the stack slots the original
// uses: the sum's pair, the rating's pair, the rating's pair again, and the result's.
i32 Placement::average_type(u32 sum_at, u32 first_at, u32 again_at, u32 result_at) {
    const i32 first = stat(0x14), sum = first + stat(0x1a);
    const double sum_value = fpu.fsub(store_pair(c, at(sum_at), sum), conversion());
    const double scaled = fpu.fmul(D(0x210), sum_value);
    const double first_value = fpu.fsub(store_pair(c, at(first_at), first), conversion());
    double chosen = scaled;
    if (fcompare(fpu, first_value, scaled).greater) {
        store_pair(c, at(again_at), first);
        chosen = fpu.fsub(c.f64(at(again_at)), conversion());
    }
    c.set_f(1, chosen);
    return i32(truncate_at(c, fpu, at(result_at), chosen));
}

// The alternative role (the place's rating halfword, when it is not 0 and not the first role) gets its
// own value, assigned to `second`: fewer cases than the first (no best-of-four, one wing-back form).
void Placement::position_second() {
    const i32 role = s16(alt_role);
    const i32 flags_word = i32(c.u32(at(at::player_flags)));
    const u32 table = c.toc(-0x5288);
    if (role & 1) {
        const i32 keeping = stat(0xf);
        c.set(3, reg(keeping * keeping));
        second = reg(keeping * keeping - 0x12c);
    } else if (flags_word & 1) {
        second = reg(-200);
    } else if (role & 2) {
        c.set(3, table);
        second = value_at(c, stat(0x10));
    } else if (role & 4) {
        c.set(3, table);
        second = value_at(c, stat(0x11));
    } else if (role & 8) {
        c.set(3, table);
        second = value_at(c, role & (bit(0x14) | bit(0x18)) ? stat(0x16) : stat(0x12));
    } else if (role & 0x10) {
        const i32 a = stat(0x14) - 5, b = stat(0x13);
        c.set({{3, table}, {4, reg(a > b ? a : b)}, {5, reg(a)}, {6, reg(wing_back())}});
        second = value_at(c, s8(reg(wing_back())));
    } else if (role & 0x20) {
        c.set(3, table);
        if (role & (bit(0x14) | bit(0x18))) {
            second = value_at(c, stat(0x14));
        } else {
            c.set(4, reg(stat(0x14)) ^ 0x80000000u);
            second = value_at(c, average_type(0x98, 0x90, 0xa0, 0xa8));
        }
    } else if (role & 0x40) {
        c.set(3, table);
        second = value_at(c, stat(0x15));
    } else {
        second = 100;
    }
}

// What the role's upper bits add to a position value: the rating above ten in tens (some roles take the
// better of half of one rating and another, plus a third rating, less 20), or 100 for none of them.
i32 Placement::role_bonus(i32 role) {
    const auto above_ten = [](i32 rating) { return (rating - 10) * 10; };
    if (role & bit(0x13)) { c.set(3, reg(stat(0x1a))); return above_ten(stat(0x1a)); }
    if (role & bit(0x18)) { c.set(3, reg(stat(0x18))); return above_ten(stat(0x18)); }
    if (role & bit(0x17)) {
        const i32 better = std::max(half(stat(0x18)), stat(0x19));
        c.set({{3, reg(stat(0x30))}, {4, reg(better)}});
        return above_ten(better) + stat(0x30) - 20;
    }
    if (role & bit(0x16)) { c.set(3, reg(stat(0x19))); return above_ten(stat(0x19)); }
    if (role & bit(0x15)) {
        const i32 better = std::max(half(stat(0x17)), stat(0x19));
        c.set({{3, reg(stat(0x3b))}, {4, reg(better)}});
        return above_ten(better) + stat(0x3b) - 20;
    }
    if (role & bit(0x14)) { c.set(3, reg(stat(0x17))); return above_ten(stat(0x17)); }
    return 100;
}
}

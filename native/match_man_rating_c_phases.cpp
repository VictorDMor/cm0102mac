// 0x10211e70: the prologue's checks, the context it loads and the first components of the estimate
// (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man::placement {

std::optional<i32> Placement::begin(u32 role, u32 side) {
    // The place's words and rating come from the caller's stack: r14 and r21 hold its two words, r22 the
    // halfword of the rating word (sign-extended).
    adjust = c.u32(at(at::place_first));
    second_word = c.u32(at(at::place_second));
    alt_role = reg(c.i16(at(at::place_rating) + 2));
    compare(c, i32(person), 0);  // or. r15,r3,r3
    c.put16(at(at::judged), 0xffff);
    c.put8(at(at::has_floor), 0);
    c.put16(at(at::limit), 10000);
    c.put16(at(at::role), role);
    c.put32(at(at::side), side);
    c.put16(at(at::ceiling), 100);
    if (!person) return guest::result(c, reg(rejected));
    if (!nonzero_logical(c, data)) return guest::result(c, reg(rejected));
    return std::nullopt;
}

void Placement::load_context() {
    const u32 lookups = c.u32(c.toc(-0x7b7c));
    age = reg(s8(u32(c.i16(person + appraisal::person::age))));
    if (nonzero_logical(c, lookups)) {
        c.set({{3, lookups}, {4, person}, {5, club}});
        c.call(0x10082f90, 0x10211f38);
        relation = c.r(3);
    } else {
        relation = 0;
    }
    const u32 anchor_flag = u32(flags) & component::anchor;
    compare(c, i32(anchor_flag), 0);
    c.put32(at(at::flag_800), anchor_flag);
    // The game's own limits (0x101878d0) are asked unless the anchor flag alone is set.
    bool ask = !anchor_flag;
    if (!ask) {
        ask = compare(c, i32(u32(flags) & bit(0xa)), 0) != 0;
        if (!ask) ask = compare(c, i32(u32(flags) & bit(0xf)), 0) != 0;
    }
    if (ask) {
        c.set({{3, c.u32(game())}, {4, person}, {5, 0}});
        c.call(0x101878d0, 0x10211f74);
        const i32 limit = s16(c.r(3));
        c.put16(at(at::limit), reg(limit));
        c.put16(at(at::ceiling), reg(magic_div(s16(c.u16(at(at::limit))), 0x51eb851f, 5)));  // / 100
    }
    // A human manager in the call (the last stack word) turns the floor into 0, in the caller's frame.
    if (nonzero_logical(c, c.u32(at(at::human)))) c.put32(at(at::floor), 0);
    if (compare(c, c.i32(at(at::floor)), -1) > 0) c.put8(at(at::has_floor), 1);
    // The judge: the side's (its record's +0x20, staff at +0x24), else the judge given, else the club's.
    const u32 side = c.u32(at(at::side));
    if (nonzero_logical(c, side)) {
        staff = c.u32(side + 0x24);
        c.put32(at(at::judge), c.u32(side + 0x20));
        c.set(3, side);
    } else if (nonzero_logical(c, judge_arg)) {
        staff = c.u32(judge_arg + appraisal::person::staff);
        c.put32(at(at::judge), judge_arg);
    } else {
        u32 chosen = 0;
        if (nonzero_logical(c, club)) {
            chosen = c.u32(club + appraisal::club::default_judge);
            c.set(3, chosen);
            if (!nonzero_logical(c, chosen)) chosen = 0;
        }
        staff = chosen ? c.u32(chosen + appraisal::person::staff) : 0;
        c.put32(at(at::judge), chosen);
    }
    c.set({{3, person}, {4, c.u32(at(at::judge))}, {5, 0}});
    c.call(appraisal::callee::estimate, 0x10212038);
    c.put16(at(at::estimate), c.r(3));
    c.set(3, data);
    c.call(appraisal::callee::rating_flags, 0x10212048);
    c.put32(at(at::player_flags), c.r(3));
    if (compare(c, s16(c.r(3)), 0) == 0) position = reg(-100);
}

// The alternative role (the place's rating halfword) counts when it is neither 0 nor the first role.
bool Placement::alt_applies() {
    if (compare(c, s16(alt_role), 0) == 0) return false;
    return compare(c, i32(u32(alt_role)), c.i16(at(at::role))) != 0;
}

// The value of the place's roles: the first role's value and bonus, the alternative role's own, blended
// as (2 * first + second) / 3. Flag 0x400 and 0x200 replace the first role's value and bonus by 100;
// the blend is skipped after the bonus's 100.
void Placement::position_values() {
    const i32 role = c.i16(at(at::role));
    if (wants(bit(0x15))) {
        position = u32(position) + 100;
    } else {
        position_value();
        if (alt_applies()) position_second();
    }
    if (wants(bit(0x16))) {
        position = u32(position) + 100;
        return;
    }
    c.set(3, reg(role));  // lha r3,0x186(r1)
    position = u32(position) + reg(role_bonus(role));
    if (!alt_applies()) return;
    second = u32(second) + reg(role_bonus(s16(alt_role)));
    // mulhw by 0x55555556 and the quotient's sign added: a division by 3.
    const i32 sum = i32(reg(i32(u32(position))) * 2 + u32(second));
    c.set(3, reg(appraisal::mulhw(0x55555556, sum)));
    position = reg(sum / 3);
}
}

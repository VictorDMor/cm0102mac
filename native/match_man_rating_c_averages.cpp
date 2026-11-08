// 0x10211e70: the components of the running estimate (`total` over `count`): the ratings, the match's
// form, the judge's own views, and the trends and form measures that scale it.
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man::placement {

// The club is the person's own and the judge's date lies within the window (the original's inlined
// date checks, copies and buffer at the slots given): the person is still developing.
bool Placement::inside_window(const DateSlots& slots) {
    return u32(club) == c.u32(person + appraisal::person::club) && nonzero_logical(c, judge_arg) &&
           judge_date_window(c, frame, person, judge_arg, slots);
}

std::optional<i32> Placement::averages() {
    constexpr u32 rejected_result = reg(rejected);
    if (wants(component::dated_current)) {
        count = 1;
        const i32 own = c.i16(data + 0xa);
        total = reg(magic_div(own, 0x51eb851f, 4));  // / 50
        c.set({{3, u32(appraisal::mulhw(0x51eb851f, own) >> 4 < 0 ? 1 : 0)}, {4, reg(own)}});
        compare_logical(c, club, c.u32(person + appraisal::person::club));
        if (inside_window({0x60, 0x78, 0x58, 0x102128f0})) {
            // Still developing: the estimate shrinks with the person's age in days.
            const i32 days = appraisal::age_in_days(c, person);
            const u32 today = c.toc(appraisal::slot::date);
            c.set({{3, reg(days)}, {4, 0x43300000}, {5, u32(total) ^ 0x80000000u}, {6, reg(c.i16(person + appraisal::person::birth_day))},
                   {7, c.u16(today + appraisal::date_year) - c.u16(person + appraisal::person::birth_year)}});
            const double age_value = fpu.fsubs(store_pair(c, at(0x98), days), conversion());
            const double total_before = store_pair(c, at(0x90), i32(u32(total)));
            const double factor = fpu.multiply_add(0x102129a0, D(0x2f0), age_value, D(0x2e8));
            const double rounded = fpu.frsp(factor);
            const double total_value = fpu.fsubs(total_before, conversion());
            c.set_f(1, total_value);
            total = truncate_at(c, fpu, at(0xa0), fpu.fmuls(total_value, rounded));
        }
        if (has_floor() && floor_fails(false)) return guest::result(c, rejected_result);
    }
    if (wants(bit(0x1b))) {
        const u32 nation = c.u32(person + appraisal::person::nation);
        const bool checked = compare(c, s8(c.u8(at(at::has_floor))), 0) != 0;
        count = u32(count) + 1;
        const i32 rating = c.i16(data + 0x8);
        c.set({{3, reg(rating) ^ 0x80000000u}, {4, 0x43300000}, {5, reg(rating)}});
        const double reputation = fpu.fsub(store_pair(c, at(0x98), c.i16(nation + 0xac)), conversion());
        const double ratio = fpu.fdiv(reputation, D(0x140));
        const double rating_value = fpu.fsub(store_pair(c, at(0x90), rating), conversion());
        const double divisor = fpu.fmul(D(0x1b8), fpu.fsub(D(0x78), ratio));
        c.set_f(1, rating_value);
        total = u32(total) + truncate_at(c, fpu, at(0xa0), fpu.fdiv(rating_value, divisor));
        if (checked && floor_fails(true)) return guest::result(c, rejected_result);
    }
    if (wants(bit(0x1a))) {
        const bool checked = compare(c, s8(c.u8(at(at::has_floor))), 0) != 0;
        count = u32(count) + 1;
        const i32 held = c.i16(data + 0xc);
        c.set({{3, u32(appraisal::mulhw(0x51eb851f, held) >> 4 < 0 ? 1 : 0)}, {4, reg(held)}});
        total = u32(total) + reg(magic_div(held, 0x51eb851f, 4));  // / 50
        if (checked && floor_fails(true)) return guest::result(c, rejected_result);
    }
    form_of_match();
    flag_4_measure();
    if (const auto early = judged_estimates()) return early;
    return std::nullopt;
}

// Flag 1, for a person on the side being played (`record`, not at fault in the flags word): the
// match's own view of the person, from the side's form, his stamina and his condition, as an offset
// `alt_role` (r22) that moves the total by one more component.
void Placement::form_of_match() {
    if (!wants(bit(0x1f)) || !nonzero_logical(c, record)) return;
    if (compare(c, i32(c.u32(at(at::player_flags)) & 1), 0) != 0) return;
    const u32 match = c.u32(record + 0x1a4);
    second = match;
    alt_role = 0;
    bool special = compare(c, c.i16(match + 0x794c), 0x1ef) == 0;
    if (!special) {
        special = human_manager_on_side(match);
        compare(c, special, 0);  // extsb. r0,r0 on the flag
    }
    if (special && compare(c, s8(c.u8(record)), 0xb) < 0) alt_role = reg(-0x7d0);
    // The player's own condition byte: -1 asks for the match's clock, a value for the time since.
    const i32 mark = s8(c.u8(record + 0x19));
    const i32 clock = c.i16(c.u32(record + 0x1a4) + 0x7948);
    c.set(3, c.u8(record + 0x19));
    if (compare(c, mark, -1) == 0) {
        const i32 minutes = magic_div(clock, 0x2e8ba2e9, 1);  // / 11
        c.set(4, reg(appraisal::mulhw(0x2e8ba2e9, clock)));
        const i32 left = (0x3c - minutes) * 0x96;
        c.set(0, reg(left));
        const i32 term = compare(c, left, 0xbb8) >= 0 ? 0xbb8 : left;
        alt_role = u32(alt_role) + reg(term);
        if (compare(c, s8(c.u8(data + 0xf)), 1) > 0) {
            alt_role = u32(alt_role) + 0xdac;
        } else if (compare(c, i32(u32(alt_role)), -0x7d0) < 0) {
            alt_role = reg(-0x7d0);
        }
    } else if (mark >= 0) {
        c.set({{4, c.u32(record + 0x1a4)}, {5, c.u8(record + 0x1f)}});
        const i32 minutes = magic_div(clock, 0x2e8ba2e9, 1);  // / 11
        const i32 since = s8(reg(minutes - i32(c.u8(record + 0x1f))));
        if (compare(c, since, 0x3c) < 0) alt_role = reg(since * 0x28) + u32(alt_role) - 0x960;
    }
    count = u32(count) + 1;
    const i32 gap = c.i16(record + 0x36) - i32(u32(alt_role));
    c.set(3, u32(appraisal::mulhw(0x51eb851f, gap) >> 4 < 0 ? 1 : 0));
    total = u32(total) + reg(magic_div(gap, 0x51eb851f, 4));  // / 50
}

// Whether the side's human manager is in a state the original's four calls ask about (a person of the
// last 16 of the people table with the flag at +0x6b, not disqualified by 0x10147e70, who is the
// side's own or answers 0x100c7d00 / 0x101481d0).
bool Placement::human_manager_on_side(u32 match) {
    c.put8(at(at::side_index), c.u8(record + 0x27));
    const i32 index = s8(c.u8(at(at::side_index)));
    c.put32(at(at::side_offset), reg(index * 0xd8));
    const u32 side_record = match + reg(index * 0xd8);
    c.put32(at(at::side_record), side_record);
    const u32 manager = c.u32(side_record + 0x7acc);
    c.set({{3, side_record}, {4, manager}});
    bool listed = false;
    if (nonzero_logical(c, manager)) {
        const u32 count_at = c.toc(-0x7be4);
        const i32 id = c.i32(manager), people = c.i32(count_at);
        c.set({{3, reg(people)}, {5, reg(id)}, {0, reg(people - 0x10)}});
        if (compare(c, id, people - 0x10) >= 0 && compare(c, id, people) < 0)
            listed = compare(c, s8(c.u8(manager + 0x6b)), 0) != 0;
    }
    if (compare(c, listed, 0) == 0) return false;
    c.set({{3, c.u32(c.toc(-0x7a54))}, {4, manager}});
    c.call(0x10147e70, 0x10212b98);
    if (compare(c, s8(c.r(3)), 0) != 0) return false;
    const u32 again = match + c.u32(at(at::side_offset));
    if (compare(c, s8(c.u8(again + 0x7ac7)), 0) == 0) return true;
    const u32 team = c.u32(match + 0x3118);
    c.set(3, c.u32(team + (compare(c, s8(c.u8(at(at::side_index))), 0) != 0 ? 0x20 : 0x1c)));
    c.call(0x100c7d00, 0x10212bd8);
    if (compare(c, s8(c.r(3)), 0) != 0) return true;
    c.set({{3, c.u32(c.toc(-0x7a54))}, {4, c.u32(c.u32(at(at::side_record)) + 0x7acc)}});
    c.call(0x101481d0, 0x10212bf4);
    return compare(c, s8(c.r(3)), 0) != 0;
}
}

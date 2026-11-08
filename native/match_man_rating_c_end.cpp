// 0x10211e70: the checks against the side, the relations with the club, the last adjustments, the result
// and the human manager's tail (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"
#include <algorithm>

namespace cm::aot::match_man::placement {

// The side being played (the stack word 0x190) names the person as its captain (+0x40, +0x80000 for its
// next ones at +8, +0xc, +0x10), and a foreigner to the club's nation is worth less.
void Placement::side_and_nation() {
    if (wants(bit(0x12))) {
        const u32 side = c.u32(at(at::side));
        if (nonzero_logical(c, side) && compare_logical(c, c.u32(side + 0x40), person) == 0) modifier = u32(modifier) + 0x64;
    }
    if (wants(bit(7)) && nonzero_logical(c, club)) {
        const u32 club_nation = c.u32(club + appraisal::club::nation);
        if (compare_logical(c, c.u32(person + appraisal::person::nation), club_nation) != 0 &&
            compare_logical(c, c.u32(person + appraisal::person::second_nation), club_nation) != 0)
            modifier = u32(modifier) - 0x32;
    }
    if (wants(bit(0xc))) {
        const u32 side = c.u32(at(at::side));
        if (nonzero_logical(c, side)) {
            if (compare_logical(c, c.u32(side + 8), person) == 0) modifier = u32(modifier) + 0x1e;
            else if (compare_logical(c, c.u32(side + 0xc), person) == 0) modifier = u32(modifier) + 0x14;
            else if (compare_logical(c, c.u32(side + 0x10), person) == 0) modifier = u32(modifier) + 0xa;
        }
    }
}

// The three calls that find the person's relation record and the share (a signed byte, in percent) of two
// counts in it; the value returned is 50 less half the share (nullopt when a call or the share ends the
// lookup). `relation` takes the first call's record, `judge_arg` (r20) keeps the share.
std::optional<i32> Placement::relation_share(u32 first_back, u32 second_back, u32 third_back) {
    c.set({{3, c.u32(c.toc(-0x7b40))}, {4, person}});
    c.call(0x103ecb00, first_back);
    relation = c.r(3);
    if (compare(c, i32(c.r(3)), 0) == 0) return std::nullopt;
    c.set({{3, c.u32(c.toc(-0x7b7c))}, {4, person}});
    c.call(0x10082f20, second_back);
    const u32 found = c.r(3);
    c.set(4, c.toc(-0x7b78));
    compare_logical(c, found, 0);
    c.put32(c.r(4), found);
    u32 found_club = 0;
    if (found) found_club = c.u32(c.toc(-0x7be8)) + c.u32(found + 4) * 0x248;
    c.set({{3, c.u32(c.toc(-0x7b40))}, {4, person}, {5, found_club}, {6, 1}});
    c.call(0x103ecef0, third_back);
    if (compare(c, s8(c.r(3)), 0) == 0) return std::nullopt;
    const u32 second_count = c.u16(relation + 6), first_count = c.u16(relation + 4);
    c.set({{4, second_count}, {5, first_count}});
    if (compare(c, i32(second_count + first_count), 0) != 0)
        judge_arg = reg(s8(reg(divw(i32(first_count * 100), i32(first_count + second_count)))));
    const i32 percent = s8(judge_arg);
    if (compare(c, percent, 0x46) >= 0) return std::nullopt;
    return 0x32 - half(percent);
}

// The side's match clock scales the relation's adjustment (single precision): r14 = r14 * (0 - clock / pool).
void Placement::scale_by_clock() {
    if (!nonzero_logical(c, record)) return;
    const u32 match = c.u32(record + 0x1a4);
    const i32 clock = c.i16(match + 0x7948);
    const double clock_value = fpu.fsub(store_pair(c, at(0x88), clock), conversion());
    c.set({{3, match}, {4, 0x43300000}, {5, u32(adjust) ^ 0x80000000u}});  // the int-to-double idiom's leftovers
    const double adjust_pair = store_pair(c, at(0x80), i32(u32(adjust)));
    const double ratio = fpu.fsub(D(0x0), fpu.fdiv(clock_value, D(0x420)));
    const double adjust_value = fpu.fsubs(adjust_pair, conversion());
    c.set_f(1, adjust_value);
    adjust = truncate_at(c, fpu, at(0x90), fpu.fmuls(adjust_value, fpu.frsp(ratio)));
}

// Flag 0x10000: the relation record (the one looked up at the start) adds to the modifier by its kind byte
// (+0x60) and its mood; flag 0x20000: the kind alone, with the share. Both are then scaled by the match's
// clock; the first by the human manager's weight too.
void Placement::relations() {
    const auto mood = [&](u32 divisor, u32 cap) {
        const double mood_value = fpu.fsub(store_pair(c, at(0x80), s8(c.u8(relation + 0x4c))), conversion());
        double chosen = D(cap);
        c.set_f(1, mood_value);
        const double scaled = fpu.fdiv(mood_value, D(divisor));
        if (!fcompare(fpu, chosen, scaled).less) chosen = scaled;
        adjust = truncate_at(c, fpu, at(0x88), chosen);
    };
    if (wants(bit(0xf))) {
        const bool present = nonzero_logical(c, relation);
        adjust = 0;
        judge_arg = 0;
        if (!present) return;
        const u32 relation_flags = c.u32(relation + 0x50);
        const u32 masked = relation_flags & 0x2808;
        if (compare(c, i32(masked), 0) != 0) mood(0x20, 0x408);
        else if (compare(c, i32(relation_flags), 0) != 0) mood(0x80, 0x410);
        const u32 kind = c.u8(relation + 0x60) & 0xf;
        const auto add_share = [&](std::optional<i32> value) { if (value) adjust = u32(adjust) + reg(*value); };
        if (compare(c, i32(kind), 3) == 0) {
            if (wants(bit(8))) {
                adjust = u32(adjust) + 0xa;
            } else {
                const i32 diff = (s8(c.u8(relation + 0x45)) - s8(c.u8(relation + 0x46))) * 5;
                const i32 ceiling = c.i16(at(at::ceiling));
                const double ceiling_value = fpu.fsub(store_pair(c, at(0x80), ceiling), conversion());
                const double diff_pair = store_pair(c, at(0x88), diff);
                const double mixed = fpu.multiply_add(0x102160cc, D(0x80), ceiling_value, D(0x418));
                c.set_f(1, mixed);
                const double diff_value = fpu.fsub(diff_pair, conversion());
                adjust = u32(adjust) + truncate_at(c, fpu, at(0x90), fpu.fadd(mixed, diff_value));
                // The original keeps the ceiling in r6 (lha r6,0xb0(r1)) and 0x43300000 in r5; the first call may end the
                // lookup, and the callee's leftover r6 then reaches the return.
                c.set({{5, 0x43300000}, {6, reg(ceiling)}});
                add_share(relation_share(0x102160f4, 0x1021610c, 0x1021614c));
            }
        } else if (compare(c, i32(kind), 3) > 0) {
            if (kind == 7) adjust = u32(adjust) - 0x14;
        } else if (compare(c, i32(kind), 1) == 0) {
            adjust = u32(adjust) + 0x19;
            if (!wants(bit(8))) add_share(relation_share(0x10215f04, 0x10215f1c, 0x10215f5c));
        } else if (compare(c, i32(kind), 1) > 0) {
            adjust = u32(adjust) + 0xf;
            if (!wants(bit(8))) add_share(relation_share(0x10215fc4, 0x10215fdc, 0x1021601c));
        }
        scale_by_clock();
        if (nonzero_logical(c, c.u32(at(at::human)))) {
            const double value = fpu.fsub(store_pair(c, at(0x98), i32(u32(adjust))), conversion());
            c.set_f(1, conversion());
            adjust = truncate_at(c, fpu, at(0xa0), fpu.fmul(D(0x1c0), value));
        }
        modifier = u32(modifier) + u32(adjust);
        return;
    }
    if (!wants(bit(0xe))) return;
    const bool present = nonzero_logical(c, relation);
    judge_arg = 0;
    if (present) {
        const u32 kind = c.u8(relation + 0x60) & 0xf;
        if (compare(c, i32(kind), 6) >= 0) {
            adjust = compare(c, i32(kind), 8) == 0 ? 0x19u : 0u;
        } else if (compare(c, i32(kind), 4) >= 0) {
            adjust = 0x19;
        } else if (compare(c, i32(kind), 1) >= 0) {
            adjust = reg(-0x19);
            if (!wants(bit(8)))
                if (const auto value = relation_share(0x102162a4, 0x102162bc, 0x102162fc)) adjust = reg(-0x19 - *value);
        } else {
            adjust = 0;
        }
        scale_by_clock();
        modifier = u32(modifier) + u32(adjust);
    }
    if (c.u32(at(at::flag_800)) != 0) return;
    // The game's opinion of the person: a very low one lowers the modifier by the pool's scale of it, a
    // high one (over 500) by another.
    const auto lower_by = [&](i32 opinion, u32 site, double scale, double base) {
        const double value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        c.set_f(1, scale);
        modifier = u32(modifier) - truncate_at(c, fpu, at(0x88), fpu.multiply_add(site, scale, value, base));
    };
    const i32 first = ask_opinion(0x102163d8);
    compare(c, first, -0x3e8);
    if (first < -0x3e8) lower_by(ask_opinion(0x102163f4), 0x1021641c, D(0x428), D(0x348));
    else if (compare(c, ask_opinion(0x10216444), 0x1f4) > 0) lower_by(ask_opinion(0x10216460), 0x10216488, D(0x158), D(0x28));
}

// The last adjustments: caps (flag 0x100000), or the unrated player of a club (flag 4: a person of the
// table with no club under 5000, or his club's other record scaled), the people table's bound, and the
// club's own player raised.
void Placement::ending() {
    if (wants(bit(0xb))) {
        const i32 capped = i32(c.u8(person + appraisal::person::byte_5e)) * 5;
        compare(c, capped, 0x32);
        modifier = u32(modifier) - reg(capped >= 0x32 ? 0x32 : capped);
    } else if (compare(c, c.i32(at(at::flag_4)), 0) != 0 && in_people_table()) {
        const u32 person_club = c.u32(person + appraisal::person::club);
        bool done = false;
        if (!nonzero_logical(c, person_club)) {
            if (compare(c, c.i16(data + 8), 0x1388) < 0) {
                modifier = u32(modifier) - 0x1e;
                done = true;
            }
        }
        if (!done && nonzero_logical(c, person_club)) {
            const u32 record_of_club = c.u32(person_club + 0x30);
            if (nonzero_logical(c, record_of_club)) {
                const double value = fpu.fsub(store_pair(c, at(0x80), c.i16(record_of_club + 0x18) - 0x10), conversion());
                c.set_f(1, conversion());
                modifier = u32(modifier) + truncate_at(c, fpu, at(0x88), fpu.fmul(D(0x100), value));
            }
        }
    }
    // A person beyond the people table with a club whose nation has a flag at +0x120 costs more.
    if (nonzero_logical(c, person)) {
        const u32 count_at = c.toc(-0x7be4);
        const i32 index = c.i32(person), people = c.i32(count_at);
        c.set({{3, count_at}, {4, reg(index)}, {0, reg(people)}});
        compare(c, index, people);
        if (index >= people && nonzero_logical(c, club)) {
            const u32 nation = c.u32(club + appraisal::club::nation);
            if (nonzero_logical(c, nation) && compare(c, s8(c.u8(nation + 0x120)), 0) != 0) {
                modifier = u32(modifier) - 0x46;
                position = u32(position) - 0x78;
            }
        }
    }
    if (compare_logical(c, c.u32(person + appraisal::person::club), club) == 0) {
        // The club's own player: the modifier is raised to 0x2d8 times itself or five more.
        const i32 value = i32(u32(modifier));
        const double pair = store_pair(c, at(0x88), value);
        const double raised_pair = store_pair(c, at(0x80), value + 5);
        const double current = fpu.fsub(pair, conversion());
        double chosen = fpu.fmul(D(0x2d8), current);
        const double raised = fpu.fsub(raised_pair, conversion());
        c.set_f(1, chosen);
        if (fcompare(fpu, raised, chosen).greater) {
            store_pair(c, at(0x90), value + 5);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
            c.set_f(1, chosen);
        }
        modifier = truncate_at(c, fpu, at(0x98), chosen);
    }
}

// The result (r14): the position's value and the modifier: unmixed for the anchor flag, otherwise the two
// multiplied, added or scaled by what is left of 500 (a negative position); then the human manager's
// slot for the person raises or lowers it.
std::int32_t Placement::finish() {
    i32 value;
    if (compare(c, i32(u32(flags)), 0x800) == 0) value = i32(u32(position));
    else if (compare(c, i32(u32(position)), 0) < 0) value = i32(reg(500 - i32(u32(modifier))) * u32(position));
    else if (compare(c, i32(u32(modifier)), 1) < 0) value = i32(u32(position) + u32(modifier));
    else value = i32(u32(position) * u32(modifier));
    adjust = reg(value);
    if (!nonzero_logical(c, c.u32(at(at::human)))) return guest::result(c, u32(adjust));
    relation = reg(c.i16(at(at::role)));
    if (compare(c, i32(u32(relation) & 1), 0) != 0 && compare(c, i32(c.u32(at(at::player_flags)) & 1), 0) == 0)
        return guest::result(c, u32(adjust));
    u32 entry_at = c.u32(at(at::human));
    data = 0;
    i32 extra_value = 0;
    for (; compare(c, s8(data), 0x10) < 0; entry_at += 8, data = u32(data) + 1) {
        c.set(3, entry_at);
        const u32 listed = c.u32(entry_at);
        const u32 id = c.u32(person);
        c.set({{4, id}, {0, listed}});
        if (compare(c, i32(id), i32(listed)) != 0) continue;
        bool strong = false;
        if (compare(c, s8(data), 0xb) < 0) {
            c.set({{3, c.u32(c.toc(-0x7a54))}, {4, 2}, {5, c.u32(club + 0x90)}});
            c.call(0x10149df0, 0x102166a8);
            if (compare(c, s8(c.r(3)), 0) == 0) {
                strong = true;
            } else {
                const u32 slot = c.u32(at(at::human)) + u32(s8(data)) * 8;
                const i32 holder = c.i32(slot + 4);
                c.set(0, reg(holder));
                strong = compare(c, i32(u32(relation)), holder) == 0 || compare(c, holder, -1) == 0;
            }
        }
        const i32 weighted = i32(reg(i32(u32(position))) * 200);
        if (strong) {
            c.set({{0, reg(weighted)}, {3, 0x2710}});
            const i32 base = compare(c, weighted, 0x2710) < 0 ? 0x2710 : weighted;
            extra_value = base + 0x9c40;
            if (compare(c, i32(u32(adjust)), 0) < 0) adjust = 0;
        } else {
            extra_value = weighted < 0 ? 0 : weighted;
            if (compare(c, i32(u32(adjust)), 0) < 0) adjust = reg(half(i32(u32(adjust))));
        }
        c.set(4, reg(extra_value));
        if (c.u32(at(at::flag_800)) == 0) {
            const i32 w = i32(u32(worth));
            const double w_value = fpu.fsub(store_pair(c, at(0x80), w), conversion());
            double ratio = fpu.fdiv(w_value, D(0x430));
            compare(c, w, 0x251c);
            if (w < 0x251c) {
                ratio = fpu.fmul(ratio, fpu.fsub(store_pair(c, at(0x80), w), conversion()));
                compare(c, w, 0x222e);
                if (w < 0x222e) ratio = fpu.fmul(ratio, fpu.fsub(store_pair(c, at(0x80), w), conversion()));
                else ratio = fpu.fmul(ratio, D(0x140));
            } else {
                ratio = fpu.fmul(ratio, D(0x430));
            }
            const double scaled = fpu.fdiv(ratio, D(0x1a8));
            c.set_f(1, scaled);
            const double extra_pair = fpu.fsub(store_pair(c, at(0x80), extra_value), conversion());
            const double shifted = fpu.fsub(scaled, D(0x0));
            extra_value = i32(truncate_at(c, fpu, at(0x88), fpu.fmul(extra_pair, shifted)));
            c.set(4, reg(extra_value));
        }
        if (compare(c, extra_value, 0) > 0) adjust = u32(adjust) + reg(extra_value);
        break;
    }
    return guest::result(c, u32(adjust));
}
}

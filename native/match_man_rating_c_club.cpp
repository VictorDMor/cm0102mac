// 0x10211e70: what the club and the person's age do to the modifier (the club factor, the age scaling)
// and the person's development (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man::placement {

// modifier = trunc(pool[offset] * modifier): the conversion words at 0x80, the result at 0x88.
void Placement::scale_modifier(u32 pool_offset) {
    c.set(3, u32(modifier) ^ 0x80000000u);
    const double value = fpu.fsub(store_pair(c, at(0x80), i32(u32(modifier))), conversion());
    c.set_f(1, conversion());
    modifier = truncate_at(c, fpu, at(0x88), fpu.fmul(D(pool_offset), value));
}

// modifier = trunc(modifier * factor) in single precision steps (fsubs, fmuls).
void Placement::scale_single(double factor) {
    c.set(3, u32(modifier) ^ 0x80000000u);
    const double value = fpu.fsubs(store_pair(c, at(0x80), i32(u32(modifier))), conversion());
    c.set_f(1, conversion());
    modifier = truncate_at(c, fpu, at(0x88), fpu.fmuls(value, factor));
}

// By the person's age and his caps (the byte at +0x5e): the modifier is scaled down for the old, the unused
// and the unfit, and set to 5 for those who have played too much.
void Placement::scale_by_age() {
    const i32 years = s8(age);
    const auto played = [&] { return u32(c.u8(person + appraisal::person::byte_5e)); };
    if (compare(c, years, 0x22) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x1c0);
        else if (compare_logical(c, caps, 8) < 0) scale_modifier(0x210);
        else if (compare(c, years, 0x23) > 0 && compare_logical(c, caps, 0x19) < 0) scale_modifier(0x380);
        else if (compare_logical(c, caps, 0xf) < 0) scale_modifier(0x190);
        else if (compare_logical(c, caps, 0x23) < 0) scale_modifier(0x370);
    } else if (compare(c, years, 0x1b) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x210);
        else if (compare_logical(c, caps, 8) < 0) scale_modifier(0x380);
        else if (compare_logical(c, caps, 0xf) < 0) scale_modifier(0x290);
    } else if (compare(c, years, 0x18) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x380);
        else if (compare_logical(c, caps, 8) < 0) scale_modifier(0x190);
        else if (compare_logical(c, caps, 0xf) < 0) scale_modifier(0x370);
        else if (compare_logical(c, caps, 0x14) > 0) modifier = 5;
    } else if (compare(c, years, 0x15) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x370);
        else if (compare_logical(c, caps, 6) < 0) scale_modifier(0x198);
        else if (compare_logical(c, caps, 0xa) < 0) scale_modifier(0x378);
        else if (compare_logical(c, caps, 0x14) > 0) modifier = 5;
    } else if (compare_logical(c, played(), 0xa) > 0) {
        modifier = 5;
    }
}

// The club's reputation against the person's club's, the relation's rating (the byte at +0x44) and the
// person's age decide a factor for the modifier (single precision), narrowed again for goalkeepers.
void Placement::club_factor() {
    if (!nonzero_logical(c, relation) || !nonzero_logical(c, club) || !wants(bit(6)) || !in_people_table()) return;
    const u32 person_club = c.u32(person + appraisal::person::club);
    if (!nonzero_logical(c, person_club)) {
        // A free agent for more than 60 days: the modifier scaled by the pool's factor.
        c.set({{3, c.toc(appraisal::slot::date)}, {4, c.u32(person + appraisal::person::birth_day)},
               {5, c.u32(person + appraisal::person::birth_day + 4)}});
        c.call(appraisal::callee::date_days, 0x102140bc);
        if (compare(c, i32(c.r(3)), 0x3c) > 0) scale_modifier(0x190);
        scale_by_age();
        return;
    }
    const i32 person_reputation = c.i16(person_club + appraisal::club::reputation);
    const i32 club_reputation = c.i16(club + appraisal::club::reputation);
    const i32 floor_reputation = person_reputation - 0x4e2;
    const auto narrowed = [&](double& factor, u32 offset) { factor = fpu.frsp(fpu.fmul(D(offset), factor)); };
    const auto goalkeeper_narrowing = [&](double& factor, i32 limit) {
        if (compare_logical(c, c.u8(data + 0xf), 0x14) != 0) return;
        if (compare(c, s8(age), 0x12) < 0) narrowed(factor, 0x190);
        else if (compare(c, s8(age), limit) < 0) narrowed(factor, 0x370);
        else narrowed(factor, 0x378);
    };
    if (compare(c, club_reputation, floor_reputation) > 0) {
        const i32 relation_rating = s16(reg(s8(c.u8(relation + 0x44)) + 100));
        if (compare(c, relation_rating, 0x7d) < 0) {
            double f2 = F(0x3c);
            if (compare(c, s16(reg(club_reputation)), 0) != 0) {
                f2 = F(0x40);
                const double floor_value = fpu.fsubs(store_pair(c, at(0x80), floor_reputation), conversion());
                const double reputation_value = fpu.fsubs(store_pair(c, at(0x88), club_reputation), conversion());
                const double quotient = fpu.fdivs(floor_value, reputation_value);
                const double rating_value = fpu.fsubs(store_pair(c, at(0x90), relation_rating), conversion());
                const double product = fpu.fmuls(quotient, rating_value);
                c.set_f(1, product);
                f2 = fpu.fmuls(f2, product);
                if (fcompare(fpu, f2, D(0x0)).greater) f2 = F(0x44);
                else if (fcompare(fpu, f2, D(0x190)).less) f2 = F(0x3c);
            }
            if (compare(c, s8(age), 0x22) > 0) narrowed(f2, 0x190);
            else if (compare(c, s8(age), 0x1e) > 0) narrowed(f2, 0x370);
            else if (compare(c, s8(age), 0x1b) > 0) narrowed(f2, 0x378);
            goalkeeper_narrowing(f2, 0x14);
            scale_single(f2);
        }
    } else {
        const i32 relation_rating = s8(c.u8(relation + 0x44));
        if (compare(c, relation_rating, 0x19) < 0 && compare(c, s8(age), 0x1e) > 0) {
            double f2;
            if (compare(c, s8(age), 0x25) > 0) f2 = F(0x28);
            else if (compare(c, s8(age), 0x22) > 0) f2 = F(0x3c);
            else f2 = F(0x48);
            // Never true: the branch already requires an age above 30 (the original's dead code, kept).
            if (compare(c, club_reputation, 0x1482) > 0 && compare(c, s8(age), 0x14) < 0) {
                if (compare(c, relation_rating, -0x4b) < 0) f2 = F(0x28);
                else if (compare(c, relation_rating, -0x19) < 0) f2 = F(0x4c);
                else if (compare(c, s16(reg(relation_rating)), 0) < 0) f2 = F(0x48);
            }
            goalkeeper_narrowing(f2, 0x15);
            scale_single(f2);
        }
    }
    c.set({{3, person}, {4, 0}});
    c.call(0x100d3040, 0x1021437c);
    if (compare(c, s8(c.r(3)), 0) != 0) scale_modifier(0x378);
    scale_by_age();
}

// The jump table at TOC -0x528c holds the code address of each case of the nation bias (0 to 9):
// 0x10215044 for case 0 down to 0x10214c0c for case 9, the original's reverse order.
namespace {
// The factors by which the modifier is scaled for a nation of region 14 and for the others.
struct NationFactor { u32 region_14, other; };
constexpr NationFactor nation_factors[10] = {{0x3f8, 0x3f0}, {0x3f0, 0x3e8}, {0x3e8, 0x3e0}, {0x3e0, 0x3d8}, {0x3d8, 0x3d0},
                                             {0x3d0, 0x3c8}, {0x3c8, 0x3c0}, {0x3c0, 0x3b8}, {0x3b8, 0x3a8}, {0x3a8, 0x3b0}};
constexpr u32 region_offset = 0x118;
}

// The person's development: the injury and the club's own ban on the age (a person older than 40 or 42
// is worth -1000000), the age's cost, the sum of half of each of ten ratings, and the nation's bias.
std::optional<i32> Placement::development() {
    constexpr u32 withdrawn = reg(-1000000);
    if (!wants(bit(0x11))) return std::nullopt;
    c.set({{3, c.u32(game())}, {4, person}});
    c.call(0x1018c340, 0x1021477c);
    if (nonzero_logical(c, c.r(3))) modifier = u32(modifier) - reg(c.i16(c.r(3) + 0x34));
    const u32 id = c.u32(person);
    const u32 table = c.u32(c.toc(-0x7b9c));
    const u32 marks = c.u8(table + id * 100 + 12);
    c.set({{3, id * 100}, {4, table}, {0, marks}});
    const u32 banned = (marks >> 2) & 3;
    compare(c, i32(banned), 0);
    if (banned) {
        if (compare(c, s8(age), 0x28) > 0) return guest::result(c, withdrawn);
        if (compare(c, s8(age), 0x22) > 0) modifier = u32(modifier) - 0x4b;
        else modifier = u32(modifier) - 0x19;
        const u32 own = c.u32(person + appraisal::person::player);
        c.set(3, own);
        if (compare(c, s8(c.u8(own + 0x3c)), 0xa) < 0) modifier = u32(modifier) - 0x32;
    }
    if (compare(c, s8(age), 0x2a) > 0) return guest::result(c, withdrawn);
    // The age's cost: past 34 unless the player's flags word has bit 1 (which makes the young cost
    // instead), then past 32 for a role of 0x40.
    const u32 flags_word = c.u32(at(at::player_flags));
    bool charged = false;
    if (compare(c, s8(age), 0x22) > 0 && compare(c, i32(flags_word & 1), 0) == 0) {
        modifier = u32(modifier) - reg((s8(age) - 0x22) * 10);
        charged = true;
    }
    if (!charged && compare(c, s8(age), 0x17) < 0 && compare(c, i32(flags_word & 1), 0) != 0) {
        modifier = u32(modifier) - reg((0x17 - s8(age)) * 10);
        charged = true;
    }
    if (!charged && compare(c, s8(age), 0x20) > 0 && compare(c, c.i16(at(at::role)) & 0x40, 0) != 0)
        modifier = u32(modifier) - reg((s8(age) - 0x20) * 5);
    if (compare(c, i32(u32(alt_role)), 0) != 0) {
        const i32 limit = s8(flags_word & 1 ? 0x1du : 0x19u);
        c.set({{3, 0x43300000}, {4, reg(counted()) ^ 0x80000000u}, {5, flags_word & 1 ? 0x1du : 0x19u}});
        const double count_value = fpu.fsub(store_pair(c, at(0x80), counted()), conversion());
        const double decay = fpu.multiply_add(0x102148b4, D(0x388), count_value, D(0x368));
        c.set_f(1, decay);
        if (compare(c, s8(age), limit) > 0 && fcompare(fpu, decay, D(0x178)).greater) {
            const double years = fpu.fsub(store_pair(c, at(0x80), s8(age) - limit), conversion());
            modifier = u32(modifier) - truncate_at(c, fpu, at(0x88), fpu.fmul(decay, years));
        }
    }
    if (compare(c, c.i16(at(at::role)) & 1, 0) == 0) halves_of_ratings();
    nation_bias();
    return std::nullopt;
}

// The sum of half of each of the ratings 0x10..0x19 (signed bytes, halved toward zero): above 35 the
// excess over 20, squared, in 250ths, raises the modifier. The compiler's register use clobbers r24 and
// r25 (total and count) and leaves the halves in r5-r12; the sign bits go through the stack words
// 0xd8..0xf0.
void Placement::halves_of_ratings() {
    i32 s[0x1a];
    for (u32 n = 0x10; n < 0x1a; ++n) s[n] = stat(n);
    const auto h = [](i32 value) { return half(value); };
    c.put32(at(0xd8), u32(sign_bit(s[0x13])));
    c.put32(at(0xdc), u32(sign_bit(s[0x14])));
    c.put32(at(0xe0), u32(sign_bit(s[0x16])));
    c.put32(at(0xe4), u32(sign_bit(s[0x15])));
    c.put32(at(0xe8), u32(sign_bit(s[0x18])));
    c.put32(at(0xec), u32(sign_bit(s[0x17])));
    c.put32(at(0xf0), u32(sign_bit(s[0x19])));
    i32 sum = 0;
    for (u32 n = 0x10; n < 0x1a; ++n) sum += h(s[n]);
    total = reg(s[0x19]);
    count = u32(sign_bit(s[0x12]));
    c.set({{3, reg(h(s[0x17]))}, {4, reg(h(s[0x18]))}, {5, reg(h(s[0x15]))}, {6, reg(h(s[0x16]))}, {7, reg(sum)},
           {8, reg(h(s[0x14]))}, {9, reg(h(s[0x12]))}, {10, reg(h(s[0x11]))},
           {11, reg(s[0x14] + sign_bit(s[0x14]))}, {12, reg(s[0x10] + sign_bit(s[0x10]))}});
    if (compare(c, sum, 0x23) > 0) {
        const i32 over = sum - 0x14;
        modifier = u32(modifier) + reg(i32(reg(over) * reg(over)) / 250);  // mulhw by 0x10624dd3, srawi 4
        c.set(3, 0x10624dd3);
    }
}

// The player's adaptability (attribute 0x2f), and for a club of another region than the person's nation
// 0x103092f0's opinion of the move, the person's caps and one case of the jump table scale the modifier.
void Placement::nation_bias() {
    modifier = u32(modifier) + scaled_term(0x80, stat(0x2f) - 10, 0x20, 0x88);
    const u32 club_nation = c.u32(club + appraisal::club::nation);
    c.set({{3, reg(stat(0x2f))}, {4, club_nation}});
    compare_logical(c, club_nation, 0);
    if (!club_nation) return;
    const u32 nation = c.u32(person + appraisal::person::nation);
    const u32 club_region_raw = c.u8(club_nation + 0x119);
    const i32 club_region = s8(club_region_raw), person_region = s8(c.u8(nation + 0x119));
    c.set({{3, reg(club_region)}, {6, club_region_raw}, {0, reg(person_region)}});
    compare(c, club_region, person_region);
    if (club_region == person_region) return;
    c.set({{3, c.u32(world())}, {4, person}, {5, club}, {7, 1}});
    c.call(0x103092f0, 0x10214ad0);
    const bool wants_extra = compare(c, i32(u32(alt_role)), 0) != 0;
    i32 opinion = s8(c.r(3));
    c.set(5, reg(opinion));
    if (wants_extra) {
        const i32 base = half(s8(c.u8(person + appraisal::person::byte_61)));
        const i32 offset = s8(reg(s8(age) - base + 5));
        c.set({{3, reg(s8(age) - base)}, {4, 0x43300000}});
        const double opinion_value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        const double offset_value = fpu.fsub(store_pair(c, at(0x88), offset), conversion());
        c.set_f(1, offset_value);
        double chosen = fpu.multiply_add(0x10214b34, D(0x398), offset_value, D(0x390));
        if (fcompare(fpu, opinion_value, chosen).greater) {
            store_pair(c, at(0x90), opinion);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
        }
        opinion = i32(truncate_at(c, fpu, at(0x98), chosen));
    } else {
        c.set(3, 0x43300000);
        const double modifier_value = fpu.fsub(store_pair(c, at(0x88), i32(u32(modifier))), conversion());
        const double bound = fpu.multiply_add(0x10214b94, D(0x3a0), modifier_value, D(0x100));
        c.set_f(1, bound);
        const double opinion_value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        if (fcompare(fpu, opinion_value, bound).less) opinion = i32(truncate_at(c, fpu, at(0x80), bound));
    }
    c.set(5, reg(opinion));
    const u32 caps = c.u8(person + appraisal::person::byte_5e);
    if (compare_logical(c, caps, 0) != 0) {
        const i32 reputation = c.i16(c.u32(person + appraisal::person::nation) + 0xac);
        c.set(3, reg(reputation));
        if (compare(c, reputation + i32(caps * caps), 0x1388) > 0) {
            const i32 clipped = s8(reg(opinion));
            compare(c, clipped, 5);
            c.set(3, reg(clipped > 5 ? clipped : 5));
            opinion = s8(reg(clipped > 5 ? clipped : 5));
            c.set(5, reg(opinion));
        }
    }
    const u32 chosen_case = reg(s8(reg(opinion)));
    c.set(0, chosen_case);
    if (compare_logical(c, chosen_case, 9) > 0) return;
    const u32 target = c.u32(c.toc(-0x528c) + (chosen_case << 2));
    c.set({{3, target}, {0, chosen_case << 2}});
    c.s.wr<4>(guest::reg::ctr, target);
    // The jump lands on case `chosen_case`'s code; the table is constant data.
    const u32 selected = chosen_case;
    const u32 nation_record = c.u32(person + appraisal::person::nation);
    c.set(3, nation_record);
    const u32 region = c.u8(nation_record + region_offset);
    c.set(0, region);
    compare(c, i32(region), 14);
    scale_modifier(region == 14 ? nation_factors[selected].region_14 : nation_factors[selected].other);
}
}

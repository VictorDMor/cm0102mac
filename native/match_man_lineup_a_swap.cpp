// MATCH_MAN.CPP lineup_a block: the first phase, where a strong side may decide
// to rest players (sets side+0x18 and side+0x1c from the two clubs' strength).
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;
namespace {
// The date records the routine compares the fixture's date with, by the index
// 0x10717890 is asked for: where it returns, its result buffer, the copy that is
// compared and the pool factor the matching one scales by (0: not scaled).
struct DateCheck { std::uint32_t back, buffer, copy, factor; };
constexpr DateCheck date_checks[7] = {
    {0x102078a0, 0xc0, 0x88, 0x48}, {0x10207928, 0xb8, 0x80, 0x50}, {0x102079b0, 0xb0, 0x78, 0x58},
    {0x10207a38, 0xa8, 0x70, 0x00}, {0x10207ab8, 0xa0, 0x68, 0x60}, {0x10207b40, 0x98, 0x60, 0x68},
    {0x10207bc8, 0x90, 0x58, 0x70}};
}

// 0x10207020-0x10207d58. Computes the strength of the opposing side (r17) and the
// own one (r16), and from them the urgency byte side+0x1c; every early exit of the
// original goes to 0x10207d58, here a return.
void ComputerLineup::assess_replacement() {
    const auto self = side();
    if (compare(c, s8(c.u8(self + lineup_side::human))) != 0) return;
    if (compare(c, c.i16(club() + 0x1ec), 0x1770) < 0) return;
    const auto extras = c.u32(self + lineup_side::extras);
    if (compare(c, c.i32(extras), -2) == 0) return;
    c.set({{3, club()}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x10207070);
    if (compare(c, s8(c.r(3))) != 0) return;
    c.set(3, club());
    c.call(0x100c7d00, 0x10207080);
    if (compare(c, s8(c.r(3))) != 0) return;
    if (compare(c, s8(r15), 0x1e) <= 0) return;

    c.set({{3, at(0xd8)}, {4, c.toc(-0x7b80)}, {5, 1}});
    c.call(0x10717890, 0x102070a4);
    r16 = extended(c.i16(at(0xd8)));
    r17 = c.u16(at(0xda));
    c.set({{3, at(0xd0)}, {5, 7}, {4, c.toc(-0x7b80)}});
    c.call(0x10717890, 0x102070bc);
    c.put32(at(0x38), 0xfffffffe);
    c.put32(at(0x3c), 1);
    c.set({{3, c.u32(c.toc(-0x7a60))}, {4, r16 & 0xffff}, {5, r17}, {6, c.u16(at(0xd0))}, {7, c.u16(at(0xd2))},
           {8, c.u32(club())}, {9, at(0x130)}, {10, 0xffffffff}});
    c.call(0x1048b180, 0x102070fc);
    if (compare(c, std::int32_t(c.r(3) & 0xffff)) == 0) return;

    const auto competition_row = [&] { return c.u32(c.toc(-0x7bbc)) + c.u32(c.r(27)) * 0x6c; };
    // Our strength (r4) from the opposing record `team` (the caller's argument),
    // and a rating row: the competition's own when the reference record's first
    // word is the current one (the word r28 points at), else the reference record.
    const auto reference = c.u32(extras + lineup_extras::reference);
    std::uint32_t own;
    if (compare(c, c.i32(reference), c.i32(r28)) == 0) {
        own = std::uint32_t(rating_of(team(), competition_row()));
    } else {
        own = std::uint32_t(rating_of(team(), reference));
        if (compare(c, s8(c.u8(extras + lineup_extras::kind_byte)), 8) == 0) {
            // A fixture of kind 8: a flagged round (compared against the side's number,
            // or against the competition's code in several ranges).
            bool wanted = compare(c, c.u8(self + lineup_side::slot), 1) == 0;
            if (!wanted) {
                const std::int32_t code = c.i16(extras + 0x30);
                wanted = compare(c, code, 0x78) == 0 || compare_logical(c, std::uint16_t(code - 0x79), 3) <= 0 ||
                         compare_logical(c, std::uint16_t(code - 0x82), 5) <= 0 ||
                         compare_logical(c, std::uint16_t(code - 0x96), 4) <= 0 || compare(c, code, 0x9b) == 0;
            }
            if (wanted) c.put8(self + lineup_side::changed, 1);
        } else if (compare(c, c.i32(reference), c.i32(c.toc(-0x7a30))) == 0 &&
                   compare(c, s8(c.u8(extras + 0x42)), -1) == 0) {
            c.put8(self + lineup_side::changed, 1);
        }
    }

    // Own strength scaled by a factor of the squad's form (a signed byte at the
    // match's team data), capped at the pool's ceiling: r16, stored by fctiwz at 0x8ec.
    {
        const auto form = c.i8(c.u32(self + lineup_side::match) + s8(c.u8(self + lineup_side::slot)) + 0x1c6);
        c.put32(at(0x8fc), 0x43300000);
        c.put32(at(0x900), std::uint32_t(std::int32_t(form)) ^ 0x80000000u);
        const double conversion = c.f64(pool() + lineup_pool::conversion);
        const double raw = fpu.fsub(c.f64(at(0x8fc)), conversion);
        const double quotient = fpu.fdiv(raw, c.f64(pool() + lineup_pool::divisor));
        c.set_f(1, quotient);
        double factor = fpu.fadd(c.f64(pool() + lineup_pool::floor), quotient);
        fpu.fcmp(0, factor, c.f64(pool() + lineup_pool::ceiling));
        if (c.cr(0) & 4) factor = c.f64(pool() + lineup_pool::ceiling);
        const double strength = convert(at(0x8f4), std::int32_t(own));
        c.set_f(1, c.f64(pool() + lineup_pool::conversion));
        r16 = truncated_to_stack(c, fpu, at(0x8ec), fpu.fmul(strength, factor));
    }
    c.set({{3, team()}, {4, club()}});
    c.call(0x10464160, 0x102072ac);
    if (compare(c, s8(c.r(3))) != 0) {
        c.put8(self + lineup_side::changed, 1);
        r16 = scaled(r16, 0x18);
    } else if (is_club_slot(club(), team())) {
        r16 = scaled(r16, 0x20);
    }

    // The fixture's person (extras word 0 indexes the people table) must be ours to judge.
    const auto person_of = [&](std::uint32_t id) { return c.u32(c.u32(c.toc(-0x7b5c)) + id * 4); };
    c.set({{3, person_of(c.u32(c.u32(self + lineup_side::extras)))}, {4, club()}});
    c.call(0x10452ed0, 0x10207374);
    if (compare(c, s8(c.r(3))) == 0) {
        c.put8(self + lineup_side::changed, 0);
        const auto manager = c.u32(self + lineup_side::manager);
        if (compare_logical(c, manager) != 0) {
            // r16 times the manager's byte (+0x68) over the pool's 0x28 divisor.
            const double ratio = fpu.fdiv(convert(at(0x8f4), s8(c.u8(manager + 0x68))), c.f64(pool() + 0x28));
            const double product = fpu.fmul(convert(at(0x8ec), std::int32_t(r16)), ratio);
            c.set_f(1, c.f64(pool() + lineup_pool::conversion));
            r16 = truncated_to_stack(c, fpu, at(0x8fc), product);
        } else {
            r16 = std::uint32_t(std::int32_t(r16) / 2);  // rlwinm/add/srawi: halves toward zero
        }
    }

    // The record of the opposing fixture (the first of the container at sp+0x130).
    c.put32(at(0x13c), c.u32(at(0x134)));
    c.set(3, at(0x130));
    c.call(0x10738e00, 0x10207404);
    r26 = c.r(3);
    if (compare(c, std::int32_t(std::uint32_t(r26))) == 0) return;
    if (compare(c, c.i32(r26), -2) == 0) return;
    c.set({{3, at(0xc8)}, {5, c.u16(r26 + 0x28)}, {4, std::uint32_t(std::int32_t(std::int16_t(c.u16(r26 + 0x2a))))}});
    c.call(0x107156b0, 0x1020742c);
    c.put16(at(0x122), c.u16(at(0xca)));
    c.put16(at(0x120), c.u16(at(0xc8)));
    c.put8(at(0x124), c.u8(at(0xcc)));

    opponent_strength();
    apply_date_factor();
    set_urgency();
}

// 0x10207444-0x10207890: the opposing strength r17 of the fixture r26, from the
// other club's rating, then scaled by what kind of club and rival it is.
void ComputerLineup::opponent_strength() {
    const auto self = side();
    const bool at_home = compare_logical(c, club(), c.u32(r26 + 0x1c)) == 0;
    const auto other = c.u32(r26 + (at_home ? 0x20 : 0x1c));
    const auto reference = c.u32(r26 + 0x14);
    const auto row = compare(c, c.i32(reference), c.i32(r28)) == 0
                         ? c.u32(c.toc(-0x7bbc)) + c.u32(c.r(27)) * 0x6c : reference;
    r17 = std::uint32_t(rating_of(other, row));
    c.set({{3, other}, {4, club()}});
    c.call(0x10464160, at_home ? 0x102074d8 : 0x10207688);
    if (compare(c, s8(c.r(3))) != 0) r17 = scaled(r17, 0x20);
    else if (is_club_slot(club(), other)) r17 = scaled(r17, 0x30);
    // A rival of the same group as the fixture's own club: a different factor.
    if (compare_logical(c, c.u32(r26 + 0x18), c.u32(club() + 0x3c)) != 0) {
        r17 = compare_logical(c, c.u32(club() + 0x2c), c.u32(other + 0x2c)) == 0 ? scaled(r17, 0x40) : scaled(r17, 0x38);
    }
    const auto person = c.u32(c.u32(c.toc(-0x7b5c)) + c.u32(r26) * 4);
    c.set({{3, person}, {4, club()}});
    c.call(0x10452ed0, 0x102077d8);
    if (compare(c, s8(c.r(3))) == 0) {
        r17 = std::uint32_t(std::int32_t(r17) / 3);  // mulhw by 0x55555556
        return;
    }
    c.set(3, c.u32(c.u32(self + lineup_side::extras)));
    c.call(0x104646d0, 0x10207804);
    if (compare(c, s8(c.r(3))) == 0) return;
    const auto kind = s8(c.u8(r26 + 0x3a));
    if (compare(c, kind, 8) != 0 && compare(c, kind, 7) != 0) return;
    const auto id = c.i32(r26);
    if (compare(c, id, -1) <= 0) return;
    const auto row_person = c.u32(c.u32(c.toc(-0x7b5c)) + std::uint32_t(id) * 4);
    if (compare_logical(c, row_person) == 0) return;
    const auto detail = c.u32(row_person + 8);
    if (compare_logical(c, detail) == 0) return;
    c.set({{3, detail}, {4, club()}});
    c.call(0x10459b50, 0x1020785c);
    bool halve = compare_logical(c, c.r(3)) == 0;
    if (!halve) {
        c.set(3, c.u8(c.r(3) + 0x75));
        halve = compare_logical(c, c.r(3)) == 0;
        if (!halve) {
            // More than six: scaled down to six over the count (divw).
            if (compare_logical(c, c.r(3), 6) > 0) r17 = std::uint32_t(std::int32_t(r17 * 6) / std::int32_t(c.r(3)));
            return;
        }
    }
    r17 = std::uint32_t(std::int32_t(r17) / 2);
}

// 0x10207890-0x10207c3c: the fixture's date is looked up among seven dated
// records (0x10717890's results 1 to 7); the first that matches scales r17 by
// its pool factor (the fourth only truncates it).
void ComputerLineup::apply_date_factor() {
    std::uint32_t index = 0;
    for (const auto& check : date_checks) {
        c.set({{4, c.toc(-0x7b80)}, {3, at(check.buffer)}, {5, ++index}});
        c.call(0x10717890, check.back);
        // The eight bytes of the result are copied, and compared with the fixture's date.
        c.put32(at(check.copy), c.u32(at(check.buffer)));
        c.put32(at(check.copy + 4), c.u32(at(check.buffer + 4)));
        const bool same = compare_logical(c, c.u16(at(check.copy + 2)), c.u16(at(0x122))) == 0 &&
                          compare(c, c.i16(at(check.copy)), c.i16(at(0x120))) == 0;
        if (compare(c, same, 0) == 0) continue;
        if (check.factor) {
            r17 = scaled(r17, check.factor);
        } else {
            const double plain = convert(at(0x8ec), std::int32_t(r17));
            c.set_f(1, c.f64(pool() + lineup_pool::conversion));
            r17 = truncated_to_stack(c, fpu, at(0x8f4), plain);
        }
        return;
    }
}

// 0x10207c3c-0x10207d58: the urgency byte, from how far the opposing strength r17
// rises above the pool's fractions of ours (r16), for a side of enough quality.
void ComputerLineup::set_urgency() {
    const auto self = side();
    const auto rises_above = [&](std::uint32_t factor) {
        const double limit = fpu.fmul(c.f64(pool() + factor), convert(at(0x8f4), std::int32_t(r16)));
        const double opposing = convert(at(0x8ec), std::int32_t(r17));
        c.set_f(1, opposing);
        fpu.fcmp(0, opposing, limit);
        return (c.cr(0) & 4) != 0;
    };
    if (rises_above(0x78) && compare(c, s8(r15), 0x28) > 0 && compare(c, std::int32_t(r16), 0x1b58) < 0) {
        c.put8(self + lineup_side::urgency, 0);
        return;
    }
    if (rises_above(0x10) && compare(c, s8(r15), 0x23) > 0 && compare(c, std::int32_t(r16), 0x2134) < 0) {
        c.put8(self + lineup_side::urgency, 1);
        return;
    }
    if (rises_above(0x40) && compare(c, std::int32_t(r16), 0x2710) < 0) c.put8(self + lineup_side::urgency, 2);
}
}

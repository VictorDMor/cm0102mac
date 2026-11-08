// The position groups of 0x10216830's staff fit (match_man_rating_b_fit.hpp).
//
// Every term is a signed byte of the player data (p<offset>) or of the judge's staff data
// (s<offset>), multiplied and divided by a constant. The compiler divides by multiplying with a
// magic number (mulhw), shifting, and adding the quotient's sign bit; that is `x / divisor`
// (truncating) and nothing else, checked for the magic of each divisor over +-2^17 and by the
// checker's cases. The scratch values of those sequences (the high product, the shifted value,
// the sign bit) stay in r3-r22, which the later calls and the caller see, so each term keeps
// them and each stage sets the registers the original leaves holding them.
#include "match_man_rating_b_fit.hpp"

namespace cm::aot::match_man::appraisal {
namespace {
using guest::compare;

struct Divisor {
    u32 magic;
    unsigned shift;
    i32 divisor;
    bool add_dividend;  // the magic is above 2^31: the dividend is added back to the high product
};
constexpr Divisor by_3{0x55555556, 0, 3, false}, by_5{0x66666667, 1, 5, false}, by_6{0x2aaaaaab, 0, 6, false},
    by_10{0x66666667, 2, 10, false}, by_15{0x88888889, 3, 15, true}, by_40{0x66666667, 4, 40, false},
    by_60{0x88888889, 5, 60, true}, by_75{0x1b4e81b5, 3, 75, false}, by_100{0x51eb851f, 5, 100, false},
    by_150{0x1b4e81b5, 4, 150, false}, by_175{0x5d9f7391, 6, 175, false}, by_200{0x51eb851f, 6, 200, false},
    by_250{0x10624dd3, 4, 250, false}, by_325{0x64d319ff, 7, 325, false}, by_350{0x5d9f7391, 7, 350, false},
    by_400{0x51eb851f, 7, 400, false}, by_500{0x10624dd3, 5, 500, false}, by_1500{0x057619f1, 5, 1500, false};

// x / divisor, with the values the instructions leave in registers on the way.
struct Quotient {
    i32 mul;      // mulhw
    i32 shifted;  // after the arithmetic shift
    i32 sign;     // the shifted value's sign bit (rlwinm)
    i32 value;    // the quotient
};
Quotient divide(i32 x, const Divisor& d) {
    i32 high = mulhw(i32(d.magic), x);
    const i32 mul = high;
    if (d.add_dividend) high = i32(reg(high) + reg(x));
    const i32 shifted = high >> d.shift;
    return {mul, shifted, sign_bit(shifted), x / d.divisor};
}
// x / 2^k by srawi and addze: the shifted value, then the quotient.
struct Halved {
    i32 shifted, value;
};
Halved halve(i32 x, unsigned k) { return {x >> k, x / (1 << k)}; }

constexpr u32 to_u32(i32 v) { return u32(v); }
i32 byte_at(Cpu& c, u32 base, u32 offset) { return s8(c.u8(base + offset)); }

constexpr u32 magic_88888889 = 0x88888889, magic_66666667 = 0x66666667, magic_10624dd3 = 0x10624dd3;
}

void staff_fit_sum(Cpu& c, const FitRecords& records) {
    const auto p = [&](u32 offset) { return byte_at(c, records.player_data, offset); };
    const auto s = [&](u32 offset) { return byte_at(c, records.staff, offset); };

    // The three terms min(coach * (rating - 10) / 3, 10): the staff's 0x34 against the player's 0x44,
    // the same against the subject's 0x67, and the judge's 0x65 against the subject's 0x65.
    const i32 p44 = p(0x44), s34 = s(0x34);
    const i32 floor_term = p44 - 10;  // also r19: the staff terms below weigh it
    const Quotient first = divide(s34 * floor_term, by_3);
    compare(c, first.value, 10);
    const i32 first_capped = first.value < 10 ? first.value : 10;
    const Quotient second = divide(s34 * (byte_at(c, records.subject, 0x67) - 10), by_3);
    compare(c, second.value, 10);
    const i32 second_capped = second.value < 10 ? second.value : 10;
    const i32 subject_65 = byte_at(c, records.subject, 0x65), judge_65 = byte_at(c, records.judge, 0x65);
    const Quotient third = divide(judge_65 * (subject_65 - 10), by_3);
    compare(c, third.value, 10);
    const i32 third_capped = third.value < 10 ? third.value : 10;
    i32 sum = first_capped + second_capped + third_capped;
    const i32 role = s16(records.role);
    compare(c, role & 1, 0);
    c.set({{0, to_u32(role)}, {3, to_u32(p44)}, {4, to_u32(sum)}, {5, to_u32(role & 1)}, {6, to_u32(judge_65)},
           {7, 0x55555556}, {8, to_u32(third_capped)}, {19, to_u32(floor_term)}});
    if (role & 1) return;  // goalkeepers: the sum as it stands

    // flag: 1 unless the role has 0x80 or 0x800; the tests run in order, each compare leaving its
    // result in CR0.
    compare(c, role & 0x80, 0);
    bool plain = false;
    if ((role & 0x80) == 0) {
        compare(c, role & 0x800, 0);
        plain = (role & 0x800) == 0;
    }
    const i32 flag = plain ? 1 : 0;
    const i32 sides = (role & 4) ? (role & 4) : (role & 2);
    compare(c, role & 4, 0);
    if ((role & 4) == 0) compare(c, role & 2, 0);
    c.set({{5, to_u32(flag)}, {6, to_u32(sides)}, {7, to_u32(flag)}});

    // The role's tests, one register each: 8, 0x10, 0x20, 0x40 (r6 for the first two, r3 for the others).
    const auto has = [&](u32 mask, unsigned into) {
        compare(c, role & i32(mask), 0);
        c.set(into, to_u32(role & i32(mask)));
        return (role & i32(mask)) != 0;
    };

    if (sides == 0) {
        if (has(8, 6)) {  // position 8
            const i32 s3a = s(0x3a), p32 = p(0x32), s3f = s(0x3f), p26 = p(0x26), p43 = p(0x43), p27 = p(0x27),
                      p28 = p(0x28), s2e = s(0x2e);
            const Quotient t_p = divide(s3a * p32, by_40);
            const Quotient t_d = divide(p27, by_15);
            const Quotient t_h = divide(s3f * floor_term, by_6);
            const Halved t_e = halve(s2e * (p28 - 10), 3);
            const Quotient t_g = divide(s2e * p26, by_325);
            const Quotient t_f = divide(s2e * p43, by_150);
            sum = sum - 10 + t_p.value + t_h.value + t_d.value + t_e.value + t_g.value + t_f.value;
            compare(c, flag, 0);
            c.set({{4, to_u32(sum)}, {5, to_u32(t_f.value)}, {6, to_u32(t_f.sign)}, {7, to_u32(t_g.sign)},
                   {8, magic_88888889}, {9, to_u32(t_e.value)}, {10, to_u32(t_e.shifted)}, {11, to_u32(t_h.value)},
                   {12, to_u32(t_d.sign)}, {13, to_u32(t_p.sign)}, {14, to_u32(s2e)}, {16, 0x1b4e81b5},
                   {20, to_u32(p43)}, {21, 0x64d319ff}, {22, to_u32(p26)}});
            if (flag != 0) {
                // r3 still holds the player's 0x44 rating from the first terms.
                const Quotient t_j = divide(p(0x3e), by_15);
                sum = sum + p44 - 15 + t_j.value;
                c.set({{3, to_u32(t_j.value)}, {4, to_u32(sum)}, {5, to_u32(t_j.sign)}});
            }
            return;
        }
        if (has(0x10, 6)) {  // position 0x10
            const i32 s3a = s(0x3a), p32 = p(0x32), p28 = p(0x28), s3f = s(0x3f), p26 = p(0x26), p27 = p(0x27),
                      s2e = s(0x2e), p43 = p(0x43);
            const Quotient t_p = divide(s3a * p32, by_60);
            const Quotient t_h = divide(s3f * floor_term, by_6);
            const Quotient t_d = divide(p27, by_10);
            const Quotient t_e = divide(s2e * (p28 - 10), by_5);
            const Quotient t_g = divide(s2e * p26, by_150);
            const Quotient t_f = divide(s2e * p43, by_100);
            sum = sum - 10 + t_p.value + t_h.value + t_d.value + t_e.value + t_g.value + t_f.value;
            compare(c, flag, 0);
            c.set({{4, to_u32(sum)}, {5, to_u32(t_f.value)}, {6, to_u32(t_f.sign)}, {7, to_u32(t_g.sign)},
                   {8, to_u32(t_e.sign)}, {9, to_u32(t_h.value)}, {10, to_u32(t_h.mul)}, {11, to_u32(s2e * p26)},
                   {12, to_u32(t_p.sign)}, {13, magic_88888889}, {14, to_u32(s2e)}, {16, 0x51eb851f},
                   {19, 0x1b4e81b5}, {20, to_u32(p43)}, {21, 0x1b4f0000}, {22, to_u32(p26)}});
            if (flag != 0) {
                const Quotient t_l = divide(p(0x3e), by_15), t_m = divide(p(0x37), by_15);
                sum = sum + p44 - 15 + t_l.value + t_m.value;
                c.set({{3, to_u32(t_m.value)}, {4, to_u32(sum)}, {5, to_u32(t_m.sign)}, {6, to_u32(t_l.value)},
                       {7, to_u32(t_l.mul)}});
            } else {
                const Quotient t_k = divide(p(0x23), by_15);
                sum += t_k.value;
                c.set({{3, to_u32(t_k.value)}, {4, to_u32(sum)}, {5, to_u32(t_k.sign)}});
            }
            return;
        }
        if (has(0x20, 3)) {  // position 0x20
            const i32 s3a = s(0x3a), p32 = p(0x32), s3f = s(0x3f), p28 = p(0x28), s2e = s(0x2e), p43 = p(0x43),
                      p26 = p(0x26), p27 = p(0x27);
            const u32 p26_raw = c.u8(records.player_data + 0x26);
            const Quotient t_p = divide(s3a * p32, by_150);
            const Halved t_h = halve(s3f * floor_term, 3);
            const Halved t_d = halve(p27, 3);
            const Halved t_e = halve(s2e * (p28 - 10), 2);
            const Quotient t_g = divide(s2e * p26, by_150);
            const Quotient t_f = divide(s2e * p43, by_75);
            sum = sum - 10 + t_p.value + t_h.value + t_d.value + t_e.value + t_g.value + t_f.value;
            compare(c, flag, 0);
            c.set({{3, to_u32(t_f.value)}, {4, to_u32(sum)}, {5, to_u32(t_f.sign)}, {6, to_u32(t_g.shifted)},
                   {7, to_u32(p43)}, {8, p26_raw}, {9, to_u32(p26)}, {10, to_u32(s2e * p26)},
                   {11, to_u32(t_e.value)}, {12, to_u32(t_d.shifted)}, {13, to_u32(t_h.value)},
                   {14, to_u32(t_p.shifted)}, {16, to_u32(t_p.sign)}});
            if (flag != 0) {
                const i32 p1a = p(0x1a);
                const Quotient t_n = divide(p43, by_15);
                sum = sum + p1a - 15 + t_n.value;
                c.set({{3, to_u32(t_n.value)}, {4, to_u32(sum)}, {5, to_u32(t_n.sign)}});
            } else {
                const Quotient t_k = divide(p(0x23), by_15);
                sum += t_k.value;
                c.set({{3, to_u32(t_k.value)}, {4, to_u32(sum)}, {5, to_u32(t_k.sign)}, {6, magic_88888889}});
            }
            // Only a player over 30 (0x26) with at least 16 (0x36) gets the last term.
            compare(c, p26, 0x1e);
            c.set(3, to_u32(p26));
            if (p26 <= 0x1e) return;
            const u32 p36_raw = c.u8(records.player_data + 0x36);
            const i32 p36 = s8(p36_raw);
            compare(c, p36, 0xf);
            c.set({{3, p36_raw}, {8, to_u32(p36)}});
            if (p36 <= 0xf) return;
            const i32 p1b = p(0x1b);
            const Quotient thirds = divide(p26, by_3);
            const Quotient t_q = divide(p1b * (thirds.value * p36), by_1500);
            sum += t_q.value;
            c.set({{3, to_u32(t_q.value)}, {4, to_u32(sum)}, {5, to_u32(t_q.sign)}, {6, to_u32(p1b)}, {7, 0x057619f1}});
            return;
        }
        if (has(0x40, 3)) {  // position 0x40
            const i32 s3a = s(0x3a), p32 = p(0x32), p28 = p(0x28), s3f = s(0x3f), s2e = s(0x2e), p26 = p(0x26),
                      p27 = p(0x27), p43 = p(0x43);
            const Quotient t_p = divide(s3a * p32, by_400);
            const Quotient t_h = divide(s3f * floor_term, by_10);
            const Quotient t_e = divide(s2e * (p28 - 10), by_3);
            const Quotient t_d = divide(p27, by_5);
            const Quotient t_g = divide(s2e * p26, by_175);
            const Quotient t_f = divide(s2e * p43, by_200);
            sum = sum - 10 + t_p.value + t_h.value + t_e.value + t_d.value + t_g.value + t_f.value;
            compare(c, flag, 0);
            c.set({{3, 0x55555556}, {4, to_u32(sum)}, {5, to_u32(t_f.value)}, {6, to_u32(t_f.sign)},
                   {7, to_u32(t_g.sign)}, {8, 0x51eb851f}, {9, to_u32(t_e.value)}, {10, to_u32(p27)},
                   {11, to_u32(p26)}, {12, to_u32(t_d.sign)}, {13, 0x5d9f7391}, {14, to_u32(t_h.value)},
                   {16, to_u32(t_h.sign)}, {19, to_u32(t_p.sign)}, {20, to_u32(t_p.mul)}});
            if (flag == 0) return;
            const i32 p33 = p(0x33), p40 = p(0x40);
            const u32 p36_raw = c.u8(records.player_data + 0x36);
            const i32 p36 = s8(p36_raw);
            const Quotient t_r = divide(p27, by_15), t_s = divide(p33, by_15);
            sum = sum + t_r.value + t_s.value + p40 - 10 + p36 - 10;
            compare(c, p26, 0x1e);
            c.set({{4, to_u32(sum)}, {5, to_u32(p36)}, {6, to_u32(p40)}, {7, to_u32(t_s.value)}, {8, to_u32(t_s.sign)},
                   {9, to_u32(t_r.value)}, {10, to_u32(t_r.sign)}, {12, p36_raw}});
            if (p26 <= 0x1e) return;
            compare(c, p36, 0xf);
            if (p36 <= 0xf) return;
            const i32 p1b = p(0x1b);
            const Quotient thirds = divide(p26, by_3);
            const Quotient t_q = divide(p1b * (thirds.value * p36), by_1500);
            sum += t_q.value;
            c.set({{3, to_u32(t_q.value)}, {4, to_u32(sum)}, {5, to_u32(t_q.sign)}, {6, to_u32(thirds.mul)},
                   {7, to_u32(p1b)}, {8, 0x057619f1}});
        }
        return;
    }

    // Positions 2 and 4.
    const i32 s3a = s(0x3a), p32 = p(0x32), s2e = s(0x2e), p27 = p(0x27), p28 = p(0x28), p43 = p(0x43),
              p26 = p(0x26), s3f = s(0x3f);
    const Quotient t_p = divide(s3a * p32, by_40);
    const Halved t_h = halve(s3f * floor_term, 3);
    const Quotient t_d = divide(s2e * p27, by_350);
    const Quotient t_e = divide(s2e * (p28 - 10), by_10);
    const Quotient t_g = divide(s2e * p26, by_500);
    const Quotient t_f = divide(s2e * p43, by_250);
    sum = sum - 10 + t_p.value + t_h.value + t_d.value + t_e.value + t_g.value + t_f.value;
    compare(c, flag, 0);
    c.set({{3, to_u32(t_f.value)}, {4, to_u32(sum)}, {5, to_u32(t_f.sign)}, {6, to_u32(t_g.sign)},
           {7, to_u32(t_e.sign)}, {8, to_u32(t_d.sign)}, {9, magic_66666667}, {10, to_u32(s2e)}, {11, to_u32(p43)},
           {12, magic_10624dd3}, {13, to_u32(p26)}, {14, to_u32(s2e * (p28 - 10))}, {16, 0x5d9f7391}});
    if (flag != 0) {
        const i32 p2e = p(0x2e);
        sum = sum + p2e - 15;
        c.set({{3, to_u32(p2e)}, {4, to_u32(sum)}});
    }
}

void goalkeeper_trait_bonus(Cpu& c, u32 staff, u32 player_data) {
    const i32 p2e = byte_at(c, player_data, 0x2e), p36 = byte_at(c, player_data, 0x36), s33 = byte_at(c, staff, 0x33);
    const Quotient bonus = divide(s33 * (p2e * p36 - 200), by_500);
    c.set({{0, to_u32(bonus.value)}, {3, to_u32(bonus.sign)}, {4, c.r(4) + to_u32(bonus.value)},
           {5, to_u32(s33)}, {6, magic_10624dd3}});
}
}

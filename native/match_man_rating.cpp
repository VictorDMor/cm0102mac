// MATCH_MAN.CPP rating block (match_man_rating.hpp).
#include "match_man_rating.hpp"
#include "match_man_rating_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::nonzero_logical;

// The values as registers hold them after extsb, extsh.
constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
constexpr std::uint32_t reg(std::int32_t value) { return std::uint32_t(value); }

// The pool at TOC -0x5278 (r31): double constants, read by offset.
namespace pool {
constexpr std::uint32_t conversion = 0x90;       // 2^52 + 2^31, for the int-to-double idiom
constexpr std::uint32_t rounding = 0x210;        // added to the scaled judging value before truncation (inferred)
constexpr std::uint32_t noise_divisor = 0x2a8;   // the noise is the seed's remainder over this (inferred)
}
// The pool at TOC -0x527c: a single-precision tuning value, read at +0x24.
constexpr std::uint32_t lucky_scale = 0x24;

constexpr std::uint32_t known_scale = 50;
// 0x100cdef0 and 0x100d2f50 (DATABASE.CPP).
constexpr std::uint32_t estimate_of = 0x100cdef0, attribute_rating = 0x100d2f50;

// The common ending of the blend: the true value `base` of the player and the
// judge's `estimate` are mixed by the skill, with a noise whose span narrows as
// the skill rises, and the divisions by 20 are the original's mulhw idiom.
// Leaves r4-r10 as the original does.
std::int32_t blend(Cpu& c, std::int32_t seed, std::uint32_t skill, std::uint32_t estimate, std::int32_t base) {
    const std::int32_t level = s8(skill);                  // 1 to 20 (or any lower byte)
    const std::int32_t room = 21 - level;                  // subfic r10
    const std::int32_t span = room * 2;                    // rlwinm r9: always 2 or more
    const std::int32_t quotient = seed / span;             // divw r8
    const std::int32_t remainder = seed - quotient * span; // mullw, subf
    const std::int32_t weight = s8(reg(level / 2 + 10));   // srawi 1 with the sign added: rounds to zero
    const std::int32_t noise = s16(reg(remainder - room));
    const std::int32_t mixed = weight * (base + noise) + s16(estimate) * (20 - weight);
    const std::int32_t shown = s16(reg(mixed / 20));       // mulhw by 0x66666667, srawi 3: a division
    c.set({{4, reg(s16(estimate))}, {5, reg(base)}, {6, reg(weight)}, {7, 0x66666667}, {8, reg(quotient)},
           {9, reg(span)}, {10, reg(room)}});
    return guest::result(c, reg(shown));
}
}

std::int32_t judged_ability(Cpu& c, std::uint32_t judge, std::uint32_t person_record, std::uint32_t kind,
                            std::int16_t known, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x10219e40;
    c.set(0, c.lr());
    // r23-r31 at -0x34 (not the stmw default: a word of padding under f31).
    Frame frame(c, entry, 0x80, {.first_gpr = 23, .gpr_offset = -0x34, .first_fpr = 31});
    Fpu fpu(c);
    const auto pool_address = c.toc(-0x5278);
    const auto seed_global = c.toc(-0x7450);
    // r7 and r8 reach the callees and, on one path, the caller.
    c.set({{7, c.u32(person_record + person::id)}, {8, seed_global}});
    // What the original keeps in r23-r31 across its calls. r26 and r27 keep the
    // caller's values until set.
    guest::keep(c, {{23, judge}, {24, person_record}, {25, kind}, {31, pool_address}});
    Kept<26> estimate(c, c.r(26));      // the judge's estimate, small (inferred: 0 to 4)
    Kept<27> age_penalty(c, c.r(27));   // years past the player's prime, as a signed byte (inferred)
    // The noise seed: the person's id plus a global word (inferred: the day),
    // plus the judge's id below.
    Kept<28> seed_register(c, c.r(7) + c.u32(seed_global));
    Kept<29> skill(c, 1);               // the judge's effective skill, 1 to 20
    Kept<30> staff(c, 0);               // the judge's staff data
    const auto seed = [&] { return std::int32_t(std::uint32_t(seed_register)); };

    // The estimate: the given one, else the judge's (or the club's judge's) own.
    if (compare(c, std::int32_t(known), -1) > 0) {
        estimate = reg(std::int32_t(known) / std::int32_t(known_scale));  // divw
    } else {
        std::uint32_t asked = judge;
        if (!nonzero_logical(c, judge)) asked = c.u32(club + rating_club::default_judge);
        c.set({{3, person_record}, {4, asked}, {5, 0}});
        c.call(estimate_of, judge ? 0x10219eb0 : 0x10219ee4);
        // mulhw by 0x51eb851f, srawi 4/6: truncating divisions by 50 and 200.
        estimate = reg(std::int32_t(std::int16_t(c.r(3))) / std::int32_t(known_scale));
    }

    const auto data = [&] { return c.u32(person_record + person::player); };
    if (compare(c, s8(kind), current_kind) == 0) {
        age_penalty = 0;
    } else {
        // Years past 26 (goalkeepers) or 23, as signed bytes: used only to
        // lower a potential estimate below.
        const bool goalkeeper = compare(c, c.u8(data() + player_data::goalkeeper_rating), 20) == 0;
        const auto age = std::int8_t(c.i16(person_record + person::age));
        age_penalty = reg(std::int8_t(age - (goalkeeper ? 0x1a : 0x17)));
    }

    if (nonzero_logical(c, judge)) {
        staff = c.u32(judge + person::staff);
        seed_register = seed_register + c.u32(judge + person::id);
        // A judge of a current ability whose estimate is above 50 sometimes sees
        // the true value: when the judging skill, as a bonus over 10, raises the
        // estimate above the seed's remainder over 200 (inferred reading).
        if (nonzero_logical(c, staff) && compare(c, s8(kind), current_kind) == 0 &&
            compare(c, s16(estimate), 50) > 0) {
            c.set({{3, staff}, {4, c.u8(staff + staff_data::judging_current)}});
            c.call(attribute_rating, 0x10219f8c);
            const std::int32_t bonus = s8(c.r(3)) - 10;
            const std::int32_t remainder = seed() - seed() / 200 * 200;
            c.set({{4, reg(s16(estimate))}, {5, reg(bonus)}});
            if (compare(c, s16(estimate) + bonus * 10, remainder) > 0)
                return guest::result(c, reg(c.i16(data() + player_data::current)));
        }
    }

    // The noise in [0, 1): the seed's remainder over 101, divided by the pool's
    // divisor (a double division, rounded to single).
    const auto conversion = c.f64(pool_address + pool::conversion);
    const std::int32_t roll = seed() - seed() / 101 * 101;  // mulhw by 0x288df0cb, srawi 4
    const double numerator = converted_int(c, fpu, frame.at(0x40), roll, conversion);
    c.set_f(1, numerator);
    const double noise = fpu.frsp(fpu.fdiv(numerator, c.f64(pool_address + pool::noise_divisor)));
    c.set_f(31, noise);

    // The judge's effective skill, from the judging attribute scaled by the noise.
    if (nonzero_logical(c, staff)) {
        const auto rounding = c.f64(pool_address + pool::rounding);
        if (compare(c, seed() - seed() / 10 * 10, 0) == 0) {
            // One roll in ten: the tuning value instead of the attribute.
            c.set_f(1, rounding);
            const double scaled = fpu.fmuls(double(c.f32(c.toc(-0x527c) + lucky_scale)), noise);
            skill = truncated_to_stack(c, fpu, frame.at(0x40), fpu.fadd(rounding, scaled));
        } else {
            // The attribute is read from +0x37 for a current ability, +0x38 for
            // the potential. The stack words used by the conversion and by the
            // result swap places between the two cases.
            const bool current = compare(c, s8(kind), current_kind) == 0;
            c.set({{3, staff}, {4, c.u8(staff + (current ? staff_data::judging_current : staff_data::judging_potential))}});
            c.call(attribute_rating, current ? 0x1021a084 : 0x1021a0cc);
            const auto convert_at = frame.at(current ? 0x40 : 0x38), store_at = frame.at(current ? 0x38 : 0x40);
            const double attribute = converted_int(c, fpu, convert_at, s8(c.r(3)), conversion);
            const double product = fpu.fmuls(noise, attribute);
            c.set_f(1, product);
            // The stored result is read at +4 from its address: skill takes the low word.
            skill = truncated_to_stack(c, fpu, store_at, fpu.fadd(rounding, product));
        }
        if (compare(c, s8(skill), 20) > 0) skill = 20;
    }

    // A current ability keeps its estimate; a potential first loses the square of
    // the years past prime. Each then blends with the true value.
    if (compare(c, s8(kind), current_kind) == 0)
        return blend(c, seed(), skill, estimate, c.i16(data() + player_data::current));
    if (compare(c, s8(age_penalty), 0) > 0) estimate = estimate - reg(s16(reg(s8(age_penalty) * s8(age_penalty))));
    return blend(c, seed(), skill, estimate, c.i16(data() + player_data::potential));
}
}

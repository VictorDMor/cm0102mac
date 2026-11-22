// PLAYER_REGEN.CPP, block `candidate_score`: the score of a person as the replacement of a retiring one (0x102dc410, the
// module's hot routine: one call per person per pick) and the pick of the best candidate (0x102dbf00).
//
// The score draws four times at most, in this order: a draw below (age - 31) * 150, a draw below a cubed term of the
// player record (through pow), a draw modulo 10000 for an unattached person, and a draw below the sum. Each is the game's
// pooled draw (player_regen_random.cpp) with the refill's return address of its own inline copy in the original, so the
// calls the checker traces are paired by them. The floating-point operations run in the original's order through
// guest::Fpu (the FPSCR it leaves is compared): conversions of integers through the bias, the fcmpu against zero, the
// fctiwz of each pow result, and the scaled draws' own. Constants come from the guest's table (TOC -0x50a8). pow is the
// host's std::pow, as the CarbonLib bridge computes it (carbon_bridge.cpp), without the glue's dispatch: the glue only
// saves and reloads r2.
#include "player_regen_detail.hpp"
#include <cmath>

namespace cm::aot::player_regen {
namespace {
constexpr std::int32_t not_a_candidate = -10000;
// A signed 32-bit sum the original computes with add (wrapping).
std::int32_t add(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) + std::uint32_t(b)); }
std::int32_t times(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) * std::uint32_t(b)); }
// A signed byte of a record as a double through the bias (exact), and the integer low word of an fctiwz.
std::int32_t low_word(std::uint64_t bits) { return std::int32_t(std::uint32_t(bits)); }
}

namespace {
// The score of one person (the body of 0x102dc410); the pick calls it directly, as the original's `bl` does.
std::int32_t score_of(Cpu& c, std::uint32_t person) {
    if (c.u32(person + person::staff) != 0) return not_a_candidate;
    const auto kind = c.u8(c.u32(c.toc(toc::kind_table)) + std::uint32_t(c.u32(person + person::id) * kind_record::size) + kind_record::class_byte);
    if (((kind >> 2) & 3) != 0) return not_a_candidate;
    // The age: the low byte of the halfword at +0x5c, signed.
    const auto age = sext8(c.u16(person + person::age));
    if (age < 31) return not_a_candidate;
    const Pool pool(c);
    Fpu fp(c);
    const auto constants = c.toc(toc::constants);
    const auto bias = c.f64(constants + constant::bias);

    // 1. A draw below (age - 31) * 150: nothing for exactly 31.
    const auto age_bound = times(age - 31, 150);
    auto score = age_bound == 0 ? 0 : pool.draw(age_bound, refill_site::age_term, refill_site::random_first);

    // 2. The club's reputation against the player's value: (value / reputation) * 250 more, when the person has a club
    // with a reputation and a player record.
    if (const auto club = c.u32(person + person::club); club != 0) {
        const auto reputation = sext16(c.u16(club + club_row::reputation));
        if (reputation != 0) {
            if (const auto player = c.u32(person + person::player); player != 0)
                score = add(score, times(divide_signed(sext16(c.u16(player + player_record::value_a)), reputation), 250));
        }
    }

    // 3. The player record: a cubed term (the byte +0x2f halved, toward zero, to the third power) drawn below, and two
    // thresholds on its halfwords.
    if (const auto player = c.u32(person + person::player); player != 0) {
        const auto half = std::int32_t(sext8(c.u8(player + player_record::potential_byte)) / 2);
        const auto base = fp.fsub(biased(half), bias);
        const auto cubed = std::pow(base, c.f64(constants + constant::cube_exponent));
        fp.fcmp(0, cubed, c.f64(constants + constant::zero));
        std::int32_t term = 0;
        if (cubed != c.f64(constants + constant::zero)) {
            // The bound is the cubed value as fctiwz truncates it (saturating, NaN to the most negative).
            pool.advance(refill_site::nation_term);
            const auto bound = low_word(fp.fctiwz(0, cubed));
            // The scaled form's inline copy leaves the sum in r7 (r8 is not written); the small form leaves both alone.
            if (bound >= 0x10000 || bound <= -0x10000) c.set(7, std::uint32_t(pool.sum()));
            term = pool.draw_advanced(bound, refill_site::random_first, &cubed);
        }
        score = add(score, term);
        const auto record = c.u32(person + person::player);
        if (sext16(c.u16(record + player_record::value_c)) > 0x1d4c) score = add(score, 100);
        if (sext16(c.u16(record + player_record::value_a)) > 0x1d4c) score = add(score, 0x32);
    }

    // 4. The attribute bytes: linear weights, and two of them squared through pow (each fctiwz'd).
    score = add(score, times(sext8(c.u8(person + person::rating_69)), 15));
    score = add(score, times(sext8(c.u8(person + person::rating_66)), 15));
    score = add(score, times(sext8(c.u8(person + person::rating_61)), 5));
    const auto square = c.f64(constants + constant::square_exponent);
    const auto first_square = std::pow(fp.fsub(biased(sext8(c.u8(person + person::rating_63))), bias), square);
    score = add(score, low_word(fp.fctiwz(0, first_square)));
    score = add(score, times(sext8(c.u8(person + person::rating_64)), 10));
    score = add(score, times(sext8(c.u8(person + person::rating_65)), 10));
    const auto second_square = std::pow(fp.fsub(biased(sext8(c.u8(person + person::rating_67))), bias), square);
    score = add(score, low_word(fp.fctiwz(0, second_square)));

    // 5. The club: without a word +0x90 (a club that has no manager, inferred) 75 more; an unattached person gets the
    // role byte squared and, when a draw modulo 10000 is below the player's halfword +0xc, a doubled score.
    if (const auto club = c.u32(person + person::club); club != 0) {
        if (c.u32(club + club_row::word_90) == 0) score = add(score, 0x4b);
    } else {
        const auto role = std::uint32_t(c.u8(person + person::role_flag));
        score = add(score, std::int32_t(role * role));
        pool.advance(refill_site::unattached_term);
        const auto draw = pool.sum();
        const auto player = c.u32(person + person::player);
        const auto rest = std::int32_t(std::uint32_t(draw) - std::uint32_t(draw / 10000) * 10000u);
        if (sext16(c.u16(player + player_record::value_c)) > rest) score = std::int32_t(std::uint32_t(score) << 1);
    }

    // 6. The last draw, below the score itself.
    if (score == 0) return 0;
    // The scaled form's inline copy leaves the sum in r7 and the sample in r8 (the small form leaves them alone): the pick's caller reads them.
    if (score >= 0x10000 || score <= -0x10000) {
        pool.advance(refill_site::final_term);
        c.set({{7, std::uint32_t(pool.sum())}, {8, c.u16(cell(c, toc::rng_cursor))}});
        return pool.draw_advanced(score, refill_site::random_first);
    }
    return pool.draw(score, refill_site::final_term, refill_site::random_first);
}
}

std::int32_t candidate_score(Cpu& c, std::uint32_t person) {
    save_link(c);
    return score_of(c, person);
}

std::uint32_t best_candidate(Cpu& c, std::uint32_t key) {
    save_link(c);
    const auto table = c.toc(toc::people), count = c.toc(toc::person_count);
    std::int32_t best = -1000;
    std::uint32_t chosen = 0, record = c.u32(table);
    const auto remaining = [&](std::int32_t index) { return index < std::int32_t(c.u32(count) - 0x10); };
    // First among the people whose nation record carries the key, then (if none scored above 0) among everyone.
    for (std::int32_t index = 0; remaining(index); ++index, record += person::size) {
        const auto nation = c.u32(record + person::nation);
        if (nation == 0 || c.i32(nation + person::nation_key) != std::int32_t(key)) continue;
        if (const auto score = score_of(c, record); score > best && score > 0) {
            best = score;
            chosen = record;
        }
    }
    if (chosen != 0) return chosen;
    best = -15000;
    record = c.u32(table);
    for (std::int32_t index = 0; remaining(index); ++index, record += person::size)
        if (const auto score = score_of(c, record); score > best && score > 0) {
            best = score;
            chosen = record;
        }
    return chosen;
}
}

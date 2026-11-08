// MANAGER_MANAGER.CPP post-match manager update block (manager_manager_update.hpp): 0x1019e8f0,
// the module's hook of the post-match broadcast; 0x1019e990, a club's manager after a match; and
// 0x101c15f0, the pending list's clean-up it ends with.
//
// Every routine keeps the original's stack image: the prologue stores, the int-to-double words
// (0x43300000 and value ^ 0x80000000), the eight bytes of each fctiwz + stfd, the argument block
// of the long calls (nine to sixteen arguments), and the registers the original holds at each
// call (the callee's prologue saves r13-r31 and f14-f31) and leaves at its end (r3-r10, cr0).
#include "manager_manager_update.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_review.hpp"
#include "manager_manager_update_detail.hpp"
#include <cmath>

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using namespace update_detail;
namespace {
namespace callee {
constexpr std::uint32_t league_has = 0x10452ed0, cup_flag = 0x10494d20, club_flag = 0x100c7d00,
    native_flag = 0x100cab10, club_state = 0x1014c2e0, ask = 0x101bdf80,
    distance_glue = 0x10780fd8, rivals = 0x10464160, club_test = 0x100ce250, event_kind = 0x101b9b50,
    event_check = 0x102cdbf0, event_post = 0x101b9130, bad_run = 0x100d3c40, notice = 0x106891b0,
    results = 0x10459b50, manager_flag = 0x102de3c0, support = 0x101b8e80, list_next = 0x10738e00,
    list_remove = 0x10738e60;
}
namespace toc {
constexpr std::int32_t pool = -0x536c, strings = -0x5368, leagues = -0x7b5c, cups = -0x7b54, people = -0x7be0,
                       events = -0x7bf0, notices = -0x7b94, scale_owner = -0x7b28, cup_holder = -0x7a2c,
                       draw_rows = -0x7984;
}

// A floating-point register the original keeps across its calls (the callees' prologues save
// it): assigning writes the register.
template <unsigned N>
class KeptFloat {
    static_assert(N >= 14 && N <= 31, "KeptFloat: not a non-volatile register");

public:
    KeptFloat(Cpu& c, double value) : c(c) { c.set_f(N, value); }
    KeptFloat(const KeptFloat&) = delete;
    KeptFloat& operator=(double value) {
        c.set_f(N, value);
        return *this;
    }
    operator double() const { return c.f(N); }

private:
    Cpu& c;
};

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::int32_t shalf(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
// The compiler's divisions and remainders by constants (mulhw, shifts, the dividend's sign bit
// added back) are the truncating C++ ones for every 32-bit input; each form was checked over all
// 2^32 values (the division by ten over the 16-bit values it receives). So the
// divisions and remainders by constants in this file (/-35, /-40, /10, %90, %2000, %120, %6) are
// written as the C++ operators.
// A step bounded to +-100 as a halfword.
std::int32_t hundred_bound(std::int32_t step) {
    const auto half = shalf(word(step));
    return half < -100 ? -100 : half > 100 ? 100 : half;
}

// The table's current number: the halfword at the cursor plus the base.
std::uint32_t table_value(const Cpu& c, const draw_table::Table& t) {
    return std::uint32_t(c.u16(c.u32(t.cursor_cell))) + c.u32(t.base_cell);
}
// A draw's remainder step for the bound of a sign-extended byte plus one (never beyond +-0xffff,
// so the original's floating-point stage for larger bounds, 0x1019f404..0x1019f4d4, is dead): the
// result takes the divisor's sign.
std::int32_t draw_remainder(std::int32_t number, std::int32_t bound) {
    const auto rest = remainder(number, bound);
    return bound > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}

// Frame slots (offsets from the frame base): the int-to-double words and fctiwz results.
namespace slot {
constexpr std::uint32_t arguments = 0x38, spill = 0x58, convert_a = 0x60, truncated_a = 0x68, convert_b = 0x70,
                        truncated_b = 0x78, convert_c = 0x80, truncated_c = 0x88, convert_d = 0x90, average = 0x98,
                        away_flag = 0x9c, toc_save = 0x14;
}

// 0x1019e990. The original keeps, in registers: the object r15, the club r16, the fixture r17, the
// opponent r29, the draw table's cells r13 and r30 and the pool r31; r19 starts as the home byte;
// r18 starts as the match record and from the result on holds the club's goals (r20 the other's,
// r23/r24 the shoot-out); r14 starts as 1 for an away side and from the outcome on holds the
// outcome flags; r21 builds those flags; r22 is the running score; r20, r23-r28 are the squad
// tallies' sums and cursors in between, and r19 a bounded score.
class Update {
public:
    Update(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t club, std::uint32_t fixture, std::uint32_t match,
           std::uint32_t home)
        : c(c), frame(frame), fpu(c), object(object), club(club), fixture(fixture), table(draw_table::cells(c)),
          pool(c.toc(toc::pool)), r14(c, c.r(14)), r18(c, match), r19(c, home), r20(c, c.r(20)), r21(c, c.r(21)),
          r22(c, c.r(22)), r23(c, c.r(23)), r24(c, c.r(24)), r25(c, c.r(25)), r26(c, c.r(26)), r27(c, c.r(27)),
          r28(c, c.r(28)), f29(c, c.f(29)), f30(c, c.f(30)), f31(c, c.f(31)) {
        guest::keep(c, {{31, pool}, {15, object}, {13, table.base_cell}, {16, club}, {30, table.cursor_cell},
                        {17, fixture}});
        f31 = c.f64(pool + pool::one);
        const bool home_side = compare(c, sbyte(home)) != 0;  // extsb. r0,r19
        opponent = c.u32(fixture + (home_side ? fixture::away : fixture::home));
        guest::keep(c, {{29, opponent}});
        const auto competition = c.u32(fixture + fixture::competition);
        f29 = c.f64(pool + pool::tier_b);
        compare_logical(c, competition);
        r21 = 0;
        r20 = 0;
        // 1 for the side that is not home, as the original keeps it (cntlzw, shifted).
        const std::uint32_t away = sbyte(home) == 0 ? 1 : 0;
        c.put32(frame.at(slot::away_flag), away);
        r14 = away;
        r23 = 0;
    }

    void run() {
        const double factor = expectation();
        f31 = fpu.fmul(f31, factor);
        c.set(3, club);
        r22 = 0;
        standing();
        squads_and_gap();
        result_outcome();
        shootout();
        rivalry();
        outcome_weights();
        scale_and_halve();
        margin_flags();
        first_event();
        second_event();
        third_event();
        board_mood();
        apply_to_row();
        clamp_row();
        board_effects();
        closing();
    }

private:
    double constant(std::uint32_t at) { return c.f64(pool + at); }
    std::uint32_t global(std::int32_t toc_slot) { return c.u32(c.toc(toc_slot)); }
    std::int32_t signed_byte(std::uint32_t at) { return sbyte(c.u8(at)); }
    std::int32_t signed_half(std::uint32_t at) { return shalf(c.u16(at)); }
    std::uint32_t row_address() { return c.u32(object + object::table) + c.u32(club + club::id) * row::size; }
    // A record-form bit test (`rlwinm. r0,rX,0,n,n`): cr0 compares the masked value with zero.
    bool has(std::uint32_t value, std::uint32_t mask) { return compare(c, std::int32_t(value & mask)) != 0; }
    void add_outcome(std::uint32_t bits) { r21 = word(shalf(r21 | bits)); }
    bool home_byte_set() { return compare(c, sbyte(r19)) != 0; }  // extsb. r0,r19
    std::uint32_t fixture_flags() { return word(signed_half(fixture + fixture::flags)); }
    std::uint32_t score() { return word(shalf(r22)); }

    // The compiler's int-to-double: the words go to the stack, the double is loaded back.
    double load_converted(std::uint32_t at, std::uint32_t flipped) {
        c.put32(frame.at(at), conversion_high);
        c.put32(frame.at(at + 4), flipped);
        return c.f64(frame.at(at));
    }
    // fctiwz into the stack double at `at`; the low word is read back.
    std::uint32_t truncate_to(std::uint32_t at, double value) {
        c.put64(frame.at(at), fpu.fctiwz(0, value));
        return c.u32(frame.at(at + 4));
    }
    // The score (a halfword) times a pool factor, truncated through the frame at 0x60 / 0x68.
    std::uint32_t scaled_score(double factor) {
        auto converted = fpu.fsub(load_converted(slot::convert_a, score() ^ 0x80000000u), constant(pool::bias));
        return truncate_to(slot::truncated_a, fpu.fmul(factor, converted));
    }

    // 0x1019ea14..0x1019ebd0: what the fixture is worth to the club, as the pool's factor f31
    // is multiplied by. A match of the club's own league also sets f29 (the score's step).
    double expectation() {
        const auto competition = c.u32(fixture + fixture::competition);
        if (competition == 0) return constant(pool::no_competition);
        const auto leagues = global(toc::leagues);
        c.set({{3, c.u32(leagues + (c.u32(fixture + fixture::id) << 2))}, {4, club}});
        c.call(callee::league_has, 0x1019ea38);
        const auto listed = sbyte(c.r(3));
        compare(c, listed);
        if (listed == 0) return constant(pool::outsider);
        c.set({{3, global(toc::cups)}, {4, c.u32(fixture + fixture::competition)}});
        c.call(callee::cup_flag, 0x1019ea58);
        const auto cup = sbyte(c.r(3));
        compare(c, cup);
        if (cup != 0) {
            c.set(3, c.u32(fixture + fixture::home));
            c.call(callee::club_flag, 0x1019ea68);
            const auto flagged = sbyte(c.r(3));
            compare(c, flagged);
            return constant(flagged != 0 ? pool::cup_home : pool::cup_away);
        }
        const auto played = sbyte(c.u8(fixture + fixture::played));
        compare(c, played);
        if (played == 0) return constant(pool::unplayed);
        const auto first = c.u32(fixture + fixture::home);
        const auto first_league = c.u32(first + club::league);
        compare_logical(c, competition, first_league);
        const auto level = signed_half(competition + competition::level);
        if (competition == first_league) {
            // The first club's own league: the step grows with the competition's level.
            const auto converted = fpu.fsub(load_converted(slot::convert_d, word(level) ^ 0x80000000u), constant(pool::bias));
            f29 = fpu.multiply_add(0x1019eacc, constant(pool::league_slope), converted, constant(pool::league_intercept));
            return constant(pool::one);
        }
        // Another competition: the gap between the level (times 500) and the club's reputation,
        // as a share of the pool's divisor, lowers the value from one.
        const auto kind = c.u8(fixture + fixture::kind);
        const auto gap = word(level) * 0x1f4u - word(signed_half(club + club::reputation));
        compare(c, std::int32_t(kind), 8);
        auto distance = fpu.fsub(load_converted(slot::convert_d, gap ^ 0x80000000u), constant(pool::bias));
        // fabs sets no FPSCR bits (as in the generated code), so it needs no Fpu.
        auto value = fpu.fsub(constant(pool::one), fpu.fdiv(std::fabs(distance), constant(pool::level_gap)));
        if (kind == fixture::league_kind) value = fpu.fmul(value, constant(pool::round_factor));
        const auto round = signed_half(fixture + fixture::round);
        compare(c, round, 0x78);
        if (round == 0x78) return fpu.fmul(value, constant(pool::round_factor));
        compare(c, round, 0x82);
        if (round == 0x82) return fpu.fmul(value, constant(pool::tier_c));
        compare(c, round, 0x8c);
        if (round == 0x8c) return fpu.fmul(value, constant(pool::tier_a));
        compare(c, round, 0x96);
        if (round == 0x96) return fpu.fmul(value, constant(pool::strong));
        compare(c, round, 0xa0);
        if (round == 0xa0) return fpu.fmul(value, constant(pool::strong));
        // The original tests 0x96 and 0x82 once more (each would then test the kind byte); both
        // rounds were handled above, so only these compares run.
        compare(c, round, 0x96);
        compare(c, round, 0x82);
        return value;
    }

    // 0x1019ebdc..0x1019ecac: whether the club or the opponent is the fixture's group favourite
    // (r22 starts at -20 / +20, +30 for another region's group).
    void standing() {
        c.call(callee::club_flag, 0x1019ebe0);
        const auto flagged = sbyte(c.r(3));
        compare(c, flagged);
        const auto venue = c.u32(fixture + fixture::venue);
        if (flagged != 0) {
            if (venue != 0) {
                const auto leader = c.u32(venue + venue::leader);
                if (leader != 0 && c.u32(leader + leader::region) == c.u32(club + club::region)) {
                    r22 = word(-0x14);
                    return;
                }
            }
            if (c.u32(club + club::group) == venue) {
                r22 = word(-0x14);
                return;
            }
            if (c.u32(opponent + club::group) == venue) {
                r22 = 0x14;
                return;
            }
            if (venue == 0) return;
            const auto leader = c.u32(venue + venue::leader);
            if (leader == 0) return;
            if (c.u32(leader + leader::region) == c.u32(opponent + club::region)) r22 = 0x14;
            return;
        }
        if (c.u32(club + club::group) == venue) {
            r22 = word(-0x14);
            return;
        }
        if (c.u32(opponent + club::group) != venue) return;
        r22 = c.u32(opponent + club::region) == c.u32(club + club::region) ? 0x14 : 0x1e;
    }

    // The mark a squad slot earns (r5 / r4 of the original): `drift` collects the spare strength
    // beyond the slot's first mark, as the first tally does (which also leaves r9).
    std::int32_t slot_mark(std::uint32_t at, unsigned k, std::int32_t* drift, bool leaves_registers = false) {
        const auto spare = [&] { return signed_half(r18 + match::spare); };
        if (signed_byte(r18 + match::neutral) != 0 && k < 11) return 0x5a;
        if (k < 11) {
            const auto first = signed_byte(at + match::mark_high);
            if (first > -1) {
                if (drift) *drift += shalf(word(spare() - first));
                return first;
            }
            const auto second = signed_byte(at + match::mark_other);
            if (second > -1) return second;
            return sbyte(word(spare()));
        }
        const auto low = c.u8(at + match::mark_low);
        if (sbyte(low) == -1) return 0;
        const auto high = c.u8(at + match::mark_high);
        if (leaves_registers) c.set(9, word(sbyte(high)));
        if (sbyte(high) > -1) {
            if (drift) *drift += shalf(word(spare() - sbyte(high)));
            return sbyte(sbyte(high) - word(low));
        }
        const auto other = c.u8(at + match::mark_other);
        if (sbyte(other) > -1) return sbyte(other - word(low));
        return sbyte(word(spare() - std::int32_t(low)));
    }

    // 0x1019ecac..0x1019f13c: with a match record, the two squads' marks move r22 through
    // floating point; without one only the reputation gap does.
    void squads_and_gap() {
        compare_logical(c, r18);
        if (r18 == 0) {
            const auto gap = std::int32_t(signed_half(club + club::reputation) - signed_half(opponent + club::reputation));
            const auto step_value = hundred_bound(gap / -35);
            r22 = r22 + word(step_value);
            c.set(6, word(step_value));
            return;
        }
        const std::uint32_t side = r14 * match::side_size;
        std::int32_t drift = 0;  // r7
        c.set(7, 0);
        // Tally 1: this side's slots (r23 the marks' sum, r20 the marks times their weights). The
        // compares end before any call: the compare after the loop overwrites cr0. The original's
        // r7-r10 survive to its end where a callee leaves them alone: r7 the drift, r8 the last
        // mark, r9 and r10 the last high and low bytes of a late slot.
        for (unsigned k = 0; k < match::slot_count; ++k) {
            const auto at = r18 + side + k * match::slot_size;
            if (c.i32(at + match::person) == -1) continue;
            if (k >= 11) c.set(10, c.u8(at + match::mark_low));
            const auto mark = slot_mark(at, k, &drift, true);
            c.set(8, word(sbyte(word(mark))));
            if (sbyte(word(mark)) == 0) continue;
            r23 = r23 + word(sbyte(word(mark)));
            r20 = r20 + word(sbyte(word(mark))) * std::uint32_t(c.u8(at + match::weight));
        }
        c.set(7, word(drift));
        compare(c, std::int32_t(r23));
        const auto bias = constant(pool::bias);
        if (r23 != 0) {  // f30 keeps the caller's value when there are no marks
            c.put32(frame.at(0x94), r20 ^ 0x80000000u);
            c.put32(frame.at(0x90), conversion_high);
            c.put32(frame.at(0x8c), r23 ^ 0x80000000u);
            c.put32(frame.at(0x88), conversion_high);
            const auto numerator = fpu.fsub(c.f64(frame.at(0x90)), bias);
            const auto denominator = fpu.fsub(c.f64(frame.at(0x88)), bias);
            f30 = fpu.fdiv(numerator, denominator);
        }
        // The sides' strength bytes (less ten for the club's, and their difference) and the drift,
        // each over its own pool divisor, added to f30 in this order.
        const std::uint32_t other_side = (r14 == 0 ? 1u : 0u) * match::side_size;
        r24 = 0;
        const auto mine_a = c.u8(r18 + side + match::strength_a);
        const auto mine_b = c.u8(r18 + side + match::strength_b);
        r27 = other_side;
        const auto other_a = c.u8(r18 + other_side + match::strength_a);
        c.put32(frame.at(slot::convert_c), conversion_high);
        c.put32(frame.at(slot::convert_c + 4), (mine_a - 10u) ^ 0x80000000u);
        c.put32(frame.at(slot::truncated_b), conversion_high);
        c.put32(frame.at(slot::truncated_b + 4), (mine_b - 10u) ^ 0x80000000u);
        c.put32(frame.at(slot::convert_b), conversion_high);
        c.put32(frame.at(slot::convert_b + 4), (std::uint32_t(mine_a) - other_a) ^ 0x80000000u);
        c.put32(frame.at(slot::truncated_a), conversion_high);
        c.put32(frame.at(slot::truncated_a + 4), word(shalf(word(drift))) ^ 0x80000000u);
        r23 = 0;
        r28 = 0;
        r25 = 0;
        const auto a1 = fpu.fsub(c.f64(frame.at(slot::convert_c)), bias);
        const auto a2 = fpu.fdiv(a1, constant(pool::strength_a));
        const auto b1 = fpu.fsub(c.f64(frame.at(slot::truncated_b)), bias);
        f30 = fpu.fadd(f30, a2);
        const auto c1 = fpu.fsub(c.f64(frame.at(slot::convert_b)), bias);
        const auto b2 = fpu.fdiv(b1, constant(pool::strength_a));
        const auto d1 = fpu.fsub(c.f64(frame.at(slot::truncated_a)), bias);
        const auto c2 = fpu.fdiv(c1, constant(pool::strength_b));
        f30 = fpu.fadd(f30, b2);
        const auto d2 = fpu.fdiv(d1, constant(pool::drift));
        f30 = fpu.fadd(f30, c2);
        f30 = fpu.fadd(f30, d2);
        const auto base = fpu.fadd(constant(pool::base), f30);
        auto scaled = fpu.multiply_add(0x1019eef4, constant(pool::score_scale), base, constant(pool::score_offset));
        scaled = fpu.fmul(f29, scaled);
        c.put64(frame.at(slot::convert_a), fpu.fctiwz(0, scaled));
        r22 = r22 + c.u32(frame.at(slot::convert_a + 4));
        // Tally 2: the other side's slots, weighing each person by the club's region view of them.
        for (;;) {
            r26 = r18 + r27 + r28;
            const auto person_index = c.i32(r26 + match::person);
            compare(c, person_index, -1);
            if (person_index != -1) {
                const auto people = global(toc::people);
                const auto opponent_region = c.u32(opponent + club::region), region = c.u32(club + club::region);
                c.set(4, region);
                r20 = people + word(person_index) * person::size;
                compare_logical(c, opponent_region, region);
                std::int32_t value;
                if (opponent_region == region) {
                    value = signed_half(c.u32(r20 + person::career) + 0xa);
                } else {
                    const auto person_region = c.u32(r20 + person::home_region);
                    compare_logical(c, person_region, region);
                    if (person_region == region) {
                        value = signed_half(c.u32(r20 + person::career) + 8);
                    } else {
                        c.set({{3, r20}, {4, region}, {5, 1}});
                        c.call(callee::native_flag, 0x1019ef74);
                        const auto native = sbyte(c.r(3));
                        compare(c, native);
                        value = signed_half(c.u32(r20 + person::career) + (native != 0 ? 0xc : 8));
                    }
                }
                const auto mark = slot_mark(r26, r23, nullptr);
                if (sbyte(word(mark)) != 0) {
                    r24 = r24 + word(sbyte(word(mark)));
                    r25 = r25 + word(sbyte(word(mark))) * word(value);
                }
            }
            r23 = r23 + 1;
            r28 = r28 + match::slot_size;
            compare(c, std::int32_t(r23), 0x14);
            if (std::int32_t(r23) >= 0x14) break;
        }
        compare(c, std::int32_t(r24));
        if (r24 != 0) c.put32(frame.at(slot::average), word(divide(std::int32_t(r25), std::int32_t(r24))));
        // With no marks the average is whatever the frame held (the original reads it back).
        const auto average = c.u32(frame.at(slot::average));
        const auto together = word(signed_half(opponent + club::reputation)) + average;
        const auto half = std::int32_t(together) / 2;
        const auto difference = std::int32_t(word(signed_half(club + club::reputation)) - word(half));
        const auto step_value = hundred_bound(difference / -40);
        r22 = r22 + word(step_value);
        c.set(6, word(step_value));
    }

    // 0x1019f13c..0x1019f5bc: the goals each side scored (the aggregate's where the fixture has
    // one) and what the result is worth.
    void result_outcome() {
        const auto total = signed_byte(fixture + fixture::total_home);
        const bool home_side = home_byte_set();
        if (total > -1) {
            if (home_side) {
                r20 = word(signed_byte(fixture + fixture::total_away));
                r18 = word(total);
                r23 = word(signed_byte(fixture + fixture::penalties_home));
                r24 = word(signed_byte(fixture + fixture::penalties_away));
            } else {
                r18 = word(signed_byte(fixture + fixture::total_away));
                r20 = word(total);
                r23 = word(signed_byte(fixture + fixture::penalties_away));
                r24 = word(signed_byte(fixture + fixture::penalties_home));
            }
        } else if (home_side) {
            r18 = word(signed_byte(fixture + fixture::goals_home));
            r20 = word(signed_byte(fixture + fixture::goals_away));
            r23 = word(signed_byte(fixture + fixture::penalties_home));
            r24 = word(signed_byte(fixture + fixture::penalties_away));
        } else {
            r18 = word(signed_byte(fixture + fixture::goals_away));
            r20 = word(signed_byte(fixture + fixture::goals_home));
            r23 = word(signed_byte(fixture + fixture::penalties_away));
            r24 = word(signed_byte(fixture + fixture::penalties_home));
        }
        const auto goals_for = std::int32_t(r18), goals_against = std::int32_t(r20);
        compare(c, goals_for, goals_against);
        const auto last_step = shalf(c.r(6));  // the squads' / gap step the branches scale
        if (goals_for > goals_against) {
            add_outcome(outcome::won);
            const auto weight = last_step / 10 + 20;
            r22 = r22 + word(shalf(word(weight) * word(goals_for - goals_against)));
        } else if (goals_for < goals_against) {
            lost(last_step);
        } else {
            drawn();
        }
    }
    void lost(std::int32_t last_step) {
        const auto competition = c.u32(fixture + fixture::competition);
        c.set(4, competition);
        add_outcome(outcome::lost);
        const auto weight = 20 - last_step / 10;
        r22 = r22 + word(shalf(word(weight) * word(std::int32_t(r18) - std::int32_t(r20))));
        compare_logical(c, competition);
        if (competition != 0) {
            c.set({{3, global(toc::cups)}, {4, competition}});
            c.call(callee::cup_flag, 0x1019f278);
            const auto cup = sbyte(c.r(3));
            compare(c, cup);
            if (cup == 0) {
                const auto behind = std::int32_t(r20) - std::int32_t(r18);
                compare(c, behind, 10);
                if (behind > 10) {
                    r22 = word(shalf(r22 * 5));
                    compare(c, std::int32_t(r22), 0x4e2);
                    if (std::int32_t(r22) < 0x4e2) r22 = 0x4e2;
                } else {
                    compare(c, behind, 5);
                    if (behind > 5) r22 = word(shalf(word(shalf(r22)) << 1));
                }
            }
        }
        const auto bound = shalf(r22);
        compare(c, bound, -0x32);
        if (bound >= -0x32) return;
        compare_logical(c, club);
        if (club == 0) return;
        const auto manager = c.u32(club + club::manager);
        compare_logical(c, manager);
        if (manager == 0) return;
        const auto staff_record = c.u32(manager + manager::staff);
        compare_logical(c, staff_record);
        if (staff_record == 0) return;
        c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::club_state, 0x1019f2fc);
        const auto settled = sbyte(c.r(3));
        compare(c, settled);
        if (settled != 0) return;
        draw_table::step(c, table, 0x1019f32c);
        // The manager's edge (a signed byte) against a draw modulo six, plus six.
        const auto number = std::int32_t(table_value(c, table));
        const auto edge = signed_byte(c.u32(club + club::manager) + manager::edge);
        c.set(7, word(edge));
        const auto against = sbyte(word(number % 6 + 6));
        compare(c, edge, against);
        if (edge >= against) return;
        const auto rows = global(toc::draw_rows) + (c.u32(club + club::id) << draw_row::shift);
        const auto marked = sbyte(c.u8(rows + draw_row::mark));
        compare(c, marked);
        if (marked >= 0) return;
        const auto trait = signed_byte(c.u32(c.u32(club + club::manager) + manager::staff) + staff::trait);
        r14 = word(trait + 1);
        compare(c, std::int32_t(r14));
        std::int32_t drawn_value = 0;
        if (std::int32_t(r14) != 0) {
            draw_table::step(c, table, 0x1019f3dc);
            const auto bound_value = std::int32_t(r14);
            compare(c, bound_value);
            drawn_value = draw_remainder(std::int32_t(table_value(c, table)), bound_value);
        }
        r25 = word(drawn_value);
        compare(c, drawn_value);
        if (drawn_value != 0) return;
        c.set({{3, object}, {4, club}, {5, 1}});
        c.call(callee::ask, 0x1019f51c);
    }
    void drawn() {
        add_outcome(outcome::drawn);
        const auto kind = c.u8(fixture + fixture::kind);
        compare(c, std::int32_t(kind), 8);
        if (kind != fixture::league_kind) return;
        const auto number = signed_half(fixture + fixture::number);
        compare(c, number, 0x427);
        if (number < 0x427) {
            compare(c, number, 0x41f);
            if (number >= 0x41f && compare(c, sbyte(r14)) != 0) r22 = r22 + 0xf;
        }
        // Rounds 0x7b-0x7c, 0x85-0x86 and 0x99-0x9a count (the compares between them end in
        // the record form below, so only the result matters).
        const auto round = signed_half(fixture + fixture::round);
        const bool counts = round >= 0x87 ? (round >= 0x99 && round < 0x9b) : round >= 0x7d ? round >= 0x85 : round >= 0x7b;
        if (!counts) return;
        if (compare(c, sbyte(r14)) != 0) r22 = r22 + word(shalf(r18 * 10));
        else r22 = r22 - word(shalf(r20 * 5));
    }

    // 0x1019f5bc..0x1019f710: a shoot-out or a second leg that decides the tie moves r22 and the
    // flags, by the sign of the side.
    void shootout() {
        const auto home_penalties = signed_byte(fixture + fixture::penalties_home);
        compare(c, home_penalties, -1);
        if (home_penalties == -1) return;
        const auto away_penalties = signed_byte(fixture + fixture::penalties_away);
        compare(c, home_penalties, away_penalties);
        if (home_penalties != away_penalties) {
            const bool home_wins = home_penalties > away_penalties;
            tie(home_wins, 0x14);
            return;
        }
        const auto home_leg = signed_byte(fixture + fixture::leg_home), away_leg = signed_byte(fixture + fixture::leg_away);
        compare(c, home_leg, away_leg);
        if (home_leg != away_leg) {
            tie(home_leg > away_leg, 0xa);
            return;
        }
        if (has(fixture_flags(), fixture::flag_replay)) return;
        const auto margin = home_penalties - signed_byte(fixture + fixture::goals_home);
        const auto away_goals = signed_byte(fixture + fixture::goals_away);
        compare(c, margin, away_goals);
        tie(margin <= away_goals, 0xf);
    }
    // A decided tie: for the home side a win adds `amount` to the score and flags a won tie; the
    // away side takes the opposite signs. `home_wins` is whether the fixture's first club won it.
    void tie(bool home_wins, std::uint32_t amount) {
        const bool for_club = home_byte_set() == home_wins;
        if (for_club) {
            add_outcome(outcome::tie_won);
            r22 = r22 + amount;
        } else {
            add_outcome(outcome::tie_lost);
            r22 = r22 - amount;
        }
    }

    // 0x1019f710..0x1019f834: the opponent as a rival (flags near_rival / rival).
    bool is_rival() {
        for (const auto offset : {club::rival_a, club::rival_b, club::rival_c}) {
            const auto listed = c.u32(club + offset);
            compare_logical(c, listed, opponent);
            if (listed == opponent) return true;
        }
        return false;
    }
    void rivalry() {
        const auto mine = c.u32(club + club::group);
        compare_logical(c, mine);
        bool same_group = false;
        if (mine != 0) {
            const auto theirs = c.u32(opponent + club::group);
            compare_logical(c, theirs);
            if (theirs != 0) {
                const auto a = c.u32(mine + venue::leader), b = c.u32(theirs + venue::leader);
                compare_logical(c, a, b);
                same_group = a == b;
            }
        }
        if (same_group) {
            add_outcome(is_rival() ? outcome::near_rival : outcome::rival);
            return;
        }
        if (is_rival()) {
            compare_logical(c, mine);
            add_outcome(outcome::rival);
            if (mine == 0) return;
            const auto here = c.u32(mine + venue::leader);
            compare_logical(c, here);
            if (here == 0) return;
            const auto other_group = c.u32(opponent + club::group);
            compare_logical(c, other_group);
            if (other_group == 0) return;
            const auto there = c.u32(other_group + venue::leader);
            compare_logical(c, there);
            if (there == 0) return;
            // The distance between the two groups' coordinates (an import: f1 in, f1 out).
            const auto dx = fpu.fsub(c.f64(here + leader::x), c.f64(there + leader::x));
            const auto dy = fpu.fsub(c.f64(here + leader::y), c.f64(there + leader::y));
            const auto squared = fpu.fmul(dx, dx);
            c.set_f(1, fpu.multiply_add(0x1019f7e8, dy, dy, squared));
            c.call(callee::distance_glue, 0x1019f7f0);
            c.set(2, c.u32(frame.at(slot::toc_save)));
            const auto distance = fpu.fmul(constant(pool::distance_scale), c.f(1));
            fpu.fcmp(0, distance, constant(pool::distance_limit));
            const auto field = c.cr(0);  // cror eq,lt,eq
            const bool close = (field & 8) != 0 || (field & 2) != 0;
            c.set_cr(0, std::uint8_t((field & ~2) | (close ? 2 : 0)));
            if (close) add_outcome(outcome::near_rival);
            return;
        }
        c.set({{3, club}, {4, opponent}});
        c.call(callee::rivals, 0x1019f824);
        const auto known = sbyte(c.r(3));
        compare(c, known);
        if (known != 0) add_outcome(outcome::rival);
    }

    // 0x1019f834..0x1019f8c8: the outcome flags weigh the score and the pool factor.
    void outcome_weights() {
        r14 = word(shalf(r21));
        if (has(r14, outcome::near_rival)) {
            f31 = fpu.fmul(f31, constant(pool::near_rival));
            if (has(r14, outcome::won)) r22 = r22 + 0xc;
            else if (has(r14, outcome::lost)) r22 = r22 - 0xc;
        } else if (has(r14, outcome::rival)) {
            f31 = fpu.fmul(f31, constant(pool::tier_c));
            if (has(r14, outcome::won)) r22 = r22 + 5;
            else if (has(r14, outcome::lost)) r22 = r22 - 5;
        }
        c.set(3, club);
        c.call(callee::club_test, 0x1019f89c);
        const auto first = sbyte(c.r(3));
        compare(c, first);
        if (first != 0) {
            f31 = fpu.fmul(f31, constant(pool::tier_b));
        } else {
            c.set(3, club);
            c.call(callee::club_flag, 0x1019f8b8);
            const auto second = sbyte(c.r(3));
            compare(c, second);
            if (second != 0) f31 = fpu.fmul(f31, constant(pool::strong));
        }
        // A league fixture of a closed club row halves the factor again and moves a falling score.
        const auto kind = c.u8(fixture + fixture::kind);
        compare(c, std::int32_t(kind), 8);
        if (kind != fixture::league_kind) return;
        const auto flags = signed_half(row_address() + row::flags);
        if (has(word(flags), row::flag_closed)) {
            const auto factor = constant(pool::strong);
            compare(c, shalf(r22));
            f31 = fpu.fmul(f31, factor);
            if (shalf(r22) < 0) r22 = scaled_score(constant(pool::tier_c));
        } else {
            f31 = fpu.fmul(f31, constant(pool::tier_c));
        }
    }

    // 0x1019f940..0x1019f9f0: the score is scaled by the factor and cut by outcome.
    void scale_and_halve() {
        const auto test = has(r14, outcome::won);
        const auto converted = fpu.fsub(load_converted(slot::convert_a, score() ^ 0x80000000u), constant(pool::bias));
        r22 = truncate_to(slot::truncated_a, fpu.fmul(converted, f31));
        if (test) {
            compare(c, shalf(r22));
            if (shalf(r22) < 0) r22 = word(shalf(word(divide(shalf(r22), 3))));
        }
        if (has(r14, outcome::lost)) {
            compare(c, shalf(r22));
            if (shalf(r22) > 0) r22 = word(shalf(word(divide(shalf(r22), 3))));
        }
        if (has(r14, outcome::tie_won)) {
            compare(c, shalf(r22));
            if (shalf(r22) < 0) r22 = word(shalf(word(divide(shalf(r22), 2))));
        }
        if (has(r14, outcome::tie_lost)) {
            compare(c, shalf(r22));
            if (shalf(r22) > 0) r22 = word(shalf(word(divide(shalf(r22), 2))));
        }
    }

    // The club's row value as the halfword (0 without a club), left in r0 of the original.
    std::int32_t row_value() {
        compare_logical(c, club);
        return club != 0 ? signed_half(row_address() + row::value) : 0;
    }
    // 0x1019f9f0..0x1019fb70: the margin and the score pick the margin flags.
    void margin_flags() {
        const auto goals_for = std::int32_t(r18), goals_against = std::int32_t(r20);
        compare(c, std::int32_t(word(goals_for) + 5), goals_against);
        if (std::int32_t(word(goals_for) + 5) < goals_against) {
            const auto value = row_value();
            compare(c, value, 0x10b3);
            add_outcome(value < 0x10b3 ? outcome::margin_a : outcome::margin_b);
            return;
        }
        compare(c, std::int32_t(word(goals_for) - 5), goals_against);
        if (std::int32_t(word(goals_for) - 5) > goals_against) {
            add_outcome(outcome::margin_d);
            return;
        }
        auto bound = shalf(r22);
        compare(c, bound, -0x78);
        if (bound < -0x78) {
            const auto value = row_value();
            compare(c, value, 0x10b3);
            if (value < 0x10b3) return add_outcome(outcome::margin_a);
        }
        bound = shalf(r22);
        compare(c, bound, -0x4b);
        if (bound < -0x4b) {
            const auto value = row_value();
            compare(c, value, 0x1964);
            if (value < 0x1964) return add_outcome(outcome::margin_b);
        }
        bound = shalf(r22);
        compare(c, bound, -0x1e);
        if (bound < -0x1e) return add_outcome(outcome::margin_c);
        compare(c, bound, 0x78);
        if (bound > 0x78) {
            const auto value = row_value();
            compare(c, value, 0x1964);
            if (value > 0x1964) return add_outcome(outcome::margin_d);
        }
        bound = shalf(r22);
        compare(c, bound, 0x4b);
        if (bound > 0x4b) return add_outcome(outcome::margin_f);
        compare(c, bound, 0x1e);
        if (bound > 0x1e) add_outcome(outcome::margin_e);
    }

    // The long call's nine to sixteen arguments sit in the frame from 0x38 (zero here, the ninth
    // overwritten before the second call).
    // 0x1019fb70..0x1019fce0: a closed or friendly fixture raises nothing; any other asks the
    // event layer (0x101b9b50) and the news item the outcome calls for (0x101b9130).
    void first_event() {
        const auto flags = fixture_flags();
        if (has(flags, fixture::flag_closed) || has(flags, fixture::flag_hidden) || has(flags, fixture::flag_friendly)) return;
        for (std::uint32_t at = 0x38; at <= 0x54; at += 4) c.put32(frame.at(at), 0);
        c.set({{3, object}, {4, club}, {5, opponent}, {6, c.u32(fixture + fixture::competition)}, {7, word(shalf(r21))},
               {8, c.toc(toc::strings) + 0x10000 + 0x3362}, {9, word(-1)}, {10, word(-1)}});
        c.call(callee::event_kind, 0x1019fbdc);
        const auto raised = sbyte(c.r(3));
        compare(c, raised);
        if (raised == 0) return;
        c.set({{3, global(toc::events)}, {4, club}});
        c.call(callee::event_check, 0x1019fbf4);
        const auto allowed = sbyte(c.r(3));
        compare(c, allowed);
        if (allowed == 0) return;
        const auto value = signed_half(row_address() + row::value);
        compare(c, value, 0x1a5e);
        if (value > 0x1a5e) {
            if (has(shalf(r21), 1)) r21 = word(shalf(r21 ^ 5));
            else if (has(r21, 2)) r21 = word(shalf(r21 ^ 6));
        } else {
            compare(c, value, 0x1482);
            if (value > 0x1482) {
                if (has(r21, 1)) r21 = word(shalf(r21 ^ 3));
            } else if (value < 0x1482) {
                if (has(r21, 8)) r21 = word(shalf(r21 ^ 0x28));
            }
        }
        if (has(r21, 7)) {
            const auto rep = shalf(word(signed_half(opponent + club::reputation) + std::int32_t(c.u32(frame.at(slot::away_flag)) * 0x4e2) + 0x1f4));
            const auto mine = signed_half(club + club::reputation);
            compare(c, mine, rep);
            if (mine < rep) {
                const auto a = signed_byte(fixture + fixture::goals_home), b = signed_byte(fixture + fixture::goals_away);
                compare(c, a, b);
                if (a == b) return;
            }
        }
        c.put32(frame.at(slot::arguments), r24);
        c.set({{3, object}, {4, club}, {5, opponent}, {6, fixture}, {7, r21}, {8, r18}, {9, r20}, {10, r23}});
        c.call(callee::event_post, 0x1019fce0);
    }

    // The notice with a code (0x3d or 0x5c): the board's source, the club, the fixture.
    void notice(std::uint32_t code, std::uint32_t back) {
        c.set({{3, global(toc::notices)}, {4, club}, {5, code}, {6, 0}, {7, fixture}, {8, 0}, {9, word(-1)}});
        c.call(callee::notice, back);
    }
    // 0x1019fce0..0x1019feec: a bad run (0x100d3c40) after a lost game, by the score and the draw
    // table, spoils the board's trust.
    void second_event() {
        r14 = word(shalf(r21));
        if (has(r14, outcome::won) || has(r14, outcome::tie_won)) return;
        const auto flags = fixture_flags();
        if (has(flags, fixture::flag_closed) || has(flags, fixture::flag_hidden) || has(flags, fixture::flag_friendly)) return;
        c.set(3, club);
        c.call(callee::bad_run, 0x1019fd18);
        const auto bad = sbyte(c.r(3));
        compare(c, bad);
        if (bad == 0) return;
        draw_table::step(c, table, 0x1019fd48);
        r19 = score();
        const auto first = std::int32_t(table_value(c, table));
        const auto first_limit = -0x5a - first % 90;
        compare(c, std::int32_t(r19), first_limit);
        bool spoiled = false;
        if (std::int32_t(r19) < first_limit) {
            draw_table::step(c, table, 0x1019fdb8);
            const auto number = std::int32_t(table_value(c, table));
            const auto limit = 0xfa0 - number % 2000;
            c.set(7, c.u32(object));
            const auto value = std::int32_t(word(signed_half(row_address() + row::figure_b)) + r19 * 10);
            compare(c, value, limit);
            spoiled = value < limit;
        }
        if (!spoiled) {
            draw_table::step(c, table, 0x1019fe40);
            const auto number = std::int32_t(table_value(c, table));
            const auto limit = -0x1e - number % 120;
            compare(c, std::int32_t(r19), limit);
            if (std::int32_t(r19) >= limit) return;
            const auto value = std::int32_t(word(signed_half(row_address() + row::figure_b)) + r19 * 10);
            compare(c, value, 0x898);
            if (value >= 0x898) return;
        }
        notice(0x3d, 0x1019fed0);
        const auto row_at = row_address();
        c.put16(row_at + row::trust, word(signed_half(row_at + row::trust) - 0xfa));
    }

    // 0x1019feec..0x1019ffe0: a league fixture of the club's tie that the results record's
    // run calls out raises a notice (0x5c).
    void third_event() {
        const auto id = c.i32(fixture + fixture::id);
        compare(c, id, -1);
        if (id <= -1) return;
        const auto link = c.i32(club + club::tie_link);
        compare(c, id, link);
        if (id != link) return;
        const auto holder = std::int32_t(global(toc::cup_holder));
        compare(c, id, holder);
        if (id == holder) return;
        const auto kind = c.u8(fixture + fixture::kind);
        compare(c, std::int32_t(kind), 8);
        if (kind != fixture::league_kind) return;
        const auto tens = signed_half(fixture + fixture::round) / 10 * 10;
        compare(c, tens, 0x96);
        if (tens == 0x96) return;
        const auto entry = c.u32(global(toc::leagues) + (word(link) << 2));
        compare_logical(c, entry);
        if (entry == 0) return;
        const auto owner = c.u32(entry + league_entry::owner);
        compare_logical(c, owner);
        if (owner == 0) return;
        c.set({{3, owner}, {4, club}});
        c.call(callee::results, 0x1019ff74);
        if (has(r14, outcome::tie_lost)) return;
        const auto record = c.r(3);
        const auto streak = c.u8(record + results::streak);
        compare_logical(c, streak, 9);
        if (streak >= 9) return;
        compare_logical(c, streak);
        if (streak == 0) return;
        const auto live = c.u8(record + results::live);
        compare_logical(c, live, 1);
        if (live != 1) return;
        const auto squares = std::int32_t(std::uint32_t(streak) * streak);
        const auto limit = squares / 2 + 0x32;
        compare(c, shalf(r22), limit);
        if (shalf(r22) <= limit) return;
        notice(0x5c, 0x1019ffe0);
    }

    // 0x1019ffe0..0x101a003c: a falling score is cut once more for a manager the board
    // protects (0x102de3c0).
    void board_mood() {
        compare(c, shalf(r22));
        if (shalf(r22) >= 0) return;
        const auto manager = c.u32(club + club::manager);
        c.set(4, manager);
        compare_logical(c, manager);
        if (manager == 0) return;
        c.set({{3, global(toc::scale_owner)}, {4, manager}});
        c.call(callee::manager_flag, 0x101a0000);
        const auto protected_manager = sbyte(c.r(3));
        compare(c, protected_manager);
        if (protected_manager != 0) r22 = scaled_score(constant(pool::tier_b));
    }

    // 0x101a003c..0x101a014c: the score, scaled four ways, moves the row's four figures. The
    // four conversions and truncations interleave as the original's do (FPSCR and the stack
    // words follow that order).
    void apply_to_row() {
        const auto flipped = score() ^ 0x80000000u;
        c.set(5, conversion_high);
        const auto bias = constant(pool::bias);
        c.put32(frame.at(slot::convert_a + 4), flipped);
        const auto row_a = row_address();
        c.put32(frame.at(slot::convert_a), conversion_high);
        auto f0 = c.f64(frame.at(slot::convert_a));
        c.set(6, row_a);
        f0 = fpu.fsub(f0, bias);
        c.put32(frame.at(slot::convert_b + 4), flipped);
        const auto value = signed_half(row_a + row::value);
        c.put32(frame.at(slot::convert_b), conversion_high);
        const auto scaled_value = fpu.fmul(constant(pool::value_gain), f0);
        f0 = c.f64(frame.at(slot::convert_b));
        c.put32(frame.at(slot::convert_c + 4), flipped);
        f0 = fpu.fsub(f0, bias);
        const auto truncated_value = fpu.fctiwz(4, scaled_value);
        c.put32(frame.at(slot::convert_c), conversion_high);
        const auto scaled_a = fpu.fmul(constant(pool::figure_a_gain), f0);
        c.set_f(1, constant(pool::figure_c_gain));
        f0 = c.f64(frame.at(slot::convert_c));
        c.put64(frame.at(slot::truncated_a), truncated_value);
        f0 = fpu.fsub(f0, bias);
        const auto delta_value = c.u32(frame.at(slot::truncated_a + 4));
        const auto truncated_a = fpu.fctiwz(3, scaled_a);
        c.put32(frame.at(slot::convert_d + 4), flipped);
        const auto scaled_b = fpu.fmul(constant(pool::figure_b_gain), f0);
        c.put16(row_a + row::value, word(value) + delta_value);
        c.put64(frame.at(slot::truncated_b), truncated_a);
        const auto truncated_b = fpu.fctiwz(2, scaled_b);
        const auto row_b = row_address();
        const auto delta_a = c.u32(frame.at(slot::truncated_b + 4));
        c.put32(frame.at(slot::convert_d), conversion_high);
        const auto figure_a = signed_half(row_b + row::figure_a);
        f0 = c.f64(frame.at(slot::convert_d));
        c.put64(frame.at(slot::truncated_c), truncated_b);
        f0 = fpu.fsub(f0, bias);
        c.put16(row_b + row::figure_a, word(figure_a) + delta_a);
        const auto delta_b = c.u32(frame.at(slot::truncated_c + 4));
        const auto row_c = row_address();
        const auto scaled_c = fpu.fmul(c.f(1), f0);
        const auto figure_b = signed_half(row_c + row::figure_b);
        c.put64(frame.at(slot::spill), fpu.fctiwz(0, scaled_c));
        c.put16(row_c + row::figure_b, word(figure_b) + delta_b);
        const auto delta_c = c.u32(frame.at(slot::spill + 4));
        const auto row_d = row_address();
        c.set(4, row_d);
        c.put16(row_d + row::figure_c, word(signed_half(row_d + row::figure_c)) + delta_c);
    }

    // 0x101a014c..0x101a022c: the four figures stay within 1..10000.
    void clamp_row() {
        for (const auto field : {row::value, row::figure_a, row::figure_b, row::figure_c}) {
            const auto at = row_address();
            const auto figure = signed_half(at + field);
            compare(c, figure, row::figure_max);
            if (figure > row::figure_max) {
                c.put16(at + field, row::figure_max);
                continue;
            }
            compare(c, figure, row::figure_min);
            if (figure < row::figure_min) c.put16(at + field, row::figure_min);
        }
    }
    // Raises a row figure to `floor` (the original's compare, `bge`, store).
    void raise_to(std::uint32_t field, std::int32_t floor) {
        const auto at = row_address();
        const auto figure = signed_half(at + field);
        compare(c, figure, floor);
        if (figure < floor) c.put16(at + field, word(floor));
    }
    // 0x101a022c..0x101a0500: a rising score with a manager lifts the row to the floors its
    // competition's level and round call for; a falling one in a certain round caps it.
    void board_effects() {
        compare(c, shalf(r22));
        const auto manager = c.u32(club + club::manager);
        const auto competition = c.u32(fixture + fixture::competition);
        bool lifted = false;
        if (shalf(r22) > 0) {
            compare_logical(c, manager);
            if (manager != 0) {
                compare_logical(c, competition);
                lifted = competition != 0;
            }
        }
        if (lifted) {
            raise_for_level(competition);
        } else {
            cap_after_loss();
        }
        // The tail: with a manager and a bad enough score and row, the board's support (0x101b8e80).
        const auto manager_now = c.u32(club + club::manager);
        compare_logical(c, manager_now);
        if (manager_now != 0) {
            const auto bound = shalf(r22);
            compare(c, bound, -0x4b);
            if (bound < -0x4b) {
                const auto value = signed_half(row_address() + row::figure_b);
                compare(c, value, 0xbb8);
                if (value < 0xbb8) {
                    c.set({{3, object}, {4, club}});
                    c.call(callee::support, 0x101a0540);
                }
            }
        }
    }
    void raise_for_level(std::uint32_t competition) {
        const auto level = signed_half(competition + competition::level);
        compare(c, level, 5);
        if (level <= 5) return;
        const auto top = level * 500;
        const auto reputation = signed_half(club + club::reputation);
        c.set(4, word(reputation));
        compare(c, top, reputation);
        const auto round = signed_half(fixture + fixture::round);
        if (top > reputation) {
            compare(c, round, 0x82);
            if (round == 0x82 || (compare(c, round, 0x78), round == 0x78)) {
                raise_to(row::value, 0x1964);
                raise_to(row::figure_b, 0x1964);
                return;
            }
            compare(c, round, 0x96);
            if (round == 0x96) {
                raise_to(row::value, 0x2134);
                raise_to(row::figure_b, 0x2134);
                return;
            }
            const auto league = c.u32(club + club::league);
            compare_logical(c, competition, league);
            if (competition == league) return;
            compare(c, reputation, 0xea6);
            if (reputation >= 0xea6) {
                const auto kind = c.u8(fixture + fixture::kind);
                compare(c, std::int32_t(kind), 8);
                if (kind != fixture::league_kind) return;
            }
            raise_to(row::value, 0x10b3);
            raise_to(row::figure_b, 0x1964);
            return;
        }
        c.set(4, word(round));
        compare(c, round, 0x82);
        if (round == 0x82 || (compare(c, round, 0x96), round == 0x96)) {
            raise_to(row::value, 0x1964);
            raise_to(row::figure_b, 0x1964);
            return;
        }
        const auto league = c.u32(club + club::league);
        compare_logical(c, competition, league);
        if (competition == league) return;
        compare(c, round, 0x78);
        if (round != 0x78) {
            const auto kind = c.u8(fixture + fixture::kind);
            compare(c, std::int32_t(kind), 8);
            if (kind != fixture::league_kind) return;
        }
        raise_to(row::value, 0x10b3);
        raise_to(row::figure_b, 0x10b3);
    }
    void cap_after_loss() {
        compare(c, shalf(r22));
        if (shalf(r22) >= 0) return;
        const auto competition = c.u32(fixture + fixture::competition);
        compare_logical(c, competition);
        if (competition == 0) return;
        const auto round = signed_half(fixture + fixture::round);
        compare(c, round, 0x96);
        if (round != 0x96) return;
        const auto kind = c.u8(fixture + fixture::kind);
        compare(c, std::int32_t(kind), fixture::cup_kind);
        if (kind != fixture::cup_kind) return;
        const auto at = row_address();
        if (!has(word(signed_half(at + row::flags)), row::flag_cup_run)) return;
        const auto value = signed_half(at + row::value);
        compare(c, value, 0x10b3);
        if (value > 0x10b3) {
            c.put16(at + row::value, 0x10b3);
        } else {
            const auto second = signed_half(at + row::figure_b);
            compare(c, second, 0xdac);
            // The original stores the cap in the value (+6), not the figure it tested (+8).
            if (second > 0xdac) c.put16(at + row::value, 0xdac);
        }
        const auto again = row_address();
        const auto figure = signed_half(again + row::figure_b);
        compare(c, figure, 0x10b3);
        if (figure > 0x10b3) {
            c.put16(again + row::figure_b, 0x10b3);
        } else {
            compare(c, figure, 0xdac);
            if (figure > 0xdac) c.put16(again + row::figure_b, 0xdac);
        }
    }

    // 0x101a0540..0x101a05a0: the board's review, or the pending list's clean-up.
    void closing() {
        const auto competition = c.u32(fixture + fixture::competition);
        c.set(3, competition);
        const auto league = c.u32(club + club::league);
        compare_logical(c, competition, league);
        bool review_it = competition == league;
        if (!review_it) {
            compare(c, shalf(r22));
            review_it = shalf(r22) < 0;
        }
        if (review_it) {
            const auto round = signed_half(fixture + fixture::round);
            compare(c, round, 0xa0);
            if (round == 0xa0) return;
            compare(c, std::int32_t(r18), std::int32_t(r20));
            const bool trailing = std::int32_t(r18) < std::int32_t(r20);
            c.set({{3, object}, {4, club}, {5, trailing ? 1u : 0u}});
            c.link(trailing ? 0x101a057c : 0x101a0590);
            review(c, object, club, trailing ? 1u : 0u);
        } else {
            c.set({{3, object}, {4, club}});
            c.link(0x101a05a0);
            drop_pending(c, object, club);
        }
    }

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    std::uint32_t object, club, fixture;
    draw_table::Table table;
    std::uint32_t pool;
    Kept<14> r14;
    Kept<18> r18;
    Kept<19> r19;
    Kept<20> r20;
    Kept<21> r21;
    Kept<22> r22;
    Kept<23> r23;
    Kept<24> r24;
    Kept<25> r25;
    Kept<26> r26;
    Kept<27> r27;
    Kept<28> r28;
    KeptFloat<29> f29;
    KeptFloat<30> f30;
    KeptFloat<31> f31;
    std::uint32_t opponent = 0;
};
}  // namespace

void update(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t fixture, std::uint32_t match,
            std::uint32_t home) {
    c.set(0, c.lr());
    Frame frame(c, 0x1019e990, 0x110, {.first_gpr = 13, .gpr_offset = -0x6c, .first_fpr = 29});
    Update(c, frame, object, club, fixture, match, home).run();
    lmw_leaves(c, frame.caller());
}

// 0x1019e8f0. Frame 0x50 (r29-r31 saved one by one, LR at +8).
void after_match(Cpu& c, std::uint32_t object, std::uint32_t fixture, std::uint32_t match) {
    c.set(0, c.lr());
    Frame frame(c, 0x1019e8f0, 0x50, {.first_gpr = 29});
    guest::keep(c, {{31, match}, {30, fixture}, {29, object}});
    const auto id = c.i32(fixture + fixture::id);
    compare(c, id, -2);
    if (id == -2) return;
    const auto played = std::int32_t(c.i8(fixture + fixture::played));
    compare(c, played);
    if (played == 0) return;
    const auto first = c.u32(fixture + fixture::home);
    c.set(4, first);
    const auto first_manager = c.u32(first + club::manager);
    compare_logical(c, first_manager);
    if (first_manager != 0) {
        c.set({{5, fixture}, {6, match}, {7, 1}});
        c.link(0x1019e94c);
        update(c, c.r(3), first, fixture, match, 1);
    }
    const auto second = c.u32(fixture + fixture::away);
    c.set(4, second);
    const auto second_manager = c.u32(second + club::manager);
    compare_logical(c, second_manager);
    if (second_manager == 0) return;
    c.set({{3, object}, {5, fixture}, {6, match}, {7, 0}});
    c.link(0x1019e970);
    update(c, object, second, fixture, match, 0);
}

// 0x101c15f0. Frame 0x50 (r29-r31 saved one by one, LR at +8). The pending list is walked with
// the list's own cursor; the club's entry is removed and the walk ends.
void drop_pending(Cpu& c, std::uint32_t object, std::uint32_t club) {
    c.set(0, c.lr());
    Frame frame(c, 0x101c15f0, 0x50, {.first_gpr = 29});
    guest::keep(c, {{30, club}, {29, object}});
    auto entries = c.u32(object + object::pending);
    c.set(3, entries);
    const auto count = c.i16(entries);
    compare(c, count);  // extsh. r0,r0
    if (count == 0) return;
    c.put32(entries + list::cursor, c.u32(entries + list::head));
    Kept<31> index(c, 0);
    for (;; index = std::uint32_t(std::int32_t(index) + 1)) {
        entries = c.u32(object + object::pending);
        c.set(3, entries);
        const auto limit = c.i16(entries);
        compare(c, std::int32_t(index), limit);
        if (std::int32_t(index) >= limit) return;
        c.call(callee::list_next, 0x101c1634);
        const auto entry = c.r(3);
        c.set(4, entry);
        compare(c, std::int32_t(entry));  // or. r4,r3,r3
        if (!entry) continue;
        const auto wanted = std::int32_t(c.u32(club));
        c.set(3, word(wanted));
        const auto listed = std::int32_t(c.u32(entry));
        compare(c, wanted, listed);
        if (wanted != listed) continue;
        c.set(3, c.u32(object + object::pending));
        c.call(callee::list_remove, 0x101c1654);
        return;
    }
}
}  // namespace cm::aot::manager_manager

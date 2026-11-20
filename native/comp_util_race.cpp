// COMP_UTIL.CPP, block `club_queries`: 0x10464160, the hottest routine of the unit (150,000 calls in a day of
// the 26-country career). Whether two clubs of one league are contenders in the same race, from their rows in
// the league's COMP_STATS statistics: a weighted average of four figures per club, compared with thresholds
// from the league's club count and promotion places.
//
// Native doubles, the original's operations in the original's order (so FPSCR ends as the original leaves it),
// multiply-adds fused as the instructions at their sites are. What the original leaves on its frame is part of
// the contract (tools/stack_contract.py module COMP_UTIL.CPP; captures daily-transition, season-day and
// same-save-continue): four readers read its prologue image (saved r26-r31, f30, f31, LR), which `Frame`
// writes, and SHORTLIST_MANAGER.CPP 0x10369ce0 reads a word of its local slots 0x38..0x54 (left by the run
// starting at 0x104643d4). Each integer-to-double conversion therefore stores its two words where the original
// does. r4-r10 are not left as the original does: its 12 callers read r3 only.
#include "comp_util_detail.hpp"
#include <bit>
#include <cstdlib>

namespace cm::aot::comp_util {
namespace {
// The frame offsets a club's integers are converted through (the high word, then the integer at +4), and the
// addresses of the original's multiply-adds that combine the figures.
struct Slots {
    std::uint32_t games_first, games_second, figure_75, figure_76, figure_78, figure_77;
    std::uint32_t site_76, site_78, site_77;
};
constexpr Slots first_club_slots{0x50, 0x48, 0x38, 0x40, 0x50, 0x48, 0x10464358, 0x10464374, 0x10464380};
constexpr Slots second_club_slots{0x40, 0x38, 0x50, 0x48, 0x40, 0x38, 0x10464434, 0x10464448, 0x10464450};

class Race {
public:
    Race(Cpu& cpu, const Frame& frame_, Fpu& fpu) : constants(cpu.toc(toc::constants)), c(cpu), frame(frame_), fp(fpu) {}
    const std::uint32_t constants;

    double constant(std::uint32_t offset) const { return c.f64(constants + offset); }
    // An integer the way the original converts it: the pair stored on the frame, read back as a double (the
    // caller subtracts the bias).
    double converted(std::uint32_t slot, std::uint32_t low) {
        c.put32(frame.at(std::int32_t(slot)), conversion_high);
        c.put32(frame.at(std::int32_t(slot + 4)), low);
        return std::bit_cast<double>(std::uint64_t(conversion_high) << 32 | low);
    }
    // A signed integer's conversion: the integer with its sign bit flipped.
    double converted_signed(std::uint32_t slot, std::int32_t value) { return converted(slot, std::uint32_t(value) ^ 0x80000000u); }

    // The weighted average of a club's four figures. Fewer than 25 games played scale the weights.
    double rating(std::uint32_t row, const Slots& slot) {
        Weights w{constant(constant::weight_a), constant(constant::weight_a), constant(constant::weight_b), constant(constant::weight_b)};
        const std::uint32_t games = c.u8(row + stats_row::games);
        const auto bias = constant(constant::unsigned_bias);
        if (games < 25) {
            const auto first = converted(slot.games_first, games);
            const auto second = converted(slot.games_second, games);
            const auto elapsed = fp.fsub(first, bias);
            const auto fraction = fp.fdiv(elapsed, constant(constant::ramp_scale));
            const auto played = fp.fsub(second, bias);
            const auto ramp = fp.fsub(constant(constant::ramp_start), fraction);
            const auto share = fp.fdiv(played, constant(constant::games_scale));
            w.figure_78 = fp.fmul(w.figure_78, share);
            w.figure_77 = fp.fmul(w.figure_77, share);
            w.figure_76 = fp.fmul(w.figure_76, share);
            w.figure_75 = fp.fmul(w.figure_75, ramp);
        }
        const auto total = fp.fadd(w.figure_77, fp.fadd(w.figure_78, fp.fadd(w.figure_76, w.figure_75)));
        const auto v75 = fp.fsub(converted(slot.figure_75, c.u8(row + stats_row::figure_75)), bias);
        const auto weighted_75 = fp.fmul(v75, w.figure_75);
        const auto v76 = fp.fsub(converted(slot.figure_76, c.u8(row + stats_row::figure_76)), bias);
        auto sum = fp.multiply_add(slot.site_76, v76, w.figure_76, weighted_75);
        const auto v78 = fp.fsub(converted(slot.figure_78, c.u8(row + stats_row::figure_78)), bias);
        sum = fp.multiply_add(slot.site_78, v78, w.figure_78, sum);
        const auto v77 = fp.fsub(converted(slot.figure_77, c.u8(row + stats_row::figure_77)), bias);
        sum = fp.multiply_add(slot.site_77, v77, w.figure_77, sum);
        return fp.fdiv(sum, total);
    }

    // Whether both clubs' figure 77 (less the unsigned bias) stays within the other club's rating: the test every
    // positive answer ends with. The original converts it through frame slot 0x38.
    bool both_within(std::uint32_t first_row, std::uint32_t second_row, double first_rating, double second_rating) {
        const auto bias = constant(constant::unsigned_bias);
        const auto first = fp.fsub(converted(0x38, c.u8(first_row + stats_row::figure_77)), bias);
        fp.fcmp(0, first, second_rating);
        if (!(first <= second_rating)) return false;
        const auto second = fp.fsub(converted(0x38, c.u8(second_row + stats_row::figure_77)), bias);
        fp.fcmp(0, second, first_rating);
        return second <= first_rating;
    }
    // Both ratings at or below `threshold`.
    bool ratings_within(double first_rating, double second_rating, double threshold) {
        fp.fcmp(0, first_rating, threshold);
        if (!(first_rating <= threshold)) return false;
        fp.fcmp(0, second_rating, threshold);
        return second_rating <= threshold;
    }

    Cpu& c;
    const Frame& frame;
    Fpu& fp;
};
}

std::uint32_t contenders_class(Cpu& c, std::uint32_t first_club_row, std::uint32_t second_club_row) {
    const Frame frame(c, 0x10464160, 0x80, {.first_gpr = 26, .first_fpr = 30});
    Fpu fp(c);
    Race race(c, frame, fp);
    // The non-volatile registers at each call, as the original holds them: r26 and r27 the two clubs, r28 the
    // league object, r29 and r30 the statistics rows (the second club's, the first's), r31 the constants. The
    // callees' prologues save them into their frames.
    Held h(c);
    h.v[26] = first_club_row; h.v[27] = second_club_row; h.v[29] = 0; h.v[30] = 0; h.v[31] = race.constants;
    const auto record = c.u32(first_club_row + club::competition_record);
    if (record == 0 || record != c.u32(second_club_row + club::competition_record)) return 0;
    h.publish();
    if (std::int8_t(invoke(c, callee::marks_record, 0x104641b4, {c.u32(c.toc(toc::stats_owner)), record})) != 0) return 0;
    const auto id = c.u32(c.u32(first_club_row + club::competition_record));
    const auto object = c.u32(c.u32(c.toc(toc::competitions)) + id * 4);
    h.v[28] = object;
    std::uint32_t first_row = 0, second_row = 0;
    if (object != 0 && c.u32(object + competition::stats) != 0) {
        h.publish();
        first_row = invoke(c, callee::stats_row, 0x104641f0, {c.u32(object + competition::stats), first_club_row});
        h.v[30] = first_row;
        h.publish();
        second_row = invoke(c, callee::stats_row, 0x10464204, {c.u32(object + competition::stats), second_club_row});
        h.v[29] = second_row;
    }
    if (first_row == 0 || second_row == 0) return 0;
    if (c.i32(object + competition::last_stage) != -1 || c.u8(object + competition::kind) != league::kind) return 0;

    // The clubs' difference of figure 76 against a limit from the league's size (a halfword through fctiwz).
    const auto difference = std::abs(std::int32_t(c.u8(first_row + stats_row::figure_76)) - std::int32_t(c.u8(second_row + stats_row::figure_76)));
    const auto league_size = [&] { return std::int32_t(c.i16(object + competition::club_count)); };
    const auto limit_input = race.converted_signed(0x50, league_size());
    const auto limit = fp.fmul(race.constant(constant::limit_scale), fp.fsub(limit_input, race.constant(constant::signed_bias)));
    c.put64(frame.at(0x48), fp.fctiwz(0, limit));
    if (difference > std::int16_t(c.u32(frame.at(0x4c)))) return 0;

    const auto first_rating = race.rating(first_row, first_club_slots);
    const auto second_rating = race.rating(second_row, second_club_slots);
    // The ratings stay in f31 and f30 across the virtual calls.
    c.set_f(31, first_rating);
    c.set_f(30, second_rating);
    // The league's virtual +0xa0 (inferred: whether it has promotion places), asked up to three times.
    const auto promotion_places = [&](std::uint32_t back) {
        h.publish();
        return std::int8_t(virtual_call(c, 0xa0, back, {object}));
    };
    const auto bias = race.constant(constant::signed_bias);
    if (promotion_places(0x1046445c) == 0) {
        const auto threshold = fp.fdiv(fp.fsub(race.converted_signed(0x38, league_size()), bias), race.constant(constant::share_divisor));
        fp.fcmp(0, first_rating, threshold);
        if (first_rating < threshold) {
            fp.fcmp(0, second_rating, threshold);
            if (second_rating < threshold && race.both_within(first_row, second_row, first_rating, second_rating)) return 1;
        }
    }
    if (promotion_places(0x104644fc) != 0) {
        const auto automatic = std::int32_t(c.i8(object + league::automatic_promotion));
        const auto quotient = fp.fdiv(fp.fsub(race.converted_signed(0x40, league_size()), bias), race.constant(constant::promotion_divisor));
        const auto places = fp.fsub(race.converted_signed(0x38, automatic), bias);
        const auto threshold = fp.fadd(places, quotient);
        if (race.ratings_within(first_rating, second_rating, threshold) && race.both_within(first_row, second_row, first_rating, second_rating))
            return 3;
    }
    if (promotion_places(0x104645c4) == 0) return 0;
    const auto playoff = std::int32_t(c.i8(object + league::playoff_clubs));
    if (playoff == 0) return 0;
    const auto size = race.converted_signed(0x38, league_size());
    const auto automatic = std::int32_t(c.i8(object + league::automatic_promotion));
    const auto quotient = fp.fdiv(fp.fsub(size, bias), race.constant(constant::promotion_divisor));
    const auto places = fp.fsub(race.converted_signed(0x40, automatic), bias);
    const auto playoffs = fp.fsub(race.converted_signed(0x48, playoff), bias);
    const auto threshold = fp.fadd(quotient, fp.fadd(places, playoffs));
    if (race.ratings_within(first_rating, second_rating, threshold) && race.both_within(first_row, second_row, first_rating, second_rating))
        return 3;
    return 0;
}
}

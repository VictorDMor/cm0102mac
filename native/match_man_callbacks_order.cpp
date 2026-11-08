// MATCH_MAN.CPP callbacks block (match_man_callbacks.hpp): the squad-ordering comparators.
#include "match_man_callbacks.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "match_man_callbacks_detail.hpp"
#include "match_man_detail.hpp"
#include <array>
#include <optional>

namespace cm::aot::match_man {
using namespace guest;

namespace {
// The role bits of the player (MATCH_PL), asked at the original's call site `back`.
std::int16_t role(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    c.call(entries::player_category, back);
    return std::int16_t(c.r(3));
}

// The checks both comparators open with. Each sets cr0 as the original's compare does.
// Returns the order they settle (negative: `a` first), or nothing when the scores decide.
std::optional<std::int32_t> settled_order(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t back_a,
                                          std::uint32_t back_b) {
    const auto staff = [&](std::uint32_t player) { return c.u32(player + callback_player::staff); };
    const auto position = [&](std::uint32_t player) { return std::int32_t(c.i8(player + callback_player::position)); };
    if (compare_logical(c, a) == 0 || compare_logical(c, staff(a)) == 0) return 1;
    if (compare_logical(c, b) == 0 || compare_logical(c, staff(b)) == 0) return -1;
    if (compare(c, position(a), -2) == 0) return -1;
    if (compare(c, position(b), -2) == 0) return 1;
    // A role-1 player whose staff rating is below 20 goes first (the original reads the
    // byte through r3 = the staff record, which stays there until the next call).
    const auto below_20 = [&](std::uint32_t player, std::uint32_t back) {
        if (compare(c, role(c, player, back), 1) != 0) return false;
        c.set(3, staff(player));
        return compare(c, std::int32_t(c.i8(c.r(3) + staff_record::goalkeeping)), 0x14) < 0;
    };
    if (below_20(a, back_a)) return -1;
    if (below_20(b, back_b)) return 1;
    return std::nullopt;
}

// The int-to-double idiom: the words 0x43300000 and value ^ 0x80000000 stored at `at`,
// read back as a double. Returns it.
double converted(Cpu& c, std::uint32_t at, std::uint32_t value) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, value ^ 0x80000000);
    return c.f64(at);
}

// int(halfword + float) of the player, as the first scores are computed: the halfword
// (+0x36) converted at `convert_at` and left in r4, the float (+0x50) added; the result
// of fctiwz stored at `result_at` and its low word returned. Leaves f1 as the original.
std::uint32_t player_score(Cpu& c, Fpu& fpu, std::uint32_t pool, std::uint32_t player, std::uint32_t convert_at,
                           std::uint32_t result_at) {
    const auto rating = std::uint32_t(std::int32_t(c.i16(player + callback_player::rating)));
    c.set(4, rating);
    const double bias = c.f64(pool + constants::conversion);
    const double form = c.f32(player + callback_player::form);
    const double rating_in_double = fpu.fsubs(converted(c, convert_at, rating), bias);
    c.set_f(1, rating_in_double);
    c.put64(result_at, fpu.fctiwz(0, fpu.fadds(rating_in_double, form)));
    return c.u32(result_at + 4);
}

// What the first player's role bits select for a score: a factor (pool offset) to
// multiply by, or an unscaled conversion, or nothing. The original asks MATCH_PL once
// per test, in this order, at the call sites `back`.
struct Scaling {
    bool applies;
    std::uint32_t factor_at;  // 0: converts without multiplying
};
Scaling scaling_of(Cpu& c, std::uint32_t player, std::uint32_t factor_for_8, const std::array<std::uint32_t, 6>& back) {
    const auto is = [&](unsigned n, std::int16_t bits) { return compare(c, role(c, player, back[n]), bits) == 0; };
    if (is(0, 4) || is(1, 2)) return {true, constants::factor_4_or_2};
    if (is(2, 8)) return {true, factor_for_8};
    if (is(3, 0x10)) return {true, constants::factor_16};
    if (is(4, 0x20) || is(5, 0x40)) return {true, 0};
    return {false, 0};
}

// score = int(factor * (score as a signed word)): the stack slots 0x38/0x3c hold the
// conversion and 0x40/0x44 the fctiwz result, left in r28 or r29 (`score`).
template <unsigned R>
void rescale(Cpu& c, Fpu& fpu, const Frame& frame, std::uint32_t pool, Kept<R>& score, Scaling scaling) {
    if (!scaling.applies) return;
    const double bias = c.f64(pool + constants::conversion);
    c.set_f(1, bias);
    double value = fpu.fsubs(converted(c, frame.at(0x38), score), bias);
    if (scaling.factor_at) value = fpu.fmul(c.f64(pool + scaling.factor_at), value);
    c.put64(frame.at(0x40), fpu.fctiwz(0, value));
    score = c.u32(frame.at(0x44));
}
}

// 0x1020ea00. Frame 0x60 saving r28-r31; r31 the pool of doubles (TOC -0x5278), r30 and
// r29 the players, r28 and then r29 their scores. Answers score(a) - score(b).
// Quirk kept: the second player's score is scaled by the first player's role bits.
std::int32_t compare_by_scaled_score(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x1020ea00, 0x60, {.first_gpr = 28});
    const auto pool = c.toc(slots::constants);
    keep(c, {{31, pool}});
    const auto a = c.u32(first), b = c.u32(second);
    keep(c, {{30, a}, {29, b}});
    if (const auto order = settled_order(c, a, b, 0x1020ea98, 0x1020eac8))
        return std::int32_t(result(c, std::uint32_t(*order)));
    Fpu fpu(c);
    Kept<28> score_a(c, c.r(28));
    c.set(3, a);
    score_a = player_score(c, fpu, pool, a, frame.at(0x40), frame.at(0x38));
    rescale(c, fpu, frame, pool, score_a,
            scaling_of(c, a, constants::factor_8_first, {0x1020eb2c, 0x1020eb40, 0x1020eb88, 0x1020ebd0, 0x1020ec18, 0x1020ec2c}));
    Kept<29> score_b(c, c.r(29));
    c.set(3, a);
    score_b = player_score(c, fpu, pool, b, frame.at(0x38), frame.at(0x40));
    rescale(c, fpu, frame, pool, score_b,
            scaling_of(c, a, constants::factor_8_second, {0x1020ec9c, 0x1020ecb0, 0x1020ecf8, 0x1020ed40, 0x1020ed88, 0x1020ed9c}));
    return std::int32_t(result(c, score_a - score_b));
}

// 0x1020ee00. Frame 0x60 saving r30 and r31 (the second and first player). Answers
// score(a) - score(b) with no role scaling. The conversions and results of both players
// are interleaved as the original does: a's at sp+0x40 (result sp+0x38), b's at sp+0x50
// (result sp+0x48).
std::int32_t compare_by_score(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x1020ee00, 0x60, {.first_gpr = 30});
    const auto a = c.u32(first), b = c.u32(second);
    keep(c, {{31, a}, {30, b}});
    if (const auto order = settled_order(c, a, b, 0x1020ee8c, 0x1020eebc))
        return std::int32_t(result(c, std::uint32_t(*order)));
    Fpu fpu(c);
    const auto pool = c.toc(slots::constants);
    const auto rating_b = std::uint32_t(std::int32_t(c.i16(b + callback_player::rating)));
    const auto rating_a = std::uint32_t(std::int32_t(c.i16(a + callback_player::rating)));
    // r4 ends as b's flipped halfword, r5 as the pool.
    c.set({{4, rating_b ^ 0x80000000}, {5, pool}});
    const double bias = c.f64(pool + constants::conversion);
    const double b_input = fpu.fsubs(converted(c, frame.at(0x50), rating_b), bias);
    const double sum_b = fpu.fadds(b_input, c.f32(b + callback_player::form));
    const double a_input = fpu.fsubs(converted(c, frame.at(0x40), rating_a), bias);
    c.set_f(1, a_input);
    c.put64(frame.at(0x48), fpu.fctiwz(2, sum_b));
    const double sum_a = fpu.fadds(a_input, c.f32(a + callback_player::form));
    c.put64(frame.at(0x38), fpu.fctiwz(0, sum_a));
    const auto score_b = c.u32(frame.at(0x4c));
    const auto score_a = c.u32(frame.at(0x3c));
    return std::int32_t(result(c, score_a - score_b));
}
}

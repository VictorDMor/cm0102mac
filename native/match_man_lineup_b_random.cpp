// MATCH_MAN.CPP lineup_b block (match_man_lineup_b.hpp): the scaled random value, 0x10209d10.
#include "match_man_lineup_b.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "match_man_adapter.hpp"
#include "match_man_detail.hpp"
#include "match_man_lineup_b_detail.hpp"
#include <array>
#include <bit>

namespace cm::aot::match_man {
using namespace guest;
namespace {
constexpr std::uint32_t entry = 0x10209d10;
constexpr std::int32_t scale = 0xffff;
// The registers the original keeps across its calls: level k's bound lives in
// bound_register[k] and its scaled value in value_register[k].
constexpr std::array<unsigned, 4> bound_register{30, 22, 21, 20};
constexpr std::array<unsigned, 4> value_register{31, 24, 23, 25};
// Where each level's three stack words are: the bound converted to a double, the
// remainder converted to a double (+0x40), and the fctiwz result. Level 0 uses the
// other two slots than the rest, so the last store of a deeper level overwrites them.
constexpr std::uint32_t remainder_slot = 0x40;
constexpr std::uint32_t bound_slot(unsigned level) { return level == 0 ? 0x48 : 0x38; }
constexpr std::uint32_t result_slot(unsigned level) { return level == 0 ? 0x38 : 0x48; }
// The returns of each draw and of the recursion.
constexpr std::array<std::uint32_t, 4> draw_back{0x10209d64, 0x10209e50, 0x10209f3c, 0x1020a028};
constexpr std::uint32_t recursion_back = 0x1020a0f0;

struct Tables {
    std::uint32_t seed, table, limit, cursor, pool;
};

// Moves the cursor on by one 16-bit entry; past the table's end it goes back to the start and
// the seed is redrawn from the RNG (its low 16 bits).
void advance(Cpu& c, const Tables& t, std::uint32_t back) {
    const auto cursor = c.u32(t.cursor) + 2;
    c.set(3, cursor);
    const auto limit = c.u32(t.limit);
    c.put32(t.cursor, cursor);
    if (compare_logical(c, cursor, limit) > 0) {
        c.put32(t.cursor, t.table);
        c.call(lineup_entries::random, back);
        c.put32(t.seed, c.r(3) & 0xffff);
    }
}

// The bound's class: |bound| <= 0xffff is "small" (the original tests > 0xffff, then >= -0xffff).
bool is_small(Cpu& c, std::int32_t bound) {
    if (compare(c, bound, scale) > 0) return false;
    return compare(c, bound, -scale) >= 0;
}

// The 16-bit entry under the cursor plus the seed.
std::uint32_t draw_sum(Cpu& c, const Tables& t) { return std::uint32_t(c.u16(c.u32(t.cursor))) + c.u32(t.seed); }

// A small bound: the draw's remainder (divw), with the bound's sign (a negative bound negates it).
std::uint32_t small_value(Cpu& c, const Tables& t, std::int32_t bound) {
    compare(c, bound, 0);
    const auto sum = std::int32_t(draw_sum(c, t));
    const auto remainder = std::uint32_t(sum - (sum / bound) * bound);  // wrapping, as mullw/subf
    return bound > 0 ? remainder : 0u - remainder;
}

struct Scaled {
    std::uint32_t value;  // the scaled value of the level
    std::int32_t next;    // the next level's bound: ten times this one over 65535
};
// A large bound at `level`: value = (bound / 65535.0) * (draw % 65535) truncated, and the next
// level's bound = bound * 10 / 65535. Both divisions are mulhw-and-shift magics for a division
// by 65535 that round toward zero (verified for every 32-bit input). The original's stack words
// and registers are left as they end up; the last block's f1 is the quotient.
Scaled scale_level(Cpu& c, Fpu& fpu, const Frame& frame, const Tables& t, unsigned level, std::int32_t bound) {
    constexpr std::uint32_t exponent_word = 0x43300000;
    const auto converted = [&](std::uint32_t at, std::int32_t value) {
        const auto flipped = std::uint32_t(value) ^ 0x80000000u;
        c.put32(at + 4, flipped);
        c.put32(at, exponent_word);
        return std::bit_cast<double>(std::uint64_t(exponent_word) << 32 | flipped);
    };
    const auto bias = c.f64(c.toc(slots::constants) + random_pool::conversion);
    const auto divisor = c.f64(c.toc(slots::constants) + random_pool::divisor);
    const auto bound_double = fpu.fsub(converted(frame.at(bound_slot(level)), bound), bias);
    const auto cursor = c.u32(t.cursor);
    const auto draw = c.u16(cursor);
    const auto sum = std::int32_t(std::uint32_t(draw) + c.u32(t.seed));
    const auto quotient = fpu.fdiv(bound_double, divisor);
    const auto ten = std::int32_t(std::uint32_t(bound) * 10);
    const auto next = ten / scale;
    const auto sum_over = sum / scale;
    const auto product = std::uint32_t(sum_over) * std::uint32_t(scale);
    const auto remainder = std::uint32_t(sum) - product;
    const auto remainder_double = fpu.fsub(converted(frame.at(remainder_slot), std::int32_t(remainder)), bias);
    const auto scaled = fpu.fmul(quotient, remainder_double);
    const auto value = truncated_to_stack(c, fpu, frame.at(result_slot(level)), scaled);
    c.set_f(1, quotient);
    c.set_f(2, bias);
    if (level < 3) {
        // The sign bits of the two floored quotients, the product and the constants left behind.
        c.set({{3, cursor}, {4, ten < 0}, {5, product}, {6, sum < 0}, {7, std::uint32_t(sum)}, {8, draw},
               {9, exponent_word}});
        c.set(bound_register[level + 1], std::uint32_t(next));
        compare(c, next);  // `add.`
    } else {
        // The last level reuses r3-r6 for its temporaries; r3 is the recursion's argument.
        c.set({{3, std::uint32_t(next)}, {4, product}, {5, 0xffff}, {6, 0x80008001}, {7, std::uint32_t(sum)}, {8, draw}});
    }
    c.set(value_register[level], value);
    return {value, next};
}

// x / 10 as the original's mulhw magic computes it (rounding toward zero; verified for every
// 32-bit input).
std::int32_t tenth(std::uint32_t value) { return std::int32_t(value) / 10; }
}

std::uint32_t scaled_random(Cpu& c, std::int32_t bound) {
    Frame frame(c, entry, 0x80, {.first_gpr = 20});
    Fpu fpu(c);
    const Tables t{c.toc(-0x7b90), c.toc(-0x7b8c), c.toc(-0x7b88), c.toc(-0x7b84), c.toc(slots::constants)};
    keep(c, {{25, t.seed}, {26, t.table}, {27, t.limit}, {28, t.cursor}, {29, t.pool}});
    c.set(30, std::uint32_t(bound));
    if (compare(c, bound) == 0) return result(c, 0);

    std::array<std::int32_t, 4> bounds{bound};
    std::array<std::uint32_t, 4> values{};
    unsigned levels = 0;       // levels that scaled
    std::uint32_t child = 0;   // what the deepest level's value is refined by
    for (unsigned level = 0;; ++level) {
        advance(c, t, draw_back[level]);
        if (is_small(c, bounds[level])) {
            child = small_value(c, t, bounds[level]);
            // Nothing is called after this, so r20-r31 are restored by the epilogue unseen.
            if (level == 0) return result(c, child);
            break;
        }
        const auto scaled = scale_level(c, fpu, frame, t, level, bounds[level]);
        values[level] = scaled.value;
        levels = level + 1;
        if (level == 3) {
            // The routine calls itself with the next bound; r3-r9 are as the last level left them.
            match_man::call(c, entry, recursion_back);
            child = c.r(3);
            break;
        }
        bounds[level + 1] = scaled.next;
        if (scaled.next == 0) { child = 0; break; }  // no draw, the deeper value is 0
    }
    // Each level's value is refined by a tenth of the level below it, then kept below its bound.
    std::uint32_t below = child;
    std::int32_t last_sum = 0;
    for (unsigned level = levels; level-- > 0;) {
        const auto sum = std::int32_t(below + 5);
        last_sum = sum;
        const auto refined = values[level] + std::uint32_t(tenth(below + 5));
        const auto cap = std::int32_t(std::uint32_t(bounds[level]) - 1);  // addi: wraps at INT_MIN
        compare(c, std::int32_t(refined), cap);
        values[level] = std::int32_t(refined) > cap ? std::uint32_t(cap) : refined;
        below = values[level];
    }
    // The first level's stage leaves the sign of the floored tenth in r4.
    c.set(4, last_sum < 0);
    return result(c, values[0]);
}
}

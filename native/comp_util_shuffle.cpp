// COMP_UTIL.CPP, block `random_shuffle`: the unit's out-of-line copy of the game's pooled Random(n) (0x10463000)
// and the two shuffles built on it (0x10462cd0 and 0x104635b0). Every draw must be the original's: the same
// pool cells in the same order, the refill through the generator (0x1077b770) at the same moment, and the same
// number of draws.
//
// The pooled draw: the cursor (a cell holding the address of the sample the next draw reads) moves two bytes; past
// the pool's end it returns to the pool's start and the salt (a cell) is renewed from the generator's next value
// (its low 16 bits). A draw is then (sample halfword + salt) taken modulo n (the remainder's sign made that of n),
// and 0 without any draw for n = 0. For a bound of 0x10000 or more in magnitude the draw is scaled instead: a
// first part, n / 65535 times the remainder of the sum by 65535 (in doubles, truncated), plus a tenth (rounded)
// of a second draw below n * 10 / 65535, capped at n - 1. The shuffles only ever pass a bound that fits in a
// signed halfword, so only 0x10463000 reaches the scaled form (and its second draw is always below 0x10000, so
// the recursion is one level deep; the original inlines four levels and then recurses, which no input reaches).
//
// The registers the shuffles' callers read after the call (r7..r10) are the ones the pooled draw never writes
// (the generator's refill leaves only r0, r3 and r4, and the swap rewrites r3..r5), so those are left alone; r3..r5
// hold the last swap's values as in the original.
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
// The pool the draws read: the cells' addresses and the pool's first sample (an address held by the TOC slot).
struct Pool {
    Cpu& c;
    std::uint32_t salt, end, cursor, start;
    explicit Pool(Cpu& cpu)
        : c(cpu), salt(cpu.toc(toc::rng_salt)), end(cpu.toc(toc::rng_end)), cursor(cpu.toc(toc::rng_cursor)), start(cpu.toc(toc::rng_start)) {}
    // Moves to the next sample; past the end, back to the start with a new salt. `back` is the address after the
    // original's call of the generator (the callee saves it).
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        const auto last = c.u32(end);
        c.put32(cursor, next);
        if (next > last) {
            c.put32(cursor, start);
            c.call(callee::reseed, back);
            c.put32(salt, c.r(3) & 0xffff);
        }
    }
    // The sample under the cursor plus the salt.
    std::int32_t sum() const { return std::int32_t(std::uint32_t(c.u16(c.u32(cursor))) + c.u32(salt)); }
};
// The double the original builds on its frame from a signed integer: the high word 0x43300000 beside the integer
// with its sign bit flipped, then the bias (the constants' +8) subtracted.
double biased(std::int32_t value) {
    return std::bit_cast<double>((std::uint64_t(conversion_high) << 32) | (std::uint32_t(value) ^ 0x80000000u));
}
std::int32_t divide_by_ten(std::int32_t value) { return value / 10; }

// One draw below `bound` (0x10463000's result). `back` is the generator call's return address.
std::int32_t pooled_random(Cpu& c, std::int32_t bound, std::uint32_t back) {
    if (bound == 0) return 0;
    const Pool pool(c);
    pool.advance(back);
    const auto sum = pool.sum();
    if (bound < 0x10000 && bound > -0x10000) {
        // divw / mullw / subf: the remainder, negated for a negative bound (-1 divides everything).
        const std::int32_t rest = bound == -1 ? 0 : sum % bound;
        return bound > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
    }
    // The scaled form: the sums' quotient and remainder by 65535, and the bound's tenfold (wrapping) over 65535.
    const auto constants = c.toc(toc::constants);
    Fpu fp(c);
    c.set_f(2, c.f64(constants + constant::signed_bias));
    c.set_f(1, fp.fsub(biased(bound), c.f(2)));
    c.set_f(0, c.f64(constants + constant::word_range));
    c.set_f(1, fp.fdiv(c.f(1), c.f(0)));
    const std::int32_t quotient = sum / 0xffff;
    const std::int32_t remainder = std::int32_t(std::uint32_t(sum) - std::uint32_t(quotient) * 0xffffu);
    const std::int32_t smaller = std::int32_t(std::uint32_t(bound) * 10u) / 0xffff;
    c.set_f(0, fp.fsub(biased(remainder), c.f(2)));
    c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    const auto converted = fp.fctiwz(0, c.f(0));
    const auto scaled = std::int32_t(std::uint32_t(converted));
    // The second draw (0 without drawing when the smaller bound is 0), a tenth of it rounded, added and capped.
    const auto second = pooled_random(c, smaller, 0x10463140);
    std::int32_t result = std::int32_t(std::uint32_t(scaled) + std::uint32_t(divide_by_ten(std::int32_t(std::uint32_t(second) + 5u))));
    const auto top = std::int32_t(std::uint32_t(bound) - 1u);
    if (result > top) result = top;
    return result;
}

// The signed halfword of a register (`extsh`).
std::int32_t halfword(std::uint32_t value) { return std::int16_t(value); }

// The swap both shuffles end an iteration with: the words at the two (signed halfword) indices change places;
// r3 the second's byte offset, r4 the first's, r5 the first's old word.
void swap_words(Cpu& c, std::uint32_t array, std::int32_t first, std::int32_t second) {
    const auto first_at = std::uint32_t(first) << 2, second_at = std::uint32_t(second) << 2;  // rlwinm ...,2,0,29: the offset wraps
    const auto first_word = c.u32(array + first_at);
    const auto second_word = c.u32(array + second_at);
    c.put32(array + first_at, second_word);
    c.put32(array + second_at, first_word);
    c.set({{3, second_at}, {4, first_at}, {5, first_word}});
}
}

void shuffle(Cpu& c, std::uint32_t array, std::uint32_t count_register) {
    Frame frame(c, 0x10462cd0, 0x80, {.first_gpr = 21});
    const auto bound = halfword(count_register);
    const auto rounds = bound * 3;
    // A halfword counter: from 10923 items on the loop never ends (the original's counter wraps at 32768 too).
    for (std::int16_t round = 0; std::int32_t(round) < rounds; round = std::int16_t(round + 1)) {
        const auto first = halfword(std::uint32_t(pooled_random(c, bound, 0x10462d38)));
        const auto second = halfword(std::uint32_t(pooled_random(c, bound, 0x10462e94)));
        swap_words(c, array, first, second);
    }
}

void shuffle_front(Cpu& c, std::uint32_t array, std::uint32_t count_register, std::uint32_t take_register) {
    Frame frame(c, 0x104635b0, 0xb0, {.first_gpr = 20});
    const auto pool = c.toc(toc::pool), buffer = c.toc(toc::message_buffer);
    // The prologue's registers: the pool's strings, the array, the message buffer, the arguments, the unit's
    // constants and the generator's cells (the assertion's dialog reaches imports).
    c.set({{20, pool}, {21, array}, {22, buffer}, {23, take_register}, {25, count_register}, {27, c.toc(toc::constants)},
           {28, c.toc(toc::rng_salt)}, {29, c.toc(toc::rng_start)}, {30, c.toc(toc::rng_end)}, {31, c.toc(toc::rng_cursor)}});
    const auto count = halfword(count_register), take = halfword(take_register);
    if (count >= 0) c.set(4, std::uint32_t(count));
    if (count < 0 || count < take) {
        // The count is negative or below the number to take: the game's assertion (line 0xf1).
        guest::report_assertion(c, {0x10463614, pool, pool + 0xe, pool + 0x1b, frame.at(0x40), frame.at(0x38), 0xf1});
        c.set(3, c.toc(toc::success_flag));
        return;
    }
    if (take < 1) return;
    // The array is sorted first (by the descriptor at TOC -0x6188), whatever the draws then do.
    invoke(c, callee::sort, 0x1046366c, {array, std::uint32_t(count), 4, c.toc(-0x6188)});
    c.set(3, take_register * 3);
    // The number of positions to shuffle: three times `take` (a halfword), at most the count.
    std::int32_t bound = halfword(take_register * 3);
    if (bound > count) bound = count;
    for (std::int16_t round = 0; std::int32_t(round) < bound; round = std::int16_t(round + 1)) {
        const auto first = halfword(std::uint32_t(pooled_random(c, bound, 0x104636c4)));
        const auto second = halfword(std::uint32_t(pooled_random(c, bound, 0x10463820)));
        swap_words(c, array, first, second);
    }
}

std::int32_t pooled_draw(Cpu& c, std::int32_t bound) {
    Frame frame(c, 0x10463000, 0x80, {.first_gpr = 20});
    return pooled_random(c, bound, 0x10463054);
}
}

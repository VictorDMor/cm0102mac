// PLAYER_REGEN.CPP, block `candidate_score`: the game's pooled Random(n) (0x102da630) and the inline copies of it
// the module's routines carry. Every draw must be the original's: the same pool cells in the same order, the refill
// through the generator (0x1077b770) at the same moment with the same return address, the same number of draws, and the
// same floating-point operations in the same order (the FPSCR the original leaves is compared).
//
// The draw: the cursor (a cell holding the address of the sample the next draw reads) moves two bytes; past the pool's
// end it returns to the pool's start and the salt (a cell) is renewed from the generator's next value (its low 16
// bits). The draw is then (sample halfword + salt) taken modulo n (the remainder's sign made that of n), and 0 without
// any draw for n = 0. For a bound of 0x10000 or more in magnitude it is scaled: (n / 65535) * (the remainder of that sum
// by 65535), in doubles and truncated, plus a tenth (rounded) of a second draw below n * 10 / 65535, capped at n - 1.
// The product n * 10 wraps in a word, so the second bound lies in -32768..32768: the second draw is never scaled and the
// original's third and fourth inlined levels and its recursion are unreachable.
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
void Pool::advance(std::uint32_t back) const {
    const auto next = c.u32(cursor) + 2;
    const auto last = c.u32(end);
    c.put32(cursor, next);
    if (next > last) {
        c.put32(cursor, start);
        // The refill's call sets the link register; a routine that finishes through it must get its own back.
        const auto link = c.lr();
        c.call(callee::reseed, back);
        c.set_lr(link);
        c.put32(salt, c.r(3) & 0xffff);
    }
}

std::int32_t Pool::draw(std::int32_t bound, std::uint32_t back, std::uint32_t second_back, const double* converted,
                        ScaledLeftovers* leftovers) const {
    if (bound == 0) return 0;
    advance(back);
    return draw_advanced(bound, second_back, converted, leftovers);
}

std::int32_t Pool::draw_advanced(std::int32_t bound, std::uint32_t second_back, const double* converted,
                                 ScaledLeftovers* leftovers) const {
    const auto total = sum();
    if (bound < 0x10000 && bound > -0x10000) {
        // divw / mullw / subf: the remainder, negated for a bound that is not positive.
        const auto rest = std::int32_t(std::uint32_t(total) - std::uint32_t(divide_signed(total, bound)) * std::uint32_t(bound));
        return bound > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
    }
    // The scaled form.
    Fpu fp(c);
    const auto constants = c.toc(toc::constants);
    const auto bias = c.f64(constants + constant::bias), range = c.f64(constants + constant::word_range);
    if (converted) fp.fctiwz(0, *converted);
    auto scale = fp.fdiv(fp.fsub(biased(bound), bias), range);
    // The quotient of the sum by 65535 and of the bound's tenfold (the product wraps), as the compiler's reciprocal
    // multiplication gives them (a quotient rounded toward zero by adding the sign bit).
    constexpr std::int32_t reciprocal = std::int32_t(0x80008001);
    const auto quotient_floor = std::int32_t(std::uint32_t(mulhw(reciprocal, total)) + std::uint32_t(total)) >> 15;  // the add wraps in a word
    const auto quotient_sign = quotient_floor < 0;
    const auto quotient = quotient_floor + quotient_sign;
    const auto product = std::int32_t(std::uint32_t(quotient) * 0xffffu);
    const auto remainder = std::int32_t(std::uint32_t(total) - std::uint32_t(product));
    const auto tenfold = std::int32_t(std::uint32_t(bound) * 10u);
    const auto smaller_floor = std::int32_t(std::uint32_t(mulhw(reciprocal, tenfold)) + std::uint32_t(tenfold)) >> 15;
    const auto smaller_sign = smaller_floor < 0;
    const auto smaller = smaller_floor + smaller_sign;
    scale = fp.fmul(scale, fp.fsub(biased(remainder), bias));
    const auto scaled = std::int32_t(std::uint32_t(fp.fctiwz(0, scale)));
    if (leftovers) {
        // r5 holds the quotient times 65535, r6 the sign of the quotient's floor, r7 the sum, r8 the sample, r9 the high word.
        leftovers->scaled_product = std::uint32_t(product);
        leftovers->sign_of_quotient = quotient_sign;
        leftovers->sum = std::uint32_t(total);
        leftovers->sample = std::uint32_t(total) - c.u32(salt);
        leftovers->high_word = conversion_high;
    }
    // The second draw (0 without drawing when its bound is 0), a tenth of it rounded, added and capped.
    const auto second = smaller == 0 ? 0 : draw(smaller, second_back, second_back);
    const auto fifth = std::int32_t(std::uint32_t(second) + 5u);
    const auto tenth_floor = mulhw(0x66666667, fifth) >> 2;
    const auto tenth = tenth_floor + (tenth_floor < 0);
    if (converted) fp.fctiwz(0, *converted);
    auto result = std::int32_t(std::uint32_t(scaled) + std::uint32_t(tenth));
    const auto top = std::int32_t(std::uint32_t(bound) - 1u);
    if (result > top) result = top;
    // The post-step's sign is the last value r4 holds.
    if (leftovers) leftovers->sign_of_tenth = tenth_floor < 0;
    return result;
}

std::int32_t draw_below(Cpu& c, std::int32_t bound) {
    save_link(c);
    const Pool pool(c);
    ScaledLeftovers left;
    const auto result = pool.draw(bound, refill_site::random_first, refill_site::random_second, nullptr, &left);
    // Only a scaled draw writes r4..r9; the other forms leave them to the caller (and the refill's own leftovers).
    if (bound >= 0x10000 || bound <= -0x10000)
        c.set({{4, left.sign_of_tenth}, {5, left.scaled_product}, {6, left.sign_of_quotient}, {7, left.sum}, {8, left.sample},
               {9, left.high_word}});
    return result;
}
}

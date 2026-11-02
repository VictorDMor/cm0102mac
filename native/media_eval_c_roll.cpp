// Kinds 0x3c and 0x3d. Side 1 gains rating, side 2 loses and the loss is rolled out of a range of
// the story's size, side 3 moves the first rating by the difference of two draws of twenty.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media::eval_c {
void Press::story_3c() {
    by_side([&] { up_3c(); }, [&] { down_3c(); }, [&] { third_3c(); });
}

void Press::up_3c() {
    if (flag(0x48f)) scale_by(0x78);
    else if (flag(0x30)) scale_by(0x98);
    c.set_f(1, bias());
    change_a(truncated(0x88, fpu.fmul(scale(), integer(0x80, strength()))), 0x10672a6c);
    const auto k = fpu.fmul(scale(), pull());
    const auto s = integer(0x78, strength());
    change_b(truncated(0x70, fpu.fmul(s, k)), 0, 0x10672abc);
    post(0x41, truncated(0x60, fpu.fmul(dbl(0xa0), integer(0x68, strength()))), 0x10672b10);
}

void Press::down_3c() {
    if (flag(0x48f)) scale_by(0x70);
    change_a(truncated(0x68, fpu.fmul(-scale(), integer(0x60, strength()))), 0x10672b9c);
    c.set(17, loss_roll());
    change_b(extended(std::int16_t(c.r(17))), 0, 0x10672f18);
    post(2, extended(std::int16_t(c.r(30) + 0x64)), 0x10672f40);
}

// The loss of side 2, rolled out of a range of the story's size (random(n) in line, with n recomputed
// from the float expression at every use): none when the amount is the pool's limit exactly, a
// remainder of the table's sample for a range within 16 bits, otherwise the float share of the range
// plus a tenth of a finer draw from 0x10647c00, at most n - 1. Returns the draw (r17).
std::uint32_t Press::loss_roll() {
    const auto k = [&] { return fpu.fmul(-scale(), pull()); };
    const auto amount = [&](std::uint32_t at) {
        const auto factor_k = k();
        return fpu.fmul(integer(at, strength()), factor_k);
    };
    // n by the stack pair at `at`, its fctiwz result in the pair at `out`.
    const auto range = [&](std::uint32_t at, std::uint32_t out) { return std::int32_t(truncated(out, amount(at))); };
    {
        const auto factor_k = k();
        const auto first = fpu.fmul(integer(0x70, strength()), factor_k);
        fpu.fcmp(0, first, dbl(0x28));
        if (first == dbl(0x28)) return 0;
    }
    rng.advance(0x10672bfc);
    const auto n = range(0x60, 0x68);
    c.set(3, 0xffff);
    if (compare(c, n, 0xffff) <= 0) {
        const auto again = range(0x60, 0x68);
        c.set(3, std::uint32_t(-0xffff));
        if (compare(c, again, -0xffff) >= 0) {
            // within 16 bits: the sample's remainder by n, negated for a range that is not positive
            const auto low = range(0x38, 0x40);
            const auto entry = c.u16(c.u32(rng.cursor));
            const auto sample = std::int32_t(entry + c.u32(rng.seed));
            const auto later = range(0x48, 0x50);
            // divw leaves 0 for a zero divisor (the generated code tests it)
            const auto rest = low ? remainder_of(sample, low) : sample;
            return compare(c, later, 0) > 0 ? std::uint32_t(rest) : 0u - std::uint32_t(rest);
        }
    }
    // beyond 16 bits (the float form)
    const auto first = range(0x60, 0x68);
    const double share = [&] {
        const auto conv = integer(0x70, first);
        return fpu.fdiv(conv, dbl(0));
    }();
    const auto entry = c.u16(c.u32(rng.cursor));
    const auto x = entry + c.u32(rng.seed);
    const auto whole = by_65535(x);
    const auto rest = x - std::uint32_t(whole.value) * 0xffff;
    const auto second = range(0x88, 0x58);
    const double unit = integer(0x78, std::int32_t(rest));
    const auto part = std::int32_t(truncated(0x80, fpu.fmul(share, unit)));
    const auto tenfold = by_65535(std::uint32_t(second) * 10);
    c.set(17, std::uint32_t(part));
    leave(c, {{4, 0x80008001u}, {5, 0x43300000}, {6, whole.sign}, {7, 0xffff}, {8, std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(x)))}, {9, x}});
    enter(c, 0x10672db4, {std::uint32_t(tenfold.value)});
    const auto finer = random_below(c, tenfold.value);
    const auto tenth = rounded_tenth(finer);
    leave(c, {{3, tenth.sign}, {4, 0x66666667}});
    const auto check = range(0x50, 0x48);
    c.set(3, std::uint32_t(check));
    c.set(17, std::uint32_t(part) + std::uint32_t(tenth.value));
    if (compare(c, std::int32_t(c.r(17)), std::int32_t(std::uint32_t(check) - 1)) <= 0) return c.r(17);
    const auto clamp = range(0x40, 0x38);
    c.set(3, std::uint32_t(clamp));
    return std::uint32_t(clamp) - 1;
}

void Press::third_3c() {
    const auto small = single_at(8);
    fpu.fcmp(0, factor(), small);
    if (factor() < small && flag(0x48f)) {
        // a small factor and a flagged club: the scaled amounts of the first two ratings
        scale_by(0x98);
        change_a(truncated(0x40, fpu.fmul(scale(), integer(0x38, strength()))), 0x10672fd4);
        const auto k = fpu.fmul(scale(), pull());
        const auto s = integer(0x48, strength());
        change_b(truncated(0x50, fpu.fmul(s, k)), 0, 0x10673024);
    } else {
        // otherwise the difference of two draws of twenty, as a share of the factor
        rng.advance(0x10673048);
        const auto first = reduced(0x66666667, 3, 20);
        c.set(17, std::uint32_t(first.rest));
        rng.advance(0x106730a0, {{3, first.sign}, {4, std::uint32_t(first.high)}, {5, std::uint32_t(first.sample)}, {6, first.entry}});
        const auto second = reduced(0x66666667, 3, 20);
        const auto difference = std::int32_t(c.r(17) - std::uint32_t(second.rest));
        const auto share = truncated(0x40, fpu.fmuls(factor(), fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(difference)), bias())));
        c.set(9, std::uint32_t(second.sample));
        change_a(share, 0x10673120);
    }
    post(0x44, 0, 0x10673144);
}
}

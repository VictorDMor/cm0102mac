#include "news_random.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::news {
using namespace detail;

std::uint32_t sample_range(Cpu& c, std::uint32_t bound_word) {
    Frame frame(c, 0x102c9880, 0x80, {.first_gpr = 23});
    Kept<29> bound(c, bound_word);
    keep(c, {{24, c.toc(-0x7b90)}, {25, c.toc(-0x7b8c)}, {26, c.toc(-0x7b88)},
             {27, c.toc(-0x7b84)}, {28, c.toc(-0x510c)}});
    if (compare(c, bound) == 0) return 0;
    const SamplePool pool{c, c.r(24), c.r(25), c.r(26), c.r(27)};
    pool.advance(0x102c98d4);
    const auto limit = std::int32_t(std::uint32_t(bound));
    if (limit >= -65535 && limit <= 65535) {
        const auto at = pool.position();
        compare(c, limit);
        const auto bias = pool.bias();
        return oriented_remainder(c.u16(at) + bias, limit);
    }

    // The cursor read precedes the upper conversion word. Keep it separate
    // from the later word/bias reads, including for overlapping guest views.
    c.put32(frame.at(0x4c), std::uint32_t(bound) ^ 0x80000000);
    const auto at = pool.position();
    c.put32(frame.at(0x48), 0x43300000);
    Fpu fp(c);
    c.set_f(2, c.f64(c.r(28) + 0x18)); c.set_f(1, c.f64(frame.at(0x48)));
    c.set_f(0, c.f64(c.r(28) + 8));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    const auto word = c.u16(at);
    const auto bias = pool.bias();
    const auto sum = std::uint32_t(word) + bias;
    c.put32(frame.at(0x40), 0x43300000);
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(1, c.f64(c.r(28) + 0x18));
    const auto remainder = remainder_word(sum, 65535);
    c.put32(frame.at(0x44), remainder ^ 0x80000000);
    c.set_f(0, c.f64(frame.at(0x40)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));

    const auto reduced = std::int32_t(std::uint32_t(bound) * 10u) / 65535;
    compare(c, reduced);
    const auto converted = fp.fctiwz(0, c.f(0));
    c.put64(frame.at(0x38), converted);
    Kept<30> coarse(c, c.u32(frame.at(0x3c)));
    leave(c, {{4, remainder ^ 0x80000000}, {5, 65535},
              {6, std::int32_t(sum) < 0 ? 1u : 0u}, {7, sum}, {8, word}});

    // wrap32(N*10)/65535 is in [-32768,32768] for every signed32 N.
    // The original's deeper unrolled wide arms and self-call cannot execute;
    // the first correction always uses its small signed-remainder arm.
    std::uint32_t correction = 0;
    if (reduced != 0) {
        pool.advance(0x102c99d4);
        const auto correction_at = pool.position();
        const auto correction_bias = pool.bias();
        const auto correction_sum = std::uint32_t(c.u16(correction_at)) + correction_bias;
        correction = oriented_remainder(correction_sum, reduced);
        leave(c, {{5, std::uint32_t(reduced)}, {6, correction_sum}});
    }
    const auto numerator = std::int32_t(correction + 5);
    const auto adjustment = numerator / 10;
    const auto high_product = std::int32_t(std::uint64_t(std::int64_t(numerator) * 0x66666667) >> 32);
    // Preserve the final srawi carry independently of the domain quotient.
    c.s.wr<1>(reg::xer_ca, high_product < 0 && (std::uint32_t(high_product) & 3));
    c.set(4, numerator < 0 ? 1 : 0);
    const auto candidate = std::uint32_t(coarse) + std::uint32_t(adjustment);
    const auto upper = std::uint32_t(bound) - 1;
    return compare(c, candidate, upper) > 0 ? upper : candidate;
}
}

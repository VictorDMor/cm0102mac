#include "match_pl.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
struct SamplePool {
    Cpu& c;
    std::uint32_t seed, begin, end, cursor;
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        const auto ordering = compare_logical(c, next, c.u32(end));
        c.set(3, next); c.put32(cursor, next);
        if (ordering > 0) {
            c.put32(cursor, begin); c.call(0x1077b770, back);
            c.put32(seed, c.r(3) & 0xffff);
        }
    }
};
std::uint32_t remainder(std::uint32_t value, std::int32_t bound) {
    const auto quotient = std::uint32_t(divide_signed<4>(value, std::uint32_t(bound)));
    return value - quotient * std::uint32_t(bound);
}
std::uint32_t oriented(std::uint32_t value, std::int32_t bound) {
    const auto residual = remainder(value, bound);
    return bound > 0 ? residual : 0u - residual;
}
}

std::uint32_t sample_range(Cpu& c, std::uint32_t bound_word) {
    Frame frame(c, 0x10226520, 0x80, {.first_gpr = 23});
    Kept<29> bound(c, bound_word);
    keep(c, {{24, c.toc(-0x51e8)}, {25, c.toc(-0x7b90)}, {26, c.toc(-0x7b8c)},
             {27, c.toc(-0x7b88)}, {28, c.toc(-0x7b84)}});
    if (compare(c, std::int32_t(bound_word)) == 0) return result(c, 0);
    const SamplePool pool{c, c.r(25), c.r(26), c.r(27), c.r(28)};
    pool.advance(0x10226574);
    const auto limit = std::int32_t(bound_word);
    if (limit >= -65535 && limit <= 65535) {
        const auto at = c.u32(pool.cursor);
        compare(c, limit);
        const auto bias = c.u32(pool.seed);
        return result(c, oriented(std::uint32_t(c.u16(at)) + bias, limit));
    }

    // The wide path converts signed32 bounds exactly through the original
    // stack image, and scales a signed remainder of the first table draw.
    c.put32(frame.at(0x4c), bound_word ^ 0x80000000);
    const auto at = c.u32(pool.cursor);
    c.put32(frame.at(0x48), 0x43300000);
    const auto constants = c.r(24);
    Fpu fp(c);
    c.set_f(2, c.f64(constants + 0x80)); c.set_f(1, c.f64(frame.at(0x48)));
    c.set_f(0, c.f64(constants));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    const auto word = c.u16(at);
    const auto bias = c.u32(pool.seed);
    const auto sum = std::uint32_t(word) + bias;
    c.put32(frame.at(0x40), 0x43300000);
    c.set_f(2, fp.fdiv(c.f(1), c.f(0))); c.set_f(1, c.f64(constants + 0x80));
    const auto residual = remainder(sum, 65535);
    c.put32(frame.at(0x44), residual ^ 0x80000000);
    c.set_f(0, fp.fsub(c.f64(frame.at(0x40)), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto reduced = std::int32_t(bound_word * 10u) / 65535;
    compare(c, reduced);
    c.put64(frame.at(0x38), fp.fctiwz(0, c.f(0)));
    Kept<30> coarse(c, c.u32(frame.at(0x3c)));
    leave(c, {{4, residual ^ 0x80000000}, {5, 65535},
              {6, std::int32_t(sum) < 0 ? 1u : 0u}, {7, sum}, {8, word}});

    // wrap32(N*10)/65535 lies in [-32768,32768]. Therefore every original
    // deeper wide arm and the self-call are unreachable for signed32 N.
    // Keep both reachable draw counts, including a zero correction bound.
    std::uint32_t correction = 0;
    if (reduced != 0) {
        pool.advance(0x10226674);
        const auto correction_at = c.u32(pool.cursor), correction_bias = c.u32(pool.seed);
        const auto correction_sum = std::uint32_t(c.u16(correction_at)) + correction_bias;
        correction = oriented(correction_sum, reduced);
        leave(c, {{5, std::uint32_t(reduced)}, {6, correction_sum}});
    }
    c.set(31, correction);
    const auto numerator = std::int32_t(correction + 5);
    const auto high_product = std::int32_t(std::uint64_t(std::int64_t(numerator) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high_product < 0 && (std::uint32_t(high_product) & 3));
    c.set(4, numerator < 0 ? 1 : 0);
    const auto candidate = std::uint32_t(coarse) + std::uint32_t(numerator / 10);
    const auto upper = bound_word - 1;
    return result(c, compare(c, std::int32_t(candidate), std::int32_t(upper)) > 0 ? upper : candidate);
}
}

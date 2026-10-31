#include "awol_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::awol {
using namespace detail;
namespace {
// The compiler's 0x80008001 division by 65535.
struct Tenfold { std::int32_t product, reduced; std::uint32_t sign; };
Tenfold reduce(std::int32_t bound) {
    const auto product = std::int32_t(std::uint32_t(bound) * 10u);
    const auto floor = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x80008001), product)) + std::uint32_t(product)) >> 15;
    const auto sign = std::uint32_t(floor) >> 31;
    return {product, std::int32_t(std::uint32_t(floor) + sign), sign};
}
struct Word { std::uint32_t sum, sign, product, rest; };
Word reduce_word(std::uint32_t sum) {
    const auto floor = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(sum))) + sum) >> 15;
    const auto sign = std::uint32_t(floor) >> 31;
    const auto product = (std::uint32_t(floor) + sign) * 0xffffu;
    return {sum, sign, product, sum - product};
}
// The double the wide form converts to: (bound/65535-scale)*(word/65535-scale)
// truncated. Returns its low word.
std::uint32_t scaled_draw(Cpu& c, const Pool& pool, std::int32_t bound, const Word& word, std::uint32_t input, std::uint32_t work, std::uint32_t output) {
    Fpu fp(c);
    c.put32(input + 4, std::uint32_t(bound) ^ 0x80000000u);
    c.put32(input, 0x43300000);
    c.set_f(2, c.f64(pool.constants + 8));
    c.set_f(1, c.f64(input));
    c.set_f(0, c.f64(pool.constants));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(1, fp.fdiv(c.f(1), c.f(0)));
    c.put32(work, 0x43300000);
    c.put32(work + 4, word.rest ^ 0x80000000u);
    c.set_f(0, c.f64(work));
    c.set_f(0, fp.fsub(c.f(0), c.f(2)));
    c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.put64(output, fp.fctiwz(0, c.f(0)));
    return c.u32(output + 4);
}
std::uint32_t small_draw(Cpu& c, const Pool& pool, std::int32_t bound) {
    c.set(3, pool.position());
    compare(c, bound);
    c.set(3, c.u16(c.r(3)) + pool.bias());
    const auto rest = remainder_of(c.r(3), std::uint32_t(bound));
    return bound > 0 ? rest : 0u - rest;
}
}

std::uint32_t sample(Cpu& c, std::uint32_t bound) {
    Frame frame(c, 0x10006080, 0x80, {.first_gpr = 20});
    const auto pool = Pool::at(c);
    keep(c, {{25, pool.constants}, {26, pool.salt}, {27, pool.base}, {28, pool.end}, {29, pool.cursor}, {30, bound}});
    if (compare(c, std::int32_t(bound)) == 0) return result(c, 0);
    pool.advance(0x100060d4);
    c.set(3, 0x10000);
    const auto above = compare(c, std::int32_t(bound), 65535) > 0;
    if (!above) c.set(3, 0xffff0000);
    if (!above && compare(c, std::int32_t(bound), -65535) >= 0) return result(c, small_draw(c, pool, std::int32_t(bound)));
    const auto at = pool.position();
    const auto word_value = c.u16(at);
    const auto word = reduce_word(word_value + pool.bias());
    const auto tenfold = reduce(std::int32_t(bound));
    c.set(3, at);
    c.set(9, 0x43300000);
    c.set(22, std::uint32_t(tenfold.reduced));
    compare(c, tenfold.reduced);
    c.set(31, scaled_draw(c, pool, std::int32_t(bound), word, frame.at(0x48), frame.at(0x40), frame.at(0x38)));
    leave(c, {{4, tenfold.sign}, {5, word.product}, {6, word.sign}, {7, word.sum}, {8, word_value}});
    // wrap32(bound*10)/65535 is at most 32768 in size, so the original's
    // deeper unrolled wide arms and its recursive call are unreachable.
    if (tenfold.reduced) {
        pool.advance(0x100061c0);
        c.set(3, 0x10000);
        compare(c, tenfold.reduced, 65535);
        c.set(3, 0xffff0000);
        compare(c, tenfold.reduced, -65535);
        c.set(24, small_draw(c, pool, tenfold.reduced));
    } else c.set(24, 0);
    const auto numerator = std::int32_t(c.r(24) + 5);
    c.set(4, numerator < 0 ? 1 : 0);
    c.set(3, std::uint32_t(numerator / 10));
    c.set(31, c.r(31) + c.r(3));
    const auto upper = bound - 1;
    if (compare(c, std::int32_t(c.r(31)), std::int32_t(upper)) > 0) c.set(31, upper);
    return result(c, c.r(31));
}

namespace detail {
std::int32_t draw(Cpu& c, const Pool& pool, const Draw& site) {
    const auto bound = std::int32_t(c.r(site.bound));
    if (site.cached) pool.advance_from(c.r(3), site.start + 0x1c);
    else pool.advance(site.start + 0x20);
    c.set(3, 0x10000);
    compare(c, bound, 65535);
    c.set(3, 0xffff0000);
    compare(c, bound, -65535);
    c.set(site.result, small_draw(c, pool, bound));
    return std::int32_t(c.r(site.result));
}
}
}

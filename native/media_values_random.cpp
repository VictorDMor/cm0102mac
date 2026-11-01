// 0x10647c00, the press's random(n): the draw every other routine of the module reaches.
// The compiler inlined it into itself several levels deep; within 16 bits the draw is a
// remainder of the table's sample, beyond it part = n / 65535.0 * (x mod 65535) plus a tenth
// of the next level's draw, over n * 10 / 65535 (wrapping), clamped to n - 1.
// The next range is a signed 32-bit word divided by 65535, so |range| <= 32768 and the
// second level is always the remainder form: the original's deeper levels (the float form
// again, and the real call of the routine itself) can never run, and are not written.
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;

std::int32_t random_below(Cpu& c, std::int32_t n) {
    Frame frame(c, 0x10647c00, 0x80, {.first_gpr = 23});
    Fpu fpu(c);
    const auto rng = Stream::world(c);
    keep(c, {{24, c.toc(slot::floats)}, {25, rng.seed}, {26, rng.start}, {27, rng.end}, {28, rng.cursor}, {29, std::uint32_t(n)}});
    if (!nonzero(c, n)) return std::int32_t(result(c, 0));
    rng.advance(0x10647c54);
    if (!beyond_table(n)) {
        const auto value = narrow_draw(rng, n);
        compare(c, n);  // cmpwi r29,0: the sign test
        return std::int32_t(result(c, std::uint32_t(value)));
    }
    const Floats pool(c);  // only the wide form reads the floats
    const auto d = wide_draw(c, fpu, rng, pool, n, frame.at(0x48), frame.at(0x40), frame.at(0x38));
    c.set(30, std::uint32_t(d.part));
    leave(c, {{3, d.coarse.sign}, {4, signed_word(std::int32_t(d.rest))}, {5, 0xffff}, {6, d.whole.sign}, {7, d.x}, {8, d.entry}});
    const auto range = d.coarse.value;
    compare(c, range);  // add. r0: the finer range against 0
    std::int32_t finer = 0;
    if (range != 0) {
        rng.advance(0x10647d54);
        finer = narrow_draw(rng, range);
        const auto x = std::int32_t(rng.sample());
        const auto product = std::uint32_t(divide(x, range)) * std::uint32_t(range);
        leave(c, {{3, d.coarse.sign}, {4, std::uint32_t(product)}, {5, std::uint32_t(range)}, {6, std::uint32_t(x)}});
        compare(c, range);
    }
    c.set(31, std::uint32_t(finer));
    // part + a tenth of the finer draw, at most n - 1 (quirk: no lower bound).
    const auto tenth = rounded_tenth(finer);
    const auto limit = std::int32_t(std::uint32_t(n) - 1);
    const auto total = std::int32_t(std::uint32_t(d.part) + std::uint32_t(tenth.value));
    const auto value = compare(c, total, limit) > 0 ? limit : total;
    c.set(30, std::uint32_t(value));
    leave(c, {{4, tenth.sign}});
    return std::int32_t(result(c, std::uint32_t(value)));
}
}

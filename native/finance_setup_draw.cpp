// FINANCE.CPP: the draw below a limit (0x100f2100), the unit's helper for random amounts.
#include "finance_setup.hpp"
#include "guest_fpu.hpp"
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;

std::int32_t random_below(Cpu& c, std::int32_t limit) {
    Frame frame(c, 0x100f2100, 0x80, {.first_gpr = 20});
    if (limit == 0) return 0;
    guest::Fpu fpu(c);
    const auto pool = c.toc(toc::doubles);
    struct Level { std::int32_t range = 0, partial = 0; } level[2];
    constexpr std::uint32_t redraw_back[2] = {0x100f2154, 0x100f2240};
    // The registers the original holds when a draw at depth d redraws: the sample-table cells, the doubles
    // pool and the limit; the second level also holds its range and the first level's partial result.
    const auto hold = [&](int depth) {
        c.set({{25, c.toc(sample_table::base)}, {26, c.toc(sample_table::first)}, {27, c.toc(sample_table::last)},
               {28, c.toc(sample_table::cursor)}, {29, pool}, {30, std::uint32_t(limit)}});
        if (depth >= 1) c.set({{22, std::uint32_t(level[1].range)}, {31, std::uint32_t(level[0].partial)}});
    };
    std::int32_t range = limit, result = 0;
    int depth = 0;
    for (;; ++depth) {
        // The original has two more inline levels (0x100f2328 and 0x100f2414 on) and a call of itself
        // (0x100f24dc). None can run: a level goes on past its draw only for a range outside +-0xffff, and the
        // next range is `range * 10 / 0xffff` with the product wrapping in a word, which is at most 32768 in
        // size for every word. So the second level always draws once or has range 0.
        if (depth == 2) throw std::logic_error("random below: a range past 0xffff after two levels");
        level[depth].range = range;
        if (range == 0) {
            result = 0;
            break;
        }
        const auto sample = setup::sample(c, redraw_back[depth], [&] { hold(depth); });
        if (setup::fits_one_draw(range)) {
            result = setup::remainder_in(sample, range);
            break;
        }
        const double scale = fpu.fdiv(fpu.fsub(setup::magic(range), c.f64(pool + doubles::int_magic)), c.f64(pool + 8));
        const std::int32_t rest = std::int32_t(sample) % 0xffff;
        const double part = fpu.fmul(scale, fpu.fsub(setup::magic(rest), c.f64(pool + doubles::int_magic)));
        level[depth].partial = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
        range = std::int32_t(std::uint32_t(range) * 10u) / 0xffff;
    }
    for (int d = depth - 1; d >= 0; --d) {
        std::int32_t part = std::int32_t(std::uint32_t(level[d].partial) +
                                         std::uint32_t(std::int32_t(std::uint32_t(result) + 5u) / 10));
        const std::int32_t limit_here = std::int32_t(std::uint32_t(level[d].range) - 1u);
        if (part > limit_here) part = limit_here;
        result = part;
    }
    return result;
}
}

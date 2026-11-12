// DISCIPLINE.CPP review block: this module's copy of the game's range sampler (0x100e8d10), native-first.
#include "discipline_review.hpp"
#include "discipline_review_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace review;

namespace {
// The compiler's int-to-double idiom: the word xor 0x80000000 under the exponent word 0x43300000, less the pool's
// double 2^52 + 2^31 (done by the caller with the Fpu).
double widened(std::uint32_t word) { return std::bit_cast<double>(std::uint64_t(0x43300000) << 32 | (word ^ 0x80000000u)); }
}

std::int32_t sample_range(Cpu& c, std::int32_t bound) {
    Frame frame(c, 0x100e8d10, 0x80, {.first_gpr = 20});
    if (bound == 0) return 0;
    const auto table = draw_table(c);
    const auto position = step(c, table, c.u32(table.cursor_cell), 0x100e8d64);
    // The signed word compare of the original: -65535 <= bound <= 65535 takes the small arm.
    if (bound >= -65535 && bound <= 65535) return oriented_remainder(draw_sum(c, table, position), bound);

    // The wide arm: bound/65535 (the pool's first double) times the draw reduced mod 65535, truncated by fctiwz.
    guest::Fpu fpu(c);
    const auto pool = c.toc(globals::doubles);
    const auto scale = c.f64(pool), magic = c.f64(pool + 8);
    auto ratio = fpu.fsub(widened(std::uint32_t(bound)), magic);
    const auto sum = std::int32_t(draw_sum(c, table, position));
    ratio = fpu.fdiv(ratio, scale);
    const auto residue = sum % 65535;  // the compiler's signed division by 65535
    const auto scaled = fpu.fmul(ratio, fpu.fsub(widened(std::uint32_t(residue)), magic));
    const auto coarse = std::int32_t(std::uint32_t(fpu.fctiwz(0, scaled)));

    // The correction is a draw bounded by wrap32(bound * 10) / 65535, which the signed multiply and divide keep within
    // [-32768, 32768]: its own wide arm and the self-call at 0x100e90ec (and every deeper clone of this arm) cannot run.
    const auto reduced = std::int32_t(std::uint32_t(bound) * 10u) / 65535;
    std::int32_t correction = 0;
    if (reduced != 0) {
        const auto next = step(c, table, position, 0x100e8e50);
        correction = oriented_remainder(draw_sum(c, table, next), reduced);
    }
    // (x + 5) / 10, the compiler's signed division by 10.
    const auto result = std::int32_t(std::uint32_t(coarse) + std::uint32_t((correction + 5) / 10));
    const auto limit = std::int32_t(std::uint32_t(bound) - 1u);
    return result > limit ? limit : result;
}
}

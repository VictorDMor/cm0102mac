// DATABASE.CPP's generation block, part 8: 0x100a0d80, a random number below a bound from the sample
// pool (database_generation.hpp).
//
// Contract. For |bound| <= 65535 the number is |(sample + salt) % bound| (one pool advance). A wider bound
// scales the sample in floating point (generation::wide_scale), adds the number the next level draws below
// bound' = trunc(int32(bound * 10) / 65535) divided by 10, and clamps to bound - 1.
//
// The original is that recursion unrolled four levels deep in one frame, with a real self-call for the fifth.
// Here the only reachable shape is: level 0, then at most one more level. int32(bound * 10) wraps, so
// |bound'| <= 32768 for every 32-bit bound: level 1 is always the plain remainder (it needs no scaling), or does
// not run at all when bound' is 0. A level 2 would need |bound'| > 65535, so levels 2-4 and the self-call are
// unreachable and are not written; reaching them would need a bound of 2^64 or more.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"

namespace cm::aot::database {
using namespace generation;

std::uint32_t range_sample(Cpu& c, std::int32_t bound) {
    constexpr std::uint32_t entry = 0x100a0d80;
    c.set(3, std::uint32_t(bound));
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 23});
    compare(c, bound);
    // r24-r28: the pool's salt, start, end and cursor words and the constant pool; r29 the bound.
    const auto pool = load_pool(c);
    c.set({{29, std::uint32_t(bound)}, {24, pool.salt_at}, {25, pool.start}, {26, pool.end_at}, {27, pool.cursor_at},
           {28, pool.constants}});
    if (bound == 0) {
        c.set(3, 0);
        return 0;
    }
    advance(c, pool, entry + 0x54);
    // The two tests of the bound against +-65535 (r3 holds the limit, cr0 the compare).
    c.set(3, narrow_limit);
    bool narrow = compare(c, bound, narrow_limit) <= 0;
    if (narrow) {
        c.set(3, 0xffff0001);
        narrow = compare(c, bound, -narrow_limit) >= 0;
    }
    if (narrow) {
        const auto draw = draw_below(c, pool, bound, 3);
        c.set(3, std::uint32_t(draw));
        return c.r(3);
    }

    // Wide: the floating-point scaling of this level (into r30), then the next level's bound.
    const auto next = wide_scale(c, pool.constants, std::uint32_t(bound), 27, 24, 30);
    const auto lower = std::int32_t(next.shifted + next.sign);
    c.set(0, std::uint32_t(lower));
    compare(c, lower);
    if (lower == 0) {
        c.set(31, 0);
    } else {
        // |lower| <= 32768 (see the contract above): the next level is the plain remainder.
        advance(c, pool, entry + 0x154);
        const auto draw = draw_below(c, pool, lower, 3);
        c.set(6, c.r(3));  // the sum, as the original's register holds it
        c.set(5, std::uint32_t(lower));
        c.set(31, std::uint32_t(draw));
    }
    // Add the next level's number divided by 10 and clamp to bound - 1 (multiply-high by 0x66666667 and a shift).
    c.set(3, c.r(31) + 5);
    c.set(3, std::uint32_t((std::int64_t(0x66666667) * std::int32_t(c.r(3))) >> 32));
    c.set(3, srawi(c, c.r(3), 2));
    c.set(4, c.r(3) >> 31);
    c.set(3, c.r(3) + c.r(4));
    c.set(30, c.r(30) + c.r(3));
    const auto cap = std::uint32_t(bound) - 1;  // wraps for INT32_MIN, as subi does
    if (compare(c, std::int32_t(c.r(30)), std::int32_t(cap)) > 0) c.set(30, cap);
    c.set(3, c.r(30));
    return c.r(3);
}
}

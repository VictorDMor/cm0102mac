// COMP_UTIL.CPP, block `random_shuffle`: cases for the unit's pooled Random(n) (0x10463000) and the two shuffles
// built on it (0x10462cd0, 0x104635b0). The cursor, the salt and the generator's state are the career's real ones
// (the generator itself, 0x1077b770, is rng_core's, called through the dispatcher on both sides); what a case needs
// of the pool is written over them in the case's own memory (undone afterwards): the cursor `back` samples before
// the pool's end, so the refill falls at the first draw, the second, mid-shuffle, or not at all, and a few salts
// (0, 1, 0x7fff, 0xffff) so that the sums reach the extremes of the quotient and remainder by 65535. The arrays
// are staged in the scratch area (this block's area is scratch_base + 0xb0000..0xc0000): words that tell the
// elements apart for the plain shuffle, club rows of the career (and empty slots) for the one that sorts first.
//
// 0x10463000 is also called with bounds of 0x10000 and above in magnitude (the scaled draw: a first part from the
// double arithmetic, truncated, and a second draw below the bound's tenfold over 65535), including the bounds
// whose tenfold wraps the 32-bit product (a negative or small second bound) and the extremes. The two shuffles only
// pass a bound that fits a signed halfword (their counts go through `extsh`), so only this entry reaches the
// scaled form: the shuffles' cases take counts from 0 (no draw, no swap) and negative (no iteration) to a few
// hundred, with the high bits of the registers set (they are ignored), and 0x10462cd0's counter of 10923 and above
// is not run (it never ends in the original either). The bounds assertion of 0x104635b0 (a negative count, a count
// below the number to take) is answered by the checker's scripted dialog.
// Labels start with "random_shuffle:" (CM_COMP_UTIL_BLOCK=random_shuffle).
#include "comp_util_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t shuffle = 0x10462cd0, draw = 0x10463000, shuffle_front = 0x104635b0;
constexpr std::int32_t salt_slot = -0x7b90, end_slot = -0x7b88, cursor_slot = -0x7b84, club_count_slot = -0x7b50;
constexpr std::uint32_t area = scratch_base + 0xb0000;
constexpr std::uint32_t words_at = area, clubs_at = area + 0x4000;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }

// The pool as a case sets it: the cursor `back` samples before the pool's end (0: at the end, so the next draw
// refills), or untouched (-1), and the salt (or untouched).
struct Pool { std::int32_t back; std::int64_t salt; std::string what; };
std::vector<Write> pool_memory(const Scenario& scenario, const Pool& pool) {
    std::vector<Write> memory;
    if (pool.back >= 0) memory.push_back(w32(scenario.toc(cursor_slot), scenario.global(end_slot) - 2 * std::uint32_t(pool.back)));
    if (pool.salt >= 0) memory.push_back(w32(scenario.toc(salt_slot), std::uint32_t(pool.salt)));
    return memory;
}
}

COMP_UTIL_CASES("random_shuffle") {
    // The scaled and small draws of 0x10463000.
    const std::int32_t bounds[] = {0, 1, -1, 2, -2, 3, 10, -10, 100, 0x7fff, -0x7fff, 0xffff, -0xffff, 0x10000, -0x10000, 0x10001, -0x10001,
                                   0x12345, -0x12345, 0x7ffff, 0xfffff, 0x1999a, 0x66666, 0x100000, 0x19999999, 0x1999999a, 0x20000000, -0x20000000,
                                   0x30000000, -0x30000000, 0x40000000, 0x7ffffffe, 0x7fffffff, std::int32_t(0x80000000), std::int32_t(0x80000001),
                                   std::int32_t(0xcccccccd), std::int32_t(0xe6666666), 123456789, -123456789, 65535 * 7, -65535 * 7, 65535 * 10 + 5};
    const Pool pools[] = {
        {-1, -1, "the real pool"}, {-1, 0, "salt 0"}, {-1, 1, "salt 1"}, {-1, 0xffff, "salt 0xffff"}, {-1, 0x7fff, "salt 0x7fff"}, {-1, 0xfffe, "salt 0xfffe"},
        {0, -1, "refill at the first draw"}, {1, -1, "refill at the second draw"}, {2, -1, "refill at the third draw"}, {0, 0xffff, "refill, then salt 0xffff"},
        {1, 0, "second draw refills, salt 0"}, {5, -1, "five samples left"},
    };
    for (const auto bound : bounds)
        for (const auto& pool : pools)
            cases.push_back({.entry = draw, .registers = {{3, std::uint32_t(bound)}}, .memory = pool_memory(scenario, pool),
                             .label = "random_shuffle: draw below " + hex(std::uint32_t(bound)) + ", " + pool.what});
}

COMP_UTIL_CASES("random_shuffle") {
    // The plain shuffle: words staged in the scratch area, counts from none to a few hundred.
    State& s = scenario.s;
    const auto saved_observer = s.memory.observer;
    s.memory.observer = nullptr;
    for (std::uint32_t n = 0; n < 0x400; ++n) s.memory.write<4>(words_at + n * 4, 0x1000 + n * 0x11);
    s.memory.observer = saved_observer;
    const std::uint32_t counts[] = {0, 1, 2, 3, 4, 5, 7, 8, 13, 24, 33, 100, 255, 0x10001 /* 1 */, 0xffff0003u /* 3 */, 0x00ff0005u /* 5 */,
                                    0xffffffffu /* -1 */, 0xfffffffbu /* -5 */, 0x8000 /* -32768 */, 0x12348000u /* -32768 */};
    const Pool pools[] = {{-1, -1, "the real pool"}, {0, -1, "refill at the first draw"}, {1, -1, "refill at the second draw"}, {2, -1, "refill at the third draw"},
                          {3, 0xffff, "refill after three draws, salt 0xffff"}, {7, 0, "refill in the middle, salt 0"}, {11, 1, "refill in the middle, salt 1"},
                          {40, 0x1234, "refill late"}};
    for (const auto count : counts)
        for (const auto& pool : pools)
            cases.push_back({.entry = shuffle, .registers = {{3, words_at}, {4, count}}, .memory = pool_memory(scenario, pool),
                             .label = "random_shuffle: shuffle " + hex(count) + " words, " + pool.what});
    // Larger runs: the cursor several draws before the end, counts that need hundreds of draws.
    for (const std::uint32_t count : {50u, 120u, 300u})
        for (const std::int32_t back : {10, 99, 100, 101, 299, 300, 301})
            cases.push_back({.entry = shuffle, .registers = {{3, words_at}, {4, count}}, .memory = pool_memory(scenario, {back, -1, ""}),
                             .label = "random_shuffle: shuffle " + hex(count) + " words, refill " + std::to_string(back) + " samples before the end"});
}

COMP_UTIL_CASES("random_shuffle") {
    // The shuffle of the front: the array holds club rows of the career (the descriptor sorts by reputation, empty
    // slots last), some slots empty; pairs of (count, take) inside the bounds, on them and outside.
    State& s = scenario.s;
    const auto saved_observer = s.memory.observer;
    s.memory.observer = nullptr;
    const auto club_total = scenario.global(club_count_slot);
    for (std::uint32_t n = 0; n < 0x400; ++n)
        s.memory.write<4>(clubs_at + n * 4, n % 11 == 5 ? 0 : scenario.club((n * 37 + 11) % club_total));
    s.memory.observer = saved_observer;
    struct Pair { std::uint32_t count, take; };
    const Pair pairs[] = {{0, 0}, {1, 0}, {1, 1}, {2, 1}, {2, 2}, {3, 1}, {3, 3}, {5, 1}, {5, 2}, {5, 5}, {10, 3}, {10, 4}, {10, 10}, {18, 6}, {18, 7}, {30, 10}, {30, 11},
                          {40, 7}, {40, 40}, {100, 5}, {100, 34}, {256, 100}, {500, 200},
                          // The register's high bits are ignored (extsh): a count of 5 and a take of 2.
                          {0x10005, 0xff0002}, {0xffff0007u, 0x30003}, {5, 0x10001},
                          // Outside the bounds: a negative count, a count below the number to take, a negative take.
                          {0xffffffffu, 0}, {0xffffffffu, 1}, {0xfffffff0u, 0xfffffff0u}, {3, 4}, {0, 1}, {5, 6}, {40, 41}, {0x8000, 0}, {5, 0xffffffffu}, {1, 0xffffffffu}, {0, 0xffffffffu},
                          // A take whose triple wraps the halfword (10923 * 3 = 32769 is negative as one): no iteration, but the sort.
                          {11000, 10923}};
    const Pool pools[] = {{-1, -1, "the real pool"}, {0, -1, "refill at the first draw"}, {1, -1, "refill at the second draw"}, {5, 0xffff, "refill after five draws, salt 0xffff"},
                          {13, 0, "refill in the middle, salt 0"}, {29, 1, "refill late"}};
    for (const auto& pair : pairs)
        for (const auto& pool : pools) {
            if (pair.count == 11000 && &pool != &pools[0]) continue;
            cases.push_back({.entry = shuffle_front, .registers = {{3, clubs_at}, {4, pair.count}, {5, pair.take}}, .memory = pool_memory(scenario, pool),
                             .label = "random_shuffle: take " + hex(pair.take) + " of " + hex(pair.count) + ", " + pool.what});
        }
}
}

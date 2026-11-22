// The font engine, block `util` (0x10767ce0..0x10768074): cases for the fixed-point helpers, the distance and the sort of
// halfwords. The helpers are pure functions of r3 and r4, so the cases are values: every pair of an edge set (zero, one, the
// extremes of both signs, the 16-bit boundaries, the halves that make the cross products wrap, sign flips) and seeded random
// pairs. A zero divisor faults the original (division by zero in translated code) and the rewrite alike ("both faulted").
// The sort runs on halfword arrays the case builds in the scratch area (empty, one element, sorted, reversed, equal, random,
// the sign edge of the halfwords) with the counts the original's loop treats differently (0, 1, negative). Labels start with
// "util:" (CM_FONT_ENGINE_BLOCK=util).
#include "font_engine_checks.hpp"
#include <array>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t fix_multiply = 0x10767d40, frac_multiply = 0x10767dc0, scale_multiply = 0x10767e40, fix_divide = 0x10767eb0,
                        distance = 0x10767f20, sort_shorts = 0x10767ce0;
constexpr std::array<std::uint32_t, 34> edges = {
    0, 1, 2, 3, 7, 0x7f, 0x80, 0xff, 0x100, 0x7fff, 0x8000, 0x8001, 0xffff, 0x10000, 0x10001, 0x1ffff, 0x20000, 0x7fffffff,
    0x80000000, 0x80000001, 0xffff0000, 0xffff0001, 0xffff8000, 0xffffffff, 0xfffffffe, 0xffff7fff, 0xfffe0000, 0x12345678,
    0xedcba988, 0x00010000 + 0x8000, 0x40000000, 0xc0000000, 0x0000ffff + 0x10000 * 0x7fff, 0x55555555};
}

FONT_ENGINE_CASES("util") {
    std::mt19937 random(0x7e47);
    const auto add = [&](std::uint32_t entry, std::uint32_t a, std::uint32_t b, const char* what) {
        cases.push_back({.entry = entry, .registers = {{3, a}, {4, b}}, .label = std::string("util: ") + what + " " + hex(a) + " " + hex(b)});
    };
    for (const auto entry : {fix_multiply, frac_multiply, fix_divide, distance, scale_multiply}) {
        // The edge set squared; the distance and the division run 3-4 divisions a case, so their lists are shorter.
        const bool heavy = entry == fix_divide || entry == distance;
        for (std::size_t i = 0; i < edges.size(); ++i)
            for (std::size_t j = 0; j < edges.size(); ++j) {
                if (heavy && (i + j) % 2) continue;
                add(entry, edges[i], edges[j], "edge");
            }
        for (unsigned n = 0; n < (heavy ? 400u : 700u); ++n) {
            // Random pairs at every magnitude: a value is a random word shifted right by a random amount, signed or not.
            const auto narrow = [&] {
                const auto shift = random() % 32;
                const auto value = std::uint32_t(random()) >> shift;
                return random() % 2 ? 0u - value : value;
            };
            add(entry, narrow(), narrow(), "random");
        }
    }
    // The scale of 0x10767e40 is a sign-extended halfword in every real call: those values, with every sign of the value.
    for (const std::int32_t scale : {0, 1, -1, 2, -2, 0x7fff, -0x8000, -0x7fff, 0x4000, -0x4000, 0x1234, -0x1234, 100, -100, 0x6000, -0x6001})
        for (const auto value : edges) add(scale_multiply, value, std::uint32_t(scale), "scale");
    // The sort: arrays of halfwords in the scratch area.
    constexpr std::uint32_t array = scratch_base + 0x20000;
    const auto sorted_case = [&](const std::vector<std::int16_t>& values, std::int32_t count, const char* what) {
        std::vector<Write> memory;
        for (std::size_t n = 0; n < values.size() + 4; ++n) memory.push_back(w16(array + std::uint32_t(n) * 2, n < values.size() ? std::uint16_t(values[n]) : 0x7777));
        cases.push_back({.entry = sort_shorts, .registers = {{3, array}, {4, std::uint32_t(count)}}, .memory = memory,
                         .label = std::string("util: sort ") + what + " n=" + std::to_string(count)});
    };
    for (const std::int32_t count : {-5, -1, 0, 1, 2, 3}) sorted_case({5, -3, 9, 0, -3, 7}, count, "short counts");
    sorted_case({1, 2, 3, 4, 5, 6, 7, 8}, 8, "sorted");
    sorted_case({8, 7, 6, 5, 4, 3, 2, 1}, 8, "reversed");
    sorted_case({4, 4, 4, 4, 4}, 5, "equal");
    sorted_case({32767, -32768, 0, -1, 1, 32767, -32768}, 7, "extremes");
    sorted_case({2, 1}, 2, "pair");
    sorted_case({1, 2}, 2, "ordered pair");
    for (unsigned n = 0; n < 150; ++n) {
        std::vector<std::int16_t> values(1 + random() % 40);
        for (auto& value : values) value = std::int16_t(random() % 5 == 0 ? (random() % 2 ? 32767 : -32768) : std::int32_t(random() % 61) - 30);
        sorted_case(values, std::int32_t(values.size()), "random");
    }
    // The count past the rows read: the original reads on into the zero fill and a sentinel.
    sorted_case({3, 2, 1}, 6, "count past the rows");
}
}

// Cases for the report block: the restarts of each play state and the review of the match
// state, from a real match with the side, the ball's cell, the clock and the tactics varied.
// A live match reaches the review in normal play only.
#include "match_eng_checks.hpp"
#include <cstdlib>
#include <string>

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("report") {
    const auto match = scenario.match;
    const auto add = [&](std::uint32_t entry, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = {{3, match}}, .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    // Every restart, for each side that touched the ball last, over the ball's cells (some outside the pitch).
    for (const std::uint32_t entry : {0x101e50c0u, 0x101e53a0u, 0x101e59b0u, 0x101e6420u, 0x101e66b0u, 0x101e68b0u}) {
        for (const std::uint32_t side : {0u, 1u, 0xffu}) {
            for (const std::uint32_t across : {0u, 1u, 4u, 7u, 8u, 9u, 0xffu}) {
                for (const std::uint32_t along : {0u, 2u, 3u, 5u, 6u, 8u, 9u, 11u, 12u, 0xfeu}) {
                    add(entry, {{1, match + 0x7924, side}, {1, match + 0x791d, across}, {1, match + 0x791e, along},
                                {4, match + 0xe01c, scenario.player(0, 7)}, {4, match + 0xe020, scenario.player(1, 7)}}, "ball cell");
                }
            }
        }
    }
    // A pending player is released first; the ball in the corners of the pitch.
    for (const std::uint32_t entry : {0x101e53a0u, 0x101e59b0u, 0x101e6420u})
        for (const std::uint32_t side : {0u, 1u})
            add(entry, {{1, match + 0x7924, side}, {4, match + 0xe000, scenario.player(1, 4)}, {1, match + 0x791d, 8}, {1, match + 0x791e, 11}}, "pending player");

    // 0x101e6c50 over the play state, the clock, the tactics, the kinds of teams and the flags
    // the review reads. The values are drawn from fixed lists (the same on every run).
    const auto toc = std::uint32_t(scenario.s.r<4>(8));
    const auto pool = scenario.r32(toc - 0x52cc);
    const std::uint32_t texts[] = {pool + 0x3f0, pool + 0x3f6, pool + 0x3e8, pool + 0x406};
    const std::uint32_t floats[] = {0x00000000, 0x3f800000, 0x42c80000, 0xc2c80000, 0x461c4000, 0x7fc00000, 0x3e99999a};
    const std::uint32_t clocks[] = {0, 1, 2, 90, 200, 440, 450, 600, 671, 700, 935, 945, 980, 1100, 1300, 0xffff};
    const std::uint32_t marks[] = {0x1ef, 0x3de, 0x483, 0x528, 0, 0x100};
    std::uint32_t seed = 12345;
    const auto pick = [&](auto&& list) {
        seed = seed * 1103515245u + 12345u;
        return list[(seed >> 16) % (sizeof(list) / sizeof(list[0]))];
    };
    const auto below = [&](std::uint32_t limit) {
        seed = seed * 1103515245u + 12345u;
        return (seed >> 16) % limit;
    };
    for (unsigned n = 0; n < 1500; ++n) {
        std::vector<Write> memory;
        const std::uint32_t state = n < 40 ? n % 10 : below(10);
        memory.push_back({4, match + 0xe01c, scenario.player(0, 7)});
        memory.push_back({4, match + 0xe020, scenario.player(1, 7)});
        memory.push_back({1, match + 0x7928, state == 9 ? 0xffu : state});
        memory.push_back({2, match + 0x7948, pick(clocks)});
        memory.push_back({2, match + 0x794c, pick(marks)});
        memory.push_back({1, match + 0x792d, below(40)});
        memory.push_back({1, match + 0x7924, below(4) == 0 ? 0xffu : below(2)});
        memory.push_back({1, match + 0x791d, below(10)});
        memory.push_back({1, match + 0x791e, below(13)});
        memory.push_back({1, match + 0x7931, below(2)});
        memory.push_back({1, match + 0x1c1, below(8) == 0 ? 1u : 0u});
        memory.push_back({1, match + 0x77be, below(2)});
        memory.push_back({1, match + 0x77bf, below(2)});
        memory.push_back({2, match + 0x7944, below(3) == 0 ? 0u : pick(clocks)});
        memory.push_back({2, match + 0x7946, below(3) == 0 ? 0u : pick(clocks)});
        memory.push_back({2, match + 0x1ec, pick(clocks)});
        memory.push_back({2, match + 0x1ee, pick(clocks)});
        for (const std::uint32_t side : {0u, 1u}) {
            const auto stats = match + 0x7aac + side * 0xd8;
            const auto team = scenario.r32(stats + 0x34);
            memory.push_back({2, team + 0x1ec, below(7)});
            memory.push_back({1, match + 0x1c4 + side, below(7)});
            memory.push_back({1, match + 0x1c6 + side, below(3) == 0 ? 0xffu : below(2)});
            memory.push_back({4, stats + 0x30, pick(texts)});
            memory.push_back({4, match + 0x30f0 + 4 * side, pick(floats)});
            const auto style = scenario.r32(stats + 0x24);
            if (style && below(8) == 0) memory.push_back({4, stats + 0x24, 0});
            else if (style) {
                memory.push_back({1, style + 0x2e, below(24)});
                memory.push_back({1, style + 0x33, below(24)});
            }
        }
        cases.push_back({.entry = 0x101e6c50, .registers = {{3, match}}, .memory = std::move(memory), .f1 = {}, .label = "review " + std::to_string(n) + " state " + std::to_string(state)});
    }
}

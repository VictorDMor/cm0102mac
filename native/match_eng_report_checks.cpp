// Cases for the report block: the restarts of each play state and the review of the match
// state, from a real match with the side, the ball's cell, the clock and the tactics varied.
// A live match reaches the review in normal play only.
#include "match_eng_checks.hpp"
#include <cstdlib>
#include <string>
#include <tuple>

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("report") {
    const auto match = scenario.match;
    const auto add = [&](std::uint32_t entry, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = {{3, match}}, .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    // Every restart, for each side that touched the ball last, over the ball's cells (some outside the pitch).
    for (const std::uint32_t entry : {0x101e50c0u, 0x101e53a0u, 0x101e59b0u, 0x101e6420u, 0x101e66b0u, 0x101e68b0u}) {
        for (const std::uint32_t side : {0u, 1u, 0xffu}) {
            // The kick-off takes its side from the kick-off flip, never -1: the nearest-player search would read
            // before the player table (both runs fault there).
            if (entry == 0x101e68b0u && side == 0xffu) continue;
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
        memory.push_back({1, match + 0x7924, below(4) == 0 && state != 8 ? 0xffu : below(2)});
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

    // --- Roles and formation cells of the restarts of states 3 and 6 ---
    // The role of a player comes from the low word of his side's tendency flags (0x20000: 1, 0x40000000: 2 for the
    // taking side; 0x80000000: 3, 0x80000: 4 for the other); the formation cells are bytes (along << 4 | across).
    const auto tendency_words = [&](std::uint32_t first, std::uint32_t second) {
        std::vector<Write> writes;
        for (std::uint32_t side = 0; side < 2; ++side)
            for (std::uint32_t position = 0; position <= 10; ++position)
                writes.push_back({4, match + 0x7c6c + side * 0x18e0 + 0x188c + position * 8, side ? second : first});
        return writes;
    };
    const auto cell_bytes = [&](std::uint32_t mode, std::uint32_t offset, std::uint32_t value) {
        std::vector<Write> writes;
        for (const std::uint32_t table_at : {0xdfecu, 0xdff0u}) {
            const auto table = scenario.r32(match + table_at);
            for (std::uint32_t position = 0; position <= 10; ++position) writes.push_back({1, table + position * 0xd8 + mode * 0x6c + offset, value});
        }
        return writes;
    };
    struct Ball { std::uint32_t across, along; };
    const std::uint32_t roles[] = {0, 0x20000, 0x40000000, 0x80000000, 0x80000};
    for (const std::uint32_t entry : {0x101e53a0u, 0x101e59b0u}) {
        for (const std::uint32_t first : roles) {
            for (const std::uint32_t second : roles) {
                for (const std::uint32_t side : {0u, 1u}) {
                    for (const auto ball : {Ball{4, 6}, Ball{2, 5}, Ball{7, 8}, Ball{4, 3}, Ball{3, 4}}) {
                        auto memory = tendency_words(first, second);
                        for (const auto& write : std::vector<Write>{{1, match + 0x7924, side}, {1, match + 0x791d, ball.across}, {1, match + 0x791e, ball.along},
                                                                    {4, match + 0xe01c, scenario.player(0, 7)}, {4, match + 0xe020, scenario.player(1, 7)}})
                            memory.push_back(write);
                        add(entry, std::move(memory), "roles");
                    }
                }
            }
        }
    }
    // A formation cell past the pitch's far line (state 3), and cells equal to the ball's own cell near the goal of
    // the side that did not take the ball (state 6).
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t mode : {0u, 1u}) {
            for (const std::uint32_t value : {0xc0u, 0xc4u, 0xf8u}) {
                for (const std::uint32_t offset : {0x30u, 0x3bu}) {
                    auto memory = cell_bytes(mode, offset, value);
                    for (const auto& write : std::vector<Write>{{1, match + 0x7924, side}, {1, match + 0x791d, 4}, {1, match + 0x791e, 6},
                                                                {4, match + 0xe01c, scenario.player(0, 7)}, {4, match + 0xe020, scenario.player(1, 7)}})
                        memory.push_back(write);
                    add(0x101e53a0, std::move(memory), "far cell");
                }
            }
        }
    }
    for (const auto [side, across, along] : {std::tuple{0u, 4u, 1u}, {0u, 3u, 0u}, {1u, 4u, 10u}, {1u, 5u, 11u}, {0u, 4u, 2u}, {1u, 6u, 9u}}) {
        for (const std::uint32_t first : {0u, 0x20000u, 0x40000000u}) {
            auto memory = cell_bytes(1, 0x36, (along << 4) | across);
            for (const auto& write : tendency_words(first, first)) memory.push_back(write);
            for (const auto& write : std::vector<Write>{{1, match + 0x7924, side}, {1, match + 0x791d, across}, {1, match + 0x791e, along},
                                                        {4, match + 0xe01c, scenario.player(0, 7)}, {4, match + 0xe020, scenario.player(1, 7)}})
                memory.push_back(write);
            add(0x101e59b0, std::move(memory), "ball cell equals the formation cell");
        }
    }

    // --- The review's commentary: the goals of the period decide which team the lines are about ---
    // Per case: the goals (so the three numbers of 0x101f21d0 vary), the first-leg result, the kind bytes of both
    // teams' detail records, the random seed, tired players with and without a status, and the answers of the other
    // modules' routines (the team test of the lines, the name comparison, whether a side can supply a substitute).
    const auto fixture = scenario.r32(match + 0x3118);
    const auto seed_at = scenario.r32(toc - 0x7b90);
    const auto top = scenario.r32(scenario.r32(toc - 0x7be4));
    const std::uint32_t kinds[] = {0, 4, 5, 0xc, 0xc, 0xd, 0xe, 0xf};
    for (unsigned n = 0; n < 600; ++n) {
        std::vector<Write> memory;
        const std::uint32_t states[] = {1, 1, 1, 1, 2, 4, 6};
        const auto state = states[below(std::size(states))];
        for (const auto& write : std::vector<Write>{{4, match + 0xe01c, scenario.player(0, 7)}, {4, match + 0xe020, scenario.player(1, 7)},
                                                    {1, match + 0x7928, state}, {1, match + 0x7931, below(8) == 0 ? 0u : 1u},
                                                    {1, match + 0x7924, below(3)}, {1, match + 0x791d, 1 + below(7)}, {1, match + 0x791e, 3 + below(6)},
                                                    {1, match + 0xe03d, below(5)}, {1, match + 0xe073, below(5)}, {2, match + 0x7948, pick(clocks)},
                                                    {2, match + 0x794c, pick(marks)}, {1, match + 0x792d, below(40)}, {4, seed_at, 7331u * (n + 1)},
                                                    {1, match + 0x1c1, 0}}) {
            memory.push_back(write);
        }
        if (fixture) {
            const std::uint32_t legs[] = {0xff, 0xff, 0, 1, 2, 3};
            const std::uint32_t flag_sets[] = {0, 4, 0x4000, 0x80, 0x4004, 2};
            memory.push_back({1, fixture + 0x4b, legs[below(std::size(legs))]});
            memory.push_back({1, fixture + 0x4c, legs[below(std::size(legs))]});
            memory.push_back({2, fixture + 0x38, flag_sets[below(std::size(flag_sets))]});
            for (const std::uint32_t at : {0x1cu, 0x20u}) {
                const auto detail = scenario.r32(scenario.r32(fixture + at) + 0x2c);
                if (!detail) continue;
                memory.push_back({1, detail + 0x118, kinds[below(std::size(kinds))]});
                memory.push_back({1, detail + 0x119, below(2) ? 8u : 0x13u});
            }
        }
        for (const std::uint32_t side : {0u, 1u}) {
            const auto stats = match + 0x7aac + side * 0xd8;
            const auto record = scenario.r32(stats + 0x20);
            if (record && below(2)) {
                memory.push_back({4, record, top - 1});
                memory.push_back({1, record + 0x6b, 1});
            }
            if (below(3) == 0) memory.push_back({1, stats + 0x1b, 0});
        }
        if (below(3) == 0) {
            // The first players of the first side are tired, the first of them with a status.
            for (unsigned index = 0; index < 3; ++index) {
                memory.push_back({4, scenario.player(0, index) + 0x50, 0});
                memory.push_back({1, scenario.player(0, index) + 0x110, index < 2 ? 1u : 0u});
            }
            memory.push_back({1, scenario.player(0, 3) + 0x110, 0});
        }
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{0x101b0020, below(2)}, {0x1036ff00, below(2)}, {0x10147e70, below(2) && below(2)},
                                                                        {0x100c7d00, below(2)}, {0x101481d0, below(2)}};
        cases.push_back({.entry = 0x101e6c50, .registers = {{3, match}}, .memory = std::move(memory), .f1 = {}, .label = "commentary " + std::to_string(n) + " state " + std::to_string(state),
                         .answers = std::move(answers)});
    }
}

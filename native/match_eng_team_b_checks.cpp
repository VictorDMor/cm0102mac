// Cases for the team_b block: the contest of a cell with the game's random table forced to
// fixed and pseudo-random draws (the draws decide which attempts succeed and which
// announcements follow), out-of-range cells, and every action kind of the protected-action test.
#include "match_eng_checks.hpp"
#include <random>

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("team_b") {
    const auto match = scenario.match;
    const auto toc = std::uint32_t(scenario.s.r<4>(8));
    const auto word = [&](std::uint32_t at) { return scenario.r32(at); };
    // The random table: draws are table[cursor] + seed, the cursor stepping two bytes each draw.
    const auto seed_at = word(toc - 0x7b90), cursor = word(word(toc - 0x7b84));
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    const auto table = [&](auto&& value) {
        std::vector<Write> writes{{4, seed_at, 0}};
        for (unsigned k = 0; k < 120; ++k) writes.push_back({2, cursor + 2 * k, std::uint32_t(value(k)) & 0xffff});
        return writes;
    };
    // Fixed draws.
    for (const std::uint32_t fixed : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 20u, 100u, 199u, 200u, 1000u, 4000u, 20000u, 32767u, 32768u, 60000u, 65534u, 65535u})
        add(0x101ec890, {{3, match}, {4, 4}, {5, 5}}, table([&](unsigned) { return fixed; }), "fixed draws");
    // Pseudo-random draws, with the usual cells; some variants draw zero more often, which the
    // "one in n" decisions of the announcements need.
    std::mt19937 random(0x7e4b);
    for (unsigned variant = 0; variant < 6000; ++variant) {
        const std::uint32_t moduli[] = {3, 5, 10, 30, 100, 1000, 65536, 20000};
        const std::uint32_t modulus = moduli[variant % 8];
        const unsigned zero_percent = (variant / 8) % 5 * 20;  // 0, 20, 40, 60, 80
        std::vector<std::uint32_t> values;
        for (unsigned k = 0; k < 120; ++k) values.push_back(random() % 100 < zero_percent ? 0 : random() % modulus);
        const auto [row, column] = std::pair<std::uint32_t, std::uint32_t>{variant % 9, (variant / 9) % 12};
        auto memory = table([&](unsigned k) { return values[k]; });
        memory.push_back({1, match + 0x7931, random() % 3 == 0 ? 0u : 1u});             // the flag that opens the shot check
        memory.push_back({2, match + 0x7950, std::uint32_t(random() % 3 == 0 ? random() % 360 : 0)});  // the turn the bearings are compared with
        // Sometimes all the players stand in a goal area (the decisions about a player there).
        if (const auto area = random() % 5; area != 0) {
            const std::uint32_t columns[] = {0, 11, 1, 10};
            for (unsigned side = 0; side < 2; ++side)
                for (unsigned index = 0; index < 20; ++index) {
                    memory.push_back({1, scenario.player(side, index) + 0x105, 2 + random() % 5});
                    memory.push_back({1, scenario.player(side, index) + 0x106, columns[(area - 1 + (random() % 4 == 0 ? 1 : 0)) % 4]});
                }
        }
        add(0x101ec890, {{3, match}, {4, row}, {5, column}}, std::move(memory), "random draws");
    }
    // Cells outside the pitch.
    for (const auto [row, column] : {std::pair{9u, 0u}, {0u, 12u}, {0xffu, 3u}, {3u, 0xffu}, {8u, 11u}, {0u, 0u}, {0x80u, 0u}, {0u, 0x80u}})
        add(0x101ec890, {{3, match}, {4, row}, {5, column}}, {}, "cells");
    // Every action kind.
    for (std::uint32_t kind = 0; kind < 0x100; ++kind) add(0x101ee530, {{3, match}, {4, kind}}, {}, "action kinds");
    // The small queries over the players: sides, columns, the pending player and the cell flag table.
    const auto fill_table = [&](std::uint32_t value) {
        std::vector<Write> writes;
        for (std::uint32_t at = match + 0xa60; at < match + 0xa60 + 2 * 9 * 12 * 0x5c + 0x450; at += 4) writes.push_back({4, at, value});
        return writes;
    };
    for (const std::uint32_t side : {0u, 1u, 2u, 0xffu}) {
        add(0x101eee20, {{3, match}, {4, side}}, {}, "side count");
        for (const std::uint32_t threshold : {0u, 3u, 6u, 11u, 0xffu})
            for (const std::uint32_t skip : {0u, 1u}) {
                add(0x101ee680, {{3, match}, {4, threshold}, {5, side}, {6, skip}}, {}, "players ahead");
                add(0x101ee680, {{3, match}, {4, threshold}, {5, side}, {6, skip}}, {{4, match + 0xe000, scenario.player(side & 1, 4)}}, "players ahead, pending");
            }
    }
    for (const unsigned side : {0u, 1u})
        for (const unsigned index : {0u, 4u, 10u, 19u})
            for (const std::uint32_t row : {0u, 1u, 3u, 4u, 5u, 8u})
                for (const std::uint32_t column : {0u, 1u, 2u, 3u, 8u, 9u, 10u, 11u}) {
                    const auto player = scenario.player(side, index);
                    for (const std::uint32_t table : {0u, 1u}) {
                        auto memory = fill_table(table);
                        memory.push_back({1, player + 0x105, row});
                        memory.push_back({1, player + 0x106, column});
                        add(0x101eecb0, {{3, match}, {4, player}}, memory, "adjacent cell");
                        add(0x101ee770, {{3, match}, {4, player}}, memory, "lane");
                    }
                    for (const std::uint32_t target_row : {0u, 2u, 3u, 5u, 6u, 8u, 0xffu})
                        for (const std::uint32_t target_column : {0u, 2u, 5u, 9u, 11u}) {
                            add(0x101eeac0, {{3, match}, {4, player}, {5, target_row}, {6, target_column}},
                                {{1, player + 0x105, row}, {1, player + 0x106, column}}, "run");
                        }
                }
}

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
    // Outcomes and topologies the snapshot's own attempts never produce. The two attempt routines of another
    // module (0x1021c260 for the first try of a contender, 0x1021b860 for the second) are answered with a chosen
    // outcome, and the ball's cell is given one player per side (or one side only, whose bearing then sits exactly
    // opposite the turn: the only way the contest sees a single contender). The attempts' follow-ups (0x10221e00,
    // 0x1021bec0) are answered too. Draws come from tables with runs of zeros; a few use a negative seed, which the
    // random routine turns into a negative draw (a state the game's masked seed never has, but the code sees).
    {
        const auto byte_at = [&](std::uint32_t at) { return std::uint32_t(scenario.s.memory.read<1>(at)); };
        const auto sp = std::uint32_t(scenario.s.r<4>(4));
        const auto frame = sp - 0xd0;
        const auto ball_cell = match + 0xa08 + std::int32_t(std::int8_t(byte_at(match + 0x791d))) * 0x450 +
                               std::int32_t(std::int8_t(byte_at(match + 0x791e))) * 0x5c;
        const auto flag_table = [&](unsigned side, std::uint32_t flags) {
            std::vector<Write> writes;
            const auto at = word(match + 0x7934 + side * 4);
            for (std::uint32_t position = 0; position <= 10; ++position) writes.push_back({2, at + position * 2, flags});
            return writes;
        };
        const auto join = [](std::vector<Write>& a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); };
        std::mt19937 driven(0x5eed);
        const auto table_start = word(toc - 0x7b8c), cursor_at = word(toc - 0x7b84), table_end = word(word(toc - 0x7b88));
        const bool restartable = table_end >= table_start + 2 * 125;
        struct Pair { std::uint32_t first, second; bool negative = false; };
        std::vector<Pair> pairs;
        for (const std::uint32_t k : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0x80u, 0xffu}) pairs.push_back({k, 0});
        for (const std::uint32_t k : {1u, 2u, 3u, 4u, 5u, 6u, 7u}) pairs.push_back({0, k});
        pairs.push_back({3, 5}); pairs.push_back({7, 4}); pairs.push_back({5, 6}); pairs.push_back({4, 3});
        // Outcomes without a message of their own draw one at random; with negative draws and small bytes the draw's sign is seen.
        for (const std::uint32_t k : {1u, 8u, 0x80u, 0xffu}) pairs.push_back({k, 0, true});
        unsigned serial = 0;
        for (const auto [first, second, negative] : pairs)
            for (unsigned variant = 0; variant < 48; ++variant, ++serial) {
                const bool restart = restartable && variant % 2 == 0;
                const unsigned topology = variant % 3;  // 0: both sides, 1: side 1 only (bearing opposite), 2: side 0 only
                const std::uint32_t moduli[] = {3, 10, 100, 1000, 65536, 5};
                const std::uint32_t modulus = moduli[(variant / 3) % 6];
                const unsigned zero_percent = (variant / 18) % 3 * 30;  // 0, 30, 60
                std::vector<std::uint32_t> values;
                for (unsigned k = 0; k < 120; ++k) values.push_back(driven() % 100 < zero_percent ? 0 : driven() % modulus);
                // The first draws of the contest decide which attempt is cleared: make them zero for some variants.
                if (variant % 4 == 1) for (unsigned k = 0; k < 6; ++k) values[k] = 0;
                auto memory = table([&](unsigned k) { return values[k]; });
                if (restart) {  // the cursor at the table's start, so that no draw wraps and re-seeds
                    memory.clear();
                    memory.push_back({4, seed_at, 0});
                    memory.push_back({4, cursor_at, table_start});
                    for (unsigned k = 0; k < 120; ++k) memory.push_back({2, table_start + 2 * k, values[k]});
                }
                if (negative || serial % 5 < 2) memory[0] = {4, seed_at, 0xfffe7960u + serial % 7};  // seeds around -100000
                const unsigned index0 = driven() % 11, index1 = driven() % 11;
                const auto p0 = scenario.player(0, index0), p1 = scenario.player(1, index1);
                memory.push_back({1, ball_cell + 0x58, topology == 1 ? 0u : 1u});
                memory.push_back({1, ball_cell + 0x59, topology == 2 ? 0u : 1u});
                memory.push_back({4, ball_cell + 0, p0});
                memory.push_back({4, ball_cell + 0x2c, p1});
                memory.push_back({1, match + 0x791f, driven() % 4 == 0 ? 0u : 1u});
                const std::uint32_t turn = driven() % 360;
                memory.push_back({2, match + 0x7950, turn});
                const auto opposite = (turn + 180) % 360;
                memory.push_back({2, p0 + 0x19e, topology == 2 ? opposite : driven() % 360});
                memory.push_back({2, p1 + 0x19e, topology == 1 ? opposite : driven() % 360});
                // The attempts' extra outputs (the bytes at +0x42, +0x43, +0x4c of the contest's frame).
                const std::uint32_t outputs[] = {0, 1, 5, 0x20, 0x7f, 0x80, 0xff};
                memory.push_back({1, frame + 0x42, negative ? 0x80u + driven() % 2 * 0x7f : outputs[driven() % 7]});
                memory.push_back({1, frame + 0x43, outputs[driven() % 7]});
                memory.push_back({1, frame + 0x4c, driven() % 2});
                // The halfword flags of the player tables (what 0x10289560 reports) and the strengths compared in the fallback.
                join(memory, flag_table(0, driven() % 3));
                join(memory, flag_table(1, driven() % 3));
                const std::uint32_t floats[] = {0x00000000u, 0x3f800000u, 0x41200000u, 0x461c4000u, 0xc61c4000u, 0x42c80000u};
                for (const auto p : {p0, p1}) memory.push_back({4, p + 0xb0, floats[driven() % 6]});
                const unsigned row = driven() % 9, column = driven() % 12;
                cases.push_back({.entry = 0x101ec890, .registers = {{3, match}, {4, row}, {5, column}}, .memory = std::move(memory), .f1 = {},
                                 .label = "driven outcomes", .answers = {{0x1021c260, first}, {0x1021b860, second}, {0x10221e00, 0}, {0x1021bec0, 0}}});
            }
    }
    // Cells outside the pitch.
    for (const auto [row, column] : {std::pair{9u, 0u}, {0u, 12u}, {0xffu, 3u}, {3u, 0xffu}, {8u, 11u}, {0u, 0u}, {0x80u, 0u}, {0u, 0x80u}})
        add(0x101ec890, {{3, match}, {4, row}, {5, column}}, {}, "cells");
    // Every action kind.
    for (std::uint32_t kind = 0; kind < 0x100; ++kind) add(0x101ee530, {{3, match}, {4, kind}}, {}, "action kinds");
    // The small queries over the players: sides, columns, the pending player and the cell flag table.
    const auto fill_table = [&](std::uint32_t value) {
        std::vector<Write> writes;
        for (std::uint32_t at = match + 0xa60; at < match + 0xa60 + 9 * 0x450; at += 4) writes.push_back({4, at, value});
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

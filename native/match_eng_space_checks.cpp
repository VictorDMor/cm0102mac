// Cases for the space block (target cells, nearest players, line limits) from a real match state.
// A live match reaches most routines thinly or only on their common paths, so the tactics
// modes, the team flags, the cell sentinels and the out-of-range bytes are set by memory overrides.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("space") {
    const auto match = scenario.match;
    const auto sp = std::uint32_t(scenario.s.r<4>(4));
    const auto byte_at = [&](std::uint32_t at) { return std::uint32_t(scenario.s.memory.read<1>(at)); };
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    const auto team = [&](unsigned side) { return match + 0x7c6c + 0x18e0 * side; };

    // 0x101e3060: every ordering of the last touch, the holder and the ball cell.
    for (const std::uint32_t side : {0u, 1u, 2u, 0xffu}) {
        for (const std::uint32_t last : {0u, 1u, 0xffu, 2u}) {
            for (const std::uint32_t holder : {0u, scenario.player(0, 2), scenario.player(1, 2)}) {
                for (const std::uint32_t shot : {0xffu, 0u, 3u}) {
                    for (const auto [row, column] : {std::pair{4u, 5u}, {0u, 0u}, {8u, 11u}, {9u, 12u}, {200u, 3u}, {3u, 10u}, {4u, 11u}, {2u, 0u}, {6u, 1u}}) {
                        add(0x101e3060, {{3, match}, {4, side}},
                            {{1, match + 0x7924, last}, {4, match + 0xdff8, holder}, {1, match + 0x791f, shot},
                             {1, match + 0x791d, row}, {1, match + 0x791e, column}},
                            "side has ball");
                    }
                }
            }
        }
    }

    // 0x101e28b0: the tactics modes, last touch, team flags and ball cells.
    const std::uint32_t flag_sets[] = {0, 0x1000, 0x4000, 0x8000, 0x10000, 0x14000, 0x18000, 0x1c000, 0x5000, 0x1c000 | 0x1000};
    for (const auto [side, index] : {std::pair{0u, 3u}, {1u, 3u}, {0u, 0u}, {1u, 12u}, {0u, 8u}}) {
        const auto player = scenario.player(side, index);
        for (const std::uint32_t mode : {0u, 1u, 0x80u, 0xffu}) {
            for (const std::uint32_t last : {0u, 1u}) {
                for (const std::uint32_t flags : flag_sets) {
                    for (const std::uint32_t held : {0u, 1u}) {
                        add(0x101e28b0, {{3, match}, {4, player}, {5, sp + 0x3b}, {6, sp + 0x3a}, {7, held}},
                            {{1, match + 0x7927, mode}, {1, match + 0x7924, last}, {4, team(0) + 0x588, flags}, {4, team(1) + 0x588, flags},
                             {1, match + 6, held}, {1, match + 7, held ^ 1}},
                            "home cell");
                    }
                }
            }
        }
        // Ball cells, including the ones that clamp and the wide-play moves around the ball.
        for (const auto [row, column] : {std::pair{4u, 5u}, {0u, 0u}, {8u, 11u}, {9u, 12u}, {200u, 3u}, {3u, 255u}, {2u, 1u}, {6u, 10u}, {5u, 0x7fu}, {5u, 0x80u}}) {
            for (const std::uint32_t mode : {0u, 1u}) {
                for (const std::uint32_t last : {0u, 1u}) {
                    for (const std::uint32_t flags : {0u, 0x4000u, 0x14000u, 0x1000u | 0x4000u}) {
                        for (const std::uint32_t held : {0u, 1u}) {
                            add(0x101e28b0, {{3, match}, {4, player}, {5, sp + 0x3b}, {6, sp + 0x3a}, {7, 0}},
                                {{1, match + 0x7927, mode}, {1, match + 0x7924, last}, {1, match + 0x791d, row}, {1, match + 0x791e, column},
                                 {4, team(0) + 0x588, flags}, {4, team(1) + 0x588, flags}, {1, match + 6, held}, {1, match + 7, held}},
                                "ball cell");
                        }
                    }
                }
            }
        }
    }

    // Nearest players and distances: targets on a player's own cell (distance zero) and elsewhere.
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t index : {0u, 4u, 11u, 19u}) {
            const auto player = scenario.player(side, index);
            const auto row = byte_at(player + 0x105), column = byte_at(player + 0x106);
            for (const auto [target_row, target_column] : {std::pair{row, column}, {4u, 5u}, {0u, 0u}, {8u, 11u}}) {
                for (const std::uint32_t any : {0u, 1u}) {
                    for (const std::uint32_t restricted : {0u, 1u, 2u}) {
                        const std::vector<Write> memory = {{1, scenario.player(side, (index + 1) % 20) + 0x2c, restricted},
                                                           {1, scenario.player(side, (index + 2) % 20) + 0x2c, restricted}};
                        add(0x101e32e0, {{3, match}, {4, side}, {5, target_row}, {6, target_column}, {7, any}}, memory, "nearest player");
                        add(0x101e3f10, {{3, match}, {4, side}, {5, target_row}, {6, target_column}, {7, any}}, memory, "nearest distance");
                        add(0x101e4020, {{3, match}, {4, side}, {5, target_row}, {6, target_column}, {7, any}}, memory, "nearby players");
                    }
                    add(0x101e31a0, {{3, match}, {4, player}, {5, target_row}, {6, target_column}}, {}, "is nearest");
                }
            }
            // A neighbour standing on the same cell as the player (ties), and a lone player (no one else counts).
            const auto neighbour = scenario.player(side, (index + 1) % 20);
            add(0x101e31a0, {{3, match}, {4, player}, {5, row}, {6, column}},
                {{1, neighbour + 0x105, row}, {1, neighbour + 0x106, column}, {1, neighbour + 0x2c, 0}}, "tie");
            for (unsigned other = 0; other < 20; ++other)
                if (other != index)
                    add(0x101e31a0, {{3, match}, {4, player}, {5, row}, {6, column}},
                        {{1, scenario.player(side, other) + 0x19, 0xff}}, "alone");
        }
    }

    // Position tables: every position, previous position, and the side bytes.
    for (const std::uint32_t side : {0u, 1u}) {
        const auto player = scenario.player(side, 5);
        for (const std::uint32_t position : {0xffu, 0x80u, 0u, 1u, 5u, 10u, 11u, 0x7fu}) {
            for (const std::uint32_t entry : {0x101e3d50u, 0x101e3de0u, 0x101e3e70u})
                add(entry, {{3, match}, {4, player}}, {{1, player + 0x19, position}, {1, player + 0x1a, position}}, "position table");
            add(0x101e3d50, {{3, match}, {4, player}}, {{1, player + 0x19, position}, {1, player + 0x27, side ^ 1}}, "other side");
        }
    }

    // Line limits: positions that skip players and columns around the starting values.
    for (const std::uint32_t column : {0u, 5u, 6u, 7u, 11u, 0xffu}) {
        for (const std::uint32_t skip : {0xffu, 3u}) {
            std::vector<Write> memory;
            for (unsigned index = 0; index < 20; index += 3) {
                memory.push_back({1, scenario.player(1, index) + 0x106, column});
                memory.push_back({1, scenario.player(0, index) + 0x106, column});
                memory.push_back({1, scenario.player(1, index + 1) + 0x19, skip});
                memory.push_back({1, scenario.player(0, index + 1) + 0x19, skip});
            }
            add(0x101e4130, {{3, match}}, memory, "line limits");
        }
    }
    add(0x101e4130, {{3, match}}, {{1, scenario.player(1, 19) + 0x106, 11}, {1, scenario.player(0, 19) + 0x106, 0}}, "last players");

    // Distance behind the line: both sides, the pending player and columns around the limits.
    for (const std::uint32_t side : {0u, 1u}) {
        const auto player = scenario.player(side, 6);
        add(0x101e4e10, {{3, match}, {4, player}}, {{4, match + 0xe000, player}}, "pending player");
        for (const std::uint32_t column : {0u, 3u, 11u, 0xc8u})
            add(0x101e4e10, {{3, match}, {4, player}}, {{4, match + 0xe000, 0}, {1, player + 0x106, column}, {1, match + 0x7925, 6}, {1, match + 0x7926, 5}},
                "columns");
    }

    // Open lanes: opposing players on the cell, in front of it, beside it, or none at all.
    for (const std::uint32_t side : {0u, 1u, 2u}) {
        for (const std::uint32_t row : {0u, 4u, 8u, 0xffu}) {
            for (const std::uint32_t column : {0u, 2u, 3u, 5u, 8u, 9u, 11u}) {
                const auto opponent = scenario.player((side == 0) ? 1 : 0, 2);
                for (const std::int32_t row_shift : {0, 1, -1, 2}) {
                    for (const std::int32_t column_shift : {0, 1, -1, 3, -3}) {
                        add(0x101e4ed0, {{3, match}, {4, side}, {5, row}, {6, column}},
                            {{1, opponent + 0x19, 5}, {1, opponent + 0x105, row + std::uint32_t(row_shift)},
                             {1, opponent + 0x106, column + std::uint32_t(column_shift)}},
                            "lane");
                    }
                }
            }
        }
    }
}

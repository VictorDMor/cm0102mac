// Cases for the routes block: line walks over the grid, the cells' players, the routes to a
// target, and the ball's landing. A live match reaches these thinly, so the endpoints, flags
// and fields the branches test are set explicitly on the real match objects.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("routes") {
    const auto match = scenario.match;
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label, std::optional<double> f1 = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = f1, .label = std::move(label)});
    };
    const auto cell = [&](int column, int row) { return match + 0xa08 + std::uint32_t(column * 0x450 + row * 0x5c); };
    const auto byte = [&](std::uint32_t at) { return std::uint32_t(scenario.s.memory.read<1>(at)); };
    const auto neg = [](int v) { return std::uint32_t(v); };

    // The cells' players: every cell that holds somebody, and a few that hold nobody.
    for (int column = 0; column < 9; ++column) {
        for (int row = 0; row < 12; ++row) {
            for (const std::uint32_t side : {0u, 1u, 0xffu}) {
                const auto at = cell(column, row);
                const bool holds = side < 2 && byte(at + 0x58 + side) != 0;
                if (!holds && (column + row * 3 + side) % 11) continue;
                for (const auto& [r7, r8, r9, r10] : {std::array<std::uint32_t, 4>{0xffffffff, 0xffffffff, 0xffffffff, 2}, {0, 2, 6, 0}, {5, 1, 2, 1}, {0xfffe, 1, 3, 3}}) {
                    add(0x101f09d0, {{3, match}, {4, side}, {5, at}, {7, r7}, {8, r8}, {9, r9}, {10, r10}}, {}, "occupants", 2.5);
                    add(0x101f0ad0, {{3, match}, {4, side}, {5, at}, {7, r7}, {8, r8}, {9, r9}, {10, r10}}, {}, "obstruction", 0.25);
                }
            }
        }
    }
    // The occupants' states: a player off the pitch (state > 1 or position < 0) is skipped.
    for (int column = 0; column < 9; ++column) {
        for (int row = 0; row < 12; ++row) {
            for (const std::uint32_t side : {0u, 1u}) {
                const auto at = cell(column, row);
                if (!byte(at + 0x58 + side)) continue;
                const auto first = scenario.r32(at + side * 0x2c);
                for (const auto& [state, position] : {std::pair{2u, 5u}, {0u, 0xffu}, {1u, 3u}, {0xffu, 3u}}) {
                    add(0x101f09d0, {{3, match}, {4, side}, {5, at}, {7, 0}, {8, 2}, {9, 6}, {10, 0}}, {{1, first + 0x2c, state}, {1, first + 0x19, position}}, "occupant state");
                    add(0x101f0ad0, {{3, match}, {4, side}, {5, at}, {7, 0}, {8, 2}, {9, 6}, {10, 0}}, {{1, first + 0x2c, state}, {1, first + 0x19, position}}, "obstruction state");
                    add(0x101f0ad0, {{3, match}, {4, side}, {5, at}, {7, 0}, {8, 2}, {9, 6}, {10, 0}}, {{1, at + 0x58 + side, 0}}, "empty");
                }
            }
        }
    }

    // The line walks: horizontal, vertical, the four slanted directions along either side, and endpoints off the pitch.
    const std::array<std::pair<int, int>, 12> ends{{{0, 0}, {4, 6}, {8, 11}, {2, 9}, {7, 1}, {4, 0}, {0, 11}, {8, 0}, {3, 3}, {5, 4}, {-1, 5}, {4, 12}}};
    for (const auto [column_1, row_1] : ends) {
        for (const auto [column_2, row_2] : ends) {
            for (const std::uint32_t entry : {0x101eeeb0u, 0x101ef3c0u})
                add(entry, {{3, match}, {4, neg(column_1)}, {5, neg(row_1)}, {6, neg(column_2)}, {7, neg(row_2)}}, {},
                    "line " + std::to_string(column_1) + "," + std::to_string(row_1) + ">" + std::to_string(column_2) + "," + std::to_string(row_2));
        }
    }
    for (const auto [column_2, row_2] : {std::pair{9, 5}, {4, 13}, {5, -1}, {-3, 2}, {8, 8}, {1, 2}, {6, 7}, {2, 5}}) {
        for (const std::uint32_t entry : {0x101eeeb0u, 0x101ef3c0u}) {
            add(entry, {{3, match}, {4, 3}, {5, 4}, {6, neg(column_2)}, {7, neg(row_2)}}, {}, "line from (3, 4)");
            add(entry, {{3, match}, {4, 0x103}, {5, 0x204}, {6, 0x100 + neg(column_2)}, {7, 0x300 + neg(row_2)}}, {}, "line, high bytes");
        }
    }

    // Straight along a row to columns past the grid (the counting walk stops after column 8, the blocking one after 11).
    for (const std::uint32_t entry : {0x101eeeb0u, 0x101ef3c0u})
        for (const auto [column_1, row_1] : {std::pair{0, 4}, {5, 0}, {3, 7}, {8, 11}})
            for (const int column_2 : {9, 10, 11, 12, 13, 15})
                add(entry, {{3, match}, {4, neg(column_1)}, {5, neg(row_1)}, {6, neg(column_2)}, {7, neg(row_1)}}, {}, "row walk past the grid");

    // 0x101ef8d0: the distance table, spans on and off the grid.
    for (const int d_column : {0, 1, 4, 8, 9, 127, 128, 200}) {
        for (const int d_row : {0, 1, 5, 11, 12, 100, 129, 255}) {
            add(0x101ef8d0, {{3, match}, {4, 1}, {5, 2}, {6, 1 + neg(d_column)}, {7, 2 + neg(d_row)}}, {}, "distance");
            add(0x101ef8d0, {{3, match}, {4, 100}, {5, 100}, {6, 100 - neg(d_column)}, {7, 100 - neg(d_row)}}, {}, "distance, other way");
        }
    }

    // The two routes: from cells all over the pitch, toward either goal line, with every flag byte the
    // second route distinguishes.
    for (int column = 0; column < 9; column += 2) {
        for (int row = 0; row < 12; row += 3) {
            for (const std::uint32_t flag : {0u, 1u, 2u, 0xffu}) {
                add(0x101ef9a0, {{3, match}, {4, neg(column)}, {5, neg(row)}, {6, 7}, {7, flag}, {8, 1}, {9, 3}}, {}, "route to goal");
            }
        }
    }
    // Starts above the first row, and on the last row with nowhere to go (the step then leaves the pitch).
    for (const int row : {-1, -3, -128, 12, 100})
        for (const int column : {0, 4, 8})
            for (const std::uint32_t flag : {0u, 1u}) add(0x101ef9a0, {{3, match}, {4, neg(column)}, {5, neg(row)}, {6, 7}, {7, flag}, {8, 1}, {9, 3}}, {}, "route from off the pitch");
    for (const int row : {11, 0, 5, 12})
        for (const std::uint32_t flag : {6u, 8u, 0x36u, 0x5au})
            add(0x101f0090, {{3, match}, {4, 2}, {5, neg(row)}, {6, 2}, {7, neg(row)}, {8, flag}, {9, 0}, {10, 4}}, {}, "route of no length at the edge");
    add(0x101ef9a0, {{3, match}, {4, 4}, {5, 11}, {6, 7}, {7, 1}, {8, 1}, {9, 3}}, {}, "already at the goal");
    add(0x101ef9a0, {{3, match}, {4, 4}, {5, 0}, {6, 7}, {7, 0}, {8, 1}, {9, 3}}, {}, "already at the goal, row 0");
    for (const std::uint32_t flag : {0u, 6u, 8u, 9u, 0x1eu, 0x1fu, 0x20u, 0x34u, 0x35u, 0x36u, 0x37u, 0x38u, 0x3bu, 0x3cu, 0x3du, 0x3eu, 0x3fu,
                                     0x59u, 0x5au, 0x5bu, 0x5cu, 0x5du, 0x5eu, 0x5fu, 7u, 0xffu, 0x80u}) {
        for (const auto& [column, row, target_column, target_row] :
             {std::array<int, 4>{1, 1, 6, 9}, {6, 9, 1, 1}, {2, 8, 7, 3}, {7, 3, 2, 8}, {4, 4, 4, 9}, {4, 9, 4, 2}, {1, 5, 7, 5}, {7, 5, 1, 5}, {3, 3, 3, 3},
                              {0, 0, 8, 11}, {8, 11, 0, 0}, {-1, 2, 5, 5}, {1, 1, 3, 8}, {8, 1, 3, 9}}) {
            add(0x101f0090, {{3, match}, {4, neg(column)}, {5, neg(row)}, {6, neg(target_column)}, {7, neg(target_row)}, {8, flag}, {9, 0x30}, {10, 4}}, {}, "route between cells");
        }
    }
    add(0x101f0090, {{3, match}, {4, 2}, {5, 2}, {6, 2}, {7, 2}, {8, 6}, {9, 0xffff8000}, {10, 4}}, {}, "route of no length");
    add(0x101f0090, {{3, match}, {4, 2}, {5, 2}, {6, 5}, {7, 7}, {8, 9}, {9, 0x7fff}, {10, 4}}, {}, "chance of 5");
    add(0x101f0090, {{3, match}, {4, 2}, {5, 2}, {6, 5}, {7, 7}, {8, 9}, {9, 0xfffff448}, {10, 4}}, {}, "chance, negative");

    // 0x101f0c00: where the ball is, the play state, the kick-off side and a held player.
    for (const std::uint32_t flag : {0u, 1u, 2u}) {
        for (const std::uint32_t column : {0xffu, 0u, 4u, 8u, 9u, 0x7fu}) {
            for (const std::uint32_t row : {0xffu, 0u, 6u, 11u, 12u, 13u}) {
                add(0x101f0c00, {{3, match}, {4, flag}}, {{1, match + 0x7928, 0}, {1, match + 0x791d, column}, {1, match + 0x791e, row}}, "ball out");
            }
        }
    }
    add(0x101f0c00, {{3, match}, {4, 1}}, {{1, match + 0x7928, 5}}, "play stopped");
    for (const std::uint32_t side : {0u, 1u, 0xffu})
        add(0x101f0c00, {{3, match}, {4, 1}}, {{1, match + 0x7928, 0}, {1, match + 0x7924, side}, {4, match + 0xe000, scenario.player(0, 1)}}, "held player");

    // The lines of players: the recorded extremes and the test at 0x10283690.
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t index : {0u, 4u, 9u, 19u}) {
            const auto player = scenario.player(side, index);
            for (const auto& [low, high] : {std::pair{0u, 0u}, {scenario.player(0, 2), scenario.player(1, 3)}, {scenario.player(1, 5), 0u}})
                add(0x101f0dd0, {{3, match}, {4, player}}, {{4, match + 0xe00c, low}, {4, match + 0xe010, high}}, "within the line");
        }
    }
    for (const std::uint32_t word : {0u, scenario.player(0, 2), scenario.player(1, 7)}) {
        add(0x101f0e80, {{3, match}}, {{4, match + 0xe010, word}}, "highest");
        add(0x101f0ed0, {{3, match}}, {{4, match + 0xe00c, word}}, "lowest");
    }
    // 0x101f0f20: squads with players off the pitch, then a squad in one column.
    add(0x101f0f20, {{3, match}}, {}, "extremes");
    for (const std::uint32_t side : {0u, 1u}) {
        std::vector<Write> memory;
        for (std::uint32_t index = 0; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, index % 3 ? 0xffu : 4u});
        add(0x101f0f20, {{3, match}}, memory, "extremes, some players out");
        std::vector<Write> same;
        for (std::uint32_t index = 0; index < 20; ++index) same.push_back({1, scenario.player(side, index) + 0x106, 5});
        add(0x101f0f20, {{3, match}}, same, "extremes, one column");
        std::vector<Write> none;
        for (std::uint32_t index = 0; index < 20; ++index) none.push_back({1, scenario.player(side, index) + 0x19, 0xff});
        add(0x101f0f20, {{3, match}}, none, "extremes, nobody");
    }

    // 0x101f1100: every side argument, spread, and a goalkeeper on the goal line.
    const auto keeper = scenario.player(0, 0);
    for (const std::uint32_t spread : {0u, 1u, 3u}) {
        for (const std::uint32_t side : {0xffu, 0u, 1u}) {
            for (const std::uint32_t confined : {0u, 1u}) {
                for (const std::uint32_t everyone : {0u, 1u}) {
                    for (const auto [column, row] : {std::pair{4u, 6u}, {1u, 2u}, {7u, 9u}, {4u, 0xffu}, {4u, 12u}, {0u, 0u}, {8u, 11u}}) {
                        // (a confined landing never ends when the target row is off the pitch and the spread is 0)
                        if (confined && !spread && row > 11) continue;
                        add(0x101f1100, {{3, match}, {4, column}, {5, row}, {6, spread}, {7, side}, {8, confined}, {9, everyone}, {10, keeper}}, {}, "scatter");
                        add(0x101f1100, {{3, match}, {4, column}, {5, row}, {6, spread}, {7, side}, {8, confined}, {9, everyone}, {10, 0}}, {}, "scatter, no keeper");
                    }
                }
            }
        }
    }
    for (const std::uint32_t home : {0u, 1u}) {
        for (const std::uint32_t row : {0xffu, 12u}) {
            add(0x101f1100, {{3, match}, {4, 4}, {5, row}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, keeper}}, {{1, keeper + 0x27, home}}, "keeper on the line");
            add(0x101f1100, {{3, match}, {4, 4}, {5, row}, {6, 0}, {7, 1}, {8, 0}, {9, 1}, {10, keeper}}, {{1, keeper + 0x27, home}, {1, match + 0x7928, 3}}, "keeper, play stopped");
        }
    }
    add(0x101f1100, {{3, match}, {4, 4}, {5, 6}, {6, 2}, {7, 0}, {8, 1}, {9, 0}, {10, keeper}}, {{4, match + 0xe000, scenario.player(1, 1)}, {1, match + 0x7924, 1}}, "held player");
}

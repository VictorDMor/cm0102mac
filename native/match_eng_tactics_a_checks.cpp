// Cases for the tactics A block from a real match state. A live match reaches the answers by
// marking target, the pinned and holder shortcuts of the actor comparator, and most mode bits of
// the cell choice only rarely, so memory overrides on the real objects put them in play.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

namespace {
std::vector<Write> joined_writes(std::vector<Write> first, const std::vector<Write>& second) {
    first.insert(first.end(), second.begin(), second.end());
    return first;
}
}

MATCH_ENG_CASES("tactics_a") {
    const auto match = scenario.match;
    const auto toc = std::uint32_t(scenario.s.r<4>(8));
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };

    // 0x101d70d0: every player, then with a ball carrier set to stand beside him and each opposing
    // player (and the carrier) as his marking target.
    for (const unsigned side : {0u, 1u}) {
        for (unsigned index = 0; index < 20; ++index) {
            const auto player = scenario.player(side, index);
            add(0x101d70d0, {{3, match}, {4, player}}, {}, "target");
            if (index % 3) continue;
            const auto holder = scenario.player(1 - side, 5);
            const Write holding{4, match + 0xe000, holder};
            for (const std::uint32_t column : {0u, 5u, 11u}) {
                for (const std::uint32_t carrier_trait : {0u, 1u}) {
                    for (const std::uint32_t rows : {1u, 4u, 7u}) {
                        add(0x101d70d0, {{3, match}, {4, player}},
                            {holding, {1, holder + 0x105, rows}, {1, holder + 0x106, column}, {1, player + 0x105, rows},
                             {1, player + 0x106, column}, {1, holder + 0x10c, carrier_trait}, {1, holder + 0x104, 0}},
                            "carrier beside");
                    }
                    add(0x101d70d0, {{3, match}, {4, player}},
                        {holding, {1, holder + 0x105, 4}, {1, holder + 0x106, column}, {1, player + 0x105, 4}, {1, player + 0x106, column},
                         {1, holder + 0x10c, carrier_trait}, {1, holder + 0x104, 2}, {4, player + 0x1b0, holder}},
                        "carrier marked");
                }
            }
            add(0x101d70d0, {{3, match}, {4, player}}, {holding, {4, player + 0x1b0, holder}}, "target is the carrier");
            add(0x101d70d0, {{3, match}, {4, player}}, {holding, {4, player + 0x1b0, holder}, {1, holder + 0x104, 0}}, "carrier unmarked");
            for (unsigned other = 0; other < 20; other += 1) {
                const auto candidate = scenario.player(1 - side, other);
                add(0x101d70d0, {{3, match}, {4, player}}, {{4, player + 0x1b0, candidate}, {1, candidate + 0x104, 0}}, "target in a cell");
                add(0x101d70d0, {{3, match}, {4, player}},
                    {{4, player + 0x1b0, candidate}, {1, candidate + 0x104, 0}, {4, match + 0xdff8, candidate}}, "target is the second holder");
            }
            // A candidate nobody can reach (0x101ee770 answers 1): every player off the pitch and none of his
            // own side's players counted in the cells, so the marking target is an outright answer.
            if (index % 9 == 0) {
                std::vector<Write> empty;
                for (const unsigned owner : {0u, 1u})
                    for (unsigned other = 0; other < 20; ++other) empty.push_back({1, scenario.player(owner, other) + 0x19, 0xff});
                for (unsigned row = 0; row < 9; ++row)
                    for (unsigned column = 0; column < 12; ++column) empty.push_back({1, match + 0xa60 + row * 0x450 + column * 0x5c + side, 0});
                for (unsigned other = 0; other < 20; ++other) {
                    const auto candidate = scenario.player(1 - side, other);
                    auto memory = empty;
                    memory.push_back({4, player + 0x1b0, candidate});
                    memory.push_back({1, candidate + 0x104, 0});
                    add(0x101d70d0, {{3, match}, {4, player}}, memory, "unreachable target");
                }
            }
            // A candidate whose own match record names him the holder (0x102775c0 answers 1).
            if (index % 9 == 0) {
                for (unsigned other = 0; other < 20; ++other) {
                    const auto candidate = scenario.player(1 - side, other);
                    for (const std::uint32_t shift : {4u, 8u, 0x10u, 0x20u, 0x40u, 0x100u})
                        add(0x101d70d0, {{3, match}, {4, player}},
                            {{4, player + 0x1b0, candidate}, {1, candidate + 0x104, 0}, {4, candidate + 0x1a4, match + shift},
                             {4, match + shift + 0xe000, candidate}},
                            "candidate is the holder of his own record");
                }
            }
            for (const std::uint32_t trait : {0u, 1u, 7u}) {
                add(0x101d70d0, {{3, match}, {4, player}}, {{1, player + 0x10d, trait}}, "trait");
                add(0x101d70d0, {{3, match}, {4, player}}, {{1, player + 0x10d, trait}, {4, match + 0xdff8, scenario.player(1 - side, 2)}},
                    "trait, second holder");
            }
        }
    }

    // 0x101d8260: every mode, the player's kind and side, and the output bytes the caller starts from.
    const auto row_out = match + 0xd100, column_out = match + 0xd101;
    for (const unsigned side : {0u, 1u}) {
        for (const unsigned index : {0u, 3u, 7u, 10u, 15u, 19u}) {
            const auto player = scenario.player(side, index);
            for (std::uint32_t mode = 0; mode < 16; ++mode) {
                for (const std::uint32_t kind : {0u, 1u, 2u}) {
                    for (const auto [row, column] : {std::pair{0xffu, 0xffu}, {3u, 5u}, {4u, 6u}}) {
                        add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                            {{1, player + 0x2d, kind}, {1, row_out, row}, {1, column_out, column}}, "cell choice");
                    }
                }
                add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                    {{1, player + 0x10a, 0}, {1, player + 0x108, 0}}, "no judgment, no rating");
                add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                    {{1, player + 0x10a, 0}, {1, player + 0x108, 3}}, "no judgment");
                add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                    {{1, player + 0x10a, 3}, {1, player + 0x109, 1}}, "judgment, adjusted");
            }
        }
    }

    // The same on every edge and corner of the grid, and where the cells around hold nobody or everybody.
    for (const unsigned side : {0u, 1u}) {
        for (const unsigned index : {2u, 8u}) {
            const auto player = scenario.player(side, index);
            for (const std::uint32_t row : {0u, 1u, 4u, 7u, 8u}) {
                for (const std::uint32_t column : {0u, 1u, 5u, 10u, 11u}) {
                    for (std::uint32_t mode = 0; mode < 16; ++mode) {
                        add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                            {{1, player + 0x105, row}, {1, player + 0x106, column}, {1, row_out, 0xff}, {1, column_out, 0xff}},
                            "cell choice at " + std::to_string(row) + "," + std::to_string(column));
                    }
                }
            }
        }
    }

    // 0x101d91b0: the comparator through two queue slots.
    const auto first_slot = match + 0x79f8, second_slot = match + 0x79fc;
    for (const std::uint32_t state : {0u, 8u}) {
        for (const auto [a, b] : {std::pair{scenario.player(0, 1), scenario.player(1, 4)}, {scenario.player(0, 7), scenario.player(0, 2)}}) {
            for (const std::uint32_t pinned : {0u, a, b, scenario.player(1, 9)}) {
                for (const std::uint32_t carrying : {0u, a, b, scenario.player(1, 9)}) {
                    add(0x101d91b0, {{3, first_slot}, {4, second_slot}},
                        {{4, first_slot, a}, {4, second_slot, b}, {4, toc - 0x3910, match}, {1, match + 0x7928, state},
                         {4, match + 0x7c68, pinned}, {4, match + 0xe000, carrying}},
                        "comparator");
                }
            }
        }
    }

    // The short routines with arguments a live match does not pass.
    for (const std::uint32_t argument : {0u, 1u, 7u, 0xffffffffu}) add(0x101d9510, {{3, match}, {4, argument}}, {}, "all players");
    for (const std::uint32_t side : {0u, 1u, 2u, 0xffu, 0xffffffffu}) {
        add(0x101d7d30, {{3, match}, {4, side}}, {}, "side flag");
        add(0x101d8f20, {{3, match}, {4, side}}, {}, "goal pressure");
        add(0x101d8f20, {{3, match}, {4, side}}, {{1, match + 0xa60 + 0x450 * 3 + 0x5c * 4, 9}, {1, match + 0xa61 + 0x450 * 3 + 0x5c * 8, 9}}, "goal pressure, crowded");
    }
    for (const unsigned side : {0u, 1u}) {
        for (const unsigned index : {0u, 6u, 13u, 19u}) {
            const auto player = scenario.player(side, index);
            const auto row = scenario.s.memory.read<1>(player + 0x105), column = scenario.s.memory.read<1>(player + 0x106);
            for (const std::uint32_t flag : {0u, 1u, 0xffu}) add(0x101d8d70, {{3, match}, {4, player}, {5, flag}}, {}, "lane");
            add(0x101d8120, {{3, match}, {4, player}, {5, std::uint32_t(row)}, {6, std::uint32_t(column)}}, {}, "marker at the player's own cell");
            add(0x101d8120, {{3, match}, {4, player}, {5, std::uint32_t(row)}, {6, std::uint32_t(column) + 1}}, {{1, player + 0x2c, 1}}, "marker, young");
            add(0x101d8120, {{3, match}, {4, 0}, {5, 1}, {6, 2}}, {}, "no player");
            for (const std::uint32_t copy : {0u, match + 0xd200}) {
                add(0x101d7dc0, {{3, match}, {4, player}, {5, match + 0xd1f0}, {6, copy}}, {}, "assess cell");
                add(0x101d7dc0, {{3, match}, {4, player}, {5, match + 0xd1f0}, {6, copy}}, {{2, match + 0xd1f0, 0x7ff0}}, "assess cell, running total");
            }
        }
    }

    // --- Coverage closure.
    const auto record = scenario.r32(match + 0x3118);
    const auto tactical_word = [&](unsigned side) { return match + 0x7c6c + side * 0x18e0 + 0x588; };
    const auto position_values = [&](std::uint32_t value) {
        std::vector<Write> writes;
        for (unsigned side = 0; side < 2; ++side) {
            const auto positions = scenario.r32(match + 0x7934 + side * 4);
            for (unsigned k = 0; k < 11; ++k) writes.push_back({2, positions + 2 * k, value});
        }
        return writes;
    };
    for (const unsigned side : {0u, 1u}) {
        for (const unsigned index : {0u, 3u, 6u, 9u, 12u, 15u, 18u}) {
            const auto player = scenario.player(side, index);
            // Without a team record for either side (the shared-club test of the relation term is skipped).
            for (const auto& [home, away] : {std::pair{0u, 1u}, {1u, 0u}, {0u, 0u}}) {
                std::vector<Write> memory;
                if (home) memory.push_back({4, record + 0x1c, 0});
                if (away) memory.push_back({4, record + 0x20, 0});
                if (!home && !away) { memory.push_back({4, record + 0x1c, 0}); memory.push_back({4, record + 0x20, 0}); }
                add(0x101d70d0, {{3, match}, {4, player}}, memory, "target, a side without a team record");
            }
            // A side flagged 0x400, with and without the player's trait.
            for (const std::uint32_t flags : {0x400u, 0u})
                for (const std::uint32_t trait : {0u, 1u, 7u})
                    add(0x101d70d0, {{3, match}, {4, player}},
                        {{4, tactical_word(0), flags}, {4, tactical_word(1), flags}, {1, player + 0x10d, trait}}, "target, flagged side");
            // The position values the candidates' state tests read.
            for (const std::uint32_t value : {0x20u, 0x820u, 0xa0u, 0x8a0u, 0x140u, 0x240u, 0x440u, 0x40u, 0x1u})
                add(0x101d70d0, {{3, match}, {4, player}}, position_values(value), "target, position values");
            // A marking target who has left the pitch, and a carrier without the trait next to the player.
            const auto candidate = scenario.player(1 - side, 3);
            add(0x101d70d0, {{3, match}, {4, player}}, {{4, player + 0x1b0, candidate}, {1, candidate + 0x104, 0}, {1, candidate + 0x19, 0xff}},
                "target off the pitch");
        }
    }
    // A carrier standing off the grid next to the player: the goal distance falls back to the pool's default figure.
    for (const unsigned side : {0u, 1u}) {
        for (const unsigned index : {0u, 6u, 12u}) {
            const auto player = scenario.player(side, index);
            const auto holder = scenario.player(1 - side, 5);
            for (const auto [row, column] : {std::pair{0x7fu, 5u}, {0x80u, 5u}, {4u, 0x7fu}, {4u, 0x80u}, {0xffu, 0xffu}, {0x30u, 0x30u}})
                for (const std::uint32_t trait : {0u, 1u})
                    add(0x101d70d0, {{3, match}, {4, player}},
                        {{4, match + 0xe000, holder}, {1, holder + 0x105, row}, {1, holder + 0x106, column}, {1, player + 0x105, row},
                         {1, player + 0x106, column}, {1, holder + 0x10c, trait}, {1, holder + 0x104, 0}},
                        "carrier off the grid");
        }
    }
    // The cell choice for a player on a rim row who cannot take a contested cell: tired or restricted position values
    // (0x30 and either 0x80 or 0x800) with the caller's mode asking for the first cell.
    for (const unsigned side : {0u, 1u}) {
        const auto player = scenario.player(side, 2);
        for (const std::uint32_t row : {0u, 1u, 7u, 8u})
            for (const std::uint32_t value : {0xb0u, 0x830u, 0x8b0u})
                for (std::uint32_t mode = 0; mode < 16; ++mode)
                    add(0x101d8260, {{3, match}, {4, player}, {5, row_out}, {6, column_out}, {7, mode}},
                        joined_writes(position_values(value), {{1, player + 0x105, row}, {1, player + 0x106, 5}, {1, row_out, 0xff}, {1, column_out, 0xff}}),
                        "cell choice, restricted position");
    }
}

// Direct calls of the rating block's picking routines and small scans: the entries a live match reaches rarely or never.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("rating_pick") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory = {},
                         std::string label = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    const auto toc_word = [&](std::int32_t offset) { return scenario.r32(scenario.r32(std::uint32_t(scenario.s.r<4>(8)) + std::uint32_t(offset))); };
    // The fixture tests: kind, the flag halfword, and the competition word 0x104646d0 compares.
    const std::uint32_t words[] = {0, 1, 2, 5, 0xffffu, toc_word(-0x6888), toc_word(-0x67bc), toc_word(-0x6734)};
    for (const std::uint32_t entry : {0x101fce60u, 0x101fcf20u, 0x101feae0u})
        for (const std::uint32_t kind : {0x82u, 0x96u, 0x40u}) for (const std::uint32_t pending : {0u, 1u}) for (const std::uint32_t word : words)
            for (const std::uint32_t argument : {0u, 1u, 0x101u}) {
                add(entry, {{3, match}, {4, argument}}, {{2, fixture + 0x32, pending}, {2, fixture + 0x30, kind}, {4, fixture, word}}, "fixture");
                add(entry, {{3, match}, {4, argument}}, {{2, fixture + 0x32, pending}, {2, fixture + 0x30, kind}, {4, fixture, word}, {1, fixture + 0x3a, 9}}, "fixture, byte");
            }
    // 0x101fd000: fixture byte 0x3a (1 and 9 end at once), the club test, the side's tactical word, positions 3 and 4.
    for (const std::uint32_t kind_byte : {1u, 9u, 0u, 5u}) for (const std::uint32_t side : {0u, 1u, 0xffu, 2u})
        for (const std::uint32_t position : {2u, 3u, 4u, 0xffu}) for (const std::uint32_t word : {0u, 0x4000u, 0xffffffffu}) {
            const auto signed_side = std::uint32_t(std::int32_t(std::int8_t(side)));
            std::vector<Write> memory = {{1, fixture + 0x3a, kind_byte}, {1, match + 0x1c4 + signed_side, position},
                                         {4, match + 0x7c6c + 0x18e0 * signed_side + 0x588, word}};
            add(0x101fd000, {{3, match}, {4, side}}, memory, "side flagged");
            memory.push_back({4, fixture + 0x20, 0});
            memory.push_back({4, fixture + 0x24, 0});
            add(0x101fd000, {{3, match}, {4, side}}, memory, "side flagged, no club");
        }
    // 0x101fd100: flag words against flag words, the nearest first (the pairing returns an index), else the best score.
    const std::uint32_t flag_values[] = {0, 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x42, 0x180, 0x8c4, 0xfff, 0x7e};
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 9u}) {
        const auto base = scenario.r32(scenario.player(side, 0) + 0x1a4);
        const auto own_table = scenario.r32(base + 0x7934 + side * 4), other_table = scenario.r32(base + 0x7934 + (1 - side) * 4);
        for (const auto own : flag_values) for (const auto other : flag_values) {
            std::vector<Write> memory;
            for (unsigned k = 0; k < 11; ++k) {
                memory.push_back({2, own_table + 2 * k, own});
                memory.push_back({2, other_table + 2 * k, other});
            }
            add(0x101fd100, {{3, match}, {4, side}, {5, index}}, memory, "pick_opponent");
        }
    }
    for (const unsigned side : {0u, 1u, 0xffu, 2u}) {
        add(0x101fd100, {{3, match}, {4, side}, {5, 3}}, {}, "pick_opponent, plain");
        add(0x101fd100, {{3, match}, {4, side}, {5, 3}}, {{1, scenario.player(side & 1, 3) + 0x19, 0xff}}, "pick_opponent, off pitch");
        add(0x101fd100, {{3, match}, {4, side}, {5, 3}}, {{1, scenario.player(1 - (side & 1), 5) + 0x19, 0xff}}, "pick_opponent, opponent off");
    }
    // The scans over one side's players: the key is a player of the other side.
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 4u, 10u, 19u}) {
        const auto key = scenario.player(side, index);
        for (const unsigned other : {0u, 3u, 7u, 19u}) {
            const auto opponent = scenario.player(1 - side, other);
            for (const std::uint32_t entry : {0x101feba0u, 0x101fec50u}) {
                add(entry, {{3, match}, {4, key}}, {}, "scan");
                add(entry, {{3, match}, {4, key}}, {{4, key + 0x6c, opponent}}, "scan, linked");
                add(entry, {{3, match}, {4, key}}, {{4, key + 0x6c, opponent}, {4, opponent + 0x6c, key}}, "scan, both linked");
            }
        }
        for (const unsigned row : {0u, 1u, 2u, 4u, 6u, 7u, 9u}) for (const unsigned column : {0u, 2u, 3u, 5u, 9u}) {
            std::vector<Write> memory;
            for (unsigned k = 0; k < 20; ++k) {
                const auto player = scenario.player(side, k);
                memory.push_back({1, player + 0x105, row});
                memory.push_back({1, player + 0x106, column});
            }
            add(0x101fed00, {{3, match}, {4, key}}, memory, "eligible");
            memory.push_back({1, scenario.player(side, 5) + 0x19, 0xff});
            add(0x101fed00, {{3, match}, {4, key}}, memory, "eligible, one off");
        }
        add(0x101fed00, {{3, match}, {4, key}}, {}, "eligible, plain");
    }
    // 0x101fea10: the ball cell, either side.
    add(0x101fea10, {{3, match}}, {}, "ball cell");
    for (const std::uint32_t column : {0u, 3u, 5u, 9u, 0xffu}) for (const std::uint32_t row : {0u, 4u, 8u, 0xffu})
        add(0x101fea10, {{3, match}}, {{1, match + 0x791d, column}, {1, match + 0x791e, row}}, "ball cell, moved");
    // 0x101fedd0 and 0x101fee30: either record.
    for (const std::uint32_t entry : {0x101fedd0u, 0x101fee30u}) for (const std::uint32_t second : {0u, 1u, 0x100u, 0xffu})
        add(entry, {{3, match}, {4, second}}, {}, "record");
    // 0x101fee90: frees the tables of the records as they stand, and with only the sentinel or empty slots.
    add(0x101fee90, {}, {}, "free tables");
    // A record with a table of no entries, of one entry with both slots empty, and a record holding the sentinel.
    const auto records = scenario.r32(std::uint32_t(scenario.s.r<4>(8)) - 0x5298), sentinel = scenario.r32(std::uint32_t(scenario.s.r<4>(8)) - 0x529c);
    const auto table = match + 0x9000;
    for (const std::uint32_t count : {0u, 1u, 3u, 0xffffu}) {
        std::vector<Write> memory = {{4, records + 8, table}, {2, records + 0xc, count}};
        for (unsigned k = 0; k < 3; ++k) { memory.push_back({4, table + 0x104 + 0x10c * k, 0}); memory.push_back({4, table + 0x108 + 0x10c * k, 0}); }
        add(0x101fee90, {}, memory, "free tables, a record");
        memory[0].value = sentinel;
        add(0x101fee90, {}, memory, "free tables, sentinel");
    }
}

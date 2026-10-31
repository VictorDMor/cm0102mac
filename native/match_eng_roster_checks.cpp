// Direct calls of the roster block from a real match: the entries live matches reach rarely or never.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("roster") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    (void)fixture;
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory = {},
                         std::string label = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    constexpr std::uint32_t keeper_slots[2] = {0xe01c, 0xe020};
    // A player arriving or coming back, at each period clock, in each state of the match.
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 1u, 7u, 10u, 15u}) {
        const auto player = scenario.player(side, index);
        for (const std::uint32_t carried : {0u, 1u, 0x100u}) for (const std::uint32_t clock : {0x1efu, 0x3deu, 0x483u, 0x528u, 0x100u, 0x7ffu}) {
            add(0x101f1530, {{3, match}, {4, player}, {5, carried}}, {{2, match + 0x794c, clock}, {2, match + 0x7948, clock * 3 + 7}}, "arrive");
            add(0x101f1530, {{3, match}, {4, player}, {5, carried}}, {{2, match + 0x794c, clock}, {1, match, 1}}, "arrive, background");
            add(0x101f1530, {{3, match}, {4, player}, {5, carried}}, {{2, match + 0x794c, clock}, {1, match + 0x1bc, 1}}, "arrive, locked");
            add(0x101f1530, {{3, match}, {4, player}, {5, carried}},
                {{2, match + 0x794c, clock}, {1, match + 0x2f0c, 0}, {1, player + 0x19, 0xfe}}, "arrive from the bench");
        }
        add(0x101f1880, {{3, match}, {4, player}}, {}, "off pitch");
        add(0x101f1880, {{3, match}, {4, player}}, {{1, match + 0x2f08, 0xf0}, {1, match + 0x2f09, 0x7f}}, "off pitch, counts wrap");
        for (const std::uint32_t mode : {0xffu, 0xfdu, 0u, 1u, 0x105u}) {
            add(0x101f1950, {{3, match}, {4, player}, {5, mode}}, {}, "leave");
            add(0x101f1950, {{3, match}, {4, player}, {5, mode}}, {{4, match + keeper_slots[0], player}}, "leave, first keeper");
            add(0x101f1950, {{3, match}, {4, player}, {5, mode}}, {{4, match + keeper_slots[1], player}}, "leave, second keeper");
            add(0x101f1950, {{3, match}, {4, player}, {5, mode}}, {{1, player + 0x19, 0xff}}, "leave, not on pitch");
            add(0x101f1950, {{3, match}, {4, player}, {5, mode}}, {{1, player + 0x104, 2}, {4, player + 0x1b4, scenario.player(1 - side, 2)},
                {4, player + 0x1b8, scenario.player(1 - side, 3)}, {4, player + 0x1b0, scenario.player(1 - side, 1)}}, "leave, marking");
        }
    }
    // Leaving with every other player of the side out of the game: the assertion.
    for (const unsigned side : {0u, 1u}) {
        std::vector<Write> gone;
        for (unsigned index = 0; index < 20; ++index) if (index != 4) gone.push_back({1, scenario.player(side, index) + 0x19, 0xff});
        add(0x101f1950, {{3, match}, {4, scenario.player(side, 4)}, {5, 0xfd}}, gone, "no replacement");
        add(0x101f1950, {{3, match}, {4, scenario.player(side, 0)}, {5, 1}}, {{4, match + keeper_slots[side], scenario.player(side, 0)}}, "keeper, plain");
    }
    // The goalkeeper search.
    for (const std::uint32_t second : {0u, 1u, 0x101u}) {
        add(0x101f1e10, {{3, match}, {4, second}}, {{4, match + keeper_slots[0], 0}, {4, match + keeper_slots[1], 0}}, "no keepers");
        add(0x101f1e10, {{3, match}, {4, second}}, {{4, match + keeper_slots[0], scenario.player(0, 3)}, {4, match + keeper_slots[1], scenario.player(1, 3)}}, "keepers set");
        add(0x101f1e10, {{3, match}, {4, second}}, {{4, match + keeper_slots[0], scenario.player(0, 3)}, {1, scenario.player(0, 3) + 0x19, 0xfe}, {4, match + keeper_slots[1], scenario.player(1, 3)}, {1, scenario.player(1, 3) + 0x19, 0xff}}, "keepers gone");
        for (const unsigned side : {0u, 1u}) {
            std::vector<Write> memory = {{4, match + keeper_slots[0], 0}, {4, match + keeper_slots[1], 0}};
            for (unsigned index = 0; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xfe});
            memory.push_back({1, scenario.player(side, 6) + 0x20, 0xff});
            add(0x101f1e10, {{3, match}, {4, second}}, memory, "all off, one candidate");
            for (unsigned index = 0; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
            add(0x101f1e10, {{3, match}, {4, second}}, memory, "all out");
            std::vector<Write> early = {{4, match + keeper_slots[0], 0}, {4, match + keeper_slots[1], 0}};
            for (unsigned index = 0; index < 4; ++index) early.push_back({1, scenario.player(side, index) + 0x19, 0xff});
            add(0x101f1e10, {{3, match}, {4, second}}, early, "first out");
        }
    }
    // The tie totals; output bytes in a spare corner of the match object.
    for (const std::uint32_t side : {0u, 1u}) for (const std::uint32_t leg : {0u, 1u}) for (const std::uint32_t lead : {0xffu, 0u, 2u, 0xfeu})
        for (const std::uint32_t flags : {0u, 4u, 0x4000u, 0x4004u}) for (const std::uint32_t clock : {0x100u, 0x5dcu})
            add(0x101f21d0, {{3, match}, {4, side}, {5, match + 0x40}, {6, match + 0x41}, {7, match + 0x42}, {8, match + 0x43}, {9, match + 0x44}, {10, leg}},
                {{1, fixture + 0x4b, lead}, {1, fixture + 0x4c, lead}, {2, fixture + 0x38, flags}, {2, match + 0x794c, clock}, {1, match + 0xe03d, 3}, {1, match + 0xe073, 3}}, "tie");
    // Period statistics for each mode and clock.
    for (const std::uint32_t mode : {0x3deu, 0u, 1u, 2u, 0xffffu, 0x7fffu}) for (const std::uint32_t clock : {0x1efu, 0x3deu, 0x5dcu}) {
        add(0x101f2400, {{3, match}, {4, mode}}, {{2, match + 0x794c, clock}}, "statistics");
        add(0x101f2400, {{3, match}, {4, mode}}, {{2, match + 0x794c, clock}, {1, match + 0xe03d, 40}, {1, match + 0xe073, 50}}, "statistics, goals");
        add(0x101f2400, {{3, match}, {4, mode}}, {{2, match + 0x794c, clock}, {2, scenario.player(1, 19) + 0x36, 0x7fff}, {2, scenario.player(0, 2) + 0x36, 0x8000}}, "statistics, ratings");
    }
    // Shares of the counters: all zero, sparse, one-sided.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 0xffu, 0x80u}) {
        add(0x101f3480, {{3, match}, {4, kind}}, {}, "shares");
        std::vector<Write> zero;
        for (const std::uint32_t base : {0x797cu, 0x7990u, 0x79a4u, 0x79b8u, 0x79ccu, 0x79e0u}) for (unsigned k = 0; k < 5; ++k) zero.push_back({4, match + base + k * 4, 0});
        add(0x101f3480, {{3, match}, {4, kind}}, zero, "shares, none");
        auto skewed = zero;
        for (auto& write : skewed) if ((write.address & 4) && write.address % 3 != 0) write.value = 9;
        add(0x101f3480, {{3, match}, {4, kind}}, skewed, "shares, some");
        auto negative = zero;
        for (auto& write : negative) write.value = (write.address & 8) ? 0xfffffff0u : 5;
        add(0x101f3480, {{3, match}, {4, kind}}, negative, "shares, negative");
        auto one_sided = zero;
        one_sided[0].value = 7; one_sided[6].value = 3;
        add(0x101f3480, {{3, match}, {4, kind}}, one_sided, "shares, one-sided");
    }
    // The best player for each kind of action.
    for (const unsigned side : {0u, 1u}) for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0xffu}) {
        for (const std::uint32_t row : {0u, 2u, 4u, 5u, 8u}) for (const std::uint32_t column : {0u, 3u, 6u, 11u}) {
            const std::vector<Write> ball = {{1, match + 0x791d, row}, {1, match + 0x791e, column}};
            add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, 0}}, ball, "pick");
            add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, match + 0x2000}}, ball, "pick, excluded");
        }
        std::vector<Write> none;
        for (unsigned index = 0; index < 20; ++index) none.push_back({1, scenario.player(side, index) + 0x19, 0xff});
        add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, 0}}, none, "pick, nobody");
        add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, match + 0x2000}}, none, "pick, nobody, excluded");
        auto keeper = std::vector<Write>{{4, match + keeper_slots[side], scenario.player(side, 3)}, {1, match + 0x791d, 1}};
        add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, 0}}, keeper, "pick, keeper");
        auto far_pos = std::vector<Write>{{1, scenario.player(side, 5) + 0x19, 12}};
        add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, 0}}, far_pos, "pick, bad position");
        add(0x101f3a30, {{3, match}, {4, side}, {5, kind}, {6, 0}}, {{4, scenario.player(side, 5) + 0x70, 0}}, "pick, no record");
    }
    // Rating adjustments.
    for (const unsigned side : {0u, 1u}) for (const std::uint32_t amount : {0u, 1u, 0xfffbu, 0x7fffu, 0x8000u})
        add(0x101f4860, {{3, match}, {4, side}, {5, amount}}, {}, "ratings");
    // Event counts.
    for (const std::uint32_t side : {0u, 1u, 2u, 0xffu}) for (const std::uint32_t flag : {0u, 1u, 0xffu}) for (const std::uint32_t clock : {0x1efu, 0x3deu, 0x5dcu})
        for (const std::uint32_t column : {0u, 3u, 4u, 7u, 8u, 11u}) for (const std::uint32_t slot : {0u, 0x35u, 0x36u, 0x20u})
            add(0x101f4900, {{3, match}, {4, side}, {5, flag}}, {{2, match + 0x794c, clock}, {1, match + 0x791e, column}, {1, match + 0x7911, slot}}, "event");
}

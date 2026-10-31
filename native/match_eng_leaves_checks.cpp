// Direct calls of the getters, setters and short call-free routines from a real
// match: generated call sites run them in line, past the hook a live check sees.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("leaves") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto call = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::string label = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = {}, .f1 = {}, .label = std::move(label)});
    };
    // Every player routine from players on both sides.
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 5u, 10u, 15u, 19u}) {
        const auto player = scenario.player(side, index);
        for (const std::uint32_t entry : {0x101d3b50u, 0x101d5660u, 0x101d5670u, 0x101d5680u, 0x101d5690u, 0x101d56a0u, 0x101d56b0u, 0x101d56c0u,
                                          0x101d6770u, 0x101d7d90u, 0x101d7da0u, 0x101d7db0u, 0x101d8100u, 0x101d8110u, 0x101d8d60u, 0x101d93a0u,
                                          0x101e3150u, 0x101e3f00u, 0x101e8910u, 0x101eaf50u, 0x101ec7a0u, 0x101ee510u, 0x101f1860u, 0x101f93f0u})
            call(entry, {{3, player}});
        call(0x101d5730, {{3, player}, {4, scenario.player(1 - side, index)}});
        call(0x101d5740, {{3, player}});
        call(0x101d5750, {{3, player}, {4, 7}});
        call(0x101db320, {{3, player}, {4, 1234}});
        call(0x101ee520, {{3, player}, {4, 54}});
        call(0x101f1870, {{3, player}, {4, index}});
        call(0x101f1930, {{3, player}, {4, 4}});
        call(0x101f1940, {{3, player}, {4, 6}});
        call(0x101f21c0, {{3, player}, {4, 3}});
        for (const std::uint32_t amount : {0u, 50u, 200u, 201u, 0xffffff38u, 0xffffff37u, 0x7fffu, 0x8000u}) call(0x101d57d0, {{3, player}, {4, amount}});
        for (const std::uint32_t index_at : {0u, 1u, 2u}) call(0x101f1dc0, {{3, player}, {4, index_at}});
        call(0x101d5700, {{3, player}, {4, scenario.player(1 - side, 1)}});
        cases.push_back({.entry = 0x101d5700, .registers = {{3, player}, {4, 1}}, .memory = {{1, player + 0x104, 3}}, .f1 = {}, .label = "full list"});
        cases.push_back({.entry = 0x101d5700, .registers = {{3, player}, {4, 1}}, .memory = {{1, player + 0x104, 2}}, .f1 = {}, .label = "last slot"});
        cases.push_back({.entry = 0x101e15e0, .registers = {{3, match}, {4, player}}, .memory = {}, .f1 = 0.5, .label = {}});
        cases.push_back({.entry = 0x101e15e0, .registers = {{3, match}, {4, player}}, .memory = {}, .f1 = 1e6, .label = {}});
        cases.push_back({.entry = 0x101e15e0, .registers = {{3, match}, {4, player}}, .memory = {}, .f1 = -1e6, .label = {}});
    }
    // Match routines.
    for (const std::uint32_t entry : {0x101d56d0u, 0x101d56e0u, 0x101d57c0u, 0x101d5870u, 0x101d6930u, 0x101d6980u, 0x101e8900u, 0x101e8920u,
                                      0x101e8930u, 0x101e8940u, 0x101f9410u, 0x101f9420u, 0x101fcde0u, 0x101fce40u, 0x101fea90u})
        call(entry, {{3, match}});
    for (const std::uint32_t value : {0u, 1u, 0xffffffffu, 5u}) {
        for (const std::uint32_t entry : {0x101d3bf0u, 0x101d3c50u, 0x101d66a0u, 0x101d66b0u, 0x101d66c0u, 0x101d66d0u, 0x101d6690u, 0x101f1520u, 0x101f1de0u, 0x101f93d0u})
            call(entry, {{3, match}, {4, value}});
        call(0x101d66f0, {{3, match}, {4, value & 1}});
        call(0x101de040, {{3, match}, {4, value & 1}});
        call(0x101de060, {{3, match}, {4, value & 1}});
        call(0x101de080, {{3, match}, {4, value & 1}});
    }
    for (const std::uint32_t side : {0u, 1u, 0xffffffffu}) {
        call(0x101f14b0, {{3, match}, {4, side}});
        cases.push_back({.entry = 0x101f14b0, .registers = {{3, match}, {4, side}}, .memory = {{1, match + 0x77bc, 5}, {1, match + 0x77bd, 12}}, .f1 = {}, .label = "tallies"});
        for (const std::uint32_t column : {0u, 3u, 9u, 0xfffffffeu}) call(0x101e4ea0, {{3, match}, {4, side}, {5, column}});
        for (const std::uint32_t index : {0u, 7u}) {
            call(0x101e3180, {{3, match}, {4, side & 1}, {5, index}});
            call(0x101f9440, {{3, match}, {4, side & 1}, {5, index}});
        }
    }
    // Pending restart states.
    const auto player = scenario.player(0, 3);
    cases.push_back({.entry = 0x101d6980, .registers = {{3, match}}, .memory = {{1, match + 0x30dc, 1}, {4, match + 0x30e4, player}}, .f1 = {}, .label = "pending"});
    cases.push_back({.entry = 0x101d6980, .registers = {{3, match}}, .memory = {{1, match + 0x30dc, 1}, {4, match + 0x30e4, 0}}, .f1 = {}, .label = "no player"});
    cases.push_back({.entry = 0x101d6980, .registers = {{3, match}}, .memory = {{1, match + 0x30dc, 0}}, .f1 = {}, .label = "inactive"});
    for (const std::uint32_t held : {0u, player}) {
        const auto sp = std::uint32_t(scenario.s.r<4>(4));
        cases.push_back({.entry = 0x101d68f0, .registers = {{3, match}, {4, 11}, {5, player}, {6, scenario.player(1, 2)}, {7, 1}, {8, 1}, {9, 4}, {10, 6}},
                         .memory = {{4, match + 0x30e4, held}, {1, sp + 0x3b, 9}, {1, sp + 0x3f, 2}}, .f1 = {}, .label = "request"});
    }
    // Period clocks.
    for (const std::uint32_t clock : {0u, 0x1efu, 0x3deu, 500u}) for (const std::uint32_t flags : {0u, 0x80u})
        cases.push_back({.entry = 0x101fcde0, .registers = {{3, match}}, .memory = {{2, match + 0x794c, clock}, {2, fixture + 0x38, flags}}, .f1 = {}, .label = "scheduled"});
    for (const std::uint32_t quarter : {0xffu, 0u, 1u, 3u}) for (const std::uint32_t clock : {0u, 11u, 33u, 90u})
        cases.push_back({.entry = 0x101fea90, .registers = {{3, match}}, .memory = {{1, match + 0xe0b8, quarter}, {2, match + 0x7948, clock}}, .f1 = {}, .label = "time"});
    cases.push_back({.entry = 0x101fce40, .registers = {{3, match}}, .memory = {{4, fixture, 0xfffffffe}}, .f1 = {}, .label = "statistics fixture"});
    // Pure argument tests.
    for (const std::int32_t a : {-1, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12}) {
        call(0x101ee9c0, {{3, match}, {4, std::uint32_t(a)}});
        for (const std::uint32_t side : {0u, 1u, 2u}) call(0x101ee9f0, {{3, match}, {4, side}, {5, std::uint32_t(a)}});
        for (const std::uint32_t side : {0u, 1u}) for (const std::int32_t b : {-1, 0, 1, 2, 9, 10, 11, 12})
            call(0x101eea30, {{3, match}, {4, side}, {5, std::uint32_t(a)}, {6, std::uint32_t(b)}});
    }
    for (const std::uint32_t to_column : {4u, 5u, 3u}) for (const std::uint32_t to_row : {6u, 7u, 5u})
        call(0x101ee610, {{3, match}, {4, 4}, {5, 6}, {6, to_column}, {7, to_row}});
    // Generic records: a player, its match and the fixture stand in for the objects
    // these one-line readers are called on; the bytes are what is compared.
    const auto first = scenario.player(0, 0);
    for (const std::uint32_t object : {first, match, fixture}) {
        for (const std::uint32_t entry : {0x101d3ac0u, 0x101d3b40u, 0x101d56f0u, 0x101da6d0u, 0x101da750u, 0x101db330u, 0x101e5070u, 0x101e5080u,
                                          0x101e88f0u, 0x101f9400u, 0x101f9430u, 0x101fb570u, 0x101fdb20u})
            call(entry, {{3, object}});
    }
    cases.push_back({.entry = 0x101e5080, .registers = {{3, fixture}}, .memory = {{4, fixture + 0x30, 0}}, .f1 = {}, .label = "no text"});
    cases.push_back({.entry = 0x101fdb50, .registers = {{3, fixture}}, .memory = {{4, fixture + 4, 0}}, .f1 = {}, .label = "no list"});
    cases.push_back({.entry = 0x101fdb50, .registers = {{3, fixture}}, .memory = {{4, fixture + 4, match}}, .f1 = {}, .label = "list"});
    for (const std::uint32_t year : {0u, 999u, 1000u, 2001u, 0xfc18u, 0x8000u, 0x7fffu})
        cases.push_back({.entry = 0x101fdb20, .registers = {{3, fixture}}, .memory = {{2, fixture + 8, year}}, .f1 = {}, .label = "thousands"});
    for (const std::uint32_t object : {first, fixture}) {
        call(0x101d3c40, {{3, object}, {4, 3}});
        call(0x101d66e0, {{3, object}});
        call(0x101f0bf0, {{3, object}});
        call(0x101f9460, {{3, object}});
    }
    const auto spare = scenario.player(1, 19);
    call(0x101fe9f0, {{3, spare + 0x60}, {4, first + 0x60}});
    call(0x101fe9f0, {{3, first + 0x62}, {4, first + 0x60}}, "overlap");
    // The rates: both sides, zero divisors and the fallback.
    const auto rates = spare + 0x80;
    for (const std::uint32_t second : {0u, 1u}) for (const std::uint32_t divisor : {0u, 0x40000000u})
        cases.push_back({.entry = 0x101d3ad0, .registers = {{3, rates}, {4, second}},
                         .memory = {{4, rates + 0x18, divisor}, {4, rates + 0x1c, 0}, {4, rates + 0x28, divisor}, {4, rates + 0x2c, 0},
                                    {4, rates + 0x10, 0x40080000}, {4, rates + 0x14, 0}, {4, rates + 0x20, 0x40140000}, {4, rates + 0x24, 0}},
                         .f1 = {}, .label = "rate"});
    // A competition record: below, inside and above the last 16 ids, with and without +0x6b.
    const auto limit = scenario.r32(scenario.r32(std::uint32_t(scenario.s.r<4>(8)) - 0x7be4));
    call(0x101da6f0, {{3, match}, {4, 0}}, "null");
    for (const std::uint32_t id : {limit - 0x11, limit - 0x10, limit - 1, limit}) for (const std::uint32_t flag : {0u, 1u})
        cases.push_back({.entry = 0x101da6f0, .registers = {{3, match}, {4, spare}}, .memory = {{4, spare, id}, {1, spare + 0x6b, flag}}, .f1 = {}, .label = "record"});
    call(0x101d3c80, {{3, match}});
    cases.push_back({.entry = 0x101d3c80, .registers = {}, .memory = {}, .f1 = -2.5, .label = {}});
    call(0x101e50b0, {});
}

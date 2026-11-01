// Direct calls of the side tendency (0x101fc670) over fixture kinds, team strengths, weights and flags, and of the two
// person rating routines (0x101f9470, 0x101fb580) in the shapes of their callers at 0x101f8bd4 and 0x101f935c.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("rating") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto add = [&](unsigned side, std::uint32_t rate_flag, std::uint32_t flag, double weight, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = 0x101fc670, .registers = {{3, match}, {4, side}, {6, rate_flag}, {7, flag}}, .memory = std::move(memory),
                         .f1 = weight, .label = std::move(label)});
    };
    // 0x101f9470(player, person, club, opponent, fixture, weight, flag, 1) over players, teams, tiers and fixture ids.
    const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 3u, 7u, 10u, 15u, 19u}) {
        const auto player = scenario.player(side, index);
        const auto person = scenario.r32(player + 0x6c);
        if (!person) continue;
        const auto own = side ? away : home, opposing = side ? home : away;
        const auto rate = [&](std::uint32_t output, double weight, std::uint32_t flag, std::uint32_t second, std::vector<Write> memory, std::string label) {
            cases.push_back({.entry = 0x101f9470, .registers = {{3, output}, {4, person}, {5, own}, {6, opposing}, {7, fixture}, {9, flag}, {10, second}},
                             .memory = std::move(memory), .f1 = weight, .label = std::move(label)});
        };
        for (const double weight : {-5.0, 0.0, 3.0, 10.0, 20.0}) for (const std::uint32_t flag : {0u, 1u}) {
            rate(player, weight, flag, 1, {}, "ratings");
            rate(0, weight, flag, 1, {}, "ratings, no output");
            rate(player, weight, flag, 0, {}, "ratings, second flag clear");
            rate(player, weight, flag, 1, {{4, fixture, 0xfffffffe}}, "ratings, id -2");
            rate(player, weight, flag, 1, {{4, own, 0x7fffffff}, {1, fixture + 0x3a, 10}}, "ratings, national team");
            rate(player, weight, flag, 1, {{4, opposing, 0x7fffffff}, {1, fixture + 0x3a, 10}}, "ratings, national opponent");
            for (const std::uint32_t code : {0x78u, 0x82u, 0x87u, 0x96u, 0x9bu, 0x8cu})
                rate(player, weight, flag, 1, {{2, fixture + 0x30, code}}, "ratings, cup code");
            for (const std::uint32_t level : {0u, 1u, 11u, 15u, 20u}) rate(player, weight, flag, 1, {{1, player + 0x2d, level}}, "ratings, level byte");
        }
        for (const std::uint32_t nibble : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 15u})
            rate(player, 3.0, 0, 1, {{1, person + 0x60, nibble}}, "ratings, contract kind");
    }
    // 0x101fb580(side record, club, opponent, fixture, weight, flag, 1).
    for (const unsigned side : {0u, 1u}) {
        const auto record = match + 0x7aac + side * 0xd8;
        const auto own = side ? away : home, opposing = side ? home : away;
        const auto rate = [&](double weight, std::uint32_t flag, std::uint32_t second, std::vector<Write> memory, std::string label) {
            cases.push_back({.entry = 0x101fb580, .registers = {{3, record}, {4, own}, {5, opposing}, {6, fixture}, {8, flag}, {9, second}},
                             .memory = std::move(memory), .f1 = weight, .label = std::move(label)});
        };
        for (const double weight : {-5.0, 0.0, 3.0, 10.0, 20.0}) for (const std::uint32_t flag : {0u, 1u}) {
            rate(weight, flag, 1, {}, "side ratings");
            rate(weight, flag, 0, {}, "side ratings, second clear");
            rate(weight, flag, 1, {{4, record + 0x20, 0}}, "side ratings, no person");
            rate(weight, flag, 1, {{4, fixture, 0xfffffffe}}, "side ratings, id -2");
            rate(weight, flag, 1, {{4, own, 0x7fffffff}, {1, fixture + 0x3a, 10}}, "side ratings, national");
            for (const std::uint32_t code : {0x41fu, 0x423u, 0x426u})
                rate(weight, flag, 1, {{2, fixture + 0x32, code}}, "side ratings, league span");
            for (const std::uint32_t code : {0x78u, 0x82u, 0x96u, 0x9bu})
                rate(weight, flag, 1, {{2, fixture + 0x30, code}}, "side ratings, cup span");
        }
    }
    const std::uint32_t strengths[] = {0, 0x100, 0x800, 0x157d, 0x2000, 0x2710, 0x7fff};
    const std::uint32_t codes[][2] = {{0, 0}, {0x41f, 0x8c}, {0x426, 0x14}, {0x500, 0x78}, {0x43d, 0x82}, {0, 0x96}, {0, 0x9b}, {0, 0x77}};
    const unsigned sides[] = {0, 1};
    for (const unsigned side : sides) for (const double weight : {-3.0, -0.2, 0.0, 0.4, 1.5, 8.0}) for (const std::uint32_t flag : {0u, 1u})
        for (const std::uint32_t kind : {10u, 8u, 3u}) for (const auto& code : codes) for (const std::uint32_t strength : strengths) {
            const auto team = scenario.r32(fixture + (side ? 0x20 : 0x1c)), other = scenario.r32(fixture + (side ? 0x1c : 0x20));
            std::vector<Write> memory = {{1, fixture + 0x3a, kind}, {2, fixture + 0x32, code[0]}, {2, fixture + 0x30, code[1]},
                                         {2, team + 0x1ec, strength}, {2, other + 0x1ec, strength ^ 0x300}};
            add(side, side, flag, weight, memory, "tendency");
            memory.push_back({4, fixture, 0xfffffffe});
            add(side, 1 - side, flag, weight, memory, "tendency, id -2");
            memory.pop_back();
            memory.push_back({1, fixture + 0x40, 1});
            add(side, 0, flag, weight, memory, "tendency, weighted level");
            memory.pop_back();
            memory.push_back({4, team, 0x7fffffff});
            add(side, side, flag, weight, memory, "tendency, national team");
        }
}

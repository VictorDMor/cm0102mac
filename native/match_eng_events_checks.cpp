// Cases for the events block: whole simulated matches over fixture configurations (result carried over from a
// first leg, the flag halfword that picks extra time, golden goal and shoot-out, the cup reference, the
// "no ratings" marker) and random-table seeds, and the six role-flag adjusters over every flag bit.
#include "guest_abi.hpp"
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("events") {
    const auto match = scenario.match;
    const auto toc = [&](std::int32_t offset) { return scenario.r32(scenario.s.r<4>(cm::aot::guest::reg::gpr(2)) + std::uint32_t(offset)); };
    const auto fixture = scenario.r32(match + 0x3118);
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    if (fixture) {
        const auto seed_at = toc(-0x7b90);
        const auto team_one = scenario.r32(fixture + 0x1c), team_two = scenario.r32(fixture + 0x20);
        const std::uint32_t flag_values[] = {0, 0x80, 1, 2, 4, 0x40, 0x4000, 0x4041, 0x4001, 0x41, 0x84, 0xc0, 0x4004, 0x44};
        struct Legs { std::uint32_t first, second; };
        const Legs legs[] = {{0xff, 0xff}, {0, 0}, {1, 0}, {0, 2}, {2, 2}, {1, 1}};
        unsigned seed = 0;
        for (const auto flags : flag_values) {
            for (const auto& leg : legs) {
                for (const bool unrated : {false, true}) {
                    std::vector<Write> memory = {{2, fixture + 0x38, flags}, {1, fixture + 0x4b, leg.first}, {1, fixture + 0x4c, leg.second},
                                                 {4, seed_at, 7919u * ++seed}};
                    if (unrated) memory.push_back({4, fixture, 0xfffffffe});
                    add(0x101db340, {{3, match}}, memory, "match");
                }
            }
        }
        // A cup tie between teams of different ratings: the weaker side may score an extra goal.
        if (team_one && team_two) {
            const auto reference = scenario.r32(team_one + 0x30);
            for (const std::uint32_t one : {1000u, 2000u, 3000u}) {
                for (const std::uint32_t two : {1000u, 1100u, 3000u, 4000u}) {
                    for (const std::uint32_t flags : {0u, 0x80u, 0x4041u}) {
                        std::vector<Write> memory = {{4, fixture + 0x14, reference}, {2, team_one + 0x1ec, one}, {2, team_two + 0x1ec, two},
                                                     {2, fixture + 0x38, flags}, {4, seed_at, 104729u * ++seed}};
                        add(0x101db340, {{3, match}}, memory, "cup tie");
                        memory.push_back({1, fixture + 0x4b, 0xff});
                        add(0x101db340, {{3, match}}, memory, "cup tie, no first leg");
                    }
                }
            }
            add(0x101db340, {{3, match}}, {{4, fixture + 0x14, 0}, {4, seed_at, 31u}}, "no reference");
            add(0x101db340, {{3, match}}, {{4, fixture + 0x1c, 0}, {4, seed_at, 37u}}, "no first team");
        }
        // Few players left: one side already dismissed four, then more.
        for (const std::uint32_t booked : {4u, 5u, 0u}) {
            for (const unsigned side : {0u, 1u}) {
                for (unsigned more = 0; more < 4; ++more) {
                    const std::uint32_t bytes = match + 0x10000 + side * 0x36;
                    std::vector<Write> memory = {{1, bytes - 0x1fb5, booked}, {1, bytes - 0x1fb4, 1}, {4, seed_at, 12007u * ++seed + more}};
                    add(0x101db340, {{3, match}}, memory, "dismissals");
                }
            }
        }
        // Low-scoring matches (weak players), which end level: extra time, golden goal, the shoot-out and the
        // cup upset, over several seeds and leg results.
        constexpr std::uint32_t byte_offsets[] = {0x1b, 0x1e, 0x21, 0x26, 0x2b, 0x2e, 0x2f, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37, 0x39, 0x3c, 0x3e, 0x3f, 0x40, 0x43, 0x44};
        const auto attributes = [&](std::uint32_t value, std::uint32_t stat) {
            std::vector<Write> writes;
            for (unsigned side = 0; side < 2; ++side) {
                for (unsigned index = 0; index < 20; ++index) {
                    const auto record = scenario.r32(scenario.player(side, index) + 0x70);
                    if (!record) continue;
                    for (const auto offset : byte_offsets) writes.push_back({1, record + offset, value});
                    writes.push_back({2, record + 4, stat});
                }
            }
            return writes;
        };
        const std::uint32_t weak_flags[] = {0, 1, 2, 0x82, 0x80, 0x42, 0x44, 0x41, 0x4001, 0x4004, 0x84, 0x22, 0x20, 0xc4, 0x46};
        for (const auto flags : weak_flags) {
            for (const auto& leg : legs) {
                for (const std::uint32_t value : {0u, 6u}) {
                    for (unsigned repeat = 0; repeat < 3; ++repeat) {
                        auto memory = attributes(value, 0x10);
                        memory.push_back({2, fixture + 0x38, flags});
                        memory.push_back({1, fixture + 0x4b, leg.first});
                        memory.push_back({1, fixture + 0x4c, leg.second});
                        memory.push_back({4, seed_at, 6101u * ++seed});
                        if (team_one && team_two) {
                            memory.push_back({4, fixture + 0x14, scenario.r32(team_one + 0x30)});
                            memory.push_back({2, team_one + 0x1ec, repeat == 0 ? 3000u : 1000u});
                            memory.push_back({2, team_two + 0x1ec, repeat == 2 ? 1000u : 3000u});
                        }
                        add(0x101db340, {{3, match}}, memory, "weak match");
                    }
                }
            }
        }
        // Players off the pitch (position -1): the searches for a player run out, ending the match early or late.
        for (const unsigned side : {0u, 1u}) {
            for (const unsigned kept : {0u, 1u, 2u}) {
                for (const std::uint32_t flags : {0u, 2u, 0x80u, 0x44u}) {
                    for (unsigned repeat = 0; repeat < 4; ++repeat) {
                        std::vector<Write> memory = {{2, fixture + 0x38, flags}, {4, seed_at, 9157u * ++seed}};
                        for (unsigned index = kept; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
                        if (repeat & 1) for (unsigned index = 0; index < 20; ++index) memory.push_back({1, scenario.player(1 - side, index) + 0x19, index < 12 ? 0x2u : 0xffu});
                        add(0x101db340, {{3, match}}, memory, "off the pitch");
                    }
                }
            }
        }
        // Many dismissals already, in matches that run into extra time.
        for (const std::uint32_t flags : {2u, 0x82u, 0x80u, 0x42u}) {
            for (const unsigned side : {0u, 1u}) {
                for (unsigned repeat = 0; repeat < 12; ++repeat) {
                    const std::uint32_t bytes = match + 0x10000 + side * 0x36;
                    auto memory = attributes(repeat & 1 ? 0u : 6u, 0x10);
                    memory.push_back({2, fixture + 0x38, flags});
                    memory.push_back({1, bytes - 0x1fb5, 4});
                    memory.push_back({1, bytes - 0x1fb4, 1});
                    memory.push_back({4, seed_at, 4451u * ++seed});
                    add(0x101db340, {{3, match}}, memory, "dismissals in extra time");
                }
            }
        }
        // A match already decided (the 0x1c1 flag) and one with a stored result.
        add(0x101db340, {{3, match}}, {{1, match + 0x1c1, 1}}, "flag 0x1c1");
        add(0x101db340, {{3, match}}, {{1, match + 0x1c1, 1}, {1, fixture + 0x4b, 0xff}}, "flag 0x1c1, no first leg");

        // Combinations the lists above do not cross, drawn from fixed lists (the same on every run): the tie rules
        // (all flag bits, with and without the 0x4000 aggregate rule), the first-leg results (a difference of three
        // makes an abandoned match level on aggregate), dismissals that abandon the match, squads off the pitch, a
        // weaker or stronger side of the cup reference, a null word at the shoot-out check's pointer, an unrated
        // fixture and low ratings (the rebuilt rating is clamped).
        unsigned pick_state = 977;
        const auto below = [&](unsigned limit) {
            pick_state = pick_state * 1103515245u + 12345u;
            return (pick_state >> 16) % limit;
        };
        const std::uint32_t all_flags[] = {0, 1, 2, 3, 4, 0x20, 0x21, 0x22, 0x23, 0x40, 0x41, 0x42, 0x43, 0x80, 0x81, 0x82, 0x83, 0x4000, 0x4001,
                                           0x4002, 0x4003, 0x4004, 0x4020, 0x4022, 0x4041, 0x44, 0x84, 0xc3, 0x4023, 0x4083};
        const Legs sampled_legs[] = {{0xff, 0xff}, {0xff, 0xff}, {0, 0}, {1, 0}, {0, 1}, {0, 2}, {2, 2}, {1, 1}, {3, 0}, {0, 3}, {4, 1}, {1, 4}, {2, 0}};
        for (unsigned n = 0; n < 700; ++n) {
            const auto flags = all_flags[below(std::size(all_flags))];
            const auto& leg = sampled_legs[below(std::size(sampled_legs))];
            std::vector<Write> memory;
            if (below(4) != 0) {
                const std::uint32_t levels[] = {0, 0, 2, 6, 10};
                memory = attributes(levels[below(std::size(levels))], 0x10);
            }
            if (below(2) == 0) memory.push_back({1, scenario.player(0, 0) + 0x19, below(8) == 0 ? 0xffu : 0u});
            memory.push_back({2, fixture + 0x38, flags});
            memory.push_back({1, fixture + 0x4b, leg.first});
            memory.push_back({1, fixture + 0x4c, leg.second});
            if (below(3) == 0) memory.push_back({1, fixture + 0x47, 0xff});
            memory.push_back({4, seed_at, 3571u * ++seed + n});
            if (team_one && team_two) {
                if (below(2) == 0) memory.push_back({4, fixture + 0x14, scenario.r32(team_one + 0x30)});
                const std::uint32_t ratings[] = {1000, 1100, 2000, 3000, 4000};
                memory.push_back({2, team_one + 0x1ec, ratings[below(std::size(ratings))]});
                memory.push_back({2, team_two + 0x1ec, ratings[below(std::size(ratings))]});
            }
            if (below(6) == 0) memory.push_back({4, toc(-0x7b54), 0});
            if (below(8) == 0) memory.push_back({4, fixture, 0xfffffffe});
            if (below(3) == 0) {
                const unsigned side = below(2);
                const std::uint32_t bytes = match + 0x10000 + side * 0x36;
                memory.push_back({1, bytes - 0x1fb5, below(2) ? 4u : 3u});
                memory.push_back({1, bytes - 0x1fb4, 1});
            }
            if (below(6) == 0) {
                const unsigned side = below(2), kept = below(3);
                for (unsigned index = kept; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
            }
            if (below(4) == 0) {
                const std::uint32_t ratings_low[] = {0x100, 0x600, 0xd00, 0xf000, 0x7000, 0x2f00};
                for (unsigned side = 0; side < 2; ++side)
                    for (unsigned index = 0; index < 20; ++index) memory.push_back({2, scenario.player(side, index) + 0x36, ratings_low[below(std::size(ratings_low))]});
            }
            add(0x101db340, {{3, match}}, memory, "sampled match");
        }
        // A side with a single outfield player left on the pitch (a lone goalkeeper ends the match at once): the draws for a player run out now and then, and the match
        // ends on the spot. Over many seeds this happens in extra time too, with and without a first leg.
        for (const unsigned side : {0u, 1u}) {
            for (const std::uint32_t flags : {2u, 0x82u, 0x80u, 0x22u}) {
                for (const auto& leg : {Legs{0xff, 0xff}, Legs{0, 0}, Legs{1, 1}, Legs{3, 0}}) {
                    for (unsigned repeat = 0; repeat < 14; ++repeat) {
                        auto memory = attributes(0, 0x10);
                        for (unsigned index = 0; index < 20; ++index)
                            if (index != 5) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
                        memory.push_back({2, fixture + 0x38, flags});
                        memory.push_back({1, fixture + 0x4b, leg.first});
                        memory.push_back({1, fixture + 0x4c, leg.second});
                        memory.push_back({4, seed_at, 2053u * ++seed});
                        add(0x101db340, {{3, match}}, memory, "one player left");
                    }
                }
            }
        }
        // Level on aggregate after 90 minutes under the 0x4000 rule: the first leg's home goals against the away goals.
        for (const std::uint32_t flags : {0x4000u, 0x4001u, 0x4020u, 0x4021u, 0x4040u}) {
            for (const auto& leg : {Legs{2, 2}, Legs{1, 1}, Legs{3, 3}}) {
                for (unsigned repeat = 0; repeat < 6; ++repeat) {
                    auto memory = attributes(0, 0x10);
                    memory.push_back({2, fixture + 0x38, flags});
                    memory.push_back({1, fixture + 0x4b, leg.first});
                    memory.push_back({1, fixture + 0x4c, leg.second});
                    memory.push_back({4, seed_at, 3011u * ++seed});
                    add(0x101db340, {{3, match}}, memory, "aggregate rule");
                }
            }
        }
        // One side's players far above the other's in every attribute, both ways round, so the tendency pairs hit
        // their clamps from either side.
        for (const unsigned strong : {0u, 1u}) {
            for (const std::uint32_t weak_level : {0u, 2u}) {
                for (unsigned repeat = 0; repeat < 6; ++repeat) {
                    std::vector<Write> memory;
                    for (unsigned side = 0; side < 2; ++side) {
                        for (unsigned index = 0; index < 20; ++index) {
                            const auto record = scenario.r32(scenario.player(side, index) + 0x70);
                            if (!record) continue;
                            for (const auto offset : byte_offsets) memory.push_back({1, record + offset, side == strong ? 20u : weak_level});
                        }
                    }
                    memory.push_back({4, seed_at, 4211u * ++seed});
                    add(0x101db340, {{3, match}}, memory, "lopsided match");
                }
            }
        }
    }

    // The role-flag adjusters: every flag bit, both sides, a few amounts.
    constexpr std::uint32_t adjusters[] = {0x101de0a0, 0x101de340, 0x101de610, 0x101de8b0, 0x101deb50, 0x101dee30};
    for (const auto entry : adjusters) {
        for (const unsigned side : {0u, 1u}) {
            const auto table = scenario.r32(match + 0x7934 + side * 4);
            if (!table) continue;
            for (const std::uint32_t flags : {0u, 1u, 2u, 4u, 8u, 0x10u, 0x20u, 0x40u, 0x80u, 0x3u, 0x41u, 0x6u, 0x18u, 0x8000u}) {
                std::vector<Write> memory;
                for (std::uint32_t position = 0; position <= 10; ++position) memory.push_back({2, table + position * 2, flags});
                for (const std::uint32_t amount : {0u, 1u, 0x1cu, 0xfffffffau, 0x28u, 0xffffffecu, 0x7fffu, 0x8000u, 0xe1u}) {
                    add(entry, {{3, match}, {4, side}, {5, amount}}, memory, "adjust");
                }
            }
        }
    }
}

// Cases for the flow block (period ends, line-up check, shoot-out) from a real match state.
// A live match reaches only the clock checks of 0x101d95a0 and 0x101da5a0, so the periods,
// the tie rules and the shoot-out are set up by memory overrides on the real objects.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("flow") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto toc = [&](std::int32_t offset) { return scenario.r32(std::uint32_t(scenario.s.r<4>(8)) + std::uint32_t(offset)); };
    const auto seed_at = toc(-0x7b90);
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label),
                         .answers = std::move(answers)});
    };

    // 0x101d95a0: each period mark, with the clock before, at and past it, and the tie rules of the fixture.
    for (const std::uint32_t mark : {0u, 1u, 2u, 0x1efu, 0x3deu, 0x483u, 0x528u, 0x600u, 0xffffu}) {
        for (const std::uint32_t clock : {mark, mark + 1, mark + 5}) {
            add(0x101d95a0, {{3, match}}, {{2, match + 0x794c, mark}, {2, match + 0x7948, clock}, {1, match + 0x792d, 2}}, "period clock");
        }
    }
    // The pending player (+0xe000) decides whether 0x1022e710 runs, and in what order with 0x101f2400.
    for (const std::uint32_t mark : {0x1efu, 0x3deu, 0x483u})
        for (const std::uint32_t flags : {0u, 2u, 0x4002u, 6u})
            for (const auto [first_a, first_b] : {std::pair{0xffu, 0xffu}, {0u, 0u}, {1u, 0u}})
                for (const std::uint32_t extra_flag : {0u, 1u})
                    add(0x101d95a0, {{3, match}},
                        {{4, match + 0xe000, scenario.player(0, 3)}, {2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0},
                         {2, fixture + 0x38, flags}, {1, fixture + 0x4b, first_a}, {1, fixture + 0x4c, first_b}, {1, match + 0xe03d, 1},
                         {1, match + 0xe073, 1}, {1, match + 0x1c1, extra_flag}},
                        "pending player");
    add(0x101d95a0, {{3, match}}, {{1, match + 0x7928, 5}, {2, match + 0x794c, 0x3de}, {2, match + 0x7948, 0x400}}, "state 5");
    for (const std::uint32_t mark : {0x3deu, 0x483u, 0x528u}) {
        for (const std::uint32_t flags : {0u, 1u, 2u, 3u, 4u, 6u, 0x20u, 0x21u, 0x22u, 0x26u, 0x4002u, 0x4006u, 0x4001u, 0x4021u, 0x4020u, 0x23u}) {
            for (const auto [first_a, first_b] : {std::pair{0xffu, 0xffu}, {0u, 0u}, {1u, 0u}, {0u, 1u}, {2u, 1u}}) {
                for (const auto [goals_a, goals_b] : {std::pair{0u, 0u}, {1u, 0u}, {1u, 1u}, {2u, 1u}}) {
                    for (const std::uint32_t extra_flag : {0u, 1u}) {
                        if (mark != 0x528 && mark != 0x483 && extra_flag) continue;
                        add(0x101d95a0, {{3, match}},
                            {{2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0}, {2, fixture + 0x38, flags},
                             {1, fixture + 0x4b, first_a}, {1, fixture + 0x4c, first_b}, {1, match + 0xe03d, goals_a},
                             {1, match + 0xe073, goals_b}, {1, fixture + 0x43, 1}, {1, fixture + 0x44, extra_flag ? 0u : 1u},
                             {1, match + 0x1c1, extra_flag}},
                            "tie rules");
                    }
                }
            }
        }
    }

    // 0x101da020: both sides, with the pitch-side checks on or off and teams of different strength.
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t statistics : {0u, 1u}) {
            for (const std::uint32_t extra : {0u, 1u}) {
                for (const std::uint32_t strength : {0u, 300u, 2500u, 5000u}) {
                    const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
                    add(0x101da020, {{3, match}, {4, side}},
                        {{1, match + 0x7931, statistics}, {1, match + 0x1c1, extra}, {2, home + 0x1ec, strength},
                         {2, away + 0x1ec, 5000 - strength}},
                        "line-up");
                }
            }
        }
    }

    // The early reports: statistics fixture checks (0x101fce60/0x101fcf20 read +0x30, +0x32 of the fixture).
    for (const std::uint32_t side : {0u, 1u})
        for (const std::uint32_t code : {0x82u, 0x81u, 0u})
            for (const std::uint32_t second : {0u, 1u})
                add(0x101da020, {{3, match}, {4, side}}, {{2, fixture + 0x30, code}, {2, fixture + 0x32, second}, {1, match + 0x7931, 1}}, "reports");

    // Code 0x96 reports through 0x101fcf20 when the side leads (the goals of the period, 0x101f21d0); code 0x82 through
    // 0x101fce60. The second side also asks about the first one (the extra-time flag clear).
    for (const std::uint32_t side : {0u, 1u})
        for (const std::uint32_t code : {0x96u, 0x82u})
            for (const auto [goals_a, goals_b] : {std::pair{2u, 0u}, {0u, 2u}, {1u, 1u}})
                for (const std::uint32_t extra_flag : {0u, 1u})
                    add(0x101da020, {{3, match}, {4, side}},
                        {{2, fixture + 0x30, code}, {2, fixture + 0x32, 0}, {1, match + 0x7931, 1}, {1, match + 0xe03d, goals_a},
                         {1, match + 0xe073, goals_b}, {1, match + 0x1c1, extra_flag}},
                        "reports by goals");

    // Ratings of the whole squad (+0x1b) against the teams' strengths.
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t rating : {0u, 6u, 10u, 32u, 60u, 100u, 127u, 0x80u, 0xffu}) {
            // (6 rates a spread of exactly 1, where 2500/2500 and 4000/5000 make the two sides' products equal)
            for (const auto [strength, other] : {std::pair{0u, 20000u}, {100u, 19900u}, {3000u, 17000u}, {10000u, 10000u}, {20000u, 0u}, {4000u, 5000u}, {2500u, 2500u}, {5000u, 4000u}}) {
                std::vector<Write> memory{{1, match + 0x7931, 1}};
                for (std::uint32_t index = 0; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x1b, rating});
                const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
                memory.push_back({2, home + 0x1ec, strength});
                memory.push_back({2, away + 0x1ec, other});
                add(0x101da020, {{3, match}, {4, side}}, memory,
                    "ratings side " + std::to_string(side) + " rating " + std::to_string(rating) + " strength " + std::to_string(strength));
            }
        }
    }

    // 0x101da3c0: the kick orders (the live match supplies the squads). Without a side's statistics record the
    // kicker is searched among the players, and none is found when all of them are out of the squad.
    add(0x101da3c0, {{3, match}}, {}, "kick order");
    add(0x101da3c0, {{3, match}}, {{1, match + 0x7952, 1}, {1, match + 0x7953, 1}}, "kick order, flags set");
    for (const std::uint32_t side : {0u, 1u}) {
        const auto stats = match + 0x7aac + side * 0xd8;
        add(0x101da3c0, {{3, match}}, {{4, stats + 0x20, 0}}, "kick order, no record");
        for (const std::uint32_t kept : {3u, 5u, 8u, 10u}) {
            std::vector<Write> memory{{4, stats + 0x20, 0}};
            for (std::uint32_t index = kept; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
            add(0x101da3c0, {{3, match}}, memory, "kick order, " + std::to_string(kept) + " players");
        }
    }

    // A side's statistics record that passes the competition test (its first word just below the limit): the side can
    // supply a kicker unless the record, the team or the side's flag byte say otherwise. The routines of other modules
    // that decide it answer 0 or 1.
    {
        const auto top = scenario.r32(toc(-0x7be4));
        for (const std::uint32_t record_ok : {0u, 1u}) {
            for (const std::uint32_t by_record : {0u, 1u})
                for (const std::uint32_t by_team : {0u, 1u})
                    for (const std::uint32_t by_competition : {0u, 1u})
                        for (const std::uint32_t flag_byte : {0u, 1u}) {
                            std::vector<Write> memory;
                            for (const std::uint32_t side : {0u, 1u}) {
                                const auto stats = match + 0x7aac + side * 0xd8;
                                const auto record = scenario.r32(stats + 0x20);
                                if (record_ok) {
                                    memory.push_back({4, record, top - 1});
                                    memory.push_back({1, record + 0x6b, 1});
                                }
                                memory.push_back({1, stats + 0x1b, flag_byte});
                            }
                            const std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {
                                {0x10147e70, by_record}, {0x100c7d00, by_team}, {0x101481d0, by_competition}};
                            for (const std::uint32_t side : {0u, 1u}) add(0x101da5a0, {{3, match}, {4, side}}, memory, "can supply", answers);
                            add(0x101da3c0, {{3, match}}, memory, "kick order by team", answers);
                        }
        }
    }

    // 0x101da760: a player of the squad by its person pointer, and failures.
    for (const std::uint32_t side : {0u, 1u}) {
        for (const std::uint32_t index : {0u, 5u, 10u, 11u, 19u}) {
            const auto player = scenario.player(side, index);
            const auto target = scenario.r32(player + 0x6c);
            add(0x101da760, {{3, match}, {4, target}, {5, side}}, {}, "found");
            add(0x101da760, {{3, match}, {4, target}, {5, 1 - side}}, {}, "other side");
            add(0x101da760, {{3, match}, {4, target}, {5, side}}, {{1, player + 0x19, 0xff}}, "no position");
        }
        add(0x101da760, {{3, match}, {4, 0}, {5, side}}, {}, "no target");
        add(0x101da760, {{3, match}, {4, 0x12345678}, {5, side}}, {}, "unknown target");
    }

    // 0x101da5a0 is reached live; one direct case per side as well.
    for (const std::uint32_t side : {0u, 1u}) add(0x101da5a0, {{3, match}, {4, side}}, {}, "kicker");

    // 0x101da820: every stage. The kick stages need a shoot-out in progress: the kick order, the kicks left,
    // the goals so far and the kicking side are set.
    // The shoot-out's start and the exhausted kick read stack the original never wrote (a null pointer
    // when the harness's stack is bare), so these cases fill the stack below the frame with a valid pointer.
    const auto sp = std::uint32_t(scenario.s.r<4>(4));
    std::vector<Write> stack;
    for (std::uint32_t at = sp - 0xc00; at < sp; at += 4) stack.push_back({4, at, match});
    std::vector<Write> shootout;
    for (std::uint32_t slot = 0; slot < 22; ++slot) shootout.push_back({1, match + 0x1c8 + slot, slot % 11});
    for (const std::uint32_t stage : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0xffu}) {
        for (const std::uint32_t mode : {0u, 3u, 4u}) {
            if ((stage == 0xffu && mode) || stage == 2 || stage == 5) continue;  // the kick cases below set the shoot-out up
            auto memory = (stage == 1 || stage == 4) ? stack : std::vector<Write>{};
            memory.push_back({1, match + 0x7929, stage});
            add(0x101da820, {{3, match}, {4, mode}}, memory, "stage " + std::to_string(stage) + " mode " + std::to_string(mode));
        }
    }
    for (const std::uint32_t stage : {2u, 5u}) {
        for (const std::uint32_t kicking : {0u, 1u}) {
            for (const std::uint32_t mode : {3u, 4u, 0u}) {
                for (const auto [goals_a, goals_b] : {std::pair{0u, 0u}, {2u, 0u}, {0u, 3u}, {4u, 4u}}) {
                    for (const auto [left_a, left_b] : {std::pair{5u, 5u}, {1u, 1u}, {0u, 0u}, {5u, 1u}, {0u, 5u}, {0u, 1u}}) {
                        auto memory = shootout;
                        for (const auto& write : std::vector<Write>{{1, match + 0x7929, stage}, {1, match + 0x7914, kicking},
                                                                    {1, match + 0x7915, 2}, {1, match + 0x7916, 3},
                                                                    {1, match + 0x7917, left_a}, {1, match + 0x7918, left_b},
                                                                    {1, match + 0x7919, 0}, {1, match + 0xe03c, goals_a},
                                                                    {1, match + 0xe072, goals_b}, {1, match + 0x7928, 8}})
                            memory.push_back(write);
                        add(0x101da820, {{3, match}, {4, mode}}, memory, "kick");
                    }
                }
            }
        }
    }
    {
        auto memory = stack;
        memory.insert(memory.end(), shootout.begin(), shootout.end());
        for (const auto& write : std::vector<Write>{{1, match + 0x7929, 2}, {1, match + 0x7917, 0}, {1, match + 0x7918, 0}, {1, match + 0x7914, 0},
                                                    {1, match + 0x7915, 0}, {1, match + 0x7916, 0}, {1, match + 0x7928, 8}})
            memory.push_back(write);
        add(0x101da820, {{3, match}, {4, 0}}, memory, "both exhausted");
    }
    // The kicker is the pending player (forces the retry loop), at several kicks taken (the order slot is taken % 11).
    for (const std::uint32_t stage : {2u, 5u}) {
        for (const std::uint32_t kicking : {0u, 1u}) {
            for (const std::uint32_t taken : {0u, 3u, 10u, 11u, 22u, 0x80u}) {
                const auto slot = std::int32_t(kicking * 11) + std::int32_t(std::int8_t(taken)) % 11;
                if (slot < 0) continue;
                const auto taker = scenario.player(kicking, std::uint32_t(slot) % 11);
                for (const std::uint32_t mode : {3u, 4u}) {
                    auto memory = shootout;
                    for (const auto& write : std::vector<Write>{{1, match + 0x7929, stage}, {1, match + 0x7914, kicking}, {1, match + 0x7915 + kicking, taken},
                                                                {1, match + 0x7917, 5}, {1, match + 0x7918, 5}, {1, match + 0x7919, 0},
                                                                {1, match + 0xe03c, 1}, {1, match + 0xe072, 0}, {1, match + 0x7928, 8},
                                                                {4, match + 0xe000, taker}})
                        memory.push_back(write);
                    add(0x101da820, {{3, match}, {4, mode}}, memory, "kick with pending player");
                }
            }
        }
    }
    for (const std::uint32_t stage : {3u, 6u})
        for (const auto [goals_a, goals_b] : {std::pair{3u, 1u}, {1u, 3u}, {2u, 2u}})
            add(0x101da820, {{3, match}, {4, 0}}, {{1, match + 0x7929, stage}, {1, match + 0xe03c, goals_a}, {1, match + 0xe072, goals_b}}, "finish");
    // Kicks over random seeds: the keeper's and the taker's moves vary with the draws, and so does the state the
    // kick leaves behind (the loop that retries while the taker is still pending).
    unsigned lcg = 4099;
    const auto below = [&](unsigned limit) {
        lcg = lcg * 1103515245u + 12345u;
        return (lcg >> 16) % limit;
    };
    for (unsigned n = 0; n < 240; ++n) {
        const std::uint32_t stage = below(2) ? 2u : 5u, kicking = below(2), mode = below(3) == 0 ? 0u : (below(2) ? 3u : 4u);
        auto memory = shootout;
        const std::uint32_t taken = below(12);
        const auto slot = kicking * 11 + taken % 11;
        for (const auto& write : std::vector<Write>{{1, match + 0x7929, stage}, {1, match + 0x7914, kicking}, {1, match + 0x7915 + kicking, taken},
                                                    {1, match + 0x7917, 1 + below(5)}, {1, match + 0x7918, 1 + below(5)}, {1, match + 0x7919, below(2)},
                                                    {1, match + 0xe03c, below(4)}, {1, match + 0xe072, below(4)}, {1, match + 0x7928, 8},
                                                    {4, seed_at, 1237u * (n + 1)}})
            memory.push_back(write);
        if (below(3) == 0) memory.push_back({4, match + 0xe000, scenario.player(kicking, slot % 11)});
        else if (below(2) == 0) memory.push_back({4, match + 0xe000, scenario.player(1 - kicking, below(2) ? 0u : below(11))});
        // The keeper's routine looks at the side that last touched the ball (-1: none) and at the held player.
        memory.push_back({1, match + 0x7924, below(3) == 0 ? 0xffu : below(2)});
        memory.push_back({1, match + 0x1de, below(2)});
        memory.push_back({1, match + 0x1be + below(2), below(2)});
        add(0x101da820, {{3, match}, {4, mode}}, memory, "kick over seeds");
    }
}

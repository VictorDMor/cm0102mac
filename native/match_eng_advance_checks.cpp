// Cases for the advance block from a real match state. The live match reaches the common
// tick; the match ends, the shoot-out stages, the pending restarts and the rarer branches of
// the tick are set up by memory overrides on the real objects. Seeds vary the random draws.
#include "match_eng_checks.hpp"
#include <bit>
#include <string_view>

using namespace cm::aot::match_eng_checks;

namespace {
std::vector<Write> join_writes(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
std::uint32_t spare_player(const Scenario& scenario) { return scenario.player(0, 9); }
}

MATCH_ENG_CASES("advance") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto r2 = std::uint32_t(scenario.s.r<4>(8));
    const auto seed_at = scenario.r32(r2 - 0x7b90);
    const auto first = scenario.player(0, 3), second = scenario.player(1, 7);
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    // Different random draws: the seed moves every draw.
    const std::uint32_t seeds[] = {0, 1, 2, 3, 5, 7, 11, 13, 17, 23, 31, 47};

    // --- 0x101d3f60 ---
    // A match that was completed before: the closing calls, with the fixture's flags clear and set.
    for (const std::uint32_t flag_a : {0u, 1u})
        for (const std::uint32_t flag_b : {0u, 1u})
            add(0x101d3f60, {{3, match}, {4, 1}}, {{1, match, 1}, {1, fixture + 0x40, flag_a}, {1, fixture + 0x41, flag_b}}, "completed");
    // A restart stage of 1 plays the restart at once; a stage of 2 is reset.
    for (const std::uint32_t count : {1u, 3u}) {
        add(0x101d3f60, {{3, match}, {4, count}}, {{1, match + 0x792a, 1}}, "stage 1");
        add(0x101d3f60, {{3, match}, {4, count}}, {{1, match + 0x792a, 2}, {1, match + 0x7928, 8}}, "stage 2");
    }
    // The shoot-out: every stage, finished or not, with the fixture's flags.
    for (const std::uint32_t stage : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 0xffu})
        for (const std::uint32_t flags : {0u, 1u})
            add(0x101d3f60, {{3, match}, {4, 1}},
                {{1, match + 0x7929, stage}, {1, fixture + 0x40, flags}, {1, fixture + 0x41, flags ^ 1}, {1, match + 0x792a, 0}}, "shoot-out");
    // The kick-off message: once per stopped match, early or late in the clock.
    for (const std::uint32_t clock : {0u, 1u, 2u, 40u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            {{1, match + 0x1e3, 0}, {1, match + 0x7928, 8}, {1, match + 0x1c1, 0}, {1, match + 0x792a, 0}, {1, match + 0x7929, 0}, {2, match + 0x7948, clock}},
            "kick-off");
    // A restart taker is picked while play is stopped; the sides' players may or may not be in play.
    for (const std::uint32_t stopped : {0u, 8u, 5u})
        for (const std::uint32_t seed : seeds)
            add(0x101d3f60, {{3, match}, {4, 1}},
                {{1, match + 0x7928, stopped}, {1, match + 0x792a, 0}, {1, match + 0x1c1, 0}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0},
                 {4, seed_at, seed}},
                "pick");
    // A pending restart: with and without a second player, by the kinds of the original's table.
    for (const std::uint32_t flag : {1u, 2u, 7u, 8u})
        for (const std::uint32_t stopped : {0u, 8u})
            for (const std::uint32_t seed : {0u, 3u, 11u})
                add(0x101d3f60, {{3, match}, {4, 1}},
                    {{1, match + 0x30dc, flag}, {4, match + 0x30e4, first}, {4, match + 0x30e8, flag == 2 ? second : 0u},
                     {2, match + 0x30e2, 12}, {1, match + 0x7928, stopped}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0},
                     {1, match + 0x30dd, 4}, {1, match + 0x30de, 6}, {4, seed_at, seed}},
                    "pending restart");
    // The tick: the counter and the deadline for the stoppage, extra time, the stopped clock.
    for (const std::uint32_t counter : {0xfffdu, 0xffffu, 0u, 3u})
        for (const std::uint32_t extra : {0u, 1u})
            for (const std::uint32_t quarter : {0xffu, 0u, 1u, 3u})
                for (const std::uint32_t clock : {5u, 20u, 60u})
                    add(0x101d3f60, {{3, match}, {4, 1}},
                        {{2, match + 0x794a, counter}, {1, match + 0x1c1, extra}, {1, match + 0xe0b8, quarter}, {2, match + 0x7948, clock},
                         {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}},
                        "tick");
    // The period marks (half time, full time, extra time) end the tick with 4 or play on.
    for (const std::uint32_t mark : {0u, 1u, 0x1efu, 0x3deu, 0x483u, 0x528u})
        for (const std::uint32_t over : {1u, 2u, 40u})
            for (const std::uint32_t flags : {0u, 2u, 0x20u})
                add(0x101d3f60, {{3, match}, {4, 1}},
                    {{2, match + 0x794c, mark}, {2, match + 0x7948, mark + over}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff},
                     {2, fixture + 0x38, flags}, {1, match + 0x7928, 0}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}},
                    "period");
    // The ball's carrier and the stopped players: a carrier at the ball with and without a rating.
    for (const std::uint32_t wait : {0u, 1u, 2u})
        for (const std::uint32_t seed : seeds)
            add(0x101d3f60, {{3, match}, {4, 1}},
                {{1, match + 0x1de, wait}, {4, match + 0x3104, first}, {1, match + 0x791f, 4}, {1, match + 0x7928, 0}, {1, match + 0x1c1, 0},
                 {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0}, {4, seed_at, seed}},
                "carrier");
    // The tick under many seeds, in play and stopped.
    for (const std::uint32_t seed : seeds)
        for (const std::uint32_t stopped : {0u, 8u})
            add(0x101d3f60, {{3, match}, {4, 2}},
                {{4, seed_at, seed}, {1, match + 0x7928, stopped}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}},
                "seeds");

    // --- 0x101d5760: the flag 0x1000 of a side's team record ---
    for (const std::uint32_t side : {0u, 1u, 0xffu})
        for (const std::uint32_t flags : {0u, 0x1000u, 0xefffu, 0xffffffffu})
            add(0x101d5760, {{3, match}, {4, side}}, {{4, match + 0x7c6c + (side == 1 ? 0x18e0u : 0u) + 0x588, flags}}, "flag 1000");

    // --- 0x101d5810 and 0x101d6710: messages ---
    const auto target = scenario.r32(match + 0x79f4);
    for (const std::uint32_t id : {0x20f0u, 0x1ff5u, 0xffffu, 0x18000u}) for (const std::uint32_t byte : {0u, 1u, 0xffu, 0x180u}) {
        add(0x101d5810, {{3, target}, {4, id}, {5, match}, {6, byte}, {7, 0x12345678}}, {}, "message");
        add(0x101d6710, {{3, target}, {4, id}, {5, match}, {6, byte}, {7, byte ^ 0x55}, {8, 0x87654321}}, {}, "player message");
    }

    // --- 0x101d5880 and 0x101d69c0: a pending restart ---
    for (const std::uint32_t flag : {0u, 1u, 8u})
        for (const std::uint32_t holder : {0u, first, second})
            for (const std::uint32_t taker : {0u, first, second})
                for (const std::uint32_t row : {2u, 5u, 6u, 9u}) {
                    const std::vector<Write> memory{{1, match + 0x30dc, flag}, {4, match + 0x30e4, taker}, {4, match + 0xe000, holder},
                                                    {1, match + 0x30de, 5}, {1, match + 0x791e, row}, {1, match + 0x7924, row & 1}};
                    add(0x101d5880, {{3, match}}, memory, "still valid");
                    add(0x101d69c0, {{3, match}}, memory, "owner");
                }
    for (const std::uint32_t seed : seeds) for (const std::uint32_t kind : {1u, 8u}) for (const std::uint32_t last : {0u, 1u})
        for (const std::uint32_t row : {0u, 1u, 4u, 10u, 11u}) for (const std::uint32_t column : {1u, 3u, 7u})
            add(0x101d69c0, {{3, match}},
                {{1, match + 0x30dc, kind}, {4, match + 0x30e4, first}, {1, match + 0x7924, last}, {1, match + 0x30dd, column},
                 {1, match + 0x30de, row}, {4, seed_at, seed}},
                "owner draw");

    // --- 0x101d5980: the restart is resolved ---
    for (const std::uint32_t kind : {0xffffu, 0u, 1u, 12u, 19u, 21u, 33u, 34u, 41u, 42u, 43u, 44u, 45u, 46u, 47u, 100u})
        for (const std::uint32_t other : {0u, second})
            for (const std::uint32_t seed : {0u, 5u})
                add(0x101d5980, {{3, match}},
                    {{1, match + 0x30dc, 1}, {4, match + 0x30e4, first}, {4, match + 0x30e8, other}, {2, match + 0x30e2, kind},
                     {1, match + 0x7928, 0}, {1, match + 0x30dd, 4}, {1, match + 0x30de, 6}, {4, seed_at, seed}},
                    "restart kind");
    for (const std::uint32_t flag : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u})
        for (const std::uint32_t other : {0u, second})
            for (const std::uint32_t stopped : {0u, 8u})
                for (const std::uint32_t last : {0u, 1u})
                    for (const std::uint32_t row : {0u, 3u, 5u, 6u, 10u})
                        for (const std::uint32_t seed : {0u, 1u, 2u, 7u})
                          for (const std::uint32_t taker : {first, second}) {
                            add(0x101d5980, {{3, match}},
                                {{1, match + 0x30dc, flag}, {4, match + 0x30e4, taker}, {4, match + 0x30e8, other}, {2, match + 0x30e2, 12},
                                 {1, match + 0x7928, stopped}, {1, match + 0x7924, last}, {1, match + 0x791e, row},
                                 {1, match + 0x30dd, 4}, {1, match + 0x30de, 5}, {1, match + 0x30e0, 6 + seed},
                                 {4, seed_at, seed * 3}},
                                "restart");
                        }
    // A restart in each end area (the ball lands there), and with a restart player of the second side.
    for (const std::uint32_t row : {0u, 1u, 10u, 11u}) for (const std::uint32_t column : {1u, 2u, 4u, 6u, 7u})
        for (const std::uint32_t player : {first, second}) for (const std::uint32_t other : {0u, spare_player(scenario)})
            add(0x101d5980, {{3, match}},
                {{1, match + 0x30dc, 1}, {4, match + 0x30e4, player}, {4, match + 0x30e8, other}, {2, match + 0x30e2, 12},
                 {1, match + 0x7928, 0}, {1, match + 0x30dd, column}, {1, match + 0x30de, row}},
                "end area");

    // --- 0x101d6780 and 0x101d6860: moves over a side ---
    for (const double factor : {0.0, 1.0, -2.5, 123456.0})
        for (const std::uint32_t side : {0u, 1u}) {
            add(0x101d6780, {{3, match}, {4, scenario.player(side, 4)}}, {}, "around");
            cases.back().f1 = factor;
            add(0x101d6860, {{3, match}, {4, side}}, {}, "side");
            cases.back().f1 = factor;
            add(0x101d6860, {{3, match}, {4, side}}, {{4, scenario.player(side, 2) + 0x6c, 0}}, "side without a record");
            cases.back().f1 = factor;
            add(0x101d6780, {{3, match}, {4, scenario.player(side, 2)}}, {{4, scenario.player(side, 5) + 0x6c, 0}, {1, scenario.player(side, 6) + 0x19, 0xff}}, "around");
            cases.back().f1 = factor;
        }

    // --- 0x101d6a90: a side's pressure from its players ---
    for (const std::uint32_t side : {0u, 1u}) for (const std::uint32_t seed : seeds)
        add(0x101d6a90, {{3, match}, {4, side}}, {{4, seed_at, seed}}, "pressure");
    for (const std::uint32_t side : {0u, 1u}) for (const std::uint32_t index : {0u, 6u, 19u}) for (const std::uint32_t position : {0u, 0xffu})
        add(0x101d6a90, {{3, match}, {4, side}}, {{1, scenario.player(side, index) + 0x19, position}}, "pressure position");

    // --- 0x101d6da0: the ball drifts ---
    for (const std::uint32_t holder : {0u, first}) for (const std::uint32_t pending : {0u, first}) for (const std::uint32_t timer : {0u, 1u, 2u, 5u, 0xffu})
        for (const std::uint32_t stopped : {0u, 8u})
            add(0x101d6da0, {{3, match}},
                {{4, match + 0xe000, holder}, {4, match + 0xdff8, pending}, {1, match + 0x7920, timer}, {1, match + 0x7928, stopped}}, "drift gates");
    for (const std::uint32_t turn : {0u, 0x28u, 0x29u, 0x8bu, 0x8cu, 0x82u, 0x83u, 0xe5u, 0xe6u, 0x31u, 0x32u, 0x136u, 0x137u, 0xdcu, 0xddu, 0x13fu, 0x140u})
        for (const std::uint32_t column : {0u, 1u, 7u, 8u, 9u, 0xffu}) for (const std::uint32_t row : {0u, 5u, 10u, 11u, 12u, 0xffu})
            for (const std::uint32_t step : {0u, 3u})
                add(0x101d6da0, {{3, match}},
                    {{4, match + 0xe000, 0}, {4, match + 0xdff8, 0}, {1, match + 0x7920, 5}, {1, match + 0x7928, 0}, {2, match + 0x7950, turn},
                     {1, match + 0x791d, column}, {1, match + 0x791e, row}, {1, match + 0x791f, step}},
                    "drift");
    for (const std::uint32_t seed : seeds) for (const std::uint32_t timer : {1u, 2u})
        add(0x101d6da0, {{3, match}},
            {{4, match + 0xe000, 0}, {4, match + 0xdff8, 0}, {1, match + 0x7920, timer}, {1, match + 0x7928, 0}, {4, seed_at, seed},
             {2, match + 0x7950, 0x30}, {1, match + 0x791d, 4}, {1, match + 0x791e, 5}},
            "drift draws");

    // --- 0x101d6f90: the teams' players ---
    add(0x101d6f90, {{3, match}}, {}, "teams players");

    // A restart without a player ends at once.
    for (const std::uint32_t flag : {1u, 8u})
        add(0x101d5980, {{3, match}}, {{1, match + 0x30dc, flag}, {4, match + 0x30e4, 0}, {1, match + 0x7928, 0}}, "no player");
    // The random draws of a restart, over every residue the original's draws can take (4, 9, 7, 3 and the rating).
    for (std::uint32_t seed = 0; seed < 252; ++seed) for (const std::uint32_t other : {0u, second}) for (const std::uint32_t flag : {3u, 7u, 1u}) {
        add(0x101d5980, {{3, match}},
            {{1, match + 0x30dc, flag}, {4, match + 0x30e4, first}, {4, match + 0x30e8, other}, {2, match + 0x30e2, 12},
             {1, match + 0x7928, 0}, {1, match + 0x30dd, 4}, {1, match + 0x30de, 5}, {1, match + 0x30e0, 9 + seed % 5}, {4, seed_at, seed}},
            "restart draws");
    }
    for (std::uint32_t seed = 0; seed < 252; ++seed)
        add(0x101d5980, {{3, match}},
            {{1, match + 0x30dc, 2}, {4, match + 0x30e4, second}, {4, match + 0x30e8, first}, {2, match + 0x30e2, 41},
             {1, match + 0x7928, 0}, {1, match + 0x30dd, 4}, {1, match + 0x30de, 0}, {1, match + 0x30e0, 12 + seed % 3}, {4, seed_at, seed}},
            "restart end-area draws");
    // The teams' players: the tactic bytes of every position set for the substitution.
    for (const std::uint32_t flags : {0x14u, 0x10u, 0x04u, 0u}) {
        std::vector<Write> memory;
        for (const std::uint32_t team_side : {0u, 1u})
            for (std::uint32_t position = 0; position < 24; ++position)
                memory.push_back({1, match + 0x7c6c + team_side * 0x18e0 + 0x5b8 + position, flags});
        add(0x101d6f90, {{3, match}}, memory, "teams players flags");
    }
    // The advance loop over the tactic bytes (bit 0 marks a player as handled).
    for (const std::uint32_t flags : {0u, 1u, 0xffu}) for (const std::uint32_t seed : {0u, 4u, 9u}) {
        std::vector<Write> memory{{4, seed_at, seed}, {1, match + 0x7928, 0}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}};
        for (const std::uint32_t team_side : {0u, 1u})
            for (std::uint32_t position = 0; position < 24; ++position)
                memory.push_back({1, match + 0x7c6c + team_side * 0x18e0 + 0x5b8 + position, flags});
        add(0x101d3f60, {{3, match}, {4, 1}}, memory, "tick tactics");
    }
    // The tick over every residue the draws of the tick can take (10, 2, 20, 5, 7, 9).
    for (std::uint32_t seed = 0; seed < 252; ++seed) for (const std::uint32_t stopped : {0u, 8u}) for (const std::uint32_t flag : {0u, 1u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            {{4, seed_at, seed}, {1, match + 0x7928, stopped}, {1, match + 0x1e3, 1}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0},
             {2, match + 0x794a, 0xffff}, {1, match + 0x792b, flag}},
            "tick draws");

    // --- more of the tick (0x101d3f60) ---
    const auto base = [&](std::uint32_t seed) {
        return std::vector<Write>{{4, seed_at, seed}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}};
    };
    const auto with = [&](std::vector<Write> memory, std::initializer_list<Write> more) {
        memory.insert(memory.end(), more.begin(), more.end());
        return memory;
    };
    // The kick-off message only goes out for a stopped match in regular time.
    for (const std::uint32_t stopped : {0u, 5u, 8u}) for (const std::uint32_t extra : {0u, 1u}) for (const std::uint32_t clock : {0u, 2u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            {{1, match + 0x1e3, 0}, {1, match + 0x7928, stopped}, {1, match + 0x1c1, extra}, {1, match + 0x792a, 0}, {1, match + 0x7929, 0},
             {2, match + 0x7948, clock}},
            "kick-off gates");
    // A taker is picked while every player is out of play, or only one side's.
    for (const std::uint32_t seed : {0u, 1u, 3u, 8u}) for (const std::uint32_t out_side : {0u, 1u, 2u}) {
        std::vector<Write> memory = with(base(seed), {{1, match + 0x7928, 8}, {1, match + 0x1c1, 0}});
        for (std::uint32_t side = 0; side < 2; ++side)
            if (out_side == side || out_side == 2)
                for (std::uint32_t index = 0; index < 20; ++index) memory.push_back({1, scenario.player(side, index) + 0x19, 0xff});
        add(0x101d3f60, {{3, match}, {4, 1}}, memory, "pick out of play");
    }
    // A pending restart in the tick: dropped, resolved, or still valid, with a holder of the ball.
    for (const std::uint32_t stopped : {0u, 8u}) for (const std::uint32_t last : {0u, 1u}) for (const std::uint32_t taker : {first, second})
        for (const std::uint32_t holder : {0u, first, second}) for (const std::uint32_t row : {3u, 5u, 6u}) for (const std::uint32_t seed : {0u, 5u}) {
            add(0x101d3f60, {{3, match}, {4, 1}},
                with(base(seed), {{1, match + 0x30dc, 1}, {4, match + 0x30e4, taker}, {4, match + 0xe000, holder}, {1, match + 0x7928, stopped},
                                  {1, match + 0x7924, last}, {1, match + 0x791e, row}, {1, match + 0x30de, 5}, {1, match + 0x30dd, 4},
                                  {2, match + 0x30e2, 12}, {1, match + 0x1c1, 0}}),
                "pending restart in the tick");
        }
    // The taker of a stopped restart is chosen when nobody holds the ball.
    for (const std::uint32_t last : {0u, 1u}) for (const std::uint32_t holder : {0u, first}) for (const std::uint32_t seed : {0u, 2u, 7u}) {
        std::vector<Write> memory = with(base(seed), {{1, match + 0x7928, 8}, {1, match + 0x7924, last}, {4, match + 0xe000, holder}, {1, match + 0x1c1, 0}});
        if (holder) memory.push_back({1, first + 0x19, 0xff});
        add(0x101d3f60, {{3, match}, {4, 1}}, memory, "choose taker");
    }
    // The end of a period or the match: the first leg, the extra-time score, the shoot-out rules.
    // The competition table the match's closing code asks (0x104646d0: is this competition one of the
    // three special ids) isn't in a match snapshot, so both sides answer it: 0, or 1 for the special ones.
    for (const std::uint32_t special : {0u, 1u})
    for (const std::uint32_t mark : {0x3deu, 0x528u}) for (const std::uint32_t flags : {0u, 1u, 0x20u, 0x21u, 2u, 4u, 0x4002u})
        for (const std::uint32_t first_leg : {0xffu, 0u, 2u}) for (const std::uint32_t extra_score : {0xffu, 0u, 3u})
            for (const auto [goals_a, goals_b] : {std::pair{0u, 0u}, {1u, 1u}, {2u, 1u}}) for (const std::uint32_t statistics : {0u, 1u})
              for (const std::uint32_t word : {0u, 0xfffffffeu}) {
                if (special && ((flags != 0 && flags != 0x20) || first_leg == 2 || extra_score == 3 || statistics)) continue;
                cases.push_back({.entry = 0x101d3f60, .registers = {{3, match}, {4, 1}},
                    .memory = {{4, fixture, word == 0 ? scenario.r32(fixture) : word}, {2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff},
                     {2, fixture + 0x38, flags}, {1, fixture + 0x4b, first_leg}, {1, fixture + 0x4c, first_leg}, {1, fixture + 0x47, extra_score},
                     {1, fixture + 0x48, extra_score}, {1, match + 0xe03d, goals_a}, {1, match + 0xe073, goals_b}, {1, match + 0x7928, 0},
                     {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}, {1, match + 0x1c1, mark == 0x528 ? 1u : 0u},
                     {1, match + 0x7931, statistics}},
                    .f1 = {}, .label = "match end", .answers = {{0x104646d0, special}}});
              }
    // A period ended with the statistics report; the clock stopped or not.
    for (const std::uint32_t mark : {0x1efu, 0x3deu}) for (const std::uint32_t statistics : {0u, 1u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            {{2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff}, {1, match + 0x7931, statistics},
             {1, match + 0x7928, 0}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}},
            "period report");
    // Extra time or a stopped match: the actors' turns run the clock on.
    for (const std::uint32_t extra : {0u, 1u}) for (const std::uint32_t stopped : {0u, 8u}) for (const std::uint32_t flag : {0u, 1u}) for (const std::uint32_t seed : {0u, 3u, 6u, 9u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            with(base(seed), {{1, match + 0x1c1, extra}, {1, match + 0x7928, stopped}, {1, match + 0x792b, flag}, {2, match + 0x794c, 0x528},
                              {2, match + 0x7948, 10}, {2, match + 0x794a, 0xffff}, {1, match + 0x30dc, 0}, {1, match + 0xe0b8, 0xff}}),
            "stopped turns");
    // Shots (0x11) and headers (0x12) happen as the actors' actions decide; the rating that makes the draw's
    // bound 0 or 1 is set for every player.
    {
        const auto pool = scenario.r32(r2 - 0x52c8);
        const auto f0 = std::bit_cast<float>(std::uint32_t(scenario.r32(pool + 0x1c))), f2 = std::bit_cast<float>(std::uint32_t(scenario.r32(pool + 0x20)));
        for (const float tune : {0.0f, 1.0f, -1.0f}) for (const std::uint32_t seed : {0u, 2u, 5u, 11u, 17u, 29u}) {
            const auto rating = std::bit_cast<std::uint32_t>(f2 != 0 ? (f0 / f2) * (1 + tune * 1e-3f) : 0.0f);
            std::vector<Write> memory = base(seed);
            for (std::uint32_t side = 0; side < 2; ++side) for (std::uint32_t index = 0; index < 20; ++index) {
                memory.push_back({4, scenario.player(side, index) + 0x74, rating});
                memory.push_back({4, scenario.player(side, index) + 0xc4, rating});
            }
            memory.push_back({1, match + 0x7928, 0});
            add(0x101d3f60, {{3, match}, {4, 1}}, memory, "shots");
        }
    }
    // A pending restart over every residue of the draws (5, 7).
    for (std::uint32_t seed = 0; seed < 35; ++seed) for (const std::uint32_t stopped : {0u, 8u}) for (const std::uint32_t taker : {first, second})
        for (const std::uint32_t row : {3u, 6u}) for (const std::uint32_t last : {0u, 1u})
            add(0x101d3f60, {{3, match}, {4, 1}},
                with(base(seed), {{1, match + 0x30dc, 1}, {4, match + 0x30e4, taker}, {4, match + 0xe000, 0}, {1, match + 0x7928, stopped},
                                  {1, match + 0x7924, last}, {1, match + 0x791e, row}, {1, match + 0x30de, 5}, {1, match + 0x30dd, 4},
                                  {2, match + 0x30e2, 12}, {1, match + 0x1c1, 0}}),
                "pending restart draws");
    // A fixture of the shoot-out kind (word 0 is -2) after the match.
    for (const std::uint32_t word : {0xfffffffeu, 5u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            {{2, match + 0x794c, 0x3de}, {2, match + 0x7948, 0x3e0}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff}, {4, fixture, word},
             {2, fixture + 0x38, 0}, {1, match + 0x7928, 0}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1}},
            "fixture kind");
    // The ball's carrier: waiting, at the ball, switched; with the ratings the switch asks for.
    {
        const auto row = std::uint32_t(scenario.s.memory.read<1>(first + 0x105)), column = std::uint32_t(scenario.s.memory.read<1>(first + 0x106));
        for (const std::uint32_t wait : {0u, 1u, 3u}) for (const std::uint32_t stage : {4u, 0u}) for (const std::uint32_t holder : {0u, first})
            for (const std::uint32_t at_ball : {0u, 1u}) for (const std::uint32_t seed : {0u, 4u}) {
                add(0x101d3f60, {{3, match}, {4, 1}},
                    with(base(seed), {{1, match + 0x1de, wait}, {4, match + 0x3104, first}, {1, match + 0x791f, stage}, {4, match + 0xdff8, holder},
                                      {1, match + 0x7928, 0}, {1, match + 0x1c1, 0}, {1, first + 0x2c, 0}, {1, first + 0x108, 5}, {1, first + 0x10a, 3},
                                      {1, match + 0x791d, at_ball ? row : 15}, {1, match + 0x791e, at_ball ? column : 15}, {4, match + 0x7c68, 0}}),
                    "carrier turn");
            }
    }

    // The club reports: a team whose club id is among the last 16 is asked, and the published ones follow.
    {
        const auto limit = scenario.r32(scenario.r32(r2 - 0x7be4));
        const auto home_club = scenario.r32(scenario.r32(fixture + 0x1c) + 0x90), away_club = scenario.r32(scenario.r32(fixture + 0x20) + 0x90);
        for (const std::uint32_t patch : {1u, 2u, 3u}) {
            std::vector<Write> reports;
            if (patch & 1) reports.push_back({4, home_club, limit - 1});
            if (patch & 2) reports.push_back({4, away_club, limit - 2});
            add(0x101d3f60, {{3, match}, {4, 1}}, with(reports, {{1, match, 1}, {1, fixture + 0x40, 0}, {1, fixture + 0x41, 0}}), "completed reports");
            for (const std::uint32_t stage : {3u, 6u})
                add(0x101d3f60, {{3, match}, {4, 1}},
                    with(reports, {{1, match + 0x7929, stage}, {1, fixture + 0x40, 0}, {1, fixture + 0x41, 0}, {1, match + 0x792a, 0}}), "shoot-out reports");
            for (const std::uint32_t mark : {0x3deu, 0x528u}) for (const auto [goals_a, goals_b] : {std::pair{2u, 1u}, {1u, 2u}})
                add(0x101d3f60, {{3, match}, {4, 1}},
                    with(reports, {{2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff},
                                   {2, fixture + 0x38, 0}, {1, fixture + 0x4b, 0xff}, {1, fixture + 0x4c, 0xff}, {1, match + 0xe03d, goals_a},
                                   {1, match + 0xe073, goals_b}, {1, match + 0x7928, 0}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0},
                                   {1, match + 0x1e3, 1}, {1, fixture + 0x40, 0}, {1, fixture + 0x41, 0}, {1, match + 0x1c1, mark == 0x528 ? 1u : 0u}}),
                    "match end reports");
        }
    }
    // Teams whose tactics flag 0x4000 asks the actors to find a marker by the second rule.
    for (const std::uint32_t flags : {0x4000u, 0u}) for (const std::uint32_t seed : {0u, 3u, 9u}) for (const std::uint32_t stopped : {0u, 8u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            with(base(seed), {{4, match + 0x7c6c + 0x588, flags}, {4, match + 0x7c6c + 0x18e0 + 0x588, flags}, {1, match + 0x7928, stopped}}),
            "tactics flag");
    // A restart landing in an end area, over every residue of its draws.
    for (std::uint32_t seed = 0; seed < 63; ++seed) for (const std::uint32_t row : {0u, 10u}) for (const std::uint32_t player : {first, second})
        for (const std::uint32_t other : {0u, spare_player(scenario)}) for (const std::uint32_t kind : {1u, 3u, 7u})
            add(0x101d5980, {{3, match}},
                {{1, match + 0x30dc, kind}, {4, match + 0x30e4, player}, {4, match + 0x30e8, other}, {2, match + 0x30e2, 12},
                 {1, match + 0x7928, 0}, {1, match + 0x30dd, 3}, {1, match + 0x30de, row}, {1, match + 0x30e0, 9}, {4, seed_at, seed}},
                "end area draws");
    // --- coverage closure ---
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    const auto add_answered = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                                  std::string label, Answers answers) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label),
                         .answers = std::move(answers)});
    };
    // A side's pressure with players found in the people list: every player of the side gets list entry 0, a zeroed
    // entry at a scratch spot whose club is the first club record; the entry's flags and the players' records decide the rest.
    {
        const auto nodes = scenario.r32(scenario.r32(r2 - 0x7be8)), list = scenario.r32(scenario.r32(r2 - 0x7b7c));
        const auto index_table = scenario.r32(scenario.r32(r2 - 0x7b9c));
        const auto entry = std::uint32_t(scenario.s.r<4>(4)) - 0x10000;
        // Variants of the players' records: rating of the club, +0x2f byte and +0xa height of the player's record.
        struct Variant { std::uint32_t rating, byte2f, height; };
        const Variant variants[] = {{0x1f40, 0x7f, 0x7fff}, {0x7000, 0x7f, 0x7fff}, {0x1f40, 0, 0x7fff}, {0x1f40, 0x7f, 0x100}, {0x7000, 0x7f, 0x1c00}};
        for (const unsigned side : {0u, 1u}) for (const auto& variant : variants) for (const std::uint32_t bits : {1u, 0u})
            for (const std::uint32_t byte67 : {0u, 3u, 5u}) for (const std::uint32_t seed : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u, 11u, 13u, 17u, 19u, 23u, 31u, 47u}) {
                if (bits == 0 && (&variant != &variants[0] || byte67 != 0 || seed > 3)) continue;
                std::vector<Write> memory = {{4, list, entry}, {4, entry + 4, 0}, {4, entry + 0x50, bits}, {4, nodes, 0}, {2, nodes + 0x1ec, variant.rating}, {4, seed_at, seed}};
                for (unsigned index = 0; index < 20; ++index) {
                    const auto player = scenario.player(side, index);
                    const auto record = scenario.r32(player + 0x70), person = scenario.r32(player + 0x6c);
                    if (record) { memory.push_back({1, record + 0x2f, variant.byte2f}); memory.push_back({2, record + 0xa, variant.height}); }
                    if (person) { memory.push_back({1, person + 0x67, byte67}); memory.push_back({4, index_table + scenario.r32(person) * 0x64, 0}); }
                }
                add(0x101d6a90, {{3, match}, {4, side}}, memory, "pressure nodes");
            }
    }
    // Seeds that make the rare draws come up: the owner's chance of 1 in 200 and the restart's 1 in 9, over many seeds.
    for (std::uint32_t seed = 0; seed < 1000; ++seed)
        add(0x101d69c0, {{3, match}},
            {{1, match + 0x30dc, 8}, {4, match + 0x30e4, first}, {1, match + 0x7924, 1}, {1, match + 0x30dd, 3}, {1, match + 0x30de, 4}, {4, seed_at, seed}},
            "owner rare draw");
    for (std::uint32_t seed = 0; seed < 400; ++seed) for (const std::uint32_t other : {0u, second}) for (const std::uint32_t flag : {3u, 4u, 5u, 6u})
        add(0x101d5980, {{3, match}},
            {{1, match + 0x30dc, flag}, {4, match + 0x30e4, first}, {4, match + 0x30e8, other}, {2, match + 0x30e2, 12},
             {1, match + 0x7928, 0}, {1, match + 0x30dd, 4}, {1, match + 0x30de, 5}, {1, match + 0x30e0, 9}, {4, seed_at, seed}},
            "restart rare draws");
    // The club reports: the team's club is asked about and the report is held back (the other modules answer).
    {
        const auto limit = scenario.r32(scenario.r32(r2 - 0x7be4));
        const auto home_club = scenario.r32(scenario.r32(fixture + 0x1c) + 0x90), away_club = scenario.r32(scenario.r32(fixture + 0x20) + 0x90);
        for (const std::uint32_t held : {0u, 1u}) {
            const Answers answers = {{0x1014c2e0, 1}, {0x101481d0, held}};
            const std::vector<Write> clubs = {{4, home_club, limit - 1}, {4, away_club, limit - 2}, {1, fixture + 0x40, 0}, {1, fixture + 0x41, 0}};
            add_answered(0x101d3f60, {{3, match}, {4, 1}}, join_writes(clubs, {{1, match, 1}}), "completed reports answered", answers);
            for (const std::uint32_t stage : {3u, 6u})
                add_answered(0x101d3f60, {{3, match}, {4, 1}}, join_writes(clubs, {{1, match + 0x7929, stage}, {1, match + 0x792a, 0}}), "shoot-out reports answered", answers);
            for (const std::uint32_t mark : {0x3deu, 0x528u})
                add_answered(0x101d3f60, {{3, match}, {4, 1}},
                             join_writes(clubs, {{2, match + 0x794c, mark}, {2, match + 0x7948, mark + 2}, {1, match + 0x792d, 0}, {2, match + 0x794a, 0xffff},
                                                 {2, fixture + 0x38, 0}, {1, fixture + 0x4b, 0xff}, {1, fixture + 0x4c, 0xff}, {1, match + 0xe03d, 2}, {1, match + 0xe073, 1},
                                                 {1, match + 0x7928, 0}, {1, match + 0x7929, 0}, {1, match + 0x792a, 0}, {1, match + 0x1e3, 1},
                                                 {1, match + 0x1c1, mark == 0x528 ? 1u : 0u}}),
                             "match end reports answered", {{0x1014c2e0, 1}, {0x101481d0, held}, {0x104646d0, 0}});
        }
    }
    // A stopped match whose restart picks are locked: the first call hands the restart over (stage 1) and the tick ends.
    for (const std::uint32_t seed : {0u, 1u, 3u, 8u}) for (const std::uint32_t last : {0u, 1u}) for (const std::uint32_t locked : {0u, 1u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            with(base(seed), {{1, match + 0x7928, 8}, {1, match + 0x1c1, 0}, {1, match + 0x1bc, locked}, {1, match + 0x7924, last}, {4, match + 0xe000, 0},
                              {4, match + 0xb4, first}, {4, match + 0xb8, second}, {1, match + 0xbc, 0}, {1, match + 0xbe, 0}}),
            "pick locked");

    // The carrier's switch over many seeds: waiting out its countdown and the decision of the action routine.
    {
        const auto row = std::uint32_t(scenario.s.memory.read<1>(first + 0x105)), column = std::uint32_t(scenario.s.memory.read<1>(first + 0x106));
        for (std::uint32_t seed = 0; seed < 60; ++seed) for (const std::uint32_t wait : {0u, 1u}) for (const std::uint32_t at_ball : {0u, 1u})
            add(0x101d3f60, {{3, match}, {4, 1}},
                with(base(seed), {{1, match + 0x1de, wait}, {4, match + 0x3104, first}, {1, match + 0x791f, 4}, {4, match + 0xdff8, first},
                                  {1, match + 0x7928, 0}, {1, match + 0x1c1, 0}, {1, first + 0x2c, 0}, {1, first + 0x108, 5}, {1, first + 0x10a, 3},
                                  {1, match + 0x791d, at_ball ? row : 15}, {1, match + 0x791e, at_ball ? column : 15}, {4, match + 0x7c68, 0}}),
                "carrier switch");
    }
    // The taker of a restart for every state of the stopped play and either side.
    for (const std::uint32_t last : {0u, 1u}) for (const std::uint32_t stopped : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u}) for (const std::uint32_t seed : {0u, 1u, 2u, 5u})
        add(0x101d3f60, {{3, match}, {4, 1}},
            with(base(seed), {{1, match + 0x7928, stopped}, {1, match + 0x1c1, 0}, {1, match + 0x7924, last}, {4, match + 0xe000, 0}}),
            "choose taker, state " + std::to_string(stopped) + " last " + std::to_string(last));
}

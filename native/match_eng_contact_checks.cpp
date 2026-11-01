// Cases for the contact block: the injuries a contact causes, the hurt players leaving the pitch, the man of the
// match, the performance marks, the queue of contacts and the grid of players, over states a live match reaches
// thinly or never.
#include "guest_abi.hpp"
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("contact") {
    const auto match = scenario.match;
    const auto sp = std::uint32_t(scenario.s.r<4>(4));
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label, std::optional<double> f1 = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = f1, .label = std::move(label)});
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    // The per-position flag table of a side (0x7934 + side*4), one halfword per position.
    const auto flag_writes = [&](unsigned side, std::uint32_t flags) {
        std::vector<Write> writes;
        const auto table = scenario.r32(match + 0x7934 + side * 4);
        for (std::uint32_t position = 0; position <= 10; ++position) writes.push_back({2, table + position * 2, flags});
        return writes;
    };
    const auto every_player = [&](std::uint32_t at, unsigned size, std::uint32_t value) {
        std::vector<Write> writes;
        for (unsigned side = 0; side < 2; ++side)
            for (unsigned index = 0; index < 20; ++index) writes.push_back({size, scenario.player(side, index) + at, value});
        return writes;
    };

    // Wrappers: byte and halfword arguments with their high bits set.
    add(0x101f5050, {{3, match}, {4, 0x11223344}, {5, 0x55667788}, {6, 0xfffffe81}, {7, 0xabcdef12}, {8, 9}, {9, 10}, {10, 11}}, {}, "forward");
    add(0x101f50c0, {{3, match}, {4, 1}, {5, 0xfffffff3}, {6, 2}, {7, 3}}, {}, "forward half");
    // The shape of MATCH_PL's calls (0x1028829c): a player, an output below the frame, a byte kind.
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 3u, 7u, 10u, 19u}) for (const std::uint32_t kind : {1u, 2u, 6u, 7u})
        add(0x101f51e0, {{3, match}, {4, scenario.player(side, index)}, {5, sp - 0x800}, {6, 0x100 + kind}, {7, 0x200 + side}}, {}, "referee response");
    add(0x101f5180, {{3, match}, {4, 5}, {5, 6}, {6, 0x1ff}, {7, 0x2ff}, {8, 0x3ff}, {9, 0x44444}}, {}, "forward bytes more");
    add(0x101f5230, {{3, match}, {4, 0x1ff}, {5, 0x2ff}, {6, 0x3ff}, {7, 0x12345}}, {}, "forward bytes less");

    // The ball holder: every flag value the first test names, with the holder fields pointing at the player or not.
    for (const unsigned side : {0u, 1u}) {
        const auto player = scenario.player(side, 3);
        for (const std::uint32_t flags : {1u, 0u, 2u}) {
            for (const std::uint32_t holder : {0u, player}) {
                add(0x101f5280, {{3, match}, {4, player}}, join(flag_writes(side, flags), {{4, match + 0xe008, holder}, {4, match + 0xe004, holder}}), "holder");
                add(0x101f5280, {{3, match}, {4, player}}, join(flag_writes(side, flags), {{1, match + 0x7922, 5}, {1, match + 0x7923, 9}}), "side bytes");
            }
        }
    }

    // Injuries: kind, severity, the period's injury counter, the player's health and his record's grade.
    const auto culprit = scenario.player(1, 2);
    const std::uint32_t healths[] = {0x3f000000, 0x457a0000, 0x45abe000, 0x460ca000, 0x469c4000, 0xc0a00000, 0x45fa0000, 0x43480000};
    for (const unsigned side : {0u, 1u}) {
        const auto player = scenario.player(side, 3);
        const auto record = scenario.r32(player + 0x70);
        for (const std::uint32_t kind : {0xffu, 3u}) {
            for (const std::uint32_t severity : {10u, 50u, 51u, 100u, 0xe0u, 0u}) {
                for (const std::uint32_t counter : {0u, 4u, 7u}) {
                    for (const std::uint32_t health : healths) {
                        std::vector<Write> memory = {{2, match + 0xe0ba, counter}, {4, player + 0x50, health}};
                        for (const std::uint32_t other : {0u, culprit}) {
                            add(0x101f6940, {{3, match}, {4, kind}, {5, severity}, {6, player}, {7, other}}, memory, "injury");
                        }
                    }
                }
            }
        }
        if (record) {
            for (const std::uint32_t grade : {0u, 10u, 0xfbu, 0x7fu}) {
                for (const std::uint32_t health : healths) {
                    for (const std::uint32_t hurt : {0u, 1u}) {
                        add(0x101f6940, {{3, match}, {4, 3}, {5, 20}, {6, player}, {7, 0}},
                            {{1, record + 0x2d, grade}, {4, player + 0x50, health}, {1, player + 0x1c, hurt}, {2, match + 0xe0ba, 1}}, "injury grade");
                    }
                }
            }
        }
    }

    // Hurt players leaving the pitch: flagged players in every position state, at every clock the caps name.
    for (const std::uint32_t clock : {0x1efu, 0x3deu, 0x483u, 0x528u, 0x100u, 0x7ffu}) {
        for (const std::uint32_t hurt : {0u, 1u}) {
            for (const std::uint32_t position : {5u, 0xffu, 0xfdu, 0xfeu}) {
                for (const std::uint32_t culprit_set : {0u, 1u}) {
                    std::vector<Write> memory = {{2, match + 0x794c, clock}, {2, match + 0x7948, clock + 17}};
                    for (const unsigned side : {0u, 1u}) {
                        for (const unsigned index : {0u, 2u, 7u, 12u}) {
                            const auto player = scenario.player(side, index);
                            memory.push_back({1, player + 0x1d, 1});
                            memory.push_back({1, player + 0x1c, hurt});
                            memory.push_back({1, player + 0x19, position});
                            memory.push_back({1, player + 0x22, 0x70});
                            if (culprit_set) memory.push_back({4, player + 0x58, scenario.player(1 - side, 4)});
                        }
                    }
                    add(0x101f6c60, {{3, match}}, memory, "injured");
                    add(0x101f6c60, {{3, match}}, join(memory, {{1, match + 0x7aac + 0x19, 1}, {1, match + 0x7aac + 0xd8 + 0x19, 1}}), "injured statistics");
                }
            }
        }
    }
    add(0x101f6c60, {{3, match}}, {}, "nobody");

    // The man of the match: form and rating bytes, with absent players and the fixture's roster size.
    for (const std::uint32_t form : {0u, 100u, 0x7000u, 0x8000u, 0xffffu}) {
        for (const std::uint32_t weights : {0u, 1u, 7u, 0xffu, 0x80u}) {
            std::vector<Write> memory = every_player(0x36, 2, form);
            for (const auto at : {0x10u, 0xcu, 3u, 6u}) memory = join(memory, every_player(at, 1, weights));
            add(0x101f7100, {{3, match}}, memory, "man of the match");
            add(0x101f7100, {{3, match}}, join(memory, every_player(0x19, 1, 0xff)), "no one on");
            add(0x101f7100, {{3, match}}, join(join(memory, every_player(0x19, 1, 0xff)), every_player(0x20, 1, 4)), "reserves");
        }
    }
    for (unsigned index = 0; index < 20; index += 3) {
        const auto player = scenario.player(1, index);
        add(0x101f7100, {{3, match}}, {{2, scenario.player(0, 3) + 0x36, 500}, {2, player + 0x36, 700}, {1, player + 0x10, 30}}, "better player");
        add(0x101f7100, {{3, match}}, {{2, scenario.player(0, 3) + 0x36, 700}, {2, player + 0x36, 500}, {1, player + 0x10, 30}}, "worse key");
    }

    // The performance marks: every flag pattern, fitness above and below the line.
    const std::uint32_t flag_values[] = {0, 1, 2, 4, 8, 0x10, 0x20, 0x80, 0x800, 4 | 0x800, 4 | 0x80, 8 | 0x800, 8 | 0x80, 0x10 | 4, 0x30};
    for (const unsigned side : {0u, 1u}) {
        for (const std::uint32_t flags : flag_values) {
            for (const std::uint32_t fitness : {0u, 0x45fa0000u, 0x460ca000u, 0x469c4000u}) {
                std::vector<Write> memory = join(flag_writes(side, flags), every_player(0x50, 4, fitness));
                add(0x101f7280, {{3, match}, {4, side}}, memory, "stars");
                add(0x101f7280, {{3, match}, {4, side}}, join(memory, every_player(0x19, 1, 0xff)), "stars absent");
            }
        }
        add(0x101f7500, {{3, match}, {4, side}}, every_player(0x19, 1, 0xff), "all absent");
        add(0x101f7500, {{3, match}, {4, side}}, {}, "all");
    }

    // The queue of contacts: events built from the match's players, in each phase.
    for (const std::uint32_t phase : {0u, 1u, 2u}) {
        for (const std::uint32_t count : {0u, 1u, 3u}) {
            for (const std::uint32_t variant : {0u, 1u, 2u, 3u, 4u}) {
                std::vector<Write> memory = {{1, match + 0x792a, phase}, {1, match + 0x1bc, count}};
                for (unsigned i = 0; i < 3; ++i) {
                    const auto at = match + 0xb4 + i * 12;
                    memory.push_back({4, at, scenario.player(i & 1, 2 + i * 3)});
                    memory.push_back({4, at + 4, scenario.player(1 - (i & 1), 4 + i + variant)});
                    memory.push_back({1, at + 8, 3 + variant * 2 + i * 7});
                    memory.push_back({1, at + 9, variant * 5 + i});
                    memory.push_back({1, at + 10, 1 + variant + i * 4});
                }
                add(0x101f75a0, {{3, match}, {4, sp + 0x400}}, memory, "contacts");
                add(0x101f75a0, {{3, match}, {4, sp + 0x400}}, join(memory, {{1, match + 0x1e4, 1}, {1, match + 0x1e5, 1}}), "contacts sides");
                add(0x101f75a0, {{3, match}, {4, sp + 0x400}}, join(memory, flag_writes(variant & 1, 1)), "contacts flags");
            }
        }
    }
    // The sides' flag bytes alone, after the events.
    for (const std::uint32_t flags : {0u, 1u, 2u}) {
        add(0x101f75a0, {{3, match}, {4, sp + 0x400}}, {{1, match + 0x792a, 2}, {1, match + 0x1bc, 0}, {1, match + 0x1e4, 1}, {1, match + 0x1e5, flags}}, "sides flagged");
        add(0x101f75a0, {{3, match}, {4, sp + 0x400}}, {{1, match + 0x792a, 2}, {1, match + 0x1bc, 0}, {1, match + 0x1e4, flags}, {1, match + 0x1e5, 1}, {1, match + 0x7aac + 0x19, 1}}, "sides flagged stats");
    }

    // The list of pending events and the players' ratings: events are built in scratch memory below the stack.
    {
        struct Event { std::uint32_t kind, side, index, club; };
        const auto fixture = scenario.r32(match + 0x3118);
        const auto team_one = fixture ? scenario.r32(fixture + 0x1c) : 0, team_two = fixture ? scenario.r32(fixture + 0x20) : 0;
        const auto list = sp - 0x3000, nodes = list + 0x100, items = list + 0x200, chain = list + 0x400;
        const auto list_writes = [&](const std::vector<Event>& events) {
            std::vector<Write> writes = {{2, list, std::uint32_t(events.size())}, {4, list + 4, events.empty() ? 0 : nodes},
                                         {4, match + 0x3114, list}};
            for (std::size_t n = 0; n < events.size(); ++n) {
                const auto node = nodes + std::uint32_t(n) * 0x10, item = items + std::uint32_t(n) * 0x20;
                writes.push_back({4, node, item});
                writes.push_back({4, node + 8, n + 1 < events.size() ? node + 0x10 : 0});
                writes.push_back({4, item, events[n].index});
                writes.push_back({4, item + 4, events[n].club});
                writes.push_back({1, item + 0xd, events[n].side});
                writes.push_back({1, item + 0xe, 3});
                writes.push_back({4, item + 0x10, 7});
                writes.push_back({2, item + 0x14, events[n].kind});
            }
            return writes;
        };
        // The competition's chain of records (+0x94, +0x1c) both teams point to.
        const auto chain_writes = [&](std::uint32_t reach, std::uint32_t standing) {
            std::vector<Write> writes;
            for (const auto team : {team_one, team_two}) {
                if (!team) continue;
                writes.push_back({4, team + 0x94, chain});
            }
            writes.push_back({4, chain + 0x1c, chain + 0x40});
            writes.push_back({4, chain, 77});
            writes.push_back({1, chain + 0x40 + 0x34, reach});
            writes.push_back({1, chain + 0x40 + 0x37, 5});
            writes.push_back({1, fixture + 0x40, 1});
            (void)standing;
            return writes;
        };
        for (const std::uint32_t kind : {0x1eu, 0x82u, 6u, 0x10u, 0x1eu}) {
            for (const std::uint32_t side : {0u, 1u}) {
                for (const auto goals : {std::pair{0u, 0u}, std::pair{1u, 0u}, std::pair{0u, 1u}, std::pair{2u, 2u}}) {
                    for (const std::uint32_t reach : {3u, 9u}) {
                        for (const std::uint32_t index : {0u, 2u}) {
                            auto memory = join(list_writes({{kind, side, index, index + 1}, {kind, 1 - side, 1, 0}}), chain_writes(reach, 0));
                            memory.push_back({1, fixture + 0x43, goals.first});
                            memory.push_back({1, fixture + 0x44, goals.second});
                            if (team_one) memory.push_back({2, team_one + 0x1ec, 500 * (index + 1)});
                            if (team_two) memory.push_back({2, team_two + 0x1ec, 900});
                            add(0x101f84e0, {{3, match}}, memory, "events");
                            add(0x101f84e0, {{3, match}}, join(memory, {{4, chain + 0x30, 0}, {4, chain, 0}}), "events no record");
                        }
                    }
                }
            }
        }
        // Players: the three tests of the per-player events.
        for (const std::uint32_t value : {0u, 5u, 12u, 0x7fu, 0xffu}) {
            std::vector<Write> memory = join(every_player(0x20, 1, value), every_player(0x22, 1, 0xff));
            for (unsigned side = 0; side < 2; ++side) {
                for (unsigned index = 0; index < 20; ++index) {
                    const auto record = scenario.r32(scenario.player(side, index) + 0x6c);
                    if (!record) continue;
                    for (const auto at : {0x63u, 0x67u, 0x69u, 0x66u}) memory.push_back({1, record + at, value});
                }
            }
            add(0x101f84e0, {{3, match}}, memory, "players");
            add(0x101f84e0, {{3, match}}, join(memory, every_player(0x50, 4, 0x469c4000)), "players tired");
            add(0x101f84e0, {{3, match}}, join(join(memory, every_player(0x1a8, 4, 0x42c80000)), every_player(0x50, 4, 0x469c4000)), "players rested");
        }
        // The rating of the step: entry and exit minutes, the match clock and the flag.
        for (const std::uint32_t clock : {0u, 400u, 1000u}) {
            for (const std::uint32_t value : {0xffu, 0u, 30u}) {
                for (const std::uint32_t other : {0xffu, 20u}) {
                    std::vector<Write> memory = join(every_player(0x1f, 1, value), every_player(0x20, 1, other));
                    memory = join(memory, every_player(0x21, 1, other == 0xff ? 0xff : 40));
                    memory.push_back({2, match + 0x7948, clock});
                    add(0x101f84e0, {{3, match}}, memory, "ratings");
                    add(0x101f84e0, {{3, match}}, join(memory, {{1, match, 1}}), "ratings flagged");
                    add(0x101f84e0, {{3, match}}, join(memory, {{4, fixture + 0x18, 0}}), "ratings other competition");
                }
            }
        }
    }

    // The contact between two players: absent players, the flag, the holder and the other's action codes.
    for (const unsigned side : {0u, 1u}) {
        const auto pa = scenario.player(side, 3), pb = scenario.player(1 - side, 4);
        for (const std::uint32_t flag : {0u, 1u}) {
            for (const std::uint32_t code_a : {0u, 5u, 1u, 45u}) {
                for (const std::uint32_t code_b : {0u, 1u, 12u, 19u, 21u, 33u, 34u, 41u, 42u, 43u, 44u, 45u, 46u, 47u, 5u, 0x30u, 0xffffu}) {
                    for (const std::uint32_t holder : {0u, pa, pb}) {
                        add(0x101f5350, {{3, match}, {4, pa}, {5, code_a}, {6, pb}, {7, code_b}, {8, flag}}, {{4, match + 0xe000, holder}}, "describe");
                        add(0x101f5350, {{3, match}, {4, pa}, {5, code_a}, {6, pb}, {7, code_b}, {8, flag}}, {{4, match + 0xe000, holder}, {1, match + 0x791f, 4}}, "describe type 4");
                    }
                }
            }
            add(0x101f5350, {{3, match}, {4, 0}, {5, 5}, {6, pb}, {7, 1}, {8, flag}}, {}, "describe no first");
            add(0x101f5350, {{3, match}, {4, pa}, {5, 1}, {6, 0}, {7, 5}, {8, flag}}, {}, "describe no second");
            for (const std::uint32_t fatigue : {0u, 0x3f000000u, 0x469c4000u}) {
                add(0x101f5350, {{3, match}, {4, pa}, {5, 1}, {6, pb}, {7, 5}, {8, flag}}, {{4, pa + 0x50, fatigue}, {4, pb + 0x50, fatigue}}, "describe fitness");
            }
        }
    }

    // The grid of players: with and without a ball holder, and the ball in other cells.
    const auto grid = sp - 0x1800;
    for (const std::uint32_t holder_side : {2u, 0u, 1u}) {
        for (const std::uint32_t ball : {0u, 5u, 11u}) {
            std::vector<Write> memory = {{1, match + 0x791d, ball}, {1, match + 0x791e, ball ? ball - 1 : 0}};
            if (holder_side == 2) memory.push_back({4, match + 0xe000, 0});
            else memory.push_back({4, match + 0xe000, scenario.player(holder_side, 5)});
            add(0x101f7c10, {{3, match}, {4, grid}}, memory, "grid");
        }
    }
    add(0x101f7c10, {{3, match}, {4, grid}}, every_player(0x19, 1, 0xff), "grid empty");
    add(0x101f7c10, {{3, match}, {4, grid}}, every_player(0x19, 1, 3), "grid crowded");
}

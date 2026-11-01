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
                         std::string label, std::optional<double> f1 = {}, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = f1, .label = std::move(label),
                         .answers = std::move(answers)});
    };
    const auto r2 = std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(2)));
    // The value of a TOC slot: the address of a global (or of a table).
    const auto toc = [&](std::int32_t offset) { return scenario.r32(r2 + std::uint32_t(offset)); };
    // The random table: a draw is (word at the cursor + salt) mod bound, the cursor stepping two bytes before each draw.
    // Variant 0 draws zero throughout, 1 draws one; the others are pseudo-random small words with some large ones.
    const auto salt_at = toc(-0x7b90), cursor = scenario.r32(toc(-0x7b84));
    // Variants 100 + c draw the word c throughout.
    const auto stream = [&](unsigned variant) {
        std::vector<Write> writes = {{4, salt_at, 0}};
        std::uint32_t state = variant * 2654435761u + 12345u;
        for (unsigned k = 1; k <= 160; ++k) {
            state = state * 1664525u + 1013904223u;
            const std::uint32_t word = variant >= 100 ? variant - 100 : variant == 0 ? 0 : variant == 1 ? 1 : (state >> 24) % 10 == 0 ? (state >> 8) & 0xffff : (state >> 16) % 24;
            writes.push_back({2, cursor + 2 * k, word});
        }
        return writes;
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

    // Wrappers: byte and halfword arguments with their high bits set. The discipline routines they call belong to another
    // module and need the match's discipline object, which a snapshot lacks: both sides answer them.
    for (const std::uint32_t answer : {0u, 0x5a5au}) {
        add(0x101f5050, {{3, match}, {4, 0x11223344}, {5, 0x55667788}, {6, 0xfffffe81}, {7, 0xabcdef12}, {8, 9}, {9, 10}, {10, 11}}, {}, "forward", {},
            {{0x1021c260, answer}});
        add(0x101f50c0, {{3, match}, {4, 1}, {5, 0xfffffff3}, {6, 2}, {7, 3}}, {}, "forward half", {}, {{0x1021b860, answer}});
        add(0x101f50c0, {{3, match}, {4, 0xfeedbeef}, {5, 0x1234abcd}, {6, 0xffffffff}, {7, 0x80000000}}, {}, "forward half high", {}, {{0x1021b860, answer}});
        add(0x101f5110, {{3, match}, {4, 0x1234}, {5, 0x5678}, {6, 0x9abc}, {7, 0x1ff}, {8, 0x2ff}, {9, 0x3ff}, {10, 0xdef0}}, {{1, sp + 0x3b, 0x9c}},
            "forward ninth", {}, {{0x10221e00, answer}});
        add(0x101f5110, {{3, match}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}}, {{1, sp + 0x3b, 0xff}}, "forward ninth byte", {},
            {{0x10221e00, answer}});
        add(0x101f5180, {{3, match}, {4, 5}, {5, 6}, {6, 0x1ff}, {7, 0x2ff}, {8, 0x3ff}, {9, 0x44444}}, {}, "forward bytes more", {}, {{0x1021bec0, answer}});
    }
    // The shape of MATCH_PL's calls (0x1028829c): a player, an output below the frame, a byte kind.
    for (const unsigned side : {0u, 1u}) for (const unsigned index : {0u, 3u, 7u, 10u, 19u}) for (const std::uint32_t kind : {1u, 2u, 6u, 7u})
        add(0x101f51e0, {{3, match}, {4, scenario.player(side, index)}, {5, sp - 0x800}, {6, 0x100 + kind}, {7, 0x200 + side}}, {}, "referee response");
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
                        // The loss of health is a draw: small, middling and large draws reach each side of the limit.
                        for (const unsigned variant : {2u, 100u, 103u, 140u, 600u, 10100u, 30100u}) {
                            add(0x101f6940, {{3, match}, {4, 3}, {5, 20}, {6, player}, {7, 0}},
                                join({{1, record + 0x2d, grade}, {4, player + 0x50, health}, {1, player + 0x1c, hurt}, {2, match + 0xe0ba, 1}}, stream(variant)), "injury grade");
                        }
                    }
                }
            }
        }
    }

    // A heavy but not severe contact (loss of health up to 1250 against the 500 line) on a player already marked hurt.
    for (const unsigned side : {0u, 1u}) {
        const auto player = scenario.player(side, 3), record = scenario.r32(player + 0x70);
        for (const std::uint32_t severity : {40u, 45u, 50u}) {
            for (const std::uint32_t health : {0x469c4000u, 0x460ca000u, 0x45fa0000u}) {
                for (const std::uint32_t hurt : {0u, 1u}) {
                    for (const unsigned variant : {100u, 600u, 700u, 1000u, 1100u}) {
                        add(0x101f6940, {{3, match}, {4, 3}, {5, severity}, {6, player}, {7, 0}},
                            join({{4, player + 0x50, health}, {1, player + 0x1c, hurt}, {2, match + 0xe0ba, 1}, {1, record + 0x2d, 0}}, stream(variant)), "injury heavy");
                    }
                }
            }
        }
    }

    // After the third injury of a period most contacts do nothing: the first draw (below the counter) is non-zero and the
    // second (below 500) is zero, or both are zero.
    for (const std::uint32_t counter : {4u, 5u, 6u, 7u}) {
        for (const std::uint32_t kind : {0xffu, 3u}) {
            for (const std::uint32_t severity : {10u, 100u}) {
                for (const auto& words : {std::pair{1u, 0u}, std::pair{0u, 0u}, std::pair{1u, 1u}, std::pair{0u, 1u}}) {
                    std::vector<Write> memory = {{2, match + 0xe0ba, counter}, {4, salt_at, 0}, {2, cursor + 2, words.first}, {2, cursor + 4, words.second}};
                    for (unsigned k = 3; k <= 12; ++k) memory.push_back({2, cursor + 2 * k, 1});
                    add(0x101f6940, {{3, match}, {4, kind}, {5, severity}, {6, scenario.player(0, 3)}, {7, 0}}, memory, "injury later");
                }
            }
        }
    }

    // A side whose statistics name a person who manages it: the last person on file, with his flag byte set.
    const auto human_side = [&](unsigned side) {
        const auto count = scenario.r32(toc(-0x7be4)), person = scenario.r32(toc(-0x7be0)) + (count - 1) * 0x70;
        const auto stats = match + 0x7aac + side * 0xd8;
        return std::vector<Write>{{4, stats + 0x20, person}, {4, person, count - 1}, {1, person + 0x6b, 1}, {1, stats + 0x1b, 1}};
    };

    // Hurt players leaving the pitch: flagged players in every position state, at every clock the caps name.
    for (const std::uint32_t clock : {0x1efu, 0x3deu, 0x483u, 0x528u, 0x100u, 0x7ffu}) {
        for (const std::uint32_t hurt : {0u, 1u}) {
            for (const std::uint32_t position : {5u, 0xffu, 0xfdu, 0xfeu}) {
                for (const std::uint32_t culprit_set : {0u, 1u}) {
                  for (const std::int32_t lag : {17, -60}) {
                    if (lag < 0 && (position != 5 || !culprit_set)) continue;
                    std::vector<Write> memory = {{2, match + 0x794c, clock}, {2, match + 0x7948, clock + std::uint32_t(lag)}};
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
                    // A side managed by a person (stats +0x20 names the last person on file, flag +0x1b set): its bookkeeping is skipped.
                    if (lag > 0 && culprit_set) {
                        auto managed = memory;
                        for (const unsigned side : {0u, 1u}) managed = join(managed, human_side(side));
                        add(0x101f6c60, {{3, match}}, managed, "injured managed", {}, {{0x10147e70, 0}, {0x101481d0, 1}});
                    }
                  }
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
                add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, memory, "contacts");
                add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, join(memory, {{1, match + 0x1e4, 1}, {1, match + 0x1e5, 1}}), "contacts sides");
                add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, join(memory, flag_writes(variant & 1, 1)), "contacts flags");
            }
        }
    }
    // One queued event over the judgement of the discipline routine (outcomes 1..4 come out of it with the arguments the
    // routine passes): the player's yellow-card byte (+0x24) and position, the event's severity (+8) and kind (+9), and a
    // side managed by a person (whose bookkeeping is skipped).
    for (const unsigned b8 : {2u, 12u, 100u}) {
        for (const unsigned b9 : {3u, 0x2du, 0x60u}) {
            for (const unsigned p24 : {0u, 1u}) {
                for (const unsigned p19 : {5u, 0xffu}) {
                    for (const unsigned human : {0u, 1u}) {
                        for (const unsigned variant : {0u, 1u, 3u, 4u, 5u, 6u}) {
                            const auto at = match + 0xb4, subject = scenario.player(variant & 1, 2 + variant);
                            std::vector<Write> memory = {{1, match + 0x792a, 1}, {1, match + 0x1bc, 1}, {4, at, subject},
                                                         {4, at + 4, scenario.player(1 - (variant & 1), 4)}, {1, at + 8, b8}, {1, at + 9, b9},
                                                         {1, at + 10, 3 + variant}, {1, subject + 0x24, p24}, {1, subject + 0x19, p19},
                                                         {1, match + 0x1e4, 1}, {1, match + 0x1e5, 1}};
                            if (human) for (const unsigned side : {0u, 1u}) memory = join(memory, human_side(side));
                            add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, join(memory, stream(variant)), "contact judged", {},
                                {{0x10147e70, 0}, {0x101481d0, 1}});
                        }
                    }
                }
            }
        }
    }
    // The sides' flag bytes alone, after the events.
    for (const std::uint32_t flags : {0u, 1u, 2u}) {
        add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, {{1, match + 0x792a, 2}, {1, match + 0x1bc, 0}, {1, match + 0x1e4, 1}, {1, match + 0x1e5, flags}}, "sides flagged");
        add(0x101f75a0, {{3, match}, {4, scenario.player(0, 5)}}, {{1, match + 0x792a, 2}, {1, match + 0x1bc, 0}, {1, match + 0x1e4, flags}, {1, match + 0x1e5, 1}, {1, match + 0x7aac + 0x19, 1}}, "sides flagged stats");
    }

    // The list of pending events and the players' ratings: events are built in scratch memory below the stack.
    {
        struct Event { std::uint32_t kind, side, index, club; };
        const auto fixture = scenario.r32(match + 0x3118);
        const auto team_one = fixture ? scenario.r32(fixture + 0x1c) : 0, team_two = fixture ? scenario.r32(fixture + 0x20) : 0;
        const auto list = sp - 0x3000, nodes = list + 0x100, items = list + 0x200, chain = list + 0x400, sink = list + 0x500;
        // The list the step's reports are appended to: the snapshot's match has none (its global is 0).
        const auto sink_global = scenario.r32(std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(2))) - 0x6428);
        const std::vector<Write> sink_writes = {{4, sink_global, sink}, {2, sink, 0}, {4, sink + 4, 0}, {4, sink + 8, 0}};
        const auto list_writes = [&](const std::vector<Event>& events) {
            std::vector<Write> writes = {{2, list, std::uint32_t(events.size())}, {4, list + 4, events.empty() ? 0 : nodes},
                                         {4, match + 0x3114, list}};
            writes.insert(writes.end(), sink_writes.begin(), sink_writes.end());
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
        // The pending events name people and clubs by their index in the world's tables: the match's own players and teams.
        const auto people_table = scenario.r32(toc(-0x7be0)), clubs_table = scenario.r32(toc(-0x7be8));
        const auto person_of = [&](unsigned side, unsigned index) { return (scenario.r32(scenario.player(side, index) + 0x6c) - people_table) / 0x70; };
        const auto club_of = [&](std::uint32_t team) { return (team - clubs_table) / 0x248; };
        const auto standing_at = [&](unsigned side, unsigned index) { return scenario.r32(scenario.player(side, index) + 0x70) + 0xa; };
        const std::uint32_t standings[] = {0, 100, 8000, 20000};
        for (const std::uint32_t kind : {0x1eu, 0x82u, 6u, 0x10u}) {
            for (const std::uint32_t side : {0u, 1u}) {
                for (const auto goals : {std::pair{0u, 0u}, std::pair{1u, 0u}, std::pair{0u, 1u}}) {
                    for (const std::uint32_t reach : {3u, 9u}) {
                        for (const unsigned variant : {0u, 1u, 2u, 3u, 4u, 5u}) {
                            // The chain of records: complete, without the team's record, or the record without its competition.
                            for (const unsigned mode : {0u, 1u, 2u}) {
                                if (mode && (kind != 0x1e || variant > 2)) continue;
                                const std::uint32_t index = variant % 2 ? 2 : 4;
                                auto memory = join(list_writes({{kind, side, person_of(side, index), club_of(side ? team_two : team_one)},
                                                                {kind, 1 - side, person_of(1 - side, 3), club_of(side ? team_one : team_two)}}),
                                                   chain_writes(reach, 0));
                                memory.push_back({1, fixture + 0x43, goals.first});
                                memory.push_back({1, fixture + 0x44, goals.second});
                                if (team_one) memory.push_back({2, team_one + 0x1ec, variant == 5 ? 30000u : 500 * (variant + 1)});
                                if (team_two) memory.push_back({2, team_two + 0x1ec, variant == 5 ? 30000u : 900u});
                                memory.push_back({2, standing_at(side, index), variant == 5 ? 8000u : standings[variant % 4]});
                                memory.push_back({2, standing_at(1 - side, 3), standings[(variant + 1) % 4]});
                                if (mode == 1) { memory.push_back({4, team_one + 0x94, 0}); memory.push_back({4, team_two + 0x94, 0}); }
                                if (mode == 2) memory.push_back({4, chain + 0x1c, 0});
                                // The events of kind 6 and the others look for the player: his yellow-card and fitness bytes.
                                for (const unsigned s2 : {0u, 1u})
                                    for (const unsigned k : {2u, 3u, 4u}) {
                                        const auto player = scenario.player(s2, k);
                                        memory.push_back({1, player + 0x24, variant % 3 == 0 ? 2u : 0u});
                                        memory.push_back({1, player + 0x23, variant % 2});
                                        memory.push_back({1, player + 0x1b, variant < 4 ? 3u : 9u});
                                        memory.push_back({1, player + 0x17, 5});
                                    }
                                add(0x101f84e0, {{3, match}}, join(memory, stream(variant)), "events");
                                if (mode == 0 && variant < 2) add(0x101f84e0, {{3, match}}, join(memory, {{4, chain + 0x30, 0}, {4, chain, 0}}), "events no record");
                                if (mode == 0 && variant == 0) add(0x101f84e0, {{3, match}}, join(memory, {{4, match + 0x3114, 0}}), "events no list");
                            }
                        }
                    }
                }
            }
        }
        // Without a fixture the step goes on to read the teams through it: both sides fault alike (no returned comparison exists).
        add(0x101f84e0, {{3, match}}, join(sink_writes, {{4, match + 0x3118, 0}}), "events no fixture");
        // Players: the three tests of the per-player events.
        for (const std::uint32_t value : {0u, 5u, 12u, 0x7fu, 0xffu}) {
            std::vector<Write> memory = join(join(every_player(0x20, 1, value), every_player(0x22, 1, 0xff)), sink_writes);
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
                    std::vector<Write> memory = join(join(every_player(0x1f, 1, value), every_player(0x20, 1, other)), sink_writes);
                    memory = join(memory, every_player(0x21, 1, other == 0xff ? 0xff : 40));
                    memory.push_back({2, match + 0x7948, clock});
                    add(0x101f84e0, {{3, match}}, memory, "ratings");
                    add(0x101f84e0, {{3, match}}, join(memory, {{1, match, 1}}), "ratings flagged");
                    add(0x101f84e0, {{3, match}}, join(memory, {{4, fixture + 0x18, 0}}), "ratings other competition");
                }
            }
        }

        // The report of each player in the rating loop: the tests of a promising player's event (kind 0xa) and of the morale
        // event (kind 0xb), each failed in turn from a state that passes them all. `fail` names the test to fail (0: none).
        if (fixture && team_one && team_two) {
            const auto record_object = scenario.r32(toc(-0x7b7c)), links = scenario.r32(toc(-0x7b9c)), competitions = scenario.r32(toc(-0x7b54));
            // The registry of the snapshot has no entries: one is built in scratch memory.
        const auto base = list + 0x700, people_count = scenario.r32(toc(-0x7be4));
            const auto report_world = [&](unsigned fail, unsigned answers_mode) {
                const std::uint32_t id_one = scenario.r32(team_one), id_two = scenario.r32(team_two);
                std::vector<Write> w = sink_writes;
                w.insert(w.end(), {{4, record_object, base}, {1, fixture + 0x40, fail == 5 ? 0u : 1u}, {4, fixture + 0x18, scenario.r32(team_one + 0x3c)},
                                        {4, base + 4, fail == 6 ? 0x12345u : id_one}, {4, team_one + 0x90, 0}, {4, team_two + 0x90, 0},
                                        {4, fixture + 0x14, fail == 8 ? 0u : scenario.r32(fixture + 0x14) ? scenario.r32(fixture + 0x14) : base},
                                        {1, base + 0x44, fail == 12 ? 0u : 5u}});
                (void)id_two;
                // The registry's own competition is the fixture's in the snapshot; failing test 9 keeps that.
                w.push_back({4, competitions + 4, fail == 9 ? scenario.r32(fixture + 0x14) : 0u});
                // The rating routine reads the competition unless the fixture is marked unrated.
                if (fail == 8) w.push_back({4, fixture, 0xfffffffeu});
                if (fail == 4) { w.push_back({4, team_one, 0x7fffffffu}); w.push_back({4, team_two, 0x7fffffffu}); }
                for (unsigned side = 0; side < 2; ++side) {
                    for (unsigned index = 0; index < 20; ++index) {
                        const auto player = scenario.player(side, index);
                        const auto record = scenario.r32(player + 0x6c), detail = scenario.r32(player + 0x70);
                        w.push_back({1, player + 0x1f, 0x10});
                        w.push_back({1, player + 0x20, fail == 10 ? 0xffu : 0x30u});
                        w.push_back({1, player + 0x22, fail == 11 ? 0x30u : 0xffu});
                        w.push_back({4, player + 0x50, fail == 16 ? 0x44fa0000u : 0x469c4000u});
                        w.push_back({4, player + 0x1a8, 0x42c80000});
                        if (fail == 1) w.push_back({4, player + 0x6c, 0});
                        if (fail == 2) w.push_back({4, player + 0x70, 0});
                        if (!record) continue;
                        w.push_back({4, record, fail == 3 ? people_count + 3 : scenario.r32(record)});
                        w.push_back({4, links + scenario.r32(record) * 0x64, 0});
                        w.push_back({1, record + 0x63, fail == 18 ? 5u : 15u});
                        w.push_back({1, record + 0x67, fail == 17 ? 9u : 3u});
                        w.push_back({1, record + 0x69, fail == 19 ? 12u : 9u});
                        w.push_back({1, record + 0x66, 7});
                        if (detail) w.push_back({2, detail + 0xa, fail == 13 ? 100u : 5000u});
                        if (fail == 7 && side == 0 && index < 11) w.push_back({4, team_one + 0x90, record});
                        if (fail == 20) w.push_back({2, player + 0x32, 0x7000});
                        if (answers_mode & 1) w.push_back({4, team_one + 0x74, team_two});
                    if (answers_mode & 2) w.push_back({4, team_one + 0x78, team_two});
                    }
                }
                return w;
            };
            // Answers of the relation routines (another module): town, rivals, relatives and enemies.
            const std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>> relations = {
                {{0x100d94d0, 0}, {0x100d9480, 0}, {0x100da050, 0}, {0x100da0b0, 0}}, {{0x100d94d0, 1}, {0x100d9480, 0}, {0x100da050, 1}, {0x100da0b0, 0}},
                {{0x100d94d0, 0}, {0x100d9480, 1}, {0x100da050, 0}, {0x100da0b0, 1}}};
            for (const unsigned fail : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 16u, 17u, 18u, 19u, 20u}) {
                for (const unsigned variant : {2u, 3u, 4u, 5u, 6u, 7u}) {
                    if (fail && variant > 4) continue;
                    const unsigned count = fail ? 1 : 4;
                    for (unsigned mode = 0; mode < count; ++mode)
                        add(0x101f84e0, {{3, match}}, join(report_world(fail, mode), stream(variant)), "report " + std::to_string(fail), {}, relations[mode % 3]);
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

    // The descriptors: the names object gives a player a descriptor record (an index into the descriptor table, whose byte
    // is the kind of contact he tends to cause) and a level. The snapshot's players have none, so the lookup routine
    // (another module) is answered and the entries written. `kinds` are the descriptor bytes of the two players.
    {
        const auto names_object = scenario.r32(toc(-0x7ba0));
        const auto name_base = scenario.r32(names_object), record_base = scenario.r32(names_object + 4), descriptors = toc(-0x64d4);
        const auto descriptor_writes = [&](std::uint32_t player, unsigned k, std::uint32_t kind, unsigned level) {
            const auto record = scenario.r32(player + 0x6c);
            const auto entry = name_base + scenario.r32(record) * 0x24;
            const std::uint32_t level_bytes[3][2] = {{0, 0}, {0, 5}, {0xff, 0}};
            return std::vector<Write>{{4, entry + 0x10, k}, {1, record_base + k * 0x38, k + 1}, {1, descriptors + (k + 1) * 12 + 1, kind},
                                      {1, entry + 0x14, level_bytes[level % 3][0]}, {1, entry + 0x15, level_bytes[level % 3][1]}};
        };
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> lookup = {{0x10187a40, 0}};
        for (const unsigned side : {0u, 1u}) {
            const auto pa = scenario.player(side, 3), pb = scenario.player(1 - side, 4);
            for (const std::uint32_t flag : {0u, 1u}) {
                for (const std::uint32_t kind : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 0u, 11u, 0xffu}) {
                    for (const bool second : {true, false}) {
                        if (flag && !second) continue;
                        for (const std::uint32_t code : {5u, 1u}) {
                            for (const std::uint32_t state : {0u, 4u}) {
                                if (side && (code != 5 || state)) continue;
                                for (unsigned variant = 0; variant < 8; ++variant) {
                                    auto memory = join(descriptor_writes(pa, 0, kind, variant), second ? descriptor_writes(pb, 1, kind, variant / 2) : std::vector<Write>{});
                                    memory = join(memory, stream(variant));
                                    memory.push_back({1, match + 0x791f, state});
                                    add(0x101f5350, {{3, match}, {4, pa}, {5, code}, {6, second ? pb : 0}, {7, code}, {8, flag}}, memory, "descriptor", {}, lookup);
                                }
                            }
                        }
                    }
                }
                for (const unsigned constant : {3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u})
                    for (const std::uint32_t kind : {0u, 11u})
                        for (const bool second : {true, false})
                            add(0x101f5350, {{3, match}, {4, pa}, {5, 1}, {6, second ? pb : 0}, {7, 1}, {8, flag}},
                                join(join(descriptor_writes(pa, 0, kind, 0), second ? descriptor_writes(pb, 1, kind, 0) : std::vector<Write>{}), stream(100 + constant)),
                                "descriptor constant", {}, lookup);
                // The first of two players absent: the second reads the first's record (a quirk), so both absent is the case that returns.
                for (unsigned variant = 0; variant < 4; ++variant)
                    add(0x101f5350, {{3, match}, {4, 0}, {5, 5}, {6, 0}, {7, 5}, {8, flag}}, stream(variant), "describe nobody", {}, lookup);
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
    // The ball holder shares a cell with other players: the first cell holds him and two team-mates and two opponents, and
    // the draws are chosen so that a player's roll lands on the holder's middle ninth (retried) and elsewhere.
    {
        const auto cell = match + 0xa08;
        const auto holder = scenario.player(0, 5);
        const auto fill = [&] {
            std::vector<Write> writes = {{4, cell, holder}, {4, cell + 4, scenario.player(0, 6)}, {4, cell + 8, scenario.player(0, 7)},
                                         {4, cell + 0x2c, scenario.player(1, 4)}, {4, cell + 0x30, scenario.player(1, 5)}};
            writes.push_back({1, cell + 0x58, 3});
            writes.push_back({1, cell + 0x59, 2});
            writes.push_back({4, match + 0xe000, holder});
            writes.push_back({1, holder + 0x105, 0});
            writes.push_back({1, holder + 0x106, 0});
            return writes;
        };
        const std::vector<std::vector<std::uint32_t>> draws = {{0, 1, 1, 0, 2, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0},
                                                               {1, 1, 1, 0, 2, 1, 1, 2, 2, 1, 1, 2, 2, 0, 0, 0},
                                                               {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
                                                               {1, 0, 0, 1, 1, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0}};
        for (const auto& words : draws) {
            std::vector<Write> memory = fill();
            memory.push_back({4, salt_at, 0});
            for (unsigned k = 0; k < words.size(); ++k) memory.push_back({2, cursor + 2 * (k + 1), words[k]});
            for (unsigned k = words.size(); k < 160; ++k) memory.push_back({2, cursor + 2 * (k + 1), k % 3});
            add(0x101f7c10, {{3, match}, {4, grid}}, memory, "grid holder shares");
        }
    }
    // No grid: the original reports an assertion and goes on to store through the null pointer.
    add(0x101f7c10, {{3, match}, {4, 0}}, {}, "grid null");
}

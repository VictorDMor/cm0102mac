// MATCH_MAN.CPP checker cases for the match block (match_man_match.hpp).
#include "match_man_checks.hpp"

namespace cm::aot::match_man_checks {
namespace {
constexpr std::uint32_t pick = 0x1020d7d0, manage = 0x1020ef70, apply = 0x102104d0;
}

MATCH_MAN_CASES("match") {
    if (!scenario.match) return;
    for (unsigned side = 0; side < 2; ++side) {
        const auto record = scenario.side(side);
        const std::string name = "side " + std::to_string(side);
        cases.push_back({.entry = manage, .registers = {{3, record}, {4, 0}}, .memory = {{1, record + 0x19, 3}, {1, scenario.r32(record + 0x3c) + 0x4e, 0x70}, {1, record + 0x1b, 0}}, .label = "manage, written: allowance 3, bench of 7, kind 0, registry check answered 0", .answers = {{0x1014c2e0, 1}, {0x10147e70, 0}}});
        cases.push_back({.entry = manage, .registers = {{3, record}, {4, 0}}, .memory = {{1, record + 0x19, 3}, {1, scenario.r32(record + 0x3c) + 0x4e, 0x70}, {1, record + 0x1b, 0}}, .label = "manage, written: allowance 3, bench of 7, kind 0, registry check real", .answers = {{0x1014c2e0, 1}}});
        cases.push_back({.entry = apply, .registers = {{3, record}}, .label = "apply lineup, " + name});
        // The pick answering a count (only when 0x1020d7d0 isn't under check, so with --only 0x102104d0):
        // the announcement (events 0x2007) of the full eleven, with the match's events on.
        cases.push_back({.entry = apply, .registers = {{3, record}}, .memory = {{1, scenario.match + 0x7931, 1}},
                         .label = "apply lineup, pick answered 1 (under --only), " + name, .answers = {{pick, 1}}});
        for (std::uint32_t flag : {0u, 1u})
            cases.push_back({.entry = manage, .registers = {{3, record}, {4, flag}}, .label = "manage, flag " + std::to_string(flag) + ", " + name});
        // The persons of the first n players, in a list built in the scratch area.
        for (unsigned count : {11u, 14u, 5u, 0u}) {
            const auto list = scratch_base + 0x100 * (side + 1);
            Case base{.entry = pick, .registers = {{3, record}, {4, list}, {5, count}, {6, 0x8c1}, {7, 2}}};
            for (unsigned n = 0; n < count && n < 20; ++n)
                base.memory.push_back({4, list + n * 4, scenario.r32(scenario.player(side, n) + 0x6c)});
            for (std::uint32_t mode : {2u, 1u}) {
                auto one = base;
                one.registers[4].second = mode;
                one.label = "pick, " + std::to_string(count) + " candidates, mode " + std::to_string(mode) + ", " + name;
                cases.push_back(one);
            }
        }
        // Paths the snapshot's data does not take, reached by writing values the routine reads.
        const auto match = scenario.r32(record + 0x14), club = scenario.r32(record + 0x34);
        const auto list = scratch_base + 0x100 * (side + 1), registry = scenario.global(-0x7a6c);
        const auto last_person = scenario.person(scenario.people() - 1);
        const auto pick_case = [&](const std::string& label, std::uint32_t mode, std::vector<Write> memory,
                                   std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            Case one{.entry = pick, .registers = {{3, record}, {4, list}, {5, 11}, {6, 0x8c1}, {7, mode}}, .memory = std::move(memory),
                     .label = "pick, " + label + ", " + name, .answers = std::move(answers)};
            for (unsigned n = 0; n < 11; ++n) one.memory.push_back({4, list + n * 4, scenario.r32(scenario.player(side, n) + 0x6c)});
            cases.push_back(std::move(one));
        };
        // The substitution manager: its allowance, bench, mode bytes and the events flag.
        const auto extras = scenario.r32(record + 0x3c);
        const auto manage_case = [&](const std::string& label, std::vector<Write> memory, std::uint32_t flag = 0) {
            cases.push_back({.entry = manage, .registers = {{3, record}, {4, flag}}, .memory = std::move(memory),
                             .label = "manage, " + label + ", " + name});
        };
        for (std::uint32_t allowed : {0u, 1u, 2u, 3u, 5u})
            manage_case("allowance " + std::to_string(allowed), {{1, record + 0x19, allowed}});
        for (std::uint32_t mode : {1u, 9u, 2u})
            for (std::uint32_t gap : {0u, 100u, 3000u}) {
                // The club's rating over the opponent's (halfwords at +0x1ec) decides the rolls.
                manage_case("extras mode " + std::to_string(mode) + " rating gap " + std::to_string(gap),
                            {{1, extras + 0x3a, mode}, {1, record + 0x1b, 0}, {2, club + 0x1ec, 20000 + gap}, {2, scenario.r32(record + 0x38) + 0x1ec, 20000}});
            }
        for (std::uint32_t kind : {1u, 0u})
            manage_case("kind " + std::to_string(kind), {{1, record + 0x1b, kind}});
        manage_case("mode 3, extras word -1", {{1, record + 0x1b, 0}, {1, record + 0x1c, 3}, {4, extras, 0xffffffff}, {1, extras + 0x3a, 4}});
        manage_case("mode 3, extras word 0", {{1, record + 0x1b, 0}, {1, record + 0x1c, 3}, {4, extras, 0}, {1, extras + 0x3a, 4}});
        manage_case("mode 3, extras word 1", {{1, record + 0x1b, 0}, {1, record + 0x1c, 3}, {4, extras, 1}, {1, extras + 0x3a, 4}});
        manage_case("human manager flag", {{1, record + 0x18, 1}});
        manage_case("events off", {{1, match + 0x7931, 0}});
        manage_case("events on", {{1, match + 0x7931, 1}});
        manage_case("empty bench", {{1, extras + 0x4e, 0}});
        manage_case("big bench", {{1, extras + 0x4e, 0x70}, {1, record + 0x19, 5}});
        manage_case("rating of players zero", {{2, record + 0, 0}, {1, record + 0x19, 5}}, 1);
        for (std::uint32_t competition : {0x1efu, 0x3deu, 0x483u, 0x528u})
            manage_case("competition " + std::to_string(competition), {{2, match + 0x794c, competition}, {2, match + 0x7948, 6000}, {1, record + 0x19, 5}});
        // Substitutions that happen. The snapshot's sides have no allowance, no bench and no
        // events; these memory writes give an allowance, a bench of 7 (the extras nibble) and
        // tired players on the pitch (fitness +0x36), so the assignment brings bench players on.
        const auto substitutions = [&](std::uint32_t allowed, std::uint32_t fitness = 100) {
            std::vector<Write> memory{{1, record + 0x19, allowed}, {1, extras + 0x4e, 0x70}};
            for (unsigned n = 0; n < 11; ++n) memory.push_back({2, scenario.player(side, n) + 0x36, fitness});
            return memory;
        };
        const auto with = [](std::vector<Write> memory, std::initializer_list<Write> more) {
            memory.insert(memory.end(), more);
            return memory;
        };
        // Form (+0x32), morale (+0x34) of the players on the pitch (0..10) and on the bench (11..19).
        const auto field = [&](std::vector<Write> memory, unsigned first, unsigned last, std::uint32_t offset, std::uint32_t value) {
            for (unsigned n = first; n <= last; ++n) memory.push_back({2, scenario.player(side, n) + offset, value});
            return memory;
        };
        for (std::uint32_t allowed : {1u, 2u, 3u, 5u})
            manage_case("written: " + std::to_string(allowed) + " substitutions allowed, bench of 7, tired pitch", substitutions(allowed));
        {
            const auto events = with(substitutions(3), {{1, match + 0x7931, 1}});
            manage_case("written: substitutions with events, snapshot forms", events);
            manage_case("written: substitutions with events, pitch form -50", field(events, 0, 10, 0x32, 0xffce));
            manage_case("written: substitutions with events, bench form -50", field(events, 11, 19, 0x32, 0xffce));
            manage_case("written: substitutions with events, forms 50", field(field(events, 0, 19, 0x32, 50), 0, 19, 0x34, 50));
            manage_case("written: substitutions with events, forms -12", field(events, 0, 19, 0x32, 0xfff4));
            manage_case("written: substitutions with events, pitch morale 300", field(events, 0, 10, 0x34, 300));
            manage_case("written: substitutions with events, bench morale 300", field(events, 11, 19, 0x34, 300));
            // Morale 230 with fitness 100: 230 + 100 / 30 - 233 = 0, the zero arm; bench morale 0.
            manage_case("written: substitutions with events, zero spreads",
                        field(field(events, 0, 10, 0x34, 230), 11, 19, 0x34, 0));
            manage_case("written: substitutions with events, morale -300", field(events, 0, 19, 0x34, 0xfed4));
            manage_case("written: substitutions with events, substituted bytes 0",
                        field(field(events, 0, 19, 0x34, 300), 0, 10, 0x22, 0) /* halfword write covers +0x22/+0x23 */);
        }
        // A player sent off without the flag (position -2, +0x1e clear) is considered (0x102895a0).
        manage_case("written: player 0 sent off unflagged, substitutions",
                    with(substitutions(3), {{1, scenario.player(side, 0) + 0x19, 0xfe}, {1, scenario.player(side, 0) + 0x1e, 0}}));
        manage_case("written: player 3 sent off unflagged, substitutions, flag 1",
                    with(substitutions(3), {{1, scenario.player(side, 3) + 0x19, 0xfe}, {1, scenario.player(side, 3) + 0x1e, 0}}), 1);
        // The minutes byte capped by competition (+0x794c) and minute (+0x7948).
        for (std::uint32_t competition : {0x1efu, 0x3deu, 0x483u, 0x528u, 0x100u})
            for (std::uint32_t minutes : {1400u, 300u})
                manage_case("written: substitutions, competition " + std::to_string(competition) + " minute " + std::to_string(minutes),
                            with(substitutions(3), {{2, match + 0x794c, competition}, {2, match + 0x7948, minutes}}));
        // The flags: kind 0 with the club's rating (+0x1ec) far above the opponent's.
        for (std::uint32_t mode : {4u, 1u, 9u})
            for (std::uint32_t gap : {7000u, 5000u})
                manage_case("written: kind 0, extras mode " + std::to_string(mode) + ", rating gap " + std::to_string(gap) + ", substitutions",
                            with(substitutions(3), {{1, record + 0x1b, 0}, {1, extras + 0x3a, mode}, {2, club + 0x1ec, 20000 + gap},
                                                    {2, scenario.r32(record + 0x38) + 0x1ec, 20000}}));
        // Mode 3: the extras word indexes the table at TOC -0x7b5c (0x10452ed0 asked, or answered).
        for (std::uint32_t word : {0u, 1u}) {
            const auto mode3 = with(substitutions(3), {{1, record + 0x1b, 0}, {1, record + 0x1c, 3}, {4, extras, word}, {1, extras + 0x3a, 4}});
            const auto label = "written: mode 3, extras word " + std::to_string(word) + ", substitutions";
            manage_case(label, mode3);
            for (std::uint32_t answer : {0u, 1u})
                cases.push_back({.entry = manage, .registers = {{3, record}, {4, 0}}, .memory = mode3,
                                 .label = "manage, " + label + ", 0x10452ed0 answers " + std::to_string(answer) + ", " + name,
                                 .answers = {{0x10452ed0, answer}}});
        }
        manage_case("written: mode 3, extras word -1, substitutions",
                    with(substitutions(3), {{1, record + 0x1b, 0}, {1, record + 0x1c, 3}, {4, extras, 0xffffffff}, {1, extras + 0x3a, 4}}));
        // The registry checks of the club's manager: 0x1014c2e0, 0x10147e70 and 0x10149df0 answered
        // (generated routines), kind 0 written; the manager pointer (club +0x90) is the snapshot's.
        const std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>> registry_answers{
            {{0x1014c2e0, 1}},
            {{0x1014c2e0, 1}, {0x10147e70, 0}},
            {{0x1014c2e0, 1}, {0x10147e70, 1}, {0x10149df0, 0}},
            {{0x1014c2e0, 1}, {0x10147e70, 1}, {0x10149df0, 1}}};
        for (std::size_t n = 0; n < registry_answers.size(); ++n) {
            const auto label = " (registry answers " + std::to_string(n) + "), kind 0 written, " + name;
            cases.push_back({.entry = manage, .registers = {{3, record}, {4, 0}}, .memory = with(substitutions(3), {{1, record + 0x1b, 0}}),
                             .label = "manage, substitutions" + label, .answers = registry_answers[n]});
            cases.push_back({.entry = apply, .registers = {{3, record}}, .memory = {{1, record + 0x1b, 0}},
                             .label = "apply lineup" + label, .answers = registry_answers[n]});
        }
        // The pick returns a count when the match flag is set (written, undone): the announcement of the
        // pick (events 0x2007) in the lineup application, with the match's events flag on and off.
        for (std::uint32_t flag : {0u, 1u})
            for (std::uint32_t events : {0u, 1u}) {
                cases.push_back({.entry = apply, .registers = {{3, record}}, .memory = {{1, match, flag}, {1, match + 0x7931, events}},
                                 .label = "apply lineup, written: match flag " + std::to_string(flag) + " events " + std::to_string(events) + ", " + name});
                cases.push_back({.entry = apply, .registers = {{3, record}}, .memory = {{1, match, flag}, {1, match + 0x7931, events}, {1, record + 0x1b, 0}},
                                 .label = "apply lineup, written: match flag " + std::to_string(flag) + " events " + std::to_string(events) + " kind 0, " + name});
            }
        // A bench nibble of 10 reaches a record past the side's 20 (side 0: side 1's first player,
        // written to position -1): its person is no player of the side, so 0x102107c0 finds none
        // and the assertion at 0x1020f65c is reported.
        if (side == 0) {
            auto memory = substitutions(3);
            memory.push_back({1, extras + 0x4e, 0xa0});
            memory.push_back({1, scenario.player(1, 0) + 0x19, 0xff});
            manage_case("written: bench nibble 10, side 1's first player at -1 (person not found)", memory);
        }
        pick_case("match flag set (random pick)", 2, {{1, match, 1}});
        pick_case("match flag set, wide draw", 2, {{1, match, 1}, {4, registry + 0x5b8, 70000}});
        pick_case("match flag set, no candidates", 2, {{1, match, 1}, {4, registry + 0x5b8, 0}, {4, registry + 0x5c0, 0}});
        pick_case("mode 3 (stored lineup only)", 3, {});
        pick_case("human manager of the club", 2, {{1, record + 0x1b, 0}, {4, club + 0x90, last_person}, {1, last_person + 0x6b, 1}});
        // The human manager's stored lineup: the registry checks answered (generated routines), and
        // 0x101466c0 answered 0 (no lineup record: the assertion at 0x1020e86c, then exit).
        const std::vector<Write> human{{1, record + 0x1b, 0}, {4, club + 0x90, last_person}, {1, last_person + 0x6b, 1}};
        pick_case("human manager, registry check answered 0", 2, human, {{0x10147e70, 0}});
        pick_case("human manager, registry answers 1/0", 2, human, {{0x10147e70, 1}, {0x10149df0, 0}});
        pick_case("human manager, registry answers 1/1", 2, human, {{0x10147e70, 1}, {0x10149df0, 1}});
        pick_case("human manager, registry answers 1/1, no lineup record (assertion)", 2, human,
                  {{0x10147e70, 1}, {0x10149df0, 1}, {0x101466c0, 0}});
        pick_case("manager not in the last 16",2, {{1, record + 0x1b, 0}, {4, club + 0x90, scenario.person(0)}, {1, scenario.person(0) + 0x6b, 1}});
        pick_case("no manager", 2, {{1, record + 0x1b, 0}, {4, club + 0x90, 0}});
        pick_case("no coach", 2, {{4, record + 0x24, 0}});
        for (std::uint32_t formation : {0u, 3u, 4u, 6u})
            pick_case("formation mode " + std::to_string(formation), 2, {}, {{0x1020c5d0, formation}});
    }
}
}

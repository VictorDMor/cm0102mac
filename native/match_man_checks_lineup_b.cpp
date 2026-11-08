// MATCH_MAN.CPP checker cases for the lineup_b block: the computer's lineup choice
// (0x102065f0) and the scaled random value (0x10209d10).
#include "guest_abi.hpp"
#include "match_man_checks.hpp"
#include <algorithm>

namespace cm::aot::match_man_checks {
namespace {
constexpr std::uint32_t choose = 0x102065f0, scaled = 0x10209d10;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t person_size = 0x70;

// The routines the choice dispatches and a case answers (generated ones; the DATABASE ones run).
constexpr std::uint32_t state_query = 0x1014c2e0, registry_c = 0x101481d0, registry_d = 0x101484b0, next_manager = 0x1014c5f0, registry_b_entry = 0x101466c0;

struct Call {
    std::uint32_t side, match, manager, squad_list, players, slot, club, rival, fixture;
};

// The call's stack parameters live in the caller's frame: sp+0x38 and sp+0x3c.
// With `environment`, the world routines the lineup routines (0x10206ea0, 0x10206d80) reach and a
// match snapshot has no world for are answered on both sides, as lineup_a's cases answer them
// (generated routines): the choice then returns, and its tail after the lineup calls is compared.
bool environment = false;
const Answers environment_answers{{0x100e7920, 0}, {0x102b7c70, 0}, {0x103f93e0, 0}, {0x103ecef0, 0}, {0x103f98d0, 0}};

void add_choice(const Scenario& scenario, std::vector<Case>& cases, const Call& call, std::vector<Write> memory,
                Answers answers, std::string label, std::uint32_t bench_out = scratch_base + 0x100) {
    if (environment) {
        for (const auto& answer : environment_answers)
            if (std::none_of(answers.begin(), answers.end(), [&](const auto& given) { return given.first == answer.first; }))
                answers.push_back(answer);
        label += " (world routines answered)";
    }
    const auto sp = guest::Cpu(scenario.s).sp();
    memory.push_back({4, sp + 0x38, call.fixture});
    memory.push_back({4, sp + 0x3c, bench_out});
    memory.push_back({1, bench_out, 0x77});
    cases.push_back({.entry = choose,
                     .registers = {{3, call.side}, {4, call.match}, {5, call.manager}, {6, call.squad_list}, {7, call.players},
                                   {8, call.slot}, {9, call.club}, {10, call.rival}},
                     .memory = std::move(memory), .label = label, .answers = std::move(answers)});
}

void choice_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto people = scenario.global(-0x7be4);
    const auto person_at = [&](unsigned index) { return scenario.global(-0x7be0) + index * person_size; };
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto name = "side " + std::to_string(n) + ": ";
        // The real call's arguments, as the setup passes them: the side's own fields.
        const Call real{side, scenario.r32(side + 0x14), scenario.r32(side + 0x20), scenario.r32(side + 0x2c),
                        scenario.r32(side + 0x28), n /* the slot: 0 home, 1 away, as the setup passes it */, scenario.r32(side + 0x34),
                        scenario.r32(side + 0x38), scenario.r32(side + 0x3c)};
        const auto fixture = real.fixture;
        const auto manager_a = person_at(people - 1), manager_b = person_at(people / 2);
        add_choice(scenario, cases, real, {}, {}, name + "real");
        // Answers for the generated callees on the own-lineup path (the club query, the registry
        // checks, the next manager) and for the two lineup routines.
        const Answers none{};
        for (std::uint32_t state : {0u, 1u, 0x100u, 0x80u})
            for (std::uint32_t second : {0u, 1u, 0x100u})
                for (std::uint32_t third : {0u, 1u, 0x100u})
                    {
                        add_choice(scenario, cases, real, {{1, scenario.match, 0}},
                                   {{state_query, state}, {registry_c, second}, {registry_d, third}, {next_manager, manager_a}},
                                   name + "own path state " + std::to_string(state) + " c " + std::to_string(second) + " d " +
                                       std::to_string(third));
                    }
        // The fixture decides kind 1 (the club is the home or away side with its hold byte clear).
        for (std::uint32_t home_hold : {0u, 1u, 0x80u, 0x100u})
            for (std::uint32_t away_hold : {0u, 1u, 0x80u})
                for (unsigned who = 0; who < 3; ++who) {
                    const auto club = who == 0 ? real.club : who == 1 ? real.rival : 0x1234u;
                    Call call = real;
                    call.club = club;
                    add_choice(scenario, cases, call,
                               {{1, fixture + 0x40, home_hold}, {1, fixture + 0x41, away_hold},
                                {4, fixture + 0x1c, who == 2 ? club : real.club}, {4, fixture + 0x20, who == 1 ? real.rival : real.club}},
                               {{state_query, 1}, {registry_c, 1}, {registry_d, 1}, {next_manager, manager_a}},
                               name + "kind from fixture holds " + std::to_string(home_hold) + "," + std::to_string(away_hold) + " club " +
                                   std::to_string(who));
                }
        // The own path past the squad copy: 0x101466c0 (registry_b, generated) answered with a record
        // (the squad list, readable) so the copy runs, the club's competition check (0x100c7d00) and the
        // club flag (0x100ce250) answered both ways, kind 1 and 0 written through the fixture's holds. Answered
        // routines are the ones the dispatcher reaches; a difference would show a wrong answer.
        for (std::uint32_t kind_one : {1u, 0u})
            for (std::uint32_t flag : {0u, 1u})
                for (std::uint32_t check : {0u, 1u})
                    for (std::uint32_t second : {0u, 1u})
                        for (std::uint32_t third : {0u, 1u}) {
                            std::vector<Write> memory{{1, scenario.match, 0}, {4, fixture + 0x1c, real.club}, {1, fixture + 0x40, kind_one ? 0u : 1u},
                                                      {4, fixture + 0x20, real.rival}, {1, fixture + 0x41, 1}};
                            add_choice(scenario, cases, real, memory,
                                       {{state_query, 1}, {registry_b_entry, real.squad_list}, {0x100ce250, flag}, {0x100c7d00, check},
                                        {registry_c, second}, {registry_d, third}, {next_manager, manager_a}},
                                       name + "own path, copy runs, kind " + std::to_string(kind_one) + " flag " + std::to_string(flag) +
                                           " check " + std::to_string(check) + " c " + std::to_string(second) + " d " + std::to_string(third));
                        }
        // The match has started, the club is absent, or not a computer club.
        add_choice(scenario, cases, real, {{1, scenario.match, 1}}, {}, name + "match started");
        add_choice(scenario, cases, real, {{1, scenario.match, 0x80}}, {}, name + "match started 0x80");
        // The other path (the club does not pick): the manager's id against the people count.
        for (std::uint32_t id : {0u, people - 0x11, people - 0x10, people - 1, people, people + 1, 0xffffffffu})
            for (std::uint32_t flag : {0u, 1u, 0x80u}) {
                add_choice(scenario, cases, {side, real.match, manager_a, real.squad_list, real.players, real.slot, real.club, real.rival, fixture},
                           {{4, manager_a, id}, {1, manager_a + 0x6b, flag}, {1, scenario.match, 1}},
                           {{next_manager, manager_b}},
                           name + "manager id " + std::to_string(id) + " flag " + std::to_string(flag));
            }
        add_choice(scenario, cases, {side, real.match, 0, real.squad_list, real.players, real.slot, real.club, real.rival, fixture},
                   {{1, scenario.match, 1}}, {}, name + "no manager");
        add_choice(scenario, cases, {side, real.match, manager_b, real.squad_list, real.players, real.slot, real.club, real.rival, fixture},
                   {{1, scenario.match, 1}, {4, manager_b, people - 1}, {1, manager_b + 0x6b, 1}}, {{next_manager, 0}},
                   name + "human manager, no replacement");
        // The count of eligible players and the bench bytes: what the lineup returned and the nibbles.
        {
            for (std::uint32_t bench : {0x00u, 0x05u, 0x0fu, 0x35u, 0x70u, 0xf0u, 0xffu, 0x80u, 0x8fu}) {
                add_choice(scenario, cases, real, {{1, scenario.match, 1}, {1, fixture + 0x4e, bench}}, {},
                           name + "bench " + std::to_string(bench));
                add_choice(scenario, cases, real, {{1, scenario.match, 0}, {1, fixture + 0x4e, bench}},
                           {{state_query, 1}, {registry_c, 0}, {registry_d, 0}},
                           name + "own, bench " + std::to_string(bench));
            }
        }
        // Players without a person, or with ids around the people count.
        const auto first_player = real.players;
        for (std::uint32_t id : {0u, people - 1, people, 0x7fffffffu, 0xffffffffu})
            add_choice(scenario, cases, real, {{1, scenario.match, 1}, {4, scenario.r32(first_player + 0x6c), id}}, {},
                       name + "first player's person id " + std::to_string(id));
        add_choice(scenario, cases, real, {{1, scenario.match, 1}, {4, first_player + 0x6c, 0}, {4, first_player + 0x1c4 + 0x6c, 0}},
                   {}, name + "two players without a person");
        // Absent arguments: each asserts.
        for (unsigned which = 0; which < 5; ++which) {
            Call call = real;
            (which == 0 ? call.match : which == 1 ? call.squad_list : which == 2 ? call.players : which == 3 ? call.club : call.rival) = 0;
            add_choice(scenario, cases, call, {{1, scenario.match, 1}}, {}, name + "absent argument " + std::to_string(which));
        }
        // The slot byte and the squad copy: the squad's record absent.
        for (std::uint32_t slot : {0u, 1u, 0x101u, 0xffu})
            add_choice(scenario, cases, {side, real.match, real.manager, real.squad_list, real.players, slot, real.club, real.rival, fixture}, {},
                       {{state_query, 1}, {registry_c, 1}}, name + "slot " + std::to_string(slot));
    }
}

// The globals the random reads: the seed, the table, its limit and the cursor (TOC -0x7b90, -0x7b8c,
// -0x7b88, -0x7b84; the table's address, the limit's variable, the cursor's variable).
void random_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto seed = scenario.toc(-0x7b90), limit = scenario.toc(-0x7b88), cursor = scenario.toc(-0x7b84), table = scenario.toc(-0x7b8c);
    const std::int32_t bounds[] = {0, 1, -1, 2, 7, 100, -100, 0xfffe, 0xffff, -0xffff, 0x10000, -0x10000, 0x10001, -0x10001, 65536 * 3,
                                   655350, 655351, 1000000, -1000000, 20000000, 0x19999999, 0x1999999a, -0x1999999a, 0x7fffffff,
                                   std::int32_t(0x80000000), std::int32_t(0x80000001), 0x1999999b, 0x33333334};
    for (const auto bound : bounds) {
        const auto label = "bound " + std::to_string(bound);
        cases.push_back({.entry = scaled, .registers = {{3, std::uint32_t(bound)}}, .label = label});
        // The cursor at the table's last entry: the next step refills the seed from the RNG.
        cases.push_back({.entry = scaled, .registers = {{3, std::uint32_t(bound)}}, .memory = {{4, cursor, scenario.r32(limit)}},
                         .label = label + ", cursor at the limit"});
        cases.push_back({.entry = scaled, .registers = {{3, std::uint32_t(bound)}}, .memory = {{4, cursor, scenario.r32(limit) - 2}},
                         .label = label + ", cursor one before the limit"});
        for (std::uint32_t value : {0u, 1u, 0xffffu, 0x8000u, 0x12345678u, 0xffffffffu})
            cases.push_back({.entry = scaled, .registers = {{3, std::uint32_t(bound)}}, .memory = {{4, seed, value}},
                             .label = label + ", seed " + std::to_string(value)});
        for (std::uint32_t entry : {0u, 1u, 0xffffu, 0x8000u, 0xabcdu})
            cases.push_back({.entry = scaled, .registers = {{3, std::uint32_t(bound)}},
                             .memory = {{2, scenario.r32(cursor) + 2, entry}, {4, seed, 0xffffu}},
                             .label = label + ", table entry " + std::to_string(entry)});
    }
    (void)table;
}

MATCH_MAN_CASES("lineup_b") {
    random_cases(scenario, cases);
    if (scenario.match) {
        environment = true;
        choice_cases(scenario, cases);
        environment = false;
        choice_cases(scenario, cases);
    }
}
}
}

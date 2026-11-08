// MATCH_MAN.CPP checker cases for the rating_c block (match_man_rating_c.hpp, 0x10211e70).
#include "guest_abi.hpp"
#include "match_man_checks.hpp"
#include <array>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include "match_man_adapter.hpp"

namespace cm::aot::match_man_checks {
namespace {
constexpr std::uint32_t entry = 0x10211e70;
constexpr std::uint32_t person_size = 0x70;
std::uint32_t person_at(const Scenario& scenario, unsigned index) { return scenario.global(-0x7be0) + index * person_size; }
std::uint32_t stack_pointer(const Scenario& scenario) { return std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1))); }

// The components (r7) the original tests, by the bits the listing reads, and the roles (r6) its position
// groups read; the places' words and role bytes the selection really passes (traced from the career).
constexpr std::uint32_t component_bits[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000,
                                            0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x800000, 0x1000000, 0x2000000};
constexpr std::uint32_t every_component = 0x3bfffff;
constexpr std::uint32_t roles[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 | 0x10, 0x800 | 0x20, 0x84, 0x100, 0x6, 0x30, 0x1000 | 0x8, 0x200 | 0x2, 0x400 | 0x40, 0x808, 0x88, 0x104, 0x404, 0x204, 0x410, 0x220, 0x140, 0x440, 0x110};
constexpr std::uint32_t place_second[] = {0x15552221, 0x56982221, 0x155a4221, 0x155a2221, 0x259a2221, 0x95522221, 0x1, 0x4, 0x8, 0x10, 0x8000, 0x2000, 0x40, 0x100, 0x200000, 0x20000, 0x40000000, 0x80000000, 0x80000, 0x800000, 0x2000000, 0x8000000, 0x20000000, 0x1 | 0x2000};
constexpr std::uint32_t place_first[] = {0xa, 0x6, 0, 0xa, 0x6};
constexpr std::uint32_t place_role[] = {0x11, 0x12, 0x14, 0x1, 0x2, 0x4, 0x8, 0x3, 0x0};
constexpr std::uint32_t place_extra[] = {0x22aa2, 0x2, 0x4, 0x8, 0x100, 0x0, 0xffff};
constexpr std::uint32_t modes[] = {0xca1ca, every_component, every_component & ~0x1000u, every_component & ~0x10000u, every_component & ~0x30000u, every_component & ~0x800u,
                                   every_component & ~0x800u & ~0x10000u, 0x800 | 0x10000, 0x800 | 0x200000 | 0x20000, 0x4000 | 0x2000000 | 0x1000, 0x40 | 0x10 | 0x20 | 0x4 | 0x80 | 0x100,
                                   0x40000 | 0x2 | 0x8 | 0x400 | 0x200, 0x100000 | 0x1000000, 0x8000 | 0x1000 | 0x1, 0x8000 | 0x2 | 0x4 | 0x40000};

struct Call {
    std::uint32_t person = 0, record = 0, role = 1, flags = 0, club = 0, side = 0, judge = 0;
    std::uint32_t floor = 0xffffffff, first = 0xa, second = 0x15552221, role_word = 0x11, extra = 0x22aa2, rating = 0, human = 0;
};

Case make(const Scenario& scenario, const Call& call, const std::string& label, std::vector<Write> writes = {},
          std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
    const auto sp = stack_pointer(scenario);
    writes.push_back({4, sp + 0x38, call.floor});
    writes.push_back({4, sp + 0x3c, call.first});
    writes.push_back({4, sp + 0x40, call.second});
    writes.push_back({4, sp + 0x44, call.role_word});
    writes.push_back({4, sp + 0x48, call.extra});
    writes.push_back({4, sp + 0x4c, call.rating});
    writes.push_back({4, sp + 0x50, call.human});
    return {.entry = entry,
            .registers = {{3, call.person}, {4, call.person ? scenario.r32(call.person + 0x14) : 0}, {5, call.record}, {6, call.role}, {7, call.flags}, {8, call.club}, {9, call.side}, {10, call.judge}},
            .memory = std::move(writes),
            .label = label + ", role " + std::to_string(call.role) + ", flags " + std::to_string(call.flags),
            .answers = std::move(answers)};
}

}

MATCH_MAN_CASES("rating_c") {
    std::uint32_t state = 0x7654321;
    const auto next = [&] { state = state * 1664525u + 1013904223u; return state >> 8; };
    const auto relation = scratch_base + 0x1000, counts = scratch_base + 0x2000, found = scratch_base + 0x3000, humans = scratch_base + 0x4000;
    const auto answers_for = [&](unsigned k) {
        return std::vector<std::pair<std::uint32_t, std::uint32_t>>{
            {0x103ecb00, counts}, {0x103ecef0, k % 5 == 4 ? 0u : 1u}, {0x10082f20, k % 4 == 3 ? 0u : found},
            {0x10187650, std::uint32_t(std::array<std::uint32_t, 7>{0xf830, 0xfc18, 600, 0x3e9, 1500, 0x1f5, 100}[k % 7])},
            {0x100da050, k % 3 == 0}, {0x100da0b0, k % 3 == 1}, {0x100d9580, k % 4 == 0}, {0x100d9520, k % 4 == 1}};
    };
    const auto relation_writes = [&](unsigned k) {
        const std::uint32_t kinds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x21, 0x33};
        return std::vector<Write>{{1, relation + 0x60, kinds[k % std::size(kinds)]},
                                  {4, relation + 0x50, k % 3 == 0 ? 0x2808u : k % 3 == 1 ? 0x10u : 0u},
                                  {1, relation + 0x4c, (k * 37) & 0xff}, {1, relation + 0x44, (k * 11) & 0xff},
                                  {1, relation + 0x45, (k * 5) & 0xff}, {1, relation + 0x46, (k * 7) & 0xff},
                                  {2, counts + 4, k % 7 == 0 ? 0u : 5 + k}, {2, counts + 6, k % 7 == 0 ? 0u : 40 - k}, {4, found + 4, 1}};
    };
    std::vector<std::uint32_t> clubs;
    for (unsigned index = 0; index < 400; index += 17) clubs.push_back(scenario.club(index));
    if (scenario.match) {
        // The match's own calls: each side's players on the side's record, the side's judge (+0x20) and club (+0x34).
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            const auto judge = scenario.r32(record + 0x20), club = scenario.r32(record + 0x34);
            for (unsigned index = 0; index < 20; ++index) {
                const auto player = scenario.player(side, index);
                const auto person = scenario.r32(player + 0x6c);
                if (!person) continue;
                for (unsigned k = 0; k < 6; ++k) {
                    Call call{.person = person, .record = player, .role = roles[(index + k) % std::size(roles)], .flags = modes[(index * 3 + k) % std::size(modes)],
                              .club = club, .side = record, .judge = k % 2 ? judge : 0, .floor = k % 3 == 0 ? 0xffffffffu : k % 3 == 1 ? 120u : 971u,
                              .first = place_first[k % std::size(place_first)], .second = place_second[(index + k * 5) % std::size(place_second)],
                              .role_word = place_role[(index + k) % std::size(place_role)], .extra = place_extra[(index + k) % std::size(place_extra)],
                              .rating = (index + k) % 4 ? roles[(index + k * 3) % std::size(roles)] : 0u};
                    cases.push_back(make(scenario, call, "match side " + std::to_string(side) + " player " + std::to_string(index)));
                }
            }
        }
        // A match whose clock word is not the special 0x1ef, with the side's manager a person among the last 16 of the
        // table flagged at +0x6b: the original's four calls about the human manager run.
        {
            const auto people = scenario.people();
            for (unsigned side = 0; side < 2; ++side) {
                const auto record = scenario.side(side), club = scenario.r32(record + 0x34);
                for (unsigned index = 0; index < 20; index += 2) {
                    const auto player = scenario.player(side, index);
                    const auto person = scenario.r32(player + 0x6c);
                    if (!person) continue;
                    for (unsigned k = 0; k < 6; ++k) {
                        const auto manager = scenario.person(people - 1 - (index + k) % 14);
                        std::vector<Write> writes{{2, scenario.match + 0x794c, k % 2 ? 0u : 0x1efu}, {4, record + 0x7acc, manager},
                                                  {1, manager + 0x6b, k % 3 ? 1u : 0u}, {1, player + 0x27, side},
                                                  {1, player + 0x19, k % 3 == 0 ? 0xffu : k % 3 == 1 ? 12u : 0xfdu}, {1, player, k % 2 ? 5u : 20u},
                                                  {2, player + 0x36, k % 2 ? 200u : 0xffffu}, {1, scenario.match + 0x7ac7 + side * 0xd8, k % 2},
                                                  {1, scenario.r32(person + 0x14) + 0xf, k % 4 < 2 ? 5u : 1u}, {2, scenario.match + 0x7948, (k / 3) % 2 ? 0x3000u : 100u}};
                        Call call{.person = person, .record = player, .role = roles[(index + k) % std::size(roles)], .flags = 0x1 | 0x800 | modes[k % std::size(modes)],
                                  .club = club, .side = record, .judge = 0, .floor = 0xffffffffu, .second = place_second[(index + k) % std::size(place_second)]};
                        cases.push_back(make(scenario, call, "human manager " + std::to_string(k), writes));
                    }
                }
            }
        }
        // The human manager's list (16 entries of person and slot) with the person among them.
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            const auto club = scenario.r32(record + 0x34);
            for (unsigned index = 0; index < 20; index += 3) {
                const auto player = scenario.player(side, index);
                const auto person = scenario.r32(player + 0x6c);
                if (!person) continue;
                for (unsigned k = 0; k < 12; ++k) {
                    std::vector<Write> writes;
                    for (unsigned entry_index = 0; entry_index < 16; ++entry_index) {
                        writes.push_back({4, humans + entry_index * 8, entry_index == (index + k) % 16 ? scenario.r32(person) : 0x7fff0000u + entry_index});
                        writes.push_back({4, humans + entry_index * 8 + 4, k % 3 == 0 ? 0xffffffffu : k % 3 == 1 ? 0u : 1u + entry_index});
                    }
                    Call call{.person = person, .record = k % 2 ? player : 0, .role = roles[(index + k) % std::size(roles)], .flags = modes[(k * 2) % std::size(modes)] | 0x1,
                              .club = club, .side = k % 4 ? record : 0, .judge = 0, .floor = 0xffffffffu,
                              .second = place_second[(index + k) % std::size(place_second)], .rating = 0, .human = humans};
                    cases.push_back(make(scenario, call, "human list " + std::to_string(k), writes));
                }
            }
        }
        return;
    }
    const auto count = scenario.global(-0x7be4);
    std::vector<std::uint32_t> players, judges;
    for (unsigned index = 0; index < count && (players.size() < 600 || judges.size() < 100); ++index) {
        const auto person = person_at(scenario, index);
        if (scenario.r32(person + 0x14) && players.size() < 600) players.push_back(person);
        if (scenario.r32(person + 0x1c) && judges.size() < 100) judges.push_back(person);
    }
    if (players.empty() || judges.empty() || clubs.empty()) return;
    // Pairs spread over the tables: each role with each mode, places and judges.
    unsigned n = 0;
    for (const auto role : roles)
        for (const auto mode : modes)
            for (unsigned k = 0; k < 4; ++k, ++n) {
                Call call{.person = players[(n * 37 + 3) % players.size()], .role = role, .flags = mode, .club = clubs[(n * 5) % clubs.size()],
                          .judge = k % 2 ? judges[(n * 11 + 1) % judges.size()] : 0, .floor = k % 3 == 0 ? 0xffffffffu : k % 3 == 1 ? 120u : 971u,
                          .first = place_first[n % std::size(place_first)], .second = place_second[n % std::size(place_second)],
                          .role_word = place_role[n % std::size(place_role)], .extra = place_extra[n % std::size(place_extra)],
                          .rating = n % 3 ? roles[(n * 7) % std::size(roles)] : 0u};
                cases.push_back(make(scenario, call, "pair " + std::to_string(n)));
            }
    // The person's own club and the paths the answers open (the relation and the game's opinion).
    for (unsigned k = 0; k < 200; ++k) {
        const auto person = players[(k * 53) % players.size()];
        const auto own = scenario.r32(person + 0x24);
        auto writes = relation_writes(k);
        writes.push_back({4, person + 0x24, k % 2 ? own : clubs[k % clubs.size()]});
        if (k % 3 == 0) { writes.push_back({2, person + 0x5c, 16 + k % 26}); writes.push_back({1, person + 0x5e, k % 40}); }
        if (k % 4 == 1) writes.push_back({2, person + 0x4e, 0x7ff0});
        Call call{.person = person, .role = roles[k % std::size(roles)], .flags = modes[k % std::size(modes)], .club = k % 2 ? own : clubs[(k * 3) % clubs.size()],
                  .judge = judges[k % judges.size()], .floor = k % 5 == 0 ? 300u : 0xffffffffu, .first = place_first[k % std::size(place_first)],
                  .second = place_second[k % std::size(place_second)], .role_word = place_role[k % std::size(place_role)], .extra = place_extra[k % std::size(place_extra)],
                  .rating = k % 3 ? roles[(k * 5) % std::size(roles)] : 0u};
        cases.push_back(make(scenario, call, "answered " + std::to_string(k), writes, k % 2 ? answers_for(k) : decltype(answers_for(0)){}));
    }
    // The relation record of kind 3 (0x10082f90 answered with the scratch row, flag 0x8000 set, 0x100 clear) whose
    // second lookup (0x103ecb00) finds nothing: the call ends the lookup and the callee's r6 reaches the return,
    // where the original holds the ceiling (100, the lha r6,0xb0(r1)) that it loaded before the call. Found in
    // the live replay (call 607000 of the career: person 0x21213b50, role 0x84, flags 0xda0ca).
    for (unsigned k = 0; k < 48; ++k) {
        const auto person = players[(k * 29 + 5) % players.size()];
        auto writes = relation_writes(k);
        for (auto& write : writes) if (write.address == relation + 0x60) write.value = 3;
        const std::uint32_t kind_flags[] = {0xda0ca, 0x8000, 0x8000 | 0x4, 0xda0ca | 0x1000, 0x8000 | 0x10000};
        auto answers = answers_for(k);
        for (auto& answer : answers) if (answer.first == 0x103ecb00) answer.second = k % 3 == 2 ? counts : 0u;
        answers.push_back({0x10082f90, relation});
        Call call{.person = person, .role = roles[k % std::size(roles)], .flags = kind_flags[k % std::size(kind_flags)], .club = clubs[(k * 7) % clubs.size()],
                  .judge = k % 2 ? judges[k % judges.size()] : 0, .floor = 0xffffffffu, .second = place_second[k % std::size(place_second)]};
        cases.push_back(make(scenario, call, "relation kind 3 " + std::to_string(k), writes, answers));
    }
    // The person's own club, as a free agent (his club word zeroed), and with a human manager's list.
    for (unsigned k = 0; k < 160; ++k) {
        const auto person = players[(k * 41 + 9) % players.size()];
        const auto own = scenario.r32(person + 0x24);
        std::vector<Write> writes;
        if (k % 2) writes.push_back({4, person + 0x24, 0});
        if (k % 4 >= 2) {
            for (unsigned entry_index = 0; entry_index < 16; ++entry_index) {
                writes.push_back({4, humans + entry_index * 8, entry_index == k % 16 ? scenario.r32(person) : 0x7fff0000u + entry_index});
                writes.push_back({4, humans + entry_index * 8 + 4, k % 3 == 0 ? 0xffffffffu : 1u + entry_index});
            }
        }
        Call call{.person = person, .role = roles[(k * 3) % std::size(roles)], .flags = every_component | (k % 3 ? 0u : 0x800u), .club = own ? own : clubs[k % clubs.size()],
                  .judge = judges[k % judges.size()], .floor = 0xffffffffu, .second = place_second[k % std::size(place_second)],
                  .rating = k % 3 ? 0u : roles[(k * 5) % std::size(roles)], .human = k % 4 >= 2 ? humans : 0u};
        cases.push_back(make(scenario, call, "free agent or human " + std::to_string(k), writes));
    }
    // Random everything: component sets, roles, places, ages, caps, the goalkeeper byte, clubs, judges, answers.
    for (unsigned m = 0; m < 1200; ++m) {
        std::uint32_t flags = 0;
        for (const auto bit : component_bits) if (next() % 3 == 0) flags |= bit;
        if (m % 2) flags |= 0x1000 | 0x2000000 | 0x4000 | 0x8000;
        const auto person = players[next() % players.size()], club = clubs[next() % clubs.size()], judge = judges[next() % judges.size()];
        const auto data = scenario.r32(person + 0x14);
        std::uint32_t role = 0, rating = 0;
        for (const std::uint32_t bit : {1u, 2u, 4u, 8u, 0x10u, 0x20u, 0x40u, 0x80u, 0x100u, 0x200u, 0x400u, 0x800u, 0x1000u}) {
            if (next() % 4 == 0) role |= bit;
            if (next() % 5 == 0) rating |= bit;
        }
        std::vector<Write> writes{{2, person + 0x5c, 16 + next() % 26}, {1, person + 0x5e, next() % 50}};
        if (next() % 4 == 0) writes.push_back({1, data + 0xf, 20});
        if (next() % 3 == 0) writes.push_back({4, person + 0x24, club});
        if (next() % 6 == 0) writes.push_back({4, person + 0x24, 0});
        if (next() % 3 == 0) writes.push_back({2, person + 0x4e, 0x7ff0});
        if (next() % 3 == 0) writes.push_back({2, club + 0x1ec, 200 + next() % 9000});
        if (next() % 5 == 0) writes.push_back({1, data + 0x1b, next() % 12});
        if (next() % 5 == 0) writes.push_back({1, data + 0x36, next() % 12});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (next() % 2) {
            answers = answers_for(next() % 50);
            const auto extra = relation_writes(next() % 50);
            writes.insert(writes.end(), extra.begin(), extra.end());
        }
        Call call{.person = person, .role = role, .flags = flags, .club = club, .judge = next() % 3 ? judge : 0,
                  .floor = std::array<std::uint32_t, 6>{0xffffffffu, 0xffffffffu, 0, 100, 300, 971}[next() % 6],
                  .first = place_first[next() % std::size(place_first)], .second = next() % 3 ? place_second[next() % std::size(place_second)] : next(),
                  .role_word = next() % 3 ? place_role[next() % std::size(place_role)] : next() & 0xff, .extra = next() % 3 ? place_extra[next() % std::size(place_extra)] : next(),
                  .rating = rating};
        cases.push_back(make(scenario, call, "fuzz " + std::to_string(m), writes, answers));
    }
    // No person, and a person that is not a player.
    cases.push_back(make(scenario, Call{.person = 0, .flags = every_component, .club = clubs[0]}, "no person"));
    for (std::uint32_t index = 0; index < count && index < 3000; ++index) {
        const auto person = person_at(scenario, index);
        if (!scenario.r32(person + 0x14)) {
            auto item = make(scenario, Call{.person = person, .flags = every_component, .club = clubs[0]}, "no player data");
            cases.push_back(std::move(item));
            break;
        }
    }
}

// Coverage cases for paths the cases above leave: each from real state, with the memory writes named in its
// label (the checker undoes them after the case).
MATCH_MAN_CASES("rating_c paths") {
    const auto lookups = scenario.global(-0x7b7c), index_table = scenario.global(-0x7b9c);
    const auto records = lookups ? scenario.r32(lookups) : 0u;
    // A person's primary contract (TOC -0x7b9c index row, records at the contract manager's +0): what
    // 0x10082f90 returns for the person and the contract's club.
    const auto contract_of = [&](std::uint32_t person) -> std::uint32_t {
        if (!index_table || !records) return 0;
        const auto index = scenario.r32(index_table + scenario.r32(person) * 100);
        return std::int32_t(index) < 0 ? 0u : records + index * 100;
    };
    const auto reputation = [&](std::uint32_t club) { return std::int32_t(std::int16_t(scenario.s.memory.read<2>(club + 0x1ec))); };
    if (scenario.match) {
        const auto contract = scratch_base + 0x5000, humans = scratch_base + 0x6000;
        const auto people = scenario.people();
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side), club = scenario.r32(record + 0x34);
            for (unsigned index = 0; index < 20; index += 3) {
                const auto player = scenario.player(side, index);
                const auto person = scenario.r32(player + 0x6c);
                if (!person) continue;
                const auto id = scenario.r32(person);
                // The contract manager has no records on the match snapshot: the person's index row (TOC -0x7b9c)
                // written to 0 and the manager's records pointer to a contract row built in scratch, for the side's
                // club. Flags 0x10000 / 0x20000 then run the relation's adjustment and the match clock's scaling.
                for (unsigned k = 0; k < 20; ++k) {
                    std::vector<Write> writes{{4, index_table + id * 100, 0}, {4, lookups, contract}, {4, contract + 4, scenario.r32(club)},
                                              {1, contract + 0x60, k % 10}, {4, contract + 0x50, k % 3 == 0 ? 0x2808u : k % 3 == 1 ? 0x10u : 0u},
                                              {1, contract + 0x4c, (k * 37 + index) & 0xff}, {1, contract + 0x44, (k * 11) & 0xff},
                                              {1, contract + 0x45, (k * 5) & 0xff}, {1, contract + 0x46, (k * 7) & 0xff}};
                    if (k % 4 == 1) writes.push_back({2, scenario.match + 0x7948, k % 8 == 1 ? 0u : 0x2a00u});
                    const std::uint32_t mixes[] = {0x800 | 0x10000, 0x800 | 0x20000, 0x10000, 0x20000, 0x800 | 0x10000 | 0x800000, 0x800 | 0x20000 | 0x800000};
                    if (k % 5 == 4) {
                        for (unsigned entry_index = 0; entry_index < 16; ++entry_index) {
                            writes.push_back({4, humans + entry_index * 8, 0x7fff0000u + entry_index});
                            writes.push_back({4, humans + entry_index * 8 + 4, 0xffffffffu});
                        }
                    }
                    Call call{.person = person, .record = player, .role = roles[(index + k) % std::size(roles)], .flags = mixes[k % std::size(mixes)],
                              .club = club, .side = record, .judge = 0, .floor = 0xffffffffu, .human = k % 5 == 4 ? humans : 0u};
                    cases.push_back(make(scenario, call, "match relation (index row, records pointer, scratch contract written) " + std::to_string(k), writes));
                }
                // The human manager's list with the person at an entry below 11, 0x10149df0 (generated) answered
                // nonzero: the slot's holder decides. The worth comes from the player's record floats (+0x50 written,
                // +0x5c zeroed, the clock word +0x794c off 0x1ef) so that the worth's three bands are taken.
                for (unsigned k = 0; k < 18; ++k) {
                    const std::uint32_t role = std::array<std::uint32_t, 4>{0x2, 0x4, 0x10, 0x84}[k % 4];
                    const std::uint32_t worths[] = {0x460ca000 /* 9000.0f */, 0x4610e000 /* 9272.0f */, 0x46138000 /* 9440.0f */,
                                                    0x46000000 /* 8192.0f */, 0x46160000 /* 9600.0f */, 0x46080000 /* 8704.0f */};
                    const auto matched = (index + k) % 11;
                    std::vector<Write> writes{{4, player + 0x50, worths[k % std::size(worths)]}, {4, player + 0x5c, 0}, {2, scenario.match + 0x794c, 100}};
                    for (unsigned entry_index = 0; entry_index < 16; ++entry_index) {
                        writes.push_back({4, humans + entry_index * 8, entry_index == matched ? id : 0x7fff0000u + entry_index});
                        const std::uint32_t holder = k % 3 == 0 ? role : k % 3 == 1 ? 0xffffffffu : 0x7777u;
                        writes.push_back({4, humans + entry_index * 8 + 4, holder});
                    }
                    Call call{.person = person, .record = player, .role = role, .flags = k % 2 ? 0x40000u | 0x8u : 0x1000u | 0x2u,
                              .club = club, .side = record, .judge = 0, .floor = 0xffffffffu, .first = 0, .second = 1, .role_word = 0, .extra = 0, .human = humans};
                    cases.push_back(make(scenario, call, "human list, worth from the record floats (+0x50, +0x5c, clock written) " + std::to_string(k), writes,
                                         {{0x10149df0, k % 2 ? 1u : 2u}}));
                }
                // The side's human manager (the word at match + side * 0xd8 + 0x7acc, the side record's +0x20)
                // written to a person of the last 16 of the table flagged at +0x6b, the clock word off 0x1ef: the
                // four calls about him (0x10147e70 and 0x101481d0 answered, both generated; 0x100c7d00 runs).
                for (unsigned k = 0; k < 12; ++k) {
                    const auto manager = scenario.person(people - 1 - (index + k) % 16);
                    if (scenario.r32(manager) + 16 < people) continue;
                    std::vector<Write> writes{{2, scenario.match + 0x794c, 100}, {4, scenario.match + side * 0xd8 + 0x7acc, manager},
                                              {1, manager + 0x6b, 1}, {1, player + 0x27, side}, {1, scenario.match + side * 0xd8 + 0x7ac7, k % 3 == 0 ? 0u : 1u}};
                    if (k % 4 == 3) writes.push_back({1, manager + 0x6b, 0});
                    Call call{.person = person, .record = player, .role = roles[(index + k) % std::size(roles)], .flags = 0x1 | (k % 2 ? 0x800u : 0x40000u),
                              .club = club, .side = record, .judge = 0, .floor = 0xffffffffu};
                    cases.push_back(make(scenario, call, "side's human manager (+0x7acc, +0x6b, +0x7ac7, clock written) " + std::to_string(k), writes,
                                         {{0x10147e70, k % 6 == 5 ? 1u : 0u}, {0x101481d0, (k / 2) % 2}}));
                }
                // No contract manager (the global at TOC -0x7b7c written to 0): no relation is looked up.
                if (index < 6) {
                    Call call{.person = person, .record = player, .role = roles[index], .flags = every_component, .club = club, .side = record, .floor = 0xffffffffu};
                    cases.push_back(make(scenario, call, "no contract manager (TOC -0x7b7c global written 0)", {{4, scenario.toc(-0x7b7c), 0}}));
                }
            }
        }
        return;
    }
    if (!records) return;
    const auto count = scenario.people();
    // Players with a primary contract (the relation 0x10082f90 finds for the contract's club).
    struct Contracted { std::uint32_t person, contract, club; };
    std::vector<Contracted> contracted;
    for (unsigned index = 0; index < count && contracted.size() < 400; index += 7) {
        const auto person = scenario.person(index);
        if (!scenario.r32(person + 0x14)) continue;
        const auto contract = contract_of(person);
        if (!contract) continue;
        const auto club = scenario.club(scenario.r32(contract + 4));
        if (scenario.r32(club) != scenario.r32(contract + 4)) continue;
        contracted.push_back({person, contract, club});
    }
    std::vector<std::uint32_t> clubs;
    for (unsigned index = 0; index < 2000; index += 7) clubs.push_back(scenario.club(index));
    std::uint32_t famous = clubs[0];
    for (const auto club : clubs) if (reputation(club) > reputation(famous)) famous = club;
    // The club factor's other side: the person's club (+0x24 written to the most reputed club of a sample) at
    // least 1250 above the contract's club, the relation's rating (+0x44 written) below 25, ages over 30
    // (+0x5c written), goalkeepers (+0xf of the player data written 20) among them.
    {
        unsigned n = 0;
        for (const auto& item : contracted) {
            if (reputation(item.club) + 1250 >= reputation(famous)) continue;
            if (n >= 48) break;
            const auto data = scenario.r32(item.person + 0x14);
            for (unsigned a = 0; a < 4; ++a, ++n) {
                const std::uint32_t ages[] = {31, 33, 35, 36, 38, 40, 34, 37};
                const std::uint32_t ratings[] = {0x9c, 0xd8, 0x00, 0x18, 0x19, 0xf0};
                std::vector<Write> writes{{4, item.person + 0x24, famous}, {2, item.person + 0x5c, ages[(n + a) % std::size(ages)]},
                                          {1, item.contract + 0x44, ratings[(n * 3 + a) % std::size(ratings)]}};
                if (n % 3 == 0) writes.push_back({1, data + 0xf, 20});
                if (n % 5 == 2) writes.push_back({2, item.club + 0x1ec, 0x1500});  // over 0x1482 (the dead inner test's first half)
                Call call{.person = item.person, .role = roles[n % std::size(roles)], .flags = n % 2 ? every_component : 0x2000000u | 0x800u | 0x4000u,
                          .club = item.club, .judge = 0, .floor = 0xffffffffu, .second = place_second[n % std::size(place_second)]};
                cases.push_back(make(scenario, call, "club factor, person's club (+0x24), age, relation +0x44 written " + std::to_string(n), writes));
            }
        }
    }
    // The development's ban marks: bits 2-3 of the byte at +12 of the person's TOC -0x7b9c row written,
    // ages over 40, over 34 and younger (+0x5c written), the player's +0x3c byte below and above 10.
    for (unsigned k = 0; k < 36 && k < contracted.size(); ++k) {
        const auto& item = contracted[(k * 7) % contracted.size()];
        const auto row = index_table + scenario.r32(item.person) * 100 + 12;
        const auto marks = std::uint32_t(scenario.s.memory.read<1>(row));
        const std::uint32_t ages[] = {41, 36, 30, 25, 42, 35};
        std::vector<Write> writes{{1, row, (marks & ~0xcu) | ((1 + k % 3) << 2)}, {2, item.person + 0x5c, ages[k % std::size(ages)]},
                                  {1, scenario.r32(item.person + 0x14) + 0x3c, k % 2 ? 5u : 15u}};
        Call call{.person = item.person, .role = roles[k % std::size(roles)], .flags = k % 3 ? 0x4000u | 0x800u : every_component,
                  .club = item.club, .floor = 0xffffffffu};
        cases.push_back(make(scenario, call, "development ban marks (row +12, age, player +0x3c written) " + std::to_string(k), writes));
    }
    // Flag 0x20000 alone over the real contract with each kind (+0x60 written): kind 0 leaves no adjustment.
    for (unsigned k = 0; k < 40 && k < contracted.size(); ++k) {
        const auto& item = contracted[(k * 11 + 3) % contracted.size()];
        const auto kind_byte = std::uint32_t(scenario.s.memory.read<1>(item.contract + 0x60));
        std::vector<Write> writes{{1, item.contract + 0x60, (kind_byte & 0xf0) | (k % 10)}};
        const std::uint32_t mixes[] = {0x20000 | 0x800, 0x20000, 0x20000 | 0x800000, 0x20000 | 0x800 | 0x200000};
        Call call{.person = item.person, .role = roles[k % std::size(roles)], .flags = mixes[k % std::size(mixes)], .club = item.club, .floor = 0xffffffffu};
        cases.push_back(make(scenario, call, "contract kind (+0x60 written) " + std::to_string(k), writes));
    }
    // No contract manager (the global at TOC -0x7b7c written to 0).
    for (unsigned k = 0; k < 6 && k < contracted.size(); ++k) {
        const auto& item = contracted[k];
        Call call{.person = item.person, .role = roles[k], .flags = every_component, .club = item.club, .floor = 0xffffffffu};
        cases.push_back(make(scenario, call, "no contract manager (TOC -0x7b7c global written 0)", {{4, scenario.toc(-0x7b7c), 0}}));
    }
}
}

// MATCH_MAN.CPP checker cases for the rating_b block (match_man_rating_b.hpp, 0x10216830).
#include "match_man_checks.hpp"
#include "match_man_rating_b_detail.hpp"

namespace cm::aot::match_man_checks {
namespace {
constexpr std::uint32_t entry = 0x10216830;
constexpr std::uint32_t person_size = 0x70;
std::uint32_t person_at(const Scenario& scenario, unsigned index) { return scenario.global(-0x7be0) + index * person_size; }

// The components (r5) the original tests, by the bits the listing reads, and the roles (r4)
// its position groups read.
constexpr std::uint32_t every_component = 0x3dfffffe;
constexpr std::uint32_t roles[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 | 0x10, 0x800 | 0x20, 0x84, 0x100, 0x6, 0x30, 0x1000 | 0x8, 0x200 | 0x2, 0x400 | 0x40, 0x80 | 0x100};
struct Mix { std::uint32_t flags; const char* label; };
constexpr Mix mixes[] = {
    {every_component, "every component"},
    {every_component & ~0x10000u, "without 0x10000"},
    {every_component & ~0x30000u, "without 0x10000 and 0x20000"},
    {every_component & ~0x1000u, "without the staff fit"},
    {0x800 | 0x10000, "anchor and relation"},
    {0x800 | 0x200000 | 0x20000, "anchor, 0x200000 and club relation"},
    {0x4000 | 0x2000000 | 0x1000, "development, club factor, staff fit"},
    {0x40 | 0x10 | 0x20 | 0x4 | 0x80 | 0x100, "averages"},
    {0x40000 | 0x2 | 0x8 | 0x400 | 0x200, "trends, form, overall"},
    {0x100000 | 0x1000000, "caps and foreign"},
    {0x5902, "the game's call"},
    {0x5902 | 0x40 | 0x20 | 0x10 | 0x80, "the game's call with every average"},
    {every_component & ~0x800u & ~0x10000u, "no anchor, club relation"},
    {every_component & ~0x800u, "no anchor, relation"},
};

void add(std::vector<Case>& cases, std::uint32_t person, std::uint32_t club, std::uint32_t judge, std::uint32_t role,
         const Mix& mix, std::int32_t floor, const std::string& label) {
    cases.push_back({.entry = entry,
                     .registers = {{3, person}, {4, role}, {5, mix.flags}, {6, club}, {7, std::uint32_t(floor)}, {8, judge}},
                     .label = label + ", role " + std::to_string(role) + ", " + mix.label + ", floor " + std::to_string(floor)});
}
}

MATCH_MAN_CASES("rating_b") {
    if (scenario.match) {
        // The match's own call: the side's judge (+0x20), its club (+0x34), each player's person.
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            const auto judge = scenario.r32(record + 0x20), club = scenario.r32(record + 0x34);
            for (unsigned index = 0; index < 20; index += 4) {
                const auto person = scenario.r32(scenario.player(side, index) + 0x6c);
                for (const auto role : {0x1u, 0x4u, 0x20u})
                    for (const auto& mix : {mixes[0], mixes[7]})
                        add(cases, person, club, judge, role, mix, -1, "match side " + std::to_string(side) + " player " + std::to_string(index));
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
    if (players.empty() || judges.empty()) return;
    std::vector<std::uint32_t> clubs;
    for (unsigned index = 0; index < 400; index += 17) clubs.push_back(scenario.club(index));
    // Pairs spread over the tables: each role with each mix, a floor now and then.
    unsigned n = 0;
    for (const auto role : roles) {
        for (const auto& mix : mixes) {
            for (unsigned k = 0; k < 6; ++k, ++n) {
                const auto person = players[(n * 37 + 3) % players.size()], judge = judges[(n * 11 + 1) % judges.size()];
                const auto club = clubs[(n * 5) % clubs.size()];
                const std::int32_t floor = k % 3 == 0 ? -1 : k % 3 == 1 ? 120 : 971;
                add(cases, person, club, k % 2 ? judge : 0, role, mix, floor, "pair " + std::to_string(n));
            }
        }
    }
    // The person's own club (the club factor and the "own player" paths) and no club at all.
    for (unsigned k = 0; k < 40; ++k) {
        const auto person = players[(k * 53) % players.size()];
        const auto own = scenario.r32(person + 0x24);
        add(cases, person, own, judges[k % judges.size()], roles[k % std::size(roles)], mixes[k % std::size(mixes)], -1, "own club " + std::to_string(k));
        add(cases, person, 0, judges[k % judges.size()], roles[(k + 3) % std::size(roles)], mixes[(k + 2) % std::size(mixes)], -1, "no club " + std::to_string(k));
    }
    // Records the pairs don't reach, written over real ones (undone after each case): a person's age and
    // caps, a goalkeeper's byte, a free agent, a club with no default judge.
    const auto with = [&](std::uint32_t person, std::uint32_t club, std::uint32_t judge, std::uint32_t role, const Mix& mix,
                          std::int32_t floor, const std::string& label, std::vector<Write> writes,
                          std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            cases.push_back({.entry = entry,
                         .registers = {{3, person}, {4, role}, {5, mix.flags}, {6, club}, {7, std::uint32_t(floor)}, {8, judge}},
                         .memory = std::move(writes),
                         .label = label + ", role " + std::to_string(role) + ", " + mix.label + ", floor " + std::to_string(floor),
                         .answers = std::move(answers)});
    };
    for (unsigned k = 0; k < 24; ++k) {
        const auto person = players[(k * 29 + 7) % players.size()], club = clubs[(k * 3) % clubs.size()];
        const auto judge = judges[(k * 7) % judges.size()], data = scenario.r32(person + 0x14);
        const std::uint32_t ages[] = {17, 19, 21, 23, 26, 29, 31, 33, 36, 39};
        const std::uint32_t caps[] = {0, 3, 7, 12, 18, 30, 60};
        const auto role = roles[k % std::size(roles)];
        for (const auto& mix : {mixes[0], mixes[10]})
            for (unsigned a = 0; a < std::size(ages); ++a) {
                std::vector<Write> writes{{2, person + 0x5c, ages[a]}, {1, person + 0x5e, caps[(a + k) % std::size(caps)]}};
                if (k % 3 == 0) writes.push_back({1, data + 0xf, 20});       // a goalkeeper's rating
                if (k % 4 == 1) writes.push_back({4, person + 0x24, 0});     // a free agent
                if (k % 4 == 2) writes.push_back({4, person + 0x24, club});  // the club's own player
                if (k % 5 == 3) writes.push_back({4, club + 0x90, 0});       // a club with no default judge
                if (k % 6 == 4) writes.push_back({1, data + 0x14, 20});      // a rating of 20 in a defender slot
                // Some of them as the judge's elder with the club's own player (the date window), and a
                // club whose reputation is low or high (the club factor's two sides).
                if (k % 2 == 0) { writes.push_back({4, person + 0x24, club}); writes.push_back({2, person + 0x4e, 0x7ff0}); }
                if (k % 3 == 1) writes.push_back({2, club + 0x1ec, 300 + 9000 * (a % 2)});
                std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
                if (k % 2 == 0) answers = {{0x10082f90, scratch_base + 0x1000}, {0x10082f20, 0}};
                const std::vector<Write> relation_bytes{{1, scratch_base + 0x1060, 1 + a % 3}, {1, scratch_base + 0x1044, (a * 41) & 0xff}};
                if (k % 2 == 0) writes.insert(writes.end(), relation_bytes.begin(), relation_bytes.end());
                with(person, club, k % 4 == 0 ? 0 : judge, role, mix, k % 3 ? -1 : 300, "aged " + std::to_string(ages[a]) + " person " + std::to_string(k), writes, answers);
            }
    }
    // What the game's other modules answer: the staff checks (0x100da050 ...), the game's opinion of the
    // person, and the relation records (0x10082f90, 0x10082f20, 0x103ecb00, 0x103ecef0).
    const auto relation = scratch_base + 0x1000, counts = scratch_base + 0x2000, found = scratch_base + 0x3000;
    for (unsigned k = 0; k < 40; ++k) {
        const auto person = players[(k * 31 + 5) % players.size()], club = clubs[(k * 5 + 1) % clubs.size()];
        const auto judge = judges[(k * 3) % judges.size()];
        const std::uint32_t kinds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x21, 0x33};
        std::vector<Write> writes{{1, relation + 0x60, kinds[k % std::size(kinds)]},
                                  {4, relation + 0x50, k % 3 == 0 ? 0x2808u : k % 3 == 1 ? 0x10u : 0u},
                                  {1, relation + 0x4c, (k * 37) & 0xff}, {1, relation + 0x44, (k * 11) & 0xff},
                                  {1, relation + 0x45, (k * 5) & 0xff}, {1, relation + 0x46, (k * 7) & 0xff},
                                  {2, counts + 4, k % 7 == 0 ? 0u : 5 + k}, {2, counts + 6, k % 7 == 0 ? 0u : 40 - k},
                                  {4, found + 4, 1}, {4, person + 0x24, club}};
        // Young players with many caps (the youngest age band of the club factor), others older.
        if (k % 3 != 1) { writes.push_back({2, person + 0x5c, 17 + k % 5}); writes.push_back({1, person + 0x5e, 12 + k % 40}); }
        const std::uint32_t opinions[] = {0xf830, 0xfc18, 600, 0x3e9, 1500, 0x1f5, 100};
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers{
            {0x10082f90, relation}, {0x103ecb00, counts}, {0x103ecef0, k % 5 == 4 ? 0u : 1u},
            {0x10082f20, k % 4 == 3 ? 0u : found}, {0x10187650, opinions[k % std::size(opinions)]},
            {0x100da050, k % 3 == 0}, {0x100da0b0, k % 3 == 1}, {0x100d9580, k % 4 == 0}, {0x100d9520, k % 4 == 1}};
        for (const auto& mix : {mixes[0], mixes[1], mixes[2], mixes[4], mixes[5], mixes[6], mixes[10], mixes[11], mixes[12], mixes[13]})
            with(person, club, k % 2 ? judge : 0, roles[(k * 3) % std::size(roles)], mix, k % 3 ? -1 : 300,
                 "answered " + std::to_string(k), writes, answers);
    }
    // The world object's two doubles (+0x10 over +0x18) the form ratio reads: 0 / 1 makes any form beat it.
    const auto object = scenario.global(-0x7b3c);
    for (unsigned k = 0; k < 6; ++k) {
        const auto person = players[(k * 13 + 2) % players.size()], club = clubs[k % clubs.size()], judge = judges[k % judges.size()];
        std::vector<Write> writes{{4, object + 0x10, 0}, {4, object + 0x14, 0}, {4, object + 0x18, 0x3ff00000}, {4, object + 0x1c, 0},
                                  {2, person + 0x5c, 36 + k}, {4, person + 0x24, club}, {2, person + 0x4e, 0x7ff0}};
        if (k % 2) writes.push_back({4, object + 0x18, 0});  // a zero length: the ratio is the pool's constant
        with(person, club, judge, roles[k], mixes[0], -1, "old own player " + std::to_string(k), writes);
    }
    // No lookups table, and neither judge nor club.
    with(players[0], clubs[0], judges[0], 0x2, mixes[0], -1, "no lookups table", {{4, scenario.toc(-0x7b7c), 0}});
    with(players[1], 0, 0, 0x4, {0x40 | 0x10, "averages only"}, -1, "neither judge nor club", {});
    // A spread of everything at once: random component sets over the bits the listing reads, random roles,
    // ages, caps, goalkeeper bytes, own or foreign clubs, judges or none, the relation answers or not.
    {
        std::uint32_t state = 0x1234567;
        const auto next = [&] { state = state * 1664525u + 1013904223u; return state >> 8; };
        constexpr std::uint32_t component_bits[] = {0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x4000, 0x40000,
                                                    0x10000, 0x20000, 0x100000, 0x200000, 0x1000000, 0x2000000};
        for (unsigned n = 0; n < 1500; ++n) {
            std::uint32_t flags = 0;
            for (const auto bit : component_bits) if (next() % 3 == 0) flags |= bit;
            if (n % 2) flags |= 0x1000 | 0x2000000 | 0x4000;  // the groups, club factor and development more often
            const auto person = players[next() % players.size()], club = clubs[next() % clubs.size()], judge = judges[next() % judges.size()];
            const auto data = scenario.r32(person + 0x14);
            std::uint32_t role = 0;
            for (const std::uint32_t bit : {1u, 2u, 4u, 8u, 0x10u, 0x20u, 0x40u, 0x80u, 0x100u, 0x200u, 0x400u, 0x800u, 0x1000u})
                if (next() % 4 == 0) role |= bit;
            std::vector<Write> writes{{2, person + 0x5c, 16 + next() % 26}, {1, person + 0x5e, next() % 50}};
            if (next() % 4 == 0) writes.push_back({1, data + 0xf, 20});
            if (next() % 3 == 0) writes.push_back({4, person + 0x24, club});
            if (next() % 6 == 0) writes.push_back({4, person + 0x24, 0});
            if (next() % 3 == 0) writes.push_back({2, person + 0x4e, 0x7ff0});
            if (next() % 3 == 0) writes.push_back({2, club + 0x1ec, 200 + next() % 9000});
            std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
            if (next() % 2) {
                answers = {{0x10082f90, relation}, {0x103ecb00, counts}, {0x103ecef0, next() % 5 != 4}, {0x10082f20, next() % 4 == 3 ? 0u : found},
                           {0x10187650, next() % 3000 + 0xf000u * (next() % 2)}, {0x100da050, next() % 3 == 0}, {0x100da0b0, next() % 3 == 1},
                           {0x100d9580, next() % 4 == 0}, {0x100d9520, next() % 4 == 1}};
                writes.push_back({1, relation + 0x60, next() % 10});
                writes.push_back({4, relation + 0x50, next() % 3 == 0 ? 0x2808u : next() % 2 ? 0x10u : 0u});
                writes.push_back({1, relation + 0x4c, next() & 0xff});
                writes.push_back({1, relation + 0x44, next() & 0xff});
                writes.push_back({1, relation + 0x45, next() & 0xff});
                writes.push_back({1, relation + 0x46, next() & 0xff});
                writes.push_back({2, counts + 4, next() % 30});
                writes.push_back({2, counts + 6, next() % 30});
                writes.push_back({4, found + 4, 1});
            }
            const Mix mix{flags, "random components"};
            const std::int32_t floors[] = {-1, -1, 0, 100, 300, 971};
            with(person, club, next() % 3 ? judge : 0, role, mix, floors[next() % 6], "fuzz " + std::to_string(n), writes, answers);
        }
    }
    // A person that is not a player, and no person at all.
    add(cases, 0, clubs[0], judges[0], 0x1, mixes[0], -1, "no person");
    for (std::uint32_t index = 0; index < count && index < 3000; ++index) {
        const auto person = person_at(scenario, index);
        if (!scenario.r32(person + 0x14)) {
            add(cases, person, clubs[0], judges[0], 0x1, mixes[0], -1, "no player data");
            break;
        }
    }
}
}

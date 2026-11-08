// MATCH_MAN.CPP checker cases for the selection block (match_man_selection.hpp).
#include "match_man_checks.hpp"
#include "match_man_selection_detail.hpp"

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
constexpr std::uint32_t rating_entry = 0x10210800, best_index_entry = 0x10210430, best_person_entry = 0x102102d0;

// The people table: 0x70-byte rows at the word TOC -0x7be0 points to, counted at -0x7be4.
constexpr std::uint32_t person_size = 0x70;
std::uint32_t person_at(const Scenario& scenario, unsigned index) { return scenario.global(-0x7be0) + index * person_size; }

// Real records of the career: people with player data (+0x14), people with staff data (+0x1c).
struct Career {
    std::vector<std::uint32_t> players, staff, others;  // others: no player data
};
Career career(const Scenario& scenario) {
    Career result;
    const auto count = scenario.people();
    for (unsigned index = 0; index < count && (result.players.size() < 400 || result.staff.size() < 60); ++index) {
        const auto person = person_at(scenario, index);
        if (scenario.r32(person + person::player)) result.players.push_back(person);
        else if (result.others.size() < 8) result.others.push_back(person);
        if (scenario.r32(person + person::staff)) result.staff.push_back(person);
    }
    return result;
}

// A side built in the scratch area from real records: the manager's staff data (+0x24), the
// player table (+0x28, real match records in a match, none in the career) and the club (+0x34).
std::uint32_t scratch_side(std::vector<Write>& memory, std::uint32_t staff_data, std::uint32_t players, std::uint32_t club) {
    const auto side = scratch_base;
    memory.push_back({4, side + selection_side::manager_staff, staff_data});
    memory.push_back({4, side + selection_side::players, players});
    memory.push_back({4, side + selection_side::club, club});
    return side;
}

// Players with a contract at a club of the career: the pair 0x10082f90 finds a record for
// (the contract manager's table, indexed through the TOC -0x7b9c table, 0x64 bytes a row, +4 the club id).
struct Contract { std::uint32_t person, club, record; };
std::vector<Contract> contracts_of(const Scenario& scenario, const Career& people) {
    std::vector<Contract> found;
    const auto manager = scenario.global(-0x7b7c);
    const auto rows = scenario.r32(manager);
    const auto index_table = scenario.global(-0x7b9c);
    for (const auto person : people.players) {
        const auto id = scenario.r32(person);
        const auto row = std::int32_t(scenario.r32(index_table + id * 0x64));
        if (row < 0) continue;
        const auto record = rows + std::uint32_t(row) * 0x64;
        const auto club_id = scenario.r32(record + 4);
        if (club_id >= 800) continue;
        const auto club = scenario.club(club_id);
        if (scenario.r32(club) != club_id) continue;
        found.push_back({person, club, record});
        if (found.size() >= 12) break;
    }
    return found;
}

void rating_cases(const Scenario& scenario, std::vector<Case>& cases) {
    cases.push_back({.entry = rating_entry, .registers = {{3, 0}, {4, 0}, {5, 0}}, .label = "no person"});
    if (scenario.match) {
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            const auto club = scenario.r32(record + selection_side::club);
            for (unsigned index = 0; index < 20; ++index)
                cases.push_back({.entry = rating_entry, .registers = {{3, 0}, {4, scenario.player(side, index)}, {5, club}},
                                 .label = "match side " + std::to_string(side) + " player " + std::to_string(index)});
            // The person itself, with a match player that is absent (r4 = 0).
            const auto person = scenario.r32(scenario.player(side, 2) + match_player::person);
            cases.push_back({.entry = rating_entry, .registers = {{3, person}, {4, 0}, {5, club}}, .label = "match person only"});
        }
        return;
    }
    const auto people = career(scenario);
    if (people.players.empty()) return;
    for (unsigned n = 0; n < 12; ++n) {
        const auto club = scenario.club(n * 7);
        for (unsigned m = 0; m < 25; ++m)
            cases.push_back({.entry = rating_entry, .registers = {{3, people.players[(n * 31 + m * 17) % people.players.size()]}, {4, 0}, {5, club}},
                             .label = "career club " + std::to_string(n) + " player " + std::to_string(m)});
    }
    // A found contract: its signed byte +0x4c and the low nibble of +0x60 (the switch's cases 0..8
    // and the values past it, which the game's data may not hold: written for the case).
    const auto found = contracts_of(scenario, people);
    for (unsigned n = 0; n < found.size(); ++n) {
        cases.push_back({.entry = rating_entry, .registers = {{3, found[n].person}, {4, 0}, {5, found[n].club}},
                         .label = "contract " + std::to_string(n) + " as it is"});
        if (n >= 4) continue;
        for (std::uint32_t nibble = 0; nibble <= 10; ++nibble)
            for (std::uint32_t byte : {0u, 7u, 0x90u})
                cases.push_back({.entry = rating_entry, .registers = {{3, found[n].person}, {4, 0}, {5, found[n].club}},
                                 .memory = {{1, found[n].record + 0x60, 0xf0 | nibble}, {1, found[n].record + 0x4c, byte}},
                                 .label = "contract " + std::to_string(n) + " nibble " + std::to_string(nibble) + " byte " + std::to_string(byte) + " (written)"});
    }
}

void best_index_cases(const Scenario& scenario, std::vector<Case>& cases) {
    if (scenario.match) {
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            cases.push_back({.entry = best_index_entry, .registers = {{3, record}}, .label = "match side " + std::to_string(side)});
            // Players flagged unavailable (negative byte +0x19): all, then all but the last.
            std::vector<Write> none, last;
            for (unsigned index = 0; index < 20; ++index) {
                none.push_back({1, scenario.player(side, index) + selection_player::unavailable, 0x80});
                if (index < 19) last.push_back({1, scenario.player(side, index) + selection_player::unavailable, 0x80});
            }
            cases.push_back({.entry = best_index_entry, .registers = {{3, record}}, .memory = none, .label = "no available player " + std::to_string(side)});
            cases.push_back({.entry = best_index_entry, .registers = {{3, record}}, .memory = last, .label = "only the last available " + std::to_string(side)});
        }
        return;
    }
}

void best_person_cases(const Scenario& scenario, std::vector<Case>& cases) {
    // Lists of people go in the scratch area after the side.
    const auto list = scratch_base + 0x400;
    const auto fill = [&](std::vector<Write>& memory, const std::vector<std::uint32_t>& persons) {
        for (unsigned index = 0; index < persons.size(); ++index) memory.push_back({4, list + index * 4, persons[index]});
    };
    if (scenario.match) {
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            std::vector<std::uint32_t> persons;
            for (unsigned index = 0; index < 20; ++index) persons.push_back(scenario.r32(scenario.player(side, index) + match_player::person));
            for (const std::int32_t count : {0, 1, 5, 11, 20, -3}) {
                std::vector<Write> memory;
                fill(memory, persons);
                cases.push_back({.entry = best_person_entry, .registers = {{3, record}, {4, list}, {5, std::uint32_t(count)}}, .memory = memory,
                                 .label = "match side " + std::to_string(side) + " count " + std::to_string(count)});
            }
            std::vector<Write> memory;
            auto holes = persons;
            holes[1] = 0;
            fill(memory, holes);
            cases.push_back({.entry = best_person_entry, .registers = {{3, record}, {4, list}, {5, 8}}, .memory = memory, .label = "match side with a null entry"});
            // No manager staff data: the staff test is skipped.
            memory.push_back({4, record + selection_side::manager_staff, 0});
            cases.push_back({.entry = best_person_entry, .registers = {{3, record}, {4, list}, {5, 8}}, .memory = memory, .label = "match side without staff data"});
        }
        return;
    }
    const auto people = career(scenario);
    if (people.players.size() < 24 || people.staff.empty()) return;
    for (unsigned n = 0; n < 24; ++n) {
        std::vector<Write> memory;
        const auto staff_data = n % 6 == 5 ? 0 : scenario.r32(people.staff[(n * 5) % people.staff.size()] + person::staff);
        const auto side = scratch_side(memory, staff_data, 0, scenario.club(n * 11));
        std::vector<std::uint32_t> persons;
        for (unsigned index = 0; index < 16; ++index) persons.push_back(people.players[(n * 13 + index * 29) % people.players.size()]);
        if (n % 4 == 3) persons[2] = 0;
        if (n % 4 == 2 && !people.others.empty()) persons[5] = people.others[n % people.others.size()];
        fill(memory, persons);
        for (const std::int32_t count : {1, 16, 7})
            cases.push_back({.entry = best_person_entry, .registers = {{3, side}, {4, list}, {5, std::uint32_t(count)}}, .memory = memory,
                             .label = "career list " + std::to_string(n) + " count " + std::to_string(count)});
    }
}
}

MATCH_MAN_CASES("selection") {
    rating_cases(scenario, cases);
    best_index_cases(scenario, cases);
    best_person_cases(scenario, cases);
}
}

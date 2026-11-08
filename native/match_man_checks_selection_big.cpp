// MATCH_MAN.CPP checker cases for the selection block's two large routines: 0x10210c50 (the places of a
// side) and 0x1021a240 (the selection driver).
#include "match_man_checks.hpp"
#include "match_man_selection_detail.hpp"
#include <algorithm>
#include <array>
#include <map>

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
constexpr std::uint32_t places_entry = 0x10210c50, refill_entry = 0x1021a240;

// The caller's frame: its stack arguments sit at sp + 0x38 on (the state's own r1 at entry).
std::uint32_t stack_pointer(const Scenario& scenario) { return std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1))); }

// One call's inputs, in the shape real callers pass (traced from the career's own calls): the places' flags
// (halfwords), roles (bytes), ratings (halfwords) and slots (two words each), `people` persons.
struct Layout {
    std::uint32_t people = scratch_base, flags = scratch_base + 0x100, chosen = scratch_base + 0x200, slots = scratch_base + 0x300,
                  roles = scratch_base + 0x500, ratings = scratch_base + 0x600;
};
constexpr std::uint16_t flag_values[20] = {0x1, 0x808, 0x88, 0x104, 0x404, 0x204, 0x410, 0x220, 0x140, 0x440, 0x110, 0x4, 0x1, 0x4, 0x10, 0x4, 0x4, 0x40, 0x4, 0x4};
constexpr std::uint32_t slot_first[11] = {0xa, 0x6, 0x6, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa};
constexpr std::uint32_t slot_second[11] = {0x15552221, 0x56982221, 0x56982221, 0x155a4221, 0x155a4221, 0x155a2221, 0x259a2221, 0x259a2221, 0x95522221, 0x95522221, 0x259a2221};
constexpr std::uint8_t role_values[11] = {0x11, 0x12, 0x12, 0x14, 0x14, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11};

struct Setup {
    std::uint32_t club, side = 0, weights = 0, extra = 0x22aa2, mode = 0xca1ca;
    std::vector<std::uint32_t> people;
    unsigned primary = 11, secondary = 9;
    bool give_slots = true, give_roles = true, give_ratings = true;
    std::vector<Write> extra_writes;      // values written for the case (a branch the data can't reach)
    std::vector<std::int16_t> ratings;    // the places' ratings halfwords (zero when empty)
};

Case places_case(const Scenario& scenario, const Setup& setup, const std::string& label) {
    const Layout layout;
    Case item{.entry = places_entry, .label = label};
    for (unsigned index = 0; index < setup.people.size(); ++index) item.memory.push_back({4, layout.people + index * 4, setup.people[index]});
    for (unsigned index = 0; index < 20; ++index) item.memory.push_back({2, layout.flags + index * 2, flag_values[index]});
    for (unsigned index = 0; index < 11; ++index) {
        item.memory.push_back({4, layout.slots + index * 8, slot_first[index]});
        item.memory.push_back({4, layout.slots + index * 8 + 4, slot_second[index]});
        item.memory.push_back({1, layout.roles + index, role_values[index]});
    }
    for (unsigned index = 0; index < setup.ratings.size(); ++index) item.memory.push_back({2, layout.ratings + index * 2, std::uint32_t(std::uint16_t(setup.ratings[index]))});
    for (const auto& write : setup.extra_writes) item.memory.push_back(write);
    const auto sp = stack_pointer(scenario);
    item.memory.push_back({4, sp + 0x38, setup.side});
    item.memory.push_back({4, sp + 0x3c, setup.weights});
    item.memory.push_back({4, sp + 0x40, 0});
    item.memory.push_back({4, sp + 0x44, setup.give_slots ? layout.slots : 0});
    item.memory.push_back({4, sp + 0x48, setup.give_roles ? layout.roles : 0});
    item.memory.push_back({4, sp + 0x4c, setup.extra});
    item.memory.push_back({4, sp + 0x50, setup.give_ratings ? layout.ratings : 0});
    item.registers = {{3, layout.people}, {4, std::uint32_t(setup.people.size())}, {5, layout.flags}, {6, setup.primary}, {7, setup.secondary},
                      {8, layout.chosen}, {9, setup.mode}, {10, setup.club}};
    return item;
}

constexpr std::uint32_t person_size = 0x70;
std::uint32_t person_at(const Scenario& scenario, unsigned index) { return scenario.global(-0x7be0) + index * person_size; }

// The inputs of 0x1021a240 as 0x10206ea0 passes them: the holders of 11 places, a pool of the next people, candidates
// from elsewhere, and the six config bytes. Arrays live in the scratch area after the places' own.
struct Refill {
    std::uint32_t club, judge, side = 0, mode = 0xca1ca;
    std::vector<std::uint32_t> holders, pool, candidates;
    std::array<std::uint8_t, 6> config{1, 25, 4, 2, 5, 1};
    unsigned places = 11;
    bool give_ratings = true, give_slots = true, give_roles = true, give_candidates = true;
    std::vector<Write> extra_writes;
};

Case refill_case(const Scenario& scenario, const Refill& setup, const std::string& label) {
    const std::uint32_t holders = scratch_base + 0x1000, pool = scratch_base + 0x1100, candidates = scratch_base + 0x1200, chosen = scratch_base + 0x1300,
                        flags = scratch_base + 0x1400, ratings = scratch_base + 0x1500, slots = scratch_base + 0x1600, roles = scratch_base + 0x1800,
                        config = scratch_base + 0x1900;
    Case item{.entry = refill_entry, .label = label};
    for (unsigned index = 0; index < setup.holders.size(); ++index) {
        item.memory.push_back({4, holders + index * 4, setup.holders[index]});
        item.memory.push_back({4, chosen + index * 4, setup.holders[index]});
    }
    for (unsigned index = 0; index < setup.pool.size(); ++index) {
        item.memory.push_back({4, pool + index * 4, setup.pool[index]});
        item.memory.push_back({4, chosen + (std::uint32_t(setup.holders.size()) + index) * 4, setup.pool[index]});
    }
    for (unsigned index = 0; index < setup.candidates.size(); ++index) item.memory.push_back({4, candidates + index * 4, setup.candidates[index]});
    for (unsigned index = 0; index < 20; ++index) {
        item.memory.push_back({2, flags + index * 2, flag_values[index]});
        item.memory.push_back({2, ratings + index * 2, std::uint32_t(index * 7)});
    }
    for (unsigned index = 0; index < 11; ++index) {
        item.memory.push_back({4, slots + index * 8, slot_first[index]});
        item.memory.push_back({4, slots + index * 8 + 4, slot_second[index]});
        item.memory.push_back({1, roles + index, role_values[index]});
    }
    for (unsigned index = 0; index < 6; ++index) item.memory.push_back({1, config + index, setup.config[index]});
    for (const auto& write : setup.extra_writes) item.memory.push_back(write);
    const auto sp = stack_pointer(scenario);
    item.memory.push_back({4, sp + 0x38, pool});
    item.memory.push_back({4, sp + 0x3c, setup.give_candidates ? candidates : 0});
    item.memory.push_back({4, sp + 0x40, chosen});
    item.memory.push_back({4, sp + 0x44, flags});
    item.memory.push_back({4, sp + 0x48, setup.give_ratings ? ratings : 0});
    item.memory.push_back({4, sp + 0x4c, setup.give_slots ? slots : 0});
    item.memory.push_back({4, sp + 0x50, setup.give_roles ? roles : 0});
    item.memory.push_back({4, sp + 0x54, setup.side});
    item.registers = {{3, setup.club}, {4, setup.judge}, {5, config}, {6, setup.mode}, {7, setup.places}, {8, std::uint32_t(setup.pool.size())},
                      {9, std::uint32_t(setup.candidates.size())}, {10, holders}};
    return item;
}

void refill_cases(const Scenario& scenario, std::vector<Case>& cases, const std::vector<Refill>& setups, const std::string& where) {
    // Config bytes: the rule's kind (0 none, 1 and 2 database tests, 3 age over the limit, 4 beyond the kinds), the
    // limit, and the counts (byte 2 against byte 5 picks the first group's size, bytes 4 and 5 gate the whole).
    const std::vector<std::array<std::uint8_t, 6>> configs = {
        {1, 25, 4, 2, 5, 1}, {2, 25, 4, 2, 5, 1}, {3, 24, 4, 2, 5, 1}, {3, 20, 6, 3, 9, 2}, {3, 29, 2, 3, 6, 4}, {3, 22, 0, 4, 6, 2},
        {3, 26, 3, 0, 5, 3}, {0, 25, 4, 2, 5, 1}, {4, 25, 4, 2, 5, 1}, {3, 24, 4, 2, 2, 5}, {3, 18, 7, 5, 9, 1}, {1, 21, 3, 4, 6, 0}, {3, 26, 6, 0, 9, 2}, {2, 25, 6, 0, 9, 2}};
    // Each setup also with its candidates reversed and cut short: the person chosen is then at other places of the
    // list, the last among them.
    std::vector<Refill> all;
    for (const auto& setup : setups) {
        all.push_back(setup);
        auto reversed = setup;
        std::reverse(reversed.candidates.begin(), reversed.candidates.end());
        all.push_back(reversed);
        auto few = reversed;
        few.candidates.resize(6);
        all.push_back(few);
        auto without_pool = setup;
        without_pool.pool.clear();
        all.push_back(without_pool);
        for (const unsigned turn : {2u, 5u, 9u}) {
            auto rotated = without_pool;
            std::rotate(rotated.candidates.begin(), rotated.candidates.begin() + std::min<std::size_t>(turn, rotated.candidates.size()), rotated.candidates.end());
            all.push_back(rotated);
        }
        auto single = setup;
        single.pool.clear();
        single.candidates.resize(1);
        all.push_back(single);
        auto pair = setup;
        pair.pool.clear();
        pair.candidates.resize(2);
        all.push_back(pair);
        for (unsigned which = 0; which < 3; ++which) {
            auto holes = setup;
            if (which == 0) holes.holders[3] = 0;
            if (which == 1) holes.pool[1] = 0;
            if (which == 2) holes.candidates[2] = 0;
            all.push_back(holes);
        }
        auto bare = setup;
        bare.give_ratings = bare.give_slots = bare.give_roles = false;
        all.push_back(bare);
        auto no_candidates = setup;
        no_candidates.give_candidates = false;
        all.push_back(no_candidates);
        auto held = setup;
        held.candidates[1] = setup.pool[2];
        held.candidates[4] = setup.holders[5];
        all.push_back(held);
        auto small_pool = reversed;
        small_pool.pool.resize(2);
        all.push_back(small_pool);
    }
    for (unsigned n = 0; n < all.size(); ++n)
        for (unsigned m = 0; m < configs.size(); ++m) {
            auto setup = all[n];
            setup.config = configs[m];
            cases.push_back(refill_case(scenario, setup, where + " " + std::to_string(n) + " config " + std::to_string(m)));
        }
}

void places_cases(const Scenario& scenario, std::vector<Case>& cases) {
    if (scenario.match) {
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            Setup setup;
            setup.club = scenario.r32(record + selection_side::club);
            setup.side = record;
            for (unsigned index = 0; index < 20; ++index) setup.people.push_back(scenario.r32(scenario.player(side, index) + match_player::person));
            const auto name = "match side " + std::to_string(side) + ": ";
            cases.push_back(places_case(scenario, setup, name + "20 people, 11 + 9"));
            auto fewer = setup;
            fewer.people.resize(7);
            cases.push_back(places_case(scenario, fewer, name + "7 people, 11 + 9"));
            auto first_only = setup;
            first_only.secondary = 0;
            cases.push_back(places_case(scenario, first_only, name + "20 people, 11 + 0"));
            auto given_nothing = setup;
            given_nothing.give_slots = given_nothing.give_roles = given_nothing.give_ratings = false;
            cases.push_back(places_case(scenario, given_nothing, name + "arrays made by the call"));
            // People that are not on the side (the other side's), and no places at all.
            auto strangers = setup;
            strangers.people.clear();
            for (unsigned index = 0; index < 14; ++index) strangers.people.push_back(scenario.r32(scenario.player(1 - side, index) + match_player::person));
            cases.push_back(places_case(scenario, strangers, name + "14 people of the other side"));
            auto none = given_nothing;
            none.primary = none.secondary = 0;
            cases.push_back(places_case(scenario, none, name + "no places, arrays not given"));
            // A null entry in the list, with enough people for the places and without.
            for (const unsigned keep : {20u, 9u}) {
                auto holes = setup;
                holes.people.resize(keep);
                holes.people[2] = 0;
                holes.people[5] = 0;
                cases.push_back(places_case(scenario, holes, name + std::to_string(keep) + " people, two null entries"));
            }
            auto only_second = setup;
            only_second.primary = 0;
            only_second.secondary = 6;
            cases.push_back(places_case(scenario, only_second, name + "only secondary places"));
            // Counts that cancel (3 primary, -3 secondary): no place in all, yet places are asked for.
            auto cancelling = setup;
            cancelling.primary = 3;
            cancelling.secondary = 0xfffffffd;
            cases.push_back(places_case(scenario, cancelling, name + "primary 3, secondary -3"));
            for (unsigned shape = 0; shape < 6; ++shape) {
                auto rated = setup;
                for (unsigned index = 0; index < 20; ++index) rated.ratings.push_back(std::int16_t(((index * 37 + shape * 101) % 400) - 150 * (shape % 2)));
                rated.secondary = shape % 3 ? 6 : 9;
                cases.push_back(places_case(scenario, rated, name + "ratings shape " + std::to_string(shape)));
            }
        }
        std::vector<Refill> setups;
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            Refill refill;
            refill.club = scenario.r32(record + selection_side::club);
            refill.judge = scenario.r32(record + 0x20);
            for (unsigned index = 0; index < 11; ++index) refill.holders.push_back(scenario.r32(scenario.player(side, index) + match_player::person));
            for (unsigned index = 11; index < 20; ++index) refill.pool.push_back(scenario.r32(scenario.player(side, index) + match_player::person));
            for (unsigned index = 0; index < 15; ++index) refill.candidates.push_back(scenario.r32(scenario.player(1 - side, index) + match_player::person));
            for (const auto given : {0u, record}) {
                refill.side = given;
                setups.push_back(refill);
            }
        }
        refill_cases(scenario, cases, setups, "match refill");
        return;
    }
    // The players of each club, from the contract manager's table (the TOC -0x7b9c table gives the row of a
    // person's contract, 0x64 bytes, +4 the club id), as the callers' squads are.
    const auto manager = scenario.global(-0x7b7c);
    const auto rows = scenario.r32(manager);
    const auto index_table = scenario.global(-0x7b9c);
    std::map<std::uint32_t, std::vector<std::uint32_t>> squads;
    for (unsigned index = 0; index < scenario.people(); ++index) {
        const auto person = person_at(scenario, index);
        if (!scenario.r32(person + person::player)) continue;
        const auto row = std::int32_t(scenario.r32(index_table + scenario.r32(person) * 0x64));
        if (row < 0) continue;
        squads[scenario.r32(rows + std::uint32_t(row) * 0x64 + 4)].push_back(person);
    }
    // Clubs with a full squad whose judge (+0x90) is a person of the table: the rating reads the judge's staff data.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> clubs;
    const auto first = person_at(scenario, 0), last = person_at(scenario, scenario.people());
    for (unsigned index = 0; index < 800 && clubs.size() < 12; ++index) {
        const auto club = scenario.club(index);
        const auto judge = scenario.r32(club + selection_club::judge);
        if (judge >= first && judge < last && squads[scenario.r32(club)].size() >= 20) clubs.push_back({club, judge});
    }
    std::vector<Refill> refills;
    for (unsigned n = 0; n < clubs.size(); ++n) {
        Setup setup;
        setup.club = clubs[n].first;
        setup.weights = n % 2 ? clubs[n].second : 0;
        const auto& squad = squads[scenario.r32(clubs[n].first)];
        const unsigned count = std::array<unsigned, 12>{20, 30, 11, 5, 1, 20, 18, 30, 12, 9, 20, 25}[n];
        for (unsigned index = 0; index < count; ++index) setup.people.push_back(squad[index % squad.size()]);
        setup.secondary = n % 3 == 0 ? 9 : n % 3 == 1 ? 0 : 5;
        setup.primary = n % 5 == 4 ? 9 : 11;
        const auto name = "career " + std::to_string(n) + ": ";
        cases.push_back(places_case(scenario, setup, name + std::to_string(count) + " people"));
        if (n < 6 && squad.size() >= 40) {
            Refill refill;
            refill.club = setup.club;
            refill.judge = clubs[n].second;
            for (unsigned index = 0; index < 11; ++index) refill.holders.push_back(squad[index]);
            for (unsigned index = 11; index < 20; ++index) refill.pool.push_back(squad[index]);
            const auto& other = squads[scenario.r32(clubs[(n + 1) % clubs.size()].first)];
            for (unsigned index = 0; index < 15; ++index) refill.candidates.push_back(other[index % other.size()]);
            refills.push_back(refill);
        }
        auto given_nothing = setup;
        given_nothing.give_slots = given_nothing.give_roles = given_nothing.give_ratings = false;
        cases.push_back(places_case(scenario, given_nothing, name + "arrays made by the call"));
        if (n >= 4) continue;
        // The club's judge is a human manager's record (an id among the last 16 people, flag +0x6b set).
        const auto total = scenario.people();
        for (std::uint32_t id_offset : {1u, 5u, 16u, 17u}) {
            const auto judge = person_at(scenario, total - id_offset);
            for (std::uint32_t flag : {1u, 0u}) {
                auto asked = setup;
                asked.mode |= 0x400000;
                asked.extra_writes = {{4, setup.club + selection_club::judge, judge}, {1, judge + selection_person::human_flag, flag}};
                cases.push_back(places_case(scenario, asked, name + "judge " + std::to_string(id_offset) + " from the end, flag " + std::to_string(flag) + " (written)"));
            }
        }
        auto no_judge = setup;
        no_judge.mode |= 0x400000;
        no_judge.extra_writes = {{4, setup.club + selection_club::judge, 0}};
        cases.push_back(places_case(scenario, no_judge, name + "judge asked, club has none (written)"));
        // A person without player data, too few people to fill the primary places: the passes run to the limit.
        auto stuck = setup;
        std::uint32_t staff_only = 0;
        for (unsigned index = 0; index < total && !staff_only; ++index)
            if (!scenario.r32(person_at(scenario, index) + person::player)) staff_only = person_at(scenario, index);
        stuck.people = {setup.people[0], staff_only, setup.people[1]};
        cases.push_back(places_case(scenario, stuck, name + "a person without player data among 3 (written)"));
        // Fewer people than places: a goalkeeper place gives up a holder under 10 as goalkeeper, any other
        // place a natural goalkeeper. The flags and the people's goalkeeper byte (+0xf of the player data) are
        // written so that each case happens.
        for (const bool keeper_places : {true, false}) {
            auto few = setup;
            few.people.resize(3);
            few.secondary = 0;
            for (unsigned index = 0; index < 11; ++index) few.extra_writes.push_back({2, Layout{}.flags + index * 2, keeper_places ? 0x1u : 0x4u});
            for (const auto person : few.people)
                few.extra_writes.push_back({1, scenario.r32(person + person::player) + selection_data::goalkeeper, keeper_places ? 5u : 20u});
            cases.push_back(places_case(scenario, few, name + (keeper_places ? "keeper places, holders under 10 (written)" : "other places, natural keepers (written)")));
        }
        // Null entries and a person without player data in the list, with enough people and without.
        std::uint32_t without_data = 0;
        for (unsigned index = 0; index < total && !without_data; ++index)
            if (!scenario.r32(person_at(scenario, index) + person::player)) without_data = person_at(scenario, index);
        for (const unsigned keep : {20u, 9u}) {
            if (setup.people.size() < keep) continue;
            auto holes = setup;
            holes.people.resize(keep);
            holes.people[2] = 0;
            holes.people[4] = without_data;
            cases.push_back(places_case(scenario, holes, name + std::to_string(keep) + " people, a null entry and one without player data (written)"));
        }
        // A judge whose id is past the people table (the range test's upper bound).
        auto past = setup;
        past.mode |= 0x400000;
        past.extra_writes = {{4, setup.club + selection_club::judge, person_at(scenario, total - 2)}, {4, person_at(scenario, total - 2), total + 5}, {1, person_at(scenario, total - 2) + selection_person::human_flag, 1}};
        cases.push_back(places_case(scenario, past, name + "judge id past the table (written)"));
        auto no_club = setup;
        no_club.club = 0;
        no_club.mode |= 0x400000;
        cases.push_back(places_case(scenario, no_club, name + "no club, judge asked"));
        auto none = given_nothing;
        none.primary = none.secondary = 0;
        cases.push_back(places_case(scenario, none, name + "no places, arrays not given"));
        auto no_places = setup;
        no_places.primary = no_places.secondary = 0;
        cases.push_back(places_case(scenario, no_places, name + "no places"));
    }
    refill_cases(scenario, cases, refills, "career refill");
}
}

MATCH_MAN_CASES("selection places") { places_cases(scenario, cases); }
}

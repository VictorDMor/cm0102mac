// PLAYER_REGEN.CPP, block `detach`: cases for the player's (0x102e1f20) and the staff member's (0x102e21c0) departure from club and
// national team. Real people are the starting point (season-day): players with a club link (+0x24), managers and the people held in
// the club rows' slot arrays (+0x84, +0x160, +0x174, +0x190) for the staff cases; the job bytes (+0x62 club side, +0x60
// national side) are written over 0..17, 0x7f, 0x80 and 0xff (the jump tables' cases 0..15, the 16 and the unknown ones), each
// with the person linked to a club (and, for the national side, a club row standing in for the national team). Labels start with
// "detach:" (CM_PLAYER_REGEN_BLOCK=detach).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t detach_player = 0x102e1f20, detach_staff = 0x102e21c0;
constexpr std::int32_t club_table_slot = -0x7be8, club_count_slot = -0x7b50;
constexpr std::array<std::uint32_t, 22> jobs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0x7f, 0x80, 0xff, 0x10};

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> clubs;
    std::vector<std::uint32_t> managers, in_84, in_160, in_174, in_190;  // people found in the clubs' slots
    std::vector<std::uint32_t> last_84, last_160, last_174, last_190;    // people found in the last slot of each array
    std::vector<std::uint32_t> players;                                  // people with a player record and a club link
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto count = s.global(club_count_slot);
        for (std::uint32_t n = 0; n < count; ++n) {
            const auto club = s.global(club_table_slot) + n * 0x248;
            clubs.push_back(club);
            if (s.r32(club + 0x90)) managers.push_back(s.r32(club + 0x90));
            for (std::uint32_t k = 0; k < 3; ++k) if (s.r32(club + 0x84 + k * 4)) { in_84.push_back(s.r32(club + 0x84 + k * 4)); if (k == 2) last_84.push_back(s.r32(club + 0x84 + k * 4)); }
            for (std::uint32_t k = 0; k < 5; ++k) if (s.r32(club + 0x160 + k * 4)) { in_160.push_back(s.r32(club + 0x160 + k * 4)); if (k == 4) last_160.push_back(s.r32(club + 0x160 + k * 4)); }
            for (std::uint32_t k = 0; k < 7; ++k) if (s.r32(club + 0x174 + k * 4)) { in_174.push_back(s.r32(club + 0x174 + k * 4)); if (k == 6) last_174.push_back(s.r32(club + 0x174 + k * 4)); }
            for (std::uint32_t k = 0; k < 3; ++k) if (s.r32(club + 0x190 + k * 4)) { in_190.push_back(s.r32(club + 0x190 + k * 4)); if (k == 2) last_190.push_back(s.r32(club + 0x190 + k * 4)); }
        }
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x14) != 0 && s.r32(p + 0x24) != 0) players.push_back(p);
        }
    }
    static std::uint32_t at(const std::vector<std::uint32_t>& list, std::size_t n) { return list.empty() ? 0 : list[(n * 2654435761u) % list.size()]; }
};
}

PLAYER_REGEN_CASES("detach") {
    // 0x102e21c0 first (its cases and sides, each traced), then the full sweeps.
    const World w(scenario);
    std::fprintf(stderr, "detach: %zu clubs, %zu managers, slots held: %zu at +0x84, %zu at +0x160, %zu at +0x174, %zu at +0x190; %zu players with a club\n",
                 w.clubs.size(), w.managers.size(), w.in_84.size(), w.in_160.size(), w.in_174.size(), w.in_190.size(), w.players.size());
    if (w.managers.empty() || w.in_160.empty()) throw std::runtime_error("detach: the career has no managers or no scouting slots");
    const auto person_for = [&](std::uint32_t job, std::size_t n) {
        // Every third pick is a person in the LAST slot of the array (the loops' last iteration).
        const bool last = n % 3 == 2;
        switch (job) {
            case 2: return World::at(last && !w.last_84.empty() ? w.last_84 : w.in_84, n);
            case 8: case 15: return World::at(last && !w.last_160.empty() ? w.last_160 : w.in_160, n);
            case 9: return World::at(last && !w.last_174.empty() ? w.last_174 : w.in_174, n);
            case 10: return World::at(last && !w.last_190.empty() ? w.last_190 : w.in_190, n);
            default: return World::at(w.managers, n);
        }
    };
    const auto club_of = [&](std::uint32_t person) { return scenario.r32(person + 0x24); };
    // The manager manager's, training manager's, scout manager's and transfer manager's calls answered (some of their bodies reach the
    // file system, which a snapshot has no host for): what is compared is this routine's own order, arguments and stores. The human-club
    // call and the eligibility call take either answer, and the id test of case 5 (the person's id against the people count less 16)
    // both ways.
    const auto count_cell = scenario.toc(-0x7be4);
    for (std::size_t round = 0; round < 3; ++round)
        for (const bool national : {false, true})
            for (const std::uint32_t job : {5u, 12u, 8u, 15u, 9u})
                for (const std::uint32_t answer : {0u, 1u}) {
                    const auto person = person_for(job, job * 3 + round);
                    auto record = club_of(person);
                    if (!person || !record) continue;
                    std::vector<Write> memory{w32(person + (national ? 0x20 : 0x24), record), w32(person + (national ? 0x24 : 0x20), 0),
                                              w8(person + (national ? 0x60 : 0x62), job), w8(person + (national ? 0x62 : 0x60), 0)};
                    // Case 5: the person is the record's manager (written for the cases whose slots do not hold him) and his id is
                    // at or below the people count less 16 (the transfer manager is told) or above it.
                    if (job == 5 || job == 12) append(memory, {w32(record + 0x90, person)});
                    if (job == 5) append(memory, {w32(count_cell, answer ? scenario.r32(person) + 0x10 : scenario.r32(person) + 0x11)});
                    std::vector<Case::NthAnswer> nth{{0x1014c2e0, 1, answer}, {0x100d3c40, 1, answer}};
                    cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, person}}, .memory = memory, .nth_answers = nth,
                                     .answers = {{0x101aa680, 0}, {0x101aaa90, 0}, {0x103e9ac0, 0}, {0x103d2b30, 0}, {0x1030f950, 0}},
                                     .label = std::string("detach: staff answered, ") + (national ? "national" : "club") + " side job " + std::to_string(job) + " answer " + std::to_string(answer) + " person " + std::to_string(round)});
                }
    for (std::size_t round = 0; round < 3; ++round)
        for (const bool national : {false, true})
            for (const std::uint32_t job : {1u, 2u, 5u, 6u, 8u, 9u, 10u, 12u, 15u, 0u, 3u, 11u, 13u, 16u, 0x80u, 0xffu}) {
                auto person = person_for(job, job * 3 + round);
                if (!person) continue;
                // The record the side's link names: the person's own club (found through the people table's club link or the slot).
                auto record = club_of(person);
                if (!record) {
                    // A person found in a club's slots may have no club link: find the club that holds him.
                    for (const auto club : w.clubs) {
                        bool held = false;
                        for (const std::uint32_t offset : {0x84u, 0x160u, 0x174u, 0x190u, 0x90u})
                            for (std::uint32_t k = 0; k < 7; ++k) if (scenario.r32(club + offset + k * 4) == person) held = true;
                        if (held) { record = club; break; }
                    }
                }
                if (!record) continue;
                std::vector<Write> memory{w32(person + (national ? 0x20 : 0x24), record), w32(person + (national ? 0x24 : 0x20), 0),
                                          w8(person + (national ? 0x60 : 0x62), job), w8(person + (national ? 0x62 : 0x60), 0)};
                cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, person}}, .memory = memory,
                                 .label = std::string("detach: staff, ") + (national ? "national" : "club") + " side job " + std::to_string(job) + " person " + std::to_string(round)});
            }
    // The two sides at once (the club link and the national link both set), and a staff member with neither link.
    for (const std::uint32_t job_club : {0u, 5u, 9u, 15u}) for (const std::uint32_t job_nation : {1u, 8u, 12u, 0u}) {
        const auto person = World::at(w.managers, job_club + 3 * job_nation + 1);
        const auto record = club_of(person);
        if (!record) continue;
        cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, person}},
                         .memory = {w32(person + 0x24, record), w32(person + 0x20, World::at(w.clubs, job_nation + 4)), w8(person + 0x62, job_club), w8(person + 0x60, job_nation)},
                         .label = "detach: staff, both links, jobs " + std::to_string(job_club) + "/" + std::to_string(job_nation)});
    }
    cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, World::at(w.managers, 3)}},
                     .memory = {w32(World::at(w.managers, 3) + 0x24, 0), w32(World::at(w.managers, 3) + 0x20, 0), w8(World::at(w.managers, 3) + 0x62, 5), w8(World::at(w.managers, 3) + 0x60, 9)},
                     .label = "detach: staff, no links"});
    // Every job byte on the club side for the managers of a few clubs (the byte decides what the manager manager is told).
    for (std::size_t n = 0; n < 4; ++n)
        for (const auto job : jobs) {
            const auto person = World::at(w.managers, n + 11);
            const auto record = club_of(person);
            if (!record) continue;
            cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, person}}, .memory = {w8(person + 0x62, job), w8(person + 0x60, 0), w32(person + 0x20, 0)},
                             .label = "detach: staff manager, club job " + std::to_string(job) + " manager " + std::to_string(n)});
        }
    // The same through the national side's table (a club row as the national team) on real managers.
    for (std::size_t n = 0; n < 4; ++n)
        for (const auto job : jobs) {
            const auto person = World::at(w.managers, n + 21);
            const auto record = club_of(person);
            if (!record) continue;
            cases.push_back({.entry = detach_staff, .registers = {{3, 0}, {4, person}}, .memory = {w32(person + 0x20, record), w32(person + 0x24, 0), w8(person + 0x62, 0), w8(person + 0x60, job)},
                             .label = "detach: staff manager, national job " + std::to_string(job) + " manager " + std::to_string(n)});
        }
}

PLAYER_REGEN_CASES("detach") {
    // 0x102e1f20: players with a club link (and the national link of one), the squad slot found or not, the job bytes.
    const World w(scenario);
    if (w.players.size() < 20) throw std::runtime_error("detach: the career has too few players with a club");
    for (std::size_t n = 0; n < 40; ++n) {
        const auto person = World::at(w.players, n);
        cases.push_back({.entry = detach_player, .registers = {{3, 0}, {4, person}}, .label = "detach: player " + std::to_string(n)});
    }
    for (const bool club : {true, false})
        for (const bool nation : {true, false})
            for (const auto job_club : jobs)
                for (const std::uint32_t job_nation : {0u, 5u, 12u, 13u, 15u, 16u, 11u, 0x80u, 3u}) {
                    if (!club && !nation && job_nation != 0) continue;
                    const auto person = World::at(w.players, job_club * 7 + job_nation + (club ? 3 : 9));
                    const auto record = scenario.r32(person + 0x24);
                    std::vector<Write> memory{w8(person + 0x62, job_club), w8(person + 0x60, job_nation), w32(person + 0x24, club ? record : 0),
                                              w32(person + 0x20, nation ? World::at(w.clubs, job_club + 2) : 0)};
                    cases.push_back({.entry = detach_player, .registers = {{3, 0}, {4, person}}, .memory = memory,
                                     .label = std::string("detach: player, ") + (club ? "club" : "no club") + " " + (nation ? "nation" : "no nation") + ", jobs " + std::to_string(job_club) + "/" + std::to_string(job_nation)});
                }
    // The national team's record holds the player in a slot of its own (found on the national side's scan).
    for (const std::uint32_t slot : {0u, 3u, 49u}) {
        const auto person = World::at(w.players, 6 + slot);
        const auto nation_record = World::at(w.clubs, 90 + slot);
        if (nation_record == scenario.r32(person + 0x24)) continue;
        cases.push_back({.entry = detach_player, .registers = {{3, 0}, {4, person}}, .memory = {w32(person + 0x20, nation_record), w32(nation_record + 0x98 + slot * 4, person), w8(person + 0x62, 0), w8(person + 0x60, 0)},
                         .label = "detach: player in the national record's slot " + std::to_string(slot)});
    }
    // A player not in the squad slots of his club (the loop runs to 50), and one in the last slot.
    const auto person = World::at(w.players, 4);
    const auto club = scenario.r32(person + 0x24);
    std::vector<Write> absent, last;
    for (std::uint32_t k = 0; k < 50; ++k) { absent.push_back(w32(club + 0x98 + k * 4, 0x2000 + k)); last.push_back(w32(club + 0x98 + k * 4, k == 49 ? person : 0x2000 + k)); }
    cases.push_back({.entry = detach_player, .registers = {{3, 0}, {4, person}}, .memory = absent, .label = "detach: player not in the squad slots"});
    cases.push_back({.entry = detach_player, .registers = {{3, 0}, {4, person}}, .memory = last, .label = "detach: player in the last squad slot"});
}
}

#include "croatia_cup_entrants.hpp"
#include "croatia_cup_profile.hpp"
#include "cm0001_cup_order.hpp"
#include "league_base_detail.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace cm::aot::croatia_cup {
namespace {
using guest::Cpu;
using u32 = std::uint32_t;
using cm0001_cups::Candidate;
constexpr unsigned entry_size = 8;
constexpr unsigned reputation = 0x1ec, division_field = 0x30, nation_field = 0x2c;
constexpr int first_slot = -0x6a40, second_slot = -0x6a20;
constexpr std::array lower_slots{-0x7a0c, -0x6a28, -0x6a30, -0x6a2c, -0x6a34};

struct Seeded {
    u32 club;
    std::int16_t reputation;
    std::uint8_t seed;
};
}

std::int32_t select_entrants(Cpu& c, u32 cup, const Profile& profile) {
    const auto entrants = profile.entrants();
    const bool national = profile.second_division == SecondDivision::national_table;
    const bool sort_seeds = profile.second_order == SecondOrder::seed_then_reputation;
    const guest::Frame frame(c, 0x106bf9b0, 0xa0, {.first_gpr = 17});
    const auto pool = league_base::Pool::at(c);
    const auto strings = c.toc(-0x4120);
    c.set({{17, cup}, {18, strings}, {19, c.toc(comp::toc::message)}, {20, 0},
           {22, c.toc(first_slot)}, {23, c.toc(-0x7b04)}, {24, c.toc(comp::toc::club_count)},
           {25, pool.salt}, {26, pool.start}, {27, pool.end}, {28, pool.cursor},
           {29, c.toc(-0x6a48)}, {30, c.toc(-0x6a44)}, {31, c.toc(comp::toc::clubs)}});
    c.put16(cup + comp::competition::club_count, entrants);
    const auto entries = comp::invoke(c, comp::callee::allocate_block, 0x106bfa0c, {entrants * entry_size});
    c.put32(cup + comp::competition::table, entries);
    if (!entries) {
        guest::report_assertion(c, {0x106bfa2c, strings, strings + 0xc, strings + 0x19,
                                   frame.at(0x40), frame.at(0x38), 0x12b});
        return -1;
    }

    std::vector<Candidate> lower;
    lower.reserve(profile.lower_pool_limit);
    std::vector<Seeded> second(profile.second_places);
    std::vector<u32> first(profile.first_places);
    unsigned second_count = 0, first_count = 0;
    const auto first_id = c.i32(c.r(22));
    const auto second_a = c.i32(national ? c.toc(second_slot) : c.r(30));
    const auto second_b = national ? second_a : c.i32(c.r(29));
    std::array<std::int32_t, lower_slots.size()> lower_ids{};
    std::int32_t croatia = -1;
    if (profile.lower_places) {
        croatia = c.i32(c.toc(-0x7590));
        std::transform(lower_slots.begin(), lower_slots.end(), lower_ids.begin(),
                       [&](int slot) { return c.i32(c.toc(slot)); });
    }
    const auto clubs = c.u32(c.r(31));
    // The final two club records per nation belong to the national sides.
    const auto count = c.i32(c.r(24)) - 2 * c.i32(c.r(23));
    for (int i = 0; i < count; ++i) {
        const auto club = clubs + u32(i) * comp::club::size;
        const auto division = c.u32(club + division_field);
        if (!division) continue;
        const auto id = c.i32(division);
        if (id == first_id) {
            if (first_count < first.size()) first[first_count] = club;
            ++first_count;
        } else if (id == second_a || id == second_b) {
            if (second_count < second.size())
                second[second_count] = {club, sort_seeds ? c.i16(club + reputation) : std::int16_t{0}, 0};
            ++second_count;
        } else if (profile.lower_places && lower.size() < profile.lower_pool_limit &&
                   std::find(lower_ids.begin(), lower_ids.end(), id) != lower_ids.end()) {
            const auto nation = c.u32(club + nation_field);
            if (nation && c.i32(nation + 0x68) == croatia)
                lower.push_back({club, c.i16(club + reputation)});
        }
    }
    if (first_count != first.size() || second_count != second.size() || lower.size() < profile.lower_places) {
        // The originals leave rows uninitialised or overrun the array on a
        // mismatched database. Fail the draw before consuming any random numbers.
        std::fprintf(stderr, "Croatian Cup cannot draw %u entrants: %u first division, %u second division, %zu lower division\n",
                     entrants, first_count, second_count, lower.size());
        comp::invoke(c, comp::callee::free_block, 0x106bfbc0, {entries});
        c.put32(cup + comp::competition::table, 0);
        c.put8(c.toc(comp::toc::success), 0);
        return -1;
    }

    const auto draw = [&](unsigned bound) {
        pool.advance(0x106bfae4);
        return u32(std::int32_t(pool.sample()) % std::int32_t(bound));
    };
    unsigned index = 0;
    const auto put = [&](u32 club, int seed) {
        const auto row = entries + index++ * entry_size;
        c.put32(row, club);
        c.put8(row + 5, 0);
        if (seed >= 0) c.put8(row + 4, u32(seed));
    };
    if (profile.lower_places) {
        cm0001_cups::choose_front(lower, profile.lower_places, draw);
        for (unsigned i = 0; i < profile.lower_places; ++i) put(lower[i].club, 0);
    }
    auto seeded = std::span(second);
    const auto seeded_start = index;
    for (auto& team : seeded) {
        const auto slot = index;
        put(team.club, -1);
        // The original has already written the club and its status when a
        // wrapped random pool calls the guest generator for a new salt.
        if (!national)
            c.set({{18, team.club}, {19, (team.club - clubs) / comp::club::size},
                   {20, slot}, {21, slot * entry_size}});
        team.seed = std::uint8_t(draw(profile.seed_range));
        c.put8(entries + slot * entry_size + 4, team.seed);
    }
    if (sort_seeds) {
        // The Windows comparator takes the larger random byte first, then the
        // lower signed reputation. CRT ordering decides ties in both fields.
        cm0001_cups::sort_descending(seeded, [](const Seeded& team) {
            return int(team.seed) * 65536 - int(team.reputation);
        });
        index = seeded_start;
        for (const auto& team : seeded) put(team.club, team.seed);
    }
    for (unsigned i = 0; i < first_count; ++i) put(first[i], -1);
    c.set({{4, 0}, {5, entrants * entry_size}, {6, u32(count)}, {7, clubs + u32(count) * comp::club::size}});
    return 1;
}
}

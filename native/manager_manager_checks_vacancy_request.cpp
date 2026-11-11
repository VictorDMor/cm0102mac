// MANAGER_MANAGER.CPP checker cases for the vacancy block's request answer (manager_manager_vacancy.hpp): 0x101bdf80.
//
// The club is a record of the career with its fields the routine reads overridden (and undone afterwards); the news-row
// table, the board and standing tables, the limits and the interest records are the career's, with the values a case
// writes. The search's result (a list of nodes) is poked into the frame's list object; the club-state and event
// callees, the scout, the interest queries, the moves' tables and the news desks are answered; the message and list
// routines, the date routines and the board's review run for real. The draw table is written for the draws of each case.
#include "manager_manager_checks_tail.hpp"
#include <cstring>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t kRequest = 0x101bdf80;
constexpr std::uint32_t injured = 0x100c7d00, club_state = 0x1014c2e0, event = 0x106a15f0, scout_note = 0x10103510,
                        scout_flag = 0x100f70a0, search = 0x1048b180, next_a = 0x10738e30, next_b = 0x10738e00,
                        interest = 0x102d58c0, interest_byte = 0x102d5880, field_a = 0x100c4400, field_b = 0x100c4cf0,
                        interest_update = 0x102d51a0, move_slot = 0x10191fd0, job_check = 0x102b7c70,
                        finder = 0x10082f20, role_check = 0x10187680, move_remove = 0x1048a140, move_add = 0x10489900,
                        move_apply = 0x104888f0, desk = 0x102cc140, desk_c = 0x102cc400;
constexpr std::uint32_t area = scratch_base + 0x3000, chain_x = area, link = area + 0x100, table_cell = area + 0x200,
                        rows = scratch_base + 0x30000, links = area + 0x1000, nodes = area + 0x2000, found = area + 0x300,
                        scout = area + 0x400;
constexpr unsigned club_index = 9;

using Put = std::function<void(unsigned, std::uint32_t, std::uint32_t)>;

void add_request(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0xdf80};
    const auto sp = std::uint32_t(sc.s.r<4>(4));
    const auto frame_list = sp - 0xf20 + 0x70;
    const auto club = sc.club(club_index);
    const auto id = sc.r32(club);
    const auto constants = sc.toc(-0x536c);
    const auto limit = std::bit_cast<double>(std::uint64_t(sc.s.memory.read<8>(constants + 0x2d0)));
    for (unsigned i = 0; i < 2400; ++i) {
        Case item{.entry = kRequest, .label = "vacancy request " + std::to_string(i)};
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        const bool focus = i % 3 == 0;  // kind 2 with a pending move that passes the early tests
        const bool null_club = i % 61 == 1, no_link = !focus && i % 37 == 2, no_manager = i % 41 == 3, no_chain = i % 43 == 4;
        const std::uint32_t kind = focus ? 2 : r.pick<std::uint32_t>({1, 1, 1, 2, 2, 2, 0, 3, 5, 0x101, 0x102, 0xff, 4, 0x81, 7});
        item.registers = {{3, table_cell}, {4, null_club ? 0 : club}, {5, kind}};
        put(4, club + 0x30, no_link ? 0 : link);
        put(4, club + 0x90, no_manager ? 0 : chain_x);
        put(4, chain_x + 0x1c, no_chain ? 0 : link);
        const std::int32_t reputation = focus ? r.pick<std::int32_t>({0x1000, 0}) : r.pick<std::int32_t>({0xcb2, 0xcb3, 0x1000, 0x7fff, -0x8000, 0, 0x2000, 0x1e47, 0x1483});
        put(2, club + 0x1ec, std::uint32_t(reputation) & 0xffff);
        put(4, table_cell, rows);
        put(1, rows + id * 0x4c, focus ? r.below(2) : r.pick<std::uint32_t>({0, 1, 2, 3, 4, 0xff, 0x7e, 0x7f, 0x80}));
        put(2, rows + id * 0x4c + 6, r.pick<std::uint32_t>({0, 5, 0x2710, 0x2711, 0x7fff, 0x8000, 0xffff, 1}));
        put(1, sc.global(-0x7984) + id * 8 + 4, r.pick<std::uint32_t>({0xff, 0xfe, 0, 1, 0x80, 0xfd}));
        const auto standing = sc.global(-0x79f0);
        put(2, sc.r32(standing) + id * 0x4c + 6, r.pick<std::uint32_t>({0, 5, 0x2710, 0x2711, 0x7fff, 0x8000, 1}));
        const auto date = sc.toc(-0x7b80);
        const std::int32_t day = r.pick<std::int32_t>({300, 300, 0x155, 0x7fff, 0xffff, 1}), year = 2001;
        put(2, date, std::uint32_t(day) & 0xffff);
        put(2, date + 2, std::uint32_t(year));
        put(1, date + 4, 0);
        {
            const unsigned held = focus ? r.below(7) : 10;  // the number of slots holding a player (the first ones)
            for (unsigned k = 0; k < 10; ++k) put(4, club + 0x98 + 4 * k, k < held && (focus || r.chance(70)) ? sc.person(30 + k) : 0);
        }
        // The search's result: nodes chained by +8 links, with the fields both kinds read.
        const unsigned count = focus ? 1 + r.below(2) : r.pick<unsigned>({0, 1, 2, 3, 4});
        std::vector<Write> poke;
        poke.push_back({2, frame_list, count});
        poke.push_back({4, frame_list + 4, count ? links : 0});
        poke.push_back({4, frame_list + 8, count ? links : 0});
        for (unsigned n = 0; n < count; ++n) {
            const auto l = links + n * 0x20, node = nodes + n * 0x100;
            poke.push_back({4, l, node});
            poke.push_back({4, l + 8, n + 1 < count ? l + 0x20 : 0});
            const bool today = r.chance(30);
            poke.push_back({2, node + 0x2a, today ? std::uint32_t(day) & 0xffff : r.pick<std::uint32_t>({std::uint32_t(day) & 0xffff, 299, 0x7d0, 0x8000})});
            poke.push_back({2, node + 0x28, today ? year : r.pick<std::uint32_t>({year, 1, 0xffff, 2000})});
            poke.push_back({1, node + 0x43, focus && r.chance(85) ? 0xffu : r.pick<std::uint32_t>({0xff, 0, 1, 0x7f, 0x80, 0xfe})});
            poke.push_back({4, node + 0x14, focus || r.chance(70) ? link : link + 4});
            poke.push_back({2, node + 0x2e, r.pick<std::uint32_t>({0, 1, 2, 3, 4})});
            for (unsigned k = 0; k < 0x50; k += 4) poke.push_back({4, node + 0x50 + k, r.next()});
        }
        item.pokes.push_back({search, poke});
        item.pokes.push_back({next_a, {{4, frame_list + 4, 0}, {4, frame_list + 8, 0}}});
        item.pokes.push_back({next_b, {{4, frame_list + 4, 0}, {4, frame_list + 8, 0}}});
        item.answers = {{injured, r.chance(5) && !focus ? 1u : 0u}, {club_state, focus || r.chance(50) ? 1u : 0u}, {event, 0},
                        {scout_note, r.chance(50) ? scout : 0}, {scout_flag, 0}, {search, 0},
                        {interest_byte, r.pick<std::uint32_t>({0, 3, 6, 7, 8, 0x80, 0xff, 0x106})}, {interest, 0},
                        {field_a, r.next()}, {field_b, r.next()}, {interest_update, 0},
                        {move_slot, r.pick<std::uint32_t>({0xffffffff, 0, 5, 0x21, 0x22, 0x80, 0x1f, 3, 4, 6})},
                        {job_check, r.chance(60) ? 1u : 0u}, {finder, r.chance(85) ? found : 0},
                        {role_check, r.pick<std::uint32_t>({2, 1, 2, 0})}, {move_remove, 0}, {move_add, 0}, {move_apply, 0},
                        {desk, 0}, {desk_c, 0}};
        put(1, found + 0x44, r.pick<std::uint32_t>({0x14, 0x15, 0x30, 0x7f, 0x80, 0}));
        item.f1 = r.pick<double>({limit - 1, limit, limit + 1, 0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), 1e9});
        const auto slots = sc.toc(-0x7b44);
        for (unsigned n = 0; n < 0x22; n += 3) {
            put(2, slots + n * 0x4c + 0x3c, r.pick<std::uint32_t>({0x3c, 0x3d, 0x3b, 0x80, 0, 0x154, 0xffff}));
            put(2, slots + n * 0x4c + 0x3e, r.pick<std::uint32_t>({std::uint32_t(year), 1, 2000}));
        }
        tail::put_draws(sc, item, r.chance(25), r.next(), {r.pick<std::uint32_t>({0, 1, 2, 10, 20, 200, 400, r.next() % 400}), r.pick<std::uint32_t>({0, 10, 20, 7, 100}), r.below(30), r.below(30), r.below(30), r.below(30), r.below(30)});
        cases.push_back(std::move(item));
    }
}
}

MANAGER_MANAGER_CASES("vacancy_request") {
    if (scenario.match) return;
    add_request(scenario, cases);
}
}

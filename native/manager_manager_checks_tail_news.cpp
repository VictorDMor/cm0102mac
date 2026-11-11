// MANAGER_MANAGER.CPP checker cases for the tail block's small routines (manager_manager_tail.hpp): 0x101c42c0, the news
// routines 0x101c1180 and 0x101c1a10, the search-and-announce 0x101c1830 and the queue walk 0x101c0fc0.
//
// A scene of scratch records carries each call (persons, clubs, their record links, the queue and its linked nodes);
// the career's date cell, draw table and tables are the real ones with the values a case writes (undone afterwards).
// The news desk, the effects, the node removal, the assertion routines and the search are answered (the search's
// result is poked into the caller's frame where a case needs it); the message buffer's routines, the date routines
// and the list iteration run for real, so each field and each argument is compared at its call.
#include "manager_manager_checks_tail.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t kCheck = 0x101c42c0, kMoveNews = 0x101c1180, kRivalNews = 0x101c1a10, kAnnounce = 0x101c1830,
                        kDue = 0x101c0fc0;
constexpr std::uint32_t news_desk = 0x102cc140, news_desk_b = 0x102cc280, news_desk_c = 0x102cc400, apply_effect = 0x100e0410,
                        remove_node = 0x10738e60, search = 0x1048b180, prepare = 0x10191fd0,
                        append = 0x10738d10;
constexpr std::uint32_t person = scratch_base + 0x1000, club = scratch_base + 0x1400, region = scratch_base + 0x1500,
                        manager = scratch_base + 0x1600, refs = scratch_base + 0x2000, other_a = scratch_base + 0x1800,
                        other_b = scratch_base + 0x1900, list = scratch_base + 0x3000, queue = scratch_base + 0x3100,
                        links = scratch_base + 0x4000, nodes = scratch_base + 0x5000, queued = scratch_base + 0x6000;

using Put = std::function<void(unsigned, std::uint32_t, std::uint32_t)>;

// A person record with its three links (ids at +0x20) and its club with a region record carrying the league id.
void put_person(const Put& put, std::uint32_t at, std::uint32_t at_club, std::uint32_t id, bool with_club, bool with_region) {
    put(4, at, id);
    put(4, at + 0x24, with_club ? at_club : 0);
    for (unsigned n = 0; n < 3; ++n) {
        put(4, at + 0x28 + 4 * n, refs + 0x100 * n);
        put(4, refs + 0x100 * n + 0x20, 0xa0 + id % 7 + n);
    }
    put(4, at_club + 0x2c, with_region ? region : 0);
    put(4, region + 0x68, 0x3300 + id % 11);
}

void add_check(const Scenario& sc, std::vector<Case>& cases) {
    for (const std::uint32_t kind : {2u, 0x102u, 0xffffff02u, 0x82u, 1u, 0u, 3u, 0xffu, 0x12u})
        for (const std::uint32_t standing : {0x14u, 0x13u, 0x15u, 0x94u, 0u, 0xffu, 0x1014u & 0xffu})
            cases.push_back({.entry = kCheck, .registers = {{4, sc.club(3)}, {5, kind}},
                             .memory = {{1, sc.club(3) + 0x240, standing}},
                             .label = "tail check kind " + std::to_string(kind) + " standing " + std::to_string(standing)});
}

void add_move_news(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0x1180a};
    for (unsigned i = 0; i < 160; ++i) {
        const bool real_desk = i % 40 == 0;
        Case item{.entry = kMoveNews, .label = "tail move news " + std::to_string(i)};
        const bool null_person = i % 29 == 1, with_club = i % 17 != 2, with_region = i % 13 != 3;
        item.registers = {{3, list}, {4, null_person ? 0 : person}, {5, r.next()}, {6, r.next()},
                          {7, r.pick<std::uint32_t>({0, 1, 0x7fff, 0x8000, 0xffff, r.next()})}, {8, r.next()}};
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        put_person(put, person, club, r.next(), with_club, with_region);
        if (!real_desk) item.answers = {{news_desk, 0}};
        cases.push_back(std::move(item));
    }
    (void)sc;
}

// A club with its manager (+0x90), league (+0x30), region (+0x2c) and reputation, for the rival news.
void put_club(const Put& put, std::uint32_t at, std::uint32_t id, std::int32_t reputation, bool with_manager, bool with_league) {
    put(4, at, id);
    put(4, at + 0x30, with_league ? other_b : 0);
    put(4, at + 0x2c, region);
    put(4, at + 0x90, with_manager ? manager : 0);
    put(2, at + 0x1ec, std::uint32_t(reputation) & 0xffff);
    put(4, manager, 0x2222);
    for (unsigned n = 0; n < 3; ++n) {
        put(4, manager + 0x28 + 4 * n, refs + 0x100 * n);
        put(4, refs + 0x100 * n + 0x20, 0xb0 + n);
    }
    put(4, other_a, 0x77);
    put(4, other_b, 0x88);
}

void add_rival_news(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0x1a10a};
    for (unsigned i = 0; i < 200; ++i) {
        Case item{.entry = kRivalNews, .label = "tail rival news " + std::to_string(i)};
        const auto reputation = r.pick<std::int32_t>({0x1675, 0x1676, 0x1677, 0x7fff, -0x8000, 0, 0x2000, -1});
        const bool null_club = i % 31 == 1, no_manager = i % 19 == 2, no_league = i % 23 == 3, null_b = i % 37 == 4;
        item.registers = {{3, list}, {4, null_club ? 0 : club}, {5, other_a}, {6, null_b ? 0 : other_b}};
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        put_club(put, club, r.next(), reputation, !no_manager, !no_league);
        put(4, region + 0x68, 0x3300);
        tail::put_draws(sc, item, r.chance(30), r.next(), {r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, 9, 12}), r.pick<std::uint32_t>({0, 1, 2, 3, 6, 9, 5})});
        item.answers.push_back({news_desk, 0});
        item.answers.push_back({news_desk_b, 0});
        item.answers.push_back({news_desk_c, 0});
        cases.push_back(std::move(item));
    }
}

// The announce search: the frame of 0x101c1830 is at sp-0x80, its list object at +0x54 (count, head, cursor).
void add_announce(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0x1830a};
    const auto sp = std::uint32_t(sc.s.r<4>(4));
    const auto frame_list = sp - 0x80 + 0x54;
    for (unsigned i = 0; i < 400; ++i) {
        Case item{.entry = kAnnounce, .label = "tail announce " + std::to_string(i)};
        const bool null_club = i % 31 == 1, no_manager = i % 19 == 2, no_league = i % 23 == 3;
        item.registers = {{3, list}, {4, null_club ? 0 : club}};
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        put_club(put, club, r.next(), r.pick<std::int32_t>({0x1675, 0x1676, 0x7fff, 0x2000}), !no_manager, !no_league);
        put(4, list + 0x10, queued);
        const auto date = sc.toc(-0x7b80);
        constexpr std::uint32_t today = 300, year = 2001;
        put(2, date, today);
        put(2, date + 2, year);
        put(1, date + 4, 0);
        // The search's result list poked into the frame: n nodes chained by +8 links, the first n_a of them skipped
        // for different reasons.
        const unsigned count = r.pick<unsigned>({0, 1, 2, 3, 4, 5});
        std::vector<Write> poke;
        poke.push_back({2, frame_list, count});
        poke.push_back({4, frame_list + 4, count ? links : 0});
        for (unsigned n = 0; n < count; ++n) {
            const auto link = links + n * 0x20, node = nodes + n * 0x100;
            poke.push_back({4, link, node});
            poke.push_back({4, link + 8, n + 1 < count ? link + 0x20 : 0});
            const bool is_today = r.chance(30);
            // The node's halfword at +0x2a is compared with the date's first halfword (the day), +0x28 with its second.
            poke.push_back({2, node + 0x2a, is_today ? today : r.pick<std::uint32_t>({today, 0x7d0, 0xffff, 0x8000, 299})});
            poke.push_back({2, node + 0x28, is_today ? year : r.pick<std::uint32_t>({year, 1, 0xffff, 2000})});
            poke.push_back({1, node + 0x40, r.pick<std::uint32_t>({0, 1, 0x80, 0xff})});
            poke.push_back({1, node + 0x43, r.pick<std::uint32_t>({0xff, 0, 1, 0x7f})});
            poke.push_back({4, node + 0x14, r.pick<std::uint32_t>({0, other_b, other_b})});
            poke.push_back({4, node + 0x1c, r.chance(50) ? (null_club ? club : club) : other_a});
            poke.push_back({4, node + 0x20, other_a});
        }
        item.pokes.push_back({search, poke});
        // The destructor (0x10738c20, run for real: the generated side never answers it) must find an empty chain.
        // (the walk follows the cursor, which the routine copied from the head before the first step)
        item.pokes.push_back({0x10738e00, {{4, frame_list + 4, 0}}});
        if (r.chance(50)) item.answers.push_back({append, 0});
        item.answers.push_back({search, 0});
        item.answers.push_back({prepare, 0});
        item.answers.push_back({news_desk, 0});
        item.answers.push_back({news_desk_b, 0});
        item.answers.push_back({news_desk_c, 0});
        tail::put_draws(sc, item, r.chance(30), r.next(), {r.pick<std::uint32_t>({0, 1, 2, 3, 6, 5, 7}), r.pick<std::uint32_t>({0, 1, 3, 6, 4})});
        cases.push_back(std::move(item));
    }
}

// The due queue: the queue object at list+0x14 with count/head/cursor, its linked nodes' data records.
void add_due(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0x0fc0a};
    const unsigned people = 40;
    for (unsigned i = 0; i < 400; ++i) {
        Case item{.entry = kDue, .registers = {{3, list}}, .label = "tail due " + std::to_string(i)};
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        put(4, list + 0x14, queue);
        const unsigned count = r.pick<unsigned>({0, 1, 2, 3, 4, 5});
        const unsigned chained = r.chance(8) && count ? count - 1 : count;  // a short chain: the walk meets a null node
        put(2, queue, count);
        put(4, queue + 4, chained ? links : 0);
        put(4, queue + 0xc, 0xdeadbeef);  // the cursor, set by the routine
        const auto date = sc.toc(-0x7b80);
        constexpr std::int32_t today = 300, year = 2001;
        put(2, date, today);
        put(2, date + 2, year);
        put(1, date + 4, 0);
        for (unsigned n = 0; n < chained; ++n) {
            const auto link = links + n * 0x20, node = nodes + n * 0x100;
            put(4, link, node);
            put(4, link + 8, n + 1 < chained ? link + 0x20 : 0);
            const std::uint32_t person_index = r.below(people);
            put(4, node, person_index);
            put(1, node + 4, r.pick<std::uint32_t>({0, 1, 2, 0x80, 0xff, 5}));
            // the date against today's: earlier, equal, later in the year halfword, and in the day halfword when equal
            const auto year_choice = r.below(4);
            const std::uint32_t node_year = year_choice == 0 ? year - 1 : year_choice == 1 ? year + 1 : year;
            put(2, node + 0xa, node_year);
            put(2, node + 8, r.pick<std::int32_t>({today, today - 1, today + 1, -1, 0, 0x7fff, -0x8000, 0x4000, -0x4000, today ^ 1, 0x8000, 1}) & 0xffff);
            for (unsigned k = 0; k < 4; ++k) put(1, node + 0x10 + k, r.pick<std::uint32_t>({0, 5, 0x7f, 0x80, 0xff, 0xfe, 0x20}));
            for (const std::uint32_t off : {0x14u, 0x15u, 0x18u, 0x1cu}) put(1, node + off, r.next());
            put(2, node + 0x16, r.next());
            put(2, node + 0x1a, r.next());
        }
        // The people's club, regions and links come from the real records; some persons lose their club or its region.
        for (unsigned n = 0; n < people; n += 7) {
            const auto p = sc.person(n);
            if (r.chance(30)) put(4, p + 0x24, 0);
        }
        item.answers = {{news_desk, 0}, {apply_effect, 0}, {remove_node, 0}};
        if (r.chance(50))
            for (const std::uint32_t report : {0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u}) item.answers.push_back({report, 0});
        cases.push_back(std::move(item));
    }
}
}

MANAGER_MANAGER_CASES("tail_news") {
    if (scenario.match) return;
    add_check(scenario, cases);
    add_move_news(scenario, cases);
    add_rival_news(scenario, cases);
    add_announce(scenario, cases);
    add_due(scenario, cases);
}
}

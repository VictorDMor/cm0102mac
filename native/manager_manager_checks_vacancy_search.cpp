// MANAGER_MANAGER.CPP checker cases for the vacancy block's candidate search (manager_manager_vacancy.hpp): 0x101c00d0
// (the roster test), 0x101bf5c0 (the candidates a club takes) and 0x101b9130 (the news that reports them).
//
// A scene of scratch records carries each call (a club record of the career with its +0x80 chain overridden, the move,
// the link, the news rows); the staff routines, the finder, the link record and the roster are answered, and the staff
// list and the roster written into the frames the callees would fill (pokes at the callees' dispatch, with the frame
// bases of the nested entries). The module's draw table is written for the draws of each case.
#include "manager_manager_checks_tail.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t kRoster = 0x101c00d0, kGather = 0x101bf5c0, kEvent = 0x101b9130;
constexpr std::uint32_t roster_get = 0x1029ec40, injured = 0x100c7d00, staff_a = 0x100db060, staff_b = 0x100dafa0,
                        finder = 0x10082f20, link_record = 0x10459b50, desk = 0x102cc140, message_a = 0x1030d4a0,
                        message_b = 0x1030dcd0;
constexpr std::uint32_t side = scratch_base + 0x2000, chain_x = side, chain_y = side + 0x100, link = side + 0x200,
                        move = side + 0x300, table_cell = side + 0x400, mode = side + 0x500, outs = side + 0x510,
                        ids = side + 0x600, rec = side + 0x700, found = side + 0x800, zs = side + 0x1000,
                        rows = scratch_base + 0x20000, person_ptr = side + 0x900, second_ptr = side + 0xa00;
constexpr unsigned club_index = 7, first_candidate = 20;

using Put = std::function<void(unsigned, std::uint32_t, std::uint32_t)>;

std::uint32_t entry_sp(const Scenario& sc) { return std::uint32_t(sc.s.r<4>(4)); }

// The roster the 0x1029ec40 callee leaves in the frame whose base is `base`: 2 squads of 20 slots (index word at
// +0x1cc, rating byte at +0x201, 0x4c apart, 0x664 per squad). Some slots hold the candidates' person indexes.
void put_roster(const Scenario& sc, Case& item, Rng& r, std::uint32_t base, unsigned candidates) {
    std::vector<Write> writes;
    for (unsigned squad = 0; squad < 2; ++squad)
        for (unsigned slot = 0; slot < 20; ++slot) {
            const auto at = base + squad * 0x664 + slot * 0x4c;
            const auto pick = r.below(10);
            const std::uint32_t index = pick < 4 ? first_candidate + r.below(candidates ? candidates : 1) : pick < 6 ? 0xffffffffu : r.below(40);
            writes.push_back({4, at + 0x1cc, index});
            writes.push_back({1, at + 0x201, r.pick<std::uint32_t>({0, 3, 6, 7, 8, 9, 0xff, 5})});
        }
    item.pokes.push_back({roster_get, writes});
    item.answers.push_back({roster_get, r.chance(90) ? 1u : 0u});
    (void)sc;
}

struct Scene {
    std::uint32_t club;
    unsigned candidates;
};

// The club, the move, the staff list and the callee answers for a gather at the frame base `base`; `draws` come first.
Scene gather_scene(const Scenario& sc, Case& item, Rng& r, std::uint32_t base) {
    const auto club = sc.club(club_index);
    const auto id = sc.r32(club);
    const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    put(4, club + 0x80, r.chance(3) ? 0u : chain_x);
    put(4, chain_x + 0x1c, r.chance(3) ? 0u : chain_y);
    put(1, chain_y + 0x36, r.pick<std::uint32_t>({0, 1, 5, 10, 0x7f, 0x80, 0xff, 0xf6, 3}));
    put(1, chain_x + 0x63, r.pick<std::uint32_t>({0, 1, 2, 0xff, 0x80, 5}));
    put(4, club + 0x30, link);
    const std::int32_t reputation = r.pick<std::int32_t>({0x1000, 0x2000, 0x7fff, -0x8000, 0, 0x300, 0xc00});
    put(2, club + 0x1ec, std::uint32_t(reputation) & 0xffff);
    const auto link_word = r.pick<std::uint32_t>({0, 1, 2, 3, 5});
    put(4, link, link_word);
    if (const auto entry = sc.r32(sc.global(-0x7b5c) + link_word * 4); entry && r.chance(15)) put(4, entry + 8, 0);  // (a real record, undone)
    put(4, move + 0x14, r.pick<std::uint32_t>({link, link, link, link, link + 4, 0}));
    put(4, move + 8, r.next());
    put(4, table_cell, rows);
    put(1, mode, r.pick<std::uint32_t>({0, 1, 0xff, 0x80, 2}));
    // The staff list of up to six real persons with their records' fields.
    const unsigned count = r.below(7);
    std::vector<Write> staff;
    staff.push_back({1, base + 0x38, r.chance(5) ? 0x80u : count});
    for (unsigned k = 0; k < 6; ++k) {
        const auto p = sc.person(first_candidate + k);
        staff.push_back({4, base + 0x54 + 4 * k, k < count ? (r.chance(8) ? 0u : p) : 0u});
        put(4, p + 0x14, r.chance(6) ? 0u : zs + 0x40 * k);
        put(2, zs + 0x40 * k + 0xa, std::uint32_t(reputation + r.pick<std::int32_t>({-0x300, -0x2ee, -0x2ed, -1, 0, 1, 0x100, -0x400, 0x400})) & 0xffff);
        put(1, p + 0x62, r.chance(90) ? 0xbu : r.below(20));
        put(2, p + 0x5c, r.pick<std::uint32_t>({0x23, 0x24, 0x25, 0x0, 0xff, 0x80, 0xff23, 0x1223}));
    }
    item.pokes.push_back({staff_a, staff});
    item.pokes.push_back({staff_b, staff});
    item.answers.push_back({staff_a, 0});
    item.answers.push_back({staff_b, 0});
    // The finder's record names the club's own index (or another one, or nothing).
    put(4, found + 4, r.chance(85) ? id : id + 1);
    item.answers.push_back({finder, r.chance(90) ? found : 0});
    // The link record.
    put(1, rec + 4, r.pick<std::uint32_t>({0, 8, 9, 20, 0xff}));
    put(1, rec + 0x77, r.next()); put(1, rec + 0x76, r.next());
    put(4, rec + 0x1c, r.pick<std::uint32_t>({2, 0, 5}));
    put(4, rec + 0x30, r.pick<std::uint32_t>({2, 0, 7}));
    item.answers.push_back({link_record, r.chance(85) ? rec : 0});
    item.answers.push_back({injured, r.chance(5) ? 1u : 0u});
    put_roster(sc, item, r, base - 0xe50, count);
    return {club, count};
}

void put_stack(Case& item, std::uint32_t sp, std::initializer_list<std::uint32_t> words) {
    std::uint32_t at = sp + 0x38;
    for (const auto word : words) { item.memory.push_back({4, at, word}); at += 4; }
}

void add_roster(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0xc00d0};
    for (unsigned i = 0; i < 300; ++i) {
        Case item{.entry = kRoster, .label = "vacancy roster " + std::to_string(i)};
        item.registers = {{3, rows}, {4, i % 37 == 1 ? 0 : sc.person(first_candidate)}, {5, r.next()}};
        put_roster(sc, item, r, entry_sp(sc) - 0xe50, 1);
        cases.push_back(std::move(item));
    }
}

void add_gather(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0xbf5c0};
    const auto sp = entry_sp(sc);
    for (unsigned i = 0; i < 700; ++i) {
        Case item{.entry = kGather, .label = "vacancy gather " + std::to_string(i)};
        const auto scene = gather_scene(sc, item, r, sp - 0x190);
        const bool no_club = i % 53 == 1, with_ids = i % 7 != 0;
        item.registers = {{3, table_cell}, {4, move}, {5, no_club ? 0 : scene.club}, {6, with_ids ? ids : 0}, {7, r.next()},
                          {8, mode}, {9, outs}, {10, outs + 1}};
        put_stack(item, sp, {outs + 2, outs + 3, outs + 4});
        tail::put_draws(sc, item, r.chance(25), r.next(), {r.below(200), r.below(200)});
        cases.push_back(std::move(item));
    }
}

void add_event(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0xb9130};
    const auto sp = entry_sp(sc);
    for (unsigned i = 0; i < 400; ++i) {
        Case item{.entry = kEvent, .label = "vacancy event " + std::to_string(i)};
        const auto scene = gather_scene(sc, item, r, sp - 0x1a0 - 0x190);
        const Put put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        put(4, person_ptr, scene.club);  // (r4 is the club record itself: the id is read through it)
        put(4, second_ptr, r.next());
        const bool null4 = i % 41 == 1, null5 = i % 43 == 2, null6 = i % 47 == 3, no_link = i % 29 == 4;
        if (no_link) put(4, move + 0x14, 0);
        item.registers = {{3, rows}, {4, null4 ? 0 : scene.club}, {5, null5 ? 0 : second_ptr}, {6, null6 ? 0 : move},
                          {7, r.pick<std::uint32_t>({0, 1, 0x7fff, 0x8000, 0xffff, r.next()})},
                          {8, r.pick<std::uint32_t>({0, 5, 7, 0xffffffff, 0x80000000, r.next()})},
                          {9, r.pick<std::uint32_t>({0, 5, 6, 0xffffffff, 0x7fffffff, r.next()})}, {10, r.next()}};
        put_stack(item, sp, {r.next()});
        item.answers.push_back({message_a, 0});
        item.answers.push_back({message_b, 0});
        if (r.chance(90)) item.answers.push_back({desk, 0});
        tail::put_draws(sc, item, r.chance(25), r.next(), {r.below(200), r.below(200), r.below(200)});
        cases.push_back(std::move(item));
    }
    (void)person_ptr;
}
}

MANAGER_MANAGER_CASES("vacancy_search") {
    if (scenario.match) return;
    add_roster(scenario, cases);
    add_gather(scenario, cases);
    add_event(scenario, cases);
}
}

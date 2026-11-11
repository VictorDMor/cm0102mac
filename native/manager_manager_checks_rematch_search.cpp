// MANAGER_MANAGER.CPP checker cases for the rematch block's walkers (manager_manager_rematch.hpp):
// 0x101abda0, 0x101aaa90, 0x101ac280, 0x101acf00, 0x101aa510 and 0x101aa680.
//
// The career has 101016 people and 7981 clubs, and the walkers visit all of them, so each case writes a short
// count into the count cells (the first N people and clubs are the world's; the "last 16" of the original are
// then people N-16..N-1). The first N people are scrambled from menus of cut values: staff records (scratch,
// with the three ratings), home regions, clubs and their rivals, posts, statuses, the rows of 100 bytes the
// walkers test, so that every test of every walker is passed by some people and failed by others. The routines
// of other modules that answer yes/no (the club flags, the deal records, the date score ...) are answered per
// case from a menu; the module's own owners (the assessment, the list search, the draw) and the news writers
// run for real. The module's draw table is written per case (the halfwords after the cursor), so the chances
// are chosen. The offer list of the object is the world's (53 entries), so a club is found or missing by its
// number.
#include "manager_manager_checks_rematch.hpp"
#include <algorithm>
#include <string>

namespace cm::aot::manager_manager_checks {
using namespace rematch;
namespace {
constexpr std::uint32_t plan = 0x101aaa90, approach = 0x101ac280, best_score = 0x101abda0, plan_all = 0x101acf00,
                        settle = 0x101aa510, end_tenure = 0x101aa680;
namespace routine {
constexpr std::uint32_t flag = 0x100c7d00, club_ok = 0x100ce250, manager_flag = 0x100d3c40, pending = 0x100c96e0,
                        deals = 0x10083110, deal = 0x10082f20, deal_for = 0x10082f90, days = 0x10718130,
                        owner_gate = 0x1014c2e0, dispatch_a = 0x10147e70, dispatch_b = 0x10149df0, notify = 0x100da320,
                        extra = 0x102eb610, answered = 0x100ce320, deal_hook = 0x10382890;
}
constexpr std::uint32_t slot_word = scratch_base + 0x40000;

struct Setup {
    Setup(const Scenario& scenario, Case& c, Rng& rng) : sc(scenario), item(c), r(rng), put{c} {}
    const Scenario& sc;
    Case& item;
    Rng& r;
    Put put;
    // Null homes and club regions are for the walker that never reaches the assessment with them (0x101abda0).
    bool nulls = false;
    // How often the club flag (0x100c7d00: a human club) is answered yes; a human club sends the post routines to the screens.
    unsigned human = 30;
    std::uint32_t object() const { return sc.global(-0x79f0); }

    // The count cells, the draw table and the answers of the routines of other modules.
    void world(unsigned people, unsigned clubs) {
        if (people) put(4, sc.toc(-0x7be4), people);
        if (clubs) put(4, sc.toc(-0x7b50), clubs);  // the human clubs are the last of this count (0x100c7d00)
    }
    // The event date against today's: after it (year first, then day) sends 0x101aaa90's dates the other way.
    void event_date() {
        const auto event = sc.toc(-0x7a9c), today = sc.toc(-0x7b80);
        if (r.chance(50)) return;
        put(2, today + 2, 2001);
        put(2, today, r.pick<std::uint32_t>({1, 100, 300, 354, 364}));
        put(2, event + 2, r.pick<std::uint32_t>({2000, 2001, 2001, 2002}));
        put(2, event, r.pick<std::uint32_t>({0, 50, 100, 300, 360, 364, 365, 0x8000}));
    }
    void draws(bool wrap) {
        event_date();
        const auto first = sc.toc(-0x7b8c), last = sc.r32(sc.toc(-0x7b88));
        put(4, sc.toc(-0x7b84), wrap ? last : first);
        put(4, sc.toc(-0x7b90), r.chance(20) ? r.below(0x30000) : 0);
        for (unsigned n = 0; n < 160; ++n) {
            static const std::uint32_t menu[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 20, 199, 200, 400, 499, 500, 1000, 1999, 2000, 3999, 4000};
            put(2, first + 2 * (n + (wrap ? 0 : 1)), r.chance(60) ? menu[r.below(sizeof menu / sizeof *menu)] : r.below(0x10000));
        }
    }
    void answers(unsigned flavour) {
        const bool deep = flavour % 3 != 0;
        const auto yes = [&](unsigned percent) { return std::uint32_t(r.chance(percent)); };
        item.answers = {{routine::flag, yes(human)}, {routine::club_ok, yes(50)}, {routine::manager_flag, yes(deep ? 85 : 40)},
                        {routine::pending, r.chance(10) ? scratch::spare : 0}, {routine::deals, r.chance(10) ? scratch::deal : 0},
                        {routine::deal, r.chance(60) ? scratch::deal : 0}, {routine::deal_for, r.chance(60) ? scratch::deal : 0},
                        {routine::days, r.pick<std::uint32_t>({0x50, 0x96, 0x95, 0xb3, 0xb4, 0x100, 0x1000, 0xd1, 0xd2})},
                        {routine::owner_gate, yes(50)}, {routine::dispatch_a, yes(80)}, {routine::dispatch_b, yes(80)},
                        {routine::notify, 0}, {routine::extra, yes(20)}, {routine::answered, yes(50)}};
        put(1, scratch::deal + 0x54, r.pick<std::uint32_t>({0, 0, 1}));
        put(1, scratch::deal + 0x20, r.pick<std::uint32_t>({0, 1, 1}));
        put(1, scratch::deal + 0x60, r.pick<std::uint32_t>({0, 1, 4, 5, 0x10}));
        put(1, scratch::deal + 0x46, r.pick<std::uint32_t>({0, 1, 2}));
        put(1, scratch::deal + 0x45, r.pick<std::uint32_t>({0, 1}));
        put(1, scratch::deal + 0x44, r.pick<std::uint32_t>({0, 1, 0x7f}));
        put(4, scratch::deal + 0x50, r.next());
    }
    // Real records the people point to (the assessment reads deep into them, so only scalars are written).
    std::vector<std::uint32_t> regions, staff, posts_a, posts_b, clubs, leagues, careers;
    void pools() {
        for (unsigned k = 0; k < 40; ++k) {
            const auto region = sc.r32(sc.club(k) + 0x2c);
            if (region && std::find(regions.begin(), regions.end(), region) == regions.end()) regions.push_back(region);
            if (const auto league = sc.r32(sc.club(k) + 0x30); league && std::find(leagues.begin(), leagues.end(), league) == leagues.end()) leagues.push_back(league);
        }
        for (const auto k : staff_people(sc, 40)) {
            const auto p = sc.person(k);
            staff.push_back(sc.r32(p + 0x1c));
            if (sc.r32(p + 0x14)) posts_a.push_back(sc.r32(p + 0x14));
            if (sc.r32(p + 0x18)) posts_b.push_back(sc.r32(p + 0x18));
        }
        for (unsigned k = 0; k < 14; ++k) clubs.push_back(sc.club(k));
        for (unsigned k = 0; k < 4000 && careers.size() < 10; ++k)
            if (sc.r32(sc.person(k) + 0x14)) careers.push_back(sc.r32(sc.person(k) + 0x14));
    }
    template <class V> std::uint32_t any(const V& pool, unsigned null_percent) {
        return pool.empty() || r.chance(null_percent) ? 0 : pool[r.below(unsigned(pool.size()))];
    }
    // The club of the call (a real club): the fields the walkers read.
    void club_fields(std::uint32_t club) {
        if (staff.empty()) pools();
        put(4, club + 0x2c, any(regions, nulls ? 25 : 0));
        put(2, club + 0x1ec, std::uint16_t(r.pick<std::int32_t>({0x1387, 0x1388, 0x1000, 0x1194, 0x1195, 0x1964, 0x1965, 0x2000, 0x500, 0x5dc, 0x7fff, 0x8000})));
        put(4, club + 0x30, any(leagues, 40));
        put(4, club + 0x94, r.chance(15) ? slot_word : 0);
        put(4, club + 0x160, r.chance(15) ? slot_word : 0);
        put(4, slot_word, r.pick<std::uint32_t>({1, 2, 3, 0xffffffffu}));
        for (const auto region : regions) {
            put(4, region + 0x6c, r.pick<std::uint32_t>({0x64, 0x65, 0x3e8, 0x3e9, 0, 0xffffffffu, 0x1000}));
            put(1, region + 0x117, r.below(2));
            put(1, region + 0x11f, r.pick<std::uint32_t>({5, 9, 10, 0x80, 100}));
            put(1, region + 0x120, r.below(2));
        }
    }
    // Person i of the table, scrambled.
    void person(unsigned i, bool deep) {
        if (staff.empty()) pools();
        const auto p = sc.person(i);
        const auto record = any(staff, deep ? 8 : 30);
        put(4, p + 0x1c, record);
        if (record) {
            put(2, record + 0x28, std::uint16_t(around(r, r.pick<std::int32_t>({0x5dc, 0x186a, 0x600, 0xfa0}))));
            put(2, record + 0x2a, std::uint16_t(around(r, r.pick<std::int32_t>({0x128e, 0xea6, 0x1a5e, 0x4e2}))));
            put(2, record + 0x2c, std::uint16_t(around(r, r.pick<std::int32_t>({0x109a, 0xfa0, 0xfa, 0x1000}))));
        }
        put(4, p + 0xc, any(regions, nulls ? (deep ? 10 : 40) : 0));
        put(1, p + 0x62, r.pick<std::uint32_t>({0, 0, 5, 5, 6, 7, 8, 0xb, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 1}));
        put(1, p + 0x60, r.pick<std::uint32_t>({0, 5, 1}));
        put(4, p + 0x20, any(clubs, 80));
        put(4, p + 0x24, any(clubs, 35));
        put(4, p + 0x14, any(posts_a, 40));
        put(4, p + 0x18, any(posts_b, 70));
        put(1, p + 0x65, r.pick<std::uint32_t>({0, 1, 2, 10, 0x7f, 0x80, 0xff}));
        put(1, p + 0x5e, r.pick<std::uint32_t>({0x4b, 0x4c, 0x10, 0xff}));
        put(1, p + 0x6b, r.pick<std::uint32_t>({0, 1, 0xff}));
        const auto rows = sc.r32(sc.toc(-0x7b9c));
        put(1, rows + i * 100 + 0xc, r.pick<std::uint32_t>({0, 0, 0, 0, 4, 8, 0xc, 0x10, 0x30}));
    }
    // The reputation of the real clubs the people may belong to.
    void owners() {
        if (staff.empty()) pools();
        for (const auto club : clubs)
            put(2, club + 0x1ec, std::uint16_t(r.pick<std::int32_t>({0x1964, 0x1965, 0x1482, 0x1483, 0x1e45, 0x1e46, 0x1000, 0x2400, 0x5000, 0x900, 0x1c52})));
    }
    void people(unsigned count, bool deep) {
        owners();
        for (unsigned i = 0; i < count; ++i) person(i, deep);
    }
};

// A count under 17 makes 0x101aaa90's backward walk circle forever (in the original too: its end test is
// never met), so those counts are for the walker that has no such walk (0x101abda0) only.
unsigned counts[] = {17, 18, 20, 24, 30, 40, 19, 21, 40, 60, 25};
unsigned score_counts[] = {17, 18, 20, 24, 30, 40, 16, 2, 40, 60, 25, 3, 1};

std::vector<std::uint32_t> offered_clubs(const Scenario& sc) {
    std::vector<std::uint32_t> ids;
    const auto list = sc.r32(sc.global(-0x79f0) + 8);
    for (auto node = sc.r32(list + 4); node && ids.size() < 60; node = sc.r32(node + 8)) ids.push_back(sc.r32(sc.r32(node) + 0x10));
    return ids;
}

// Clubs of the world that have a manager whose post (or side post) is the club: the indices.
const std::vector<unsigned>& managed_clubs(const Scenario& sc) {
    static std::vector<unsigned> found;
    if (!found.empty()) return found;
    for (unsigned k = 0; k < 6000 && found.size() < 24; ++k) {
        const auto club = sc.club(k);
        const auto manager = sc.r32(club + 0x90);
        if (manager && (sc.r32(manager + 0x24) == club || sc.r32(manager + 0x20) == club) && sc.r32(manager + 0x28)) found.push_back(k);
    }
    return found;
}
// Club k's real manager gets a status (the byte of the side that is the club's) from a menu.
void manager(Setup& s, unsigned k, bool present) {
    const auto club = s.sc.club(k);
    const auto person = s.sc.r32(club + 0x90);
    s.put(4, club + 0x90, present ? person : 0);
    const auto status = [&] { return s.r.pick<std::uint32_t>({0, 1, 5, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x0a, 0x82}); };
    if (person) {
        s.owners();  // fills the pools
        s.put(4, person + 0x14, s.r.chance(85) ? s.any(s.careers, 0) : 0);  // without it the post routines assert
        s.put(1, person + 0x62, status());
        s.put(1, person + 0x60, status());
    }
    s.owners();
    for (unsigned w = 0x40; w < 0x60; w += 4) s.put(4, scratch::deal + w, s.r.next());
}
void put_club_id(Setup& s, unsigned k, const std::vector<std::uint32_t>& offers) {
    s.put(4, s.sc.club(k), s.r.chance(50) && !offers.empty() ? offers[s.r.below(unsigned(offers.size()))] : 0x7000 + k);
}
Case walker_case(const Scenario& sc, Rng& r, std::uint32_t entry, unsigned flavour, const std::string& label) {
    Case item{.entry = entry, .label = label};
    Setup s(sc, item, r);
    s.nulls = entry == best_score;
    const auto people = entry == best_score ? score_counts[flavour % (sizeof score_counts / sizeof *score_counts)]
                                            : counts[flavour % (sizeof counts / sizeof *counts)];
    s.world(people, 6);
    s.draws(r.chance(8));
    s.answers(flavour);
    s.people(people, flavour % 3 != 0);
    const auto offers = offered_clubs(sc);
    const auto club_index = r.chance(50) && !offers.empty() ? offers[r.below(unsigned(offers.size()))] : r.below(4000);
    const auto club = sc.club(club_index);
    s.club_fields(club);
    const auto leaving = r.chance(55) ? sc.person(r.below(people)) : 0;
    const auto object = s.object();
    switch (entry) {
        case best_score:
            item.registers = {{3, object}, {4, club}, {5, r.chance(40) ? sc.person(r.below(people)) : 0}};
            break;
        case plan:
            item.registers = {{3, object}, {4, club}, {5, leaving}};
            break;
        case approach: {
            const auto person = sc.person(r.chance(50) ? r.below(people) : people - 1 - r.below(std::min(people, 16u)));
            item.registers = {{3, object}, {4, club}, {5, person}};
            // the person's own club in the rows of the world: a small id; the offer list entry
            break;
        }
        default: break;
    }
    return item;
}

// The object-level routines: 0x101acf00 walks the clubs, 0x101aa510 the queue (the world's offer list is borrowed as
// the queue: its first entries' bytes are written), 0x101aa680 ends one manager's post.
Case object_case(const Scenario& sc, Rng& r, std::uint32_t entry, unsigned flavour, const std::string& label) {
    Case item{.entry = entry, .label = label};
    Setup s(sc, item, r);
    const auto people = counts[flavour % (sizeof counts / sizeof *counts)];
    const auto clubs = 1 + flavour % 4;
    s.human = 0;
    if (entry == plan_all) {
        s.human = 30;
        s.world(people, clubs);
    }
    if (entry != end_tenure) {
        if (entry == settle) s.world(people, 0);
        s.draws(r.chance(8));
        s.answers(flavour);
        s.people(people, flavour % 3 != 0);
        if (entry == settle)
            for (auto& answer : item.answers)
                if (answer.first == routine::owner_gate) answer.second = r.chance(10);
        if (entry == settle) item.answers.push_back({routine::deal_hook, 0});
    } else {
        // A real club with a real manager and the world's own count: the post routines run for real. The
        // gate of the news (0x1014c2e0) is answered, since the news and the screens behind it reach OS
        // bridges the checker has none of.
        s.draws(r.chance(8));
        item.answers = {{routine::owner_gate, r.chance(12)}, {routine::deal_hook, 0}};
    }
    const auto object = s.object();
    const auto offers = offered_clubs(sc);
    if (entry != end_tenure)
        for (unsigned k = 0; k < 6; ++k) s.club_fields(sc.club(k));
    switch (entry) {
        case plan_all:
            for (unsigned k = 0; k < clubs; ++k) put_club_id(s, k, offers);
            item.registers = {{3, object}};
            break;
        case settle: {
            const auto list = sc.r32(object + 8);
            s.put(4, object + 4, list);
            const unsigned queued = 1 + flavour % 3;
            s.put(2, list, queued);
            auto node = sc.r32(list + 4);
            for (unsigned n = 0; n < queued && node; ++n, node = sc.r32(node + 8)) {
                const auto data = sc.r32(node);
                s.put(1, data, r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5}));
                s.put(1, data + 1, r.pick<std::uint32_t>({0, 1, 2, 7}));
                const auto k = managed_clubs(sc)[r.below(unsigned(managed_clubs(sc).size()))];
                s.put(4, data + 4, k);
                manager(s, k, r.chance(90));
            }
            item.registers = {{3, object}};
            break;
        }
        default: {  // end_tenure
            const unsigned k = managed_clubs(sc)[r.below(unsigned(managed_clubs(sc).size()))];
            manager(s, k, r.chance(85));
            // The manager is one of the last 16 people (the world's count just past his number) or not.
            if (r.chance(40)) s.put(4, sc.toc(-0x7be4), sc.r32(sc.r32(sc.club(k) + 0x90)) + 1 + r.below(16));
            const auto byte = [&] { return r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 0x82, 0x103, 0xfffffffeu}); };
            item.registers = {{3, object}, {4, sc.club(k)}, {5, byte()}, {6, byte()}, {7, r.pick<std::uint32_t>({0, 1, 0x100, 0xff})}};
            break;
        }
    }
    return item;
}
}

MANAGER_MANAGER_CASES("rematch_walk") {
    if (scenario.match) return;
    Rng r{0xa11ce};
    for (unsigned n = 0; n < 220; ++n) cases.push_back(walker_case(scenario, r, best_score, n, "rematch best score " + std::to_string(n)));
    for (unsigned n = 0; n < 160; ++n) cases.push_back(walker_case(scenario, r, plan, n, "rematch plan " + std::to_string(n)));
    for (unsigned n = 0; n < 200; ++n) cases.push_back(walker_case(scenario, r, approach, n, "rematch approach " + std::to_string(n)));
    for (unsigned n = 0; n < 60; ++n) cases.push_back(object_case(scenario, r, plan_all, n, "rematch plan all " + std::to_string(n)));
    for (unsigned n = 0; n < 60; ++n) cases.push_back(object_case(scenario, r, settle, n, "rematch settle " + std::to_string(n)));
    for (unsigned n = 0; n < 220; ++n) cases.push_back(object_case(scenario, r, end_tenure, n, "rematch end tenure " + std::to_string(n)));
}
}

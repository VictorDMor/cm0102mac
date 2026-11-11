// MANAGER_MANAGER.CPP checker cases for the rematch block's small routines (manager_manager_rematch.hpp):
// 0x101aa650, 0x101b2030, 0x101b39f0 and 0x101b0f90.
//
// People are real (staff with a club and names); the event date cell and today's date take the values a case
// writes, the club's fields the ones its branch needs. 0x101b0f90's many tests are steered by random scenes
// built from cut values (the person's ratings, status, dates and the club's reputation, region and rival are
// written into real and scratch records), with the flag, the deal, the date score and the clubs-extra routine
// answered; the tactics object's rating runs for real and the bar the case passes (f1) decides the last test.
#include "manager_manager_checks_rematch.hpp"
#include <bit>
#include <limits>

namespace cm::aot::manager_manager_checks {
using namespace rematch;
namespace {
constexpr std::uint32_t forward = 0x101aa650, approach_news = 0x101b2030, move_news = 0x101b39f0, willing = 0x101b0f90;
namespace routine {
constexpr std::uint32_t flag = 0x100c7d00, manager_flag = 0x100d3c40, deal = 0x10082f20, days = 0x10718130,
                        extra = 0x102eb610, desk = 0x102cc8e0, desk_club = 0x102cc400;
}

// ---- 0x101b2030 and 0x101b39f0: the event date against today's ----
void dates(const Scenario& sc, std::vector<Case>& cases) {
    const auto people = staff_people(sc, 12);
    if (people.size() < 6) return;
    const auto today = sc.toc(-0x7b80), event = sc.toc(-0x7a9c);
    const auto club_a = sc.club(5), club_b = sc.club(9);
    unsigned n = 0;
    const auto date_case = [&](std::uint32_t entry, std::uint32_t person, std::uint32_t club, std::uint32_t year, std::uint32_t day,
                               std::uint32_t kind, const std::string& what) {
        Case item{.entry = entry, .label = "rematch " + what + " " + std::to_string(n++)};
        Put put{item};
        put(2, today + 2, 2001);
        put(2, today, 100);
        put(2, event + 2, year);
        put(2, event, day);
        if (entry == approach_news) item.registers = {{3, 0}, {4, person}, {5, club}};
        else item.registers = {{3, 0}, {4, person}, {5, kind}, {6, 0x7f}, {7, club}, {8, 2}};
        cases.push_back(std::move(item));
    };
    const std::uint32_t years[] = {2000, 2001, 2002, 0, 0xffff, 0x8000};
    const std::uint32_t day_values[] = {99, 100, 101, 0, 0x7fff, 0x8000, 0x8001, 0xffff, 365, 0x16c};
    for (const auto entry : {approach_news, move_news})
        for (const auto year : years)
            for (const auto day : day_values) date_case(entry, sc.person(people[0]), club_a, year, day, 1, "date");
    // the gates
    date_case(approach_news, 0, club_a, 2000, 1, 1, "no person");
    date_case(approach_news, sc.person(people[0]), 0, 2000, 1, 1, "no club");
    date_case(move_news, 0, club_a, 2000, 1, 1, "no person");
    date_case(move_news, sc.person(people[0]), 0, 2000, 1, 1, "no club");
    {
        Case item{.entry = approach_news, .registers = {{3, 0}, {4, sc.person(people[1])}, {5, club_a}}, .label = "rematch no club of the person"};
        Put put{item};
        put(2, today + 2, 2001);
        put(2, event + 2, 2000);
        put(4, sc.person(people[1]) + 0x24, 0);
        cases.push_back(std::move(item));
    }
    // 0x101b39f0: the kind byte (5 files nothing), the extra and mode bytes, and the club's branches
    const std::uint32_t kinds[] = {0, 1, 2, 3, 4, 5, 0x105, 0xfffffffb, 0x85, 6, 0x7f, 0x80};
    for (const auto kind : kinds) date_case(move_news, sc.person(people[2]), club_a, 2000, 1, kind, "kind");
    const auto count_cell = sc.toc(-0x7be4);
    const auto count = sc.r32(count_cell);
    for (const auto person : {sc.person(people[3]), sc.person(count - 1), sc.person(count - 16), sc.person(count - 17), sc.person(2)})
        for (const auto flag : {0u, 1u})
            for (const auto managed : {0u, 1u})
                for (const auto reputation : {0x1c52u, 0x1c53u, 0x100u})
                    for (const auto region : {0u, 1u})
                        for (const auto staff_at : {0u, 1u}) {
                            Case item{.entry = move_news, .registers = {{3, 0}, {4, person}, {5, 1}, {6, 3}, {7, club_b}, {8, 3}},
                                      .label = "rematch club " + std::to_string(n++)};
                            Put put{item};
                            put(2, today + 2, 2001);
                            put(2, today, 100);
                            put(2, event + 2, 2000);
                            put(2, club_b + 0x1ec, reputation);
                            put(4, club_b + 0x2c, region ? scratch::region_a : 0);
                            put(4, club_b + 0x80, staff_at ? sc.person(people[4]) : 0);
                            item.answers = {{routine::flag, flag}, {routine::manager_flag, managed}, {routine::desk, 0},
                                            {routine::desk_club, 0}};
                            cases.push_back(std::move(item));
                        }
    // the same with the real routines (the desks and the flags)
    for (const auto person : {sc.person(people[5]), sc.person(count - 2)})
        for (unsigned k = 0; k < 6; ++k) {
            Case item{.entry = move_news, .registers = {{3, 0}, {4, person}, {5, 1}, {6, 3}, {7, sc.club(k * 11)}, {8, 2}},
                      .label = "rematch real club " + std::to_string(n++)};
            Put put{item};
            put(2, today + 2, 2001);
            put(2, event + 2, 2000);
            cases.push_back(std::move(item));
        }
    for (unsigned k = 0; k < 4; ++k) {
        Case item{.entry = approach_news, .registers = {{3, 0}, {4, sc.person(people[6 + k])}, {5, sc.club(k + 2)}},
                  .label = "rematch real approach " + std::to_string(n++)};
        Put put{item};
        put(2, today + 2, 2001);
        put(2, event + 2, 2000);
        cases.push_back(std::move(item));
    }
}

// ---- 0x101b0f90: the person's willingness ----
struct Scene {
    std::uint32_t region_mode = 0, home_mode = 0;  // club.region / person.home: 0 none, 1 region_a, 2 region_b, 3 equal to the club's
    std::int16_t staff_a = 0x600, staff_b = 0x1000, staff_c = 0x1000, reputation = 0x1000, owner_reputation = 0x1000;
    std::uint32_t status = 0, side_status = 0, side_club = 0, owner_mode = 0;  // owner_mode: 0 none, 1 scratch owner, 2 the club itself
    std::uint32_t post_a_mode = 0, post_b_mode = 0;  // 0 none, 1 present
    std::int16_t post_a_rating = 0x1000;
    std::uint32_t post_b_hit = 0;  // which of the post's three words holds the club (0 none, 1..3)
    std::uint32_t level = 10, tenure = 0x40, outlook = 0x20;
    std::uint32_t region_byte_a = 5, region_byte_b = 5, region_117_a = 1, region_117_b = 1;
    std::uint32_t rival_mode = 0;  // 0 no rival, 1 rivals differ, 2 the same
    std::int16_t rival_a = 5, rival_b = 5;
    std::uint32_t flag = 0, deal_present = 0, extra = 0;
    std::uint32_t days = 0x100;
    std::uint32_t deal_kind = 0, deal_46 = 0, deal_45 = 1, deal_44 = 0;
    double bar = 0.0;
};

Case willing_case(const Scenario& sc, unsigned person_k, unsigned club_k, const Scene& s, const std::string& label) {
    const auto person = sc.person(person_k), club = sc.club(club_k);
    Case item{.entry = willing, .registers = {{3, 0}, {4, person}, {5, club}}, .f1 = s.bar, .label = label};
    Put put{item};
    const auto staff = sc.r32(person + 0x1c);
    put(2, staff + 0x28, std::uint16_t(s.staff_a));
    put(2, staff + 0x2a, std::uint16_t(s.staff_b));
    put(2, staff + 0x2c, std::uint16_t(s.staff_c));
    const auto club_region = s.region_mode == 0 ? 0 : s.region_mode == 2 ? scratch::region_b : scratch::region_a;
    const auto home = s.home_mode == 0 ? 0 : s.home_mode == 3 ? club_region : s.home_mode == 2 ? scratch::region_b : scratch::region_a;
    put(4, club + 0x2c, club_region);
    put(4, person + 0xc, home);
    put(2, club + 0x1ec, std::uint16_t(s.reputation));
    put(1, scratch::region_a + 0x11f, s.region_byte_a);
    put(1, scratch::region_b + 0x11f, s.region_byte_b);
    put(1, scratch::region_a + 0x117, s.region_117_a);
    put(1, scratch::region_b + 0x117, s.region_117_b);
    put(1, person + 0x62, s.status);
    put(1, person + 0x60, s.side_status);
    put(4, person + 0x20, s.side_club);
    const auto owner = s.owner_mode == 0 ? 0 : s.owner_mode == 2 ? club : scratch::owner;
    put(4, person + 0x24, owner);
    put(2, scratch::owner + 0x1ec, std::uint16_t(s.owner_reputation));
    put(4, scratch::owner + 0x2c, scratch::owner_region);
    put(1, scratch::owner_region + 0x11f, s.region_byte_b);
    put(4, person + 0x14, s.post_a_mode ? scratch::post_a : 0);
    put(2, scratch::post_a + 0xa, std::uint16_t(s.post_a_rating));
    put(4, person + 0x18, s.post_b_mode ? scratch::post_b : 0);
    put(4, scratch::post_b + 0x10, s.post_b_hit == 1 ? club : 0);
    put(4, scratch::post_b + 0x14, s.post_b_hit == 2 ? club : 0);
    put(4, scratch::post_b + 0x18, s.post_b_hit == 3 ? club : 0);
    put(1, person + 0x65, s.level);
    put(1, person + 0x5e, s.tenure);
    put(2, person + 0x5c, s.outlook);
    // the club's and the person's club's rivals
    put(4, club + 0x30, s.rival_mode ? scratch::rival_a : 0);
    put(4, owner ? owner + 0x30 : scratch::spare, s.rival_mode ? (s.rival_mode == 2 ? scratch::rival_a : scratch::rival_b) : 0);
    put(2, scratch::rival_a + 0x18, std::uint16_t(s.rival_a));
    put(2, scratch::rival_b + 0x18, std::uint16_t(s.rival_b));
    // the deal and the date scores
    put(1, scratch::deal + 0x60, s.deal_kind);
    put(1, scratch::deal + 0x46, s.deal_46);
    put(1, scratch::deal + 0x45, s.deal_45);
    put(1, scratch::deal + 0x44, s.deal_44);
    put(4, scratch::deal + 0x28, 0);
    put(4, scratch::deal + 0x2c, 0);
    item.answers = {{routine::flag, s.flag}, {routine::deal, s.deal_present ? scratch::deal : 0}, {routine::days, s.days},
                    {routine::extra, s.extra}};
    return item;
}

void willing_cases(const Scenario& sc, std::vector<Case>& cases) {
    const auto people = staff_people(sc, 12);
    if (people.size() < 12) return;
    Rng r{0x5eed1};
    const auto scene = [&](bool deep) {
        Scene s;
        s.region_mode = r.below(4);
        s.home_mode = r.below(4);
        s.staff_a = around(r, r.pick<std::int32_t>({0x5dc, 0x186a, 0x600}));
        s.staff_b = around(r, r.pick<std::int32_t>({0x128e, 0xea6, 0x1a5e, 0x1000}));
        s.staff_c = around(r, r.pick<std::int32_t>({0x109a, 0xfa0, 0x1000}));
        s.reputation = around(r, r.pick<std::int32_t>({0x1676, 0x1a5e, 0x1c52, 0x8ca, 0xcb2, 0x9c4, 0x1e46, 0x1000, 0x2000, 0x203a}));
        s.owner_reputation = around(r, r.pick<std::int32_t>({0x1482, 0x1c52, 0x1e46, 0x1000, 0x203a, 0x2400}));
        s.status = r.pick<std::uint32_t>({0, 5, 5, 0xb, 0xb, 0xb, 0xc, 0xf});
        s.side_status = r.pick<std::uint32_t>({0, 5, 5, 1});
        s.side_club = r.chance(25) ? sc.club(3) : 0;
        s.owner_mode = r.below(3);
        s.post_a_mode = r.below(2);
        s.post_b_mode = r.chance(40);
        s.post_a_rating = around(r, 0x128e);
        s.post_b_hit = r.below(4);
        s.level = r.pick<std::uint32_t>({0, 1, 10, 0x7f, 0x80, 0xff, 3});
        s.tenure = r.pick<std::uint32_t>({0x4b, 0x4c, 0x40, 0xff});
        s.outlook = r.pick<std::uint32_t>({0x22, 0x23, 0x24, 0x20, 0x80, 0x180});
        s.region_byte_a = r.pick<std::uint32_t>({5, 9, 10, 0x80, 0xff, 100, 0x7f});
        s.region_byte_b = r.pick<std::uint32_t>({5, 9, 10, 0x80, 100, 0x7f});
        s.region_117_a = r.below(2);
        s.region_117_b = r.below(2);
        s.rival_mode = r.below(3);
        s.rival_a = around(r, 5);
        s.rival_b = around(r, 5);
        s.flag = r.below(2);
        s.deal_present = r.chance(70);
        s.extra = r.chance(25);
        s.days = r.pick<std::uint32_t>({0xb3, 0xb4, 0xb5, 0x100, 0, 0xd1, 0xd2, 0xd3, 0x1c0, 0x7fffffff, 0x80000000u});
        s.deal_kind = r.pick<std::uint32_t>({0, 1, 3, 4, 0x15, 0xf0});
        s.deal_46 = r.pick<std::uint32_t>({0, 1, 2, 0x80});
        s.deal_45 = r.pick<std::uint32_t>({0, 1, 0x80});
        s.deal_44 = r.pick<std::uint32_t>({0, 1, 0x7f, 0x80});
        s.bar = r.pick<double>({0.0, -1e300, 1e300, std::numeric_limits<double>::quiet_NaN(), -1.0, 1.0});
        if (deep && r.chance(70)) {  // make the early exits less likely
            s.region_mode = 3 * (s.home_mode != 0);
            s.home_mode = s.region_mode;
            s.status = r.pick<std::uint32_t>({0, 5, 0xb});
        }
        return s;
    };
    for (unsigned n = 0; n < 1400; ++n) {
        auto s = scene(n % 2);
        if (n % 5 == 0) {  // the reputation fold past 0x203a for the club and its owner
            s.reputation = std::int16_t(0x1e47 + r.below(0x2000));
            s.owner_mode = 1;
            s.owner_reputation = std::int16_t(0x1000 + r.below(0x2000));
            s.post_b_mode = 0;
            s.side_club = 0;
            s.flag = 0;
            s.status = 0;
            s.tenure = 0x40;
            s.staff_b = 0x1000;
            s.staff_a = 0x600;
            s.extra = 0;
            s.post_a_mode = 1;
        }
        cases.push_back(willing_case(sc, people[n % people.size()], 3 + n % 40, s, "rematch willing " + std::to_string(n)));
    }
    // the real routines (no answers): the tactics object decides with the bar
    for (unsigned n = 0; n < 40; ++n) {
        auto s = scene(true);
        auto item = willing_case(sc, people[n % people.size()], 7 + n, s, "rematch willing real " + std::to_string(n));
        item.answers.clear();
        cases.push_back(std::move(item));
    }
}

// The ends of 0x101b0f90 the random scenes seldom reach: no staff record or no club, the deal's last two tests, a staff
// record that vanishes while the flag is asked, a reputation that changes while the deal is looked up, and a person
// with a second club that passes every earlier test.
void willing_edge_cases(const Scenario& sc, std::vector<Case>& cases) {
    const auto people = staff_people(sc, 12);
    if (people.size() < 12) return;
    unsigned n = 0;
    const auto name = [&](const char* what) { return std::string("rematch willing edge ") + what + " " + std::to_string(n++); };
    {
        auto item = willing_case(sc, people[0], 5, Scene{}, name("no staff"));
        item.memory.push_back({4, sc.person(people[0]) + 0x1c, 0});
        cases.push_back(std::move(item));
        item = willing_case(sc, people[1], 6, Scene{}, name("no club"));
        item.registers[2] = {5, 0};
        cases.push_back(std::move(item));
    }
    // the deal of a person whose post is rated high at a club of a lower reputation (status 0xb)
    for (const auto home_mode : {0u, 1u, 2u, 3u})
        for (const auto owner_mode : {0u, 1u, 2u})
            for (const auto kind : {0u, 4u})
                for (const auto poked : {false, true}) {
                    Scene s;
                    s.status = 0xb;
                    s.region_mode = 1;
                    s.home_mode = home_mode;
                    s.owner_mode = owner_mode;
                    s.post_a_mode = 1;
                    s.post_a_rating = 0x1300;
                    s.reputation = 0x1000;
                    s.deal_present = 1;
                    s.deal_kind = kind;
                    s.outlook = 0x40;
                    s.bar = kind ? 1.0 : -1.0;
                    const auto club_k = 7 + n % 5, person_k = people[2 + n % 8];
                    auto item = willing_case(sc, person_k, club_k, s, name("deal"));
                    // the reputation read again after the deal routine answers (0x101b10dc): it rose past 0x1c52
                    if (poked) item.pokes.push_back({routine::deal, {{2, sc.club(club_k) + 0x1ec, 0x1d00}}});
                    cases.push_back(std::move(item));
                }
    // the staff record is gone when the club's flag has been asked (0x101b105c)
    for (const auto reputation : {0x500, 0x8c9}) {
        Scene s;
        s.reputation = std::int16_t(reputation);
        s.region_mode = 1;
        s.home_mode = 3;
        auto item = willing_case(sc, people[4], 9, s, name("staff vanishes"));
        item.pokes.push_back({routine::flag, {{4, sc.person(people[4]) + 0x1c, 0}}});
        cases.push_back(std::move(item));
    }
    // a second club that is not a loan (status 5): a small club at the person's own region
    for (const auto post_a : {0u, 1u})
        for (const auto bar : {-1.0, 1.0}) {
            Scene s;
            s.reputation = 0x900;
            s.region_mode = 1;
            s.home_mode = 3;
            s.staff_a = 0x500;
            s.staff_b = 0x1000;
            s.side_club = sc.club(3);
            s.side_status = 0;
            s.status = 0;
            s.post_a_mode = post_a;
            s.bar = bar;
            cases.push_back(willing_case(sc, people[5], 10, s, name("second club")));
        }
}
}

MANAGER_MANAGER_CASES("rematch") {
    if (scenario.match) return;
    for (const auto person : {scenario.person(1), scenario.person(0), std::uint32_t(0)}) {
        Case item{.entry = forward, .registers = {{3, 0}, {4, person}}, .label = "rematch forward"};
        cases.push_back(std::move(item));
    }
    dates(scenario, cases);
    willing_cases(scenario, cases);
    willing_edge_cases(scenario, cases);
}
}

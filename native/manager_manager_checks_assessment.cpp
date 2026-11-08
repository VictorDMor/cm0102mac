// MANAGER_MANAGER.CPP checker cases for the assessment block (manager_manager_assessment.hpp):
// 0x101ad080, a staff member's score for a job at a club. Arguments are real records of the
// career: people with a staff record (+0x1c), their own club or another one, the second person
// the callers pass (the club's people at +0x80 and +0x90, or none), and every job code the
// two switches distinguish plus values outside them. The captured days add the real calls.
#include "manager_manager_checks.hpp"
#include <cstdio>
#include <random>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t entry = 0x101ad080;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50;
std::string hex(std::uint32_t v) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", v);
    return text;
}
}

MANAGER_MANAGER_CASES("assessment") {
    if (scenario.match) return;
    const auto people = scenario.people(), clubs = scenario.global(club_total);
    if (!people || !clubs) return;
    const auto object = scenario.global(manager_object);
    // People with a staff record, spread over the table (the callers only pass those, but the
    // routine answers -10000 for one without: one such case below).
    std::vector<std::uint32_t> staff, plain;
    const unsigned stride = people > 4000 ? people / 4000 : 1;
    for (unsigned n = 0; n < people && staff.size() < 28; n += stride) {
        const auto person = scenario.person(n);
        (scenario.r32(person + 0x1c) ? staff : plain).push_back(person);
    }
    for (std::size_t k = 0; k < staff.size(); ++k) {
        const auto person = staff[k];
        const auto own = scenario.r32(person + 0x24);
        const auto other = scenario.club(unsigned(k * 37 % clubs));
        const auto club = own && k % 3 != 2 ? own : other;
        // The second person: none (the routine falls back to club+0x80), the club's +0x90
        // (most callers), or a person without a staff record (rejected the same way).
        const std::uint32_t seconds[] = {0, scenario.r32(club + 0x90), plain.empty() ? 0 : plain[k % plain.size()]};
        for (std::int32_t job = -1; job <= 17; ++job) {
            const auto second = seconds[std::size_t(job + 1) % 3];
            cases.push_back({.entry = entry,
                             .registers = {{3, object}, {4, person}, {5, club}, {6, second}, {7, std::uint32_t(job) & 0xff}},
                             .label = "person " + hex(person) + " club " + hex(club) + " second " + hex(second) +
                                      " job " + std::to_string(job)});
        }
        // The job byte is sign-extended: a high byte is a negative job.
        if (k < 4)
            cases.push_back({.entry = entry, .registers = {{3, object}, {4, person}, {5, club}, {6, 0}, {7, 0x185}},
                             .label = "job 0x185 (byte 0x85) " + hex(person)});
    }
    if (!plain.empty())
        cases.push_back({.entry = entry, .registers = {{3, object}, {4, plain[0]}, {5, scenario.club(0)}, {6, 0}, {7, 5}},
                         .label = "no staff record"});
}

// The branches the career's records leave unreached: a club the flag callee (0x100c7d00)
// reports, the region tests (0x100cab10), the pair callees (0x100d9480...), and the thresholds
// on the club's reputation, the person's role, rating, club links and staff levels, which a
// real career holds at a few values. The callees are answered on both sides (no body run);
// the fields are written on both sides and undone afterwards. A fixed seed picks the combinations.
MANAGER_MANAGER_CASES("assessment thresholds") {
    if (scenario.match) return;
    const auto people = scenario.people(), clubs = scenario.global(club_total);
    if (!people || !clubs) return;
    const auto object = scenario.global(manager_object);
    std::vector<std::uint32_t> staff;
    const unsigned stride = people > 4000 ? people / 4000 : 1;
    for (unsigned n = 0; n < people && staff.size() < 40; n += stride)
        if (const auto person = scenario.person(n); scenario.r32(person + 0x1c)) staff.push_back(person);
    if (staff.empty()) return;
    // Clubs with a region record: the links the callees follow stay valid.
    std::vector<std::uint32_t> good_clubs;
    for (unsigned n = 0; n < clubs; ++n)
        if (scenario.r32(scenario.club(n) + 0x2c)) good_clubs.push_back(scenario.club(n));
    if (good_clubs.empty()) return;
    std::mt19937 random(0x1ad080);
    const auto any_club = [&] { return good_clubs[random() % good_clubs.size()]; };
    const auto pick = [&](auto&&... values) {
        const std::uint32_t list[] = {std::uint32_t(values)...};
        return list[random() % (sizeof list / sizeof list[0])];
    };
    constexpr std::uint32_t flag = 0x100c7d00, region_test = 0x100cab10, pairs[] = {0x100d9480, 0x100d94d0, 0x100d9520, 0x100d9580};
    const std::int32_t jobs[] = {5, 12, 6, 7, 8, 9, 10, 11, 13, 14, 15, 5, 12, 5, 12};
    for (unsigned n = 0; n < 3000; ++n) {
        const auto person = staff[random() % staff.size()];
        const auto own = scenario.r32(person + 0x24);
        const auto club = (own && random() % 4) ? own : any_club();
        const auto candidate = staff[random() % staff.size()];
        const std::uint32_t second = random() % 3 == 0 ? 0 : random() % 2 ? scenario.r32(club + 0x90) : candidate;
        const auto job = jobs[random() % std::size(jobs)];
        Case item{.entry = entry,
                  .registers = {{3, object}, {4, person}, {5, club}, {6, second}, {7, std::uint32_t(job) & 0xff}}};
        const auto write = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        const auto staff_record = scenario.r32(person + 0x1c);
        // The club's reputation, the person's role, rating and links, the staff levels.
        write(2, club + 0x1ec, pick(0x0f00, 0x1300, 0x1483, 0x1500, 0x1677, 0x1700, 0x186b, 0x1900, 0x1a5f, 0x1b59, 0x1c53, 0x1d00, 0x203b, 0x2100, 0x128f, 0xcb3));
        write(1, person + 0x62, pick(0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 3, 20, 0x85));
        write(1, person + 0x60, pick(0, 5, 9, 12, 17));
        write(2, person + 0x5c, pick(20, 36, 37, 44, 45, 58, 59, 60, 70));
        write(2, staff_record + 0x2c, pick(0x800, 0xcb3, 0x1000, 0x109a, 0x10aa, 0x1500, 0x1c00));
        write(2, staff_record + 0x24, pick(10, 40, 60, 100));
        write(2, staff_record + 0x26, pick(5, 50, 120, 200));
        if (random() % 4 == 0) write(4, person + 0x24, pick(0, club, any_club()));
        if (random() % 4 == 0) write(4, person + 0x20, pick(0, club, any_club()));
        if (random() % 5 == 0) write(4, person + 0xc, pick(scenario.r32(club + 0x2c), scenario.r32(any_club() + 0x2c)));
        if (random() % 4 == 0) write(4, person + 0x0, pick(0x10, 0x2000, 0x7000, scenario.people() - 0x20, scenario.people() - 1, scenario.people() - 8, scenario.people() - 0x10));
        // The regions: the club's own record's tier and category.
        if (const auto region = scenario.r32(club + 0x2c)) {
            write(1, region + 0x11f, pick(0, 5, 9, 11, 12, 0x90));
            write(1, region + 0x11c, pick(0, 1, 2, 3, 4, 5));
        }
        if (random() % 5 == 0) write(4, club + 0x2c, pick(0, scenario.r32(any_club() + 0x2c)));
        if (random() % 5 == 0) write(4, club + 0x30, pick(0, scenario.r32(any_club() + 0x30)));
        // Answers for the callees: the flag, the region test and the pair callees.
        item.answers.push_back({flag, std::uint32_t(random() % 3 == 0)});
        item.answers.push_back({region_test, std::uint32_t(random() % 2)});
        // The age gap callee (0x100cdef0) answered with a value on either side of the thresholds, and
        // the calendar's date (0x107156f0) with a record built in scratch memory around the
        // game date (the halfwords the rule compares).
        if (random() % 2) item.answers.push_back({0x100cdef0, pick(0x1000, 0x1099, 0x109a, 0x1100, 0x1a5d, 0x1a5e, 0x1b00, 0x1c51, 0x1c52, 0x2000, 0xf000)});
        if (random() % 3 == 0) {
            const auto game_date = scenario.r32(scenario.toc(-0x7b80));
            const std::uint32_t high = (game_date >> 16) + pick(0, 1, std::uint32_t(-1)), low = (game_date & 0xffff) + pick(0, 1, std::uint32_t(-1), 0, 0);
            write(4, scratch_base, (high & 0xffff) << 16 | (low & 0xffff));
            write(4, scratch_base + 4, 0x12345678);
            item.answers.push_back({0x107156f0, scratch_base});
        }
        // The statistic callee (0x103092f0): its byte, answered in half of the cases.
        if (random() % 2) item.answers.push_back({0x103092f0, pick(0, 1, 2, 3, 0xfe, 9, 10, 0xff)});
        for (const auto entry_point : pairs) item.answers.push_back({entry_point, std::uint32_t(random() % 3 == 0)});
        item.label = "threshold case " + std::to_string(n) + " job " + std::to_string(job) + " person " + hex(person) + " club " + hex(club);
        cases.push_back(std::move(item));
    }
}

// Targeted combinations for the branches the random block leaves rare: a person without a
// club reference or club, a staff level on either side of 4250 at a club over 8250 whose
// region tier is above 10, the person's club sharing the club's region, and a club without a
// second person. Every callee is answered (the statistic callee reads the person's club and
// reference), so each case isolates one rule.
MANAGER_MANAGER_CASES("assessment targeted") {
    if (scenario.match) return;
    const auto people = scenario.people(), clubs = scenario.global(club_total);
    if (!people || !clubs) return;
    const auto object = scenario.global(manager_object);
    std::vector<std::uint32_t> staff, good_clubs;
    const unsigned stride = people > 4000 ? people / 4000 : 1;
    for (unsigned n = 0; n < people && staff.size() < 6; n += stride)
        if (const auto person = scenario.person(n); scenario.r32(person + 0x1c)) staff.push_back(person);
    for (unsigned n = 0; n < clubs && good_clubs.size() < 6; ++n)
        if (scenario.r32(scenario.club(n) + 0x2c) && scenario.r32(scenario.club(n) + 0x30)) good_clubs.push_back(scenario.club(n));
    if (staff.empty() || good_clubs.size() < 2) return;
    std::uint32_t plain = 0;  // a person without a staff record
    for (unsigned n = 0; n < people && !plain; n += stride)
        if (!scenario.r32(scenario.person(n) + 0x1c)) plain = scenario.person(n);
    // extra: 1 the person's club has no league, 2 no region, 4 the club's second person has no staff
    // record, 8 a large age gap and every pair callee true.
    struct Setup { std::uint32_t reputation, level, reference_null, own_link, region_test, flag, tier, second_null, previous_link, extra; };
    // own_link: 0 none, 1 the club itself, 2 another club.
    const Setup setups[] = {
        {0x1500, 0x800, 1, 2, 1, 0, 5, 0, 0, 0},    // reference null
        {0x1500, 0x800, 0, 0, 1, 0, 5, 0, 0, 0},    // no club of the person, tests pass
        {0x1500, 0x800, 0, 0, 1, 1, 5, 0, 0, 0},
        {0x2100, 0x1500, 0, 2, 0, 0, 12, 0, 0, 0},  // level at or over 4250 over 8250
        {0x2100, 0x800, 0, 2, 1, 0, 12, 0, 0, 0},   // region test true
        {0x2100, 0x800, 0, 0, 0, 0, 12, 0, 0, 0},   // no club
        {0x2100, 0x800, 0, 2, 0, 0, 12, 0, 0, 0},
        {0x2100, 0x800, 0, 1, 0, 0, 12, 0, 0, 0},
        {0x1500, 0x800, 0, 1, 1, 0, 5, 0, 0, 0},    // the club itself: shared region
        {0x1500, 0x800, 0, 0, 1, 0, 5, 0, 1, 0},    // previous club: shared region
        {0x1500, 0x800, 0, 2, 0, 0, 5, 1, 0, 0},    // no second person at the club
        {0x1500, 0x800, 0, 2, 0, 1, 5, 1, 0, 0},
        {0x2100, 0x800, 0, 2, 0, 0, 12, 0, 0, 1},   // the person's club has no league
        {0x1500, 0x800, 0, 2, 1, 0, 5, 0, 0, 2},    // the person's club has no region
        {0x1500, 0x800, 0, 2, 0, 0, 5, 0, 0, 4},    // the club's second person has no staff record
        {0x1500, 0x800, 0, 2, 0, 0, 5, 0, 0, 8},    // a floor: the scaled score beats the additive one
        {0x1500, 0x800, 0, 2, 0, 1, 5, 0, 0, 8}
    };
    for (const auto& setup : setups)
        for (const auto job : {5, 12, 6, 8, 9, 10})
            for (std::size_t k = 0; k < std::min<std::size_t>(staff.size(), 3); ++k) {
                const auto person = staff[k];
                const auto club = good_clubs[k % good_clubs.size()];
                const auto other = good_clubs[(k + 1) % good_clubs.size()];
                const auto region = scenario.r32(club + 0x2c);
                Case item{.entry = entry,
                          .registers = {{3, object}, {4, person}, {5, club}, {6, 0}, {7, std::uint32_t(job)}, {26, region}}};
                const auto write = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
                write(2, club + 0x1ec, setup.reputation);
                write(2, scenario.r32(person + 0x1c) + 0x2c, setup.level);
                write(4, person + 0xc, setup.reference_null ? 0 : region);
                write(4, person + 0x24, setup.own_link == 1 ? club : setup.own_link == 2 ? other : 0);
                write(4, person + 0x20, setup.previous_link ? club : 0);
                write(1, region + 0x11f, setup.tier);
                if (setup.second_null) write(4, club + 0x80, 0);
                if (setup.extra == 1) write(4, other + 0x30, 0);
                if (setup.extra == 2) write(4, other + 0x2c, 0);
                if (setup.extra == 4 && plain) write(4, club + 0x80, plain);
                const std::uint32_t big = setup.extra == 8;
                item.answers = {{0x100c7d00, setup.flag}, {0x100cab10, setup.region_test}, {0x103092f0, 3},
                                {0x100cdef0, big ? 0x7fff : 0x1100}, {0x100d9480, big}, {0x100d94d0, 0}, {0x100d9520, big}, {0x100d9580, 0}};
                item.label = "targeted job " + std::to_string(job) + " person " + hex(person) + " club " + hex(club) +
                             " setup " + std::to_string(&setup - setups);
                cases.push_back(std::move(item));
            }
}
}

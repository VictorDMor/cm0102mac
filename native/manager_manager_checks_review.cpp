// MANAGER_MANAGER.CPP checker cases for the manager review block (manager_manager_review.hpp):
// 0x101a8ad0, the board's review of a club's manager; 0x101c1690, the pending-departure check it
// makes first when its flag is set; 0x10195e10, the module's draw from the halfword table.
// Arguments are the career's real records: the module object (TOC -0x79f0), clubs spread over the
// table, both flag values, and draw bounds on each side of the routine's range tests. The
// captured days add the real calls.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t review = 0x101a8ad0, departure = 0x101c1690, draw = 0x10195e10;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50;
// The draw table's cursor (a pointer to the next halfword) and its last entry.
constexpr std::int32_t draw_cursor = -0x7b84, draw_end = -0x7b88;
std::string hex(std::uint32_t v) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", v);
    return text;
}
}

MANAGER_MANAGER_CASES("review") {
    if (scenario.match) return;
    const auto clubs = scenario.global(club_total);
    if (!clubs) return;
    const auto object = scenario.global(manager_object);
    // Clubs spread over the table, with and without the departure check (callers pass 0 or 1).
    const unsigned stride = clubs > 240 ? clubs / 240 : 1;
    for (unsigned n = 0; n < clubs; n += stride) {
        const auto club = scenario.club(n);
        for (std::uint32_t flag : {0u, 1u})
            cases.push_back({.entry = review, .registers = {{3, object}, {4, club}, {5, flag}},
                             .label = "club " + std::to_string(n) + " flag " + std::to_string(flag)});
        if (n % (stride * 4) == 0)
            cases.push_back({.entry = departure, .registers = {{3, object}, {4, club}},
                             .label = "departure club " + std::to_string(n)});
    }
    // The flag is a byte: a high byte with a clear low byte means no check.
    cases.push_back({.entry = review, .registers = {{3, object}, {4, scenario.club(0)}, {5, 0x100}},
                     .label = "flag 0x100 (byte 0)"});

    // The draw: 0 answers 0; bounds up to 0xffff and down to -0xffff take the modulo path, the
    // rest the floating-point one. Each also from the table's last entry, where the cursor wraps
    // to the start and the base is redrawn (0x1077b770).
    const std::int32_t bounds[] = {0, 1, 2, 3, 5, 10, 100, 1000, 0x7fff, 0x8000, 0xfffe, 0xffff, 0x10000, 0x10001,
                                   0x12345, 0x7fffffff, -1, -2, -10, -1000, -0xfffe, -0xffff, -0x10000, -0x10001,
                                   -0x12345, std::int32_t(0x80000001u), std::int32_t(0x80000000u)};
    const auto cursor_cell = scenario.toc(draw_cursor), end = scenario.global(draw_end);
    for (const auto bound : bounds) {
        cases.push_back({.entry = draw, .registers = {{3, std::uint32_t(bound)}}, .label = "draw " + std::to_string(bound)});
        cases.push_back({.entry = draw, .registers = {{3, std::uint32_t(bound)}}, .memory = {{4, cursor_cell, end}},
                         .label = "draw " + std::to_string(bound) + " at the table's end"});
    }
    cases.push_back({.entry = draw, .registers = {{3, 100}}, .memory = {{4, cursor_cell, end - 2}},
                     .label = "draw at the entry before the end " + hex(end)});
}

// Targeted cases for what the career's clubs rarely reach: random combinations (a fixed seed) of
// the values the rules read, written over real clubs and undone afterwards. The other modules'
// routines (days between dates, the results record, the club flags, the list append) answer the
// same on both sides; the game's table draws read halfwords built in the scratch area.
namespace {
struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};
constexpr std::int32_t date_slot = -0x7b80, stamp_slot = -0x7a9c, leagues_slot = -0x7b5c, base_slot = -0x7b90,
                       cursor_slot = -0x7b84, last_slot = -0x7b88, owner_slot = -0x79f0;
constexpr std::uint32_t table_area = scratch_base + 0x9000, record_area = scratch_base + 0x8000,
                        list_area = scratch_base + 0xa000, node_area = scratch_base + 0xa100, element_area = scratch_base + 0xa400;
constexpr std::uint32_t calendar = 0x10718130, results = 0x10459b50, manager_flag = 0x102de3c0, club_flag = 0x100c7d00,
                        club_state = 0x1014c2e0, append = 0x10738d10, glue = 0x10781320, redraw = 0x1077b770, remove = 0x10738e60;

// How a batch leans: percentages that steer the random values toward rules a uniform draw rarely
// reaches (a staff record missing, an idle form, a table of multiples of 3, the real calendar).
struct Bias {
    unsigned staff_null = 8, second_null = 10, trait_zero = 0, league_null = 5, idle_form = 0, multiples_of_three = 0,
             real_calendar = 0, signals = 0, low_value = 0, flag_one = 55, attributes = 0, empty_people = 0, manager_null = 0;
};
// One randomized call of review (or of departure) over a real club.
Case randomized(Rng& rng, const Scenario& sc, bool departure_only, const Bias& bias = {}) {
    const auto clubs = sc.global(-0x7b50);
    unsigned n = rng.below(clubs);
    // Prefer clubs with a manager and a second person: the rules reach further with them.
    for (unsigned tries = 0; tries < 40 && !(sc.r32(sc.club(n) + 0x90) && sc.r32(sc.club(n) + 0x80)); ++tries) n = rng.below(clubs);
    const auto club = sc.club(n), object = sc.global(owner_slot);
    Case item{.entry = departure_only ? departure : review, .registers = {{3, object}, {4, club}}};
    if (!departure_only) item.registers.push_back({5, rng.chance(bias.flag_one) ? 1u : 0u});
    item.label = std::string(departure_only ? "departure" : "review") + " random club " + std::to_string(n);
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    auto byte_at = [&](std::uint32_t at) { return std::uint32_t(sc.s.memory.read<1>(at)); };
    // The game date and the stamp: mostly the stamp is not after the date.
    const auto date = sc.toc(date_slot), stamp = sc.toc(stamp_slot);
    const auto day = sc.r32(date) >> 16, year = sc.r32(date) & 0xffff;
    const auto relation = rng.below(100);
    if (relation < 12) { put(2, stamp + 2, year + 1); put(2, stamp, day); }
    else if (relation < 30) { put(2, stamp + 2, year - 1); put(2, stamp, rng.pick<std::uint32_t>({0, 0xffff, 0x7fff, 0x8000})); }
    else if (relation < 85) { put(2, stamp + 2, year); put(2, stamp, rng.pick<std::uint32_t>({day, day + 1, day - 1, 0xffff, 0x8000, 0x7fff, 0})); }
    else { put(2, stamp + 2, year); put(2, stamp, day); put(2, date, rng.pick<std::uint32_t>({0, 0xffff, 0x8000, 0x7fff})); }
    // The row.
    const auto row = sc.r32(object) + sc.r32(club) * 0x4c;
    if (rng.chance(10)) put(1, row + 2, byte_at(row + 2) | 0x40);
    else if (byte_at(row + 2) & 0x40) put(1, row + 2, byte_at(row + 2) & ~0x40u);
    if (bias.signals)
        put(2, row + 6, bias.signals == 1 ? 0x800 : 0xc00);  // thresholds 500 and 200
    else if (bias.low_value)
        put(2, row + 6, rng.pick<std::uint32_t>({0, 1, 2, 0xffff}));  // scaled below 1 once the row's value is cut
    else
        put(2, row + 6, rng.chance(8) ? 0x10b4 : rng.pick<std::uint32_t>({0x1000, 0xe00, 0xdad, 0xdac, 0xbb9, 0xbb8, 0x800, 0x100, 0, 0xfff0, 0x10b3, 1, 2}));
    // The club: its manager, the people and the league.
    put(2, club + 0x1ec, rng.pick<std::uint32_t>({0x100, 0x1000, 0x2000, 0x2ee, 0x7fff, 0x2000 + 0x2ee, 0x1389 + 0x2ee, 0x9000}));
    const auto manager = sc.r32(club + 0x90);
    // The callers' clubs always have a manager (tenure() reads through it first, a fault in this
    // port). `manager_null` clears it anyway for --null-page runs, where page 0 reads as on the
    // Mac and the review's later null retests (which a callee clearing the manager would reach)
    // are compared.
    if ((departure_only && rng.chance(10)) || (bias.manager_null && rng.chance(bias.manager_null))) put(4, club + 0x90, 0);
    else if (manager) {
        if (rng.chance(bias.staff_null)) put(4, manager + 0x1c, 0);
        else if (const auto staff = sc.r32(manager + 0x1c)) {
            put(2, staff + 0x2a, rng.pick<std::uint32_t>({0x1388, 0x1389, 0x2000, 0x7fff, 0, 0xff9c, 0x1fff, 0x600, 0x5dd}));
            // The staff attributes (signed bytes at +0x2e...+0x42) the job assessment weighs.
            if (bias.attributes)
                for (std::uint32_t at = 0x2e; at <= 0x42; ++at) put(1, staff + at, bias.attributes == 1 ? 20 : rng.below(4));
        }
        put(1, manager + 0x66, rng.pick<std::uint32_t>({0, 1, 0xff, 5, 0x7f, 0x80, 0xfb}));
    }
    const auto second = sc.r32(club + 0x80);
    const auto second_mode = rng.below(100);
    if (second_mode < bias.second_null) put(4, club + 0x80, 0);
    else if (second && second_mode < bias.second_null + 10) put(4, second + 0x1c, 0);
    else if (second) {
        if (const auto staff = sc.r32(second + 0x1c))
            put(1, staff + 0x3d, rng.chance(bias.trait_zero) ? 0 : rng.pick<std::uint32_t>({0, 1, 0xff, 3, 0xfb, 0x7f, 0x80, 0x10, 2, 0xfe}));
    }
    const auto league = sc.r32(club + 0x30);
    if (rng.chance(bias.league_null)) put(4, club + 0x30, 0);
    else if (league) {
        const auto entry = sc.r32(sc.global(leagues_slot) + sc.r32(league) * 4);
        if (entry) {
            put(1, entry + 0xc0, rng.pick<std::uint32_t>({0, 1, 5, 0x30, 0x7f, 0xff, 9}));
            put(1, entry + 0xc1, rng.pick<std::uint32_t>({0, 1, 5, 0x30, 0x7f, 0xff, 9}));
            if (rng.chance(4)) put(4, entry + 8, 0);
        }
    }
    // The draw table: halfwords in the scratch area, the base, and where the table ends.
    const bool threes = rng.chance(bias.multiples_of_three);
    for (unsigned k = 0; k < 16; ++k)
        put(2, table_area + k * 2, threes ? 3 * rng.below(600) : rng.chance(50) ? rng.below(24) : rng.below(2000));
    put(4, sc.toc(cursor_slot), table_area - 2 + 2 * rng.below(3));
    put(4, sc.toc(last_slot), rng.chance(15) ? table_area + 2 * rng.below(6) : table_area + 30);
    put(4, sc.toc(base_slot), threes ? 0 : rng.pick<std::uint32_t>({0, 0, 1, 2, 3, 7, 20, 0xffff, 0x10001}));
    // The results record 0x10459b50 answers (or none).
    put(1, record_area + 4, rng.pick<std::uint32_t>({0, 5, 20, 25, 30, 35, 255}));
    put(1, record_area + 0x77, rng.pick<std::uint32_t>({0, 5, 20, 100, 255}));
    for (unsigned g = 0; g < 5; ++g) put(1, record_area + 0x16 + g, rng.chance(bias.idle_form) ? rng.pick<std::uint32_t>({0, 4, 7}) : rng.pick<std::uint32_t>({3, 2, 1, 0xfd, 0, 4, 7, 3, 3, 1}));
    for (std::uint32_t at : {0x1cu, 0x30u, 0x44u, 0x58u}) {
        std::uint32_t value = rng.pick<std::uint32_t>({0, 1, 2, 3, 4, 0xffffffff, 2, 1});
        if (bias.signals == 1) value = at == 0x44 ? 2 + rng.below(2) : rng.below(2);  // only the second signal decides
        if (bias.signals == 2) value = at == 0x30 ? 2 + rng.below(2) : rng.below(2);  // only the fourth
        put(4, record_area + at, value);
    }
    // With no people to scan, the offer value (0x101abda0) is its floor, -10000: any assessment passes it.
    if (rng.chance(bias.empty_people)) put(4, sc.toc(-0x7be4), 0);
    const auto answer = [&](std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); };
    if (rng.chance(bias.real_calendar)) {
        // The real CALENDAR: the manager in post since a year ago, the row's dates today.
        put(4, row + 0x18, sc.r32(date));
        put(4, row + 0x1c, sc.r32(date + 4));
        if (manager) {
            put(4, manager + 0x4c, (sc.r32(date) & 0xffff0000u) | ((sc.r32(date) - 1) & 0xffff));
            put(4, manager + 0x50, sc.r32(date + 4));
        }
    } else {
        answer(calendar, rng.pick<std::uint32_t>({0x32, 0x96, 0x97, 200, 0x18f, 0x190, 1000, 101, 100, 0x3c, 0x1f, 0x1e, 0xf, 0xe}));
    }
    answer(results, rng.chance(5) ? 0 : record_area);
    for (const auto entry : {manager_flag, club_flag, club_state}) answer(entry, rng.chance(35) ? 1 : 0);
    // The module's own routines (0x101c1830, 0x101abda0, 0x101a9e00, 0x101b2a30) run as they are: the
    // original calls them directly, so an answer would reach only the rewrite.
    answer(append, 0);
    answer(glue, rng.chance(50) ? 1 : 0);
    answer(redraw, rng.next());
    answer(remove, 0);
    // The pending list for the departure check: the club's own entry among others, or none.
    if (departure_only || rng.chance(70)) {
        const unsigned count = rng.below(4);
        put(4, object + 0x10, list_area);
        put(2, list_area, count);
        put(4, list_area + 4, count ? node_area : 0);
        put(4, list_area + 0xc, rng.chance(80) ? 0 : node_area);
        const unsigned mine = rng.below(count + 1);
        for (unsigned k = 0; k < count; ++k) {
            put(4, node_area + k * 12, rng.chance(10) ? 0 : element_area + k * 16);
            put(4, node_area + k * 12 + 8, k + 1 < count ? node_area + (k + 1) * 12 : 0);
            put(4, element_area + k * 16, k == mine ? sc.r32(club) : sc.r32(club) + 1 + k);
        }
    }
    return item;
}
}

MANAGER_MANAGER_CASES("review randomized") {
    if (scenario.match || !scenario.global(-0x7b50)) return;
    Rng rng{0x1ee75eed};
    for (unsigned k = 0; k < 1500; ++k) cases.push_back(randomized(rng, scenario, false));
    // Leaning batches for the rules a uniform draw rarely reaches.
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.trait_zero = 80, .low_value = 1, .flag_one = 0, .attributes = 1, .empty_people = 70}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.second_null = 60, .multiples_of_three = 90, .real_calendar = 50, .low_value = 1, .flag_one = 0, .attributes = 1, .empty_people = 70}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.trait_zero = 80, .real_calendar = 60, .low_value = 1, .flag_one = 0, .attributes = 2, .empty_people = 70}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.staff_null = 40, .idle_form = 60, .low_value = 1}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.second_null = 50, .multiples_of_three = 70, .flag_one = 0}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.trait_zero = 70, .league_null = 40, .flag_one = 0}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, false, {.real_calendar = 100, .flag_one = 0}));
    for (unsigned k = 0; k < 200; ++k) cases.push_back(randomized(rng, scenario, false, {.signals = 1, .flag_one = 0}));
    for (unsigned k = 0; k < 200; ++k) cases.push_back(randomized(rng, scenario, false, {.signals = 2, .flag_one = 0}));
    for (unsigned k = 0; k < 60; ++k) cases.push_back(randomized(rng, scenario, false, {.flag_one = 0, .manager_null = 100}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, true));
}
}

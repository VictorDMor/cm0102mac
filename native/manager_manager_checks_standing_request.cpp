// MANAGER_MANAGER.CPP checker cases for 0x101bb9f0 (club_request, manager_manager_standing.hpp).
//
// No capture reaches it. A real club of the career carries the call: its row of the table is built in the scratch
// area (the table pointer cell is written), and the records the rules read (manager, owner, board, league, the
// employer's counter) are scratch records the club's pointers are written to. The real window table, date cell and
// draw table take the values a case writes (undone afterwards). The other modules' routines are answered (the
// flag, the class, the league entry, the employer, the relation, the two new record writers, the date score, the
// draw's redraw, the news desk), so every value a rule tests is chosen: a generator with a fixed seed draws each
// field from the values at and around its cuts, then single changes from a passing base of each kind steer the
// outcomes the random draws reach rarely. Draws of the module's table come from halfwords the case writes after
// the cursor, and the wrap of the table (which calls the redraw) is chosen per case.
#include "manager_manager_checks.hpp"
#include <algorithm>
#include <bit>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t entry = 0x101bb9f0;
constexpr std::uint32_t list_cell = scratch_base + 0x100, rows = scratch_base + 0x10000, owner = scratch_base + 0x200,
                        owner_profile = scratch_base + 0x300, manager = scratch_base + 0x400,
                        manager_profile = scratch_base + 0x500, board = scratch_base + 0x600,
                        league = scratch_base + 0x700, employer = scratch_base + 0x800;
namespace routine {
constexpr std::uint32_t flag = 0x100c7d00, region_class = 0x10191f80, league_entry = 0x10459b50,
                        employer_record = 0x10103510, relation = 0x100f55b0, adjust = 0x100f5770, writer = 0x100f4f20,
                        date_score = 0x10718130, redraw = 0x1077b770, news_desk = 0x102cc140;
}
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
constexpr std::uint64_t nan_bits = 0x7ff8000000000000ULL;

// Every field a case steers.
struct Scene {
    std::uint32_t kind = 1;  // r5
    std::uint32_t flag_answer = 0, region_class = 0, relation = 0, score = 0x50, redraw_value = 0;
    bool league_found = true, employer_found = true;
    // club
    bool has_league = true, has_manager = true, manager_profile = true;
    unsigned owner_mode = 2;  // 0 none, 1 no profile, 2 with profile
    bool has_board = true;
    std::uint32_t board_flag = 1, budget = 20000, worth = 1000, reputation = 0x1400, standing = 3, value = 0x2000;
    std::uint32_t region_word = 1;
    // row
    std::uint32_t count = 0, flags = 0, mood = 0, spare = 0, rating = 0x1000, amount = 0, tag = 0;
    std::uint64_t high = 0, low = 0;
    // owner
    std::uint32_t level = 10, temper = 5, trait = 5;
    // employer and league
    std::uint32_t funds_high = 0, funds_low = 10000000, employer_extra = 1000, league_tag = 0;
    // window and date
    std::uint32_t year = 2001, day = 100, low_year = 2000, low_day = 50, high_year = 2002, high_day = 150;
    // draws: the halfwords after the cursor; wrap makes the first draw call the redraw
    std::uint32_t x[4] = {1, 1, 1, 1};
    bool wrap = false;
};

Case make(const Scenario& sc, unsigned club_k, const Scene& s, std::string label) {
    const auto club = sc.club(club_k);
    const auto index = sc.r32(club);
    Case item{.entry = entry, .registers = {{3, list_cell}, {4, club}, {5, s.kind}}, .label = std::move(label)};
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    auto put_double = [&](std::uint32_t at, std::uint64_t value) {
        put(4, at, std::uint32_t(value >> 32));
        put(4, at + 4, std::uint32_t(value));
    };
    put(4, list_cell, rows);
    put(4, club + 0x30, s.has_league ? sc.r32(club + 0x30) : 0);
    put(4, club + 0x90, s.has_manager ? manager : 0);
    put(4, manager + 0x1c, s.manager_profile ? manager_profile : 0);
    put(2, manager_profile + 0x2a, s.value);
    put(4, manager + 0x4c, 0x1111);
    put(4, manager + 0x50, 0x2222);
    put(4, club + 0x3c, s.has_board ? board : 0);
    put(4, board + 4, s.budget);
    put(1, club + 0x23e, s.board_flag);
    put(4, club + 8, s.worth);
    put(2, club + 0x1ec, s.reputation);
    put(1, club + 0x240, s.standing);
    put(4, club + 0x80, s.owner_mode ? owner : 0);
    put(4, owner + 0x1c, s.owner_mode == 2 ? owner_profile : 0);
    put(1, owner + 0x63, s.temper);
    put(1, owner_profile + 0x2f, s.trait);
    put(1, owner_profile + 0x3d, s.level);
    // the club's row
    const auto row = rows + index * 0x4c;
    put(1, row + 2, s.flags);
    put(1, row + 3, s.mood);
    put(1, row + 4, s.count);
    put(1, row + 5, s.spare);
    put(2, row + 6, s.rating);
    put(4, row + 0x28, s.amount);
    put(4, row + 0x2c, s.tag);
    put_double(row + 0x30, s.high);
    put_double(row + 0x38, s.low);
    // the clubs table's region word, the window table and the date
    put(4, sc.global(-0x7be8) + index * 0x248 + 0x2c, s.region_word);
    const auto window = sc.toc(-0x7b44) + std::uint32_t(std::int32_t(std::int8_t(s.region_class))) * 0x4c;
    put(2, sc.toc(-0x7b80), s.year);
    put(2, sc.toc(-0x7b80) + 2, s.day);
    put(2, window + 0x2c, s.low_year);
    put(2, window + 0x2e, s.low_day);
    put(2, window + 0x3c, s.high_year);
    put(2, window + 0x3e, s.high_day);
    // the employer and the league entry
    put(4, employer, s.funds_high);
    put(4, employer + 4, s.funds_low);
    put(4, employer + 0x14, s.employer_extra);
    put(1, league + 0x75, s.league_tag);
    // the draw table: the cursor at its first entry (or its last, to wrap), halfwords after it
    const auto first = sc.toc(-0x7b8c), last = sc.r32(sc.toc(-0x7b88));
    put(4, sc.toc(-0x7b84), s.wrap ? last : first);
    put(4, sc.toc(-0x7b90), 0);
    for (unsigned n = 0; n < 4; ++n) put(2, first + 2 * (n + (s.wrap ? 0 : 1)), s.x[n]);
    item.answers = {{routine::flag, s.flag_answer}, {routine::region_class, s.region_class},
                    {routine::league_entry, s.league_found ? league : 0}, {routine::employer_record, s.employer_found ? employer : 0},
                    {routine::relation, s.relation}, {routine::adjust, 0}, {routine::writer, 0},
                    {routine::date_score, s.score}, {routine::redraw, s.redraw_value}, {routine::news_desk, 0}};
    return item;
}

// A scene that gets through the common checks and reaches the kind's tests.
Scene base(std::uint32_t kind) {
    Scene s;
    s.kind = kind;
    s.level = 10;  // due = 4
    s.count = 2;   // becomes 3 < due
    s.rating = 0x1000;
    s.mood = 0;
    return s;
}

std::uint32_t pick_funds(Rng& r, std::uint32_t& high) {
    high = r.pick<std::uint32_t>({0, 0, 0, 0, 0xffffffffu, 1, 0x80000000u, 0x7fffffffu});
    constexpr std::uint32_t cuts[] = {0, 1, 49999, 50000, 249999, 250000, 499999, 500000, 500001, 1249999, 1250000, 1250001,
                                      2000000, 2499999, 2500000, 2500001, 4999999, 5000000, 5000001, 7499999, 7500000, 7500001,
                                      12500000, 20000000, 0x7fffffff, 0x80000000u, 0xffffffffu};
    return cuts[r.below(sizeof cuts / sizeof *cuts)] + r.pick<std::uint32_t>({0, 0, 0, 0, 1, 0xffffffffu, 0xffffffe0u});
}

// A scene that passes every gate on the way to the deepest outcome of its kind (the pool's ratio is read
// from the game's memory so that kind 1's comparison passes).
Scene passing(const Scenario& sc, std::uint32_t kind) {
    Scene s = base(kind);
    s.region_class = 1;
    s.region_word = 1;
    s.league_tag = 0;
    s.tag = 0;
    s.funds_low = 10000000;
    s.low = 0;
    s.high = 0x3ff0000000000000ULL;
    s.x[0] = 3;
    s.x[1] = 0;
    s.x[2] = 7;
    s.x[3] = 9;
    const auto pool = sc.toc(-0x536c);
    const auto ratio = std::bit_cast<double>((std::uint64_t(sc.r32(pool + 0x2c0)) << 32) | sc.r32(pool + 0x2c4));
    s.worth = std::uint32_t(std::int32_t(std::clamp(ratio * s.budget + 100.0, 0.0, 2e9)));
    if (kind == 2) {
        s.standing = 8;
        s.reputation = 0x800;
        s.value = 0x2000;
    }
    if (kind == 4) s.x[0] = 9;
    return s;
}
// Takes each field of `other` with the given chance: a passing scene with a few fields changed.
Scene mixed(Rng& r, Scene s, const Scene& other, unsigned percent) {
    const auto take = [&](auto& field, const auto& value) { if (r.chance(percent)) field = value; };
    take(s.kind, other.kind); take(s.flag_answer, other.flag_answer); take(s.region_class, other.region_class);
    take(s.relation, other.relation); take(s.score, other.score); take(s.redraw_value, other.redraw_value);
    take(s.league_found, other.league_found); take(s.employer_found, other.employer_found);
    take(s.has_league, other.has_league); take(s.has_manager, other.has_manager); take(s.manager_profile, other.manager_profile);
    take(s.owner_mode, other.owner_mode); take(s.has_board, other.has_board); take(s.board_flag, other.board_flag);
    take(s.budget, other.budget); take(s.worth, other.worth); take(s.reputation, other.reputation);
    take(s.standing, other.standing); take(s.value, other.value); take(s.region_word, other.region_word);
    take(s.count, other.count); take(s.flags, other.flags); take(s.mood, other.mood); take(s.spare, other.spare);
    take(s.rating, other.rating); take(s.amount, other.amount); take(s.tag, other.tag); take(s.high, other.high);
    take(s.low, other.low); take(s.level, other.level); take(s.temper, other.temper); take(s.trait, other.trait);
    if (r.chance(percent)) { s.funds_high = other.funds_high; s.funds_low = other.funds_low; }
    take(s.employer_extra, other.employer_extra); take(s.league_tag, other.league_tag);
    if (r.chance(percent)) { s.low_year = other.low_year; s.low_day = other.low_day; s.high_year = other.high_year; s.high_day = other.high_day; }
    for (unsigned n = 0; n < 4; ++n) take(s.x[n], other.x[n]);
    take(s.wrap, other.wrap);
    // Steering: most of the time the count does not meet the due count (the first exit), and a third of kind 3's
    // scenes put the funds at the cut of the amount (the tail with the owner's second chance).
    if (r.chance(80)) {
        const std::int32_t level = std::int8_t(s.level);
        const std::int8_t due = s.owner_mode == 2 ? std::int8_t(level / 5 + 2) : 3;
        s.count = std::uint32_t(due) + r.pick<std::uint32_t>({0xff, 0xfe, 0xfd});
    }
    if (s.kind == 3 && r.chance(35)) {
        const std::int32_t rating = std::int16_t(s.rating == 0x1000 ? s.reputation : s.reputation);
        const std::int32_t reputation = std::int16_t(s.reputation);
        (void)rating;
        const std::int32_t amount = reputation > 0x1d4c   ? (reputation - 0x1d4c) * 0x1388 + 5000000
                                    : reputation > 0x109a ? (reputation - 0x109a) * 1000 + 2000000
                                                          : reputation * 100 + 50000;
        s.funds_high = 0;
        s.funds_low = std::uint32_t(amount) + r.pick<std::uint32_t>({0xffffffffu, 0, 1, 0xfffff000u});
    }
    return s;
}

Scene random_scene(Rng& r, std::uint32_t kind) {
    Scene s = base(kind);
    s.kind = r.chance(5) ? r.pick<std::uint32_t>({0, 7, 0xff, 0x101, 0x80, 0xfffffff9u, 0x15}) : kind;
    if (kind == 5 && r.chance(20)) s.kind = 5;
    s.flag_answer = r.chance(4) ? r.pick<std::uint32_t>({1, 0xff, 0x100, 0x80}) : 0;
    s.region_class = r.pick<std::uint32_t>({0, 1, 5, 0x21, 0x22, 0x7f, 0x80, 0xff, 0xffffff00u, 0x30, 0x1f});
    s.relation = r.pick<std::uint32_t>({0, 1, 2, 3, 0xff, 0xfe, 0xfd, 0xfc, 0x80, 0x7f, 0x102, 0x1ff});
    s.score = r.pick<std::uint32_t>({0, 0xb3, 0xb4, 0xb5, 0xffffffffu, 100, 0x7fffffffu, 0x80000000u, 0x7fffff5eu, 50, 0x114});
    s.redraw_value = r.below(0x30000);
    s.league_found = !r.chance(5);
    s.employer_found = !r.chance(5);
    s.has_league = !r.chance(3);
    s.has_manager = !r.chance(3);
    s.manager_profile = !r.chance(3);
    s.owner_mode = r.chance(40) ? 0 : r.chance(15) ? 1 : 2;
    s.has_board = !r.chance(15);
    s.board_flag = r.pick<std::uint32_t>({0, 1, 1, 0x80, 0xff});
    s.budget = r.pick<std::uint32_t>({0, 1, 14999, 15000, 15001, 24999, 25000, 25001, 60000, 74999, 75000, 75001, 0x7fffffffu, 0x80000000u, 20000});
    s.worth = r.pick<std::uint32_t>({0, 1, 100, 1000, 50000, 1000000, 0x7fffffffu, 0x80000000u, 0xffffffffu, 30000, 8000});
    s.reputation = r.pick<std::uint32_t>({0, 0x1000, 0x109a, 0x109b, 0x1388, 0x1389, 0x1482, 0x1483, 0x1d4c, 0x1d4d, 0x7fff, 0x8000, 0xcb2, 0xcb3, 0x128e, 0x128f, 0x1c52, 0x1c53, 0x5dc, 0x1400, 0x2000});
    s.standing = r.pick<std::uint32_t>({0, 1, 5, 6, 9, 0xa, 0xe, 0xf, 0x13, 0x14, 0x15, 0x7f, 0x80, 0xff, 3});
    s.value = r.pick<std::uint32_t>({0, 4999, 5000, 0x109a, 0x1099, 0x2000, 0x7fff, 0x8000, 0xffff, s.reputation - 0x5dc, s.reputation - 0x5dd, s.reputation - 0x5db, 0x1700});
    s.region_word = r.chance(30) ? 0 : r.below(100) + 1;
    s.level = r.pick<std::uint32_t>({0, 4, 5, 9, 10, 14, 15, 0x7f, 0x80, 0xff, 0xf6, 20, 30});
    const std::int32_t level = std::int8_t(s.level);
    const std::int8_t due = s.owner_mode == 2 ? std::int8_t(level / 5 + 2) : 3;
    s.count = std::uint32_t(due) + r.pick<std::uint32_t>({0xfe, 0xff, 0xff, 0, 1, 2, 0x7f, 0x80});
    s.temper = r.pick<std::uint32_t>({0, 1, 2, 5, 6, 7, 10, 0x7f, 0x80, 0xff, 0xf0});
    s.trait = r.pick<std::uint32_t>({0, 1, 5, 6, 10, 11, 0x7f, 0x80, 0xff});
    s.flags = r.below(8);
    s.mood = r.below(256);
    if (r.chance(30)) s.mood = (s.mood & 0x87) | ((s.kind & 0xf) << 3);
    s.spare = r.pick<std::uint32_t>({0, 0, 0, 1, 0x80, 5});
    s.rating = r.pick<std::uint32_t>({0, 0xdab, 0xdac, 0xdad, 0x10b3, 0x10b4, 0x10b2, 0x8000, 0x7fff, 0xfffe, 0x1000, 0xc00, 0xcb5});
    s.amount = r.chance(85) ? 0 : r.below(1000);
    s.tag = r.below(256) + (r.chance(20) ? 0x300 : 0);
    s.league_tag = (s.tag + r.pick<std::uint32_t>({0xff, 0, 1, 0xfb, 0xfc, 4, 5, 6, 0xfa, 0x80, 0x7f})) & 0xff;
    constexpr std::uint64_t doubles[] = {0, 0x3ff0000000000000ULL, 0x4000000000000000ULL, 0xbff0000000000000ULL, nan_bits, 0x8000000000000000ULL,
                                         0x4059000000000000ULL, 0x40f86a0000000000ULL, 0x41cdcd6500000000ULL, 0xc1cdcd6500000000ULL, 0x4197d78400000000ULL};
    s.high = doubles[r.below(sizeof doubles / sizeof *doubles)];
    s.low = doubles[r.below(sizeof doubles / sizeof *doubles)];
    s.funds_low = pick_funds(r, s.funds_high);
    s.employer_extra = r.pick<std::uint32_t>({0, 59999, 60000, 60001, 0x7fffffffu, 0x80000000u, 1000});
    // the window and the date: each bound at, below and above the date
    s.year = 2001;
    s.day = 100;
    const auto around = [&](std::uint32_t centre) { return centre + r.pick<std::uint32_t>({0xffffffffu, 0, 1}); };
    s.low_year = around(s.year);
    s.low_day = around(s.day);
    s.high_year = around(s.year);
    s.high_day = around(s.day);
    for (auto& v : s.x) v = r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 15, 19, 20, 21, 25, 99, 100, 101, 0xffff, 0x7fff, 0x8000, 0x1234, r.below(0x10000)});
    s.wrap = r.chance(15);
    return s;
}
}

MANAGER_MANAGER_CASES("club_request") {
    if (scenario.match) return;
    // a real club with a league record and a small index
    unsigned club_k = 0;
    const auto total = scenario.global(-0x7b50);
    for (unsigned k = 0; k < total && k < 200; ++k)
    {
        const auto league_index = scenario.r32(scenario.club(k) + 0x30) ? scenario.r32(scenario.r32(scenario.club(k) + 0x30)) : 0;
        const auto table = scenario.global(-0x7b5c);
        if (scenario.r32(scenario.club(k) + 0x30) && scenario.r32(scenario.club(k)) < 0x400 && league_index < 0x1000 &&
            scenario.r32(table + 4 * league_index) && scenario.r32(scenario.r32(table + 4 * league_index) + 8)) {
            club_k = k;
            break;
        }
    }
    // a call with the real routines: the first clubs of the career, as the game would call it
    for (std::uint32_t kind : {1u, 2u, 3u, 4u, 5u, 6u}) {
        Case item{.entry = entry, .registers = {{3, scenario.global(-0x79f0)}, {4, scenario.club(club_k)}, {5, kind}},
                  .label = "real world, kind " + std::to_string(kind)};
        cases.push_back(std::move(item));
    }
    cases.push_back({.entry = entry, .registers = {{3, list_cell}, {4, 0}, {5, 1}}, .label = "club null"});
    // single changes from the base of each kind
    for (std::uint32_t kind = 1; kind <= 6; ++kind) cases.push_back(make(scenario, club_k, base(kind), "base kind " + std::to_string(kind)));
    Rng rng{0x5eed0101ULL};
    for (unsigned n = 0; n < 3000; ++n) {
        const auto kind = 1 + n % 6;
        cases.push_back(make(scenario, club_k, random_scene(rng, kind), "random " + std::to_string(n) + " kind " + std::to_string(kind)));
    }
    // The passing scene of each kind with a few fields taken from random scenes (the deep outcomes).
    for (unsigned n = 0; n < 6000; ++n) {
        const auto kind = 1 + n % 6;
        const auto percent = n % 3 == 0 ? 6u : n % 3 == 1 ? 12u : 25u;
        cases.push_back(make(scenario, club_k, mixed(rng, passing(scenario, kind), random_scene(rng, kind), percent),
                             "mixed " + std::to_string(n) + " kind " + std::to_string(kind)));
    }
    for (std::uint32_t kind = 1; kind <= 6; ++kind) cases.push_back(make(scenario, club_k, passing(scenario, kind), "passing kind " + std::to_string(kind)));
    // Families around the cuts the random draws reach rarely.
    const auto label = [](const char* what, unsigned n) { return std::string(what) + " " + std::to_string(n); };
    unsigned family = 0;
    // kind 1: the budget's three bands against the funds at their limits; no owner with every residue of the draw.
    for (std::uint32_t budget : {14999u, 15000u, 15001u, 24999u, 25000u, 25001u, 74999u, 75000u, 75001u})
        for (std::uint32_t limit : {2500000u, 5000000u, 7500000u})
            for (std::uint32_t delta : {0xffffffffu, 0u, 1u}) {
                auto s = passing(scenario, 1);
                s.budget = budget;
                s.funds_low = limit + delta;
                cases.push_back(make(scenario, club_k, s, label("kind 1 budget", family++)));
            }
    for (std::uint32_t x : {0u, 1u, 4u, 5u, 10u, 15u, 0xffffu})
        for (unsigned owner_mode : {0u, 2u})
            for (std::uint32_t temper : {0u, 1u, 5u, 0xffu}) {
                auto s = passing(scenario, 1);
                s.owner_mode = owner_mode;
                s.level = 0;  // due 2 (3 without an owner); the count of 1 stays below it
                s.count = 0;
                s.x[0] = x;
                s.temper = temper;
                cases.push_back(make(scenario, club_k, s, label("kind 1 draw", family++)));
            }
    // kind 3: the funds at the amount's cuts with each shape of owner and trait.
    for (std::uint32_t reputation : {0x109au, 0x109bu, 0x1099u, 0x1d4cu, 0x1d4du, 0x1482u, 0x1483u, 0x8000u, 0xffffu, 0u})
        for (std::uint32_t owner_mode : {0u, 1u, 2u})
            for (std::uint32_t trait : {0u, 1u, 5u, 0xfdu, 0x80u})
                for (std::uint32_t x : {0u, 3u, 10u}) {
                    auto s = passing(scenario, 3);
                    s.reputation = reputation;
                    s.value = 0x7fff;
                    s.owner_mode = owner_mode;
                    s.count = owner_mode == 2 ? 0 : 0;
                    s.level = 0;
                    s.trait = trait;
                    s.x[0] = 3;
                    s.x[1] = x;
                    s.x[2] = x;
                    const std::int32_t rep = std::int16_t(reputation);
                    const std::int32_t amount = rep > 0x1d4c ? (rep - 0x1d4c) * 0x1388 + 5000000
                                                : rep > 0x109a ? (rep - 0x109a) * 1000 + 2000000
                                                               : rep * 100 + 50000;
                    s.funds_high = 0;
                    s.funds_low = std::uint32_t(amount) - 1 + (x == 3 ? 1 : 0);
                    if (s.funds_low < 2500000) s.funds_low = 2500000;
                    cases.push_back(make(scenario, club_k, s, label("kind 3 amount", family++)));
                }
    // The date window: each bound at, below and above the date, for the kinds that scale by it, with the class gap
    // on each side of its cuts; the class from the region (a flag word, none) too.
    for (std::uint32_t kind : {3u, 4u})
        for (std::uint32_t ly : {2000u, 2001u, 2002u})
            for (std::uint32_t ld : {99u, 100u, 101u})
                for (std::uint32_t hy : {2000u, 2001u, 2002u})
                    for (std::uint32_t hd : {99u, 100u, 101u}) {
                        auto s = passing(scenario, kind);
                        s.low_year = ly;
                        s.low_day = ld;
                        s.high_year = hy;
                        s.high_day = hd;
                        s.level = 0;
                        s.count = 0;
                        s.tag = 10;
                        s.league_tag = 10 + (ly + ld + hy + hd) % 7 - 1;  // the gap -1..5
                        s.funds_low = 3000000;
                        cases.push_back(make(scenario, club_k, s, label("window", family++)));
                    }
    // kind 4: the temper and trait cuts of the owner with each relation.
    for (std::uint32_t temper : {4u, 5u, 6u, 7u, 0x80u})
        for (std::uint32_t owner_mode : {0u, 1u, 2u})
            for (std::uint32_t relation : {0xfdu, 0xfeu, 0xffu, 0u, 1u, 2u, 3u})
                for (std::uint32_t trait : {0u, 5u, 10u, 0xffu}) {
                    auto s = passing(scenario, 4);
                    s.temper = temper;
                    s.owner_mode = owner_mode;
                    s.relation = relation;
                    s.trait = trait;
                    s.level = 0;
                    s.count = 0;
                    s.x[0] = 9;
                    s.x[1] = 0;
                    s.x[2] = 7;
                    cases.push_back(make(scenario, club_k, s, label("kind 4 owner", family++)));
                }
}
}

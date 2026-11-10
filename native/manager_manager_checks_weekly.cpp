// MANAGER_MANAGER.CPP checker cases for the weekly pass (manager_manager_weekly.hpp): 0x101a3350.
//
// The real call over every club is one case. While the routine is written in parts, only the first
// club's body runs in C++, so the other cases write the club count as 1 and copy a real club's
// record and its row of the board over club 0 and row 0 (undone afterwards): a spread of real
// clubs, then randomized variants that write, over what the body reads, values steering the rules
// of the parts, with several seeds of the generator.
#include "manager_manager_checks.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t weekly = 0x101a3350;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50;
constexpr std::uint32_t club_size = 0x248, row_size = 0x4c;
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
// A case for the first club's body with club `k` and row `k` copied over club 0 and row 0.
Case one_club(const Scenario& sc, unsigned k, std::string label) {
    Case item{.entry = weekly, .registers = {{3, sc.global(manager_object)}}, .label = std::move(label)};
    item.memory.push_back({4, sc.toc(club_total), 1});
    const auto rows = sc.r32(sc.global(manager_object));
    for (unsigned at = 0; at < club_size; at += 4) item.memory.push_back({4, sc.club(0) + at, sc.r32(sc.club(k) + at)});
    for (unsigned at = 0; at < row_size; at += 4) item.memory.push_back({4, rows + at, sc.r32(rows + k * row_size + at)});
    return item;
}
}

constexpr std::uint32_t seed_address = 0x108bba90 - 0x34c4;
// The other modules' routines whose answers steer the rules: the club's flag, the "does the
// club run" test, the region's own club, the region person's acceptance.
namespace other {
constexpr std::uint32_t club_flag = 0x100c7d00, bad_run = 0x100d3c40, own_club = 0x100c76c0, person_ok = 0x102aed80,
                        region_person = 0x102b7450, staff_of = 0x10459b50, hire_by_club = 0x106487d0, hire_any = 0x10647730;
}

MANAGER_MANAGER_CASES("weekly") {
    if (scenario.match) return;
    const auto clubs = scenario.global(club_total);
    // The real call over every club (the anchor: the loop over clubs is C++).
    cases.push_back({.entry = weekly, .registers = {{3, scenario.global(manager_object)}}, .label = "weekly: every club"});
    if (!clubs) return;
    // No clubs: the loop's test fails at once (zero, and a negative count as a signed compare sees it).
    for (const std::uint32_t count : {0u, 0xffffffffu, 0x80000000u}) {
        Case none{.entry = weekly, .registers = {{3, scenario.global(manager_object)}}, .label = "weekly: club count " + std::to_string(count)};
        none.memory.push_back({4, scenario.toc(club_total), count});
        cases.push_back(std::move(none));
    }
    const auto rows = scenario.r32(scenario.global(manager_object));
    const auto date = scenario.toc(-0x7b80);
    Rng rng{0x77656b6c79ULL};
    // The draw table's cells (TOC -0x7b90 base, -0x7b8c first entry, -0x7b88 last entry's cell,
    // -0x7b84 cursor): each case starts the table at its own place, so that the draws differ, and
    // a few at the last entry, so that the next draw wraps and redraws the base from the seed.
    const auto table_first = scenario.toc(-0x7b8c), table_last = scenario.r32(scenario.toc(-0x7b88));
    const auto entries = (table_last - table_first) / 2 + 1;
    // The first N real clubs: the loop runs later clubs, whose events can set the frame flags (sp+0xdd, sp+0xdc) that
    // stop a later club's hire. Several seeds each (the draws decide which clubs act).
    {
        Rng rl{0x6c6f6f70ULL};
        for (const unsigned count : {2u, 3u, 4u, 6u, 10u, 25u, 60u, 150u, 400u, 677u}) {
            if (count > clubs) continue;
            for (unsigned variant = 0; variant < 12; ++variant) {
                Case item{.entry = weekly, .registers = {{3, scenario.global(manager_object)}},
                          .label = "weekly: first " + std::to_string(count) + " clubs v" + std::to_string(variant)};
                item.memory.push_back({4, scenario.toc(club_total), count});
                item.memory.push_back({4, seed_address, rl.next()});
                item.memory.push_back({4, scenario.toc(-0x7b84), table_first + 2 * rl.below(entries)});
                item.memory.push_back({4, scenario.toc(-0x7b90), (variant % 2 ? 10 * rl.below(6000) : rl.below(0x10000))});
                cases.push_back(std::move(item));
            }
        }
    }
    const auto seeded = [&](Case item, bool ten = false) {
        item.memory.push_back({4, seed_address, rng.next()});
        const auto cursor = table_first + 2 * (rng.chance(8) ? entries - 1 : rng.below(entries));
        auto base = rng.below(0x10000);
        // Most cases make the first draw a multiple of ten (the rule the first section opens with
        // passes one time in ten): the base completes the halfword at the cursor to one.
        if (ten || rng.chance(70)) {
            // the draw steps the cursor first: the halfword after it (a wrap redraws the base: unsteered)
            const auto halfword = std::uint32_t(scenario.s.memory.read<2>(std::min(cursor + 2, table_last)));
            base = (10 - halfword % 10) % 10 + 10 * rng.below(6000);
        }
        // A few with a base so large that the sum wraps negative (the draw's modulo of a negative number),
        // still a multiple of ten for the first rule.
        if (ten && rng.chance(25)) {
            const auto halfword = std::uint32_t(scenario.s.memory.read<2>(std::min(cursor + 2, table_last)));
            base = 0x7ffff000u + rng.below(0x1000);
            while (std::int32_t(halfword + base) >= 0 || std::int32_t(halfword + base) % 10 != 0) ++base;
        }
        item.memory.push_back({4, scenario.toc(-0x7b84), cursor});
        item.memory.push_back({4, scenario.toc(-0x7b90), base});
        return item;
    };
    // The real clubs: every fifth with a manager record, every 29th of the others, as they are.
    for (unsigned k = 0; k < clubs; ++k) {
        const bool managed = scenario.r32(scenario.club(k) + 0x90) != 0;
        if (managed ? k % 5 == 0 : k % 29 == 0)
            cases.push_back(seeded(one_club(scenario, k, "weekly: club " + std::to_string(k))));
    }
    // Randomized: a real managed club with the rules' inputs written over it, today's date moved,
    // and some of the other modules' answers forced.
    std::vector<unsigned> managed;
    for (unsigned k = 0; k < clubs; ++k)
        if (scenario.r32(scenario.club(k) + 0x90)) managed.push_back(k);
    if (managed.empty()) return;
    // Indices into the leagues' table (TOC -0x7b5c) whose record exists and has its +8 field: the
    // region person's index word, which a case builds in the scratch area, names one of them.
    std::vector<std::uint32_t> league_slots, bare_slots, null_slots;
    {
        const auto table = scenario.r32(scenario.toc(-0x7b5c));
        for (std::uint32_t k = 0; k < 400; ++k) {
            try {
                const auto record = scenario.r32(table + 4 * k);
                if (record && scenario.r32(record + 8)) league_slots.push_back(k);
                else if (record) bare_slots.push_back(k);
                else null_slots.push_back(k);
            } catch (...) { break; }
        }
    }
    constexpr std::uint32_t person_word = scratch_base + 0x2000, staff_record = scratch_base + 0x3000,
                            hired_person = scratch_base + 0x4000;
    const auto small = [&] { return rng.pick<std::uint32_t>({0, 1, 2, 3, 7, 50, 100, 200, 1000, 5000, 0x7fff, 0x8000, 0xff00, 0xffff}); };
    for (unsigned n = 0; n < 9000; ++n) {
        const auto k = managed[rng.below(unsigned(managed.size()))];
        auto item = seeded(one_club(scenario, k, "weekly: random " + std::to_string(n) + " over club " + std::to_string(k)), n % 3 != 0);
        const auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        // Every other case aims at the later rules: the club passes the tests, is its region's own, the
        // region's person is a real league record's and accepted, the table's first draw is a multiple
        // of ten, and today's day is swept (the windows open on some days only).
        const bool aimed = n % 3 != 0 && !league_slots.empty();
        // Some of them aim at the second rule: a staff record that skips the first one, a high level and
        // an idle count between the bounds.
        const bool second = aimed && n % 4 == 1;
        // The league's person: his record loses its +8 field, or the staff lookup finds nothing.
        if (!aimed && rng.chance(6)) {
            const auto league = scenario.r32(scenario.club(k) + 0x30);
            const auto table = scenario.r32(scenario.toc(-0x7b5c));
            if (league)
                if (const auto record = scenario.r32(table + 4 * scenario.r32(league))) put(4, record + 8, 0);
        }
        if (!aimed && rng.chance(8)) item.answers.push_back({other::staff_of, 0});
        // The row's three halfwords.
        if (rng.chance(2)) put(4, scenario.r32(scenario.club(k) + 0x90) + 0x1c, 0);
        put(2, rows + 6, small());
        put(2, rows + 8, small());
        if (rng.chance(50)) put(2, rows + 0x16, small());
        if (rng.chance(40)) put(2, scenario.club(0) + 0x1ec, small());
        if (aimed ? rng.chance(90) : rng.chance(40)) put(2, date, rng.below(aimed ? 366 : 400));
        if (rng.chance(aimed ? 10 : 40)) put(2, date + 2, 2000 + rng.below(8));
        // The other modules' answers.
        if (aimed || rng.chance(60)) item.answers.push_back({other::club_flag, aimed ? 1u : rng.below(2)});
        if (aimed || rng.chance(75)) item.answers.push_back({other::bad_run, 1});
        if (aimed || rng.chance(60)) item.answers.push_back({other::own_club, aimed || rng.chance(70) ? scenario.club(0) : scenario.club(1)});
        // The frame's two flags as an earlier club's event leaves them (the prologue clears them, so for the first club
        // they are zero): written when the club's own-club test is dispatched, before the rules read them.
        if (aimed && n % 5 >= 2) {
            const auto frame = std::uint32_t(scenario.s.r<4>(4)) - 0x150;
            item.pokes.push_back({other::own_club, {{1, frame + 0xdd, n % 5 == 3 ? 0u : 1u}, {1, frame + 0xdc, n % 5 == 2 ? (n / 5) % 2 : 1u}}});
        }
        if (aimed || (!league_slots.empty() && rng.chance(60))) {
            // The region's person (and his acceptance) forced, naming a real league record.
            // Now and then an index whose record is missing, or has no +8 field.
            const auto slot = !null_slots.empty() && rng.chance(3) ? null_slots[rng.below(unsigned(null_slots.size()))]
                              : !bare_slots.empty() && rng.chance(6) ? bare_slots[rng.below(unsigned(bare_slots.size()))]
                                                                 : league_slots[rng.below(unsigned(league_slots.size()))];
            put(4, person_word, slot);
            item.answers.push_back({other::region_person, person_word});
            item.answers.push_back({other::person_ok, aimed ? 1u : rng.pick<std::uint32_t>({1, 1, 1, 0})});
            // The league person's byte at +0x36 (the level), and the staff record the club's lookup
            // answers: its busy byte (+4), idle (+0x75), +0x76/+0x77/+0x78 bytes; the hire lookups answer a
            // person index or none.
            const auto table = scenario.r32(scenario.toc(-0x7b5c));
            if (const auto record = scenario.r32(table + 4 * slot))
                put(2, record + 0x36, second ? 30 + rng.below(30) : rng.chance(80) ? rng.below(80) - 20u : (rng.chance(50) ? rng.below(256) - 128u : rng.below(0x10000)));
            put(2, person_word + 0x18, second ? rng.below(4) : rng.below(40));
            if (second || rng.chance(70)) {
                item.answers.push_back({other::staff_of, staff_record});
                put(1, staff_record + 4, !second && rng.chance(10) ? 1 : 0);
                put(1, staff_record + 0x75, second ? 8 + rng.below(6) : rng.below(24));
                put(1, staff_record + 0x76, rng.below(4));
                put(1, staff_record + 0x77, second ? 0 : rng.chance(75) ? 1 : rng.below(3));
                put(1, staff_record + 0x78, rng.below(3));
                item.answers.push_back({other::hire_by_club, rng.pick<std::uint32_t>({0xffffffffu, 0xfffffffeu, 0, 5, 300})});
                item.answers.push_back({other::hire_any, rng.chance(50) ? 0 : hired_person});
            }
        }
        cases.push_back(std::move(item));
    }

    // ---- Part B family: the hub's rules and what follows (0x101a3cb8..0x101a4e08). The club's staff
    // lookup answers a scratch record whose posts, counts and bytes steer the factor and the staff
    // rules; the news constant f31 is compared with, the club's and the league's numbers, the
    // row's doubles, and the transfer list of 0x103f3670 (answered, with its outputs poked into
    // the frame) are written over the real ones. ----
    {
        Rng rb{0x7061727462ULL};
        const auto sp = std::uint32_t(scenario.s.r<4>(4));
        const auto frame = sp - 0x150;
        const auto news = scenario.toc(-0x5370);
        const auto table = scenario.r32(scenario.toc(-0x7b5c));
        const auto date_words = date;
        constexpr std::uint32_t staff = scratch_base + 0x3000, holder = scratch_base + 0x5000, inner = scratch_base + 0x5100,
                                list = scratch_base + 0x6000;
        const auto bits = [](float f) { return std::bit_cast<std::uint32_t>(f); };
        const auto doubles = [&](Case& item, std::uint32_t at, double v) {
            const auto raw = std::bit_cast<std::uint64_t>(v);
            item.memory.push_back({4, at, std::uint32_t(raw >> 32)});
            item.memory.push_back({4, at + 4, std::uint32_t(raw)});
        };
        // n >= 6000: the Part C family, the same scaffolding with the staff record's numbers steered into one of the
        // eleven sections of 0x101a4e08..0x101a6958 (aim 1..11: s60 s43 s63 s61 s62 s40 s41 s5e s3e s3f s6790).
        for (unsigned n = 0; n < 6000 + 22000; ++n) {
            const unsigned aim = n < 6000 ? 0 : 1 + (n - 6000) % 11;
            const auto k = managed[rb.below(unsigned(managed.size()))];
            auto item = seeded(one_club(scenario, k, std::string(aim ? "weekly: C" + std::to_string(aim) + " " : "weekly: B ") + std::to_string(n) + " over club " + std::to_string(k)), rb.chance(50));
            const auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
            const auto league = scenario.r32(scenario.club(k) + 0x30);
            const auto club0 = scenario.club(0);
            const auto record = league ? scenario.r32(table + 4 * scenario.r32(league)) : 0;
            if (!league || !record || !scenario.r32(record + 8)) continue;
            const auto region = scenario.r32(scenario.club(k) + 0x2c);
            const bool flagged_zero = rb.chance(50);
            item.answers.push_back({other::club_flag, flagged_zero ? 0u : 1u});
            item.answers.push_back({other::bad_run, 1});
            item.answers.push_back({other::own_club, scenario.club(1)});
            item.answers.push_back({other::staff_of, staff});
            item.registers.push_back({13, rb.chance(70) ? rb.below(100) + 10u : rb.below(256)});
            // The staff record.
            put(1, staff + 4, rb.chance(60) ? 0 : rb.chance(50) ? rb.below(9) : 5 + rb.below(60));
            put(1, staff + 0x75, rb.chance(40) ? rb.below(4) : rb.below(30));
            put(1, staff + 0x76, rb.below(6));
            const auto v77 = rb.below(4);
            put(1, staff + 0x77, v77);
            put(1, staff + 0x78, rb.chance(50) ? v77 : rb.below(4));
            const auto small_count = [&] { return rb.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, 9, 0xffffffffu, 0x10003}); };
            const std::uint32_t counts[] = {0x1c, 0x30, 0x58, 0x44}, since[] = {0x28, 0x3c, 0x64, 0x50};
            for (unsigned post = 0; post < 4; ++post) {
                put(4, staff + counts[post], small_count());
                const bool recent = rb.chance(70);
                for (unsigned word = 0; word < 2; ++word)
                    put(4, staff + since[post] + 4 * word, recent ? scenario.r32(date_words + 4 * word) : rb.next());
            }
            put(4, staff + 0x1c, small_count());
            put(4, staff + 0x44, small_count());
            // The frame bytes the stats call leaves, written when the power call (always on the way to the rules
            // that read them) is dispatched: the stats routine itself runs for real.
            const auto flag_byte = rb.chance(40) ? 1u : rb.below(3), stat_byte = rb.below(12);
            for (const std::uint32_t entry : {0x10718130u, 0x100c7d00u, 0x10780fc0u})
                item.pokes.push_back({entry, {{1, frame + 0x38, flag_byte}, {1, frame + 0x3a, stat_byte}}});
            // The news constants f31 is compared with, and the club, league and row.
            put(4, news + 4, bits(rb.pick<float>({-1.0e9f, -300.0f, -10.0f, 0.0f, 10.0f, 30.0f, 300.0f, 1.0e9f})));
            put(1, club0 + 0x23d, rb.below(3));
            put(2, club0 + 0x1ec, rb.chance(15) ? rb.below(3) : rb.pick<std::uint32_t>({100, 1000, 3000, 6000, 6500, 7000, 8000, 9000, 12000, 20000}));
            put(2, league + 0x18, rb.chance(8) ? 0 : rb.pick<std::uint32_t>({1, 5, 7, 8, 10, 12, 20, 60}));
            if (rb.chance(12)) put(4, club0 + 0x2c, 0);
            put(4, record + 0x1c, rb.chance(55) ? 0xffffffffu : rb.below(5));
            put(2, record + 0x3e, rb.pick<std::uint32_t>({0xff80, 0, 1, 5, 50, 100, 127, 0x7f00}));
            put(1, record + 0xc0, rb.below(256));
            put(1, record + 0xc1, rb.below(256));
            if (region && rb.chance(85)) put(1, region + 0x120, rb.chance(85) ? 1 : 0);
            // The club's +0x80 holder with its record's byte at +0x3d.
            if (rb.chance(85)) {
                put(4, club0 + 0x80, holder);
                put(4, holder + 0x1c, rb.chance(90) ? inner : 0);
                put(1, inner + 0x3d, rb.pick<std::uint32_t>({0, 0xff, 1, 2, 10, 19, 20, 21, 30}));
            } else {
                put(4, club0 + 0x80, rb.chance(50) ? 0 : holder);
                put(4, holder + 0x1c, 0);
            }
            // A quarter aim at the last stanza (0x101a4a74..): bytes the long chain of tests passes.
            if (aim || rb.chance(25)) {
                put(1, staff + 4, 10 + rb.below(18));
                put(1, staff + 0x75, rb.below(3));
                put(1, staff + 0x76, rb.below(4));
                put(1, staff + 0x77, 0);
                put(1, staff + 0x78, 2 + rb.below(2));
                put(1, rows + 2, rb.below(0x80));
                put(4, record + 0x1c, rb.below(5));
                put(1, record + 0xc0, 40 + rb.below(60));
                put(1, record + 0xc1, 40 + rb.below(60));
                put(4, staff + 0x1c, rb.below(8));
                put(4, staff + 0x44, rb.below(9));
                if (rb.chance(60)) put(4, news + 4, bits(1.0e9f));
                if (region) put(1, region + 0x120, 1);
                put(4, club0 + 0x2c, region);
            }
            const auto manager = scenario.r32(scenario.club(k) + 0x90);
            if (manager) {
                for (unsigned word = 0; word < 2; ++word)
                    put(4, manager + 0x4c + 4 * word, rb.chance(50) ? scenario.r32(date_words + 4 * word) : rb.next());
            }
            put(1, rows + 2, rb.chance(60) ? rb.below(256) & 0x7f : rb.below(256));
            put(2, rows + 0x14, small());
            put(2, rows + 6, small());
            put(2, rows + 0x12, small());
            doubles(item, rows + 0x30, rb.pick<double>({1.0, 100.0, 5000.0, 0.0}));
            doubles(item, rows + 0x38, rb.pick<double>({0.5, 150.0, 5000.0, 20000.0, 0.0}));
            // The transfer list: entries for this club (id at +0x10 = club's own id), or for others.
            const auto id = scenario.r32(scenario.club(k));
            const bool tight = rb.chance(25);
            const auto entries_n = tight ? 2 + rb.below(3) : rb.chance(40) ? 0 : 1 + rb.below(4);
            // The pool's constants a few cases move: the amount's scales (to reach the cap) and the unit bound.
            const auto pool = scenario.toc(-0x536c);
            if (rb.chance(10)) { doubles(item, pool + 0x170, 1.0e8); doubles(item, pool + 0x178, 1.0e8); }
            if (rb.chance(10)) doubles(item, pool + 0x98, rb.pick<double>({0.0, 0.1}));
            for (unsigned e = 0; e < entries_n; ++e) {
                const auto at = list + 0x28 * e;
                put(4, at, rb.below(5000));
                const auto role = rb.below(3);
                put(4, at + 0x10, role == 0 || tight ? id : rb.below(300));
                put(4, at + 0x14, role == 1 && !tight ? id : rb.below(300));
                put(4, at + 0x18, tight ? rb.pick<std::uint32_t>({3000000, 4000000, 6000000}) : rb.pick<std::uint32_t>({0, 100, 1000000, 3000000, 5000000, 8000000, 20000000, 0x7fffffff}));
            }
            item.answers.push_back({0x103f3670, 0});
            item.pokes.push_back({0x103f3670, {{4, frame + 0x3c, entries_n || rb.chance(50) ? list : 0}, {4, frame + 0x40, entries_n}}});
            if (tight) item.pokes.push_back({0x10195e10, {{4, frame + 0x3c, 0}}});

            if (aim) {
                const auto between = [&](std::uint32_t lo, std::uint32_t hi) { return lo + rb.below(hi - lo + 1); };
                const auto clamp8 = [](std::int32_t v) { return std::uint32_t(std::clamp(v, 0, 255)); };
                std::uint32_t idle = 0, b75 = 0, b76 = 0, b3a = 1, b38 = 0, b39 = 0, w1c = 0, w44 = 0, w30 = 0, w58 = 0, c3 = rb.below(6), c0 = 0, c1 = 0;
                std::int32_t lvl = std::int32_t(between(20, 80));
                std::uint32_t w = 0xffffffffu;
                b38 = rb.pick<std::uint32_t>({0, 2, 3, 5});
                b39 = rb.pick<std::uint32_t>({2, 3});
                switch (aim) {
                case 1:
                    lvl = std::int32_t(between(30, 90));
                    idle = between(30, 255);
                    b76 = clamp8(lvl - std::int32_t(c3) + std::int32_t(rb.below(10)) - 3);
                    b75 = rb.below(20);
                    b3a = rb.below(std::uint32_t(std::max(1, lvl / 2 - std::int32_t(c3) + 2)));
                    b38 = clamp8(lvl - std::int32_t(c3) + std::int32_t(rb.below(30)) - 3);
                    w1c = rb.pick<std::uint32_t>({0, 1, 2, 6});
                    w44 = rb.below(7);
                    break;
                case 2:
                    idle = between(205, 255);
                    w44 = rb.below(4);
                    w1c = rb.below(4);
                    w58 = rb.below(4);
                    b76 = clamp8(lvl / 2 + (rb.chance(70) ? 0 : std::int32_t(rb.below(5)) - 2));
                    b75 = rb.below(10);
                    b3a = std::uint32_t(lvl / 10 + 1) + rb.below(5);
                    b38 = rb.below(std::uint32_t(std::max(1, lvl - std::int32_t(c3))));
                    break;
                case 3: case 4:
                    idle = between(10, 28);
                    b76 = between(2, 3);
                    b75 = aim == 3 ? between(4, 5) : between(6, 20);
                    w1c = rb.below(9);
                    w44 = rb.below(10);
                    break;
                case 5:
                    idle = between(10, 29);
                    b76 = 1;
                    b75 = 1;
                    w1c = rb.below(7);
                    w44 = rb.below(10);
                    break;
                case 6:
                    w = rb.below(5);
                    idle = rb.chance(20) ? between(10, 40) : between(34, 255);
                    b76 = rb.chance(10) ? rb.below(2) : between(2, 8);
                    b75 = rb.below(7);
                    c0 = between(5, 60);
                    c1 = rb.below(60);
                    b3a = rb.below(c0 + c1 + 1);
                    b38 = c0 + c1 + 1 + rb.below(20);
                    w30 = rb.below(7);
                    w58 = rb.below(10);
                    break;
                case 7:
                    idle = between(34, 255);
                    b76 = rb.below(6);
                    b75 = rb.below(5);
                    w30 = rb.below(7);
                    w58 = rb.below(10);
                    break;
                case 8:
                    idle = between(15, 22);
                    b75 = between(1, 10);
                    b76 = rb.below(std::min(b75, 5u));
                    w1c = rb.pick<std::uint32_t>({0, 1, 3, 5, 9});
                    w44 = rb.below(13);
                    put(2, club0 + 0x1ec, between(1, 5000));
                    break;
                case 10: {
                    // +0x3a strictly between gap + level/2 and gap: a negative level and a negative league byte +0xc3.
                    lvl = -std::int32_t(between(4, 30));
                    const auto c3s = -std::int32_t(between(40, 120));
                    c3 = std::uint32_t(c3s) & 0xff;
                    const auto gap = lvl - c3s, lower = gap + lvl / 2;
                    idle = between(31, 255);
                    b76 = clamp8(gap + 2 + std::int32_t(rb.below(5)));
                    b39 = clamp8(gap + std::int32_t(rb.below(10)));
                    b3a = clamp8(lower + 1 + std::int32_t(rb.below(std::uint32_t(std::max(1, gap - lower - 1)))));
                    w30 = rb.below(5);
                    w58 = rb.below(6);
                    break;
                }
                case 11:
                    idle = between(21, 255);
                    b75 = rb.below(20);
                    b76 = clamp8(std::int32_t(b75) + 6 + std::int32_t(rb.below(20)));
                    b3a = clamp8(std::int32_t(b75) + 1 + std::int32_t(rb.below(20)));
                    w30 = rb.below(7);
                    w58 = rb.below(10);
                    put(2, rows + 8, rb.pick<std::uint32_t>({100, 4000, 4274, 4275, 5000, 0x8000}));
                    put(4, club0 + 0x14, rb.chance(85) ? 0xffffffffu : 7);
                    item.answers.push_back({0x100d94d0, rb.below(2)});
                    break;
                default:
                    lvl = std::int32_t(between(5, 30));
                    c3 = rb.below(5);
                    idle = between(15, 22);
                    b75 = rb.below(5);
                    b76 = std::max(clamp8(lvl - std::int32_t(c3) + std::int32_t(rb.below(10)) - 2), b75 + 1);
                    b3a = rb.below(std::uint32_t(std::max(1, lvl - std::int32_t(c3))));
                    w30 = rb.below(7);
                    w58 = rb.below(13);
                    break;
                }
                if (rb.chance(12)) b38 = 1;
                // Now and then one number is thrown over a wide range, so that a stanza's tests fail at each point.
                if (rb.chance(25)) {
                    std::uint32_t* fields[] = {&idle, &b75, &b76, &b3a, &b38, &b39, &w1c, &w44, &w30, &w58};
                    *fields[rb.below(10)] = rb.below(256);
                }
                put(1, staff + 4, idle);
                put(1, staff + 0x75, b75);
                put(1, staff + 0x76, b76);
                put(1, staff + 0x77, 0);
                put(1, staff + 0x78, b75 + 1 + rb.below(40));
                put(4, staff + 0x1c, w1c);
                put(4, staff + 0x44, w44);
                put(4, staff + 0x30, w30);
                put(4, staff + 0x58, w58);
                put(1, staff + 0x6c, rb.below(20));
                put(1, staff + 0x6d, rb.below(20));
                put(4, record + 0x1c, w);
                put(1, record + 0x3f, std::uint32_t(lvl) & 0xff);  // r13 = the low byte of the halfword at +0x3e
                put(1, record + 0xc3, c3);
                put(1, record + 0xc0, c0);
                put(1, record + 0xc1, c1);
                put(1, rows + 2, rb.below(0x80));
                put(2, league + 0x18, between(1, 60));
                if (aim != 8) put(2, club0 + 0x1ec, rb.pick<std::uint32_t>({100, 1000, 3000, 6000, 7000, 9000}));
                if (region) {
                    // The stanzas' own first tests: the region (+0x2c) or its byte at +0x120 missing now and then.
                    put(1, region + 0x120, rb.chance(8) ? 0 : 1);
                    put(4, club0 + 0x2c, rb.chance(8) ? 0 : region);
                }
                std::erase_if(item.pokes, [](const Case::Poke& p) { return p.entry == 0x10718130 || p.entry == 0x100c7d00 || p.entry == 0x10780fc0; });
                for (const std::uint32_t entry : {0x10718130u, 0x100c7d00u, 0x10780fc0u})
                    item.pokes.push_back({entry, {{1, frame + 0x38, b38}, {1, frame + 0x39, b39}, {1, frame + 0x3a, b3a}}});
            }
            if (n >= 6000 && n % 4 == 0) {
                // Part D: the row's value, target and flag halfword steered over the ladder's bands and the final stage.
                Rng rd{0x6430ULL + n};
                const auto value = [&] { return rd.pick<std::uint32_t>({0, 100, 3000, 0xdac, 4000, 0x10b3, 5000, 0x1964, 6500, 0x251c, 9700, 10000}); };
                put(2, rows + 6, value());
                put(2, rows + 8, value());
                put(2, rows + 0xe, rd.chance(35) ? 8 : rd.below(32));
                if (rd.chance(35)) {
                    const auto high = rd.pick<std::uint32_t>({0x1964, 6500, 0x251c, 9700, 10000});
                    put(2, rows + 6, high);
                    put(2, rows + 8, high);
                }
                put(4, club0 + 0x14, rd.chance(50) ? 0xffffffffu : rd.below(5));
                put(1, rows + 2, rd.below(256));
                put(1, club0 + 0x23d, rd.below(3));
            }
            if (n >= 6000 && n % 3 == 0) {
                // Part E (0x101a7f7c..): the reputation, the manager record's three standings (the record r14 names), the news
                // constants of the floating-point stage, the row's standing byte (+1 of the club's own index row), the club's
                // +0x241 byte and the answers of the switch (0x100f55b0) and of 0x10103510.
                Rng re{0x65ULL * 1000003ULL + n};
                const auto manager_record = manager ? scenario.r32(manager + 0x1c) : 0u;
                put(2, club0 + 0x1ec, re.pick<std::uint32_t>({0x8ad0, 0, 100, 3000, 5000, 5001, 0xdab, 0xdac, 6000, 11500, 11750, 11999, 12000, 20000, 32767}));
                if (manager_record) {
                    const auto standing = [&] { return re.pick<std::uint32_t>({0, 1, 0xffff, 100, 999, 1000, 1001, 3000, 5000, 9999, 32767, 0x8000, 0xb000, 20000, 0x1388, 0x2710}); };
                    for (const std::uint32_t at : {0x28u, 0x2au, 0x2cu}) put(2, manager_record + at, standing());
                }
                const auto news_value = [&] { return bits(re.pick<float>({-1.0e9f, -100.0f, -10.0f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 10.0f, 100.0f, 1.0e9f, std::bit_cast<float>(0x7fc00000u)})); };
                for (const std::uint32_t at : {0x38u, 0x3cu, 0x40u, 0xcu}) if (re.chance(70)) put(4, news + at, news_value());
                put(1, rows + scenario.r32(scenario.club(k)) * 0x4c + 1, re.pick<std::uint32_t>({0x80, 0xfe, 0xff, 0, 1, 2, 5, 0x7f, 0xfd}));
                put(1, club0 + 0x241, re.below(2));
                { const auto byte2 = re.below(256); put(1, rows + 2, re.chance(70) ? byte2 & 0x4fu : byte2); }
                if (re.chance(40)) put(2, rows + 6, re.pick<std::uint32_t>({0, 5, 0x40, 0x4a, 3000, 0x1482, 0x109a}));
                if (re.chance(75)) item.answers.push_back({0x100f55b0, re.pick<std::uint32_t>({0xfffffffdu, 0xfffffffeu, 0xffffffffu, 0, 1, 2, 3, 0x1ff, 0x180, 0x7f})});
                if (re.chance(80)) item.answers.push_back({0x10103510, re.chance(15) ? 0u : 1u});
            }
            if (n >= 6000 && n % 7 == 3) {
                // Null outcomes of the repeated tests: club 0's league (+0x30), manager (+0x90) or the league record's table slot
                // zeroed when a callee the path dispatches after the first test runs (stats 0x10462870 before the staff rules, the
                // power call 0x10780fc0 before the last stanzas, the module draw 0x10195e10 inside the stanzas).
                const auto variant = (n / 7) % 9;
                const std::uint32_t entries[] = {0x10462870, 0x10462870, 0x10780fc0, 0x10780fc0, 0x10195e10, 0x10195e10, 0x10195e10, 0x100c7d00, 0x106891b0};
                const std::uint32_t targets[] = {club0 + 0x30, club0 + 0x90, club0 + 0x30, club0 + 0x90,
                                                 table + 4 * scenario.r32(league), club0 + 0x30, club0 + 0x90,
                                                 club0 + 0x90, table + 4 * scenario.r32(league)};
                item.label += " N" + std::to_string(variant);
                item.pokes.push_back({entries[variant], {{4, targets[variant], 0}}});
            }
            cases.push_back(std::move(item));
        }
    }
}
}

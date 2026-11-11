// MANAGER_MANAGER.CPP checker cases for 0x101c1e20 (the verdict) and 0x101c3220 (the assessment that calls it and
// 0x101c3860), manager_manager_assess.hpp.
//
// No capture reaches them. A scene of scratch records carries each call: the list object and its row of the club,
// the club, its tier record, board, owner and manager, the person, the employer. The real date routine (0x10718130)
// scores day differences within one year, so each score a rule reads is chosen by the days written; the real
// date, last-date and tier-table cells take the values a case writes (undone afterwards). The employer, the relation
// (0x100f55b0), the money adjustment (0x100f5770), the owner's rating writer, the draw's redraw, the news desk and
// the assertion routines are answered; the message buffer's routines and the list append run for real in some cases.
// A generator with a fixed seed draws each field from the values at and around its cuts (the cuts that move with the
// club's mood are computed from the scene), then single changes from a passing base of each kind steer the outcomes
// the random draws reach rarely. Draws of the module's table come from halfwords written after the cursor, and the wrap
// of the table (which calls the redraw) is chosen per case.
#include "manager_manager_checks.hpp"
#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t verdict_entry = 0x101c1e20, assess_entry = 0x101c3220;
constexpr std::uint32_t list = scratch_base + 0x100, rows = scratch_base + 0x10000, person = scratch_base + 0x200,
                        person_profile = scratch_base + 0x300, club = scratch_base + 0x1000, tier_record = scratch_base + 0x2000,
                        board = scratch_base + 0x2200, owner = scratch_base + 0x2400, owner_profile = scratch_base + 0x2600,
                        manager = scratch_base + 0x2800, manager_profile = scratch_base + 0x2a00,
                        employer = scratch_base + 0x3000, record = scratch_base + 0x3200, league = scratch_base + 0x3400,
                        appended = scratch_base + 0x3600, person_refs = scratch_base + 0x4000;
constexpr std::uint32_t club_index = 3;
namespace routine {
constexpr std::uint32_t employer_record = 0x10103510, relation = 0x100f55b0, adjust = 0x100f5770, writer = 0x100f4f20,
                        redraw = 0x1077b770, news_desk = 0x102cc140, append = 0x10738d10, decision = 0x100d9b80;
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

struct Scene {
    bool assess = false;           // 0x101c3220 (otherwise 0x101c1e20)
    std::uint32_t kind = 1;        // the record's byte at +8 / the assessment's byte argument (r6)
    std::uint32_t kind_junk = 0;   // high bits of r6
    std::int32_t since = 100, away = 100, last = 200;  // the three day scores (today minus the other day)
    std::int32_t person_value = 0x1800, mgr_value = 0x1800, rep = 0x1500, rated = 0x1500, size_class = 0x1500;
    std::int32_t tier_class = 5, kind_byte = 0, standing_byte = 3, table_byte = 0, budget = 3000;
    unsigned owner_mode = 2;       // 0 none, 1 without profile, 2 with
    std::int32_t level = 3, trait = 3, temper = 3;
    bool employer_found = true, record_given = true, club_given = true, person_given = true;
    std::uint32_t emp_hi = 0, emp_lo = 1000000, emp14 = 0, amount = 0, spare_byte = 0, relation = 0;
    std::uint64_t earn_high = 0, earn_low = 0;
    std::uint32_t x[6] = {0, 1, 2, 3, 4, 5};
    bool wrap = false;
    std::uint32_t redraw_value = 0;
    // 0x101c3220 only
    bool has_league = true, has_board = true, has_profile = true, owner_link = true;
    bool real_adjust = false, real_append = false, real_report = false;
    bool poke_profile = false;  // 0x101c3220: the person's profile pointer is cleared by the third news-desk call
};

std::int16_t h(std::int64_t v) { return std::int16_t(std::uint16_t(v)); }

// ---- the models the generator steers by (the routine's own arithmetic, restated) ----
std::int32_t mood_of(const Scene& s) {
    std::int32_t a = 0xfa;
    if (s.away < 0x1e) a = -500; else if (s.away < 0xb4) a = -0xfa;
    if (s.since < 0xf) a += 0x2ee; else if (s.since < 0x1e) a += 500; else if (s.since < 0xb4) a -= 0xfa;
    if (s.owner_mode == 2) {
        if (std::int8_t(s.level) == 1) a -= 500; else if (std::int8_t(s.level) < 6) a -= 0xfa; else if (std::int8_t(s.level) > 0xf) a += 0xfa;
    }
    return a;
}
std::uint32_t base_for(std::int32_t rep) {
    return rep > 0x1d4c ? std::uint32_t((rep - 0x1d4c) * 0x1388 + 12500000)
         : rep > 0x1482 ? std::uint32_t((rep - 0x1482) * 0x3e8 + 3000000)
                        : std::uint32_t((rep - 0x1388) * 100 + 50000);
}
std::uint32_t asked_for(std::int32_t rep) {
    return rep > 0x1d4c ? std::uint32_t((rep - 0x1d4c) * 0x1388 + 5000000)
         : rep > 0x109a ? std::uint32_t((rep - 0x109a) * 0x3e8 + 2000000)
                        : std::uint32_t(rep * 100 + 50000);
}
std::int32_t limit_one(const Scene& s, double scale) {
    const std::int8_t tier = std::int8_t(s.tier_class);
    std::int32_t v = tier > 0x12 ? s.rep * 7 + 0x1388 : tier > 0xf ? s.rep * 5 + 0x9c4 : tier > 0xb ? s.rep * 3 + 0x2710
                   : tier > 8 ? s.rep * 2 + 0x9c4 : s.rep * 2 + 0x4e2;
    if (std::uint8_t(s.kind_byte) == 3) v = std::int32_t(std::int64_t(scale * double(v)));
    return v;
}
std::int32_t limit_two(const Scene& s, double scale) {
    const std::int8_t tier = std::int8_t(s.tier_class);
    std::int32_t v = tier > 0xe ? (s.rep > 0x186a ? 0x14 : s.rep > 0x109a ? 0xf : s.rep > 0xcb2 ? 0xa : 7)
                   : tier > 8 ? (s.rep > 0x186a ? 0xf : s.rep > 0x109a ? 0xa : 4)
                              : (s.rep > 0x186a ? 0xa : s.rep > 0x109a ? 7 : 2);
    if (std::uint8_t(s.kind_byte) == 3) v = std::int32_t(std::int64_t(scale * double(v)));
    return h(v);
}

struct Pools {
    double scale_a, scale_b, scale_c, scale_d;  // pool +0, +0x10, +0x20, +0x28
};

// A value at or near a cut, with the club's mood moved in or out.
std::int32_t near_cut(Rng& r, std::initializer_list<std::int32_t> cuts, std::int32_t mood) {
    const auto cut = r.pick(cuts);
    const std::int32_t shift = r.pick<std::int32_t>({0, 0, mood, -mood});
    return cut + shift + r.pick<std::int32_t>({-1, 0, 1, 0});
}

Scene random_scene(Rng& r, std::uint32_t kind, bool assess, const Pools& p) {
    Scene s;
    s.assess = assess;
    s.kind = kind;
    s.kind_junk = r.chance(30) ? (r.next() << 8) : 0;
    const auto score = [&] { return r.pick<std::int32_t>({-100, 0, 1, 14, 15, 29, 30, 89, 90, 91, 92, 179, 180, 181, 239, 300}); };
    s.since = r.chance(20) ? std::int32_t(r.below(400)) - 50 : score();
    s.away = r.chance(20) ? std::int32_t(r.below(400)) - 50 : score();
    s.last = r.pick<std::int32_t>({0, 100, 178, 179, 180, 181, 300, -20});
    s.owner_mode = r.chance(15) ? 0 : r.chance(15) ? 1 : 2;
    s.level = r.pick<std::int32_t>({0, 1, 2, 5, 6, 7, 15, 16, 17, -1, 127, -128, 3});
    s.trait = r.pick<std::int32_t>({0, 1, 2, 3, 5, 10, -1, -5, 127, -128, 200});
    s.temper = r.pick<std::int32_t>({0, 1, 2, 3, 5, 6, 7, -1, -5, 127, -128, 200});
    const auto mood = mood_of(s);
    s.rep = r.chance(40) ? std::int32_t(r.below(0x3000)) : near_cut(r, {0x109a, 0x109b, 0x1388, 0x1482, 0x1676, 0x186a, 0x1d4c, 0x1d4d, 0xcb2, 0xcb3, 0x1c52}, mood);
    s.person_value = r.chance(20) ? std::int32_t(r.below(0x2400)) - 0x100 : near_cut(r, {0x109a, 0x1388, 0x1482, 0x1676, 0x1c52, 0x1964, 0x1a5e, s.rep + 0x2ee, s.rep - 0x2ee, s.rep - 0x5dc, s.rep - 0x4e2}, mood);
    s.mgr_value = r.chance(20) ? std::int32_t(r.below(0x2400)) - 0x100 : near_cut(r, {0x1388, 0x1676, s.rep - 0x5dc, s.rep - 0x4e2, s.rep - 0x2ee, 0x1c52}, mood);
    s.rated = r.chance(25) ? std::int32_t(r.below(0x2400)) - 0x100 : near_cut(r, {0x109a, 0x1c52, 0x1b58}, mood);
    s.size_class = r.pick<std::int32_t>({0x1482, 0x1483, 0x186a, 0x186b, 0x1000, 0x2000, -5, 0x1481});
    s.tier_class = r.pick<std::int32_t>({-128, -1, 0, 4, 8, 9, 11, 12, 14, 15, 16, 18, 19, 127});
    s.kind_byte = r.chance(50) ? 3 : r.pick<std::int32_t>({0, 1, 2, 4, 0x103});
    s.standing_byte = r.pick<std::int32_t>({-128, -1, 0, 1, 2, 3, 4, 7, 8, 9, 10, 127});
    s.table_byte = r.pick<std::int32_t>({-128, -2, -1, 0, 1, 2, 3, 4, 5, 6, 127});
    s.emp_hi = r.pick<std::uint32_t>({0, 0, 0xffffffffu, 1, 0x80000000u, 0x7fffffffu});
    s.emp_lo = r.pick<std::uint32_t>({0, 1000000, 5000000, 0xffffffffu, 0x80000000u, 2000000, 100000, 50000});
    s.emp14 = r.pick<std::uint32_t>({0, 0x15f90, 0x15f91, 0x15f8f, 0xffffffffu, 0x80000000u, 0x7fffffffu});
    s.amount = r.chance(70) ? 0 : r.pick<std::uint32_t>({1, 5000, 0x80000000u});
    s.spare_byte = r.pick<std::uint32_t>({0, 0, 1, 5, 0xff, 0x80});
    s.relation = r.pick<std::uint32_t>({0, 1, 2, 3, 0xff, 0xfe, 0xfd, 0xfc, 0x80, 0x7f, 0x100, 0xabcdff01u, 0xabcdfffeu});
    const std::uint32_t base = base_for(s.rep);
    const auto want = [&](std::int32_t tier) {
        const double scale = tier > 4 ? p.scale_d : tier < 0 ? p.scale_a : p.scale_b;
        return std::int32_t(std::int64_t(scale * double(std::int32_t(base))));
    };
    const std::int32_t w = want(std::int8_t(s.table_byte));
    const auto double_near = [&](std::int64_t centre) -> std::uint64_t {
        const std::int64_t at = centre + r.pick<std::int64_t>({-1, 0, 1, -1000, 1000});
        return std::bit_cast<std::uint64_t>(double(at));
    };
    const auto earn = [&]() -> std::uint64_t {
        switch (r.below(8)) {
            case 0: return 0;
            case 1: return nan_bits;
            case 2: return std::bit_cast<std::uint64_t>(1e30);
            case 3: return std::bit_cast<std::uint64_t>(-1e30);
            case 4: return std::bit_cast<std::uint64_t>(double(r.below(30000000)));
            default: return double_near(w);
        }
    };
    s.earn_low = earn();
    s.earn_high = r.chance(30) ? earn() : std::bit_cast<std::uint64_t>(double(w) + r.pick<std::int64_t>({-5, 0, 5, -1000000, 1000000}));
    if (r.chance(10)) s.earn_high = s.earn_low;
    const std::uint32_t ask = asked_for(s.rep);
    if (r.chance(60)) {
        const std::int64_t centre = std::int64_t(std::int32_t(ask)) + r.pick<std::int64_t>({-1, 0, 1});
        s.emp_hi = std::uint32_t(centre >> 32);
        s.emp_lo = std::uint32_t(centre);
    }
    s.budget = r.chance(30) ? std::int32_t(r.next()) : limit_one(s, p.scale_c) + r.pick<std::int32_t>({-1, 0, 1, -1, 1});
    if (kind == 2 && r.chance(60)) s.standing_byte = limit_two(s, p.scale_c) + r.pick<std::int32_t>({-1, 0, 1});
    for (auto& v : s.x) v = r.chance(40) ? r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 9, 10, 0x31, 0x32, 0x1481, 0x1482, 0x1d4b, 0x1d4c, 0xffff}) : r.below(0x10000);
    s.wrap = r.chance(15);
    s.redraw_value = r.chance(50) ? r.below(0x20000) : r.below(0x40);
    s.has_league = !r.chance(5);
    s.has_board = !r.chance(5);
    s.has_profile = !r.chance(5);
    s.owner_link = !r.chance(10);
    if (r.chance(5)) s.employer_found = false;
    if (assess && r.chance(10)) s.person_value = r.pick<std::int32_t>({0x1a5e, 0x1a5f, 0x1a5d, 0x1000, 0x7fff, -0x8000});
    return s;
}

Case make(const Scenario& sc, const Scene& s, std::string label) {
    Case item{.entry = s.assess ? assess_entry : verdict_entry, .label = "assess verdict " + std::move(label)};
    if (s.assess)
        item.registers = {{3, list}, {4, s.person_given ? person : 0}, {5, s.club_given ? club : 0}, {6, s.kind | s.kind_junk}};
    else
        item.registers = {{3, list}, {4, s.record_given ? record : 0}, {5, s.club_given ? club : 0}, {6, s.person_given ? person : 0}};
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto put_double = [&](std::uint32_t at, std::uint64_t bits) { put(4, at, std::uint32_t(bits >> 32)); put(4, at + 4, std::uint32_t(bits)); };
    constexpr std::int32_t today = 300;
    const auto date = sc.toc(-0x7b80);
    put(2, date, today);
    put(2, date + 2, 2001);
    put(1, date + 4, 0);
    const auto last_date = sc.toc(-0x7a9c);
    put(2, last_date, std::uint32_t(today - s.last) & 0xffff);
    put(2, last_date + 2, 2001);
    put(1, last_date + 4, 0);
    // the list object and the club's row
    put(4, list, rows);
    put(4, list + 4, appended);
    const auto row = rows + club_index * 0x4c;
    put(1, row + 2, 0x10);
    put(1, row + 5, s.spare_byte);
    put(2, row + 6, std::uint32_t(s.rated) & 0xffff);
    put(2, row + 8, std::uint32_t(s.size_class) & 0xffff);
    put(4, row + 0x28, s.amount);
    put_double(row + 0x30, s.earn_high);
    put_double(row + 0x38, s.earn_low);
    put(2, row + 0x44, std::uint32_t(today - s.since) & 0xffff);
    put(2, row + 0x46, 2001);
    put(1, row + 0x48, 0);
    // the person
    put(4, person, 0x1234);
    for (unsigned n = 0; n < 3; ++n) {  // the three records the message names (their ids at +0x20)
        put(4, person + 0x28 + 4 * n, person_refs + 0x100 * n);
        put(4, person_refs + 0x100 * n + 0x20, 0xa0 + n);
    }
    put(4, person + 0x1c, s.has_profile ? person_profile : 0);
    put(2, person_profile + 0x2a, std::uint32_t(s.person_value) & 0xffff);
    put(2, person + 0x4c, std::uint32_t(today - s.away) & 0xffff);
    put(2, person + 0x4e, 2001);
    put(1, person + 0x50, 0);
    // the club
    put(4, club, club_index);
    put(4, club + 0x2c, tier_record);
    put(1, tier_record + 0x11f, std::uint32_t(s.tier_class) & 0xff);
    put(4, club + 0x30, s.has_league ? league : 0);
    put(4, club + 0x3c, s.has_board ? board : 0);
    put(4, board + 4, std::uint32_t(s.budget));
    put(4, club + 0x80, s.owner_mode == 0 ? 0 : owner);
    put(4, owner + 0x18, s.owner_link ? 0x1000 : 0);
    put(4, owner + 0x1c, s.owner_mode == 2 ? owner_profile : 0);
    put(1, owner + 0x63, std::uint32_t(s.temper) & 0xff);
    put(1, owner_profile + 0x2f, std::uint32_t(s.trait) & 0xff);
    put(1, owner_profile + 0x3d, std::uint32_t(s.level) & 0xff);
    put(4, club + 0x90, manager);
    put(4, manager + 0x1c, manager_profile);
    put(2, manager_profile + 0x2a, std::uint32_t(s.mgr_value) & 0xffff);
    put(2, club + 0x1ec, std::uint32_t(s.rep) & 0xffff);
    put(1, club + 0x23d, std::uint32_t(s.kind_byte) & 0xff);
    put(1, club + 0x240, std::uint32_t(s.standing_byte) & 0xff);
    put(1, sc.global(-0x7984) + club_index * 8 + 4, std::uint32_t(s.table_byte) & 0xff);
    // the employer
    put(4, employer, s.emp_hi);
    put(4, employer + 4, s.emp_lo);
    put(4, employer + 0x14, s.emp14);
    // the record the verdict reads and writes
    put(1, record + 8, s.kind & 0xff);
    put(1, record + 9, 0);
    // the draw table
    const auto first = sc.toc(-0x7b8c), last = sc.r32(sc.toc(-0x7b88));
    put(4, sc.toc(-0x7b84), s.wrap ? last : first);
    put(4, sc.toc(-0x7b90), 0);
    for (unsigned n = 0; n < 6; ++n) put(2, first + 2 * (n + (s.wrap ? 0 : 1)), s.x[n]);
    item.answers = {{routine::employer_record, s.employer_found ? employer : 0}, {routine::relation, s.relation},
                    {routine::writer, 0}, {routine::redraw, s.redraw_value}, {routine::news_desk, 0}};
    if (!s.real_adjust) item.answers.push_back({routine::adjust, 0});
    if (!s.real_report)
        for (const std::uint32_t report : {0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u}) item.answers.push_back({report, 0});
    if (s.assess) {
        if (!s.real_append) item.answers.push_back({routine::append, 0});
        item.answers.push_back({routine::decision, 0});
    }
    if (s.poke_profile) item.pokes.push_back({routine::news_desk, {{4, person + 0x1c, 0}}});
    return item;
}

MANAGER_MANAGER_CASES("assess_verdict") {
    if (scenario.match) return;
    const auto pool = scenario.toc(-0x536c);
    const auto d = [&](std::uint32_t at) { return std::bit_cast<double>((std::uint64_t(scenario.r32(pool + at)) << 32) | scenario.r32(pool + at + 4)); };
    const Pools pools{d(0), d(0x10), d(0x20), d(0x28)};
    Rng r{0x5eed1e20};
    unsigned n = 0;
    const auto label = [&](const std::string& what) { return what + " " + std::to_string(n++); };
    for (const std::uint32_t kind : {1u, 2u, 3u, 0u, 4u, 5u, 0x7fu, 0x80u, 0xffu, 0x103u})
        for (unsigned i = 0; i < 700; ++i)
            cases.push_back(make(scenario, random_scene(r, kind, false, pools), label("random verdict kind " + std::to_string(kind))));
    for (const std::uint32_t kind : {1u, 2u, 3u, 0u, 4u, 5u})
        for (unsigned i = 0; i < 500; ++i)
            cases.push_back(make(scenario, random_scene(r, kind, true, pools), label("random assessment kind " + std::to_string(kind))));
    // The assertions: a null record, club or person (0x101c1e20), answered and run for real.
    for (const bool real : {false, true}) {
        Scene s = random_scene(r, 1, false, pools);
        s.real_report = real;
        Scene a = s; a.record_given = false;
        cases.push_back(make(scenario, a, label("verdict null record")));
        Scene b = s; b.club_given = false;
        cases.push_back(make(scenario, b, label("verdict null club")));
        Scene c = s; c.person_given = false;
        cases.push_back(make(scenario, c, label("verdict null person")));
        for (const std::uint32_t kind : {1u, 2u, 3u, 0u}) {
            Scene t = random_scene(r, kind, true, pools);
            t.real_report = real;
            Scene d = t; d.person_given = false;
            cases.push_back(make(scenario, d, label("assessment null person")));
            Scene e = t; e.club_given = false;
            cases.push_back(make(scenario, e, label("assessment null club")));
        }
    }
    // The rating clamp after a refusal by the club's board: ratings far above the cap and wrapping below it.
    for (const std::int32_t rated : {0x7fff, 0x2710 + 0x4e2, 0x2710 + 0x4e3, 0x2710 + 500, 0x2710 + 501, 0x4e2, 0x4e3, 0x4e1, 501, 500, 499, 1, 0, -1, -0x8000, -0x7fff + 0x4e1, -0x8000 + 0x4e2, 0x2710, 0x2711, 0x2712})
        for (const std::uint32_t kind : {1u, 2u, 3u, 0u, 4u}) {
            for (const std::uint32_t verdict_kind : {1u, 2u, 3u, 0u, 4u}) {
                (void)kind;
                Scene s = random_scene(r, verdict_kind, true, pools);
                s.rated = rated;
                cases.push_back(make(scenario, s, label("assessment rating " + std::to_string(rated))));
            }
            break;
        }
    // The person's profile pointer cleared before the post of a refusal.
    for (unsigned i = 0; i < 400; ++i) {
        Scene s = random_scene(r, r.pick<std::uint32_t>({1u, 2u, 3u, 0u, 4u}), true, pools);
        s.poke_profile = true;
        // Not interested (the draw 0x1000 is below the person's value 0x1000 + 0x7d0 cut), so the first news-desk call
        // is the third post, after the verdict.
        s.person_value = 0x1000;
        s.x[0] = 0x1000;
        s.wrap = false;
        cases.push_back(make(scenario, s, label("assessment profile cleared")));
    }
    // The money adjustment and the list append run for real over scratch records (the leftover bytes of the append's
    // 8-byte record are read from the stack).
    for (unsigned i = 0; i < 300; ++i) {
        Scene s = random_scene(r, r.pick<std::uint32_t>({3u, 3u, 2u, 0u}), r.chance(50), pools);
        s.real_adjust = true;
        s.real_append = r.chance(50);
        cases.push_back(make(scenario, s, label("real adjust")));
    }
}
}
}

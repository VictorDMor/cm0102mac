// MANAGER_MANAGER.CPP checker cases for the people text block (manager_manager_people_text.hpp): the text
// chooser 0x101980a0 and its helpers 0x10197e00, 0x10197e80 and 0x10197f00.
//
// No capture reaches these routines, so the inputs are enumerated over the career snapshot's real clubs and
// module object. The band helpers read halfwords of the club's row: each gets values at and around every
// cut. The third helper walks a league record chain that a case builds in the scratch area (the club's +0x30
// points into it; the record is answered for 0x10459b50), with a count and a date near or far (two years either side) for each of
// the four entries. The chooser takes every relation answer (-3..3, and a value whose low byte is 2) for
// 0x100f55b0, every first band (and second, on the long days) from the row's ratings, and every value of the
// byte at sp+0x39 / sp+0x38 (written when 0x10103510 is dispatched, after the helper has written its own),
// each on the three day bands and at the edges of the bands. The tail's tests (date, ratings) and the
// flagged club's texts are enumerated separately. Memory writes are undone after each case.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t first_band = 0x10197e00, second_band = 0x10197e80, upcoming = 0x10197f00, chooser = 0x101980a0;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50, leagues_slot = -0x7b5c, today_slot = -0x7b80;
constexpr std::uint32_t club_flag = 0x100c7d00, level_of = 0x100f55b0, employer_record = 0x10103510, league_entry = 0x10459b50;
constexpr std::uint32_t league_area = scratch_base + 0x2000, kind_area = scratch_base + 0x3000, buffer_area = scratch_base + 0x4000;

struct World {
    const Scenario& sc;
    std::uint32_t object, clubs;
    std::uint32_t row(std::uint32_t club) const { return sc.r32(object) + sc.r32(club) * 0x4c; }
};
struct Draft {
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
};
// The halfwords of a club's row (+6, +0x12, +0x16) and its dates at +0x44 / +0x48 (the chooser's tail).
constexpr std::uint32_t first_rating = 6, second_rating = 0x12, third_rating = 0x16, row_date = 0x44;

// The league chain of the third helper: club +0x30 -> record (index word) -> table (TOC -0x7b5c) -> record
// (+8 = a record) -> 0x10459b50, answered with the entries record. `break_at` cuts it: 1 no league, 2 no table
// entry, 3 no record at +8, 4 the answer 0.
struct Chain {
    std::uint32_t counts[4] = {0, 0, 0, 0};
    bool near[4] = {false, false, false, false};
    unsigned break_at = 0;
};
void build_chain(const World& w, Draft& d, std::uint32_t club, const Chain& chain) {
    const auto table = w.sc.r32(w.sc.toc(leagues_slot));
    const auto league = league_area, slot = league_area + 0x10, record = league_area + 0x20, entries = league_area + 0x100;
    d.put(4, club + 0x30, chain.break_at == 1 ? 0 : league);
    d.put(4, league, (slot - table) / 4);
    d.put(4, slot, chain.break_at == 2 ? 0 : record);
    d.put(4, record + 8, chain.break_at == 3 ? 0 : league_area + 0x30);
    d.item.answers.push_back({league_entry, chain.break_at == 4 ? 0 : entries});
    // The entries: where the count is, then the date's two words.
    constexpr std::uint32_t count_at[4] = {0x1c, 0x30, 0x58, 0x44}, date_at[4] = {0x28, 0x3c, 0x64, 0x50};
    const auto today = w.sc.toc(today_slot);
    for (unsigned n = 0; n < 4; ++n) {
        d.put(4, entries + count_at[n], chain.counts[n]);
        d.put(4, entries + date_at[n], w.sc.r32(today) + (chain.near[n] ? 0 : 0x100000));
        d.put(4, entries + date_at[n] + 4, w.sc.r32(today + 4) + (chain.near[n] ? 0 : 0x100000));
    }
}

std::string number(std::int64_t v) { return std::to_string(v); }

// Clubs the cases draw from: spread over the table, one per row (nation) the table has.
std::vector<std::uint32_t> pick_clubs(const World& w) {
    std::vector<std::uint32_t> picked;
    std::vector<std::uint32_t> rows;
    for (unsigned n = 0; n < w.clubs && picked.size() < 3; ++n) {
        const auto club = w.sc.club(n), row = w.row(club);
        bool fresh = true;
        for (const auto r : rows) fresh = fresh && r != row;
        if (!fresh) continue;
        rows.push_back(row);
        picked.push_back(club);
    }
    return picked;
}
}  // namespace

MANAGER_MANAGER_CASES("people text") {
    if (scenario.match || !scenario.global(club_total) || !scenario.toc(leagues_slot)) return;
    World w{scenario, scenario.global(manager_object), scenario.global(club_total)};
    const auto clubs = pick_clubs(w);
    if (clubs.empty()) return;
    const auto club = clubs[0];

    // The band helpers: values of the row's halfword at and around every cut, on each club.
    constexpr std::int32_t first_values[] = {0, 0xbb7, 0xbb8, 0xdab, 0xdac, 0x10b2, 0x10b3, 0x1963, 0x1964, 0x2133, 0x2134,
                                             0x251b, 0x251c, 0x7fff, 0xffff, 0x8000, -1};
    constexpr std::int32_t second_values[] = {-0x7d1, -0x7d0, -0x3e9, -0x3e8, -0x1f5, -0x1f4, 0x1f3, 0x1f4, 0x3e7, 0x3e8,
                                              0x7cf, 0x7d0, 0x7fff, 0x8000, 0xffff, 0, -0x8000};
    for (const auto c : clubs) {
        for (const auto value : first_values) {
            Draft d;
            d.item = {.entry = first_band, .registers = {{3, w.object}, {4, c}}, .label = "first band, rating " + number(value)};
            d.put(2, w.row(c) + first_rating, std::uint32_t(value));
            cases.push_back(d.item);
        }
        for (const auto value : second_values) {
            Draft d;
            d.item = {.entry = second_band, .registers = {{3, w.object}, {4, c}}, .label = "second band, rating " + number(value)};
            d.put(2, w.row(c) + second_rating, std::uint32_t(value));
            cases.push_back(d.item);
        }
        cases.push_back({.entry = first_band, .registers = {{3, w.object}, {4, c}}, .label = "first band, the row as it is"});
        cases.push_back({.entry = second_band, .registers = {{3, w.object}, {4, c}}, .label = "second band, the row as it is"});
    }

    // The third helper: every chain cut, then every choice of counts above/below 2 and dates near/far.
    const auto upcoming_case = [&](std::uint32_t c, const Chain& chain, const std::string& label) {
        Draft d;
        d.item = {.entry = upcoming, .registers = {{3, w.object}, {4, c}, {5, kind_area + 1}, {6, kind_area}}, .label = label};
        build_chain(w, d, c, chain);
        // The out bytes start as something else, so a missed write shows.
        d.put(1, kind_area, 0x77);
        d.put(1, kind_area + 1, 0x66);
        cases.push_back(d.item);
    };
    for (unsigned cut = 0; cut <= 4; ++cut)
        upcoming_case(club, {{3, 3, 3, 3}, {true, true, true, true}, cut}, "upcoming, chain cut " + number(cut));
    for (const auto c : clubs) cases.push_back({.entry = upcoming, .registers = {{3, w.object}, {4, c}, {5, kind_area + 1}, {6, kind_area}},
                                                 .label = "upcoming, the club's own league"});
    constexpr std::uint32_t count_values[] = {0, 1, 2, 0x80, 0xffffffffu};
    for (unsigned mask = 0; mask < 16; ++mask)
        for (unsigned near = 0; near < 16; ++near)
            for (const auto count : count_values) {
                Chain chain;
                for (unsigned n = 0; n < 4; ++n) {
                    chain.counts[n] = (mask >> n & 1) ? (count > 1 && count < 0x80 ? count : 3) : count;
                    chain.near[n] = near >> n & 1;
                }
                if (count != count_values[2] && mask != 0 && mask != 15) continue;
                upcoming_case(club, chain, "upcoming, counts " + number(mask) + "/" + number(count) + " near " + number(near));
            }

    // The chooser. The answers of the other modules' routines, the row's ratings and the bytes at
    // sp+0x38 / sp+0x39 are the inputs.
    const auto frame = std::uint32_t(scenario.s.r<4>(4) - 0xa90);
    constexpr std::int32_t relations[] = {-3, -2, -1, 0, 1, 2, 3, 0x102, 0x80};
    constexpr std::int32_t first_bands[] = {0x800, 0xc00, 0x1000, 0x1500, 0x2000, 0x2400, 0x3000};
    constexpr std::int32_t second_bands[] = {-0x800, -0x500, -0x300, 0, 0x300, 0x500, 0x900};
    constexpr std::int32_t kinds[] = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 0x7f, 0x80};
    constexpr std::uint32_t no_nation = 0xffffffffu;  // writes 0 over the club's nation record
    constexpr std::uint32_t days_short = 0x1d, days_middle = 0x3b, days_long = 0x64;
    struct Input {
        std::uint32_t days;
        std::uint32_t club;
        std::int32_t relation, first, second, kind, count;
        bool flagged = false;
        std::uint32_t third = 0x100, date_far = 0, nation = 0;
        std::int32_t extra_rating = INT32_MIN;  // the first rating of the tail's tests (replaces `first`)
    };
    const auto chooser_case = [&](const Input& in, const std::string& label) {
        Draft d;
        d.item = {.entry = chooser, .registers = {{3, w.object}, {4, in.club}, {5, in.days}, {6, buffer_area}, {7, 0x1388}}, .label = label};
        const auto row = w.row(in.club);
        d.put(2, row + first_rating, std::uint32_t(in.extra_rating != INT32_MIN ? in.extra_rating : in.first));
        d.put(2, row + second_rating, std::uint32_t(in.second));
        d.put(2, row + third_rating, in.third);
        d.put(4, row + row_date, w.sc.r32(w.sc.toc(today_slot)) + in.date_far);  // +2: two years on
        d.put(4, row + row_date + 4, w.sc.r32(w.sc.toc(today_slot) + 4));
        if (in.nation) d.put(4, in.club + 0x2c, in.nation == no_nation ? 0 : in.nation);
        d.put(1, buffer_area, 0x55);
        d.item.answers.push_back({level_of, std::uint32_t(in.relation)});
        d.item.answers.push_back({club_flag, in.flagged ? 1u : 0u});
        // After the helper has written its two bytes, before the text reads them.
        d.item.pokes.push_back({employer_record, {{1, frame + 0x39, std::uint32_t(in.kind)}, {1, frame + 0x38, std::uint32_t(in.count)}}});
        cases.push_back(d.item);
    };
    unsigned serial = 0;
    for (const auto relation : relations) {
        // Days under 30 (the text depends on the relation only), and the edges of the day bands.
        for (const auto days : {days_short, 0u, 0x1du, 0x80000000u, 0xffffffffu})
            chooser_case({days, club, relation, 0x1000, 0, 1, 5}, "short days " + number(days) + " relation " + number(relation));
        for (const auto days : {0x1eu, days_middle, 0x3cu, 0x7fffffffu, days_long, 0x3du}) {
            chooser_case({days, club, relation, 0x1000, 0x300, 3, 5}, "edge days " + number(days) + " relation " + number(relation));
            if (days == 0x3c) chooser_case({days, club, relation, 0x1500, 0x500, 2, 5}, "days 60 relation " + number(relation));
        }
        for (const auto first : first_bands)
            for (const auto kind : kinds) {
                const auto count = std::int32_t(kind * 3 + 1);
                chooser_case({days_middle, club, relation, first, 0, kind, count},
                             "middle days, relation " + number(relation) + " first " + number(first) + " kind " + number(kind));
                for (const auto second : second_bands) {
                    ++serial;
                    chooser_case({days_long, clubs[serial % clubs.size()], relation, first, second, kind, count},
                                 "long days, relation " + number(relation) + " first " + number(first) + " second " + number(second) +
                                     " kind " + number(kind));
                }
            }
    }

    // The tail (long days, and the flagged club): the date's distance (near/far), the first rating at and
    // around the tail's cuts, and the third rating at and around its cuts.
    constexpr std::int32_t tail_first[] = {0xdab, 0xdac, 0xdad, 0xea5, 0xea6, 0x10b2, 0x10b3, 0x10b4, 0x157b, 0x157c, 0x1963, 0x1964, 0x2000, 0x7fff, 0, 0x251d};
    constexpr std::uint32_t tail_third[] = {0x5db, 0x5dc, 0x5dd, 0xfffffd11u, 0xfffffd12u, 0xfffffd13u, 0xfffffe0bu, 0xfffffe0cu, 0xfffffe0du, 0, 0x7fff, 0x8000};
    for (const std::uint32_t far : {0u, 2u, 0xfffffffeu})
        for (const auto first : tail_first)
            for (const auto third : tail_third) {
                chooser_case({days_long, club, 2, first, 0x300, 3, 5, false, third, far}, "tail, date far " + number(far) + " first " + number(first) + " third " + number(std::int16_t(third)));
                chooser_case({days_long, club, 2, first, 0x300, 3, 5, true, third, far}, "flagged, date far " + number(far) + " first " + number(first) + " third " + number(std::int16_t(third)));
            }
    // A flagged club with and without a nation record (+0x2c: a real club's, whose name the text takes) and
    // at every cut of the first rating. Without one the original formats stack bytes it never wrote.
    std::uint32_t real_nation = 0;
    for (unsigned n = 0; n < 2000 && !real_nation; ++n) real_nation = scenario.r32(scenario.club(n) + 0x2c);
    for (const auto first : first_values)
        for (const auto nation : {no_nation, real_nation})
            chooser_case({days_long, club, 0, first, 0, 5, 0, true, 0x100, 0, nation}, "flagged, first " + number(first) + " nation " + number(nation == no_nation ? 0 : 1));

    // The extra text already holding something when the tail tests it (its first byte is written when the
    // tail's own date call is dispatched): a state the original never reaches, compared all the same.
    for (const auto first : {0x1000, 0x2000})
        for (const auto rest_byte : {0x41u, 0x80u}) {
            Draft d;
            d.item = {.entry = chooser, .registers = {{3, w.object}, {4, club}, {5, days_long}, {6, buffer_area}, {7, 0x1388}},
                      .label = "extra text present, first " + number(first) + " byte " + number(rest_byte)};
            d.put(2, w.row(club) + first_rating, std::uint32_t(first));
            d.put(2, w.row(club) + second_rating, 0x300);
            d.item.answers.push_back({level_of, 2});
            d.item.answers.push_back({club_flag, 0});
            d.item.pokes.push_back({employer_record, {{1, frame + 0x39, 3}, {1, frame + 0x38, 5}}});
            d.item.pokes.push_back({0x10718130, {{1, frame + 0xa0, rest_byte}}});
            cases.push_back(d.item);
        }

    // The chain through the helper itself, on the three day bands: the byte at sp+0x39 is what the helper wrote.
    for (const auto days : {days_short, days_middle, days_long})
        for (unsigned entry = 0; entry < 4; ++entry) {
            Draft d;
            Chain chain;
            chain.counts[entry] = 4 + entry;
            chain.near[entry] = true;
            d.item = {.entry = chooser, .registers = {{3, w.object}, {4, club}, {5, days}, {6, buffer_area}, {7, 0x7d0}},
                      .label = "through the helper, days " + number(days) + " entry " + number(entry)};
            build_chain(w, d, club, chain);
            d.put(2, w.row(club) + first_rating, 0x1000);
            d.put(2, w.row(club) + second_rating, 0x300);
            d.item.answers.push_back({level_of, 2});
            d.item.answers.push_back({club_flag, 0});
            cases.push_back(d.item);
        }
    for (const auto relation : {-2, 1, 2}) {
        cases.push_back({.entry = chooser, .registers = {{3, w.object}, {4, club}, {5, 100}, {6, buffer_area}, {7, 0x1388}},
                         .answers = {{level_of, std::uint32_t(relation)}, {club_flag, 0}},
                         .label = "the club as it is, relation " + number(relation)});
    }
    for (const auto c : clubs)
        for (const auto days : {10u, 40u, 90u, 400u})
            cases.push_back({.entry = chooser, .registers = {{3, w.object}, {4, c}, {5, days}, {6, buffer_area}, {7, 0x1388}},
                             .label = "real club, days " + number(days)});
}
}

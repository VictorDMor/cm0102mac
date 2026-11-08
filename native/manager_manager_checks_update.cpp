// MANAGER_MANAGER.CPP checker cases for the post-match manager update block
// (manager_manager_update.hpp): 0x1019e8f0, the module's hook of the post-match broadcast;
// 0x1019e990, a club's manager after a match; 0x101c15f0, the pending list's clean-up.
//
// The career's snapshot still holds the fixtures of past match days (0x6c-byte records in the
// competitions' lists, found by their layout: real clubs, competition and dates), and most cases
// run over those with some fields overwritten; a share builds its fixture in the scratch area
// instead, filling exactly the fields the routines read. Random combinations (a fixed seed) of the
// values the rules read are written over real clubs, competitions, people, board rows and tables
// and undone afterwards, with leaning batches for the rules a uniform draw rarely reaches. Other
// modules' routines answer the same on both sides; the unrewritten ones of this module run as
// they are. The match record (two sides of twenty slots whose people are real) is built in the
// scratch area: the snapshot holds none. The captured week and day add the real calls.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t after_match = 0x1019e8f0, update = 0x1019e990, drop_pending = 0x101c15f0;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50;
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
constexpr std::int32_t leagues_slot = -0x7b5c, base_slot = -0x7b90, cursor_slot = -0x7b84, last_slot = -0x7b88,
                       cup_holder_slot = -0x7a2c, draw_rows_slot = -0x7984, people_slot = -0x7be0;
constexpr std::uint32_t fixture_area = scratch_base + 0x1000, venue_area = scratch_base + 0x1100,
                        leader_area = scratch_base + 0x1200, group_a_area = scratch_base + 0x1300,
                        group_b_area = scratch_base + 0x1400, coordinates_a = scratch_base + 0x1500,
                        coordinates_b = scratch_base + 0x1520, match_area = scratch_base + 0x2000,
                        record_area = scratch_base + 0x8000, table_area = scratch_base + 0x9000,
                        list_area = scratch_base + 0xa000, node_area = scratch_base + 0xa100,
                        element_area = scratch_base + 0xa400;
constexpr std::uint32_t league_has = 0x10452ed0, cup_flag = 0x10494d20, club_flag = 0x100c7d00, native_flag = 0x100cab10,
                        club_state = 0x1014c2e0, club_test = 0x100ce250, rivals = 0x10464160, event_check = 0x102cdbf0,
                        bad_run = 0x100d3c40, notice = 0x106891b0, results = 0x10459b50, manager_flag = 0x102de3c0,
                        redraw = 0x1077b770, calendar = 0x10718130, append = 0x10738d10,
                        remove = 0x10738e60;
std::string hex(std::uint32_t v) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", v);
    return text;
}

// How a batch leans: percentages that steer the random values toward rules a uniform draw rarely
// reaches.
struct Bias {
    unsigned no_match = 20, no_competition = 4, cup_kind = 12, same_league = 35, unplayed = 6, flags_set = 8,
             neutral = 30, manager_null = 0, won = 0, lost = 0, drawn = 0, marks = 0, ties = 0, small_rows = 0,
             zero_table = 0, rival = 0, rows_mid = 0, cup_run = 0, staff_null = 0,
             huge_goals = 3, leader_null = 8, open_review = 25;
};

// What a real career offers the cases.
struct World {
    std::uint32_t clubs, object, rows, people_table, people;
    std::vector<unsigned> managed;               // clubs with a manager and its staff record
    std::vector<std::uint32_t> staffed_people;   // people with a record at +0x14
    std::vector<unsigned> league_ids;            // ids whose league entry exists with an owner
    std::vector<unsigned> empty_ids;             // ids with no league entry
    std::vector<std::uint32_t> competitions;     // real competition records
    std::vector<std::uint32_t> fixtures;         // real fixture records the career still holds (see below)
};
World world_of(const Scenario& sc) {
    World w{};
    w.clubs = sc.global(club_total);
    w.object = sc.global(manager_object);
    w.rows = sc.r32(w.object);
    w.people_table = sc.global(people_slot);
    w.people = sc.people();
    for (unsigned n = 0; n < w.clubs; ++n)
        if (sc.r32(sc.club(n) + 0x90) && sc.r32(sc.r32(sc.club(n) + 0x90) + 0x1c)) w.managed.push_back(n);
    for (unsigned n = 0; n < w.people && w.staffed_people.size() < 400; ++n)
        if (sc.r32(sc.person(n) + 0x14)) w.staffed_people.push_back(n);
    const auto leagues = sc.global(leagues_slot);
    for (unsigned id = 0; id < 80; ++id) {
        const auto entry = sc.r32(leagues + id * 4);
        if (entry && sc.r32(entry + 8)) w.league_ids.push_back(id);
        if (!entry) w.empty_ids.push_back(id);
    }
    for (unsigned n = 0; n < 60; ++n) w.competitions.push_back(sc.competition(n));
    // The fixtures the competitions' lists still hold from past match days (0x6c bytes: id, the
    // competition at +0x14, the two clubs at +0x1c/+0x20, dates, the result bytes): records whose
    // clubs and competition are real, with a year in the date words, kept when one side has a
    // manager. Every league/cup kind of fixture they hold is taken, a spread of the others.
    const auto clubs_base = sc.club(0), clubs_end = clubs_base + w.clubs * 0x248, comps_base = sc.competition(0);
    std::vector<std::uint8_t> buffer;
    std::vector<std::uint32_t> others;
    for (const auto& [start, size] : sc.s.memory.mapped_ranges()) {
        if (size > 0x20000000) continue;
        buffer.resize(size);
        if (!sc.s.memory.try_copy_unobserved(start, buffer.data(), size)) continue;
        const auto at32 = [&](std::uint64_t at) {
            return std::uint32_t(buffer[at] << 24 | buffer[at + 1] << 16 | buffer[at + 2] << 8 | buffer[at + 3]);
        };
        for (std::uint64_t at = 0; at + 0x6c <= size; at += 4) {
            const auto home = at32(at + 0x1c), away = at32(at + 0x20), competition = at32(at + 0x14);
            if (home < clubs_base || home >= clubs_end || (home - clubs_base) % 0x248) continue;
            if (away < clubs_base || away >= clubs_end || (away - clubs_base) % 0x248) continue;
            if (competition < comps_base || competition >= comps_base + 400 * 0x6c || (competition - comps_base) % 0x6c) continue;
            if (at32(at) > 0xffff || at32(at + 0x24) >> 16 < 0x7c0 || at32(at + 0x24) >> 16 > 0x7e0) continue;
            const auto managed = [&](std::uint32_t club) { return sc.r32(club + 0x90) && sc.r32(sc.r32(club + 0x90) + 0x1c); };
            if (!managed(home) && !managed(away)) continue;
            const auto kind = buffer[at + 0x3a];
            (kind == 8 || kind == 5 ? w.fixtures : others).push_back(std::uint32_t(start + at));
        }
    }
    for (std::size_t k = 0; k < others.size() && w.fixtures.size() < 1500; k += 1 + others.size() / 1100)
        w.fixtures.push_back(others[k]);
    return w;
}

// The draw table and the results record the callees leave: halfwords in the scratch area, the
// base, where the table ends, and the record 0x10459b50 answers.
void table_and_record(Rng& rng, const Scenario& sc, Case& item, const Bias& bias, bool open_review) {
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    for (unsigned k = 0; k < 16; ++k)
        put(2, table_area + k * 2,
            rng.chance(bias.zero_table) ? 0 : rng.chance(50) ? rng.below(24) : rng.chance(30) ? 90 * rng.below(20) : rng.below(2000) );
    put(4, sc.toc(cursor_slot), table_area - 2 + 2 * rng.below(3));
    put(4, sc.toc(last_slot), rng.chance(15) ? table_area + 2 * rng.below(6) : table_area + 30);
    put(4, sc.toc(base_slot), rng.chance(bias.zero_table) ? 0 : rng.pick<std::uint32_t>({0, 0, 1, 2, 3, 7, 20, 90, 0xffff, 0x10001, 2000, 120}));
    put(1, record_area + 4, rng.pick<std::uint32_t>({0, 5, 20, 25, 30, 35, 255}));
    put(1, record_area + 0x75, rng.pick<std::uint32_t>({0, 1, 2, 3, 5, 8, 9, 10, 0xff}));
    put(1, record_area + 0x77, rng.pick<std::uint32_t>({0, 1, 1, 1, 2, 5, 20, 100, 255}));
    for (unsigned g = 0; g < 5; ++g) put(1, record_area + 0x16 + g, rng.pick<std::uint32_t>({3, 2, 1, 0xfd, 0, 4, 7, 3, 3, 1}));
    for (std::uint32_t at : {0x1cu, 0x30u, 0x44u, 0x58u}) put(4, record_area + at, rng.pick<std::uint32_t>({0, 1, 2, 3, 4, 0xffffffff, 2, 1}));
    // The review the update ends with runs its own gates (the review block's checks cover them):
    // mostly it stops at its first one, the date (the stamp a year ahead). Where it runs on, the
    // people table is empty so that the offer value (0x101abda0) has nothing to scan: a scan is
    // thousands of nested job assessments, which only outgrow the call trace's limit; and no news
    // item is posted then (the news module sizes its list by the people).
    if (open_review) {
        put(4, sc.toc(-0x7be4), 0);
    } else {
        const auto date = sc.toc(-0x7b80), stamp = sc.toc(-0x7a9c);
        put(2, stamp + 2, (sc.r32(date) & 0xffff) + 1);
        put(2, stamp, sc.r32(date) >> 16);
    }
    // The answers: other modules' routines, the same on both sides.
    const auto answer = [&](std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); };
    answer(league_has, rng.chance(85) ? 1 : 0);
    const auto event_answer = open_review ? 0u : rng.chance(45) ? 1u : 0u;
    answer(cup_flag, rng.chance(30) ? 1 : 0);
    answer(event_check, event_answer);
    for (const auto entry : {club_flag, native_flag, club_test, rivals, bad_run, manager_flag})
        answer(entry, rng.chance(45) ? 1 : 0);
    answer(club_state, rng.chance(25) ? 1 : 0);
    answer(notice, 0);
    answer(results, record_area);
    answer(redraw, rng.next());
    answer(calendar, rng.pick<std::uint32_t>({0x32, 0x96, 0x97, 200, 0x190, 1000, 101, 100, 0x3c, 0x1f, 0xf}));
    answer(append, 0);
    answer(remove, 0);
}

// One randomized call of update (or of the hook): over a real fixture the career still holds
// (`real`, with some of its fields overwritten) or over one built in the scratch area, with the
// match record built there too.
Case randomized(Rng& rng, const Scenario& sc, const World& w, bool hook, std::uint32_t real, const Bias& bias = {}) {
    Case item;
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto managed = [&](std::uint32_t club) { return sc.r32(club + 0x90) && sc.r32(sc.r32(club + 0x90) + 0x1c); };
    const auto fixture = real ? real : fixture_area;
    std::uint32_t first, second;  // the fixture's clubs
    if (real) {
        first = sc.r32(real + 0x1c);
        second = sc.r32(real + 0x20);
    } else {
        first = sc.club(w.managed[rng.below(unsigned(w.managed.size()))]);
        second = sc.club(w.managed[rng.below(unsigned(w.managed.size()))]);
        if (first == second) second = sc.club((unsigned((first - sc.club(0)) / 0x248) + 1) % w.clubs);
    }
    // The side the club plays: the home byte (a signed byte, so 0x100 reads clear and 0xff set).
    bool home = rng.chance(50);
    if (!managed(home ? first : second)) home = !home;
    const std::uint32_t flag_value = hook ? (home ? 1u : 0u)
                                          : home ? rng.pick<std::uint32_t>({1, 1, 1, 0xff, 0x80, 0x7f}) : rng.pick<std::uint32_t>({0, 0, 0, 0x100});
    const auto club = home ? first : second, opponent = home ? second : first;
    const auto club_id = sc.r32(club);
    const auto manager = sc.r32(club + 0x90), staff = sc.r32(manager + 0x1c);
    const auto club_n = unsigned((club - sc.club(0)) / 0x248), other_n = unsigned((opponent - sc.club(0)) / 0x248);
    // The fixture: id, competition, the round, flags, the kind and the result bytes.
    const bool has_competition = real ? true : !rng.chance(bias.no_competition);
    const auto competition = real ? sc.r32(real + 0x14) : has_competition ? w.competitions[rng.below(unsigned(w.competitions.size()))] : 0;
    const auto id = w.league_ids.empty() ? 0u : w.league_ids[rng.below(unsigned(w.league_ids.size()))];
    const bool voided = hook && rng.chance(3), unplayed = hook ? rng.chance(4) : rng.chance(bias.unplayed);
    if (!real) {
        put(4, fixture + 0x14, competition);
        put(4, fixture + 0x18, rng.chance(70) ? venue_area : rng.chance(50) ? group_a_area : 0);
        put(4, fixture + 0x1c, first);
        put(4, fixture + 0x20, second);
    }
    // A real id indexes the league table as it is; a scratch one picks an entry that exists.
    if (!real || voided || rng.chance(15)) put(4, fixture + 0x0, voided ? 0xfffffffeu : rng.chance(3) ? 0xffffffffu : id);
    const auto fixture_id = voided ? 0u : real ? sc.r32(real) : id;
    if (rng.chance(20)) put(4, club + 0x2c, sc.r32(opponent + 0x2c));  // (opponent set below; the regions swap)
    const auto override_chance = real ? 35u : 100u;  // how often a real fixture's field is replaced
    if (rng.chance(override_chance))
        put(2, fixture + 0x30, rng.chance(real ? 85 : 60) ? rng.pick<std::uint32_t>({0x78, 0x82, 0x8c, 0x96, 0xa0, 0x7b, 0x7c, 0x85, 0x86, 0x99, 0x9a, 0x7d, 0x87, 0x9b, 0x79, 0x84, 0x98})
                                                          : rng.pick<std::uint32_t>({0, 1, 0x50, 0x100, 0x7a, 0x88, 0x9c, 0xffff, 0x8000}));
    if (rng.chance(override_chance))
        put(2, fixture + 0x32, rng.chance(40) ? rng.pick<std::uint32_t>({0x41e, 0x41f, 0x420, 0x426, 0x427, 0x428}) : rng.below(0x500));
    if (!real || rng.chance(bias.flags_set))
        put(2, fixture + 0x38, rng.chance(real ? 50 : bias.flags_set) ? rng.pick<std::uint32_t>({0x10, 0x800, 0x2000, 0x4, 0x814, 0xffff, 0x2800})
                                                                      : rng.pick<std::uint32_t>({0, 0, 0, 0x4, 0x1, 0x20}));
    if (!real || rng.chance(25))
        put(1, fixture + 0x3a, rng.chance(bias.cup_kind) ? 5u : rng.chance(real ? 97 : 88) ? (rng.chance(50) ? 8u : rng.pick<std::uint32_t>({1, 2, 4, 5, 6, 7, 0xa})) : rng.pick<std::uint32_t>({0, 9, 0xff}));
    if (!real || unplayed) put(1, fixture + 0x40, unplayed ? 0 : rng.pick<std::uint32_t>({1, 1, 1, 0x80, 0xff}));
    if (!real || rng.chance(70)) {
        std::uint32_t goals_home = rng.chance(25) ? rng.below(10) : rng.below(4), goals_away = rng.chance(25) ? rng.below(10) : rng.below(4);
        if (bias.won) { goals_home = 2 + rng.below(6); goals_away = rng.below(2); }
        if (bias.lost) { goals_home = rng.below(2); goals_away = 2 + rng.below(6); }
        if (bias.drawn) goals_away = goals_home;
        if (rng.chance(bias.huge_goals)) (rng.chance(50) ? goals_home : goals_away) = 11 + rng.below(20);
        if (!home) std::swap(goals_home, goals_away);  // the club's goals are the fixture's first side's when home
        put(1, fixture + 0x43, goals_home);
        put(1, fixture + 0x44, goals_away);
        const bool aggregate = rng.chance(bias.ties ? 10 : 25);
        put(1, fixture + 0x47, aggregate ? rng.below(7) : 0xff);
        put(1, fixture + 0x48, aggregate ? rng.below(7) : 0xff);
        const bool shootout = rng.chance(bias.ties ? 85 : 30);
        const auto pens = shootout ? rng.below(6) : 0xffu;
        put(1, fixture + 0x4b, rng.chance(8) ? 0xff : pens);
        put(1, fixture + 0x4c, shootout ? (rng.chance(bias.ties ? 55 : 25) ? pens : rng.below(6)) : rng.pick<std::uint32_t>({0xff, 0, 3}));
        const bool equal_legs = rng.chance(bias.ties ? 55 : 30);
        const auto leg = rng.below(3);
        put(1, fixture + 0x49, rng.chance(8) ? 0xff : leg);
        put(1, fixture + 0x4a, equal_legs ? leg : rng.pick<std::uint32_t>({0, 1, 2, 0xff}));
    }
    // The competition (its level), the first club's league and the clubs' own fields.
    if (competition && (!real || rng.chance(50))) put(2, competition + 0x18, rng.pick<std::uint32_t>({0, 3, 5, 6, 7, 10, 20, 0xffff, 8}));
    if (competition && rng.chance(bias.same_league)) put(4, first + 0x30, competition);
    if (competition && rng.chance(25)) put(4, club + 0x30, competition);
    const auto reputation = [&] {
        return rng.pick<std::uint32_t>({0x100, 0x500, 0xe00, 0xea5, 0xea6, 0x1000, 0x1388, 0x1770, 0x2000, 0x2710, 0x3000, 0x7fff, 0x9000}) + rng.below(4) * rng.below(300);
    };
    put(2, club + 0x1ec, reputation());
    put(2, opponent + 0x1ec, rng.chance(20) ? sc.r32(club + 0x1ec) & 0xffff : reputation());
    if (rng.chance(35)) put(4, opponent + 0x2c, sc.r32(club + 0x2c));
    // The club's current competition id: the fixture's own (a tie), another real one, or as it is.
    if (rng.chance(60)) put(4, club + 0x14, fixture_id);
    else if (rng.chance(50) && !w.league_ids.empty()) put(4, club + 0x14, w.league_ids[rng.below(unsigned(w.league_ids.size()))]);
    if (!w.empty_ids.empty() && rng.chance(8)) {  // a tie whose league entry doesn't exist
        const auto empty_id = w.empty_ids[rng.below(unsigned(w.empty_ids.size()))];
        put(4, fixture + 0x0, empty_id);
        put(4, club + 0x14, empty_id);
    }
    put(4, sc.toc(cup_holder_slot), rng.chance(15) ? fixture_id : fixture_id + 7);
    const auto venue = real ? sc.r32(real + 0x18) : venue_area;
    if (!real) {
        put(4, leader_area + 0x18, rng.chance(50) ? sc.r32(club + 0x2c) : sc.r32(opponent + 0x2c));
        put(4, venue_area + 0x10, rng.chance(bias.leader_null) ? 0 : leader_area);
        put(4, group_a_area + 0x10, rng.chance(bias.leader_null) ? 0 : coordinates_a);
        put(4, group_b_area + 0x10, rng.chance(bias.leader_null) ? 0 : rng.chance(40) ? coordinates_a : coordinates_b);
        // The groups' coordinates (doubles): far apart, equal, or exactly 5/11 apart, where 55 times
        // the distance is the limit 25 (the comparison's equal outcome).
        const auto bits = [](double v) { std::uint64_t b; __builtin_memcpy(&b, &v, 8); return b; };
        const auto point = [&](std::uint32_t at, double x, double y) {
            put(4, at, std::uint32_t(bits(x) >> 32));
            put(4, at + 4, std::uint32_t(bits(x)));
            put(4, at + 8, std::uint32_t(bits(y) >> 32));
            put(4, at + 12, std::uint32_t(bits(y)));
        };
        const auto mode = rng.below(10);
        if (mode == 0) {
            point(coordinates_a, 0.0, 0.0);
            point(coordinates_b, 5.0 / 11.0, 0.0);
        } else if (mode <= 2) {
            point(coordinates_a, 3.0, 4.0);
            point(coordinates_b, 3.0, 4.0);
        } else {
            point(coordinates_a, double(rng.below(100)), double(rng.below(100)));
            point(coordinates_b, double(rng.below(100)), double(rng.below(100)));
        }
        const auto groups = std::initializer_list<std::uint32_t>{0, venue_area, group_a_area, group_b_area};
        put(4, club + 0x3c, rng.chance(bias.rival ? 70 : 50) ? (rng.chance(50) ? venue_area : group_a_area) : *(groups.begin() + rng.below(4)));
        put(4, opponent + 0x3c, *(groups.begin() + rng.below(4)));
    } else if (rng.chance(40)) {
        // A real group pointer on one side, equal to the fixture's venue or the other club's.
        put(4, club + 0x3c, rng.chance(50) ? venue : sc.r32(opponent + 0x3c));
        if (rng.chance(30)) put(4, opponent + 0x3c, sc.r32(club + 0x3c));
    }
    (void)venue;
    const bool rival = rng.chance(bias.rival ? 70 : 20);
    put(4, club + 0x74, rival && rng.chance(34) ? opponent : 0);
    put(4, club + 0x78, rival && rng.chance(50) ? opponent : 0);
    put(4, club + 0x7c, rival && rng.chance(60) ? opponent : 0);
    // The manager, the board's row and the draw rows.
    if (rng.chance(bias.staff_null)) put(4, manager + 0x1c, 0);
    put(1, manager + 0x69, rng.pick<std::uint32_t>({0, 1, 3, 5, 0xfb, 0x7f, 0x80, 0xff, 7, 6, 12}));
    put(1, staff + 0x3d, rng.pick<std::uint32_t>({0, 1, 0xff, 2, 3, 0xfe, 0x7f, 0x80, 4, 5, 8, 0xfd}));
    if (bias.manager_null && rng.chance(bias.manager_null)) put(4, club + 0x90, 0);
    const auto row = w.rows + club_id * 0x4c;
    const auto figure = [&] {
        if (bias.small_rows) return rng.pick<std::uint32_t>({0, 1, 2, 5, 0xffff, 0x8000, 0x10, 0x7fff});
        if (bias.rows_mid) return rng.pick<std::uint32_t>({0x1964, 0x1963, 0x1965, 0x10b3, 0x10b2, 0x10b4, 0xdac, 0xdab, 0xdad, 0xbb8, 0xbb7, 0xbb9, 0x2134, 0x2133, 0x1a5e, 0x1a5f, 0x1482, 0x1481, 0x1483, 0x898, 0x899});
        return rng.chance(8) ? rng.pick<std::uint32_t>({0x2710, 0x2711, 0x2700, 0, 1, 0xffff, 0x7fff}) : rng.pick<std::uint32_t>({0x1964, 0x1963, 0x10b3, 0x10b2, 0xdac, 0xbb8, 0x2134, 0x1a5e, 0x1482, 0x1483, 0x898, 0x899, 0x500, 0x2000, 0x2600, 0x300, 0x1c00, 0x1000});
    };
    for (const std::uint32_t at : {6u, 8u, 0xau, 0xcu}) put(2, row + at, figure());
    put(2, row + 0xe, rng.pick<std::uint32_t>({0, 0x400, 0x4, 0x404, 0x40, 0xffff}));
    put(2, row + 0x16, rng.pick<std::uint32_t>({0, 100, 0x1000, 0xfa, 0xff06, 0x7fff}));
    put(1, sc.global(draw_rows_slot) + club_id * 8 + 4, rng.chance(50) ? 0x80 | rng.below(0x7f) : rng.below(0x80));
    // The match record: the neutral flag, the spare strength, each side's strengths, 20 slots.
    const bool with_match = !rng.chance(bias.no_match);
    if (with_match) {
        put(1, match_area + 0x4c, rng.chance(bias.neutral) ? rng.pick<std::uint32_t>({1, 0xff, 0x80}) : 0);
        put(2, match_area + 0x4e, rng.pick<std::uint32_t>({0, 1, 5, 10, 11, 20, 50, 0xfffb, 0x80, 0x100, 0xffff}));
        for (unsigned side = 0; side < 2; ++side) {
            put(1, match_area + side * 0x664 + 0x128, rng.chance(15) ? rng.below(256) : 5 + rng.below(16));
            put(1, match_area + side * 0x664 + 0x129, rng.chance(15) ? rng.below(256) : 5 + rng.below(16));
            for (unsigned k = 0; k < 20; ++k) {
                const auto slot = match_area + side * 0x664 + k * 0x4c;
                const bool empty = rng.chance(bias.marks ? 12 : 30);
                put(4, slot + 0x194, empty ? 0xffffffffu : w.staffed_people[rng.below(unsigned(w.staffed_people.size()))]);
                const auto mark = [&] { return rng.pick<std::uint32_t>({0xff, 0xff, 0, 1, 5, 10, 0x7f, 0x80, 0xfe, 20, 3, 0x30, 0xc8}); };
                put(1, slot + 0x1c4, mark());
                put(1, slot + 0x1c6, mark());
                put(1, slot + 0x1c8, mark());
                put(1, slot + 0x1c9, rng.pick<std::uint32_t>({0, 1, 2, 5, 10, 100, 255}));
            }
        }
        // Persons' region words: some equal the club's region (the squad weighing's first branch).
        for (unsigned k = 0; k < 8; ++k) {
            const auto person = sc.person(w.staffed_people[rng.below(unsigned(w.staffed_people.size()))]);
            put(4, person + 0xc, rng.chance(40) ? sc.r32(club + 0x2c) : sc.r32(person + 0xc));
        }
    }
    // A cup run: round 0x96 of a cup fixture for a club whose row has the run's flag; a manager
    // cleared before the update (the callers check it, a callee could clear it): the round that
    // asks for no review, which the missing manager would fault.
    if (bias.cup_run) {
        put(2, fixture + 0x30, 0x96);
        put(1, fixture + 0x3a, 5);
        put(1, fixture + 0x40, 1);
        put(2, row + 0xe, rng.pick<std::uint32_t>({4, 0x404, 0xffff, 0x4}));
        put(2, fixture + 0x38, 0);
    }
    if (bias.manager_null && !hook) {
        put(2, fixture + 0x30, 0xa0);
        put(1, fixture + 0x40, 1);
    }
    table_and_record(rng, sc, item, bias, rng.chance(bias.open_review));
    if (hook) {
        item.entry = after_match;
        item.registers = {{3, w.object}, {4, fixture}, {5, with_match ? match_area : 0}};
    } else {
        item.entry = update;
        item.registers = {{3, w.object}, {4, club}, {5, fixture}, {6, with_match ? match_area : 0}, {7, flag_value}};
    }
    item.label = std::string(hook ? "hook" : "update") + (real ? " real fixture " + hex(real) : " scratch fixture") + " club " + std::to_string(club_n) +
                 " vs " + std::to_string(other_n) + (with_match ? " with match" : " no match") + " flag " + hex(flag_value);
    return item;
}
}

MANAGER_MANAGER_CASES("update randomized") {
    if (scenario.match || !scenario.global(club_total)) return;
    const auto w = world_of(scenario);
    if (w.managed.size() < 2 || w.staffed_people.empty() || w.competitions.empty()) return;
    Rng rng{0x7a11d1ce};
    const auto real = [&] { return w.fixtures.empty() ? 0u : w.fixtures[rng.below(unsigned(w.fixtures.size()))]; };
    for (unsigned k = 0; k < 1500; ++k) cases.push_back(randomized(rng, scenario, w, false, real()));
    for (unsigned k = 0; k < 400; ++k) cases.push_back(randomized(rng, scenario, w, true, real()));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, w, false, 0));
    for (unsigned k = 0; k < 100; ++k) cases.push_back(randomized(rng, scenario, w, true, 0));
    // Leaning batches for the rules a uniform draw rarely reaches.
    for (unsigned k = 0; k < 250; ++k) cases.push_back(randomized(rng, scenario, w, false, 0, {.cup_kind = 60, .lost = 1, .rival = 1, .huge_goals = 20, .leader_null = 15}));
    for (unsigned k = 0; k < 250; ++k) cases.push_back(randomized(rng, scenario, w, false, real(), {.lost = 1, .cup_run = 1, .small_rows = 0}));
    for (unsigned k = 0; k < 250; ++k) cases.push_back(randomized(rng, scenario, w, false, real(), {.lost = 1, .rows_mid = 1, .cup_run = 1}));
    for (unsigned k = 0; k < 200; ++k) cases.push_back(randomized(rng, scenario, w, false, 0, {.manager_null = 70, .lost = 1, .staff_null = 0}));
    for (unsigned k = 0; k < 200; ++k) cases.push_back(randomized(rng, scenario, w, false, 0, {.manager_null = 0, .lost = 1, .staff_null = 60}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, w, false, 0, {.won = 1, .rival = 1, .leader_null = 12, .rows_mid = 1}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(randomized(rng, scenario, w, false, 0, {.drawn = 1, .ties = 1, .small_rows = 1}));
}

// The pending list of 0x101c15f0: lists built in the scratch area (the club's entry among
// others, first, last, absent, a null element, an empty list, a cursor left mid-list). The list
// is the game's doubly linked one: a header {count (halfword), first node, last node, cursor} and
// nodes {data pointer, size (halfword), next, previous}; list_remove compares each node's data
// with the entry, so a null element is only walked past when the club's entry is absent. The
// removal itself is answered (the game frees the node); the call's arguments are compared.
MANAGER_MANAGER_CASES("pending list") {
    if (scenario.match || !scenario.global(club_total)) return;
    const auto clubs = scenario.global(club_total);
    const auto object = scenario.global(manager_object);
    Rng rng{0x5eed1157};
    for (unsigned k = 0; k < 200; ++k) {
        const auto n = rng.below(clubs);
        const auto club = scenario.club(n);
        // list_remove frees the node it unlinks (a heap block the scratch list is not): it answers.
        Case item{.entry = drop_pending, .registers = {{3, object}, {4, club}}, .answers = {{remove, 0}}};
        auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
        const unsigned count = k % 8 == 0 ? 0 : 1 + rng.below(5);
        const unsigned mine = rng.below(count + 2);  // beyond the list: the club is absent
        const bool absent = mine >= count;
        put(4, object + 0x10, list_area);
        put(2, list_area, count);
        put(4, list_area + 4, count ? node_area : 0);
        put(4, list_area + 8, count ? node_area + (count - 1) * 16 : 0);
        put(4, list_area + 0xc, rng.chance(70) ? 0 : node_area + 16 * rng.below(count + 1));
        for (unsigned e = 0; e < count; ++e) {
            const bool null_element = absent && rng.chance(25);
            put(4, node_area + e * 16, null_element ? 0 : element_area + e * 16);
            put(2, node_area + e * 16 + 4, 4);
            put(4, node_area + e * 16 + 8, e + 1 < count ? node_area + (e + 1) * 16 : 0);
            put(4, node_area + e * 16 + 12, e > 0 ? node_area + (e - 1) * 16 : 0);
            put(4, element_area + e * 16, e == mine ? scenario.r32(club) : scenario.r32(club) + 1 + e);
        }
        item.label = "pending club " + std::to_string(n) + " count " + std::to_string(count) + " mine " + std::to_string(mine);
        cases.push_back(item);
    }
}
}

// COMP_UTIL.CPP, block `club_queries`: cases for whether a competition is active (0x104646d0), the class of a
// fixture for a club (0x10464900) and whether two clubs of a league are contenders (0x10464160).
//
// The career's real competitions, standings and statistics are the starting point (a captured state: season-day).
// What a branch needs that the career lacks is written over a scratch copy of a real object, or over the real one
// in a case's own memory (undone afterwards): the league object's club count, kind, promotion places and last
// stage, the statistics rows' figures (answered through 0x10459b50, which a case answers on both sides with the
// scratch rows), the league's virtual +0xa0 (its vtable is copied to the scratch area and the slot points at a
// dummy routine of another module that the case answers: the answers are the virtual's results, call by call),
// and the clubs' reputations. Fixtures and clubs the career has no instance of are built in the scratch area.
// Labels start with "club_queries:" (CM_COMP_UTIL_BLOCK=club_queries).
#include "comp_util_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <random>
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t is_active = 0x104646d0, fixture_class = 0x10464900, contenders = 0x10464160;
constexpr std::uint32_t stats_row = 0x10459b50, marks_record = 0x104abf40;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8,
                       club_count_slot = -0x7b50, special_a = -0x6888, special_b = -0x67bc, special_c = -0x6734;
constexpr std::uint32_t area = scratch_base + 0x30000;
constexpr std::uint32_t object_at = area, vtable_at = area + 0x100, descriptors_at = area + 0x200, row_first = area + 0x400,
                        row_second = area + 0x500, club_first = area + 0x600, club_second = area + 0x900, club_third = area + 0xc00,
                        fixture_at = area + 0x1000;
constexpr std::uint32_t real_toc = 0x108bba90;
// A routine of another module the league's virtual +0xa0 is pointed at; a case answers it.
constexpr std::uint32_t dummy = 0x102aed80;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

// A league of the career that the contender query can reach: its object is the competition table's entry for its
// record's id, a league (kind 2) without a parent stage, with statistics and standings.
struct League {
    std::uint32_t object, record, id, stats, standings, clubs_total;
    std::vector<std::uint32_t> clubs;  // the standing rows' clubs, in table order
};
struct World {
    const Scenario& s;
    std::vector<League> leagues;
    std::uint32_t table, count;
    explicit World(const Scenario& scenario) : s(scenario), table(s.global(competitions_slot)), count(s.global(competition_count_slot)) {
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object || s.u8(object + 0x43) != 2 || std::int32_t(s.r32(object + 0x30)) != -1) continue;
            const auto record = s.r32(object + 4), stats = s.r32(object + 8), standings = s.r32(object + 0xb0);
            const auto total = std::int16_t(s.u16(object + 0x3e));
            if (!record || !stats || !standings || total < 2 || total > 64 || s.r32(record) != id) continue;
            League league{object, record, id, stats, standings, std::uint32_t(total), {}};
            for (std::int32_t n = 0; n < total; ++n) league.clubs.push_back(s.r32(standings + std::uint32_t(n) * 0x44));
            if (std::ranges::all_of(league.clubs, [&](std::uint32_t club) { return club != 0; })) leagues.push_back(std::move(league));
        }
    }
    std::uint32_t club_total() const { return s.global(club_count_slot); }
    std::uint32_t club(unsigned index) const { return s.global(clubs_slot) + index * 0x248; }
};

using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

// The league object the competition table names for `league.id`, replaced by a scratch copy: `memory` writes the
// copy, a vtable copy whose slot +0xa0 points at the dummy routine, and the table entry.
std::vector<Write> scratch_league(const Scenario& s, const League& league, std::uint32_t kind, std::int32_t last_stage,
                                  std::uint32_t club_count, std::uint32_t automatic, std::uint32_t playoff) {
    std::vector<Write> memory;
    for (std::uint32_t offset = 0; offset < 0xec; offset += 4) memory.push_back(w32(object_at + offset, s.r32(league.object + offset)));
    const auto vtable = s.r32(league.object);
    for (std::uint32_t offset = 0; offset < 0xa8; offset += 4) memory.push_back(w32(vtable_at + offset, s.r32(vtable + offset)));
    append(memory, {w32(descriptors_at, dummy), w32(descriptors_at + 4, real_toc), w32(vtable_at + 0xa0, descriptors_at), w32(object_at, vtable_at),
                    w8(object_at + 0x43, kind), w32(object_at + 0x30, std::uint32_t(last_stage)), w16(object_at + 0x3e, club_count),
                    w8(object_at + 0xc0, automatic), w8(object_at + 0xc1, playoff), w32(object_at + 4, league.record)});
    memory.push_back(w32(s.global(competitions_slot) + league.id * 4, object_at));
    return memory;
}
// The statistics rows' bytes the query reads: games played (+4) and the four figures (+0x75..+0x78).
std::vector<Write> stats_rows(std::uint32_t games_first, const std::array<std::uint32_t, 4>& first, std::uint32_t games_second,
                              const std::array<std::uint32_t, 4>& second) {
    std::vector<Write> memory{w8(row_first + 4, games_first), w8(row_second + 4, games_second)};
    for (unsigned n = 0; n < 4; ++n) {
        memory.push_back(w8(row_first + 0x75 + n, first[n]));
        memory.push_back(w8(row_second + 0x75 + n, second[n]));
    }
    return memory;
}
std::vector<Write> scratch_clubs(std::uint32_t first_record, std::uint32_t second_record) {
    return {w32(club_first + 0x30, first_record), w32(club_second + 0x30, second_record)};
}
std::string join(const char* what, const League& league, const std::string& detail) { return std::string("club_queries: ") + what + " " + hex(league.id) + " " + detail; }

COMP_UTIL_CASES("club_queries") {
    // 0x104646d0 on the career's real competitions: every id of the table.
    const World w(scenario);
    std::fprintf(stderr, "club_queries: %zu real leagues (kind 2, no parent stage, with statistics and standings) of %u competitions\n",
                 w.leagues.size(), w.count);
    // The league's virtual +0xa0 targets in this career (each is checked to read r3 only: tools/register_contract.py).
    std::set<std::uint32_t> virtual_targets;
    for (const auto& league : w.leagues) virtual_targets.insert(scenario.r32(scenario.r32(scenario.r32(league.object) + 0xa0)));
    for (const auto target : virtual_targets) std::fprintf(stderr, "club_queries: league virtual +0xa0 target %x\n", target);
    if (w.leagues.size() < 8) throw std::runtime_error("club_queries: the career has too few leagues");
    for (std::uint32_t id = 0; id < w.count && id < 4096; ++id)
        cases.push_back({.entry = is_active, .registers = {{3, id}}, .label = "club_queries: is_active real id " + hex(id)});
    // The three special ids: the cells name them; the same ids through their cells' words.
    for (const auto slot : {special_a, special_b, special_c}) {
        const auto id = scenario.r32(scenario.toc(slot));
        cases.push_back({.entry = is_active, .registers = {{3, id}}, .label = "club_queries: is_active special id " + hex(id)});
    }
    // Branches the career's own competitions do not take, by writing over a real active league (undone afterwards):
    // the first league whose answer is yes, and its fields one at a time.
    const auto& league = w.leagues.front();
    const auto top = league.clubs.front();
    const auto table_entry = scenario.global(competitions_slot) + league.id * 4;
    const std::vector<std::pair<std::string, std::vector<Write>>> edits = {
        {"as is", {}},
        {"special a", {w32(scenario.toc(special_a), league.id)}},
        {"special b", {w32(scenario.toc(special_b), league.id)}},
        {"special c", {w32(scenario.toc(special_c), league.id)}},
        {"no nation", {w32(league.record + 8, 0)}},
        {"no object", {w32(table_entry, 0)}},
        {"kind 1", {w8(league.object + 0x43, 1)}},
        {"kind 3", {w8(league.object + 0x43, 3)}},
        {"no clubs", {w16(league.object + 0x3e, 0)}},
        {"clubs 0x10000", {w16(league.object + 0x3e, 0x8000)}},
        {"no standings", {w32(league.object + 0xb0, 0)}},
        {"top club's record is the league's", {w32(top + 0x30, league.record), w32(top + 0x38, 0)}},
        {"top club's second record is the league's", {w32(top + 0x30, 0), w32(top + 0x38, league.record)}},
        {"neither record", {w32(top + 0x30, 0), w32(top + 0x38, 0)}},
        {"both records", {w32(top + 0x30, league.record), w32(top + 0x38, league.record)}},
    };
    for (const auto& [name, memory] : edits)
        cases.push_back({.entry = is_active, .registers = {{3, league.id}}, .memory = memory, .label = join("is_active", league, name)});
}

COMP_UTIL_CASES("club_queries") {
    // 0x10464900: the class of a fixture. A fixture and its two clubs are built in the scratch area; the clubs'
    // reputations (+0x1ec) and the fixture's goal bytes are the inputs. `club` is the first side, the second, or
    // a third club (which takes the second side's tables, as the original's `club != first` test does).
    const World w(scenario);
    const auto fixture = [&](std::uint32_t club, std::int32_t reputation_first, std::int32_t reputation_second,
                             std::vector<Write> goals, const std::string& label) {
        std::vector<Write> memory{w32(fixture_at + 0x1c, club_first), w32(fixture_at + 0x20, club_second),
                                  w16(club_first + 0x1ec, std::uint32_t(reputation_first)), w16(club_second + 0x1ec, std::uint32_t(reputation_second))};
        append(memory, {w8(fixture_at + 0x43, 0), w8(fixture_at + 0x44, 0), w8(fixture_at + 0x47, 0xff), w8(fixture_at + 0x48, 0),
                        w8(fixture_at + 0x4b, 0xff), w8(fixture_at + 0x4c, 0)});
        append(memory, goals);
        cases.push_back({.entry = fixture_class, .registers = {{3, club}, {4, fixture_at}}, .memory = std::move(memory),
                         .label = "club_queries: fixture_class " + label});
    };
    const auto reputation = [](std::int32_t step, bool fraction) { return step * 500 + (fraction ? (step < 0 ? -499 : 499) : 0); };
    // The three places the goal difference can come from (the latest leg played), for a margin m.
    const auto goals = [](unsigned path, std::int32_t margin, unsigned k) -> std::vector<Write> {
        const auto high = std::uint32_t(margin + std::int32_t(k)) & 0xff, low = k & 0xff;
        if (path == 0) return {w8(fixture_at + 0x4b, high), w8(fixture_at + 0x4c, low)};
        if (path == 1) return {w8(fixture_at + 0x47, high), w8(fixture_at + 0x48, low)};
        return {w8(fixture_at + 0x43, high), w8(fixture_at + 0x44, low)};
    };
    // An unplayed byte (-1) as the high byte would change the path: pick the first spare constant.
    const auto spare = [&](unsigned path, std::int32_t margin) {
        for (const unsigned k : {0u, 3u, 100u, 17u})
            if (path == 2 || ((std::uint32_t(margin + std::int32_t(k)) & 0xff) != 0xff)) return k;
        return 5u;
    };
    const std::int32_t margins[] = {-7, -4, -3, -2, -1, 0, 1, 2, 3, 4, 7};
    for (const std::int32_t base : {0, 6, 12, 25})
        for (std::int32_t d = -19; d <= 19; ++d)
            for (const auto margin : margins)
                for (const auto club : {club_first, club_second}) {
                    const auto fraction = (d + margin) % 2 != 0;
                    fixture(club, reputation(base, fraction), reputation(base + d, !fraction), goals(0, margin, spare(0, margin)),
                            "step " + std::to_string(base) + "/" + std::to_string(base + d) + " margin " + std::to_string(margin) + " side " + (club == club_first ? "1" : "2"));
                }
    for (const std::int32_t base : {0, 12})
        for (const std::int32_t d : {-18, -9, -4, 0, 4, 9, 18})
            for (const std::int32_t margin : {-3, -1, 0, 1, 3})
                for (const unsigned path : {1u, 2u})
                    for (const auto club : {club_first, club_second, club_third})
                        fixture(club, reputation(base, false), reputation(base + d, true), goals(path, margin, spare(path, margin)),
                                "path " + std::to_string(path) + " step " + std::to_string(base) + "/" + std::to_string(base + d) + " margin " + std::to_string(margin) + " side " + std::to_string(club == club_first ? 1 : club == club_second ? 2 : 3));
    // The margin's byte wrap: 127 and -128, played and negated.
    for (const std::int32_t margin : {127, -128, 100, -100, 5, -5})
        for (const auto club : {club_first, club_second})
            fixture(club, 3000, 2500, goals(0, margin, 0), "margin byte " + std::to_string(margin));
    // The reputation extremes (halfword, signed) and the division by 500 at its steps.
    const std::int32_t extremes[] = {-32768, -32767, -1000, -501, -500, -499, -1, 0, 1, 249, 250, 499, 500, 501, 1000, 32766, 32767};
    for (const auto a : extremes)
        for (const auto b : {-32768, -500, 0, 499, 500, 32767})
            for (const auto club : {club_first, club_second})
                fixture(club, a, b, goals(0, 0, 0), "reputation " + std::to_string(a) + "/" + std::to_string(b));
    // Real clubs and a real-looking fixture: clubs of the career in pairs, a few results each.
    for (unsigned n = 0; n + 1 < w.club_total() && n < 120; n += 3) {
        const auto left = w.club(n), right = w.club(n + 1);
        for (const std::int32_t margin : {-2, 0, 1}) {
            std::vector<Write> memory{w32(fixture_at + 0x1c, left), w32(fixture_at + 0x20, right), w8(fixture_at + 0x47, 0xff), w8(fixture_at + 0x4b, 0xff)};
            append(memory, goals(2, margin, 0));
            for (const auto club : {left, right})
                cases.push_back({.entry = fixture_class, .registers = {{3, club}, {4, fixture_at}}, .memory = memory,
                                 .label = "club_queries: fixture_class real clubs " + std::to_string(n) + " margin " + std::to_string(margin) + (club == left ? " first" : " second")});
        }
    }
}

COMP_UTIL_CASES("club_queries") {
    // 0x10464160 on the career: pairs of clubs of every real league (the real statistics rows, the real league
    // virtual), the early exits (clubs of different leagues, one without a record), then scratch states.
    const World w(scenario);
    for (const auto& league : w.leagues) {
        const auto n = std::uint32_t(league.clubs.size());
        const std::pair<std::uint32_t, std::uint32_t> pairs[] = {{0, 1}, {1, 0}, {0, n - 1}, {n / 2, n / 2 + 1}, {n - 2, n - 1}, {0, 0}, {n - 1, 0}};
        for (const auto& [a, b] : pairs)
            cases.push_back({.entry = contenders, .registers = {{3, league.clubs[a]}, {4, league.clubs[b]}},
                             .label = join("contenders real pair", league, std::to_string(a) + "/" + std::to_string(b))});
    }
    // The early exits, on a real league's clubs.
    const auto& league = w.leagues.front();
    const auto& other = w.leagues.back();
    const auto a = league.clubs[0], b = league.clubs[1];
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, other.clubs[0]}}, .label = join("contenders", league, "clubs of two leagues")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .memory = {w32(a + 0x30, 0), w32(b + 0x30, 0)},
                     .label = join("contenders", league, "no record")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .memory = {w32(a + 0x30, 0)}, .label = join("contenders", league, "first without a record")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .answers = {{marks_record, 1}}, .label = join("contenders", league, "record marked")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .answers = {{marks_record, 0x100}}, .label = join("contenders", league, "record mark byte zero")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .answers = {{stats_row, 0}}, .label = join("contenders", league, "no rows")});
    cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .nth_answers = {{stats_row, 2, 0}}, .label = join("contenders", league, "no second row")});
    const std::vector<std::pair<std::string, std::vector<Write>>> real_edits = {
        {"no stats object", {w32(league.object + 8, 0)}},
        {"no object", {w32(scenario.global(competitions_slot) + league.id * 4, 0)}},
        {"has a parent stage", {w32(league.object + 0x30, 0)}},
        {"kind 1", {w8(league.object + 0x43, 1)}},
        {"no clubs", {w16(league.object + 0x3e, 0)}},
        {"negative clubs", {w16(league.object + 0x3e, 0xfff0)}},
    };
    for (const auto& [name, memory] : real_edits)
        cases.push_back({.entry = contenders, .registers = {{3, a}, {4, b}}, .memory = memory, .label = join("contenders", league, name)});
}

COMP_UTIL_CASES("club_queries") {
    // 0x10464160 on scratch states: a copy of a real league with chosen size, promotion places and virtual answers,
    // two statistics rows with chosen games and figures, and two clubs of that league. Figures are mostly near each
    // other (the ratings are weighted averages), so that every threshold is crossed both ways.
    const World w(scenario);
    const auto& league = w.leagues[w.leagues.size() / 2];
    std::mt19937 random(20261120);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + random() % values.size()); };
    const std::uint32_t answers[] = {0, 1, 0x100, 0xff, 0x80, 0x7f};
    for (unsigned n = 0; n < 4000; ++n) {
        const auto clubs = 2 + random() % 38;
        const auto automatic = pick({0, 0, 1, 2, 3, 4, 0xff, 0x80});
        const auto playoff = pick({0, 0, 1, 2, 4, 0xff, 0x7f});
        const auto base = random() % (clubs + 2);
        const auto games_first = pick({0, 1, 5, 12, 23, 24, 25, 26, 38, 255}), games_second = pick({0, 2, 7, 24, 25, 40, 255});
        const auto noise = [&]() { return random() % 4 == 0 ? random() % 60 : std::uint32_t(std::int32_t(base) + std::int32_t(random() % 5) - 2) & 0xff; };
        std::array<std::uint32_t, 4> first{}, second{};
        for (auto& figure : first) figure = noise();
        for (auto& figure : second) figure = noise();
        if (random() % 6 == 0) second = first;              // equal figures: ratings equal the bytes, up to rounding
        if (random() % 8 == 0) first.fill(first[0]), second.fill(first[0]);
        auto memory = scratch_league(scenario, league, pick({2, 2, 2, 2, 1}), pick({0xffffffffu, 0xffffffffu, 0xffffffffu, 0, 3}) , clubs, automatic, playoff);
        append(memory, stats_rows(games_first, first, games_second, second));
        append(memory, scratch_clubs(league.record, random() % 40 ? league.record : 0));
        Answers marks;
        if (random() % 30 == 0) marks.push_back({marks_record, random() % 2 ? 1u : 0x100u});
        cases.push_back({.entry = contenders, .registers = {{3, club_first}, {4, club_second}}, .memory = std::move(memory), .answers = std::move(marks),
                         .nth_answers = {{stats_row, 1, row_first}, {stats_row, 2, row_second},
                                         {dummy, 1, answers[random() % 6]}, {dummy, 2, answers[random() % 6]}, {dummy, 3, answers[random() % 6]}},
                         .label = join("contenders scratch", league, std::to_string(n) + " clubs " + std::to_string(clubs))});
    }
}

COMP_UTIL_CASES("club_queries") {
    // 0x10464160 where the ratings sit on a threshold: every figure of both clubs equal (or one apart), so a rating
    // is the figure up to the rounding of the weighted average, and the tests "figure 77 within the other club's
    // rating" depend on the last bit. The early-season weights (fewer than 25 games) are not exactly
    // representable, so fused and separately rounded multiply-adds give different ratings in some of these.
    const World w(scenario);
    const auto& league = w.leagues[w.leagues.size() / 3];
    std::mt19937 random(7);
    for (unsigned n = 0; n < 6000; ++n) {
        const auto clubs = 6 + random() % 35;
        const auto figure = random() % (clubs / 2 + 2);
        std::array<std::uint32_t, 4> first{figure, figure, figure, figure}, second = first;
        if (random() % 3 == 0) second[random() % 4] += random() % 3 - 1;       // one figure of the second club one apart
        if (random() % 3 == 0) first[random() % 4] += random() % 3 - 1;
        second[1] = first[1];                                                   // figure 76: the difference test passes
        auto memory = scratch_league(scenario, league, 2, -1, clubs, random() % 5, random() % 3);
        append(memory, stats_rows(random() % 26, first, random() % 26, second));
        append(memory, scratch_clubs(league.record, league.record));
        cases.push_back({.entry = contenders, .registers = {{3, club_first}, {4, club_second}}, .memory = std::move(memory),
                         .nth_answers = {{stats_row, 1, row_first}, {stats_row, 2, row_second}, {dummy, 1, 0}, {dummy, 2, random() % 2}, {dummy, 3, 1}},
                         .label = join("contenders on a threshold", league, std::to_string(n))});
    }
}
}
}

// LEAGUE.CPP, block `rollover`: cases for the daily update, the season's end and the club moves, from the real
// career (--world): the game's own league objects, standings rows, clubs and competition records. What the career
// lacks (a row's outcome, a date, a flag) is written over real values in a case's own memory (undone after each
// case); other modules' answers are given only where the career has no state that reaches the path.
#include "league_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <random>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t prize_money = 0x1049f320, move_club = 0x1049f780, set_division = 0x1049f8e0,
                        announce = 0x104a0ab0, daily = 0x104a1540, season_end = 0x104a1c30, finished = 0x104a8f40,
                        chain = 0x104a9160, exchange = 0x104a9210;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8,
                       club_count_slot = -0x7b50;
constexpr std::uint32_t object_size = 0xec, club_size = 0x248, row_size = 0x44;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write w16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;  // every league object the career has (competitions and their stages)
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) { if (object && is_league(object) && seen.insert(object).second) leagues.push_back(object); };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    std::uint32_t u8(std::uint32_t at) const { return (s.r32(at & ~3u) >> (24 - 8 * (at & 3))) & 0xff; }
    std::int32_t i8(std::uint32_t at) const { return std::int8_t(u8(at)); }
    std::int32_t i16(std::uint32_t at) const { return std::int16_t(u8(at) << 8 | u8(at + 1)); }
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || u8(object + 0x43) != 2) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::int32_t clubs(std::uint32_t league) const { return i16(league + 0x3e); }
    std::uint32_t rows(std::uint32_t league) const { return s.r32(league + 0xb0); }
    std::uint32_t row(std::uint32_t league, std::uint32_t index) const { return rows(league) + index * row_size; }
    std::uint32_t club(std::uint32_t league, std::uint32_t index) const { return s.r32(row(league, index)); }
    std::uint32_t record(std::uint32_t league) const { return s.r32(league + 4); }
    std::uint32_t table() const { return s.global(competitions_slot); }
    std::uint32_t club_table() const { return s.global(clubs_slot); }
    std::uint32_t club_count() const { return s.global(club_count_slot); }
    std::uint32_t club_at(unsigned index) const { return club_table() + index * club_size; }
    // The leagues with standings of at least `minimum` clubs.
    std::vector<std::uint32_t> with_rows(std::int32_t minimum) const {
        std::vector<std::uint32_t> found;
        for (const auto league : leagues) if (rows(league) && clubs(league) >= minimum) found.push_back(league);
        return found;
    }
};
std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) writes.push_back(w32(at + offset, s.r32(object + offset)));
    return writes;
}

LEAGUE_CASES("rollover: prize") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.leagues.empty()) throw std::runtime_error("The career has no league object");
    const auto leagues = w.with_rows(1);
    const auto table_cell = scenario.toc(competitions_slot);
    for (std::size_t n = 0; n < leagues.size() && n < 40; ++n)
        cases.push_back({.entry = prize_money, .registers = {{3, leagues[n]}}, .label = "rollover: prize real " + hex(leagues[n])});
    const auto copy = scratch_base + 0x2000;
    for (std::size_t n = 0; n < leagues.size() && n < 6; ++n) {
        const auto base = object_copy(scenario, copy, leagues[n]);
        const auto count = w.clubs(leagues[n]);
        const auto label = [&](const std::string& what) { return "rollover: prize " + what + " of " + hex(leagues[n]); };
        for (const auto kind : {0u, 1u, 2u, 3u, 4u, 0xffu})
            cases.push_back({.entry = prize_money, .registers = {{3, copy}}, .memory = joined(base, {w8(copy + 0xe1, kind)}),
                             .label = label("kind " + hex(kind))});
        // The pool: nothing, one share, around the news threshold, the largest and a negative one.
        for (const auto pool : {0u, 1u, 55u, 1000u, 1001u, 100000u, 0x7fffffffu, 0x80000000u, 0xfffffff0u})
            cases.push_back({.entry = prize_money, .registers = {{3, copy}}, .memory = joined(base, {w8(copy + 0xe1, 0), w32(copy + 0x28, pool)}),
                             .label = label("pool " + hex(pool))});
        for (const auto clubs : {0, -1, 1, 2, 7, 8, 9, 12, count})
            if (clubs <= count)
                cases.push_back({.entry = prize_money, .registers = {{3, copy}},
                                 .memory = joined(base, {w8(copy + 0xe1, 0), w32(copy + 0x28, 50000), w16(copy + 0x3e, std::uint32_t(clubs))}),
                                 .label = label("clubs " + std::to_string(clubs))});
        // Kind 3 asks the competition's own object; the assertion when the table of competitions is missing.
        cases.push_back({.entry = prize_money, .registers = {{3, copy}}, .memory = joined(base, {w8(copy + 0xe1, 3), w32(table_cell, 0)}),
                         .label = label("kind 3 without the table")});
    }
}

// A competition id's slot in the table of competition objects (the word a division's id indexes).
std::uint32_t slot_of(const World& w, std::uint32_t record) { return w.table() + w.s.r32(record) * 4; }

LEAGUE_CASES("rollover: moves") {
    if (scenario.match) return;
    const World w(scenario);
    const auto leagues = w.with_rows(2);
    if (leagues.size() < 4) throw std::runtime_error("The career has too few leagues with standings");
    const auto table_cell = scenario.toc(competitions_slot);
    std::vector<std::uint32_t> clubs;
    for (unsigned n = 0; n < 5; ++n) clubs.push_back(w.club_at(n));
    for (std::size_t n = 0; n < leagues.size() && n < 12; n += 3) clubs.push_back(w.club(leagues[n], 0));
    std::vector<std::uint32_t> divisions = {0};
    for (std::size_t n = 0; n < leagues.size() && n < 9; n += 2) divisions.push_back(w.record(leagues[n]));
    // 0x1049f780 and 0x1049f8e0: every club into every division.
    for (const auto club : clubs)
        for (const auto division : divisions) {
            const auto name = hex(club) + " into " + hex(division);
            cases.push_back({.entry = set_division, .registers = {{3, leagues[0]}, {4, club}, {5, division}}, .label = "rollover: moves set " + name});
            for (const auto flag : {0u, 1u, 0x100u, 0xffu})
                cases.push_back({.entry = move_club, .registers = {{3, leagues[0]}, {4, club}, {5, division}, {6, flag}},
                                 .label = "rollover: moves move " + name + " flag " + hex(flag)});
        }
    // The branches' inputs written over real values: the club's flags, the competition object's two halfwords.
    const auto club = clubs.front();
    for (std::size_t n = 0; n < leagues.size() && n < 8; ++n) {
        const auto object = leagues[n], record = w.record(object), slot = slot_of(w, record);
        const auto label = [&](const std::string& what) { return "rollover: moves variant " + what + " " + hex(object); };
        const std::vector<std::pair<std::string, std::vector<Write>>> variants = {
            {"object halfwords", {w16(object + 0xe2, 3), w16(object + 0xe4, 9)}},
            {"object halfword -1", {w16(object + 0xe2, 0xffff), w16(object + 0xe4, 9)}},
            {"club no nation", {w32(club + 0x3c, 0), w16(object + 0xe2, 3)}},
            {"club other nation", {w32(club + 0x3c, scenario.r32(w.club_at(3) + 0x3c)), w16(object + 0xe2, 3), w16(object + 0xe4, 0x7fff)}},
            {"transfer state", {w32(club + 0x90, scenario.r32(w.club_at(1) + 0x90))}},
            {"club byte 3", {w8(club + 0x23d, 3), w16(object + 0xe2, 3)}},
            {"club byte 1", {w8(club + 0x23d, 1), w16(object + 0xe2, 3)}},
            {"object missing", {w32(slot, 0)}},
            {"object other kind", {w8(object + 0x43, 1), w16(object + 0xe2, 3)}},
            {"no table", {w32(scenario.global(competitions_slot) ? table_cell : table_cell, 0)}},
        };
        for (const auto& [name, memory] : variants) {
            for (const auto flag : {0u, 1u})
                cases.push_back({.entry = move_club, .registers = {{3, object}, {4, club}, {5, record}, {6, flag}}, .memory = memory,
                                 .label = label("move " + name + " flag " + hex(flag))});
            cases.push_back({.entry = set_division, .registers = {{3, object}, {4, club}, {5, record}}, .memory = memory, .label = label("set " + name)});
            cases.push_back({.entry = set_division, .registers = {{3, object}, {4, club}, {5, record}}, .memory = memory,
                             .answers = {{0x1014c2e0, 1}}, .label = label("set announced " + name)});
        }
    }
}

// Writes that set the outcome byte (+0x40) of standings rows of `league`.
std::vector<Write> outcomes(const World& w, std::uint32_t league, const std::vector<std::pair<std::int32_t, std::uint32_t>>& rows) {
    std::vector<Write> writes;
    const auto count = w.clubs(league);
    for (const auto& [index, value] : rows) {
        const auto row = index < 0 ? count + index : index;
        if (row >= 0 && row < count) writes.push_back(w8(w.row(league, std::uint32_t(row)) + 0x40, value));
    }
    return writes;
}

LEAGUE_CASES("rollover: exchange") {
    if (scenario.match) return;
    const World w(scenario);
    const auto leagues = w.with_rows(4);
    if (leagues.size() < 6) throw std::runtime_error("The career has too few leagues with standings");
    // Pairs of distinct leagues: `from` is the lower division, `to` the upper (the chain's order).
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;
    for (std::size_t n = 0; n + 1 < leagues.size() && pairs.size() < 4; n += 3) pairs.emplace_back(leagues[n], leagues[n + 1]);
    struct Layout { const char* name; std::vector<std::pair<std::int32_t, std::uint32_t>> from, to; };
    const std::vector<Layout> layouts = {
        {"two up two down", {{0, 3}, {1, 3}}, {{-1, 5}, {-2, 0}}},
        {"two up one down", {{0, 3}, {2, 3}}, {{-1, 0}}},
        {"one up two down", {{1, 3}}, {{-1, 5}, {-3, 5}}},
        {"three up two down", {{0, 3}, {1, 3}, {3, 3}}, {{0, 0}, {-1, 5}}},
        {"none", {}, {}},
        {"others", {{0, 1}, {1, 2}, {2, 0xff}, {3, 5}}, {{0, 3}, {1, 1}, {2, 0xfe}, {3, 0xfc}}},
        {"down only", {{0, 0xff}}, {{0, 0}, {1, 5}, {2, 0}}},
    };
    const std::pair<std::uint32_t, std::uint32_t> expectations[] = {
        {0xffffffffu, 0xffffffffu}, {2, 2}, {1, 2}, {2, 1}, {3, 3}, {0, 0}, {0x1ff, 0x102}, {0xff, 2}};
    for (const auto& [from, to] : pairs) {
        for (const auto& layout : layouts) {
            const auto memory = joined(outcomes(w, from, layout.from), outcomes(w, to, layout.to));
            for (const auto announce : {0u, 1u})
                for (const auto pairwise : {0u, 1u})
                    for (const auto& [down, up] : expectations) {
                        // The full set of expectations only for the first layouts; the others take two.
                        if (&layout - layouts.data() > 1 && &down != &expectations[0].first && down != 2 && down != 3) continue;
                        cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, from}, {5, to}, {6, announce}, {7, pairwise}, {8, down}, {9, up}},
                                         .memory = memory,
                                         .label = std::string("rollover: exchange ") + layout.name + " " + hex(from) + " to " + hex(to) + " announce " +
                                                  std::to_string(announce) + " pairwise " + std::to_string(pairwise) + " expect " + hex(down) + "," + hex(up)});
                    }
        }
    }
    // The flag registers' upper bytes are ignored (extsb of the low byte).
    const auto& [from, to] = pairs[0];
    const auto memory = joined(outcomes(w, from, {{0, 3}}), outcomes(w, to, {{-1, 0}}));
    for (const auto flag : {0x100u, 0x180u, 0xffffff00u})
        cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, from}, {5, to}, {6, flag}, {7, flag}, {8, 0xffffffffu}, {9, 0xffffffffu}},
                         .memory = memory, .label = "rollover: exchange flags " + hex(flag)});
    // A division whose competition object is missing, and a division without a record: the move's news path.
    const auto& pair = pairs[1];
    for (const auto pairwise : {0u, 1u}) {
        const auto base = joined(outcomes(w, pair.first, {{0, 3}}), outcomes(w, pair.second, {{-1, 0}}));
        cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, pair.first}, {5, pair.second}, {6, 0}, {7, pairwise}, {8, 1}, {9, 1}},
                         .memory = joined(base, {w32(slot_of(w, w.record(pair.second)), 0)}),
                         .label = "rollover: exchange object missing pairwise " + std::to_string(pairwise)});
        cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, pair.first}, {5, pair.second}, {6, 0}, {7, pairwise}, {8, 1}, {9, 1}},
                         .memory = joined(base, {w32(pair.second + 4, 0)}),
                         .label = "rollover: exchange no record pairwise " + std::to_string(pairwise)});
        cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, pair.first}, {5, pair.second}, {6, 0}, {7, pairwise}, {8, 1}, {9, 1}},
                         .memory = joined(base, {w32(scenario.toc(competitions_slot), 0)}),
                         .label = "rollover: exchange no table pairwise " + std::to_string(pairwise)});
    }
}

LEAGUE_CASES("rollover: chain") {
    if (scenario.match) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.leagues.size(); ++n) {
        const auto league = w.leagues[n];
        const auto parent = std::int32_t(scenario.r32(league + 0x1c)), next = std::int32_t(scenario.r32(league + 0x20));
        if (n >= 30 && parent >= 0 && next < 0) continue;
        for (const auto argument : {0u, 1u})
            cases.push_back({.entry = chain, .registers = {{3, league}, {4, argument}},
                             .label = "rollover: chain real " + hex(league) + " argument " + std::to_string(argument)});
    }
}

// Every row of `league` as played: the played byte (+0x14) set to the league's matches per club.
std::vector<Write> all_played(const World& w, std::uint32_t league) {
    std::vector<Write> writes;
    const auto limit = w.i16(league + 0xa4) == -1 ? (w.clubs(league) - 1) * w.i16(league + 0x3c) : w.i16(league + 0xa4);
    for (std::int32_t n = 0; n < w.clubs(league); ++n) writes.push_back(w8(w.row(league, std::uint32_t(n)) + 0x14, std::uint32_t(limit)));
    return writes;
}

LEAGUE_CASES("rollover: finished") {
    if (scenario.match) return;
    const World w(scenario);
    const std::pair<std::uint32_t, std::uint32_t> modes[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {2, 1}, {0x101, 0x100}};
    for (std::size_t n = 0; n < w.leagues.size() && n < 24; ++n)
        for (const auto& [mode, finish] : modes)
            cases.push_back({.entry = finished, .registers = {{3, w.leagues[n]}, {4, mode}, {5, finish}},
                             .label = "rollover: finished real " + hex(w.leagues[n]) + " mode " + hex(mode) + " finish " + hex(finish)});
    // Leagues of the career by shape: with stages, a stage of another (owner 0), a chained one.
    std::vector<std::uint32_t> picked;
    for (const auto league : w.leagues) {
        if (!w.rows(league) || w.clubs(league) < 2) continue;
        const bool staged = std::int32_t(scenario.r32(league + 0x30)) >= 0, owned = w.i8(league + 0x44) != -1;
        if (picked.size() < 3 || (staged && picked.size() < 7) || (owned && picked.size() < 9)) picked.push_back(league);
    }
    for (const auto league : picked) {
        const auto rows = w.rows(league);
        const auto limit = w.i16(league + 0xa4) == -1 ? (w.clubs(league) - 1) * w.i16(league + 0x3c) : w.i16(league + 0xa4);
        const auto played = all_played(w, league);
        const auto label = [&](const std::string& what, std::uint32_t mode, std::uint32_t finish) {
            return "rollover: finished " + what + " " + hex(league) + " mode " + hex(mode) + " finish " + hex(finish);
        };
        const std::vector<std::pair<std::string, std::vector<Write>>> variants = {
            {"as is", {}},
            {"no rows", {w32(league + 0xb0, 0)}},
            {"stopped", {w8(league + 0xb4, 1)}},
            {"inactive", {w8(league + 0x45, 0)}},
            {"owned, done", {w8(league + 0x44, 0), w8(league + 0xe0, 1)}},
            {"owned, open", {w8(league + 0x44, 0), w8(league + 0xe0, 0)}},
            {"done flag", {w8(league + 0xe0, 1)}},
            {"count unset", {w16(league + 0xa4, 0xffff)}},
            {"count unset, one club", {w16(league + 0xa4, 0xffff), w16(league + 0x3e, 1)}},
            {"count unset, no clubs", {w16(league + 0xa4, 0xffff), w16(league + 0x3e, 0)}},
            {"count unset, played", joined({w16(league + 0xa4, 0xffff)}, played)},
            {"first row short", joined(played, {w8(rows + 0x14, std::uint32_t(limit - 1))})},
            {"last row short", joined(played, {w8(w.row(league, std::uint32_t(w.clubs(league) - 1)) + 0x14, std::uint32_t(limit - 1))})},
            {"played", played},
            {"played, prize done", joined(played, {w8(league + 0xe6, 1)})},
            {"played, 9 rounds", joined(played, {w16(league + 0xa6, 9)})},
            {"played, 9 rounds, prize done", joined(played, {w16(league + 0xa6, 9), w8(league + 0xe6, 1)})},
            {"played, kind 2", joined(played, {w8(league + 0xe1, 2), w16(league + 0xa6, 9)})},
            {"played, kind 3", joined(played, {w8(league + 0xe1, 3), w16(league + 0xa6, 9)})},
            {"played, kind 3 no table", joined(played, {w8(league + 0xe1, 3), w32(scenario.toc(competitions_slot), 0)})},
            {"played, other kind", joined(played, {w8(league + 0xe1, 0)})},
            {"played, stages more", joined(played, {w32(league + 0x2c, 100)})},
            {"played, stages none", joined(played, {w32(league + 0x2c, 0)})},
            {"played, stages exact", joined(played, {w32(league + 0x2c, std::uint32_t(std::int32_t(scenario.r32(league + 0x30)) + 1))})},
        };
        for (const auto& [name, memory] : variants)
            for (const auto& [mode, finish] : modes)
                for (const auto national : {2u, 0u, 1u}) {
                    // national: 2 the real answer, else 0x104646d0 answered 0 or 1.
                    if (national != 2 && !(mode == 1 && finish == 1) && !(mode == 0 && finish == 0)) continue;
                    Case item{.entry = finished, .registers = {{3, league}, {4, mode}, {5, finish}}, .memory = memory,
                              .label = label(name + (national == 2 ? "" : national ? " national" : " not national"), mode, finish)};
                    if (national != 2) item.answers = {{0x104646d0, national}};
                    cases.push_back(std::move(item));
                }
    }
}

// A date (day halfword, year halfword, byte) as the game keeps it.
std::vector<Write> date_at(std::uint32_t at, std::uint32_t day, std::uint32_t year, std::uint32_t extra = 0) {
    return {w16(at, day), w16(at + 2, year), w8(at + 4, extra)};
}

LEAGUE_CASES("rollover: daily") {
    if (scenario.match) return;
    const World w(scenario);
    const auto today = scenario.toc(-0x7b80);
    const auto given = scratch_base + 0x8000;
    const auto day = w.u8(today) << 8 | w.u8(today + 1), year = w.u8(today + 2) << 8 | w.u8(today + 3);
    const auto descriptors = scenario.toc(-0x7b44);
    std::vector<std::uint32_t> picked;
    for (const auto league : w.leagues) {
        const bool staged = std::int32_t(scenario.r32(league + 0x30)) >= 0;
        if (w.rows(league) && (picked.size() < 4 || (staged && picked.size() < 8) || (w.i8(league + 0x44) != -1 && picked.size() < 10)))
            picked.push_back(league);
    }
    for (std::size_t n = 0; n < w.leagues.size() && n < 12; ++n)
        for (const auto active : {1u, 0u})
            cases.push_back({.entry = daily, .registers = {{3, w.leagues[n]}, {4, given}, {5, active}},
                             .memory = date_at(given, std::uint32_t(day), std::uint32_t(year)),
                             .label = "rollover: daily real " + hex(w.leagues[n]) + " active " + std::to_string(active)});
    for (const auto league : picked) {
        const auto rows = w.rows(league), schedule = scenario.r32(league + 0xbc);
        const auto sched_day = schedule ? std::uint32_t(w.u8(schedule) << 8 | w.u8(schedule + 1)) : 100u;
        const auto name = hex(league);
        // Date relations: the given date against today, the last-update date, the start date.
        for (const auto given_is_today : {1, 0})
            for (const auto last_is_given : {0, 1})
                for (const auto start_is_today : {0, 1})
                    for (const auto finished : {0u, 1u})
                        for (const auto active : {1u, 0u}) {
                            const auto d = given_is_today ? std::uint32_t(day) : std::uint32_t(day) + 7;
                            auto memory = joined(date_at(given, d, std::uint32_t(year)),
                                                 last_is_given ? date_at(league + 0xd4, d, std::uint32_t(year)) : date_at(league + 0xd4, d - 1, std::uint32_t(year)));
                            memory = joined(memory, start_is_today ? date_at(league + 0xcc, std::uint32_t(day), std::uint32_t(year)) : date_at(league + 0xcc, 0x28, 0x7b4));
                            memory.push_back(w8(league + 0xe0, finished));
                            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, active}}, .memory = memory,
                                             .label = "rollover: daily dates " + name + (given_is_today ? " today" : " other") + (last_is_given ? " same-last" : " new-last") +
                                                      (start_is_today ? " start" : " nostart") + (finished ? " finished" : " open") + " active " + std::to_string(active)});
                        }
        // The first day of the competition: the day after a descriptor's day, the first row unplayed.
        for (std::uint32_t row = 0; row < 34; ++row) {
            const auto desc_day = (std::uint32_t(w.u8(descriptors + row * 0x4c + 0x16) << 8 | w.u8(descriptors + row * 0x4c + 0x17)) + 1) & 0xffff;
            auto memory = joined(date_at(given, std::uint32_t(day), std::uint32_t(year)), date_at(today, desc_day > 0x16c ? 0 : desc_day, std::uint32_t(w.i16(league + 0x40))));
            memory.push_back(w8(rows + 0x14, 0));
            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, 1}}, .memory = memory,
                             .label = "rollover: daily first day " + name + " row " + std::to_string(row)});
        }
        // The stages' end: the league's flags, the stages open or done.
        for (const auto flag : {0u, 1u}) for (const auto stages : {0u, 1u, 3u}) for (const auto notified : {0u, 1u}) for (const auto prize : {0u, 1u}) {
            auto memory = joined(date_at(given, std::uint32_t(day), std::uint32_t(year)), date_at(league + 0xd4, std::uint32_t(day) - 1, std::uint32_t(year)));
            memory.push_back(w8(league + 0xe0, flag));
            memory.push_back(w32(league + 0x2c, stages));
            memory.push_back(w8(league + 0x49, notified));
            memory.push_back(w8(league + 0xe6, prize));
            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, 1}}, .memory = memory,
                             .label = "rollover: daily end " + name + " flag " + std::to_string(flag) + " stages " + std::to_string(stages) +
                                      " notified " + std::to_string(notified) + " prize " + std::to_string(prize)});
        }
        // The season-start news: the flag bit, the schedule's first day shifted by 5, 20 or 14 days.
        for (const auto shift : {5u, 0x14u, 0xeu}) for (const auto kind : {0, 1, 2}) {
            const auto record = w.record(league);
            const auto stats = record ? scenario.r32(record + 8) : 0u;
            std::vector<Write> memory = date_at(given, sched_day - shift, std::uint32_t(w.i16(league + 0x40)));
            memory = joined(memory, date_at(league + 0xd4, 1, 1));
            memory = joined(memory, {w8(league + 0x4c, 2), w8(league + 0xe0, 0), w8(rows + 0x14, 0)});
            if (stats && kind) memory.push_back(w32(scenario.toc(kind == 1 ? -0x7508 : -0x7a14), scenario.r32(stats + 0x68)));
            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, 1}}, .memory = memory,
                             .label = "rollover: daily news " + name + " shift " + hex(shift) + " kind " + std::to_string(kind)});
        }
        for (const auto bits : {0u, 2u, 3u}) {
            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, 1}},
                             .memory = joined(date_at(given, std::uint32_t(day), std::uint32_t(year)), {w8(league + 0x4c, bits), w8(league + 0xb4, bits & 1), w8(league + 0x45, (bits & 2) ? 1 : 0), w16(league + 0xa4, 0xffff)}),
                             .label = "rollover: daily early " + name + " bits " + std::to_string(bits)});
        }
    }
}

LEAGUE_CASES("rollover: season") {
    if (scenario.match) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.leagues.size() && n < 30; ++n)
        cases.push_back({.entry = season_end, .registers = {{3, w.leagues[n]}}, .label = "rollover: season real " + hex(w.leagues[n])});
    std::vector<std::uint32_t> picked;
    for (const auto league : w.leagues)
        if (w.rows(league) && w.clubs(league) >= 4 && w.clubs(league) <= 24 && picked.size() < 5) picked.push_back(league);
    std::mt19937 random(0x5eed);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + random() % values.size()); };
    for (const auto league : picked) {
        const auto name = hex(league);
        const std::int32_t clubs = w.clubs(league);
        // The allocation failure and the already-finished league.
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .answers = {{0x10775130, 0}}, .label = "rollover: season no memory " + name});
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = {w8(league + 0xe0, 1)}, .label = "rollover: season done " + name});
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = {w16(league + 0xa4, 0xffff)}, .label = "rollover: season count unset " + name});
        for (unsigned variant = 0; variant < 140; ++variant) {
            std::vector<Write> memory;
            const auto limit = w.i16(league + 0xa4) > 0 ? w.i16(league + 0xa4) : 10;
            const unsigned flags = pick({0x0001, 0x0003, 0x0005, 0x0009, 0x0023, 0x0021, 0x0020, 0x0025, 0x002b, 0x0000, 0x0002, 0x0004, 0x0008, 0x0029});
            memory.push_back(w16(league + 0xdc, flags));
            for (unsigned k = 0; k < 4; ++k) memory.push_back(w8(league + 0xc0 + k, pick({0, 1, 2, 3, 4, 0, 1})));
            memory.push_back(w8(league + 0xc4, pick({0, 2, 3, 3})));
            memory.push_back(w8(league + 0xe6, pick({0, 1})));
            const auto profile = variant % 7;  // 0 fresh, 1 mid-season, 2..5 finished with ties and different places, 6 played beyond the count
            std::vector<std::int32_t> order(static_cast<std::size_t>(clubs), 0);
            for (std::int32_t k = 0; k < clubs; ++k) order[std::size_t(k)] = k;
            std::shuffle(order.begin(), order.end(), random);
            for (std::int32_t k = 0; k < clubs; ++k) {
                const auto row = w.row(league, std::uint32_t(k));
                const auto played = profile == 0 ? 0 : profile == 1 ? std::uint32_t(random() % std::uint32_t(limit + 1)) : profile == 6 ? std::uint32_t(limit + int(random() % 4)) : std::uint32_t(limit);
                const auto points = profile >= 2 && profile <= 3 ? std::uint32_t(order[std::size_t(k)]) * 3 + (random() % 2 ? 0 : 1) : std::uint32_t(random() % 60);
                memory.push_back(w8(row + 0x14, played));
                memory.push_back(w16(row + 0x1c, points));
                // Place and best place: equal (skipped by the recount), differing, or out of range.
                const auto place = profile == 2 ? std::uint32_t(k + 1) : std::uint32_t(random() % std::uint32_t(clubs + 2));
                const auto best = profile == 2 ? std::uint32_t(k + 1) : pick({place, place, std::uint32_t(random() % std::uint32_t(clubs + 2))});
                memory.push_back(w8(row + 0x42, place));
                memory.push_back(w8(row + 0x43, best));
                memory.push_back(w8(row + 0x40, pick({0xff, 0xff, 0xff, 0, 1, 2, 3, 5, 0xfe, 0xfc, 6, 7, 0xff})));
            }
            if (variant % 7 == 0) memory.push_back(w16(league + 0xa4, 0xffff));
            cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = memory,
                             .label = "rollover: season random " + name + " " + std::to_string(variant)});
        }
    }
}

LEAGUE_CASES("rollover: announce") {
    if (scenario.match) return;
    const World w(scenario);
    const auto area = scratch_base + 0x20000;
    std::vector<std::uint32_t> picked;
    for (const auto league : w.leagues)
        if (w.rows(league) && w.i8(league + 0x44) == -1 && w.clubs(league) >= 3 && scenario.r32(league + 0xbc) && picked.size() < 7) picked.push_back(league);
    for (const auto league : picked)
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, 0}}, .label = "rollover: announce real " + hex(league)});
    // The draw's matches, built when the lookup 0x1048b4e0 runs (its list sits at sp-0x1b0+0x80 of the checked call).
    const auto draw_list = scenario.sp() - 0x1b0 + 0x80;
    const auto matches = [&](std::uint32_t league, std::uint32_t home_club) {
        std::vector<Write> memory = {w32(area, area + 0x100), w32(area + 8, area + 0x10), w32(area + 0x10, area + 0x140), w32(area + 0x18, 0),
                                     w32(area + 0x100 + 0x1c, home_club), w32(area + 0x100 + 0x20, area + 0x200), w16(area + 0x100 + 0x28, 3), w16(area + 0x100 + 0x2a, 4),
                                     w32(area + 0x140 + 0x1c, area + 0x240), w32(area + 0x140 + 0x20, area + 0x200), w16(area + 0x140 + 0x28, 5), w16(area + 0x140 + 0x2a, 6),
                                     w32(area + 0x200, scenario.r32(w.club(league, 2))), w32(area + 0x240, scenario.r32(w.club(league, 3)))};
        return memory;
    };
    for (const auto league : picked) {
        const auto name = hex(league);
        const auto first = w.club(league, 0), second = w.club(league, 1);
        const auto poke = Case::Poke{0x1048b4e0, {w32(draw_list + 4, area), w32(draw_list + 8, area + 0x10)}};
        const auto label = [&](const char* what) { return "rollover: announce " + std::string(what) + " " + name; };
        for (const auto involved : {0u, 1u})
            for (const auto drawn : {0u, 3u, 0x10003u}) {
                Case base{.entry = announce, .registers = {{3, league}, {4, 0}}, .memory = matches(league, first),
                          .answers = {{0x104646d0, 1}, {0x102cdbf0, involved}, {0x1048b4e0, drawn}},
                          .label = label(("national involved " + std::to_string(involved) + " drawn " + hex(drawn)).c_str())};
                cases.push_back(base);
                if (drawn == 3 && involved) {
                    base.pokes = {poke};
                    // The news item's field setter stops on a hand-built match (its field types come from the real draw).
                    base.answers.push_back({0x102cb4b0, 0}); base.answers.push_back({0x102cc140, 0});
                    base.label = label("matches home first");
                    cases.push_back(base);
                    base.memory = matches(league, second);
                    base.label = label("matches home second");
                    cases.push_back(base);
                    // A club listed twice: the second row finds the first in the seen list.
                    base.memory = joined(matches(league, first), {w32(w.row(league, 1), first)});
                    base.label = label("matches duplicate row");
                    cases.push_back(base);
                }
            }
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, 0}}, .answers = {{0x104646d0, 0}}, .label = label("not national")});
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, 0}}, .memory = {w8(league + 0x44, 0)}, .answers = {{0x104646d0, 1}}, .label = label("owned")});
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, area + 0x400}}, .memory = {w8(league + 0x44, 0), w32(area + 0x400, 0), w32(area + 0x404, 0), w32(area + 0x408, 0), w32(area + 0x40c, 0)},
                         .answers = {{0x104646d0, 1}}, .label = label("owned with list")});
    }
}

// Cases added after the first coverage run: the outcomes the first lists did not reach.
LEAGUE_CASES("rollover: more") {
    if (scenario.match) return;
    const World w(scenario);
    const auto leagues = w.with_rows(4);
    if (leagues.size() < 6) throw std::runtime_error("The career has too few leagues with standings");
    const auto today = scenario.toc(-0x7b80);
    const auto given = scratch_base + 0x8000, area = scratch_base + 0x30000;
    const auto day = std::uint32_t(w.u8(today) << 8 | w.u8(today + 1)), year = std::uint32_t(w.u8(today + 2) << 8 | w.u8(today + 3));
    // 0x1049f780 with the competition table null: 0x104ae570 (called first) reads it and stops the original, so that
    // outcome has no returning case (the 'no table' cases of the moves list fault alike on both sides).
    // The news answers of exchange.
    for (const auto pairwise : {0u, 1u})
        for (const auto answer : {1u, 0u}) {
            const auto from = leagues[1], to = leagues[2];
            const auto memory = joined(joined(outcomes(w, from, {{0, 3}}), outcomes(w, to, {{-1, 0}})), {w32(slot_of(w, w.record(to)), 0)});
            cases.push_back({.entry = exchange, .registers = {{3, leagues[0]}, {4, from}, {5, to}, {6, 0}, {7, pairwise}, {8, 1}, {9, 1}},
                             .memory = memory, .answers = {{0x1014c2e0, answer}},
                             .label = "rollover: more exchange announced " + std::to_string(answer) + " pairwise " + std::to_string(pairwise)});
        }
    // The daily update: a league without clubs, without rows, with a stage of another kind, a date's other year, a
    // descriptor day that wraps, a record without a nation or without a record, the end of the stages announced.
    for (const auto league : {leagues[0], leagues[3]}) {
        const auto rows = w.rows(league), record = w.record(league);
        const auto name = hex(league);
        const auto base = date_at(given, day, year);
        const auto run = [&](const std::string& what, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            cases.push_back({.entry = daily, .registers = {{3, league}, {4, given}, {5, 1}}, .memory = joined(base, memory), .answers = std::move(answers),
                             .label = "rollover: more daily " + what + " " + name});
        };
        run("no clubs", {w16(league + 0xa4, 0xffff), w16(league + 0x3e, 0)});
        run("no rows", {w32(league + 0xb0, 0), w8(league + 0x4c, 2)});
        run("no rows first", {w32(league + 0xb0, 0)});
        run("record without nation", joined(date_at(league + 0xd4, day, year), {w32(record + 8, 0), w8(league + 0x4c, 2), w8(league + 0xe0, 0)}), {{0x104a28d0, 0}});
        run("no record", joined(date_at(league + 0xd4, day, year), {w32(league + 4, 0), w8(league + 0x4c, 2), w8(league + 0xe0, 0)}), {{0x104a28d0, 0}});
        run("tail nation zero", joined(date_at(league + 0xd4, day - 1, year), {w32(record + 8, 0), w8(league + 0x4c, 2), w8(league + 0xe0, 0), w8(rows + 0x14, 0)}), {{0x104a28d0, 0}});
        const auto stats = scenario.r32(record + 8);
        for (std::uint32_t row = 0; row < 34; ++row) {
            const auto at = scenario.toc(-0x7b44) + row * 0x4c + 0x16;
            run("descriptor wraps " + std::to_string(row), joined(date_at(today, 0, year), {w16(at, 0x16c), w8(rows + 0x14, 0)}));
            run("descriptor year " + std::to_string(row), joined(date_at(today, w.u8(at - 0) << 8 | w.u8(at + 1) + 1, year + 1), {w8(rows + 0x14, 0)}));
        }
        (void)stats;
        // The end of the stages, announced: every row played, no stages, the league's own virtual closes it.
        for (const auto national : {1u, 0u}) for (const auto prize : {0u, 1u}) for (const auto staged : {0u, 1u}) {
            auto memory = joined(all_played(w, league), date_at(league + 0xd4, day - 1, year));
            memory = joined(memory, {w16(league + 0xa6, 0), w8(league + 0xe0, 0), w8(league + 0x49, 0), w8(league + 0xe6, prize), w32(league + 0x2c, 0), w32(league + 0x30, staged ? 0 : 0xffffffffu)});
            run("end announced national " + std::to_string(national) + " prize " + std::to_string(prize) + " staged " + std::to_string(staged), memory,
                {{0x104646d0, national}});
        }
    }
    // A league's stage of another kind: the recursion skips it.
    for (const auto league : w.leagues) {
        const auto stages = scenario.r32(league + 0xc);
        if (std::int32_t(scenario.r32(league + 0x30)) < 0 || !stages || w.i8(league + 0x44) != -1) continue;
        const auto stage = scenario.r32(stages);
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, 0}}, .memory = {w8(stage + 0x43, 1)}, .answers = {{0x104646d0, 1}, {0x102cdbf0, 0}},
                         .label = "rollover: more announce stage of another kind " + hex(league)});
        break;
    }
    // A club listed twice whose id is its own address: the seen list finds it.
    for (const auto league : leagues) {
        if (w.i8(league + 0x44) != -1 || !scenario.r32(league + 0xbc)) continue;
        const auto first = w.club(league, 0);
        const auto draw_list = scenario.sp() - 0x1b0 + 0x80;
        std::vector<Write> memory = {w32(w.row(league, 1), first), w32(area, area + 0x100), w32(area + 8, 0),
                                     w32(area + 0x100 + 0x1c, first), w32(area + 0x100 + 0x20, area + 0x200), w16(area + 0x100 + 0x28, 3), w16(area + 0x100 + 0x2a, 4), w32(area + 0x200, scenario.r32(w.club(league, 2)))};
        cases.push_back({.entry = announce, .registers = {{3, league}, {4, 0}}, .memory = memory, .answers = {{0x104646d0, 1}, {0x102cdbf0, 1}, {0x1048b4e0, 3}},
                         .pokes = {{0x1048b4e0, {w32(draw_list + 4, area), w32(draw_list + 8, area)}}}, .label = "rollover: more announce seen club " + hex(league)});
        cases.back().answers.push_back({0x102cb4b0, 0}); cases.back().answers.push_back({0x102cc140, 0});
        break;
    }
    // The season's end: no clubs with the count unset, rows all excluded, counts above the clubs, and the places table.
    const auto place_cell = scenario.toc(-0x7984), kind_a_cell = scenario.toc(-0x7a1c), kind_b_cell = scenario.toc(-0x7a30 + 0x0 * 0);
    (void)kind_b_cell;
    const auto kind_b = scenario.toc(-0x7a70), places = scenario.toc(-0x7a48);
    (void)places;
    const auto table = scenario.r32(place_cell);
    for (const auto league : leagues) {
        const auto name = hex(league);
        const auto count = std::uint32_t(w.clubs(league));
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = {w16(league + 0xa4, 0xffff), w8(league + 0x45, 0)}, .label = "rollover: more season inactive " + name});
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = {w16(league + 0xa4, 0xffff), w16(league + 0x3e, 0)}, .label = "rollover: more season no clubs " + name});
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = {w16(league + 0xa4, 0xffff), w16(league + 0x3e, 1)}, .label = "rollover: more season one club " + name});
        std::vector<Write> excluded = {w16(league + 0xdc, 1)};
        for (std::uint32_t n = 0; n < count; ++n) excluded.push_back(w8(w.row(league, n) + 0x40, 0xfe));
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = excluded, .label = "rollover: more season all excluded " + name});
        std::vector<Write> large = {w8(league + 0xc0, 30), w8(league + 0xc1, 30), w8(league + 0xc2, 30), w8(league + 0xc3, 30)};
        cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = large, .label = "rollover: more season counts above the clubs " + name});
        // Places table: equal place and best place below 9, the record's id national, no parent, a free table entry.
        const auto record = w.record(league), stats = scenario.r32(record + 8);
        if (!table || !stats) continue;
        for (const auto bytes : {std::pair<std::uint32_t, std::uint32_t>{20, 3}, {1, 3}, {0, 3}, {0, 0}})
            for (const auto free_entry : {1u, 0u})
                for (const auto kind_match : {1u, 0u})
                  for (const auto national : {1u, 0u}) {
                    if (!national && (bytes.first != 20 || !free_entry || !kind_match)) continue;
                    std::vector<Write> memory = {w16(league + 0xdc, 0x20), w32(league + 0x1c, 0xffffffffu), w32(stats + 0x74, area), w32(area + 8, 0x1234),
                                                 w32(kind_a_cell, scenario.r32(stats + 0x68) + (kind_match ? 1 : 0)), w32(kind_b, kind_match ? 0x1234u : 0x4321u)};
                    for (std::uint32_t n = 0; n < count; ++n) {
                        const auto row = w.row(league, n);
                        memory.push_back(w8(row + 0x42, n + 1));
                        memory.push_back(w8(row + 0x43, n + 1));
                        memory.push_back(w8(row + 0x40, 0xff));
                        memory.push_back(w32(table + w.s.r32(w.club(league, n)) * 8, free_entry ? 0xffffffffu : 0));
                    }
                    cases.push_back({.entry = season_end, .registers = {{3, league}}, .memory = memory, .answers = {{0x104646d0, national}, {0x104b7e30, 0}},
                                     .pokes = {{0x104b7e30, {w8(scenario.sp() - 0xa0 + 0x39, bytes.first), w8(scenario.sp() - 0xa0 + 0x38, bytes.second)}}},
                                     .label = "rollover: more season places " + name + " bytes " + std::to_string(bytes.first) + "," + std::to_string(bytes.second) +
                                              " free " + std::to_string(free_entry) + " match " + std::to_string(kind_match) + " national " + std::to_string(national)});
                }
        break;
    }
}
}
}

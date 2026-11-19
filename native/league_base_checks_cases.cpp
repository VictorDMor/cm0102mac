// LEAGUE.CPP's pilot cases, from the real career (--world): the game's own competition objects (one per
// competition id, toc -0x7b5c) and their stages, of which the leagues are those whose vtable has the league's
// slots (+0x88..+0xb8) with LEAGUE.CPP's bodies in them. Objects a case needs that the career has no instance
// of (a fresh object, rows for the comparators, a league with other rules) are built in the scratch area from
// real values with one field changed.
#include "league_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t construct = 0x1049e4f0, destroy = 0x1049e600, unsupported_44 = 0x1049e970,
                        unsupported_94 = 0x1049ea10, unsupported_10 = 0x1049eaa0, standing = 0x1049ea00,
                        promotes = 0x104a14f0, relegates = 0x104a1520, by_word = 0x104aaaf0,
                        by_reputation = 0x104aab00, by_half_a = 0x104aab20;
constexpr std::uint32_t defaults[] = {0x1049e960, 0x104a60c0, 0x104a60d0};
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8,
                       club_count_slot = -0x7b50;
constexpr std::uint32_t object_size = 0xec;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;  // every league object the career has: competitions and their stages
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
    // The league's vtable slots +0x88..+0xb8 exist only in the league's and its derived classes' tables; a slot the
    // class does not override names LEAGUE.CPP's body.
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 2 /* the kind byte +0x43 */) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::uint32_t club_count() const { return s.global(club_count_slot); }
    std::uint32_t club(unsigned index) const { return s.global(clubs_slot) + index * 0x248; }
};
std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) writes.push_back({4, at + offset, s.r32(object + offset)});
    return writes;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

LEAGUE_CASES("object") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.leagues.empty()) throw std::runtime_error("The career has no league object");
    const auto fresh = scratch_base + 0x100;
    // A fresh object: the scratch bytes it does not write keep a pattern, so a stray store shows.
    std::vector<Write> pattern;
    for (std::uint32_t offset = 0; offset < 0x100; offset += 4) pattern.push_back({4, fresh + offset, 0xa5a5a5a5u + offset});
    cases.push_back({.entry = construct, .registers = {{3, fresh}}, .memory = pattern, .label = "object: construct fresh"});
    for (std::size_t n = 0; n < w.leagues.size() && n < 6; ++n)
        cases.push_back({.entry = construct, .registers = {{3, w.leagues[n]}}, .label = "object: construct over " + hex(w.leagues[n])});
    // The destructor: null; a copy without blocks or preset fixtures under each flag word that frees nothing;
    // real leagues with their blocks, keeping and freeing the object.
    cases.push_back({.entry = destroy, .registers = {{3, 0}, {4, 1}}, .label = "object: destroy null"});
    auto empty = object_copy(scenario, fresh, w.leagues.front());
    empty.push_back({4, fresh + 0x14, 0});
    empty.push_back({4, fresh + 0x18, 0});
    empty.push_back({4, fresh + 0xe8, 0});
    for (const auto flag : {0u, 0x10000u, 0xffffu, 0x8000u})
        cases.push_back({.entry = destroy, .registers = {{3, fresh}, {4, flag}}, .memory = empty,
                         .label = "object: destroy without blocks, flag " + hex(flag)});
    std::size_t taken = 0, presets = 0;
    for (const auto object : w.leagues) {
        const bool fixtures = scenario.r32(object + 0xe8) != 0;
        presets += fixtures;
        if (!fixtures && taken > 8) continue;
        for (const auto flag : {0u, 1u})
            cases.push_back({.entry = destroy, .registers = {{3, object}, {4, flag}},
                             .label = "object: destroy " + hex(object) + (fixtures ? " with preset fixtures" : "") + " flag " + hex(flag)});
        ++taken;
    }
    if (!presets) std::fprintf(stderr, "object: no league of the career has preset fixtures (+0xe8)\n");
    // Preset fixtures: the career has none (only the Italian leagues build them), so the array is stood in for
    // by another real league object, an allocation of `new` (0x10773520; the release 0x10773610 calls
    // `delete` 0x107735a0): +0xe8 points 16 bytes into it, past where the array's header would be.
    if (w.leagues.size() > 2) {
        const auto block = w.leagues[1];
        auto with_fixtures = joined(empty, {{4, fresh + 0xe8, block + 0x10}});
        for (const auto flag : {0u, 0x8000u})
            cases.push_back({.entry = destroy, .registers = {{3, fresh}, {4, flag}}, .memory = with_fixtures,
                             .label = "object: destroy a copy with preset fixtures, flag " + hex(flag)});
        for (const auto flag : {0u, 1u})
            cases.push_back({.entry = destroy, .registers = {{3, w.leagues[2]}, {4, flag}},
                             .memory = {{4, w.leagues[2] + 0xe8, block + 0x10}},
                             .label = "object: destroy " + hex(w.leagues[2]) + " with preset fixtures, flag " + hex(flag)});
    }
    std::fprintf(stderr, "object: %zu league objects\n", w.leagues.size());
    // The virtuals a derived league must override, and the one-instruction virtuals, on real leagues.
    for (std::size_t n = 0; n < w.leagues.size() && n < 2; ++n) {
        for (const auto entry : {unsupported_44, unsupported_94, unsupported_10})
            cases.push_back({.entry = entry, .registers = {{3, w.leagues[n]}, {4, 1}}, .label = "object: unsupported " + hex(entry)});
        for (const auto entry : defaults)
            cases.push_back({.entry = entry, .registers = {{3, w.leagues[n]}, {4, 1}}, .label = "object: default " + hex(entry)});
    }
}

LEAGUE_CASES("fields") {
    if (scenario.match) return;
    const World w(scenario);
    for (std::size_t n = 0; n < w.leagues.size(); ++n) {
        const auto object = w.leagues[n];
        cases.push_back({.entry = promotes, .registers = {{3, object}}, .label = "fields: promotes " + hex(object)});
        cases.push_back({.entry = relegates, .registers = {{3, object}}, .label = "fields: relegates " + hex(object)});
        if (n >= 12) continue;
        const auto clubs = std::uint32_t(std::int16_t(scenario.r32(object + 0x3c) & 0xffff));
        for (const auto index : {0u, 1u, clubs - 1, clubs, 0xffffffffu, 0x80000000u, 0x7fffffffu})
            cases.push_back({.entry = standing, .registers = {{3, object}, {4, index}}, .label = "fields: standing " + hex(index) + " of " + hex(object)});
    }
    if (w.leagues.empty()) return;
    // Every outcome of the two flags on a copy of a real league: the promotion byte and the flag word's bit 1
    // with its neighbours, and the relegation byte's signs.
    const auto copy = scratch_base + 0x300;
    const auto base = object_copy(scenario, copy, w.leagues.front());
    for (const auto places : {0u, 1u, 2u, 0x7fu, 0x80u, 0xffu})
        for (const auto flags : {0u, 1u, 2u, 3u, 0xfffdu, 0x8002u, 0xfffeu}) {
            cases.push_back({.entry = promotes, .registers = {{3, copy}}, .memory = joined(base, {{1, copy + 0xc0, places}, {2, copy + 0xdc, flags}}),
                             .label = "fields: promotes places " + hex(places) + " flags " + hex(flags)});
        }
    for (const auto places : {0u, 1u, 3u, 0x7fu, 0x80u, 0xffu})
        cases.push_back({.entry = relegates, .registers = {{3, copy}}, .memory = joined(base, {{1, copy + 0xc3, places}}),
                         .label = "fields: relegates places " + hex(places)});
}

LEAGUE_CASES("order") {
    if (scenario.match) return;
    const World w(scenario);
    const auto left = scratch_base + 0x400, right = scratch_base + 0x440;
    const std::pair<std::uint32_t, std::uint32_t> words[] = {
        {7, 3}, {3, 7}, {5, 5}, {0x80000000u, 1}, {1, 0x80000000u}, {0x7fffffffu, 0xffffffffu}, {0, 0xffffffffu}};
    for (const auto& [a, b] : words)
        cases.push_back({.entry = by_word, .registers = {{3, left}, {4, right}}, .memory = {{4, left, a}, {4, right, b}},
                         .label = "order: word " + hex(a) + " " + hex(b)});
    const std::pair<std::uint32_t, std::uint32_t> halves[] = {
        {30, 20}, {20, 30}, {12, 12}, {0xffff, 1}, {0x8000, 0x7fff}, {0x7fff, 0x8000}, {0, 0xffff}};
    for (const auto& [a, b] : halves)
        cases.push_back({.entry = by_half_a, .registers = {{3, left}, {4, right}},
                         .memory = {{2, left + 0xa, a}, {2, right + 0xa, b}, {2, left + 8, 0x1111}, {2, right + 0xc, 0x2222}},
                         .label = "order: half +a " + hex(a) + " " + hex(b)});
    // Rows that point at records: scratch records with the halfword +0x1ec, then pairs of the career's clubs.
    const auto record_a = scratch_base + 0x1000, record_b = scratch_base + 0x1400;
    for (const auto& [a, b] : halves)
        cases.push_back({.entry = by_reputation, .registers = {{3, left}, {4, right}},
                         .memory = {{4, left, record_a}, {4, right, record_b}, {2, record_a + 0x1ec, a}, {2, record_b + 0x1ec, b},
                                    {2, record_a + 0x1ea, 0x3333}, {2, record_b + 0x1ee, 0x4444}},
                         .label = "order: reputation " + hex(a) + " " + hex(b)});
    for (unsigned n = 0; n + 1 < w.club_count() && n < 40; n += 3)
        cases.push_back({.entry = by_reputation, .registers = {{3, left}, {4, right}},
                         .memory = {{4, left, w.club(n)}, {4, right, w.club(n + 1)}},
                         .label = "order: reputation clubs " + std::to_string(n) + " " + std::to_string(n + 1)});
}
}
}

// LEAGUE.CPP, block `fixtures`: cases for 0x104a4970 (the order of the standings rows). Real league objects
// of the career (their real standings rows and clubs), with the values each branch reads written over them:
// the club counts, the flag word's preset bit, the buffered stream's cursor, limit and seed (a table of
// halfwords in the scratch area, so the sample's parity is chosen), the derby globals, the regions the clubs
// point at (scratch records whose +0x14 is the parent), allocation failures per call, and the virtual that
// answers the preset rows (the base class's 0x104a60d0 answered per call).
#include "league_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t entry = 0x104a4970, default_place = 0x104a60d0, allocate = 0x107735f0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::int32_t cursor_slot = -0x7b84, limit_slot = -0x7b88, seed_slot = -0x7b90, derby_record_slot = -0x67c0,
                       derby_a_slot = -0x7a04, derby_b_slot = -0x7a08, base_vtable_slot = -0x6158;
constexpr std::uint32_t area = scratch_base + 0x40000, table = scratch_base + 0x42000, regions = scratch_base + 0x41000;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;  // league objects with standings rows
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto list = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (object && seen.insert(object).second && is_league(object) && s.r32(object + 0xb0) && clubs(object) >= 2) leagues.push_back(object);
        };
        for (std::uint32_t id = 0; list && id < count && id < 4096; ++id) {
            const auto object = s.r32(list + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 2) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::int32_t clubs(std::uint32_t object) const { return std::int16_t(s.r32(object + 0x3c) & 0xffff); }
    std::uint32_t row(std::uint32_t object, std::uint32_t index) const { return s.r32(object + 0xb0) + index * 0x44; }
    std::uint32_t club(std::uint32_t object, std::uint32_t index) const { return s.r32(row(object, index)); }
    std::uint32_t club_id(std::uint32_t object, std::uint32_t index) const { return s.r32(club(object, index)); }
};
std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < 0xec; offset += 4) writes.push_back({4, at + offset, s.r32(object + offset)});
    return writes;
}

LEAGUE_CASES("fixtures") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.leagues.empty()) throw std::runtime_error("fixtures: the career has no league with standings");
    // Leagues with different club counts first.
    std::vector<std::uint32_t> picks;
    std::set<std::int32_t> sizes;
    for (const auto object : w.leagues)
        if (sizes.insert(w.clubs(object)).second && picks.size() < 8) picks.push_back(object);
    std::fprintf(stderr, "fixtures: %zu leagues, %zu picked, sizes", w.leagues.size(), picks.size());
    for (const auto size : sizes) std::fprintf(stderr, " %d", size);
    std::fprintf(stderr, "\n");
    const auto label = [](const std::string& text) { return "fixtures: " + text; };
    const auto real = picks.front();
    const auto real_count = w.clubs(real);
    const auto flags_of = [&](std::uint32_t object) { return scenario.r32(object + 0xdc) >> 16; };

    // The league as the career has it, with the caller's argument as the match slots per round and variants.
    for (const auto object : picks) {
        const auto n = std::uint32_t(w.clubs(object));
        for (const auto pairs : {n, n / 2, n - 1, 1u, 0u})
            cases.push_back({.entry = entry, .registers = {{3, object}, {4, pairs}}, .label = label("career league " + hex(object) + " clubs " + std::to_string(n) + " pairs " + std::to_string(pairs))});
    }

    // The stream: refill or not at the first advance, seeds of both parities and signs.
    const auto cursor = scenario.toc(cursor_slot), limit = scenario.toc(limit_slot), seed = scenario.toc(seed_slot);
    const auto stream = [&](std::uint32_t at, std::uint32_t top, std::uint32_t seed_value, std::uint32_t first, std::uint32_t second) {
        return std::vector<Write>{{4, cursor, at}, {4, limit, top}, {4, seed, seed_value}, {2, table + 2, first}, {2, table + 4, second}, {2, table + 6, 3}};
    };
    for (const auto seed_value : {0u, 1u, 29999u, 30000u, 0xffffu, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        for (const auto first : {0u, 1u, 0xffffu}) {
            cases.push_back({.entry = entry, .registers = {{3, real}, {4, std::uint32_t(real_count)}}, .memory = stream(table, table + 0x100, seed_value, first, 0),
                             .label = label("stream no refill seed " + hex(seed_value) + " sample " + hex(first))});
        }
    for (const auto top : {0u, table + 1, table + 2, table + 3})
        cases.push_back({.entry = entry, .registers = {{3, real}, {4, std::uint32_t(real_count)}}, .memory = stream(table, top, 5, 1, 0),
                         .label = label("stream refill at limit " + hex(top - table))});
    cases.push_back({.entry = entry, .registers = {{3, real}, {4, std::uint32_t(real_count)}}, .memory = stream(0xfffffffcu, 0xfffffffeu, 5, 1, 0),
                     .label = label("stream cursor wraps")});

    // Club counts on a copy of a real league (the rows stay the career's): 0 and 1 and small ones, odd and even.
    const auto copy = area;
    const auto copy_memory = object_copy(scenario, copy, real);
    for (const auto n : {0, 1, 2, 3, 4, 5, 6, 7, 8, real_count - 1}) {
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = joined(copy_memory, {{2, copy + 0x3e, std::uint32_t(n)}}),
                         .label = label("count " + std::to_string(n))});
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = joined(copy_memory, {{2, copy + 0x3e, std::uint32_t(n)}, {2, copy + 0x40, 2003}}),
                         .label = label("count " + std::to_string(n) + " year 2003")});
    }
    cases.push_back({.entry = entry, .registers = {{3, copy}, {4, 4}}, .memory = joined(copy_memory, {{2, copy + 0x3e, 0xfffeu}}), .label = label("negative count")});

    // Allocation failures, each of the three calls and all.
    cases.push_back({.entry = entry, .registers = {{3, real}, {4, std::uint32_t(real_count)}}, .answers = {{allocate, 0}}, .label = label("no allocation")});
    for (unsigned nth = 1; nth <= 3; ++nth)
        cases.push_back({.entry = entry, .registers = {{3, real}, {4, std::uint32_t(real_count)}}, .nth_answers = {{allocate, nth, 0}},
                         .label = label("allocation " + std::to_string(nth) + " fails")});

    // The seeded pair: the competition's record id equals the global, two club ids sit in the standings.
    const auto record_id = scenario.r32(scenario.r32(real + 4));
    const auto seeded = [&](std::uint32_t a, std::uint32_t b, bool same_competition) {
        return std::vector<Write>{{4, scenario.toc(derby_record_slot), same_competition ? record_id : record_id + 1},
                                  {4, scenario.toc(derby_a_slot), a}, {4, scenario.toc(derby_b_slot), b}};
    };
    for (const auto n : {2, 3, 4, 5, 6, 8, real_count}) {
        const auto sized = joined(copy_memory, {{2, copy + 0x3e, std::uint32_t(n)}});
        const auto id = [&](int i) { return w.club_id(real, std::uint32_t(i)); };
        const std::pair<int, int> pairs[] = {{0, 1}, {1, 0}, {0, n - 1}, {n - 1, 0}, {1, n - 1}, {n / 2, 0}, {0, 0}, {n - 1, n - 1}};
        for (const auto& [a, b] : pairs) {
            if (a >= n || b >= n) continue;
            for (const auto [first, second] : {std::pair<std::uint32_t, std::uint32_t>{0, 0}, {0, 1}, {1, 1}, {7, 2}}) {
                auto memory = joined(joined(sized, seeded(id(a), id(b), true)), stream(table, table + 0x100, 0, first, second));
                cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory,
                                 .label = label("seeded n " + std::to_string(n) + " a " + std::to_string(a) + " b " + std::to_string(b) + " samples " + hex(first) + " " + hex(second))});
            }
        }
        // One of the ids not in the standings, the other competition, a refill at the second advance.
        auto memory = joined(joined(sized, seeded(0x7fffff00u, id(1), true)), stream(table, table + 0x100, 0, 0, 0));
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory, .label = label("seeded first id missing n " + std::to_string(n))});
        memory = joined(joined(sized, seeded(0x7ffffe00u, 0x7fffff00u, true)), stream(table, table + 0x100, 0, 0, 0));
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory, .label = label("seeded neither id found n " + std::to_string(n))});
        // The argument as a negative or large number of match slots: the scratch copy's slot index leaves the block.
        for (const auto bad : {0xffffffffu, 0xfffffffeu, std::uint32_t(2 * n), std::uint32_t(2 * n + 1)}) {
            memory = joined(joined(sized, seeded(id(0), id(1), true)), stream(table, table + 0x100, 0, 0, 0));
            cases.push_back({.entry = entry, .registers = {{3, copy}, {4, bad}}, .memory = memory, .label = label("seeded n " + std::to_string(n) + " pairs " + hex(bad))});
        }
        memory = joined(joined(sized, seeded(id(0), 0x7fffff00u, true)), stream(table, table + 0x100, 0, 0, 0));
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory, .label = label("seeded second id missing n " + std::to_string(n))});
        memory = joined(joined(sized, seeded(id(0), id(1), false)), stream(table, table + 0x100, 0, 0, 0));
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory, .label = label("other competition n " + std::to_string(n))});
        for (const auto bit : {0u, 1u}) {
            memory = joined(joined(sized, seeded(id(0), id(1), true)), stream(table, table + 2, 4, 1, 0));
            memory.push_back({2, scenario.toc(-0x7b8c), bit});  // the refill's cursor is the start pointer: its halfword is the sample
            cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory,
                             .label = label("seeded refill at the second advance n " + std::to_string(n) + " bit " + std::to_string(bit))});
        }
    }

    // Regions: the pairing rules by the clubs' +0x3c pointers (records in the scratch area whose +0x14 is the parent).
    // pattern[i] = region index of club i, -1 a null pointer; parent[r] = region index or -1 (no parent).
    struct Pattern { std::string name; std::vector<int> region; std::vector<int> parent; };
    const std::vector<Pattern> patterns = {
        {"all null", {-1, -1, -1, -1, -1, -1, -1, -1}, {}},
        {"all one region", {0, 0, 0, 0, 0, 0, 0, 0}, {-1}},
        {"pairs of regions", {0, 0, 1, 1, 2, 2, 3, 3}, {-1, -1, -1, -1}},
        {"interleaved", {0, 1, 0, 1, 0, 1, 0, 1}, {-1, -1}},
        {"null among regions", {-1, 0, 0, -1, 1, 1, 0, 0}, {-1, -1}},
        {"parent of the next", {0, 1, 2, 3, 4, 5, 6, 7}, {1, -1, 3, -1, 5, -1, 7, -1}},
        {"child of the later", {1, 0, 3, 2, 5, 4, 7, 6}, {-1, 0, -1, 2, -1, 4, -1, 6}},
        {"mixed one and parents", {0, 0, 1, 2, 3, 4, 5, 5}, {-1, 2, -1, -1, 5, -1}},
        {"parent loop", {0, 1, 0, 1, 2, 3, 2, 3}, {1, 0, 3, 2}},
        {"one region and a parent", {0, 1, 0, 1, 0, 1, 0, 1}, {1, -1}},
        {"null first", {-1, 0, 1, 0, 1, 0, 1, 0}, {-1, 0}},
        {"region then null", {0, 0, -1, 0, 0, 0, 0, 0}, {-1}},
        {"null second", {0, -1, 0, 0, 1, 1, 0, 1}, {-1, 0}},
    };
    const auto region_address = [](int r) { return regions + std::uint32_t(r) * 0x20; };
    const auto prepared = [&](const Pattern& p, std::int32_t n, int shift) {
        std::vector<Write> memory = joined(copy_memory, {{2, copy + 0x3e, std::uint32_t(n)}});
        for (std::int32_t i = 0; i < n; ++i) {
            const auto r = p.region[(std::size_t(i) + std::size_t(shift)) % p.region.size()];
            memory.push_back({4, w.club(real, std::uint32_t(i)) + 0x3c, r < 0 ? 0u : region_address(r)});
        }
        for (std::size_t r = 0; r < p.parent.size(); ++r) memory.push_back({4, region_address(int(r)) + 0x14, p.parent[r] < 0 ? 0u : region_address(p.parent[r])});
        for (std::size_t r = p.parent.size(); r < 8; ++r) memory.push_back({4, region_address(int(r)) + 0x14, 0});
        return memory;
    };
    for (const auto& p : patterns)
        for (const auto n : {2, 3, 4, 5, 6, 7, 8})
            for (const int shift : {0, 1}) {
                if (n > real_count) continue;
                const auto base = prepared(p, n, shift);
                cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = base,
                                 .label = label("regions " + p.name + " n " + std::to_string(n) + " shift " + std::to_string(shift))});
                // The same with a seeded pair first (clubs 0 and 1, parity both ways), and the degenerate pair a == b.
                if (shift) continue;
                for (const auto [a, b] : {std::pair<int, int>{0, 1}, {1, 2}, {0, 0}, {n - 1, n - 1}, {0, n - 1}}) {
                    if (a >= n || b >= n) continue;
                    for (const auto parity : {0u, 1u}) {
                        auto memory = joined(joined(base, seeded(w.club_id(real, std::uint32_t(a)), w.club_id(real, std::uint32_t(b)), true)),
                                             stream(table, table + 0x100, 0, 0, parity));
                        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory,
                                         .label = label("regions " + p.name + " seeded n " + std::to_string(n) + " a " + std::to_string(a) + " b " + std::to_string(b) + " parity " + std::to_string(parity))});
                    }
                }
            }

    // The preset rows: bit 0x100 of the flag word, the base class's slot +0xb0 answered per call (the place of
    // club number nth), slot +0xb4 as the base class's (nothing). The copy takes the base class's vtable.
    const auto base_vtable = scenario.toc(base_vtable_slot);
    const std::pair<std::string, std::vector<std::uint32_t>> places[] = {
        {"identity", {0, 1, 2, 3, 4, 5, 6, 7}}, {"reversed", {7, 6, 5, 4, 3, 2, 1, 0}}, {"rotated", {1, 2, 3, 4, 5, 6, 7, 0}},
        {"none", {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, {"duplicates", {0, 0, 1, 1, 2, 2, 3, 3}},
        {"out of range", {0x7f, 0x80, 8, 100, 0x108, 0xff, 2, 0xfffffffeu}}, {"partial", {3, 0xff, 1, 0xff, 5, 0xff, 7, 0xff}},
        {"zero", {0, 0, 0, 0, 0, 0, 0, 0}}, {"high", {5, 6, 7, 8, 9, 10, 11, 12}}};
    for (const auto n : {0, 1, 2, 3, 4, 5, 8}) {
        if (n > real_count) continue;
        for (const auto& [name, answer] : places) {
            std::vector<Case::NthAnswer> nth;
            for (unsigned k = 0; k < unsigned(n); ++k) nth.push_back({default_place, k + 1, answer[k]});
            auto memory = joined(copy_memory, {{4, copy, base_vtable}, {2, copy + 0x3e, std::uint32_t(n)}, {2, copy + 0xdc, 0x103}});
            cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory, .nth_answers = nth,
                             .label = label("preset n " + std::to_string(n) + " places " + name)});
        }
        // The base class's own answer (-1 for every club), flags with the preset bit and others set.
        for (const auto flags : {0x100u, 0x1ffu, 0xff00u}) {
            auto memory = joined(copy_memory, {{4, copy, base_vtable}, {2, copy + 0x3e, std::uint32_t(n)}, {2, copy + 0xdc, flags}});
            cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(n)}}, .memory = memory,
                             .label = label("preset base slot n " + std::to_string(n) + " flags " + hex(flags))});
        }
    }
    // Flags without the preset bit around it.
    for (const auto flags : {0u, 0xffu, 0x200u, 0xfeffu})
        cases.push_back({.entry = entry, .registers = {{3, copy}, {4, std::uint32_t(real_count)}}, .memory = joined(copy_memory, {{2, copy + 0xdc, flags}}),
                         .label = label("flags " + hex(flags))});
    // Preset with the career's own league (its flags as they are, bit set on a copy of a pick).
    for (const auto object : picks)
        if (!(flags_of(object) & 0x100))
            cases.push_back({.entry = entry, .registers = {{3, object}, {4, std::uint32_t(w.clubs(object))}}, .memory = {{2, object + 0xdc, scenario.r32(object + 0xdc) >> 16 | 0x100}},
                             .label = label("preset flag on career league " + hex(object))});
}
}
}

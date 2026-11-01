// Direct calls of the rating block's predicates: the side's bench average, the fixture grade, the stage
// lookup and the situation predicate over every kind, minute, tie result and rules record the match allows.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("rating_pred") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto r2 = std::uint32_t(scenario.s.r<4>(8));
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory = {},
                         std::string label = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };

    // 0x101fdb70: players off the pitch (negative position) and their +0x36 halfwords.
    for (const std::uint32_t side : {0u, 1u, 0xffu}) for (const unsigned count : {0u, 1u, 3u, 20u})
        for (const std::uint32_t value : {5u, 0xfff9u, 0x7fffu, 0x8000u}) {
            std::vector<Write> memory;
            for (unsigned n = 0; n < 20; ++n) {
                const auto player = scenario.player(side & 1, n);
                memory.push_back({1, player + 0x19, n < count ? 0xfdu : 5u});
                memory.push_back({2, player + 0x36, value + n});
            }
            add(0x101fdb70, {{3, match}, {4, side}}, memory, "bench_average");
        }

    // 0x101fd900: the fixture's grade: its year, the two counter sums and the guards.
    const auto list = match + 0xe030;
    const auto global = scenario.r32(r2 - 0x7b54);
    // The object the grade is posted to: a table in the match object, so the post completes.
    const auto post_slot = scenario.r32(r2 - 0x7428);
    const Write post_target = {4, post_slot, match + 0x100}, post_table = {4, match + 0x100, match + 0x200};
    for (const std::uint32_t year : {0u, 1000u, 3000u, 5000u, 8000u, 9500u, 12000u, 0xfc18u, 0x8000u})
        for (const std::uint32_t first : {0u, 1u, 2u, 3u, 0xffu}) for (const std::uint32_t second : {0u, 3u, 4u, 5u, 6u}) {
            std::vector<Write> memory = {{2, list + 8, year}, {1, match + 0xe04b, first}, {1, match + 0xe081, 0},
                                         {1, match + 0xe04c, second}, {1, match + 0xe082, 0},
                                         post_target, post_table};
            add(0x101fd900, {{3, match}}, memory, "grade");
            memory.push_back({4, global, 0});
            add(0x101fd900, {{3, match}}, memory, "grade, no global");
        }
    // The two draws of the grade (500, then 25) come from the pool of halfwords at the cursor: all zero or all large, with a zero offset.
    const auto cursor_address = scenario.r32(r2 - 0x7b84), offset_address = scenario.r32(r2 - 0x7b90);
    const auto cursor = scenario.r32(cursor_address);
    for (const std::uint32_t pool_value : {0u, 0xfffeu}) for (const std::uint32_t year : {0u, 1000u, 5000u, 9500u, 12000u, 16000u})
        for (const std::uint32_t first : {0u, 1u, 3u}) for (const std::uint32_t second : {0u, 4u, 6u}) {
            std::vector<Write> memory = {{2, list + 8, year}, {1, match + 0xe04b, first}, {1, match + 0xe081, 0},
                                         {1, match + 0xe04c, second}, {1, match + 0xe082, 0}, post_target, post_table, {4, global, 0}, {4, offset_address, 0}};
            for (unsigned k = 1; k <= 4; ++k) memory.push_back({2, cursor + 2 * k, pool_value});
            add(0x101fd900, {{3, match}}, memory, "grade, drawn");
        }
    add(0x101fd900, {{3, match}}, {{4, list + 4, 0}, post_target, post_table}, "grade, no list");
    add(0x101fd900, {{3, match}}, {{4, match + 0x3118, 0}, post_target, post_table}, "grade, no fixture");
    add(0x101fd900, {{3, match}}, {{4, fixture + 0x14, 0}, post_target, post_table}, "grade, fixture without id");

    // 0x101fd6b0: the stage lookup over a pair record and a record whose objects fill the three stages. The objects are
    // competition objects: the category test (0x100cdae0) reads the id word at their start, so each gets one the game lists.
    const auto pair = scenario.player(1, 1), record = scenario.player(0, 0);
    const auto objects_at = ((std::uint32_t(scenario.s.r<4>(4)) - 0x6000) & ~0xfu) + 0x2000;
    std::vector<std::uint32_t> category_ids;
    for (const std::int32_t offset : {-0x7668, -0x7670, -0x7678, -0x767c, -0x7688, -0x7698}) category_ids.push_back(scenario.r32(scenario.r32(r2 + std::uint32_t(offset))));
    const unsigned id_triples[][3] = {{0, 0, 0}, {0, 1, 1}, {1, 0, 0}, {2, 3, 3}, {3, 4, 2}, {4, 5, 4}, {5, 4, 5}, {1, 1, 2}, {0, 2, 2}, {3, 3, 3}};
    for (const auto& triple : id_triples) {
        const std::uint32_t objects[] = {0, objects_at, objects_at + 0x100};
        const std::uint32_t limit = objects_at + 0x200, home = objects_at + 0x300;
        for (const std::uint32_t mode : {0u, 1u, 0x100u}) for (unsigned pattern = 0; pattern < 64; ++pattern) {
            std::vector<Write> memory = {{4, objects_at, category_ids[triple[0]]}, {4, objects_at + 0x100, category_ids[triple[1]]},
                                         {4, limit, category_ids[triple[2]]}, {4, home, category_ids[triple[0]]}};
            for (unsigned word = 0; word < 6; ++word)
                memory.push_back({4, pair + 0x44 + 4 * word, (pattern >> word) & 1 ? objects[1 + (word & 1)] : 0});
            auto full = memory;
            full.push_back({4, record + 0x44, home});
            full.push_back({4, record + 0x48, limit});
            add(0x101fd6b0, {{3, pair}, {4, record}, {5, mode}}, full, "stage");
            memory.push_back({4, record + 0x44, 0});
            memory.push_back({4, record + 0x48, 0});
            add(0x101fd6b0, {{3, pair}, {4, record}, {5, mode}}, memory, "stage, empty record");
        }
    }
    add(0x101fd6b0, {{3, 0}, {4, record}, {5, 0}}, {}, "stage, no pair");
    add(0x101fd6b0, {{3, pair}, {4, 0}, {5, 0}}, {}, "stage, no record");

    // 0x101fdc10: every kind, both sides, minutes, tie results and rules records. The rules table entry is
    // found by the fixture's first word; indexes with a nonzero entry give the cases a rules record.
    const auto table = scenario.r32(scenario.r32(r2 - 0x7b5c));
    std::vector<std::uint32_t> indexes = {scenario.r32(fixture)};
    for (std::uint32_t index = 0; index < 64 && indexes.size() < 4; ++index) {
        const auto entry = scenario.r32(table + 4 * index);
        if (entry && entry != 0xffffffffu && scenario.r32(entry + 8) && index != indexes[0]) indexes.push_back(index);
    }
    const auto out = match + 0x40;
    const std::uint32_t kinds[] = {0x1f3f, 0x1f40, 0x2000, 0x21a2, 0x21a3, 0x21a4, 0x21a5, 0x21a6, 0x21a7, 0x21a8, 0x21a9, 0x21aa, 0x21ab,
                                   0x21ac, 0x21ad, 0x21ae, 0x21af, 0x21b0, 0x21e4, 0x21e5, 0xffff9000u};
    const std::uint32_t clocks[] = {0, 341, 836, 880, 891, 3000};
    const std::pair<std::uint32_t, std::uint32_t> scores[] = {{0, 0}, {1, 0}, {0, 1}, {0, 2}, {3, 0}, {0, 4}};
    for (const std::uint32_t side : {0u, 1u}) {
        const auto team = scenario.r32(fixture + 0x1c + 4 * side);
        for (const auto index : indexes) for (const bool own : {false, true}) for (const std::uint32_t kind : kinds)
            for (const auto clock : clocks) for (const auto& [home, away] : scores) {
                if (!own && (clock == 880 || clock == 3000 || away > 2)) continue;  // plain runs: fewer
                std::vector<Write> memory = {{2, match + 0x7948, clock}, {1, match + 0xe03d, home}, {1, match + 0xe073, away},
                                             {4, fixture, index}, {4, team + 0x14, index ^ (own ? 0u : (home & 1))}, {1, fixture + 0x4b, 0xff}};
                if (own) memory.push_back({4, fixture + 0x14, scenario.r32(team + 0x30)});
                add(0x101fdc10, {{3, match}, {4, side}, {5, kind}, {6, kind & 1 ? out : 0}}, memory, "situation");
            }
    }
    add(0x101fdc10, {{3, match}, {4, 0}, {5, 0x21a3}, {6, 0}}, {{4, fixture + 0x14, 0}}, "situation, fixture without id");

    // A built world for the deep paths, below the stack where no frame reaches: the trait table (one element
    // per code, sorted as the search compares them), a rules table entry that applies, and the rules record.
    const auto scratch = ((std::uint32_t(scenario.s.r<4>(4)) - 0x6000) & ~0xfu);
    const auto trait_object = scenario.r32(r2 - 0x7b94);      // the global holding the trait table's object
    const auto applies_flag = scenario.r32(r2 - 0x7b34);      // a pointer that must hold null for the rule to apply
    std::uint32_t free_id = 1;
    const auto taken = [&](std::uint32_t id) {
        for (const std::int32_t offset : {-0x6940, -0x7aa8, -0x6944, -0x6888, -0x67bc, -0x6734})
            if (scenario.r32(scenario.r32(r2 + std::uint32_t(offset))) == id) return true;
        return false;
    };
    while (taken(free_id)) ++free_id;
    const std::uint32_t codes[] = {0x3f, 0x40, 0x41, 0x5c, 0x5f, 0x60, 0x61, 0x62, 0x63};
    const std::vector<std::vector<std::uint32_t>> kind_codes = {{0x61, 0x62, 0x63}, {0x5c}, {0x5f}, {0x3f}, {0x61, 0x62, 0x41, 0x63}, {0x5c},
                                                                {0x5f, 0x40}, {0x3f, 0x60}};
    const std::pair<std::uint32_t, std::uint32_t> rule_values[] = {{0, 0}, {2, 9}, {5, 13}, {0, 13}, {5, 0}};
    const std::pair<std::uint32_t, std::uint32_t> built_scores[] = {{0, 0}, {1, 0}, {0, 1}, {0, 2}, {3, 0}};
    for (const std::uint32_t side : {0u, 1u}) {
        const auto team = scenario.r32(fixture + 0x1c + 4 * side);
        const auto index = scenario.r32(fixture);
        for (const std::uint32_t legs : {0xffu, 1u}) for (const std::uint32_t kind : {0x21a3u, 0x21a4u, 0x21a5u, 0x21a6u, 0x21a7u, 0x21a8u, 0x21a9u, 0x21aau,
                                                                                    0x21abu, 0x21acu, 0x21adu, 0x21aeu, 0x21afu})
            for (const auto clock : {341u, 836u, 891u, 0u}) for (const auto& [home, away] : built_scores)
                for (const auto& [low, high] : rule_values) {
                    const bool asks_traits = kind <= 0x21aa;
                    if (!asks_traits && (low != 0 || high != 0) && (clock == 0)) continue;
                    if (asks_traits && (low != 0 || high != 0) && !(low == 5 && high == 13)) continue;  // the rules record is not read
                    // The codes the table holds: all of them, or only one of the kind's.
                    std::vector<std::vector<std::uint32_t>> sets = {std::vector<std::uint32_t>(std::begin(codes), std::end(codes))};
                    if (asks_traits) for (const auto code : kind_codes[kind - 0x21a3]) sets.push_back({code});
                    for (const auto& held : sets) for (const std::uint32_t late : {0xffffu, 0u}) for (const std::uint32_t average : {0u, 6000u, 6800u, 7100u, 7600u, 7800u, 8100u}) {
                        if (late == 0 && (held.size() != 9 || clock != 836 || average)) continue;
                        // The average of the side's players off the pitch is what the kinds compare: all twenty off, one value.
                        if (average && (!asks_traits || clock != 836 || legs != 0xff || home + away > 3)) continue;
                        std::vector<Write> memory = {{2, match + 0x7948, clock}, {1, match + 0xe03d, home}, {1, match + 0xe073, away},
                                                     {4, fixture, index}, {4, team + 0x14, index}, {1, fixture + 0x4b, legs},
                                                     {1, fixture + 0x4c, 0}, {4, fixture + 0x14, scenario.r32(team + 0x30)}};
                        // The rules table entry: applies (its byte 0x44 is set), holds the object with the rules record.
                        const auto entry = scratch + 0x200, object = scratch + 0x300, record = scratch + 0x1000;
                        memory.push_back({4, table + 4 * index, entry});
                        // Both guards pass: a nonzero object, a competition id none of the six kinds, a date past it.
                        memory.push_back({4, applies_flag, 1});
                        memory.push_back({4, global, scratch + 0x400});
                        memory.push_back({4, scratch + 0x404, 0});
                        memory.push_back({4, entry + 4, scratch + 0x420});
                        memory.push_back({4, scratch + 0x420, free_id});
                        memory.push_back({4, scenario.r32(r2 - 0x7b64), 0x1000});
                        memory.push_back({1, entry + 0x44, 0});
                        memory.push_back({4, entry + 8, object});
                        memory.push_back({4, object + 0x5cc, record});
                        for (const auto [at, value] : {std::pair{0x1cu, low}, {0x30u, low}, {0x44u, high}, {0x58u, high}})
                            memory.push_back({4, record + at, value});
                        // The trait table: an object with an array of 0x18-byte elements keyed by team and code.
                        memory.push_back({4, trait_object, scratch});
                        memory.push_back({4, scratch, scratch + 0x40});
                        memory.push_back({4, scratch + 8, std::uint32_t(held.size())});
                        for (std::size_t n = 0; n < held.size(); ++n) {
                            const auto element = scratch + 0x40 + 0x18 * std::uint32_t(n);
                            memory.push_back({2, element, 0xffff});
                            memory.push_back({2, element + 2, late});
                            memory.push_back({4, element + 8, 0xffffffffu});
                            memory.push_back({4, element + 0xc, scenario.r32(team)});
                            memory.push_back({2, element + 0x12, held[n]});
                            memory.push_back({4, element + 0x14, 0xffffffffu});
                        }
                        if (average)
                            for (unsigned n = 0; n < 20; ++n) {
                                memory.push_back({1, scenario.player(side, n) + 0x19, 0xfd});
                                memory.push_back({2, scenario.player(side, n) + 0x36, average});
                            }
                        add(0x101fdc10, {{3, match}, {4, side}, {5, kind}, {6, (home + clock) & 1 ? out : 0}}, memory, "situation, built");
                    }
                }
    }
}

// Cases of the `stages` block (comp_stages.cpp) from the real career (--world): the game's own competition
// objects, their stages, clubs and stats. Objects a case needs with one field changed are copies in the scratch
// area (their pointers still lead to the real rows); other modules' routines run for real except where a case
// answers them.
#include "comp_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <string>

namespace cm::aot::comp_checks {
namespace {
constexpr std::uint32_t club_finish = 0x10450710, all_done = 0x10450900, current_stage = 0x10450a50, first_stage = 0x10450c20,
                        round_played = 0x10450db0, takes_part = 0x10451570, all_clubs = 0x10451d10, still_in = 0x10452ed0,
                        lacks_property = 0x10451f50, set_club_bytes = 0x10451f80, pack_rules = 0x104530f0, virtual_5c = 0x10451ca0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t copy_size = 0xc0;  // an object's base fields and the derived ones the block reads (up to +0xb0)
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
struct Career {
    const Scenario& s;
    std::vector<std::uint32_t> objects;  // every competition object, in id order
    explicit Career(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id)
            if (const auto object = s.r32(table + id * 4)) objects.push_back(object);
    }
    std::uint8_t u8(std::uint32_t at) const { return std::uint8_t(s.s.memory.read<1>(at)); }
    std::int16_t i16(std::uint32_t at) const { return std::int16_t(s.s.memory.read<2>(at)); }
    std::int32_t last_stage(std::uint32_t object) const { return std::int32_t(s.r32(object + 0x30)); }
    std::uint32_t stage(std::uint32_t object, std::int32_t index) const {
        return index < 0 ? object : s.r32(s.r32(object + 0xc) + std::uint32_t(index) * 4);
    }
    // The clubs of a stage's rows (league: 8 bytes at +0xa4, knockout: 0x44 at +0xb0), at most `limit`.
    std::vector<std::uint32_t> clubs(std::uint32_t stage, std::size_t limit) const {
        std::vector<std::uint32_t> found;
        const bool knockout = u8(stage + 0x43) == 2;
        const auto table = s.r32(stage + (knockout ? 0xb0 : 0xa4));
        const auto step = knockout ? 0x44u : 8u;
        for (std::int32_t n = 0; table && n < i16(stage + 0x3e) && n < 64 && found.size() < limit; ++n)
            if (const auto club = s.r32(table + std::uint32_t(n) * step)) found.push_back(club);
        return found;
    }
    // Every distinct club of an object's stages' first rows, the object's own first.
    std::vector<std::uint32_t> member_clubs(std::uint32_t object, std::size_t per_stage = 2, std::size_t total = 6) const {
        std::vector<std::uint32_t> found;
        for (std::int32_t index = -1; index <= last_stage(object) && index < 20 && found.size() < total; ++index)
            for (const auto club : clubs(stage(object, index), per_stage))
                if (found.size() < total && std::ranges::find(found, club) == found.end()) found.push_back(club);
        return found;
    }
    std::uint32_t record_id(std::uint32_t object) const { return s.r32(s.r32(object + 4)); }
};
std::vector<Write> copy_of(const Scenario& s, std::uint32_t at, std::uint32_t from, std::uint32_t size) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < size; offset += 4) writes.push_back({4, at + offset, s.r32(from + offset)});
    return writes;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
// Objects worth a case: the first `n`, then every one with stages or with a chain or parent.
std::vector<std::uint32_t> picked(const Career& w, std::size_t plain, std::size_t special) {
    std::vector<std::uint32_t> found;
    std::size_t extra = 0;
    for (std::size_t n = 0; n < w.objects.size(); ++n) {
        const auto object = w.objects[n];
        const bool odd = w.last_stage(object) >= 0 || std::int32_t(w.s.r32(object + 0x1c)) >= 0 || std::int32_t(w.s.r32(object + 0x20)) >= 0;
        if (n < plain || (odd && extra++ < special)) found.push_back(object);
    }
    return found;
}
// The first `n` objects of a kind (1 league, 2 knockout) with at least two club rows.
std::vector<std::uint32_t> of_kind(const Career& w, std::uint8_t kind, std::size_t n) {
    std::vector<std::uint32_t> found;
    for (const auto object : w.objects)
        if (found.size() < n && w.u8(object + 0x43) == kind && w.i16(object + 0x3e) >= 2) found.push_back(object);
    return found;
}
// A pattern in a scratch area, so a stray or missing store shows.
std::vector<Write> pattern(std::uint32_t at, std::uint32_t bytes, std::uint32_t seed = 0x5a5a5a5au) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < bytes; offset += 4) writes.push_back({4, at + offset, seed + offset});
    return writes;
}

COMP_CASES("stages: club_finish") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto code_out = scratch_base + 0x1000, round_out = scratch_base + 0x1010, flag_out = scratch_base + 0x1020;
    const auto outs = joined(joined(pattern(code_out, 8), pattern(round_out, 8, 0x6b6b6b6bu)), pattern(flag_out, 8, 0x7c7c7c7cu));
    const auto call = [&](std::uint32_t object, std::uint32_t club, const std::vector<Write>& memory, const std::string& label) {
        cases.push_back({.entry = club_finish, .registers = {{3, object}, {4, club}, {5, code_out}, {6, round_out}, {7, flag_out}},
                         .memory = joined(outs, memory), .label = "stages: club_finish " + label});
    };
    for (const auto object : picked(w, 8, 40)) {
        auto clubs = w.member_clubs(object, 3, 8);
        // Also the last row of the last stage, where a later stage must not hide it.
        clubs.push_back(0x12345678);  // a word no row holds
        clubs.push_back(scenario.club(2));
        for (const auto club : clubs) call(object, club, {}, hex(object) + " club " + hex(club));
    }
    // Copies: a stage code of 0x3fc (skipped), no stages, only the object's own stage, a stage below the range.
    const auto copy = scratch_base + 0x2000;
    auto copied = picked(w, 0, 12);
    for (const auto object : of_kind(w, 1, 4)) copied.push_back(object);
    for (const auto object : copied) {
        const auto base = copy_of(scenario, copy, object, copy_size);
        const auto clubs = w.member_clubs(object, 2, 4);
        for (const auto club : clubs) {
            call(copy, club, joined(base, {{2, copy + 0x3a, 0x3fc}}), hex(object) + " own stage skipped, club " + hex(club));
            call(copy, club, joined(base, {{4, copy + 0x30, 0xffffffffu}}), hex(object) + " no stages, club " + hex(club));
            call(copy, club, joined(base, {{4, copy + 0x30, 0xfffffffeu}}), hex(object) + " last stage -2, club " + hex(club));
            for (const auto round : {1u, 2u, 3u, 10u})
                if (w.u8(object + 0x43) == 1) call(copy, club, joined(base, {{1, copy + 0xa8, round}}), hex(object) + " round " + std::to_string(round) + ", club " + hex(club));
        }
    }
}

COMP_CASES("stages: all_done") {
    if (scenario.match) return;
    const Career w(scenario);
    for (const auto object : picked(w, 24, 40)) cases.push_back({.entry = all_done, .registers = {{3, object}}, .label = "stages: all_done " + hex(object)});
    // Copies without a parent whose chain leads to a real object, and with a parent.
    const auto copy = scratch_base + 0x3000;
    // Every pair of the first objects: a chain whose first link is done and a later one is not.
    for (std::size_t n = 0; n < w.objects.size() && n < 24; ++n)
        for (std::size_t m = 0; m < w.objects.size() && m < 24; ++m)
            cases.push_back({.entry = all_done, .registers = {{3, copy}},
                             .memory = joined(copy_of(scenario, copy, w.objects[n], copy_size), {{4, copy + 0x1c, 0xffffffffu}, {4, copy + 0x20, w.record_id(w.objects[m])}}),
                             .label = "stages: all_done pair " + hex(w.objects[n]) + " then " + hex(w.objects[m])});
    // An object whose virtual +0x18 is the base class's +0x1c (answers 1): its chain reaches the real ones.
    const auto vtable = scratch_base + 0x3c00, base_vtable = scenario.toc(-0x619c);
    for (std::size_t m = 0; m < w.objects.size() && m < 40; ++m)
        cases.push_back({.entry = all_done, .registers = {{3, copy}},
                         .memory = joined(joined(copy_of(scenario, vtable, base_vtable, 0x88), copy_of(scenario, copy, w.objects[0], copy_size)),
                                          {{4, vtable + 0x18, scenario.r32(base_vtable + 0x1c)}, {4, copy, vtable}, {4, copy + 0x1c, 0xffffffffu}, {4, copy + 0x20, w.record_id(w.objects[m])}}),
                         .label = "stages: all_done done object chained to " + hex(w.objects[m])});
    for (std::size_t n = 0; n + 1 < w.objects.size() && n < 16; ++n) {
        const auto base = copy_of(scenario, copy, w.objects[n], copy_size);
        cases.push_back({.entry = all_done, .registers = {{3, copy}},
                         .memory = joined(base, {{4, copy + 0x1c, 0xffffffffu}, {4, copy + 0x20, w.record_id(w.objects[n + 1])}}),
                         .label = "stages: all_done chained to " + hex(w.objects[n + 1])});
        cases.push_back({.entry = all_done, .registers = {{3, copy}}, .memory = joined(base, {{4, copy + 0x1c, 0xffffffffu}, {4, copy + 0x20, 0xffffffffu}}),
                         .label = "stages: all_done single " + hex(w.objects[n])});
        cases.push_back({.entry = all_done, .registers = {{3, copy}}, .memory = joined(base, {{4, copy + 0x1c, 3}}),
                         .label = "stages: all_done with a parent " + hex(w.objects[n])});
        cases.push_back({.entry = all_done, .registers = {{3, copy}}, .memory = joined(base, {{4, copy + 0x1c, 0xffffffffu}, {4, copy + 0x20, w.record_id(w.objects[n])}}),
                         .label = "stages: all_done chained to a real twin " + hex(w.objects[n])});
    }
}

COMP_CASES("stages: current_stage") {
    if (scenario.match) return;
    const Career w(scenario);
    for (const auto object : picked(w, 30, 60)) cases.push_back({.entry = current_stage, .registers = {{3, object}}, .label = "stages: current_stage " + hex(object)});
    const auto copy = scratch_base + 0x3000;
    for (const auto object : picked(w, 0, 20)) {
        const auto base = copy_of(scenario, copy, object, copy_size);
        for (const auto last : {0xffffffffu, 0xfffffffeu, 0u, 1u})
            cases.push_back({.entry = current_stage, .registers = {{3, copy}}, .memory = joined(base, {{4, copy + 0x30, last}}),
                             .label = "stages: current_stage " + hex(object) + " last stage " + hex(last)});
        for (const auto round : {0u, 1u, 7u})
            cases.push_back({.entry = current_stage, .registers = {{3, copy}}, .memory = joined(base, {{1, copy + 0xa8, round}}),
                             .label = "stages: current_stage " + hex(object) + " current round " + std::to_string(round)});
    }
}

COMP_CASES("stages: current_stage built") {
    if (scenario.match) return;
    // The object (stage -1) is any real object, its one stage a copy of a real league that has played no round:
    // the stage found is moved back to the first of its group when the object itself says it is done.
    const Career w(scenario);
    const auto top = scratch_base + 0x3000, stage = scratch_base + 0x3400, array = scratch_base + 0x3800;
    for (const auto object : picked(w, 60, 0))
        for (const auto league : of_kind(w, 1, 3)) {
            const auto memory = joined(joined(copy_of(scenario, top, object, copy_size), copy_of(scenario, stage, league, copy_size)),
                                       {{4, array, stage}, {4, top + 0xc, array}, {4, top + 0x30, 0}, {1, stage + 0xa8, 0}, {4, stage + 0x30, 0xffffffffu}});
            cases.push_back({.entry = current_stage, .registers = {{3, top}}, .memory = memory,
                             .label = "stages: current_stage built " + hex(object) + " over league " + hex(league)});
            // The same with the object's stages array holding the league twice.
            cases.push_back({.entry = current_stage, .registers = {{3, top}}, .memory = joined(memory, {{4, array + 4, stage}, {4, top + 0x30, 1}}),
                             .label = "stages: current_stage built " + hex(object) + " over two leagues " + hex(league)});
        }
}

COMP_CASES("stages: first_stage_of_group") {
    if (scenario.match) return;
    const Career w(scenario);
    std::size_t staged = 0;
    for (const auto object : w.objects) {
        const auto last = w.last_stage(object);
        if (last < 0 && staged > 12) continue;
        if (last >= 0) ++staged;
        for (std::int32_t index = -3; index <= last + 2 && index < 10; ++index)
            cases.push_back({.entry = first_stage, .registers = {{3, object}, {4, std::uint32_t(index)}},
                             .label = "stages: first_stage_of_group " + hex(object) + " index " + std::to_string(index)});
        cases.push_back({.entry = first_stage, .registers = {{3, object}, {4, 0x7fffffffu}}, .label = "stages: first_stage_of_group " + hex(object) + " largest"});
        cases.push_back({.entry = first_stage, .registers = {{3, object}, {4, 0x80000000u}}, .label = "stages: first_stage_of_group " + hex(object) + " smallest"});
        if (staged > 20) break;
    }
}

COMP_CASES("stages: round_played") {
    if (scenario.match) return;
    const Career w(scenario);
    std::size_t staged = 0;
    for (const auto object : w.objects) {
        const auto last = w.last_stage(object);
        if (last < 0 && staged > 8) continue;
        if (last >= 0) ++staged;
        for (std::int32_t index = -2; index <= last + 1 && index < 6; ++index)
            for (const auto round : {-1, 0, 1, 2, 5, 37, 127})
                cases.push_back({.entry = round_played, .registers = {{3, object}, {4, std::uint32_t(index)}, {5, std::uint32_t(round)}},
                                 .label = "stages: round_played " + hex(object) + " stage " + std::to_string(index) + " round " + std::to_string(round)});
        if (staged > 16) break;
    }
    // Copies of league objects: the current round, the round count and the last round's kind (0x8c) set.
    const auto copy = scratch_base + 0x4000, rounds_table = scratch_base + 0x5000;
    for (const auto object : of_kind(w, 1, 1)) {
        const auto base = copy_of(scenario, copy, object, copy_size);
        for (const auto count : {1u, 2u, 3u, 0u})
            for (const auto kind : {0x8cu, 0x8du}) {
                std::vector<Write> rows = pattern(rounds_table, 0x78 * 5, 0x01010101u);
                rows.push_back({2, rounds_table + count * 0x78 - 0x70, kind});
                const auto memory = joined(joined(base, rows), {{2, copy + 0x3c, count}, {4, copy + 0xa0, rounds_table}, {4, copy + 0x30, 0xffffffffu}});
                for (const auto current : {0u, 1u, 2u, 3u})
                    for (const auto round : {0, 1, 2, 3})
                        cases.push_back({.entry = round_played, .registers = {{3, copy}, {4, 0xffffffffu}, {5, std::uint32_t(round)}},
                                         .memory = joined(memory, {{1, copy + 0xa8, current}}),
                                         .label = "stages: round_played copy of " + hex(object) + " rounds " + std::to_string(count) + " kind " + hex(kind) +
                                                  " current " + std::to_string(current) + " round " + std::to_string(round)});
            }
    }
}

COMP_CASES("stages: takes_part") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto club_copy = scratch_base + 0x6000;
    std::vector<std::uint32_t> others;  // clubs of the first objects, never the member of the object they are asked about
    for (const auto object : w.objects)
        if (others.size() < 5)
            for (const auto club : w.member_clubs(object, 1, 1)) others.push_back(club);
    for (const auto object : picked(w, 20, 50)) {
        auto clubs = w.member_clubs(object, 2, 6);
        for (const auto club : others) clubs.push_back(club);
        clubs.push_back(scenario.club(0));
        clubs.push_back(scenario.club(1));
        for (const auto club : clubs) cases.push_back({.entry = takes_part, .registers = {{3, object}, {4, club}}, .label = "stages: takes_part " + hex(object) + " club " + hex(club)});
    }
    // Copies of a real club with its id word and nation fields changed: the branch of an id past the count, no
    // nation, the club's own competition record, another nation.
    const auto object_copy = scratch_base + 0x7000;
    std::size_t done = 0;
    for (const auto object : picked(w, 6, 30)) {
        if (done++ > 12) break;
        const auto members = w.member_clubs(object, 1, 2);
        const auto club = members.empty() ? scenario.club(0) : members.front();
        const auto record = scenario.r32(object + 4);
        const auto base = joined(copy_of(scenario, club_copy, club, 0x248), copy_of(scenario, object_copy, object, copy_size));
        const auto count = scenario.global(-0x7b50);
        for (const auto id : {0u, 5u, count, count - 1, count + 100, 0x7fffffffu, 0xfffffff0u})
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}}, .memory = joined(base, {{4, club_copy, id}}),
                             .label = "stages: takes_part copy of club " + hex(club) + " id " + hex(id) + " in " + hex(object)});
        // The club's own nation, a continent the record names that is not the nation's.
        cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}}, .memory = joined(base, {{4, club_copy, 0x7fffffffu}, {4, record + 4, 5}}),
                         .label = "stages: takes_part record continent differs " + hex(object)});
        for (const auto id : {0u, 0x7fffffffu}) {
            // The record's continent set (the real record is shared: the write is undone after the case).
            for (const auto continent : {5u, 0xffffffffu})
                cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}},
                                 .memory = joined(base, {{4, club_copy, id}, {4, club_copy + 0x2c, 0}, {4, record + 4, continent}}),
                                 .label = "stages: takes_part club without nation, continent " + hex(continent) + " id " + hex(id) + " in " + hex(object)});
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}}, .memory = joined(base, {{4, club_copy, id}, {4, club_copy + 0x2c, 0}}),
                             .label = "stages: takes_part club without nation id " + hex(id) + " in " + hex(object)});
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}}, .memory = joined(base, {{4, club_copy, id}, {4, club_copy + 0x30, record}}),
                             .label = "stages: takes_part club of record id " + hex(id) + " in " + hex(object)});
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}},
                             .memory = joined(base, {{4, club_copy, id}, {4, club_copy + 0x2c, scenario.r32(record + 8)}, {4, club_copy + 0x30, record}}),
                             .label = "stages: takes_part club of the record's nation and record id " + hex(id) + " in " + hex(object)});
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}},
                             .memory = joined(base, {{4, club_copy, id}, {4, club_copy + 0x2c, scenario.r32(record + 8)}, {4, club_copy + 0x30, 0}}),
                             .label = "stages: takes_part club of the record's nation, no record " + hex(id) + " in " + hex(object)});
            cases.push_back({.entry = takes_part, .registers = {{3, object_copy}, {4, club_copy}}, .memory = joined(base, {{4, club_copy, id}, {4, object_copy + 4, scratch_base + 0x7800}, {4, scratch_base + 0x7804, 0}}),
                             .label = "stages: takes_part record without continent id " + hex(id) + " in " + hex(object)});
        }
    }
}

COMP_CASES("stages: all_clubs") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto out = scratch_base + 0x1100;
    for (const auto object : picked(w, 12, 60))
        cases.push_back({.entry = all_clubs, .registers = {{3, object}, {4, out}}, .memory = pattern(out, 8), .label = "stages: all_clubs " + hex(object)});
    for (const auto object : picked(w, 2, 2))
        cases.push_back({.entry = all_clubs, .registers = {{3, object}, {4, out}}, .memory = pattern(out, 8), .answers = {{0x10775130, 0}},
                         .label = "stages: all_clubs allocation fails " + hex(object)});
    // More than 200 distinct clubs: a copy whose league table is the club table itself (8-byte rows, but the
    // first word of each is read), and one that repeats the same club.
    const auto copy = scratch_base + 0x8000, rows = scratch_base + 0x9000;
    for (const auto object : of_kind(w, 1, 1)) {
        const auto base = copy_of(scenario, copy, object, copy_size);
        std::vector<Write> table;
        for (std::uint32_t n = 0; n < 260; ++n) table.push_back({4, rows + n * 8, n % 50 == 7 ? 0 : n < 250 ? 0x1000 + n * 0x10 : 0x1000});
        cases.push_back({.entry = all_clubs, .registers = {{3, copy}, {4, out}}, .memory = joined(joined(base, table), {{4, copy + 0xa4, rows}, {2, copy + 0x3e, 260}, {4, copy + 0x30, 0xffffffffu}, {1, copy + 0x43, 1}}),
                         .label = "stages: all_clubs more than 200 clubs"});
        cases.push_back({.entry = all_clubs, .registers = {{3, copy}, {4, out}}, .memory = joined(joined(base, table), {{4, copy + 0xa4, rows}, {2, copy + 0x3e, 40}, {4, copy + 0x30, 0xffffffffu}, {1, copy + 0x43, 1}}),
                         .label = "stages: all_clubs 40 clubs"});
    }
}

COMP_CASES("stages: club_still_in") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto copy = scratch_base + 0xa000;
    for (const auto object : picked(w, 20, 60)) {
        auto clubs = w.member_clubs(object, 2, 6);
        clubs.push_back(scenario.club(3));  // a club that is in no stage
        clubs.push_back(scenario.club(70));
        for (const auto club : clubs) {
            cases.push_back({.entry = still_in, .registers = {{3, object}, {4, club}}, .label = "stages: club_still_in " + hex(object) + " club " + hex(club)});
        }
    }
    std::size_t done = 0;
    for (const auto object : picked(w, 4, 40)) {
        if (done++ > 24) break;
        const auto base = copy_of(scenario, copy, object, copy_size);
        const auto clubs = w.member_clubs(object, 2, 3);
        for (const auto club : clubs) {
            const auto call = [&](const std::vector<Write>& memory, const std::string& what, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
                cases.push_back({.entry = still_in, .registers = {{3, copy}, {4, club}}, .memory = joined(base, memory), .answers = std::move(answers),
                                 .label = "stages: club_still_in copy of " + hex(object) + " " + what + " club " + hex(club)});
            };
            call({{1, copy + 0x44, 0}}, "owner 0");
            call({{1, copy + 0x44, 0xff}, {4, copy + 0x8, 0}}, "own, no stats");
            call({}, "owner as is, answer 104abf40 true", {{0x104abf40, 1}});
            call({}, "owner as is, answer 10494d20 true", {{0x10494d20, 1}});
            call({}, "owner as is, answer 104abf40 false, 10494d20 false", {{0x104abf40, 0}, {0x10494d20, 0}});
            call({{1, copy + 0x44, 0xff}}, "own, stats as is");
            call({{1, copy + 0x44, 0xff}}, "own, stats row answered null", {{0x10459b50, 0}});
            call({{1, copy + 0x44, 0xff}, {1, copy + 0x43, 1}}, "own, league");
            call({{1, copy + 0x44, 0xff}, {1, copy + 0x43, 2}}, "own, knockout");
            call({{1, copy + 0x44, 0xff}, {1, copy + 0x43, 3}, {4, copy + 0x30, 0xffffffffu}}, "own, other kind, no stages");
            call({{1, copy + 0x44, 0xff}, {4, copy + 0x30, 0xffffffffu}}, "own, no stages");
            call({{1, copy + 0x44, 0xff}, {4, copy + 0x30, 0xfffffffeu}}, "own, last stage -2");
            call({{1, copy + 0x44, 0xff}, {4, copy + 0xa4, 0}, {1, copy + 0x43, 1}, {4, copy + 0x30, 0xffffffffu}}, "own, league without table");
            call({{1, copy + 0x44, 0xff}, {4, copy + 0xb0, 0}, {1, copy + 0x43, 2}, {4, copy + 0x30, 0xffffffffu}}, "own, knockout without entries");
        }
    }
    // Knockout rows of every status: a copy whose entries are scratch rows naming the club.
    const auto rows = scratch_base + 0xb000;
    for (const auto object : of_kind(w, 2, 1)) {
        const auto base = copy_of(scenario, copy, object, copy_size);
        const auto clubs = w.member_clubs(object, 1, 1);
        if (clubs.empty()) continue;
        for (const std::uint32_t status : {0u, 1u, 4u, 5u, 6u, 7u, 8u, 0xfeu, 0xffu})
            for (const auto a : {0u, 1u, 0xffu, 2u})
                for (const auto b : {0u, 1u, 0xffu, 2u}) {
                    std::vector<Write> entries = pattern(rows, 0x44 * 3, 0x01010101u);
                    entries.push_back({4, rows + 0x44, clubs.front()});
                    entries.push_back({1, rows + 0x44 + 0x40, status});
                    entries.push_back({1, rows + 0x44 + 0x42, a});
                    entries.push_back({1, rows + 0x44 + 0x43, b});
                    cases.push_back({.entry = still_in, .registers = {{3, copy}, {4, clubs.front()}},
                                     .memory = joined(joined(base, entries), {{1, copy + 0x44, 0xff}, {4, copy + 0xb0, rows}, {2, copy + 0x3e, 3}, {4, copy + 0x30, 0xffffffffu}, {1, copy + 0x43, 2}, {4, copy + 0x8, 0}}),
                                     .label = "stages: club_still_in knockout row status " + hex(status) + " bytes " + hex(a) + " " + hex(b)});
                }
    }
    // The two shared objects 0x10452ed0 asks first, missing (their cells zeroed).
    for (const auto object : picked(w, 3, 0))
        for (const auto club : w.member_clubs(object, 1, 1))
            for (const auto slot : {-0x7b34, -0x7b54})
                cases.push_back({.entry = still_in, .registers = {{3, object}, {4, club}}, .memory = {{4, scenario.toc(slot), 0}},
                                 .label = "stages: club_still_in cell " + hex(std::uint32_t(-slot)) + " empty " + hex(object)});
    // The first cell empty and an unmapped competition: the original answers 0 without reading the object (0x10452ef0..0x10452ef8).
    for (const std::uint32_t object : {0u, 0xdead0000u})
        cases.push_back({.entry = still_in, .registers = {{3, object}, {4, scenario.club(3)}}, .memory = {{4, scenario.toc(-0x7b34), 0}},
                         .label = "stages: club_still_in cell 7b34 empty, unreadable competition " + hex(object)});
}

COMP_CASES("stages: lacks_property") {
    if (scenario.match) return;
    const Career w(scenario);
    for (const auto object : picked(w, 20, 40)) cases.push_back({.entry = lacks_property, .registers = {{3, object}, {4, object}}, .label = "stages: lacks_property object " + hex(object)});
    for (unsigned n = 0; n < 60; ++n)
        cases.push_back({.entry = lacks_property, .registers = {{3, 0}, {4, scenario.competition(n)}}, .label = "stages: lacks_property record " + std::to_string(n)});
    const auto row = scratch_base + 0xc000;
    for (const auto value : {0u, 1u, 2u, 3u, 5u, 0x10u, 0x11u, 0x3fcu, 0x7fffu, 0x8000u, 0xffffu})
        cases.push_back({.entry = lacks_property, .registers = {{3, 0}, {4, row}}, .memory = {{2, row + 0x30, value}}, .label = "stages: lacks_property value " + hex(value)});
}

COMP_CASES("stages: set_club_bytes") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto copy = scratch_base + 0xd000;
    std::size_t done = 0;
    for (const auto object : picked(w, 20, 60)) {
        const auto clubs = w.member_clubs(object, 1, 2);
        const auto club = clubs.empty() ? scenario.club(0) : clubs.front();
        cases.push_back({.entry = set_club_bytes, .registers = {{3, object}, {4, club}, {5, 0x55}, {6, 0x11}, {7, 0x22}, {8, 0x33}},
                         .label = "stages: set_club_bytes " + hex(object) + " club " + hex(club)});
        if (done++ > 30) continue;
        const auto base = copy_of(scenario, copy, object, copy_size);
        cases.push_back({.entry = set_club_bytes, .registers = {{3, copy}, {4, club}, {5, 0x55}, {6, 0xa1}, {7, 0xb2}, {8, 0xc3}},
                         .memory = joined(base, {{1, copy + 0x44, 0xff}}), .label = "stages: set_club_bytes own " + hex(object) + " club " + hex(club)});
        cases.push_back({.entry = set_club_bytes, .registers = {{3, copy}, {4, club}, {5, 0x55}, {6, 0xa1}, {7, 0xb2}, {8, 0xc3}},
                         .memory = joined(base, {{1, copy + 0x44, 0}}), .label = "stages: set_club_bytes not own " + hex(object) + " club " + hex(club)});
        cases.push_back({.entry = set_club_bytes, .registers = {{3, copy}, {4, club}, {5, 0x55}, {6, 0xa1}, {7, 0xb2}, {8, 0xc3}},
                         .memory = joined(base, {{1, copy + 0x44, 0xff}}), .answers = {{0x10459b50, 0x60000800}},
                         .label = "stages: set_club_bytes own, row answered " + hex(object)});
    }
}

COMP_CASES("stages: pack_fixture_rules") {
    if (scenario.match) return;
    const Career w(scenario);
    const auto copy = scratch_base + 0xe000, fixture = scratch_base + 0xf000;
    const auto count = scenario.global(competition_count_slot);
    const auto table = scenario.global(competitions_slot);
    std::uint32_t empty_id = 0xffffffffu;
    for (std::uint32_t id = 0; id < count && id < 4096; ++id)
        if (!scenario.r32(table + id * 4)) { empty_id = id; break; }
    for (const auto object : picked(w, 12, 40)) {
        cases.push_back({.entry = pack_rules, .registers = {{3, object}, {4, fixture}}, .memory = pattern(fixture, 0x60), .label = "stages: pack_fixture_rules " + hex(object)});
    }
    std::size_t subordinate = 0;
    for (const auto object : w.objects)
        if (w.u8(object + 0x44) != 0xff && subordinate++ < 4)
            cases.push_back({.entry = pack_rules, .registers = {{3, object}, {4, fixture}}, .memory = joined(pattern(fixture, 0x60), {{4, scenario.toc(competitions_slot), 0}}),
                             .label = "stages: pack_fixture_rules without the competition table " + hex(object)});
    std::size_t done = 0;
    for (const auto object : picked(w, 6, 30)) {
        if (done++ > 12) break;
        const auto base = copy_of(scenario, copy, object, copy_size);
        for (const auto bench : {0u, 5u, 0xffu, 0x18u, 0x80u})
            for (const auto subs : {0u, 3u, 0xffu, 0x1fu, 0x80u})
                for (const auto byte : {0u, 0xffu, 0x5au}) {
                    auto memory = joined(base, {{1, copy + 0x44, 0xff}, {1, copy + 0x46, bench}, {1, copy + 0x47, subs}, {1, fixture + 0x4e, byte}});
                    cases.push_back({.entry = pack_rules, .registers = {{3, copy}, {4, fixture}}, .memory = joined(pattern(fixture, 0x60), memory),
                                     .label = "stages: pack_fixture_rules own " + hex(object) + " bench " + hex(bench) + " subs " + hex(subs) + " byte " + hex(byte)});
                }
        cases.push_back({.entry = pack_rules, .registers = {{3, copy}, {4, fixture}}, .memory = joined(pattern(fixture, 0x60), joined(base, {{1, copy + 0x44, 0}})),
                         .label = "stages: pack_fixture_rules not own " + hex(object)});
        if (empty_id != 0xffffffffu) {
            const auto record = scratch_base + 0xe800;
            cases.push_back({.entry = pack_rules, .registers = {{3, copy}, {4, fixture}},
                             .memory = joined(pattern(fixture, 0x60), joined(base, joined(copy_of(scenario, record, scenario.r32(object + 4), 0x6c), {{4, copy + 4, record}, {4, record, empty_id}, {1, copy + 0x44, 0}}))),
                             .label = "stages: pack_fixture_rules owner without object " + hex(object)});
        }
    }
}

COMP_CASES("stages: stage_virtual_5c") {
    if (scenario.match) return;
    const Career w(scenario);
    // STAFF_RECORDS passes a competition record as r4 (its signed byte at +0x42 names a stage, -1 the object itself),
    // r5 = 0 and r6 = a local buffer; the virtual +0x5c of the derived classes reads them.
    const auto record = scratch_base + 0x10000, buffer = scratch_base + 0x10100;
    for (const auto object : picked(w, 12, 40)) {
        const auto base = copy_of(scenario, record, scenario.r32(object + 4), 0x6c);
        for (std::int32_t index = -1; index <= w.last_stage(object) && index < 4; ++index)
            cases.push_back({.entry = virtual_5c, .registers = {{3, object}, {4, record}, {5, 0}, {6, buffer}}, .memory = joined(pattern(buffer, 0x40), joined(base, {{1, record + 0x42, std::uint32_t(index)}})),
                             .label = "stages: stage_virtual_5c " + hex(object) + " stage " + std::to_string(index)});
        cases.push_back({.entry = virtual_5c, .registers = {{3, object}, {4, scenario.r32(object + 4)}, {5, 0}, {6, buffer}}, .memory = pattern(buffer, 0x40),
                         .label = "stages: stage_virtual_5c " + hex(object) + " own record"});
    }
}
}
}

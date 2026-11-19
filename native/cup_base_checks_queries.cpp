// CUP.CPP, block `queries`: cases for the news a cup files for a result (0x1047b360), the places it records (0x1047b5b0),
// the final places (0x1047b7d0, 0x1047bde0), a fixture's tie (0x1047ba50), the entrants of a tie's sides (0x10484b90,
// 0x10484e10), the place bytes (0x10484fd0, 0x10486480), a match's flags (0x10485340), the round difference
// (0x104858b0), the prizes (0x10485930), the comparison of two fixtures' records (0x10485ab0) and the ranks
// (0x10485b10). The career's real cups (their round tables, entrant rows, match rows, statistics) are the objects;
// what a branch needs that the career lacks is written over them in a case's own memory (undone afterwards): round
// labels and flags, entrant states, the sides and feeders of a tie, a club count. A cup's vtable is copied to the
// scratch area where a case needs a virtual (+0x4c, +0x90) to answer something the real one cannot: the slot then
// points at a dummy routine of another module that the case answers. Fixtures and records the career has no instance
// of are built in the scratch area. Labels start with "queries:" (CM_CUP_BLOCK=queries).
#include "cup_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t news_result = 0x1047b360, place_record = 0x1047b5b0, final_places = 0x1047b7d0, tie_of = 0x1047ba50,
                        seeding = 0x1047bde0, side_entrants = 0x10484b90, append_entrants = 0x10484e10, entrant_places = 0x10484fd0,
                        match_flags = 0x10485340, round_difference = 0x104858b0, pay_prizes = 0x10485930, lower_record = 0x10485ab0,
                        rank_teams = 0x10485b10, group_places = 0x10486480;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t area = scratch_base + 0x30000;
constexpr std::uint32_t real_toc = 0x108bba90;
// Scratch layout. The vtable copy and its descriptors; fixtures; a record; buffers.
[[maybe_unused]] constexpr std::uint32_t vtable_at = area, descriptors_at = area + 0x100, fixture_at = area + 0x400, fixture_b = area + 0x500,
                        record_a = area + 0x600, record_b = area + 0x700, out_at = area + 0x800, counter_at = area + 0x900,
                        holder_at = area + 0xa00, nation_at = area + 0xb00, row_at = area + 0xc00, club_at = area + 0x1000;
// Dummy routines of other modules a copied vtable's slots point at: a case answers them (the answer is r3).
[[maybe_unused]] constexpr std::uint32_t dummy_a = 0x102aed80, dummy_b = 0x1000cfc0, dummy_c = 0x10456290;
// Routines of other modules the entries call; most are answered where their effect (histories on files, the news
// files, ...) is not what a case compares.
[[maybe_unused]] constexpr std::uint32_t fixture_rules = 0x10494d20, nation_of = 0x10192130, history_win = 0x10383410, history_two = 0x10383b70,
                        history_three = 0x10384020, history_round = 0x10384500, history_other = 0x103846e0, club_to_row = 0x100c76c0,
                        place_final = 0x10463990, host_status = 0x1049d1c0, stats_row = 0x10459b50, seeding_call = 0x104b7860,
                        item_enabled = 0x102cdbf0, field_byte_call = 0x10463be0;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { append(a, b); return a; }
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

// A real cup the career has and the fields the entries read.
struct Real {
    std::uint32_t object = 0, round_table = 0, entrants = 0, matches = 0, record = 0, stats = 0;
    std::int32_t rounds = 0, clubs = 0;
};
struct World {
    const Scenario& s;
    std::vector<Real> cups;      // every cup object (kind 1 with a CUP.CPP body in its vtable)
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) { if (object && is_cup(object) && seen.insert(object).second) cups.push_back(read(object)); };
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
    std::uint32_t u16(std::uint32_t at) const { return u8(at) << 8 | u8(at + 1); }
    std::int32_t s16(std::uint32_t at) const { return std::int16_t(u16(at)); }
    Real read(std::uint32_t object) const {
        return {object, s.r32(object + 0xa0), s.r32(object + 0xa4), s.r32(object + 0xac), s.r32(object + 4), s.r32(object + 8),
                s16(object + 0x3c), s16(object + 0x3e)};
    }
    bool is_cup(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || u8(object + 0x43) != 1) return false;
        for (std::uint32_t slot = 0x08; slot <= 0xa4; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::uint32_t id(const Real& cup) const { return s.r32(cup.record); }
    // The competition table's entry for the cup's record id is the cup itself (a top-level cup, not a stage).
    bool self(const Real& cup) const { return s.r32(s.global(competitions_slot) + id(cup) * 4) == cup.object; }
    std::uint32_t club(unsigned index) const { return s.global(-0x7be8) + index * 0x248; }
    // A knockout with the tables the entries walk.
    bool usable(const Real& cup, std::int32_t rounds_at_least = 2) const {
        return cup.rounds >= rounds_at_least && cup.clubs >= 2 && cup.round_table && cup.entrants && cup.matches && cup.record;
    }
    std::uint32_t round_row(const Real& cup, std::int32_t round) const { return cup.round_table + std::uint32_t(round) * 0x78; }
    std::uint32_t tie_row(const Real& cup, std::int32_t round, std::int32_t tie) const {
        return s.r32(cup.matches + std::uint32_t(round) * 4) + std::uint32_t(tie) * 0x36;
    }
    std::uint32_t entrant_row(const Real& cup, std::int32_t index) const { return cup.entrants + std::uint32_t(index) * 8; }
    // The entrants whose club pointer is a real club (the career fills the rows of an undecided cup with -1).
    std::vector<std::int32_t> good_entrants(const Real& cup, std::size_t wanted = 6) const {
        std::vector<std::int32_t> list;
        const auto clubs = s.global(-0x7be8), count = s.global(-0x7b50);
        for (std::int32_t n = 0; n < cup.clubs && list.size() < wanted; ++n) {
            const auto club = s.r32(entrant_row(cup, n));
            if (club >= clubs && club < clubs + count * 0x248) list.push_back(n);
        }
        return list;
    }
    // The n-th club with a news inbox (+0x90): the news filed under it is kept.
    std::uint32_t inbox_club(unsigned n) const {
        const auto count = s.global(-0x7b50);
        for (unsigned k = 0; k < count; ++k) if (s.r32(club(k) + 0x90) && n-- == 0) return club(k);
        throw std::runtime_error("queries: too few clubs with a news inbox");
    }
    bool has_matches(const Real& cup, std::int32_t round) const { return round >= 0 && round < cup.rounds && s.r32(cup.matches + std::uint32_t(round) * 4) != 0; }
};

// A copy of a cup's vtable in the scratch area whose listed slots point at dummy routines, written into the object.
std::vector<Write> redirected(const Scenario& s, std::uint32_t cup, std::vector<std::pair<std::uint32_t, std::uint32_t>> slots) {
    std::vector<Write> memory;
    const auto vtable = s.r32(cup);
    for (std::uint32_t offset = 0; offset < 0xa8; offset += 4) memory.push_back(w32(vtable_at + offset, s.r32(vtable + offset)));
    std::uint32_t n = 0;
    for (const auto& [slot, code] : slots) {
        memory.push_back(w32(descriptors_at + n * 8, code));
        memory.push_back(w32(descriptors_at + n * 8 + 4, real_toc));
        memory.push_back(w32(vtable_at + slot, descriptors_at + n * 8));
        ++n;
    }
    memory.push_back(w32(cup, vtable_at));
    return memory;
}

CUP_CASES("queries") {
    if (scenario.match) return;
    const World w(scenario);
    std::vector<Real> knockouts;
    for (const auto& cup : w.cups) if (w.usable(cup)) knockouts.push_back(cup);
    if (knockouts.size() < 4) throw std::runtime_error("queries: the career has fewer than four usable cups");
    std::fprintf(stderr, "queries: %zu cups, %zu usable\n", w.cups.size(), knockouts.size());
    {
        unsigned with_inbox = 0, first = 0;
        const auto count = scenario.global(-0x7b50);
        for (unsigned n = 0; n < count; ++n) if (scenario.r32(w.club(n) + 0x90)) { if (!with_inbox) first = n; ++with_inbox; }
        std::fprintf(stderr, "queries: clubs with a news inbox (+0x90): %u of %u, first %u (%x) inbox %x\n", with_inbox, count, first, w.club(first), scenario.r32(w.club(first) + 0x90));
    }
    const auto label = [](const std::string& text, const Real& cup) { return "queries: " + text + " " + hex(cup.object); };

    // ---- 0x10486480 (vt +0x8c): the places 1, 1 and the count's low byte through the virtual +0x70 -------------------
    for (std::size_t n = 0; n < knockouts.size() && n < 8; ++n) {
        const auto& cup = knockouts[n];
        const auto club = w.s.r32(w.entrant_row(cup, 0));
        cases.push_back({.entry = group_places, .registers = {{3, cup.object}, {4, club}}, .label = label("group_places real", cup)});
        for (const std::uint32_t count : {0u, 0x1ffu, 0x80u, 0xffffu})
            cases.push_back({.entry = group_places, .registers = {{3, cup.object}, {4, club}}, .memory = {w16(cup.object + 0x3e, count)},
                             .label = label("group_places count " + hex(count), cup)});
        cases.push_back({.entry = group_places, .registers = {{3, cup.object}, {4, club}}, .memory = joined(redirected(scenario, cup.object, {{0x70, dummy_a}}), {w8(cup.object + 0x44, 0)}),
                         .answers = {{dummy_a, 1}}, .label = label("group_places owner 0", cup)});
        cases.push_back({.entry = group_places, .registers = {{3, cup.object}, {4, club}},
                         .memory = redirected(scenario, cup.object, {{0x70, dummy_a}}), .answers = {{dummy_a, 5}},
                         .label = label("group_places answered", cup)});
    }

    // ---- 0x10485ab0 (vt +0x98): the one of two fixtures with the smaller record ------------------------------------
    {
        const auto cup = knockouts.front().object;
        const auto records = [&](std::uint32_t a, std::uint32_t b) {
            return std::vector<Write>{w32(fixture_at + 0x30, a ? record_a : 0), w32(fixture_b + 0x30, b ? record_b : 0), w16(record_a + 0x18, a),
                                      w16(record_b + 0x18, b), w16(record_a + 0x1a, 0x1234), w16(record_b + 0x16, 0x4321)};
        };
        const std::pair<std::uint32_t, std::uint32_t> pairs[] = {{3, 5}, {5, 3}, {4, 4}, {0, 0}, {0x8000, 0x7fff}, {0x7fff, 0x8000}, {0xffff, 1}, {1, 0xffff},
                                                                 {0xffff, 0xffff}, {0, 1}, {1, 0}, {0x8000, 0x8000}};
        for (const auto& [a, b] : pairs)
            cases.push_back({.entry = lower_record, .registers = {{3, cup}, {4, fixture_at}, {5, fixture_b}}, .memory = records(a, b),
                             .label = "queries: lower_record " + hex(a) + " " + hex(b)});
        // A fixture without a record (+0x30 null): treated as 0.
        for (const bool first_null : {true, false})
            cases.push_back({.entry = lower_record, .registers = {{3, cup}, {4, fixture_at}, {5, fixture_b}},
                             .memory = {w32(fixture_at + 0x30, first_null ? 0 : record_a), w32(fixture_b + 0x30, first_null ? record_b : 0),
                                        w16(record_a + 0x18, 0xfff0), w16(record_b + 0x18, 0xfff0)},
                             .label = std::string("queries: lower_record ") + (first_null ? "first" : "second") + " without a record"});
    }

    // ---- 0x104858b0 (vt +0x5c): the round difference -----------------------------------------------------------------
    {
        const auto& cup = knockouts.front();
        for (const std::int32_t rounds : {5, 2, 0x7f, 0})
            for (const std::int32_t round : {0, 1, 3, -1, 0x7f})
                for (const std::uint32_t cup_byte : {20u, 3u, 0u, 0xffu, 0x80u})
                    for (const std::uint32_t match_byte : {0xffu, 2u, 9u}) {
                        const auto run = [&](std::uint32_t reverse, std::uint32_t other, const char* what) {
                            cases.push_back({.entry = round_difference, .registers = {{3, cup.object}, {4, fixture_at}, {5, reverse}, {6, out_at}},
                                             .memory = {w16(cup.object + 0x3c, std::uint32_t(rounds)), w8(cup.object + 0x4b, cup_byte), w16(fixture_at + 0x34, std::uint32_t(round)),
                                                        w8(fixture_at + 0x4b, match_byte), w8(fixture_at + 0x4c, other), w8(out_at, 0x66)},
                                             .label = "queries: round_difference " + std::string(what) + " rounds " + std::to_string(rounds) + " round " + std::to_string(round) +
                                                      " bytes " + hex(cup_byte) + " " + hex(match_byte) + " " + hex(other)});
                        };
                        if (rounds == 5 || round == 0)
                            for (const std::uint32_t other : {2u, 0u, 0xffu, 9u, 12u}) { run(0, other, "forward"); run(1, other, "reverse"); }
                    }
        // The flag byte's sign: a halfword whose low byte is 0 counts as 0; 0x80 as non-zero.
        for (const std::uint32_t flag : {0x100u, 0x80u, 0xffu, 1u})
            cases.push_back({.entry = round_difference, .registers = {{3, cup.object}, {4, fixture_at}, {5, flag}, {6, out_at}},
                             .memory = {w8(fixture_at + 0x4b, 3), w8(fixture_at + 0x4c, 9), w8(out_at, 0x66)}, .label = "queries: round_difference flag " + hex(flag)});
    }

    // ---- 0x10485340: a match's flags from its round and its entrants' states ----------------------------------------
    {
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            if (++taken > 6) break;
            const auto round = std::min<std::int32_t>(1, cup.rounds - 1);
            const auto row = w.tie_row(cup, round, 0);
            const auto base = [&](std::uint32_t flags, std::uint32_t round_flags, std::uint32_t state_a, std::uint32_t state_b, std::uint32_t side_a, std::uint32_t side_b) {
                return std::vector<Write>{w16(fixture_at + 0x34, std::uint32_t(round)), w16(fixture_at + 0x38, flags), w16(w.round_row(cup, round) + 0x10, round_flags),
                                          w16(row, side_a), w16(row + 2, side_b), w8(w.entrant_row(cup, std::int32_t(side_a & 0xf)) + 4, state_a),
                                          w8(w.entrant_row(cup, std::int32_t(side_b & 0xf)) + 4, state_b)};
            };
            for (const std::uint32_t flags : {0u, 0x800u, 0x1u, 0x8000u, 0xf7ffu})
                for (const std::uint32_t round_flags : {0u, 0x800u, 0x100u})
                    for (const auto& [a, b] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{1, 2}, {2, 1}, {3, 3}, {0x80, 0x7f}, {0x7f, 0x80}, {0xff, 0}, {0, 0xff}}) {
                        if (taken > 2 && (flags == 0x8000u || a == 0x80)) continue;
                        cases.push_back({.entry = match_flags, .registers = {{3, cup.object}, {4, fixture_at}, {5, 0}},
                                         .memory = base(flags, round_flags, a, b, 0, 1),
                                         .label = label("match_flags " + hex(flags) + " " + hex(round_flags) + " states " + hex(a) + " " + hex(b), cup)});
                    }
            // The tie index (a later tie of the round), the sides' indexes beyond the first rows.
            cases.push_back({.entry = match_flags, .registers = {{3, cup.object}, {4, fixture_at}, {5, 1}},
                             .memory = joined(base(0x800, 0, 1, 2, 2, 3), {w16(w.tie_row(cup, round, 1), 2), w16(w.tie_row(cup, round, 1) + 2, 3),
                                                                           w8(w.entrant_row(cup, 3) + 4, 5)}),
                             .label = label("match_flags tie 1", cup)});
        }
    }

    // ---- 0x10484fd0: the place bytes of an entrant after a round ----------------------------------------------------
    {
        const std::int32_t rounds_given[] = {-1, 0, 1, 2, 3, 4, 5, 6, 0x7fff, 0xffff};
        for (std::size_t n = 0; n < knockouts.size() && n < 4; ++n) {
            const auto& cup = knockouts[n];
            const auto last = w.round_row(cup, cup.rounds - 1);
            for (const bool special : {false, true})
                for (const std::int32_t round : rounds_given)
                    for (const std::uint32_t state : {0u, 1u, 2u, 3u, 4u, 5u, 0xffu}) {
                        if (n > 0 && (state == 5u || round > 4)) continue;
                        std::vector<Write> memory = {w8(w.entrant_row(cup, 1) + 5, state)};
                        if (special) memory.push_back(w16(last + 8, 0x8c));
                        cases.push_back({.entry = entrant_places, .registers = {{3, cup.object}, {4, 1}, {5, std::uint32_t(round)}},
                                         .memory = joined(redirected(scenario, cup.object, {{0x70, dummy_a}}), memory), .answers = {{dummy_a, 0}},
                                         .label = label(std::string("entrant_places ") + (special ? "8c-last" : "plain") + " round " + std::to_string(round) + " state " + hex(state), cup)});
                    }
            // The real virtual +0x70 writes the club's three place bytes in the statistics.
            for (const std::int32_t round : {0, cup.rounds - 2, cup.rounds - 1})
                for (const std::uint32_t state : {0u, 1u, 2u})
                    cases.push_back({.entry = entrant_places, .registers = {{3, cup.object}, {4, 0}, {5, std::uint32_t(round)}},
                                     .memory = {w8(w.entrant_row(cup, 0) + 5, state)},
                                     .label = label("entrant_places real round " + std::to_string(round) + " state " + hex(state), cup)});
            // The round count at the edges of a halfword and the entrant index below the table.
            for (const std::uint32_t count : {0u, 0xffffu, 0x7fffu, 1u, 2u})
                for (const std::uint32_t entrant : {0u, 0xffffu, 3u})
                    cases.push_back({.entry = entrant_places, .registers = {{3, cup.object}, {4, entrant}, {5, 0}},
                                     .memory = joined(redirected(scenario, cup.object, {{0x70, dummy_a}}), {w16(cup.object + 0x3c, count)}), .answers = {{dummy_a, 0}},
                                     .label = label("entrant_places rounds " + hex(count) + " entrant " + hex(entrant), cup)});
            if (n >= 1) break;
        }
    }

    // ---- 0x10484b90 / 0x10484e10: the entrants of a tie's sides, through the feeder ties ----------------------------
    {
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            if (cup.rounds < 3 || ++taken > 4) continue;
            // The real rows as the tree built them, every round and the first ties, both sides, and then rows written
            // to decide a side or to lead nowhere.
            for (std::int32_t round = 0; round < cup.rounds; ++round)
                for (std::int32_t tie = 0; tie < std::min(3, w.s16(w.round_row(cup, round) + 0x1c)); ++tie)
                    for (const std::uint32_t first : {0u, 1u}) {
                        if (!w.has_matches(cup, round) || (taken > 1 && round > 2)) continue;
                        cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, std::uint32_t(round)}, {5, std::uint32_t(tie)}, {6, first}, {7, out_at}},
                                         .memory = {w16(out_at, 0x5555), w16(out_at + 2, 0x6666)},
                                         .label = label("side_entrants round " + std::to_string(round) + " tie " + std::to_string(tie) + " first " + std::to_string(first), cup)});
                    }
            const auto row = w.tie_row(cup, 1, 0), row_next = w.tie_row(cup, 2, 0);
            // Decided sides: stored at once; one side decided, the other led to a decided tie one round on.
            for (const std::uint32_t first : {0u, 1u}) {
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, first}, {7, out_at}},
                                 .memory = {w16(row, 3), w16(row + 2, 4), w16(out_at, 0x5555)}, .label = label("side_entrants decided sides first " + std::to_string(first), cup)});
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 2}, {5, 0}, {6, first}, {7, out_at}},
                                 .memory = {w16(row_next, 0xffff), w16(row_next + 2, 0xffff), w16(row_next + 0xe, 1), w16(row_next + 0xa, 0), w16(row_next + 0x10, 1),
                                            w16(row_next + 0xc, 0), w16(row, 3), w16(row + 2, 4), w16(out_at, 0x5555)},
                                 .label = label("side_entrants both feeders decided first " + std::to_string(first), cup)});
                // The feeder tie itself undecided, one level deeper.
                const auto deeper = w.tie_row(cup, 0, 0);
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 2}, {5, 0}, {6, first}, {7, out_at}},
                                 .memory = {w16(row_next, 0xffff), w16(row_next + 2, 0xffff), w16(row_next + 0xe, 1), w16(row_next + 0xa, 0), w16(row_next + 0x10, 1), w16(row_next + 0xc, 0),
                                            w16(row, 0xffff), w16(row + 2, 4), w16(row + 0xe, 0), w16(row + 0xa, 0), w16(row + 0x10, 0), w16(row + 0xc, 0),
                                            w16(deeper, 7), w16(deeper + 2, 8), w16(out_at, 0x5555)},
                                 .label = label("side_entrants two levels first " + std::to_string(first), cup)});
                // Leads nowhere: the feeder round or tie -1 (the recursion asserts, the count stays 0, the walk asserts too).
                for (const std::uint32_t round_value : {0xffffu, 0u})
                    for (const std::uint32_t tie_value : {0xffffu, 0u})
                        cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, first}, {7, out_at}},
                                         .memory = {w16(row, 0xffff), w16(row + 2, 0xffff), w16(row + 0xe, round_value), w16(row + 0xa, tie_value),
                                                    w16(row + 0x10, round_value), w16(row + 0xc, tie_value), w16(w.tie_row(cup, 0, 0), 0xffff), w16(w.tie_row(cup, 0, 0) + 2, 0xffff),
                                                    w16(w.tie_row(cup, 0, 0) + 0xe, 0xffff), w16(w.tie_row(cup, 0, 0) + 0xa, 0xffff), w16(w.tie_row(cup, 0, 0) + 0x10, 0xffff),
                                                    w16(w.tie_row(cup, 0, 0) + 0xc, 0xffff), w16(out_at, 0x5555)},
                                         .label = label("side_entrants leads to nothing " + hex(round_value) + " " + hex(tie_value) + " first " + std::to_string(first), cup)});
                // No output buffer: the recursion asserts.
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, first}, {7, 0}},
                                 .memory = {w16(row, 0xffff), w16(row + 2, 0xffff), w16(row + 0xe, 0), w16(row + 0xa, 0), w16(row + 0x10, 0), w16(row + 0xc, 0)},
                                 .label = label("side_entrants no output buffer first " + std::to_string(first), cup)});
            }
            // The checks: the round out of range (either side), a cup without entrants, a tie of -1.
            for (const std::int32_t round : {-1, cup.rounds + 1, 0x7fffffff, std::int32_t(0x80000000)})
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, std::uint32_t(round)}, {5, 0}, {6, 0}, {7, out_at}},
                                 .label = label("side_entrants round " + hex(std::uint32_t(round)), cup)});
            cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, 0}, {7, out_at}}, .memory = {w32(cup.object + 0xa4, 0)},
                             .label = label("side_entrants no entrants", cup)});
            cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0xffffffffu}, {6, 0}, {7, out_at}}, .label = label("side_entrants tie -1", cup)});
            // The flag byte is its low byte.
            for (const std::uint32_t flag : {0x100u, 0x80u, 0xffu})
                cases.push_back({.entry = side_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, flag}, {7, out_at}}, .memory = {w16(row, 3), w16(row + 2, 4)},
                                 .label = label("side_entrants flag " + hex(flag), cup)});
            // 0x10484e10 directly: the counter's start, the output, the checks.
            for (const std::uint32_t start : {0u, 1u, 7u})
                for (std::int32_t round = 0; round < cup.rounds; ++round)
                    for (std::int32_t tie = 0; tie < 2; ++tie)
                        cases.push_back({.entry = append_entrants, .registers = {{3, cup.object}, {4, std::uint32_t(round)}, {5, std::uint32_t(tie)}, {6, out_at}, {7, counter_at}},
                                         .memory = {w32(counter_at, start), w16(out_at, 0x5555)},
                                         .label = label("append_entrants start " + std::to_string(start) + " round " + std::to_string(round) + " tie " + std::to_string(tie), cup)});
            for (const std::uint32_t round : {0xffffffffu, 0x80000000u})
                cases.push_back({.entry = append_entrants, .registers = {{3, cup.object}, {4, round}, {5, 0}, {6, out_at}, {7, counter_at}}, .memory = {w32(counter_at, 3)},
                                 .label = label("append_entrants round " + hex(round), cup)});
            cases.push_back({.entry = append_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0xffffffffu}, {6, out_at}, {7, counter_at}}, .memory = {w32(counter_at, 3)},
                             .label = label("append_entrants tie -1", cup)});
            cases.push_back({.entry = append_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, 0}, {7, counter_at}}, .memory = {w32(counter_at, 3)},
                             .label = label("append_entrants no output", cup)});
            cases.push_back({.entry = append_entrants, .registers = {{3, cup.object}, {4, 1}, {5, 0}, {6, out_at}, {7, counter_at}},
                             .memory = {w32(counter_at, 2), w16(row, 3), w16(row + 2, 0xffff), w16(row + 0x10, 0), w16(row + 0xc, 0), w16(w.tie_row(cup, 0, 0), 5), w16(w.tie_row(cup, 0, 0) + 2, 6)},
                             .label = label("append_entrants side 0 decided, side 1 fed", cup)});
        }
    }

    // ---- 0x1047ba50: the tie of a fixture in its round ---------------------------------------------------------------
    {
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            const auto good = w.good_entrants(cup, 4);
            if (good.size() < 4) continue;
            if (++taken > 5) break;
            const auto club_of = [&](std::int32_t index) { return w.s.r32(w.s.r32(w.entrant_row(cup, good[std::size_t(index)]))); };
            const auto index_of = [&](std::int32_t index) { return std::uint32_t(good[std::size_t(index)]); };
            for (std::int32_t round = 0; round < std::min(cup.rounds, 3); ++round) {
                if (!w.has_matches(cup, round)) continue;
                const auto ties = w.s16(w.round_row(cup, round) + 0x1c);
                for (std::int32_t tie = 0; tie < std::min(ties, 3); ++tie) {
                    const auto row = w.tie_row(cup, round, tie);
                    const auto place = [&](std::uint32_t side_a, std::uint32_t side_b, std::uint32_t id, const std::string& what) {
                        cases.push_back({.entry = tie_of, .registers = {{3, cup.object}, {4, fixture_at}},
                                         .memory = {w16(fixture_at + 0x34, std::uint32_t(round)), w32(fixture_at + 0xc, id), w16(row, side_a), w16(row + 2, side_b)},
                                         .label = label("tie_of round " + std::to_string(round) + " tie " + std::to_string(tie) + " " + what, cup)});
                    };
                    place(index_of(1), 0xffff, club_of(1), "first side");
                    place(0xffff, index_of(2), club_of(2), "second side");
                    place(0xffff, 0xffff, club_of(1), "undecided");
                    place(index_of(1), index_of(2), club_of(3), "not there");
                    place(0xfffe, index_of(2), club_of(2), "negative first side");
                    place(index_of(1), index_of(1), club_of(1), "both sides");
                }
            }
            cases.push_back({.entry = tie_of, .registers = {{3, cup.object}, {4, 0}}, .label = label("tie_of no fixture", cup)});
        }
    }

    // ---- 0x1047bde0: the UEFA seeding of the semi-finals' clubs ------------------------------------------------------
    {
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            std::int32_t semis = -1, final_round = -1;
            for (std::int32_t r = 0; r < cup.rounds; ++r) {
                if (w.s16(w.round_row(cup, r) + 8) == 0x82) semis = r;
                if (w.s16(w.round_row(cup, r) + 8) == 0x96) final_round = r;
            }
            const auto good = w.good_entrants(cup, 6);
            if (semis < 0 || final_round < 0 || good.size() < 5 || !w.has_matches(cup, semis) || !w.has_matches(cup, final_round)) continue;
            if (++taken > 3) break;
            const auto final_row = w.tie_row(cup, final_round, 0), semis_rows = w.s.r32(cup.matches + std::uint32_t(semis) * 4);
            const std::uint32_t pool[5] = {std::uint32_t(good[0]), std::uint32_t(good[1]), std::uint32_t(good[2]), std::uint32_t(good[3]), 0xffffu};
            const auto semis_ties = w.round_row(cup, semis) + 0x1c;
            for (std::uint32_t a = 0; a < 5; ++a)
                for (std::uint32_t b = 0; b < 5; ++b) {
                    if (taken > 1 && (a + b) % 2) continue;
                    for (const std::uint32_t ties : {2u, 1u, 3u, 0u}) {
                        if (ties != 2u && (a != b + 1)) continue;
                        cases.push_back({.entry = seeding, .registers = {{3, cup.object}},
                                         .memory = {w16(final_row, pool[0]), w16(final_row + 2, pool[1]), w16(semis_ties, ties),
                                                    w16(semis_rows, pool[a]), w16(semis_rows + 2, pool[b]), w16(semis_rows + 0x36, pool[(a + 1) % 5]),
                                                    w16(semis_rows + 0x38, pool[(b + 2) % 5]), w16(semis_rows + 0x6c, pool[2]), w16(semis_rows + 0x6e, pool[3])},
                                         .label = label("seeding sides " + std::to_string(a) + " " + std::to_string(b) + " ties " + std::to_string(ties), cup)});
                    }
                }
            // The labels: the semi-final round missing or later than the final, a label twice, the count at the halfword's edge.
            cases.push_back({.entry = seeding, .registers = {{3, cup.object}}, .memory = {w16(w.round_row(cup, semis) + 8, 0x78)}, .label = label("seeding no semi-final label", cup)});
            cases.push_back({.entry = seeding, .registers = {{3, cup.object}}, .memory = {w16(w.round_row(cup, final_round) + 8, 0x78)}, .label = label("seeding no final label", cup)});
            cases.push_back({.entry = seeding, .registers = {{3, cup.object}}, .memory = {w16(w.round_row(cup, 0) + 8, 0x96)}, .label = label("seeding final label first", cup)});
            cases.push_back({.entry = seeding, .registers = {{3, cup.object}}, .memory = {w16(w.round_row(cup, 0) + 8, 0x82)}, .label = label("seeding semi-final label first", cup)});
            cases.push_back({.entry = seeding, .registers = {{3, cup.object}}, .memory = {w16(cup.object + 0x3c, 0)}, .label = label("seeding no rounds", cup)});
        }
        if (taken == 0) throw std::runtime_error("queries: no cup with a semi-final and a final round");
    }

    // ---- 0x1047b7d0 (vt +0x38): the final places handed to COMP_UTIL -------------------------------------------------
    {
        const auto sp = scenario.sp();
        const auto out_first = sp - 0x60 + 0x38, out_second = sp - 0x60 + 0x3c;
        const auto table = scenario.global(competitions_slot);
        const auto nations = scenario.global(-0x7b98);          // the 0x128-byte rows the host status names
        const auto descriptors = scenario.toc(-0x7b44);         // the 34 descriptor rows of 0x4c bytes
        const auto host_cell = scenario.toc(-0x6af4);           // the record id the host loop is for
        const auto stats_hit = row_at, stats_miss = row_at + 0x100;
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            const auto good = w.good_entrants(cup, 6);
            if (good.size() < 5 || !w.self(cup)) continue;
            if (++taken > 3) break;
            const auto id = w.id(cup);
            const auto states = [&](std::vector<std::pair<std::int32_t, std::uint32_t>> set) {
                std::vector<Write> memory;
                for (const auto index : good) memory.push_back(w8(w.entrant_row(cup, index) + 5, 0));
                for (const auto& [at, state] : set) memory.push_back(w8(w.entrant_row(cup, good[std::size_t(at)]) + 5, state));
                return memory;
            };
            const auto virtual_90 = [&](std::uint32_t answer) {
                return std::make_pair(redirected(scenario, cup.object, {{0x90, dummy_b}}), Answers{{dummy_b, answer}});
            };
            const Answers calm = {{place_final, 0}, {fixture_rules, 1}};
            const std::pair<const char*, std::vector<Write>> layouts[] = {
                {"none", states({})}, {"1 2 3", states({{0, 1}, {1, 2}, {2, 3}})}, {"3 2 1", states({{0, 3}, {1, 2}, {2, 1}})}, {"1 only", states({{1, 1}})},
                {"2 only", states({{2, 2}})}, {"3 only", states({{3, 3}})}, {"1 2", states({{0, 1}, {1, 2}})}, {"1 1 2 3", states({{0, 1}, {1, 1}, {2, 2}, {3, 3}})},
                {"1 3", states({{0, 1}, {4, 3}})}, {"all late", states({{4, 1}, {3, 2}, {2, 3}})}};
            for (const auto& [name, memory] : layouts) {
                // Not the host's competition (the record's id is not the cell's): the virtual +0x90 decides the status call.
                for (const std::uint32_t answer : {0u, 1u}) {
                    auto [vt, answers] = virtual_90(answer);
                    answers.insert(answers.end(), calm.begin(), calm.end());
                    answers.push_back({host_status, 0});
                    cases.push_back({.entry = final_places, .registers = {{3, cup.object}}, .memory = joined(joined(memory, vt), {w32(host_cell, id + 1), w32(table + id * 4, cup.object)}),
                                     .answers = answers, .label = label(std::string("final_places ") + name + " virtual " + std::to_string(answer), cup)});
                }
                // The real virtual +0x90 and the real calls (the descriptor rule answered).
                cases.push_back({.entry = final_places, .registers = {{3, cup.object}}, .memory = joined(memory, {w32(host_cell, id + 1)}), .label = label(std::string("final_places real ") + name, cup)});
            }
            // The host loop: the record's id is the cell's; the statistics' row of the nth entrant has state 3 (or none does).
            for (const std::uint32_t hit : {0u, 1u, 2u, 0xffu}) {
                const auto answers = Answers{{stats_row, stats_miss}, {place_final, 0}, {fixture_rules, 1}, {host_status, 0}, {dummy_b, 0}};
                std::vector<Case::NthAnswer> nth;
                if (hit != 0xffu) nth.push_back({stats_row, hit + 1, stats_hit});
                cases.push_back({.entry = final_places, .registers = {{3, cup.object}},
                                 .memory = joined(joined(states({{0, 1}, {1, 2}, {2, 3}}), redirected(scenario, cup.object, {{0x90, dummy_b}})),
                                                  {w32(host_cell, id), w8(stats_hit + 0x76, 3), w8(stats_miss + 0x76, 1), w32(table + id * 4, cup.object)}),
                                 .answers = answers, .nth_answers = nth, .label = label("final_places host loop hit " + hex(hit), cup)});
            }
            cases.push_back({.entry = final_places, .registers = {{3, cup.object}}, .memory = joined(states({{0, 1}, {1, 2}, {2, 3}}), {w32(host_cell, id)}),
                             .answers = {{fixture_rules, 1}, {place_final, 0}}, .label = label("final_places host loop real statistics", cup)});
            // The host status: 0 none, 1 one row, 2 two rows (the smaller halfword +0xac, the first on a tie), 0xff.
            const auto row = [&](std::uint32_t index, std::uint32_t half) { return w16(nations + index * 0x128 + 0xac, half); };
            for (const std::uint32_t status : {0u, 1u, 2u, 3u, 0xffu, 0x102u}) {
                const std::vector<std::pair<std::uint32_t, std::uint32_t>> halves = {{5, 6}, {6, 5}, {5, 5}, {0xffff, 1}, {0x8000, 0x7fff}};
                for (const auto& [a, b] : halves) {
                    if (status != 2u && (a != 5 || b != 6)) continue;
                    auto [vt, answers] = virtual_90(1);
                    answers.insert(answers.end(), {{host_status, status}, {club_to_row, 0x4242}, {place_final, 0}, {fixture_rules, 1}});
                    cases.push_back({.entry = final_places, .registers = {{3, cup.object}},
                                     .memory = joined(joined(states({{0, 1}, {1, 2}, {2, 3}}), vt), {w32(host_cell, id + 1), w32(table + id * 4, cup.object), row(1, a), row(2, b)}),
                                     .answers = answers,
                                     .pokes = {{host_status, {w32(out_first, 1), w32(out_second, 2)}}},
                                     .label = label("final_places host status " + hex(status) + " halves " + hex(a) + " " + hex(b), cup)});
                }
            }
            // The descriptor rule: the fixture rules refuse; the nation's descriptor names this record or not; the nation is -1.
            for (const std::uint32_t refuse : {1u, 0u})
                for (const std::uint32_t nation : {0u, 5u, 0xffu, 0x80u, 33u})
                    for (const bool names : {true, false}) {
                        if (refuse && (nation != 0 || !names)) continue;
                        auto [vt, answers] = virtual_90(0);
                        answers.insert(answers.end(), {{host_status, 0}, {place_final, 0}, {fixture_rules, refuse}, {nation_of, nation}});
                        const auto at = descriptors + std::uint32_t(std::int32_t(std::int8_t(nation))) * 0x4c + 0x20;
                        cases.push_back({.entry = final_places, .registers = {{3, cup.object}},
                                         .memory = joined(joined(states({{0, 1}, {1, 2}, {2, 3}}), vt), {w32(host_cell, id + 1), w32(table + id * 4, cup.object), w32(at, names ? cup.record : 0x1234)}),
                                         .answers = answers,
                                         .label = label("final_places descriptor refuse " + std::to_string(refuse) + " nation " + hex(nation) + (names ? " names" : " other"), cup)});
                    }
        }
        if (taken == 0) throw std::runtime_error("queries: no usable top-level cup for final_places");
    }

    // ---- 0x1047b5b0 (vt +0x4c): records a place with the staff-records object -----------------------------------------
    {
        const Answers history = {{history_win, 0}, {history_two, 0}, {history_three, 0}, {history_round, 0}, {history_other, 0}};
        const auto answered = [&](Answers more) { auto all = history; all.insert(all.end(), more.begin(), more.end()); return all; };
        const auto table_cell = scenario.toc(-0x7984);
        const auto club_table = scenario.r32(table_cell);
        const auto descriptors = scenario.toc(-0x7b44);
        const auto cell_691c = scenario.toc(-0x691c), cell_7a70 = scenario.toc(-0x7a70), cell_7a4c = scenario.toc(-0x7a4c);
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            if (++taken > 3) break;
            const auto club = w.club(7 + unsigned(taken));
            const auto club_id = scenario.r32(club);
            const auto id = w.id(cup);
            const std::vector<Write> ready = {w32(cup.object + 0x2c, 0), w8(cup.object + 0x44, 0xff)};
            const auto place = [&](const std::string& what, std::uint32_t kind, std::vector<Write> memory, Answers more = {}, std::uint32_t fixture_byte = 0) {
                cases.push_back({.entry = place_record, .registers = {{3, cup.object}, {4, club}, {5, kind}, {8, fixture_at}, {9, 0x7777}},
                                 .memory = joined(joined(ready, {w8(fixture_at + 0x3a, fixture_byte), w16(fixture_at + 0x34, 1), w16(fixture_at + 0x32, 0x2001), w16(fixture_at + 0x30, 0x96)}), memory),
                                 .answers = answered(std::move(more)), .label = label(what + " kind " + hex(kind), cup)});
            };
            // The early returns.
            place("fixture byte 1", 1, {}, {}, 1);
            place("fixture byte 2", 2, {}, {}, 2);
            place("holder +0x2c", 2, {w32(cup.object + 0x2c, 1)});
            place("owner 0", 2, {w8(cup.object + 0x44, 0)});
            place("owner 0x7f", 2, {w8(cup.object + 0x44, 0x7f)});
            // The kinds that call the staff-records object, and the others.
            for (const std::uint32_t kind : {2u, 3u, 4u, 0x80u, 0x105u, 0x102u, 0x103u, 0x100u})
                place("history call", kind, {});
            // The staff-records object's real routines (the club histories, the file layer answered) for every kind. The first cup of
            // the career makes them fault on both sides (a read at 0xbfa28040 in 0x10383588 / 0x10384188, whatever the date), so its
            // real history stays answered above.
            for (const std::uint32_t kind : {2u, 3u, 0u, 0xffffffffu, 1u}) {
                if (taken < 2) continue;
                cases.push_back({.entry = place_record, .registers = {{3, cup.object}, {4, club}, {5, kind}, {8, fixture_at}, {9, nation_at + 0x80}},
                                 .memory = joined(ready, {w16(nation_at + 0x80, 100), w16(nation_at + 0x82, 2001), w8(nation_at + 0x84, 0), w8(fixture_at + 0x3a, 0), w16(fixture_at + 0x34, 1), w16(fixture_at + 0x32, 0x2001), w16(fixture_at + 0x30, 0x96),
                                                          w32(fixture_at, cup.object), w32(fixture_at + 0x14, cup.record), w32(fixture_at + 0x1c, club), w32(fixture_at + 0x20, w.club(30)),
                                                          w32(fixture_at + 0xc, club_id), w32(fixture_at + 0x10, scenario.r32(w.club(30)))}),
                                 .answers = {{0x10748ce0, 0}}, .label = label("real history kind " + hex(kind), cup)});
            }
            // Kind 1: the club table's entry when the club is free and the competition is its nation's champion seat.
            const auto record = cup.record;
            const auto kind1 = [&](const std::string& what, bool same_id, bool holder_ok, bool object_ok, bool class_ok, std::uint32_t rules, bool free_slot,
                                   std::uint32_t nation, bool names) {
                const std::uint32_t constant = 0x5151;
                const auto at = descriptors + std::uint32_t(std::int32_t(std::int8_t(nation))) * 0x4c + 0x20;
                place(what, 1,
                      {w32(cell_691c, same_id ? id : id + 1), w32(record + 8, holder_ok ? holder_at : 0), w32(holder_at + 0x74, object_ok ? nation_at : 0),
                       w32(nation_at + 8, class_ok ? constant : constant + 1), w32(cell_7a70, constant), w32(club_table + club_id * 8, free_slot ? 0xffffffffu : 5),
                       w32(at, names ? record : 0x1234), w32(cell_7a4c, 0x11223344)},
                      {{fixture_rules, rules}, {nation_of, nation}});
            };
            kind1("same id", true, true, true, true, 0, true, 3, true);
            kind1("same id, not free", true, true, true, true, 0, false, 3, true);
            kind1("all conditions", false, true, true, true, 0, true, 3, true);
            kind1("no holder", false, false, true, true, 0, true, 3, true);
            kind1("no object", false, true, false, true, 0, true, 3, true);
            kind1("other class", false, true, true, false, 0, true, 3, true);
            kind1("rules refuse", false, true, true, true, 1, true, 3, true);
            kind1("slot taken", false, true, true, true, 0, false, 3, true);
            kind1("nation -1", false, true, true, true, 0, true, 0xff, true);
            kind1("descriptor of another", false, true, true, true, 0, true, 3, false);
            kind1("nation 0", false, true, true, true, 0, true, 0, true);
            kind1("rules 0x100", false, true, true, true, 0x100, true, 3, true);
            // Kind 0: the round after the played one, and its share of the rounds.
            const auto rounds = std::max(cup.rounds, 3);
            for (const std::int32_t round : {-1, 0, 1, rounds - 2, rounds - 1, rounds, 0x7ffe, 0x7fff, 0xffff}) {
                for (const std::int32_t label_next : {0x64, 0x8c, 0}) {
                    if (round == -1 && cup.rounds < 2) continue;
                    place("round " + std::to_string(round) + " next " + hex(std::uint32_t(label_next)), 0,
                          {w16(fixture_at + 0x34, std::uint32_t(round)), w16(w.round_row(cup, 0) + 8, std::uint32_t(label_next)),
                           w16(w.round_row(cup, 1) + 8, std::uint32_t(label_next)), w16(w.round_row(cup, 2) + 8, std::uint32_t(label_next))});
                    place("round " + std::to_string(round), 0xffffffffu, {w16(fixture_at + 0x34, std::uint32_t(round))});
                }
            }
            // The round count at the halfword's edges (a negative count: no next round; the share needs a count of 2 or more).
            for (const std::uint32_t count : {2u, 0xffffu, 0x7fffu, 3u}) {
                place("rounds " + hex(count), 0, {w16(cup.object + 0x3c, count), w16(fixture_at + 0x34, 0)});
                place("rounds " + hex(count), 0xffffffffu, {w16(cup.object + 0x3c, count), w16(fixture_at + 0x34, 1)});
            }
        }
    }

    // ---- 0x1047b360 (vt +0x48): the news item of a result ---------------------------------------------------------------
    {
        const auto sp = scenario.sp();
        const auto out_byte = sp - 0x140 + 0x39, field_byte = sp - 0x140 + 0x38;
        const auto table = scenario.global(competitions_slot);
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            if (!w.self(cup) || w.s16(w.round_row(cup, 0) + 8) < 0) continue;
            if (++taken > 3) break;
            const auto id = w.id(cup);
            const auto club = w.inbox_club(unsigned(taken));
            const auto virtual_4c = [&] { return redirected(scenario, cup.object, {{0x4c, dummy_a}}); };
            const auto run = [&](const std::string& what, std::uint32_t kind, std::uint32_t code, std::int32_t round, std::uint32_t answer, std::uint32_t rules, std::uint32_t out,
                                 std::vector<Write> memory = {}, std::vector<Case::Poke> pokes = {}, std::uint32_t fixture = fixture_at, Answers more = {}) {
                const auto away = w.inbox_club(9 + unsigned(taken));
                auto all = joined(joined(virtual_4c(), {w32(table + id * 4, cup.object), w16(fixture_at + 0x30, code), w16(fixture_at + 0x32, 0x2002), w16(fixture_at + 0x34, std::uint32_t(round)),
                                                       w32(club + 0x2c, 0), w32(fixture_at, cup.object), w32(fixture_at + 0xc, scenario.r32(club)), w32(fixture_at + 0x10, scenario.r32(away)),
                                                       w32(fixture_at + 0x14, cup.record), w32(fixture_at + 0x1c, club), w32(fixture_at + 0x20, away)}), memory);
                pokes.push_back({fixture_rules, {w8(out_byte, out)}});
                Answers answers = {{dummy_a, answer}, {fixture_rules, rules}, {item_enabled, 1}};
                answers.insert(answers.end(), more.begin(), more.end());
                cases.push_back({.entry = news_result, .registers = {{3, cup.object}, {4, club}, {5, kind}, {6, 0x66}, {7, fixture}, {8, 0x88}}, .memory = std::move(all),
                                 .answers = std::move(answers), .pokes = std::move(pokes), .label = label(what + " kind " + hex(kind) + " code " + hex(code), cup)});
            };
            // The cup's own virtual +0x4c (the base class's record_place here, history calls answered): the real answer 0.
            for (const std::uint32_t kind : {1u, 2u, 3u, 0u}) {
                const auto away = w.inbox_club(9 + unsigned(taken));
                cases.push_back({.entry = news_result, .registers = {{3, cup.object}, {4, club}, {5, kind}, {6, 0x66}, {7, fixture_at}, {8, 0x88}},
                                 .memory = {w32(fixture_at, cup.object), w16(fixture_at + 0x30, 0x96), w16(fixture_at + 0x34, 1), w32(fixture_at + 0xc, scenario.r32(club)), w32(fixture_at + 0x10, scenario.r32(away)),
                                            w32(fixture_at + 0x14, cup.record), w32(fixture_at + 0x1c, club), w32(fixture_at + 0x20, away), w32(cup.object + 0x2c, 0), w8(cup.object + 0x44, 0xff)},
                                 .answers = {{fixture_rules, 0}, {item_enabled, 1}, {0x10383410, 0}, {0x10383b70, 0}, {0x10384020, 0}, {0x10384500, 0}, {0x103846e0, 0}},
                                 .label = label("real virtual +0x4c kind " + hex(kind), cup)});
            }
            // The refusals.
            run("rules refuse", 1, 0x96, 0, 4, 1, 0xff);
            run("virtual answers -1", 1, 0x96, 0, 0xffffffffu, 0, 0xff);
            run("no fixture", 1, 0x96, 0, 4, 0, 0xff, {}, {}, 0);
            // The filings: the kinds and codes that file under the competition's record, the others under the club.
            for (const std::uint32_t kind : {1u, 3u, 0u, 2u, 5u, 0xffu, 0x101u, 0x103u})
                for (const std::uint32_t code : {0x96u, 0x8cu, 0x64u, 0u})
                    run("filing", kind, code, 1, 4, 0, 0xff);
            // The out byte: anything but -1 files with the competition (the club's nation with and without a +0x74).
            for (const std::uint32_t out : {0u, 1u, 0x7fu, 0x80u, 0xfeu}) {
                run("out byte " + hex(out) + " no nation", 1, 0x96, 1, 4, 0, out);
                run("out byte " + hex(out) + " with nation", 1, 0x96, 1, 4, 0, out, {w32(club + 0x2c, holder_at), w32(holder_at + 0x74, 0x7777)});
                run("out byte " + hex(out) + " nation without +0x74", 3, 0x8c, 1, 4, 0, out, {w32(club + 0x2c, holder_at), w32(holder_at + 0x74, 0)});
            }
            // The out byte picks the filing of 0x102cc030: 0 the club, 1 the competition's record, 2 the nation, 3 the nation's +0x74,
            // 5 the record again, 4 another; the club's real nation (+0x2c) is left in place for these.
            for (const std::uint32_t out : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
                for (const std::uint32_t code : {0x96u, 0x64u})
                    run("out byte " + hex(out) + " real nation", 3, code, 1, 4, 0, out, {w32(club + 0x2c, scenario.r32(club + 0x2c))});
            // The next round's label (the field after the round: its label or -1) and the round count.
            const auto rounds = cup.rounds;
            for (const std::int32_t round : {0, 1, rounds - 2, rounds - 1, rounds, -1, 0x7fff})
                for (const std::uint32_t next : {0x64u, 0x8cu, 0u}) {
                    if (round < -1 || round > rounds + 1) {
                        if (next != 0x64u) continue;
                    }
                    std::vector<Write> labels;
                    for (std::int32_t r = 0; r < std::min(rounds, 6); ++r) labels.push_back(w16(w.round_row(cup, r) + 8, next));
                    run("round " + std::to_string(round) + " next " + hex(next), 1, 0x64, round, 4, 0, 0xff, labels);
                }
            run("round count 0", 1, 0x64, 0, 4, 0, 0xff, {w16(cup.object + 0x3c, 0)});
            run("round count -1", 1, 0x64, 0, 4, 0, 0xff, {w16(cup.object + 0x3c, 0xffff)});
            // The byte the field-index call leaves in the frame (5 as set, or what the call writes).
            for (const std::uint32_t byte : {5u, 6u, 7u, 0u, 0xffu})
                run("field byte " + hex(byte), 1, 0x64, 0, 4, 0, 0xff, {}, {{field_byte_call, {w8(field_byte, byte)}}}, fixture_at, {{field_byte_call, 0}});
            // The virtual's answer (the item's field 4) and the rules' low byte.
            for (const std::uint32_t answer : {0u, 5u, 0x7fffffffu, 0x80000000u, 0xfffffffeu})
                run("answer " + hex(answer), 1, 0x96, 1, answer, 0, 0xff);
            run("rules 0x100", 1, 0x96, 1, 4, 0x100, 0xff);
            run("rules 0xff", 1, 0x96, 1, 4, 0xff, 0xff);
        }
        if (taken == 0) throw std::runtime_error("queries: no top-level cup for the news cases");
    }

    // ---- 0x10485930: the prize money of a tie ---------------------------------------------------------------------------
    {
        const auto winner_row = row_at, loser_row = row_at + 0x10;
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            if (cup.rounds < 3) continue;
            if (++taken > 2) break;
            const auto winner = w.inbox_club(20 + unsigned(taken)), loser = w.inbox_club(30 + unsigned(taken));
            const std::uint32_t big = 0x7fffffffu, negative = 0x80000000u;
            const auto run = [&](const std::string& what, std::int32_t round, std::uint32_t prize_a, std::uint32_t prize_b, std::uint32_t pool, std::uint32_t state,
                                 std::uint32_t code, std::uint32_t ties, std::uint32_t next_label, std::vector<Write> more = {}, Answers answers = {{item_enabled, 1}}) {
                const auto rounds_at = w.round_row(cup, round < 0 ? 0 : round);
                std::vector<Write> memory = {w32(winner_row, winner), w8(winner_row + 5, state), w32(loser_row, loser), w8(loser_row + 5, 1),
                                             w16(fixture_at + 0x30, code), w32(fixture_at + 0x1c, winner), w32(fixture_at + 0x20, loser), w8(fixture_at + 0x3d, 0), w8(fixture_at + 0x3e, 0),
                                             w32(cup.object + 0x28, pool), w16(rounds_at + 0x1c, ties), w32(rounds_at + 0x70, prize_a), w32(rounds_at + 0x74, prize_b),
                                             w16(w.round_row(cup, (round < 0 ? 0 : round) + 1) + 8, next_label), w16(winner + 0x1ec, 20), w16(loser + 0x1ec, 20)};
                append(memory, more);
                cases.push_back({.entry = pay_prizes, .registers = {{3, cup.object}, {4, winner_row}, {5, loser_row}, {6, std::uint32_t(round)}, {7, fixture_at}}, .memory = std::move(memory),
                                 .answers = std::move(answers), .label = label(what + " round " + std::to_string(round), cup)});
            };
            const std::pair<std::uint32_t, std::uint32_t> prizes[] = {{0xffffffffu, 0xffffffffu}, {0xffffffffu, 40000}, {50000, 0xffffffffu}, {50000, 7000}, {0, 0}, {0, 7000},
                                                                    {50000, 0}, {0xffffffffu, 0}, {big, big}, {negative, negative}, {1000, 100}, {0xfffffff0u, 5}};
            for (const std::int32_t round : {0, 1, cup.rounds - 2, cup.rounds - 1})
                for (const auto& [a, b] : prizes)
                    run("prizes " + hex(a) + " " + hex(b), round, a, b, 4000000, 1, 0x64, 4, 0x64);
            // The share of the pool: the pool's sign and size, the ties (a negative one), the rounds left.
            for (const std::uint32_t pool : {0u, 1u, 7u, 0x7fffffffu, 0x80000000u, 4000000u, 0xfffff000u})
                for (const std::uint32_t ties : {1u, 2u, 3u, 0xffffu, 0x8000u})
                    run("share pool " + hex(pool) + " ties " + hex(ties), 0, 0xffffffffu, 0xffffffffu, pool, 1, 0x64, ties, 0x64);
            run("share winner only pool 100", 1, 0xffffffffu, 3, 100, 1, 0x64, 2, 0x64);
            // The state byte and the competition code: state 1 in competition 0x136 becomes 3.
            for (const std::uint32_t state : {0u, 1u, 2u, 3u, 0x80u, 0xffu})
                for (const std::uint32_t code : {0x136u, 0x64u, 0x137u, 0u})
                    run("state " + hex(state) + " code " + hex(code), 1, 50000, 7000, 4000000, state, code, 4, 0x64);
            // The next round's label: beyond the last round, the 0x8c round, another.
            for (const std::int32_t round : {cup.rounds - 1, cup.rounds - 2, 0})
                for (const std::uint32_t next : {0x8cu, 0x64u, 0u, 0xffffu})
                    run("next label " + hex(next), round, 50000, 7000, 4000000, 1, 0x64, 4, next);
            // The news item's threshold (50 times the club's halfword +0x1ec) and the news switch off.
            for (const std::uint32_t half : {0u, 20u, 1000u, 0x7fffu, 0x8000u, 0xffffu})
                run("threshold " + hex(half), 1, 50000, 7000, 4000000, 1, 0x64, 4, 0x64, {w16(winner + 0x1ec, half)});
            run("news off", 1, 50000, 7000, 4000000, 1, 0x64, 4, 0x64, {}, {{item_enabled, 0}});
            run("news real", 1, 50000, 7000, 4000000, 1, 0x64, 4, 0x64, {}, {});
            // A fixture whose home club is the loser, and one with the sides' flags set.
            run("loser at home", 1, 50000, 7000, 4000000, 1, 0x64, 4, 0x64, {w32(fixture_at + 0x1c, loser), w32(fixture_at + 0x20, winner)});
            run("flags set", 1, 50000, 7000, 4000000, 1, 0x64, 4, 0x64, {w8(fixture_at + 0x3d, 1), w8(fixture_at + 0x3e, 1), w8(fixture_at + 0x4b, 3), w8(fixture_at + 0x4c, 1)});
            // The round index at the halfword's edges (a prize of -1 would divide by the rounds left, so fixed prizes).
            for (const std::int32_t round : {-1, 0x7ffe, 0x7fff, 0xffff})
                run("fixed prizes", round, 50000, 7000, 4000000, 1, 0x64, 4, 0x64);
        }
        if (taken == 0) throw std::runtime_error("queries: no cup of three rounds for the prizes");
    }

    // ---- 0x10485b10 (vt +0x64): the ranks given to the entrants ------------------------------------------------------
    {
        constexpr std::uint32_t new_array = 0x10775130;
        std::size_t taken = 0;
        for (const auto& cup : knockouts) {
            const auto good = w.good_entrants(cup, 24);
            if (!cup.stats || good.size() < 8 || good.back() != std::int32_t(good.size()) - 1 || cup.rounds < 3) continue;
            if (++taken > 3) break;
            const auto run = [&](const std::string& what, std::vector<Write> memory, Answers answers = {}) {
                cases.push_back({.entry = rank_teams, .registers = {{3, cup.object}}, .memory = std::move(memory), .answers = std::move(answers), .label = label("rank_teams " + what, cup)});
            };
            run("real", {});
            // The rounds and their ties: the last but one round's ties are ranked first and counted again as the first round
            // of the walk down, so the clubs ranked are ties[R-2] + the sum of ties[0..R-2]; a club count that is exactly
            // that, or larger (the rest unranked). The ranks reach the real statistics (COMP_STATS 0x10457490).
            const std::vector<std::vector<std::uint32_t>> tie_lists = {{1}, {2}, {3}, {1, 1}, {2, 1}, {1, 2}, {2, 3}, {3, 2}, {1, 1, 1}, {2, 1, 2}, {3, 2, 1}, {1, 0, 2}, {0, 2},
                                                                    {2, 2, 1, 1}, {4, 2, 1}, {1, 1, 1, 1}, {3, 0, 1, 2}, {0, 0}, {5, 3}};
            for (const auto& ties : tie_lists) {
                const auto rounds = std::uint32_t(ties.size()) + 1;      // the last round (the final) has its own row
                std::uint32_t need = ties.back();
                for (const auto t : ties) need += t;
                if (need > good.size()) continue;
                for (const std::uint32_t extra : {0u, 2u}) {
                    if (need + extra > good.size()) continue;
                    std::vector<Write> memory = {w16(cup.object + 0x3c, rounds), w16(cup.object + 0x3e, need + extra), w16(w.round_row(cup, std::int32_t(rounds) - 1) + 0x1c, 1)};
                    std::string name;
                    for (std::size_t r = 0; r < ties.size(); ++r) { memory.push_back(w16(w.round_row(cup, std::int32_t(r)) + 0x1c, ties[r])); name += std::to_string(ties[r]) + " "; }
                    run("rounds " + std::to_string(rounds) + " ties " + name + "clubs +" + std::to_string(extra), memory);
                }
            }
            for (const std::uint32_t rounds : {1u, 0u, 0xffffu, 0x7fffu}) {
                run("rounds " + hex(rounds), {w16(cup.object + 0x3c, rounds), w16(cup.object + 0x3e, 6)});
                run("rounds " + hex(rounds) + " no clubs", {w16(cup.object + 0x3c, rounds), w16(cup.object + 0x3e, 0)});
            }
            run("one club", {w16(cup.object + 0x3c, 1), w16(cup.object + 0x3e, 1)});
            // A club row that is null: the passes skip it (and the outer loop stops at a null club).
            for (const std::int32_t at : {0, 3, 7}) run("null club " + std::to_string(at), {w16(cup.object + 0x3c, 3), w16(cup.object + 0x3e, 8), w16(w.round_row(cup, 0) + 0x1c, 3), w16(w.round_row(cup, 1) + 0x1c, 2), w32(w.entrant_row(cup, at), 0)});
            // The statistics missing; the array's allocation failing (answered, and for real with a negative count).
            run("no statistics", {w32(cup.object + 8, 0)});
            run("allocation answered 0", {}, {{new_array, 0}});
            run("allocation fails for a negative count", {w16(cup.object + 0x3e, 0xffff)});
            run("allocation fails for the most negative count", {w16(cup.object + 0x3e, 0x8000)});
            run("no entrants (count 0)", {w16(cup.object + 0x3e, 0)});
        }
        if (taken == 0) throw std::runtime_error("queries: no cup with statistics and eight entrants for the ranks");
    }
}
}
}

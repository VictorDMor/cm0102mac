// Cases for LEAGUE.CPP block `table` (league_base_table*.cpp): the standings' order, the match results and the
// queries, on the career's real leagues (`World`: every league object of the competitions and their stages),
// their real rows and clubs; the match record (a fixture) the results read is built in the scratch area from
// real club words. Inputs the real rows do not offer (tie levels, groups, tie-break lists, played counts, the
// alternative comparator) are written over the real rows for the call and undone afterwards (Case::memory).
#include "league_base_checks.hpp"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t sort_standings = 0x1049eb30, rank_head_to_head = 0x1049f030, compare_standings = 0x1049f9a0,
                        compare_by_name = 0x1049fc80, compare_head = 0x1049fdc0, compare_tied = 0x1049ff90,
                        decides_title = 0x104a26b0, apply_result = 0x104a3930, apply_rows = 0x104a3b90,
                        count_head = 0x104a4870, fixture_slot = 0x104a9aa0, evaluate = 0x104a9bc0, limit_byte = 0x104aa240,
                        mark_clubs = 0x104aa340, points_result = 0x104aa430, find_standing = 0x104aa560,
                        has_standing = 0x104aa5c0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t row_size = 0x44;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;
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
    std::uint32_t u8(std::uint32_t at) const { return s.r32(at & ~3u) >> (24 - 8 * (at & 3)) & 0xff; }
    std::uint32_t u16(std::uint32_t at) const { return (u8(at) << 8) | u8(at + 1); }
    std::int32_t clubs(std::uint32_t l) const { return std::int16_t(u16(l + 0x3e)); }
    std::uint32_t rows(std::uint32_t l) const { return s.r32(l + 0xb0); }
    std::uint32_t row(std::uint32_t l, std::uint32_t i) const { return rows(l) + i * row_size; }
    // Leagues with at least `least` rows; `h2h` picks those with (true) or without (false) head-to-head tables.
    std::vector<std::uint32_t> with_rows(std::int32_t least, int h2h = -1) const {
        std::vector<std::uint32_t> found;
        for (const auto l : leagues) {
            if (!rows(l) || clubs(l) < least) continue;
            const bool has = s.r32(rows(l) + 8) != 0;
            if (h2h >= 0 && has != (h2h != 0)) continue;
            found.push_back(l);
        }
        return found;
    }
};
struct Memory {
    std::vector<Write> w;
    Memory& b(std::uint32_t at, std::uint32_t v) { w.push_back({1, at, v & 0xff}); return *this; }
    Memory& h(std::uint32_t at, std::uint32_t v) { w.push_back({2, at, v & 0xffff}); return *this; }
    Memory& l(std::uint32_t at, std::uint32_t v) { w.push_back({4, at, v}); return *this; }
    Memory& toc(std::int32_t slot, std::uint32_t v) { return l(toc_base + std::uint32_t(slot), v); }
    // The sort's TOC words: the league and the alternative comparator descriptor.
    Memory& sort_words(std::uint32_t league, std::uint32_t alternative) { return toc(-0x3750, league).toc(-0x3754, alternative); }
    Memory& ties(std::uint32_t league, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d) {
        return this->b(league + 0xc7, a).b(league + 0xc8, b).b(league + 0xc9, c).b(league + 0xca, d);
    }
    Memory& copy_row(const World& world, std::uint32_t to, std::uint32_t from, std::uint32_t at, unsigned size) {
        for (std::uint32_t offset = at; offset < at + size; ++offset) b(to + offset, world.u8(from + offset));
        return *this;
    }
};
[[maybe_unused]] std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
Case make(std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, std::string label) {
    Case item{.entry = entry, .registers = registers, .memory = std::move(memory)};
    item.label = "table: " + std::move(label);
    return item;
}
// The tie-break lists the comparators are checked with (every code, the keep-order code, unknown codes, the end).
constexpr std::uint32_t tie_lists[][4] = {{1, 2, 3, 4}, {2, 1, 5, 6}, {3, 0, 0, 0}, {4, 1, 2, 0}, {5, 3, 1, 2}, {6, 1, 0, 0},
                                          {0, 0, 0, 0}, {7, 1, 0, 0}, {0xff, 2, 0, 0}, {1, 1, 1, 1}, {3, 3, 5, 5}, {6, 6, 6, 6}};
struct Rng {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
};
[[maybe_unused]] constexpr std::uint32_t fixture_at = scratch_base + 0x2000, marker_at = scratch_base + 0x2200, out_home = scratch_base + 0x2300,
                        out_away = scratch_base + 0x2310;
// A match record (the fixture) in the scratch area: the two clubs, the flags halfword, the stage byte, the goals,
// the extra-time goals, the penalties.
struct FixtureSpec {
    std::uint32_t home = 0, away = 0, flags = 0, stage = 0xff, home_goals = 0, away_goals = 0, extra_home = 0xff, extra_away = 0xff,
                  penalties_home = 0, penalties_away = 0, half_34 = 5;
};
[[maybe_unused]] Memory fixture_memory(const FixtureSpec& f) {
    Memory m;
    m.l(fixture_at + 0x1c, f.home).l(fixture_at + 0x20, f.away).h(fixture_at + 0x34, f.half_34).h(fixture_at + 0x38, f.flags)
        .b(fixture_at + 0x3d, 0).b(fixture_at + 0x3e, 0).b(fixture_at + 0x42, f.stage).b(fixture_at + 0x43, f.home_goals)
        .b(fixture_at + 0x44, f.away_goals).b(fixture_at + 0x47, f.extra_home).b(fixture_at + 0x48, f.extra_away)
        .b(fixture_at + 0x49, f.penalties_home).b(fixture_at + 0x4a, f.penalties_away);
    return m;
}
// The rows' statistics scrambled with ties: every row has the same points and played count; goal difference, goals
// scored, wins, away goals and positions vary with the seed. `entries` also scrambles the head-to-head entries
// (club-level played/won/drawn/lost/scored/conceded/points), or zeroes them.
Memory scramble(const World& w, std::uint32_t league, std::uint32_t seed, int entries) {
    Memory m;
    Rng rng{seed * 2654435761u + 12345u};
    const auto n = std::uint32_t(w.clubs(league));
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto row = w.row(league, i);
        m.b(row + 0x14, 10).h(row + 0x1c, 20 + (i % 4 == 0 ? 0 : seed % 2)).h(row + 0x18, 10 + rng.below(6)).h(row + 0x1a, 8 + rng.below(6))
            .b(row + 0x15, 3 + rng.below(3)).h(row + 0x38, rng.below(4)).b(row + 4, rng.below(n));
        if (entries >= 0 && w.s.r32(row + 8)) {
            const auto table = w.s.r32(row + 8);
            for (std::uint32_t k = 0; k + 1 < n; ++k) {
                const auto e = table + k * 0x10;
                if (entries == 0) m.b(e + 4, 0).b(e + 5, 0).b(e + 6, 0).b(e + 7, 0).h(e + 8, 0).h(e + 0xa, 0).h(e + 0xc, 0);
                else m.b(e + 4, 2).b(e + 5, rng.below(3)).b(e + 6, rng.below(2)).b(e + 7, rng.below(3)).h(e + 8, rng.below(5)).h(e + 0xa, rng.below(5)).h(e + 0xc, rng.below(7));
            }
        }
    }
    return m;
}
}

LEAGUE_CASES("table: probe") {
    if (scenario.match || !std::getenv("CM_LEAGUE_TABLE_PROBE")) return;
    const World w(scenario);
    for (const auto l : w.with_rows(4, 1)) {
        std::fprintf(stderr, "ROWS of %x\n", l);
        for (std::int32_t i = 0; i < w.clubs(l); ++i) {
            const auto r = w.row(l, i);
            std::fprintf(stderr, " row %d club %x pos %u pld %u w %u d %u l %u gf %d ga %d pts %d | home pld %u pts %d | away pld %u pts %d | 40:%02x h2h %x hist %x\n", i, scenario.r32(r), w.u8(r + 4),
                         w.u8(r + 0x14), w.u8(r + 0x15), w.u8(r + 0x16), w.u8(r + 0x17), std::int16_t(w.u16(r + 0x18)), std::int16_t(w.u16(r + 0x1a)), std::int16_t(w.u16(r + 0x1c)),
                         w.u8(r + 0x24), std::int16_t(w.u16(r + 0x2c)), w.u8(r + 0x34), std::int16_t(w.u16(r + 0x3c)), w.u8(r + 0x40), scenario.r32(r + 8), scenario.r32(r + 0xc));
        }
        break;
    }
    for (const auto l : w.leagues) {
        const auto rows = w.rows(l);
        std::fprintf(stderr, "league %x vt %x n=%d group=%x alt=%x a4=%d a6=%d tie=%02x%02x%02x%02x c0..c3=%02x %02x %02x %02x rec=%x flags=%x rows=%x row0.8=%x\n",
                     l, scenario.r32(l), w.clubs(l), scenario.r32(l + 0xac), scenario.r32(l + 0xa0), std::int16_t(w.u16(l + 0xa4)),
                     std::int16_t(w.u16(l + 0xa6)), w.u8(l + 0xc7), w.u8(l + 0xc8), w.u8(l + 0xc9), w.u8(l + 0xca), w.u8(l + 0xc0), w.u8(l + 0xc1),
                     w.u8(l + 0xc2), w.u8(l + 0xc3), scenario.r32(l + 4), w.u16(l + 0xdc), rows, rows ? scenario.r32(rows + 8) : 0);
    }
}

// ---- The comparators ----
LEAGUE_CASES("table: comparators") {
    if (scenario.match) return;
    const World w(scenario);
    // Real leagues: a head-to-head one (tie list 4,1,2), a plain one, the 20-club season-end one.
    std::vector<std::uint32_t> picks;
    for (const auto l : w.with_rows(6, 1)) { picks.push_back(l); break; }
    for (const auto l : w.with_rows(8, 0)) { picks.push_back(l); break; }
    for (const auto l : w.with_rows(18)) { if (w.u16(l + 0xa4) == w.u16(l + 0xa6)) { picks.push_back(l); break; } }
    if (picks.empty()) throw std::runtime_error("table: no league with rows");
    const std::uint32_t entries[] = {compare_standings, compare_by_name, compare_head, compare_tied};
    for (const auto l : picks) {
        const auto n = std::uint32_t(w.clubs(l));
        const std::pair<std::uint32_t, std::uint32_t> pairs[] = {{0, 1}, {1, 0}, {n - 1, n - 2}, {2, 2}, {1, 3 % n}};
        // The row fields compared, in the order of the tie levels: points, goal difference and scored, wins,
        // away goals scored, position.
        for (const auto [i, j] : pairs) {
            const auto a = w.row(l, i), b = w.row(l, j);
            if (a == b && i != 2) continue;
            for (unsigned group = 0; group < 3; ++group) {
                const std::uint32_t base = 0x14 + group * 0x10;
                for (unsigned level = 0; level <= 6; ++level) {
                    for (unsigned played = 0; played < 3; ++played) {
                        for (unsigned list = 0; list < sizeof tie_lists / sizeof tie_lists[0]; list += (level == 0 || level == 6) ? 1 : 3) {
                            Memory m;
                            m.sort_words(l, 0).l(l + 0xac, group);
                            m.ties(l, tie_lists[list][0], tie_lists[list][1], tie_lists[list][2], tie_lists[list][3]);
                            // The right row takes the left row's fields down to `level`.
                            if (level >= 1) m.copy_row(w, b, a, base + 8, 2);        // points
                            if (level >= 2) m.copy_row(w, b, a, base + 4, 4);        // scored and conceded
                            if (level >= 3) m.copy_row(w, b, a, base + 1, 1);        // wins
                            if (level >= 4) m.copy_row(w, b, a, 0x38, 2);            // away goals scored
                            if (level >= 5) m.copy_row(w, b, a, 4, 1);               // position
                            if (level >= 6) m.copy_row(w, b, a, 0x14, 0x30);         // everything else
                            if (played == 0) m.b(a + base, 0).b(b + base, 0);
                            if (played == 1) m.b(a + base, 7).b(b + base, 7);
                            if (played == 2) m.h(l + 0xa4, 9).b(a + base, 9).b(b + base, 9);
                            for (const auto entry : entries) {
                                // The head-to-head comparator reads the left row's table (+8): rows without one fault.
                                if (entry == compare_head && scenario.r32(a + 8) == 0) continue;
                                cases.push_back(make(entry, {{3, a}, {4, b}}, m.w,
                                                     hex(entry) + " compare " + hex(l) + " rows " + std::to_string(i) + "," + std::to_string(j) + " group " +
                                                     std::to_string(group) + " level " + std::to_string(level) + " played " + std::to_string(played) + " list " + std::to_string(list)));
                            }
                        }
                    }
                }
            }
        }
        // Null clubs make every pair equal; the alternative comparator (TOC -0x3754) answers first. The descriptors
        // are the pilot's three sort orders (words of the rows, reputation, the halfword at +0xa).
        const auto a = w.row(l, 0), b = w.row(l, 1);
        for (const auto entry : entries) {
            if (entry == compare_head && scenario.r32(a + 8) == 0) continue;
            cases.push_back(make(entry, {{3, a}, {4, b}}, Memory().sort_words(l, 0).l(a, 0).w, hex(entry) + " null left club"));
            cases.push_back(make(entry, {{3, a}, {4, b}}, Memory().sort_words(l, 0).l(b, 0).w, hex(entry) + " null right club"));
            for (const std::int32_t slot : {-0x4718, -0x4728, -0x472c, -0x4710, -0x6160}) {
                const auto word = scenario.r32(toc_base + std::uint32_t(slot));
                cases.push_back(make(entry, {{3, a}, {4, b}}, Memory().sort_words(l, word).w,
                                     hex(entry) + " alternative " + hex(std::uint32_t(slot)) + " " + hex(l)));
                cases.push_back(make(entry, {{3, b}, {4, a}}, Memory().sort_words(l, word).h(a + 0xa, 3).h(b + 0xa, 3).w,
                                     hex(entry) + " alternative equal " + hex(std::uint32_t(slot)) + " " + hex(l)));
            }
        }
    }
    // Every adjacent pair of every league that has rows, at the real state.
    for (const auto l : w.leagues) {
        if (!w.rows(l) || w.clubs(l) < 2) continue;
        for (std::int32_t i = 0; i + 1 < w.clubs(l) && i < 4; ++i)
            for (const auto entry : entries) {
                if (entry == compare_head && scenario.r32(w.row(l, i) + 8) == 0) continue;
                cases.push_back(make(entry, {{3, w.row(l, i)}, {4, w.row(l, i + 1)}}, Memory().sort_words(l, 0).w,
                                     hex(entry) + " real pair " + hex(l) + " " + std::to_string(i)));
            }
    }
}

// ---- The sort and the head-to-head pass ----
LEAGUE_CASES("table: sort") {
    if (scenario.match) return;
    const World w(scenario);
    // Every league at its real state: the league's own rows, then the same rows passed explicitly.
    for (const auto l : w.leagues) {
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().w, "sort real " + hex(l)));
        if (w.rows(l)) cases.push_back(make(sort_standings, {{3, l}, {4, w.rows(l)}}, Memory().w, "sort explicit rows " + hex(l)));
    }
    std::vector<std::uint32_t> picks;
    for (const auto l : w.with_rows(6, 1)) { picks.push_back(l); if (picks.size() == 3) break; }
    std::size_t plain = 0;
    for (const auto l : w.with_rows(6, 0)) { picks.push_back(l); if (++plain == 3) break; }
    for (const auto l : picks) {
        const auto rows = w.rows(l);
        const bool has_entries = scenario.r32(rows + 8) != 0;
        for (const auto& list : tie_lists) {
            for (const std::uint32_t group : {0u, 1u}) {
                for (const std::uint32_t owner : {0xffu, 0xfeu}) {
                    for (const std::uint32_t alternative : {0u, 1u}) {
                        for (int scrambled = 0; scrambled < 3; ++scrambled) {
                            if (scrambled && !has_entries && list[0] == 4) continue;
                            // A tie on points and the other keys, so the later keys and the bubble pass act.
                            auto m = scrambled ? scramble(w, l, scrambled * 7 + list[0], scrambled == 1 ? 0 : 1) : Memory();
                            m.ties(l, list[0], list[1], list[2], list[3]).l(l + 0xac, group).b(l + 0x44, owner);
                            m.l(l + 0xa0, alternative ? scenario.r32(toc_base + 0xffffb8e8u) : 0);
                            cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, m.w,
                                                 "sort " + hex(l) + " list " + hex(list[0] * 1000 + list[1] * 100 + list[2] * 10 + list[3]) + " group " +
                                                 std::to_string(group) + " owner " + hex(owner) + " alt " + std::to_string(alternative) + " scramble " + std::to_string(scrambled)));
                        }
                    }
                }
            }
        }
        // The played limit (+0xa4) left at -1 is filled from the rounds and club count (byte +0x45 set), and
        // the count 0, a null history, a row array passed in.
        for (const std::uint32_t flag : {0u, 1u})
            for (const std::uint32_t rounds : {0u, 2u, 38u})
                cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().h(l + 0xa4, 0xffff).b(l + 0x45, flag).h(l + 0x3c, rounds).w,
                                     "sort limit fill " + hex(l) + " flag " + std::to_string(flag) + " rounds " + std::to_string(rounds)));
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().h(l + 0x3e, 0).w, "sort no clubs " + hex(l)));
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().h(l + 0x3e, 0).h(l + 0xa4, 0xffff).b(l + 0x45, 1).w, "sort no clubs limit " + hex(l)));
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().l(rows + 0xc, 0).l(rows + 0xc + row_size, 0).w, "sort no history " + hex(l)));
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().l(l + 0xb0, 0).w, "sort null rows " + hex(l)));
        // The history pointer: a byte array; the played count past the limit.
        cases.push_back(make(sort_standings, {{3, l}, {4, 0}}, Memory().b(rows + 0x14, 0xff).b(rows + 0x14 + row_size, 0xe0).w, "sort history index out of range " + hex(l)));
    }
}

LEAGUE_CASES("table: rank") {
    if (scenario.match) return;
    const World w(scenario);
    std::size_t taken = 0;
    for (const auto l : w.with_rows(4, 1)) {
        const auto rows = w.rows(l);
        cases.push_back(make(rank_head_to_head, {{3, l}, {4, rows}}, Memory().sort_words(l, 0).w, "rank real " + hex(l)));
        for (const auto& list : tie_lists) {
            for (int scrambled = 0; scrambled < 4; ++scrambled) {
                auto m = scramble(w, l, scrambled * 5 + list[1], scrambled % 2);
                m.ties(l, list[0], list[1], list[2], list[3]).sort_words(l, 0);
                // Some of the rows level on points have the same position, and rows may be equal on everything.
                if (scrambled == 3) m.h(w.row(l, 1) + 0x1c, 20).h(w.row(l, 2) + 0x1c, 20).h(w.row(l, 1) + 0x18, 11).h(w.row(l, 2) + 0x18, 11);
                cases.push_back(make(rank_head_to_head, {{3, l}, {4, rows}}, m.w,
                                     "rank " + hex(l) + " list " + hex(list[0] * 1000 + list[1] * 100 + list[2] * 10 + list[3]) + " scramble " + std::to_string(scrambled)));
            }
        }
        // The allocation failure (assertion, line 0x17d): the club count is a signed halfword passed as the element count of
        // the clearing allocator 0x10775200 (element size 0x44). A negative count makes the 32-bit product 0x44 * count wrap to
        // 2^32 - 0x44 * |count|; the real allocator rejects sizes above 0xffffffb7, on both sides alike (no answer needed).
        for (const std::uint32_t negative : {0xffffu, 0xfffeu, 0x8000u})
            cases.push_back(make(rank_head_to_head, {{3, l}, {4, rows}}, scramble(w, l, 3, 1).h(l + 0x3e, negative).sort_words(l, 0).w,
                                 "rank " + hex(l) + " allocation fails, clubs " + hex(negative)));
        // Fewer clubs than rows, one club, none.
        for (const std::uint32_t count : {0u, 1u, 2u, 3u})
            cases.push_back(make(rank_head_to_head, {{3, l}, {4, rows}}, scramble(w, l, 3, 1).h(l + 0x3e, count).sort_words(l, 0).w,
                                 "rank " + hex(l) + " clubs " + std::to_string(count)));
        if (++taken == 4) break;
    }
}

// ---- Results ----
LEAGUE_CASES("table: results") {
    if (scenario.match) return;
    const World w(scenario);
    std::vector<std::uint32_t> picks;
    std::size_t h = 0, p = 0;
    for (const auto l : w.with_rows(6, 1)) { picks.push_back(l); if (++h == 2) break; }
    for (const auto l : w.with_rows(6, 0)) { picks.push_back(l); if (++p == 2) break; }
    // A record of a competition that is not a league (its kind byte is not 2), for the league-itself path.
    std::uint32_t other_record = 0;
    {
        const auto table = scenario.global(competitions_slot);
        for (std::uint32_t id = 0; table && id < scenario.global(competition_count_slot) && !other_record; ++id) {
            const auto object = scenario.r32(table + id * 4);
            if (object && (scenario.r32(object + 0x40) & 0xff) != 2) other_record = scenario.r32(object + 4);
        }
    }
    constexpr std::uint32_t flag_values[] = {0, 1, 2, 3, 0x20, 0x21, 0x22};
    constexpr std::pair<std::uint32_t, std::uint32_t> extras[] = {{0xff, 0xff}, {2, 1}, {1, 2}, {1, 1}};
    constexpr std::pair<std::uint32_t, std::uint32_t> scores[] = {{2, 1}, {0, 3}, {1, 1}};
    for (const auto l : picks) {
        const auto n = std::uint32_t(w.clubs(l));
        const auto rows = w.rows(l);
        const std::pair<std::uint32_t, std::uint32_t> pairs[] = {{0, 1}, {1, 0}, {n - 1, 0}, {2, 5 % n}};
        for (const auto [hi, ai] : pairs) {
            for (const auto [hg, ag] : scores)
                for (const auto flags : flag_values)
                    for (const auto [eh, ea] : extras)
                        for (const std::uint32_t pens : {0u, 1u}) {
                            FixtureSpec f{.home = scenario.r32(w.row(l, hi)), .away = scenario.r32(w.row(l, ai)), .flags = flags, .home_goals = hg,
                                          .away_goals = ag, .extra_home = eh, .extra_away = ea, .penalties_home = pens ? 5u : 4u, .penalties_away = pens ? 4u : 5u};
                            for (const std::uint32_t decider : {0u, 1u}) {
                                auto m = fixture_memory(f);
                                const auto outs = decider ? out_home : 0, outa = decider ? out_away : 0;
                                cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, decider}, {8, outs}, {9, outa}}, m.w,
                                                     "rows " + hex(l) + " " + std::to_string(hi) + "v" + std::to_string(ai) + " score " + std::to_string(hg) + "-" + std::to_string(ag) +
                                                     " flags " + hex(flags) + " extra " + hex(eh) + hex(ea) + " pens " + std::to_string(pens) + " decider " + std::to_string(decider)));
                            }
                        }
        }
        // Clubs not in the rows, a partial count, null record, a record of another kind.
        FixtureSpec f{.home = scenario.r32(w.row(l, 0)), .away = 0x12345678, .home_goals = 3, .away_goals = 1};
        cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, fixture_memory(f).w, "rows away missing " + hex(l)));
        f = {.home = 0x1111, .away = scenario.r32(w.row(l, 1)), .home_goals = 1, .away_goals = 3};
        cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, fixture_memory(f).w, "rows home missing " + hex(l)));
        f = {.home = 0x1111, .away = 0x2222};
        cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, fixture_memory(f).w, "rows both missing " + hex(l)));
        f = {.home = scenario.r32(w.row(l, 0)), .away = scenario.r32(w.row(l, 3)), .home_goals = 1, .away_goals = 0};
        for (const std::uint32_t count : {0u, 1u, 3u, 4u})
            cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, count}, {7, 0}, {8, 0}, {9, 0}}, fixture_memory(f).w, "rows count " + std::to_string(count) + " " + hex(l)));
        cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, joined(fixture_memory(f).w, Memory().l(l + 4, 0).w), "rows null record " + hex(l)));
        if (other_record)
            cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, joined(fixture_memory(f).w, Memory().l(l + 4, other_record).w), "rows other record " + hex(l)));
        // The same, with the table's first row lacking head-to-head entries.
        cases.push_back(make(apply_rows, {{3, l}, {4, fixture_at}, {5, rows}, {6, n}, {7, 1}, {8, out_home}, {9, out_away}}, joined(fixture_memory(f).w, Memory().l(rows + 8, 0).w), "rows no entries " + hex(l)));
        // count_head_to_head.
        for (const std::uint32_t row : {0xffffffffu, 0u, 1u, n - 1})
            for (const std::uint32_t entry : (scenario.r32(rows + 8) ? std::initializer_list<std::uint32_t>{0xffffffffu, 0u, 1u, n - 2} : std::initializer_list<std::uint32_t>{0xffffffffu}))
                for (const auto [sc, co] : {std::pair<std::uint32_t, std::uint32_t>{2, 1}, {1, 2}, {1, 1}, {0xff, 0}}) {
                    if (!scenario.r32(rows + 8) && std::int32_t(row) >= 0) continue;
                    cases.push_back(make(count_head, {{3, l}, {4, rows}, {5, row}, {6, entry}, {7, sc}, {8, co}}, Memory().w,
                                         "count " + hex(l) + " " + hex(row) + " " + hex(entry) + " " + hex(sc) + hex(co)));
                }
        // apply_result.
        for (const std::uint32_t stage : {0xffu, 0u, 1u, 5u, 0x7fu, 0x80u})
            for (const std::uint32_t owner : {0xffu, 0xfeu, 0u, 1u})
                for (const std::uint32_t extra : {0u, out_home})
                    cases.push_back(make(apply_result, {{3, l}, {4, fixture_at}, {5, extra}},
                                         joined(fixture_memory({.home = scenario.r32(w.row(l, 0)), .away = scenario.r32(w.row(l, 1)), .stage = stage, .home_goals = 2, .away_goals = 0}).w,
                                                Memory().b(l + 0x44, owner).w),
                                         "result " + hex(l) + " stage " + hex(stage) + " owner " + hex(owner) + " extra " + std::to_string(extra != 0)));
        cases.push_back(make(apply_result, {{3, l}, {4, 0}, {5, 0}}, Memory().w, "result null fixture " + hex(l)));
        // decides_title: the title question.
        for (const std::uint32_t ci : {0u, 1u, 5u}) {
            const auto club = scenario.r32(w.row(l, ci));
            for (const auto [hg, ag] : scores)
                for (const std::uint32_t pts : {0u, 30u, 150u})
                    for (const std::uint32_t answer : {0u, 1u}) {
                        Memory m = fixture_memory({.home = club, .away = scenario.r32(w.row(l, ci + 1)), .home_goals = hg, .away_goals = ag});
                        m.h(l + 0xa6, 38).h(l + 0xdc, 3).h(w.row(l, ci) + 0x1c, pts).b(w.row(l, ci) + 0x40, 0xff).h(l + 0xa4, 38);
                        Case item = make(decides_title, {{3, l}, {4, club}, {5, fixture_at}}, m.w,
                                         "title " + hex(l) + " club " + std::to_string(ci) + " pts " + std::to_string(pts) + " answer " + std::to_string(answer));
                        item.answers = {{0x104646d0, answer}};
                        cases.push_back(item);
                    }
            for (const std::uint32_t status : {5u, 2u, 3u, 0xfeu, 0xfcu, 0xfbu, 0u})
                cases.push_back([&] {
                    Memory m = fixture_memory({.home = club, .away = scenario.r32(w.row(l, ci + 1)), .home_goals = 1, .away_goals = 0});
                    m.h(l + 0xa6, 38).h(l + 0xdc, 3).b(w.row(l, ci) + 0x40, status);
                    Case item = make(decides_title, {{3, l}, {4, club}, {5, fixture_at}}, m.w, "title status " + hex(status) + " " + hex(l));
                    item.answers = {{0x104646d0, 1}};
                    return item;
                }());
        }
        const auto club = scenario.r32(w.row(l, 0));
        auto title = [&](std::string label, std::vector<Write> memory, std::uint32_t stage = 0xff) {
            Memory m = fixture_memory({.home = club, .away = scenario.r32(w.row(l, 1)), .stage = stage, .home_goals = 1});
            Case item = make(decides_title, {{3, l}, {4, club}, {5, fixture_at}}, joined(m.w, memory), label + " " + hex(l));
            item.answers = {{0x104646d0, 1}};
            cases.push_back(item);
        };
        title("title flags clear", Memory().h(l + 0xdc, 2).h(l + 0xa6, 38).w);
        title("title few rounds", Memory().h(l + 0xdc, 3).h(l + 0xa6, 23).w);
        title("title stage", Memory().h(l + 0xdc, 3).h(l + 0xa6, 38).w, 0);
        title("title missing club", Memory().h(l + 0xdc, 3).h(l + 0xa6, 38).w);
        {
            Case item = make(decides_title, {{3, l}, {4, 0x4444}, {5, fixture_at}}, joined(fixture_memory({.stage = 0xff}).w, Memory().h(l + 0xdc, 3).h(l + 0xa6, 38).w), "title unknown club " + hex(l));
            item.answers = {{0x104646d0, 1}};
            cases.push_back(item);
            Case copy_fails = make(decides_title, {{3, l}, {4, club}, {5, fixture_at}}, joined(fixture_memory({.home = club, .away = scenario.r32(w.row(l, 1)), .home_goals = 1}).w, Memory().h(l + 0xdc, 3).h(l + 0xa6, 38).w), "title no copy " + hex(l));
            copy_fails.answers = {{0x104646d0, 1}, {0x104a6440, 0}};
            cases.push_back(copy_fails);
        }
    }
}

// ---- Queries ----
LEAGUE_CASES("table: queries") {
    if (scenario.match) return;
    const World w(scenario);
    Rng rng{99};
    std::vector<std::uint32_t> picks;
    for (const auto l : w.with_rows(6)) { picks.push_back(l); if (picks.size() == 6) break; }
    for (const auto l : picks) {
        const auto n = std::uint32_t(w.clubs(l));
        const auto rows = w.rows(l);
        const auto club_of = [&](std::uint32_t i) { return scenario.r32(w.row(l, i)); };
        // find_standing / has_standing.
        for (const std::uint32_t i : {0u, 1u, n - 1})
            for (const std::uint32_t explicit_rows : {0u, 1u}) {
                cases.push_back(make(find_standing, {{3, l}, {4, club_of(i)}, {5, explicit_rows ? rows : 0}}, Memory().w, "find " + hex(l) + " " + std::to_string(i)));
                cases.push_back(make(has_standing, {{3, l}, {4, club_of(i)}}, Memory().w, "has " + hex(l) + " " + std::to_string(i)));
            }
        cases.push_back(make(find_standing, {{3, l}, {4, 0x7777}, {5, 0}}, Memory().w, "find missing " + hex(l)));
        cases.push_back(make(has_standing, {{3, l}, {4, 0x7777}}, Memory().w, "has missing " + hex(l)));
        cases.push_back(make(find_standing, {{3, l}, {4, club_of(0)}, {5, 0}}, Memory().h(l + 0x3e, 0).w, "find no clubs " + hex(l)));
        cases.push_back(make(has_standing, {{3, l}, {4, club_of(0)}}, Memory().h(l + 0x3e, 0).w, "has no clubs " + hex(l)));
        // mark_clubs.
        cases.push_back(make(mark_clubs, {{3, l}}, Memory().w, "mark real " + hex(l)));
        {
            Memory m;
            for (std::uint32_t i = 0; i < n; i += 2) m.l(club_of(i) + 0x30, scenario.r32(l + 4));
            cases.push_back(make(mark_clubs, {{3, l}}, m.w, "mark matching " + hex(l)));
        }
        // limit_club_byte.
        for (const std::uint32_t value : {0u, 3u, 0xfbu, 120u})
            for (const std::uint32_t excluded : {0xfdu, 0xffu, 6u, 0u})
                for (const std::uint32_t included : {0xfdu, 0xffu, 5u})
                    for (const std::uint32_t flag : {0u, 1u, 0x100u}) {
                        Memory m;
                        for (std::uint32_t i = 0; i < n; ++i) m.b(club_of(i) + 0x23d, i % 3 == 0 ? 0u : i % 3 == 1 ? 10u : 0xf0u).b(w.row(l, i) + 0x40, i % 4 == 0 ? 0xffu : i % 4 == 1 ? 5u : i % 4 == 2 ? 6u : 0u);
                        cases.push_back(make(limit_byte, {{3, l}, {4, value}, {5, excluded}, {6, included}, {7, flag}}, m.w,
                                             "limit " + hex(l) + " " + hex(value) + " " + hex(excluded) + " " + hex(included) + " " + hex(flag)));
                    }
        cases.push_back(make(limit_byte, {{3, l}, {4, 5}, {5, 1}, {6, 1}, {7, 0}}, Memory().h(l + 0x3e, 0).w, "limit no clubs " + hex(l)));
        // points_for_result.
        for (const std::uint32_t who : {0u, 1u, 2u})
            for (const auto [hg, ag] : {std::pair<std::uint32_t, std::uint32_t>{1, 0}, {0, 1}, {1, 1}, {0, 0}})
                for (const std::uint32_t flags : {0u, 1u, 2u, 3u, 0x20u, 0x21u, 0x22u, 0x23u})
                    for (const auto [eh, ea] : {std::pair<std::uint32_t, std::uint32_t>{0xff, 0xff}, {1, 0}, {0, 1}, {2, 2}})
                        for (const auto [ph, pa] : {std::pair<std::uint32_t, std::uint32_t>{1, 0}, {0, 1}, {1, 1}}) {
                            const auto home = club_of(0), away = club_of(1);
                            Memory m = fixture_memory({.home = home, .away = away, .home_goals = hg, .away_goals = ag, .extra_home = eh, .extra_away = ea, .penalties_home = ph, .penalties_away = pa});
                            m.h(l + 0xa8, flags);
                            const auto club = who == 0 ? home : who == 1 ? away : 0x5555u;
                            cases.push_back(make(points_result, {{3, l}, {4, club}, {5, fixture_at}}, m.w,
                                                 "points " + hex(l) + " who " + std::to_string(who) + " " + std::to_string(hg) + std::to_string(ag) + " flags " + hex(flags) +
                                                 " " + hex(eh) + hex(ea) + " " + std::to_string(ph) + std::to_string(pa)));
                        }
        {
            const auto both = club_of(0);
            cases.push_back(make(points_result, {{3, l}, {4, both}, {5, fixture_at}}, fixture_memory({.home = both, .away = both, .home_goals = 0, .away_goals = 2}).w, "points same club " + hex(l)));
            cases.push_back(make(points_result, {{3, l}, {4, both}, {5, fixture_at}}, fixture_memory({.home = both, .away = both, .home_goals = 2, .away_goals = 0}).w, "points same club 2 " + hex(l)));
        }
        // fixture_slot and evaluate_position: random scenarios on the real rows.
        const std::uint32_t places[][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {2, 1, 0, 0}, {0, 0, 2, 0}, {0, 0, 0, 3}, {1, 1, 2, 2}, {0, 0, 1, 1}};
        for (unsigned round = 0; round < 90; ++round) {
            Memory m;
            const auto& pl = places[rng.below(8)];
            m.b(l + 0xc0, pl[0]).b(l + 0xc1, pl[1]).b(l + 0xc2, pl[2]).b(l + 0xc3, pl[3]);
            m.h(l + 0xdc, rng.below(2) ? 3 : 2).h(l + 0xa6, 10 + rng.below(30)).b(l + 0xc4, rng.below(5) == 0 ? 0 : 3).b(l + 0xc7, 1 + rng.below(3));
            for (std::uint32_t i = 0; i < n; ++i) {
                const auto row = w.row(l, i);
                m.h(row + 0x1c, 10 + rng.below(8) * 2).h(row + 0x18, 10 + rng.below(5)).h(row + 0x1a, 10 + rng.below(5)).b(row + 4, rng.below(n)).b(row + 0x14, 8 + rng.below(30))
                    .b(row + 0x40, rng.below(4) == 0 ? 5u : rng.below(5) == 0 ? 0u : 0xffu);
            }
            const auto i = rng.below(n);
            const auto row = w.row(l, i);
            cases.push_back(make(evaluate, {{3, l}, {4, row}, {5, marker_at}}, m.w, "evaluate " + hex(l) + " row " + std::to_string(i) + " round " + std::to_string(round)));
            Memory slot = m;
            const auto hi = rng.below(n), ai = rng.below(n);
            slot.h(fixture_at + 0x34, rng.below(6)).b(l + 0x4b, rng.below(4) == 0 ? 0xf0u : 10 + rng.below(20));
            for (const auto& x : fixture_memory({.home = club_of(hi), .away = rng.below(8) == 0 ? 0x999u : club_of(ai), .half_34 = rng.below(6)}).w) slot.w.push_back(x);
            cases.push_back(make(fixture_slot, {{3, l}, {4, fixture_at}, {5, rng.below(3)}, {6, marker_at}}, slot.w, "slot " + hex(l) + " round " + std::to_string(round)));
        }
    }
}
}

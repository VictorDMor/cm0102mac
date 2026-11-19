// CUP.CPP, block `draws`: cases for 0x1047a9e0 (a draw below a bound), 0x1047fd00, 0x10480620 and 0x104810b0 (the
// round's club orders). Real cup objects of the career where one fits, scratch objects (a copy of a real cup with
// its rounds, entrants and match rows built in the scratch area) where a branch needs rows the career lacks. The
// sample stream's cells are set per case through Case::memory (a table of halfwords in the scratch area, so each
// sample is chosen), which the checker undoes. Labels start with "draws:" (CM_CUP_BLOCK=draws).
#include "cup_base_checks.hpp"
#include <cstdio>
#include <random>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t draw_below = 0x1047a9e0, sort_round_teams = 0x1047fd00, check_nation = 0x10480620,
                        randomize = 0x104810b0;
constexpr std::int32_t cursor_slot = -0x7b84, limit_slot = -0x7b88, salt_slot = -0x7b90, start_slot = -0x7b8c;
constexpr std::uint32_t draws_area = scratch_base + 0x20000;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

// The stream: the cursor at `at` (the next step reads the halfword two bytes on), the limit, the salt and the
// halfwords the first steps read (table + 2, + 4, ...).
std::vector<Write> stream(const Scenario& s, std::uint32_t at, std::uint32_t limit, std::uint32_t salt,
                          std::initializer_list<std::uint32_t> samples) {
    std::vector<Write> writes{{4, s.toc(cursor_slot), at}, {4, s.toc(limit_slot), limit}, {4, s.toc(salt_slot), salt}};
    std::uint32_t offset = 2;
    for (const auto sample : samples) writes.push_back({2, draws_area + offset, sample}), offset += 2;
    return writes;
}

CUP_CASES("draws") {
    if (scenario.match) return;
    const auto table = draws_area;
    const auto label = [](const std::string& text) { return "draws: below " + text; };
    // Bounds: zero, the small arm's edges and both signs, the first beyond it, and large ones whose scaled part and
    // finer bound cover the sign of the wrapped product, a finer bound of zero (the product wraps to a small
    // value) and the extremes.
    const std::int32_t bounds[] = {0, 1, -1, 2, 7, 100, -3, 32767, -32768, 65534, 65535, -65535, 65536, -65536, 65537, -65537,
                                   100000, -100000, 655350, 1000000, -1000000, 6553500, 300000000, -300000000, 0x19999999,
                                   -0x19999999, 429496730, -429496730, 429496731, 0x40000000, 0x7fffffff, std::int32_t(0x80000000u),
                                   std::int32_t(0x80000001u), 214748364, 214748365, -214748365, 65535 * 3, 65535 * 32768 / 10};
    struct Config { std::uint32_t salt; std::uint32_t first, second; };
    const Config configs[] = {{0, 0, 0}, {0, 1, 1}, {1, 0xffff, 0xffff}, {29999, 12345, 7}, {0xffff, 0xffff, 0}, {5, 0x8000, 0x7fff},
                              {0, 0xfffe, 3}, {0x1234, 0x4321, 0xffff}};
    for (const auto bound : bounds)
        for (const auto& config : configs)
            cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}},
                             .memory = stream(scenario, table, table + 0x100, config.salt, {config.first, config.second}),
                             .label = label(std::to_string(bound) + " salt " + hex(config.salt) + " samples " + hex(config.first) + " " + hex(config.second))});
    // The refills: at the first step (the limit at the cursor), at the second only (the limit one step on), with
    // the real start's first halfword chosen.
    const auto start = scenario.toc(start_slot);
    for (const auto bound : {0, 5, -5, 65536, -70000, 1000000, 300000000, 429496730, 0x7fffffff, std::int32_t(0x80000000u)}) {
        for (const auto first : {0u, 3u, 0xffffu})
            cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}},
                             .memory = joined(stream(scenario, table, table, 9, {1, 2}), {{2, start, first}}),
                             .label = label(std::to_string(bound) + " refill at the first step, start halfword " + hex(first))});
        for (const auto second : {0u, 2u, 0xffffu})
            cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}},
                             .memory = joined(stream(scenario, table, table + 2, 9, {1, 2}), {{2, start, second}}),
                             .label = label(std::to_string(bound) + " refill at the second step, start halfword " + hex(second))});
    }
    // The cursor at the real stream: no cell changed.
    for (const auto bound : {1000, 70000, 429496730, -429496730})
        cases.push_back({.entry = draw_below, .registers = {{3, std::uint32_t(bound)}}, .label = label(std::to_string(bound) + " on the live stream")});
}

// ---- The round-order routines: a scratch cup built from a real cup's header.
// The layout (one case at a time; Case::memory is undone after it): the cup copy, the rounds (0x78 bytes), the
// entrants (8 bytes: club, state byte at +4), the clubs (+0x2c the nation word, +0x68 the nation's index), the
// per-round pointers to the tie rows (0x36 bytes) and the rows.
constexpr std::uint32_t kLayout = scratch_base + 0x30000, kCup = kLayout, kRounds = kLayout + 0x100, kEntrants = kLayout + 0x400,
                        kClubs = kLayout + 0x800, kMatches = kLayout + 0x2800, kRows = kLayout + 0x2900, kRowStride = 0x900,
                        kRoundCount = 6, kNations = kLayout + 0x1000;
constexpr std::uint32_t kNone = 0xffff, kDraw = 0xfffe;  // the slot values -1 and -2
struct Tie { std::uint32_t home = kNone, away = kNone, home_first = kNone, home_second = 0, away_first = kNone, away_second = 0; };
struct RoundSpec { std::uint32_t slots = 0, ties = 0, flags = 0; std::vector<Tie> rows; };
struct Entrant { std::uint32_t state = 0, nation = 1; };  // nation 0: none, 1-3: records whose +0x68 is nation - 1
struct Spec {
    std::vector<RoundSpec> rounds;
    std::vector<Entrant> entrants;
    std::vector<Write> extra;
    std::uint32_t bias = 0;  // the tables start `bias` rows before index 0: the routine's round may be negative
};
std::vector<Write> build(const Scenario& scenario, std::uint32_t cup_source, const Spec& spec) {
    std::vector<Write> w;
    for (std::uint32_t offset = 0; offset < 0xb4; offset += 4) w.push_back({4, kCup + offset, scenario.r32(cup_source + offset)});
    w.push_back({4, kCup + 0xa0, kRounds + spec.bias * 0x78});
    w.push_back({4, kCup + 0xa4, kEntrants});
    w.push_back({4, kCup + 0xac, kMatches + spec.bias * 4});
    for (std::uint32_t r = 0; r < spec.rounds.size(); ++r) {
        const auto& round = spec.rounds[r];
        const auto base = kRounds + r * 0x78;
        w.push_back({2, base + 0x1a, round.slots});
        w.push_back({2, base + 0x1c, round.ties});
        w.push_back({2, base + 0xc, round.flags});
        w.push_back({4, kMatches + r * 4, kRows + r * kRowStride});
        for (std::uint32_t t = 0; t < round.rows.size(); ++t) {
            const auto row = kRows + r * kRowStride + t * 0x36;
            const auto& tie = round.rows[t];
            for (std::uint32_t offset = 0; offset < 0x36; offset += 2) w.push_back({2, row + offset, 0x7777});
            w.push_back({2, row + 0, tie.home});
            w.push_back({2, row + 2, tie.away});
            w.push_back({2, row + 0xe, tie.home_first});
            w.push_back({2, row + 0xa, tie.home_second});
            w.push_back({2, row + 0x10, tie.away_first});
            w.push_back({2, row + 0xc, tie.away_second});
        }
    }
    for (std::uint32_t e = 0; e < spec.entrants.size(); ++e) {
        w.push_back({4, kEntrants + e * 8, kClubs + e * 0x80});
        w.push_back({1, kEntrants + e * 8 + 4, spec.entrants[e].state});
        w.push_back({4, kClubs + e * 0x80 + 0x2c, spec.entrants[e].nation ? kNations + spec.entrants[e].nation * 0x80 : 0});
    }
    for (std::uint32_t n = 1; n < 4; ++n) w.push_back({4, kNations + n * 0x80 + 0x68, n - 1});
    return joined(w, spec.extra);
}
std::uint32_t signed_value(std::int32_t value) { return std::uint32_t(value) & 0xffff; }

// A pseudo-random round: `ties` ties over `teams` entrants; the slots hold teams, -1 or -2; the forms refer to
// ties of rounds 0..2 (the rows exist there).
Spec random_spec(std::mt19937& rng, unsigned ties, unsigned teams, bool nation_flag, unsigned round_index, bool allow_draw = true) {
    Spec spec;
    spec.rounds.resize(kRoundCount);
    const auto pick = [&](unsigned bound) { return unsigned(rng() % bound); };
    for (unsigned r = 0; r < kRoundCount; ++r) {
        auto& round = spec.rounds[r];
        const unsigned n = r == round_index ? ties : 4;
        round.ties = n;
        round.slots = 2 * n + (r == round_index ? pick(2) : 0);
        round.flags = r == round_index && nation_flag ? 4 : (pick(3) == 0 ? 0xfffb : 0);
        round.rows.resize(n);
        for (auto& tie : round.rows) {
            const auto slot = [&] {
                switch (pick(6)) {
                case 0: return kNone;
                case 1: if (allow_draw) return kDraw; [[fallthrough]];
                default: return signed_value(int(pick(teams)));
                }
            };
            tie.home = slot();
            tie.away = slot();
            const auto reference = [&](std::uint32_t& first, std::uint32_t& second) {
                if (pick(3) == 0) { first = kNone; second = pick(4); }
                else { first = pick(3); second = pick(4); }
            };
            reference(tie.home_first, tie.home_second);
            reference(tie.away_first, tie.away_second);
        }
    }
    for (unsigned e = 0; e < teams; ++e) {
        const unsigned s = pick(8);
        spec.entrants.push_back({s == 0 ? 0xffu : s == 1 ? 0x80u : s * 3 % 5, pick(4)});
    }
    return spec;
}
std::vector<Write> stream_values(const Scenario& scenario, std::mt19937& rng, std::uint32_t salt) {
    std::vector<Write> writes{{4, scenario.toc(cursor_slot), draws_area}, {4, scenario.toc(limit_slot), draws_area + 0x400},
                              {4, scenario.toc(salt_slot), salt}};
    for (std::uint32_t offset = 2; offset < 0x400; offset += 2) writes.push_back({2, draws_area + offset, std::uint32_t(rng() & 0xffff)});
    return writes;
}
std::uint32_t find_cup(const Scenario& scenario) {
    const auto table = scenario.global(-0x7b5c);
    const auto count = scenario.global(-0x7b60);
    for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
        const auto object = scenario.r32(table + id * 4);
        if (object && scenario.r32(object) >= 0x108b0000 && scenario.r32(object) < 0x10a00000 && (scenario.r32(object + 0x40) & 0xff) == 1)
            return object;
    }
    throw std::runtime_error("draws: the career has no cup object");
}

CUP_CASES("draws") {
    if (scenario.match) return;
    const auto source = find_cup(scenario);
    const auto order_entries = {sort_round_teams, check_nation, randomize};
    for (const auto entry : order_entries) {
        const auto label = [&](const std::string& text) { return "draws: " + hex(entry) + " " + text; };
        std::mt19937 rng(0x5eed + entry);
        // No ties, no slots, negative ties.
        for (const std::uint32_t ties : {0u, 0xffffu, 0x8000u}) {
            Spec spec = random_spec(rng, 3, 4, false, 1);
            spec.rounds[1].ties = ties;
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = joined(build(scenario, source, spec), stream_values(scenario, rng, 7)),
                             .label = label("ties " + hex(ties))});
        }
        const bool clean = entry == check_nation;  // the nation count reads each slot's club: valid teams only
        // Round -2 (the register 0xfffe): the tables start two rows before index 0, the back references' round word is
        // 0xfffe, and the pointer of a round the forms refer to lands inside this round's rows at an offset that makes
        // the store of that word a slot (a tie becomes -2 in the middle of the placement).
        for (unsigned n = 0; n < 240; ++n) {
            const auto ties = 2 + n % 6, teams = 2 + n % 5;
            auto spec = random_spec(rng, ties, teams, n % 2 == 0, 0, !clean);
            spec.bias = 2;
            const std::uint32_t offsets[] = {0x24, 0x26, 0x24, 0x30};
            for (auto& row : spec.rounds[0].rows) {
                row.home_first = rng() % 4 ? 1 : kNone; row.away_first = rng() % 4 ? 1 : kNone;
                row.home_second = rng() % ties; row.away_second = rng() % ties;
            }
            spec.extra.push_back({4, kMatches + 3 * 4, kRows + 0 * kRowStride + offsets[n % 4]});
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 0xfffe}}, .memory = joined(build(scenario, source, spec), stream_values(scenario, rng, rng() & 0xffff)),
                             .label = label("round -2, clobbering back references " + std::to_string(n))});
        }
        if (entry == check_nation) {
            // A slot whose club is null: the count skips it but the second pass reads club +0x2c without a test (a fault
            // on both sides alike).
            auto spec = random_spec(rng, 3, 4, false, 1, false);
            auto memory = joined(build(scenario, source, spec), stream_values(scenario, rng, 3));
            memory.push_back({4, kEntrants + 0, 0});
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = memory, .label = label("a slot's club is null")});
        }
        // The round register's high half is ignored (extsh) but stored whole into the tie rows' back references.
        for (const std::uint32_t high : {0x10000u, 0x7fff0000u, 0xffff0000u}) {
            auto spec = random_spec(rng, 3, 4, true, 1, !clean);
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, high | 1}}, .memory = joined(build(scenario, source, spec), stream_values(scenario, rng, 3)),
                             .label = label("round register " + hex(high | 1))});
        }
        // Allocation failures: the working array (and, for 0x10480620, the nation counts first), answered on both
        // sides; the allocator's entries run in place in the generated original, so they run their original bodies.
        {
            auto spec = random_spec(rng, 3, 4, true, 1, !clean);
            const auto memory = joined(build(scenario, source, spec), stream_values(scenario, rng, 3));
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = memory, .answers = {{0x10775130, 0}},
                             .label = label("new[] fails"), .original_owners = {0x10775130}});
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = memory, .nth_answers = {{0x10775130, 1, 0}},
                             .label = label("first new[] fails"), .original_owners = {0x10775130}});
            if (entry == check_nation) {
                cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = memory, .answers = {{0x10775200, 0}},
                                 .label = label("clearing new[] fails"), .original_owners = {0x10775200}});
                cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, 1}}, .memory = memory, .nth_answers = {{0x10775200, 1, 0}},
                                 .label = label("first clearing new[] fails"), .original_owners = {0x10775200}});
            }
        }
        for (unsigned n = 0; n < 480; ++n) {
            const auto ties = 1 + n % 8, teams = 2 + n % 5;
            const auto round_index = n % 3;
            auto spec = random_spec(rng, ties, teams, n % 2 == 0, round_index, !clean);
            // Every third case: the back references of the forms (round 3's pointer) land inside this round's own rows
            // six bytes before the next tie's slot, clobbering slots (the placement then can run out of free slots).
            if (n % 3 == 2) {
                for (auto& row : spec.rounds[round_index].rows) { row.home_first = 3; row.away_first = 3; row.home_second = rng() % ties; row.away_second = rng() % ties; }
                spec.extra.push_back({4, kMatches + 3 * 4, kRows + round_index * kRowStride + 0x30});
            }
            cases.push_back({.entry = entry, .registers = {{3, kCup}, {4, round_index}}, .memory = joined(build(scenario, source, spec), stream_values(scenario, rng, rng() & 0xffff)),
                             .label = label("random " + std::to_string(n))});
        }
    }
}
}
}

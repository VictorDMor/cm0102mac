// CUP.CPP, block `rounds`: cases for 0x10485d70, 0x10481ad0 (the tie orders), 0x104860a0, 0x10481e90 (news), 0x1047e980
// (a round's draw), 0x1047f3a0 (a tie's fixtures) and 0x1047ef50 (the next round). The career's own cups are used where
// they fit; the branches need ties, sides and clubs the career lacks, so scratch cups are built in the scratch area:
// a copy of a real cup's first 0xb4 bytes (its vtable, record and the fields the routines read replaced), a rounds table,
// the entrant rows, the per-round tie rows and clubs (real clubs wherever the routine files news under them, which
// writes the news manager's records, so a case compares those too). Cases never touch shared scenario state: every
// value a case needs is in its memory/answers, which the checker undoes. Labels start with "rounds:" (CM_CUP_BLOCK=rounds).
#include "cup_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t make_draw = 0x1047e980, next_round = 0x1047ef50, schedule = 0x1047f3a0, order_sides = 0x10481ad0,
                        draw_news = 0x10481e90, separate = 0x10485d70, missing_news = 0x104860a0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
[[maybe_unused]] constexpr std::uint32_t area = scratch_base + 0x70000;
constexpr std::uint32_t cup_at = area, rounds_at = area + 0x100, entrants_at = area + 0x900, pointers_at = area + 0xa80,
                        rows_at = area + 0xb00, row_stride = 0x400, clubs_at = area + 0x5000, groups_at = area + 0x8000,
                        extra_at [[maybe_unused]] = area + 0x9000;
constexpr std::uint32_t object_size = 0xb4, max_rounds = 16, max_entrants = 40;
[[maybe_unused]] std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> cups;  // every cup object the career has: competitions and their stages
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto list = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) { if (object && is_cup(object) && seen.insert(object).second) cups.push_back(object); };
        for (std::uint32_t id = 0; list && id < count && id < 4096; ++id) {
            const auto object = s.r32(list + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    bool is_cup(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 1) return false;
        for (std::uint32_t slot = 0x08; slot <= 0xa4; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    // The code address of a cup's virtual slot.
    std::uint32_t slot_code(std::uint32_t cup, std::uint32_t slot) const { return s.r32(s.r32(s.r32(cup) + slot)); }
};

constexpr std::uint32_t none = 0xffff;  // the halfword -1
struct Tie {
    std::uint32_t first = none, second = none;
    std::uint32_t first_round = none, first_match = 0, second_round = none, second_match = 0;  // the feeder links
    std::uint32_t state = 0, flag = 0x7777;
};
struct RoundSpec {
    std::uint32_t label = 0x11, flags = 0, rule = 0, leg_rule = 0, rule_byte_22 = 0, byte_24 = 3, rules_id = 0x1234;
    std::uint32_t draw_day = 100, draw_year = 0, fixture_day = 120, fixture_year = 0, entrants = 0, fixture_kind = 1, day_2c = 3;
    std::vector<Tie> ties;
};
struct Entrant { std::uint32_t club = 0; std::uint32_t state = 0, byte_5 = 0; };
struct Spec {
    std::vector<RoundSpec> rounds, pre;  // `pre`: the rows of rounds -pre.size()..-1 (a routine given a negative round)
    std::vector<Entrant> entrants;
    std::uint32_t flag45 = 1, rule_word = 0xffffffffu, current_round = 0, ninth = 0, owner = 0xff, year = 2000, code = 5;
    std::uint32_t round_count = 0;  // 0: rounds.size()
    std::vector<Write> extra;
};

// The scratch cup. Bytes the routines do not set hold a pattern, so a stray store shows.
std::vector<Write> make_cup(const Scenario& s, std::uint32_t base, const Spec& spec) {
    std::vector<Write> w;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) w.push_back({4, cup_at + offset, s.r32(base + offset)});
    const auto count = spec.round_count ? spec.round_count : std::uint32_t(spec.rounds.size());
    w.push_back({4, cup_at + 0x24, spec.rule_word});
    w.push_back({2, cup_at + 0x3a, spec.code});
    w.push_back({2, cup_at + 0x3c, count});
    w.push_back({2, cup_at + 0x3e, std::uint32_t(spec.entrants.size())});
    w.push_back({2, cup_at + 0x40, spec.year});
    w.push_back({1, cup_at + 0x44, spec.owner});
    w.push_back({1, cup_at + 0x45, spec.flag45});
    const auto bias = std::uint32_t(spec.pre.size());
    w.push_back({4, cup_at + 0xa0, rounds_at + bias * 0x78});
    w.push_back({4, cup_at + 0xa4, entrants_at});
    w.push_back({1, cup_at + 0xa8, spec.current_round});
    w.push_back({1, cup_at + 0xa9, spec.ninth});
    w.push_back({4, cup_at + 0xac, pointers_at + bias * 4});
    std::vector<RoundSpec> all = spec.pre;
    all.insert(all.end(), spec.rounds.begin(), spec.rounds.end());
    for (std::uint32_t r = 0; r < all.size() && r < max_rounds; ++r) {
        const auto at = rounds_at + r * 0x78;
        const auto& round = all[r];
        for (std::uint32_t offset = 0; offset < 0x78; offset += 2) w.push_back({2, at + offset, 0x7777});
        w.push_back({2, at + 0x00, round.draw_day});
        w.push_back({1, at + 0x02, round.draw_year});
        w.push_back({2, at + 0x04, round.fixture_day});
        w.push_back({1, at + 0x06, round.fixture_year});
        w.push_back({1, at + 0x07, round.fixture_kind});
        w.push_back({2, at + 0x08, round.label});
        w.push_back({2, at + 0x0c, round.rule});
        w.push_back({2, at + 0x0e, round.flags});
        w.push_back({4, at + 0x14, round.rules_id});
        w.push_back({2, at + 0x1c, std::uint32_t(round.ties.size())});
        w.push_back({2, at + 0x1e, round.entrants});
        w.push_back({1, at + 0x22, round.rule_byte_22});
        w.push_back({1, at + 0x23, round.leg_rule});
        w.push_back({1, at + 0x24, round.byte_24});
        w.push_back({4, pointers_at + r * 4, rows_at + r * row_stride});
        for (std::uint32_t t = 0; t < round.ties.size() && t < 16; ++t) {
            const auto row = rows_at + r * row_stride + t * 0x36;
            const auto& tie = round.ties[t];
            for (std::uint32_t offset = 0; offset < 0x36; offset += 2) w.push_back({2, row + offset, 0x7777});
            w.push_back({2, row + 0x00, tie.first});
            w.push_back({2, row + 0x02, tie.second});
            w.push_back({2, row + 0x08, tie.flag});
            w.push_back({2, row + 0x0a, tie.first_match});
            w.push_back({2, row + 0x0c, tie.second_match});
            w.push_back({2, row + 0x0e, tie.first_round});
            w.push_back({2, row + 0x10, tie.second_round});
            w.push_back({1, row + 0x34, tie.state});
        }
    }
    for (std::uint32_t e = 0; e < spec.entrants.size() && e < max_entrants; ++e) {
        w.push_back({4, entrants_at + e * 8, spec.entrants[e].club});
        w.push_back({1, entrants_at + e * 8 + 4, spec.entrants[e].state});
        w.push_back({1, entrants_at + e * 8 + 5, spec.entrants[e].byte_5});
    }
    return joined(w, spec.extra);
}
// A scratch club: the pointer to its region record at +0x3c (group g: id word and the halfword +0x18 the
// reputation, 0 = no pointer), the halfword +0x30 record pointer used by the virtual +0x98.
std::uint32_t scratch_club(std::vector<Write>& w, std::uint32_t index, int group, std::uint32_t reputation_group = 0) {
    const auto club = clubs_at + index * 0x100;
    w.push_back({4, club + 0x3c, group < 0 ? 0u : groups_at + std::uint32_t(group) * 0x40});
    w.push_back({4, club + 0x30, reputation_group ? groups_at + 0x800 + reputation_group * 0x40 : 0});
    return club;
}
void group_records(std::vector<Write>& w) {
    for (std::uint32_t g = 0; g < 16; ++g) {
        w.push_back({4, groups_at + g * 0x40, 100 + g});
        w.push_back({2, groups_at + 0x800 + g * 0x40 + 0x18, g * 3 % 7});
    }
}

CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.size() < 3) throw std::runtime_error("rounds: the career has fewer than three cup objects");
    std::fprintf(stderr, "rounds: %zu cup objects\n", w.cups.size());
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    const auto base = w.cups.front();
    std::mt19937 rng(0x5eed);
    const auto pick = [&](unsigned bound) { return unsigned(rng() % bound); };

    // 0x10485d70: same-group clubs first in two ties of a round.
    const auto separate_spec = [&](unsigned ties, unsigned groups, unsigned rounds_count, unsigned round_index) {
        Spec spec;
        spec.rounds.resize(rounds_count);
        std::vector<Write> extra;
        group_records(extra);
        const auto entrant_count = ties * 2 + 2;
        for (unsigned e = 0; e < entrant_count; ++e) {
            const auto group = pick(groups + 1) == groups ? -1 : int(pick(groups));
            spec.entrants.push_back({scratch_club(extra, e, group), 0, 0});
        }
        std::vector<unsigned> counts;
        for (unsigned r = 0; r < rounds_count; ++r) counts.push_back(r == round_index ? ties : 1 + pick(3));
        for (unsigned r = 0; r < rounds_count; ++r) {
            const unsigned n = counts[r];
            for (unsigned t = 0; t < n; ++t) {
                Tie tie;
                const auto side = [&]() { return pick(8) == 0 ? none : pick(entrant_count); };
                tie.first = side();
                tie.second = side();
                const auto feeder = [&](std::uint32_t& round, std::uint32_t& match) {
                    if (pick(3) == 0) { round = none; match = 0; return; }
                    round = pick(rounds_count);
                    match = pick(std::max(1u, counts[round]));
                };
                feeder(tie.first_round, tie.first_match);
                feeder(tie.second_round, tie.second_match);
                spec.rounds[r].ties.push_back(tie);
            }
        }
        spec.extra = extra;
        return spec;
    };
    for (unsigned n = 0; n < 160; ++n) {
        const unsigned ties = n % 9, groups = 1 + n % 4, rounds_count = 2 + n % 3, round_index = n % rounds_count;
        auto spec = separate_spec(ties, groups, rounds_count, round_index);
        cases.push_back({.entry = separate, .registers = {{3, cup_at}, {4, round_index}}, .memory = make_cup(scenario, base, spec),
                         .label = label("separate random " + std::to_string(n))});
    }

    // 0x10481ad0: the sides of the ties by the round's rule flags. Flag 1 asks the cup's virtual +0x98 (the base
    // body compares the clubs' +0x30 records' halfword +0x18 and answers the club with the lower one), flag 8 the
    // entrants' state bytes, otherwise the clubs' region against the cup's +0x24.
    std::uint32_t base_98 = 0;
    for (const auto cup : w.cups) if (w.slot_code(cup, 0x98) == 0x10485ab0) { base_98 = cup; break; }
    if (!base_98) std::fprintf(stderr, "rounds: no cup uses the base virtual +0x98\n");
    const auto order_spec = [&](unsigned ties, std::uint32_t rule, unsigned round_index, bool nation_rule) {
        Spec spec;
        spec.rounds.resize(round_index + 2);
        std::vector<Write> extra;
        group_records(extra);
        const auto entrant_count = ties * 2 + 2;
        for (unsigned e = 0; e < entrant_count; ++e) {
            const auto group = pick(5) == 0 ? -1 : int(pick(4));
            const auto reputation = pick(4);  // 0: no record pointer
            Entrant entrant{scratch_club(extra, e, group, reputation), pick(5) - 2u, 0};
            if (pick(12) == 0 && !(rule & 1)) entrant.club = 0;
            spec.entrants.push_back(entrant);
        }
        spec.rule_word = nation_rule ? 100 + pick(3) : 0xffffffffu;
        spec.rounds[round_index].rule = rule;
        for (unsigned t = 0; t < ties; ++t) {
            Tie tie;
            tie.first = pick(7) == 0 ? none : pick(entrant_count);
            tie.second = pick(7) == 0 ? none : pick(entrant_count);
            tie.first_round = pick(3) == 0 ? none : 0;
            tie.first_match = pick(4);
            tie.second_round = pick(3) == 0 ? none : 0;
            tie.second_match = pick(4);
            spec.rounds[round_index].ties.push_back(tie);
        }
        spec.extra = extra;
        return spec;
    };
    for (unsigned n = 0; n < 240; ++n) {
        const unsigned ties = n % 7, round_index = n % 2;
        const std::uint32_t rule = std::vector<std::uint32_t>{1, 8, 0, 4, 9, 0xfff0, 1}[n % 7];
        auto spec = order_spec(ties, rule, round_index, n % 3 != 0);
        if (rule & 1 && !base_98) continue;
        const auto source = rule & 1 ? base_98 : base;
        const std::uint32_t first = std::vector<std::uint32_t>{0xffffffffu, 0xffffffffu, 0, 1, 2, 0x1ffff, 0xffff0000u, 5}[n % 8];
        cases.push_back({.entry = order_sides, .registers = {{3, cup_at}, {4, round_index}, {5, first}},
                         .memory = make_cup(scenario, source, spec), .label = label("order random " + std::to_string(n))});
    }
}

// The scratch cup's entrants from the career's real clubs (news is filed under them) with the ties of one round.
CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    const auto base = w.cups.front();
    std::mt19937 rng(0xc0ffee);
    const auto pick = [&](unsigned bound) { return unsigned(rng() % bound); };
    const auto club_count = scenario.global(-0x7b50);
    // 0x104860a0: a tie with an undecided side files item 0x26, naming the clubs of the ties that feed it.
    for (unsigned n = 0; n < 240; ++n) {
        Spec spec;
        const unsigned rounds_count = 3, round_index = 1 + n % 2, entrant_count = 10;
        spec.rounds.resize(rounds_count);
        for (unsigned e = 0; e < entrant_count; ++e) spec.entrants.push_back({n % 3 == 0 && pick(4) == 0 ? 0u : scenario.club((e * 37 + n) % club_count), pick(5) - 2u, 0});
        std::vector<unsigned> counts{1 + pick(3), 2 + pick(3), 2 + pick(3)};
        for (unsigned r = 0; r < rounds_count; ++r)
            for (unsigned t = 0; t < counts[r]; ++t) {
                Tie tie;
                tie.first = pick(3) == 0 ? none : pick(entrant_count);
                tie.second = pick(3) == 0 ? none : pick(entrant_count);
                const auto feeder = [&](std::uint32_t& round, std::uint32_t& match) {
                    if (pick(5) == 0) { round = none; match = 0; return; }
                    round = pick(rounds_count);
                    match = pick(counts[round]);
                };
                feeder(tie.first_round, tie.first_match);
                feeder(tie.second_round, tie.second_match);
                spec.rounds[r].ties.push_back(tie);
            }
        spec.code = 3 + pick(50);
        const unsigned tie_index = pick(counts[round_index]);
        const std::uint32_t round_arg = n % 5 == 0 ? 0x70000u + round_index : round_index;
        const std::uint32_t tie_arg = n % 7 == 0 ? 0xabcd0000u + tie_index : tie_index;
        auto memory = make_cup(scenario, base, spec);
        if (n % 10 == 0) {
            // The buffers' default words zero (a state the game never leaves them in as far as is known): a side without a
            // club then keeps 0, which faults at the news fields unless the null page is mapped (--null-page).
            for (std::uint32_t k = 0; k < 4; ++k) memory.push_back({4, scenario.toc(-0x47cc) + 4 * k, 0});
            for (std::uint32_t k = 0; k < 2; ++k) memory.push_back({4, scenario.toc(-0x47c8) + 4 * k, 0});
        }
        cases.push_back({.entry = missing_news, .registers = {{3, cup_at}, {4, round_arg}, {5, tie_arg}},
                         .memory = memory, .label = label("missing-side news random " + std::to_string(n))});
    }
}

// 0x10481e90: the draw's news. FIX_MAN's fixture iterator (0x1048a2a0 gathers, 0x10738e00 yields a record per call,
// 0x10738c00/0x10738c20 open and close it) is answered, so a case chooses the fixtures; so are the fixture manager's
// and the stadium rules' routines the own fixtures are handed to; the date routines, the allocator and the news
// items run for real, and so does the news manager except where `enabled` is answered.
CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    const auto base = w.cups.front();
    std::mt19937 rng(0xd1ce);
    const auto pick = [&](unsigned bound) { return unsigned(rng() % bound); };
    const auto club_count = scenario.global(-0x7b50);
    constexpr std::uint32_t fixtures_at = area + 0x9400, date_at = area + 0x9300, gather = 0x1048a2a0, open_iterator = 0x10738c00,
                            next_fixture = 0x10738e00, close_iterator = 0x10738c20, enabled = 0x102cdbf0, put_fixture = 0x1048a140,
                            rules = 0x104aff20, add_fixture = 0x104888f0;
    // A record id the competitions table has no object for: the "competition vanished" outcome.
    std::uint32_t vanished = 0;
    {
        const auto table = scenario.global(competitions_slot), count = scenario.global(competition_count_slot);
        for (std::uint32_t id = 0; id < 4096 && !vanished; ++id) if (id >= count || !scenario.r32(table + id * 4)) vanished = id;
    }
    struct Fixture { std::uint32_t id = 0, kind = 1, day = 0, year = 0, stage = 0; bool own = false; };
    for (unsigned n = 0; n < 260; ++n) {
        Spec spec;
        const unsigned round_index = 1 + n % 2;
        spec.rounds.resize(3);
        spec.owner = 3 + n % 4;
        spec.year = 2000 + n % 3;
        auto& round = spec.rounds[round_index];
        round.flags = n % 17 == 0 ? 0 : 0x100;
        round.fixture_day = 150 + n % 40;
        round.fixture_year = n % 3 == 0 ? 0 : 1;
        round.fixture_kind = 1 + n % 2;
        round.leg_rule = n % 3 == 0 ? 2 : 0;
        round.label = 0x20 + n % 5;
        std::vector<Write> extra;
        // The round row's entries (+0x28.. in eights): rules id, weekday, two bytes, state.
        std::vector<Write> row_entries;
        const auto row_at = rounds_at + round_index * 0x78;
        for (unsigned k = 0; k < 9; ++k) {
            const auto at = row_at + 0x28 + 8 * k;
            if (at + 8 > row_at + 0x78) break;
            row_entries.push_back({4, at, 0x500 + k});
            row_entries.push_back({1, at + 4, pick(8)});
            row_entries.push_back({1, at + 5, pick(5)});
            row_entries.push_back({1, at + 6, n % 19 == 0 && k == 0 ? 0xffu : (k >= 1 && pick(6) == 0 ? 0xffu : (k == 5 && n % 4 == 0 ? 3u : pick(4) == 0 ? 3u : 0u))});
        }
        const unsigned count = 1 + n % 11;
        std::vector<Fixture> fixtures;
        for (unsigned k = 0; k < count; ++k) {
            Fixture f;
            f.own = pick(3) != 0;
            f.stage = pick(8) == 0 ? spec.owner + 1 : spec.owner;
            f.day = f.own ? round.fixture_day : round.fixture_day + pick(3);
            f.year = spec.year + (round.fixture_year ? 1 : 0) + (f.own || pick(2) ? 0 : 1);
            f.kind = f.own || pick(2) ? round.fixture_kind : 3 - round.fixture_kind;
            fixtures.push_back(f);
        }
        std::vector<Case::NthAnswer> nth;
        for (unsigned k = 0; k < count; ++k) {
            const auto at = fixtures_at + k * 0x50;
            for (std::uint32_t offset = 0; offset < 0x50; offset += 2) extra.push_back({2, at + offset, 0x4d4d});
            extra.push_back({4, at + 0x00, 77});
            extra.push_back({4, at + 0x04, 0x300 + k});
            extra.push_back({4, at + 0x0c, 0x400 + k});
            extra.push_back({4, at + 0x10, 0x500 + k});
            extra.push_back({4, at + 0x1c, scenario.club((k * 11 + n) % club_count)});
            extra.push_back({4, at + 0x20, scenario.club((k * 13 + n + 5) % club_count)});
            extra.push_back({2, at + 0x28, fixtures[k].year});
            extra.push_back({2, at + 0x2a, fixtures[k].day});
            extra.push_back({1, at + 0x3f, fixtures[k].kind});
            extra.push_back({1, at + 0x42, fixtures[k].stage});
            nth.push_back({next_fixture, k + 1, at});
        }
        nth.push_back({next_fixture, count + 1, 0});
        for (unsigned k = 0; k < 100; ++k) nth.push_back({enabled, k + 1, n % 5 == 0 ? 1u : pick(3) == 0 ? 0u : 1u});
        extra.push_back({2, date_at, 100 + n});
        extra.push_back({2, date_at + 2, 2000});
        extra.push_back({1, date_at + 4, 0});
        spec.extra = joined(joined(extra, row_entries), {});
        const unsigned gathered = n % 23 == 0 ? 0 : 1 + pick(3);
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers{{gather, gathered}, {open_iterator, 0}, {close_iterator, 0},
                                                                         {put_fixture, 0}, {rules, 0}, {add_fixture, 0}};
        if (n % 29 == 0 && vanished) spec.extra.push_back({4, 0, 0});  // placeholder removed below
        if (n % 29 == 0 && vanished) spec.extra.pop_back();
        auto memory = make_cup(scenario, base, spec);
        if (n % 29 == 0 && vanished) {
            // The cup's record names an id with no competition: a scratch record whose id word is `vanished`.
            memory.push_back({4, extra_at + 0x200, vanished});
            memory.push_back({4, cup_at + 4, extra_at + 0x200});
        }
        // Rows of the rounds the fixtures belong to need entries of their own: rounds 0 and 2 keep the pattern.
        Case news_case{.entry = draw_news, .registers = {{3, cup_at}, {4, date_at}, {5, round_index}}, .memory = memory,
                       .label = label("draw news random " + std::to_string(n)), .answers = answers, .nth_answers = nth};
        if (n % 41 == 0) {
            // The date's allocation fails (new reaches the dispatcher from the generated original too): the date routines then
            // work on address 0, which faults unless the null page is mapped (--null-page).
            news_case.answers.push_back({0x10773520, 0});
            news_case.label += " (allocation fails)";
        }
        cases.push_back(std::move(news_case));
    }
}

// A random cup for the round routines: real clubs as entrants (the draws and the news run for real under them), round
// rows with dates, flags and entrant counts, and the ties of every round (sides, feeders, states).
struct Random {
    const Scenario& scenario;
    std::mt19937& rng;
    unsigned pick(unsigned bound) { return unsigned(rng() % bound); }
    bool chance(unsigned one_in) { return pick(one_in) == 0; }
    unsigned null_rate = 0x7fffffff;  // one entrant in `null_rate` has no club
    // `target` is the round the routine works on; its ties are in state `state`; `missing` entrants have no club.
    Spec cup(unsigned n, unsigned rounds_count, unsigned target, std::uint32_t state, bool missing) {
        Spec spec;
        const auto club_count = scenario.global(-0x7b50);
        const unsigned entrant_count = 8 + pick(12);
        spec.rounds.resize(rounds_count);
        for (unsigned e = 0; e < entrant_count; ++e)
            spec.entrants.push_back({chance(null_rate) ? 0u : scenario.club((e * 37 + n * 11) % club_count), pick(5) - 2u, 0u});
        spec.owner = chance(5) ? 0xff : pick(6);
        spec.year = 2000 + pick(3);
        spec.code = 5 + pick(40);
        spec.flag45 = chance(25) ? 0 : 1;
        spec.current_round = chance(4) ? pick(rounds_count) : target;
        spec.ninth = pick(rounds_count + 1);
        std::vector<unsigned> counts;
        for (unsigned r = 0; r < rounds_count; ++r) counts.push_back(r == target ? 1 + pick(6) : 1 + pick(4));
        for (unsigned r = 0; r < rounds_count; ++r) {
            auto& round = spec.rounds[r];
            round.label = chance(5) ? 0x8c : pick(40);
            round.flags = std::vector<std::uint32_t>{0, 0x400, 0x200, 0x100, 0x600, 0x800, 0x300, 0x500}[pick(8)];
            round.rule = std::vector<std::uint32_t>{0, 4, 1, 8, 5, 0xfff0}[pick(6)];
            round.leg_rule = chance(3) ? 2 : pick(2);
            round.rule_byte_22 = pick(3);
            round.byte_24 = pick(8);
            round.draw_day = 100 + pick(120);
            round.draw_year = pick(3) - 1u;
            round.fixture_day = 150 + pick(150);
            round.fixture_year = pick(2);
            round.fixture_kind = 1 + pick(2);
            round.entrants = pick(9) + 2;
            round.rules_id = 0x1000 + r;
            for (unsigned t = 0; t < counts[r]; ++t) {
                Tie tie;
                tie.first = chance(8) ? none : pick(entrant_count);
                tie.second = chance(8) ? none : pick(entrant_count);
                const auto feeder = [&](std::uint32_t& fr, std::uint32_t& fm) {
                    if (chance(3)) { fr = none; fm = 0; return; }
                    fr = pick(rounds_count);
                    fm = pick(counts[fr]);
                };
                feeder(tie.first_round, tie.first_match);
                feeder(tie.second_round, tie.second_match);
                tie.state = r == target ? state : pick(4);
                round.ties.push_back(tie);
            }
        }
        if (missing) {
            // An entrant a tie of the target round uses has no club.
            auto& tie = spec.rounds[target].ties[pick(counts[target])];
            const auto which = chance(2);
            if (tie.first == none) tie.first = 0;
            if (tie.second == none) tie.second = 1;
            spec.entrants[which ? tie.first : tie.second].club = 0;
        }
        return spec;
    }
};

// 0x1047e980: a round's draw.
CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    const auto base = w.cups.front();
    std::mt19937 rng(0xd4a3);
    Random random{scenario, rng};
    constexpr std::int32_t opponents_slot = -0x7a64, rules_slot = -0x7b54;
    constexpr std::uint32_t check_a = 0x102aed80, check_b = 0x10494d20, enabled = 0x102cdbf0;
    for (unsigned n = 0; n < 400; ++n) {
        const unsigned rounds_count = 3 + n % 11, target = n % 5 == 0 ? rounds_count - 1 : random.pick(rounds_count);
        const bool missing = n % 6 == 0;
        auto spec = random.cup(n, rounds_count, target, n % 23 == 0 ? 1 : 0, missing);
        std::vector<Case::NthAnswer> nth;
        for (unsigned k = 0; k < 60; ++k) nth.push_back({enabled, k + 1, n % 4 == 0 ? 1u : random.pick(3) == 0 ? 0u : 1u});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        auto memory = make_cup(scenario, base, spec);
        const auto cell_a = scenario.toc(opponents_slot), cell_b = scenario.toc(rules_slot);
        const bool use_a = n % 3 != 0, use_b = n % 5 != 0;
        memory.push_back({4, cell_a, use_a ? 0x1234u : 0u});
        memory.push_back({4, cell_b, use_b ? 0x1234u : 0u});
        answers.push_back({check_a, n % 7 == 0 ? 1u : 0u});
        answers.push_back({check_b, n % 11 == 0 ? 1u : 0u});
        cases.push_back({.entry = make_draw, .registers = {{3, cup_at}, {4, target}}, .memory = memory,
                         .label = label("draw random " + std::to_string(n)), .answers = answers, .nth_answers = nth});
    }
}

// 0x1047f3a0 (a tie's fixtures) and 0x1047ef50 (the next round).
CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    std::uint32_t base_84 = 0;
    std::map<std::uint32_t, unsigned> codes;
    for (const auto cup : w.cups) {
        const auto code = w.slot_code(cup, 0x84);
        ++codes[code];
        if (!base_84 && code == 0x104530f0) base_84 = cup;
    }
    for (const auto& [code, count] : codes) std::fprintf(stderr, "rounds: virtual +0x84 body %x: %u cups\n", code, count);
    if (!base_84) throw std::runtime_error("rounds: no cup uses the base virtual +0x84");
    const auto base = base_84;
    std::mt19937 rng(0xf1c5);
    Random random{scenario, rng};
    constexpr std::uint32_t enabled = 0x102cdbf0, rules = 0x104aff20, add_fixture = 0x104888f0;
    const auto date_cell = scenario.toc(-0x7b80);
    for (unsigned n = 0; n < 600; ++n) {
        const unsigned rounds_count = 2 + n % 6, target = random.pick(rounds_count);
        random.null_rate = n % 4 == 0 ? 6 : 0x7fffffff;
        auto spec = random.cup(n, rounds_count, target, n % 3 == 0 ? 1 : 2, false);
        auto& rows = spec.rounds[target].ties;
        // A tie waiting in each state: the first tie 1, the second 2 (the search finds the first in either state).
        for (std::size_t t = 0; t < rows.size(); ++t) rows[t].state = t == 0 ? (n % 3 == 0 ? 1 : 2) : random.pick(4);
        if (n % 9 == 0) for (auto& tie : rows) tie.state = 0;      // none waiting: the search ends with -1
        const unsigned tie_index = random.pick(unsigned(rows.size()));
        const std::uint32_t tie_arg = n % 4 == 0 ? 0xffffffffu : n % 13 == 0 ? 0xabcdffffu : tie_index;
        const std::uint32_t first_leg = n % 2 ? 1u : 0u;
        std::vector<Case::NthAnswer> nth;
        for (unsigned k = 0; k < 20; ++k) nth.push_back({enabled, k + 1, n % 5 == 0 ? 1u : random.pick(3) == 0 ? 0u : 1u});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers{{rules, 0}, {add_fixture, 0}};
        auto memory = make_cup(scenario, base, spec);
        // The game's current date: before, equal to and after the round dates.
        memory.push_back({2, date_cell, 100 + random.pick(300)});
        memory.push_back({2, date_cell + 2, 2000 + random.pick(3)});
        memory.push_back({1, date_cell + 4, 0});
        if (n % 37 == 0) memory.push_back({4, cup_at + 4, 0});  // no record: faults unless the null page is mapped (--null-page)
        else if (n % 31 == 0) {
            std::uint32_t vanished = 0;
            const auto table = scenario.global(competitions_slot), count = scenario.global(competition_count_slot);
            for (std::uint32_t id = 0; id < 4096 && !vanished; ++id) if (id >= count || !scenario.r32(table + id * 4)) vanished = id;
            memory.push_back({4, extra_at + 0x200, vanished});
            memory.push_back({4, cup_at + 4, extra_at + 0x200});
        }
        cases.push_back({.entry = schedule, .registers = {{3, cup_at}, {4, target}, {5, tie_arg}, {6, first_leg}}, .memory = memory,
                         .label = label("fixtures random " + std::to_string(n)), .answers = answers, .nth_answers = nth});
    }
}

// 0x1047ef50: the next round. The fixture iterator and the fixture manager's and stadium rules' routines are answered as
// for 0x10481e90 (a case lists the fixtures the iterator yields); the draw, the dates, the allocator and the news run.
CUP_CASES("rounds") {
    if (scenario.match) return;
    const World w(scenario);
    const auto label = [](const std::string& text) { return "rounds: " + text; };
    std::uint32_t base_84 = 0;
    for (const auto cup : w.cups) if (w.slot_code(cup, 0x84) == 0x104530f0) { base_84 = cup; break; }
    if (!base_84) throw std::runtime_error("rounds: no cup uses the base virtual +0x84");
    std::mt19937 rng(0xadc3);
    Random random{scenario, rng};
    constexpr std::uint32_t enabled = 0x102cdbf0, rules = 0x104aff20, add_fixture = 0x104888f0, put_fixture = 0x1048a140,
                            gather = 0x1048a2a0, open_iterator = 0x10738c00, next_fixture = 0x10738e00, close_iterator = 0x10738c20,
                            check_a = 0x102aed80, check_b = 0x10494d20;
    const auto date_cell = scenario.toc(-0x7b80);
    const auto club_count = scenario.global(-0x7b50);
    for (unsigned n = 0; n < 500; ++n) {
        const unsigned rounds_count = 2 + n % 7;
        const unsigned target = n % 11 == 0 ? rounds_count + n % 3 : random.pick(rounds_count);
        const unsigned shaped = std::min(target, rounds_count - 1);
        auto spec = random.cup(n, rounds_count, shaped, n % 4 == 0 ? 1 : 0, n % 5 == 0);
        // The current round's ties in states 1 and 2 (scheduled), else 0 (drawn by the routine).
        auto& current = spec.rounds[std::min<unsigned>(spec.current_round, rounds_count - 1)].ties;
        for (std::size_t t = 0; t < current.size(); ++t) current[t].state = random.pick(4);
        if (n % 7 == 0) for (auto& tie : current) tie.state = 2;
        std::uint32_t round_arg = target;
        if (n % 17 == 0) round_arg = 0xffffffffu;
        if (n % 19 == 0) round_arg = 0x10000u + shaped;
        std::vector<Case::NthAnswer> nth;
        for (unsigned k = 0; k < 40; ++k) nth.push_back({enabled, k + 1, n % 5 == 0 ? 1u : random.pick(3) == 0 ? 0u : 1u});
        std::vector<Write> fixtures;
        const unsigned count = n % 3;
        for (unsigned k = 0; k < count; ++k) {
            const auto at = area + 0x9400 + k * 0x50;
            for (std::uint32_t offset = 0; offset < 0x50; offset += 2) fixtures.push_back({2, at + offset, 0x4d4d});
            fixtures.push_back({4, at + 0x1c, scenario.club((k * 11 + n) % club_count)});
            fixtures.push_back({4, at + 0x20, scenario.club((k * 13 + n + 5) % club_count)});
            fixtures.push_back({1, at + 0x42, spec.owner});
            nth.push_back({next_fixture, k + 1, at});
        }
        nth.push_back({next_fixture, count + 1, 0});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers{{rules, 0}, {add_fixture, 0}, {put_fixture, 0}, {open_iterator, 0},
                                                                         {close_iterator, 0}, {gather, n % 3 == 1 ? 0u : 1u},
                                                                         {check_a, n % 7 == 0 ? 1u : 0u}, {check_b, n % 11 == 0 ? 1u : 0u}};
        auto memory = joined(make_cup(scenario, base_84, spec), fixtures);
        memory.push_back({2, date_cell, 100 + random.pick(300)});
        memory.push_back({2, date_cell + 2, 2000 + random.pick(3)});
        memory.push_back({1, date_cell + 4, 0});
        memory.push_back({4, scenario.toc(-0x7a64), n % 3 != 0 ? 0x1234u : 0u});
        memory.push_back({4, scenario.toc(-0x7b54), n % 5 != 0 ? 0x1234u : 0u});
        if (n % 29 == 0) {
            std::uint32_t vanished = 0;
            const auto table = scenario.global(competitions_slot), ids = scenario.global(competition_count_slot);
            for (std::uint32_t id = 0; id < 4096 && !vanished; ++id) if (id >= ids || !scenario.r32(table + id * 4)) vanished = id;
            memory.push_back({4, extra_at + 0x200, vanished});
            memory.push_back({4, cup_at + 4, extra_at + 0x200});
        }
        Case next_case{.entry = next_round, .registers = {{3, cup_at}, {4, round_arg}}, .memory = memory,
                       .label = label("next random " + std::to_string(n)), .answers = answers, .nth_answers = nth};
        if (n % 43 == 0) {
            next_case.answers.push_back({0x10773520, 0});
            next_case.label += " (allocation fails)";
        }
        cases.push_back(std::move(next_case));
    }
}
}
}

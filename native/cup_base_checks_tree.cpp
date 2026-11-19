// CUP.CPP, block `tree`: cases for 0x10479100 (`CUP::map_fixture_tree`). Real cup objects of the career (their real
// round tables), and scratch cups (a copy of a real cup with its round rows, entrant rows, clubs and regions built in
// the scratch area) whose rounds, flags and entrants steer each branch: brackets of every shape, the next round's
// skip-draw (0x200) and taken-slot (0x400) flags, the shuffle (the next round has fewer than half the ties), the
// fall-back to the round after next, the entrant seeding with and without the region rule, the final-label rounds,
// and the failure paths (allocation answered 0 per call, a round without ties, no free slot). The sample stream is
// set per case through Case::memory (a table of halfwords in the scratch area, so each coin and draw is chosen);
// the checker undoes it. Labels start with "tree:" (CM_CUP_BLOCK=tree).
#include "cup_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
constexpr std::uint32_t map_tree = 0x10479100, allocate = 0x10775130;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::int32_t cursor_slot = -0x7b84, limit_slot = -0x7b88, salt_slot = -0x7b90;
constexpr std::uint32_t area = scratch_base + 0x60000;
constexpr std::uint32_t cup_at = area, rounds_at = area + 0x400, entrants_at = area + 0x2000, clubs_at = area + 0x3000,
                        regions_at = area + 0x5000, table_at = area + 0x6000;
constexpr std::uint32_t object_size = 0xb4;
std::string hex(std::uint32_t value) {
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
    // A cup's vtable has the base class's 40 slots (+0x08..+0xa4); one the derived class does not override names
    // CUP.CPP's body. The kind byte +0x43 is 1 (the cup constructor's; a league's is 2).
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
};

// A round's row (0x78 bytes) as the routine reads it: label +8, flags +0xe, entrants +0x1a, ties +0x1c, seeded +0x1e,
// first entrant +0x20.
struct Round {
    std::uint32_t label = 0, flags = 0;
    std::int32_t entrants = 0, ties = 1, seeded = 0, first = 0;
};
constexpr std::uint32_t skip_draw = 0x200, avoid_taken = 0x400, final_label = 0x8c;
struct Spec {
    std::vector<Round> rounds;
    std::int32_t clubs = 0;                // entrant rows (halfword +0x3e)
    bool table = true;                     // a table at +0xa4 (else null)
    std::uint32_t rule = 0xffffffffu;      // the word +0x24
    std::uint32_t flag45 = 1;
    std::vector<int> regions;              // club i's region (-1: null pointer); ids are 100 + region
    std::vector<int> null_clubs;           // entrant rows whose club pointer is 0
    std::int32_t round_count_override = 0x7fffffff;
};

// The scratch cup: a copy of a real cup's first 0xb4 bytes with the fields the routine reads replaced, and the
// round, entrant, club and region rows. The fields it writes hold a pattern, so a missing write shows.
std::vector<Write> make_cup(const Scenario& s, std::uint32_t base, const Spec& spec) {
    std::vector<Write> memory;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) memory.push_back({4, cup_at + offset, s.r32(base + offset)});
    const auto count = spec.round_count_override != 0x7fffffff ? std::uint32_t(spec.round_count_override) : std::uint32_t(spec.rounds.size());
    memory.push_back({4, cup_at + 0x24, spec.rule});
    memory.push_back({2, cup_at + 0x3c, count});
    memory.push_back({2, cup_at + 0x3e, std::uint32_t(spec.clubs)});
    memory.push_back({1, cup_at + 0x45, spec.flag45});
    memory.push_back({4, cup_at + 0xa0, rounds_at});
    memory.push_back({4, cup_at + 0xa4, spec.table ? entrants_at : 0});
    memory.push_back({1, cup_at + 0xa8, 0x5a});
    memory.push_back({1, cup_at + 0xa9, 0x5a});
    memory.push_back({4, cup_at + 0xac, 0x5a5a5a5au});
    memory.push_back({1, cup_at + 0xb0, 0});
    for (std::size_t r = 0; r < spec.rounds.size(); ++r) {
        const auto at = rounds_at + std::uint32_t(r) * 0x78;
        const auto& round = spec.rounds[r];
        memory.push_back({2, at + 0x08, round.label});
        memory.push_back({2, at + 0x0e, round.flags});
        memory.push_back({2, at + 0x1a, std::uint32_t(round.entrants)});
        memory.push_back({2, at + 0x1c, std::uint32_t(round.ties)});
        memory.push_back({2, at + 0x1e, std::uint32_t(round.seeded)});
        memory.push_back({2, at + 0x20, std::uint32_t(round.first)});
        memory.push_back({1, at + 0x68, 0x77});
    }
    for (std::int32_t i = 0; i < spec.clubs && i < 40; ++i) {
        const auto club = clubs_at + std::uint32_t(i) * 0x40;
        const bool null_club = std::find(spec.null_clubs.begin(), spec.null_clubs.end(), i) != spec.null_clubs.end();
        memory.push_back({4, entrants_at + std::uint32_t(i) * 8, null_club ? 0 : club});
        memory.push_back({1, entrants_at + std::uint32_t(i) * 8 + 4, 0x11});
        memory.push_back({1, entrants_at + std::uint32_t(i) * 8 + 5, 0x77});
        const int region = spec.regions.empty() ? -1 : spec.regions[std::size_t(i) % spec.regions.size()];
        memory.push_back({4, club + 0x3c, region < 0 ? 0u : regions_at + std::uint32_t(region) * 0x10});
    }
    for (std::uint32_t r = 0; r < 8; ++r) memory.push_back({4, regions_at + r * 0x10, 100 + r});
    return memory;
}

// The stream: the cursor at the table, the limit `steps` halfwords on (the refill then takes the real start), a
// salt, and the table's halfwords by mode.
enum class Samples { even, odd, mixed, ascending, natural };
std::vector<Write> stream(const Scenario& s, Samples mode, std::uint32_t steps = 400, std::uint32_t salt = 0) {
    if (mode == Samples::natural) return {};
    std::vector<Write> writes{{4, s.toc(cursor_slot), table_at}, {4, s.toc(limit_slot), table_at + 2 * steps},
                              {4, s.toc(salt_slot), salt}};
    std::uint32_t state = 0x2545;
    for (std::uint32_t n = 1; n <= steps + 1; ++n) {
        std::uint32_t value = 0;
        switch (mode) {
        case Samples::even: value = 0; break;
        case Samples::odd: value = 1; break;
        case Samples::ascending: value = n; break;
        case Samples::mixed: state ^= state << 7; state ^= state >> 9; state &= 0xffff; value = state; break;
        case Samples::natural: break;
        }
        writes.push_back({2, table_at + 2 * n, value});
    }
    return writes;
}
const char* name(Samples mode) {
    switch (mode) {
    case Samples::even: return "even";
    case Samples::odd: return "odd";
    case Samples::mixed: return "mixed";
    case Samples::ascending: return "ascending";
    case Samples::natural: return "natural";
    }
    return "";
}

// A bracket: `ties` per round, `entrants` twice the ties, round 0 seeded with `seeded` entrants.
Spec bracket(std::vector<int> ties, int seeded = 0, std::uint32_t second_flags = 0, std::uint32_t last_label = 0) {
    Spec spec;
    for (std::size_t r = 0; r < ties.size(); ++r) {
        Round round;
        round.label = r + 1 == ties.size() && last_label ? last_label : std::uint32_t(r);
        round.ties = ties[r];
        round.entrants = 2 * ties[r];
        round.flags = r ? second_flags : 0;
        round.seeded = r == 0 ? seeded : 0;
        spec.rounds.push_back(round);
    }
    spec.clubs = std::max(seeded, 4);
    return spec;
}

CUP_CASES("tree") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.size() < 3) throw std::runtime_error("tree: the career has fewer than three cup objects");
    std::fprintf(stderr, "tree: %zu cup objects\n", w.cups.size());
    const auto label = [](const std::string& text) { return "tree: " + text; };
    const auto base = w.cups.front();
    const auto add = [&](const std::string& text, const Spec& spec, Samples mode, std::vector<Write> extra = {},
                         std::vector<Case::NthAnswer> nth = {}) {
        Case c{.entry = map_tree, .registers = {{3, cup_at}},
               .memory = joined(joined(make_cup(scenario, base, spec), stream(scenario, mode)), extra),
               .label = label(text + " [" + name(mode) + "]")};
        if (!nth.empty()) c.nth_answers = nth, c.original_owners = {allocate};
        cases.push_back(std::move(c));
    };
    const Samples modes[] = {Samples::even, Samples::odd, Samples::mixed, Samples::ascending, Samples::natural};

    // The career's own cups, with the real stream and with chosen samples.
    for (const auto cup : w.cups) {
        cases.push_back({.entry = map_tree, .registers = {{3, cup}}, .label = label("career cup " + hex(cup))});
        for (const auto mode : {Samples::even, Samples::odd, Samples::mixed})
            cases.push_back({.entry = map_tree, .registers = {{3, cup}}, .memory = stream(scenario, mode),
                             .label = label("career cup " + hex(cup) + " [" + name(mode) + "]")});
    }

    // Brackets: every tie of a round feeds the next; the flags of the later rounds and the seeded entrants vary.
    const std::vector<std::vector<int>> shapes = {{8, 4, 2, 1}, {16, 8, 4, 2, 1}, {4, 2, 1}, {2, 1}, {1, 1}, {1},
                                                   {3, 2, 1}, {5, 3, 2}, {6, 3, 2, 1}, {7, 4, 2}, {3, 3, 2}, {2, 2, 1},
                                                   {1, 1, 1, 1, 1, 1, 1, 1, 1}};
    for (const auto& ties : shapes) {
        std::string shape;
        for (const auto n : ties) shape += std::to_string(n) + "-";
        shape.pop_back();
        for (const auto mode : modes) {
            add("bracket " + shape, bracket(ties), mode);
            add("bracket " + shape + " seeded", bracket(ties, 2 * ties[0]), mode);
        }
        for (const auto flags : {skip_draw, avoid_taken, skip_draw | avoid_taken})
            for (const auto mode : {Samples::even, Samples::odd, Samples::mixed})
                add("bracket " + shape + " flags " + hex(flags), bracket(ties, 0, flags), mode);
        for (const auto mode : {Samples::even, Samples::mixed})
            add("bracket " + shape + " final label", bracket(ties, 0, 0, final_label), mode);
    }

    // The shuffle (the next round has fewer than half the ties) and the fall-back to the round after next.
    for (const auto& ties : std::vector<std::vector<int>>{{8, 2, 1}, {10, 4, 1}, {9, 2, 2}, {6, 1, 1}, {12, 3, 1, 1}, {4, 1, 2, 1}, {5, 1, 1}, {4, 1, 1}, {3, 1, 1}, {3, 1}, {4, 1}, {4, 1, 2}, {5, 2, 2, 1}}) {
        std::string shape;
        for (const auto n : ties) shape += std::to_string(n) + "-";
        shape.pop_back();
        for (const auto mode : modes) add("pressure " + shape, bracket(ties), mode);
        for (const auto flags : {avoid_taken, skip_draw})
            for (const auto mode : {Samples::even, Samples::odd, Samples::mixed})
                add("pressure " + shape + " flags " + hex(flags), bracket(ties, 0, flags), mode);
    }

    // Entrants: seeded counts, a first entrant, flags on the seeded rounds, rounds after the first seeded too.
    for (const auto& ties : std::vector<std::vector<int>>{{4, 2, 1}, {8, 4}, {3, 2, 1}, {2, 1}, {6, 3, 1}, {5, 5}, {2, 2, 2}}) {
        std::string shape;
        for (const auto n : ties) shape += std::to_string(n) + "-";
        shape.pop_back();
        for (const auto mode : modes) {
            for (const auto seeded : {1, 2, 2 * ties[0] - 1, 2 * ties[0]}) {
                auto spec = bracket(ties, seeded);
                spec.rounds[0].first = 3;
                spec.clubs = 24;
                add("entrants " + shape + " seeded " + std::to_string(seeded), spec, mode);
            }
            auto skip = bracket(ties, 2 * ties[0]);
            skip.rounds[0].flags = skip_draw;
            skip.clubs = 24;
            add("entrants " + shape + " skip-draw seeding", skip, mode);
            auto both = bracket(ties, 2 * ties[0]);
            both.rounds[0].flags = skip_draw | avoid_taken;
            both.clubs = 24;
            add("entrants " + shape + " skip-draw and taken-slot seeding", both, mode);
            auto taken = bracket(ties, 2 * ties[0]);
            taken.rounds[0].flags = avoid_taken;
            taken.clubs = 24;
            add("entrants " + shape + " taken-slot seeding", taken, mode);
            // Seeds into the later rounds' free halves (rounds that receive fewer feeders than they have halves).
            auto later = bracket(ties, 2 * ties[0]);
            for (std::size_t r = 1; r < later.rounds.size(); ++r) later.rounds[r].seeded = 1;
            later.clubs = 30;
            add("entrants " + shape + " seeds in later rounds", later, mode);
            auto later_taken = later;
            for (std::size_t r = 0; r < later_taken.rounds.size(); ++r) later_taken.rounds[r].flags |= avoid_taken;
            add("entrants " + shape + " seeds in later rounds, taken-slot", later_taken, mode);
        }
        // More seeds than halves: the walk comes round to its start and the assertion ends it (no exit).
        for (const auto mode : {Samples::even, Samples::odd, Samples::mixed}) {
            auto over = bracket(ties, 2 * ties[0] + 1);
            over.clubs = 30;
            add("entrants " + shape + " more seeds than halves", over, mode);
            auto over_taken = bracket(ties, 2 * ties[0] + 3);
            over_taken.rounds[0].flags = avoid_taken;
            over_taken.clubs = 30;
            add("entrants " + shape + " more seeds than halves, taken-slot", over_taken, mode);
        }
    }

    // The stream's limit reached at each step: the refill (cursor back to the real start, rand reseeds the salt) at the
    // first, second, ... draw or coin of a bracket, a pressure shape and an entrant shape.
    for (const unsigned steps : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u, 12u, 17u, 25u, 40u}) {
        for (const auto salt : {0u, 12345u}) {
            const auto limit = [&](const std::string& text, const Spec& spec) {
                auto memory = joined(make_cup(scenario, base, spec), stream(scenario, Samples::mixed, steps, salt));
                cases.push_back({.entry = map_tree, .registers = {{3, cup_at}}, .memory = memory,
                                 .label = label(text + " limit " + std::to_string(steps) + " salt " + std::to_string(salt))});
            };
            limit("refill bracket 8-4-2-1", bracket({8, 4, 2, 1}));
            limit("refill pressure 8-2-1", bracket({8, 2, 1}));
            limit("refill entrants 4-2-1", bracket({4, 2, 1}, 8));
            limit("refill fallback 5-1-1", bracket({5, 1, 1}));
        }
    }

    // The refill at each draw of the fall-back (the draw into the round after next) and of the walks around it.
    for (unsigned steps = 0; steps <= 64; ++steps) {
        for (const auto& ties : std::vector<std::vector<int>>{{5, 1, 1}, {4, 2, 2}, {7, 2, 1, 1}}) {
            auto spec = bracket(ties);
            std::string shape;
            for (const auto n : ties) shape += std::to_string(n) + "-";
            auto memory = joined(make_cup(scenario, base, spec), stream(scenario, Samples::mixed, steps, 77));
            cases.push_back({.entry = map_tree, .registers = {{3, cup_at}}, .memory = memory,
                             .label = label("refill fall-back " + shape + " limit " + std::to_string(steps))});
        }
    }

    // The region rule (+0x24): a full tie's halves swap when the second club's region id equals the rule.
    const std::vector<int> region_patterns[] = {{0}, {0, 1, 2}, {0, 1}, {-1}, {1, -1, 0}, {2, 2, 0, 1}};
    for (const auto rule : {100u, 101u, 102u, 999u, 0u, 0xfffffffeu}) {
        for (const auto& regions : region_patterns) {
            for (const auto mode : {Samples::mixed, Samples::even, Samples::odd, Samples::ascending}) {
                auto spec = bracket({3, 2}, 6);
                spec.rule = rule;
                spec.regions = regions;
                spec.clubs = 20;
                add("rule " + hex(rule) + " regions " + std::to_string(regions.size()) + "/" + std::to_string(regions[0]), spec, mode);
                spec = bracket({2, 2}, 4);
                spec.rule = rule;
                spec.regions = regions;
                spec.rounds[0].first = 2;
                spec.clubs = 20;
                add("rule " + hex(rule) + " small regions " + std::to_string(regions.size()) + "/" + std::to_string(regions[0]), spec, mode);
            }
        }
        // A null club row, and a club whose region is null, under the rule.
        auto spec = bracket({3, 2}, 6);
        spec.rule = rule;
        spec.regions = {0, 1, 2};
        spec.null_clubs = {0, 1, 2, 3, 4, 5, 6, 7};
        spec.clubs = 20;
        add("rule " + hex(rule) + " null clubs", spec, Samples::mixed);
        spec.null_clubs.clear();
        spec.regions = {-1};
        add("rule " + hex(rule) + " null regions", spec, Samples::mixed);
    }
    // Seeds into the later rounds with the rule on: a tie whose open half is fed (or whose pair is half taken) is
    // neither seated nor swapped.
    for (const auto rule : {100u, 101u, 999u}) {
        for (const auto mode : {Samples::mixed, Samples::even, Samples::odd, Samples::ascending}) {
            for (const auto& ties : std::vector<std::vector<int>>{{3, 2, 1}, {4, 3, 2}, {2, 2, 2}, {6, 3, 2}}) {
                auto spec = bracket(ties, 2 * ties[0]);
                spec.rule = rule;
                spec.regions = {0, 1, 2};
                spec.clubs = 30;
                for (std::size_t r = 1; r < spec.rounds.size(); ++r) { spec.rounds[r].seeded = 4; spec.rounds[r].first = 8; }
                add("rule " + hex(rule) + " later seeds " + std::to_string(ties[0]) + "-" + std::to_string(ties[1]), spec, mode);
            }
        }
    }
    // Ties past 127: the permutation's bytes are read signed (the original's extsb).
    for (const auto mode : {Samples::mixed, Samples::even})
        add("128 ties", bracket({128, 64, 32, 16, 8, 4, 2, 1}), mode);
    for (const auto mode : {Samples::mixed, Samples::even, Samples::odd})
        add("130 ties", bracket({130, 65, 33, 17, 9, 5, 3, 2, 1}), mode);
    {
        // Seeded halves that name entrants beyond the table's rows (a first entrant past the clubs).
        auto spec = bracket({2, 1}, 4);
        spec.rule = 100;
        spec.regions = {0, 1};
        spec.rounds[0].first = 30;
        spec.clubs = 20;
        add("rule seeds past the entrant rows", spec, Samples::mixed);
    }

    // Round counts and the last rounds' places: the final label, the unrolled loop's blocks and remainder.
    for (std::size_t count = 0; count <= 11; ++count) {
        for (const auto last : {0u, final_label}) {
            Spec spec = bracket(std::vector<int>(count, 1));
            if (count && last) spec.rounds.back().label = last;
            for (std::size_t r = 0; r < spec.rounds.size(); ++r) spec.rounds[r].ties = 1 + int(r % 3 == 0);
            for (std::size_t r = 0; r < spec.rounds.size(); ++r) spec.rounds[r].entrants = 3 + int(r);
            add("rounds " + std::to_string(count) + (last ? " final label" : ""), spec, Samples::mixed);
            Spec single = bracket(std::vector<int>(count, 1));
            if (count && last) single.rounds.back().label = last;
            add("single-tie rounds " + std::to_string(count) + (last ? " final label" : ""), single, Samples::mixed);
        }
    }
    {
        Spec spec = bracket({4, 2, 1});
        spec.flag45 = 0;
        add("byte +0x45 clear", spec, Samples::mixed);
        for (const auto value : {1u, 2u, 0x80u, 0xffu}) {
            spec.flag45 = value;
            add("byte +0x45 " + hex(value), spec, Samples::mixed);
        }
        spec = bracket({4, 2, 1});
        spec.table = false;
        spec.clubs = 5;
        add("no entrant table", spec, Samples::mixed);
        for (const auto clubs : {0, 1, 2, 7, -1}) {
            spec = bracket({4, 2, 1});
            spec.clubs = clubs;
            spec.table = true;
            add("entrant table with " + std::to_string(clubs) + " clubs", spec, Samples::mixed);
        }
        // The round count as the halfword the original sign-extends.
        for (const std::int32_t count : {0, -1, -2, 1, 2, 0xffff, 0x8000}) {
            spec = bracket({4, 2, 1});
            spec.round_count_override = count;
            add("round count " + hex(std::uint32_t(count)), spec, Samples::mixed);
        }
    }

    // Rounds without ties (the assertion, and no exit), in each position and negative.
    for (const std::int32_t ties : {0, -1, -5}) {
        for (std::size_t position = 0; position < 3; ++position) {
            auto spec = bracket({4, 2, 1});
            spec.rounds[position].ties = ties;
            add("round " + std::to_string(position) + " ties " + std::to_string(ties), spec, Samples::mixed);
        }
    }

    // Allocations answered 0 per call: the round list, each round's rows, the permutations.
    for (unsigned nth = 1; nth <= 8; ++nth)
        for (const auto mode : {Samples::even, Samples::mixed})
            add("allocation " + std::to_string(nth) + " fails", bracket({4, 2, 1}), mode, {}, {{allocate, nth, 0}});
    add("allocation 1 fails, one round", bracket({1}), Samples::mixed, {}, {{allocate, 1, 0}});
    add("allocation 2 fails, one round", bracket({1}), Samples::mixed, {}, {{allocate, 2, 0}});
    for (unsigned nth = 1; nth <= 4; ++nth)
        cases.push_back({.entry = map_tree, .registers = {{3, w.cups.front()}}, .nth_answers = {{allocate, nth, 0}},
                         .label = label("career cup, allocation " + std::to_string(nth) + " fails"), .original_owners = {allocate}});
}
}
}

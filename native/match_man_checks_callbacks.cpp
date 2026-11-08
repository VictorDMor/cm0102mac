// MATCH_MAN.CPP checker cases for the callbacks block: the side's release and squad
// preparation, the actor preparation and the two squad-ordering comparators.
#include "match_man_checks.hpp"

namespace cm::aot::match_man_checks {
namespace {
constexpr std::uint32_t release = 0x10206cb0, prepare_squad = 0x10206d80, prepare_actor = 0x1020e8f0,
                        scaled = 0x1020ea00, plain = 0x1020ee00;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

// The side's fields these routines read.
constexpr std::uint32_t squad = 0x34, squad_kind = 0x1b, extras = 0x3c;

void release_cases(const Scenario& scenario, std::vector<Case>& cases) {
    // The registry global (TOC -0x7a6c) holds a pointer; 0x1014c2e0 and 0x101481d0 are the two
    // checks that decide whether the squad is released, answered per case; 0x107735a0 (free) is
    // answered where a case sets the flag. 0x103b1c00 runs for real: an answer for it reaches
    // the rewrite only (the original's call is not dispatched), so it would differ falsely.
    const auto registry = scenario.toc(-0x7a6c);
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto name = "side " + std::to_string(n) + ": ";
        for (std::uint32_t kind : {0u, 1u, 0x80u})
            for (std::uint32_t first : {0u, 1u, 0x100u, 0x180u})      // 0x100 is zero as a byte, 0x180 is not
                for (std::uint32_t second : {0u, 1u, 0x200u}) {
                    if (kind == 0 && (first || second)) continue;
                    cases.push_back({.entry = release, .registers = {{3, side}, {4, 0}},
                                     .memory = {{1, side + squad_kind, kind}},
                                     .label = name + "kind " + std::to_string(kind) + " checks " + std::to_string(first) + "," + std::to_string(second),
                                     .answers = {{0x1014c2e0, first}, {0x101481d0, second}}});
                }
        // The flag counts as a halfword: 0x10000 does not free, 0x10001 does, 0x8000 and -1 do not.
        for (std::uint32_t destroy : {0u, 1u, 0x7fffu, 0x8000u, 0x10000u, 0x10001u, 0xffffffffu})
            cases.push_back({.entry = release, .registers = {{3, side}, {4, destroy}},
                             .memory = {{1, side + squad_kind, 0}}, .label = name + "destroy " + std::to_string(destroy),
                             .answers = {{0x107735a0, 0}}});
        cases.push_back({.entry = release, .registers = {{3, side}, {4, 0}}, .label = name + "real callees"});
        cases.push_back({.entry = release, .registers = {{3, side}, {4, 0}}, .memory = {{4, side + squad, 0}},
                         .label = name + "no squad"});
        cases.push_back({.entry = release, .registers = {{3, side}, {4, 1}}, .memory = {{4, side + squad, 0}},
                         .label = name + "no squad, destroyed", .answers = {{0x107735a0, 0}}});
        cases.push_back({.entry = release, .registers = {{3, side}, {4, 0}}, .memory = {{4, registry, 0}},
                         .label = name + "no registry"});
    }
    cases.push_back({.entry = release, .registers = {{3, 0}, {4, 1}}, .label = "null side"});
}

void squad_cases(const Scenario& scenario, std::vector<Case>& cases) {
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto name = "side " + std::to_string(n) + ": ";
        const auto member = scenario.r32(scenario.player(n, 3) + 0x6c);   // a real person record
        const auto record = scenario.r32(side + extras);
        cases.push_back({.entry = prepare_squad, .registers = {{3, side}}, .label = name + "real"});
        // No member found, then every member found and each later test answered each way.
        for (std::uint32_t role : {2u, 0u, 0x102u, 0x80u})            // 0x102 is 2 as a byte
            for (std::uint32_t suspended : {0u, 1u, 0x100u})
                for (std::uint32_t seated : {0u, member})
                    cases.push_back({.entry = prepare_squad, .registers = {{3, side}},
                                     .label = name + "role " + std::to_string(role) + " suspended " + std::to_string(suspended) +
                                              (seated ? " seated" : " free"),
                                     .answers = {{0x10187680, role}, {0x100e7920, suspended}, {0x102b7c70, seated}}});
        // The extras' word -2 skips the suspension query; the bench nibble sets how many are tried.
        for (std::uint32_t bench : {0x00u, 0x30u, 0x70u, 0x80u, 0xffu})
            for (std::uint32_t word : {0xfffffffeu, 0u})
                cases.push_back({.entry = prepare_squad, .registers = {{3, side}},
                                 .memory = {{4, record, word}, {1, record + 0x4e, bench}},
                                 .label = name + "bench byte " + std::to_string(bench) + " word " + std::to_string(word)});
        // With the real members, the bench at its largest reaches indexes past 19: the assertion.
        cases.push_back({.entry = prepare_squad, .registers = {{3, side}}, .memory = {{1, record + 0x4e, 0xf0}},
                         .label = name + "bench 15 with real members"});
    }
}

void actor_cases(const Scenario& scenario, std::vector<Case>& cases) {
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto name = "side " + std::to_string(n) + ": ";
        const auto staff = scenario.r32(scenario.player(n, 12) + 0x6c);
        // Valid indexes (a signed byte; the upper bits of the register are ignored by the
        // range test but reach the actor preparation as given).
        for (std::uint32_t index : {0u, 1u, 11u, 19u, 0x113u, 0xffffff05u})
            cases.push_back({.entry = prepare_actor, .registers = {{3, side}, {4, index}, {5, staff}, {6, index}},
                             .label = name + "index " + std::to_string(index)});
        cases.push_back({.entry = prepare_actor, .registers = {{3, side}, {4, 7}, {5, 0}, {6, 0x1234}},
                         .label = name + "no staff"});
        // Out of range: the assertion, then exit; both reports end there.
        for (std::uint32_t index : {20u, 0x80u, 0xffu, 0x7fu, 0x114u, 0xffffffffu})
            for (std::uint32_t member : {0u, staff})
                cases.push_back({.entry = prepare_actor, .registers = {{3, side}, {4, index}, {5, member}, {6, 3}},
                                 .label = name + "index " + std::to_string(index) + (member ? "" : " no staff") + " (assertion)"});
    }
}

// Comparators: r3 and r4 point at two slots (in the scratch area) holding player pointers.
void comparator_cases(const Scenario& scenario, std::uint32_t entry, std::vector<Case>& cases) {
    const auto first_slot = scratch_base, second_slot = scratch_base + 4;
    const auto add = [&](std::uint32_t a, std::uint32_t b, std::vector<Write> memory, const std::string& label) {
        memory.push_back({4, first_slot, a});
        memory.push_back({4, second_slot, b});
        cases.push_back({.entry = entry, .registers = {{3, first_slot}, {4, second_slot}}, .memory = std::move(memory), .label = label});
    };
    const auto player = [&](unsigned side, unsigned n) { return scenario.player(side, n); };
    const auto position = [](std::uint32_t at, std::uint32_t value) { return Write{1, at + 0x19, value}; };
    const auto a = player(0, 2), b = player(0, 9), c = player(1, 4);
    // The real squads.
    for (unsigned i : {0u, 1u, 5u, 10u, 11u, 15u, 19u})
        for (unsigned j : {0u, 4u, 10u, 12u, 19u}) {
            add(player(0, i), player(0, j), {}, "players 0/" + std::to_string(i) + " 0/" + std::to_string(j));
            add(player(0, i), player(1, j), {}, "players 0/" + std::to_string(i) + " 1/" + std::to_string(j));
        }
    // Every pair of positions (0..10: roles by the side's table; -2: off the pitch).
    for (std::uint32_t p = 0; p <= 10; ++p)
        for (std::uint32_t q = 0; q <= 10; ++q)
            add(a, b, {position(a, p), position(b, q)}, "positions " + std::to_string(p) + "," + std::to_string(q));
    for (std::uint32_t p : {0xfeu, 3u})
        for (std::uint32_t q : {0xfeu, 3u}) add(a, c, {position(a, p), position(c, q)}, "off the pitch " + std::to_string(p) + "," + std::to_string(q));
    // Roles: the role bits of a position come from the side's table (an int16 per position,
    // found through the player's team and side byte), so the table's cells are written.
    const auto cell = [&](std::uint32_t at, unsigned where) {
        const auto team = scenario.r32(at + 0x1a4);
        const auto side = std::uint32_t(std::int32_t(std::int8_t(scenario.s.memory.read<1>(at + 0x27))));
        return scenario.r32(team + 0x7934 + 4 * side) + 2 * where;
    };
    for (std::uint32_t first : {1u, 2u, 4u, 8u, 0x10u, 0x20u, 0x40u, 0x80u, 0x8004u, 0x104u, 0u})
        for (std::uint32_t second : {1u, 2u, 4u, 8u, 0x10u, 0x20u, 0x40u, 0u})
            add(a, b, {position(a, 3), position(b, 5), {2, cell(a, 3), first}, {2, cell(b, 5), second}},
                "roles " + std::to_string(first) + "," + std::to_string(second));
    // Missing players and players without staff.
    add(0, b, {}, "first missing");
    add(a, 0, {}, "second missing");
    add(0, 0, {}, "both missing");
    add(a, b, {{4, a + 0x70, 0}}, "first without staff");
    add(a, b, {{4, b + 0x70, 0}}, "second without staff");
    add(a, b, {{4, a + 0x70, 0}, {4, b + 0x70, 0}}, "both without staff");
    // Role-1 players (position 0 is the first in most tables) against the staff rating (+0xf).
    for (std::uint32_t rating : {0x13u, 0x14u, 0x80u, 0u})
        for (std::uint32_t p : {0u, 1u, 7u}) {
            add(a, b, {position(a, p), position(b, p + 1), {1, scenario.r32(a + 0x70) + 0xf, rating}}, "first rating " + std::to_string(rating));
            add(a, b, {position(a, p + 1), position(b, p), {1, scenario.r32(b + 0x70) + 0xf, rating}}, "second rating " + std::to_string(rating));
        }
    // Scores: halfword (+0x36) and float (+0x50) extremes, so the conversions saturate or go NaN.
    for (std::uint32_t half : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu})
        for (std::uint32_t form : {0u, 0x3f800000u, 0xbf800000u, 0x4f000000u, 0xcf000000u, 0x7f800000u, 0xff800000u, 0x7fc00000u, 0x3f000000u}) {
            add(a, b, {{2, a + 0x36, half}, {4, a + 0x50, form}}, "first score " + std::to_string(half) + "/" + std::to_string(form));
            add(a, b, {{2, b + 0x36, half}, {4, b + 0x50, form}}, "second score " + std::to_string(half) + "/" + std::to_string(form));
        }
    // Equal players, and the same player twice.
    add(a, a, {}, "same player");
}

MATCH_MAN_CASES("callbacks") {
    if (!scenario.match) return;  // a side exists only in a match
    release_cases(scenario, cases);
    squad_cases(scenario, cases);
    actor_cases(scenario, cases);
    comparator_cases(scenario, scaled, cases);
    comparator_cases(scenario, plain, cases);
}
}
}

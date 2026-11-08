// MATCH_MAN.CPP checker cases for the lineup_a block (match_man_lineup_a.hpp).
#include "match_man_checks.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
constexpr std::uint32_t entry = 0x10206ea0;
}

MATCH_MAN_CASES("lineup_a") {
    // The computer's lineup choice needs a side of a match: the career has none.
    if (!scenario.match) return;
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    // Routines of other modules that have no in-place owner (answered on both sides); the squad
    // registry's (0x103af*, 0x103b0*) and the tactic's (0x1011*) run for real.
    const Answers environment{{0x100e7920, 0}, {0x102b7c70, 0}, {0x103f93e0, 0}, {0x103ecef0, 0}, {0x103f98d0, 0}};
    for (unsigned side = 0; side < 2; ++side) {
        const auto record = scenario.side(side);
        const auto opponent = scenario.r32(scenario.side(1 - side) + lineup_side::club);
        for (std::uint32_t mode : {0u, 1u, 2u}) {
            const auto label = "match side " + std::to_string(side) + " mode " + std::to_string(mode);
            cases.push_back({.entry = entry, .registers = {{3, record}, {4, opponent}, {5, mode}}, .label = label, .answers = environment});
        }
    }
    // The opposing record may not be missing (the assertion reports and exits).
    cases.push_back({.entry = entry, .registers = {{3, scenario.side(0)}, {4, 0}, {5, 0}}, .label = "no opposing record", .answers = environment});
    // A fixture the world's fixture list would give (0x10738e00's first record): built in the
    // scratch area from real pointers (clubs, a rating row), so every read lands on real data.
    const auto fixture = scratch_base + 0x100;
    const auto rating_row = scenario.competition(0);
    // The opposing strength's last branch (0x10459b50 on the fixture's competition entry): a computer side of
    // a strong club, the fixture's kind 7 or 8, and the people table's entry made to have its +8 word.
    for (unsigned side = 0; side < 2; ++side)
        for (std::uint32_t kind : {7u, 8u})
            for (unsigned at_home = 0; at_home < 2; ++at_home) {
                const auto record = scenario.side(side), other = scenario.side(1 - side);
                const auto club = scenario.r32(record + lineup_side::club), opponent = scenario.r32(other + lineup_side::club);
                const auto extras = scenario.r32(record + lineup_side::extras);
                Case item{.entry = entry, .registers = {{3, record}, {4, opponent}, {5, 0}}, .answers = environment};
                item.answers.insert(item.answers.end(), {{0x1014c2e0, 0}, {0x100c7d00, 0}, {0x100c9070, 0x28}, {0x1048b180, 1},
                                                         {0x10738e00, fixture}, {0x10452ed0, 1}, {0x104646d0, 1}});
                item.memory = {{1, record + lineup_side::human, 0}, {2, club + 0x1ec, 0x2000}, {4, extras, 5},
                               {4, scenario.r32(scenario.global(-0x7b5c) + 4 * 72) + 8, scenario.club(2)},
                               {4, fixture + 0x0, 72}, {4, fixture + 0x14, rating_row}, {4, fixture + 0x18, 0x1234},
                               {4, fixture + 0x1c, at_home ? club : opponent}, {4, fixture + 0x20, at_home ? opponent : club},
                               {1, fixture + 0x3a, kind}};
                item.label = "fixture kind " + std::to_string(kind) + (at_home ? " home" : " away") + " side " + std::to_string(side);
                cases.push_back(std::move(item));
            }
    std::uint32_t seed = 20261107;
    const auto next = [&](std::uint32_t range) { seed = seed * 1664525u + 1013904223u; return (seed >> 12) % range; };
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + next(std::uint32_t(values.size()))); };
    for (unsigned n = 0; n < 9000; ++n) {
        const unsigned side = n & 1;
        const auto record = scenario.side(side), other = scenario.side(1 - side);
        // Mostly the match's clubs, sometimes two other real clubs (so the clubs' own answers vary).
        const bool others = next(3) == 0;
        const auto club = others ? scenario.club(next(60)) : scenario.r32(record + lineup_side::club);
        const auto opponent = others ? scenario.club(next(60)) : scenario.r32(other + lineup_side::club);
        const std::uint32_t mode = pick({0, 1, 2, 1, 2, 0x101});
        Case item{.entry = entry, .registers = {{3, record}, {4, opponent}, {5, mode}}};
        auto& memory = item.memory;
        if (others) memory.push_back({4, record + lineup_side::club, club});
        const auto human = pick({0, 0, 0, 1});
        memory.push_back({1, record + lineup_side::human, human});
        memory.push_back({1, record + lineup_side::urgency, pick({0, 1, 2, 3, 0x102})});
        memory.push_back({1, record + lineup_side::slot, pick({0, 1})});
        if (!next(3)) memory.push_back({4, record + lineup_side::assistant, 0});
        if (!next(5)) memory.push_back({4, record + lineup_side::manager, 0});
        // The assistant's bytes and the per-slot floats the final flags are chosen by.
        const auto assistant = scenario.r32(record + lineup_side::assistant);
        memory.push_back({1, assistant + 0x2e, pick({0, 3, 5, 6, 9, 0xc, 0xd, 0x10, 0x11, 0x13, 0x14, 0xfe})});
        memory.push_back({1, assistant + 0x33, pick({0, 5, 9, 10, 11, 13, 14, 30, 0xff})});
        memory.push_back({1, scenario.r32(record + lineup_side::match) + next(2) + 0x1c6, pick({0, 1, 5, 0xff, 50, 100, 127, 0x80, 20})});
        for (unsigned slot = 0; slot < 27; slot += slot == 1 ? 10 : 1)
            memory.push_back({4, scenario.r32(record + lineup_side::match) + 0x30f0 + 4 * slot, pick({0u, 0x3f000000u, 0x3f800000u, 0x42c80000u, 0xbf800000u, 0x3e800000u})});
        // Squad size: the bench nibble of the settings byte.
        const auto settings = scenario.r32(scenario.r32(record + lineup_side::match) + 0x3118);
        memory.push_back({1, settings + 0x4e, (pick({next(16), next(16), 2, 3}) << 4) | pick({0, 1, 0xf})});
        // The clubs' ratings (halfwords at +0x1ec): the swap check needs a strong club.
        memory.push_back({2, club + 0x1ec, pick({0x1000, 0x1770, 0x2000, 0x2800, 0x3200, 0x4000, 100, 0xf000})});
        memory.push_back({2, opponent + 0x1ec, pick({0x800, 0x1800, 0x2400, 0x3000, 0x4000, 0x1000, 0, 0xf000})});
        const auto extras = scenario.r32(record + lineup_side::extras);
        if (!next(6)) memory.push_back({4, extras, 0xfffffffe});
        else if (next(3) == 0) memory.push_back({4, extras, scenario.global(-0x7aa8)});  // the word the squad scan compares with
        else if (next(2)) memory.push_back({4, extras, pick({72, next(40)})});
        // The random table's cursor close to its limit: a draw wraps and reseeds (TOC -0x7b84 holds the
        // cursor's address, -0x7b88 the limit's).
        if (next(3) == 0) memory.push_back({4, scenario.toc(-0x7b84), scenario.global(-0x7b88) - 2 * next(9)});
        if (next(3) == 0) memory.push_back({4, club + 0x90, next(2) ? scenario.person(1 + next(6)) : scenario.person(scenario.people() - 1 - next(10))});
        memory.push_back({1, extras + lineup_extras::kind_byte, pick({0, 1, 7, 8, 9})});
        memory.push_back({1, extras + 0x42, pick({0xff, 0, 1})});
        memory.push_back({2, extras + 0x30, pick({0x78, 0x7a, 0x84, 0x98, 0x9b, 0, 0x100})});
        // The fixture record: ids, rating rows, the two clubs (this club on either side) and a date.
        memory.push_back({1, scratch_base + 0x400 + 0x75, pick({0, 3, 6, 7, 12})});
        const bool at_home = next(2);
        memory.push_back({4, fixture + 0x0, pick({72, 72, next(40)})});
        // The one entry of the table at TOC -0x7b5c (index 72) gets the +8 word 0x10459b50 is asked about.
        if (next(2)) memory.push_back({4, scenario.r32(scenario.global(-0x7b5c) + 4 * 72) + 8, pick({scenario.club(2), scenario.club(7)})});
        memory.push_back({4, fixture + 0x14, pick({rating_row, scenario.club(3), scenario.r32(extras + lineup_extras::reference)})});
        memory.push_back({4, fixture + 0x18, pick({scenario.r32(club + 0x3c), 0x1234u})});
        memory.push_back({4, fixture + 0x1c, at_home ? club : opponent});
        memory.push_back({4, fixture + 0x20, at_home ? opponent : club});
        memory.push_back({2, fixture + 0x28, pick({0, 1, 2, 3})});
        memory.push_back({2, fixture + 0x2a, pick({0, 1, 2, 3})});
        // 0x10717890 gives the current date (the word at TOC -0x7b80; the index is added to its high half)
        // advanced by its index; the fixture's date, made equal to one of those, matches at that index.
        if (next(2)) {
            const auto dates = scenario.toc(-0x7b80);
            const auto target = 1 + next(7);
            memory.push_back({4, dates, 0x00200010});
            memory.push_back({2, fixture + 0x28, 0x10u});
            memory.push_back({2, fixture + 0x2a, 0x20u + target});
        }
        if (next(2)) {
            memory.push_back({4, club + 0x74 + 4 * next(3), pick({opponent, club, 0x2222u})});
            memory.push_back({4, club + 0x74 + 4 * next(3), at_home ? opponent : club});
        }
        memory.push_back({1, fixture + 0x3a, pick({0, 7, 8})});
        // Each decision callee answers 0 or 1 (or a value a real call gives).
        item.answers = {{0x100e7920, pick({0, 1})}, {0x102b7c70, pick({0, 0, 1})}, {0x103f93e0, pick({0, 1})},
                        {0x103ecef0, pick({0, 1})}, {0x103f98d0, pick({0, 0, 1})},                         {0x100c7d00, pick({0, 0, 1})}, {0x100c9070, pick({0x10, 0x28, 0x32})},
                        {0x1048b180, pick({0, 1, 1})}, {0x10738e00, pick({0, fixture, fixture})},
                        {0x10464160, pick({0, 1})}, {0x10459b50, pick({0, scratch_base + 0x400, scratch_base + 0x400})}, {0x10452ed0, pick({0, 1})}, {0x104646d0, pick({0, 1})},
                        {0x10147e70, pick({0, 1})}, {0x10149df0, pick({0, 1})},
                        {0x1077dc90, pick({0, 1})}, {0x10083110, pick({0, 1})},
                        {0x10187680, pick({0, 2, 0x102, 5})}};
        if (next(2)) item.answers.push_back({0x1014c2e0, pick({0, 0, 1})});
        // The classifier's kind (the original's results are 1 to 5).
        item.answers.push_back({0x1020c5d0, pick({1, 2, 3, 4, 5, 1, 3, 4, 5, 0, 0x104})});
        item.label = "random " + std::to_string(n) + " side " + std::to_string(side) + " mode " + std::to_string(mode) +
                     (human ? " human" : " computer");
        cases.push_back(std::move(item));
    }
    // The replacement check's reference record (the extras' +0x14 pointer) made the current one
    // (its first word equal to TOC -0x7a2c's: the competition's own row is rated) or the kept one
    // (TOC -0x7a30's, with extras +0x42 at -1: the side is marked changed). The gates before it are
    // passed as in the fixture cases.
    for (unsigned side = 0; side < 2; ++side)
        for (int slot : {-0x7a2c, -0x7a30}) {
            const auto record = scenario.side(side), other = scenario.side(1 - side);
            const auto club = scenario.r32(record + lineup_side::club), opponent = scenario.r32(other + lineup_side::club);
            const auto extras = scenario.r32(record + lineup_side::extras);
            Case item{.entry = entry, .registers = {{3, record}, {4, opponent}, {5, 0}}, .answers = environment};
            item.answers.insert(item.answers.end(), {{0x1014c2e0, 0}, {0x100c7d00, 0}, {0x100c9070, 0x28}, {0x1048b180, 1},
                                                     {0x10738e00, fixture}, {0x10452ed0, 1}, {0x104646d0, 1}});
            item.memory = {{1, record + lineup_side::human, 0}, {2, club + 0x1ec, 0x2000}, {4, extras, 5},
                           {4, scenario.r32(extras + lineup_extras::reference), scenario.global(slot)},
                           {1, extras + lineup_extras::kind_byte, 0}, {1, extras + 0x42, 0xff},
                           {4, fixture + 0x0, 72}, {4, fixture + 0x14, rating_row}, {4, fixture + 0x18, 0x1234},
                           {4, fixture + 0x1c, club}, {4, fixture + 0x20, opponent}, {1, fixture + 0x3a, 0}};
            item.label = std::string("reference record = TOC ") + (slot == -0x7a2c ? "-0x7a2c" : "-0x7a30") + " side " + std::to_string(side);
            cases.push_back(std::move(item));
        }
}
}

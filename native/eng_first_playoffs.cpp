#include "eng_first_detail.hpp"
#include "english_competition_rules.hpp"

namespace cm::aot::eng_first {
using namespace detail;

void advance_stage(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x105291c0, 0x70, {.first_gpr = 30});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(-0x7ba8)}});
    const Competition league{c, address};
    c.set({{4, c.u32(address + 0x2c)}, {5, std::uint32_t(std::int32_t(league.last_stage()))}});
    if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(4) - 1)) >= 0) {
        assertion(c, f, 0x10529204, 0x40, 0x38, 0x2ce);
        c.set(3, c.toc(-0x7bac)); c.put8(c.r(3), 0);
        return;
    }
    league.last_stage() = std::int32_t(c.r(5) + 1);
    if (compare(c, league.last_stage()) == 0) call(c, 0x10529280, 0x10529264);
}

void create_playoffs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10529280, 0xd0, {.first_gpr = 25});
    keep(c, {{29, c.toc(-0x7bac)}, {27, address}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    const Competition league{c, address}; Program p{c};
    const auto entrants = std::uint32_t(std::int32_t(c.i8(address + 0xc1)));
    Kept<28> clubs(c, p.call(0x10775130, 0x105292b0, {entrants * 4}));
    if (compare(c, clubs) == 0) { assertion(c, f, 0x105292cc, 0x8c, 0x64, 0x2f0); return; }
    c.set({{4, league.standings()}, {3, address}, {5, f.at(0x5a)}, {6, f.at(0x58)}, {7, 0}});
    const bool expanded = english_rules::expanded_season(c, address);
    if (expanded) {
        for (unsigned i = 0; i < english_rules::playoff_places.size(); ++i) {
            const auto place = english_rules::playoff_places[i];
            c.put32(std::uint32_t(clubs) + i * 4, c.u32(league.standings() + (place - 1) * 0x44));
            // Existing serialized seed byte: larger seeds receive home advantage.
            c.put8(f.at(0x60) + i, english_rules::seed_for_place(place));
        }
        c.set(4, 0);
    } else {
        // The seeded order is sixth, third, fifth, fourth, as in the original.
        c.put32(clubs, c.u32(c.r(4) + 5 * 0x44));
        c.set(4, 0);
        for (const auto [out, row] : {std::pair{4u, 2u}, {8, 4}, {12, 3}}) {
            c.set(8, league.standings()); c.put32(std::uint32_t(clubs) + out, c.u32(c.r(8) + row * 0x44));
        }
    }
    Kept<26> schedule(c, p.virtual_call(address, 0x44, 0x1052935c));
    Kept<25> playoff(c, p.call(0x10773520, 0x10529370, {0xb4}));
    if (compare(c, playoff) != 0) {
        for (const auto [offset, value] : {std::pair{0x38u, 0u}, {0x3c, 1}, {0x40, 160}, {0x44, 20},
                                        {0x48, 0}, {0x4c, 0}, {0x50, 0}, {0x54, 0}}) c.put32(f.at(offset), value);
        if (expanded) {
            // CupStage's ordered draw keeps the seeded pairs and their bracket.
            c.put32(f.at(0x48), 1);
            c.put32(f.at(0x54), f.at(0x60));
        }
        p.call(0x104bc3c0, 0x105293cc,
               {playoff, address, std::uint32_t(std::int32_t(c.i8(address + 0xc1))), clubs,
                extended(c.i16(f.at(0x5a))), league.definition(), schedule, extended(league.year())});
    }
    c.set({{4, league.stages()}, {3, clubs}}); c.put32(c.r(4), playoff);
    p.call(0x10775170, 0x105293dc);
    p.call(0x10775170, 0x105293e4, {schedule});
    c.set(3, c.u32(league.stages()));
    if (compare_logical(c, c.r(3)) == 0) { assertion(c, f, 0x10529408, 0x6c, 0x5c, 0x306); return; }
    p.call(0x1047ef50, 0x10529450, {c.r(3), 0});
}
}

#include "european_cup_detail.hpp"

namespace cm::aot::european_cup {
using namespace detail;
namespace {
void keep_register(Cpu& c, unsigned n, std::uint32_t value) { c.set(n, value); }
}

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x104e9fb0, 0xf0, {.first_gpr = 27});
    keep(c, {{27, year}, {28, definition}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x108), address);
    const Competition cup{c, address};
    Program p{c};
    c.call(0x10478f20, 0x104e9fe0);
    c.put32(address, c.toc(slot::vtable));
    cup.year() = std::int16_t(year);
    c.put8(address + 0xa8, 0); c.put8(address + 0x44, 0xff);
    cup.last_stage() = -1; cup.stage_count() = 13;
    cup.definition() = definition;
    c.put8(address + 0x42, 2);
    c.put32(address + 0x1c, 0xffffffff); c.put32(address + 0x20, 0xffffffff);
    c.put8(address + 0x4c, 3); c.put8(address + 0x46, 7); c.put8(address + 0x47, 3);
    c.put8(address + 0x4b, 10); c.put8(address + 0x4a, 1);
    // The stage count was stored just above, so the original's test for a
    // non-positive count always passes.
    compare(c, 13);
    c.set({{4, 10}, {5, 7}, {6, 3}, {7, 2}, {8, 13}, {9, 0xffffffff}, {10, 0}});
    const auto stages = p.call(0x10775130, 0x104ea094, {13u << 2});
    c.set(4, address);
    cup.stages() = stages;
    c.set({{3, address}, {4, 1}});
    cup.extra_rounds() = 0;
    c.put8(address + 0xb0, 0);
    if (compare(c, std::int32_t(p.virtual_call(address, 0x20, 0x104ea0cc))) != 0) return address;
    c.set(3, c.u32(c.toc(slot::reputation)));
    if (compare_logical(c, c.r(3)) != 0) { c.set(4, 0); c.call(0x104b3b60, 0x104ea0f8); }
    enter(c, 0x104ea100, {address});
    c.set(3, collect_entrants(c, address));
    enter(c, 0x104ea108, {address});
    c.set(3, collect_qualifiers(c, address));
    if (compare(c, std::int32_t(c.r(3))) == 0) { assertion(c, f, 0x104ea124, 0xb8, 0x50, 0x74); return address; }
    c.set(3, address);
    if (compare(c, std::int32_t(p.virtual_call(address, 0x94, 0x104ea178))) == 0) { assertion(c, f, 0x104ea198, 0x98, 0x48, 0x7b); return address; }
    c.set(3, address);
    if (compare(c, std::int32_t(p.virtual_call(address, 0x9c, 0x104ea1ec))) == 0) { assertion(c, f, 0x104ea20c, 0x78, 0x40, 0x82); return address; }
    const auto fixtures = p.call(0x10773520, 0x104ea258, {0x634});
    c.set(27, fixtures);
    if (compare(c, std::int32_t(fixtures)) != 0) { c.set(4, address); c.call(0x104533a0, 0x104ea268); }
    c.set(3, address);
    cup.fixtures() = c.r(27);
    if (compare_logical(c, cup.fixtures()) == 0) { assertion(c, f, 0x104ea294, 0x58, 0x38, 0x89); return address; }
    p.virtual_call(address, 0x64, 0x104ea2e4);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x104ea300, 0x50, {.first_gpr = 28});
    Kept<31> how(c, deleting);
    Kept<30> object(c, address);
    const Competition cup{c, address};
    Program p{c};
    if (compare(c, std::int32_t(address)) == 0) return address;
    c.put32(address, c.toc(slot::vtable));
    c.set(3, cup.qualifiers());
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x104ea340);
    c.set(3, cup.rounds());
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x104ea350);
    if (compare_logical(c, cup.group_tables()) != 0) {
        keep(c, {{28, 0}, {29, 0}});
        while (compare(c, std::int32_t(c.r(28)), cup.round_count()) < 0) {
            c.set(3, cup.group_tables());
            c.set(3, c.u32(c.r(3) + c.r(29)));
            if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x104ea37c);
            keep(c, {{29, c.r(29) + 4}, {28, c.r(28) + 1}});
        }
        c.set(3, cup.group_tables());
        c.call(0x10775170, 0x104ea398);
    }
    keep(c, {{28, 0}, {29, 0}});
    while (compare(c, std::int32_t(c.r(28)), cup.last_stage()) <= 0) {
        c.set(3, cup.stages());
        c.set(3, c.u32(c.r(3) + c.r(29)));
        if (compare_logical(c, c.r(3)) != 0) { c.set(4, 1); p.virtual_call(c.r(3), 8, 0x104ea3c8); }
        keep(c, {{29, c.r(29) + 4}, {28, c.r(28) + 1}});
    }
    c.set(3, cup.stages());
    if (compare_logical(c, c.r(3)) != 0) { c.call(0x10775170, 0x104ea3f0); cup.stages() = 0; }
    c.set(3, cup.fixtures());
    if (compare_logical(c, c.r(3)) != 0) { c.set(4, 1); c.call(0x104536c0, 0x104ea40c); }
    p.call(0x10478f70, 0x104ea418, {address, 0});
    if (compare(c, std::int16_t(std::uint32_t(how))) > 0) p.call(0x107735a0, 0x104ea428, {address});
    return address;
}

void award_result(Cpu& c, std::uint32_t address, std::uint32_t match, std::uint32_t extra) {
    Frame f(c, 0x104ea450, 0x50, {.first_gpr = 26});
    keep(c, {{30, match}, {27, c.toc(slot::finances)}, {28, c.toc(slot::reputation)}, {29, address}, {31, extra}, {26, 0}});
    const auto reputation = c.r(28), finances = c.r(27);
    const auto raised = [&](std::uint32_t club, std::uint32_t points, std::uint32_t back) {
        c.set({{5, points}, {3, c.u32(reputation)}, {4, c.u32(club + 0x2c)}});
        c.call(0x104b79a0, back);
    };
    const auto pay = [&](std::uint32_t club, std::uint32_t amount, std::uint32_t lookup, std::uint32_t add) {
        c.set({{3, c.u32(finances)}, {4, club}});
        c.call(0x10103510, lookup);
        c.set(4, amount);
        c.call(0x100f71f0, add);
    };
    const auto credit = [&](std::uint32_t club, std::uint32_t amount, std::uint32_t back) {
        c.set({{3, address}, {4, club}, {9, match}, {6, amount}, {5, 0}, {7, 0xffffffff}, {8, 0}, {10, 0xfffffffe}});
        c.call(0x10452010, back);
    };
    c.set(4, c.u8(match + 0x47));
    const auto second_home = signed_byte(c.r(4));
    std::uint32_t home_score, away_score;
    if (compare(c, std::int32_t(second_home), -1) != 0) { home_score = c.r(4); away_score = c.u8(match + 0x48); }
    else { home_score = c.u8(match + 0x43); away_score = c.u8(match + 0x44); }
    c.set(5, away_score);
    c.set(3, signed_byte(home_score));
    const auto order = compare(c, std::int8_t(home_score), std::int8_t(away_score));
    if (order > 0) {
        raised(c.u32(match + 0x1c), 2, 0x104ea4c0);
        c.set(26, c.u32(match + 0x1c));
    } else if (order < 0) {
        raised(c.u32(match + 0x20), 2, 0x104ea4e0);
        c.set(26, c.u32(match + 0x20));
    } else {
        raised(c.u32(match + 0x1c), 1, 0x104ea4fc);
        raised(c.u32(match + 0x20), 1, 0x104ea510);
    }
    const auto stage = std::int32_t(std::int8_t(c.u8(match + 0x42)));
    if (compare(c, stage, -1) > 0 && compare(c, stage, 12) < 0) {
        if (compare_logical(c, c.r(26)) != 0) {
            pay(c.r(26), 200000, 0x104ea53c, 0x104ea548);
            credit(c.r(26), 200000, 0x104ea570);
        } else {
            pay(c.u32(match + 0x1c), 100000, 0x104ea580, 0x104ea58c);
            pay(c.u32(match + 0x20), 100000, 0x104ea598, 0x104ea5a4);
            credit(c.u32(match + 0x1c), 100000, 0x104ea5cc);
            credit(c.u32(match + 0x20), 100000, 0x104ea5f4);
        }
    }
    c.set({{3, address}, {4, match}, {5, extra}});
    c.call(0x1047bfb0, 0x104ea604);
}

std::uint32_t collect_qualifiers(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104eb770, 0x70, {.first_gpr = 29});
    keep(c, {{31, address}, {30, c.toc(slot::pool)}, {29, c.toc(slot::message)}});
    const Competition cup{c, address};
    Program p{c};
    cup.qualifier_count() = 0x37;
    const auto list = p.call(0x10775130, 0x104eb7a8, {std::uint32_t(0x37) << 3});
    compare_logical(c, list);
    cup.qualifiers() = list;
    if (!list) {
        assertion(c, f, 0x104eb7c8, 0x40, 0x38, 0x2f6);
        c.set(4, c.toc(slot::flag));
        return 0xffffffff;
    }
    c.set({{4, 0}, {3, 0}});
    while (compare(c, std::int32_t(c.r(4)), cup.qualifier_count()) < 0) {
        const auto i = c.r(4), at = c.r(3);
        // The list holds them in reverse. With no entrants the club is null,
        // but the route is still read, before the table's start, as in the original.
        const auto count = std::uint32_t(std::int32_t(cup.entrant_count()));
        const auto q = cup.qualifiers();
        if (compare(c, std::int16_t(count)) != 0) {
            c.put32(q + at, c.u32(cup.entrants() + ((count - 1 - i) << 3)));
        } else c.put32(q + at, 0);
        const auto route = std::int32_t(c.i8(cup.entrants() + ((count - 1 - i) << 3) + 4));
        const std::uint32_t tier = compare(c, route, 8) == 0 ? 2 : compare(c, route, 13) == 0 ? 1 : 0;
        c.put8(q + at + 4, tier);
        c.put8(q + at + 5, 0);
        c.set({{5, q}, {6, 0}, {4, i + 1}, {3, at + 8}});
    }
    return 1;
}

void champion(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104eb900, 0x40, {});
    Program p{c};
    const auto stage = c.u32(c.u32(address + 0xc) + 0x30);
    c.set(3, stage);
    p.virtual_call(stage, 0x38, 0x104eb920);
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ebc00, 0x100, {.first_gpr = 25});
    keep(c, {{28, address}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const Competition cup{c, address};
    Program p{c};
    c.put8(address + 0x49, 0);
    c.set(3, cup.qualifiers());
    if (compare_logical(c, c.r(3)) != 0) { c.call(0x10775170, 0x104ebc38); cup.qualifiers() = 0; }
    c.set(3, cup.entrants());
    if (compare_logical(c, c.r(3)) != 0) { c.call(0x10775170, 0x104ebc50); cup.entrants() = 0; }
    c.set(3, cup.rounds());
    if (compare_logical(c, c.r(3)) != 0) { c.call(0x10775170, 0x104ebc68); cup.rounds() = 0; }
    if (compare_logical(c, cup.group_tables()) != 0) {
        keep(c, {{26, 0}, {25, 0}, {27, 0}});
        while (compare(c, std::int32_t(c.r(25)), cup.round_count()) < 0) {
            c.set(3, cup.group_tables());
            c.set(3, c.u32(c.r(3) + c.r(26)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.call(0x10775170, 0x104ebca0);
                c.set(3, cup.group_tables());
                c.put32(c.r(3) + c.r(26), 0);
            }
            keep(c, {{26, c.r(26) + 4}, {25, c.r(25) + 1}});
        }
        c.set(3, cup.group_tables());
        c.call(0x10775170, 0x104ebcc4);
        cup.group_tables() = 0;
    }
    keep(c, {{25, 0}, {26, 0}});
    while (compare(c, std::int32_t(c.r(25)), cup.last_stage()) <= 0) {
        c.set(3, cup.stages());
        c.set(3, c.u32(c.r(3) + c.r(26)));
        if (compare_logical(c, c.r(3)) != 0) { c.set(4, 1); p.virtual_call(c.r(3), 8, 0x104ebcfc); }
        keep(c, {{26, c.r(26) + 4}, {25, c.r(25) + 1}});
    }
    cup.last_stage() = -1;
    c.set(3, cup.fixtures());
    if (compare_logical(c, c.r(3)) != 0) { c.set(4, 1); c.call(0x104564a0, 0x104ebd30); }
    const auto year = std::uint32_t(std::int32_t(cup.year())) + 1;
    c.set(4, year);
    cup.year() = std::int16_t(year);
    c.put8(address + 0xa8, 0); c.put8(address + 0xb0, 0);
    c.set(3, c.u32(c.toc(slot::reputation)));
    if (compare_logical(c, c.r(3)) != 0) { c.set(4, 0); c.call(0x104b3b60, 0x104ebd60); }
    enter(c, 0x104ebd68, {address});
    c.set(3, collect_entrants(c, address));
    if (compare(c, std::int32_t(c.r(3))) == 0) { assertion(c, f, 0x104ebd84, 0xb8, 0x50, 0x40c); return; }
    enter(c, 0x104ebdcc, {address});
    c.set(3, collect_qualifiers(c, address));
    if (compare(c, std::int32_t(c.r(3))) < 0) { assertion(c, f, 0x104ebde8, 0x98, 0x48, 0x413); return; }
    c.set(3, address);
    if (compare(c, std::int32_t(p.virtual_call(address, 0x94, 0x104ebe38))) < 0) { assertion(c, f, 0x104ebe58, 0x78, 0x40, 0x41a); return; }
    c.set(3, address);
    if (compare(c, std::int32_t(p.virtual_call(address, 0x9c, 0x104ebea8))) < 0) { assertion(c, f, 0x104ebec8, 0x58, 0x38, 0x421); return; }
    c.set(3, address);
    p.virtual_call(address, 0x64, 0x104ebf18);
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ebf30, 0x40, {.first_gpr = 31});
    Kept<31> object(c, address);
    const Competition cup{c, address};
    Program p{c};
    c.set({{4, 0xffffffff}, {5, address + 0x3c}, {6, address + 0x3a}, {7, 0}});
    cup.rounds() = p.virtual_call(address, 0x44, 0x104ebf60);
    return 1;
}

std::uint32_t collect_entrants(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ebf80, 0xa0, {.first_gpr = 19});
    keep(c, {{27, address}, {21, c.toc(slot::club_total)}, {22, c.toc(slot::this_competition)}, {31, 0},
             {19, c.toc(slot::message)}, {23, 0}, {20, c.toc(slot::pool)}, {30, 0}, {24, 0}, {29, 0}, {25, 0}, {28, 0}, {26, 0}});
    const Competition cup{c, address};
    Program p{c};
    cup.entrant_count() = 0x47; cup.entrant_scan() = 0x47;
    c.set(3, cup.entrants());
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x104ebfe0);
    const auto list = p.call(0x10775130, 0x104ebfec, {std::uint32_t(std::int32_t(cup.entrant_count())) << 3});
    cup.entrants() = list;
    if (compare_logical(c, cup.entrants()) == 0) {
        assertion(c, f, 0x104ec010, 0x40, 0x38, 0x44e);
        c.set(4, c.toc(slot::flag));
        return 0;
    }
    // Four tier bands in the list, each with its capacity, so a band can't
    // spill into the next. Each entry holds the club, an entry route and a zero byte.
    c.set({{5, c.toc(slot::clubs)}});
    auto club = c.u32(c.r(5));
    c.set({{3, 0}, {4, 0}, {8, 0}, {7, club}, {5, 0}, {6, 0}});
    const auto total = c.r(21), wanted = c.r(22);
    // One band: its filled count in a register, its write offset in another,
    // and the route byte. Later clubs of a full band are skipped.
    const auto enter_band = [&](unsigned filled_register, unsigned offset_register, unsigned shadow_register, unsigned next_register,
                                std::int32_t capacity, std::uint32_t band, std::uint32_t route, std::uint32_t left_in_r10) {
        if (compare(c, std::int32_t(c.r(filled_register)), capacity) >= 0) return;
        const auto at = c.r(offset_register);
        c.put32(cup.entrants() + band + at, club);
        c.put8(cup.entrants() + band + at + 5, 0);
        c.put8(cup.entrants() + band + at + 4, route);
        keep_register(c, filled_register, c.r(filled_register) + 1);
        keep_register(c, shadow_register, c.r(shadow_register) + 8);
        c.set({{9, cup.entrants()}, {10, left_in_r10}, {next_register, at + 8}});
    };
    for (;;) {
        c.set(9, extended_half(c.r(8)));
        if (compare(c, std::int32_t(c.r(9)), c.i32(total)) >= 0) break;
        const Club candidate{c, club};
        const std::int32_t home = candidate.competition();
        c.set(9, std::uint32_t(home));
        if (compare(c, home, c.i32(wanted)) == 0) {
            const std::int32_t tier = candidate.tier();
            if (compare(c, tier, 1) == 0) enter_band(28, 3, 26, 3, 16, 0, 7, 7);
            else if (compare(c, tier, 2) == 0) enter_band(29, 4, 25, 4, 18, 0x80, 8, 8);
            else if (compare(c, tier, 3) == 0) enter_band(30, 5, 24, 5, 19, 0x110, 13, 13);
            else if (compare(c, tier, 4) == 0) enter_band(31, 6, 23, 6, 18, 0x1a8, 0, cup.entrants());
        }
        c.set(8, c.r(8) + 1);
        club += club_size;
        c.set(7, club);
    }
    const auto expected = c.r(28) + c.r(29) + c.r(30) + c.r(31);
    c.set(3, std::uint32_t(std::int32_t(cup.entrant_count())));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(expected)) != 0) {
        cup.entrant_count() = 0;
        return 0;
    }
    return 1;
}
}

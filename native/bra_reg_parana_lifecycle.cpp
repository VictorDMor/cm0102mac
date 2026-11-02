#include "bra_reg_parana_detail.hpp"
namespace cm::aot::bra_reg_parana {
using namespace detail;
std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    observe_entry(c, 0x106f6dc0);
    Frame f(c, 0x106f6dc0, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting);
    Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x106f6df8, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x106f6e08); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // The original uses an inclusive last index, reloaded after callbacks.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106f6e40);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        // Unlike the schedule, the stage array is released even when null and keeps its pointer.
        p.call(0x10775170, 0x106f6e60, {league.stages()});
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x106f6e74, {c.r(3), 1});
        p.call(0x1049e600, 0x106f6e80, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x106f6e90, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f7bb0);
    Frame f(c, 0x106f7bb0, 0x80, {.first_gpr = 26});
    Kept<30> object(c, address);
    Kept<26> total(c, c.toc(slot::club_total));
    Kept<27> division_id(c, c.toc(slot::regional));
    Kept<31> count(c, 0);
    Kept<28> index(c, c.toc(slot::message));
    Kept<29> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 12;
    p.call(0x10775130, 0x106f7bec, {std::uint32_t(std::int32_t(league.club_count())) * standing_size});
    const auto allocated = c.r(3);
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x106f7c0c, 0x40, 0x38, 0x169);
        c.set(4, c.toc(slot::flag)); return 0;
    }
    index = 0; club = c.u32(c.toc(slot::clubs));
    while (compare(c, std::int16_t(std::uint32_t(index)), c.i32(total)) < 0) {
        const auto division = c.u32(std::uint32_t(club) + 0x38);
        if (compare_logical(c, division) != 0 && compare(c, c.i32(division), c.i32(division_id)) == 0) {
            c.set({{4, count}, {3, object}, {5, club}, {6, 0}, {7, 0}});
            count = std::uint32_t(count) + 1;
            c.call(0x104a60e0, 0x106f7c9c);
            if (compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0) break;
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + club_size;
    }
    return compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0;
}

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    observe_entry(c, 0x106f6a80);
    Frame f(c, 0x106f6a80, 0x120, {.first_gpr = 27});
    Kept<27> fixtures(c, year);
    keep(c, {{28, definition}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x106f6ab0, {address});
    c.set({{5, c.toc(slot::vtable)}, {3, address}, {4, 1}});
    c.put32(address, c.r(5));
    league.year() = std::int16_t(year);
    league.definition() = definition;
    c.put8(address + 0x4a, 7);
    if (compare(c, p.virtual_call(address, 0x20, 0x106f6aec)) != 0) return address;
    c.set({{4, missing}, {5, 15}});
    c.put8(address + 0x4b, 15); c.put8(address + 0x44, missing);
    league.last_stage() = -1; league.stage_count() = 3;
    // The original retests the count it just stored; its no-stage arm is dead.
    compare(c, league.stage_count());
    p.call(0x10775130, 0x106f6b44, {12});
    c.set(4, address); league.stages() = c.r(3);
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x106f6b68)) == 0) {
        assertion(c, f, 0x106f6b88, 0xe0, 0x58, 0x4e); return address;
    }
    c.set(3, address); p.owned(0x106f7bb0, 0x106f6bd4, [&] { result(c, collect_clubs(c, address)); });
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x106f6bf0, 0xc0, 0x50, 0x55); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x106f6c3c, {address})) == 0) {
        assertion(c, f, 0x106f6c58, 0xa0, 0x48, 0x5b); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x106f6ca8, {address, 0})) == 0) {
        assertion(c, f, 0x106f6cc4, 0x80, 0x40, 0x62); return address;
    }
    fixtures = p.call(0x10773520, 0x106f6d10, {0x634});
    if (compare(c, fixtures) != 0) p.call(0x104533a0, 0x106f6d20, {fixtures, address});
    league.fixtures() = fixtures;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106f6d4c, 0x60, 0x38, 0x69); return address;
    }
    c.set(3, address); p.virtual_call(address, 0x64, 0x106f6d9c);
    return address;
}


void base_forward(Cpu& c) {
    observe_entry(c, 0x106f8d10);
    Frame f(c, 0x106f8d10, 0x40, {}); c.call(0x104aa430, 0x106f8d20);
}
}

namespace cm::aot::bra_reg_parana {
std::uint32_t configure(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f7d00);
    Frame f(c, 0x106f7d00, 0x40, {.first_gpr = 30});
    keep(c, {{31, 3}, {30, address}});
    Program p{c};
    c.set({{4, missing}, {5, address + 0xa6}, {6, address + 0x3a}, {7, 0}, {8, 7}, {9, missing}, {10, 4}});
    c.put16(address + 0x3c, 2);
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2}, {0x42, 1}, {0xc7, 3},
            {0xc8, 1}, {0xc9, 0}, {0xc0, 0}, {0xc1, 0}, {0xc2, 0}, {0xc3, 4}, {0x46, 7}, {0x47, 3}})
        c.put8(address + offset, value);
    c.put16(address + 0xa8, 0); c.put16(address + 0xde, 0); c.put16(address + 0xdc, 8);
    c.put32(address + 0x1c, missing); c.put32(address + 0x20, missing);
    const auto schedule = p.virtual_call(address, 0x44, 0x106f7da0);
    c.put32(address + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f7dd0);
    Frame f(c, 0x106f7dd0, 0xf0, {.first_gpr = 26});
    Kept<28> object(c, address);
    keep(c, {{29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x106f7e00, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x106f7e10); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x106f7e2c, {c.r(3), 1});
    Kept<26> index(c, 0); Kept<27> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106f7e5c);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    p.call(0x10775170, 0x106f7e7c, {league.stages()});
    league.stage_count() = 3; compare(c, league.stage_count());
    league.stages() = p.call(0x10775130, 0x106f7e98, {12});
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x106f7ecc)) == 0) {
        assertion(c, f, 0x106f7eec, 0xb8, 0x50, 0x1d4); return;
    }
    c.set(3, object); p.owned(0x106f7bb0, 0x106f7f34, [&] { result(c, collect_clubs(c, object)); });
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x106f7f50, 0x98, 0x48, 0x1db); return; }
    if (compare(c, p.call(0x104a00a0, 0x106f7f98, {object})) == 0) {
        assertion(c, f, 0x106f7fb4, 0x78, 0x40, 0x1e1); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x106f8000, {object, 0})) == 0) {
        assertion(c, f, 0x106f801c, 0x58, 0x38, 0x1e8); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x106f806c);
    c.set(4, league.definition());
    p.call(0x102d46c0, 0x106f8080, {c.u32(c.toc(slot::schedule_events))});
}
}

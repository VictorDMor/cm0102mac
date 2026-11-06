#include "bra_reg_gaucho_detail.hpp"
namespace cm::aot::bra_reg_gaucho {
using namespace detail;
std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    observe_entry(c, 0x106f1100);
    Frame f(c, 0x106f1100, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting);
    Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x106f1138, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x106f1148); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // The original uses an inclusive last index, reloaded after callbacks.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106f1180);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        // Unlike the schedule, the stage array is released even when null and keeps its pointer.
        p.call(0x10775170, 0x106f11a0, {league.stages()});
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x106f11b4, {c.r(3), 1});
        p.call(0x1049e600, 0x106f11c0, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x106f11d0, {object});
    }
    return object;
}

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    observe_entry(c, 0x106f0dc0);
    Frame f(c, 0x106f0dc0, 0x120, {.first_gpr = 27});
    Kept<27> fixtures(c, year);
    keep(c, {{28, definition}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x106f0df0, {address});
    c.set({{5, c.toc(slot::vtable)}, {3, address}, {4, 1}});
    c.put32(address, c.r(5));
    league.year() = std::int16_t(year);
    league.definition() = definition;
    c.put8(address + 0x4a, 7);
    if (compare(c, p.virtual_call(address, 0x20, 0x106f0e2c)) != 0) return address;
    c.set({{4, missing}, {5, 15}});
    c.put8(address + 0x4b, 15); c.put8(address + 0x44, missing);
    league.last_stage() = -1; league.stage_count() = 1;
    // The original retests the count it just stored; its no-stage arm is dead.
    compare(c, league.stage_count());
    p.call(0x10775130, 0x106f0e84, {4});
    c.set(4, address); league.stages() = c.r(3);
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x106f0ea8)) == 0) {
        assertion(c, f, 0x106f0ec8, 0xe0, 0x58, 0x4d); return address;
    }
    c.set(3, address); p.owned(0x106f1d60, 0x106f0f14, [&] { result(c, collect_clubs(c, address)); });
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x106f0f30, 0xc0, 0x50, 0x54); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x106f0f7c, {address})) == 0) {
        assertion(c, f, 0x106f0f98, 0xa0, 0x48, 0x5b); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x106f0fe8, {address, 0})) == 0) {
        assertion(c, f, 0x106f1004, 0x80, 0x40, 0x64); return address;
    }
    fixtures = p.call(0x10773520, 0x106f1050, {0x634});
    if (compare(c, fixtures) != 0) p.call(0x104533a0, 0x106f1060, {fixtures, address});
    league.fixtures() = fixtures;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106f108c, 0x60, 0x38, 0x6b); return address;
    }
    c.set(3, address); p.virtual_call(address, 0x64, 0x106f10dc);
    return address;
}


void base_forward(Cpu& c) {
    observe_entry(c, 0x106f2ac0);
    Frame f(c, 0x106f2ac0, 0x40, {}); c.call(0x104aa430, 0x106f2ad0);
}
}

namespace cm::aot::bra_reg_gaucho {
std::uint32_t configure(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f1eb0);
    Frame f(c, 0x106f1eb0, 0x50, {.first_gpr = 29});
    keep(c, {{31, 3}, {30, 1}, {29, address}});
    c.set({{4, missing}, {5, address + 0xa6}, {6, address + 0x3a}, {7, 0}, {8, 7}, {9, missing}, {10, 4}});
    c.put16(address + 0x3c, 1);
    c.put8(address + 0xc4, 3); c.put8(address + 0xc5, 1); c.put8(address + 0xc6, 2);
    c.put16(address + 0xa8, 0); c.put16(address + 0xde, 0);
    for (const auto [offset, value] : {std::pair{0x42u, 1u}, {0xc7, 3}, {0xc8, 1}, {0xc9, 0},
            {0xc0, 4}, {0xc1, 0}, {0xc2, 0}, {0xc3, 2}})
        c.put8(address + offset, value);
    c.put32(address + 0x1c, missing); c.put32(address + 0x20, missing);
    c.put8(address + 0x46, 7); c.put8(address + 0x47, 3); c.put16(address + 0xdc, 8);
    const auto schedule = Program{c}.virtual_call(address, 0x44, 0x106f1f54);
    c.put32(address + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f1f80);
    Frame f(c, 0x106f1f80, 0xf0, {.first_gpr = 26});
    Kept<26> object(c, address);
    keep(c, {{31, c.toc(slot::flag)}, {29, c.toc(slot::message)}, {30, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x106f1fb0, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x106f1fc0); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x106f1fdc, {c.r(3), 1});
    Kept<27> index(c, 0); Kept<28> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106f200c);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    // The Gaúcho season reuses the existing stage pointer array.
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x106f2048)) == 0) {
        assertion(c, f, 0x106f2068, 0xb8, 0x50, 0x1c6); return;
    }
    c.set(3, object); p.owned(0x106f1d60, 0x106f20b0, [&] { result(c, collect_clubs(c, object)); });
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x106f20cc, 0x98, 0x48, 0x1cd); return; }
    if (compare(c, p.call(0x104a00a0, 0x106f2114, {object})) == 0) {
        assertion(c, f, 0x106f2130, 0x78, 0x40, 0x1d3); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x106f217c, {object, 0})) == 0) {
        assertion(c, f, 0x106f2198, 0x58, 0x38, 0x1da); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x106f21e8);
    c.set(4, league.definition());
    p.call(0x102d46c0, 0x106f21fc, {c.u32(c.toc(slot::schedule_events))});
}
}

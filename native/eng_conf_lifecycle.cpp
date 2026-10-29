#include "eng_conf_detail.hpp"

namespace cm::aot::eng_conf {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x10524600, 0x120, {.first_gpr = 27});
    Kept<27> saved_year(c, year);
    Kept<28> saved_definition(c, definition);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x10524630, {address});
    c.set({{5, c.toc(slot::vtable)}, {3, address}, {4, 1}});
    c.put32(address, c.r(5));
    league.year() = std::int16_t(std::uint32_t(saved_year));
    league.definition() = saved_definition;
    c.put8(address + 0x4a, 9);
    if (compare(c, p.virtual_call(address, 0x20, 0x1052466c)) != 0) return address;
    c.set({{3, address}, {5, 600}, {4, missing}});
    c.put16(address + 0xe2, 1000); c.put16(address + 0xe4, 600);
    c.put8(address + 0x44, missing); league.last_stage() = -1;
    c.put32(address + 0x2c, 0);
    c.set({{4, address}, {3, c.u32(address + 0x2c)}});
    // The original rereads its just-cleared count before choosing stage storage.
    if (compare(c, c.r(3)) > 0) {
        p.call(0x10775130, 0x105246d0, {c.r(3) * 4});
        c.set(4, address); league.stages() = c.r(3);
    } else league.stages() = 0;
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x105246f0)) == 0) {
        assertion(c, f, 0x10524710, 0xe0, 0x58, 0x4c); return address;
    }
    enter(c, 0x1052475c, {address}); result(c, collect_clubs(c, address));
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x10524778, 0xc0, 0x50, 0x53); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x105247c4, {address})) == 0) {
        assertion(c, f, 0x105247e0, 0xa0, 0x48, 0x5a); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x10524830, {address, 0})) == 0) {
        assertion(c, f, 0x1052484c, 0x80, 0x40, 0x63); return address;
    }
    saved_year = p.call(0x10773520, 0x10524898, {0x634});
    if (compare(c, saved_year) != 0) p.call(0x104533a0, 0x105248a8, {saved_year, address});
    c.set(3, address); league.fixtures() = saved_year;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x105248d4, 0x60, 0x38, 0x6b); return address;
    }
    p.virtual_call(address, 0x64, 0x10524924);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x10524940, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting);
    Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x10524978, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x10524988); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // The original uses an inclusive last index, reloaded after callbacks.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x105249c0);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        c.set(3, league.stages());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x105249e8); league.stages() = 0;
        }
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x10524a04, {c.r(3), 1});
        p.call(0x1049e600, 0x10524a10, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x10524a20, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x105262f0, 0x80, {.first_gpr = 26});
    Kept<30> object(c, address);
    Kept<26> total(c, c.toc(slot::club_count));
    Kept<27> division_id(c, c.toc(slot::conference));
    Kept<31> count(c, 0);
    Kept<28> index(c, c.toc(-0x7ba8));
    Kept<29> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 22;
    p.call(0x10775130, 0x1052632c, {std::uint32_t(std::int32_t(league.club_count())) * 0x44});
    const auto allocated = c.r(3);
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x1052634c, 0x40, 0x38, 0x1f3);
        c.set(4, c.toc(-0x7bac)); return 0;
    }
    c.set(3, c.toc(slot::clubs)); index = 0; club = c.u32(c.r(3));
    while (true) {
        c.set(3, extended(std::int16_t(std::uint32_t(index))));
        if (compare(c, c.r(3), c.i32(total)) >= 0) break;
        c.set(3, c.u32(std::uint32_t(club) + 0x30));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(3, c.u32(c.r(3)));
            if (compare(c, c.r(3), c.i32(division_id)) == 0) {
                c.set({{4, count}, {3, object}, {5, club}, {6, 0}, {7, 0}});
                count = std::uint32_t(count) + 1;
                p.call(0x104a60e0, 0x105263dc);
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
                if (compare(c, c.r(3), league.club_count()) == 0) break;
            }
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + 0x248;
    }
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
    if (compare(c, c.r(3), league.club_count()) != 0) return 0;
    c.put32(address + 0x28, p.call(0x1049f540, 0x10526428, {object, 5000}));
    c.put8(address + 0xe1, 1);
    p.call(0x1049f6d0, 0x10526448, {object, 100000, 0});
    return 1;
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10526460, 0x40, {.first_gpr = 30});
    Kept<31> two(c, 2); Kept<30> object(c, address); Program p{c};
    c.set({{10, 0}, {8, missing}, {9, c.toc(slot::third)},
           {5, address + 0xa6}, {6, address + 0x3a}, {4, missing}, {7, 0}});
    c.put16(address + 0x3c, two);
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2},
             {0x42, 1}, {0xc7, 1}, {0xc8, 2}, {0xc9, 3}, {0xc0, 1}, {0xc1, 0}, {0xc2, 0}, {0xc3, 3}})
        c.put8(address + offset, value);
    c.set(9, c.u32(c.r(9)));
    c.put32(address + 0x1c, c.r(9)); c.put32(address + 0x20, missing);
    c.put8(address + 0x46, 5); c.put8(address + 0x47, 3); c.put8(address + 0xc7, 1);
    const auto schedule = p.virtual_call(address, 0x44, 0x105264f8);
    c.put32(std::uint32_t(object) + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10526520, 0xf0, {.first_gpr = 26});
    Kept<26> object(c, address);
    keep(c, {{31, c.toc(-0x7bac)}, {29, c.toc(-0x7ba8)}, {30, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x10526550, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x10526560); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x1052657c, {c.r(3), 1});
    Kept<27> index(c, 0); Kept<28> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x105265ac);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x105265e8)) == 0) {
        assertion(c, f, 0x10526608, 0xb8, 0x50, 0x259); return;
    }
    enter(c, 0x10526650, {object}); result(c, collect_clubs(c, object));
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x1052666c, 0x98, 0x48, 0x260); return; }
    if (compare(c, p.call(0x104a00a0, 0x105266b4, {object})) == 0) {
        assertion(c, f, 0x105266d0, 0x78, 0x40, 0x266); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x1052671c, {object, 0})) == 0) {
        assertion(c, f, 0x10526738, 0x58, 0x38, 0x26d); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x10526788);
    p.call(0x104aa3a0, 0x10526794, {object});
    c.set({{3, c.toc(-0x7428)}, {4, league.definition()}});
    p.call(0x102d46c0, 0x105267a4, {c.u32(c.r(3))});
}
}

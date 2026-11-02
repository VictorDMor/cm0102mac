#include "eng_first_detail.hpp"

namespace cm::aot::eng_first {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x105267c0, 0x120, {.first_gpr = 27});
    Kept<27> saved_year(c, year);
    Kept<28> saved_definition(c, definition);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x105267f0, {address});
    c.set({{5, c.toc(slot::vtable)}, {3, address}, {4, 1}});
    c.put32(address, c.r(5));
    league.year() = std::int16_t(std::uint32_t(saved_year));
    league.definition() = saved_definition;
    c.put8(address + 0x4a, 9);
    if (compare(c, p.virtual_call(address, 0x20, 0x1052682c)) != 0) return address;
    c.set({{3, address}, {4, missing}});
    c.put8(address + 0x44, missing); league.last_stage() = -1;
    c.put32(address + 0x2c, 1);
    // This count is reloaded, as in the original, after the observable write.
    const auto stage_count = c.i32(address + 0x2c);
    if (compare(c, stage_count) > 0) {
        p.call(0x10775130, 0x10526878, {std::uint32_t(stage_count) * 4});
        c.set(4, address); league.stages() = c.r(3);
    } else league.stages() = 0;
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x1052689c)) == 0) {
        assertion(c, f, 0x105268bc, 0xe0, 0x58, 0x49); return address;
    }
    c.set(3, address); call(c, 0x10528c60, 0x10526908);
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x10526924, 0xc0, 0x50, 0x50); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x10526970, {address})) == 0) {
        assertion(c, f, 0x1052698c, 0xa0, 0x48, 0x57); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x105269dc, {address, 0})) == 0) {
        assertion(c, f, 0x105269f8, 0x80, 0x40, 0x60); return address;
    }
    saved_year = p.call(0x10773520, 0x10526a44, {0x634});
    if (compare(c, saved_year) != 0) p.call(0x104533a0, 0x10526a54, {saved_year, address});
    c.set(3, address); league.fixtures() = saved_year;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x10526a80, 0x60, 0x38, 0x68); return address;
    }
    p.virtual_call(address, 0x64, 0x10526ad0);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x10526af0, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting); Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x10526b28, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x10526b38); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // Virtual destructors may replace the array or change its inclusive end.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x10526b70);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        c.set(3, league.stages());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x10526b98); league.stages() = 0;
        }
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x10526bb4, {c.r(3), 1});
        p.call(0x1049e600, 0x10526bc0, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x10526bd0, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10528c60, 0x80, {.first_gpr = 25});
    Kept<29> object(c, address); Kept<31> records(c, c.toc(slot::records));
    Kept<25> total(c, c.toc(slot::club_count)); Kept<30> count(c, 0);
    Kept<26> division_id(c, c.toc(slot::first));
    Kept<27> index(c, c.toc(-0x7ba8)); Kept<28> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 24;
    const auto allocated = p.call(0x10775130, 0x10528ca0, {std::uint32_t(std::int32_t(league.club_count())) * 0x44});
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x10528cc0, 0x40, 0x38, 0x22d);
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
                p.call(0x104a60e0, 0x10528d54);
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
                if (compare(c, c.r(3), league.club_count()) == 0) break;
            }
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + 0x248;
    }
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
    if (compare(c, c.r(3), league.club_count()) != 0) return 0;
    c.put32(address + 0x28, p.call(0x1049f540, 0x10528da4, {object, 70000}));
    c.set(3, c.toc(slot::date)); c.put8(address + 0xe1, 1);
    if (compare_logical(c, c.u16(c.r(3) + 2), 1999) > 0) {
        const auto record_for = [&](int slot_id) { return c.u32(records) + c.u32(c.toc(slot_id)) * 0x6c; };
        c.set(6, 0x5a0000);
        p.call(0x1049f6d0, 0x10528de4, {object, 0x5a0000u - 0x4fef, record_for(slot::premier)});
        p.call(0x1049f6d0, 0x10528df8, {object, 4100000, league.definition()});
        c.set(6, 0x3f0000);
        p.call(0x1049f6d0, 0x10528e1c, {object, 4100000, record_for(slot::second)});
    } else p.call(0x1049f6d0, 0x10528e34, {object, 1000000, 0});
    return 1;
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10528e50, 0x50, {.first_gpr = 29});
    Kept<31> three(c, 3); Kept<30> two(c, 2); Kept<29> object(c, address);
    Program p{c};
    c.set({{10, 0}, {9, c.toc(slot::premier)}, {8, c.toc(slot::second)},
           {5, address + 0xa6}, {6, address + 0x3a}, {4, missing}, {7, 0}});
    c.put16(address + 0x3c, two);
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2},
             {0x42, 1}, {0xc7, 1}, {0xc8, 2}, {0xc9, 3}, {0xc0, 2}, {0xc1, 4}, {0xc2, 0}, {0xc3, 3}})
        c.put8(address + offset, value);
    c.set(9, c.u32(c.r(9))); c.put32(address + 0x1c, c.r(9));
    c.set(8, c.u32(c.r(8))); c.put32(address + 0x20, c.r(8));
    c.put8(address + 0x46, 5); c.put8(address + 0x47, three); c.put8(address + 0x4c, two);
    const auto schedule = p.virtual_call(address, 0x44, 0x10528ef4);
    c.put32(std::uint32_t(object) + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10528f20, 0xf0, {.first_gpr = 26});
    Kept<26> object(c, address);
    keep(c, {{31, c.toc(-0x7bac)}, {29, c.toc(-0x7ba8)}, {30, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x10528f50, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x10528f60); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x10528f7c, {c.r(3), 1});
    Kept<27> index(c, 0); Kept<28> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x10528fac);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x10528fe8)) == 0) {
        assertion(c, f, 0x10529008, 0xb8, 0x50, 0x29e); return;
    }
    c.set(3, object); call(c, 0x10528c60, 0x10529050);
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x1052906c, 0x98, 0x48, 0x2a5); return; }
    if (compare(c, p.call(0x104a00a0, 0x105290b4, {object})) == 0) {
        assertion(c, f, 0x105290d0, 0x78, 0x40, 0x2ab); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x1052911c, {object, 0})) == 0) {
        assertion(c, f, 0x10529138, 0x58, 0x38, 0x2b2); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x10529188);
    p.call(0x104aa3a0, 0x10529194, {object});
    p.call(0x102d46c0, 0x105291a4, {c.u32(c.toc(-0x7428)), league.definition()});
}
}

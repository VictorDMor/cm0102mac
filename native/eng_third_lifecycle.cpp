#include "eng_third_detail.hpp"

namespace cm::aot::eng_third {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x1052f300, 0x120, {.first_gpr = 27});
    Kept<27> saved_year(c, year);
    Kept<28> saved_definition(c, definition);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x1052f330, {address});
    c.set({{7, c.toc(slot::vtable)}, {6, 0x1770}, {3, address}, {5, 0x3e8}, {4, 1}});
    c.put32(address, c.r(7));
    league.year() = std::int16_t(std::uint32_t(saved_year));
    league.definition() = saved_definition;
    // Third Division only: the two halfword limits at +0xe2 and +0xe4.
    c.put16(address + 0xe2, c.r(6)); c.put16(address + 0xe4, c.r(5));
    c.put8(address + 0x4a, 9);
    if (compare(c, p.virtual_call(address, 0x20, 0x1052f384)) != 0) return address;
    c.set({{3, address}, {4, missing}});
    c.put8(address + 0x44, missing); league.last_stage() = -1;
    c.put32(address + 0x2c, 1);
    // This count is reloaded, as in the original, after the observable write.
    const auto stage_count = c.i32(address + 0x2c);
    if (compare(c, stage_count) > 0) {
        p.call(0x10775130, 0x1052f3d0, {std::uint32_t(stage_count) * 4});
        c.set(4, address); league.stages() = c.r(3);
    } else league.stages() = 0;
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x1052f3f4)) == 0) {
        assertion(c, f, 0x1052f414, 0xe0, 0x58, 0x4e); return address;
    }
    c.set(3, address); call(c, 0x105310c0, 0x1052f460);
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x1052f47c, 0xc0, 0x50, 0x55); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x1052f4c8, {address})) == 0) {
        assertion(c, f, 0x1052f4e4, 0xa0, 0x48, 0x5c); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x1052f534, {address, 0})) == 0) {
        assertion(c, f, 0x1052f550, 0x80, 0x40, 0x65); return address;
    }
    saved_year = p.call(0x10773520, 0x1052f59c, {0x634});
    if (compare(c, saved_year) != 0) p.call(0x104533a0, 0x1052f5ac, {saved_year, address});
    c.set(3, address); league.fixtures() = saved_year;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x1052f5d8, 0x60, 0x38, 0x6d); return address;
    }
    p.virtual_call(address, 0x64, 0x1052f628);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x1052f650, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting); Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x1052f688, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x1052f698); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // Virtual destructors may replace the array or change its inclusive end.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x1052f6d0);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        c.set(3, league.stages());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x1052f6f8); league.stages() = 0;
        }
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x1052f714, {c.r(3), 1});
        p.call(0x1049e600, 0x1052f720, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x1052f730, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x105310c0, 0x80, {.first_gpr = 26});
    Kept<30> object(c, address); Kept<26> total(c, c.toc(slot::club_count));
    Kept<27> division_id(c, c.toc(slot::third)); Kept<31> count(c, 0);
    Kept<28> index(c, c.toc(-0x7ba8)); Kept<29> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 24;
    const auto allocated = p.call(0x10775130, 0x105310fc, {std::uint32_t(std::int32_t(league.club_count())) * 0x44});
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x1053111c, 0x40, 0x38, 0x1fa);
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
                p.call(0x104a60e0, 0x105311ac);
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
                if (compare(c, c.r(3), league.club_count()) == 0) break;
            }
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + 0x248;
    }
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
    if (compare(c, c.r(3), league.club_count()) != 0) return 0;
    c.put32(address + 0x28, p.call(0x1049f540, 0x105311f8, {object, 10000}));
    c.set(3, c.toc(slot::date)); c.put8(address + 0xe1, 1);
    // The balance seed depends on the game year: after 1999 one million, else 200,000.
    if (compare_logical(c, c.u16(c.r(3) + 2), 1999) > 0) p.call(0x1049f6d0, 0x10531228, {object, 1000000, 0});
    else p.call(0x1049f6d0, 0x10531240, {object, 200000, 0});
    return 1;
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10531260, 0x40, {.first_gpr = 31});
    Kept<31> object(c, address);
    Program p{c};
    c.set({{7, 2}, {6, 3}, {5, 1}, {4, 4}});
    c.put16(address + 0x3c, c.r(7));
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2},
             {0x42, 1}, {0xc7, 1}, {0xc8, 2}, {0xc9, 3}, {0xc0, 3}, {0xc1, 4}, {0xc2, 0}, {0xc3, 1}})
        c.put8(address + offset, value);
    c.set(3, c.toc(slot::second_rules)); c.put32(address + 0x1c, c.u32(c.r(3)));
    // Bit 4 of the definition's flags selects whether the second limit comes
    // from the shared rules table or stays -1.
    c.set(3, c.u32(c.u32(address + 4) + 8));
    if (compare(c, std::int32_t(c.u8(c.r(3) + 0x120) & 4)) != 0) {
        c.set(3, c.toc(slot::scheduling_rules)); c.put32(address + 0x20, c.u32(c.r(3)));
    } else c.put32(address + 0x20, missing);
    c.put8(address + 0x46, 5); c.put8(address + 0x47, 3); c.put8(address + 0x4c, 2);
    c.set({{3, address}, {5, address + 0xa6}, {6, address + 0x3a}, {4, missing}, {7, 0}});
    const auto schedule = p.virtual_call(address, 0x44, 0x10531328);
    c.put32(std::uint32_t(object) + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x10531350, 0xf0, {.first_gpr = 26});
    Kept<26> object(c, address);
    keep(c, {{31, c.toc(-0x7bac)}, {29, c.toc(-0x7ba8)}, {30, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x10531380, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x10531390); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x105313ac, {c.r(3), 1});
    Kept<27> index(c, 0); Kept<28> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x105313dc);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x10531418)) == 0) {
        assertion(c, f, 0x10531438, 0xb8, 0x50, 0x26c); return;
    }
    c.set(3, object); call(c, 0x105310c0, 0x10531480);
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x1053149c, 0x98, 0x48, 0x273); return; }
    if (compare(c, p.call(0x104a00a0, 0x105314e4, {object})) == 0) {
        assertion(c, f, 0x10531500, 0x78, 0x40, 0x279); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x1053154c, {object, 0})) == 0) {
        assertion(c, f, 0x10531568, 0x58, 0x38, 0x280); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x105315b8);
    p.call(0x104aa3a0, 0x105315c4, {object});
    p.call(0x102d46c0, 0x105315d4, {c.u32(c.toc(-0x7428)), league.definition()});
}
}

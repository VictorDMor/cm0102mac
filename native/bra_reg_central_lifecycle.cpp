#include "bra_reg_central_detail.hpp"

namespace cm::aot::bra_reg_central {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x106d73f0, 0x120, {.first_gpr = 27});
    Kept<27> fixtures(c, year);
    keep(c, {{28, definition}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c}; const Competition league{c, address};
    p.call(0x1049e4f0, 0x106d7420, {address});
    c.set({{5, c.toc(slot::vtable)}, {3, address}, {4, 1}});
    c.put32(address, c.r(5));
    league.year() = std::int16_t(year);
    league.definition() = definition;
    c.put8(address + 0x4a, 7);
    if (compare(c, p.virtual_call(address, 0x20, 0x106d745c)) != 0) return address;
    c.set({{4, missing}, {5, 15}});
    c.put8(address + 0x4b, 15); c.put8(address + 0x44, missing);
    league.last_stage() = -1; league.stage_count() = 1;
    // The original retests the count it just stored; its no-stage arm is dead.
    compare(c, league.stage_count());
    p.call(0x10775130, 0x106d74b4, {4});
    c.set(4, address); league.stages() = c.r(3);
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x106d74d8)) == 0) {
        assertion(c, f, 0x106d74f8, 0xe0, 0x58, 0x4c); return address;
    }
    enter(c, 0x106d7544, {address}); result(c, collect_clubs(c, address));
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x106d7560, 0xc0, 0x50, 0x53); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x106d75ac, {address})) == 0) {
        assertion(c, f, 0x106d75c8, 0xa0, 0x48, 0x59); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x106d7618, {address, 0})) == 0) {
        assertion(c, f, 0x106d7634, 0x80, 0x40, 0x60); return address;
    }
    fixtures = p.call(0x10773520, 0x106d7680, {0x634});
    if (compare(c, fixtures) != 0) p.call(0x104533a0, 0x106d7690, {fixtures, address});
    league.fixtures() = fixtures;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106d76bc, 0x60, 0x38, 0x67); return address;
    }
    c.set(3, address); p.virtual_call(address, 0x64, 0x106d770c);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x106d7730, 0x50, {.first_gpr = 28});
    Kept<29> delete_flag(c, deleting);
    Kept<28> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x106d7768, {object, 0});
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x106d7778); league.schedule() = 0;
        }
        Kept<30> index(c, 0); Kept<31> offset(c, 0);
        // The original uses an inclusive last index, reloaded after callbacks.
        while (compare(c, index, league.last_stage()) <= 0) {
            c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106d77b0);
            }
            offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
        }
        // Unlike the schedule, the stage array is released even when null and keeps its pointer.
        p.call(0x10775170, 0x106d77d0, {league.stages()});
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x106d77e4, {c.r(3), 1});
        p.call(0x1049e600, 0x106d77f0, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x106d7800, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d8360, 0x80, {.first_gpr = 26});
    Kept<30> object(c, address);
    Kept<26> total(c, c.toc(slot::club_total));
    Kept<27> division_id(c, c.toc(slot::central));
    Kept<31> count(c, 0);
    Kept<28> index(c, c.toc(slot::message));
    Kept<29> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 15;
    p.call(0x10775130, 0x106d839c, {std::uint32_t(std::int32_t(league.club_count())) * standing_size});
    const auto allocated = c.r(3);
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x106d83bc, 0x40, 0x38, 0x168);
        c.set(4, c.toc(slot::flag)); return 0;
    }
    index = 0; club = c.u32(c.toc(slot::clubs));
    while (compare(c, std::int16_t(std::uint32_t(index)), c.i32(total)) < 0) {
        const auto division = c.u32(std::uint32_t(club) + 0x38);
        if (compare_logical(c, division) != 0 && compare(c, c.i32(division), c.i32(division_id)) == 0) {
            c.set({{4, count}, {3, object}, {5, club}, {6, 0}, {7, 0}});
            count = std::uint32_t(count) + 1;
            c.call(0x104a60e0, 0x106d844c);
            if (compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0) break;
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + club_size;
    }
    return compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0;
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d84b0, 0x40, {.first_gpr = 31});
    Kept<31> object(c, address); Program p{c};
    c.set({{3, address}, {4, missing}, {5, address + 0xa6}, {6, address + 0x3a},
           {7, 0}, {8, missing}, {9, 8}, {10, 0}});
    c.put16(address + 0x3c, 1);
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2}, {0x42, 1}, {0xc7, 3},
             {0xc8, 1}, {0xc9, 0}, {0xc0, 8}, {0xc1, 0}, {0xc2, 0}, {0xc3, 3}, {0x46, 7}, {0x47, 3}})
        c.put8(address + offset, value);
    c.put16(address + 0xa8, 0); c.put16(address + 0xde, 0); c.put16(address + 0xdc, 8);
    c.put32(address + 0x1c, missing); c.put32(address + 0x20, missing);
    // The virtual calendar's stage -1 fills 0xa6 and 0x3a.
    const auto schedule = p.virtual_call(address, 0x44, 0x106d8548);
    c.put32(std::uint32_t(object) + 0xbc, schedule);
    return 1;
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d8570, 0xf0, {.first_gpr = 26});
    Kept<26> object(c, address);
    keep(c, {{31, c.toc(slot::flag)}, {29, c.toc(slot::message)}, {30, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.put8(address + 0x49, 0);
    p.call(0x104a6660, 0x106d85a0, {object, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x106d85b0); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x106d85cc, {c.r(3), 1});
    Kept<27> index(c, 0); Kept<28> offset(c, 0);
    while (compare(c, index, league.last_stage()) <= 0) {
        c.set(3, c.u32(std::uint32_t(league.stages()) + std::uint32_t(offset)));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1); p.virtual_call(c.r(3), 8, 0x106d85fc);
        }
        offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
    }
    league.last_stage() = -1;
    c.set({{3, object}, {4, extended(league.year())}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(object, 0x94, 0x106d8638)) == 0) {
        assertion(c, f, 0x106d8658, 0xb8, 0x50, 0x1cb); return;
    }
    enter(c, 0x106d86a0, {object}); result(c, collect_clubs(c, object));
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x106d86bc, 0x98, 0x48, 0x1d2); return; }
    if (compare(c, p.call(0x104a00a0, 0x106d8704, {object})) == 0) {
        assertion(c, f, 0x106d8720, 0x78, 0x40, 0x1d8); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x106d876c, {object, 0})) == 0) {
        assertion(c, f, 0x106d8788, 0x58, 0x38, 0x1df); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x106d87d8);
    c.set(4, league.definition());
    p.call(0x102d46c0, 0x106d87ec, {c.u32(c.toc(slot::schedule_events))});
}

void base_forward(Cpu& c) {
    Frame f(c, 0x106d9150, 0x40, {});
    c.call(0x104aa430, 0x106d9160);
}
}

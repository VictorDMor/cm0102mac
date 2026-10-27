#include "eng_prm_detail.hpp"

namespace cm::aot::eng_prm {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x10529840, 0x120, {.first_gpr = 27});
    Kept<27> saved_year(c, year);
    Kept<28> saved_definition(c, definition);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x138), address);
    Program p{c};
    p.call(0x1049e4f0, 0x10529870, {address});
    const Competition league{c, address};
    c.set({{6, c.toc(slot::vtable)}, {5, 9}, {3, address}, {4, 1}});
    c.put32(address, c.r(6));
    league.year() = std::int16_t(std::uint32_t(saved_year));
    league.definition() = saved_definition;
    c.put8(address + 0x4a, 9);
    c.put16(address + 0xe2, 10000);
    c.put16(address + 0xe4, 10000);
    if (compare(c, p.virtual_call(address, 0x20, 0x105298c0)) != 0) return address;
    c.set({{3, address}, {4, missing}});
    c.put8(address + 0x44, missing);
    c.put32(address + 0x30, missing);
    c.put32(address + 0x2c, 0);
    c.set({{4, address}, {3, c.u32(address + 0x2c)}});
    if (compare(c, c.r(3)) > 0) {
        p.call(0x10775130, 0x1052990c, {c.r(3) * 4});
        c.set(4, address);
        c.put32(address + 0xc, c.r(3));
    } else c.put32(address + 0xc, 0);
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x1052992c)) == 0) {
        assertion(c, f, 0x1052994c, 0xe0, 0x58, 0x4d); return address;
    }
    enter(c, 0x10529998, {address}); result(c, collect_clubs(c, address));
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x105299b4, 0xc0, 0x50, 0x54); return address;
    }
    if (compare(c, p.call(0x104a00a0, 0x10529a00, {address})) == 0) {
        assertion(c, f, 0x10529a1c, 0xa0, 0x48, 0x5b); return address;
    }
    if (compare(c, p.call(0x1049eb30, 0x10529a6c, {address, 0})) == 0) {
        assertion(c, f, 0x10529a88, 0x80, 0x40, 0x64); return address;
    }
    saved_year = p.call(0x10773520, 0x10529ad4, {0x634});
    if (compare(c, saved_year) != 0) p.call(0x104533a0, 0x10529ae4, {saved_year, address});
    c.set(3, address);
    league.fixtures() = saved_year;
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x10529b10, 0x60, 0x38, 0x6b); return address;
    }
    p.virtual_call(address, 0x64, 0x10529b60);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x10529b80, 0x40, {.first_gpr = 30});
    Kept<31> delete_flag(c, deleting);
    Kept<30> object(c, address);
    Program p{c};
    if (compare(c, object) != 0) {
        c.put32(address, c.toc(slot::vtable));
        p.call(0x104a6660, 0x10529bb0, {object, 0});
        const Competition league{c, object};
        c.set(3, league.schedule());
        if (compare_logical(c, c.r(3)) != 0) {
            p.call(0x10775170, 0x10529bc0); league.schedule() = 0;
        }
        c.set(3, league.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x10529bdc, {c.r(3), 1});
        p.call(0x1049e600, 0x10529be8, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x10529bf8, {object});
    }
    return object;
}

std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1052bbc0, 0x80, {.first_gpr = 26});
    Kept<31> object(c, address);
    Kept<27> total(c, c.toc(slot::club_count));
    Kept<28> division_id(c, c.toc(slot::premier));
    Kept<26> count(c, 0);
    Kept<29> index(c, c.toc(-0x7ba8));
    Kept<30> club(c, c.toc(slot::pool));
    Program p{c}; Competition league{c, object};
    league.club_count() = 20;
    p.call(0x10775130, 0x1052bbfc, {std::uint32_t(std::int32_t(league.club_count())) * 0x44});
    const auto allocated = c.r(3);
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x1052bc1c, 0x40, 0x38, 0x1ee);
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
                p.call(0x104a60e0, 0x1052bcac);
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
                if (compare(c, c.r(3), league.club_count()) == 0) break;
            }
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + 0x248;
    }
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))));
    if (compare(c, c.r(3), league.club_count()) != 0) return 0;
    c.put32(address + 0x28, p.call(0x1049f540, 0x1052bcfc, {object, 0x3d090}));
    c.set(3, c.toc(slot::date)); c.put8(address + 0xe1, 1);
    const auto year = c.u16(c.r(3) + 2);
    const bool later = compare_logical(c, year, 1999) > 0;
    p.call(0x1049f6d0, later ? 0x1052bd2c : 0x1052bd44,
           {object, later ? 12000000u : 5000000u, 0});
    return 1;
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1052bd60, 0x40, {.first_gpr = 31});
    Kept<31> object(c, address); Program p{c};
    c.set({{10, 1}, {9, 0}, {8, c.toc(slot::first)}, {5, address + 0xa6},
           {6, address + 0x3a}, {4, missing}, {7, 0}});
    c.put16(address + 0x3c, 2);
    for (const auto [offset, value] : {std::pair{0xc4u, 3u}, {0xc5, 1}, {0xc6, 2},
             {0x42, 1}, {0xc7, 1}, {0xc8, 2}, {0xc9, 3}, {0xc0, 0}, {0xc1, 0}, {0xc2, 0}, {0xc3, 3}, {0x4c, 2}})
        c.put8(address + offset, value);
    c.put16(address + 0xa8, 0); c.put16(address + 0xde, 0);
    c.put32(address + 0x1c, missing); c.put32(address + 0x20, c.u32(c.r(8)));
    const auto schedule = p.virtual_call(address, 0x44, 0x1052bdf0);
    c.put32(std::uint32_t(object) + 0xbc, schedule);
    return 1;
}
}

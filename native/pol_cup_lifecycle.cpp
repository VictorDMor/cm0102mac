#include "pol_cup_detail.hpp"

namespace cm::aot::pol_cup {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t address, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x106cd810, 0xf0, {.first_gpr = 27});
    Kept<27> saved_year(c, year);
    Kept<28> saved_definition(c, definition);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.put32(f.at(0x108), address);
    Program p{c}; Cup cup{c, address};
    p.call(0x10478f20, 0x106cd840, {address});
    c.put32(address, c.toc(slot::vtable));
    cup.year() = std::int16_t(std::uint32_t(saved_year));
    c.put8(address + 0xa8, 0); c.put8(address + 0x44, 0xff);
    c.put32(address + 0x30, 0xffffffff); c.put32(address + 0x2c, 0);
    c.put32(address + 4, saved_definition);
    c.put8(address + 0x42, 1); c.put8(address + 0x46, 7);
    c.put8(address + 0x47, 3); c.put8(address + 0x4a, 31); c.put8(address + 0xb0, 0);
    leave(c, {{3, address}, {4, 1}, {5, 3}, {6, 7}, {7, 1}, {8, 0xffffffff}, {9, 0}});
    if (compare(c, p.virtual_call(address, 0x20, 0x106cd8d0)) != 0) return address;
    enter(c, 0x106cd8ec, {address}); result(c, select_clubs(c, address));
    if (compare(c, c.r(3)) == 0) {
        assertion(c, f, 0x106cd908, 0xb8, 0x50, 0x44); return address;
    }
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x94, 0x106cd95c)) == 0) {
        assertion(c, f, 0x106cd97c, 0x98, 0x48, 0x4b); return address;
    }
    c.set(3, address);
    if (compare(c, p.virtual_call(address, 0x9c, 0x106cd9d0)) == 0) {
        assertion(c, f, 0x106cd9f0, 0x78, 0x40, 0x52); return address;
    }
    saved_year = p.call(0x10773520, 0x106cda3c, {0x634});
    if (compare(c, saved_year) != 0) p.call(0x104533a0, 0x106cda4c, {saved_year, address});
    c.set(3, address); cup.fixtures() = saved_year;
    if (compare_logical(c, cup.fixtures()) == 0) {
        assertion(c, f, 0x106cda78, 0x58, 0x38, 0x59); return address;
    }
    p.virtual_call(address, 0x64, 0x106cdac8);
    return address;
}

std::uint32_t destroy(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame f(c, 0x106cdaf0, 0x50, {.first_gpr = 28});
    Kept<28> object(c, address); Kept<29> delete_flag(c, deleting);
    Program p{c}; Cup cup{c, address};
    if (compare(c, address) != 0) {
        c.put32(address, c.toc(slot::vtable));
        c.set(3, cup.entrants());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x10775170, 0x106cdb30);
        c.set(3, cup.calendar());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x10775170, 0x106cdb40);
        if (compare_logical(c, cup.round_storage()) != 0) {
            Kept<30> round(c, 0); Kept<31> offset(c, 0);
            while (compare(c, round, cup.rounds()) < 0) {
                c.set(3, c.u32(std::uint32_t(cup.round_storage()) + std::uint32_t(offset)));
                if (compare_logical(c, c.r(3)) != 0) p.call(0x10775170, 0x106cdb6c);
                offset = std::uint32_t(offset) + 4; round = std::uint32_t(round) + 1;
            }
            p.call(0x10775170, 0x106cdb88, {cup.round_storage()});
        }
        c.set(3, cup.fixtures());
        if (compare_logical(c, c.r(3)) != 0) p.call(0x104536c0, 0x106cdb9c, {c.r(3), 1});
        p.call(0x10478f70, 0x106cdba8, {object, 0});
        if (compare(c, std::int16_t(std::uint32_t(delete_flag))) > 0)
            p.call(0x107735a0, 0x106cdbb8, {object});
    }
    return object;
}

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106ce570, 0xd0, {.first_gpr = 25});
    Kept<28> object(c, address);
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    Program p{c}; Cup cup{c, address};
    c.put8(address + 0x49, 0);
    c.set(3, cup.entrants());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x106ce5a8); cup.entrants() = 0;
    }
    c.set(3, cup.calendar());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x106ce5c0); cup.calendar() = 0;
    }
    if (compare_logical(c, cup.round_storage()) != 0) {
        Kept<26> offset(c, 0); Kept<25> round(c, 0); Kept<27> zero(c, 0);
        while (compare(c, round, cup.rounds()) < 0) {
            c.set(3, c.u32(std::uint32_t(cup.round_storage()) + std::uint32_t(offset)));
            if (compare_logical(c, c.r(3)) != 0) {
                p.call(0x10775170, 0x106ce5fc);
                c.set(3, cup.round_storage()); c.put32(c.r(3) + std::uint32_t(offset), zero);
            }
            offset = std::uint32_t(offset) + 4; round = std::uint32_t(round) + 1;
        }
        p.call(0x10775170, 0x106ce620, {cup.round_storage()}); cup.round_storage() = 0;
    }
    c.set(3, cup.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x106ce63c, {c.r(3), 1});
    const auto year = std::uint32_t(std::int32_t(cup.year())) + 1;
    cup.year() = std::int16_t(year);
    c.put8(address + 0xa8, 0); c.put8(address + 0xb0, 0);
    enter(c, 0x106ce65c, {object, year}); result(c, select_clubs(c, object));
    if (compare(c, c.r(3)) < 0) { assertion(c, f, 0x106ce678, 0x90, 0x48, 0x1dd); return; }
    c.set(3, object);
    if (compare(c, p.virtual_call(object, 0x94, 0x106ce6c8)) < 0) {
        assertion(c, f, 0x106ce6e8, 0x70, 0x40, 0x1e4); return;
    }
    c.set(3, object);
    if (compare(c, p.virtual_call(object, 0x9c, 0x106ce738)) < 0) {
        assertion(c, f, 0x106ce758, 0x50, 0x38, 0x1eb); return;
    }
    c.set(3, object); p.virtual_call(object, 0x64, 0x106ce7a8);
}

std::uint32_t configure(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106ce7c0, 0x40, {.first_gpr = 31});
    Kept<31> object(c, address); Program p{c};
    c.put32(address + 0x28, 600000);
    leave(c, {{3, address}, {4, 0xffffffff}, {5, address + 0x3c}, {6, address + 0x3a}, {7, 0}});
    Cup{c, address}.calendar() = p.virtual_call(address, 0x44, 0x106ce7fc);
    return 1;
}
}

#include "eng_prm_detail.hpp"

namespace cm::aot::eng_prm {
using namespace detail;

void next_season(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1052be10, 0x100, {.first_gpr = 25});
    keep(c, {{26, c.toc(slot::third)}, {25, address}, {27, c.toc(slot::conference)},
             {28, c.toc(slot::divisions)}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    Program p{c}; Competition league{c, address};
    c.set(4, 1); c.put8(address + 0x49, 0);
    p.virtual_call(address, 0xb8, 0x1052be54);
    c.set(3, c.u32(c.r(28)));
    const auto conference = c.u32(c.r(3) + c.u32(c.r(27)) * 4);
    if (compare_logical(c, conference) != 0) {
        enter(c, 0x1052be78, {address}); promote_with_conference(c, address);
    } else {
        enter(c, 0x1052be84, {address}); promote_without_conference(c, address);
    }
    p.call(0x104a6660, 0x1052be90, {address, 0});
    c.set(3, league.schedule());
    if (compare_logical(c, c.r(3)) != 0) {
        p.call(0x10775170, 0x1052bea0); league.schedule() = 0;
    }
    c.set(3, league.fixtures());
    if (compare_logical(c, c.r(3)) != 0) p.call(0x104564a0, 0x1052bebc, {c.r(3), 1});
    c.set({{4, extended(league.year())}, {3, address}});
    league.year() = std::int16_t(c.r(4) + 1);
    if (compare(c, p.virtual_call(address, 0x94, 0x1052bed8)) == 0) {
        assertion(c, f, 0x1052bef8, 0xb8, 0x50, 0x267); return;
    }
    enter(c, 0x1052bf40, {address}); result(c, collect_clubs(c, address));
    if (compare(c, c.r(3)) == 0) { assertion(c, f, 0x1052bf5c, 0x98, 0x48, 0x26e); return; }
    if (compare(c, p.call(0x104a00a0, 0x1052bfa4, {address})) == 0) {
        assertion(c, f, 0x1052bfc0, 0x78, 0x40, 0x274); return;
    }
    if (compare(c, p.call(0x1049eb30, 0x1052c00c, {address, 0})) == 0) {
        assertion(c, f, 0x1052c028, 0x58, 0x38, 0x27b); return;
    }
    c.set(3, address); p.virtual_call(address, 0x64, 0x1052c078);
    p.call(0x104aa240, 0x1052c0a0, {p.division(slot::third), 1, 3, std::uint32_t(-3), 1});
    c.set(3, p.division(slot::conference));
    if (compare_logical(c, c.r(3)) != 0)
        p.call(0x104aa240, 0x1052c0cc, {c.r(3), 2, 0, std::uint32_t(-3), 1});
    c.set({{3, c.toc(slot::first)}, {4, c.u32(c.r(28))}});
    c.set(3, c.u32(c.r(4) + c.u32(c.r(3)) * 4));
    p.virtual_call(c.r(3), 0x10, 0x1052c0ec);
    c.set({{3, c.toc(slot::second)}, {4, c.u32(c.r(28))}});
    c.set(3, c.u32(c.r(4) + c.u32(c.r(3)) * 4));
    p.virtual_call(c.r(3), 0x10, 0x1052c110);
    c.set(3, p.division(slot::third)); p.virtual_call(c.r(3), 0x10, 0x1052c130);
    c.set(3, p.division(slot::conference));
    if (compare_logical(c, c.r(3)) != 0) p.virtual_call(c.r(3), 0x10, 0x1052c158);
    p.call(0x104aa3a0, 0x1052c164, {address});
    c.set({{3, c.toc(-0x7428)}, {4, league.definition()}});
    p.call(0x102d46c0, 0x1052c174, {c.u32(c.r(3))});
}

void promote_without_conference(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1052c190, 0xc0, {.first_gpr = 21});
    Kept<26> third_id(c, c.toc(slot::third));
    Kept<21> object(c, address);
    c.set(4, c.toc(slot::divisions));
    Kept<23> count(c, 0);
    Kept<28> total(c, c.toc(slot::club_count));
    Kept<25> records(c, c.toc(slot::records));
    Kept<27> conference_id(c, c.toc(slot::conference));
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    c.set({{3, c.u32(third_id)}, {5, c.u32(c.r(4))}});
    c.set(4, c.r(3) * 4);
    Kept<22> third(c, c.u32(c.r(5) + c.r(4)));
    Program p{c};
    Kept<24> selected(c, p.call(0x10775130, 0x1052c1e4, {c.u32(total) * 4}));
    if (compare(c, selected) == 0) { assertion(c, f, 0x1052c200, 0x68, 0x40, 0x2a9); return; }
    c.set({{3, c.toc(slot::clubs)}, {5, 0}}); c.set(4, c.u32(c.r(3)));
    while (true) {
        c.set(3, extended(std::int16_t(c.r(5))));
        if (compare(c, c.r(3), c.i32(total)) >= 0) break;
        c.set(3, c.u32(c.r(4) + 0x30));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(3, c.u32(c.r(3)));
            if (compare(c, c.r(3), c.i32(conference_id)) == 0) {
                const auto offset = extended(std::int16_t(std::uint32_t(count))) * 4;
                count = std::uint32_t(count) + 1;
                c.put32(std::uint32_t(selected) + offset, c.r(4));
            }
        }
        c.set({{5, c.r(5) + 1}, {4, c.r(4) + 0x248}});
    }
    if (compare(c, std::int16_t(std::uint32_t(count))) == 0) {
        p.call(0x10775170, 0x1052c2a8, {selected});
        assertion(c, f, 0x1052c2bc, 0x48, 0x38, 0x2be); return;
    }
    p.call(0x104635b0, 0x1052c30c, {selected, count, 1});
    c.set({{3, c.toc(-0x7ab8)}, {4, c.u32(selected)}});
    p.call(0x10103510, 0x1052c31c, {c.u32(c.r(3))});
    c.set({{6, c.u32(std::uint32_t(third) + 4)}, {7, 1},
           {4, extended(c.i16(std::uint32_t(third) + 0xe4))}, {5, extended(c.i16(std::uint32_t(third) + 0xe2))}});
    c.set(6, c.u32(c.r(6))); p.call(0x100f7cf0, 0x1052c334);
    if (!byte_nonzero(c, c.r(3))) {
        c.set({{4, extended(c.i16(std::uint32_t(third) + 0x3e))}, {3, third}});
        object = std::uint32_t(-2);
        p.call(0x1049ea00, 0x1052c350, {third, c.r(4) - 1});
        c.put8(c.r(3) + 0x40, object);
        p.call(0x10452a60, 0x1052c378,
               {third, c.u32(records) + c.u32(conference_id) * 0x6c, c.u32(selected), 0, 2});
    } else {
        p.call(0x1049f780, 0x1052c39c,
               {object, c.u32(selected), c.u32(records) + c.u32(third_id) * 0x6c, 1});
        count = 0;
        while (compare(c, count, c.i16(std::uint32_t(third) + 0x3e)) < 0) {
            p.call(0x1049ea00, 0x1052c3b4, {third, count});
            if (compare(c, c.u8(c.r(3) + 0x40), 3) == 0) {
                p.call(0x1049ea00, 0x1052c3cc, {third, count});
                c.set(4, c.r(3));
                p.call(0x1049f8e0, 0x1052c3f0,
                       {object, c.u32(c.r(4)), c.u32(records) + c.u32(conference_id) * 0x6c, 1});
            }
            count = std::uint32_t(count) + 1;
        }
    }
    p.call(0x10775170, 0x1052c408, {selected});
}

void promote_with_conference(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1052c420, 0xb0, {.first_gpr = 16});
    Kept<21> conference_id(c, c.toc(slot::conference));
    Kept<27> object(c, address);
    c.set(4, c.toc(-0x4524));
    Kept<30> count(c, 0);
    c.set(5, c.toc(slot::divisions));
    Kept<28> first_league(c, 0);
    Kept<23> total(c, c.toc(slot::club_count));
    Kept<16> nation(c, c.toc(-0x7a10));
    Kept<17> excluded(c, c.toc(-0x7a0c));
    Kept<18> records(c, c.toc(slot::records));
    Kept<19> second_id(c, c.toc(slot::second));
    Kept<20> third_id(c, c.toc(slot::third));
    Kept<22> first_id(c, c.toc(slot::first));
    Kept<24> premier_id(c, c.toc(slot::premier));
    keep(c, {{25, c.toc(-0x7ba8)}, {26, c.toc(slot::pool)}});
    c.set({{3, c.u32(c.r(4))}, {6, c.u32(c.r(4) + 4)}, {7, c.u32(conference_id) * 4},
           {8, c.u32(c.r(5))}});
    const auto initial_third = c.u16(c.r(4) + 8);
    c.set(4, c.u16(c.r(4) + 10)); c.set(5, initial_third);
    c.put32(f.at(0x40), c.r(3)); c.put32(f.at(0x44), c.r(6));
    c.put16(f.at(0x48), c.r(5)); c.put16(f.at(0x4a), c.r(4));
    Kept<29> conference(c, c.u32(c.r(8) + c.r(7)));
    Program p{c};
    Kept<31> selected(c, p.call(0x10775130, 0x1052c4ac, {c.u32(total) * 4}));
    if (compare(c, selected) == 0) {
        assertion(c, f, 0x1052c4c8, 0x4c, 0x38, 0x2f9); c.set(3, c.toc(-0x7bac)); return;
    }
    c.set({{3, c.toc(slot::clubs)}, {4, 0}}); c.set(3, c.u32(c.r(3)));
    while (compare(c, c.r(4), c.i32(total)) < 0) {
        c.set(5, c.u32(c.r(3) + 0x30));
        if (compare_logical(c, c.r(5)) != 0) {
            c.set(6, c.u32(c.r(3) + 0x2c));
            if (compare_logical(c, c.r(6)) != 0) {
                c.set(5, c.u32(c.r(5)));
                if (compare(c, c.r(5), c.i32(excluded)) != 0 &&
                    compare(c, c.r(5), c.i32(premier_id)) != 0 &&
                    compare(c, c.r(5), c.i32(first_id)) != 0 &&
                    compare(c, c.r(5), c.i32(second_id)) != 0 &&
                    compare(c, c.r(5), c.i32(third_id)) != 0 &&
                    compare(c, c.r(5), c.i32(conference_id)) != 0) {
                    c.set(5, c.u32(c.r(6) + 0x68));
                    if (compare(c, c.r(5), c.i32(nation)) == 0) {
                        const auto offset = extended(std::int16_t(std::uint32_t(count))) * 4;
                        compare_logical(c, first_league);
                        count = std::uint32_t(count) + 1;
                        c.put32(std::uint32_t(selected) + offset, c.r(3));
                        if (std::uint32_t(first_league) == 0) first_league = c.u32(c.r(3) + 0x30);
                    }
                }
            }
        }
        c.set({{4, c.r(4) + 1}, {3, c.r(3) + 0x248}});
    }
    if (compare(c, std::int16_t(std::uint32_t(count))) == 0) {
        p.call(0x10775170, 0x1052c5d0, {selected}); return;
    }
    p.call(0x104635b0, 0x1052c5e4, {selected, count, 3});
    second_id = selected; third_id = extended(std::int16_t(std::uint32_t(count))); nation = 0;
    while (compare(c, nation, third_id) < 0) {
        c.set({{3, f.at(0x40)}, {6, 0}, {5, 0}});
        while (compare(c, std::int8_t(c.r(6)), 3) < 0) {
            c.set(4, c.u32(c.u32(second_id) + 0x30));
            if (compare_logical(c, c.r(4)) != 0 && compare_logical(c, c.r(4), c.u32(c.r(3))) == 0) break;
            c.set({{5, c.r(5) + 4}, {3, c.r(3) + 4}, {6, c.r(6) + 1}});
        }
        if (compare(c, std::int8_t(c.r(6)), 3) == 0) {
            excluded = 0; c.set({{4, f.at(0x40)}, {5, 0}, {3, f.at(0x40)}});
            while (compare(c, std::int8_t(std::uint32_t(excluded)), 3) < 0) {
                if (compare_logical(c, c.u32(c.r(3))) == 0) {
                    c.set(3, c.u32(second_id)); c.set(3, c.u32(c.r(3) + 0x30));
                    c.put32(c.r(4) + std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(excluded)))) * 4, c.r(3));
                    break;
                }
                c.set({{5, c.r(5) + 4}, {3, c.r(3) + 4}}); excluded = std::uint32_t(excluded) + 1;
            }
            // The original still promotes when all three remembered slots
            // are occupied. In that case it leaves the remembered list alone.
            p.call(0x1049f780, 0x1052c6bc,
                   {object, c.u32(second_id), c.u32(records) + c.u32(conference_id) * 0x6c, 1});
            if (compare(c, std::int8_t(std::uint32_t(excluded)), 2) == 0) break;
        }
        second_id = std::uint32_t(second_id) + 4; nation = std::uint32_t(nation) + 1;
    }
    nation = 0; excluded = 0;
    while (true) {
        if (compare(c, nation, c.i16(std::uint32_t(conference) + 0x3e)) >= 0) break;
        if (compare(c, std::int8_t(std::uint32_t(excluded)), 3) >= 0) break;
        p.call(0x1049ea00, 0x1052c6f4, {conference, nation});
        if (compare(c, c.u8(c.r(3) + 0x40), 3) == 0) {
            p.call(0x1049ea00, 0x1052c70c, {conference, nation});
            c.set(4, c.u32(c.r(3)));
            c.set(5, f.at(0x40)); c.set(5, c.u32(c.r(5) + std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(excluded)))) * 4));
            c.set({{3, object}, {6, 1}}); excluded = std::uint32_t(excluded) + 1;
            p.call(0x1049f8e0, 0x1052c730);
        }
        nation = std::uint32_t(nation) + 1;
    }
    p.call(0x10775170, 0x1052c754, {selected});
}

void link_divisions(Cpu& c, std::uint32_t address, std::uint32_t mode) {
    Frame f(c, 0x1052c770, 0x60, {.first_gpr = 23});
    Kept<28> third_id(c, c.toc(slot::third));
    Kept<24> link_mode(c, mode);
    Kept<29> conference_id(c, c.toc(slot::conference));
    Kept<23> object(c, address);
    Kept<30> divisions(c, c.toc(slot::divisions));
    Kept<27> second_id(c, c.toc(slot::second));
    Kept<31> first_id(c, c.toc(slot::first));
    c.set({{4, c.u32(third_id) * 4}, {5, c.u32(divisions)}});
    Kept<26> third(c, c.u32(c.r(5) + c.r(4)));
    Kept<25> conference(c, c.u32(c.r(5) + c.u32(conference_id) * 4));
    Program p{c}; p.virtual_call(address, 0xac, 0x1052c7c4);
    for (const auto [id, back] : {std::pair{slot::first, 0x1052c7e4u}, {slot::second, 0x1052c804u}, {slot::third, 0x1052c824u}}) {
        c.set(3, p.division(id)); p.virtual_call(c.r(3), 0xac, back);
    }
    c.set(4, c.toc(slot::premier)); c.set(5, c.u32(divisions));
    p.call(0x104a9210, 0x1052c860,
           {object, c.u32(c.r(5) + c.u32(c.r(4)) * 4), p.division(slot::first), 1, link_mode, missing, missing});
    p.call(0x104a9210, 0x1052c894,
           {object, p.division(slot::first), p.division(slot::second), 1, link_mode, missing, missing});
    p.call(0x104a9210, 0x1052c8c8,
           {object, p.division(slot::second), p.division(slot::third), 1, link_mode, missing, missing});
    if (compare_logical(c, conference) == 0) return;
    c.set(3, p.division(slot::conference)); p.virtual_call(c.r(3), 0xac, 0x1052c8ec);
    p.call(0x1049ea00, 0x1052c8fc, {conference, 0});
    c.set({{5, c.toc(-0x7ab8)}, {4, c.u32(c.r(3))}});
    p.call(0x10103510, 0x1052c90c, {c.u32(c.r(5))});
    c.set({{6, c.u32(std::uint32_t(third) + 4)}, {7, 1},
           {4, extended(c.i16(std::uint32_t(third) + 0xe4))}, {5, extended(c.i16(std::uint32_t(third) + 0xe2))}});
    c.set(6, c.u32(c.r(6))); p.call(0x100f7cf0, 0x1052c924);
    if (!byte_nonzero(c, c.r(3))) {
        c.set({{4, extended(c.i16(std::uint32_t(third) + 0x3e))}, {3, third}});
        object = std::uint32_t(-2);
        p.call(0x1049ea00, 0x1052c940, {third, c.r(4) - 1}); c.put8(c.r(3) + 0x40, object);
        p.call(0x1049ea00, 0x1052c950, {conference, 0});
        c.set(5, c.r(3)); c.set(4, c.toc(slot::records));
        p.call(0x10452a60, 0x1052c97c,
               {third, c.u32(c.r(4)) + c.u32(conference_id) * 0x6c, c.u32(c.r(5)), 0, 2});
    } else {
        p.call(0x104a9210, 0x1052c9b4,
               {object, p.division(slot::third), p.division(slot::conference), 1, link_mode, missing, missing});
    }
}
}

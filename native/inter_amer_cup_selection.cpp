#include "inter_amer_cup_detail.hpp"

namespace cm::aot::inter_amer_cup {
using namespace detail;
std::uint32_t select_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106ec330, 0x10e0, {.first_gpr = 16});
    Kept<31> object(c, address);
    keep(c, {{20, c.toc(slot::south_champion)}, {21, c.toc(slot::south_america)},
             {22, c.toc(slot::club_count)}, {23, c.toc(slot::clubs)},
             {24, c.toc(slot::north_champion)}, {25, c.toc(slot::competitions)},
             {26, c.toc(slot::north_america)}, {27, c.toc(slot::continents)},
             {28, c.toc(-0x7bac)}, {29, c.toc(-0x7ba8)}, {30, c.toc(slot::pool)}});
    Cup cup{c, address}; Program p{c};
    cup.club_count() = 2;
    const auto entrants = p.call(0x10775130, 0x106ec384, {16});
    compare_logical(c, entrants); cup.entrants() = entrants;
    if (entrants == 0) {
        assertion(c, f, 0x106ec3a4, 0xe0, 0x58, 0xc6); return 0xffffffff;
    }
    const auto competition_record = [&](int id_slot) {
        return c.u32(c.toc(slot::competitions)) + c.u32(c.toc(id_slot)) * 0x6c;
    };
    const auto continent_record = [&](int id_slot) {
        return c.u32(c.toc(slot::continents)) + c.u32(c.toc(id_slot)) * 0xc8;
    };
    const auto candidates = f.at(0x100);
    const auto north_active = p.call(0x101926b0, 0x106ec3fc, {continent_record(slot::north_america)});
    if (compare(c, std::int8_t(north_active)) != 0 || compare(c, cup.year(), 2001) == 0) {
        const auto champion = p.call(0x10463a90, 0x106ec428, {competition_record(slot::north_champion), 0});
        c.set(4, cup.entrants()); c.put32(c.r(4), champion); c.set(3, cup.entrants());
        if (compare_logical(c, c.u32(c.r(3))) == 0) {
            assertion(c, f, 0x106ec454, 0xc0, 0x50, 0xd1); return 0;
        }
    } else {
        Kept<19> club_address(c, c.u32(c.toc(slot::clubs)));
        Kept<17> index(c, 0); Kept<18> count(c, 0); Kept<16> offset(c, 0);
        while (compare(c, index, c.u32(c.toc(slot::club_count))) < 0) {
            if (compare(c, std::int8_t(p.call(0x100c7d00, 0x106ec4b4, {club_address}))) != 0) break;
            const Club club{c, club_address};
            if (compare(c, club.reputation(), 1000) > 0) {
                const auto nation = club.nation(); c.set(3, nation);
                if (compare_logical(c, nation) != 0) {
                    const auto continent = c.u32(nation + 0x74); c.set(3, continent);
                    if (compare_logical(c, continent) != 0) {
                        const auto id = c.u32(continent + 8); c.set(3, id);
                        if (compare(c, id, c.u32(c.toc(slot::north_america))) == 0) {
                            c.set(3, candidates); count = std::uint32_t(count) + 1;
                            c.put32(candidates + std::uint32_t(offset), club_address);
                            offset = std::uint32_t(offset) + 4;
                        }
                    }
                }
            }
            index = std::uint32_t(index) + 1; club_address = std::uint32_t(club_address) + Club::size;
        }
        if (compare(c, count) == 0) {
            assertion(c, f, 0x106ec530, 0xa0, 0x48, 0xea);
            c.set(5, 0);
            const auto fallback = p.call(0x10463a90, 0x106ec584, {competition_record(slot::north_champion), 0});
            // Original quirk: this first-continent fallback writes slot two,
            // leaving slot one untouched, before the second selection runs.
            c.set(4, cup.entrants()); c.put32(c.r(4) + 8, fallback);
        } else {
            p.call(0x104635b0, 0x106ec5a0, {candidates, extended(std::int16_t(std::uint32_t(count))), 1});
            c.set(3, cup.entrants()); c.put32(c.r(3), c.u32(candidates));
        }
    }
    const auto south_active = p.call(0x101926b0, 0x106ec5c0, {continent_record(slot::south_america)});
    if (compare(c, std::int8_t(south_active)) != 0 || compare(c, cup.year(), 2001) == 0) {
        const auto champion = p.call(0x10463a90, 0x106ec5ec, {competition_record(slot::south_champion), 0});
        c.set(4, cup.entrants()); c.put32(c.r(4) + 8, champion); c.set(3, cup.entrants());
        if (compare_logical(c, c.u32(c.r(3) + 8)) == 0) {
            assertion(c, f, 0x106ec618, 0x80, 0x40, 0xff); return 0;
        }
    } else {
        Kept<24> index(c, 0); Kept<26> club_address(c, c.u32(c.toc(slot::clubs)));
        Kept<23> count(c, 0); Kept<17> offset(c, 0);
        while (compare(c, index, c.u32(c.toc(slot::club_count))) < 0) {
            if (compare(c, std::int8_t(p.call(0x100c7d00, 0x106ec678, {club_address}))) != 0) break;
            const Club club{c, club_address};
            if (compare(c, club.reputation(), 5000) > 0) {
                const auto nation = club.nation(); c.set(3, nation);
                if (compare_logical(c, nation) != 0) {
                    const auto continent = c.u32(nation + 0x74); c.set(3, continent);
                    if (compare_logical(c, continent) != 0) {
                        const auto id = c.u32(continent + 8); c.set(3, id);
                        if (compare(c, id, c.u32(c.toc(slot::south_america))) == 0) {
                            c.set(3, candidates); count = std::uint32_t(count) + 1;
                            c.put32(candidates + std::uint32_t(offset), club_address);
                            offset = std::uint32_t(offset) + 4;
                        }
                    }
                }
            }
            index = std::uint32_t(index) + 1; club_address = std::uint32_t(club_address) + Club::size;
        }
        if (compare(c, count) == 0) {
            assertion(c, f, 0x106ec6f4, 0x60, 0x38, 0x118);
            c.set(5, 0);
            const auto fallback = p.call(0x10463a90, 0x106ec748, {competition_record(slot::south_champion), 0});
            c.set(4, cup.entrants()); c.put32(c.r(4) + 8, fallback);
        } else {
            p.call(0x104635b0, 0x106ec764, {candidates, extended(std::int16_t(std::uint32_t(count))), 1});
            c.set(3, cup.entrants()); c.put32(c.r(3) + 8, c.u32(candidates));
        }
    }
    c.set(4, cup.entrants()); c.put8(c.r(4) + 4, 0);
    c.set(4, cup.entrants()); c.put8(c.r(4) + 5, 0);
    c.set(4, cup.entrants()); c.put8(c.r(4) + 12, 0);
    c.set(4, cup.entrants()); c.put8(c.r(4) + 13, 0);
    return 1;
}
}

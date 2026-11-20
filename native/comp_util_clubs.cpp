// COMP_UTIL.CPP, block `club_queries`: two pure queries over the competition and club tables (0x104646d0) and
// over a fixture's two clubs (0x10464900). Neither calls anything. Callers' varargs calls spill r3-r10, so both
// leave r4-r9 as the original does on every path; the checker compares them.
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
bool competition_is_active(Cpu& c, std::int32_t competition) {
    const auto id = std::uint32_t(competition);
    // r4 holds the address of the last cell or row read.
    for (const auto slot : {toc::special_a, toc::special_b, toc::special_c}) {
        const auto cell = c.toc(slot);
        c.set(4, cell);
        if (c.i32(cell) == competition) return true;
    }
    const auto record = c.u32(c.toc(toc::records)) + id * 0x6c;
    c.set(4, record);
    if (c.u32(record + comp::record::nation) == 0) return false;
    const auto object = c.u32(c.u32(c.toc(toc::competitions)) + id * 4);
    c.set(4, object);
    if (object == 0 || c.u8(object + competition::kind) != league::kind) return false;
    if (c.i16(object + competition::club_count) == 0) return false;
    const auto standings = c.u32(object + league::standings);
    if (standings == 0) return false;
    const auto top = c.u32(standings);  // the first standing row's club
    c.set(5, top);
    const auto own = c.u32(object + competition::record);
    return c.u32(top + club::competition_record) == own || c.u32(top + club::second_record) == own;
}

namespace {
// A reputation (+0x1ec) in steps of 500, truncated towards zero, as a signed byte.
std::int32_t reputation_step(Cpu& c, std::uint32_t club_row) { return std::int8_t(c.i16(club_row + club::reputation) / 500); }
}

std::uint32_t fixture_class(Cpu& c, std::uint32_t club_row, std::uint32_t fixture_row) {
    const auto first = c.u32(fixture_row + fixture::first_club);
    const auto second = c.u32(fixture_row + fixture::second_club);
    // `a` and `b`: the reputation steps of the fixture's first and second club.
    const auto a = reputation_step(c, first);
    const auto b = reputation_step(c, second);
    const auto byte_4b = c.u8(fixture_row + fixture::goals_4b);
    // `margin`: the first club's goal difference of the latest leg played (a byte of -1 marks an unplayed leg).
    // r7 keeps the divide's multiplier unless the earlier legs are read.
    std::uint32_t seventh = 0x10624dd3;
    std::int32_t margin;
    if (std::int8_t(byte_4b) != -1) {
        margin = std::int8_t(byte_4b - c.u8(fixture_row + fixture::goals_4c));
    } else {
        seventh = c.u8(fixture_row + fixture::goals_47);
        if (std::int8_t(seventh) != -1) {
            margin = std::int8_t(seventh - c.u8(fixture_row + fixture::goals_48));
        } else {
            seventh = c.u8(fixture_row + fixture::goals_44);
            margin = std::int8_t(c.u8(fixture_row + fixture::goals_43) - seventh);
        }
    }
    c.set({{4, std::uint32_t(margin)}, {5, std::uint32_t(a)}, {6, std::uint32_t(b)}, {7, seventh}, {8, byte_4b}, {9, first}});
    // The thresholds are the original's. For the first club: a lead (margin above 0), a level game, a deficit
    // (taken as the margin negated, a byte); for the second club the same tests with the sides exchanged.
    if (club_row == first) {
        if (margin > 0) {
            if ((margin < 3 && a < b - 8) || (margin > 2 && a < b - 2)) return 6;
            if (margin > 4) return 1;
            if ((margin > 1 || b - 3 <= a) && (margin < 3 || b <= a)) return margin < 2 && a < b ? 3 : 4;
            return 2;
        }
        if (margin == 0) {
            if (a < b - 16 || b < a - 15) return 6;
            if (a < b - 11) return 2;
            if (a < b - 3) return 3;
            if (b < a - 5) return 5;
            if (b < a - 11) return 7;
            return 4;
        }
        const std::int32_t deficit = std::int8_t(-margin);
        if ((deficit < 3 && b < a - 7) || (deficit > 2 && b < a - 2)) return 6;
        if ((deficit > 1 || a - 6 <= b) && (deficit < 3 || a - 2 <= b)) return deficit < 2 && b < a - 2 ? 5 : 4;
        return 7;
    }
    if (margin < 0) {
        const std::int32_t deficit = std::int8_t(-margin);
        if ((deficit < 3 && b < a - 13) || (deficit > 2 && b < a - 7)) return 6;
        if (deficit > 4) return 1;
        if ((deficit > 1 || a - 3 <= b) && (deficit < 3 || a <= b)) return deficit < 2 && b < a ? 3 : 4;
        return 2;
    }
    if (margin == 0) {
        if (b < a - 16 || a < b - 15) return 6;
        if (b < a - 11) return 2;
        if (b < a - 3) return 3;
        if (a < b - 5) return 5;
        if (a < b - 11) return 7;
        return 4;
    }
    if ((margin < 3 && a < b - 7) || (margin > 2 && a < b - 2)) return 6;
    if ((margin > 1 || b - 6 <= a) && (margin < 3 || b - 2 <= a)) return margin < 2 && a < b - 2 ? 5 : 4;
    return 7;
}
}

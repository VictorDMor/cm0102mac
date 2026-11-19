// LEAGUE.CPP, block `table`: queries on the standings and fixtures. 0x104aa240 and 0x104aa560 leave r4-r10
// as the original does (league_base_volatile_readers.inc); the other routines return in r3 only.
#include "league_base_table_detail.hpp"

namespace cm::aot::league_base {
namespace st = table::row;
namespace fx = table::fixture;
using table::lbz_extsb;
using table::lha;

std::uint32_t find_standing(Cpu& c, std::uint32_t league, std::uint32_t club, std::uint32_t rows) {
    if (rows == 0) rows = c.u32(league + league::standings);
    c.set({{5, rows}, {6, rows}, {8, 0}});
    std::uint32_t pointer = rows, index = 0;  // r6, r8
    for (;;) {
        const auto tested = sext16(index);
        c.set(7, tested);
        if (!(std::int32_t(tested) < c.i16(league + competition::club_count))) break;
        if (c.u32(pointer) == club) {
            const auto row = rows + tested * league::standing_row;
            c.set(3, row);
            return row;
        }
        pointer += league::standing_row;
        ++index;
        c.set({{6, pointer}, {8, index}});
    }
    c.set(3, 0);
    return 0;
}

std::uint32_t has_standing(Cpu& c, std::uint32_t league, std::uint32_t club) {
    const auto rows = c.u32(league + league::standings);
    std::uint32_t found = 0;
    std::uint32_t pointer = rows, index = 0;
    while (std::int32_t(sext16(index)) < c.i16(league + competition::club_count)) {
        if (c.u32(pointer) == club) {
            found = rows + sext16(index) * league::standing_row;
            break;
        }
        pointer += league::standing_row;
        ++index;
    }
    // neg / or / rlwinm 1,31,31 / extsb: 1 when the pointer is nonzero.
    const std::uint32_t result = found != 0 ? 1u : 0u;
    c.set(3, result);
    return result;
}

void mark_clubs_of_competition(Cpu& c, std::uint32_t league) {
    std::uint32_t offset = 0, index = 0;
    // r5 the offset, r7 the count, r6 the league's record, r4 the count's halfword: left behind as the original's loop
    // leaves them (0x104aa340..0x104aa388); 0x104a9160 passes them on to its next virtual call.
    c.set({{7, 0}, {5, 0}});
    while (std::int32_t(sext16(index)) < c.i16(league + competition::club_count)) {
        c.set(6, c.u32(league + competition::record));
        auto record = c.u32(c.u32(league + league::standings) + offset);
        if (c.u32(record + 0x30) == c.u32(league + competition::record)) {
            c.put32(record + 0x34, c.u32(league + competition::record));
            record = c.u32(c.u32(league + league::standings) + offset);
            c.put8(record + 0x23c, index + 1);
        }
        offset += league::standing_row;
        ++index;
        c.set({{5, offset}, {7, index}});
    }
    c.set(4, sext16(index));
}

void limit_club_byte(Cpu& c, std::uint32_t league, std::uint32_t value, std::uint32_t excluded, std::uint32_t included,
                     std::uint32_t minimum) {
    const auto skip_excluded = sext8(excluded), match_included = sext8(included);  // r0, r11
    c.set({{6, 0}, {5, 0}});
    std::uint32_t r10 = c.r(10), offset = 0, index = 0;
    const auto apply = [&](std::uint32_t row) {
        // The club record's byte +0x23d moves toward `value`: down to it (minimum), or up to it.
        r10 = c.u32(row);
        c.set(10, r10);
        const auto wanted = sext8(value);
        const auto current = sext8(c.u8(r10 + 0x23d));
        if (sext8(minimum) != 0 ? std::int32_t(current) > std::int32_t(wanted) : std::int32_t(current) < std::int32_t(wanted))
            c.put8(r10 + 0x23d, value);
    };
    std::uint32_t r8 = c.r(8), r9 = c.r(9);
    for (;;) {
        r8 = lha(c, league + competition::club_count);
        r9 = sext16(index);
        if (!(std::int32_t(r9) < std::int32_t(r8))) break;
        if (std::int32_t(skip_excluded) != -3) {
            const auto row = c.u32(league + league::standings) + offset;
            if (std::int32_t(skip_excluded) != std::int32_t(sext8(c.u8(row + st::status)))) apply(row);
        }
        if (std::int32_t(match_included) != -3) {
            const auto row = c.u32(league + league::standings) + offset;
            if (std::int32_t(match_included) == std::int32_t(sext8(c.u8(row + st::status)))) apply(row);
        }
        offset += league::standing_row;
        ++index;
        c.set({{5, offset}, {6, index}});
    }
    c.set({{5, offset}, {6, index}, {8, r8}, {9, r9}, {10, r10}});
}

std::uint32_t points_for_result(Cpu& c, std::uint32_t league, std::uint32_t club, std::uint32_t fixture) {
    const auto done = [&](std::uint32_t value) { c.set(3, value); return value; };
    // The winner of a pair of scores takes the league's win points (+0xc4), the other club nothing; a draw
    // decided in extra time gives the winner the same, one decided on penalties the draw points (+0xc5).
    const auto home_score = c.u8(fixture + fx::home_goals), away_score = c.u8(fixture + fx::away_goals);  // r9, r8
    auto r7 = sext8(home_score), r6 = sext8(away_score);
    c.set({{9, home_score}, {8, away_score}, {7, r7}, {6, r6}});
    if (std::int32_t(r7) != std::int32_t(r6)) {
        if (club == c.u32(fixture + fx::home) && std::int32_t(r7) > std::int32_t(r6)) return done(c.u8(league + table::field::win_points));
        if (club == c.u32(fixture + fx::away)) {
            c.set(4, sext8(home_score));
            if (!(std::int32_t(sext8(home_score)) >= std::int32_t(sext8(away_score)))) return done(c.u8(league + table::field::win_points));
        }
        return done(0);
    }
    const auto flags = c.u16(league + table::field::ruling_flags);  // r10: halfword (lha, tested as bits)
    c.set(10, lha(c, league + table::field::ruling_flags));
    if ((flags & 2) != 0) {
        const auto extra_home = c.u8(fixture + fx::extra_home), extra_away = c.u8(fixture + fx::extra_away);
        r7 = sext8(extra_home);
        r6 = sext8(extra_away);
        c.set({{9, extra_home}, {8, extra_away}, {7, r7}, {6, r6}});
        if (std::int32_t(r7) != std::int32_t(r6)) {
            if (club == c.u32(fixture + fx::home) && std::int32_t(r7) > std::int32_t(r6)) return done(c.u8(league + table::field::win_points));
            if (club == c.u32(fixture + fx::away)) {
                c.set(4, sext8(extra_home));
                if (!(std::int32_t(sext8(extra_home)) >= std::int32_t(sext8(extra_away)))) return done(c.u8(league + table::field::win_points));
            }
            return done(0);
        }
    }
    if ((flags & 0x20) != 0 || (flags & 1) != 0) {
        const auto penalties_home = c.u8(fixture + fx::penalties_home), penalties_away = c.u8(fixture + fx::penalties_away);
        r7 = sext8(penalties_home);
        r6 = sext8(penalties_away);
        c.set({{9, penalties_home}, {8, penalties_away}, {7, r7}, {6, r6}});
        if (std::int32_t(r7) != std::int32_t(r6)) {
            if (club == c.u32(fixture + fx::home) && std::int32_t(r7) > std::int32_t(r6)) return done(c.u8(league + table::field::draw_points));
            if (club == c.u32(fixture + fx::away)) {
                c.set(4, sext8(penalties_home));
                if (!(std::int32_t(sext8(penalties_home)) >= std::int32_t(sext8(penalties_away)))) return done(c.u8(league + table::field::draw_points));
            }
            return done(0);
        }
    }
    return done(c.u8(league + table::field::draw_points));
}

std::int32_t evaluate_position(Cpu& c, std::uint32_t league, std::uint32_t row, std::uint32_t marker) {
    const auto done = [&](std::int32_t value) { c.set(3, std::uint32_t(value)); return value; };
    // The marker byte 0x14 says the evaluation is open (level on points with the club it is measured against).
    const auto open = [&](std::int32_t value) { c.put8(marker, 0x14); return done(value); };
    const auto win_points = [&] { return std::int32_t(sext8(c.u8(league + table::field::win_points))); };
    const auto points = [&](std::uint32_t at) { return std::int32_t(c.i16(at + 0x1c)); };
    const auto in_race = [&](std::uint32_t at) { return std::int32_t(sext8(c.u8(at + st::status))) == -1; };
    // The first tie-break's gap to `other` (goal difference as a signed byte, or goals scored), plus one, as a
    // signed byte: at least 1 is the answer, else -1. None when the first tie-break is another code.
    const auto gap = [&](std::uint32_t other) -> std::optional<std::int32_t> {
        const auto kind = std::int32_t(sext8(c.u8(league + league::tie_breaks)));
        const auto difference = [&](std::uint32_t at) { return sext8(lha(c, at + 0x18) - lha(c, at + 0x1a)); };
        std::int32_t value = 0;
        if (kind == table::tie_break::goal_difference) value = std::int32_t(sext8(difference(other) - difference(row) + 1));
        else if (kind == table::tie_break::goals_scored) value = std::int32_t(sext8(lha(c, other + 0x18) - lha(c, row + 0x18) + 1));
        else return std::nullopt;
        return value >= 1 ? value : -1;
    };
    const auto played = std::int32_t(sext8(c.u8(row + st::totals)));
    if (played < std::int32_t(c.i16(league + table::field::rounds_played)) - 1) return done(-1);
    if (std::int32_t(sext8(c.u8(row + st::status))) != -1) return done(-1);
    const auto clubs = std::int32_t(c.i16(league + competition::club_count));
    const auto first = c.u32(league + league::standings);
    // The leader: while the league promotes (flags bit 0) and the first row is not marked 5.
    if ((c.u16(league + league::flags) & 1) != 0 && c.u8(first + 0x40) != 5) {
        if (std::int32_t(sext8(c.u8(row + st::position))) > 0) {
            const auto reach = points(row) + win_points();
            if (reach > points(first)) return done(-1);
            if (points(first) == reach) {
                if (const auto g = gap(first)) return done(*g);
            }
        } else if (clubs > 1) {
            // The leader against the second row (row 1's points at +0x60).
            const auto reach = win_points() + std::int32_t(c.i16(first + 0x60));
            if (points(first) < reach) return open(-1);
            return points(first) == reach ? open(0) : done(-1);
        }
    }
    // Promotion places (+0xc0 automatic, +0xc1 playoff): the rows above the place count and those at it.
    const auto promotion = c.u8(league + league::automatic_promotion), playoff = c.u8(league + league::playoff_clubs);
    if (sext8(promotion) != 0 || sext8(playoff) != 0) {
        const auto places = std::int32_t(sext8(promotion + playoff));
        const auto position = std::int32_t(sext8(c.u8(row + st::position)));
        if (!(position < places)) {
            std::uint32_t offset = 0;
            for (std::uint32_t i = 0; std::int32_t(sext16(i)) < places; ++i, offset += league::standing_row) {
                const auto base = c.u32(league + league::standings);
                if (!in_race(base + offset)) continue;
                const auto bonus = win_points();
                if (points(row) + bonus > points(base + offset)) return done(-1);
                if (bonus != 0) continue;
                const auto other = base + sext16(i) * league::standing_row;
                if (const auto g = gap(other)) return done(*g);
            }
        } else {
            std::uint32_t offset = std::uint32_t(places) * league::standing_row;
            for (std::uint32_t i = std::uint32_t(places); std::int32_t(sext16(i)) < clubs; ++i, offset += league::standing_row) {
                const auto base = c.u32(league + league::standings);
                if (!in_race(base + offset)) continue;
                const auto other = base + sext16(i) * league::standing_row;
                const auto reach = win_points() + points(other);
                if (reach > points(row)) return open(-1);
                return points(row) == reach ? open(0) : done(-1);
            }
        }
    }
    // Relegation places (+0xc3, +0xc2) counted from the bottom.
    const auto relegation = c.u8(league + league::relegation), second = c.u8(league + league::byte_c2);
    if (sext8(relegation) != 0 || sext8(second) != 0) {
        const auto bound = clubs - std::int32_t(sext8(relegation + second));
        const auto position = std::int32_t(sext8(c.u8(row + st::position)));
        if (position < bound) {
            std::uint32_t offset = sext16(std::uint32_t(bound)) * league::standing_row;
            for (std::uint32_t i = sext16(std::uint32_t(bound)); std::int32_t(sext16(i)) < clubs; ++i, offset += league::standing_row) {
                const auto base = c.u32(league + league::standings);
                if (!in_race(base + offset)) continue;
                const auto other = base + sext16(i) * league::standing_row;
                const auto reach = win_points() + points(other);
                if (reach > points(row)) return open(-1);
                return points(row) == reach ? open(0) : done(-1);
            }
        } else {
            std::uint32_t offset = 0;
            for (std::uint32_t i = 0; std::int32_t(sext16(i)) < bound; ++i, offset += league::standing_row) {
                const auto base = c.u32(league + league::standings);
                if (!in_race(base + offset)) continue;
                const auto bonus = win_points();
                if (points(row) + bonus > points(base + offset)) return done(-1);
                if (bonus != 0) continue;
                const auto other = base + sext16(i) * league::standing_row;
                if (const auto g = gap(other)) return done(*g);
            }
        }
    }
    return done(-1);
}

std::uint32_t fixture_slot(Cpu& c, std::uint32_t league, std::uint32_t fixture, std::uint32_t away, std::uint32_t marker) {
    const Frame frame(c, 0x104a9aa0, 0x50, {.first_gpr = 31});
    c.set(31, marker);
    c.put8(marker, 0xff);
    const auto club = sext8(away) == 0 ? c.u32(fixture + fx::home) : c.u32(fixture + fx::away);
    // 10 + (byte +0x4b - 10) / 4, truncating toward zero (srawi + addze), in a local byte at sp+0x38.
    const auto spread = std::int32_t(sext8(c.u8(league + competition::byte_4b))) - 10;
    const auto base = std::uint32_t(10 + spread / 4);
    c.put8(frame.at(0x38), base);
    std::uint32_t found = 0xffffffffu;
    std::uint32_t offset = 0;
    for (std::uint32_t index = 0; std::int32_t(sext16(index)) < c.i16(league + competition::club_count); ++index, offset += league::standing_row) {
        if (c.u32(c.u32(league + league::standings) + offset) == club) {
            found = index;
            break;
        }
    }
    if (std::int32_t(sext16(found)) != -1 && std::int32_t(c.i16(league + table::field::rounds_played)) > 10) {
        if (c.i16(fixture + 0x34) < 3) {
            c.put8(frame.at(0x38), c.u8(frame.at(0x38)) + 1);
        } else {
            const auto row_offset = sext16(found) * league::standing_row;
            const auto row = c.u32(league + league::standings) + row_offset;
            if (std::int32_t(sext8(c.u8(row + st::status))) == -1) {
                c.put8(frame.at(0x38), c.u8(frame.at(0x38)) + 1);
                c.set({{3, league}, {4, c.u32(league + league::standings) + row_offset}, {5, frame.at(0x38)}});
                c.link(0x104a9ba4);
                c.put8(marker, std::uint32_t(evaluate_position(c, league, c.r(4), frame.at(0x38))));
            }
        }
    }
    const auto result = std::uint32_t(c.u8(frame.at(0x38)));
    c.set(3, result);
    return result;
}

}

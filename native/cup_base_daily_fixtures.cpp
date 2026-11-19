// CUP.CPP, block `daily`: a round's fixtures looked up in the fixture manager (FIX_MAN.CPP 0x1048aea0, still
// generated: eight register arguments and four stack words at sp+0x38, the last three -1) for the screens (0x1047bbb0)
// and for the manager's own two-leg bookkeeping (0x104864d0). The dates are DATE.CPP's (date.hpp); each routine copies
// the dates through frame halfwords as the original does, and the copies' stores are kept.
#include "cup_base_daily.hpp"
#include "cup_base_detail.hpp"
#include "date.hpp"

namespace cm::aot::cup_base::daily {
namespace {
namespace cell {
inline constexpr std::int32_t
    today = -0x7b80,         // cell: the current date (day halfword, year halfword)
    counter = -0x79ec,       // cell: a global byte (inferred: what the day counter 0x1047bbb0 starts from)
    fixtures = -0x7a60;      // cell: the fixture manager's object (FIX_MAN.CPP) (inferred)
}
constexpr std::uint32_t lookup = 0x1048aea0;

// The fixture manager's lookup for the cup's competition: r4-r6 the addresses of three frame values, r7 `kind`, r9
// the round, r10 `home`, the first stack word `away` (then three -1). The answer is the fixture row (0 for none).
std::uint32_t find_fixture(Cpu& c, const Frame& frame, std::uint32_t cup_object, std::uint32_t first,
                           std::uint32_t second, std::uint32_t third, std::uint32_t kind, std::uint32_t round,
                           std::uint32_t home, std::uint32_t away, std::uint32_t back) {
    c.put32(frame.at(0x38), away);
    c.put32(frame.at(0x3c), 0xffffffffu);
    c.put32(frame.at(0x40), 0xffffffffu);
    c.put32(frame.at(0x44), 0xffffffffu);
    c.set(8, c.u32(c.u32(cup_object + competition::record)));
    const auto manager = c.u32(c.toc(cell::fixtures));
    return invoke(c, lookup, back, {manager, first, second, third, kind, c.r(8), round, home});
}
}

std::uint32_t screen_fixture(Cpu& c, std::uint32_t cup_object, std::uint32_t round, std::uint32_t tie) {
    const Frame frame(c, 0x1047bbb0, 0xd0, {.first_gpr = 27});
    // r27 the cup, r28 the round, r29 the pool, r30 the message buffer, r31 the success flag (later the cell of the
    // fixture manager); r8 the date cell.
    c.set({{27, cup_object}, {28, round}, {29, c.toc(toc::pool)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::success)}, {8, c.toc(cell::today)}});
    if (std::int32_t(round) < 0) {
        report(c, frame, {0x1047bbf0, 0x98, 0x70, 0x3b9});
        c.set(3, 0);
        return 0;
    }
    if (std::int32_t(tie) < 0) {
        report(c, frame, {0x1047bc50, 0x78, 0x68, 0x3c1});
        c.set(3, 0);
        return 0;
    }
    const auto today = c.toc(cell::today);
    // The loads in the original's order (a failing one stops the run at the same address).
    const auto matches = c.u32(c.u32(cup_object + cup::round_matches) + (round << 2));
    const auto row = matches + tie * cup::match_row;
    const auto table = c.u32(cup_object + cup::entrants);
    const auto today_day = sext16(c.u16(today));
    const auto second_half = sext16(c.u16(row + 2)), first_half = sext16(c.u16(row));
    const auto away_row = table + (second_half << 3), home_row = table + (first_half << 3);
    const auto away_club = c.u32(away_row), home_club = c.u32(home_row);
    const auto today_year = c.u16(today + 2);
    const auto day = frame.at(0x4c), year = frame.at(0x4a), counter = frame.at(0x48);
    const auto counter_start = c.u8(c.toc(cell::counter));
    c.set({{3, c.toc(cell::counter)}, {5, today_day}, {7, table}, {6, first_half << 3}, {31, c.toc(cell::fixtures)},
           {29, away_club}, {30, home_club}, {4, today_year}});
    c.put16(day, today_day);
    c.put16(year, today_year);
    c.put8(counter, counter_start);
    for (;;) {
        // r29 and r30 hold the two entrants' clubs for good; their first words are read at every lookup.
        const auto away = c.u32(away_club), home = c.u32(home_club);
        const auto fixture = find_fixture(c, frame, cup_object, year, day, counter, 0, round, home, away, 0x1047bd24);
        if (fixture == 0) return 0;
        if (sext8(c.u8(fixture + 0x43)) != 0xffffffffu) return fixture;
        const auto left = c.u8(counter);
        if (left != 0) {
            c.put8(counter, left - 1);
            continue;
        }
        // The counter ran out: the day before (a day back from the lookup's date), the counter starting again at 2.
        const auto date = frame.at(0x60), earlier = frame.at(0x50);
        const std::uint32_t date_day = sext16(c.u16(day)), date_year = c.u16(year);
        c.set({{3, date}, {4, date_day}, {5, date_year}});
        c.link(0x1047bd5c);
        date::from_day_and_year(c, date, date_day, date_year);
        c.set({{3, earlier}, {4, date}, {5, 1}});
        c.link(0x1047bd6c);
        date::subtract_days(c, earlier, date, 1);
        const auto leap = c.u8(earlier + 4);
        const std::uint32_t earlier_year = c.u16(earlier + 2), earlier_day = sext16(c.u16(earlier));
        c.put16(date + 2, earlier_year);
        c.put16(date, earlier_day);
        c.put8(date + 4, leap);
        c.put16(frame.at(0x58), sext16(c.u16(date)));
        c.put16(frame.at(0x5a), c.u16(date + 2));
        c.put8(frame.at(0x5c), c.u8(date + 4));
        c.put16(day, sext16(c.u16(date)));
        c.put16(year, c.u16(date + 2));
        c.put8(counter, 2);
    }
}

void legs_into_fixtures(Cpu& c, std::uint32_t cup_object, std::uint32_t round, std::uint32_t seed, std::uint32_t extra) {
    const Frame frame(c, 0x104864d0, 0xb0, {.first_gpr = 25});
    // r27 the cup, r28 the round, r29 `seed`, r30 `extra` (the first stack word), r31 the fixture manager's cell.
    c.set({{28, round}, {27, cup_object}, {31, c.toc(cell::fixtures)}, {29, seed}, {30, extra}});
    const auto rounds = [&] { return c.u32(cup_object + cup::rounds); };
    const auto offset = sext16(round) * cup::round_row;
    const auto year_base = sext16(c.u16(cup_object + competition::year));
    const auto day_half = sext16(c.u16(rounds() + offset + 4));
    const auto round_year = (year_base + sext8(c.u8(rounds() + offset + 6))) & 0xffffu;
    // Three dates in the frame: the round's (0x80, also the scratch every copy goes through), two weeks before it
    // (0x70), the second leg's day (0x50) and the first fixture's (0x60).
    const auto base = frame.at(0x80), two_weeks_before = frame.at(0x70), next_day = frame.at(0x50);
    c.set({{7, year_base}, {4, day_half}, {3, base}, {5, round_year}});
    c.link(0x10486528);
    date::from_day_and_year(c, base, day_half, round_year);
    c.set({{3, two_weeks_before}, {4, base}, {5, 2}});
    c.link(0x10486538);
    date::subtract_weeks(c, two_weeks_before, base, 2);

    const auto year = frame.at(0x4c), day = frame.at(0x4a), counter = frame.at(0x48);
    // A lookup on the date at `from` (day halfword, year halfword, leap byte): the date goes through the scratch
    // (0x80..0x84), its halfwords are the values the manager reads by address, and the non-volatile registers hold
    // its day and year (r25/r26 the first time, r26/r29 the second) with copies at `copy_at`.
    const auto look_up = [&](std::uint32_t from, std::uint32_t copy_at, unsigned day_register, unsigned year_register,
                             std::uint32_t back) {
        const std::uint32_t from_year = c.u16(from + 2), from_day = sext16(c.u16(from)), from_leap = c.u8(from + 4);
        c.set({{4, from_year}, {3, 0}, {5, from_day}, {10, seed}, {7, from_leap}});
        c.put16(base + 2, from_year);
        c.put16(base, from_day);
        c.put8(base + 4, from_leap);
        const std::uint32_t scratch_year = c.u16(base + 2), scratch_day = sext16(c.u16(base)), scratch_leap = c.u8(base + 4);
        c.set({{4, year}, {5, day}, {6, counter}, {9, sext16(round)}, {8, scratch_day}, {7, 1}});
        c.set(day_register, scratch_day);
        c.set(year_register, scratch_year);
        c.put16(year, scratch_year);
        c.put16(day, scratch_day);
        c.put8(counter, 0);
        c.put16(copy_at, scratch_day);
        c.put16(copy_at + 2, scratch_year);
        c.put8(copy_at + 4, scratch_leap);
        return find_fixture(c, frame, cup_object, year, day, counter, 1, sext16(round), seed, extra, back);
    };
    const auto first = look_up(two_weeks_before, frame.at(0x78), 25, 26, 0x104865bc);
    c.set(6, first);
    if (first == 0) return;
    {
        const auto label = sext16(c.u16(rounds() + offset + 8));
        c.set({{5, rounds()}, {4, label}});
        c.put16(first + 0x30, label + 3);
    }
    const std::uint32_t first_year = c.u16(first + 0x28), first_day = sext16(c.u16(first + 0x2a));
    const auto fixture_date = frame.at(0x60);
    c.set({{3, fixture_date}, {4, first_day}, {5, first_year}});
    c.link(0x104865f4);
    date::from_day_and_year(c, fixture_date, first_day, first_year);
    const std::uint32_t leg_leap = c.u8(fixture_date + 4), leg_year = c.u16(fixture_date + 2), leg_day = sext16(c.u16(fixture_date));
    c.put16(base + 2, leg_year);
    c.put16(base, leg_day);
    c.put8(base + 4, leg_leap);
    const std::uint32_t copied_year = c.u16(base + 2), copied_day = sext16(c.u16(base)), copied_leap = c.u8(base + 4);
    c.put16(frame.at(0x68), copied_day);
    c.put16(frame.at(0x6a), copied_year);
    c.put8(frame.at(0x6c), copied_leap);
    c.set({{3, next_day}, {4, base}, {5, 1}, {6, copied_year}, {7, copied_day}});
    c.link(0x10486634);
    date::add_days(c, next_day, base, 1);
    const auto second = look_up(next_day, frame.at(0x58), 26, 29, 0x104866b8);
    if (second == 0) return;
    const auto label = sext16(c.u16(rounds() + offset + 8));
    c.set({{5, rounds()}, {4, label}});
    c.put16(second + 0x30, label + 4);
}

}

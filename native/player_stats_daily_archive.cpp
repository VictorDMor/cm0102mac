// PLAYER_STATS.CPP: a player's finished seasons go into the history archive
// (Stats::archive) as CareerRows, and his season records start again.
#include "player_stats_detail.hpp"

namespace cm::aot::player_stats {
using namespace detail;

std::uint32_t archive_nation(Cpu& c, std::uint32_t stats_address, std::uint32_t calendar) {
    const bool has_calendar = nonzero(c, std::int32_t(calendar));  // or. r23,r4,r4
    Frame frame(c, 0x10302b20, 0xa0, {.first_gpr = 19});
    Program program{c};
    const Kept<23> calendar_reg{c, calendar};
    const Kept<27> club_count{c, c.toc(slot::club_count)};
    const Kept<28> calendars{c, c.toc(slot::calendars)};
    const Kept<22> stats_reg{c, stats_address};
    const Kept<29> contracts{c, c.toc(slot::contracts)};
    const Kept<30> clubs{c, c.toc(slot::clubs)};
    const Kept<31> person_count{c, c.toc(slot::person_count)};
    Kept<25> club_index{c, c.toc(slot::message)};
    Kept<24> club{c, c.toc(slot::pool)};
    if (!has_calendar) {
        report(c, c.toc(slot::pool), 0x10302b6c, frame.at(0x40), frame.at(0x38), 0x396);
        return 0;
    }
    club_index = 0;
    Kept<26> club_offset{c, 0};
    while (compare(c, club_index, c.i32(club_count)) < 0) {
        club = c.u32(clubs) + club_offset;
        // The clubs come first in the table; the first national side ends it.
        if (nonzero(c, std::int8_t(program.call(0x100c7d00, 0x10302bd0, {club})))) break;
        const auto nation = std::int8_t(program.call(0x10191fd0, 0x10302be0, {club}));
        if (compare_logical(c, calendars + std::uint32_t(std::int32_t(nation)) * Calendar::size, calendar) == 0) {
            // Each player of the squad who is under contract here (or under
            // none) archives his seasons.
            Kept<20> n{c, 0};
            Kept<21> offset{c, 0};
            do {
                const Kept<19> person{c, c.u32(club + 0x98 + offset)};
                if (!nonzero_logical(c, person)) continue;
                if (!nonzero_logical(c, Person{c, person}.player())) continue;
                if (nonzero_logical(c, person) && compare(c, Person{c, person}.id(), c.i32(person_count)) >= 0) continue;
                if (nonzero_logical(c, program.call(0x10083110, 0x10302c40, {c.u32(contracts), person})) &&
                    compare_logical(c, Person{c, person}.club(), club) != 0)
                    continue;
                const auto none = std::uint32_t(-1);
                enter(c, 0x10302c74, {stats_address, person, none, 0, 0, none, 0});
                result(c, archive_seasons(c, stats_address, person, none, 0, 0, none, 0));
            } while (n = n + 1, offset = offset + 4, compare(c, n, 50) < 0);
        }
        club_offset = club_offset + Club::size;
        club_index = club_index + 1;
    }
    return 1;
}

namespace {
// The CareerRow archive_seasons builds on its stack for each season it
// writes: the year, the club's id, the record, `other_club` and the kind
// (category in the low 5 bits, status in the high 3). Bytes 2-3 and the
// three after the kind keep what the stack held. Returns the club's id.
std::uint32_t fill_row(Cpu& c, const CareerRow& row, std::uint32_t year, std::uint32_t club, std::uint32_t category,
              std::uint32_t status, std::uint32_t other_club) {
    c.put16(row.address, year);
    const std::uint32_t id = nonzero_logical(c, club) ? c.u32(club) : std::uint32_t(-1);
    row.club() = std::int32_t(id);
    row.kind() = std::uint8_t(((status << 5) & 0xe0) | (category & 0x1f));
    row.other_club() = std::int32_t(other_club);
    return id;
}
}

std::uint32_t archive_seasons(Cpu& c, std::uint32_t stats_address, std::uint32_t person, std::uint32_t other_club,
                              std::uint32_t transfer, std::uint32_t loan, std::uint32_t club_id, std::uint32_t third) {
    Frame frame(c, 0x10308350, 0xf0, {.first_gpr = 19});
    Program program{c};
    const Kept<21> calendars{c, c.toc(slot::calendars)};
    const Kept<23> stats_reg{c, stats_address};
    const Kept<22> today{c, c.toc(slot::today)};
    const Kept<24> person_reg{c, person};
    Kept<30> entry_address{c, c.toc(slot::success)};
    const Kept<25> other_club_reg{c, other_club};
    Kept<19> index{c, c.toc(slot::message)};  // later each season's index, then the history's iterator
    const Kept<26> loan_reg{c, loan};
    Kept<20> offset{c, c.toc(slot::pool)};  // later index << 5, then whether today is in the club's season
    const Kept<27> club_id_reg{c, club_id};
    Kept<29> archived{c, 0};
    // The rows' status: 2 for the first flag, 1 for the second, 3 for the
    // third, 0 for none (meanings not confirmed; the second is likely a loan).
    Kept<28> status{c, 0};
    if (nonzero(c, std::int8_t(transfer))) status = 2;
    else if (nonzero(c, std::int8_t(loan))) status = 1;
    else if (nonzero(c, std::int8_t(third))) status = 3;
    const std::uint32_t pool = c.toc(slot::pool);
    if (!nonzero_logical(c, person)) {
        report(c, pool, 0x103083d8, frame.at(0x68), frame.at(0x40), 0xf28, false);
        return 0;
    }
    const Person who{c, person};
    const std::uint32_t player = who.player();
    Kept<31> club{c, who.club()};
    if (!nonzero_logical(c, player)) {
        report(c, pool, 0x10308440, frame.at(0x48), frame.at(0x38), 0xf36, false);
        return 0;
    }
    const Stats stats{c, stats_address};
    entry_address = stats.players() + c.u32(player) * PlayerEntry::size;
    const PlayerEntry entry{c, entry_address};
    const CareerRow row{c, frame.at(0x88)};
    // Each season record goes into the archive and is cleared (it stays
    // allocated to the player).
    for (std::uint32_t k = 0; k < 6; ++k) {
        const std::uint32_t step = k * 0x134;  // the six copies of the original's code
        index = std::uint32_t(entry.season(std::int32_t(k + 1)).get());
        if (compare(c, std::int32_t(index), -1) == 0) continue;
        const std::uint32_t year = program.call(0x10192610, 0x103084a8 + step, {club});
        fill_row(c, row, year, club, k + 1, status, other_club);
        offset = index << 5;
        const std::uint32_t record = stats.seasons() + offset;
        for (std::uint32_t at = 0; at < SeasonRecord::size; at += 4) c.put32(row.address + 8 + at, c.u32(record + at));
        c.set(7, extended(c.i16(record + 0x1a)));
        program.call(0x10748ce0, 0x103085b4 + step,
                     {stats.archive(), entry.history_offset().address(), entry.history_size().address(), row.address});
        program.memset(stats.seasons() + offset, 0, SeasonRecord::size, 0x103085c8 + step);
        // Quirk: only the sixth category marks the seasons as archived.
        if (k == 5) archived = 1;
    }
    // Whether today lies inside the season of the club's nation (or the
    // nation has no calendar).
    offset = 0;
    std::uint32_t nation = std::uint32_t(-1);
    c.set(4, nation);
    if (nonzero_logical(c, club)) {
        const std::uint32_t club_nation = Club{c, club}.nation();
        c.set(3, club_nation);
        if (nonzero_logical(c, club_nation)) {
            nation = program.call(0x10191f80, 0x10308bf0, {club_nation});
            c.set(4, nation);
        }
    }
    const auto n = std::int8_t(nation);
    if (compare(c, n) < 0 || compare(c, n, std::int32_t(Calendar::count)) >= 0) offset = 1;
    else {
        const std::uint32_t at = std::uint32_t(std::int32_t(n)) * Calendar::size;
        const Calendar season{c, calendars + at};
        const std::int16_t day = c.i16(today);
        const std::uint16_t year = c.u16(today + 2);
        leave(c, {{5, at}, {6, extended(day)}, {4, season.address}});
        bool started = false;
        if (compare(c, day, season.start_day()) > 0) {
            c.set(3, year);
            started = compare_logical(c, year, season.start_year()) == 0;
        }
        if (!started) {
            leave(c, {{3, season.address}, {4, year}});
            started = compare_logical(c, year, season.start_year()) > 0;
        }
        if (started) {
            c.set(4, season.address);
            bool before_end = false;
            if (compare(c, day, season.end_day()) < 0) {
                c.set(3, year);
                before_end = compare_logical(c, year, season.end_year()) == 0;
            }
            if (!before_end) {
                leave(c, {{3, season.address}, {4, year}});
                before_end = compare_logical(c, year, season.end_year()) < 0;
            }
            if (before_end) offset = 1;
        }
    }
    // Unless a sixth-category season was archived, an empty senior-club row
    // marks the move, when the player leaves for another club, on loan, or
    // out of season / within 365 days of joining (0x10718130: days since Person::joined).
    bool empty_row;
    if (nonzero(c, std::int8_t(archived))) empty_row = false;
    else if (compare(c, std::int32_t(other_club), -1) > 0) empty_row = true;
    else if (nonzero(c, std::int8_t(loan))) empty_row = true;
    else if (compare(c, std::int32_t(club_id), -1) <= 0) empty_row = false;
    else if (!nonzero(c, std::int8_t(offset))) empty_row = true;
    else
        empty_row = compare(c, std::int32_t(program.call(routine::days_between, 0x10308cbc,
                                                         {today, c.u32(person + 0x4c), c.u32(person + 0x50)})),
                            0x16d) < 0;
    if (empty_row) {
        const std::uint32_t year = program.call(0x10192610, 0x10308ccc, {club});
        c.set(7, fill_row(c, row, year, club, category::senior_club, status, other_club));
        program.memset(row.address + 8, 0, SeasonRecord::size, 0x10308d18);
        program.call(0x10748ce0, 0x10308d2c,
                     {stats.archive(), entry.history_offset().address(), entry.history_size().address(), row.address});
    }
    // His league games and goals for the club `club_id` again, from the archive.
    if (compare(c, std::int32_t(club_id), -1) > 0 || compare(c, std::int32_t(other_club), -1) > 0) {
        entry.figure(1) = 0;
        entry.figure(2) = 0;
        index = program.call(history_file::read, 0x10308d58, {stats.archive(), std::uint32_t(entry.history_offset().get()),
                                                      std::uint32_t(entry.history_size().get())});
        if (nonzero(c, std::int32_t(index))) {
            c.put32(index + 0xc, c.u32(index + 4));
            for (;;) {
                const std::uint32_t item = program.call(list::next, 0x10308db4, {index});
                if (!nonzero_logical(c, item)) break;
                const CareerRow past{c, item};
                if (compare(c, past.club(), std::int32_t(club_id)) == 0 &&
                    compare_logical(c, past.category(), category::league) == 0) {
                    entry.figure(1) = std::int16_t(std::uint32_t(entry.figure(1).get()) + past.record().appearances());
                    const std::int16_t goals = entry.figure(2);
                    c.set(4, extended(goals));
                    entry.figure(2) = std::int16_t(std::uint32_t(goals) + past.record().tally(2));
                }
            }
            program.call(list::destroy, 0x10308dc8, {index, 1});
        }
    }
    return 1;
}
}

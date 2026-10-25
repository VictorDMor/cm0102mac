#include "player_stats_detail.hpp"
#include <optional>

// A person's career table (0x10305c60) and the sums the game reads from it.
// The table is built on demand from this season's records (Stats::seasons)
// and the person's history rows in the archive, returned in a malloc'd block
// the caller frees; nothing keeps it.
namespace cm::aot::player_stats {
using namespace detail;
namespace {
constexpr std::uint32_t season_rows = 6;        // categories 1-6, one row each
constexpr std::int32_t senior_club = 6;         // the category the per-club sums read (text::senior_club)
constexpr std::uint32_t capacity_step = 500;    // rows: the first block, and each growth
constexpr std::int32_t gap_limit = 500;         // rows after which seasons without history stop being filled in
constexpr std::uint32_t no_season = 0x7b3;      // 1971: seasons_since_club's "none found"
constexpr std::uint32_t season_bytes = season_rows * CareerRow::size;

CareerRow row_at(Cpu& c, std::uint32_t rows, std::int32_t index) {
    return {c, rows + std::uint32_t(index) * CareerRow::size};
}
// The original copies a row field by field and skips bytes 2-3.
void copy_row(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put16(to, c.u16(from));
    for (std::uint32_t offset = 4; offset < CareerRow::size; offset += 4) c.put32(to + offset, c.u32(from + offset));
}
void set_category(const CareerRow& row, std::uint32_t category) {
    row.kind() = std::uint8_t((row.kind() & 0xe0) | (category & 0x1f));
}
void set_status(const CareerRow& row, std::uint32_t status) {
    row.kind() = std::uint8_t((row.kind() & 0x1f) | ((status & 7) << 5));
}
// The table's realloc sites: each grows it by capacity_step rows when row
// `index` would not fit, and reports its own assertion when realloc fails.
struct Growth {
    std::uint32_t realloc_back, report_at;
    std::int32_t name, extension;
    std::uint32_t line;
};
constexpr Growth gap_growth{0x103060ec, 0x10306108, 0xa8, 0x48, 0xc00};
constexpr Growth season_growth{0x103062dc, 0x103062f8, 0x88, 0x40, 0xc26};
constexpr Growth history_growth{0x1030646c, 0x10306488, 0x68, 0x38, 0xc41};

// The career table's builder keeps its capacity in r25 and the block in r28.
bool make_room(Cpu& c, const Frame& frame, std::int32_t index, const Growth& growth) {
    if (compare(c, index, std::int32_t(c.r(25))) < 0) return true;
    keep(c, {{25, c.r(25) + capacity_step}});
    keep(c, {{28, Program{c}.realloc(c.r(28), c.r(25) * CareerRow::size, growth.realloc_back)}});
    if (nonzero(c, std::int32_t(c.r(28)))) return true;
    report(c, c.toc(slot::pool), growth.report_at, frame.at(growth.name), frame.at(growth.extension), growth.line, false);
    return false;
}

// The checks the per-club routines (0x10305700, 0x103058f0, 0x10309cc0) open
// with, at the same offsets from their entries; a value is what the routine
// returns.
struct Opening {
    std::uint32_t entry;
    std::uint32_t lines[3];      // the person, club and player-record assertions
    std::uint32_t player_check;  // the player-record assertion's first call
    std::uint32_t refused;       // what the routine returns when it refuses
    bool zero_at_own_club;       // seasons_since_club: 0 when the person is at the club now
};
std::optional<std::uint32_t> refuse(Cpu& c, const Frame& frame, const Opening& o, std::uint32_t person_address,
                                    std::uint32_t club) {
    const auto pool = c.toc(slot::pool);
    if (!nonzero_logical(c, person_address)) {
        report(c, pool, o.entry + 0x40, frame.at(0x94), frame.at(0x4c), o.lines[0], false);
        return o.refused;
    }
    if (!nonzero_logical(c, club)) {
        report(c, pool, o.entry + 0xa0, frame.at(0x74), frame.at(0x44), o.lines[1], false);
        return o.refused;
    }
    compare_logical(c, person_address);  // tested again (set here)
    const Person person{c, person_address};
    const std::int32_t id = person.id();
    c.set(5, std::uint32_t(id));
    if (compare(c, id, Field<std::int32_t>{c, c.toc(slot::person_count)}) >= 0) return o.refused;
    if (o.zero_at_own_club && compare_logical(c, person.club(), club) == 0) return 0;
    if (!nonzero_logical(c, person.player())) {
        report(c, pool, o.player_check, frame.at(0x54), frame.at(0x3c), o.lines[2], false);
        return o.refused;
    }
    return std::nullopt;
}

// The per-club routines' walk over each season's senior-club row, after
// career_table: `visit(row)` runs for the rows at `club` (club id >= 0). The
// registers are left as the original's loop leaves them before free.
template <class Visit>
void each_season_at(Cpu& c, std::uint32_t rows, std::int32_t count, std::uint32_t club, Visit visit) {
    std::uint32_t r4 = c.r(4), r5 = c.r(5), r6 = c.r(6);
    std::int32_t index = 0;
    for (; nonzero_logical(c, rows) && compare(c, index, count) < 0; index += std::int32_t(season_rows)) {
        const auto row = row_at(c, rows, index + senior_club - 1);
        r4 = std::uint32_t(index + senior_club - 1) * CareerRow::size;
        r6 = row.address;
        const std::int32_t at = row.club();
        r5 = std::uint32_t(at);
        if (compare(c, at, 0) < 0) continue;
        const std::int32_t wanted = Field<std::int32_t>{c, club};
        r4 = std::uint32_t(wanted);
        if (compare(c, at, wanted) != 0) continue;
        r4 = visit(row);
    }
    leave(c, {{4, r4}, {5, r5}, {6, r6}, {7, std::uint32_t(index)}});
}

// 0x10305700 and 0x103058f0: a tally of the senior-club rows at `club`.
template <class Tally>
std::uint32_t sum_at_club(Cpu& c, const Opening& o, std::uint32_t person, std::uint32_t club_address, Tally tally) {
    Frame frame(c, o.entry, 0xd0, {.first_gpr = 27});
    keep(c, {{27, c.toc(slot::success)}, {28, c.toc(slot::message)}, {29, c.toc(slot::pool)}});
    const Kept<30> club{c, club_address};
    Kept<31> total{c, 0};
    if (const auto refused = refuse(c, frame, o, person, club)) return *refused;
    const auto stats = Field<std::uint32_t>{c, c.toc(slot::player_stats)}.get();
    const auto count = frame.at(0x38);
    enter(c, o.entry + 0x17c, {stats, person, count});
    const auto rows = career_table(c, stats, person, count);
    each_season_at(c, rows, Field<std::int32_t>{c, count}, club, [&](const CareerRow& row) {
        const std::uint32_t value = tally(row.record());
        total = total + value;
        return value;
    });
    Program{c}.free(rows, o.entry + 0x1d0);
    return total;
}
}

std::uint32_t club_appearances(Cpu& c, std::uint32_t person, std::uint32_t club) {
    constexpr Opening opening{0x10305700, {0xabd, 0xac5, 0xad7}, 0x10305828, 0, false};
    return sum_at_club(c, opening, person, club, [](const SeasonRecord& r) { return r.appearances().get(); });
}

std::uint32_t club_goals(Cpu& c, std::uint32_t person, std::uint32_t club) {
    constexpr Opening opening{0x103058f0, {0xb04, 0xb0c, 0xb1e}, 0x10305a18, 0, false};
    return sum_at_club(c, opening, person, club, [](const SeasonRecord& r) { return r.tally(2).get(); });
}

std::uint32_t career_goals(Cpu& c, std::uint32_t person_address) {
    Frame frame(c, 0x10305ae0, 0xa0, {.first_gpr = 28});
    const auto pool = c.toc(slot::pool);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool}});
    Kept<28> total{c, 0};
    if (!nonzero_logical(c, person_address)) {
        report(c, pool, 0x10305b28, frame.at(0x6c), frame.at(0x44), 0xb4b, false);
        return 0;
    }
    const Person person{c, person_address};
    const std::int32_t id = person.id();
    c.set(5, std::uint32_t(id));
    if (compare(c, id, Field<std::int32_t>{c, c.toc(slot::person_count)}) >= 0) return 0;
    if (!nonzero_logical(c, person.player())) {
        report(c, pool, 0x10305bac, frame.at(0x4c), frame.at(0x3c), 0xb5d, false);
        return 0;
    }
    const auto stats = Field<std::uint32_t>{c, c.toc(slot::player_stats)}.get();
    const auto count_address = frame.at(0x38);
    enter(c, 0x10305c00, {stats, person_address, count_address});
    const auto rows = career_table(c, stats, person_address, count_address);
    const std::int32_t count = Field<std::int32_t>{c, count_address};
    // Every season's senior-club goals, whatever the club.
    std::uint32_t r4 = c.r(4);
    std::int32_t index = 0;
    while (nonzero_logical(c, rows) && compare(c, index, count) < 0) {
        r4 = row_at(c, rows, index + senior_club - 1).record().tally(2);
        index += std::int32_t(season_rows);
        total = total + r4;
    }
    leave(c, {{4, r4}, {5, std::uint32_t(index)}});
    Program{c}.free(rows, 0x10305c38);
    return total;
}

std::uint32_t seasons_since_club(Cpu& c, std::uint32_t person, std::uint32_t club_address) {
    constexpr Opening opening{0x10309cc0, {0x133f, 0x1347, 0x135f}, 0x10309dfc, std::uint32_t(-1), true};
    Frame frame(c, opening.entry, 0xd0, {.first_gpr = 27});
    keep(c, {{27, c.toc(slot::success)}, {28, c.toc(slot::message)}, {29, c.toc(slot::pool)}});
    const Kept<30> club{c, club_address};
    Kept<31> latest{c, no_season};
    if (const auto refused = refuse(c, frame, opening, person, club)) return *refused;
    const auto stats = Field<std::uint32_t>{c, c.toc(slot::player_stats)}.get();
    const auto count = frame.at(0x38);
    enter(c, 0x10309e50, {stats, person, count});
    const auto rows = career_table(c, stats, person, count);
    each_season_at(c, rows, Field<std::int32_t>{c, count}, club, [&](const CareerRow& row) {
        const std::uint32_t year = row.year();
        if (compare(c, std::int32_t(year), std::int32_t(std::uint32_t(latest))) > 0) latest = year;
        return year;
    });
    Program{c}.free(rows, 0x10309eac);
    if (compare(c, std::int32_t(std::uint32_t(latest)), std::int32_t(no_season)) == 0) return std::uint32_t(-1);
    const std::uint16_t this_year = Field<std::uint16_t>{c, c.toc(slot::today) + 2};
    return extended(std::int16_t(this_year - latest));
}

std::uint32_t career_table(Cpu& c, std::uint32_t stats_address, std::uint32_t person_address, std::uint32_t count_address) {
    Frame frame(c, 0x10305c60, 0x190, {.first_gpr = 20});
    Program program{c};
    const auto pool = c.toc(slot::pool);
    const bool has_person = nonzero(c, std::int32_t(person_address));  // or. r21,r4,r4
    keep(c, {{21, person_address}, {29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {24, count_address},
             {31, pool}, {20, stats_address}, {26, 0xff}});
    Kept<25> capacity{c, capacity_step};
    const Field<std::int32_t> count{c, count_address};
    count = 0;
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        report(c, pool, at, frame.at(name), frame.at(extension), line, false);
        return 0u;
    };
    if (!has_person) return fail(0x10305cb0, 0x108, 0x60, 0xb98);
    const Person person{c, person_address};
    const std::int32_t id = person.id();
    c.set(4, std::uint32_t(id));
    if (compare(c, id, Field<std::int32_t>{c, c.toc(slot::person_count)}) >= 0) return 0;
    Kept<23> player{c, person.player()};
    if (!nonzero_logical(c, player)) return fail(0x10305d34, 0xe8, 0x58, 0xbaa);
    Kept<28> rows{c, program.malloc(capacity_step * CareerRow::size, 0x10305d80)};
    if (!nonzero(c, std::int32_t(std::uint32_t(rows)))) return fail(0x10305d9c, 0xc8, 0x50, 0xbb2);

    // This season: one row per category from the person's season records,
    // at his club's season year. The original builds each row in a stack
    // local (part of the stack image) and stores it field by field.
    const Stats stats{c, stats_address};
    const CareerRow local{c, frame.at(0x128)};
    Kept<27> club{c, person.club()};
    {
        Kept<22> category{c, 1};
        do {
            program.memset(local.address, 0, CareerRow::size, 0x10305dfc);
            const auto year = program.call(0x10192610, 0x10305e04, {club});  // the club's season year
            local.year() = std::uint16_t(year);
            const std::uint32_t club_id = nonzero_logical(c, club) ? Field<std::uint32_t>{c, club}.get() : 0xffffffffu;
            set_category(local, category);
            set_status(local, 0);
            local.club() = std::int32_t(club_id);
            local.other_club() = -1;
            enter(c, 0x10305e54, {stats_address, player, extended(std::int8_t(std::uint32_t(category))), 0, 0xffffffffu,
                                  club_id});
            const auto record = season_record(c, stats_address, player, std::int8_t(std::uint32_t(category)));
            if (nonzero_logical(c, record))
                for (std::uint32_t offset = 0; offset < SeasonRecord::size; offset += 4)
                    c.put32(local.record().address + offset, c.u32(record + offset));
            copy_row(c, row_at(c, rows, count + std::int32_t(std::uint32_t(category)) - 1).address, local.address);
            category = category + 1;
        } while (compare(c, std::int32_t(std::uint32_t(category)), std::int32_t(season_rows)) <= 0);
    }

    // The past seasons, newest first, from the history archive: rows of one
    // season (year, club, status) go into one group of six; seasons with no
    // history between two groups get empty groups (until gap_limit rows).
    const PlayerEntry entry{c, stats.players() + std::uint32_t(Field<std::int32_t>{c, player}.get()) * PlayerEntry::size};
    Kept<21> year{c, local.year()};
    Kept<22> club_id{c, std::uint32_t(local.club().get())};
    Kept<26> status{c, c.r(26)};
    const Kept<27> history{c, program.call(history_file::read, 0x10306024,
                                           {stats.archive(), std::uint32_t(entry.history_offset().get()),
                                            std::uint32_t(entry.history_size().get())})};
    if (nonzero(c, std::int32_t(std::uint32_t(history)))) {
        c.put32(history + 0xc, c.u32(history + 8));  // the list's cursor back at its first node
        for (;;) {
            const Kept<23> next{c, program.call(0x10738e30, 0x103065c0, {history})};
            if (!nonzero(c, std::int32_t(std::uint32_t(next)))) break;
            const CareerRow from{c, next};
            const bool same_season = from.year() == (year & 0xffff) && from.club() == std::int32_t(std::uint32_t(club_id)) &&
                                     from.status() == (status & 0xff);
            if (!same_season) {
                count = count + std::int32_t(season_rows);
                year = (year - 1) & 0xffff;
                // r26 counts the categories while the groups are built.
                while ((year & 0xffff) > from.year()) {
                    for (status = 1;;) {
                        program.memset(local.address, 0, CareerRow::size, 0x10306098);
                        set_category(local, status & 0xff);
                        set_status(local, 0);
                        local.year() = std::uint16_t(year);
                        local.club() = -1;
                        local.other_club() = -1;
                        const auto index = count + std::int32_t(std::uint32_t(status)) - 1;
                        if (!make_room(c, frame, index, gap_growth)) return 0;
                        copy_row(c, row_at(c, rows, count + std::int32_t(std::uint32_t(status)) - 1).address, local.address);
                        status = status + 1;
                        if (std::int32_t(std::uint32_t(status)) > std::int32_t(season_rows)) break;
                    }
                    count = count + std::int32_t(season_rows);
                    if (count > gap_limit) break;
                    year = year - 1;
                }
                for (status = 1;;) {
                    program.memset(local.address, 0, CareerRow::size, 0x10306280);
                    local.year() = from.year().get();
                    set_category(local, status);
                    local.club() = from.club().get();
                    local.other_club() = from.other_club().get();
                    local.kind() = std::uint8_t((local.kind() & 0x1f) | (from.kind() & 0xe0));
                    const auto index = count + std::int32_t(std::uint32_t(status)) - 1;
                    if (!make_room(c, frame, index, season_growth)) return 0;
                    copy_row(c, row_at(c, rows, count + std::int32_t(std::uint32_t(status)) - 1).address, local.address);
                    status = status + 1;
                    if (std::int32_t(std::uint32_t(status)) > std::int32_t(season_rows)) break;
                }
                status = from.status();
                year = from.year();
                club_id = std::uint32_t(from.club().get());
            }
            // The history row itself, at its category's place in the group
            // (category 0 lands on the previous group's last row).
            const auto index = count + std::int32_t(from.category()) - 1;
            if (!make_room(c, frame, index, history_growth)) return 0;
            copy_row(c, row_at(c, rows, count + std::int32_t(from.category()) - 1).address, from.address);
        }
        program.call(list::destroy, 0x103065d4, {history, 1});  // the list disposed of
    }
    count = count + std::int32_t(season_rows);
    const std::int32_t total = count;  // fixed until the empty seasons are taken out

    // Each season takes the other club and status of the season before it
    // (the group six rows on); the oldest gets none.
    for (std::int32_t i = 0; i < total; ++i) {
        const auto row = row_at(c, rows, i);
        if (i + std::int32_t(season_rows) < total) {
            const auto before = row_at(c, rows, i + std::int32_t(season_rows));
            row.other_club() = before.other_club().get();
            set_status(row, before.status());
        } else {
            row.other_club() = -1;
            set_status(row, 0);
        }
    }

    // The same season at the same club in two groups (neither with another
    // club, the later not of status 1, both with a rating total): the first
    // row's tallies are added into the later one's and the first is dropped
    // (year 0). The tallies wrap as the original's bytes do.
    for (std::int32_t i = 0; i < total; ++i) {
        const auto from = row_at(c, rows, i);
        for (std::int32_t j = i + std::int32_t(season_rows); j < total; j += std::int32_t(season_rows)) {
            const auto into = row_at(c, rows, j);
            if (from.year() != into.year() || into.status() == 1 || into.other_club() != -1 || from.other_club() != -1 ||
                from.club() != into.club() || from.record().rating_total() == -1 || into.record().rating_total() == -1)
                continue;
            const auto to = into.record().address, add = from.record().address;
            for (std::uint32_t offset = 0; offset < 0xe; ++offset) c.put8(to + offset, c.u8(to + offset) + c.u8(add + offset));
            into.record().rating_total() = std::int16_t(into.record().rating_total() + from.record().rating_total());
            // The first test never holds (other_club was -1 above); kept as the original has it.
            const std::int32_t other = from.other_club();
            if (other != -1 || (from.status() == 1 && into.other_club() == -1)) {
                into.other_club() = other;
                set_status(into, from.status());
            }
            from.year() = 0;
            break;
        }
    }

    // Seasons dropped whole (year 0 on all six rows): one with no club in
    // its senior-club row, or one without games in categories 1 and 6 (no
    // other club, status 0) when he had no club, or when another season has
    // its year and the next season's club is his. The original's loop
    // registers reach its return when nothing is moved below.
    struct {
        std::uint32_t r5, r7, r8, r9, r10;
    } left{0, std::uint32_t(total + 5) * CareerRow::size, std::uint32_t(total), std::uint32_t(total + 5), c.r(10)};
    const auto played = [&](const CareerRow& row) {
        return row.record().appearances() != 0 || c.u8(row.record().address + 1) != 0;
    };
    for (std::int32_t i = 0; i < total; i += std::int32_t(season_rows)) {
        const auto first = row_at(c, rows, i), last = row_at(c, rows, i + senior_club - 1);
        left.r10 = first.address;
        bool drop = false;
        if (!played(first) && !played(last) && first.year() != 0 && first.other_club() == -1 && first.status() == 0) {
            const std::uint32_t passes = (std::uint32_t(total) + 5) / season_rows;
            left.r5 = passes;
            left.r7 = rows;
            left.r9 = 0;
            left.r8 = std::uint32_t(i) + season_rows;
            // The original compares the next season's club for every
            // season j with this year (not j's club).
            if (total > 0) {
                std::int32_t j = 0;
                for (std::uint32_t n = passes; n != 0; --n, j += std::int32_t(season_rows), left.r9 += season_bytes,
                                   left.r7 += season_bytes) {
                    if (i == j) continue;
                    const std::int32_t at = first.club();
                    left.r5 = std::uint32_t(at);
                    if (at == -1 || (row_at(c, rows, j).year() == first.year() && i + std::int32_t(season_rows) < total &&
                                     row_at(c, rows, i + std::int32_t(season_rows)).club() == at)) {
                        drop = true;
                        break;
                    }
                }
            }
        } else {
            drop = last.club() == -1;
        }
        if (drop) {
            for (std::uint32_t k = 0; k < season_rows; ++k) row_at(c, rows, i + std::int32_t(k)).year() = 0;
            left.r5 = std::uint32_t(i + 4) * CareerRow::size;
            left.r7 = std::uint32_t(i + 3) * CareerRow::size;
            left.r8 = std::uint32_t(i + 2) * CareerRow::size;
            left.r9 = 0;
        }
    }
    leave(c, {{4, std::uint32_t(total)}, {5, left.r5}, {6, std::uint32_t(total) * CareerRow::size}, {7, left.r7},
              {8, left.r8}, {9, left.r9}, {10, left.r10}});

    // Groups whose first row was dropped are taken out (BlockMoveData moves
    // each later group up by one).
    Kept<21> group{c, 0};
    while (compare(c, std::int32_t(std::uint32_t(group)), count) < 0) {
        if (!nonzero_logical(c, row_at(c, rows, std::int32_t(std::uint32_t(group))).year())) {
            Kept<22> next{c, group + season_rows};
            Kept<20> offset{c, next * CareerRow::size};
            while (compare(c, std::int32_t(std::uint32_t(next)), count) < 0) {
                program.toolbox(0x1077fc28, 0x10306a34,
                                {rows + offset, rows + (next - season_rows) * CareerRow::size, season_bytes});
                offset = offset + season_bytes;
                next = next + season_rows;
            }
            count = count - std::int32_t(season_rows);
            group = group - season_rows;
        }
        group = group + season_rows;
    }
    return rows;
}
}

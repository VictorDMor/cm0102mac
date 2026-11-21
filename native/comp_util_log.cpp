// COMP_UTIL.CPP, block `fixture_log`: the history log, a global table of 0x1c-byte rows (history_log in
// comp_util_detail.hpp: the competition's record, the winner, runner-up, third and host clubs, the year) that the
// competitions' season rollovers append to and the seeding and club screens read.
//   0x10463990 appends a row, reusing the trailing run of empty rows (a row whose +4 is 0), else growing the table
//              by 20 rows through DATABASE.CPP (0x100a4ce0);
//   0x10463a90 looks one competition up: it sorts the whole table with the DATABASE comparator, binary-searches a
//              key row (its record at +4, the year -1 for any) and answers the club of the requested place;
//   0x104770c0 copies the rows in which a club (or a second one) placed into a fresh block, sorted by 0x10477270,
//              which orders by the competition's standing and then by the club's best place;
//   0x10477270 is that comparator (a qsort comparator over 0x1c-byte rows, reached through the unit's descriptor).
// The appender, the lookup and the comparator leave r4..r10 as the original does (callers read them, some through
// virtual slots); the collector is read for r3 only (tools/register_contract.py --after).
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
namespace log = history_log;
}

void append_history(Cpu& c, std::uint32_t competition, std::uint32_t winner, std::uint32_t runner_up, std::uint32_t third, std::uint32_t host) {
    Frame frame(c, 0x10463990, 0x60, {.first_gpr = 25});
    const auto count_cell = c.toc(log::count), rows_cell = c.toc(log::rows);
    c.set({{25, count_cell}, {26, rows_cell}, {27, competition}, {28, winner}, {29, runner_up}, {30, third}, {31, host}});
    // The slot to reuse: the first row of the trailing run of empty rows. A table with no used row, or whose last
    // row is used, has none (the original then grows the table: all-empty tables too).
    // r4, r5 and r6 follow the original (callers read r4-r10 after the call through virtual slots): r5 the row
    // being tested (0 for a table with no rows), r4 its index, r6 the slot found (0 for none); r7..r10 are never
    // written here.
    std::uint32_t slot = 0;
    const auto total = c.u32(count_cell);
    // The scan is the original's loop on the table's length minus one (as a signed word) and a counter of `total`
    // steps (`mtctr`), so a length of -2^31 still enters it, with no row to read (and faults as the original does).
    std::uint32_t row = 0;
    if (std::int32_t(total) > 0) row = c.u32(rows_cell) + (total - 1) * log::row_size;
    const auto last = std::int32_t(total - 1);
    auto index = total - 1;
    c.set({{4, index}, {5, row}, {6, 0}});
    if (last >= 0) {
        for (std::uint32_t step = 0; step < total; ++step, --index, row -= log::row_size) {
            c.set({{4, index}, {5, row}});
            if (c.u32(row + log::competition) == 0) continue;
            if (std::int32_t(index) < last) slot = c.u32(rows_cell) + (index + 1) * log::row_size;
            break;
        }
        c.set({{4, index}, {5, row}});
    }
    c.set(6, slot);
    if (slot == 0) {
        const auto grown = invoke(c, callee::grow_log, 0x10463a3c, {log::grow_step});
        if (std::int8_t(grown) == 0) return;  // no memory: the row is not written
        // The first of the rows just added (r4 the table, r6 the slot, as the original loads them).
        slot = c.u32(rows_cell) + (std::uint32_t(c.i32(count_cell)) - log::grow_step) * log::row_size;
        c.set({{4, c.u32(rows_cell)}, {6, slot}});
    }
    c.put32(slot + log::competition, c.u32(competition + competition::record));
    c.put16(slot + log::year, c.u16(c.toc(log::year_date) + 2));
    c.put32(slot + log::winner, winner);
    c.put32(slot + log::runner_up, runner_up);
    c.put32(slot + log::third, third);
    c.put32(slot + log::host, host);
}

std::uint32_t history_club(Cpu& c, std::uint32_t record, std::uint32_t place_register) {
    Frame frame(c, 0x10463a90, 0xa0, {.first_gpr = 25});
    const auto vector = c.toc(log::compare), count_cell = c.toc(log::count), rows_cell = c.toc(log::rows);
    const auto buffer = c.toc(toc::message_buffer), pool = c.toc(toc::pool);
    c.set({{25, record}, {26, place_register}, {27, vector}, {28, count_cell}, {29, rows_cell}, {30, buffer}, {31, pool}});
    // The table is sorted at every call (the comparator is DATABASE's), then searched for the key row: its
    // competition record at +4, the year -1.
    invoke(c, callee::sort, 0x10463ad0, {c.u32(rows_cell), c.u32(count_cell), log::row_size, vector});
    const auto key = frame.at(0x60);
    c.put32(key + log::competition, record);
    c.put16(key + log::year, 0xffff);
    auto row = invoke(c, callee::search, 0x10463af4, {key, c.u32(rows_cell), c.u32(count_cell), log::row_size, vector});
    c.set(27, row);
    if (row == 0) return 0;
    // The search lands anywhere in the run of equal rows: walk back to its first row.
    while (row > c.u32(rows_cell)) {
        const auto before = row - log::row_size;
        c.set(27, before);
        if (invoke(c, callee::compare_log_rows, 0x10463b18, {key, before}) != 0) {
            c.set(27, row);
            break;
        }
        row = before;
    }
    switch (std::int8_t(place_register)) {
    case 0: return c.u32(row + log::winner);
    case 1: return c.u32(row + log::runner_up);
    case 2: return c.u32(row + log::third);
    default:
        // Only the three places exist: the game's assertion (line 0x196).
        guest::report_assertion(c, {0x10463b80, pool, pool + 0xe, pool + 0x1b, frame.at(0x40), frame.at(0x38), 0x196});
        c.set(4, c.toc(toc::success_flag));
        return 0;
    }
}

std::uint32_t club_history(Cpu& c, std::uint32_t club, std::uint32_t second_club, std::uint32_t out) {
    Frame frame(c, 0x104770c0, 0x90, {.first_gpr = 20});
    const auto count_cell = c.toc(log::count), rows_cell = c.toc(log::rows);
    const auto buffer = c.toc(toc::message_buffer), pool = c.toc(toc::pool);
    c.set({{21, club}, {22, second_club}, {23, out}, {29, count_cell}, {30, buffer}, {31, pool}, {24, 0}, {26, 0}, {28, 0}, {27, 0}, {20, 0}});
    c.put32(out, 0);
    std::uint32_t block = 0, capacity = 0, found = 0;
    auto row = c.u32(rows_cell);
    std::int32_t index = 0;
    for (; index < c.i32(count_cell); ++index, row += log::row_size) {
        c.set({{28, std::uint32_t(index)}, {25, row}});
        if (c.u32(row + log::competition) == 0) continue;
        const auto first = c.u32(row + log::winner), second = c.u32(row + log::runner_up), third = c.u32(row + log::third);
        const bool placed = first == club || second == club || third == club;
        const bool placed_second = second_club != 0 && (first == second_club || second == second_club || third == second_club);
        if (!placed && !placed_second) continue;
        if (found == capacity) {
            capacity += log::collect_step;
            c.set(26, capacity);
            block = invoke(c, callee::resize, 0x10477178, {block, capacity * log::row_size});
            c.set(24, block);
            if (block == 0) {
                guest::report_assertion(c, {0x10477194, pool, pool + 0xe, pool + 0x1b, frame.at(0x40), frame.at(0x38), 0x2702});
                c.set(4, c.toc(toc::success_flag));
                return 0;
            }
        }
        // The whole row, as words and the halfword year.
        const auto target = block + found * log::row_size;
        for (std::uint32_t at = 0; at < log::year; at += 4) c.put32(target + at, c.u32(row + at));
        c.put16(target + log::year, std::uint32_t(c.i16(row + log::year)));
        ++found;
        c.set({{27, found}, {20, found * log::row_size}});
    }
    // The comparator reads the two clubs from the unit's globals (the words at TOC offsets -0x3768 and -0x376c).
    c.put32(c.r(2) + std::uint32_t(log::context_first), club);
    c.put32(c.r(2) + std::uint32_t(log::context_second), second_club);
    c.set({{28, std::uint32_t(index)}, {25, row}});
    invoke(c, callee::sort, 0x10477250, {block, found, log::row_size, c.toc(log::rows_vector)});
    c.put32(out, block);
    return found;
}

std::int32_t compare_history_rows(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x10477270, 0x50, {.first_gpr = 28});  // r28..r31 and the link register (the typed calls below change it)
    // The competition record behind each row (+4): its first words (the record's continent and nation, 0 for
    // none) decide first, a row with a competition and a nation sorting before one without. r4 and r5 follow the
    // original's loads (r5 the first record, r4 the first record's nation, then the second record).
    const auto a = c.u32(first + log::competition);
    c.set(5, a);
    const auto b = c.u32(second + log::competition);
    const auto first_continent = c.u32(a + comp::record::continent), second_continent = c.u32(b + comp::record::continent);
    if (first_continent == 0 && second_continent != 0) return -1;
    const auto first_nation = c.u32(a + comp::record::nation);
    c.set(4, first_nation);
    if (first_nation == 0 && c.u32(b + comp::record::nation) != 0) return -1;
    if ((second_continent == 0 && first_continent != 0) || (c.u32(b + comp::record::nation) == 0 && first_nation != 0)) return 1;
    // Competitions the game marks as current (0x104646d0) before the others.
    if (first_nation != 0) {
        c.link(0x10477314);
        const bool first_active = competition_is_active(c, std::int32_t(c.u32(a + comp::record::id)));
        c.link(0x10477320);
        const bool second_active = competition_is_active(c, std::int32_t(c.u32(b + comp::record::id)));
        if (!first_active && second_active) return -1;
        if (!second_active && first_active) return 1;
    }
    c.set({{5, c.u32(first + log::competition)}, {4, c.u32(second + log::competition)}});
    // Then by the competition's reputation (higher first), its id and, for the same competition, by where the
    // context club placed (the winner column first) and the year (the older first).
    const std::int32_t reputation_difference = std::int32_t(c.i16(b + comp::record::reputation)) - std::int32_t(c.i16(a + comp::record::reputation));
    if (reputation_difference != 0) return reputation_difference;
    const std::int32_t id_difference = std::int32_t(c.u32(a + comp::record::id) - c.u32(b + comp::record::id));
    if (id_difference != 0) return id_difference;
    const auto context_first = c.u32(c.r(2) + std::uint32_t(log::context_first));
    const auto context_second = c.u32(c.r(2) + std::uint32_t(log::context_second));
    c.set(4, context_first);
    // The class of a row: 1 when the context club (or the second one, when there is one) won, 2 when it was
    // runner-up, else 3.
    const auto place = [&](std::uint32_t row) {
        const auto matches = [&](std::uint32_t club) { return club == context_first || (context_second != 0 && club == context_second); };
        if (matches(c.u32(row + log::winner))) return 1;
        return matches(c.u32(row + log::runner_up)) ? 2 : 3;
    };
    const std::int32_t first_place = place(first);
    c.set(5, std::uint32_t(first_place));
    const std::int32_t second_place = place(second);
    c.set(4, std::uint32_t(first_place));
    if (first_place == second_place) return std::int32_t(c.i16(first + log::year)) - std::int32_t(c.i16(second + log::year));
    // The original computes the sign of the (xor-derived) difference; for the places 1..3 it is a plain order.
    return first_place < second_place ? -1 : 1;
}
}

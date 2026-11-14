// INJURY.CPP: building and destroying the injury manager, a new game's first injuries and the manager's
// form in a saved game (injury_unit_file.hpp).
#include "injury_unit_file.hpp"
#include "injury_unit_file_detail.hpp"

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;
namespace mf = manager_file;
namespace sf = status_file;
namespace pf = person_file;
namespace fc = file_callee;

namespace {
// The manager's constructor: its one assertion report.
constexpr CheckSite history_missing{0x1018424c, 0x50, 0x38, 0xb4};
// The new-game pass: the status rows could not be allocated.
constexpr CheckSite rows_missing{0x10188664, 0x48, 0x40, 0x7b9};
}

std::uint32_t construct_manager(Cpu& c, std::uint32_t manager, std::uint32_t saved_game) {
    Frame frame(c, 0x101840e0, 0x380, {.first_gpr = 24});
    const auto pool = c.toc(toc::pool);
    const auto even_name = c.toc(file_toc::even_name), odd_name = c.toc(file_toc::odd_name);
    // r24 walks the even characters, r27 says the name resolved; r25/r26 keep the arguments, r28 the message
    // buffer, r29 the pool, r30/r31 the two name tables.
    c.set({{25, manager}, {26, saved_game}, {28, c.toc(toc::message)}, {29, pool}, {30, even_name}, {31, odd_name}});
    const auto odd_copy = frame.at(0x70), even_copy = frame.at(0x40), name = frame.at(0xcc), spec = frame.at(0x84);
    // The temp file's name is kept as two interleaved tables: the original copies both into its frame, joins
    // them into a Pascal string and asks for a file spec, again and again until the folder answers.
    for (;;) {
        for (unsigned word = 0; word < 4; ++word) c.put32(odd_copy + 4 * word, c.u32(odd_name + 4 * word));
        c.put8(odd_copy + 0x10, c.u8(odd_name + 0x10));
        for (unsigned word = 0; word < 4; ++word) c.put32(even_copy + 4 * word, c.u32(even_name + 4 * word));
        auto odd = odd_copy + 1, even = even_copy + 1, out = name + 1;
        // The walk reads the frame as the original does (past the copies it would read dead stack).
        for (; c.i8(odd) != 0; ++odd, ++even, out += 2) {
            c.put8(out, c.u8(odd));
            c.put8(out + 1, c.u8(even));
        }
        c.put8(name, c.u8(odd_copy) + c.u8(even_copy));
        c.set({{24, even}, {27, 0}});
        const auto status = std::int16_t(invoke(c, fc::make_spec, 0x101841b4,
                                                {0, 0, name, spec, c.u8(odd_copy), c.u32(odd_name + 8),
                                                 c.u32(odd_name + 4), c.u32(odd_name)}));
        c.set(2, c.u32(frame.at(0x14)));  // the glue leaves the table of contents in the linkage area
        const bool resolved = status == 0;
        if (resolved) c.set(27, 1);
        if (!resolved) {
            invoke(c, fc::copy_string, 0x101841d8, {frame.at(0x294), pool + file_text::retry_title});
            invoke(c, fc::copy_string, 0x101841e4, {frame.at(0x1cc), pool + file_text::retry_text});
            invoke(c, fc::show_dialog, 0x101841f4, {frame.at(0x294), frame.at(0x1cc), 1});
        }
        if (resolved) break;
    }
    // The history object: 8 bytes, built over the two temp-file names when the allocation gave memory.
    const auto history = invoke(c, callee::allocate_object, 0x10184204, {8});
    c.set(24, history);
    if (history != 0) {
        const auto history_name = invoke(c, fc::make_name, 0x10184218, {pool + file_text::folder, pool + file_text::history_name});
        invoke(c, fc::construct_history, 0x10184228, {history, history_name, 0x34});
    }
    c.put32(manager + mf::history, history);
    if (c.u32(manager + mf::history) == 0) {
        report_check(c, frame, history_missing);
        invoke(c, fc::exit_game, 0x10184294, {0xffffffff});
        return manager;
    }
    for (const auto field : {manager::injuries, manager::injury_count, mf::event_rows, mf::event_count, mf::event_capacity,
                             mf::report_rows, mf::report_count, mf::report_capacity, mf::notes})
        c.put32(manager + field, 0);
    if (saved_game != 0) {
        c.set({{3, manager}, {4, saved_game}});
        c.link(0x101842d8);
        if (read_from_save(c, manager, saved_game)) return manager;
    }
    c.set(3, manager);
    c.link(0x101842f0);
    give_starting_injuries(c, manager);
    return manager;
}

std::uint32_t destroy_manager(Cpu& c, std::uint32_t manager, std::uint32_t deleting) {
    Frame frame(c, 0x10184310, 0x40, {.first_gpr = 30});
    c.set({{30, manager}, {31, deleting}});
    if (manager == 0) return manager;
    if (const auto rows = c.u32(manager + manager::status_rows); rows != 0) invoke(c, fc::free_block, 0x10184340, {rows});
    if (const auto injuries = c.u32(manager + manager::injuries); injuries != 0)
        invoke(c, fc::free_block, 0x10184350, {injuries});
    if (const auto history = c.u32(manager + mf::history); history != 0) invoke(c, fc::destroy_history, 0x10184364, {history, 1});
    if (const auto events = c.u32(manager + mf::event_rows); events != 0) invoke(c, fc::free_block, 0x10184374, {events});
    if (const auto notes = c.u32(manager + mf::notes); notes != 0) invoke(c, callee::list_destroy, 0x10184388, {notes, 1});
    if (const auto reports = c.u32(manager + mf::report_rows); reports != 0) invoke(c, fc::free_block, 0x10184398, {reports});
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x101843a8, {manager});
    return manager;
}

void set_fitness_from_club_dates(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x101843d0, 0x70, {.first_gpr = 25});
    const auto today = c.toc(toc::today), people_cell = c.toc(toc::people_count);
    const auto club_total = c.toc(file_toc::club_total), clubs_reserved = c.toc(file_toc::clubs_reserved);
    const auto latest = frame.at(0x40), earliest = frame.at(0x38);
    c.set({{25, manager}, {28, clubs_reserved}, {29, club_total}, {30, people_cell}, {31, today}});
    invoke(c, fc::date_construct, 0x101843fc, {latest});
    invoke(c, fc::date_construct, 0x10184404, {earliest});
    auto club = c.u32(c.toc(toc::clubs));
    std::int32_t index = 0;
    // The bound (the club total less twice the reserved count) is read again at every club.
    for (; index < std::int32_t(c.u32(club_total) - (c.u32(clubs_reserved) << 1)); ++index, club += club_size) {
        c.set({{26, club}, {27, std::uint32_t(index)}});
        std::int32_t days = 0;
        if (std::int8_t(invoke(c, fc::club_date, 0x10184424, {club, latest, earliest})) == 0) {
            days = std::int32_t(invoke(c, fc::days_between, 0x1018443c, {latest, c.u32(today), c.u32(today + 4)}));
            if (days > 0x50) days = 0x28;
            if (days > 0x28) days = std::int8_t(0x50 - days);
        }
        for (std::int8_t slot = 0; slot < 0x32; ++slot) {
            const auto person = c.u32(club + 0x98 + std::uint32_t(slot) * 4);
            if (person == 0 || !(c.i32(person) < c.i32(people_cell))) continue;
            const std::int32_t attribute = c.i8(c.u32(person + pf::attributes) + 0x34);
            std::int32_t fitness = std::int32_t(std::uint32_t(days) - std::uint32_t(attribute / 2));  // wraps, as subf does
            fitness = fitness < 1 ? 1 : fitness > 0x28 ? 0x28 : fitness;
            c.put16(c.u32(manager + manager::status_rows) + std::uint32_t(c.i32(person)) * status::size + sf::fitness,
                    10000 - std::uint32_t(fitness) * 100);
        }
    }
    // The loop ends with the registers the last increment left.
    c.set({{26, club}, {27, std::uint32_t(index)}});
}

namespace {
// Reading the manager: where each assertion report starts (first call, buffer offsets, source line). Every
// one but the first two closes the file after the report.
constexpr CheckSite read_no_file{0x10184574, 0x2b8, 0xd0, 0x148}, read_not_opened{0x101845f0, 0x298, 0xc8, 0x150},
    read_no_rows{0x10184664, 0x278, 0xc0, 0x158}, read_rows{0x101846e0, 0x258, 0xb8, 0x160},
    read_injury_count{0x1018475c, 0x238, 0xb0, 0x168}, read_no_injuries{0x101847d8, 0x218, 0xa8, 0x171},
    read_injury{0x10184884, 0x1f8, 0xa0, 0x17f}, read_injuries{0x10184984, 0x1d8, 0x98, 0x18e},
    read_event_count{0x10184a00, 0x1b8, 0x90, 0x197}, read_no_events{0x10184a8c, 0x198, 0x88, 0x1a6},
    read_events{0x10184b08, 0x178, 0x80, 0x1ae}, read_report_count{0x10184b84, 0x158, 0x78, 0x1b7},
    read_no_reports{0x10184c10, 0x138, 0x70, 0x1c7}, read_reports{0x10184c8c, 0x118, 0x68, 0x1d0},
    read_note_count{0x10184d08, 0xf8, 0x60, 0x1d9}, read_note{0x10184dc4, 0xd8, 0x50, 0x1f0};
// Writing the manager.
constexpr CheckSite write_no_file{0x10184ea0, 0x1d8, 0x90, 0x210}, write_not_opened{0x10184f18, 0x1b8, 0x88, 0x218},
    write_rows{0x10184f88, 0x198, 0x80, 0x21f}, write_injury_count{0x10185000, 0x178, 0x78, 0x227},
    write_injuries{0x1018507c, 0x158, 0x70, 0x22f}, write_event_capacity{0x101850f4, 0x138, 0x68, 0x237},
    write_events{0x10185178, 0x118, 0x60, 0x242}, write_report_capacity{0x101851f0, 0xf8, 0x58, 0x24b},
    write_reports{0x10185274, 0xd8, 0x50, 0x257}, write_note_count{0x10185308, 0xb8, 0x48, 0x266},
    write_note{0x10185398, 0x98, 0x40, 0x278};
// The save form's sizes and the memory gauge's limit between reading injury records one at a time and in one go.
constexpr std::uint32_t status_row_form = 0x24, injury_form = 0x38, injury_form_read = 0x34, gauge_limit = 0x3e024;
}

bool read_from_save(Cpu& c, std::uint32_t manager, std::uint32_t saved_game) {
    Frame frame(c, 0x10184530, 0x300, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool), today = c.toc(toc::today), people_cell = c.toc(toc::people_count);
    // r26 keeps the manager, r27 the people count cell, r28 today, r29/r30 the success flag and message buffer
    // the reports use, r31 the pool; r25 is left to the loops below.
    c.set({{26, manager}, {27, people_cell}, {28, today}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool}});
    const auto handle_at = frame.at(0x3c), count_at = frame.at(0x38), note_at = frame.at(0x58);
    const auto latest = frame.at(0x48), earliest = frame.at(0x40);
    // The report, then the file closed: the original's way out of every failure after the open.
    const auto fail = [&](const CheckSite& site) {
        report_check(c, frame, site);
        invoke(c, fc::file_close, site.at + 0x44, {handle_at});
        return false;
    };
    const auto read = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return std::int32_t(invoke(c, fc::file_read, back, {buffer, size, count, c.u32(handle_at)}));
    };

    if (saved_game == 0) {
        report_check(c, frame, read_no_file);
        return false;
    }
    const auto file = invoke(c, fc::file_open, 0x101845d0, {saved_game, pool + file_text::file_name, 0, 0x16, 4});
    c.put32(handle_at, file);
    if (file == 0) {
        report_check(c, frame, read_not_opened);
        return false;
    }
    // One status row per person.
    c.put32(manager + manager::status_rows,
            invoke(c, fc::allocate_block, 0x10184640, {c.u32(people_cell) * status_row_form}));
    if (c.u32(manager + manager::status_rows) == 0) return fail(read_no_rows);
    if (c.i32(people_cell) != read(c.u32(manager + manager::status_rows), status_row_form, c.u32(people_cell), 0x101846c0))
        return fail(read_rows);
    if (read(manager + manager::injury_count, 4, 1, 0x10184740) != 1) return fail(read_injury_count);
    c.put32(manager + manager::injuries,
            invoke(c, fc::allocate_block, 0x101847b4, {c.u32(manager + manager::injury_count) * injury_form}));
    if (c.u32(manager + manager::injuries) == 0) return fail(read_no_injuries);

    if (std::int32_t(invoke(c, fc::memory_gauge, 0x10184830, {c.u32(c.toc(file_toc::memory_cell))})) < std::int32_t(gauge_limit)) {
        // Little memory: each record read in its first 0x34 bytes, the two lengths worked out from the dates.
        std::int32_t index = 0;
        std::uint32_t offset = 0;
        c.set({{27, 0}, {25, 0}});
        for (; index < c.i32(manager + manager::injury_count); ++index, offset += injury_form) {
            const auto record = [&] { return c.u32(manager + manager::injuries) + offset; };
            if (read(record(), injury_form_read, 1, 0x10184868) != 1) return fail(read_injury);
            invoke(c, fc::date_add_days, 0x101848e8,
                   {latest, record() + injury::started, std::uint32_t(std::int32_t(c.i16(record() + injury::days_out)))});
            const auto first = invoke(c, fc::days_between, 0x101848f8, {latest, c.u32(today), c.u32(today + 4)});
            c.put16(record() + injury::countdown_a, first);
            invoke(c, fc::date_add_days, 0x1018491c,
                   {earliest, record() + injury::started, std::uint32_t(std::int32_t(c.i16(record() + injury::days_unfit)))});
            const auto second = invoke(c, fc::days_between, 0x1018492c, {earliest, c.u32(today), c.u32(today + 4)});
            c.set({{27, std::uint32_t(index) + 1}, {25, offset + injury_form}});
            c.put16(record() + injury::countdown_b, second);
        }
    } else {
        const auto items = read(c.u32(manager + manager::injuries), injury_form, c.u32(manager + manager::injury_count), 0x10184964);
        if (c.i32(manager + manager::injury_count) != items) return fail(read_injuries);
    }

    if (read(manager + mf::event_count, 4, 1, 0x101849e4) != 1) return fail(read_event_count);
    if (c.u32(manager + mf::event_count) != 0) {
        c.put32(manager + mf::event_capacity, c.u32(manager + mf::event_count));
        c.put32(manager + mf::event_rows,
                invoke(c, fc::allocate_block, 0x10184a68, {c.u32(manager + mf::event_count) * mf::event_row_size}));
        if (c.u32(manager + mf::event_rows) == 0) return fail(read_no_events);
        const auto items = read(c.u32(manager + mf::event_rows), mf::event_row_size, c.u32(manager + mf::event_count), 0x10184ae8);
        if (c.i32(manager + mf::event_count) != items) return fail(read_events);
    }
    if (read(manager + mf::report_count, 4, 1, 0x10184b68) != 1) return fail(read_report_count);
    if (c.u32(manager + mf::report_count) != 0) {
        c.put32(manager + mf::report_capacity, c.u32(manager + mf::report_count));
        c.put32(manager + mf::report_rows,
                invoke(c, fc::allocate_block, 0x10184bec, {c.u32(manager + mf::report_count) * mf::report_row_size}));
        if (c.u32(manager + mf::report_rows) == 0) return fail(read_no_reports);
        const auto items = read(c.u32(manager + mf::report_rows), mf::report_row_size, c.u32(manager + mf::report_count), 0x10184c6c);
        if (c.i32(manager + mf::report_count) != items) return fail(read_reports);
    }
    if (read(count_at, 2, 1, 0x10184cec) != 1) return fail(read_note_count);
    if (c.i16(count_at) != 0) {
        // The list of notes is replaced by a new one holding the file's rows.
        if (const auto old = c.u32(manager + mf::notes); old != 0) invoke(c, callee::list_destroy, 0x10184d74, {old, 1});
        const auto list = invoke(c, callee::allocate_object, 0x10184d7c, {0x10});
        c.set(25, list);
        if (list != 0) invoke(c, fc::list_construct, 0x10184d88, {list});
        c.put32(manager + mf::notes, list);
        std::uint32_t note = 0;
        c.set(25, 0);
        for (; std::int16_t(note) < c.i16(count_at); ++note, c.set(25, note)) {
            if (read(note_at, mf::note_row_size, 1, 0x10184da8) != 1) return fail(read_note);
            invoke(c, fc::list_append, 0x10184e24, {c.u32(manager + mf::notes), note_at, mf::note_row_size});
        }
    }
    invoke(c, fc::file_close, 0x10184e40, {handle_at});
    return true;
}

void write_to_save(Cpu& c, std::uint32_t manager, std::uint32_t saved_game) {
    Frame frame(c, 0x10184e60, 0x210, {.first_gpr = 27});
    const auto pool = c.toc(toc::pool), people_cell = c.toc(toc::people_count);
    c.set({{27, people_cell}, {28, manager}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool}});
    const auto handle_at = frame.at(0x3c), count_at = frame.at(0x38);
    const auto fail = [&](const CheckSite& site) {
        report_check(c, frame, site);
        invoke(c, fc::file_close, site.at + 0x44, {handle_at});
    };
    const auto write = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return std::int32_t(invoke(c, fc::file_write, back, {buffer, size, count, c.u32(handle_at)}));
    };

    if (saved_game == 0) return report_check(c, frame, write_no_file);
    const auto file = invoke(c, fc::file_open, 0x10184ef8, {saved_game, pool + file_text::file_name, 1, 0x16, 4});
    c.put32(handle_at, file);
    if (file == 0) return report_check(c, frame, write_not_opened);
    if (c.i32(people_cell) != write(c.u32(manager + manager::status_rows), status_row_form, c.u32(people_cell), 0x10184f68))
        return fail(write_rows);
    if (write(manager + manager::injury_count, 4, 1, 0x10184fe4) != 1) return fail(write_injury_count);
    if (c.i32(manager + manager::injury_count) !=
        write(c.u32(manager + manager::injuries), injury_form, c.u32(manager + manager::injury_count), 0x1018505c))
        return fail(write_injuries);
    // The capacities are written where the reader reads the counts' neighbours: +0x18 and +0x24.
    if (write(manager + mf::event_capacity, 4, 1, 0x101850d8) != 1) return fail(write_event_capacity);
    if (const auto capacity = c.u32(manager + mf::event_capacity); capacity != 0) {
        const auto items = write(c.u32(manager + mf::event_rows), mf::event_row_size, capacity, 0x10185158);
        if (c.i32(manager + mf::event_capacity) != items) return fail(write_events);
    }
    if (write(manager + mf::report_capacity, 4, 1, 0x101851d4) != 1) return fail(write_report_capacity);
    if (const auto capacity = c.u32(manager + mf::report_capacity); capacity != 0) {
        const auto items = write(c.u32(manager + mf::report_rows), mf::report_row_size, capacity, 0x10185254);
        if (c.i32(manager + mf::report_capacity) != items) return fail(write_reports);
    }
    const auto notes = c.u32(manager + mf::notes);
    c.put16(count_at, notes != 0 ? c.u16(notes) : 0);
    if (write(count_at, 2, 1, 0x101852ec) != 1) return fail(write_note_count);
    if (c.i16(count_at) != 0) {
        // The list is walked from its head; a failed row reports without closing the file.
        const auto list = c.u32(manager + mf::notes);
        c.put32(list + 0xc, c.u32(list + 4));
        while (const auto row = invoke(c, fc::list_next, 0x101853e0, {c.u32(manager + mf::notes)}))
            if (write(row, mf::note_row_size, 1, 0x1018537c) != 1) return report_check(c, frame, write_note);
    }
    invoke(c, fc::file_close, 0x101853f0, {handle_at});
}

std::uint32_t may_start_injured(Cpu& c, std::uint32_t person) {
    Frame frame(c, 0x10188490, 0x60, {.first_gpr = 28});
    const auto today = c.toc(toc::today), holder = c.toc(file_toc::holder);
    c.set({{31, today}, {30, person}, {29, holder}, {28, 0}});
    const auto latest = frame.at(0x48), earliest = frame.at(0x40);
    invoke(c, fc::date_construct, 0x101884c4, {latest});
    invoke(c, fc::date_construct, 0x101884cc, {earliest});
    if (person == 0) return 1;
    bool after_today = false;
    if (c.u32(person + pf::team) != 0) {
        const auto kind = invoke(c, 0x102ae9c0, 0x101884fc,
                                 {c.u32(holder), c.u32(person + pf::team),
                                  std::uint32_t(std::int32_t(std::int8_t(c.u8(person + pf::flags) & 8)))});
        const auto date = invoke(c, 0x102af650, 0x10188510,
                                 {c.u32(holder), c.u32(c.u32(person + pf::team) + 0x2c), kind});
        // A date is two halfwords and a byte: the later is compared with today's second halfword first, then
        // (when equal) the first halfwords, by the compiler's sign trick on their xor.
        const std::uint32_t second = c.u16(date + 2), today_second = c.u16(today + 2);
        if (second > today_second) after_today = true;
        else if (second < today_second) after_today = false;
        else {
            const std::int32_t a = c.i16(date), x = a ^ std::int32_t(c.i16(today));
            after_today = (std::uint32_t((x >> 1) - (x & a)) >> 31) != 0;
        }
        c.set(28, after_today);
    }
    const auto contract = c.u32(person + pf::contract);
    if (contract == 0 || after_today || c.i32(contract + 0x14) != -1) return 0;
    if (std::int8_t(invoke(c, fc::club_date, 0x10188594, {contract, latest, earliest})) != 0) return 0;
    // The days-between question gets the word at frame +0x44, which holds the date's fifth byte written by
    // 0x10715880 above and three bytes this routine never wrote: dead stack that 0x10188cc0's frame (another
    // block) or other code left (stack-contract.txt, reader 0x101885b0, 30 stale reads). It decides the
    // result only through what 0x10718130 makes of the word, so it is read from the real frame.
    const auto days = invoke(c, fc::days_between, 0x101885b8, {today, c.u32(earliest), c.u32(frame.at(0x44))});
    return (days & 0xffff) < 0x28 ? 1 : 0;
}

void give_starting_injuries(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x101885f0, 0xb0, {.first_gpr = 16});
    const auto pool = c.toc(toc::pool), today = c.toc(toc::today), people_cell = c.toc(toc::people_count);
    const auto types = c.toc(toc::injury_types);
    c.set({{16, c.toc(toc::message)}, {20, c.toc(file_toc::status_text)}, {21, types}, {22, c.toc(sample_table::base)},
           {23, c.toc(sample_table::first)}, {24, c.toc(sample_table::last)}, {25, c.toc(sample_table::cursor)},
           {26, people_cell}, {27, today}, {28, pool}, {29, manager}, {30, 0}, {31, 0}});
    c.put32(manager + manager::status_rows,
            invoke(c, fc::allocate_block, 0x10188640, {c.u32(people_cell) * status_row_form}));
    if (c.u32(manager + manager::status_rows) == 0) return report_check(c, frame, rows_missing);

    std::uint32_t starting = 0, recurring = 0;
    auto row = c.u32(manager + manager::status_rows), person = c.u32(c.toc(toc::people));
    std::int32_t index = 0;
    for (;; ++index, row += status_row_form, person += person_size) {
        c.set({{17, std::uint32_t(index)}, {18, person}, {19, row}});
        if (!(index < c.i32(people_cell))) break;
        c.put16(row + 2, c.u16(today + 2));
        c.put16(row + sf::date, c.u16(today));
        c.put8(row + 4, c.u8(today + 4));
        c.set(6, c.u8(today + 4));  // the original's r6 here, still there at the final format call when nothing was called since
        c.put16(row + sf::fitness, 10000);
        c.put16(row + sf::fitness_b, 10000);
        c.put16(row + status::condition, 0);
        c.put32(row + status::injury, no_injury);
        c.put32(row + sf::recurring_a, no_injury);
        c.put32(row + sf::recurring_b, no_injury);
        c.put8(row + sf::recurring_type, 0xff);
        c.put8(row + sf::byte_21, 0xff);
        c.put8(row + sf::byte_22, 0x32);
        if (c.u32(person + pf::attributes) == 0) continue;
        // Draws of the shared sample table, each as the original steps it inline.
        const auto type_sample = std::int32_t(draw(c, 0x10188734));
        const auto type = std::int8_t(type_sample % 0x52);
        c.set(16, std::uint32_t(std::int32_t(type)));
        if (c.i8(types + std::uint32_t(std::int32_t(type)) * injury_type::size + 9) != 0) {
            // One in ten of the people whose type may start injured gets the type as a recurring-injury byte.
            if (std::int32_t(draw(c, 0x101887a0)) % 10 == 0) {
                c.put8(row + sf::recurring_type, std::uint8_t(type));
                c.set(31, ++starting);
            }
        }
        if (std::int32_t(draw(c, 0x10188804)) % 100 != 0) continue;
        const auto month = std::int8_t(std::int32_t(draw(c, 0x1018885c)) % 12);
        const auto injury_row = invoke_unit(c, 0x10188930, 0x101888a8, {manager, std::uint32_t(std::int32_t(month)), 0xffffffff, person});
        c.put32(frame.at(0x38), 0);  // the ninth argument of the next call, on the stack
        invoke_unit(c, 0x10188cc0, 0x101888d4, {manager, injury_row, 1, person, 0, 1, 0xffffffff, 0});
        c.set(30, ++recurring);
    }
    invoke(c, fc::format_text, 0x10188900, {c.toc(file_toc::status_text), pool + file_text::starting_count, starting});
    invoke(c, fc::format_text, 0x10188910, {c.toc(file_toc::status_text), pool + file_text::recurring_count, recurring});
}
}

// INJURY.CPP, block "course": the daily pass over injuries, the club's event queue and the sort comparators
// (injury_unit_course.hpp).
#include "injury_unit_course.hpp"
#include "injury_unit_course_detail.hpp"

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;
using namespace course;

namespace {
// Transition vectors of the sorts (TOC words): the vector word handed to the game's search and sort.
constexpr std::int32_t vector_date_b = -0x5454, vector_date_a = -0x5458, vector_events = -0x5460;
constexpr std::int32_t clubs_cell = -0x7be8;
constexpr std::uint32_t check_club = 0x100c7d00, sort_rows = 0x10768300, search_rows = 0x10775600,
                        date_construct = 0x10715880, block_move = 0x1077fc28, request_status = 0x10187a40,
                        add_days = 0x10717b20, injury_end = 0x10717890, days_between = callee::days_between,
                        allocate_bytes = 0x10775130, resize_bytes = 0x107751b0, move_event_target = 0x10188cc0;
constexpr CheckSite event_queue_full{0x1018bce0, 0x40, 0x38, 0xcea};

// The comparators are frameless and keep copies of the row's date below the stack pointer (two for
// the first test, a third when the row is not the later one); the bytes stay there for whatever runs
// next, so the C++ leaves them too.
std::int32_t order(Cpu& c, std::uint32_t key, std::uint32_t row, std::uint32_t offset) {
    const auto first = c.u32(row + offset), second = c.u32(row + offset + 4), sp = c.sp();
    c.put32(sp - 0x8, first);
    c.put32(sp - 0x18, first);
    c.put32(sp - 0x14, second);
    c.put32(sp - 0x4, second);
    if (later(c, row + offset, key + offset)) return -1;
    c.put32(sp - 0x10, first);
    c.put32(sp - 0xc, second);
    return later(c, key + offset, row + offset) ? 1 : 0;
}
// A copy of today into a date the daily pass keeps in its frame: day, year and the extra byte.
void put_date(Cpu& c, std::uint32_t at, std::uint32_t source) {
    c.put16(at + date::year, c.u16(source + date::year));
    c.put16(at + date::day, std::uint32_t(std::int32_t(c.i16(source + date::day))));
    c.put8(at + date::extra, c.u8(source + date::extra));
}
}

std::int32_t compare_due_dates_b(Cpu& c, std::uint32_t key, std::uint32_t row) { return order(c, key, row, 8); }
std::int32_t compare_due_dates_a(Cpu& c, std::uint32_t key, std::uint32_t row) { return order(c, key, row, 0); }
std::int32_t compare_event_dates(Cpu& c, std::uint32_t key, std::uint32_t row) { return order(c, key, row, 0); }

void queue_club_event(Cpu& c, std::uint32_t manager_object, std::uint32_t date_at, std::uint32_t kind_word,
                      std::uint32_t club, std::uint32_t person) {
    Frame frame(c, 0x1018bb50, 0x90, {.first_gpr = 21});
    const auto base_cell = c.toc(sample_table::base);
    c.set({{23, kind_word}, {26, base_cell}, {27, c.toc(sample_table::first)}, {21, manager_object},
           {28, c.toc(sample_table::last)}, {22, date_at}, {29, c.toc(sample_table::cursor)}, {24, club},
           {30, c.toc(toc::message)}, {25, person}, {31, c.toc(toc::pool)}});
    const auto kind = std::int8_t(kind_word);
    if (kind == 4) return;
    if (kind == 3) {
        // Kind 3 is queued one time in fifteen.
        if (std::int32_t(draw(c, 0x1018bbc0)) % 15 != 0) return;
    } else {
        // The others one time in four.
        if (std::int32_t(draw(c, 0x1018bc2c)) % 4 == 0) return;
    }
    const auto club_record = c.u32(c.toc(clubs_cell)) + club * club_size;
    if (std::int8_t(invoke(c, check_club, 0x1018bc70, {club_record})) != 0) return;

    if (c.i32(manager_object + queue::event_count) >= c.i32(manager_object + queue::event_capacity)) {
        // Room for thirty more rows; an allocation that fails is reported and nothing is queued.
        c.put32(manager_object + queue::event_capacity, c.u32(manager_object + queue::event_capacity) + 0x1e);
        const auto bytes = c.u32(manager_object + queue::event_capacity) * event::size;
        if (c.u32(manager_object + queue::event_rows) == 0)
            c.put32(manager_object + queue::event_rows, invoke(c, allocate_bytes, 0x1018bcbc, {bytes}));
        else
            c.put32(manager_object + queue::event_rows,
                    invoke(c, resize_bytes, 0x1018bca8, {c.u32(manager_object + queue::event_rows), bytes}));
        if (c.u32(manager_object + queue::event_rows) == 0) {
            report_check(c, frame, event_queue_full);
            return;
        }
    }
    const auto slot = c.u32(manager_object + queue::event_rows) + c.u32(manager_object + queue::event_count) * event::size;
    c.put16(slot + date::year, c.u16(date_at + date::year));
    c.put16(slot + date::day, std::uint32_t(std::int32_t(c.i16(date_at + date::day))));
    c.put8(slot + date::extra, c.u8(date_at + date::extra));
    c.put8(slot + event::kind, kind_word);
    c.put32(slot + event::club, club);
    c.put32(slot + event::person, person == 0 ? no_injury : c.u32(person + person::game_id));
    c.put32(manager_object + queue::event_count, c.u32(manager_object + queue::event_count) + 1);
    invoke(c, sort_rows, 0x1018bdbc,
           {c.u32(manager_object + queue::event_rows), c.u32(manager_object + queue::event_count), event::size,
            c.toc(vector_events)});
}

void run_club_events(Cpu& c, std::uint32_t manager_object) {
    Frame frame(c, 0x1018bdd0, 0xc0, {.first_gpr = 15});
    const auto today = c.toc(toc::today), people_count_cell = c.toc(toc::people_count), clubs = c.toc(clubs_cell);
    // What the original keeps in registers across the loop; at the first calls of a row these still hold the
    // previous row's values (or the caller's, before the first assignment).
    std::uint32_t club_record = c.r(16), bound = c.r(17), attempt = c.r(19), magic = c.r(30), passed = 0;
    c.set({{27, today}, {31, manager_object}, {22, c.toc(sample_table::base)}, {18, 0},
           {23, c.toc(sample_table::first)}, {24, c.toc(sample_table::last)}, {25, c.toc(sample_table::cursor)},
           {26, people_count_cell}, {15, c.toc(clubs_cell)}});
    // The date five days back, copied within the frame: day, year, extra byte at +0x68..+0x6c. The original
    // then loads the words at +0x68 and +0x6c into r28 and r29. The second word has three stack bytes
    // (+0x6d..+0x6f) this routine never wrote; they reach nothing here (r29 is stored to +0x54 and read
    // nowhere), only r29's low three bytes, which the callees save in their frames. The read stays on the frame.
    invoke(c, add_days, 0x1018be14, {frame.at(0x60), today, 5});
    c.put16(frame.at(0x68), std::uint32_t(std::int32_t(c.i16(frame.at(0x60)))));
    c.put16(frame.at(0x6a), c.u16(frame.at(0x62)));
    c.put8(frame.at(0x6c), c.u8(frame.at(0x64)));
    const auto window_word = c.u32(frame.at(0x68));
    c.set({{28, window_word}, {29, c.u32(frame.at(0x6c))}});
    const auto window_year = std::uint16_t(window_word);
    const auto window_day = std::int16_t(window_word >> 16);
    std::uint32_t row_index = 0, offset = 0;
    c.set({{20, 0}, {21, 0}});
    for (; std::int32_t(row_index) < c.i32(manager_object + queue::event_count); ++row_index, offset += event::size) {
        c.set({{20, row_index}, {21, offset}, {18, passed}});
        const auto row = c.u32(manager_object + queue::event_rows) + offset;
        const auto row_date = row + event::date;
        // The row is for a later day: the queue is sorted, so nothing after it is due.
        if (later(c, row_date, today)) break;
        // Older than the window: counted and dropped after the loop.
        if (later(window_year, window_day, c.u16(row_date + date::year), c.i16(row_date + date::day))) {
            ++passed;
            continue;
        }
        std::int32_t how_many;
        if (std::int8_t(c.u8(row + event::kind)) == 3) {
            // A kind-3 event is for its own day only.
            if (c.u16(today + date::year) != c.u16(row_date + date::year) ||
                c.i16(today + date::day) != c.i16(row_date + date::day))
                continue;
            how_many = std::int32_t(draw(c, 0x1018bf70)) % 4 + 1;
        } else {
            how_many = std::int32_t(draw(c, 0x1018bfc8)) % 3;
        }
        bound = std::uint32_t(std::int32_t(std::int8_t(how_many)));
        club_record = c.u32(clubs) + c.u32(row + event::club) * club_size;
        c.set({{17, bound}, {16, club_record}});
        if (std::int8_t(invoke(c, check_club, 0x1018c020, {club_record})) != 0) continue;
        attempt = 0;
        magic = 0x51eb851f;
        c.set({{19, attempt}, {30, magic}});
        // Up to twenty draws of one of the club's fifty squad places, until `bound` people have been moved.
        while (std::int32_t(attempt) < 0x14 && std::int8_t(bound) > 0) {
            const auto place = std::int8_t(std::int32_t(draw(c, 0x1018c058)) % 50);
            const auto person = c.u32(club_record + std::uint32_t(std::int32_t(place)) * 4 + 0x98);
            if (person != 0 && c.i32(person) < c.i32(people_count_cell)) {
                const auto current_row = c.u32(manager_object + queue::event_rows) + offset;
                const auto status_byte = c.u8(c.u32(manager_object + manager::status_rows) + c.u32(person) * status::size + status_byte::event_kind);
                const auto kind = c.u8(current_row + event::kind);
                if (std::int8_t(status_byte) != std::int8_t(kind)) {
                    c.put32(frame.at(0x38), 0);  // the call's ninth argument
                    invoke_unit(c, move_event_target, 0x1018c108,
                                {manager_object, kind, 0, person, 0, 0, c.u32(current_row + event::person), 0});
                    --bound;
                }
            }
            ++attempt;
            c.set({{19, attempt}, {17, bound}});
        }
    }
    const auto count = c.i32(manager_object + queue::event_count);
    if (passed != 0 && std::int32_t(passed) != count) {
        c.set({{16, club_record}, {17, bound}, {19, attempt}, {30, magic}, {18, passed}, {20, row_index}, {21, offset}});
        const auto rows = c.u32(manager_object + queue::event_rows);
        invoke(c, block_move, 0x1018c15c,
               {rows + passed * event::size, rows, (std::uint32_t(count) - passed) * event::size});
        c.set(2, c.u32(frame.at(0x14)));
    }
    c.put32(manager_object + queue::event_count, c.u32(manager_object + queue::event_count) - passed);
}

void daily_course_pass(Cpu& c, std::uint32_t manager_object) {
    Frame frame(c, 0x1018b2b0, 0xa0, {.first_gpr = 21});
    const auto people_cell = c.toc(toc::people), count_cell = c.toc(toc::people_count), today = c.toc(toc::today);
    c.set({{28, people_cell}, {31, manager_object}, {29, count_cell}, {25, 0}, {30, today}, {24, 0}});
    // What the original keeps in registers (its callees save them in their frames): the injury record or
    // queue row, the loop counters, the club and the person under examination.
    std::uint32_t row = c.r(21), place = c.r(22), club = c.r(23), group = 0, person = c.r(26), place_offset = c.r(27);
    bool any_due_later = false;
    auto hold = [&] { c.set({{21, row}, {22, place}, {23, club}, {24, group}, {26, person}, {27, place_offset}}); };
    const auto rows_of = [&] { return c.u32(manager_object + queue::due_rows); };
    const auto count_of = [&] { return c.u32(manager_object + queue::due_count); };
    const auto person_at = [&](std::uint32_t index) { return c.u32(people_cell) + index * person_size; };
    const auto status_of = [&](std::uint32_t who) { return c.u32(manager_object + manager::status_rows) + c.u32(who) * status::size; };

    // Sixteen groups from the end of the people table; each one's person record points at a club whose fifty
    // squad places are examined. An injury that ends today after more than five days gets its course run once.
    for (group = 0; std::int32_t(group) < 16; ++group) {
        club = c.u32(person_at(c.u32(count_cell) - group - 1) + person::club);
        c.set({{24, group}, {23, club}});
        if (club == 0) continue;
        place = 0;
        place_offset = 0;
        for (; std::int8_t(place) < 50; ++place, place_offset += 4) {
            person = c.u32(club + place_offset + 0x98);
            if (person == 0 || c.i32(person) >= c.i32(count_cell)) continue;
            const auto index = c.u32(status_of(person) + status::injury);
            if (index == no_injury) continue;
            row = injury_record(c, manager_object, index);
            const auto today_day = c.i16(today + date::day);
            const auto today_year = c.u16(today + date::year);
            hold();
            invoke(c, injury_end, 0x1018b374,
                   {frame.at(0x50), row + injury::started, std::uint32_t(std::int32_t(c.i16(row + injury::days_unfit)))});
            const bool ends_today = today_year == c.u16(frame.at(0x50) + date::year) && today_day == c.i16(frame.at(0x50) + date::day);
            if (ends_today && c.i16(row + injury::days_unfit) > 5 && c.i8(status_of(person) + status_byte::course_started) == 0) {
                c.put8(status_of(person) + status_byte::course_started, 1);
                c.link(0x1018b3fc);
                const auto days = std::int16_t(c.i16(row + injury::days_out) - c.i16(row + injury::days_unfit));
                injury_course(c, manager_object, person, 0, c.u8(row), 0, std::uint32_t(std::int32_t(days)));
            }
            invoke_unit(c, request_status, 0x1018b408, {manager_object, person});
        }
    }
    c.set(24, group);

    if (c.u32(manager_object + queue::due_rows) != 0) {
        const auto key = frame.at(0x58), key_b = frame.at(0x60);
        hold();
        invoke(c, date_construct, 0x1018b43c, {key});
        invoke(c, date_construct, 0x1018b444, {key_b});
        put_date(c, key, today);
        put_date(c, key_b, today);
        // Rows whose first date is today: the game's search finds one, the walk goes back to the first of
        // them and forward through all of them. Each is a recovery (stage 0).
        row = invoke(c, search_rows, 0x1018b480, {key, rows_of(), count_of(), due::size, c.toc(vector_date_a)});
        c.set(21, row);
        if (row != 0) {
            while (row > rows_of()) {
                row -= due::size;
                c.set(21, row);
                if (compare_due_dates_a(c, key, row) != 0) { row += due::size; break; }
            }
            c.set(21, row);
            while (row <= rows_of() + (count_of() - 1) * due::size) {
                if (compare_due_dates_a(c, key, row) != 0) break;
                place = person_at(c.u32(row + due::person));
                c.set({{22, place}, {21, row}});
                invoke(c, days_between, 0x1018b4f4, {frame.at(0x48), row + due::date_b, c.u32(row), c.u32(row + 4)});
                const auto days = std::int16_t(c.i16(frame.at(0x48)) + c.u16(frame.at(0x4a)) * 0x16d);
                c.link(0x1018b520);
                injury_course(c, manager_object, place, 0, c.u8(row + due::injury_type), c.u8(row + due::flag),
                              std::uint32_t(std::int32_t(days)));
                row += due::size;
                c.set(21, row);
            }
        }
        // Rows whose second date is today: the same walk, each a relapse (stage 1).
        row = invoke(c, search_rows, 0x1018b558, {key, rows_of(), count_of(), due::size, c.toc(vector_date_b)});
        c.set(21, row);
        if (row != 0) {
            c.set(25, 1);
            any_due_later = true;
            while (row > rows_of()) {
                row -= due::size;
                c.set(21, row);
                if (compare_due_dates_b(c, key, row) != 0) { row += due::size; break; }
            }
            c.set(21, row);
            while (row <= rows_of() + (count_of() - 1) * due::size) {
                if (compare_due_dates_b(c, key, row) != 0) break;
                c.link(0x1018b5d4);
                injury_course(c, manager_object, person_at(c.u32(row + due::person)), 1, c.u8(row + due::injury_type),
                              c.u8(row + due::flag), 0);
                row += due::size;
                c.set(21, row);
            }
        }
    }
    if (any_due_later) {
        // Leading rows whose second date lies before today are dropped.
        std::int32_t passed = 0;
        // Only reached after the second search found a row, and the game's search (0x10775600) answers only a
        // row its comparator calls equal, i.e. a row whose second date is today. The walk therefore stops at
        // the latest on that row (later(today, today) is false): it never runs off the end of the queue, and
        // `passed` never equals the count (0x1018b6a0..0x1018b6bc keeps that test, which only skips the move
        // when the first row is the one found: passed 0).
        for (std::int32_t i = 0; i < c.i32(manager_object + queue::due_count); ++i) {
            if (!later(c, today, rows_of() + std::uint32_t(i) * due::size + due::date_b)) break;
            ++passed;
        }
        c.set(21, std::uint32_t(passed));
        const auto passed16 = std::int16_t(passed);
        if (passed16 != 0 && std::int32_t(passed16) != c.i32(manager_object + queue::due_count)) {
            invoke(c, block_move, 0x1018b6b8,
                   {rows_of() + std::uint32_t(std::int32_t(passed16)) * due::size, rows_of(),
                    (count_of() - std::uint32_t(std::int32_t(passed16))) * due::size});
            c.set(2, c.u32(frame.at(0x14)));
        }
        c.put32(manager_object + queue::due_count, count_of() - std::uint32_t(std::int32_t(passed16)));
    }
}
}

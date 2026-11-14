#pragma once
// INJURY.CPP, block "course": what its files share (the queues of the injury manager, the date order the
// game's sorts and searches use, the bounded draw the progress routine inlines).
#include "injury_unit_detail.hpp"

namespace cm::aot::injury_unit::course {
// The injury manager's two queues, past the status rows and the records.
namespace queue {
// Rows of 0x18 bytes, kept sorted by the first date: the daily pass walks the rows that are due.
inline constexpr std::uint32_t due_rows = 0x10, due_count = 0x18;
// Rows of 0x14 bytes (club events), sorted by their date.
inline constexpr std::uint32_t event_rows = 0x1c, event_capacity = 0x20, event_count = 0x24;
}
// A row of the due queue.
namespace due {
inline constexpr std::uint32_t date_a = 0x00,      // the date (two words) the first search and the first walk go by
                               date_b = 0x08,      // the date the second search goes by; also what the first walk measures from
                               person = 0x10,      // index into the people table
                               flag = 0x14,        // byte
                               injury_type = 0x15; // byte
inline constexpr std::uint32_t size = 0x18;
}
// A row of the event queue.
namespace event {
inline constexpr std::uint32_t date = 0x00,    // day (signed halfword), year (halfword), a byte
                               kind = 0x08,    // byte
                               club = 0x0c,    // club index
                               person = 0x10;  // the person's id (+0x68 of the record), -1 without one
inline constexpr std::uint32_t size = 0x14;
}
// Bytes of a person's status row (0x24 bytes) beyond the ones injury_unit_detail.hpp names.
namespace status_byte {
inline constexpr std::uint32_t course_started = 0x20,  // set when the course of an injury has been run once
                               event_kind = 0x21;      // the kind of club event the person was last moved by
}
// A date as DATE.CPP keeps it: day of the year (signed halfword), year (halfword), a byte.
namespace date {
inline constexpr std::uint32_t day = 0, year = 2, extra = 4;
}
// A person record (0x70 bytes): the fields these routines read.
namespace person {
inline constexpr std::uint32_t id = 0x00, home = 0x0c, profile = 0x14, club = 0x24,
                               link_a = 0x28, link_b = 0x2c, link_c = 0x30,  // records whose id sits at +0x20
                               days_a = 0x34, days_b = 0x36,                 // the halfwords of the injury notice
                               attribute_a = 0x63, attribute_b = 0x65, attribute_c = 0x67, attribute_d = 0x69,
                               game_id = 0x68;
}

// The compiler's signed "x > y" on halfwords as the game's date compare spells it:
// the sign of ((x ^ y) >> 1) - ((x ^ y) & x).
inline bool half_greater(std::int16_t x, std::int16_t y) {
    const std::int32_t both = std::int32_t(x) ^ std::int32_t(y);
    return ((std::uint32_t(both >> 1) - std::uint32_t(both & std::int32_t(x))) >> 31) != 0;
}
// Whether the first date is later than the second: by year, then by day.
inline bool later(std::uint16_t year_a, std::int16_t day_a, std::uint16_t year_b, std::int16_t day_b) {
    if (year_a != year_b) return year_a > year_b;
    return half_greater(day_a, day_b);
}
inline bool later(const guest::Cpu& c, std::uint32_t date_a, std::uint32_t date_b) {
    return later(c.u16(date_a + date::year), c.i16(date_a + date::day), c.u16(date_b + date::year), c.i16(date_b + date::day));
}
}

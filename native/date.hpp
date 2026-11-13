#pragma once
#include "guest_abi.hpp"

// The original's DATE.CPP: a date is a day of the year from 0, the year, and
// a byte saying whether that year is a leap year (Date below). The unit builds
// dates, moves them by days and weeks, measures the distance between two, and
// writes them as text in the game's languages (date_text.cpp).
//
// Almost every other unit calls these routines, and what they leave behind is
// read later: their prologue images and scratch dates by readers of leftover
// stack (tools/stack_contract.py module DATE.CPP lists 104 reader sites), and
// r4-r10 by callers that go on to a varargs call (tools/register_contract.py
// --after). So each routine leaves its frame, the registers at its calls and
// the registers at its return exactly as the original does; the functions
// take the argument registers as the original receives them.
namespace cm::aot::date {
using guest::Cpu;

namespace entry {
constexpr std::uint32_t from_day_and_year = 0x107156b0, from_calendar = 0x107156f0, set_default = 0x10715880,
    assign_from_calendar = 0x107158a0, nearest_weekday = 0x10715a30, weekday = 0x10715b20,
    add_days = 0x10717890, add_weeks = 0x10717a10, subtract_days = 0x10717b20, subtract_weeks = 0x10717cb0,
    numeric_long_year = 0x10715bc0, numeric = 0x10715e00, weekday_and_numeric = 0x10716130,
    short_weekday_year = 0x107164f0, short_year = 0x107168a0, short_weekday = 0x10716b00,
    long_weekday_year = 0x10716eb0, long_weekday = 0x10717270, long_year = 0x10717620,
    interval = 0x10717dd0, days_between = 0x10718130, day_of_month = 0x107182f0, month = 0x10718360,
    is_leap_year = 0x10718440, time_of_day_short = 0x107184d0, time_of_day_name = 0x10718570,
    day_month_order = 0x10718650, short_weekday_name = 0x10718700, weekday_name = 0x107187d0,
    short_month_name = 0x107188a0, month_name = 0x107189d0, month_takes_other_article = 0x10718af0;
}

// A date record in game memory.
class Date {
public:
    Date(Cpu& cpu, std::uint32_t address) : c(&cpu), at(address) {}
    std::int16_t day() const { return c->i16(at); }
    std::uint16_t year() const { return c->u16(at + 2); }
    std::uint8_t leap() const { return c->u8(at + 4); }
    void set_day(std::uint32_t value) const { c->put16(at, value); }
    void set_year(std::uint32_t value) const { c->put16(at + 2, value); }
    void set_leap(std::uint32_t value) const { c->put8(at + 4, value); }
    std::uint32_t address() const { return at; }

private:
    Cpu* c;
    std::uint32_t at;
};

// Whether a year (the low 16 bits of `year`) is a leap year. Leaves r0, r3-r5,
// cr0 and the carry as the original's leaf does.
bool is_leap_year(Cpu&, std::uint32_t year);
// 1972, day 40: what a date holds before anything is assigned to it.
void set_default(Cpu&, std::uint32_t date);
void from_day_and_year(Cpu&, std::uint32_t date, std::uint32_t day, std::uint32_t year);
// A date from a day of the month (1-31), a month (0-11) and a year; with a
// weekday other than -1, moved to the nearest such weekday. `assigning` is the
// second copy of the routine (0x107158a0), which leaves r3 as it falls.
void from_calendar(Cpu&, std::uint32_t date, std::uint32_t day, std::uint32_t month, std::uint32_t year,
                   std::uint32_t weekday, bool assigning);
// `result` becomes the day nearest to `date` (at most three days away) that falls on `weekday`.
void nearest_weekday(Cpu&, std::uint32_t date, std::uint32_t weekday, std::uint32_t result);
// 0 = Monday ... 6 = Sunday; negative for a date before 1600.
std::int16_t weekday(Cpu&, std::uint32_t date);
void add_days(Cpu&, std::uint32_t result, std::uint32_t date, std::uint32_t days);
void subtract_days(Cpu&, std::uint32_t result, std::uint32_t date, std::uint32_t days);
void add_weeks(Cpu&, std::uint32_t result, std::uint32_t date, std::uint32_t weeks);
void subtract_weeks(Cpu&, std::uint32_t result, std::uint32_t date, std::uint32_t weeks);
// `result` becomes the distance between `date` and the date passed by value in
// two registers, as a count of days and a count of years.
void interval(Cpu&, std::uint32_t result, std::uint32_t date, std::uint32_t other_high, std::uint32_t other_low);
// Days from the date passed by value to `date` (negative when `date` is earlier).
std::int32_t days_between(Cpu&, std::uint32_t date, std::uint32_t other_high, std::uint32_t other_low);
// 1-based day of the month and 0-based month, from the table of month starts
// the leap byte selects; -1 when the day is before every start.
std::int16_t day_of_month(Cpu&, std::uint32_t date);
std::int8_t month(Cpu&, std::uint32_t date);
// Unnamed neighbour of the unit (no source name of its own; four screens and
// NEWS.CPP call it): for language 9, whether a month is outside June-July;
// otherwise whether the language is 3.
std::int8_t month_takes_other_article(Cpu&, std::uint32_t language, std::uint32_t month);

// What the weekday computation leaves behind besides its answer; the text
// routines repeat it inline and leave the parts in other registers.
struct WeekdayParts {
    std::uint32_t four_centuries, century_sign, by_400, by_400_sign, days, week_sign, left_in_r0;
    std::int16_t weekday;
};
WeekdayParts weekday_parts(Cpu&, std::uint32_t year, std::int8_t leap, std::uint32_t day);

// Text (date_text.cpp). Each returns the address of a static buffer of the unit.
// The time of day (0 morning, 1 afternoon, 2 evening; anything else: none) as
// "AM"/"PM"/"EVE" and as a word, capitalised or not.
std::uint32_t time_of_day_short(Cpu&, std::uint32_t time);
std::uint32_t time_of_day_name(Cpu&, std::uint32_t time, std::uint32_t capitalised);
// How the current language orders a date's parts (0-3), for a caller's style.
std::uint32_t day_month_order(Cpu&, std::uint32_t style);
// Weekday and month names, short and full, through the routine's jump table.
std::uint32_t name(Cpu&, std::uint32_t routine, std::uint32_t index);
// The nine formatters (date, buffer, capacity, time of day): 1 when written,
// 0 after an assertion on a null buffer or a capacity below the format's minimum.
std::uint32_t format(Cpu&, std::uint32_t routine);

// A call of one of the unit's routines from other C++ at the original call
// site whose next instruction is `back`, with the argument registers set:
// direct, or through the dispatcher where a check run wants to see the call.
void call(Cpu&, std::uint32_t entry, std::uint32_t back);
}

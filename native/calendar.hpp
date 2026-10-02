#pragma once
#include <bit>
#include <cstdint>

namespace cm::calendar {
struct Date { std::int16_t day; std::uint16_t year; };
enum class Direction { forward, backward };
constexpr bool is_leap(std::uint16_t year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
constexpr std::int16_t narrow_day(int day) {
    return std::bit_cast<std::int16_t>(std::uint16_t(day));
}
constexpr unsigned year_length(std::uint16_t year) { return is_leap(year) ? 366 : 365; }
// The original stores the initial sum/difference in a signed 16-bit day field.
// This deliberately retains wrapping and one-direction-only normalization.
constexpr Date shift_within_record(Date date, std::uint16_t magnitude, Direction direction) {
    date.day = narrow_day(int(date.day) + (direction == Direction::forward ? int(magnitude) : -int(magnitude)));
    return date;
}
constexpr bool crosses_year(Date date, Direction direction) {
    return direction == Direction::forward ? date.day >= int(year_length(date.year)) : date.day < 0;
}
constexpr Date cross_year(Date date, Direction direction) {
    if (direction == Direction::forward) {
        date.day = narrow_day(int(date.day) - int(year_length(date.year)));
        ++date.year;
    } else {
        --date.year;
        date.day = narrow_day(int(date.day) + int(year_length(date.year)));
    }
    return date;
}
constexpr Date shift(Date date, std::uint16_t magnitude, Direction direction) {
    date = shift_within_record(date, magnitude, direction);
    while (crosses_year(date, direction)) date = cross_year(date, direction);
    return date;
}
}

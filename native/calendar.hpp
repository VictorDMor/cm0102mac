#pragma once
#include <bit>
#include <cstdint>
#include <span>

namespace cm::calendar {
struct Date { std::int16_t day; std::uint16_t year; };
enum class Direction { forward, backward };
constexpr bool recognizes_month(std::int16_t day, std::int16_t previous_last_day) {
    return day > previous_last_day;
}
// Tables contain the last day before each zero-based month, including -1
// before January. Keep invalid-day and mutable-table behavior explicit.
constexpr int month_index(std::int16_t day, std::span<const std::int16_t, 12> thresholds) {
    for (int month = 11; month >= 0; --month)
        if (recognizes_month(day, thresholds[month])) return month;
    return -1;
}
constexpr bool is_leap(std::uint16_t year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
constexpr std::int16_t narrow_day(int day) {
    return std::bit_cast<std::int16_t>(std::uint16_t(day));
}
constexpr unsigned year_length(std::uint16_t year) { return is_leap(year) ? 366 : 365; }
constexpr bool is_after(Date date, Date reference) {
    return date.year > reference.year || (date.year == reference.year && date.day > reference.day);
}
constexpr bool is_before(Date date, Date reference) {
    return is_after(reference, date);
}
constexpr bool difference_second_later(Date first, Date second) {
    return is_after(second, first);
}
// The ascending original loop deliberately also uses current year minus one.
// Thus cross-year difference is not generally antisymmetric or Gregorian.
constexpr std::int32_t difference_year_step(std::uint16_t current, bool second_later) {
    const auto length = std::int32_t(year_length(std::uint16_t(current - 1)));
    return second_later ? -length : length;
}
constexpr std::int32_t difference(Date first, Date second) {
    const bool later = difference_second_later(first, second);
    std::int32_t result = int(first.day) - int(second.day);
    for (auto year = first.year; year != second.year; ) {
        result += difference_year_step(year, later);
        year = std::uint16_t(int(year) + (later ? 1 : -1));
    }
    return result;
}
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

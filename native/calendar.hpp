#pragma once
#include <bit>
#include <cstdint>
#include <span>

namespace cm::calendar {
struct Date { std::int16_t day; std::uint16_t year; };
// The Mac epoch uses truncating year divisions and the stored leap flag.
// Negative remainders deliberately survive; zero denotes Sunday (6).
constexpr std::int32_t weekday_days(Date date, std::uint8_t flag) {
    const auto years = std::int32_t(date.year) - 1600;
    return 365 * years + years / 4 - years / 100 + years / 400 + date.day - (flag != 0);
}
constexpr std::int16_t weekday(Date date, std::uint8_t flag) {
    const auto remainder = weekday_days(date, flag) % 7;
    return std::int16_t(remainder > 0 ? remainder - 1 : remainder == 0 ? 6 : remainder);
}
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
// Day-of-month is one-based on valid dates, and wraps to signed16 without
// clamping. The adapter separately preserves the search and selected reload.
constexpr std::int16_t day_from_threshold(std::int16_t day, std::int16_t threshold) {
    return narrow_day(int(day) - int(threshold));
}
constexpr std::int16_t day_of_month(std::int16_t day, std::span<const std::int16_t, 12> thresholds) {
    const auto month = month_index(day, thresholds);
    return month < 0 ? -1 : day_from_threshold(day, thresholds[month]);
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

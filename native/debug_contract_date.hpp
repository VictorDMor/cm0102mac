#pragma once
#include "calendar.hpp"
#include <cstdint>
#include <optional>

namespace cm::aot {
// The game's contract date is a zero-based day of year and a separate leap
// flag. Extend by a calendar year, clamping February 29 to February 28.
inline std::optional<calendar::Date> extend_contract_date(calendar::Date date) {
    if (date.year == 0 || date.year == 0xffff || date.day < 0 ||
        unsigned(date.day) >= calendar::year_length(date.year)) return {};
    const bool was_leap = calendar::is_leap(date.year);
    const auto next_year = std::uint16_t(date.year + 1);
    const bool next_leap = calendar::is_leap(next_year);
    auto day = date.day;
    if (was_leap && !next_leap && day >= 59) --day;
    if (!was_leap && next_leap && day >= 59) ++day;
    return calendar::Date{day, next_year};
}
}

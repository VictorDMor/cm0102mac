#pragma once
// Shared by the files of DATE.CPP (date.cpp, date_text.cpp).
#include "date.hpp"
#include "guest_values.hpp"

namespace cm::aot::date {
using guest::extended;
namespace reg = guest::reg;

// Table-of-contents slots: the month starts of an ordinary and of a leap year
// (twelve signed words, the day before each month's first), and the unit's
// function-local static dates with their guard bytes (relative to r2).
namespace slot {
constexpr std::int32_t month_starts = -0x3fa8, leap_month_starts = -0x3fa4, pool = -0x3fa0;
constexpr std::int32_t success_flag = -0x7bac, message = -0x7ba8;
struct Static { std::int32_t date, guard; };
constexpr Static add_days{-0x3fac, -0x35bb}, add_weeks{-0x3fb0, -0x35bc}, subtract_days{-0x3fb4, -0x35bd},
    subtract_weeks{-0x3fb8, -0x35be}, interval{-0x3fbc, -0x35bf}, days_between{-0x3fc0, -0x35c0};
}

inline std::uint32_t high_product(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(a)) * std::int32_t(b)) >> 32);
}
// srawi: the carry records a negative value that lost one bits.
inline std::uint32_t shift_right(Cpu& c, std::uint32_t value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, std::int32_t(value) < 0 && (value & ((1u << bits) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> bits);
}
inline std::uint32_t sign_bit(std::uint32_t value) { return value >> 31; }

struct Assertion { std::uint32_t at, name, extension, line; };
// The unit's assert macro: reports "DATE.CPP <line>" and clears the shared success flag.
inline void report(Cpu& c, guest::Frame& frame, Assertion a) {
    const auto pool = c.toc(slot::pool);
    guest::report_assertion(c, {.at = a.at, .file = pool, .format = pool + 9, .title = pool + 0x16,
                                .name = frame.at(std::int32_t(a.name)), .extension = frame.at(std::int32_t(a.extension)),
                                .line = a.line});
}
}

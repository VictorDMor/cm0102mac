// DATE.CPP: building dates, moving them, measuring between them (date.hpp).
#include "date.hpp"
#include "date_detail.hpp"

namespace cm::aot::date {
namespace {
// `bl 0x10718440` and the `extsb. r0,r3` every caller follows it with.
bool leap(Cpu& c, std::uint32_t year, std::uint32_t back) {
    c.link(back);
    const bool answer = is_leap_year(c, year);
    c.set(0, answer);
    c.compare(0, answer, 0);
    return answer;
}
// A function-local static Date: built as the default date on first use.
Date static_date(Cpu& c, slot::Static which) {
    const Date date(c, c.toc(which.date));
    const auto guard = c.r(2) + std::uint32_t(which.guard);
    const auto built = c.i8(guard);
    c.set(0, extended(built));
    c.compare(0, built, 0);
    if (!built) {
        date.set_day(40);
        date.set_year(1972);
        date.set_leap(1);
        c.put8(guard, 1);
        c.set({{3, 1972}, {0, 1}});
    }
    return date;
}
void copy(Date to, Date from) {
    to.set_day(extended(from.day()));
    to.set_year(from.year());
    to.set_leap(from.leap());
}
// "first is later than second" for two days of one year, as the original
// computes it without a branch; leaves the carry of its shift.
bool later_day(Cpu& c, std::int16_t first, std::int16_t second, std::uint32_t& half) {
    const auto difference = extended(first) ^ extended(second);
    half = shift_right(c, difference, 1);
    return sign_bit(half - (difference & extended(first))) != 0;
}

}

bool is_leap_year(Cpu& c, std::uint32_t value) {
    const std::uint32_t year = value & 0xffff, odd = value & 1;
    c.set({{5, year}, {3, 0}, {0, odd}});
    c.compare(0, std::int32_t(odd), 1);
    if (odd) return false;
    const std::uint32_t by_four = year & 3;
    c.set(0, by_four);
    c.compare(0, std::int32_t(by_four), 0);
    if (by_four) return false;
    const auto scaled = high_product(0x51eb851f, year);
    c.set(4, scaled);
    const auto by_hundred = year - shift_right(c, scaled, 5) * 100;
    c.set(0, by_hundred);
    c.compare(0, std::int32_t(by_hundred), 0);
    if (by_hundred) {
        c.set(3, 1);
        return true;
    }
    const auto by_four_hundred = year - shift_right(c, scaled, 7) * 400;
    c.set(0, by_four_hundred);
    c.compare(0, std::int32_t(by_four_hundred), 0);
    c.set(3, by_four_hundred == 0);
    return by_four_hundred == 0;
}

void set_default(Cpu& c, std::uint32_t address) {
    const Date date(c, address);
    date.set_day(40);
    date.set_year(1972);
    date.set_leap(1);
    c.set({{0, 1}, {4, 1972}});
}

void from_day_and_year(Cpu& c, std::uint32_t address, std::uint32_t day, std::uint32_t year) {
    guest::Frame frame(c, entry::from_day_and_year, 0x40, {.first_gpr = 31});
    const Date date(c, address);
    date.set_day(day);
    date.set_year(year);
    c.link(0x107156d4);
    date.set_leap(is_leap_year(c, year));
    c.set(3, address);
}

void from_calendar(Cpu& c, std::uint32_t address, std::uint32_t day, std::uint32_t month, std::uint32_t year,
                   std::uint32_t weekday, bool assigning) {
    const auto routine = assigning ? entry::assign_from_calendar : entry::from_calendar;
    guest::Frame frame(c, routine, 0xb0, {.first_gpr = 24});
    const Date date(c, address);
    // What the callees' prologues save: the arguments and the unit's shared addresses.
    c.set({{30, day}, {26, c.toc(slot::success_flag)}, {27, c.toc(slot::message)}, {29, address},
           {28, c.toc(slot::pool)}, {24, month}, {25, year}, {31, weekday}});
    const auto site = [&](std::uint32_t at) { return at + (assigning ? 0x1b0u : 0u); };
    const std::uint32_t first_line = assigning ? 0x72 : 0x34;

    auto day_of_month = std::int16_t(day);
    c.compare(0, day_of_month, 1);
    if (day_of_month >= 1) c.compare(0, day_of_month, 31);
    if (day_of_month < 1 || day_of_month > 31) {
        report(c, frame, {site(0x10715748), 0x68, 0x40, first_line});
        c.set(30, 1);
        day_of_month = 1;
    }
    auto month_index = std::int8_t(month);
    c.compare(0, month_index, 0);
    if (month_index >= 0) c.compare(0, month_index, 11);
    if (month_index < 0 || month_index > 11) {
        report(c, frame, {site(0x107157b0), 0x48, 0x38, first_line + 7});
        c.set(24, 0);
        month_index = 0;
    }
    date.set_year(year);
    const bool leap_year = leap(c, year, site(0x107157fc));
    date.set_leap(leap_year);
    const auto starts = c.toc(leap_year ? slot::leap_month_starts : slot::month_starts);
    c.set(3, starts);
    // The day register in full: only its low half is stored.
    date.set_day(c.r(30) + extended(c.i16(starts + std::uint32_t(month_index) * 2)));
    const auto wanted = std::int16_t(weekday);
    c.compare(0, wanted, -1);
    if (wanted != -1) {
        c.set({{3, address}, {4, weekday}, {5, address}});
        c.link(site(0x10715864));
        nearest_weekday(c, address, weekday, address);
    }
    if (!assigning) c.set(3, address);
}

void nearest_weekday(Cpu& c, std::uint32_t address, std::uint32_t wanted, std::uint32_t result_address) {
    guest::Frame frame(c, entry::nearest_weekday, 0x60, {.first_gpr = 29});
    const Date date(c, address), result(c, result_address);
    c.set({{31, result_address}, {30, wanted}, {29, address}});
    c.link(0x10715a58);
    const auto current = weekday(c, address);
    const auto year = date.year();
    c.set({{4, year}, {3, extended(current)}});
    result.set_year(year);
    c.compare(0, current, std::int16_t(wanted));
    result.set_day(extended(date.day()));
    result.set_leap(date.leap());
    if (current == std::int16_t(wanted)) return;
    // At most three days either way: a difference of four or more goes round the week.
    auto away = std::int16_t(extended(current) - wanted);
    c.compare(0, away, 0);
    const bool forward = away < 0;
    if (forward) {
        c.compare(0, away, -3);
        if (away < -3) away = std::int16_t(away + 7);
    } else {
        c.compare(0, away, 3);
        if (away > 3) away = std::int16_t(away - 7);
    }
    // Subtracting a negative count moves forward.
    const Date moved(c, frame.at(forward ? 0x40 : 0x38));
    c.set({{4, result_address}, {3, moved.address()}, {5, extended(away)}});
    c.link(forward ? 0x10715aac : 0x10715ae8);
    subtract_days(c, moved.address(), result_address, extended(away));
    result.set_year(moved.year());
    result.set_day(extended(moved.day()));
    result.set_leap(moved.leap());
}

WeekdayParts weekday_parts(Cpu& c, std::uint32_t year, std::int8_t leap_year, std::uint32_t day) {
    // Days since the start of 1600, with the compiler's divisions by 100, 400 and 4.
    const std::uint32_t years = year - 1600;
    const auto scaled = high_product(0x51eb851f, years);
    const auto by_400 = shift_right(c, scaled, 7), by_100 = shift_right(c, scaled, 5);
    // srawi then addze: the shift's own carry rounds the quotient toward zero.
    auto by_4 = shift_right(c, years, 2);
    by_4 += std::uint32_t(c.s.r<1>(reg::xer_ca));
    WeekdayParts parts{};
    parts.century_sign = sign_bit(by_100);
    parts.by_400 = by_400;
    parts.by_400_sign = sign_bit(by_400);
    parts.four_centuries = by_400 + parts.by_400_sign;
    parts.days = by_4 - (by_100 + parts.century_sign) + years * 365 + parts.four_centuries + day;
    if (leap_year) parts.days -= 1;
    auto weeks = shift_right(c, high_product(0x92492493, parts.days) + parts.days, 2);
    parts.week_sign = sign_bit(weeks);
    weeks += parts.week_sign;
    const auto remainder = parts.days - weeks * 7;
    parts.weekday = std::int16_t(remainder);
    c.compare(0, parts.weekday, 0);
    if (parts.weekday > 0) {
        parts.left_in_r0 = remainder;
        parts.weekday = std::int16_t(parts.weekday - 1);
    } else {
        parts.left_in_r0 = extended(parts.weekday);
        if (parts.weekday == 0) parts.weekday = 6;  // Sunday
    }
    return parts;
}

std::int16_t weekday(Cpu& c, std::uint32_t address) {
    const Date date(c, address);
    const std::uint32_t year = date.year();
    const auto leap_year = std::int8_t(date.leap());
    const auto parts = weekday_parts(c, year, leap_year, extended(date.day()));
    c.set({{0, parts.left_in_r0}, {4, parts.four_centuries}, {5, parts.century_sign}, {6, parts.by_400},
           {7, parts.by_400_sign}, {8, parts.days}, {3, extended(parts.weekday)}});
    return parts.weekday;
}

namespace {
// The count of days or weeks has been applied to `moving`'s day; carry whole
// years forward until the day lies inside its year, then set the leap byte.
void settle_forward(Cpu& c, Date moving, std::uint32_t loop_back, std::uint32_t final_back) {
    for (;;) {
        const int last = leap(c, moving.year(), loop_back) ? 365 : 364;
        const auto day = moving.day();
        c.compare(0, day, last);
        if (day <= last) break;
        moving.set_day(std::uint32_t(day - (last + 1)));
        moving.set_year(std::uint32_t(moving.year()) + 1);
    }
    moving.set_leap(leap(c, moving.year(), final_back));
}
// The same going back: a negative day borrows the length of the year before.
void settle_backward(Cpu& c, Date moving, std::uint32_t loop_back, std::uint32_t final_back) {
    for (;;) {
        const int length = leap(c, (std::uint32_t(moving.year()) - 1) & 0xffff, loop_back) ? 366 : 365;
        const auto day = moving.day();
        c.set({{3, extended(day)}, {0, extended(day)}});
        c.compare(0, day, 0);
        if (day >= 0) break;
        moving.set_day(std::uint32_t(day + length));
        moving.set_year(std::uint32_t(moving.year()) - 1);
    }
    moving.set_leap(leap(c, moving.year(), final_back));
}
// A negative count of days: the other routine does the work with the count's
// magnitude, through the static date and a local of this frame.
void by_opposite(Cpu& c, guest::Frame& frame, Date scratch, Date from, Date to, std::uint32_t days,
                 std::uint32_t opposite, std::uint32_t back) {
    const auto magnitude = extended(std::int16_t(0u - days));
    const Date local(c, frame.at(0x38));
    c.set({{29, scratch.address() + 2}, {28, scratch.address() + 4}});
    scratch.set_year(from.year());
    scratch.set_day(extended(from.day()));
    scratch.set_leap(from.leap());
    c.set({{3, local.address()}, {4, scratch.address()}, {5, magnitude}});
    c.link(back);
    if (opposite == entry::subtract_days) subtract_days(c, local.address(), scratch.address(), magnitude);
    else add_days(c, local.address(), scratch.address(), magnitude);
    const auto year = local.year();
    const auto leap_year = local.leap();
    scratch.set_year(year);
    const auto day = local.day();
    c.set(3, extended(day));
    scratch.set_day(extended(day));
    scratch.set_leap(leap_year);
    copy(to, scratch);
}
}

void add_days(Cpu& c, std::uint32_t result, std::uint32_t address, std::uint32_t days) {
    guest::Frame frame(c, entry::add_days, 0x50, {.first_gpr = 28});
    const Date scratch = static_date(c, slot::add_days), from(c, address), to(c, result);
    c.set({{31, scratch.address()}, {30, result}});
    const auto count = std::int16_t(days);
    c.set(0, extended(count));
    c.compare(0, count, 0);
    if (count < 0) return by_opposite(c, frame, scratch, from, to, days, entry::subtract_days, 0x10717918);
    scratch.set_year(from.year());
    scratch.set_day(extended(from.day()) + days);
    settle_forward(c, scratch, 0x1071796c, 0x107179b8);
    copy(to, scratch);
}

void subtract_days(Cpu& c, std::uint32_t result, std::uint32_t address, std::uint32_t days) {
    guest::Frame frame(c, entry::subtract_days, 0x50, {.first_gpr = 28});
    const Date scratch = static_date(c, slot::subtract_days), from(c, address), to(c, result);
    c.set({{31, scratch.address()}, {30, result}});
    const auto count = std::int16_t(days);
    c.set(0, extended(count));
    c.compare(0, count, 0);
    if (count < 0) return by_opposite(c, frame, scratch, from, to, days, entry::add_days, 0x10717ba8);
    scratch.set_year(from.year());
    scratch.set_day(extended(from.day()) - days);
    settle_backward(c, scratch, 0x10717c08, 0x10717c54);
    copy(to, scratch);
}

void add_weeks(Cpu& c, std::uint32_t result, std::uint32_t address, std::uint32_t weeks) {
    guest::Frame frame(c, entry::add_weeks, 0x50, {.first_gpr = 29});
    const Date scratch = static_date(c, slot::add_weeks), from(c, address), to(c, result);
    scratch.set_year(from.year());
    scratch.set_day(extended(from.day()) + (weeks & 0xff) * 7);
    settle_forward(c, scratch, 0x10717a80, 0x10717acc);
    copy(to, scratch);
}

void subtract_weeks(Cpu& c, std::uint32_t result, std::uint32_t address, std::uint32_t weeks) {
    guest::Frame frame(c, entry::subtract_weeks, 0x50, {.first_gpr = 29});
    const Date scratch = static_date(c, slot::subtract_weeks), from(c, address), to(c, result);
    scratch.set_year(from.year());
    scratch.set_day(extended(from.day()) - (weeks & 0xff) * 7);
    settle_backward(c, scratch, 0x10717d28, 0x10717d74);
    copy(to, scratch);
}

std::int32_t days_between(Cpu& c, std::uint32_t address, std::uint32_t other_high, std::uint32_t other_low) {
    guest::Frame frame(c, entry::days_between, 0x50, {.first_gpr = 28});
    // The second date arrives by value and is spilled to the caller's parameter area.
    const Date other(c, frame.at(0x6c)), compared(c, frame.at(0x38)), date(c, address);
    c.put32(other.address(), other_high);
    c.put32(other.address() + 4, other_low);
    c.set(6, c.toc(slot::days_between.date));
    const Date scratch = static_date(c, slot::days_between);
    const auto high = c.u32(other.address()), low = c.u32(other.address() + 4);
    c.put32(compared.address(), high);
    const std::uint16_t year = date.year(), other_year = compared.year();
    c.put32(compared.address() + 4, low);
    c.set(5, year);
    c.compare_logical(0, other_year, year);
    bool other_later = other_year > year;
    if (other_year == year) {
        std::uint32_t half;
        other_later = later_day(c, compared.day(), date.day(), half);
        c.set(4, extended(compared.day()));
    }
    // Whole years first, each as long as the year before the one being left.
    std::uint32_t total = 0;
    const std::uint16_t target = other.year();
    c.compare_logical(0, year, target);
    if (year != target) {
        scratch.set_year(year);
        scratch.set_day(extended(date.day()));
        scratch.set_leap(date.leap());
        for (;;) {
            const std::uint16_t reached = scratch.year();
            c.compare_logical(0, reached, target);
            if (other_later ? reached >= target : reached <= target) break;
            const bool long_year = leap(c, std::uint32_t(reached - 1) & 0xffff, other_later ? 0x1071821c : 0x10718290);
            const std::uint32_t length = long_year ? 366 : 365;
            total += other_later ? 0u - length : length;
            scratch.set_year(std::uint32_t(scratch.year()) + (other_later ? 1u : 0u - 1u));
        }
    }
    total += extended(date.day()) - extended(other.day());
    c.set(3, total);
    return std::int32_t(total);
}

void interval(Cpu& c, std::uint32_t result_address, std::uint32_t address, std::uint32_t other_high,
              std::uint32_t other_low) {
    guest::Frame frame(c, entry::interval, 0x70, {.first_gpr = 28});
    const Date other(c, frame.at(0x90)), other_copy(c, frame.at(0x50)), date(c, address), result(c, result_address);
    c.put32(other.address(), other_high);
    c.put32(other.address() + 4, other_low);
    const Date scratch = static_date(c, slot::interval);
    const auto high = c.u32(other.address()), low = c.u32(other.address() + 4);
    c.put32(other_copy.address(), high);
    const std::uint16_t year = date.year(), other_year = other_copy.year();
    c.put32(other_copy.address() + 4, low);
    c.set({{3, year}, {4, other_year}});
    c.compare_logical(0, other_year, year);
    bool other_later = other_year > year;
    if (other_year == year) {
        std::uint32_t half;
        other_later = later_day(c, other_copy.day(), date.day(), half);
        c.set({{5, extended(other_copy.day())}, {4, half}});
    }
    c.compare(0, other_later, 0);
    if (!other_later) {
        // From the other date up to this one. A day count below zero borrows a year of this date's
        // year's length: 0x107180b0 calls the leap test with the year its first compare left in r3.
        const auto other_day = other.day();
        const auto days = extended(date.day()) - extended(other_day);
        c.compare(0, std::int32_t(days), 0);
        if (std::int32_t(days) >= 0) {
            scratch.set_day(days);
            const auto first = other.year();
            c.set(3, first);
            scratch.set_year(std::uint32_t(date.year()) - first);
        } else {
            const std::uint32_t length = leap(c, year, 0x107180b4) ? 366 : 365;
            scratch.set_day(extended(date.day()) - extended(other_day) + length);
            const auto years = std::uint32_t(date.year()) - other.year();
            c.set(3, years);
            scratch.set_year(years - 1);
        }
        copy(result, scratch);
        return;
    }
    // From this date up to the other, measured on copies of both.
    const Date date_copy(c, frame.at(0x48)), compared(c, frame.at(0x40));
    const auto guard = c.i8(c.r(2) + std::uint32_t(slot::interval.guard));
    const auto first_word = c.u32(address), second_word = c.u32(address + 4);
    c.put32(date_copy.address(), first_word);
    c.put32(date_copy.address() + 4, second_word);
    c.set(4, first_word);
    // The static date was built above; 0x10717ea0 would build it again.
    if (!guard) throw std::logic_error("DATE.CPP: the interval's static date lost its guard");
    const auto copied_high = c.u32(date_copy.address()), copied_low = c.u32(date_copy.address() + 4);
    c.put32(compared.address(), copied_high);
    const std::uint16_t end_year = other.year(), start_year = compared.year();
    c.put32(compared.address() + 4, copied_low);
    c.set(3, start_year);
    c.compare_logical(0, start_year, end_year);
    bool reversed = start_year > end_year;
    if (start_year == end_year) {
        std::uint32_t half;
        reversed = later_day(c, compared.day(), other.day(), half);
        c.set({{4, extended(compared.day())}, {3, half}});
    }
    c.compare(0, reversed, 0);
    // 0x10717f18-0x10717ffc handle a date later than the other one. The other one was found later
    // than this date just above, from the same two values (the copies are of words read after that
    // test, and nothing writes them in between), so both cannot hold.
    if (reversed) throw std::logic_error("DATE.CPP: an interval's dates are each later than the other");
    const auto start_day = date_copy.day(), end_day = other.day();
    const auto days = extended(end_day) - extended(start_day);
    c.compare(0, std::int32_t(days), 0);
    if (std::int32_t(days) >= 0) {
        scratch.set_day(days);
        scratch.set_year(std::uint32_t(end_year) - date_copy.year());
        c.set(3, days);
    } else {
        const std::uint32_t length = leap(c, end_year, 0x1071802c) ? 366 : 365;
        scratch.set_day(extended(end_day) - extended(date_copy.day()) + length);
        const auto years = std::uint32_t(end_year) - date_copy.year();
        c.set(3, years);
        scratch.set_year(years - 1);
    }
    copy(result, scratch);
}

std::int16_t day_of_month(Cpu& c, std::uint32_t address) {
    const Date date(c, address);
    const auto leap_year = std::int8_t(date.leap());
    const auto starts = c.toc(leap_year ? slot::leap_month_starts : slot::month_starts);
    c.set(5, starts);
    std::int16_t start = 0, day = 0;
    int month = 11;
    for (; month >= 0; --month) {
        start = c.i16(starts + std::uint32_t(month) * 2);
        day = date.day();
        if (start < day) break;
    }
    c.compare(0, start, day);
    c.set({{6, extended(day)}, {7, std::uint32_t(month)}, {4, starts + std::uint32_t(month < 0 ? 0 : month) * 2 - (month < 0 ? 2u : 0u)}});
    c.s.wr<4>(reg::ctr, std::uint32_t(month + 1));
    if (month < 0) {
        c.set({{0, extended(start)}, {3, 0xffffffff}});
        return -1;
    }
    const auto within = extended(day) - extended(start);
    c.set({{0, within}, {3, extended(std::int16_t(within))}});
    return std::int16_t(within);
}

std::int8_t month(Cpu& c, std::uint32_t address) {
    const Date date(c, address);
    const auto leap_year = std::int8_t(date.leap());
    const auto starts = c.toc(leap_year ? slot::leap_month_starts : slot::month_starts);
    // Two rounds of six months, from December down; the day is read once a round.
    std::int16_t start = 0, day = 0;
    int month = 11;
    for (; month >= 0; --month) {
        start = c.i16(starts + std::uint32_t(month) * 2);
        if (month == 11 || month == 5) day = date.day();
        if (start < day) break;
    }
    c.compare(0, start, day);
    const unsigned rounds_done = month < 0 ? 2 : month < 6 ? 1 : 0;
    c.set({{0, extended(day)}, {5, extended(start)}, {6, 0x16 - rounds_done * 10},
           {4, starts + 0x16 - rounds_done * 12}, {7, std::uint32_t(month)}, {3, std::uint32_t(month)}});
    c.s.wr<4>(reg::ctr, 2 - rounds_done);
    return std::int8_t(month);
}

std::int8_t month_takes_other_article(Cpu& c, std::uint32_t language_value, std::uint32_t month_value) {
    const auto language = std::int8_t(language_value);
    c.compare(0, language, 9);
    if (language == 9) {
        const auto month = std::int16_t(month_value);
        c.set(0, extended(month));
        c.compare(0, month, 7);
        if (month < 7) c.compare(0, month, 5);
        const bool answer = month >= 7 || month < 5;
        c.set(3, answer);
        return answer;
    }
    // subfic, cntlzw: 1 exactly when the language is 3.
    const auto distance = 3u - extended(language);
    c.s.wr<1>(reg::xer_ca, extended(language) <= 3u);
    c.set({{0, distance == 0}, {3, distance == 0}});
    return distance == 0;
}
}

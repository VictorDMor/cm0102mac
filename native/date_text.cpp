// DATE.CPP: dates as text (date.hpp). Nine formatters over the game's format
// routine (0x10727890, which takes the localised form of a pattern such as
// "<%s - day e.g. Sat> <%s - day of month e.g. 10th> <%s - month e.g. Feb>"),
// and the names they are made of.
//
// The format routines are varargs: they store r3-r10 into their frames, and
// the frames are read later (date.hpp). So each call here is made with every
// one of those registers as the original holds it, also the ones the pattern
// does not use, which is why the weekday's intermediate values are placed in
// registers before a call that only wants a weekday.
#include "date.hpp"
#include "date_detail.hpp"

namespace cm::aot::date {
namespace {
constexpr std::uint32_t language = 0x10727880, localised_format = 0x10727890, plain_format = 0x1077b720,
    copy_text = 0x1077dbf0, ordinal_text = 0x107467a0;
constexpr std::uint32_t empty_text = 0x740;  // in the unit's string pool

// The routine's static buffer in r31 inside a 0x40-byte frame, as the seven small text routines have it.
struct TextFrame {
    TextFrame(Cpu& c, std::uint32_t routine, std::int32_t buffer_slot)
        : frame(c, routine, 0x40, {.first_gpr = 31}), buffer(c.toc(buffer_slot)), pool(c.toc(slot::pool)) {
        c.set(31, buffer);
    }
    guest::Frame frame;
    std::uint32_t buffer, pool;
};
void write_text(Cpu& c, const TextFrame& text, std::uint32_t pattern, std::uint32_t back) {
    c.set({{3, text.buffer}, {4, text.pool + pattern}});
    c.call(localised_format, back);
}
void write_nothing(Cpu& c, const TextFrame& text, std::uint32_t back) {
    c.set({{3, text.buffer}, {4, text.pool + empty_text}});
    c.call(copy_text, back);
}

struct NameTable {
    std::uint32_t routine;
    std::int32_t buffer, jumps;
    unsigned count;
    bool byte_index;
    std::uint32_t first_case, fallback;
    // Per name: its pattern in the pool, and whether the call also passes the empty text (a
    // pattern with a comment for the translators: "Mar<%s - COMMENT - March short name>").
    struct Entry { std::uint32_t pattern; bool comment; };
    std::array<Entry, 12> names;
};
constexpr NameTable short_weekdays{entry::short_weekday_name, -0x3fcc, -0x3fd0, 7, false, 0x10718738, 0x107187a8,
    {{{0x7ae, false}, {0x7b2, false}, {0x7b6, false}, {0x7ba, false}, {0x7be, false}, {0x7c2, false}, {0x7c6, false}}}};
constexpr NameTable weekdays{entry::weekday_name, -0x3fd4, -0x3fd8, 7, false, 0x10718808, 0x10718878,
    {{{0x7ca, false}, {0x7d1, false}, {0x7d8, false}, {0x7e0, false}, {0x7ea, false}, {0x7f3, false}, {0x7fa, false}}}};
constexpr NameTable short_months{entry::short_month_name, -0x3fdc, -0x3fe0, 12, true, 0x107188d8, 0x107189a0,
    {{{0x803, false}, {0x807, false}, {0x80b, true}, {0x830, false}, {0x834, false}, {0x838, false},
      {0x83c, false}, {0x840, false}, {0x844, false}, {0x848, false}, {0x84c, false}, {0x850, true}}}};
constexpr NameTable months{entry::month_name, -0x3fe4, -0x3fe8, 12, true, 0x10718a08, 0x10718acc,
    {{{0x878, false}, {0x880, false}, {0x889, false}, {0x88f, false}, {0x895, true}, {0x8ba, false},
      {0x8bf, false}, {0x8c4, false}, {0x8cb, false}, {0x8d5, false}, {0x8dd, false}, {0x8e6, false}}}};

std::uint32_t name_from(Cpu& c, const NameTable& table, std::uint32_t index_value) {
    TextFrame text(c, table.routine, table.buffer);
    const auto index = table.byte_index ? extended(std::int8_t(index_value)) : extended(std::int16_t(index_value));
    c.set({{0, index}, {table.byte_index ? 5u : 4u, text.pool}});
    c.compare_logical(0, index, table.count - 1);
    if (index >= table.count) {
        write_nothing(c, text, table.fallback + 0xc);
    } else {
        // The switch goes through a table of code addresses in game memory.
        const auto target = c.u32(c.toc(table.jumps) + index * 4);
        c.set({{0, index * 4}, {3, target}});
        c.s.wr<4>(reg::ctr, target);
        std::uint32_t at = table.first_case;
        unsigned chosen = 0;
        for (; chosen < table.count && at != target; ++chosen) at += table.names[chosen].comment ? 0x14 : 0x10;
        if (target == table.fallback) write_nothing(c, text, table.fallback + 0xc);
        else if (chosen == table.count) throw std::logic_error("DATE.CPP: a name table points outside its routine");
        else {
            const auto& name = table.names[chosen];
            if (name.comment) c.set(5, text.pool + empty_text);
            write_text(c, text, name.pattern, at + (name.comment ? 0x10 : 0xc));
        }
    }
    c.set(3, text.buffer);
    return text.buffer;
}

// Year modulo 100 as the formatters compute it; leaves the carry of its shift.
struct TwoDigits { std::uint32_t sign, hundreds, value; };
TwoDigits two_digit_year(Cpu& c, std::uint32_t year) {
    const auto quotient = shift_right(c, high_product(0x51eb851f, year), 5);
    TwoDigits digits{sign_bit(quotient), 0, 0};
    digits.hundreds = (quotient + digits.sign) * 100;
    digits.value = year - digits.hundreds;
    return digits;
}

// One formatter's registers: where it keeps its arguments and the unit's shared
// addresses across calls, and the scratch registers of its month search.
struct Layout {
    std::uint32_t routine, frame;
    unsigned first_saved;
    unsigned flag, message, buffer, date;   // date 3: it stays in r3
    std::int16_t capacity;
    std::uint32_t line, first_report, second_report;
    unsigned leap_copy;                      // 0: the leap byte is only tested
    unsigned starts, cursor, counter, tens, threshold, month;
};
constexpr Layout numeric_long_year_layout{entry::numeric_long_year, 0xa0, 26, 26, 27, 29, 28, 9, 0xf8, 0x10715bfc, 0x10715c60, 0, 5, 3, 8, 7, 4, 30};
constexpr Layout numeric_layout{entry::numeric, 0xb0, 25, 25, 29, 27, 26, 9, 0x132, 0x10715e40, 0x10715ea4, 0, 5, 3, 7, 6, 4, 30};
constexpr Layout weekday_and_numeric_layout{entry::weekday_and_numeric, 0xb0, 25, 28, 26, 27, 30, 0x12, 0x181, 0x1071616c, 0x107161d0, 8, 5, 3, 9, 7, 4, 29};
constexpr Layout short_weekday_year_layout{entry::short_weekday_year, 0xb0, 25, 28, 26, 27, 30, 0x12, 0x1bf, 0x1071652c, 0x10716590, 8, 5, 3, 9, 7, 4, 29};
constexpr Layout short_year_layout{entry::short_year, 0xa0, 26, 28, 27, 30, 29, 0x12, 0x1fb, 0x107168dc, 0x10716940, 0, 4, 4, 7, 5, 3, 26};
constexpr Layout short_weekday_layout{entry::short_weekday, 0xa0, 26, 29, 27, 28, 3, 0x12, 0x234, 0x10716b38, 0x10716b9c, 9, 7, 4, 10, 8, 5, 30};
constexpr Layout long_weekday_year_layout{entry::long_weekday_year, 0xb0, 25, 28, 26, 27, 30, 0x32, 0x270, 0x10716eec, 0x10716f50, 8, 5, 3, 9, 7, 4, 29};
constexpr Layout long_weekday_layout{entry::long_weekday, 0xa0, 26, 29, 27, 28, 3, 0x32, 0x2af, 0x107172a8, 0x1071730c, 9, 7, 4, 10, 8, 5, 30};
constexpr Layout long_year_layout{entry::long_year, 0xa0, 26, 26, 27, 29, 28, 0x32, 0x2ed, 0x1071765c, 0x107176c0, 0, 5, 3, 8, 7, 4, 30};

class Formatter {
public:
    Formatter(Cpu& cpu, const Layout& layout)
        : c(cpu), l(layout), date_address(c.r(3)), buffer(c.r(4)), capacity(std::int16_t(c.r(5))), time(c.r(6)),
          frame(c, l.routine, l.frame, {.first_gpr = l.first_saved}), pool(c.toc(slot::pool)), date(c, date_address) {
        c.set({{l.buffer, buffer}, {l.flag, c.toc(slot::success_flag)}, {l.message, c.toc(slot::message)}, {31, pool}});
        if (l.date != 3) c.set(l.date, date_address);
        if (l.routine == entry::numeric) c.set(28, time);
    }
    // The two assertions every formatter opens with; false after a report (the caller returns 0).
    bool arguments_valid() {
        c.compare_logical(0, buffer, 0);
        if (!buffer) return rejected({l.first_report, 0x68, 0x40, l.line});
        c.set(0, extended(capacity));
        c.compare(0, capacity, l.capacity);
        if (capacity < l.capacity) return rejected({l.second_report, 0x48, 0x38, l.line + 7});
        return true;
    }
    // The month: the last one whose start lies before the day. A day before every start leaves the
    // month register as the caller had it (the original never sets it then).
    void find_month() {
        const auto leap_byte = date.leap();
        if (l.leap_copy) c.set(l.leap_copy, leap_byte);
        leap_year = std::int8_t(leap_byte);
        starts = c.toc(leap_year ? slot::leap_month_starts : slot::month_starts);
        c.set(l.starts, starts);
        std::int16_t start = 0, day = 0;
        for (std::uint32_t month = 0; month < 12; ++month) {
            start = c.i16(starts + month * 2);
            day = date.day();
            if (start < day) c.set(l.month, month);
        }
        c.compare(0, start, day);
        c.set({{0, extended(day)}, {l.threshold, extended(start)}, {l.cursor, starts + 24}, {l.counter, 12}, {l.tens, 20}});
        c.s.wr<4>(reg::ctr, 0);
    }
    std::uint32_t month_register() const { return c.r(l.month); }
    std::uint32_t month_index() const { return extended(std::int8_t(month_register())); }
    // The day within the month, from the month register whatever it holds.
    struct DayInMonth { std::uint32_t day, start, value; };
    DayInMonth day_in_month() const {
        const auto day = extended(date.day());
        const auto start = extended(c.i16(starts + month_index() * 2));
        return {day, start, extended(std::int16_t(day - start))};
    }
    // Styles 0-2 name a time of day; the others leave it out.
    bool with_time() {
        const auto style = time & 0xff;
        c.compare_logical(0, style, 1);
        if (style > 1) c.compare_logical(0, style, 2);
        return style <= 2;
    }
    WeekdayParts weekday(std::uint32_t year) { return weekday_parts(c, year, leap_year, extended(date.day())); }
    // The weekday's parts as the branch with a time of day leaves them (r3-r5, r7-r9), and as the
    // branch without leaves them (r3-r8).
    void leave_with_time(const WeekdayParts& w) {
        c.set({{0, w.left_in_r0}, {3, w.week_sign}, {4, w.four_centuries}, {5, w.century_sign}, {7, w.by_400},
               {8, w.by_400_sign}, {9, w.days}});
    }
    void leave_without_time(const WeekdayParts& w) {
        c.set({{0, w.left_in_r0}, {3, w.week_sign}, {4, w.four_centuries}, {5, w.century_sign}, {6, w.by_400},
               {7, w.by_400_sign}, {8, w.days}});
    }
    template <class Routine> std::uint32_t own(std::uint32_t back, Routine&& routine) {
        c.link(back);
        return routine();
    }
    std::uint32_t short_time(std::uint32_t back) {
        c.set(3, time);
        return own(back, [&] { return time_of_day_short(c, time); });
    }
    std::uint32_t named_time(std::uint32_t back) {
        c.set({{3, time}, {4, 1}});
        return own(back, [&] { return time_of_day_name(c, time, 1); });
    }
    std::uint32_t named(std::uint32_t routine, std::uint32_t index, std::uint32_t back) {
        c.set(3, index);
        return own(back, [&] { return name(c, routine, index); });
    }
    // "10th": the game's ordinal text for a number.
    std::uint32_t ordinal(std::uint32_t number, std::uint32_t back) {
        c.set({{3, number}, {4, 1}});
        c.call(ordinal_text, back);
        return c.r(3);
    }
    void write(std::uint32_t routine, std::uint32_t pattern, std::uint32_t back) {
        c.set({{3, buffer}, {4, pool + pattern}});
        c.call(routine, back);
    }
    std::uint32_t done() {
        c.set(3, 1);
        return 1;
    }

    Cpu& c;
    const Layout& l;
    const std::uint32_t date_address, buffer;
    const std::int16_t capacity;
    const std::uint32_t time;
    guest::Frame frame;
    const std::uint32_t pool;
    const Date date;
    std::uint32_t starts = 0;
    std::int8_t leap_year = 0;

private:
    bool rejected(Assertion assertion) {
        report(c, frame, assertion);
        c.set({{0, 0}, {3, 0}});
        return false;
    }
};

// "10.02.1972 AM" and "10.02.1972".
std::uint32_t numeric_long_year(Cpu& c) {
    Formatter f(c, numeric_long_year_layout);
    if (!f.arguments_valid()) return 0;
    f.find_month();
    const auto within = f.day_in_month();
    c.set({{8, f.month_index()}, {0, f.time & 0xff}, {4, within.start}, {3, within.day - within.start}, {26, within.value}});
    if (f.with_time()) {
        const auto time = f.short_time(0x10715da0);
        c.set({{6, f.month_index() + 1}, {7, f.date.year()}, {8, time}, {5, within.value}});
        f.write(localised_format, 0x1c, 0x10715dc0);
    } else {
        c.set({{7, f.date.year()}, {5, within.value}, {6, f.month_index() + 1}});
        f.write(localised_format, 0x83, 0x10715ddc);
    }
    return f.done();
}

// "10.2.72 AM" and "10.2.72"; language 10 gets "72-02-10" through the plain formatter.
std::uint32_t numeric(Cpu& c) {
    Formatter f(c, numeric_layout);
    if (!f.arguments_valid()) return 0;
    f.find_month();
    const auto within = f.day_in_month();
    c.set({{0, f.time & 0xff}, {4, within.start}, {3, within.day - within.start}, {29, within.value}});
    const bool timed = f.with_time();
    c.call(language, timed ? 0x10715fe4 : 0x1071608c);
    const auto tongue = std::int8_t(c.r(3));
    c.set(0, extended(tongue));
    c.compare(0, tongue, 10);
    const bool reversed = tongue == 10;
    std::uint32_t time = 0;
    if (timed) time = f.short_time(reversed ? 0x10715ff8 : 0x10716044);
    const std::uint32_t year = f.date.year();
    const auto digits = two_digit_year(c, year);
    c.set({{0, digits.hundreds}, {timed ? 9u : 8u, year}});
    if (timed) c.set(8, time);
    if (reversed) {
        c.set({{5, digits.value}, {6, f.month_index() + 1}, {7, within.value}});
        f.write(plain_format, timed ? 0xcf : 0x142, timed ? 0x10716038 : 0x107160d4);
    } else {
        c.set({{5, within.value}, {6, f.month_index() + 1}, {7, digits.value}});
        f.write(localised_format, timed ? 0xe1 : 0x151, timed ? 0x10716084 : 0x10716114);
    }
    return f.done();
}

// "Saturday" on one line, "10.2.72 AM" or "10.2.72" on the next.
std::uint32_t weekday_and_numeric(Cpu& c) {
    Formatter f(c, weekday_and_numeric_layout);
    if (!f.arguments_valid()) return 0;
    f.find_month();
    const auto within = f.day_in_month();
    c.set({{0, within.day}, {3, f.time & 0xff}, {4, within.day - within.start}, {28, within.value}});
    const std::uint32_t year = f.date.year();
    if (f.with_time()) {
        const auto parts = f.weekday(year);
        f.leave_with_time(parts);
        c.set(25, extended(parts.weekday));
        const auto time = f.short_time(0x107163a4);
        c.set(26, time);
        const auto day_name = f.named(entry::weekday_name, extended(parts.weekday), 0x107163b0);
        const auto digits = two_digit_year(c, f.date.year());
        c.set({{0, digits.hundreds}, {6, day_name}, {7, within.value}, {10, time}, {8, f.month_index() + 1},
               {5, 100}, {9, digits.value}});
        f.write(localised_format, 0x197, 0x107163f8);
    } else {
        c.set(30, year);
        const auto parts = f.weekday(year);
        f.leave_without_time(parts);
        const auto day_name = f.named(entry::weekday_name, extended(parts.weekday), 0x10716494);
        const auto digits = two_digit_year(c, year);
        c.set({{0, digits.hundreds}, {6, day_name}, {8, f.month_index() + 1}, {7, within.value}, {5, 100}, {9, digits.value}});
        f.write(localised_format, 0x213, 0x107164d4);
    }
    return f.done();
}

// The formatters built from names: weekday, ordinal day, month, year and time of day, each present or not.
struct Named {
    const Layout& layout;
    bool weekday, year, long_names;
    unsigned weekday_register, day_register, time_register;   // where the routine keeps them
    std::uint32_t timed_pattern, plain_pattern;
    // Return addresses, in call order: time of day, month, ordinal, weekday, format (with a time), then
    // month, ordinal, weekday, format (without).
    std::array<std::uint32_t, 9> back;
};
constexpr Named short_weekday_year_form{short_weekday_year_layout, true, true, false, 25, 28, 26, 0x274, 0x2ed,
    {0x10716764, 0x10716770, 0x10716780, 0x1071678c, 0x107167ac, 0x1071684c, 0x1071685c, 0x10716868, 0x10716884}};
constexpr Named short_weekday_form{short_weekday_layout, true, false, false, 26, 29, 27, 0x3fa, 0x45d,
    {0x10716d74, 0x10716d80, 0x10716d90, 0x10716d9c, 0x10716db8, 0x10716e58, 0x10716e68, 0x10716e74, 0x10716e8c}};
constexpr Named long_weekday_year_form{long_weekday_year_layout, true, true, true, 25, 28, 26, 0x4a5, 0x531,
    {0x10717128, 0x10717134, 0x10717144, 0x10717150, 0x10717174, 0x10717214, 0x10717224, 0x10717230, 0x10717250}};
constexpr Named long_weekday_form{long_weekday_layout, true, false, true, 26, 29, 27, 0x59b, 0x611,
    {0x107174e8, 0x107174f4, 0x10717504, 0x10717510, 0x10717530, 0x107175d0, 0x107175e0, 0x107175ec, 0x10717608}};

// "Sat 10th Feb 1972 AM" and its relatives with a weekday.
std::uint32_t named_with_weekday(Cpu& c, const Named& form) {
    Formatter f(c, form.layout);
    if (!f.arguments_valid()) return 0;
    f.find_month();
    const auto within = f.day_in_month();
    const unsigned month_register = form.layout.month;
    const bool date_in_r3 = form.layout.date == 3;
    if (date_in_r3) c.set({{0, within.day}, {4, f.time & 0xff}, {5, within.day - within.start}});
    else c.set({{0, within.day}, {3, f.time & 0xff}, {4, within.day - within.start}});
    c.set(form.day_register, within.value);
    const std::uint32_t year = f.date.year();
    const auto month_routine = form.long_names ? entry::month_name : entry::short_month_name;
    const auto weekday_routine = form.long_names ? entry::weekday_name : entry::short_weekday_name;
    const bool timed = f.with_time();
    // Without a time of day, a formatter that prints the year keeps it where the date's address was.
    if (!timed && form.year) c.set(form.layout.date, year);
    const auto parts = f.weekday(year);
    if (timed) f.leave_with_time(parts);
    else f.leave_without_time(parts);
    c.set(form.weekday_register, extended(parts.weekday));
    const auto* back = form.back.data() + (timed ? 0 : 4);
    std::uint32_t time = 0;
    if (timed) {
        time = form.long_names ? f.named_time(back[0]) : f.short_time(back[0]);
        c.set(form.time_register, time);
    }
    const auto month_text = f.named(month_routine, f.month_register(), back[1]);
    c.set(month_register, month_text);
    const auto day_text = f.ordinal(c.r(form.day_register), back[2]);
    c.set(form.day_register, day_text);
    const auto day_name = f.named(weekday_routine, c.r(form.weekday_register), back[3]);
    // The long forms pass 100 first (the pattern's "{}" takes it) and the rest one register later.
    const unsigned first = form.long_names ? 6 : 5;
    if (timed && form.year) c.set(first + 3, f.date.year());
    c.set({{first, day_name}, {first + 1, day_text}, {first + 2, month_text}});
    if (timed) c.set(first + (form.year ? 4 : 3), time);
    else if (form.year) c.set(first + 3, year);
    if (form.long_names) c.set(5, 100);
    f.write(localised_format, timed ? form.timed_pattern : form.plain_pattern, back[4]);
    return f.done();
}

// "10th Feb 1972 AM" (short names) and "10th February 1972 (Morning)" (long), with and without the time.
std::uint32_t named_with_year(Cpu& c, const Layout& layout) {
    const bool long_names = layout.routine == entry::long_year;
    Formatter f(c, layout);
    if (!f.arguments_valid()) return 0;
    f.find_month();
    const auto month_routine = long_names ? entry::month_name : entry::short_month_name;
    std::uint32_t day_number;
    if (long_names) {
        const auto within = f.day_in_month();
        c.set({{0, f.time & 0xff}, {4, within.start}, {3, within.day - within.start}, {26, within.value}});
        day_number = within.value;
    } else {
        c.set(0, f.time & 0xff);
        day_number = 0;
    }
    const bool timed = f.with_time();
    // Return addresses: time of day, month, [day of the month,] ordinal, format.
    const auto site = [&](std::uint32_t short_timed, std::uint32_t short_plain, std::uint32_t long_timed, std::uint32_t long_plain) {
        return long_names ? (timed ? long_timed : long_plain) : (timed ? short_timed : short_plain);
    };
    std::uint32_t time = 0;
    if (timed) {
        time = long_names ? f.named_time(0x10717804) : f.short_time(0x10716a68);
        c.set(27, time);
    }
    const auto month_text = f.named(month_routine, f.month_register(), site(0x10716a74, 0x10716ab4, 0x10717810, 0x10717848));
    c.set(long_names ? 30u : 28u, month_text);
    if (!long_names) {
        // The short form asks the day of the month from the date again.
        c.set(3, f.date_address);
        c.link(timed ? 0x10716a80 : 0x10716ac0);
        day_number = extended(day_of_month(c, f.date_address));
        c.set(3, day_number);
    }
    const auto day_text = f.ordinal(day_number, site(0x10716a8c, 0x10716acc, 0x10717820, 0x10717858));
    c.set({{7, f.date.year()}, {5, day_text}, {6, month_text}});
    if (timed) c.set(8, time);
    f.write(localised_format, long_names ? (timed ? 0x665 : 0x6d6) : (timed ? 0x34b : 0x3b0),
            site(0x10716aa8, 0x10716ae4, 0x1071783c, 0x10717870));
    return f.done();
}
}

std::uint32_t time_of_day_short(Cpu& c, std::uint32_t time) {
    TextFrame text(c, entry::time_of_day_short, -0x3fc4);
    const std::uint32_t kind = time & 0xff;
    c.set({{0, kind}, {5, text.pool}});
    c.compare(0, std::int32_t(kind), 1);
    if (kind > 1) c.compare(0, std::int32_t(kind), 3);
    else if (kind < 1) c.compare(0, std::int32_t(kind), 0);
    if (kind > 2) {
        write_nothing(c, text, 0x10718558);
    } else {
        constexpr std::array<std::uint32_t, 3> patterns{0x725, 0x741, 0x75e}, back{0x10718520, 0x10718534, 0x10718548};
        c.set(5, text.pool + empty_text);  // the patterns carry a comment for the translators
        write_text(c, text, patterns[kind], back[kind]);
    }
    c.set(3, text.buffer);
    return text.buffer;
}

std::uint32_t time_of_day_name(Cpu& c, std::uint32_t time, std::uint32_t capitalised) {
    TextFrame text(c, entry::time_of_day_name, -0x3fc8);
    const std::uint32_t kind = time & 0xff;
    c.set({{0, kind}, {5, text.pool}});
    c.compare(0, std::int32_t(kind), 1);
    if (kind > 1) c.compare(0, std::int32_t(kind), 3);
    else if (kind < 1) c.compare(0, std::int32_t(kind), 0);
    if (kind > 2) {
        write_nothing(c, text, 0x10718634);
    } else {
        const auto capital = std::int8_t(capitalised);
        c.set(0, extended(capital));
        c.compare(0, capital, 0);
        constexpr std::array<std::uint32_t, 6> patterns{0x77a, 0x782, 0x78a, 0x794, 0x79e, 0x7a6},
            back{0x107185c4, 0x107185d4, 0x107185ec, 0x107185fc, 0x10718614, 0x10718624};
        const auto chosen = kind * 2 + (capital ? 0 : 1);
        write_text(c, text, patterns[chosen], back[chosen]);
    }
    c.set(3, text.buffer);
    return text.buffer;
}

std::uint32_t day_month_order(Cpu& c, std::uint32_t style) {
    guest::Frame frame(c, entry::day_month_order, 0x40, {.first_gpr = 31});
    c.set(31, style);
    // The language is asked again before each test.
    const auto speaks = [&](std::int32_t wanted, std::uint32_t back) {
        c.call(language, back);
        const auto tongue = std::int8_t(c.r(3));
        c.set(0, extended(tongue));
        c.compare(0, tongue, wanted);
        return tongue == wanted;
    };
    std::uint32_t order = 0;
    if (speaks(9, 0x10718668) || speaks(3, 0x10718678)) order = 1;
    else if (speaks(5, 0x10718690)) order = 2;
    else if (speaks(4, 0x107186a8)) {
        const std::uint32_t kind = style & 0xff;
        c.set(0, kind);
        c.compare(0, std::int32_t(kind), 2);
        if (kind < 2) c.compare(0, std::int32_t(kind), 0);
        order = kind == 2 ? 3 : kind < 2 ? 1 : 0;
    }
    c.set(3, order);
    return order;
}

std::uint32_t name(Cpu& c, std::uint32_t routine, std::uint32_t index) {
    switch (routine) {
        case entry::short_weekday_name: return name_from(c, short_weekdays, index);
        case entry::weekday_name: return name_from(c, weekdays, index);
        case entry::short_month_name: return name_from(c, short_months, index);
        case entry::month_name: return name_from(c, months, index);
    }
    throw std::logic_error("DATE.CPP: not a name routine");
}

std::uint32_t format(Cpu& c, std::uint32_t routine) {
    switch (routine) {
        case entry::numeric_long_year: return numeric_long_year(c);
        case entry::numeric: return numeric(c);
        case entry::weekday_and_numeric: return weekday_and_numeric(c);
        case entry::short_weekday_year: return named_with_weekday(c, short_weekday_year_form);
        case entry::short_year: return named_with_year(c, short_year_layout);
        case entry::short_weekday: return named_with_weekday(c, short_weekday_form);
        case entry::long_weekday_year: return named_with_weekday(c, long_weekday_year_form);
        case entry::long_weekday: return named_with_weekday(c, long_weekday_form);
        case entry::long_year: return named_with_year(c, long_year_layout);
    }
    throw std::logic_error("DATE.CPP: not a formatter");
}
}

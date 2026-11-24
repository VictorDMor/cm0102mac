#include "holiday_return.hpp"
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "gui_guest_scope.hpp"
#include "human_manager_detail.hpp"
#include "platform.hpp"
#include <array>
#include <string>
#include <vector>

namespace cm::aot::holiday_return {
namespace {
constexpr std::array<unsigned, 12> month_start{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
unsigned start_of(unsigned month, unsigned year) { return month_start[month] + (month >= 2 && year_length(year) == 366 ? 1 : 0); }
unsigned month_of(HolidayDate d) {
    unsigned month = 11;
    while (month > 0 && unsigned(d.day) < start_of(month, d.year)) --month;
    return month;
}
unsigned month_length(unsigned month, unsigned year) {
    return (month == 11 ? year_length(year) : start_of(month + 1, year)) - start_of(month, year);
}

// Texts for the game's languages (the byte at TOC-0x35ac; single-byte Latin-1, as the game's fonts expect).
struct Texts { const char *date, *none, *week, *two_weeks, *month; };
constexpr std::array<Texts, 11> texts{{
    {"Return date", "None", "+1 week", "+2 weeks", "+1 month"},
    {"Return date", "None", "+1 week", "+2 weeks", "+1 month"},
    {"Date de retour", "Aucune", "+1 semaine", "+2 semaines", "+1 mois"},
    {"R\374ckkehr am", "Kein Datum", "+1 Woche", "+2 Wochen", "+1 Monat"},
    {"Data di ritorno", "Nessuna", "+1 settimana", "+2 settimane", "+1 mese"},
    {"Fecha de regreso", "Ninguna", "+1 semana", "+2 semanas", "+1 mes"},
    {"Hjemkomst", "Ingen", "+1 uge", "+2 uger", "+1 m\345ned"},
    {"Terugkeerdatum", "Geen", "+1 week", "+2 weken", "+1 maand"},
    {"Hjemkomst", "Ingen", "+1 uke", "+2 uker", "+1 m\345ned"},
    {"Data de regresso", "Nenhuma", "+1 semana", "+2 semanas", "+1 m\352s"},
    {"\305terkomst", "Ingen", "+1 vecka", "+2 veckor", "+1 m\345nad"}}};

// The original's own routines used here.
constexpr std::uint32_t flow_append = 0x1074b830, flow_link = 0x1074bab0, flow_layout = 0x1074bcd0, flow_clear = 0x1074c570,
                        short_date = 0x107168a0, set_away = 0x10147fd0;
namespace toc {
constexpr std::int32_t language = -0x35ac, today = -0x7b80, humans = -0x7a54, game = -0x7bb0, buffer = -0x7bb8,
                       highlight = -0x7bc8, normal = -0x7bc4;
}
// The game record: the humans in play (0x2c0 bytes each, the staff record at +4), 16 of them, and the one in turn.
constexpr std::uint32_t human_size = 0x2c0, human_staff = 4, current_human = 0x2c58, human_slots = 16;

CareerRules* rules(const State& s) { return s.platform ? &s.platform->career_rules() : nullptr; }

std::uint32_t staff_in_turn(guest::Cpu& c) {
    const auto game = c.toc(toc::game);
    const auto slot = std::uint32_t(std::int32_t(c.i16(game + current_human)));
    return slot < human_slots ? c.u32(game + slot * human_size + human_staff) : 0;
}

void put_text(State& s, std::uint32_t at, const std::string& text) {
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    bytes.push_back(0);
    s.memory.copy_in(at, bytes);
}

// The game's own short date ("14 Jul 2001", in the language's order), or digits when it will not write one.
std::string date_text(guest::Cpu& c, HolidayDate date) {
    const auto record = c.sp() + 0x60, out = c.sp() + 0x100;
    c.put16(record, std::uint16_t(date.day));
    c.put16(record + 2, date.year);
    c.put8(record + 4, year_length(date.year) == 366);
    c.put8(out, 0);
    c.set({{3, record}, {4, out}, {5, 100}, {6, 255}});
    c.call(short_date, return_sentinel);
    std::string text;
    for (unsigned i = 0; i < 100; ++i) {
        const char ch = char(c.u8(out + i));
        if (!ch) break;
        text += ch;
    }
    if (text.empty()) {
        const auto month = month_of(date);
        text = std::to_string(date.day - int(start_of(month, date.year)) + 1) + "/" + std::to_string(month + 1) + "/" + std::to_string(date.year);
    }
    return text;
}

// One line of the dialog's list through the original's text flow (the object at the builder's r1+0x50): the
// text, each link name found in it, the layout from x=150 to 760 at the row counter, and the flow cleared.
struct Link { std::string name; unsigned value; };
void line(State& s, guest::Cpu& c, std::uint32_t frame, std::uint32_t grid, std::uint32_t buffer, unsigned color,
          const std::string& text, const std::vector<Link>& links) {
    const auto flow = frame + 0x50, row = frame + 0x4c;
    put_text(s, buffer, text);
    c.set({{3, flow}, {4, buffer}});
    c.call(flow_append, return_sentinel);
    for (const auto& item : links) {
        put_text(s, buffer, item.name);
        c.set({{3, flow}, {4, buffer}, {5, 3}, {6, item.value}});
        c.call(flow_link, return_sentinel);
    }
    c.put32(c.sp() + 0x38, 0);
    c.set({{3, flow}, {4, grid}, {5, 0x96}, {6, 0x2f8}, {7, row}, {8, 3}, {9, color}, {10, 2}});
    c.call(flow_layout, return_sentinel);
    c.set(3, flow);
    c.call(flow_clear, return_sentinel);
    c.put32(row, c.u32(row) + 1);
}
}

unsigned year_length(unsigned year) { return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365; }

HolidayDate add_days(HolidayDate date, int days) {
    int day = date.day + days;
    int year = date.year;
    while (day >= int(year_length(unsigned(year)))) { day -= int(year_length(unsigned(year))); ++year; }
    while (day < 0) { --year; day += int(year_length(unsigned(year))); }
    return {std::int16_t(day), std::uint16_t(year)};
}

HolidayDate add_months(HolidayDate date, int months) {
    const auto month = int(month_of(date));
    const auto dom = unsigned(date.day) - start_of(unsigned(month), date.year);
    int total = month + months, year = date.year;
    while (total >= 12) { total -= 12; ++year; }
    while (total < 0) { total += 12; --year; }
    const auto last = month_length(unsigned(total), unsigned(year)) - 1;
    return {std::int16_t(start_of(unsigned(total), unsigned(year)) + std::min(dom, last)), std::uint16_t(year)};
}

bool before(HolidayDate a, HolidayDate b) { return a.year < b.year || (a.year == b.year && a.day < b.day); }

std::optional<HolidayDate> after_click(unsigned value, std::optional<HolidayDate> current, HolidayDate today) {
    switch (value) {
    case link_day: return add_days(current && before(today, *current) ? *current : today, 1);
    case link_none: return std::nullopt;
    case link_week: return add_days(today, 7);
    case link_two_weeks: return add_days(today, 14);
    case link_month: return add_months(today, 1);
    default: return current;
    }
}

bool available(const State& s) {
    const auto* career = rules(s);
    return career && career->editable();
}

HolidayDate today(State& s) {
    guest::Cpu c(s);
    const auto record = c.toc(toc::today);
    return {c.i16(record), c.u16(record + 2)};
}

void opened(State& s) {
    if (auto* career = rules(s)) career->set_holiday_pending(std::nullopt);
}

void append_rows(State& s) {
    if (!available(s)) return;
    guest::Cpu c(s);
    const auto frame = c.sp(), grid = c.r(25), buffer = c.toc(toc::buffer);
    const auto highlight = c.u16(c.toc(toc::highlight)), normal = c.u16(c.toc(toc::normal));
    const unsigned language = c.u8(c.r(2) + std::uint32_t(toc::language));
    const auto& words = texts[language < texts.size() ? language : 0];
    const auto pending = rules(s)->holiday_pending();
    GuiGuestScope preserve(s);
    c.set(1, frame - 0x400);
    // Row 10: the date as a link; row 11: the shortcuts, each its own link.
    const std::string name = words.date;
    const auto value = pending ? date_text(c, *pending) : std::string(words.none);
    line(s, c, frame, grid, buffer, pending ? highlight : normal, name + ": " + value, {{name + ": " + value, link_day}});
    const std::string shortcuts = std::string(words.none) + "    " + words.week + "    " + words.two_weeks + "    " + words.month;
    line(s, c, frame, grid, buffer, normal, shortcuts,
         {{words.none, link_none}, {words.week, link_week}, {words.two_weeks, link_two_weeks}, {words.month, link_month}});
}

bool link(State& s) {
    guest::Cpu c(s);
    const auto value = c.r(0);
    if (value < first_link || value > last_link || !available(s)) return false;
    auto& career = *rules(s);
    career.set_holiday_pending(after_click(value, career.holiday_pending(), today(s)));
    return true;
}

void cancelled(State& s) {
    if (auto* career = rules(s)) career->set_holiday_pending(std::nullopt);
}

void started(State& s) {
    auto* career = rules(s);
    if (!career) return;
    guest::Cpu c(s);
    const auto pending = career->holiday_pending();
    career->set_holiday_pending(std::nullopt);
    const auto staff = staff_in_turn(c);
    if (!staff) return;
    // Only this manager's line changes. A date that is not after today asks for nothing: the holiday starts as it always did.
    career->set_holiday_return(c.u32(staff), pending && before(today(s), *pending) ? pending : std::nullopt);
}

void slot(State& s) {
    auto* career = rules(s);
    if (!career || career->holiday_returns().empty()) return;
    guest::Cpu c(s);
    const auto humans = c.u32(c.toc(toc::humans));
    const auto game = c.toc(toc::game);
    // No table of humans yet (a game being set up): nothing to judge, and the dates stay.
    if (!humans || !game) return;
    const auto now = today(s);
    const auto bound = career->holiday_returns();  // a copy: the loop changes the member
    for (const auto& entry : bound) {
        std::uint32_t staff = 0;
        for (std::uint32_t i = 0; i < human_slots && !staff; ++i) {
            const auto candidate = c.u32(game + i * human_size + human_staff);
            if (candidate && c.u32(candidate) == entry.staff) staff = candidate;
        }
        const auto index = staff ? human_manager::index_of(c, humans, staff) : std::nullopt;
        // Gone from the game, or no longer away: any other way back (a match, the menu) ended the holiday, and the date goes with it.
        if (!index || std::int8_t(c.u8(human_manager::record_at(c, humans, *index) + human_manager::record::away)) == 0) {
            career->set_holiday_return(entry.staff, std::nullopt);
            continue;
        }
        if (before(now, entry.date)) continue;
        // The date is here (or past): the original's way back, set_away(humans, 0, manager). The scope leaves the
        // registers and the stack under the routine as they were; the record, the counters and the news are the routine's.
        {
            GuiGuestScope preserve(s);
            c.set(1, c.sp() - 0x400);
            c.set({{3, humans}, {4, 0}, {5, staff}});
            c.call(set_away, return_sentinel);
        }
        career->set_holiday_return(entry.staff, std::nullopt);
    }
}
}

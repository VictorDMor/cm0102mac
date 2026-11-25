#include "holiday_return.hpp"
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "gui_guest_scope.hpp"
#include "human_manager_detail.hpp"
#include "platform.hpp"
#include <algorithm>
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
struct Texts { const char *date, *none, *week, *two_weeks, *month, *season, *calendar; };
constexpr std::array<Texts, 11> texts{{
    {"Return date", "None", "+1 week", "+2 weeks", "+1 month", "End of season", "Calendar"},
    {"Return date", "None", "+1 week", "+2 weeks", "+1 month", "End of season", "Calendar"},
    {"Date de retour", "Aucune", "+1 semaine", "+2 semaines", "+1 mois", "Fin de saison", "Calendrier"},
    {"R\374ckkehr am", "Kein Datum", "+1 Woche", "+2 Wochen", "+1 Monat", "Saisonende", "Kalender"},
    {"Data di ritorno", "Nessuna", "+1 settimana", "+2 settimane", "+1 mese", "Fine stagione", "Calendario"},
    {"Fecha de regreso", "Ninguna", "+1 semana", "+2 semanas", "+1 mes", "Fin de temporada", "Calendario"},
    {"Hjemkomst", "Ingen", "+1 uge", "+2 uger", "+1 m\345ned", "S\346sonslut", "Kalender"},
    {"Terugkeerdatum", "Geen", "+1 week", "+2 weken", "+1 maand", "Einde seizoen", "Kalender"},
    {"Hjemkomst", "Ingen", "+1 uke", "+2 uker", "+1 m\345ned", "Sesongslutt", "Kalender"},
    {"Data de regresso", "Nenhuma", "+1 semana", "+2 semanas", "+1 m\352s", "Fim da \351poca", "Calend\341rio"},
    {"\305terkomst", "Ingen", "+1 vecka", "+2 veckor", "+1 m\345nad", "S\344songens slut", "Kalender"}}};

// The original's own routines used here.
constexpr std::uint32_t flow_append = 0x1074b830, flow_link = 0x1074bab0, flow_layout = 0x1074bcd0, flow_clear = 0x1074c570,
                        short_date = 0x107168a0, set_away = 0x10147fd0, weekday_routine = 0x10715b20,
                        weekday_name_routine = 0x10718700, month_name_routine = 0x107188a0, pack_colour = 0x1076bb20;
// The calendar mode's list grid (navigation, weekday names, six weeks by seven days) and its footer (None, Cancel, OK).
constexpr unsigned calendar_columns = 7, calendar_rows = 8, calendar_footer_columns = 3;
// A nation's season dates in the game's table of nation calendars (the 34 rows of 0x4c bytes at TOC -0x7b44).
constexpr std::uint32_t nation_rows = 34, nation_row_size = 0x4c, nation_start_day = 0x2c, nation_end_day = 0x3c;
// The club record's nation word.
constexpr std::uint32_t club_nation = 0x2c, staff_club = 0x24;
namespace toc {
constexpr std::int32_t language = -0x35ac, today = -0x7b80, humans = -0x7a54, game = -0x7bb0, buffer = -0x7bb8,
                       highlight = -0x7bc8, normal = -0x7bc4, buttons = -0x4e18, nations = -0x7b44, selected = -0x7bd0;
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

// The calendar mode's state: host state of the open dialog, never saved (opened, started and the handler's No reset it).
CalendarState mode;
// The farthest month the calendar reaches, counted from today's year.
constexpr unsigned years_ahead = 30;
unsigned month_number(unsigned month, unsigned year) { return year * 12 + month; }
CalendarState at_month(CalendarState state, unsigned number) {
    state.month = number % 12;
    state.year = number / 12;
    return state;
}

std::string guest_text(guest::Cpu& c, std::uint32_t at) {
    std::string text;
    for (unsigned i = 0; at && i < 60; ++i) {
        const char ch = char(c.u8(at + i));
        if (!ch) break;
        text += ch;
    }
    return text;
}
// One of the game's own names (the date unit's short weekday 0x10718700 and short month 0x107188a0 routines), index from 0.
std::string name_of(guest::Cpu& c, std::uint32_t routine, unsigned index) {
    c.set(3, index);
    c.call(routine, return_sentinel);
    return guest_text(c, c.r(3));
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
unsigned month_index(HolidayDate date) { return month_of(date); }
unsigned days_in_month(unsigned month, unsigned year) { return month_length(month, year); }
HolidayDate first_of_month(unsigned month, unsigned year) { return {std::int16_t(start_of(month, year)), std::uint16_t(year)}; }

const CalendarState& calendar() { return mode; }

CalendarState calendar_opening(std::optional<HolidayDate> choice, HolidayDate today) {
    const auto tomorrow = add_days(today, 1);
    const bool ahead = choice && before(today, *choice);
    const auto base = ahead ? *choice : tomorrow;
    return {true, month_of(base), base.year, ahead ? choice : std::nullopt};
}

CalendarState calendar_after(CalendarState state, unsigned value, HolidayDate today) {
    using namespace calendar_value;
    const auto tomorrow = add_days(today, 1);
    // The months the calendar can show: tomorrow's to the end of the year `years_ahead` past today's.
    const auto first = month_number(month_of(tomorrow), tomorrow.year), last = month_number(11, today.year + years_ahead);
    const auto shown = month_number(state.month, state.year);
    const auto go = [&](int months) {
        const int target = std::clamp(int(shown) + months, int(first), int(last));
        return at_month(state, unsigned(target));
    };
    switch (value) {
    case previous_month: return go(-1);
    case next_month: return go(1);
    case previous_year: return go(-12);
    case next_year: return go(12);
    case none: state.draft.reset(); return state;
    default: break;
    }
    if (value > day && value <= day + 31 && shown >= first && shown <= last) {
        const unsigned dom = value - day;
        if (dom <= month_length(state.month, state.year)) {
            const HolidayDate picked{std::int16_t(start_of(state.month, state.year) + dom - 1), std::uint16_t(state.year)};
            if (before(today, picked)) state.draft = picked;
        }
    }
    return state;
}

HolidayDate end_of_season(bool calendar_year, HolidayDate today) {
    // 29 December, or 30 June, of a year.
    const auto of_year = [&](unsigned year) {
        return calendar_year ? HolidayDate{std::int16_t(start_of(11, year) + 28), std::uint16_t(year)}
                             : HolidayDate{std::int16_t(start_of(5, year) + 29), std::uint16_t(year)};
    };
    const auto this_year = of_year(today.year);
    return before(today, this_year) ? this_year : of_year(today.year + 1);
}

std::optional<HolidayDate> after_click(unsigned value, std::optional<HolidayDate> current, HolidayDate today, bool calendar_year) {
    switch (value) {
    case link_day: return add_days(current && before(today, *current) ? *current : today, 1);
    case link_none: return std::nullopt;
    case link_week: return add_days(today, 7);
    case link_two_weeks: return add_days(today, 14);
    case link_month: return add_months(today, 1);
    case link_season: return end_of_season(calendar_year, today);
    default: return current;
    }
}

bool calendar_year_season(State& s) {
    guest::Cpu c(s);
    const auto staff = staff_in_turn(c);
    const auto club = staff ? c.u32(staff + staff_club) : 0;
    const auto nation = club ? c.u32(club + club_nation) : 0;
    const auto rows = c.toc(toc::nations);
    if (!nation || !rows) return false;
    for (unsigned i = 0; i < nation_rows; ++i) {
        const auto row = rows + i * nation_row_size;
        if (c.u32(row) != nation) continue;
        return c.i16(row + nation_start_day) < c.i16(row + nation_end_day);
    }
    return false;
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
    mode = {};
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
    // Row 10: the date as a link, then the calendar; row 11: the shortcuts, each its own link (a line holds five).
    const std::string name = words.date;
    const auto value = pending ? date_text(c, *pending) : std::string(words.none);
    line(s, c, frame, grid, buffer, pending ? highlight : normal, name + ": " + value + "    " + words.calendar,
         {{name + ": " + value, link_day}, {words.calendar, link_calendar}});
    const std::string shortcuts = std::string(words.none) + "    " + words.week + "    " + words.two_weeks + "    " + words.month +
                                  "    " + words.season;
    line(s, c, frame, grid, buffer, normal, shortcuts,
         {{words.none, link_none}, {words.week, link_week}, {words.two_weeks, link_two_weeks}, {words.month, link_month},
          {words.season, link_season}});
}

bool link(State& s) {
    guest::Cpu c(s);
    const auto value = c.r(0);
    if (!available(s)) return false;
    auto& career = *rules(s);
    if (mode.open) {
        // The calendar has no setting: whatever is clicked on it stays on it.
        if (value == calendar_value::cancel) mode.open = false;
        else if (value == calendar_value::confirm) {
            career.set_holiday_pending(mode.draft);
            mode.open = false;
        } else mode = calendar_after(mode, value, today(s));
        return true;
    }
    if (value < first_link || value > last_link) return false;
    if (value == link_calendar) mode = calendar_opening(career.holiday_pending(), today(s));
    else career.set_holiday_pending(after_click(value, career.holiday_pending(), today(s), value == link_season && calendar_year_season(s)));
    return true;
}

bool cancelled(State& s) {
    if (mode.open) {
        mode.open = false;
        return true;
    }
    if (auto* career = rules(s)) career->set_holiday_pending(std::nullopt);
    return false;
}

bool confirmed(State& s) {
    if (!mode.open) return false;
    if (auto* career = rules(s)) career->set_holiday_pending(mode.draft);
    mode.open = false;
    return true;
}

void started(State& s) {
    mode = {};
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

// The language's texts (the byte at TOC-0x35ac).
static const Texts& words_of(guest::Cpu& c) {
    const unsigned language = c.u8(c.r(2) + std::uint32_t(toc::language));
    return texts[language < texts.size() ? language : 0];
}

void intro(State& s) {
    if (!mode.open || !available(s)) return;
    guest::Cpu c(s);
    const auto buffer = c.toc(toc::buffer);
    const auto& words = words_of(c);
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    put_text(s, buffer, std::string(words.date) + ": " + (mode.draft ? date_text(c, *mode.draft) : std::string(words.none)));
}

void list_shape(State& s) {
    if (!mode.open || !available(s)) return;
    // The list grid's columns (r8) and rows (r10).
    s.wr<4>(8 * 4, calendar_columns);
    s.wr<4>(10 * 4, calendar_rows);
}

bool build_calendar(State& s) {
    if (!mode.open || !available(s)) return false;
    using namespace calendar_value;
    guest::Cpu c(s);
    const auto frame = c.sp(), grid = c.r(3), root = c.toc(toc::game), buffer = c.toc(toc::buffer);
    const auto highlight = c.u16(c.toc(toc::highlight)), normal = c.u16(c.toc(toc::normal)), selected = c.u16(c.toc(toc::selected));
    const auto& words = words_of(c);
    const auto now = today(s), tomorrow = add_days(now, 1);
    GuiGuestScope preserve(s);
    c.set(1, frame - 0x400);
    const auto sp = c.sp(), text = sp + 0x100;
    // The month on show: its title, the weekday of its first day (0 = Monday) and the game's own names.
    const auto record = sp + 0x60;
    c.put16(record, std::uint16_t(first_of_month(mode.month, mode.year).day));
    c.put16(record + 2, mode.year);
    c.put8(record + 4, year_length(mode.year) == 366);
    c.set(3, record);
    c.call(weekday_routine, return_sentinel);
    const unsigned first_weekday = unsigned(std::int16_t(c.r(3)) % 7 + 7) % 7;
    const auto title = name_of(c, month_name_routine, mode.month) + " " + std::to_string(mode.year);
    std::array<std::string, 7> weekdays;
    for (unsigned i = 0; i < 7; ++i) weekdays[i] = name_of(c, weekday_name_routine, i);
    c.set({{3, 0x80}, {4, 0x80}, {5, 0x80}, {6, 0}});
    c.call(pack_colour, return_sentinel);
    const unsigned dim = c.r(3) & 0xffff;  // the packed 15-bit pixel of a grey

    const auto put = [&](const std::string& value) { put_text(s, text, value); };
    const auto cell = [&](std::uint32_t at, unsigned column, unsigned row, unsigned flags, unsigned colour, unsigned font,
                          unsigned style, unsigned ink, bool clickable, unsigned value) {
        c.put32(sp + 0x38, ink); c.put32(sp + 0x3c, text);
        c.put32(sp + 0x40, 3); c.put32(sp + 0x44, value);
        c.set({{3, root}, {4, at}, {5, column}, {6, row}, {7, flags}, {8, colour}, {9, font}, {10, style}});
        c.call(clickable ? 0x107444e0 : 0x107445a0, return_sentinel);
    };
    const auto link_cell = [&](unsigned column, unsigned row, const std::string& label, unsigned value) {
        put(label);
        cell(grid, column, row, 1, 0, 12, 3, highlight, true, value);
    };
    const auto label_cell = [&](unsigned column, unsigned row, const std::string& label, unsigned ink, unsigned flags = 1, unsigned colour = 0) {
        put(label);
        cell(grid, column, row, flags, colour, 12, 3, ink, false, 0);
    };

    // Row 0: previous year and month, the title, next month and year. A month before tomorrow's is not reachable.
    const auto first = month_number(month_of(tomorrow), tomorrow.year), last = month_number(11, now.year + years_ahead);
    const auto shown = month_number(mode.month, mode.year);
    if (shown > first) {
        link_cell(0, 0, "<<", previous_year);
        link_cell(1, 0, "<", previous_month);
    }
    label_cell(3, 0, title, highlight);
    if (shown < last) {
        link_cell(5, 0, ">", next_month);
        link_cell(6, 0, ">>", next_year);
    }
    // Row 1: the weekdays; rows 2..7: the days, those after today as links and the chosen one marked.
    for (unsigned i = 0; i < 7; ++i) label_cell(i, 1, weekdays[i], normal);
    const auto length = month_length(mode.month, mode.year);
    for (unsigned dom = 1; dom <= length; ++dom) {
        const HolidayDate date{std::int16_t(start_of(mode.month, mode.year) + dom - 1), std::uint16_t(mode.year)};
        const unsigned index = first_weekday + dom - 1, column = index % 7, row = 2 + index / 7;
        const auto label = std::to_string(dom);
        if (mode.draft && *mode.draft == date) label_cell(column, row, label, selected, 0x221, selected);
        else if (before(now, date)) link_cell(column, row, label, day + dom);
        else label_cell(column, row, label, dim);
    }

    // The footer, in the place of the original's Yes/No: None, Cancel and OK, equal cells under the panel.
    const auto page = c.u32(root + 0x24 + std::uint32_t(std::int32_t(c.i16(root + 0x2c58)) * 0x2c0));
    c.put32(sp + 0x38, 0); c.put32(sp + 0x3c, 1); c.put32(sp + 0x40, 0); c.put32(sp + 0x44, 0xffffffffu);
    c.set({{3, page}, {4, 100}, {5, 555}, {6, 790}, {7, 590}, {8, calendar_footer_columns}, {9, frame + 0x48}, {10, 1}});
    c.call(0x107198c0, return_sentinel);
    const auto footer = c.r(3);
    const auto pool = c.toc(toc::buttons);
    const auto button = [&](unsigned column, unsigned value, std::uint32_t pool_text) {
        if (pool_text) {
            c.set({{3, buffer}, {4, pool + pool_text}});
            c.call(0x10727890, return_sentinel);
        } else put_text(s, buffer, words.none);
        c.put32(sp + 0x38, highlight); c.put32(sp + 0x3c, buffer); c.put32(sp + 0x40, 3); c.put32(sp + 0x44, value);
        c.set({{3, root}, {4, footer}, {5, column}, {6, 0}, {7, 0x30}, {8, normal}, {9, 12}, {10, 3}});
        c.call(0x107444e0, return_sentinel);
    };
    button(0, none, 0);
    button(1, cancel, 0x544);
    button(2, confirm, 0x785);
    return true;
}

namespace {
// Managers whose holiday the date ended at the entry of this pass of GAME's routine and who have not been given
// the screen yet (game thread only). The routine still applies its own tests after stop_for (not away, and the
// word at display+0x260000-0x7e54 clear): where they refuse, he gets no screen in this pass, as any manager.
std::vector<std::uint32_t> returning;
std::uint64_t given = 0;
}
bool pause(State& s) {
    // At 0x102a6048 the menu bar's handler has the tick of the manager's last step in r3 and computes, in r16,
    // the tick from which a manager on holiday continues by himself: the original waits 2,000 ms at every stop.
    s.wr<4>(16 * 4, std::uint32_t(s.r<4>(3 * 4)) + holiday_pause_ms);
    return true;
}
void stop_for(State& s) {
    // At 0x1011b540, r24 is the manager the routine is asking about and r3 whether he has news to read.
    const auto manager = std::uint32_t(s.r<4>(24 * 4));
    if (std::find(returning.begin(), returning.end(), manager) != returning.end()) s.wr<4>(3 * 4, 1);
}
void stop_screen(State& s) {
    // At 0x1011b574 the routine opens the manager's news (r3) asking it to go on by itself when nothing needs
    // reading (r4 = 1). The manager who just came back gets the plain screen instead, which waits for him.
    const auto found = std::find(returning.begin(), returning.end(), std::uint32_t(s.r<4>(3 * 4)));
    if (found == returning.end()) return;
    returning.erase(found);
    s.wr<4>(4 * 4, 0);
    ++given;
}
std::uint64_t screens_given() { return given; }

void slot(State& s) {
    // An entry lives for one pass of the routine only: one it did not use (the routine took its other branch,
    // or its own tests kept the screen from the manager) must not change a later pass, a later day or another career.
    returning.clear();
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
        // The routine below hands the screen to a manager only when he has news: this one gets it now, on the
        // day he chose, news or not (stop_for).
        if (c.u8(human_manager::record_at(c, humans, *index) + human_manager::record::away) == 0) returning.push_back(staff);
        career->set_holiday_return(entry.staff, std::nullopt);
    }
}
}

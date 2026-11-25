#pragma once
#include "career_rules.hpp"
#include <cstdint>

namespace cm::aot {
struct State;
// An optional return date for the Manager menu's "Go on holiday" (builder 0x101c9eb0, handler 0x101ca6e0).
//
// With no date the holiday is the original's, byte for byte. With one, the holiday ends by itself on that
// game date through the original's own way back, set_away(humans, 0, manager) (0x10147fd0), and it joins
// the original's other ways back (a match, the menu): whichever comes first. The date lives in the career's
// host member (CareerRules, the `holiday_return` line); no byte of the original save changes.
//
// The dialog gets two rows after the original's seven, in rows 10 and 11 of its list (the builder makes 13
// and uses 10): the date, a link that adds one day per click (the first click gives tomorrow), with the Calendar
// link after it, and the shortcuts None, +1 week, +2 weeks, +1 month, End of season (a text line holds five
// links). They are links of action 3 with the values 7..13 in the control's +0x48, so the original handler
// ignores them (values above 6 ask for the same screen again) and the host takes them first. The value is read
// as a signed byte there, so host values stay below 128.
//
// "End of season" (the owner's rule, 2026-11-24): the next 30 June after today for a manager whose nation's
// season runs across two years (the 2001/2002 style), the next 29 December for one whose season is within one
// calendar year (Brazil, the Nordic countries). The style is the manager's club's nation's season dates in the
// game's table of 34 nation calendars (TOC -0x7b44, record +0x2c the day the season starts, +0x3c the day it
// ends): a season that ends before the day it starts goes through the new year, one that ends after it does not.
// A manager with no club, or a nation the table does not list, gets 30 June.
//
// The Calendar link turns the same screen into a month calendar (the calendar mode): the builder's list becomes a
// grid of seven columns and eight rows (navigation, weekday names, six weeks) made with the game's own cells, and
// the footer holds None, Cancel and OK. The days after today are links that choose a draft date, the months and
// years are links, OK commits the draft to the dialog's choice and Cancel (or the handler's No, i.e. Escape)
// leaves it as it was; both return to the settings, which are untouched.
//
// The generated pages call these at: the opener's entry (0x101c9e70), the builder's intro text (0x101ca028), list
// grid (0x101ca080), first row (0x101ca088) and footer (0x101ca5e4), the handler's No (0x101ca754), Yes (0x101ca7b4)
// and setting dispatch (0x101ca884), the menu bar's Yes after set_away(1) (0x102a63c8), and GAME's per-slot routine
// (0x1011b330, with 0x1011b540 and 0x1011b574); see tools/holiday_return_audit.py.
namespace holiday_return {
// Pure calendar (the game's dates: day of the year from 0, leap years Gregorian).
unsigned year_length(unsigned year);
HolidayDate add_days(HolidayDate, int days);
// The same day of the month `months` later (clamped to the month's last day).
HolidayDate add_months(HolidayDate, int months);
// Whether `a` lies before `b`.
bool before(HolidayDate a, HolidayDate b);
// Months of the game's year (0 = January) and the day of the year each starts on.
unsigned month_index(HolidayDate);
unsigned days_in_month(unsigned month, unsigned year);
HolidayDate first_of_month(unsigned month, unsigned year);
// The dialog's links.
inline constexpr unsigned first_link = 7, link_day = 7, link_none = 8, link_week = 9, link_two_weeks = 10,
                          link_month = 11, link_season = 12, link_calendar = 13, last_link = 13;
// The calendar's links (all below 128: the handler reads the value as a signed byte).
namespace calendar_value {
inline constexpr unsigned previous_month = 20, next_month = 21, previous_year = 22, next_year = 23, none = 24,
                          cancel = 25, confirm = 26, day = 64;  // day + 1..31
}
// The month on show and the draft date, as the calendar mode keeps them (host state of the open dialog).
struct CalendarState { bool open = false; unsigned month = 0, year = 0; std::optional<HolidayDate> draft; };
const CalendarState& calendar();
// The month a calendar opens on: the month of `choice`, or of tomorrow without one (or with one not after today).
CalendarState calendar_opening(std::optional<HolidayDate> choice, HolidayDate today);
// The calendar after a click on `value` (a calendar_value) with `today`: pure navigation and choice. Months
// before tomorrow's and far past it are refused (unchanged), and so is a day that is not after today.
CalendarState calendar_after(CalendarState, unsigned value, HolidayDate today);
// The next 30 June (`calendar_year` false) or 29 December (true) strictly after `today`.
HolidayDate end_of_season(bool calendar_year, HolidayDate today);
// Whether the season of the manager in turn runs within one calendar year (see above); false without a club or nation row.
bool calendar_year_season(State&);
// The date after a click on link `value` (7..12) when the choice is `current` and today is `today`.
std::optional<HolidayDate> after_click(unsigned value, std::optional<HolidayDate> current, HolidayDate today, bool calendar_year = false);

// Whether the host can keep a date (a career with a rules member it interprets).
bool available(const State&);
// The game's date (the record the TOC's -0x7b80 cell points at).
HolidayDate today(State&);
// Hooks of the generated pages.
void opened(State&);       // the dialog opens: no choice yet
void append_rows(State&);  // the builder, before its footer, with its registers live
bool link(State&);         // the handler at its dispatch of a setting; true when it took the click (r0 = the value)
// The handler's No (action 1): true when it only closed the calendar (go back to the settings).
bool cancelled(State&);    // otherwise the dialog's No: the choice is dropped
bool confirmed(State&);    // the handler's Yes (action 2): true when it only confirmed the calendar's draft
// The builder in calendar mode: the intro paragraph shows the draft, the list grid is made seven by eight, and
// the grid and the footer are filled by the host (build_calendar takes the place of the settings, true when it did).
void intro(State&);        // before the intro paragraph is drawn (0x10744390)
void list_shape(State&);   // before the list grid is created (0x107198c0)
bool build_calendar(State&);
void started(State&);      // the menu bar after Yes put the manager away: binds the choice to that manager
void slot(State&);         // each time slot: ends the holiday on its date
// In that routine, where it has asked whether a manager has news (0x1011b540): a manager whose holiday the date
// ended this slot is treated as having some, so the routine gives him the screen that day.
void stop_for(State&);
// Where it then opens that manager's news (0x1011b574): the screen that waits for him, not the one that goes on.
void stop_screen(State&);
// How many times stop_screen gave a returning manager the waiting screen (for the check).
std::uint64_t screens_given();
}
}

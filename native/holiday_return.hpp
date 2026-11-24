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
// and uses 10): the date, a link that adds one day per click (the first click gives tomorrow), and the
// shortcuts None, +1 week, +2 weeks, +1 month. They are links of action 3 with the values 7..11 in the
// control's +0x48, so the original handler ignores them (values above 6 ask for the same screen again) and
// the host takes them first. "End of season" is left out: the game has no single end date to read, each
// league ends on its own.
//
// The generated pages call these at: the opener's entry (0x101c9e70), the builder's footer (0x101ca5e4),
// the handler's No (0x101ca754) and setting dispatch (0x101ca884), the menu bar's Yes after set_away(1) (0x102a63c8), and the
// entry of GAME's per-slot routine (0x1011b330); see tools/holiday_return_audit.py.
namespace holiday_return {
// Pure calendar (the game's dates: day of the year from 0, leap years Gregorian).
unsigned year_length(unsigned year);
HolidayDate add_days(HolidayDate, int days);
// The same day of the month `months` later (clamped to the month's last day).
HolidayDate add_months(HolidayDate, int months);
// Whether `a` lies before `b`.
bool before(HolidayDate a, HolidayDate b);
// The dialog's links.
inline constexpr unsigned first_link = 7, link_day = 7, link_none = 8, link_week = 9, link_two_weeks = 10,
                          link_month = 11, last_link = 11;
// The date after a click on link `value` (7..11) when the choice is `current` and today is `today`.
std::optional<HolidayDate> after_click(unsigned value, std::optional<HolidayDate> current, HolidayDate today);

// Whether the host can keep a date (a career with a rules member it interprets).
bool available(const State&);
// The game's date (the record the TOC's -0x7b80 cell points at).
HolidayDate today(State&);
// Hooks of the generated pages.
void opened(State&);       // the dialog opens: no choice yet
void append_rows(State&);  // the builder, before its footer, with its registers live
bool link(State&);         // the handler at its dispatch of a setting; true when it took the click (r0 = the value)
void cancelled(State&);    // the dialog's No: the choice is dropped
void started(State&);      // the menu bar after Yes put the manager away: binds the choice to that manager
void slot(State&);         // each time slot: ends the holiday on its date
}
}

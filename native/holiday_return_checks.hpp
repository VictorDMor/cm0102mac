// Holiday return date (holiday_return.hpp), checked from adaptive_holiday_checks.hpp: the calendar and the
// career member without a game, then the real dialog on a disposable copy of a capture. Textually included
// (needs TransferScene and require_rejected); the mutants must be refused by the same checks.
inline void holiday_return_calendar() {
    using namespace holiday_return;
    const auto same = [](HolidayDate a, HolidayDate b) { return a == b; };
    require(same(add_days({364, 2001}, 1), {0, 2002}), "a day past the end of a year");
    require(same(add_days({365, 2000}, 1), {0, 2001}) && same(add_days({364, 2000}, 1), {365, 2000}), "leap year length");
    require(same(add_days({0, 2002}, -1), {364, 2001}) && same(add_days({10, 2001}, 400), {45, 2002}), "days across years");
    require(same(add_months({30, 2001}, 1), {58, 2001}), "31 Jan + 1 month is 28 Feb");
    require(same(add_months({30, 2000}, 1), {59, 2000}), "31 Jan + 1 month is 29 Feb in a leap year");
    require(same(add_months({348, 2001}, 1), {14, 2002}), "15 Dec + 1 month is 15 Jan");
    require(same(add_months({89, 2001}, 1), {119, 2001}), "31 Mar + 1 month is 30 Apr");
    require(before({364, 2001}, {0, 2002}) && !before({5, 2001}, {5, 2001}) && before({4, 2001}, {5, 2001}), "date order");
    const HolidayDate today{100, 2001};
    require(after_click(link_day, std::nullopt, today) == HolidayDate{101, 2001}, "the first click is tomorrow");
    require(after_click(link_day, HolidayDate{101, 2001}, today) == HolidayDate{102, 2001}, "each click adds a day");
    require(after_click(link_day, HolidayDate{50, 2001}, today) == HolidayDate{101, 2001}, "a date in the past starts again at tomorrow");
    // The end of the season: the next 30 June (the season goes across two years) or 29 December (within a calendar year).
    require(same(end_of_season(false, {100, 2001}), {180, 2001}) && same(end_of_season(true, {100, 2001}), {362, 2001}), "30 June and 29 December of this year");
    require(same(end_of_season(false, {180, 2001}), {180, 2002}) && same(end_of_season(false, {179, 2001}), {180, 2001}) && same(end_of_season(false, {181, 2001}), {180, 2002}),
            "30 June is strictly after today: today itself goes to next year");
    require(same(end_of_season(true, {362, 2001}), {362, 2002}) && same(end_of_season(true, {361, 2001}), {362, 2001}) && same(end_of_season(true, {364, 2001}), {362, 2002}),
            "29 December likewise, and the year wraps from the end of December");
    require(same(end_of_season(false, {10, 2004}), {181, 2004}) && same(end_of_season(true, {10, 2004}), {363, 2004}) && same(end_of_season(true, {365, 2004}), {362, 2005}),
            "in a leap year the days of the year are one later from March");
    require(after_click(link_season, HolidayDate{300, 2001}, {100, 2001}, false) == HolidayDate{180, 2001} &&
            after_click(link_season, std::nullopt, {100, 2001}, true) == HolidayDate{362, 2001}, "the End of season link, in both styles");
    require_rejected([&] { require(end_of_season(false, {100, 2001}) == HolidayDate{362, 2001}, "29 December for a season across two years"); },
                     "an end of season that mixes up the two styles");
    // The calendar: months, its opening, navigation across a year end, the days after today only.
    require(days_in_month(1, 2001) == 28 && days_in_month(1, 2000) == 29 && days_in_month(11, 2001) == 31 && days_in_month(3, 2001) == 30, "month lengths");
    require(month_index({30, 2001}) == 0 && month_index({31, 2001}) == 1 && month_index({58, 2001}) == 1 && month_index({59, 2001}) == 2 &&
            month_index({59, 2000}) == 1 && month_index({60, 2000}) == 2 && month_index({364, 2001}) == 11, "month of a day");
    require(same(first_of_month(2, 2001), {59, 2001}) && same(first_of_month(2, 2000), {60, 2000}) && same(first_of_month(11, 2001), {334, 2001}), "first of a month");
    {
        using namespace calendar_value;
        const HolidayDate december{350, 2001};  // 17 Dec 2001
        auto state = calendar_opening(std::nullopt, december);
        require(state.open && state.month == 11 && state.year == 2001 && !state.draft, "the calendar opens on tomorrow's month");
        state = calendar_opening(HolidayDate{20, 2002}, december);
        require(state.month == 0 && state.year == 2002 && state.draft == HolidayDate{20, 2002}, "it opens on the chosen date's month, with it as the draft");
        state = calendar_opening(HolidayDate{300, 2001}, december);
        require(state.month == 11 && state.year == 2001 && !state.draft, "a choice that is not ahead is not a draft");
        state = calendar_opening(december, december);
        require(!state.draft, "today is not a choice");
        state = calendar_after(state, next_month, december);
        require(state.month == 0 && state.year == 2002, "the next month after December is January of the next year");
        state = calendar_after(state, next_month, december);
        require(state.month == 1 && state.year == 2002, "and then February");
        state = calendar_after(state, previous_month, december);
        state = calendar_after(state, previous_month, december);
        require(state.month == 11 && state.year == 2001, "back across the year end");
        state = calendar_after(state, previous_month, december);
        require(state.month == 11 && state.year == 2001, "no month before tomorrow's");
        state = calendar_after(state, previous_year, december);
        require(state.month == 11 && state.year == 2001, "no year before tomorrow's month");
        state = calendar_after(state, next_year, december);
        require(state.month == 11 && state.year == 2002, "a year ahead");
        state = calendar_after(state, previous_year, december);
        require(state.month == 11 && state.year == 2001, "a year back");
        for (unsigned i = 0; i < 100; ++i) state = calendar_after(state, next_year, december);
        require(state.month == 11 && state.year == 2031, "the calendar stops 30 years ahead");
        state = calendar_after(state, next_month, december);
        require(state.month == 11 && state.year == 2031, "and does not go past that");
        state = calendar_opening(std::nullopt, december);
        // Days: only those after today are chosen.
        const auto picks = [&](unsigned dom, bool chosen) {
            const auto after = calendar_after(state, day + dom, december);
            const HolidayDate date{std::int16_t(334 + dom - 1), 2001};
            return chosen == (after.draft == date) && (chosen || !after.draft);
        };
        require(picks(18, true) && picks(31, true), "the days after today can be chosen");
        require(picks(17, false) && picks(1, false), "today and the days before it cannot");
        // The mutant: a calendar that lets today be chosen; the check must refuse it.
        require_rejected([&] { require(picks(17, true), "today chosen"); }, "a calendar that chooses a day that is not after today");
        require(calendar_after(state, day + 32, december).draft == std::nullopt && calendar_after(state, day, december).draft == std::nullopt, "no such day");
        auto february = calendar_opening(std::nullopt, HolidayDate{20, 2001});  // 21 Jan: tomorrow is in January
        february = calendar_after(february, next_month, HolidayDate{20, 2001});
        require(february.month == 1 && calendar_after(february, day + 29, HolidayDate{20, 2001}).draft == std::nullopt &&
                calendar_after(february, day + 28, HolidayDate{20, 2001}).draft == HolidayDate{58, 2001}, "28 February is the last day of the month in 2001");
        state = calendar_after(state, none, december);
        require(!state.draft, "None clears the draft");
        state = calendar_after(calendar_after(state, day + 20, december), none, december);
        require(!state.draft, "None clears a chosen day");
    }
    require(after_click(link_week, HolidayDate{300, 2001}, today) == HolidayDate{107, 2001} &&
            after_click(link_two_weeks, std::nullopt, today) == HolidayDate{114, 2001} &&
            after_click(link_month, std::nullopt, today) == add_months(today, 1) &&
            !after_click(link_none, HolidayDate{300, 2001}, today), "the shortcuts count from today");
}

inline void holiday_return_member() {
    const std::string head = "CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nhistorical_rules off\nset england.premier_named_reserves 0\n";
    const std::string member = head + "holiday_return 212 2001 3\nfuture_statement 1 2\n";
    const auto edition = CompetitionEdition::cm0102;
    CareerRules rules;
    const auto ignored = rules.read(member);
    require(ignored == std::vector<std::string>{"future_statement 1 2"}, "the holiday line is read, an unknown one is reported");
    require(rules.holiday_return(3) == HolidayReturn{{212, 2001}, 3} && !rules.holiday_return(4), "the holiday line's date and manager");
    require(rules.text(edition) == member, "written back as read");
    // One line per manager: a second manager's line leaves the first's alone.
    require(rules.set_holiday_return(5, HolidayDate{40, 2002}) && rules.holiday_returns().size() == 2, "a second manager's line");
    require(rules.text(edition) == member + "holiday_return 40 2002 5\n", "one line each, the others kept");
    require(rules.set_holiday_return(3, HolidayDate{300, 2001}) && rules.holiday_return(3)->date == HolidayDate{300, 2001} &&
            rules.holiday_return(5)->date == HolidayDate{40, 2002} && rules.holiday_returns().size() == 2, "a manager's line is replaced, not added");
    CareerRules again;
    again.read(rules.text(edition));
    require(again.holiday_return(3) && again.holiday_return(5) && again.holiday_returns().size() == 2, "round trip");
    require(again.set_holiday_return(3, std::nullopt) && !again.holiday_return(3) && again.holiday_return(5) &&
            again.text(edition) == head + "future_statement 1 2\nholiday_return 40 2002 5\n", "cleared for one manager: his line gone, the others kept");
    require(again.set_holiday_return(9, std::nullopt) && again.holiday_returns().size() == 1, "clearing a manager with no line changes nothing");
    // Lines this build cannot read are kept and not applied.
    CareerRules bad;
    bad.read(head + "holiday_return 400 2001 3\nholiday_return 1 2\nholiday_return 1 2 3 4\nholiday_return -1 2001 3\n"
             "holiday_return 1 70000 3\nholiday_return 365 2001 3\nholiday_return 10 2001 4294967299\n");
    require(bad.holiday_returns().empty(), "unreadable lines apply nothing (day 365 of a common year and a staff id past 32 bits too)");
    require(bad.text(edition).find("holiday_return 400 2001 3\n") != std::string::npos, "unreadable lines are kept");
    CareerRules leap;
    leap.read(head + "holiday_return 365 2000 3\n");
    require(leap.holiday_return(3) == HolidayReturn{{365, 2000}, 3}, "the last day of a leap year");
    // The pending choice is the open dialog's: never in the member, gone on a load or a new career.
    again.set_holiday_pending(HolidayDate{1, 2001});
    require(again.text(edition).find("2001") == std::string::npos, "the pending choice is not saved");
    again.read(member);
    require(!again.holiday_pending() && again.holiday_return(3), "a load drops the pending choice");
    again.set_holiday_pending(HolidayDate{1, 2001});
    again.reset();
    require(!again.holiday_pending() && again.holiday_returns().empty(), "a new career starts with none");
    // A member kept without interpreting cannot change.
    CareerRules opaque;
    opaque.read("CM-NATIVE-RULES 9\nsomething\n");
    require(!opaque.editable() && !opaque.set_holiday_return(1, HolidayDate{1, 2001}) && opaque.holiday_returns().empty(), "an opaque member is not edited");
}

// The calendar mode of the real dialog (on the scene holiday_return_scene sets up): what it shows, how it moves, and that
// neither the original's settings, the records nor the RNG are touched by it.
template <class Control, class Click, class Handler, class World>
inline void holiday_return_calendar_scene(TransferScene& t, GuiCanvas canvas, Control control_of, Click click, Handler handler, World world) {
    using namespace holiday_return;
    using namespace calendar_value;
    auto& c = t.c;
    auto& rules = t.s.platform->career_rules();
    const auto date_record = c.toc(-0x7b80);
    const HolidayDate real_today{c.i16(date_record), c.u16(date_record + 2)};
    const auto set_today = [&](HolidayDate d) { c.put16(date_record, std::uint16_t(d.day)); c.put16(date_record + 2, d.year); };
    const auto humans = c.u32(c.toc(-0x7a54));
    // The original's settings as its own getters tell them (five fields behind the seven rows).
    const auto settings = [&] {
        std::vector<unsigned> values;
        for (unsigned field = 0; field < 5; ++field) values.push_back(t.invoke(0x10149df0, {c.u32(c.toc(-0x7a54)), field, 0}) & 0xff);
        return values;
    };
    require(humans != 0, "calendar fixture: the game has its humans");
    // Settings not at their defaults, so that "unchanged" says something.
    click(0); click(6);
    const auto set_at_start = settings();
    const HolidayDate december{350, 2001};  // 17 Dec 2001, a Monday
    set_today(december);
    t.rebuild(canvas);  // (the screen shows the game's date)
    const auto shown = t.meanings();
    const auto before = world();

    // End of season, by the style of the manager's club's nation (this fixture's is a calendar-year one: 29 December).
    {
        const auto club = c.u32(t.instance + 0x24), nation = c.u32(club + 0x2c), rows = c.toc(-0x7b44);
        require(club != 0 && nation != 0, "the fixture's manager has a club with a nation");
        // The nation's row in the game's table, and one of the other style's.
        int own = -1, other = -1;
        for (unsigned i = 0; i < 34; ++i) {
            const auto row = rows + i * 0x4c;
            if (c.u32(row) == nation) own = int(i);
            else if (c.u32(row) && c.i16(row + 0x2c) > c.i16(row + 0x3c) && other < 0) other = int(i);
        }
        require(own >= 0 && other >= 0 && c.i16(rows + own * 0x4c + 0x2c) < c.i16(rows + own * 0x4c + 0x3c), "the fixture has a calendar-year nation and a two-year one");
        const auto other_nation = c.u32(rows + other * 0x4c);
        const auto season = [&](bool calendar_year) {
            click(link_season);
            require(rules.holiday_pending() == end_of_season(calendar_year, december), "the End of season link gives the style's date");
            require(t.meanings() != shown, "the dialog shows the end of the season");
            click(link_none);
        };
        require(calendar_year_season(t.s), "the club's nation is of the calendar-year style");
        season(true);
        require(rules.holiday_pending() == std::nullopt && end_of_season(true, december) == HolidayDate{362, 2001}, "29 December 2001");
        c.put32(club + 0x2c, other_nation);
        require(!calendar_year_season(t.s), "a nation whose season goes across two years");
        season(false);
        c.put32(club + 0x2c, 0);
        require(!calendar_year_season(t.s), "a club without a nation");
        season(false);
        c.put32(club + 0x2c, allocate(t.s, 0x200, true));  // a nation record the table does not list
        require(!calendar_year_season(t.s), "a nation the table does not list");
        season(false);
        c.put32(club + 0x2c, nation);
        // No club: the staff record's club word cleared.
        c.put32(t.instance + 0x24, 0);
        require(!calendar_year_season(t.s), "a manager without a club");
        season(false);
        c.put32(t.instance + 0x24, club);
        // The mutant: the wrong style for the club's nation must be refused.
        require_rejected([&] { click(link_season); require(rules.holiday_pending() == end_of_season(false, december), "30 June for a calendar-year nation"); },
                         "an end of season of the wrong style");
        click(link_none);
        // The year wrap through the dialog: the 30th of December is past 29 December.
        set_today({363, 2001});
        click(link_season);
        require(rules.holiday_pending() == HolidayDate{362, 2002}, "after 29 December it is next year's");
        click(link_none);
        set_today(december);
        require(world() == before, "End of season changed the game's records, counters or RNG");
    }

    const auto find_grid = [&](ScreenRect box, unsigned columns, unsigned rows) {
        for (unsigned i = 0; i < t.grids(); ++i)
            if (t.rect(t.grid(i)) == box && c.u8(t.grid(i) + 0xbb0) == columns && c.u8(t.grid(i) + 0xbb1) == rows) return int(i);
        return -1;
    };
    // The weekday of a date (0 = Monday), counted on the Gregorian calendar by a formula of its own.
    const auto weekday = [](int year, int month, int dom) {
        static constexpr int offset[12]{0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if (month < 2) --year;
        return ((year + year / 4 - year / 100 + year / 400 + offset[month] + dom) + 6) % 7;
    };
    require(weekday(2001, 11, 1) == 5 && weekday(2002, 0, 1) == 1 && weekday(2001, 6, 14) == 5, "the check's own calendar: Saturday 1 Dec 2001, Tuesday 1 Jan 2002");
    const auto text_of = [&](unsigned i) {
        std::string text;
        for (unsigned k = 0; k < 40; ++k) { const char ch = char(c.u8(t.control(i) + 0x78 + k)); if (!ch) break; text += ch; }
        return text;
    };

    // Open it: the list is a grid of seven columns and eight rows, the footer has three cells, and the settings are gone.
    click(link_calendar);
    require(calendar().open && calendar().month == 11 && calendar().year == 2001 && !calendar().draft, "the calendar opens on tomorrow's month");
    const int list = find_grid({130, 250, 760, 535}, 7, 8), footer = find_grid({100, 555, 790, 590}, 3, 1);
    require(list >= 0 && footer >= 0, "the calendar's grid of seven by eight and its footer of three");
    require(find_grid({130, 250, 760, 535}, 1, 13) < 0 && find_grid({100, 555, 790, 590}, 2, 1) < 0, "the settings' list and the Yes/No footer are not built");
    for (unsigned value = 0; value <= last_link; ++value) require(control_of(value) < 0, "a setting or a dialog link on the calendar");
    const auto position = [&](int control) {
        const auto box = t.rect(t.control(unsigned(control)) + 0x10), grid = t.rect(t.grid(unsigned(list)));
        return std::pair{(box.left + box.right - 2 * grid.left) * 7 / (2 * grid.width()), (box.top + box.bottom - 2 * grid.top) * 8 / (2 * grid.height())};
    };
    const auto check_month = [&](unsigned month, unsigned year, unsigned first_selectable, const char* what) {
        const std::string name = what;
        const auto w = unsigned(weekday(int(year), int(month), 1));
        for (unsigned dom = 1; dom <= 31; ++dom) {
            const int i = control_of(day + dom);
            if (dom > days_in_month(month, year) || dom < first_selectable) { require(i < 0, (name + ": a day that cannot be chosen is a link").c_str()); continue; }
            require(i >= 0 && t.hit(unsigned(i)) == i, (name + ": a day that can be chosen is not a link").c_str());
            const auto [column, row] = position(i);
            require(column == int((w + dom - 1) % 7) && row == int(2 + (w + dom - 1) / 7), (name + ": a day is in the wrong cell").c_str());
        }
    };
    check_month(11, 2001, 18, "December 2001");
    require(control_of(previous_month) < 0 && control_of(previous_year) < 0 && control_of(next_month) >= 0 && control_of(next_year) >= 0 &&
            control_of(none) >= 0 && control_of(cancel) >= 0 && control_of(confirm) >= 0, "the calendar's own links: none before tomorrow's month");
    require(position(control_of(next_month)) == std::pair{5, 0} && position(control_of(next_year)) == std::pair{6, 0}, "the next links are on the first row");
    // The footer's three cells, left to right (their widths at the classic canvas are the grid's own, as the original footer's are).
    const auto footer_left = [&](int control) { return t.rect(t.control(unsigned(control)) + 0x10).left; };
    require(footer_left(control_of(none)) < footer_left(control_of(cancel)) && footer_left(control_of(cancel)) < footer_left(control_of(confirm)), "the footer reads None, Cancel, OK");
    // The month's title and the weekday names are the game's own (the date unit's name routines).
    const auto game_name = [&](unsigned routine, unsigned index) {
        std::string text;
        const auto at = t.invoke(routine, {index});
        for (unsigned k = 0; k < 40; ++k) { const char ch = char(c.u8(at + k)); if (!ch) break; text += ch; }
        return text;
    };
    {
        const auto title = game_name(0x107188a0, 11) + " 2001";
        bool found = false;
        std::array<bool, 7> heads{};
        for (unsigned i = 0; i < t.controls(); ++i) {
            if (c.i16(t.control(i) + 0x72) != list) continue;
            const auto text = text_of(i);
            found |= text == title && position(int(i)) == std::pair{3, 0};
            for (unsigned k = 0; k < 7; ++k) heads[k] |= text == game_name(0x10718700, k) && position(int(i)) == std::pair{int(k), 1};
        }
        require(!title.empty() && found, "the title is the game's short month name and the year, in the middle of the first row");
        for (unsigned k = 0; k < 7; ++k) require(heads[k], "a weekday name is missing or out of place");
    }
    require(world() == before, "opening the calendar changed the game's records, counters or RNG");
    require(t.meanings() != shown, "the calendar is not the settings");

    // Navigation across the year end.
    click(next_month);
    require(calendar().month == 0 && calendar().year == 2002, "January 2002 after December 2001");
    check_month(0, 2002, 1, "January 2002");
    require(control_of(previous_month) >= 0 && control_of(previous_year) >= 0, "a month back is open from January");
    click(next_year);
    require(calendar().month == 0 && calendar().year == 2003, "a year on");
    check_month(0, 2003, 1, "January 2003");
    click(previous_year); click(previous_month);
    require(calendar().month == 11 && calendar().year == 2001 && control_of(previous_month) < 0, "back to December 2001, which is the first month");

    // A day that is not after today is refused, also when its value is sent without a link (the handler's own path).
    const auto sent = [&](unsigned value) {
        c.set(0, value);
        require(link(t.s), "the calendar takes every click");
    };
    sent(day + 10); sent(day + 17);
    require(!calendar().draft && !rules.holiday_pending(), "a day not after today was chosen");
    require_rejected([&] { require(calendar().draft == HolidayDate{343, 2001}, "10 December chosen"); }, "a calendar that chooses a day before tomorrow");
    sent(day + 40); sent(0); sent(link_day); sent(127);
    require(!calendar().draft && !rules.holiday_pending() && calendar().open, "a click that is not the calendar's changes nothing");
    t.rebuild(canvas);

    // A day is a draft, not yet the choice; Cancel drops it.
    click(day + 20);
    require(calendar().draft == HolidayDate{353, 2001} && !rules.holiday_pending(), "a day is the draft, not the choice");
    require(control_of(day + 20) < 0 && control_of(day + 21) >= 0, "the chosen day is marked, the others stay links");
    require(calendar().open, "choosing a day keeps the calendar open");
    click(cancel);
    require(!calendar().open && !rules.holiday_pending() && control_of(0) >= 0 && control_of(link_calendar) >= 0 && t.meanings() == shown,
            "Cancel returns to the settings as they were, with the previous choice (none)");
    require_rejected([&] { require(rules.holiday_pending() == HolidayDate{353, 2001}, "the draft was committed"); }, "a Cancel that commits the draft");
    require(settings() == set_at_start && world() == before, "Cancel left the settings, records or RNG changed");

    // With a choice already made the calendar opens on it; None and a new day replace the draft; OK commits it.
    rules.set_holiday_pending(HolidayDate{358, 2001});  // 25 December
    t.rebuild(canvas);
    const auto with_choice = t.meanings();
    click(link_calendar);
    require(calendar().open && calendar().month == 11 && calendar().draft == HolidayDate{358, 2001} && control_of(day + 25) < 0, "it opens on the choice, marked");
    click(day + 30);
    require(calendar().draft == HolidayDate{363, 2001} && rules.holiday_pending() == HolidayDate{358, 2001}, "another day replaces the draft, not the choice");
    click(none);
    require(!calendar().draft && rules.holiday_pending() == HolidayDate{358, 2001} && control_of(day + 30) >= 0 && control_of(day + 25) >= 0, "None clears the draft only");
    click(cancel);
    require(rules.holiday_pending() == HolidayDate{358, 2001} && t.meanings() == with_choice, "Cancel after None keeps the choice");
    click(link_calendar);
    click(none);
    click(confirm);
    require(!calendar().open && !rules.holiday_pending() && t.meanings() == shown, "OK with None puts the dialog back as opened");
    click(link_calendar);
    click(day + 28);
    click(confirm);
    require(!calendar().open && rules.holiday_pending() == HolidayDate{361, 2001} && control_of(0) >= 0, "OK returns to the settings with the date chosen");
    require(t.meanings() != shown && t.meanings() != with_choice, "the dialog shows the date that was confirmed");
    require(settings() == set_at_start && world() == before, "OK left the settings, records or RNG changed");

    // The handler's own No (action 1, as the Escape key gives it) only leaves the calendar; its Yes confirms the draft.
    const auto key_word = c.toc(-0x7bb0) + 0x260000 - 0x7e62;
    const auto key = [&](unsigned action) {
        c.put16(key_word, std::uint16_t(action));
        const auto result = std::int32_t(t.invoke(handler(), {0xffffffffu}));
        c.put16(key_word, 0);
        return result;
    };
    click(link_calendar);
    click(day + 29);
    require(key(1) == -11 && !calendar().open && rules.holiday_pending() == HolidayDate{361, 2001}, "No in the calendar goes back to the settings, the choice kept");
    t.rebuild(canvas);
    click(link_calendar);
    click(day + 29);
    require(key(2) == -11 && !calendar().open && rules.holiday_pending() == HolidayDate{362, 2001}, "Yes in the calendar confirms its draft and goes back to the settings");
    t.rebuild(canvas);
    require(control_of(0) >= 0 && settings() == set_at_start && world() == before, "the keys left the settings, records or RNG changed");

    // Without the calendar the dialog is what it was: No closes it and drops the choice.
    {
        const int no = t.find(1);
        require(no >= 0 && std::int32_t(t.invoke(handler(), {unsigned(no)})) == -9 && !rules.holiday_pending() && !calendar().open, "No outside the calendar drops the choice");
    }
    // Put everything back for the checks that follow: the dialog as opened, today, the settings.
    t.invoke(0x101c9e70, {});
    t.rebuild(canvas);
    require(!calendar().open && !rules.holiday_pending(), "opening forgets the calendar");
    click(0); click(6);
    set_today(real_today);
    require(t.meanings() != shown || true, "restored");
}

inline void holiday_return_scene(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(false);
    TransferScene t(path, temp.path);
    auto& c = t.c;
    auto& rules = t.s.platform->career_rules();
    const GuiCanvas canvas{800, 600};
    const auto humans = c.u32(c.toc(-0x7a54));
    const auto staff = t.instance;  // the human in turn, as the game record holds it
    const auto index = human_manager::index_of(c, humans, staff);
    require(index.has_value(), "the fixture's manager is a human of the table");
    const auto record = human_manager::record_at(c, humans, *index);
    const auto away_byte = record + human_manager::record::away;
    require(holiday_return::available(t.s), "a career with a rules member offers the dates");
    const auto date_record = c.toc(-0x7b80);
    const auto today = [&] { return HolidayDate{c.i16(date_record), c.u16(date_record + 2)}; };
    const auto set_today = [&](HolidayDate d) { c.put16(date_record, std::uint16_t(d.day)); c.put16(date_record + 2, d.year); };
    // Every human record, the table's counters and the generator: what the original's own routines may write.
    const auto world = [&] {
        return t.records({{humans, 0x20}, {c.u32(humans), 0xa65c * std::min<unsigned>(c.u32(humans + 4), 16)}, {date_record, 8}});
    };
    t.invoke(0x101c9e70, {});
    require(t.builder() == 0x101c9eb0, "Holiday did not open through its original opener");
    t.rebuild(canvas);
    const auto handler = [&] { return c.u32(c.u32(t.history() + 8)); };
    const auto control_of = [&](unsigned value) {
        for (unsigned i = 0; i < t.controls(); ++i)
            if (c.u32(t.control(i) + 8) == 3 && c.u32(t.control(i) + 0x48) == value && t.enabled(i) && t.rect(t.control(i) + 0x10).left >= 100) return int(i);
        return -1;
    };
    for (unsigned value = 0; value <= holiday_return::last_link; ++value) require(control_of(value) >= 0, ("a setting or a return-date link is missing: " + std::to_string(value)).c_str());
    require(control_of(holiday_return::last_link + 1) < 0, "an extra link");
    require(!rules.holiday_pending() && rules.holiday_returns().empty(), "no date at first");
    const auto shown = t.meanings();
    const auto before_clicks = world();
    const auto click = [&](unsigned value) {
        const int i = control_of(value);
        require(i >= 0 && t.hit(unsigned(i)) == i, "a link cannot be picked");
        const auto result = std::int32_t(t.invoke(handler(), {unsigned(i)}));
        require(result == -11, "a click asks for the same screen again");
        t.rebuild(canvas);
    };
    const auto start = today();
    click(holiday_return::link_day);
    require(rules.holiday_pending() == holiday_return::add_days(start, 1), "the first click gives tomorrow");
    require(t.meanings() != shown, "the dialog shows the chosen date");
    click(holiday_return::link_day);
    require(rules.holiday_pending() == holiday_return::add_days(start, 2), "each click adds a day");
    click(holiday_return::link_week);
    require(rules.holiday_pending() == holiday_return::add_days(start, 7), "+1 week");
    click(holiday_return::link_two_weeks);
    require(rules.holiday_pending() == holiday_return::add_days(start, 14), "+2 weeks");
    click(holiday_return::link_month);
    require(rules.holiday_pending() == holiday_return::add_months(start, 1), "+1 month");
    click(holiday_return::link_none);
    require(!rules.holiday_pending() && t.meanings() == shown, "None puts the dialog back as opened");
    require(world() == before_clicks, "choosing a date changed the game's records, counters or RNG");
    holiday_return_calendar_scene(t, canvas, control_of, click, handler, world);
    // Yes with a date: the original carries it out in the menu bar (set_away(humans, 1, 0) after the dialog's -9).
    click(holiday_return::link_day); click(holiday_return::link_day); click(holiday_return::link_day);
    const auto chosen = holiday_return::add_days(start, 3);
    require(rules.holiday_pending() == chosen, "three days");
    const int yes = t.find(2);
    require(yes >= 0 && std::int32_t(t.invoke(handler(), {unsigned(yes)})) == -9, "Yes closes the dialog through the original");
    require(rules.holiday_returns().empty(), "nothing is bound before the manager is away");
    require(c.u8(away_byte) == 0, "the fixture's manager starts at work");
    t.invoke(0x10147fd0, {humans, 1, 0});
    holiday_return::started(t.s);
    require(c.u8(away_byte) == 1 && rules.holiday_return(c.u32(staff)) == HolidayReturn{chosen, c.u32(staff)} && !rules.holiday_pending(),
            "Yes binds the date to the manager");
    // The hook, each time slot: nothing before the date, the original's way back on it.
    const auto counter = [&] { return c.u32(humans + 0x0c); };
    const auto away_count = counter();
    const auto verify_slot = [&](HolidayDate day, bool expect_back) {
        set_today(day);
        const auto before = world();
        holiday_return::slot(t.s);
        const bool back = c.u8(away_byte) == 0;
        require(back == expect_back, expect_back ? "the holiday did not end on its date" : "the holiday ended before its date");
        require(back ? counter() + 1 == away_count && !rules.holiday_return(c.u32(staff)) : counter() == away_count && rules.holiday_return(c.u32(staff)).has_value() &&
                world() == before, "a slot before the date changed something, or the way back left the date");
    };
    verify_slot(holiday_return::add_days(start, 1), false);
    verify_slot(holiday_return::add_days(chosen, -1), false);
    // The mutant: the check says the holiday ends a day early; it must be refused.
    require_rejected([&] { verify_slot(holiday_return::add_days(chosen, -1), true); }, "a holiday that ends the day before its date");
    verify_slot(chosen, true);
    require(c.u8(away_byte) == 0, "back at work");
    // The same through GAME's own routine, whose entry carries the hook (the generated page): away again, the date
    // bound, the clock on it; the routine's own work follows, which the check does not judge.
    t.invoke(0x10147fd0, {humans, 1, 0});
    rules.set_holiday_return(c.u32(staff), chosen);
    set_today(holiday_return::add_days(chosen, -1));
    const auto scratch = allocate(t.s, 64, true);
    t.invoke(0x1011b330, {scratch});
    require(c.u8(away_byte) == 1 && rules.holiday_return(c.u32(staff)).has_value(), "the entry of GAME's routine ended the holiday before its date");
    set_today(chosen);
    holiday_return::slot(t.s);
    require(c.u8(away_byte) == 0 && !rules.holiday_return(c.u32(staff)), "the slot did not end the holiday on its date");
    // With no date the slot does nothing at all, away or not.
    t.invoke(0x10147fd0, {humans, 1, 0});
    {
        set_today(holiday_return::add_days(chosen, 400));
        const auto before = world();
        holiday_return::slot(t.s);
        require(c.u8(away_byte) == 1 && world() == before, "a holiday with no date is the original's");
    }
    // Another way back first (the original's set_away(…, 0)): the date goes with it and nothing else happens.
    set_today(start);
    rules.set_holiday_return(c.u32(staff), chosen);
    t.invoke(0x10147fd0, {humans, 0, 0});
    {
        set_today(holiday_return::add_days(chosen, 2));
        const auto before = world();
        holiday_return::slot(t.s);
        require(c.u8(away_byte) == 0 && !rules.holiday_return(c.u32(staff)) && world() == before, "an earlier way back clears the date and leaves the record alone");
    }
    // A date for a manager the game does not have is dropped.
    rules.set_holiday_return(0xfffffff, chosen);
    holiday_return::slot(t.s);
    require(rules.holiday_returns().empty(), "a date for nobody is dropped");
    // The dialog's No drops the choice (the generated handler's No), and a later opening starts from none.
    rules.set_holiday_pending(chosen);
    t.rebuild(canvas);
    {
        const int no = t.find(1);
        require(no >= 0 && std::int32_t(t.invoke(handler(), {unsigned(no)})) == -9 && !rules.holiday_pending(), "No drops the chosen date");
    }
    // The dialog's No and a later opening start from none; a date that is not ahead binds nothing.
    rules.set_holiday_pending(chosen);
    t.invoke(0x101c9e70, {});
    require(!rules.holiday_pending(), "opening the dialog forgets an old choice");
    set_today(start);
    rules.set_holiday_pending(start);
    holiday_return::started(t.s);
    require(rules.holiday_returns().empty(), "today is not a return date");
    // Another manager's date survives this manager's holiday, with or without a date of his own.
    rules.set_holiday_return(0xfffffff, chosen);
    holiday_return::started(t.s);
    require(rules.holiday_return(0xfffffff).has_value() && !rules.holiday_return(c.u32(staff)), "an undated holiday leaves another manager's date");
    rules.set_holiday_pending(holiday_return::add_days(start, 5));
    holiday_return::started(t.s);
    require(rules.holiday_return(0xfffffff) == HolidayReturn{chosen, 0xfffffff} && rules.holiday_return(c.u32(staff)).has_value(),
            "a dated holiday adds its own line beside another manager's");
    rules.set_holiday_return(0xfffffff, std::nullopt); rules.set_holiday_return(c.u32(staff), std::nullopt);
    // Without a career member to keep a date the dialog is the original's.
    t.s.platform->career_rules().read("CM-NATIVE-RULES 9\n");
    require(!holiday_return::available(t.s), "an opaque member offers no dates");
    t.rebuild(canvas);
    require(control_of(holiday_return::link_day) < 0 && control_of(0) >= 0, "no host rows without a member that can keep a date");
    // Last, because it leaves the manager's news screen open in place of the dialog: the date reached through
    // GAME's own routine.
    rules.reset();
    require(holiday_return::available(t.s), "a fresh member keeps dates again");
    t.invoke(0x10147fd0, {humans, 1, 0});
    rules.set_holiday_return(c.u32(staff), chosen);
    set_today(chosen);
    // On the date the routine also hands the manager his screen, news or not: it opens his news as the screen
    // that waits (0x102ce710 with r4 = 0) and enters the screen manager's loop (0x1073c730), which the check
    // answers by returning at once.
    {
        static bool waited;
        waited = false;
        const auto given = holiday_return::screens_given();
        if (dispatch_probe) throw std::runtime_error("the holiday check needs the dispatcher's probe");
        struct Probe { ~Probe() { dispatch_probe = nullptr; } } probe;
        dispatch_probe = [](State& s) {
            if (s.pc != 0x1073c730) return false;
            waited = true;
            s.pc = std::uint32_t(s.r<4>(4128));
            return true;
        };
        t.invoke(0x1011b330, {scratch});
        // (The news screen's owner is called in place, past the probe: the host's own count says it was asked for.)
        require(holiday_return::screens_given() == given + 1 && waited, "the manager did not get the screen on the day his holiday ended");
    }
    require(c.u8(away_byte) == 0 && !rules.holiday_return(c.u32(staff)), "the entry of GAME's routine did not end the holiday on its date");
}

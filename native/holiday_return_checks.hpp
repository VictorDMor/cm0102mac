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
    for (unsigned value = 0; value <= holiday_return::last_link; ++value) require(control_of(value) >= 0, "a setting or a return-date link is missing");
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
    t.invoke(0x1011b330, {scratch});
    require(c.u8(away_byte) == 0 && !rules.holiday_return(c.u32(staff)), "the entry of GAME's routine did not end the holiday on its date");
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
}

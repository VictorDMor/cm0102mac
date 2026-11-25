"""Holiday return date: where the generated pages call the host (native/holiday_return.cpp).

Each entry is the instruction the call goes in front of, with its text so a changed export stops the generator.
The call runs first and the instruction follows, except the handler's, which can take the click and go to the
handler's "ask for the same screen again" return (0x101caa68)."""
HOLIDAY_RETURN_HOOKS = {
    # The dialog's opener: no date is chosen yet.
    0x101c9e70: ("mfspr r0,LR", "holiday_return::opened(s);"),
    # The builder, after the last setting's row and before the footer: the date row and the shortcuts.
    0x101ca5e4: ("li r4,0x64", "holiday_return::append_rows(s);"),
    # The handler's No (action 1): the choice is dropped; in calendar mode it only leaves the calendar.
    0x101ca754: ("addis r4,r5,0x26", "if (holiday_return::cancelled(s)) goto i_101caa68;"),
    # The handler's Yes (action 2): in calendar mode it only confirms the calendar's draft.
    0x101ca7b4: ("addis r5,r5,0x26", "if (holiday_return::confirmed(s)) goto i_101caa68;"),
    # The builder in calendar mode: the intro paragraph (before its text box), the list grid's shape (before its
    # creation), and the list's rows and the footer, which the host builds in place of the settings and Yes/No.
    0x101ca028: ("bl 0x10744390", "holiday_return::intro(s);"),
    0x101ca080: ("bl 0x107198c0", "holiday_return::list_shape(s);"),
    0x101ca088: ("or r25,r3,r3", "if (holiday_return::build_calendar(s)) goto i_101ca6b8;"),
    # The handler's dispatch of a setting by the control's value (r0): values 7..11 are the host's rows.
    0x101ca884: ("cmplwi r0,0x6", "if (holiday_return::link(s)) goto i_101caa68;"),
    # The menu bar, after Yes put the manager away (set_away(humans, 1, 0)), before the start tick is stored.
    0x102a63c8: ("bl 0x10746a30", "holiday_return::started(s);"),
    # GAME's per-time-slot routine: the holiday ends on its date.
    0x1011b330: ("mfspr r0,LR", "holiday_return::slot(s);"),
    # In that routine, after it asked whether the manager (r24) has news: the one who just came back gets the screen.
    0x1011b540: ("extsb. r0,r3", "holiday_return::stop_for(s);"),
    0x1011b574: ("bl 0x102ce710", "holiday_return::stop_screen(s);"),
    # The menu bar: how long a manager on holiday waits at a screen before continuing by himself (r16 = r3 + 2000).
    0x102a6048: ("addi r16,r3,0x7d0", "if (holiday_return::pause(s)) goto i_102a604c;"),
}

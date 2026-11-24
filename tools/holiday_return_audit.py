"""Holiday return date: where the generated pages call the host (native/holiday_return.cpp).

Each entry is the instruction the call goes in front of, with its text so a changed export stops the generator.
The call runs first and the instruction follows, except the handler's, which can take the click and go to the
handler's "ask for the same screen again" return (0x101caa68)."""
HOLIDAY_RETURN_HOOKS = {
    # The dialog's opener: no date is chosen yet.
    0x101c9e70: ("mfspr r0,LR", "holiday_return::opened(s);"),
    # The builder, after the last setting's row and before the footer: the date row and the shortcuts.
    0x101ca5e4: ("li r4,0x64", "holiday_return::append_rows(s);"),
    # The handler's No (action 1): the choice is dropped.
    0x101ca754: ("addis r4,r5,0x26", "holiday_return::cancelled(s);"),
    # The handler's dispatch of a setting by the control's value (r0): values 7..11 are the host's rows.
    0x101ca884: ("cmplwi r0,0x6", "if (holiday_return::link(s)) goto i_101caa68;"),
    # The menu bar, after Yes put the manager away (set_away(humans, 1, 0)), before the start tick is stored.
    0x102a63c8: ("bl 0x10746a30", "holiday_return::started(s);"),
    # GAME's per-time-slot routine: the holiday ends on its date.
    0x1011b330: ("mfspr r0,LR", "holiday_return::slot(s);"),
}

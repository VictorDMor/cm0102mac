// UEFA Coefficients (0x10074d80), opened through its original opener 0x10074d00: the table at its top, scrolled
// to the middle and scrolled to its end (history slot 0 is the grid's scroll offset; the header row is the first
// row of the scrolled list). The table always has a scrollbar, so each canvas also shows the layout leaving it
// its gutter. See adaptive_paged_checks.hpp.
void uefa_coefficients_builder(const char* path) {
    paged_screen_fixture(path, "UEFA coefficients", 0x10074d80, ScreenRect{110, 145, 780, 535}, false, {
        {"top", [](TransferScene& t) { t.invoke(0x10074d00, {}); }},
        {"middle", [](TransferScene& t) { t.invoke(0x10074d00, {}); t.set_value(0, 17); }},
        {"end", [](TransferScene& t) { t.invoke(0x10074d00, {}); t.set_value(0, 35); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x10074d80), "UEFA coefficients gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}

// Manager Statistics (0x101c4460), opened through its original opener 0x101c42f0: the first page of the
// managed club's nation, a nation without statistics (a one-column table with a centered message) and two
// later pages. See adaptive_paged_checks.hpp.
void manager_statistics_builder(const char* path) {
    paged_screen_fixture(path, "Manager statistics", 0x101c4460, ScreenRect{110, 170, 780, 535}, false, {
        {"first page", [](TransferScene& t) { t.invoke(0x101c42f0, {}); }},
        {"no statistics", [](TransferScene& t) { t.invoke(0x101c42f0, {}); t.set_value(0, 0); }},
        {"page 3", [](TransferScene& t) { t.invoke(0x101c42f0, {}); t.set_value(1, 2); }},
        {"page 14", [](TransferScene& t) { t.invoke(0x101c42f0, {}); t.set_value(1, 13); }},
    }, [](TransferScene& t) {
        // Without a human-managed instance there is nothing to adapt.
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x101c4460), "Manager statistics gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}

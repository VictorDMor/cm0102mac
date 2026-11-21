// Manager Status (0x10338b40), opened through its original opener 0x10338a90: the table of the game's managers,
// here the one human manager of the capture (name, club, status) with the invisible link that spans its row.
// Only one human manager exists in a capture, so the fixture has one state; every canvas, the original input
// and the mutants are checked as in the other table screens. See adaptive_paged_checks.hpp.
void manager_status_builder(const char* path) {
    paged_screen_fixture(path, "Manager status", 0x10338b40, ScreenRect{110, 145, 780, 535}, false, {
        {"one manager", [](TransferScene& t) { t.invoke(0x10338a90, {}); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x10338b40), "Manager status gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}

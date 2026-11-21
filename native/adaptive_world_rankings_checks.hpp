// FIFA World Rankings (0x100745f0), opened through its original opener 0x10074550: the first page, a
// confederation with no nations (an empty table), a middle page and the last page. See adaptive_paged_checks.hpp.
void world_rankings_builder(const char* path) {
    paged_screen_fixture(path, "World rankings", 0x100745f0, ScreenRect{110, 170, 780, 535}, false, {
        {"first page", [](TransferScene& t) { t.invoke(0x10074550, {}); }},
        {"confederation", [](TransferScene& t) { t.invoke(0x10074550, {}); t.set_value(0, 1); }},
        {"page 3", [](TransferScene& t) { t.invoke(0x10074550, {}); t.set_value(2, 3); }},
        {"last page", [](TransferScene& t) { t.invoke(0x10074550, {}); t.set_value(2, 13); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x100745f0), "World rankings gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}

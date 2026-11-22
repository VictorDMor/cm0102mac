// The first two screens of arranging a friendly tour, opened through their original opener 0x10045800 (the club
// screens' action 0xb) and the original handlers on a disposable copy of an existing capture: Choose Country
// (0x10045930: available nations, all nations) and Choose Dates (0x10043b00: first, a middle and the last page of
// its pager, and one with a date chosen). Built with New Screens off and on at six canvases; the adaptive run
// keeps every control, command, record and RNG draw and only moves them: a table in a centered panel, the two
// name columns of the countries growing equally, the pager of the dates ending at the panel's right edge, and a
// compact footer. Mutants of each property must be refused by the same checker. See adaptive_paged_checks.hpp.
inline void friendly_step(TransferScene& t, unsigned control) {
    t.invoke(t.c.u32(t.c.u32(t.history() + 8)), {control});
    t.rebuild({801, 601});
}
inline void friendly_countries(TransferScene& t, bool all) {
    t.invoke(0x10045800, {t.managed});
    t.rebuild({801, 601});
    if (!all) return;
    // The filter menu's second entry (all nations).
    const int entry = t.find(1);
    require(entry >= 0, "Friendly fixture: filter entry missing");
    friendly_step(t, unsigned(entry));
}
inline void friendly_dates(TransferScene& t, unsigned page, bool chosen) {
    friendly_countries(t, false);
    const int nation = t.find(2);
    require(nation >= 0, "Friendly fixture: no nation to choose");
    friendly_step(t, unsigned(nation));
    const int next = t.find(4);
    require(next >= 0, "Friendly fixture: Next did not open after a nation was chosen");
    friendly_step(t, unsigned(next));
    for (unsigned p = 1; p < page; ++p) { const int more = t.find(3); require(more >= 0, "Friendly fixture: no next page"); friendly_step(t, unsigned(more)); }
    if (chosen) { const int date = t.find(8); require(date >= 0, "Friendly fixture: no available date"); friendly_step(t, unsigned(date)); }
}
inline void friendly_gate(TransferScene& t, unsigned builder) {
    const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
    require(!adaptive_screen_available(t.s, builder), "Friendly gate accepted a missing instance");
    t.c.put32(t.manager + 4, instance);
}
void friendly_countries_builder(const char* path) {
    paged_screen_fixture(path, "Friendly countries", 0x10045930, ScreenRect{110, 170, 780, 535}, false, {
        {"available nations", [](TransferScene& t) { friendly_countries(t, false); }},
        {"all nations", [](TransferScene& t) { friendly_countries(t, true); }},
    }, [](TransferScene& t) { friendly_gate(t, 0x10045930); }, {}, PagedOptions{2, false, 3});
}
void friendly_dates_builder(const char* path) {
    paged_screen_fixture(path, "Friendly dates", 0x10043b00, ScreenRect{110, 145, 780, 535}, false, {
        {"first page", [](TransferScene& t) { friendly_dates(t, 1, false); }},
        {"page five", [](TransferScene& t) { friendly_dates(t, 5, false); }},
        {"last page", [](TransferScene& t) { friendly_dates(t, 20, false); }},
        {"date chosen", [](TransferScene& t) { friendly_dates(t, 1, true); }},
    }, [](TransferScene& t) { friendly_gate(t, 0x10043b00); }, {}, PagedOptions{1, true});
}

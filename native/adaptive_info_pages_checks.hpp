// The Game menu's Credits (0x105e9ad0, opener 0x105e9a40) and Web Sites (0x1062c380, opener 0x1062c300), opened
// through their original openers on a disposable copy of an existing capture. Credits: the first section, the last
// section of its menu (the original handler picks it), the first section scrolled by the scrollbar's offset; both:
// also with no career, as when they are opened from the start menu (the layout reads no record). Built with New
// Screens off and on at six canvases; the adaptive run keeps every control, command, record and RNG draw and only
// moves them: a table in a centered panel with its measured text areas carried to the panel's middle, the
// scrollbar's gutter at the table's right edge and a compact footer. Mutants must be refused by the same checker.
// See adaptive_paged_checks.hpp.
inline void info_page_open(TransferScene& t, unsigned opener, bool career) {
    if (!career) t.c.put32(t.manager + 4, 0);
    t.invoke(opener, {});
    t.rebuild({801, 601});
}
inline void credits_section(TransferScene& t, bool last) {
    info_page_open(t, 0x105e9a40, true);
    int pick = -1;
    for (unsigned i = 0; i < t.controls(); ++i) if (t.c.u32(t.control(i) + 8) == 100 && t.enabled(i)) { pick = int(i); if (!last) break; }
    require(pick >= 0, "Credits fixture: no section menu entry");
    t.invoke(t.c.u32(t.c.u32(t.history() + 8)), {unsigned(pick)});
}
void credits_builder(const char* path) {
    paged_screen_fixture(path, "Credits", 0x105e9ad0, ScreenRect{110, 170, 780, 535}, false, {
        {"first section", [](TransferScene& t) { info_page_open(t, 0x105e9a40, true); }},
        {"last section", [](TransferScene& t) { credits_section(t, true); }},
        {"scrolled", [](TransferScene& t) { info_page_open(t, 0x105e9a40, true); t.set_value(0, 40); }},
        {"no career", [](TransferScene& t) { info_page_open(t, 0x105e9a40, false); }},
    }, [](TransferScene& t) {
        // Plain text pages: the gate is the same with or without a career.
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(adaptive_screen_available(t.s, 0x105e9ad0), "Credits gate refused a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}
void web_sites_builder(const char* path) {
    paged_screen_fixture(path, "Web sites", 0x1062c380, ScreenRect{110, 145, 780, 535}, false, {
        {"web sites", [](TransferScene& t) { info_page_open(t, 0x1062c300, true); }},
        {"no career", [](TransferScene& t) { info_page_open(t, 0x1062c300, false); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(adaptive_screen_available(t.s, 0x1062c380), "Web sites gate refused a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
}

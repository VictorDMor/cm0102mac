// Board Confidence (0x101c8fb0), opened through its original opener 0x101c8ed0 for three clubs of an existing
// capture: the managed club and two others, whose messages wrap to different lines. See adaptive_paged_checks.hpp.
void board_confidence_builder(const char* path) {
    const auto club_before = [](TransferScene& t) { return t.managed - 0x248 >= t.c.u32(t.c.toc(-0x7be8)) ? t.managed - 0x248 : t.managed + 0x248; };
    paged_screen_fixture(path, "Board confidence", 0x101c8fb0, ScreenRect{110, 155, 780, 540}, true, {
        {"managed club", [](TransferScene& t) { t.invoke(0x101c8ed0, {t.managed}); }},
        {"next club", [](TransferScene& t) { t.invoke(0x101c8ed0, {t.managed + 0x248}); }},
        {"previous club", [&](TransferScene& t) { t.invoke(0x101c8ed0, {club_before(t)}); }},
    }, [](TransferScene& t) {
        // Only a club of the database is a state the layout handles.
        const auto club = t.value(0);
        for (const auto bad : {0u, club + 4, t.c.u32(t.c.toc(-0x7be8)) - 0x248}) {
            t.set_value(0, bad);
            require(!adaptive_screen_available(t.s, 0x101c8fb0), "Board confidence gate accepted an invalid club");
        }
        t.set_value(0, club);
        require(adaptive_screen_available(t.s, 0x101c8fb0), "Board confidence gate rejected its own club");
    });
}

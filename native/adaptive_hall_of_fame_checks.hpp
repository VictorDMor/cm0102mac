// Hall of Fame (0x1033fcf0), opened through its original opener 0x1033fc60: the empty table of a new world and
// tables that the fixture fills by writing ranked records into the live Hall of Fame table (in the disposable
// copy of the capture; the original builder then reads them): a full table of twenty managers and a partial one.
// The table is part of the compared records, so the layout must leave it as it was. See adaptive_paged_checks.hpp.
std::vector<std::uint8_t> hall_of_fame_table(TransferScene& t) {
    return t.s.memory.copy_out(t.c.u32(t.c.toc(-0x7414)), 324000);
}
void hall_of_fame_rows(TransferScene& t, unsigned count) {
    auto& c = t.c;
    const auto table = c.u32(c.toc(-0x7414));
    for (unsigned category = 0; category < 27; ++category)
        for (unsigned k = 0; k < count; ++k) {
            // A record is 0x78 bytes: the manager's name, the club's name from +0x32, a date and the score at +0x68.
            const auto row = table + category * 12000 + k * 0x78;
            const std::string manager = "Manager Number " + std::to_string(k + 1) + (k % 3 == 0 ? " With A Long Name" : ""),
                              club = "Club of " + std::to_string(k + 1);
            for (unsigned i = 0; i <= manager.size(); ++i) c.put8(row + i, i < manager.size() ? manager[i] : 0);
            for (unsigned i = 0; i <= club.size(); ++i) c.put8(row + 0x32 + i, i < club.size() ? club[i] : 0);
            c.put32(row + 0x64, 11300 + k * 40); c.put32(row + 0x68, 1000 - k * 7);
        }
}
void hall_of_fame_builder(const char* path) {
    paged_screen_fixture(path, "Hall of Fame", 0x1033fcf0, ScreenRect{110, 170, 780, 535}, false, {
        {"empty", [](TransferScene& t) { t.invoke(0x1033fc60, {}); }},
        {"twenty managers", [](TransferScene& t) { hall_of_fame_rows(t, 20); t.invoke(0x1033fc60, {}); }},
        {"seven managers", [](TransferScene& t) { hall_of_fame_rows(t, 7); t.invoke(0x1033fc60, {}); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x1033fcf0), "Hall of Fame gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    }, hall_of_fame_table);
}

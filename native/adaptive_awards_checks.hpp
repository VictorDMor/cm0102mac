// The awards of a competition: the list of an award over the seasons (0x10000180, opener 0x10000070 with the award's
// subject) and the nominees of an award (0x100026d0, opener 0x100025b0 with the award entry), opened through their
// original openers on a disposable copy of an existing capture. The list: two awards (one with the Ver menu under the
// subtitle, one without); in this world nobody has won any award yet, so the table is one centered message. The nominees:
// the same award with no nominee (a message-less empty table), with a few, with a page and with more than a page
// (scrollbar), the records written straight into the award's nominee list in the format the builder reads (a person index and a
// club index per sixteen-byte record). Built with New Screens off and on at six canvases; the adaptive run keeps every
// control, command, record and RNG draw and only moves them. See adaptive_paged_checks.hpp.
inline std::uint32_t award_entry(TransferScene& t, unsigned category, unsigned index) {
    const auto table = t.c.u32(t.c.u32(t.c.toc(-0x7bb4)) + 8);
    const auto list = t.c.u32(table + category * 4);
    require(list && index < unsigned(t.c.i16(list + 8)), "Awards fixture: no such award");
    return t.c.u32(t.c.u32(list + 4) + index * 4);
}
inline void award_list_open(TransferScene& t, unsigned category) {
    t.invoke(0x10000070, {t.c.u32(award_entry(t, category, 0) + 8)});
}
inline void award_nominees_open(TransferScene& t, unsigned count) {
    auto& c = t.c;
    const auto entry = award_entry(t, 1, 0);
    const auto nominees = c.u32(entry + 0x24);
    require(nominees, "Awards fixture: the award has no nominee list");
    if (count) {
        const auto records = allocate(t.s, count * 16, true);
        const auto club_count = c.u32(c.toc(-0x7b50));
        for (unsigned i = 0; i < count; ++i) {
            c.put32(records + i * 16, 200 + i * 7);
            c.put32(records + i * 16 + 4, (i * 3) % club_count);
        }
        c.put32(nominees, records); c.put16(nominees + 4, count);
    }
    t.invoke(0x100025b0, {entry});
    if (!count) return;
    // The builder must show the records written above: linked players.
    t.rebuild({801, 601});
    unsigned named = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) == 19 && c.u32(at + 8) == 0x7d7) ++named;   // a player's link
    }
    require(named >= std::min(count, 16u), "Awards fixture: the nominees are not shown");
}
void awards_builder(const char* path) {
    paged_screen_fixture(path, "Award list", 0x10000180, ScreenRect{110, 170, 780, 535}, false, {
        {"award with the Ver menu", [](TransferScene& t) { award_list_open(t, 5); }},
        {"award without it", [](TransferScene& t) { award_list_open(t, 1); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x10000180), "Award list gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    });
    const auto nominees = [](TransferScene& t) { const auto list = t.c.u32(award_entry(t, 1, 0) + 0x24); auto bytes = t.s.memory.copy_out(list, 8);
        if (t.c.i16(list + 4) > 0) { const auto more = t.s.memory.copy_out(t.c.u32(list), t.c.i16(list + 4) * 16); bytes.insert(bytes.end(), more.begin(), more.end()); } return bytes; };
    paged_screen_fixture(path, "Award nominees", 0x100026d0, ScreenRect{110, 145, 780, 535}, false, {
        {"no nominee", [](TransferScene& t) { award_nominees_open(t, 0); }},
        {"three nominees", [](TransferScene& t) { award_nominees_open(t, 3); }},
        {"a page", [](TransferScene& t) { award_nominees_open(t, 16); }},
        {"more than a page", [](TransferScene& t) { award_nominees_open(t, 30); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x100026d0), "Award nominees gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    }, nominees);
}
// The friendly tour's invited clubs (0x10048450, opener 0x10048330 with a link to a club and a flag): the managed club's own
// tournament (the table at 170 with the "invite another" button above its right edge, disabled here) and another club's
// (the table at 145, no button). The capture invites nobody, so the table's cells are empty (their rows and columns still
// exist). The footer's first button answers action 2 in the first state and Back in the second.
inline void invited_clubs_open(TransferScene& t, bool managed) {
    unsigned other = 0;
    const auto clubs = t.c.u32(t.c.toc(-0x7be8));
    for (unsigned k = 0; k < t.c.u32(t.c.toc(-0x7b50)) && !other; ++k) if (clubs + k * 0x248 != t.managed) other = clubs + k * 0x248;
    require(other, "Invited clubs fixture needs a second club");
    const auto link = allocate(t.s, 4, true);
    t.c.put32(link, managed ? t.managed : other);
    t.invoke(0x10048330, {link, 0});
}
void invited_clubs_builder(const char* path) {
    const auto gate = [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x10048450), "Invited clubs gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    };
    paged_screen_fixture(path, "Invited clubs (own tournament)", 0x10048450, ScreenRect{110, 170, 780, 535}, false, {
        {"managed club", [](TransferScene& t) { invited_clubs_open(t, true); }},
    }, gate, {}, PagedOptions{1, false, 2});
    paged_screen_fixture(path, "Invited clubs (another club)", 0x10048450, ScreenRect{110, 145, 780, 535}, false, {
        {"another club", [](TransferScene& t) { invited_clubs_open(t, false); }},
    }, gate);
}

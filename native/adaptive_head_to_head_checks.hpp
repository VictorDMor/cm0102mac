// "Last meetings" (0x10041550), opened through its original opener 0x100413b0 with two clubs on a disposable copy of
// an existing capture: the managed club against a club of its nation, the same pair the other way round, and against a
// club of another nation. Built with New Screens off and on at six canvases; the adaptive run keeps every control,
// command, record and RNG draw and only moves them: the summary (eleven columns, one of them empty) and the meetings table
// as one panel centered under the title bars and at most 960 wide, the summary at its height and the meetings as tall as
// their rows can use, the widest column of each table taking the extra width, the toolbar menus on their sides of the
// panel and the Back/Next buttons right-aligned to it. The capture's world holds no earlier meetings, so the meetings'
// cells are empty (their rows and columns still exist). Mutants of each promise must be refused by the checker. Needs
// TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct HeadToHeadShape { int summary = -1, meetings = -1, footer = -1; };

inline HeadToHeadShape head_to_head_shape(TransferScene& t) {
    HeadToHeadShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        if (box == ScreenRect{110, 170, 780, 249} && columns == 11) r.summary = int(i);
        else if (box == ScreenRect{110, 254, 780, 535} && columns == 7) r.meetings = int(i);
        else if (rows == 1 && columns == 2 && box.left == 100 && box.height() == 35) r.footer = int(i);
    }
    return r;
}

inline void verify_head_to_head(TransferScene& t, GuiCanvas size, const HeadToHeadShape& shape, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const int extra = size.height - 600;
    const auto content = shell.content(170);
    const int width = std::min(960, content.width());
    const auto summary = t.rect(t.grid(shape.summary)), meetings = t.rect(t.grid(shape.meetings)), footer = t.rect(t.grid(shape.footer));
    // (At 800 pixels wide the summary legitimately keeps its classic rectangle: the meetings change height or the footer moves.)
    require((summary != ScreenRect{110, 170, 780, 249} || meetings != ScreenRect{110, 254, 780, 535}) && footer != ScreenRect{100, 555, 790, 590},
            (name + ": layout did not run, the tables stay at classic coordinates").c_str());
    require(summary.left == meetings.left && summary.right == meetings.right && summary.width() == width &&
            summary.left == content.left + (content.width() - width) / 2 && summary.right <= size.width - 10 &&
            std::abs((summary.left + summary.right) - (shell.title().left + shell.title().right)) <= 2,
            (name + ": tables not one panel centered under the title bars, bounded to 960").c_str());
    require(summary.top == 170 && summary.height() == 79, (name + ": summary changed height or place").c_str());
    const unsigned rows = c.u8(t.grid(shape.meetings) + 0xbb1);
    require(rows == 15 && c.u8(t.grid(shape.summary) + 0xbb1) == 3, (name + ": a table lost rows").c_str());
    require(meetings.top == 254 && meetings.height() == std::min(281 + extra, int(rows) * 28 + 16) && meetings.bottom <= size.height - 45,
            (name + ": meetings not as tall as their rows can use").c_str());
    // The Back/Next buttons: two equal cells right-aligned to the panel, in the shell's footer row.
    require(footer.right == meetings.right && footer.top == shell.footer().top && footer.bottom == shell.footer().bottom &&
            footer.width() <= 340 && footer.bottom <= size.height - 10, (name + ": footer not right-aligned to the panel").c_str());
    std::vector<std::pair<unsigned, ScreenRect>> buttons;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) == shape.footer) buttons.push_back({c.u32(at + 0x20), t.rect(at + 0x10)});
    }
    std::sort(buttons.begin(), buttons.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    require(buttons.size() == 2, (name + ": footer lost a button").c_str());
    for (const auto& [column, b] : buttons)
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && b.width() >= 60,
                (name + ": footer button outside its footer or too small").c_str());
    require(buttons[0].second.right <= buttons[1].second.left && std::abs(buttons[0].second.width() - buttons[1].second.width()) <= 1 &&
            buttons[1].second.right == footer.right && buttons[0].second.left == footer.left, (name + ": footer buttons unequal, overlapping or not flush").c_str());

    const int lead = summary.left - 110, trail = summary.right - 780;
    struct Cell { int owner; unsigned row, column; ScreenRect box; };
    std::vector<Cell> placed;
    unsigned menus = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        if (owner == shape.summary || owner == shape.meetings) {
            if (box.top == 0 && box.bottom == 0) continue;
            const auto& panel = owner == shape.summary ? summary : meetings;
            require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom,
                    (name + ": cell outside its table").c_str());
            // Only the widest column of each table took the extra width.
            const int grown = box.width() - before.width();
            require(grown >= 0 && (grown == 0 || before.width() >= 200), (name + ": a narrow column grew or a cell shrank").c_str());
            placed.push_back({owner, c.u32(at + 0x24), c.u32(at + 0x20), box});
        } else if (owner < 0) {
            if (before == ScreenRect{100, 80, 790, 125}) require(box == shell.subtitle(), (name + ": subtitle did not follow the shell").c_str());
            else if (before == ScreenRect{100, 10, 790, 70}) require(box == shell.title(), (name + ": title did not follow the shell").c_str());
            else if (before.top == 145 && before.bottom == 165 && before.left < 400) { require(box == ScreenRect{before.left + lead, before.top, before.right + lead, before.bottom}, (name + ": left menu did not follow the panel's left edge").c_str()); ++menus; }
            else if (before.top == 145 && before.bottom == 165) { require(box == ScreenRect{before.left + trail, before.top, before.right + trail, before.bottom}, (name + ": right menu did not follow the panel's right edge").c_str()); ++menus; }
            else if (before.left >= 90) require(box == before, (name + ": an unrelated standalone control moved").c_str());
        }
    }
    require(menus == 2 && placed.size() >= 60, (name + ": fixture found too few menus or cells to check").c_str());
    // Cells keep their row and column order without overlapping, within each table: across a row, and row below row.
    std::sort(placed.begin(), placed.end(), [](const Cell& a, const Cell& b) {
        return a.owner != b.owner ? a.owner < b.owner : a.row != b.row ? a.row < b.row : a.column < b.column; });
    int row_bottom = 0;
    for (std::size_t i = 0; i < placed.size(); ++i) {
        const bool new_table = i == 0 || placed[i - 1].owner != placed[i].owner, new_row = new_table || placed[i - 1].row != placed[i].row;
        if (new_table) row_bottom = 0;
        else if (!new_row) require(placed[i - 1].box.right <= placed[i].box.left, (name + ": cells of one row overlap").c_str());
        if (new_row) {
            // The previous row's lowest cell ends above this row's highest.
            int top = placed[i].box.top;
            for (std::size_t j = i; j < placed.size() && placed[j].owner == placed[i].owner && placed[j].row == placed[i].row; ++j) top = std::min(top, placed[j].box.top);
            require(top >= row_bottom, (name + ": rows overlap").c_str());
        }
        row_bottom = new_row ? placed[i].box.bottom : std::max(row_bottom, placed[i].box.bottom);
    }
}

inline void head_to_head_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 3;
    const char* names[scenarios]{"same nation", "reversed", "other nation"};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = true;
                const auto clubs = c.u32(c.toc(-0x7be8));
                unsigned other = 0;
                for (unsigned k = 0; k < c.u32(c.toc(-0x7b50)) && !other; ++k) {
                    const auto club = clubs + k * 0x248;
                    if (club != t.managed && (c.u32(club + 0x30) == c.u32(t.managed + 0x30)) == (scenario != 2)) other = club;
                }
                require(other, "Head-to-head fixture needs a second club");
                if (scenario == 1) t.invoke(0x100413b0, {other, t.managed});
                else t.invoke(0x100413b0, {t.managed, other});
                require(t.builder() == 0x10041550, "Head-to-head did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records(); };
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.left < 90 || box.top == 0 || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const int got = t.hit(i);
                        require(got == int(i), ("Head-to-head picker missed an original control " + std::to_string(i) + " got " + std::to_string(got)).c_str());
                    }
                };
                HeadToHeadShape shape;
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Head-to-head gate disagrees with preference");
                    t.rebuild(size);
                    shape = head_to_head_shape(t);
                    require(shape.summary >= 0 && shape.meetings >= 0 && shape.footer >= 0, "Original head-to-head tables absent");
                    old = t.layout(records, "Head-to-head");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy head-to-head changed"); pick(); return; }
                    verify_head_to_head(t, size, shape, old, "Head-to-head");
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    {   // Without a career there is no screen to lay out.
                        const auto instance = c.u32(t.manager + 4); c.put32(t.manager + 4, 0);
                        require(!adaptive_screen_available(t.s, 0x10041550), "Head-to-head gate accepted a missing instance");
                        c.put32(t.manager + 4, instance);
                    }
                    for (const auto size : transfer_canvases()) build(size);
                    build(canvas);
                    // The original in-place refresh keeps the layout's bounds.
                    std::vector<std::uint8_t> boxes, again;
                    const auto sample = [&](std::vector<std::uint8_t>& out) {
                        out.clear();
                        for (unsigned k = 0; k < t.controls(); ++k) { const auto part = t.s.memory.copy_out(t.control(k) + 0x10, 16); out.insert(out.end(), part.begin(), part.end()); }
                    };
                    sample(boxes);
                    for (unsigned k = 0; k < t.grids(); ++k) { t.invoke(0x10714630, {t.grid(k)}); t.invoke(0x10714110, {t.grid(k), 0}); }
                    sample(again);
                    require(again == boxes, "Head-to-head redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_head_to_head(t, canvas, shape, old, "Head-to-head"); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    const auto put_rect = [&](unsigned at, ScreenRect r) { c.put32(at, r.left); c.put32(at + 4, r.top); c.put32(at + 8, r.right); c.put32(at + 12, r.bottom); };
                    verify();
                    {   // the summary stays at its classic rectangle
                        put_rect(t.grid(shape.summary), {110, 170, 780, 249});
                        require_rejected(verify, "summary left at classic coordinates"); restore();
                    }
                    {   // the meetings stop sharing the summary's width
                        auto r = t.rect(t.grid(shape.meetings)); r.right -= 9;
                        put_rect(t.grid(shape.meetings), r);
                        require_rejected(verify, "tables differ in width"); restore();
                    }
                    {   // the meetings keep their classic height
                        auto r = t.rect(t.grid(shape.meetings)); r.bottom = r.top + 281;
                        put_rect(t.grid(shape.meetings), r);
                        require_rejected(verify, "meetings not grown"); restore();
                    }
                    {   // the footer stays at the left of the panel
                        auto r = t.rect(t.grid(shape.footer)); r.right -= 30;
                        put_rect(t.grid(shape.footer), r);
                        require_rejected(verify, "footer not right-aligned"); restore();
                    }
                    {   // the last footer button drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore();
                    }
                    {   // the filter menu stays at the classic right edge
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k); const auto box = t.rect(at + 0x10);
                            if (c.i16(at + 0x72) >= 0 || box.top != 145 || box.left < 400) continue;
                            put_rect(at + 0x10, {655, 145, 780, 165}); done = true;
                        }
                        require(done, "Head-to-head fixture found no right menu to mutate");
                        require_rejected(verify, "right menu left at its classic place"); restore();
                    }
                    {   // a cell of the meetings climbs onto the row above
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != shape.meetings || c.u32(at + 0x24) != 3 || c.u32(at + 0x20) != 2) continue;
                            c.put32(at + 0x14, c.i32(at + 0x14) - 30); done = true;
                        }
                        require(done, "Head-to-head fixture found no cell to mutate");
                        require_rejected(verify, "rows overlap"); restore();
                    }
                    {   // a narrow column of the meetings takes extra width
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != shape.meetings || c.u32(at + 0x24) != 1 || c.u32(at + 0x20) != 4) continue;
                            c.put32(at + 0x18, c.i32(at + 0x18) + 40); done = true;
                        }
                        require(done, "Head-to-head fixture found no narrow cell to mutate");
                        require_rejected(verify, "narrow column grew"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing the screen through its original Back button: same result and records with and without the layout.
                std::vector<std::uint8_t> outcome;
                const int back = t.find(0xfffffffeu);
                require(back >= 0 && t.hit(back) == back, "Head-to-head Back button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(back)}));
                const auto after = records();
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                outcome.insert(outcome.end(), after.begin(), after.end());
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive head-to-head changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Head-to-head scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "head-to-head state " << scenario << " (" << names[scenario] << "): tables, menus and footer geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

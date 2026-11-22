// Disciplinary action on a player (0x1010ff90), opened through its original opener 0x1010fef0 (the player), and the
// press question opened from a news item (0x1067bea0, opener 0x1067b920 with the manager's instance and the item's ID;
// the same two grids, its three lines carry their own bounds and its seven-column grid has an empty column), on a
// disposable copy of an existing capture. Built with New Screens off and on at six canvases for each state the
// builder produces: the first level of action (the minus button disabled), a middle level and the heaviest one (the
// plus button disabled), with different reasons chosen. The adaptive run keeps every control, command, record and
// RNG draw and only moves them: both grids as one panel centered under the title bars, at most 800 wide and only as
// tall as the reasons need, the intro line carried with its edges and the player's name link with its left edge, the
// wide column of each grid taking the extra width, and the footer's equal 190px buttons right-aligned under the
// panel. Mutants of each promise must be refused by the checker. Needs TransferScene
// (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct DisciplinaryShape { int wide = -1, narrow = -1, footer = -1; };

inline DisciplinaryShape disciplinary_shape(TransferScene& t) {
    DisciplinaryShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        // The two panels in order; the footer is a row of two buttons (at the classic strip, or where the shell hooked it).
        if (box == ScreenRect{110, 140, 780, 545}) (r.wide < 0 ? r.wide : r.narrow) = int(i);
        else if (rows == 1 && columns == 2 && box.left == 100 && box.height() == 35) r.footer = int(i);
    }
    return r;
}

inline void verify_disciplinary(TransferScene& t, GuiCanvas size, const DisciplinaryShape& shape, const std::vector<std::uint8_t>& old, const char* what, unsigned kind) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(140);
    const int width = std::min(800, content.width());
    const auto panel = t.rect(t.grid(shape.wide)), footer = t.rect(t.grid(shape.footer));
    require(panel != ScreenRect{110, 140, 780, 545} && footer != ScreenRect{100, 555, 790, 590},
            (name + ": layout did not run, the dialog stays at classic coordinates").c_str());
    require(t.rect(t.grid(shape.narrow)) == panel, (name + ": the two grids no longer share one panel").c_str());
    require(panel.width() == width && panel.left == content.left + (content.width() - width) / 2 && panel.top == 140 && panel.right <= size.width - 10 &&
            std::abs((panel.left + panel.right) - (shell.title().left + shell.title().right)) <= 2,
            (name + ": panel not centered under the title bars, bounded to 800").c_str());
    unsigned last = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const int owner = c.i16(t.control(i) + 0x72);
        if (owner == shape.wide || owner == shape.narrow) last = std::max(last, c.u32(t.control(i) + 0x24));
    }
    require(c.u8(t.grid(shape.wide) + 0xbb1) == last + 2 && c.u8(t.grid(shape.narrow) + 0xbb1) == last + 2 && panel.height() == int(last + 2) * 28 + 16,
            (name + ": panel not bounded to its reasons").c_str());
    // Buttons: equal cells in one row under the panel, right-aligned to it, inside the canvas.
    require(footer.right == panel.right && footer.top == panel.bottom + 10 && footer.bottom == footer.top + 35 && footer.bottom <= size.height - 10 &&
            footer.width() == std::min(panel.width(), 2 * 190), (name + ": footer not compact under the panel").c_str());
    std::vector<std::pair<unsigned, ScreenRect>> buttons;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) == shape.footer) buttons.push_back({c.u32(at + 0x20), t.rect(at + 0x10)});
    }
    std::sort(buttons.begin(), buttons.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    require(buttons.size() == 2, (name + ": footer lost a button").c_str());
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const auto& b = buttons[i].second;
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && std::abs(b.width() - 190) <= 3,
                (name + ": footer button outside its footer or not 190 wide").c_str());
        if (i) require(buttons[i - 1].second.right <= b.left, (name + ": footer buttons overlap or are out of order").c_str());
    }
    require(buttons.back().second.right == footer.right && buttons.front().second.left == footer.left, (name + ": footer buttons not flush").c_str());

    const int lead = panel.left - 110, trail = panel.right - 780;
    unsigned lines = 0, cells = 0;
    struct Cell { unsigned row, column; ScreenRect box; };
    std::vector<Cell> placed;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        const auto old_left = display_word(old, old_at + 0x28), old_right = display_word(old, old_at + 0x2c);
        if (owner == shape.wide || owner == shape.narrow) {
            if (box.top == 0 && box.bottom == 0) continue;
            require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom,
                    (name + ": control outside its panel").c_str());
            if (old_left && old_right) {
                // The intro line follows the panel's left and right edges; the measured name link keeps its width and offset.
                require(c.i32(at + 0x28) == old_left + lead && c.i32(at + 0x2c) == (old_right >= 770 ? old_right + trail : old_right + lead),
                        (name + ": line not carried with its panel").c_str());
                ++lines;
            } else {
                // A cell: only the wide column of its grid took the extra width.
                const int grown = box.width() - before.width();
                if (owner == shape.narrow) require(grown >= 0, (name + ": reason narrower than before").c_str());
                else require(grown >= 0 && (grown == 0 || before.width() >= 300), (name + ": a narrow column of the action row grew").c_str());
                placed.push_back({c.u32(at + 0x24), c.u32(at + 0x20), box});
                ++cells;
            }
        } else if (owner < 0) {
            if (before == ScreenRect{100, 80, 790, 125}) require(box == shell.subtitle(), (name + ": subtitle did not follow the shell").c_str());
            else if (before == ScreenRect{100, 10, 790, 70}) require(box == shell.title(), (name + ": title did not follow the shell").c_str());
            else require(box == before, (name + ": an unrelated standalone control moved").c_str());
        }
    }
    // Cells keep their row and column order without overlapping, down the rows and across each row.
    std::sort(placed.begin(), placed.end(), [](const Cell& a, const Cell& b) { return a.row != b.row ? a.row < b.row : a.column < b.column; });
    for (std::size_t i = 1; i < placed.size(); ++i) {
        if (placed[i - 1].row == placed[i].row) require(placed[i - 1].box.right <= placed[i].box.left, (name + ": cells of one row overlap").c_str());
        else require(placed[i - 1].box.bottom <= placed[i].box.top, (name + ": rows overlap").c_str());
    }
    require(lines >= (kind == 1 ? 3u : kind == 2 ? 0u : kind == 3 ? 1u : 2u) && cells >= (kind == 1 ? 3u : kind == 2 ? 8u : kind == 3 ? 2u : 9u), (name + ": fixture found too few lines or cells to check").c_str());
}

inline void disciplinary_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 8;
    const char* names[scenarios]{"first level", "level 2, reason 3", "heaviest level, reason 2", "press question", "press question, answer", "role in the club", "meeting with the board", "meeting with the league"};
    const unsigned levels[scenarios]{1, 2, 5}, reasons[scenarios]{0, 3, 2};
    const unsigned builders[scenarios]{0x1010ff90, 0x1010ff90, 0x1010ff90, 0x1067bea0, 0x1067bea0, 0x100952b0, 0x105d6ac0, 0x106ee9a0};
    // The answer exercised by each scenario: Cancel (4), the plus button (1) and a reason (3); the press question's Cancel (4252) and a reply (5253).
    const unsigned answers[scenarios]{4, 1, 3, 4252, 5253, 1, 8, 4};
    // The row of the second reason (the mutant that climbs onto the one above it) and how many lines and cells the fixture must find.
    const unsigned second_reason[scenarios]{7, 7, 7, 5, 5, 3, 3, 3};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = true;
                const bool press = builders[scenario] == 0x1067bea0, role = builders[scenario] == 0x100952b0;
                const bool meeting = builders[scenario] == 0x105d6ac0 || builders[scenario] == 0x106ee9a0;
                if (meeting) t.invoke(builders[scenario] == 0x105d6ac0 ? 0x105d6a20 : 0x106ee900, {t.managed});   // the Manager menu's meetings: the club's record is the previous screen's first value
                else if (role) t.invoke(0x10094ae0, {t.person, 0, 0, 1});
                else if (press) t.invoke(0x1067b920, {t.instance, t.news_item(6000, {0, 0, 0})});
                else {
                    t.invoke(0x1010fef0, {t.person});
                    t.set_value(1, levels[scenario]); t.set_value(3, reasons[scenario]);
                }
                require(t.builder() == builders[scenario], "Disciplinary action did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records(); };
                const auto pick = [&] {
                    const auto shape = disciplinary_shape(t);
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if ((owner != shape.wide && owner != shape.narrow && owner != shape.footer) || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        { const int got = t.hit(i); require(got == int(i), ("Disciplinary picker missed an original control " + std::to_string(i) + " got " + std::to_string(got)).c_str()); }
                    }
                };
                DisciplinaryShape shape;
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Disciplinary gate disagrees with preference");
                    t.rebuild(size);
                    shape = disciplinary_shape(t);
                    require(shape.wide >= 0 && shape.narrow >= 0 && shape.footer >= 0, "Original disciplinary panels absent");
                    old = t.layout(records, "Disciplinary");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy disciplinary screen changed"); pick(); return; }
                    verify_disciplinary(t, size, shape, old, "Disciplinary", press ? 1 : role ? 2 : meeting ? 3 : 0);
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    {   // Without a career there is no dialog to lay out.
                        const auto instance = c.u32(t.manager + 4); c.put32(t.manager + 4, 0);
                        require(!adaptive_screen_available(t.s, 0x1010ff90), "Disciplinary gate accepted a missing instance");
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
                    require(again == boxes, "Disciplinary redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_disciplinary(t, canvas, shape, old, "Disciplinary", press ? 1 : role ? 2 : meeting ? 3 : 0); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    const auto put_rect = [&](unsigned at, ScreenRect r) { c.put32(at, r.left); c.put32(at + 4, r.top); c.put32(at + 8, r.right); c.put32(at + 12, r.bottom); };
                    verify();
                    {   // the reasons' grid stays at its classic rectangle
                        put_rect(t.grid(shape.narrow), {110, 140, 780, 545});
                        require_rejected(verify, "reasons left at classic coordinates"); restore();
                    }
                    {   // the two grids stop sharing a rectangle
                        auto r = t.rect(t.grid(shape.narrow)); r.right -= 7;
                        put_rect(t.grid(shape.narrow), r);
                        require_rejected(verify, "grids differ"); restore();
                    }
                    {   // the panel keeps its nineteen rows
                        c.put8(t.grid(shape.wide) + 0xbb1, 19); c.put8(t.grid(shape.narrow) + 0xbb1, 19);
                        require_rejected(verify, "panel not compacted"); restore();
                    }
                    {   // the last button drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore();
                    }
                    {   // the footer sits at the canvas's bottom row instead of under the panel
                        c.put32(t.grid(shape.footer) + 4, canvas.height - 45); c.put32(t.grid(shape.footer) + 12, canvas.height - 10);
                        require_rejected(verify, "footer left at the bottom row"); restore();
                    }
                    if (!role) {   // the intro line's bounds stay at their classic values
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(t.control(k) + 0x72) != shape.wide || !display_word(old, old_at + 0x28)) continue;
                            c.put32(t.control(k) + 0x28, display_word(old, old_at + 0x28)); done = true;
                        }
                        require(done, "Disciplinary fixture found no line to mutate");
                        require_rejected(verify, "line left at classic bounds"); restore();
                    }
                    {   // a reason climbs onto the one above it
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != shape.narrow || c.u32(at + 0x24) != second_reason[scenario]) continue;
                            c.put32(at + 0x14, c.i32(at + 0x14) - 20); done = true;
                        }
                        require(done, "Disciplinary fixture found no reason to mutate");
                        require_rejected(verify, "reasons overlap"); restore();
                    }
                    if (!press && !role && !meeting) {   // a narrow column of the action row takes the extra width
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != shape.wide || c.u32(at + 0x20) != 4 || c.u32(at + 0x24) != 2) continue;
                            c.put32(at + 0x18, c.i32(at + 0x18) + 400); done = true;
                        }
                        require(done, "Disciplinary fixture found no plus button to mutate");
                        require_rejected(verify, "narrow column grew"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Answering through the original handler (Cancel, the plus button, a reason): same result, records and RNG with and without the layout.
                // (The controls' colors are not compared: a reason is highlighted under the mouse, which the picker moved.)
                std::vector<std::uint8_t> outcome;
                const int answer = t.find(answers[scenario]);
                require(answer >= 0 && t.hit(answer) == answer, "Disciplinary answer button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(answer)}));
                const auto after = records();
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                outcome.insert(outcome.end(), after.begin(), after.end());
                // The press answers read the news item and may mark it: that effect must be the same too.
                if (press) { const auto queue = t.news_queue(); outcome.insert(outcome.end(), queue.begin(), queue.end()); }
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive disciplinary screen changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Disciplinary scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "disciplinary state " << scenario << " (" << names[scenario] << "): dialog geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

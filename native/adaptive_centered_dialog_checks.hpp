// The friendly tour's tournament setup (0x10046de0, opener 0x10046c80 with a club): a modal dialog at fixed coordinates
// (centered_dialog_layout.cpp), opened through its original opener on a disposable copy of an existing capture for the
// managed club and for another club, and built with New Screens off and on at six canvases. The adaptive run keeps every
// control, command, record and RNG draw and moves the whole dialog, grids, controls and measured text areas, by the same
// offset (half of the canvas' extra width and height) without changing any size. Mutants must be refused by the same
// checker. Needs TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
inline void verify_centered_dialog(TransferScene& t, GuiCanvas size, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const int dx = (size.width - 800) / 2, dy = (size.height - 600) / 2;
    require(t.grids() >= 2, (name + ": the dialog's grids are missing").c_str());
    for (unsigned i = 0; i < t.grids(); ++i) {
        const unsigned old_at = 4 + i * 0xbf0;
        const ScreenRect before{display_word(old, old_at), display_word(old, old_at + 4), display_word(old, old_at + 8), display_word(old, old_at + 12)};
        require(t.rect(t.grid(i)) == ScreenRect{before.left + dx, before.top + dy, before.right + dx, before.bottom + dy}, (name + ": a grid did not move with the dialog").c_str());
        for (unsigned field : {0xbb0u, 0xbb1u, 0xbefu}) require(old[old_at + field] == c.u8(t.grid(i) + field), (name + ": a grid changed its columns, rows or scrollbar").c_str());
    }
    unsigned moved = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        const int x0 = display_word(old, old_at + 0x28), x1 = display_word(old, old_at + 0x2c), y0 = display_word(old, old_at + 0x30), y1 = display_word(old, old_at + 0x34);
        require(c.i32(at + 0x28) == (x0 || x1 ? x0 + dx : 0) && c.i32(at + 0x2c) == (x0 || x1 ? x1 + dx : 0) &&
                c.i32(at + 0x30) == (y0 || y1 ? y0 + dy : 0) && c.i32(at + 0x34) == (y0 || y1 ? y1 + dy : 0), (name + ": a measured text area did not move with the dialog").c_str());
        if (c.i16(at + 0x72) < 0 && (before.right > before.left || before.bottom > before.top))
            require(box == ScreenRect{before.left + dx, before.top + dy, before.right + dx, before.bottom + dy}, (name + ": a standalone control did not move with the dialog").c_str());
        else if (c.i16(at + 0x72) >= 0 && box.right > box.left) {
            // A cell keeps its size and follows its grid.
            require(box.width() == before.width() && box.height() == before.height() && box.left == before.left + dx && box.top == before.top + dy, (name + ": a cell did not move with its grid").c_str());
            ++moved;
        }
    }
    require(moved >= 20, (name + ": fixture found too few cells to check").c_str());
}

void centered_dialog_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 2;
    const char* names[scenarios]{"managed club", "another club"};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = false;
                const auto clubs = c.u32(c.toc(-0x7be8));
                unsigned other = 0;
                for (unsigned k = 0; k < c.u32(c.toc(-0x7b50)) && !other; ++k) if (clubs + k * 0x248 != t.managed) other = clubs + k * 0x248;
                t.invoke(0x10046c80, {scenario ? other : t.managed});
                require(t.builder() == 0x10046de0, "Tournament setup did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records(); };
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.right <= box.left || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const int got = t.hit(i);
                        require(got == int(i), ("Tournament setup picker missed an original control " + std::to_string(i) + " got " + std::to_string(got)).c_str());
                    }
                };
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Tournament setup gate disagrees with preference");
                    t.rebuild(size);
                    old = t.layout(records, "Tournament setup");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy tournament setup changed"); pick(); return; }
                    verify_centered_dialog(t, size, old, "Tournament setup");
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    {   // Without a career there is no dialog to move.
                        const auto instance = c.u32(t.manager + 4); c.put32(t.manager + 4, 0);
                        require(!adaptive_screen_available(t.s, 0x10046de0), "Tournament setup gate accepted a missing instance");
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
                    require(again == boxes, "Tournament setup redraw lost the layout's control bounds");
                    // Mutants: each promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_centered_dialog(t, canvas, old, "Tournament setup"); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    verify();
                    {   // the frame stays at its classic place
                        c.put32(t.grid(0), 170); c.put32(t.grid(0) + 8, 719);
                        require_rejected(verify, "frame left at classic coordinates"); restore();
                    }
                    {   // the footer moves apart from the frame
                        c.put32(t.grid(1) + 4, c.i32(t.grid(1) + 4) + 6); c.put32(t.grid(1) + 12, c.i32(t.grid(1) + 12) + 6);
                        require_rejected(verify, "footer drifted"); restore();
                    }
                    {   // a measured text area stays behind
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k)
                            if (c.i32(t.control(k) + 0x28) && c.i32(t.control(k) + 0x2c)) { c.put32(t.control(k) + 0x28, c.i32(t.control(k) + 0x28) - 40); done = true; }
                        require(done, "Tournament setup fixture found no measured text area to mutate");
                        require_rejected(verify, "text area left behind"); restore();
                    }
                    {   // a cell is resized
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k)
                            if (c.i16(t.control(k) + 0x72) == 3 && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 12); done = true; }
                        require(done, "Tournament setup fixture found no cell to mutate");
                        require_rejected(verify, "cell resized"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // The options' original commands (points per win, tie-breaker, extra time, deciding game, teams): same results and
                // records with and without the layout. (Cancel and OK leave the dialog through the screens that opened it, which
                // this fixture does not have.)
                std::vector<std::uint8_t> outcome;
                for (unsigned action : {3u, 4u, 5u, 6u, 7u, 8u}) {
                    const int choice = t.find(action);
                    require(choice >= 0 && t.hit(choice) == choice, "Tournament setup option missing");
                    const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(choice)}));
                    const auto after = records();
                    for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                    outcome.insert(outcome.end(), after.begin(), after.end());
                }
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive tournament setup changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Tournament setup scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "tournament setup state " << scenario << " (" << names[scenario] << "): dialog moved whole, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

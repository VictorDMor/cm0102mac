// Holiday (0x101c9eb0), opened through its original opener 0x101c9e70 on a disposable copy of an existing
// capture, with the original's own setting links clicked for the other states (each link carries its value,
// so its measured width changes). Built with New Screens off and on at six canvases; the adaptive run keeps
// every control, command, record and RNG draw and only moves them: a panel centered under the title bars and
// at most 800 wide, the settings carried with its left edge, and two equal 190px buttons right-aligned under
// it. Mutants of each geometric promise must be refused by the same checker. Needs TransferScene
// (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct HolidayShape { int panel = -1, list = -1, footer = -1; };

inline HolidayShape holiday_shape(TransferScene& t) {
    HolidayShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        if (box == ScreenRect{110, 145, 780, 535} && columns == 1 && rows == 1) r.panel = int(i);
        else if (box == ScreenRect{130, 250, 760, 535} && columns == 1) r.list = int(i);
        else if (box == ScreenRect{100, 555, 790, 590} && columns == 2 && rows == 1) r.footer = int(i);
    }
    return r;
}

inline void verify_holiday(TransferScene& t, GuiCanvas size, const HolidayShape& shape, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(145);
    const int width = std::min(800, content.width());
    const auto panel = t.rect(t.grid(shape.panel)), list = t.rect(t.grid(shape.list)), footer = t.rect(t.grid(shape.footer));
    // At a width of 800 the bounded panel is the classic one; the footer, which moves under it, tells that the layout ran.
    require(footer != ScreenRect{100, 555, 790, 590} && (panel != ScreenRect{110, 145, 780, 535} || size.width == 800),
            (name + ": layout did not run, the dialog stays at classic coordinates").c_str());
    require(panel.width() == width && panel.left == content.left + (content.width() - width) / 2 && panel.top == 145 && panel.height() == 390,
            (name + ": panel is not centered at its classic height").c_str());
    require(panel.right <= size.width - 10 && std::abs((panel.left + panel.right) - (shell.title().left + shell.title().right)) <= 2,
            (name + ": panel not centered under the title bars").c_str());
    require(list == ScreenRect{panel.left + 20, 250, panel.right - 20, 535} && c.u8(t.grid(shape.list) + 0xbb1) == 13 && c.u8(t.grid(shape.list) + 0xbef) == 0,
            (name + ": settings list not inside the panel").c_str());
    // Buttons: two equal cells under the panel, right-aligned to it, inside the canvas.
    require(footer.right == panel.right && footer.top == panel.bottom + 10 && footer.bottom == footer.top + 35 && footer.bottom <= size.height - 10 &&
            footer.width() == std::min(panel.width(), 380), (name + ": footer not compact under the panel").c_str());
    std::vector<ScreenRect> buttons;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) == shape.footer) buttons.push_back(t.rect(at + 0x10));
    }
    require(buttons.size() == 2, (name + ": footer lost a button").c_str());
    for (const auto& b : buttons)
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && b.width() >= 100,
                (name + ": footer button outside its footer").c_str());
    require(std::abs(buttons[0].width() - buttons[1].width()) <= 1 && buttons[0].right <= buttons[1].left &&
            buttons[1].right == footer.right && buttons[0].left == footer.left, (name + ": footer buttons unequal, overlapping or not flush").c_str());

    const int lead = panel.left - 110, trail = panel.right - 780;
    unsigned moved = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        const auto old_left = display_word(old, old_at + 0x28), old_right = display_word(old, old_at + 0x2c);
        if (owner == shape.list) {
            if (box.top == 0 && box.bottom == 0) continue;
            require(box.left >= list.left && box.right <= list.right && box.top >= list.top && box.bottom <= list.bottom,
                    (name + ": setting outside its list").c_str());
            if (old_left && old_right) {
                // A line follows the list's left edge and its right edge; a measured link keeps its width and offset.
                require(c.i32(at + 0x28) == old_left + lead && c.i32(at + 0x2c) == (old_right >= 750 ? old_right + trail : old_right + lead),
                        (name + ": setting not carried with its panel").c_str());
                ++moved;
            }
        } else if (owner == shape.footer) {
            continue;
        } else if (owner < 0) {
            if (before == ScreenRect{100, 80, 790, 125}) require(box == shell.subtitle(), (name + ": subtitle did not follow the shell").c_str());
            else if (before == ScreenRect{100, 10, 790, 70}) require(box == shell.title(), (name + ": title did not follow the shell").c_str());
            else if (before == ScreenRect{110, 165, 780, 200}) {
                require(box.left == panel.left && box.right == panel.right && box.top == 165 && box.bottom == 200 &&
                        c.i32(at + 0x28) == panel.left && c.i32(at + 0x2c) == panel.right, (name + ": intro does not span the panel").c_str());
                ++moved;
            } else require(box == before, (name + ": an unrelated standalone control moved").c_str());
        }
    }
    require(moved >= 3, (name + ": fixture found too few settings to check").c_str());
}

void holiday_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    // The original's own links: clicking each setting (action 3) flips its value, and the link's width with it.
    constexpr unsigned scenarios = 5;
    const char* names[scenarios]{"as opened", "first setting flipped", "second", "third", "all flipped"};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.invoke(0x101c9e70, {});
                require(t.builder() == 0x101c9eb0, "Holiday did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] {
                    return t.records();
                };
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if (owner >= int(t.grids()) || (owner >= 0 && owner < 19) || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        if (t.rect(t.control(i) + 0x10).left < 90) continue;
                        require(t.hit(i) == int(i), "Holiday picker missed an original control");
                    }
                };
                HolidayShape shape;
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Holiday gate disagrees with preference");
                    t.rebuild(size);
                    shape = holiday_shape(t);
                    require(shape.panel >= 0 && shape.list >= 0 && shape.footer >= 0, "Original holiday panels absent");
                    old = t.layout(records, "Holiday");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy holiday changed"); pick(); return; }
                    verify_holiday(t, size, shape, old, "Holiday");
                    pick();
                };
                // Flip settings through the original handler (a result of -11 asks for the same screen again).
                const auto flip = [&](unsigned nth) {
                    unsigned seen = 0;
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        if (c.u32(t.control(i) + 8) != 3 || !t.enabled(i) || c.i16(t.control(i) + 0x72) != shape.list) continue;
                        if (seen++ != nth) continue;
                        require(t.hit(i) == int(i), "Holiday setting hit missed");
                        t.invoke(c.u32(c.u32(t.history() + 8)), {i});
                        return true;
                    }
                    return false;
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                const auto opened = t.meanings();
                if (scenario >= 1 && scenario <= 3) require(flip(scenario - 1), "Holiday has no such setting");
                if (scenario == 4) for (unsigned k = 0; k < 7; ++k) require(flip(k), "Holiday has too few settings");
                if (scenario) { build(canvas); require(t.meanings() != opened, "Holiday flip changed nothing the fixture can see"); }
                if (adaptive) {
                    // Only a career is a state the layout handles.
                    c.put32(t.manager + 4, 0);
                    require(!adaptive_screen_available(t.s, 0x101c9eb0), "Holiday gate accepted a missing instance");
                    c.put32(t.manager + 4, t.instance);
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
                    require(again == boxes, "Holiday redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_holiday(t, canvas, shape, old, "Holiday"); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    verify();
                    {   // the panel stays at its classic rectangle
                        c.put32(t.grid(shape.panel), 110); c.put32(t.grid(shape.panel) + 8, 780);
                        c.put32(t.grid(shape.panel) + 4, 145); c.put32(t.grid(shape.panel) + 12, 535);
                        require_rejected(verify, "panel left at classic coordinates"); restore();
                    }
                    {   // the second button drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore();
                    }
                    {   // the footer sits at the canvas's bottom row instead of under the panel
                        c.put32(t.grid(shape.footer) + 4, canvas.height - 45); c.put32(t.grid(shape.footer) + 12, canvas.height - 10);
                        require_rejected(verify, "footer left at the bottom row"); restore();
                    }
                    {   // a setting's text stays at its classic bounds
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(t.control(k) + 0x72) != shape.list || !display_word(old, old_at + 0x28)) continue;
                            c.put32(t.control(k) + 0x28, display_word(old, old_at + 0x28)); done = true;
                        }
                        require(done, "Holiday fixture found no setting to mutate");
                        require_rejected(verify, "setting left at classic bounds"); restore();
                    }
                    {   // the intro paragraph stays at its classic span
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(t.control(k) + 0x72) >= 0 || display_word(old, old_at + 0x10) != 110 || display_word(old, old_at + 0x14) != 165) continue;
                            c.put32(t.control(k) + 0x10, 110); c.put32(t.control(k) + 0x18, 780); done = true;
                        }
                        require(done, "Holiday fixture found no intro paragraph");
                        require_rejected(verify, "intro left at its classic span"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing through the original No: same result, records and RNG with and without the layout.
                std::vector<std::uint8_t> outcome;
                const auto saved = records();
                // The result of each flip is in its links' texts: compare them as well.
                const auto texts = t.meanings();
                const int no = t.find(1);
                require(no >= 0 && t.hit(no) == no, "Holiday No button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(no)}));
                require(records() == saved, "Holiday No changed records");
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                outcome.insert(outcome.end(), saved.begin(), saved.end()); outcome.insert(outcome.end(), texts.begin(), texts.end());
                if (!adaptive) baseline = outcome;
                else {
                    std::size_t at = 0; while (at < outcome.size() && at < baseline.size() && outcome[at] == baseline[at]) ++at;
                    require(outcome == baseline, ("Adaptive holiday changed the original command outcome at byte " + std::to_string(at) + " of " + std::to_string(outcome.size())).c_str());
                }
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Holiday scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "holiday state " << scenario << " (" << names[scenario] << "): dialog geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

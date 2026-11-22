// The text entry dialogs (the player's nickname 0x103ac1c0, opener 0x103ac0d0, with Cancel/Remove/OK; Send Message
// 0x102d0500, opener 0x102d02b0, with Cancel/Send, to a person and to the manager), opened through their original
// openers on a disposable copy of an existing capture. Built with New Screens off and on at six canvases; the
// adaptive run must keep every control, command, record and RNG draw and only move them: the edit control centered
// under the title bars at most 800 wide and 60 high in the upper middle of the content area, and equal buttons
// under it, right-aligned to its edge. Mutants of each promise must be refused by the same checker. Needs
// TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct TextEntryShape { int input = -1, footer = -1; std::vector<unsigned> buttons; };

inline TextEntryShape text_entry_shape(TransferScene& t) {
    TextEntryShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i));
        if (box == ScreenRect{100, 555, 790, 590} && t.c.u8(t.grid(i) + 0xbb1) == 1) r.footer = int(i);
    }
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const int owner = t.c.i16(at + 0x72);
        if (owner < 0 && t.rect(at + 0x10) == ScreenRect{150, 310, 740, 370}) r.input = int(i);
        else if (owner == r.footer && r.footer >= 0) r.buttons.push_back(i);
    }
    return r;
}

inline void verify_text_entry(TransferScene& t, GuiCanvas size, const TextEntryShape& shape, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(145);
    const int width = std::min(800, content.width());
    const auto at = t.control(unsigned(shape.input));
    const auto box = t.rect(at + 0x10), footer = t.rect(t.grid(shape.footer));
    require(footer != ScreenRect{100, 555, 790, 590} && box != ScreenRect{150, 310, 740, 370}, (name + ": layout did not run, the dialog stays at classic coordinates").c_str());
    require(box.width() == width && box.left == content.left + (content.width() - width) / 2 && box.height() == 60 &&
            std::abs((box.left + box.right) - (shell.title().left + shell.title().right)) <= 2 + (size.width - 800) % 2,
            (name + ": edit control not centered at its bounded size").c_str());
    require(c.i32(at + 0x28) == box.left && c.i32(at + 0x2c) == box.right && c.i32(at + 0x30) == box.top && c.i32(at + 0x34) == box.bottom,
            (name + ": edit control's text bounds do not follow its rectangle").c_str());
    require(box.top >= content.top && box.top <= content.top + content.height() / 3 + 1 && footer.bottom <= size.height - 10 && box.right <= size.width - 10,
            (name + ": dialog not in the upper middle of the content area").c_str());
    // Buttons: equal, on one row under the edit control, right-aligned to it.
    const unsigned count = unsigned(shape.buttons.size());
    require(count >= 2 && count <= 3 && c.u8(t.grid(shape.footer) + 0xbb0) == count, (name + ": footer lost a button").c_str());
    require(footer.right == box.right && footer.top == box.bottom + 10 && footer.bottom == footer.top + 35 &&
            footer.width() == std::min(box.width(), int(count) * 190), (name + ": footer not compact under the edit control").c_str());
    std::vector<ScreenRect> cells;
    for (const auto i : shape.buttons) cells.push_back(t.rect(t.control(i) + 0x10));
    for (const auto& b : cells)
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && b.width() >= 100,
                (name + ": footer button outside its footer or too small").c_str());
    for (std::size_t i = 1; i < cells.size(); ++i)
        require(cells[i - 1].right <= cells[i].left && std::abs(cells[i].width() - cells[0].width()) <= 1, (name + ": footer buttons unequal or overlapping").c_str());
    require(cells.back().right == footer.right && cells.front().left == footer.left, (name + ": footer buttons not flush with the footer").c_str());
    // Everything else keeps its place, except the title and subtitle, which follow the shell as built.
    for (unsigned i = 0; i < t.controls(); ++i) {
        if (int(i) == shape.input || std::find(shape.buttons.begin(), shape.buttons.end(), i) != shape.buttons.end()) continue;
        const auto now = t.rect(t.control(i) + 0x10); const unsigned old_at = 0xba864 + i * 0x17c;
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        require(now == before, (name + ": an unrelated control moved").c_str());
    }
}

void text_entry_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 3;
    const char* names[scenarios]{"nickname", "message to a person", "message to the manager"};
    const unsigned builders[scenarios]{0x103ac1c0, 0x102d0500, 0x102d0500};
    const unsigned buttons[scenarios]{3, 2, 2};
    // The control that answers Cancel (action 52 for the nickname, 7 for the message).
    const unsigned dismissals[scenarios]{52, 7, 7};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                if (scenario == 0) t.invoke(0x103ac0d0, {t.person});
                else t.invoke(0x102d02b0, {scenario == 1 ? t.person : t.instance});
                require(t.builder() == builders[scenario], "Text entry did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records(); };
                TextEntryShape shape;
                std::vector<std::uint8_t> old;
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        // The edit control is not a pickable control in the original either (typing reaches it by focus).
                        if (owner < 19 || owner != shape.footer || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.left < 90 || box.top == 0) continue;
                        { const int got = t.hit(i); require(got == int(i), ("Text entry picker missed an original control " + std::to_string(i) + " action " + std::to_string(c.u32(t.control(i) + 8)) + " got " + std::to_string(got)).c_str()); }
                    }
                };
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Text entry gate disagrees with preference");
                    t.rebuild(size);
                    shape = text_entry_shape(t);
                    require(shape.input >= 0 && shape.footer >= 0, "Original text entry panels absent");
                    require(shape.buttons.size() == buttons[scenario], "Text entry footer has an unexpected number of buttons");
                    old = t.layout(records, "Text entry");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy text entry changed"); pick(); return; }
                    verify_text_entry(t, size, shape, old, names[scenario]);
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    {   // Without a career there is no dialog to lay out.
                        const auto instance = c.u32(t.manager + 4); c.put32(t.manager + 4, 0);
                        require(!adaptive_screen_available(t.s, builders[scenario]), "Text entry gate accepted a missing instance");
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
                    require(again == boxes, "Text entry redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the same checker.
                    const auto verify = [&] { verify_text_entry(t, canvas, shape, old, names[scenario]); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    verify();
                    {   // the edit control stays at its classic rectangle
                        const auto at = t.control(unsigned(shape.input));
                        c.put32(at + 0x10, 150); c.put32(at + 0x14, 310); c.put32(at + 0x18, 740); c.put32(at + 0x1c, 370);
                        require_rejected(verify, "edit control left at classic coordinates"); restore();
                    }
                    {   // the edit control's text bounds stay behind
                        const auto at = t.control(unsigned(shape.input));
                        c.put32(at + 0x28, 150);
                        require_rejected(verify, "edit control's text bounds left behind"); restore();
                    }
                    {   // the last button drifts right
                        const auto at = t.control(shape.buttons.back());
                        c.put32(at + 0x10, c.i32(at + 0x10) + 5); c.put32(at + 0x18, c.i32(at + 0x18) + 5);
                        require_rejected(verify, "button drifted"); restore();
                    }
                    {   // the first button climbs into the edit control
                        const auto at = t.control(shape.buttons.front());
                        c.put32(at + 0x14, t.rect(t.control(unsigned(shape.input)) + 0x10).bottom - 5);
                        require_rejected(verify, "button inside the edit control"); restore();
                    }
                    {   // the footer returns to the classic strip
                        c.put32(t.grid(shape.footer), 100); c.put32(t.grid(shape.footer) + 4, 555); c.put32(t.grid(shape.footer) + 8, 790); c.put32(t.grid(shape.footer) + 12, 590);
                        require_rejected(verify, "footer left at classic coordinates"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing the dialog through its original Cancel button: same result and records with and without the layout.
                std::vector<std::uint8_t> outcome;
                const auto saved_records = records();
                const int cancel = t.find(dismissals[scenario]);
                require(cancel >= 0 && t.hit(cancel) == cancel, "Text entry Cancel button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(cancel)}));
                require(records() == saved_records, "Text entry Cancel changed records");
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive text entry changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Text entry ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "text entry " << scenario << " (" << names[scenario] << "): dialog geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

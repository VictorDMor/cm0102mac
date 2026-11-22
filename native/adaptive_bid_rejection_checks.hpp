// Rejecting an offer (0x1041f8f0), opened through its original opener 0x1041f830 on a disposable copy of an
// existing capture, with the arguments its callers pass (a link to the offer, which the opener also keeps
// as the second history value). Built with New Screens off and on at six canvases for each state the
// original builder produces: which reason is chosen, the offer found in the second history value instead of
// the first, and one pending offer (a No/Yes footer) or two (No/All/Yes). The adaptive run keeps every control, command,
// record and RNG draw and only moves them: a panel centered under the title bars, at most 800 wide and only as tall
// as its choices need, the lines and links carried with its edges, the choices in cells that follow its width,
// and the footer's equal 190px buttons right-aligned under it. Mutants of each promise must be refused by the
// checker. Needs TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct BidShape { int panel = -1, footer = -1; unsigned buttons = 0; };

inline BidShape bid_shape(TransferScene& t) {
    BidShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        if (box == ScreenRect{110, 140, 780, 545} && columns == 2) r.panel = int(i);
        else if (box == ScreenRect{100, 555, 790, 590} && rows == 1) { r.footer = int(i); r.buttons = columns; }
    }
    return r;
}

inline void verify_bid_rejection(TransferScene& t, GuiCanvas size, const BidShape& shape, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(140);
    const int width = std::min(800, content.width());
    const auto panel = t.rect(t.grid(shape.panel)), footer = t.rect(t.grid(shape.footer));
    require(panel != ScreenRect{110, 140, 780, 545} && footer != ScreenRect{100, 555, 790, 590},
            (name + ": layout did not run, the dialog stays at classic coordinates").c_str());
    require(panel.width() == width && panel.left == content.left + (content.width() - width) / 2 && panel.top == 140 && panel.right <= size.width - 10 &&
            std::abs((panel.left + panel.right) - (shell.title().left + shell.title().right)) <= 2,
            (name + ": panel not centered under the title bars, bounded to 800").c_str());
    unsigned last = 0;
    for (unsigned i = 0; i < t.controls(); ++i)
        if (c.i16(t.control(i) + 0x72) == shape.panel) last = std::max(last, c.u32(t.control(i) + 0x24));
    require(c.u8(t.grid(shape.panel) + 0xbb1) == last + 2 && panel.height() == int(last + 2) * 28 + 16,
            (name + ": panel not bounded to its choices").c_str());
    // Buttons: equal cells in one row under the panel, right-aligned to it, inside the canvas.
    require(footer.right == panel.right && footer.top == panel.bottom + 10 && footer.bottom == footer.top + 35 && footer.bottom <= size.height - 10 &&
            footer.width() == std::min(panel.width(), int(shape.buttons) * 190), (name + ": footer not compact under the panel").c_str());
    std::vector<std::pair<unsigned, ScreenRect>> buttons;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) == shape.footer) buttons.push_back({c.u32(at + 0x20), t.rect(at + 0x10)});
    }
    std::sort(buttons.begin(), buttons.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    require(buttons.size() == shape.buttons, (name + ": footer lost a button").c_str());
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const auto& b = buttons[i].second;
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && std::abs(b.width() - 190) <= 3,
                (name + ": footer button outside its footer or not 190 wide").c_str());
        if (i) require(buttons[i - 1].second.right <= b.left, (name + ": footer buttons overlap or are out of order").c_str());
    }
    require(buttons.back().second.right == footer.right && buttons.front().second.left == footer.left, (name + ": footer buttons not flush").c_str());

    const int lead = panel.left - 110, trail = panel.right - 780;
    unsigned lines = 0, choices = 0;
    std::vector<std::pair<unsigned, ScreenRect>> column_cells;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        const auto old_left = display_word(old, old_at + 0x28), old_right = display_word(old, old_at + 0x2c);
        if (owner == shape.panel) {
            if (box.top == 0 && box.bottom == 0) continue;
            require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom,
                    (name + ": control outside its panel").c_str());
            if (old_left && old_right) {
                // A line follows the panel's left edge and its right edge; a measured link keeps its width and offset.
                require(c.i32(at + 0x28) == old_left + lead && c.i32(at + 0x2c) == (old_right >= 770 ? old_right + trail : old_right + lead),
                        (name + ": line not carried with its panel").c_str());
                ++lines;
            } else {
                // A choice's cell: the wide column took the width, the box is no narrower than before.
                require(box.width() >= before.width(), (name + ": choice narrower than before").c_str());
                column_cells.push_back({c.u32(at + 0x24), box});
                ++choices;
            }
        } else if (owner < 0) {
            if (before == ScreenRect{100, 80, 790, 125}) require(box == shell.subtitle(), (name + ": subtitle did not follow the shell").c_str());
            else if (before == ScreenRect{100, 10, 790, 70}) require(box == shell.title(), (name + ": title did not follow the shell").c_str());
            else require(box == before, (name + ": an unrelated standalone control moved").c_str());
        }
    }
    // Choices stack without overlapping in row order.
    std::sort(column_cells.begin(), column_cells.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (std::size_t i = 1; i < column_cells.size(); ++i)
        require(column_cells[i - 1].second.bottom <= column_cells[i].second.top, (name + ": choices overlap").c_str());
    require(lines >= 2 && choices >= 7, (name + ": fixture found too few lines or choices to check").c_str());
}

inline void bid_rejection_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 6;
    const char* names[scenarios]{"one offer", "reason 0x200", "reason 0x1000", "two offers", "two offers, reason 0x400", "offer in history 1"};
    const unsigned reasons[scenarios]{0, 0x200, 0x1000, 0, 0x400, 0};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = true;
                unsigned other = 0;
                const auto clubs = c.u32(c.toc(-0x7be8));
                for (unsigned k = 0; k < c.u32(c.toc(-0x7b50)) && !other; ++k) {
                    const auto club = clubs + k * 0x248;
                    if (club != t.managed && c.u32(club + 0x30) == c.u32(t.managed + 0x30)) other = club;
                }
                require(other, "Bid rejection fixture needs a second club");
                const auto offer = allocate(t.s, 0x54, true), link = allocate(t.s, 4, true);
                t.invoke(0x10411ea0, {offer, t.person, other, 0, 0xffffffffu, 1});
                c.put32(offer + 0x28, 0xffffffffu); c.put32(offer + 0x2c, 0); c.put32(link, offer);
                const bool two = scenario == 3 || scenario == 4;
                const auto second = allocate(t.s, 0x54, true), array = allocate(t.s, 8, true), list = allocate(t.s, 0x20, true), nodes = allocate(t.s, 0x20, true);
                if (two) {
                    // A second pending offer for the same player: the manager's offer array and the first offer's chain list both.
                    t.invoke(0x10411ea0, {second, t.person, other, 0, 0xffffffffu, 1});
                    c.put32(second + 0x28, 0xffffffffu); c.put32(second + 0x2c, 0);
                    c.put32(array, offer); c.put32(array + 4, second);
                    const auto manager = c.u32(c.toc(-0x7b40));
                    c.put32(manager, array); c.put32(manager + 4, 2);
                    c.put32(list + 4, nodes);
                    for (unsigned k = 0; k < 2; ++k) {
                        const auto node = nodes + k * 0x10;
                        c.put32(node, node + 0xc); c.put32(node + 8, k ? 0 : node + 0x10); c.put32(node + 0xc, k);
                    }
                    c.put32(offer, list);
                }
                t.invoke(0x1041f830, {link, link});
                require(t.builder() == 0x1041f8f0, "Bid rejection did not open through its original opener");
                t.set_value(2, reasons[scenario]);
                if (scenario == 5) t.set_value(0, 0);
                bool on = adaptive;
                const auto records = [&] { return t.records({{offer, 0x54}, {link, 4}, {second, 0x54}, {array, 8}, {list, 0x20}, {nodes, 0x20}}); };
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if (owner < 0 || owner >= int(t.grids()) || (owner > 0 && owner != 20) || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        require(t.hit(i) == int(i), "Bid rejection picker missed an original control");
                    }
                };
                BidShape shape;
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Bid rejection gate disagrees with preference");
                    t.rebuild(size);
                    shape = bid_shape(t);
                    require(shape.panel >= 0 && shape.footer >= 0, "Original bid rejection panels absent");
                    require(shape.buttons == (two ? 3u : 2u), "Bid rejection footer has an unexpected number of buttons");
                    old = t.layout(records, "Bid rejection");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy bid rejection changed"); pick(); return; }
                    verify_bid_rejection(t, size, shape, old, "Bid rejection");
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    // Without an offer in either history value, or with one naming a person outside the database, the screen stays classic.
                    const auto first = t.value(0), fallback = t.value(1);
                    t.set_value(0, 0); t.set_value(1, 0);
                    require(!adaptive_screen_available(t.s, 0x1041f8f0), "Bid rejection gate accepted a missing offer");
                    t.set_value(0, first); t.set_value(1, fallback);
                    const auto person = c.u32(offer + 12); c.put32(offer + 12, 0x7fffffff);
                    require(!adaptive_screen_available(t.s, 0x1041f8f0), "Bid rejection gate accepted an invalid person");
                    c.put32(offer + 12, person);
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
                    require(again == boxes, "Bid rejection redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_bid_rejection(t, canvas, shape, old, "Bid rejection"); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    verify();
                    {   // the panel stays at its classic rectangle
                        c.put32(t.grid(shape.panel), 110); c.put32(t.grid(shape.panel) + 8, 780);
                        c.put32(t.grid(shape.panel) + 4, 140); c.put32(t.grid(shape.panel) + 12, 545);
                        require_rejected(verify, "panel left at classic coordinates"); restore();
                    }
                    {   // the last button drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == shape.buttons - 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore();
                    }
                    {   // the footer sits at the canvas's bottom row instead of under the panel
                        c.put32(t.grid(shape.footer) + 4, canvas.height - 45); c.put32(t.grid(shape.footer) + 12, canvas.height - 10);
                        require_rejected(verify, "footer left at the bottom row"); restore();
                    }
                    {   // a line's text stays at its classic bounds
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(t.control(k) + 0x72) != shape.panel || !display_word(old, old_at + 0x28)) continue;
                            c.put32(t.control(k) + 0x28, display_word(old, old_at + 0x28)); done = true;
                        }
                        require(done, "Bid rejection fixture found no line to mutate");
                        require_rejected(verify, "line left at classic bounds"); restore();
                    }
                    {   // a choice climbs onto the one above it
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != shape.panel || c.u32(at + 0x24) != 6) continue;
                            c.put32(at + 0x14, c.i32(at + 0x14) - 20); done = true;
                        }
                        require(done, "Bid rejection fixture found no choice to mutate");
                        require_rejected(verify, "choices overlap"); restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing through the original No (action 3): same result, records and RNG with and without the layout.
                std::vector<std::uint8_t> outcome;
                const auto saved = records();
                const auto texts = t.meanings();
                const int no = t.find(3);
                require(no >= 0 && t.hit(no) == no, "Bid rejection No button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(no)}));
                require(records() == saved, "Bid rejection No changed records");
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                outcome.insert(outcome.end(), saved.begin(), saved.end()); outcome.insert(outcome.end(), texts.begin(), texts.end());
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive bid rejection changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Bid rejection scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "bid rejection state " << scenario << " (" << names[scenario] << "): dialog geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

// Shared fixture of the Back/Next screens that share paged_screen_layout.hpp: Board Confidence, Manager
// Statistics and the FIFA World Rankings. Each is opened through its original opener on a disposable copy of
// an existing capture, built with New Screens off and on at six canvases, and the adaptive run must keep every
// control, command, record and RNG draw and only move them: a centered content panel, the toolbar above it on
// its own side, and a compact two-button footer, equal and right-aligned to the panel. A mutant of each
// geometric property must be rejected by the same checker. Needs TransferScene (adaptive_transfer_status_checks.hpp).
#include <functional>
#include <map>
struct PagedCase { const char* name; std::function<void(TransferScene&)> open; };
struct PagedShape { int panel = -1, footer = -1; };

inline PagedShape paged_shape(TransferScene& t, ScreenRect panel_box, GuiCanvas size) {
    PagedShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i));
        if (box == panel_box) r.panel = int(i);
        else if (t.c.u8(t.grid(i) + 0xbb0) == 2 && t.c.u8(t.grid(i) + 0xbb1) == 1 && box.left >= 100 && box.bottom == size.height - 10) r.footer = int(i);
    }
    return r;
}

// Every property the layout promises, read from the guest after it ran. `old` is the display image from before.
// `message` selects the one-column message check (lines carried with the panel) instead of the column check.
inline void verify_paged(TransferScene& t, GuiCanvas size, const PagedShape& shape, const std::vector<std::uint8_t>& old,
                         ScreenRect panel_box, bool message, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(panel_box.top);
    const int width = std::min(960, content.width());
    const auto body = t.rect(t.grid(shape.panel)), footer = t.rect(t.grid(shape.footer));
    require(body != panel_box, (name + ": layout did not run, panel stays at classic coordinates").c_str());
    require(body.width() == width && body.left == content.left + (content.width() - width) / 2 && body.top == panel_box.top,
            (name + ": panel is not centered in the content area").c_str());
    require(body.height() >= panel_box.height() && body.bottom <= content.bottom && body.bottom < footer.top &&
            body.height() <= int(c.u8(t.grid(shape.panel) + 0xbb1)) * 28 + 16, (name + ": panel height invalid").c_str());
    // Footer: right-aligned to the panel, on the canvas's bottom row, two equal buttons that fit inside it.
    require(footer.right == body.right && footer.top == size.height - 45 && footer.bottom == size.height - 10 &&
            footer.width() == std::min(body.width(), 340), (name + ": footer not compact and aligned to the panel").c_str());
    std::vector<ScreenRect> buttons;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const int owner = c.i16(at + 0x72);
        if (owner == shape.footer) buttons.push_back(t.rect(at + 0x10));
    }
    require(buttons.size() == 2, (name + ": footer lost a button").c_str());
    for (const auto& b : buttons)
        require(b.left >= footer.left && b.right <= footer.right && b.top == footer.top && b.bottom == footer.bottom && b.width() >= 100,
                (name + ": footer button outside its footer").c_str());
    require(std::abs(buttons[0].width() - buttons[1].width()) <= 1 && buttons[0].right <= buttons[1].left,
            (name + ": footer buttons unequal or overlapping").c_str());
    require(buttons[1].right == footer.right && buttons[0].left == footer.left, (name + ": footer buttons not flush with the footer").c_str());

    std::map<unsigned, std::pair<int, int>> widths; // column -> (old width, new width)
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        if (owner == shape.panel) {
            if (box.top == 0 && box.bottom == 0) continue;
            require(box.left >= body.left && box.right <= body.right && box.top >= body.top && box.bottom <= body.bottom,
                    (name + ": panel control outside its panel").c_str());
            const int old_left = display_word(old, old_at + 0x28), old_right = display_word(old, old_at + 0x2c);
            if (message) {
                // A message line carries with the panel's left edge and its text area's right edge.
                const int lead = body.left - panel_box.left, trail = body.right - panel_box.right;
                require(old_left && old_right && c.i32(at + 0x28) == old_left + lead &&
                        c.i32(at + 0x2c) == (old_right >= panel_box.right - 10 ? old_right + trail : old_right + lead),
                        (name + ": message line not carried with its panel").c_str());
            } else if (!old_left && !old_right) {
                // Columns: only one grows, every cell of a column has the column's width, order is kept.
                const unsigned column = c.u32(at + 0x20);
                auto [it, inserted] = widths.try_emplace(column, std::pair{before.width(), box.width()});
                require(inserted || it->second == std::pair{before.width(), box.width()}, (name + ": cells of one column differ").c_str());
            } else {
                // Explicit text bounds (an empty-table message) stay centered in the panel.
                require(std::abs((c.i32(at + 0x28) + c.i32(at + 0x2c)) - (old_left + old_right) - (body.left + body.right - panel_box.left - panel_box.right)) <= 1 &&
                        c.i32(at + 0x2c) - c.i32(at + 0x28) == old_right - old_left, (name + ": panel message not centered").c_str());
            }
        } else if (owner == shape.footer) {
            continue;
        } else if (owner < 0) {
            const bool toolbar = before.top >= 125 && before.bottom <= panel_box.top && before.left >= panel_box.left && before.right <= panel_box.right;
            if (before == ScreenRect{100, 80, 790, 125}) {
                require(box == shell.subtitle(), (name + ": subtitle did not follow the shell").c_str());
            } else if (toolbar) {
                const int shift = before.left < (panel_box.left + panel_box.right) / 2 ? body.left - panel_box.left : body.right - panel_box.right;
                require(box.left == before.left + shift && box.right == before.right + shift && box.top == before.top && box.bottom == before.bottom,
                        (name + ": toolbar control not moved with its panel edge").c_str());
                require(box.bottom <= body.top && box.left >= body.left && box.right <= body.right, (name + ": toolbar outside the panel's width").c_str());
                // Its explicit text bounds follow when they mirrored the rectangle.
                if (display_word(old, old_at + 0x28) == before.left && display_word(old, old_at + 0x2c) == before.right)
                    require(c.i32(at + 0x28) == box.left && c.i32(at + 0x2c) == box.right, (name + ": toolbar text bounds stayed behind").c_str());
            } else {
                require(box == before, (name + ": an unrelated standalone control moved").c_str());
            }
        }
    }
    if (!message && !widths.empty()) {
        int grown = 0, shrunk = 0, previous_right = -1;
        for (const auto& [column, pair] : widths) { grown += pair.second > pair.first; shrunk += pair.second < pair.first; }
        require(!shrunk && grown <= 1 && (grown == 1 || body.width() == panel_box.width()), (name + ": more than the widest column changed width").c_str());
        (void)previous_right;
    }
}

// Runs the checker on a deliberately broken layout: it must refuse.
inline void require_rejected(const std::function<void()>& verify, const char* mutant) {
    bool rejected = false;
    try { verify(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, (std::string("Checker accepted mutant: ") + mutant).c_str());
}

inline void paged_screen_fixture(const char* path, const char* title, unsigned builder, ScreenRect panel_box, bool message,
                                 const std::vector<PagedCase>& cases, const std::function<void(TransferScene&)>& negative_gate) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    for (unsigned scenario = 0; scenario < cases.size(); ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                cases[scenario].open(t);
                require(t.builder() == builder, "Paged screen did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records(); };
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if (owner >= int(t.grids()) || (owner >= 0 && owner < 19) || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.left < 90) continue; // the sidebar and closed menus
                        require(t.hit(i) == int(i), "Paged screen picker missed an original control");
                    }
                };
                PagedShape shape;
                std::vector<std::uint8_t> old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Paged screen gate disagrees with preference");
                    t.rebuild(size);
                    shape = paged_shape(t, panel_box, size);
                    require(shape.panel >= 0 && shape.footer >= 0, "Original paged panels absent");
                    old = t.layout(records, title);
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy paged screen changed"); pick(); return; }
                    verify_paged(t, size, shape, old, panel_box, message, title);
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    negative_gate(t);
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
                    require(again == boxes, "Paged screen redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_paged(t, canvas, shape, old, panel_box, message, title); };
                    const auto restore = [&](const std::vector<std::uint8_t>& image) { t.s.memory.copy_in(t.display, image); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    verify();
                    {   // the second footer button drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore(saved);
                    }
                    {   // the panel stays at its classic rectangle
                        c.put32(t.grid(shape.panel), panel_box.left); c.put32(t.grid(shape.panel) + 8, panel_box.right);
                        c.put32(t.grid(shape.panel) + 4, panel_box.top); c.put32(t.grid(shape.panel) + 12, panel_box.bottom);
                        require_rejected(verify, "panel left at classic coordinates"); restore(saved);
                    }
                    {   // a toolbar or pager control goes back to its classic place
                        bool moved = false;
                        for (unsigned k = 0; k < t.controls() && !moved; ++k) {
                            const auto at = t.control(k); const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(at + 0x72) >= 0 || display_word(old, old_at + 0x14) < 125 || display_word(old, old_at + 0x1c) > panel_box.top ||
                                c.i32(at + 0x10) == display_word(old, old_at + 0x10)) continue;
                            c.put32(at + 0x10, display_word(old, old_at + 0x10)); c.put32(at + 0x18, display_word(old, old_at + 0x18)); moved = true;
                        }
                        require(moved, "Paged screen fixture found no toolbar control to mutate");
                        require_rejected(verify, "toolbar control left at classic coordinates"); restore(saved);
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing the screen through its original Back button: same result and RNG with and without the layout.
                std::vector<std::uint8_t> outcome;
                const int back = t.find(0xfffffffeu);
                require(back >= 0 && t.hit(back) == back, "Paged screen Back button missing");
                const auto rng = random_state(t.s);
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(back)}));
                require(random_state(t.s) == rng, "Paged screen Back changed the RNG");
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive paged screen changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string(title) + " scenario " + cases[scenario].name + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << title << " state " << scenario << " (" << cases[scenario].name << "): panel, toolbar and footer geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

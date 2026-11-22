// Pages with a tab bar (tabbed_page_layout.cpp): the transfer rules article of the news (0x106ff680), the Historial
// (0x10145290), the game database (0x10106840) and a club's Historial (0x1003b7f0), each opened through its original
// opener on a disposable copy of an existing capture and built with New Screens off and on at six canvases. The
// adaptive run keeps every control, command, record and RNG draw and only moves them: one centered panel of at most 960,
// the tab bar over it with equal tabs, the body as tall as its rows can use (a second tab bar under it when the page has
// one), the controls above the body at their side of the panel (a summary line spanning it), only the widest column of
// the body growing and the Back/Next footer right-aligned to the panel. Mutants of each promise must be refused by the
// same checker. Needs TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
struct TabbedCase { const char* name; std::function<void(TransferScene&)> open; };
struct TabbedShape { int upper_backdrop = -1, upper_tabs = -1, lower_backdrop = -1, lower_tabs = -1, body = -1, footer = -1, filter = -1; int bar_top = 0; };

inline TabbedShape tabbed_shape(TransferScene& t, GuiCanvas size) {
    TabbedShape r;
    const ScreenLayout shell{size};
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        if ((box == shell.footer() || box == ScreenRect{100, 555, 790, 590}) && columns == 2 && rows == 1) r.footer = int(i);
        else if (box.left == 100 && box.right == 790 && box.height() == 35 && rows == 1) {
            const bool above = box.top == 80 || box.top == 140;
            if (above) r.bar_top = box.top;
            (above ? (columns == 1 ? r.upper_backdrop : r.upper_tabs) : (columns == 1 ? r.lower_backdrop : r.lower_tabs)) = int(i);
        } else if (box.left == 110 && box.right == 780 && box.height() >= 100) r.body = int(i);
        else if (box.left == 110 && box.height() == 20 && box.right <= 400 && box.top == 125 && rows == 1) r.filter = int(i);
    }
    return r;
}

// Every property the layout promises, read from the guest after it ran. `old` is the display image from before it.
inline void verify_tabbed(TransferScene& t, GuiCanvas size, const TabbedShape& shape, const std::vector<std::uint8_t>& old, ScreenRect body_before, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const ScreenLayout shell{size};
    const auto content = shell.content(body_before.top);
    const int width = std::min(960, content.width());
    const bool has_lower = shape.lower_tabs >= 0;
    const auto panel = t.rect(t.grid(shape.body)), footer = t.rect(t.grid(shape.footer));
    const unsigned rows = c.u8(t.grid(shape.body) + 0xbb1);
    require(panel != body_before || t.rect(t.grid(shape.upper_tabs)) != ScreenRect{100, shape.bar_top, 790, shape.bar_top + 35},
            (name + ": layout did not run, the page stays at classic coordinates").c_str());
    require(panel.width() == width && panel.left == content.left + (content.width() - width) / 2 && panel.right <= size.width - 10 &&
            std::abs((panel.left + panel.right) - (shell.title().left + shell.title().right)) <= 2,
            (name + ": body is not centered under the title bars and bounded to 960").c_str());
    // As tall as its rows can use (28 pixels each and a margin), never below the classic height; the extra height of the canvas is all there is to give.
    const int extra = size.height - 600;
    require(panel.top == body_before.top && panel.height() == std::max(body_before.height(), std::min(body_before.height() + extra, int(rows) * 28 + 16)),
            (name + ": body is not as tall as its rows can use").c_str());
    // The tab bar spans the panel: its backdrop and its tabs.
    const ScreenRect upper{panel.left, shape.bar_top, panel.right, shape.bar_top + 35};
    require(t.rect(t.grid(shape.upper_backdrop)) == upper && t.rect(t.grid(shape.upper_tabs)) == upper, (name + ": tab bar does not span the panel").c_str());
    if (has_lower) {
        const ScreenRect lower{panel.left, panel.bottom + 10, panel.right, panel.bottom + 45};
        require(t.rect(t.grid(shape.lower_backdrop)) == lower && t.rect(t.grid(shape.lower_tabs)) == lower, (name + ": second tab bar not under the body").c_str());
        require(lower.bottom <= footer.top - 10, (name + ": second tab bar collides with the footer").c_str());
    } else {
        require(panel.bottom <= content.bottom && panel.bottom < footer.top, (name + ": body runs into the footer").c_str());
    }
    // Footer: two equal buttons flush with the panel's right edge on the canvas's bottom row.
    require(footer.right == panel.right && footer.top == size.height - 45 && footer.bottom == size.height - 10 && footer.width() == std::min(panel.width(), 340),
            (name + ": footer not compact and aligned to the panel").c_str());
    // Filter pair: its size and place at the panel's left edge.
    if (shape.filter >= 0) {
        const auto before = ScreenRect{display_word(old, 4 + unsigned(shape.filter) * 0xbf0), display_word(old, 8 + unsigned(shape.filter) * 0xbf0),
                                       display_word(old, 12 + unsigned(shape.filter) * 0xbf0), display_word(old, 16 + unsigned(shape.filter) * 0xbf0)};
        const auto now = t.rect(t.grid(shape.filter));
        require(now == ScreenRect{before.left + panel.left - body_before.left, before.top, before.right + panel.left - body_before.left, before.bottom},
                (name + ": filter pair not at the panel's left edge").c_str());
    }

    const int lead = panel.left - body_before.left, trail = panel.right - body_before.right;
    std::map<unsigned, std::pair<int, int>> widths;   // body column -> (old width, new width)
    std::vector<std::pair<unsigned, ScreenRect>> tabs;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        const int owner = c.i16(at + 0x72);
        const auto box = t.rect(at + 0x10);
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        const int old_left = display_word(old, old_at + 0x28), old_right = display_word(old, old_at + 0x2c);
        if (owner == shape.body) {
            if (box.top == 0 && box.bottom == 0) continue;
            require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom, (name + ": body control outside its panel").c_str());
            if (old_left && old_right) {
                require(c.i32(at + 0x28) == old_left + lead && c.i32(at + 0x2c) == (old_right >= body_before.right - 10 ? old_right + trail : old_right + lead),
                        (name + ": text line not carried with its panel").c_str());
            } else {
                const unsigned column = c.u32(at + 0x20);
                auto [it, inserted] = widths.try_emplace(column, std::pair{before.width(), box.width()});
                require(inserted || it->second == std::pair{before.width(), box.width()}, (name + ": cells of one column differ").c_str());
            }
        } else if (owner == shape.upper_tabs) {
            tabs.push_back({c.u32(at + 0x20), box});
        } else if (owner < 0 && before.left >= 90) {
            if (before.top >= shape.bar_top + 35 && before.bottom <= body_before.top && before.left >= body_before.left && before.right <= body_before.right) {
                if (before.left == body_before.left && before.right == body_before.right)
                    require(box == ScreenRect{panel.left, before.top, panel.right, before.bottom}, (name + ": summary line does not span the panel").c_str());
                else {
                    const int shift = before.left < (body_before.left + body_before.right) / 2 ? lead : trail;
                    require(box == ScreenRect{before.left + shift, before.top, before.right + shift, before.bottom}, (name + ": control above the body did not follow its panel edge").c_str());
                }
            } else require(box == before, (name + ": an unrelated standalone control moved").c_str());
        }
    }
    // The tabs: every one kept, in order, equal, flush with the bar, one row.
    std::sort(tabs.begin(), tabs.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    require(tabs.size() == c.u8(t.grid(shape.upper_tabs) + 0xbb0) && tabs.size() >= 2, (name + ": a tab was lost").c_str());
    for (std::size_t i = 0; i < tabs.size(); ++i) {
        const auto& b = tabs[i].second;
        require(b.top == upper.top && b.bottom == upper.bottom && b.left >= upper.left && b.right <= upper.right && b.width() >= 60, (name + ": tab outside its bar").c_str());
        if (i) require(tabs[i - 1].second.right <= b.left && std::abs(tabs[i - 1].second.width() - b.width()) <= 1, (name + ": tabs unequal or overlapping").c_str());
    }
    require(tabs.front().second.left - upper.left <= 2 && upper.right - tabs.back().second.right <= 2, (name + ": tabs not flush with their bar").c_str());
    // Only the widest column of the body took the extra width, the others keep theirs.
    int grown = 0, shrunk = 0;
    for (const auto& [column, pair] : widths) { grown += pair.second > pair.first; shrunk += pair.second < pair.first; }
    require(!shrunk && grown <= 1, (name + ": more than the widest column changed width").c_str());
}

inline void tabbed_page_fixture(const char* path, const char* title, unsigned builder, const std::vector<TabbedCase>& cases, bool lower_bar,
                                const std::function<std::vector<std::uint8_t>(TransferScene&)>& extra_records = {}) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    for (unsigned scenario = 0; scenario < cases.size(); ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = false;
                cases[scenario].open(t);
                require(t.builder() == builder, "Tabbed page did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { auto bytes = t.records(); if (extra_records) { const auto more = extra_records(t); bytes.insert(bytes.end(), more.begin(), more.end()); } return bytes; };
                // A few controls cannot be picked even in the original layout (a menu's second cell); the adaptive layout must pick
                // every control the original does.
                std::set<unsigned> unpickable;
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.left < 90 || box.top == 0 || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const int got = t.hit(i);
                        if (!on) { if (got != int(i)) unpickable.insert(i); continue; }
                        require(got == int(i) || unpickable.count(i), (std::string("Tabbed page picker missed an original control ") + std::to_string(i) + " got " + std::to_string(got)).c_str());
                    }
                };
                TabbedShape shape;
                std::vector<std::uint8_t> old;
                ScreenRect body_before{};
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Tabbed page gate disagrees with preference");
                    t.rebuild(size);
                    shape = tabbed_shape(t, size);
                    require(shape.upper_backdrop >= 0 && shape.upper_tabs >= 0 && shape.body >= 0 && shape.footer >= 0 && (shape.lower_tabs >= 0) == lower_bar,
                            "Original tabbed page grids absent");
                    body_before = t.rect(t.grid(shape.body));
                    old = t.layout(records, title);
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy tabbed page changed"); pick(); return; }
                    verify_tabbed(t, size, shape, old, body_before, title);
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                if (adaptive) { on = false; select_new_screens(false); build({800, 600}); on = true; select_new_screens(true); }
                build(canvas);
                if (adaptive) {
                    {   // Without a career there is no screen to lay out.
                        const auto instance = c.u32(t.manager + 4); c.put32(t.manager + 4, 0);
                        require(!adaptive_screen_available(t.s, builder), "Tabbed page gate accepted a missing instance");
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
                    require(again == boxes, "Tabbed page redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_tabbed(t, canvas, shape, old, body_before, title); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    const auto put_rect = [&](unsigned at, ScreenRect r) { c.put32(at, r.left); c.put32(at + 4, r.top); c.put32(at + 8, r.right); c.put32(at + 12, r.bottom); };
                    verify();
                    {   // the tab bar stays at its classic rectangle
                        put_rect(t.grid(shape.upper_tabs), {100, shape.bar_top, 790, shape.bar_top + 35});
                        require_rejected(verify, "tab bar left at classic coordinates"); restore();
                    }
                    {   // the body stops sharing the panel's width
                        auto r = t.rect(t.grid(shape.body)); r.right -= 9;
                        put_rect(t.grid(shape.body), r);
                        require_rejected(verify, "body width differs"); restore();
                    }
                    if (t.rect(t.grid(shape.body)).height() != body_before.height()) {   // the body keeps its classic height (a body of one row has none to gain)
                        auto r = t.rect(t.grid(shape.body)); r.bottom = r.top + body_before.height();
                        put_rect(t.grid(shape.body), r);
                        require_rejected(verify, "body not grown"); restore();
                    }
                    {   // the footer stays at the left of the panel
                        auto r = t.rect(t.grid(shape.footer)); r.right -= 30;
                        put_rect(t.grid(shape.footer), r);
                        require_rejected(verify, "footer not right-aligned"); restore();
                    }
                    {   // one tab drifts
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.upper_tabs && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) - 25); done = true; }
                        require(done, "Tabbed page fixture found no tab to mutate");
                        require_rejected(verify, "a tab narrower than the others"); restore();
                    }
                    if (shape.filter >= 0) {   // the filter pair stays at its classic place
                        auto r = t.rect(t.grid(shape.filter)); r.left = 110; r.right = 360;
                        put_rect(t.grid(shape.filter), r);
                        require_rejected(verify, "filter pair left at the classic place"); restore();
                    }
                    if (lower_bar) {   // the second bar stays at its classic place
                        put_rect(t.grid(shape.lower_tabs), {100, 510, 790, 545});
                        require_rejected(verify, "second bar left at the classic place"); restore();
                    }
                    {   // a standalone control above the body (a summary line or a page control) goes back to its classic place
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k); const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(at + 0x72) >= 0 || display_word(old, old_at + 0x14) < shape.bar_top + 35 || display_word(old, old_at + 0x1c) > body_before.top || display_word(old, old_at + 0x10) < 110) continue;
                            c.put32(at + 0x10, display_word(old, old_at + 0x10)); c.put32(at + 0x18, display_word(old, old_at + 0x18)); done = true;
                        }
                        if (done) require_rejected(verify, "a control above the body left at its classic place");
                        restore();
                    }
                    {   // a text line of the body goes back to its classic bounds
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k) {
                            const auto at = t.control(k); const unsigned old_at = 0xba864 + k * 0x17c;
                            if (c.i16(at + 0x72) != shape.body || !display_word(old, old_at + 0x28) || !display_word(old, old_at + 0x2c)) continue;
                            c.put32(at + 0x28, display_word(old, old_at + 0x28)); done = true;
                        }
                        if (done) require_rejected(verify, "a text line left at its classic bounds");
                        restore();
                    }
                    verify();
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing the page through its original Back button: same result and records with and without the layout.
                std::vector<std::uint8_t> outcome;
                const int back = t.find(0xfffffffeu);
                require(back >= 0 && t.hit(back) == back, "Tabbed page Back button missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(back)}));
                const auto after = records();
                for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                outcome.insert(outcome.end(), after.begin(), after.end());
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive tabbed page changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string(title) + " scenario " + cases[scenario].name + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << title << " state " << scenario << " (" << cases[scenario].name << "): tab bar, body, controls and footer geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

// The article: an item of the news, the first tab and each of the other three (the page's own handler selects it).
inline void tabbed_open_article(TransferScene& t, unsigned tab) {
    const auto news = t.news_item(6000, {0, 0, 0});
    t.invoke(0x106ff550, {t.instance, news});
    if (!tab) return;
    t.rebuild({801, 601});
    int pick = -1;
    for (unsigned i = 0; i < t.controls(); ++i)
        if (t.c.i16(t.control(i) + 0x72) >= 0 && t.c.u32(t.control(i) + 0x20) == tab && t.c.u32(t.control(i) + 8) == 1 && t.rect(t.control(i) + 0x10).top == 140) pick = int(i);
    require(pick >= 0, "Article fixture: no tab control");
    t.invoke(t.c.u32(t.c.u32(t.history() + 8)), {unsigned(pick)});
}
void tabbed_pages_builder(const char* path) {
    tabbed_page_fixture(path, "Transfer rules article", 0x106ff680, {
        {"summary", [](TransferScene& t) { tabbed_open_article(t, 0); }},
        {"periods", [](TransferScene& t) { tabbed_open_article(t, 1); }},
        {"young players", [](TransferScene& t) { tabbed_open_article(t, 2); }},
        {"contracts", [](TransferScene& t) { tabbed_open_article(t, 3); }},
    }, false, [](TransferScene& t) { return t.news_queue(); });
    tabbed_page_fixture(path, "Historial", 0x10145290, {
        {"nations", [](TransferScene& t) { t.invoke(0x101451d0, {0}); }},
        {"competitions", [](TransferScene& t) { t.invoke(0x101451d0, {1}); }},
        {"clubs", [](TransferScene& t) { t.invoke(0x101451d0, {3}); }},
        {"players", [](TransferScene& t) { t.invoke(0x101451d0, {4}); }},
    }, false);
    tabbed_page_fixture(path, "Game database", 0x10106840, {
        {"first tab", [](TransferScene& t) { t.invoke(0x10106530, {0}); }},
        {"nations", [](TransferScene& t) { t.invoke(0x10106530, {1}); }},
        {"clubs", [](TransferScene& t) { t.invoke(0x10106530, {2}); }},
        {"staff", [](TransferScene& t) { t.invoke(0x10106530, {3}); }},
    }, false);
    tabbed_page_fixture(path, "Club Historial", 0x1003b7f0, {
        {"managed club", [](TransferScene& t) { t.invoke(0x1003b3a0, {t.managed}); }},
        {"another club", [](TransferScene& t) {
            const auto clubs = t.c.u32(t.c.toc(-0x7be8)); unsigned other = 0;
            for (unsigned k = 0; k < t.c.u32(t.c.toc(-0x7b50)) && !other; ++k) if (clubs + k * 0x248 != t.managed) other = clubs + k * 0x248;
            t.invoke(0x1003b3a0, {other}); }},
    }, true);
}

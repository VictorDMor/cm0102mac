void nations_and_clubs_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
            "Nations & Clubs pixel initialization");
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000;
        c.call(entry, return_sentinel);
        return c.r(3);
    };
    constexpr auto builder = 0x10103850u;
    const auto continent = invoke(0x10192250, {}), nation = invoke(0x101922e0, {});
    invoke(0x10103600, {1, continent, nation, 0});
    const auto display = c.u32(manager + 0x24), history = c.u32(manager + 0x10);
    const auto setting = [history](unsigned slot) { return history + 0x14 + slot * 8; };
    const auto grid = [display](unsigned index) { return display + 4 + index * 0xbf0; };
    const auto control = [display](unsigned index) { return display + 0xba864 + index * 0x17c; };
    const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at), c.i32(at+4), c.i32(at+8), c.i32(at+12)}; };
    const auto meanings = [&] {
        std::vector<std::uint8_t> bytes;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            for (const auto [offset, size] : {std::pair{8u,8u}, {0x20u,8u}, {0x38u,20u}, {0x68u,0x114u}}) {
                const auto part = s.memory.copy_out(control(i) + offset, size);
                bytes.insert(bytes.end(), part.begin(), part.end());
            }
        }
        return bytes;
    };
    const auto hit = [&](unsigned i) {
        const auto box = rect(control(i) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left + box.right) / 2);
        c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
        c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
        return std::int16_t(invoke(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
    };

    require(continent && nation && adaptive_screen_available(s, builder), "Original browser did not opt in");
    c.put32(setting(1), 0); require(!adaptive_screen_available(s, builder), "Missing continent opted in");
    c.put32(setting(1), continent);
    c.put32(setting(0), 7); require(!adaptive_screen_available(s, builder), "Unknown tab opted in");
    c.put32(setting(0), 1);
    c.put32(setting(19), 3); require(!adaptive_screen_available(s, builder), "Unknown browser mode opted in");
    c.put32(setting(19), 0);
    c.put16(manager + 0x2c58, 1); require(!adaptive_screen_available(s, builder), "Secondary browser opted in");
    c.put16(manager + 0x2c58, 0);
    c.put32(manager + 0x2c64, 1); require(!adaptive_screen_available(s, builder), "Network browser opted in");
    c.put32(manager + 0x2c64, 0);

    unsigned filter = 0, tab_link = 0, row_link = 0;
    const auto build = [&](unsigned mode, unsigned tab, GuiCanvas canvas, bool enabled, unsigned scroll) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, canvas), "Nations & Clubs resize");
        c.put32(setting(0), tab); c.put32(setting(19), mode);
        const unsigned selected = mode == 2 ? 1 : tab;
        c.put32(setting(3 * selected - 1), scroll);
        require(adaptive_screen_available(s, builder) == enabled, "Wrong browser preference gate");
        invoke(0x10719560, {display, 1, 0}); invoke(builder, {});
        require(c.u32(setting(0)) == selected, "Original builder chose an unexpected tab");
        const auto body_index = c.u32(setting(3 * selected)), body = grid(body_index);
        require(body_index < c.u16(display + 0x129da4), "Original builder lost its list index");
        const auto old = rect(body);
        const int old_rows = c.u8(body + 0xbb1), last = c.i32(body + 0x14), old_scroll = c.i32(body + 0x1fc);
        const auto saved_history = s.memory.copy_out(history, 0x1f4), rng = random_state(s);
        const auto saved_nation = s.memory.copy_out(nation, 0x16c);
        const auto saved_continent = s.memory.copy_out(continent, 0x9c);
        const auto original = s.memory.copy_out(display, 0x129da8), semantic = meanings();
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto stack_address = c.sp() - 0x8000;
        const auto stack = s.memory.copy_out(stack_address, 0x8100);
        layout_built_screen(s, display, builder);
        require(s.memory.copy_out(history, 0x1f4) == saved_history && random_state(s) == rng &&
                s.memory.copy_out(nation, 0x16c) == saved_nation && s.memory.copy_out(continent, 0x9c) == saved_continent,
                "Nations & Clubs layout changed browser history, records or RNG");
        require(s.registers == registers && s.unique == unique && s.pc == pc &&
                s.memory.copy_out(stack_address, 0x8100) == stack, "Nations & Clubs layout changed guest ABI");
        require(meanings() == semantic, "Layout changed browser labels, styles or commands");
        if (!enabled || canvas.original()) {
            require(s.memory.copy_out(display, 0x129da8) == original, "Classic browser layout changed");
            return;
        }
        const auto panel = rect(body);
        const int rows = c.u8(body + 0xbb1);
        require(panel.top == old.top && panel.width() == std::min(1400, canvas.width - 130) &&
                panel.left == 110 + (canvas.width - 130 - panel.width()) / 2 && panel.bottom <= canvas.height - (mode == 0 ? 100 : 55),
                "Browser list did not fit its centered panel");
        require(rows >= old_rows && rows <= 30 && (canvas.height < 900 || rows > old_rows) &&
                (canvas.height < 1800 || rows == 30), "Nations & Clubs visible row capacity");
        require(c.i32(body + 0x1fc) == std::clamp(old_scroll, 0, std::max(0, last + 1 - rows)) &&
                bool(c.u8(body + 0xbef)) == (last + 1 > rows), "Browser scroll offset or scrollbar changed incorrectly");
        for (int row = old_rows; row < rows; ++row) require(c.u8(body + 0xbd0 + row) == 1, "Browser row weight");

        // The picker uses the original grid and control hit areas after the
        // usual display refresh, including clipped rows beyond the list.
        unsigned strips = 0, buttons = 0, links = 0, headings = 0;
        filter = tab_link = row_link = 0;
        const int footer_top = panel.bottom + (mode == 0 ? 55 : 10);
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            const auto area = grid(i);
            invoke(0x10714630, {area}); invoke(0x10714110, {area, 0});
            const auto box = rect(area);
            if (box == ScreenRect{panel.left, 80, panel.right, 115} ||
                box == ScreenRect{panel.left, panel.bottom+10, panel.right, panel.bottom+45}) ++strips;
            if (box == ScreenRect{panel.right-340, footer_top, panel.right, footer_top+35}) {
                ++buttons; require(c.u8(area+0xbb2) == 1 && c.u8(area+0xbb3) == 1, "Unequal browser buttons");
            }
        }
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = control(i), action = c.u32(at + 8);
            const auto box = rect(at + 0x10);
            if (c.i16(at + 0x72) == int(body_index) && action == 2005 && box.top >= panel.top && box.bottom <= panel.bottom &&
                box.right > box.left && box.bottom > box.top) {
                require(box.left >= panel.left && box.right <= panel.right, "Browser link outside its columns");
                if (!row_link) row_link = i;
                ++links;
            }
            if (box.top == old.top - 40 && box.bottom == old.top - 5 && box.left == panel.left && box.right == panel.right) ++headings;
            if (box.top == old.top - 65 && box.bottom == old.top - 45 && box.left == panel.left && c.i16(at + 0x72) < 0) filter = i;
            if (action == 1 && c.u32(at + 0x48) == 4 && box.top >= panel.bottom && box.bottom <= footer_top) tab_link = i;
        }
        require(strips == (mode == 0 ? 4u : mode == 1 ? 2u : 0u) && buttons == 1 && links && headings == 1 && filter,
                "Browser layout is missing tabs, footer, heading, filter or list links");
        require(hit(row_link) == int(row_link), "Browser link missed its adaptive click bounds");
    };

    for (unsigned tab = 1; tab <= 6; ++tab) {
        build(0, tab, {800,600}, true, 0);
        build(0, tab, {1280,900}, true, tab >= 3 ? 7 : 0);
    }
    build(0, 5, {801,601}, true, 6);
    build(0, 5, {3200,1800}, true, 118);
    build(0, 5, {800,900}, true, 6);
    build(0, 4, {1280,900}, false, 7);
    build(1, 4, {1280,900}, true, 5);
    build(2, 6, {1280,900}, true, 0);

    build(0, 1, {1280,900}, true, 0);
    const auto handle = c.u32(c.u32(history + 8));
    require(handle == 0x10105bc0, "Unexpected Nations & Clubs event handler");
    require(hit(filter) == -1 && c.u8(display) == 1, "Continent popup did not open");
    unsigned choice = 0;
    for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
        if (c.u32(control(i) + 8) == 2 && c.u32(control(i) + 0x48) != continent) { choice = i; break; }
    require(choice && hit(choice) == int(choice), "Moved continent popup item missed input");
    require(std::int32_t(invoke(handle, {choice})) == -11 && c.u32(setting(1)) == c.u32(control(choice) + 0x48),
            "Original handler did not change continent");
    c.put32(setting(1), continent);
    require(tab_link && hit(tab_link) == int(tab_link), "Moved club tab missed input");
    require(std::int32_t(invoke(handle, {tab_link})) == -11 && c.u32(setting(0)) == 4, "Original club tab action did not change view");
    build(0, 4, {1280,900}, true, 0);
    const auto club = c.u32(control(row_link) + 0x48);
    require(hit(row_link) == int(row_link), "Moved club link missed input");
    invoke(handle, {row_link});
    const auto opened = c.u32(manager + 0x10);
    require(opened != history && c.u32(c.u32(opened)) == 0x1001e2b0 && c.u32(opened + 0x14) == club,
            "Original browser link did not open the selected club");
    initialize_new_screens();
}

void competition_fixtures_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    std::filesystem::create_directories(temp.path / "user" / "Files");
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
            "Competition fixtures pixel initialization");
    unsigned active_section = 0;
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000;
        try { c.call(entry, return_sentinel); }
        catch (const std::exception& error) {
            throw std::runtime_error("Competition tab " + std::to_string(active_section) + " " + hex_address(entry) +
                                     " at " + hex_address(s.pc) + ": " + error.what());
        }
        return c.r(3);
    };
    constexpr unsigned builder = 0x10065340, handler = 0x1006d500;
    const auto definition = c.u32(c.toc(-0x7bbc)) + 262 * 0x6c;
    invoke(0x100648e0, {definition, 0xfffffffeu, 0xffffffffu, 0});
    const auto display = c.u32(manager + 0x24), history = c.u32(manager + 0x10);
    const auto setting = [history](unsigned slot) { return history + 0x14 + slot * 8; };
    const auto competition = c.u32(setting(0));
    require(c.u32(c.u32(history)) == builder && competition, "Competition screen did not open");
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto control = [display](unsigned i) { return display + 0xba864 + i * 0x17c; };
    const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at), c.i32(at+4), c.i32(at+8), c.i32(at+12)}; };
    const auto find = [&](const auto& predicate) {
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
            if (predicate(control(i))) return i;
        throw std::runtime_error("Missing competition fixture control");
    };
    const auto hit = [&](unsigned index) {
        const auto box = rect(control(index) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left + box.right) / 2);
        c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
        c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
        return std::int16_t(invoke(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
    };
    const auto click = [&](unsigned index) {
        require(hit(index) == int(index), "Competition fixture control missed its hit box");
        return std::int32_t(invoke(handler, {index}));
    };
    const auto meanings = [&] {
        std::vector<std::uint8_t> bytes;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
            for (const auto [offset, size] : {std::pair{8u,8u}, {0x20u,8u}, {0x38u,20u}, {0x68u,0x114u}}) {
                const auto part = s.memory.copy_out(control(i) + offset, size);
                bytes.insert(bytes.end(), part.begin(), part.end());
            }
        return bytes;
    };
    const auto date = c.toc(-0x7b80);
    unsigned body_index = 0;
    ScreenRect panel{};
    const auto build = [&](unsigned section, GuiCanvas canvas, bool enabled, bool empty) {
        active_section = section;
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, canvas), "Competition fixtures resize");
        c.put32(setting(1), section);
        require(adaptive_screen_available(s, builder) == enabled, "Wrong competition fixtures gate");
        invoke(0x10719560, {display, 1, 0}); invoke(builder, {});
        const auto count = c.u16(display + 0x129da4);
        const auto columns = empty ? 1 : section == 4 ? 2 : 9;
        body_index = count;
        for (unsigned i = 0; i < count; ++i) {
            const auto box = rect(grid(i));
            if (box.left == 110 && box.right == 780 && box.bottom == 500 && c.u8(grid(i)+0xbb0) == columns)
                body_index = i;
        }
        require(body_index < count, "Original competition fixture list missing");
        const auto body = grid(body_index);
        const auto old = rect(body);
        const int old_rows = c.u8(body + 0xbb1), last = c.i32(body + 0x14), scroll = c.i32(body + 0x1fc);
        const auto original = s.memory.copy_out(display, 0x129da8), semantic = meanings();
        const auto saved_history = s.memory.copy_out(history, 0x1f4), rng = random_state(s);
        const auto saved_comp = s.memory.copy_out(competition, 0xec), saved_date = s.memory.copy_out(date, 8);
        const auto cache = section == 2 ? c.u32(setting(14)) : section == 3 ? c.u32(setting(23)) : 0;
        const auto cache_state = cache ? s.memory.copy_out(cache, 0x10) : std::vector<std::uint8_t>{};
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto stack_base = c.sp() - 0x8000;
        const auto stack = s.memory.copy_out(stack_base, 0x8100);
        layout_built_screen(s, display, builder);
        require(meanings() == semantic, "Competition layout changed labels, styles, actions or payloads");
        require(s.memory.copy_out(history, 0x1f4) == saved_history && s.memory.copy_out(competition, 0xec) == saved_comp &&
                s.memory.copy_out(date, 8) == saved_date && random_state(s) == rng &&
                (!cache || s.memory.copy_out(cache, 0x10) == cache_state), "Competition layout changed game or browser state");
        require(s.registers == registers && s.unique == unique && s.pc == pc &&
                s.memory.copy_out(stack_base, 0x8100) == stack, "Competition layout changed guest ABI");
        panel = rect(body);
        if (!enabled || canvas.original()) {
            require(s.memory.copy_out(display, 0x129da8) == original, "Classic competition fixture display changed");
        } else {
            const int rows = c.u8(body + 0xbb1);
            require(panel.top == old.top && panel.width() == std::min(1600, canvas.width - 130) &&
                    panel.left == 110 + (canvas.width - 130 - panel.width()) / 2 && panel.bottom <= canvas.height - 100,
                    "Competition fixture list exceeds its panel");
            require(rows >= old_rows && rows <= 30 && (empty ? rows == old_rows : canvas.height < 900 || rows > old_rows),
                    "Competition fixture rows did not adapt");
            if (!empty) {
                require(c.i32(body + 0x1fc) == std::clamp(scroll, 0, std::max(0, last+1-rows)) &&
                        bool(c.u8(body + 0xbef)) == (last+1 > rows), "Competition fixture scrollbar or scroll position");
                for (int row = old_rows; row < rows; ++row)
                    require(c.u8(body + 0xbd0 + row) == 1, "New fixture row lacks a weight");
            }
            unsigned strips = 0, date_buttons = 0, footer = 0, spans = 0;
            for (unsigned i = 0; i < count; ++i) {
                const auto at = grid(i);
                const auto box = rect(at);
                if (box == ScreenRect{100,80,canvas.width-10,115} ||
                    box == ScreenRect{100,canvas.height-90,canvas.width-10,canvas.height-55}) ++strips;
                if (box == ScreenRect{panel.left,125,panel.left+250,145}) ++date_buttons;
                if (box == ScreenRect{panel.right-340,canvas.height-45,panel.right,canvas.height-10}) {
                    ++footer;
                    require(c.u8(at+0xbb2) == 1 && c.u8(at+0xbb3) == 1, "Competition footer widths");
                }
            }
            const auto print = find([&](unsigned at) { return rect(at+0x10) == ScreenRect{canvas.width-90,15,canvas.width-15,35}; });
            require(c.i16(control(print)+0x72) < 0 && strips == 4 && footer == 1 &&
                    date_buttons == (!empty && section != 4 ? 1u : 0u), "Competition fixture navigation geometry");
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = control(i);
                if (c.i16(at + 0x72) != int(body_index) || c.i32(at + 0x2c) <= c.i32(at + 0x28)) continue;
                ++spans;
                if (empty) require(std::abs(c.i32(at+0x28)+c.i32(at+0x2c)-panel.left-panel.right) <= 2,
                                   "Empty result message lost centering");
                else require(c.i32(at+0x28) == panel.left+5, "Grouped result label lost inset");
            }
            if (empty || section == 2) require(spans > 0, "Measured competition text was not exercised");
        }
        for (unsigned i = 0; i < count; ++i) {
            invoke(0x10714630, {grid(i)}); invoke(0x10714110, {grid(i),0});
        }
    };

    build(2, {800,600}, true, true);
    build(2, {3200,1800}, true, true);
    build(3, {1280,900}, true, false);
    const auto fixture_link = find([&](unsigned at) { return c.i16(at+0x72) == int(body_index) && c.u32(at+8) == 2002; });
    const auto sample = c.u32(control(fixture_link)+0x48);
    require(sample && hit(fixture_link) == int(fixture_link), "Future fixture score link did not follow its row");
    const auto sample_bytes = s.memory.copy_out(sample, 0x50);
    const auto first_day = c.u16(sample + 0x30), year = c.u16(sample + 0x32);
    const auto next_date = find([&](unsigned at) { return c.u32(at+8) == 17 && rect(at+0x10).top >= 125; });
    const auto day = c.u32(setting(17));
    require(click(next_date) == -11 && c.u32(setting(17)) != day, "Competition calendar date action");
    build(3, {801,601}, true, false);
    build(3, {3200,1800}, true, false);
    build(3, {1280,900}, false, false);
    c.put32(setting(28), 8);
    build(4, {800,900}, true, false);
    const auto other_tab = find([&](unsigned at) { return c.u32(at+8) == 2 && c.u32(at+0x48) == 3 && rect(at+0x10).top >= 80; });
    require(click(other_tab) == -11 && c.u32(setting(1)) == 3, "Competition tab switch");
    build(4, {3200,1800}, true, false);
    const auto scheduled_date = find([&](unsigned at) { return c.i16(at+0x72) == int(body_index) && c.u32(at+8) == 20; });
    require(click(scheduled_date) == -11 && c.u32(setting(1)) == 3, "Match Dates did not open Calendar");

    // No match in the captured season has reached the results archive yet.
    // Supply a private result-list cache, using real fixture shapes and the
    // original list constructor. The original formatter emits every row.
    const auto items = invoke(0x10773520, {0x10});
    invoke(0x10738c00, {items});
    const auto match = allocate(s, 0x50, true);
    for (unsigned i = 0; i < 36; ++i) {
        s.memory.copy_in(match, sample_bytes);
        c.put32(match + 8, 0xffffffff);
        c.put16(match + 0x30, first_day + i/18);
        c.put8(match + 0x3d, 2); c.put8(match + 0x3e, 1);
        c.put8(match + 0x43, 2); c.put8(match + 0x44, 1);
        invoke(0x10738d10, {items, match, 0x50});
    }
    c.put32(setting(8), first_day);
    c.put32(setting(9), year);
    c.put32(setting(10), first_day-1); c.put32(setting(11), year);
    c.put32(setting(12), first_day+1); c.put32(setting(13), year);
    c.put32(setting(14), items);
    c.put32(setting(15), 7);
    build(2, {1280,900}, true, false);
    build(2, {3200,1800}, true, false);
    const auto grouped_label = find([&](unsigned at) {
        const auto box = rect(at+0x10);
        return c.i16(at+0x72) == int(body_index) && c.i32(at+0x28) == panel.left+5 &&
               c.u32(at+8) == 14 && box.top >= panel.top && box.bottom <= panel.bottom;
    });
    require(hit(grouped_label) == int(grouped_label), "Grouped match-day link lost its hit box");
    initialize_new_screens();
}

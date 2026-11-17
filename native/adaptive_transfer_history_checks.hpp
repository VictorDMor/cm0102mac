void transfer_history_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    const auto club = c.u32(c.u32(manager + 0x10) + 0x14);
    std::vector<unsigned> players;
    for (unsigned i = 0; i < 50; ++i) {
        const auto player = c.u32(club + 0x98 + i * 4);
        if (player && c.u32(player + 0x14)) players.push_back(player);
    }
    require(players.size() > 1, "Transfer fixture lacks players");
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
            "Transfer history pixel initialization");
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000;
        c.call(entry, return_sentinel);
        return c.r(3);
    };
    constexpr auto builder = 0x10426aa0u, handler = 0x10427df0u;
    invoke(0x10426970, {});
    const auto display = c.u32(manager + 0x24), history = c.u32(manager + 0x10);
    require(c.u32(c.u32(history)) == builder && c.u32(c.u32(history + 8)) == handler &&
            adaptive_screen_available(s, builder), "Original transfer browser did not opt in");
    const auto setting = [history](unsigned slot) { return history + 0x14 + slot * 8; };
    const auto grid = [display](unsigned index) { return display + 4 + index * 0xbf0; };
    const auto control = [display](unsigned index) { return display + 0xba864 + index * 0x17c; };
    const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at), c.i32(at+4), c.i32(at+8), c.i32(at+12)}; };
    const auto find = [&](const auto& matches) {
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
            if (matches(control(i))) return i;
        throw std::runtime_error("Missing transfer history control");
    };
    const auto hit = [&](unsigned i) {
        const auto box = rect(control(i) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left + box.right) / 2); c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
        c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
        return std::int16_t(invoke(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
    };
    const auto action_control = [&](unsigned action) {
        return find([&](unsigned at) { return c.u32(at + 8) == action && c.i16(at + 0x72) < 0; });
    };
    const auto click = [&](unsigned i) {
        require(hit(i) == int(i), "Transfer history control missed its click bounds");
        return std::int32_t(invoke(handler, {i}));
    };

    select_new_screens(false); require(!adaptive_screen_available(s, builder), "Disabled transfer history opted in");
    select_new_screens(true);
    c.put16(manager + 0x2c58, 1); require(!adaptive_screen_available(s, builder), "Secondary transfer history opted in");
    c.put16(manager + 0x2c58, 0);
    c.put32(manager + 0x2c64, 1); require(!adaptive_screen_available(s, builder), "Network transfer history opted in");
    c.put32(manager + 0x2c64, 0);

    // Supply only the original browser's transient query results. All
    // formatting, dates, page controls, row links and dispatch remain real.
    constexpr unsigned record_count = 100, record_size = 0x28;
    const auto records = allocate(s, record_count * record_size, true);
    const auto date = c.toc(-0x7b80), club_base = c.u32(c.toc(-0x7be8));
    const auto other_club = club_base + 500 * 0x248;
    for (unsigned i = 0; i < record_count; ++i) {
        const auto record = records + i * record_size, person = players[i % players.size()];
        c.put32(record, c.u32(person));
        for (unsigned offset : {4u, 8u, 12u}) {
            const auto name = c.u32(person + 0x24 + offset);
            c.put32(record + offset, name ? c.u32(name + 0x20) : 0);
        }
        c.put32(record + 0x10, c.u32(i % 2 ? club : other_club));
        c.put32(record + 0x14, c.u32(i % 2 ? other_club : club));
        c.put32(record + 0x18, 25000 * (i + 1));
        c.put8(record + 0x1c, i % 2 ? 1 : 0);
        c.put32(record + 0x20, c.u32(date));
    }
    const auto saved = s.memory.copy_out(records, record_count * record_size);
    const auto original_club = s.memory.copy_out(club, 0x248), original_date = s.memory.copy_out(date, 4);
    std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> saved_players;
    for (const auto person : players) saved_players.emplace_back(person, s.memory.copy_out(person, 0x70));
    const auto rng_cursor = c.toc(-0x7b84), initial_cursor = c.u32(rng_cursor);
    std::map<unsigned, std::vector<std::uint8_t>> classic_rng;

    ScreenRect list{};
    unsigned body_index = 0;
    std::vector<std::vector<std::uint8_t>> row_meanings;
    const auto build = [&](GuiCanvas canvas, unsigned count, unsigned page) {
        require(s.platform->resize_gui_canvas(s, canvas), "Transfer history resize");
        c.put32(setting(4), records); c.put32(setting(5), count); c.put32(setting(6), page);
        invoke(0x10719560, {display, 1, 0});
        // Background selection consumes the original game's RNG. Compare
        // fresh builds from the same cursor and empty background name.
        c.put32(rng_cursor, initial_cursor);
        c.put8(history + 0x1fc, 0);
        invoke(builder, {});
        const auto grids = c.u16(display + 0x129da4);
        require(grids >= 2, "Transfer history list/footer missing");
        body_index = count ? c.u32(setting(9)) : grids - 2;
        require(body_index == unsigned(grids - 2), "Incorrect transfer history body ownership");
        const auto body = grid(body_index);
        list = rect(body);
        const int width = std::min(1440, canvas.width - 130);
        const int left = 110 + (canvas.width - 130 - width) / 2;
        const bool adaptive = new_screens_enabled() && !canvas.original();
        const auto expected_width = adaptive ? width : 670;
        const auto expected_left = adaptive ? left : 110;
        const auto rows = c.u8(body + 0xbb1);
        require(list.top == 170 && list.left == expected_left && list.width() == expected_width && list.bottom <= canvas.height - 65 &&
                rows > 0 && (adaptive ? rows <= 30 : rows == (count ? 19 : 16)),
                "Transfer list geometry/row capacity");
        require(c.u8(body + 0xbb0) == (count ? 5 : 1), "Transfer history column count");
        const auto buttons = grid(grids - 1);
        if (adaptive) {
            require(rect(buttons) == ScreenRect{list.right-340,list.bottom+20,list.right,list.bottom+55} &&
                    c.u8(buttons + 0xbb2) == 1 && c.u8(buttons + 0xbb3) == 1, "Transfer history footer alignment");
            const auto filter = find([&](unsigned at) {
                const auto box = rect(at + 0x10);
                return c.i16(at + 0x72) < 0 && c.u32(at + 8) == 0 && box.top == 145 && box.width() >= 125;
            });
            require(rect(control(filter) + 0x10).left == list.left &&
                    rect(control(action_control(3)) + 0x10).right == list.right + 5, "Transfer history controls did not follow the list");
        }
        for (unsigned i = 0; i < grids; ++i) {
            invoke(0x10714630, {grid(i)}); invoke(0x10714110, {grid(i), 0});
        }
        row_meanings.clear();
        unsigned local_row = 0, player_links = 0;
        const unsigned pages = count ? (count + rows - 1) / rows : 1;
        const unsigned selected_page = count && adaptive ? std::clamp(page, 1u, pages) : page;
        require(c.u32(setting(6)) == selected_page && c.u32(setting(7)) == pages, "Transfer page count or resize clamp");
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = control(i);
            if (c.i16(at + 0x72) != int(body_index)) continue;
            const auto action = c.u32(at + 8);
            auto meaning = s.memory.copy_out(at + 8, 8);
            for (const auto [offset, size] : {std::pair{0x48u,4u}, {0x6au,8u}, {0x78u,200u}}) {
                const auto part = s.memory.copy_out(at + offset, size);
                meaning.insert(meaning.end(), part.begin(), part.end());
            }
            row_meanings.push_back(std::move(meaning));
            if (count && action == 2007) {
                const auto index = (selected_page - 1) * rows + local_row++;
                require(index < count && c.u32(at + 0x48) == players[index % players.size()], "Transfer moved or lost a record");
                ++player_links;
            }
        }
        require(count ? player_links == std::min<unsigned>(rows, count - (selected_page - 1) * rows) :
                        row_meanings.size() == 1, "Wrong number of transfer rows or empty messages");
        require(s.memory.copy_out(records, saved.size()) == saved, "Transfer browser changed its cached records");
        require(s.memory.copy_out(club, 0x248) == original_club, "Transfer browser changed the club record");
        require(s.memory.copy_out(date, 4) == original_date, "Transfer browser changed the current date");
        const auto after_rng = random_state(s);
        const auto nation = c.u32(setting(0));
        if (!adaptive) classic_rng.try_emplace(nation, after_rng);
        require(after_rng == classic_rng.at(nation), "Viewport changed the transfer browser's original RNG effects");
        for (const auto& [person, bytes] : saved_players)
            require(s.memory.copy_out(person, 0x70) == bytes, "Transfer view rewrote player records");
        return rows;
    };
    const auto legacy_rows = build({800,600}, record_count, 1);
    const auto original_rows = row_meanings;
    select_new_screens(false);
    build({800,600}, record_count, 1);
    require(row_meanings == original_rows, "New Screens changed classic transfer rows");
    select_new_screens(true);
    for (const auto canvas : {GuiCanvas{801,601}, GuiCanvas{960,720}, GuiCanvas{1280,900}, GuiCanvas{3200,1800}, GuiCanvas{800,900}}) {
        const auto rows = build(canvas, record_count, 1);
        require(row_meanings.size() >= original_rows.size() &&
                std::equal(original_rows.begin(), original_rows.end(), row_meanings.begin()),
                "Expanded browser changed an existing record's text, colors or actions");
        require(canvas.height < 720 || rows > legacy_rows, "Taller viewport did not fit more transfers");
    }
    build({3200,1800}, record_count, 6);
    require(c.u32(setting(6)) == 4 && c.u32(setting(7)) == 4, "Resize stranded the former last page");
    require(click(action_control(6)) == -11 && c.u32(setting(6)) == 3, "Adaptive previous-page action");
    build({3200,1800}, record_count, 3);
    require(click(action_control(7)) == -11 && c.u32(setting(6)) == 4, "Adaptive next-page action");
    build({960,720}, 1, 1);
    const auto person = find([&](unsigned at) { return c.u32(at + 8) == 2007 && c.i16(at + 0x72) == int(body_index); });
    require(hit(person) == int(person), "Adaptive transfer player link");
    const auto team = find([&](unsigned at) { return c.u32(at + 8) == 2005 && c.i16(at + 0x72) == int(body_index); });
    require(hit(team) == int(team), "Adaptive transfer club link");

    build({3200,1800}, 0, 1);
    require(c.u8(grid(body_index) + 0xbb1) == 30, "Empty browser did not adapt");
    // Unpublish the fixture cache before exercising handlers that free the
    // original owned cache. The click still uses the live generated menu.
    c.put32(setting(4), 0);
    const auto filter = find([&](unsigned at) {
        const auto box = rect(at + 0x10);
        return box.top == 145 && box.left == list.left && box.width() >= 125;
    });
    require(hit(filter) == -1 && c.u8(display) == 1, "Transfer nation popup did not open");
    const auto nation = find([&](unsigned at) { return c.u32(at + 8) == 4 && c.i16(at + 0x72) >= 0; });
    require(click(nation) == -11 && c.u32(setting(0)) == c.u32(control(nation) + 0x48), "Transfer nation change");
    build({800,600}, 0, 1);
    build({1280,900}, 0, 1);
    c.put32(setting(4), 0);
    const auto view = find([&](unsigned at) {
        const auto box = rect(at + 0x10);
        return box.top == 85 && box.left == list.left && c.u32(at + 8) == 0;
    });
    require(hit(view) == -1 && c.u8(display) == 1, "Transfer period popup did not open");
    const auto annual = find([&](unsigned at) { return c.u32(at + 8) == 1 && c.u32(at + 0x48) == 0; });
    require(click(annual) == -11 && c.u32(setting(3)) == 0, "Transfer annual view action");
    build({1280,900}, 0, 1);
    c.put32(setting(4), 0);
    const auto year = c.u32(setting(2));
    require(click(action_control(2)) == -11 && c.u32(setting(2)) == year - 1, "Transfer date navigation");
    initialize_new_screens();
}

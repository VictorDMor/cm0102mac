// Drive the original competition opener, round builder and stage menu on a
// disposable capture. The European playoff contains completed two-leg ties.
void knockout_rounds_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned builder = 0x10065340, handler = 0x1006d500;
    for (unsigned id : {68u, 395u, 409u}) {
        Clock clock; clock.idle_input = true;
        auto state = std::make_unique<State>();
        auto& s = *state;
        load_match_snapshot(s, path);
        s.code_fragment_observer = &clock;
        guest::Cpu c(s);
        c.set(2, c.u32(c.sp() + 0x14));
        const auto manager = c.toc(-0x7bb0);
        c.put16(manager + 0x2c58, 0);
        s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true);
        world(s, scratch, scratch + 16);
        const auto call = [&](unsigned address, std::initializer_list<unsigned> args) {
            unsigned r = 3;
            for (auto arg : args) c.set(r++, arg);
            s.budget = s.executed + 400000000;
            c.call(address, return_sentinel);
            return c.r(3);
        };
        require(s.platform->resize_gui_canvas(s, {801,601}), "Initialize knockout canvas");
        call(0x100648e0, {c.u32(c.toc(-0x7bbc)) + id * 0x6c, 0xfffffffeu, 0xfffffffeu, 0});
        const auto history = c.u32(manager + 0x10);
        const auto display = c.u32(manager + 0x24);
        const GuestScreen screen{c, display};
        const auto setting = [history](unsigned slot) { return history + 0x14 + slot * 8; };
        const auto root = c.u32(setting(0));
        require(c.u32(c.u32(history)) == builder && c.u32(setting(1)) == 1, "Original cup screen opener");
        const auto hit = [&](unsigned index) {
            const auto box = screen.rect(screen.control(index) + 0x10);
            c.put32(c.toc(-0x5c10), (box.left + box.right) / 2);
            c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
            c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
            return std::int16_t(call(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
        };
        const auto meanings = [&] {
            std::vector<std::uint8_t> bytes;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                for (const auto [off, len] : {std::pair{8u,8u}, {0x20u,8u}, {0x38u,20u}, {0x68u,0x114u}}) {
                    const auto data = s.memory.copy_out(screen.control(i) + off, len);
                    bytes.insert(bytes.end(), data.begin(), data.end());
                }
            }
            return bytes;
        };
        for (const auto [size, enabled] : {
                std::pair{GuiCanvas{800,600},true}, {{1280,900},false}, {{800,900},true},
                {{801,601},true}, {{1280,900},true}, {{3200,1800},true}, {{1600,1000},true}}) {
            select_new_screens(enabled);
            require(s.platform->resize_gui_canvas(s, size), "Resize cup canvas");
            // Choose the forthcoming final of the friendly cup to expose the
            // live draw controls without advancing a day or drawing a team.
            c.put32(setting(3), id == 409 ? 1u : 0xfffffffeu);
            const bool overflow = id == 395 && size.width == 1600;
            if (overflow) {
                // Repeat the four captured playoff ties in a private round to
                // exercise an overflowing list and links beyond the classic page.
                c.put32(setting(2), 9);
                const auto playoff = c.u32(c.u32(root + 0xc) + 9 * 4);
                const auto matches = c.u32(playoff + 0xac);
                const auto original_four = s.memory.copy_out(c.u32(matches), 4 * 0x36);
                const auto expanded = allocate(s, 40 * 0x36, true);
                for (unsigned copy = 0; copy < 10; ++copy)
                    s.memory.copy_in(expanded + copy * original_four.size(), original_four);
                c.put32(matches, expanded);
                c.put16(c.u32(playoff + 0xa0) + 0x1c, 40);
                c.put32(setting(6), 23);
            }
            call(0x10719560, {display, 1, 0});
            call(builder, {});
            require(adaptive_screen_available(s, builder) == enabled, "Knockout stage eligibility");
            const auto selected = c.i32(setting(2));
            const auto stage = selected == -1 ? root : c.u32(c.u32(root + 0xc) + 4 * selected);
            require(c.u8(stage + 0x43) == 1, "Expected original knockout stage");
            const bool pending = id == 68;
            int body = -1;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                const auto grid = screen.grid(i);
                if (screen.rect(grid).left == 110 && screen.rect(grid).bottom == 500 &&
                    screen.columns(grid) == (pending ? 1 : 9)) body = int(i);
            }
            require(body >= 0, "Knockout round grid missing");
            const auto grid = screen.grid(body);
            const auto old = screen.rect(grid);
            const auto rows = screen.rows(grid);
            const auto original_scroll = c.i32(grid + 0x1fc);
            const auto original = s.memory.copy_out(display, 0x129da8);
            const auto rng = random_state(s), semantic = meanings();
            const auto registers = s.registers;
            const auto saved_stack = s.memory.copy_out(c.sp() - 0x8000, 0x8100);
            const auto unique = s.unique;
            const auto pc = s.pc;
            const auto stage_data = s.memory.copy_out(stage, 0xb4), navigation = s.memory.copy_out(history, 0x1f4);
            const auto round = pending ? 0u : c.u32(setting(3));
            const auto record = c.u32(stage + 0xa0) + 0x78 * round;
            const auto ties = c.u32(c.u32(stage + 0xac) + 4 * round);
            const auto pairings = s.memory.copy_out(ties, c.u16(record + 0x1c) * 0x36);
            layout_built_screen(s, display, builder);
            require(random_state(s) == rng && meanings() == semantic && s.registers == registers && s.unique == unique &&
                    s.pc == pc && s.memory.copy_out(c.sp() - 0x8000, 0x8100) == saved_stack, "Knockout UI changed commands, RNG or ABI");
            require(s.memory.copy_out(stage, stage_data.size()) == stage_data &&
                    s.memory.copy_out(ties, pairings.size()) == pairings &&
                    s.memory.copy_out(history, navigation.size()) == navigation, "Knockout layout changed pairings or navigation");
            if (size.original() || !enabled) {
                require(s.memory.copy_out(display, original.size()) == original, "Classic knockout display changed");
                continue;
            }
            const auto panel = screen.rect(grid);
            require(panel.width() == std::min(size.width - 130, 1600) && panel.top == old.top &&
                    panel.bottom <= size.height - 100, "Knockout list bounds");
            require(screen.rows(grid) >= rows && screen.rows(grid) <= 30 &&
                    (pending ? screen.rows(grid) == rows : size.height < 900 || screen.rows(grid) > rows),
                    "Knockout row capacity");
            // DISPLAY refreshes every area after the builder, including popup
            // lists whose own bounds did not change during the layout pass.
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                call(0x10714630, {screen.grid(i)});
                call(0x10714110, {screen.grid(i), 0});
            }
            unsigned team_links = 0, score_links = 0, draw_commands = 0, centered = 0, leg_scores = 0;
            unsigned stage_menu = 0, group_option = 0;
            bool expanded_link = false;
            if (overflow)
                require(screen.rows(grid) == 30 && c.u8(grid + 0xbef) &&
                        c.i32(grid + 0x1fc) == std::clamp(original_scroll, 0, 10), "Knockout overflow scroll");

            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = screen.control(i);
                const auto box = screen.rect(at + 0x10);
                const auto action = c.u32(at + 8);
                if (c.i16(at + 0x72) == body) {
                    if (pending && c.i32(at + 0x2c) > c.i32(at + 0x28)) {
                        require(std::abs(box.left + box.right - panel.left - panel.right) <= 2, "Draw date centering");
                        ++centered;
                    }
                    const auto row = c.i32(at + 0x24) - c.i32(grid + 0x1fc);
                    if (row < 0 || row >= screen.rows(grid)) continue;
                    if (action == 2003 || action == 2005) {
                        require(hit(i) == int(i), "Cup team or score missed its hit box");
                        if (action == 2005) ++team_links;
                        else ++score_links;
                        if (row >= rows) expanded_link = true;
                    }
                    const auto col = c.u32(at + 0x20);
                    if ((col == 1 || col == 7) && c.u8(at + 0x78)) ++leg_scores;
                } else if (action >= 9 && action <= 11 && box.bottom + 5 == panel.top) {
                    require(hit(i) == int(i), "Draw button missed its hit box");
                    if (action == 9) require(box.left == panel.left, "Draw toolbar lost alignment");
                    ++draw_commands;
                }
                if (box == ScreenRect{panel.right-125,125,panel.right,145} && c.i16(at + 0x74) >= 0) stage_menu = i;
                if (action == 5 && c.u32(at + 0x48) == 0) group_option = i;
            }
            require(!pending || centered == 1, "Draw date not exercised");
            require(pending || team_links >= 2, "Knockout team links not exercised");
            require(id != 409 || draw_commands == 3, "Live draw controls not exercised");
            if (id == 395) {
                const unsigned ties_shown = overflow ? 30u : 4u;
                require(score_links == ties_shown && leg_scores == 2 * ties_shown, "Two-leg scores not exercised");
                require(!overflow || expanded_link, "Expanded knockout rows not clickable");
            }
            if (id == 395 && size.width == 3200) {
                require(stage_menu && group_option, "Missing original playoff stage menu or group option");
                require(hit(stage_menu) == -1 && c.u8(display) == 1, "Stage menu missed its hit box");
                require(hit(group_option) == int(group_option), "Group option missed its popup hit box");
                require(std::int32_t(call(handler, {group_option})) == -11 && c.i32(setting(2)) == -1,
                        "Original menu did not leave the playoff for the group stage");
            }
        }
    }
    initialize_new_screens();
}

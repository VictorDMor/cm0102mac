enum class TrainingPage { roster, coaches, schedule, staff_profile };
void training_builder(const char* path, TrainingPage page = TrainingPage::roster) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path);
    s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3;
        for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000;
        c.call(entry, return_sentinel);
        return std::int32_t(c.r(3));
    };
    require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}), "Training pixels");
    invoke(0x103d6cc0, {c.u32(c.u32(manager + 0x10) + 0x14)});
    const auto history = c.u32(manager + 0x10), engine = c.u32(history + 0x14);
    const GuestScreen screen{c, c.u32(manager + 0x24)};
    const auto display = screen.display;
    constexpr auto builder = 0x103d6db0u, handler = 0x103db0d0u;
    require(c.u32(c.u32(history)) == builder && c.u32(c.u32(history + 8)) == handler, "Original training screen");
    const auto build = [&](GuiCanvas canvas, bool enabled) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, canvas), "Training canvas resize");
        invoke(0x10719560, {display, 1, 0});
        invoke(builder, {});
        const auto original = s.memory.copy_out(display, 0x129da8);
        const auto settings = s.memory.copy_out(engine, 0x861), saved_history = s.memory.copy_out(history, 0x1f4);
        const auto rng = random_state(s);
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto stack = s.memory.copy_out(c.sp() - 0x8000, 0x8100);
        require(adaptive_screen_available(s, builder) == enabled, "Training preference");
        layout_built_screen(s, display, builder);
        require(settings == s.memory.copy_out(engine, 0x861) && rng == random_state(s) &&
                saved_history == s.memory.copy_out(history, 0x1f4), "Training layout mutated schedules, history or RNG");
        require(registers == s.registers && unique == s.unique && pc == s.pc &&
                stack == s.memory.copy_out(c.sp() - 0x8000, 0x8100), "Training layout mutated guest ABI");
        if (!enabled || canvas.original())
            require(original == s.memory.copy_out(display, 0x129da8), "Classic training display changed");
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            screen.measure(screen.grid(i));
            screen.publish(screen.grid(i));
        }
        return screen.grid(c.u32(history + 0x14 + 5 * 8));
    };
    const auto find = [&](unsigned action, unsigned payload) {
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = screen.control(i);
            if (c.u32(at + 8) == action && c.u32(at + 0x48) == payload) return i;
        }
        throw std::runtime_error("Missing training control");
    };
    const auto point_at = [&](unsigned i, bool pressed) {
        const auto box = screen.rect(screen.control(i) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left + box.right) / 2);
        c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
        c.put8(c.toc(-0x7bac), pressed); c.put8(c.toc(-0x5c0c), 0);
    };
    const auto hit = [&](unsigned i) {
        point_at(i, true);
        return std::int16_t(invoke(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
    };

    // These focused checks call the screen handlers outside the monitor loop.
    // Consume their -9 result as the monitor does before opening another dialog.
    const auto reopen = [&](unsigned action) {
        const auto node = c.u32(manager + 0x10);
        require(c.u8(node + 0x10) && c.u32(node + 0x1f4) == history, "Training modal history");
        c.put32(manager + 0x10, history);
        c.put32(manager + 0x0c, history);
        c.put32(history + 0x1f8, 0);
        c.put16(manager + 0x14, c.i16(manager + 0x14) - 1);
        c.put16(manager + 0x16, c.i16(manager + 0x16) - 1);
        for (unsigned i = 0; i < 60; ++i) {
            const auto slot = node + 0x14 + i * 8;
            if (c.u32(slot) && c.u8(slot + 4)) invoke(0x10773610, {c.u32(slot)});
        }
        invoke(0x107735a0, {node});
        c.put8(manager + 0x2c5f, 1);
        require(invoke(handler, {0xffffffffu}) == 0, "Training parent return callback");
        c.put8(manager + 0x2c5f, 0);
        build({1280,900}, true);
        const auto button = find(action,0);
        require(hit(button) == int(button) && invoke(handler, {button}) == -4, "Reopen training dialog from parent");
    };
    if (page == TrainingPage::schedule) {
        build({1280,900}, true);
        const auto general = find(1,2);
        require(hit(general) == int(general) && invoke(handler, {general}) == -11, "Select Geral from Training");
        build({1280,900}, true);
        const auto edit = find(5,0);
        require(hit(edit) == int(edit) && invoke(handler, {edit}) == -4, "Open Edit from training schedule");
        check_training_schedule(s, screen, engine, invoke, find, hit, reopen);
        initialize_new_screens();
        return;
    }

    if (page == TrainingPage::coaches || page == TrainingPage::staff_profile) {
        constexpr unsigned coaches_builder = 0x103ddd10, coaches_handler = 0x103de370;
        build({1280,900}, true);
        const auto coaches_button = find(4,0);
        require(hit(coaches_button) == int(coaches_button) && invoke(handler, {coaches_button}) == -4,
                "Open coaching assignments from Training");
        const auto original_assignments = s.memory.copy_out(engine + 0x818, 0x48);
        const auto draft = [&] {
            const auto page = c.u32(manager + 0x10);
            require(c.u32(c.u32(page)) == coaches_builder && c.u32(c.u32(page + 8)) == coaches_handler &&
                    c.u32(page + 0x14) == engine, "Unexpected coaching assignment screen");
            return c.u32(page + 0x1c);
        };
        const auto draw = [&](GuiCanvas canvas, bool enabled) {
            select_new_screens(enabled);
            require(s.platform->resize_gui_canvas(s, canvas), "Coaches canvas resize");
            invoke(0x10719560, {display, 1, 0});
            invoke(coaches_builder, {});
            const auto before = s.memory.copy_out(display, 0x129da8);
            const auto working = s.memory.copy_out(draft(), 0x48), schedules = s.memory.copy_out(engine, 0x86c);
            const auto rng = random_state(s); const auto stack = s.memory.copy_out(c.sp() - 0x8000, 0x8100);
            const auto regs = s.registers; const auto unique = s.unique; const auto pc = s.pc;
            require(adaptive_screen_available(s, coaches_builder) == enabled, "Coaches layout preference");
            layout_built_screen(s, display, coaches_builder);
            require(regs == s.registers && s.unique == unique && s.pc == pc &&
                    stack == s.memory.copy_out(c.sp() - 0x8000, 0x8100), "Coaches layout changed the guest ABI");
            require(rng == random_state(s) && working == s.memory.copy_out(draft(), 0x48) &&
                    schedules == s.memory.copy_out(engine, 0x86c), "Coaches layout changed assignments or RNG");
            if (!enabled || canvas.original()) {
                require(s.memory.copy_out(display, 0x129da8) == before, "Classic coaches display changed");
                return;
            }
            const auto matrix = screen.rect(screen.grid(3)), panel = screen.rect(screen.grid(1));
            const auto footer = screen.rect(screen.grid(2));
            require(screen.rect(screen.grid(0)) == ScreenRect{0,0,canvas.width-1,canvas.height-1} &&
                    panel.left + panel.right == canvas.width && panel.top + panel.bottom == canvas.height &&
                    matrix.left > panel.left && matrix.right < panel.right && footer.top > matrix.bottom,
                    "Coaches panel alignment");
            const auto first_assignment = find(3,0);
            const auto cell = screen.rect(screen.control(first_assignment) + 0x10);
            const auto name = screen.rect(screen.control(first_assignment - 1) + 0x10);
            require(name.width() > 300 && cell.left > name.right && cell.top == name.top,
                    "Coach name and assignment columns");
            // The original builder places the decorative pitch in explicit
            // control rectangles, independent of the assignment grid.
            const auto pitch = screen.rect(screen.control(3) + 0x10);
            require(pitch.left < matrix.left && pitch.top < matrix.top &&
                    pitch.right > matrix.right && pitch.bottom > matrix.bottom, "Coaches pitch alignment");
        };
        draw({800,600}, true);
        draw({1280,900}, false);
        if (page == TrainingPage::staff_profile) {
            check_staff_profiles(s, screen, engine, invoke, hit, draw);
            initialize_new_screens();
            return;
        }
        const auto click = [&](unsigned action, unsigned payload) {
            const auto control = find(action,payload);
            require(hit(control) == int(control), "Coaches input missed the displayed control");
            return invoke(coaches_handler, {control});
        };
        // The far-right assignment of the first coach also exercises clicks
        // beyond the original canvas. Original actions 1 and 2 free the draft.
        constexpr int assignment = 4 + 4;
        auto changed = original_assignments;
        changed[assignment] = !changed[assignment];
        for (const bool confirm : {false, true}) {
            draw({1280,900}, true);
            require(s.memory.copy_out(draft(), 0x48) == original_assignments, "Reopened coach assignment mismatch");
            require(click(3,4) == -11 && changed == s.memory.copy_out(draft(), 0x48), "Coach assignment toggle");
            draw({1280,900}, true);
            require(original_assignments == s.memory.copy_out(engine + 0x818, 0x48), "Premature coach assignment commit");
            require(click(confirm ? 2 : 1,0) == -9, "Coaches close action");
            const auto& expected = confirm ? changed : original_assignments;
            require(expected == s.memory.copy_out(engine + 0x818, 0x48), "Coaches confirm or cancel failed");
            reopen(4);
            require(expected == s.memory.copy_out(draft(), 0x48), "Reopening lost coach assignments");
        }
        draw({1280,900}, true);
        initialize_new_screens();
        return;
    }

    auto body = build({800,600}, true);
    const int classic_rows = screen.rows(body);
    require(classic_rows == 17 && c.i32(body + 0x14) == 39, "Cruzeiro training fixture");
    build({1280,900}, false);
    body = build({801,601}, true);
    require(screen.rows(body) == classic_rows, "Training row height at minimum adaptive size");
    // The schedule stores the top player row across redraws. The old last
    // page must still show the end of the squad after the list grows.
    c.put32(engine + 0x1a8 + 8, 23);
    body = build({1280,900}, true);
    require(c.i32(body + 0x1fc) == 10, "Expanded training scroll not clamped to the last page");
    c.put32(engine + 0x1a8 + 8, 0);
    build({1280,900}, true);
    require(invoke(handler, {find(3,4)}) == -11, "Training alternate view selection");
    body = build({1280,900}, true);
    require(screen.rows(body) == 30 && screen.rect(body).right == 1260, "Training alternate player view");
    require(invoke(handler, {find(3,1)}) == -11, "Return to general player view");
    body = build({1280,900}, true);
    const auto panel = screen.rect(body);
    const int rows = screen.rows(body);
    require(rows == 30 && panel.width() == 1150 && panel.bottom < 810, "Widescreen training list");
    const auto header = body - GuestScreen::grid_size;
    require(screen.rect(header).right == panel.right - 22, "Training header scroll gutter");
    unsigned player = 0;
    for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
        const auto at = screen.control(i);
        if (c.u32(at + 8) == 12 && c.u32(at + 0x24) == unsigned(classic_rows)) { player = i; break; }
    }

    const auto target = find(1, 5), staff = c.u32(screen.control(player) + 0x48);
    require(player && staff && hit(target) == int(target), "Moved training tab missed input");
    // Press a newly visible row, drag to the lower strip and release.
    const auto player_box = screen.rect(screen.control(player) + 0x10);
    require(player_box.top >= panel.top && player_box.bottom <= panel.bottom && player_box.width() > 200,
            "Expanded player row or name column is clipped");
    require(hit(player) == -1 && c.u8(display) == 3, "Player press did not start a drag");
    point_at(target, true);
    require(invoke(0x1071afc0, {display}) == -1, "Training drag move");
    point_at(target, false);
    require(invoke(0x1071afc0, {display}) == int(player) && c.i16(screen.control(player) + 0x76) == int(target),
            "Training drop missed its destination");
    require(invoke(handler, {player}) == -11, "Training drop did not request a redraw");
    build({1280,900}, true);
    require(invoke(handler, {find(1,5)}) == -11 && c.u8(engine + 0x860) == 5, "Schedule selection failed");
    body = build({1280,900}, true);
    require(c.i32(body + 0x14) == 0 && find(12,staff) && !c.u8(body + 0xbef) &&
            screen.rect(body - GuestScreen::grid_size).right == screen.rect(body).right, "Player missing in destination schedule");
    require(invoke(handler, {find(1,3)}) == -11, "Empty schedule selection failed");
    body = build({1280,900}, true);
    require(screen.rect(body) == ScreenRect{panel.left,150,panel.right,170} &&
            hit(find(1,2)) == int(find(1,2)), "Empty schedule lost navigation");
    initialize_new_screens();
}

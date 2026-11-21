// The caller opens the editor via the original Geral tab and Edit button.
void check_training_schedule(State& s, const GuestScreen& screen, unsigned engine,
                             const auto& invoke, const auto& find, const auto& hit, const auto& reopen) {
    constexpr unsigned builder = 0x103dc920, handler = 0x103dd6e0;
    auto& c = screen.c;
    const auto display = screen.display, manager = c.toc(-0x7bb0);
    const auto draft = [&] {
        const auto page = c.u32(manager + 0x10);
        require(c.u32(c.u32(page)) == builder && c.u32(c.u32(page + 8)) == handler &&
                c.u32(page + 0x14) == engine, "Unexpected training schedule editor");
        return c.u32(page + 0x1c);
    };
    const auto draw = [&](GuiCanvas canvas, bool enabled) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, canvas), "Training editor resize");
        invoke(0x10719560, {display, 1, 0});
        invoke(builder, {});
        const auto before = s.memory.copy_out(display, 0x129da8);
        const auto working = s.memory.copy_out(draft(), 0x98), records = s.memory.copy_out(engine, 0x86c);
        const auto rng = random_state(s);
        const auto regs = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto stack = s.memory.copy_out(c.sp() - 0x8000, 0x8100);
        require(adaptive_screen_available(s, builder) == enabled, "Training editor preference");
        layout_built_screen(s, display, builder);
        require(regs == s.registers && unique == s.unique && pc == s.pc &&
                stack == s.memory.copy_out(c.sp() - 0x8000, 0x8100), "Training editor layout changed guest ABI");
        require(rng == random_state(s) && working == s.memory.copy_out(draft(), 0x98) &&
                records == s.memory.copy_out(engine, 0x86c), "Training editor layout changed schedules or RNG");
        if (!enabled || canvas.original()) {
            require(before == s.memory.copy_out(display, 0x129da8), "Classic training editor changed");
            return;
        }
        // DISPLAY publishes child-menu links when it lays out the popup
        // areas during its usual refresh, after the adaptive geometry pass.
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            screen.measure(screen.grid(i));
            screen.publish(screen.grid(i));
        }
        const auto matrix = screen.rect(screen.grid(4)), panel = screen.rect(screen.grid(1));
        const auto name = screen.rect(screen.control(find(9,0)) + 0x10);
        require(panel.left + panel.right >= canvas.width - 1 && panel.left + panel.right <= canvas.width + 1 &&
                matrix == screen.rect(screen.grid(3)) && matrix == screen.rect(screen.grid(5)) &&
                matrix.bottom < screen.rect(screen.grid(2)).top && name.width() > 300, "Training editor field alignment");
        // All five rows share the same four button columns, including the
        // currently selected intensity (which is a label rather than a button).
        std::array<ScreenRect, 4> columns{};
        unsigned cells = 0;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = screen.control(i);
            const unsigned col = c.u32(at + 0x20), row = c.u32(at + 0x24);
            if (c.i16(at + 0x72) != 4 || col == 0) continue;
            require(col <= 4 && row >= 1 && row <= 5, "Unexpected intensity cell");
            const auto box = screen.rect(at + 0x10);
            if (row == 1) columns[col - 1] = box;
            require(box.left == columns[col - 1].left && box.right == columns[col - 1].right &&
                    box.top > name.bottom && box.bottom <= matrix.bottom && box.width() > 90,
                    "Training editor intensity hit areas");
            ++cells;
        }
        require(cells == 20, "Missing intensity cells");
    };
    const auto click = [&](unsigned action, unsigned payload) {
        const auto id = find(action, payload);
        require(hit(id) == int(id), "Training editor click missed control");
        return invoke(handler, {id});
    };

    require(c.u8(engine + 0x860) == 2, "Expected Geral training schedule");
    const auto saved = engine + 0x190 + 2 * 0x98;
    const auto original = s.memory.copy_out(saved + 4, 5);
    auto intense = original;
    intense[2] = 50;
    require(original[2] == 25, "Expected medium shooting intensity in capture");
    draw({800,600}, true);
    draw({1280,900}, false);
    draw({801,601}, true);
    for (const bool confirm : {false, true}) {
        draw({1280,900}, true);
        require(click(6,2) == -11 && intense == s.memory.copy_out(draft() + 4, 5),
                "Intensive shooting changed another area");
        draw({1280,900}, true);
        require(original == s.memory.copy_out(saved + 4, 5), "Premature schedule update");
        require(click(confirm ? 1 : 2,0) == -9, "Training editor close action");
        const auto& expected = confirm ? intense : original;
        require(expected == s.memory.copy_out(saved + 4, 5), "Training editor confirm or cancel failed");
        reopen(5);
        require(expected == s.memory.copy_out(draft() + 4, 5), "Reopened schedule has different intensities");
    }

    const auto original_position = c.u16(saved + 0x16);
    draw({1280,900}, true);
    require(click(7,1) == -11, "Enable specific position");
    draw({1280,900}, true);
    require(click(8,0x80) == -11, "Enable specific side");
    draw({1280,900}, true);
    for (const auto [row, action, choice] : {std::tuple{6u,7u,4u}, std::tuple{7u,8u,0x800u}}) {
        unsigned arrow = 0;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = screen.control(i);
            if (c.i16(at + 0x72) == 5 && c.u32(at + 0x20) == 4 && c.u32(at + 0x24) == row) arrow = i;
        }
        require(arrow && hit(arrow) == -1 && c.u8(display) == 1, "Training popup missed its opener");
        require(click(action,choice) == -11 && (c.u16(draft() + 0x16) & choice) == choice,
                "Training popup selection did not reach the draft");
        draw({1280,900}, true);
    }
    require(click(2,0) == -9 && c.u16(saved + 0x16) == original_position, "Cancel changed position training");
}

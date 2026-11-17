// The career sidebar and processing strip share the committed framebuffer.
// Exercise their real builders without opening a window or advancing a day.
void chrome_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    struct Imports final : CodeFragmentBoundaryObserver {
        Clock clock;
        std::vector<std::pair<ScreenRect, ScreenRect>> blits;
        Imports() { clock.idle_input = true; }
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State&, std::uint32_t, bool) override {}
        bool import_fixture(State& state, const char* name) override {
            if (std::string_view(name) == "CarbonLib::CopyBits") {
                guest::Cpu c(state);
                const auto rectangle = [&](unsigned address) {
                    return ScreenRect{c.i16(address + 2), c.i16(address),
                                      c.i16(address + 6), c.i16(address + 4)};
                };
                blits.emplace_back(rectangle(c.r(5)), rectangle(c.r(6)));
            }
            return clock.import_fixture(state, name);
        }
    } imports;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &imports;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    require(c.r(2) == 0x108bba90, "Chrome capture has no game TOC");
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
    require(history && display && !c.u32(manager + 0x2c64), "Chrome fixture needs a local career screen");
    const auto club = c.u32(history + 0x14), person = c.u32(manager + 4);
    require(club && person, "Chrome fixture needs the captured club and manager");
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
        const auto stack = c.sp(), toc = c.r(2);
        std::array<unsigned, 19> saved{};
        for (unsigned i = 0; i < saved.size(); ++i) saved[i] = c.r(i + 13);
        s.budget = s.executed + 400000000;
        try { c.call(entry, return_sentinel); }
        catch (const std::exception& e) {
            throw std::runtime_error("Chrome " + hex_address(entry) + " at " + hex_address(s.pc) + ": " + e.what());
        }
        require(s.pc == return_sentinel && c.sp() == stack && c.r(2) == toc, "Chrome call changed stack/TOC or return boundary");
        for (unsigned i = 0; i < saved.size(); ++i)
            require(c.r(i + 13) == saved[i], "Chrome call clobbered a nonvolatile register");
        return c.r(3);
    };
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user",
                                                AudioMode::offline, EventMode::isolated);
    const auto scratch = allocate(s, 4096, true);
    require(scratch != 0, "Chrome scratch allocation failed");
    world(s, scratch, scratch + 16);
    // NewGWorld leaves drawing globals to its caller. A real resize installs
    // those globals before returning to the original 800x600 baseline.
    require(s.platform->resize_gui_canvas(s, {801, 601}) && s.platform->resize_gui_canvas(s, {800, 600}),
            "Chrome canvas initialization failed");
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto control = [display](unsigned i) { return display + 0xba864 + i * 0x17c; };
    const auto rectangle = [&](unsigned at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto meanings = [&] {
        std::vector<std::uint8_t> bytes;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            // Actions, flags, style, payload, colors, font, and complete label.
            // Geometry and allocation-dependent auxiliary text pointers are omitted.
            for (const auto [offset, size] : {std::pair{8u, 8u}, {0x38u, 8u}, {0x48u, 4u},
                                            {0x68u, 10u}, {0x78u, 256u}}) {
                const auto part = s.memory.copy_out(control(i) + offset, size);
                bytes.insert(bytes.end(), part.begin(), part.end());
            }
        }
        return bytes;
    };
    const auto build = [&] {
        invoke(0x10719560, {display, 1, 0});
        // This is the same common builder called by the original club page.
        invoke(0x102a1070, {4, 0});
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            invoke(0x10714630, {grid(i)});
            invoke(0x10714110, {grid(i), 0});
        }
    };
    build();
    const auto original_meanings = meanings();
    require(!original_meanings.empty(), "Chrome fixture built no controls");
    const auto tactics = c.u32(c.toc(-0x7a6c)), teams = c.u32(tactics + 0x5c4);
    const auto team_size = c.u32(tactics + 0x5c8) * 0x94;
    require(teams && team_size, "Chrome fixture has no saved team rosters");
    const auto records = [&] {
        std::vector<std::uint8_t> bytes;
        for (const auto [address, size] : {std::pair{club, 0x248u}, {person, 0x70u},
                                          {teams, team_size}, {c.toc(-0x7b80), 8u}}) {
            const auto part = s.memory.copy_out(address, size);
            bytes.insert(bytes.end(), part.begin(), part.end());
        }
        const auto random = random_state(s);
        bytes.insert(bytes.end(), random.begin(), random.end());
        return bytes;
    };
    const auto saved_records = records();
    const auto sidebar_bitmap = [&](GuiCanvas canvas, unsigned kind, bool has_image) {
        require(s.platform->resize_gui_canvas(s, canvas), "Sidebar bitmap resize failed");
        const auto object = scratch + 512, bitmap = scratch + 1024, source = scratch + 1088;
        s.memory.clear(object, 0x17c);
        c.put32(object + 12, kind);
        c.put32(object + 0x18, 89); c.put32(object + 0x1c, canvas.height - 1);
        c.put32(object + 0x5c, has_image ? bitmap : 0); c.put8(object + 0x17a, 0x5a);
        // A tiny RGB555 image makes both axes and the final row independently
        // observable without borrowing or changing any original game artwork.
        c.put32(bitmap, 9); c.put32(bitmap + 4, 3); c.put32(bitmap + 12, source);
        for (unsigned y = 0; y < 3; ++y) for (unsigned x = 0; x < 9; ++x)
            c.put16(source + (y * 9 + x) * 2, 0x4000 + y * 0x100 + x);
        const auto fixture = s.memory.copy_out(scratch + 512, 768);
        const auto pixels = c.u32(c.r(2) - 0x3564);
        const unsigned stride = c.u16(c.r(2) - 0x3566);
        std::vector<std::uint8_t> background(std::size_t(stride) * canvas.height, 0x33);
        s.memory.copy_in(pixels, background);
        const auto registers = s.registers;
        c.set(31, object); c.set(0, 0x12345678);
        const auto before = s.registers;
        c.set(0, 0x5a); const auto loaded = s.registers; s.registers = before;
        const auto executed = s.executed;
        s.budget = s.executed + 1;
        require(adaptive_gui_instruction(s, 0x10726050) == !canvas.original(),
                "Sidebar bitmap hook intercepted original canvas or missed expanded canvas");
        require(s.executed == executed + (canvas.original() ? 0 : 1), "Sidebar bitmap hook lost instruction accounting");
        if (canvas.original()) {
            require(s.registers == before, "Original canvas bitmap hook changed guest registers");
            instruction(s, 0x10726050);
        }
        require(s.registers == loaded, "Sidebar bitmap hook did not preserve the original lbz and other registers");
        const auto drawn = s.memory.copy_out(pixels, background.size());
        const bool extended = !canvas.original() && kind == 1024 && has_image;
        for (int y = 0; y < canvas.height; ++y) for (unsigned x = 0; x < stride; ++x) {
            std::uint8_t expected = 0x33;
            if (extended && x < 180) {
                const unsigned color = 0x4000 + unsigned(y * 3 / canvas.height) * 0x100 + x / 20;
                expected = std::uint8_t(x & 1 ? color : color >> 8);
            }
            require(drawn[std::size_t(y) * stride + x] == expected,
                    "Sidebar bitmap missed its bottom row or changed adjacent pixels");
        }
        require(s.memory.copy_out(scratch + 512, fixture.size()) == fixture,
                "Sidebar bitmap changed its source, control or neighboring guest bytes");
        s.registers = registers;
        require(records() == saved_records, "Sidebar bitmap changed career records or RNG");
    };
    sidebar_bitmap({800, 600}, 1024, true);
    sidebar_bitmap({801, 601}, 1024, true);
    sidebar_bitmap({1280, 900}, 1024, true);
    sidebar_bitmap({1280, 900}, 1024, false);
    sidebar_bitmap({1280, 900}, 0, true);
    const auto sidebar = [&](GuiCanvas canvas) {
        require(s.platform->resize_gui_canvas(s, canvas), "Sidebar resize failed");
        build();
        require(meanings() == original_meanings, "Sidebar resize changed original actions, text, fonts or colors");
        unsigned outer = 0, menus = 0;
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            const auto box = rectangle(grid(i));
            if (box.left == 0 && box.top == 0 && box.right == 89) {
                require(box.bottom == canvas.height - 1, "Sidebar background stopped at the legacy bottom");
                ++outer;
            }
            if (box.left == 5 && box.top == 10 && box.right == 85) {
                require(box.bottom == canvas.height - 10, "Sidebar menu grid retained legacy height");
                ++menus;
            }
        }
        require(outer == 1 && menus >= 1, "Chrome fixture missed the original sidebar grids");
        const auto describe = [&](unsigned index) {
            const auto at = control(index);
            const auto box = rectangle(at + 0x10);
            return " canvas=" + std::to_string(canvas.width) + "x" + std::to_string(canvas.height) +
                " index=" + std::to_string(index) + " rect=" + std::to_string(box.left) + "," +
                std::to_string(box.top) + "," + std::to_string(box.right) + "," + std::to_string(box.bottom) +
                " action=" + std::to_string(c.u32(at + 8)) + " flags=" + hex_address(c.u32(at + 12)) +
                " owner=" + std::to_string(c.i16(at + 0x72));
        };
        int continued = -1, status = -1;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = control(i);
            const auto box = rectangle(at + 0x10);
            if (c.u32(at + 8) == 1000 && (c.u32(at + 12) & 2)) continued = int(i);
            if (c.i16(at + 0x72) != -1 || box.left != 10 || box.right != 80) continue;
            // The eight original loads use 0x226 (550), preserving a 40px
            // status button. Its action depends on the captured game state.
            if (box.top != canvas.height - 50 || box.bottom != canvas.height - 10)
                throw std::runtime_error("Sidebar status button did not follow the lower edge:" + describe(i));
            if (c.u32(at + 12) & 2) status = int(i);
        }
        require(continued >= 0 && status >= 0, "Chrome fixture lacks original Continue/status hit targets");
        const auto hit = [&](int x, int y) {
            c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
            c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
            return std::int16_t(invoke(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
        };
        const auto continue_box = rectangle(control(unsigned(continued)) + 0x10);
        const auto continue_hit = hit((continue_box.left + continue_box.right) / 2,
                                      (continue_box.top + continue_box.bottom) / 2);
        if (continue_hit != continued)
            throw std::runtime_error("Sidebar Continue hit returned " + std::to_string(continue_hit) + ":" + describe(continued));
        const auto status_hit = hit(45, canvas.height - 30);
        if (status_hit != status)
            throw std::runtime_error("Sidebar status hit returned " + std::to_string(status_hit) + ":" + describe(status));
        if (canvas.height > 650)
            require(hit(45, 570) != status, "Sidebar status retained an invisible legacy hit target");
        require(records() == saved_records, "Sidebar construction/hit tests changed career records or RNG");
    };
    for (auto canvas : {GuiCanvas{800, 600}, GuiCanvas{801, 601}, GuiCanvas{800, 900},
                        GuiCanvas{1440, 600}, GuiCanvas{1280, 900}, GuiCanvas{3200, 1800}})
        sidebar(canvas);

    const char message[] = "Processing days and matches";
    for (unsigned i = 0; i < sizeof(message); ++i) c.put8(scratch + 64 + i, message[i]);
    const auto progress = [&](GuiCanvas canvas) {
        require(s.platform->resize_gui_canvas(s, canvas), "Processing resize failed");
        const auto pixels = c.u32(c.r(2) - 0x3564);
        const unsigned stride = c.u16(c.r(2) - 0x3566);
        require(pixels && stride >= unsigned(canvas.width * 2), "Processing has no live framebuffer");
        std::vector<std::uint8_t> background(std::size_t(stride) * canvas.height);
        for (std::size_t i = 0; i < background.size(); ++i) background[i] = i & 1 ? 0x34 : 0x12;
        // Continue closes screen histories before daily/match progress draws.
        // The committed framebuffer remains authoritative until the next builder.
        c.put32(manager + 0x10, 0);
        c.put8(c.r(2) - 0x3550, 0); c.put8(c.r(2) - 0x354f, 1); c.put8(c.r(2) - 0x3558, 0);
        std::vector<std::uint8_t> quarter;
        for (const auto [done, total] : {std::pair{1u, 4u}, {4u, 4u}, {0xffffffffu, 0u}}) {
            s.memory.copy_in(pixels, background);
            imports.blits.clear();
            require(invoke(0x1076e5f0, {scratch + 64, done, total}) == 1, "Original progress draw failed");
            require(gui_canvas(s) == canvas && !c.u32(manager + 0x10), "Progress changed the committed canvas/history");
            // CopyBits rectangles have exclusive right/bottom coordinates.
            const ScreenRect repaint{100, canvas.height - 45, canvas.width - 9, canvas.height - 9};
            require(!imports.blits.empty() && imports.blits.back().first == repaint && imports.blits.back().second == repaint,
                    "Processing partial repaint retained the legacy rectangle");
            const auto drawn = s.memory.copy_out(pixels, background.size());
            bool changed = false;
            for (int y = 0; y < canvas.height; ++y) for (unsigned x = 0; x < stride; ++x) {
                const auto at = std::size_t(y) * stride + x;
                if (drawn[at] == background[at]) continue;
                require(y >= repaint.top && y < repaint.bottom && x >= unsigned(repaint.left * 2) &&
                        x < unsigned(repaint.right * 2), "Processing draw escaped its partial repaint");
                changed = true;
            }
            require(changed, "Progress check observed no framebuffer writes");
            if (done == 1) quarter = drawn;
            if (done == 4) {
                const auto sample = std::size_t(canvas.height - 28) * stride + (canvas.width - 40) * 2;
                require(drawn[sample] != quarter[sample] || drawn[sample + 1] != quarter[sample + 1],
                        "Progress fill failed to reach the expanded track's end");
            }
            require(records() == saved_records, "Processing presentation changed career records or RNG");
        }
        c.put32(manager + 0x10, history);
    };
    for (auto canvas : {GuiCanvas{800, 600}, GuiCanvas{801, 601}, GuiCanvas{800, 900},
                        GuiCanvas{1440, 600}, GuiCanvas{1280, 900}})
        progress(canvas);

    // Off is applied at the existing safe boundary, then both common builders
    // reproduce the original geometry and control meanings on the small canvas.
    select_new_screens(false);
    require(s.platform->request_adaptive_screen(0x1001e2b0) && gui_canvas(s) == GuiCanvas{1280, 900},
            "Disabling New Screens bypassed the deferred canvas boundary");
    invoke(0x10719560, {display, 1, 0});
    s.platform->apply_adaptive_screen(s, display);
    require(gui_canvas(s).original(), "New Screens Off retained an expanded canvas");
    sidebar({800, 600}); progress({800, 600});
    require(records() == saved_records, "Chrome fallback changed career records or RNG");
    initialize_new_screens();
    std::cout << "Adaptive sidebar styles/actions/hits and history-free progress pixels/partial repaint/ABI/RNG passed\n";
}

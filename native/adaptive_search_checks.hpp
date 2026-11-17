// Included inside adaptive_gui_checks.cpp's anonymous namespace. These checks
// use a disposable captured world and the original search opener/builders.
// Recruitment population is explicitly controlled in the copied SEARCH_ENG;
// the saved SEARCH_MGR presets are compared and never intentionally modified.
void search_builder(const char* path, bool recruitment = false) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    const auto presets = recruitment ? std::vector<unsigned>{0,3,8} : std::vector<unsigned>{1,2};
    for (unsigned preset : presets) {
        const std::string label = preset == 0 ? "shortlist" : preset == 1 ? "players" : preset == 2 ? "staff" :
            "scout preset " + std::to_string(preset);
        std::cout << "search " << label << ": opening captured fixture" << std::endl;
        Clock clock; clock.idle_input = true;
        auto state = std::make_unique<State>(); auto& s = *state;
        load_match_snapshot(s, path); s.code_fragment_observer = &clock;
        guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
        const auto manager = c.toc(-0x7bb0);
        c.put16(manager + 0x2c58, 0);
        unsigned current_view = 0;
        GuiCanvas current_canvas;
        const char* phase = "open";
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> values) {
            unsigned reg = 3; for (auto value : values) c.set(reg++, value);
            s.budget = s.executed + 400000000;
            try { c.call(entry, return_sentinel); }
            catch (const std::exception& error) {
                throw std::runtime_error("Search preset " + std::to_string(preset) + " view " +
                    hex_address(current_view) + " " + phase + " " + hex_address(entry) + " at " +
                    hex_address(s.pc) + " canvas " + std::to_string(current_canvas.width) + "x" +
                    std::to_string(current_canvas.height) + ": " + error.what());
            }
            return c.r(3);
        };
        std::filesystem::create_directories(temp.path / "user" / "Files");
        s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
        require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
                "Search baseline pixel globals");
        // SEARCH_ENG first copies the captured human's presets. Recruitment
        // cases test their real empty state before supplying private row copies.
        invoke(0x1031ad90, {preset});
        const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
        const auto setting = [&](unsigned slot) { return history + 0x14 + slot * 8; };
        const auto engine = c.u32(setting(0));
        const auto config = engine + 4 + preset * 0x2b0, meta = config + 0x29c;
        require(c.u32(c.u32(history)) == 0x1031af20 && c.u8(engine) == preset &&
                c.u8(config + 0xd) == (preset <= 2 ? preset + 1 : 4) && c.u32(setting(5)) == 0,
                "Original search opener selected a different search or selection mode");
        require(adaptive_screen_available(s, 0x1031af20), "Supported search preset did not pass its gate");
        for (unsigned index : {10u, 255u}) {
            c.put8(engine, index);
            require(!adaptive_screen_available(s, 0x1031af20), "Invalid search preset passed the gate");
        }
        c.put8(engine, preset);
        for (unsigned kind : {0u, 1u, 2u, 3u, 4u, 5u}) {
            c.put8(config + 0xd, kind);
            require(adaptive_screen_available(s, 0x1031af20) == (kind >= 1 && kind <= 4),
                    "Search gate did not distinguish supported and unknown modes");
        }
        c.put8(config + 0xd, preset <= 2 ? preset + 1 : 4);
        c.put32(setting(0), 0);
        require(!adaptive_screen_available(s, 0x1031af20), "Absent search engine passed the gate");
        c.put32(setting(0), engine);
        c.put16(manager + 0x2c58, 1);
        require(!adaptive_screen_available(s, 0x1031af20), "Secondary screen group entered adaptive Search");
        c.put16(manager + 0x2c58, 0);
        const auto network = c.u32(manager + 0x2c64);
        c.put32(manager + 0x2c64, 1);
        require(!adaptive_screen_available(s, 0x1031af20), "Network search entered adaptive layout");
        c.put32(manager + 0x2c64, network);
        select_new_screens(false);
        require(!adaptive_screen_available(s, 0x1031af20), "Disabled New Screens still adapts Search");
        select_new_screens(true);
        require(!adaptive_screen_available(s, 0x10329fd0), "Search filter modal entered the results template");
        const auto search_manager = c.u32(c.toc(-0x7b1c));
        const auto human_index = c.u32(config) - c.u32(c.toc(-0x7be4)) + 16;
        require(human_index < 16, "Captured search owner is not a human manager");
        const auto saved_presets_address = c.u32(search_manager) + human_index * 10 * 0x2b0;
        const auto saved_presets = s.memory.copy_out(saved_presets_address, 10 * 0x2b0);

        const auto grid_at = [&](unsigned i) { return display + 4 + i * 0xbf0; };
        const auto control_at = [&](unsigned i) { return display + 0xba864 + i * 0x17c; };
        const auto rectangle = [&](unsigned at) {
            return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
        };
        const auto refresh = [&] {
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                invoke(0x10714630, {grid_at(i)}); invoke(0x10714110, {grid_at(i), 0});
            }
        };
        struct Meaning {
            unsigned action, payload, kind, style, alignment, foreground, background, font, size;
            int owner, popup;
            unsigned column, row;
            std::string text;
            bool operator==(const Meaning&) const = default;
        };
        const auto meanings = [&] {
            std::vector<Meaning> result;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = control_at(i);
                std::string text;
                for (unsigned j = 0; j < 256 && c.u8(at + 0x78 + j); ++j)
                    text.push_back(char(c.u8(at + 0x78 + j)));
                result.push_back({c.u32(at + 8),c.u32(at + 0x48),c.u32(at + 0xc),c.u32(at + 0x38),
                    c.u32(at + 0x3c),c.u16(at + 0x6a),c.u16(at + 0x6c),c.u16(at + 0x6e),c.u16(at + 0x70),
                    c.i16(at + 0x72),c.i16(at + 0x74),c.u32(at + 0x20),c.u32(at + 0x24),std::move(text)});
            }
            return result;
        };
        // LIST is engine+35e4: count/head/tail/cursor at +0/+4/+8/+c.
        // Each node owns an item whose +c is the original staff ID. Reading
        // the links directly leaves the original iteration cursor untouched.
        const auto results = [&] {
            const auto list = engine + 0x35e4;
            const auto count = c.i16(list);
            require(count >= 0, "Search result list has a negative count");
            std::vector<unsigned> result;
            auto node = c.u32(list + 4);
            const auto staff_base = c.u32(c.toc(-0x7be0));
            const auto staff_count = c.u32(c.toc(-0x7be4));
            for (int i = 0; i < count; ++i) {
                require(node && c.u32(node), "Search result list ended before its count");
                const auto id = c.u32(c.u32(node) + 0xc);
                require(id < staff_count, "Search result refers to an invalid staff ID");
                result.push_back(staff_base + id * 0x70);
                node = c.u32(node + 8);
            }
            require(!node, "Search result list continued beyond its count");
            return result;
        };
        constexpr unsigned absent = 0xffffffffu;
        unsigned body = absent, header = absent, overlay = absent, background = absent, footer = absent;
        ScreenRect original_body{}, original_header{};
        const auto identify_grids = [&] {
            body = header = overlay = background = footer = absent;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                const auto at = grid_at(i);
                const auto bounds = rectangle(at);
                if (bounds.left == 110 && bounds.right == 780 && bounds.top > 223 &&
                    bounds.bottom == current_canvas.height - 148) {
                    if (c.u8(at + 0xbb0) > 1) body = i; else overlay = i;
                }
                if (bounds.left == 110 && bounds.right == 780 && bounds.top == 223 && c.u8(at + 0xbb1) == 1)
                    header = i;
                if (bounds == ScreenRect{110,215,780,current_canvas.height-140}) background = i;
                if (bounds == ScreenRect{100,555,790,590} && c.u8(at + 0xbb0) == 2 && c.u8(at + 0xbb1) == 1)
                    footer = i;
            }
            const bool populated = !results().empty();
            require((!populated || (body != absent && header != absent && overlay != absent)) &&
                    background != absent && footer != absent,
                    "Original search lacks the audited table/header/overlay/footer structure");
            original_body = body == absent ? ScreenRect{} : rectangle(grid_at(body));
            original_header = header == absent ? ScreenRect{} : rectangle(grid_at(header));
        };
        const auto visible_staff = [&] {
            std::vector<unsigned> result;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = control_at(i);
                if (c.u32(at + 8) == 2007 && c.i16(at + 0x72) >= 0) {
                    if (c.i16(at + 0x72) == int(body))
                        result.push_back(c.u32(at + 0x48));
                }
            }
            return result;
        };
        const auto find_action = [&](unsigned action, unsigned payload = 0xffffffffu) {
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = control_at(i);
                if (c.u32(at + 8) == action && (payload == absent || c.u32(at + 0x48) == payload)) return i;
            }
            return absent;
        };
        const auto hit = [&](unsigned index) {
            require(index != absent, "Missing search control for real input check");
            const auto at = control_at(index);
            const auto bounds = rectangle(at + 0x10);
            require(bounds.width() > 0 && bounds.height() > 0, "Search input target has empty bounds");
            const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
            const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
            c.put32(c.toc(-0x5c10), (bounds.left + bounds.right) / 2);
            c.put32(c.toc(-0x5c14), (bounds.top + bounds.bottom) / 2);
            c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
            const auto clicked = std::int16_t(invoke(0x1071a280, {display}));
            c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
            c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
            return clicked;
        };
        const auto layout_pass = [&] {
            const auto before = meanings(); const auto before_results = results();
            const auto registers = s.registers; const auto unique = s.unique;
            const auto pc = s.pc, stack_base = c.sp() - 0x8000;
            const auto stack = s.memory.copy_out(stack_base, 0x8100);
            const auto query = s.memory.copy_out(engine, 0x35fc), random = random_state(s);
            std::vector<std::pair<unsigned, unsigned>> scroll;
            struct DescriptionSpan { unsigned index; ScreenRect bounds; std::array<int,4> overrides; };
            struct Description { unsigned grid; ScreenRect bounds; std::vector<DescriptionSpan> spans; };
            std::vector<Description> descriptions;
            std::vector<std::string> description_toolbar_diagnostics;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i)
                scroll.emplace_back(grid_at(i), c.u32(grid_at(i) + 0x1fc));
            if (recruitment && preset == 3 && c.u8(engine) == 3 && c.i32(config + 0x194) >= 0) {
                for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                    const auto area = grid_at(i);
                    const auto bounds = rectangle(area);
                    if (bounds.top == 125 || bounds.bottom == 145)
                        description_toolbar_diagnostics.push_back("grid="+std::to_string(i)+
                            " before="+std::to_string(bounds.left)+","+std::to_string(bounds.top)+","+
                            std::to_string(bounds.right)+","+std::to_string(bounds.bottom)+
                            " cols="+std::to_string(c.u8(area+0xbb0))+" rows="+std::to_string(c.u8(area+0xbb1)));
                    if (bounds.left != 110 || bounds.top != 150 || bounds.height() != 20 ||
                        c.u8(area + 0xbb0) != 1 || c.u8(area + 0xbb1) != 1) continue;
                    Description item{i,bounds,{}};
                    for (unsigned j = 0; j < c.u16(display + 0x129da6); ++j) {
                        const auto at = control_at(j);
                        if (c.i16(at + 0x72) == int(i) && c.i32(at + 0x28) > 0 && c.i32(at + 0x2c) > 0)
                            item.spans.push_back({j,rectangle(at + 0x10),
                                {c.i32(at+0x28),c.i32(at+0x2c),c.i32(at+0x30),c.i32(at+0x34)}});
                    }
                    require(!item.spans.empty(), "Original Scout description lacks its explicit centered spans");
                    descriptions.push_back(std::move(item));
                }
                require(descriptions.size() == 1, "Original Scout club assignment lacks its description grid");
            }
            // Check actual visible staff/player/club records around this exact
            // added pass, after the original medical/status formatter ran.
            std::vector<std::pair<unsigned, std::vector<std::uint8_t>>> records;
            for (const auto staff : visible_staff()) {
                records.emplace_back(staff, s.memory.copy_out(staff, 0x70));
                if (const auto player = c.u32(staff + 0x14)) records.emplace_back(player, s.memory.copy_out(player, 0x46));
                if (const auto club = c.u32(staff + 0x24)) records.emplace_back(club, s.memory.copy_out(club, 0x248));
            }
            layout_built_screen(s, display, 0x1031af20);
            require(s.registers == registers && s.unique == unique && s.pc == pc &&
                    s.memory.copy_out(stack_base, stack.size()) == stack,
                    "Search layout clobbered caller registers, unique bytes, PC or stack");
            require(s.memory.copy_out(engine, query.size()) == query && results() == before_results && random_state(s) == random,
                    "Search layout changed query, result cache or RNG");
            require(s.memory.copy_out(saved_presets_address, saved_presets.size()) == saved_presets,
                    "Search layout changed persistent shortlist/scout settings");
            for (const auto& [at, bytes] : records)
                require(s.memory.copy_out(at, bytes.size()) == bytes, "Search layout changed a saved staff/player/club record");
            for (const auto& [at, value] : scroll)
                require(c.u32(at + 0x1fc) == value, "Search layout changed an AutoGrid scroll offset");
            refresh();
            require(meanings() == before, "Search layout changed actions, payloads, order, labels, fonts or colours");
            for (const auto& item : descriptions) {
                const int extra = current_canvas.width - 800;
                require(rectangle(grid_at(item.grid)) == ScreenRect{item.bounds.left,item.bounds.top,
                    item.bounds.right+extra,item.bounds.bottom}, "Scout description did not fill its available width");
                bool scout_link = false, club_link = false;
                for (const auto& [index,rect,overrides] : item.spans) {
                    const auto at = control_at(index);
                    const auto expected = ScreenRect{rect.left+extra/2,rect.top,rect.right+extra/2,rect.bottom};
                    if (rectangle(at + 0x10) != expected) {
                        const auto show = [](ScreenRect r) {
                            return std::to_string(r.left)+","+std::to_string(r.top)+","+
                                std::to_string(r.right)+","+std::to_string(r.bottom);
                        };
                        std::cerr << "Scout description mismatch canvas=" << current_canvas.width << 'x' << current_canvas.height
                            << " grid=" << item.grid << " grid-before=" << show(item.bounds)
                            << " grid-after=" << show(rectangle(grid_at(item.grid)))
                            << " control=" << index << " action=" << c.u32(at+8) << " kind=" << c.u32(at+0xc)
                            << " alignment=" << c.u32(at+0x3c) << " bounds-before=" << show(rect)
                            << " bounds-after=" << show(rectangle(at+0x10)) << " expected=" << show(expected)
                            << " overrides-before=" << overrides[0] << ',' << overrides[1] << ',' << overrides[2] << ',' << overrides[3]
                            << " overrides-after=" << c.i32(at+0x28) << ',' << c.i32(at+0x2c) << ',' << c.i32(at+0x30) << ',' << c.i32(at+0x34)
                            << std::endl;
                        for (const auto& diagnostic : description_toolbar_diagnostics)
                            std::cerr << "Scout toolbar " << diagnostic << std::endl;
                    }
                    require(rectangle(at + 0x10) == expected,
                            "Scout description changed its centered text/link widths or hit bounds");
                    scout_link |= c.u32(at + 8) == 2007;
                    club_link |= c.u32(at + 8) == 2005;
                    if (c.u32(at + 8) == 2007 || c.u32(at + 8) == 2005)
                        require(hit(index) == int(index), "Scout description link lost its displayed input target");
                }
                require(scout_link && club_link, "Scout assignment lost its original scout and club links");
            }
        };
        const auto build = [&] {
            invoke(0x10719560, {display, 1, 0}); invoke(0x1031af20, {});
            refresh(); identify_grids(); layout_pass();
        };
        const auto resize_build = [&](GuiCanvas canvas) {
            current_canvas = canvas;
            require(s.platform->resize_gui_canvas(s, canvas), "Search canvas resize failed");
            build();
        };
        phase = "baseline";
        if (recruitment) {
            std::cout << "search " << label << ": captured empty/locked state" << std::endl;
            for (const auto canvas : {GuiCanvas{800,600},GuiCanvas{1280,900}}) {
                resize_build(canvas);
                require(results().empty() && body == absent && header == absent && overlay == absent,
                        "Captured recruitment session is not the expected empty case");
                require(rectangle(grid_at(background)) == ScreenRect{110,215,canvas.width-20,canvas.height-140} &&
                        rectangle(grid_at(footer)) == ScreenRect{100,canvas.height-45,canvas.width-10,canvas.height-10},
                        "Empty recruitment background/footer did not follow the canvas");
            }
            if (preset >= 3)
                require(bool(invoke(0x10323cb0, {engine,0xffffffffu})) == (preset == 3),
                        "Captured assigned/unavailable Scout fixture differs from the expected state");
            const auto lock_bytes = c.u16(config + 0x13c);
            c.put16(config + 0x13c, 0x5800); // Temporary password marker: "X".
            build();
            bool locked_label = false;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
                locked_label |= rectangle(control_at(i) + 0x10) == ScreenRect{110,175,210,195};
            require(locked_label, "Locked recruitment session lost the original fixed-size label");
            c.put16(config + 0x13c, lock_bytes);
            require(s.memory.copy_out(saved_presets_address, saved_presets.size()) == saved_presets,
                    "Controlled lock changed persistent recruitment settings");
            if (preset == 8) {
                std::cout << "search " << label << ": empty, unavailable and locked layout passed" << std::endl;
                continue;
            }
            // Populate only the copied SEARCH_ENG session, using real result
            // records produced by the original Player Search query. No add,
            // remove, assignment or save handler runs; SEARCH_MGR stays intact.
            std::cout << "search " << label << ": controlled 35-result session" << std::endl;
            invoke(0x10315450, {engine,1}); resize_build({800,600});
            require(results().size() >= 35, "Original Player Search lacks controlled recruitment inputs");
            std::vector<std::uint8_t> items;
            auto node = c.u32(engine + 0x35e4 + 4);
            for (unsigned i = 0; i < 35; ++i) {
                const auto bytes = s.memory.copy_out(c.u32(node), 0x30);
                items.insert(items.end(), bytes.begin(), bytes.end()); node = c.u32(node + 8);
            }
            invoke(0x10315450, {engine,preset});
            require(!c.u32(config + 0x1f4), "Controlled recruitment source unexpectedly owns an existing array");
            const auto source = allocate(s, items.size(), false); s.memory.copy_in(source, items);
            c.put32(config + 0x1f4, source); c.put32(config + 0x1f8,35); c.put32(config + 0x1fc,35);
            if (preset == 3) {
                const auto human_club = c.u32(c.u32(manager + 4) + 0x24);
                require(human_club, "Captured manager has no club for the Scout description");
                c.put32(config + 0x194,c.u32(human_club));
                // One result removes the pager, producing the other original
                // description width. The same original builder makes its links.
                c.put32(config + 0x1fc,1); c.put8(engine + 0x35f9,0);
                resize_build({800,600}); resize_build({801,600}); resize_build({1280,900});
                require(results().size() == 1 && c.u32(setting(7)) == 1,
                        "Controlled Scout description did not exercise the no-pager case");
                c.put32(config + 0x1fc,35); c.put8(engine + 0x35f9,0);
            }
            resize_build({800,600});
        }
        build();
        const auto search_font_height = invoke(0x1076d470, {2});
        require(search_font_height == 18, "Captured Search font-two height differs from the original eighteen pixels");
        const auto initial_results = results();
        require(initial_results.size() > 30 && visible_staff().size() == 11,
                "Captured search lacks enough results to test expanded pagination");
        std::vector<unsigned> views;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = control_at(i);
            if (c.u32(at + 8) == 18) {
                const auto view = c.u32(at + 0x48);
                if (std::find(views.begin(), views.end(), view) == views.end()) views.push_back(view);
            }
        }
        for (unsigned required : {1u,8u,16u})
            require(std::find(views.begin(), views.end(), required) != views.end(),
                    "Search View menu lacks General, Attributes or Notes");
        if (preset == 1) for (unsigned required : {2u,4u,32u,64u})
            require(std::find(views.begin(), views.end(), required) != views.end(),
                    "Player Search View menu lacks an original player-specific view");
        if (recruitment) views = {1,16}; // Existing modes already cover the shared column families.
        require(original_body == ScreenRect{110,242,780,452} && original_header == ScreenRect{110,223,780,241},
                "Captured Search has different original table metrics");
        const auto verify_page = [&](const std::vector<unsigned>& ordered) {
            const auto capacity = unsigned(c.u8(grid_at(body) + 0xbb1));
            require(capacity >= 11 && capacity <= 30, "Search row capacity exceeded AutoGrid or shrank");
            const auto page = unsigned(c.i16(meta + 0xc));
            require(page >= 1 && c.u32(setting(7)) == (ordered.size() + capacity - 1) / capacity,
                    "Search page count and visible capacity disagree");
            auto expected = ordered;
            if (c.u8(meta + 0xa)) std::reverse(expected.begin(), expected.end());
            const auto start = std::size_t(page - 1) * capacity;
            require(start < expected.size(), "Search page points beyond its result cache");
            expected = std::vector<unsigned>(expected.begin() + start,
                expected.begin() + std::min(expected.size(), start + capacity));
            require(visible_staff() == expected, "Search resize changed the visible ordered staff slice or payloads");
        };
        for (unsigned view : views) {
            current_view = view; phase = "view";
            std::cout << "search preset " << preset << " view " << hex_address(view) << std::endl;
            const auto choice = find_action(18, view);
            require(choice != absent && std::int32_t(invoke(0x10324100, {choice})) == -11,
                    "Original View action did not request a search redraw");
            c.put16(meta + 0xc, 1);
            resize_build({800,600});
            const auto ordered = results();
            require(ordered == initial_results, "Original View action unexpectedly changed result order");
            const auto filters = s.memory.copy_out(config, 0x29c);
            const auto sort = s.memory.copy_out(meta + 4, 8);
            const auto original_rows = visible_staff();
            const auto original_meanings = meanings();
            std::vector<Meaning> baseline_cells;
            for (const auto& item : original_meanings)
                if (item.owner == int(body)) baseline_cells.push_back(item);
            const auto random = random_state(s);
            const auto canvases = recruitment
                ? (view == 1 ? std::vector<GuiCanvas>{{1280,900},{1200,645}} : std::vector<GuiCanvas>{{1280,900}})
                : view == views.front()
                ? std::vector<GuiCanvas>{{1200,680},{1280,900},{3200,1800},{1200,645},{822,680},{801,601},{800,600}}
                : std::vector<GuiCanvas>{{1280,900}};
            for (const auto canvas : canvases) {
                phase = "resize"; resize_build(canvas);
                require(results() == ordered && s.memory.copy_out(config, filters.size()) == filters &&
                        s.memory.copy_out(meta + 4, sort.size()) == sort && random_state(s) == random,
                        "Search resize changed filtering, sorting, result order or RNG");
                verify_page(ordered);
                const auto rows = unsigned(c.u8(grid_at(body) + 0xbb1));
                require(rectangle(grid_at(header)) == ScreenRect{110,original_header.top,canvas.width-20,original_header.bottom} &&
                        rectangle(grid_at(body)) == ScreenRect{110,original_body.top,canvas.width-20,canvas.height-148} &&
                        rectangle(grid_at(overlay)) == rectangle(grid_at(body)) &&
                        c.u8(grid_at(overlay) + 0xbb1) == rows &&
                        rectangle(grid_at(background)) == ScreenRect{110,215,canvas.width-20,canvas.height-140} &&
                        rectangle(grid_at(footer)) == ScreenRect{100,canvas.height-45,canvas.width-10,canvas.height-10},
                        "Search table, selection overlay, background or footer failed to follow the canvas");
                unsigned bottom_tabs = 0;
                for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i)
                    if (rectangle(grid_at(i)) == ScreenRect{100,canvas.height-90,canvas.width-10,canvas.height-55})
                        ++bottom_tabs;
                require(bottom_tabs == 2, "Search lower preset tabs moved twice or overlapped the footer");
                if (canvas.height >= 900) require(rows > 11, "Taller Search did not expose more results");
                const auto expected_rows = std::min(31u, unsigned(canvas.height - 371) / search_font_height) - 1;
                require(rows == expected_rows, "Search row capacity disagrees with original font height and available space");
                if (canvas.height == 900) require(rows == 28, "Search 900px row-capacity golden changed");
                if (canvas.height == 1800) require(rows == 30, "Tall Search did not clamp at thirty body rows");
                std::vector<Meaning> common_cells;
                for (const auto& item : meanings())
                    if (item.owner == int(body) && item.row < original_rows.size()) common_cells.push_back(item);
                require(common_cells == baseline_cells, "Search resize changed existing row content/actions/style");
                for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                    const auto at = control_at(i);
                    if (c.i16(at + 0x72) != int(body)) continue;
                    const auto bounds = rectangle(at + 0x10), area = rectangle(grid_at(body));
                    require(c.u32(at + 0x24) < rows && bounds.left >= area.left && bounds.right <= area.right &&
                            bounds.top >= area.top && bounds.bottom <= area.bottom && bounds.height() > 0,
                            "Search result hit rectangle escaped its displayed table");
                }
                // Names use the original global profile action (2007). Never
                // dispatch it or the footer's persistent-search Save action.
                const auto name = find_action(2007, visible_staff().front());
                require(hit(name) == int(name), "Search staff click did not hit the displayed result");
                bool view_menu = false;
                for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                    const auto at = control_at(i);
                    if (c.i32(at + 0x14) != 125 || c.i32(at + 0x1c) != 145 ||
                        c.i16(at + 0x74) < 0) continue;
                    const auto popup = c.i16(at + 0x74);
                    bool is_view = false;
                    for (unsigned j = 0; j < c.u16(display + 0x129da6); ++j)
                        is_view |= c.i16(control_at(j) + 0x72) == popup && c.u32(control_at(j) + 8) == 18;
                    if (!is_view) continue;
                    require(hit(i) == -1 && c.u8(display) == 1 && c.i16(display + 0x12a930) == popup,
                            "Search View click did not open its original menu");
                    view_menu = true; break;
                }
                require(view_menu, "Search lost its View popup control");
            }
        }
        // The real Name sort handler changes only query presentation/cache.
        // Check both directions, and the page forward/back actions, against
        // the linked-list order rather than merely comparing two UI builders.
        current_view = views.front(); phase = "sort";
        std::cout << "search preset " << preset << ": original sort and pagination" << std::endl;
        require(std::int32_t(invoke(0x10324100, {find_action(18, current_view)})) == -11, "Search general View action");
        c.put16(meta + 0xc, 1); resize_build({1280,900});
        for (unsigned repetition = 0; repetition < 2; ++repetition) {
            const auto name = find_action(30, 4);
            const auto old_direction = c.u8(meta + 0xa); const auto old_sort = c.u32(meta + 4);
            require(hit(name) == int(name) && std::int32_t(invoke(0x10324100, {name})) == -11,
                    "Search Name heading lost its real sorting action");
            require(c.u8(meta + 0xa) == ((old_sort & 4) && !old_direction ? 1 : 0) &&
                    c.u32(meta + 4) == ((old_sort & 0xc0000000u) | 4u) && c.u8(engine + 0x35f9) == 0,
                    "Original Name sort toggle/cache invalidation changed");
            resize_build({800,600}); const auto ordered = results(); verify_page(ordered);
            const auto sort_random = random_state(s), filter = s.memory.copy_out(config, 0x29c);
            resize_build({1280,900}); verify_page(ordered);
            require(results() == ordered && random_state(s) == sort_random && s.memory.copy_out(config, filter.size()) == filter,
                    "Adaptive sorted Search changed the original query/results");
            const auto next = find_action(25);
            require(hit(next) == int(next) && std::int32_t(invoke(0x10324100, {next})) == -11 && c.i16(meta + 0xc) == 2,
                    "Search next-page click did not advance the original page state");
            build(); verify_page(ordered);
            const auto previous = find_action(24);
            require(hit(previous) == int(previous) && std::int32_t(invoke(0x10324100, {previous})) == -11 &&
                    c.i16(meta + 0xc) == 1, "Search previous-page click lost the original page state");
            build(); verify_page(ordered);
        }
        if (preset == 1) {
            // This capture has no populated scout Report in Player Search.
            // Reproduce its audited GUIO shape without creating scouting data:
            // thirteen original weights, one explicit text span, two club
            // links. Existing sources lend semantics, never owned caches.
            phase = "controlled report";
            std::cout << "search players: controlled Report spans and links" << std::endl;
            current_canvas = {800,600};
            require(s.platform->resize_gui_canvas(s, current_canvas), "Report baseline resize");
            invoke(0x10719560, {display,1,0}); invoke(0x1031af20, {}); refresh(); identify_grids();
            unsigned label_source = absent, link_source = absent;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = control_at(i);
                if (c.i16(at + 0x72) != int(body)) continue;
                if (!c.u32(at + 8) && label_source == absent) label_source = i;
                if (c.u32(at + 8) == 2005 && link_source == absent) link_source = i;
            }
            require(label_source != absent && link_source != absent, "Controlled Report lacks original cell/link sources");
            constexpr unsigned weights[]{6,5,22,6,6,6,6,30,0,0,0,0,10};
            for (const auto owner : {header,body}) {
                require(c.u8(grid_at(owner) + 0xbb0) == 13, "Report source has a different column count");
                for (unsigned column = 0; column < 13; ++column) c.put8(grid_at(owner) + 0xbb2 + column, weights[column]);
                invoke(0x10714630, {grid_at(owner)});
            }
            // AREA stores column offsets relative to its left edge. GUIO
            // refresh publishes the absolute drawing/input coordinates.
            require(c.i32(grid_at(body)) + c.i32(grid_at(body) + 0x1c + 7*4) == 505 &&
                    c.i32(grid_at(body)) + c.i32(grid_at(body) + 0x10c + 7*4) == 710,
                    "Controlled Report original column-seven bounds differ from the audited layout");
            const auto append = [&](unsigned source, int left, int right) {
                const auto index = unsigned(c.u16(display + 0x129da6)), at = control_at(index);
                require(index < 1199, "Controlled Report exceeds the GUIO limit");
                s.memory.copy_in(at, s.memory.copy_out(control_at(source), 0x17c));
                c.put32(at + 0x4c, 0); c.put32(at + 0x5c, 0);
                c.put32(at + 0x20, 7); c.put32(at + 0x24, 0);
                c.put32(at + 0x28, left); c.put32(at + 0x2c, right);
                c.put32(at + 0x30, 0); c.put32(at + 0x34, 0);
                c.put16(at + 0x72, body); c.put16(at + 0x74, 0xffff);
                c.put16(display + 0x129da6, index + 1); invoke(0x107144c0, {grid_at(body),index});
                return index;
            };
            const auto base = append(label_source, 500,710), first = append(link_source,512,550), second = append(link_source,565,610);
            refresh();
            const auto view = c.u32(meta); c.put32(meta, (view & ~0x207fu) | 0x2000u);
            current_canvas = {1280,900};
            require(s.platform->resize_gui_canvas(s, current_canvas), "Controlled Report wide resize");
            layout_pass();
            require(c.i32(control_at(base) + 0x28) == 782 && c.i32(control_at(base) + 0x2c) == 1141 &&
                    c.i32(control_at(base) + 0x10) == 782 && c.i32(control_at(base) + 0x18) == 1141,
                    "Report text span did not expand to the independent 1280px golden bounds");
            for (const auto [index,left,right] : {std::array{first,794u,832u},std::array{second,847u,892u}})
                require(c.u32(control_at(index) + 0x28) == left && c.u32(control_at(index) + 0x2c) == right &&
                        c.u32(control_at(index) + 0x10) == left && c.u32(control_at(index) + 0x18) == right &&
                        hit(index) == int(index), "Report club link changed its width, payload or input rectangle");
            c.put32(meta, view); build();
        }
        // The UI selection flag is transient history, not a shortlist edit.
        // Exercise its original full-screen cancellation mask and row overlay
        // without dispatching action27, which would prompt to edit a list.
        phase = "selection";
        std::cout << "search preset " << preset << ": selection and filter transition" << std::endl;
        c.put32(setting(5), 1); resize_build({1280,900});
        const auto selection = find_action(27, visible_staff().front());
        require(selection != absent && c.i16(control_at(selection) + 0x72) == int(overlay) &&
                hit(selection) == int(selection), "Search selection overlay lost its displayed staff payload");
        const auto cancel_selection = find_action(1);
        require(cancel_selection != absent && rectangle(control_at(cancel_selection) + 0x10) == ScreenRect{0,0,1279,899},
                "Search selection cancellation mask does not cover the expanded canvas");
        const auto pointer_x = c.u32(c.toc(-0x5c10)), pointer_y = c.u32(c.toc(-0x5c14));
        const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
        c.put32(c.toc(-0x5c10), 1278); c.put32(c.toc(-0x5c14), 898);
        c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
        require(std::int16_t(invoke(0x1071a280, {display})) == int(cancel_selection),
                "Search selection cancellation misses the expanded corner");
        c.put32(c.toc(-0x5c10), pointer_x); c.put32(c.toc(-0x5c14), pointer_y);
        c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
        c.put32(setting(5), 0); build();
        phase = "filter modal";
        const auto filters = s.memory.copy_out(config, 0x29c), random = random_state(s);
        const auto ordered = results();
        const auto filter = find_action(19);
        require(hit(filter) == int(filter) && std::int32_t(invoke(0x10324100, {filter})) == -4,
                "Search Filter button did not open its original modal");
        const auto modal_history = c.u32(manager + 0x10);
        require(modal_history != history && c.u32(c.u32(modal_history)) == 0x10329fd0 &&
                adaptive_screen_available(s, 0x10329fd0), "Filter transition did not select its own form template");
        // The form has an independent recipe; opening it still copies filters.
        require(s.platform->resize_gui_canvas(s, {1280,900}), "Search filter resize");
        invoke(0x10719560, {display, 1, 0}); invoke(0x10329fd0, {}); refresh();
        const auto modal_meanings = meanings();
        layout_built_screen(s, display, 0x10329fd0);
        require(meanings() == modal_meanings && s.memory.copy_out(config, filters.size()) == filters &&
                results() == ordered && random_state(s) == random,
                "Opening the adaptive filter changed query/results/RNG");
        require(s.memory.copy_out(saved_presets_address, saved_presets.size()) == saved_presets,
                "Search fixture changed persistent recruitment settings");
        std::cout << "search " << label << ": " << views.size() << " primary views at 800/1280, General at "
                  << (recruitment ? 3 : 7) << " sizes, ordered pages, sort/input/menu, ABI/records/RNG and scope" << std::endl;
    }
    initialize_new_screens();
}

// Included inside adaptive_gui_checks.cpp's anonymous namespace. These are
// presentation fixtures, never a new saved career. The capture has no history
// archive files: temporarily mark only the selected person's archive as absent
// while the original builder runs, then restore the captured words. Career
// tables still come from the original current-season records and allocator.
// Populated achievements/overflow below use declared GUIO-only fixtures.
void player_history_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    for (bool goalkeeper : {false, true}) {
        Clock clock; clock.idle_input = true;
        auto state = std::make_unique<State>(); auto& s = *state;
        load_match_snapshot(s, path); s.code_fragment_observer = &clock;
        guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
        const auto manager = c.toc(-0x7bb0);
        c.put16(manager + 0x2c58, 0);
        const auto captured = c.u32(manager + 0x10);
        require(c.u32(c.u32(captured)) == 0x1001e2b0, "History fixture is not a captured club page");
        const auto club = c.u32(captured + 0x14);
        unsigned staff = 0;
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto person = c.u32(club + 0x98 + slot * 4);
            if (!person || c.u8(person + 0x62) != 11) continue;
            const auto player = c.u32(person + 0x14);
            if (player && (c.u8(player + 0xf) == 20) == goalkeeper) { staff = person; break; }
        }
        require(staff, "History capture lacks the requested outfield player/goalkeeper");
        const auto player = c.u32(staff + 0x14);
        unsigned active_view = 0;
        const char* phase = "open";
        unsigned current_grid = 0;
        GuiCanvas current_canvas{800,600};
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> values) {
            unsigned reg = 3; for (auto value : values) c.set(reg++, value);
            s.budget = s.executed + 400000000;
            try { c.call(entry, return_sentinel); }
            catch (const std::exception& error) {
                throw std::runtime_error(std::string("Player history view ") + std::to_string(active_view) +
                    (goalkeeper ? " GK " : " outfield ") + hex_address(entry) + " at " +
                    hex_address(s.pc) + " phase " + phase + " grid " + std::to_string(current_grid) +
                    " canvas " + std::to_string(current_canvas.width) + "x" + std::to_string(current_canvas.height) +
                    " LR " + hex_address(c.lr()) + ": " + error.what());
            }
            return c.r(3);
        };
        std::filesystem::create_directories(temp.path / "user" / "Files");
        s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
        require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
                "History baseline pixel globals");
        invoke(0x10389660, {staff, 5, 1, 0, 0});
        const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
        const auto setting = [&](unsigned slot) { return history + 0x14 + slot * 8; };
        const auto grid_at = [&](unsigned i) { return display + 4 + i * 0xbf0; };
        const auto control_at = [&](unsigned i) { return display + 0xba864 + i * 0x17c; };
        const auto rectangle = [&](unsigned at) {
            return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
        };
        require(c.u32(c.u32(history)) == 0x10389ce0 && c.u32(setting(0)) == staff &&
                c.u32(setting(1)) == 5, "Original opener did not select player History");
        const auto stats = c.u32(c.toc(-0x7b3c)), discipline = c.u32(c.toc(-0x7aa4));
        const auto stats_entry = c.u32(stats + 0x30) + c.u32(player) * 0x28;
        const auto discipline_row = c.u32(discipline + 0x14) + c.u32(staff) * 0x2c;
        const auto career_archive_size = c.u32(stats_entry + 0x1c);
        const auto ban_archive_size = c.u32(discipline_row + 0x24);
        const auto original_build = [&] {
            // 10749fd0 returns an empty history at size==-1 before file I/O.
            // Do not replace the reader or any career/statistics service.
            c.put32(stats_entry + 0x1c, 0xffffffffu);
            c.put32(discipline_row + 0x24, 0xffffffffu);
            try {
                invoke(0x10719560, {display, 1, 0});
                invoke(0x10389ce0, {});
            } catch (...) {
                c.put32(stats_entry + 0x1c, career_archive_size);
                c.put32(discipline_row + 0x24, ban_archive_size);
                throw;
            }
            c.put32(stats_entry + 0x1c, career_archive_size);
            c.put32(discipline_row + 0x24, ban_archive_size);
            // Original AutoGrid refresh resolves each popup's owning button
            // (+74). Compare/layout only this same completed original stage.
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                current_grid = i;
                invoke(0x10714630,{grid_at(i)}); invoke(0x10714110,{grid_at(i),0});
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
        const auto layout_pass = [&] {
            const auto registers = s.registers;
            const auto unique = s.unique;
            const auto pc = s.pc, stack_base = c.sp() - 0x8000;
            const auto stack = s.memory.copy_out(stack_base, 0x8100), rng = random_state(s);
            const auto before = meanings();
            std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> records;
            for (const auto [at, size] : {std::pair{staff,0x70u},std::pair{player,0x48u},
                    std::pair{club,0x248u},std::pair{stats_entry,0x28u},std::pair{discipline_row,0x2cu},
                    std::pair{history,0x14u + 22u * 8u}})
                records.emplace_back(at, s.memory.copy_out(at, size));
            const auto cache = c.u32(setting(6)), count = c.u32(setting(7));
            if (cache && count) records.emplace_back(cache, s.memory.copy_out(cache, count * 0x30));
            std::vector<std::pair<unsigned,unsigned>> scroll;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i)
                scroll.emplace_back(grid_at(i), c.u32(grid_at(i) + 0x1fc));
            layout_built_screen(s, display, 0x10389ce0);
            require(s.registers == registers && s.unique == unique && s.pc == pc &&
                    s.memory.copy_out(stack_base, stack.size()) == stack,
                    "History layout changed registers, unique bytes, PC or stack");
            require(meanings() == before && random_state(s) == rng,
                    "History layout changed text, style, action, payload, ownership or RNG");
            for (const auto& [at, bytes] : records)
                require(s.memory.copy_out(at, bytes.size()) == bytes, "History layout changed career records or history/cache state");
            for (const auto [at, offset] : scroll)
                require(c.u32(at + 0x1fc) == offset, "History layout changed an existing scroll offset");
        };
        const auto refresh = [&] {
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                invoke(0x10714630, {grid_at(i)}); invoke(0x10714110, {grid_at(i), 0});
            }
        };
        const auto hit = [&](unsigned index) {
            const auto at = control_at(index);
            const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
            const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
            c.put32(c.toc(-0x5c10), (c.u32(at + 0x10) + c.u32(at + 0x18)) / 2);
            c.put32(c.toc(-0x5c14), (c.u32(at + 0x14) + c.u32(at + 0x1c)) / 2);
            c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
            const auto result = std::int16_t(invoke(0x1071a280, {display}));
            c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
            c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
            return result;
        };
        constexpr unsigned absent = 0xffffffffu;
        const auto initial_view = c.u32(setting(5));
        for (unsigned candidate : {0u,1u,2u,3u,4u,8u,16u,32u}) {
            c.put32(setting(5),candidate);
            const bool supported = candidate == 1 || candidate == 2 || candidate == 4 || candidate == 8 || candidate == 16;
            require(adaptive_screen_available(s,0x10389ce0) == supported,
                    "History selector gate admitted an unsupported combination or excluded an original menu value");
        }
        c.put32(setting(5),initial_view);
        const auto original_status = c.u8(staff + 0x62);
        for (unsigned excluded_status : {1u,2u}) {
            c.put8(staff + 0x62,excluded_status);
            const bool excluded = !adaptive_screen_available(s,0x10389ce0);
            c.put8(staff + 0x62,original_status);
            require(excluded, "Unsupported staff status passed the History gate");
        }
        for (unsigned view : {1u,2u,4u,8u}) {
            active_view = view;
            phase = "warm";
            c.put32(setting(5), view); c.put32(setting(9), 0);
            require(s.platform->resize_gui_canvas(s, {800,600}), "Original History canvas");
            original_build(); original_build(); refresh();
            require(c.u32(setting(5)) == view && adaptive_screen_available(s, 0x10389ce0),
                    "Original History view was normalized or excluded unexpectedly");
            if (view == 2)
                require(c.u32(setting(6)) && c.u32(setting(7)) == 6,
                        "Controlled Career table did not contain six original current-season categories");
            const auto original_meanings = meanings();
            const auto grid_count = c.u16(display + 0x129da4), control_count = c.u16(display + 0x129da6);
            unsigned body = absent, summary = absent, header = absent, toolbar = absent, title = absent;
            unsigned position = absent, footer = absent, label_source = absent, link_source = absent;
            for (unsigned i = 0; i < grid_count; ++i) {
                const auto at = grid_at(i);
                const auto bounds = rectangle(at);
                if (bounds == ScreenRect{110,125,360,145}) toolbar = i;
                if (bounds.left == 110 && bounds.bottom == 379 && c.u8(at + 0xbb0) ==
                    (view == 2 ? 14 : view == 4 ? 4 : 3)) body = i;
                if (bounds == ScreenRect{110,384,780,510} && c.u8(at + 0xbb0) == 12) summary = i;
                if (view == 2 && bounds.left == 110 && bounds.top == 185 &&
                    c.u8(at + 0xbb0) == 12 && c.u8(at + 0xbb1) == 1) header = i;
                if (bounds == ScreenRect{100,555,790,590}) footer = i;
            }
            for (unsigned i = 0; i < control_count; ++i) {
                const auto at = control_at(i);
                const auto bounds = rectangle(at + 0x10);
                if (c.i16(at + 0x72) == -1 && bounds == ScreenRect{110,150,780,180}) title = i;
                if (c.i16(at + 0x72) == -1 && bounds == ScreenRect{110,515,780,545}) position = i;
                if (c.i16(at + 0x72) >= 0 && c.i32(at + 0x28) > 0) {
                    if (c.u32(at + 8) == 2005) link_source = i;
                    else if (!c.u32(at + 8) && c.u8(at + 0x78)) label_source = i;
                }
            }
            require(body != absent && summary != absent && toolbar != absent && title != absent &&
                    position != absent && footer != absent && label_source != absent && link_source != absent &&
                    (view != 2 || header != absent), "Original History controls or grids missing");
            const auto body_original = rectangle(grid_at(body)), summary_original = rectangle(grid_at(summary));
            const auto header_original = header == absent ? ScreenRect{} : rectangle(grid_at(header));
            const auto rows = unsigned(c.u8(grid_at(body) + 0xbb1));
            require(rows == (view == 2 ? 9 : 10) && c.u8(grid_at(summary) + 0xbb1) == 7 &&
                    body_original.top == (view == 2 ? 204 : 185), "History baseline capacity/header height changed");
            std::vector<unsigned> summary_panels;
            for (unsigned i = 0; i < grid_count; ++i)
                if (rectangle(grid_at(i)) == summary_original) summary_panels.push_back(i);
            for (const auto canvas : {GuiCanvas{800,600},GuiCanvas{801,601},GuiCanvas{822,680},
                                     GuiCanvas{1200,680},GuiCanvas{1280,900},GuiCanvas{3200,1800}}) {
                current_canvas = canvas; phase = "resized original";
                require(s.platform->resize_gui_canvas(s, canvas), "History resize failed");
                original_build();
                const auto rebuilt_meanings = meanings();
                if (rebuilt_meanings != original_meanings) {
                    std::cerr << "Original History semantic difference: view " << view << " GK " << goalkeeper
                              << " canvas " << canvas.width << 'x' << canvas.height << " counts "
                              << original_meanings.size() << '/' << rebuilt_meanings.size() << '\n';
                    for (unsigned i = 0; i < std::min(original_meanings.size(),rebuilt_meanings.size()); ++i) {
                        if (original_meanings[i] == rebuilt_meanings[i]) continue;
                        const auto print = [&](const Meaning& item) {
                            std::cerr << " action=" << item.action << " payload=" << item.payload << " kind=" << item.kind
                                      << " style=" << item.style << " alignment=" << item.alignment
                                      << " colors=" << item.foreground << ',' << item.background << " font=" << item.font
                                      << " size=" << item.size << " owner/popup=" << item.owner << ',' << item.popup
                                      << " cell=" << item.column << ',' << item.row << " text=" << item.text << '\n';
                        };
                        std::cerr << "control " << i << " baseline:"; print(original_meanings[i]);
                        std::cerr << "control " << i << " rebuilt:"; print(rebuilt_meanings[i]);
                        break;
                    }
                    throw std::runtime_error("Resized original History changed control semantics");
                }
                layout_pass(); refresh();
                const int extra = canvas.height - 600, shift = extra * rows / (rows + 7);
                require(rectangle(grid_at(body)) == ScreenRect{110,body_original.top,canvas.width-20,379+shift} &&
                        c.u8(grid_at(body) + 0xbb1) == rows && c.u8(grid_at(body) + 0xbb0) ==
                        (view == 2 ? 14 : view == 4 ? 4 : 3), "History body geometry/capacity changed");
                for (const auto i : summary_panels)
                    require(rectangle(grid_at(i)) == ScreenRect{110,384+shift,canvas.width-20,510+extra},
                            "History summary panels no longer align");
                require(rectangle(grid_at(toolbar)) == ScreenRect{110,125,360,145} &&
                        rectangle(control_at(title) + 0x10) == ScreenRect{110,150,canvas.width-20,180} &&
                        rectangle(control_at(position) + 0x10) == ScreenRect{110,515+extra,canvas.width-20,545+extra} &&
                        rectangle(grid_at(footer)) == ScreenRect{100,555+extra,canvas.width-10,590+extra},
                        "History toolbar/title/position/footer lost its anchor");
                if (header != absent)
                    require(rectangle(grid_at(header)) == ScreenRect{110,185,header_original.right+canvas.width-800,204},
                            "Career header changed its fixed height or scrollbar gutter");
                if (canvas == GuiCanvas{1280,900})
                    require(c.i32(grid_at(body) + 12) == (view == 2 ? 547 : 555) &&
                            c.i32(grid_at(summary) + 4) == (view == 2 ? 552 : 560) &&
                            c.i32(grid_at(summary) + 12) == 810, "History wide-canvas golden boundaries changed");
                require(c.u16(display + 0x129da4) == grid_count && c.u16(display + 0x129da6) == control_count &&
                        meanings() == original_meanings, "History layout added/removed controls or changed their meaning");
                // Verify every visible cell is inside its owner and all cells
                // on a row share its actual AutoGrid vertical interval.
                for (const auto owner : {body, summary}) {
                    std::array<int,30> top{}, bottom{}; std::array<bool,30> seen{};
                    const auto bounds = rectangle(grid_at(owner));
                    for (unsigned i = 0; i < control_count; ++i) {
                        const auto at = control_at(i);
                        if (c.i16(at + 0x72) != int(owner)) continue;
                        const auto row = c.u32(at + 0x24);
                        const auto cell = rectangle(at + 0x10);
                        require(row < 30 && cell.left >= bounds.left && cell.right <= bounds.right &&
                                cell.top >= bounds.top && cell.bottom <= bounds.bottom && cell.height() > 0,
                                "History cell escaped its visible owner");
                        if (seen[row]) require(top[row] == cell.top && bottom[row] == cell.bottom,
                                               "History cells on one row disagree vertically");
                        seen[row] = true; top[row] = cell.top; bottom[row] = cell.bottom;
                    }
                }
                if (view == 2) {
                    bool linked_club = false;
                    for (unsigned i = 0; i < control_count; ++i) {
                        const auto at = control_at(i);
                        if (c.i16(at + 0x72) == int(body) && c.u32(at + 8) == 2005) {
                            const auto payload = c.u32(at + 0x48);
                            require(hit(i) == int(i) && c.u32(at + 0x48) == payload,
                                    "Career club link lost its hit rectangle or payload");
                            linked_club = true;
                        }
                    }
                    require(linked_club, "Current-season Career table did not expose its original club link");
                }
                // Open the actual View/Filter popup. Never dispatch menu
                // entries or any career-changing action handler.
                unsigned menu = absent;
                for (unsigned i = 0; i < control_count; ++i)
                    if (c.i16(control_at(i) + 0x72) == int(toolbar) && c.u32(control_at(i) + 0x20) == 0 &&
                        c.i16(control_at(i) + 0x74) >= 0) { menu = i; break; }
                require(menu != absent && hit(menu) == -1 && c.u8(display) == 1 &&
                        c.i16(display + 0x12a930) == c.i16(control_at(menu) + 0x74),
                        "History View click did not open its original popup");
                if (view == 1) for (bool overflow : {false,true}) {
                    phase = overflow ? "overflow original" : "richtext original";
                    original_build();
                    phase = overflow ? "overflow clone" : "richtext clone";
                    const auto area = grid_at(body);
                    const auto append = [&](unsigned source, unsigned row, unsigned column, int left, int right) {
                        const auto index = unsigned(c.u16(display + 0x129da6)), at = control_at(index);
                        require(index < 1199, "Controlled History GUIO capacity exhausted");
                        s.memory.copy_in(at, s.memory.copy_out(control_at(source), 0x17c));
                        // 10725d44/d60 initialize these owned bitmap/text
                        // caches to zero; 10726a50 frees them. A rendered
                        // source must not lend its cache ownership to a clone.
                        c.put32(at + 0x4c,0); c.put32(at + 0x5c,0);
                        c.put16(at + 0x68,index); c.put16(at + 0x72,body); c.put16(at + 0x74,0xffff);
                        c.put32(at + 0x20,column); c.put32(at + 0x24,row);
                        c.put32(at + 0x28,left); c.put32(at + 0x2c,right);
                        c.put32(at + 0x30,0); c.put32(at + 0x34,0);
                        c.put32(at + 0x3c,1); // GUIO alignment emitted by 1074bcd0's mode two.
                        c.put16(display + 0x129da6,index + 1); invoke(0x107144c0,{area,index});
                        return index;
                    };
                    unsigned last_cell = absent;
                    for (unsigned i = 0; i < control_count; ++i)
                        if (c.i16(control_at(i) + 0x72) == int(body) && c.u32(control_at(i) + 0x24) == rows-1 &&
                            c.u32(control_at(i) + 0x20) == 2) last_cell = i;
                    require(last_cell != absent, "Controlled History fixture lacks last-row source");
                    const int right = overflow ? 758 : 780, left = 110 + (right-110)*8/23;
                    const auto label = append(label_source,rows-1,2,left,right);
                    const auto link = append(link_source,rows-1,2,left+12,left+62);
                    if (overflow) for (unsigned row = rows; row <= rows+8; ++row)
                        append(last_cell,row,2,0,0);
                    invoke(0x10714630,{area}); invoke(0x107145f0,{area,overflow ? 3u : 0u});
                    invoke(0x10714630,{area}); invoke(0x10714110,{area,0});
                    const auto old_column = c.i32(area) + c.i32(area + 0x24);
                    require(c.u32(area + 0x1fc) == (overflow ? 3u : 0u) && bool(c.u8(area + 0xbef)) == overflow,
                            "Controlled History scrollbar fixture did not initialize");
                    // The original Achievements builder adds this standalone
                    // group only above 100 records. Call its actual helper to
                    // cover the same three controls without inventing events.
                    // Page one also exercises the disabled previous button.
                    const auto first_page_control = unsigned(c.u16(display + 0x129da6));
                    invoke(0x1074b270,{780,125,overflow ? 1u : 2u,11,0,33,34,0});
                    require(c.u16(display + 0x129da6) == first_page_control + 3,
                            "Original History pagination helper did not publish three controls");
                    std::vector<ScreenRect> page_bounds;
                    for (unsigned i = first_page_control; i < first_page_control+3; ++i) {
                        require(c.i16(control_at(i) + 0x72) == -1, "Pagination control unexpectedly owns a grid");
                        page_bounds.push_back(rectangle(control_at(i) + 0x10));
                    }
                    layout_pass();
                    const auto delta = c.i32(area) + c.i32(area + 0x24) - old_column;
                    const auto row = rectangle(control_at(last_cell) + 0x10);
                    const auto expected_right = canvas.width-20-(overflow ? 22 : 0);
                    require(rectangle(control_at(label) + 0x10) == ScreenRect{left+delta,row.top,expected_right,row.bottom} &&
                            rectangle(control_at(link) + 0x10) == ScreenRect{left+12+delta,row.top,left+62+delta,row.bottom},
                            "Achievements rich text/link lost column alignment, gutter or scrolled Y");
                    require(c.u32(control_at(link) + 8) == 2005 &&
                            c.u32(control_at(link) + 0x48) == c.u32(control_at(link_source) + 0x48) &&
                            hit(link) == int(link), "Achievements link lost its original payload/hit area");
                    if (canvas == GuiCanvas{1280,900})
                        require(delta == 167, "Achievements rich text lost known wide-canvas column shift");
                    unsigned page_buttons = 0;
                    for (unsigned i = first_page_control; i < first_page_control+3; ++i) {
                        const auto bounds = page_bounds[i-first_page_control];
                        require(rectangle(control_at(i) + 0x10) == ScreenRect{
                            bounds.left+canvas.width-800,bounds.top,bounds.right+canvas.width-800,bounds.bottom},
                            "History pagination button/counter did not follow the right edge");
                        const auto action = c.u32(control_at(i) + 8);
                        if (action == 33 || action == 34) {
                            require(hit(i) == int(i), "History pagination button lost its hit rectangle");
                            ++page_buttons;
                        }
                    }
                    require(page_buttons == (overflow ? 1u : 2u),
                            "History pagination enabled-state semantics changed");
                }
                if (view == 2) {
                    // A private display cache, not twelve invented saved
                    // seasons: repeat the six captured current-season rows.
                    // The original builder consumes complete six-row groups
                    // (103a5574/5bc8); 72 rows force its 760px header branch.
                    // Restore ownership before any opener/cleanup can free it.
                    phase = "controlled Career cache";
                    const std::array<std::pair<unsigned,unsigned>,4> saved_settings{{
                        {setting(6),c.u32(setting(6))},{setting(7),c.u32(setting(7))},
                        {setting(8),c.u32(setting(8))},{setting(9),c.u32(setting(9))}}};
                    require(saved_settings[0].second && saved_settings[1].second == 6,
                            "Career overflow fixture lacks original six-row source");
                    const auto one_group = s.memory.copy_out(saved_settings[0].second,6*0x30);
                    const auto private_cache = allocate(s,72*0x30,true);
                    for (unsigned group = 0; group < 12; ++group)
                        s.memory.copy_in(private_cache+group*6*0x30,one_group);
                    const auto cache_bytes = s.memory.copy_out(private_cache,72*0x30);
                    const auto restore = [&] {
                        for (const auto [at,value] : saved_settings) c.put32(at,value);
                    };
                    try {
                        c.put32(setting(6),private_cache); c.put32(setting(7),72);
                        c.put32(setting(8),0); c.put32(setting(9),3);
                        original_build();
                        const auto area = grid_at(body), head = grid_at(header);
                        require(rectangle(head) == ScreenRect{110,185,760,204} &&
                                rectangle(area) == ScreenRect{110,204,780,379} && c.u8(area+0xbb1) == 9 &&
                                c.u8(area+0xbef) && c.u32(area+0x1fc) == 3 && c.i32(area+0x14) >= 12,
                                "Original Career cache did not exercise its header gutter and scroll branch");
                        layout_pass();
                        require(rectangle(head) == ScreenRect{110,185,canvas.width-40,204} &&
                                rectangle(area) == ScreenRect{110,204,canvas.width-20,379+shift} &&
                                c.u32(area+0x1fc) == 3 && c.u8(area+0xbb1) == 9 &&
                                s.memory.copy_out(private_cache,cache_bytes.size()) == cache_bytes,
                                "Career overflow layout changed the header gutter, scroll or borrowed cache");
                        bool visible_link = false;
                        for (unsigned i = 0; i < c.u16(display+0x129da6); ++i) {
                            const auto at = control_at(i);
                            if (c.i16(at+0x72) != int(body) || c.u32(at+8) != 2005 || c.u32(at+0x24) != 3) continue;
                            const auto payload = c.u32(at+0x48);
                            require(hit(i) == int(i) && c.u32(at+0x48) == payload,
                                    "Scrolled Career cache club link lost its action payload/hit rectangle");
                            visible_link = true;
                        }
                        require(visible_link,"Career cache fixture did not expose a scrolled visible club link");
                    } catch (...) { restore(); throw; }
                    restore();
                }
                phase = "disabled original"; original_build(); select_new_screens(false);
                const auto untouched = s.memory.copy_out(display,0x12aa00);
                require(!adaptive_screen_available(s,0x10389ce0), "Disabled preference still admits History");
                layout_pass();
                require(s.memory.copy_out(display,0x12aa00) == untouched, "Disabled History layout is not a true no-op");
                select_new_screens(true);
                require(c.u32(stats_entry + 0x1c) == career_archive_size &&
                        c.u32(discipline_row + 0x24) == ban_archive_size, "History fixture did not restore captured archive sizes");
            }
            std::cout << "Player History " << (goalkeeper ? "GK" : "outfield") << " view " << view
                      << ": six sizes, original actions/styles, ABI/state and off gate passed\n";
        }
    }
    std::cout << "History archive fixtures: Career uses captured current-season records; Bans is explicitly empty. "
                 "Populated Achievements/overflow are GUIO-only; Manager Stats is source-audited, not built.\n";
    initialize_new_screens();
}

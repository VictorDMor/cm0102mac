// The six message screens (offer decision from the news, transfer completion, transfer-list confirmation,
// an offer's restriction and feedback messages, staff release), opened through their original openers on
// real records of an existing capture. Built with New Screens off and on; the adaptive run must keep every
// control, action, record and RNG draw, and make a dialog of it: a panel centered under the title bars, the
// message centered in it, equal buttons on one row inside the panel's horizontal bounds, right-aligned to it.
// The manager's three confirmations (apply for a job, rescind the contract, retire) are message screens whose
// panel is 110,145-780,535: they are opened through their own openers and answer No (action 1) like the others.
// The transfer dialogs of TRANSFER_SCREENS are message screens too, each opened through its original opener with
// the arguments its callers pass: remove the exchange player from a proposal (a link and a person ID), add a player
// to the shortlist or set one to compare (a person and a mode), withdraw a bid (a link), release a player, end a
// loan early (a person), and the trial period selection (a person; its seven-column row of controls is the one
// panel that is not a single column, tried at its shortest, middle and longest periods).
// FILE_SCREENS' generic message box (render 0x100f08c0, opened by 0x100f05a0 from the menu bar, the file screens, the setup
// and the search screens) and the confirmation before a match (render 0x1029ad50, opener 0x1029ab60, "Proceed to the
// match ?", opened by 0x1029a680 when the manager plays) are message screens too: a wrapped message in a one-column grid
// at 110,145-780,535 and a No/Yes table or a standalone Ok at 100,555-790,590. Their scenarios open the box through its original
// opener with the arguments the menu bar passes (quit has parent value 0xb, restart 0x3d), with short and long texts, with
// and without a career, and answer No (1) or Ok (3); the line-up confirmation answers No (2). Yes is never exercised: quit's
// calls exit and the line-up's starts the match.
// Needs TransferScene from adaptive_transfer_status_checks.hpp.
void message_screens_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 52;
    const char* names[scenarios]{"offer decision", "completion", "listing", "listing (loan)", "restriction", "feedback", "staff release", "apply for job", "rescind contract", "retire",
        "remove exchange player", "shortlist add", "shortlist remove", "compare first", "compare second", "withdraw bid", "release", "release (no contract)", "trial 2 weeks", "trial 1 week", "trial 4 weeks", "loan recall",
        "scout player", "two players needed", "Intertoto place", "job offer", "transfer request", "invalid squad (national team)", "invalid squad (under 21)", "friendly tournament invitation",
        "disk full", "no memory", "FA Cup round", "take control of a club", "reassume control", "appeal against punishment", "ultimatum to the board", "national squad: remove from the main squad", "national squad: remove from the reserve squad", "national squad: first-team limit", "national squad: Under-21 limit", "special holiday for a player", "counsellor for a player",
        "quit (Exit the game)", "restart (Restart the game)", "quit without a career", "notice (Ok)", "notice without a career (Ok)", "long question", "long notice (Ok)", "very long question", "confirmation before the match"};
    const unsigned builders[scenarios]{0x10421fb0, 0x10423270, 0x1042a810, 0x1042a810, 0x10420b50, 0x10421070, 0x1009e8f0, 0x101c8730, 0x101c8b60, 0x101c98c0,
        0x10422b50, 0x10424f80, 0x10424f80, 0x104249e0, 0x104249e0, 0x104265c0, 0x1042ae90, 0x1042ae90, 0x1042b580, 0x1042b580, 0x1042b580, 0x1042f7c0, 0x103ac810, 0x103acc50, 0x10075410, 0x101c94e0, 0x10093bd0, 0x102bd090, 0x102bd920, 0x10049550,
        0x10340db0, 0x10341120, 0x104c65a0, 0x1033bdc0, 0x1033c970, 0x106fcbb0, 0x10702ba0, 0x102bc400, 0x102bc400, 0x102bc400, 0x102bc400, 0x10702580, 0x10702580,
        0x100f08c0, 0x100f08c0, 0x100f08c0, 0x100f08c0, 0x100f08c0, 0x100f08c0, 0x100f08c0, 0x100f08c0, 0x1029ad50};
    // The control that answers No or Back: dismisses the screen without touching a record (0: none exercised).
    const unsigned dismissals[scenarios]{1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 2, 2, 2, 11, 1, 1, 1, 3, 3, 3, 1, 54, 56, 1, 1, 34, 1, 1, 1, 51, 52, 2, 32, 37, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 3, 3, 1, 3, 1, 2};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                t.rows_may_move = true;
                // A bidding club of the same nation, and an offer of it for the managed club's player.
                unsigned other = 0;
                const auto clubs = c.u32(c.toc(-0x7be8));
                for (unsigned k = 0; k < c.u32(c.toc(-0x7b50)) && !other; ++k) {
                    const auto club = clubs + k * 0x248;
                    if (club != t.managed && c.u32(club + 0x30) == c.u32(t.managed + 0x30)) other = club;
                }
                require(other, "Message fixture needs a second club");
                const auto offer = allocate(t.s, 0x54, true), link = allocate(t.s, 4, true);
                t.invoke(0x10411ea0, {offer, t.person, other, 0, 0xffffffffu, 1});
                c.put32(offer + 0x28, 0xffffffffu); c.put32(offer + 0x2c, 0); c.put32(link, offer);
                switch (scenario) {
                case 0:
                    t.invoke(0x1073e4b0, {t.manager, c.toc(-0x4af8), c.toc(-0x4afc), 1, 0, 0});
                    t.set_value(0, t.instance); t.set_value(1, 0); t.set_value(2, link); break;
                case 1: t.invoke(0x10423160, {link, t.instance, 0}); break;
                case 2: case 3: t.invoke(0x1042a780, {t.person, scenario == 3 ? 1u : 0u}); break;
                case 4: t.invoke(0x10420aa0, {link, 35, 5}); break;
                case 5: t.invoke(0x10420f00, {t.person, other, 1, 0}); break;
                case 6: t.invoke(0x1009e880, {t.person}); break;
                case 7: t.invoke(0x101c8650, {other}); break;
                case 8: t.invoke(0x101c8a80, {t.managed}); break;
                case 9: t.invoke(0x101c9880, {}); break;
                case 10: t.invoke(0x10422ac0, {link, c.u32(t.person)}); break;
                case 11: case 12: t.invoke(0x104248a0, {t.person, scenario == 11 ? 1u : 0u}); break;
                case 13: case 14: t.invoke(0x10424930, {t.person, 0, scenario == 13 ? 1u : 0u}); break;
                case 15: t.invoke(0x10426550, {link}); break;
                case 16: t.invoke(0x1042ae20, {t.person}); break;
                case 17: c.put8(t.contract + 0x38, c.u8(t.contract + 0x38) & 3); t.invoke(0x1042ae20, {t.person}); break;
                case 18: case 19: case 20: t.invoke(0x1042b500, {t.person}); t.set_value(1, scenario == 18 ? 2 : scenario == 19 ? 1 : 4); break;
                case 22: t.invoke(0x103ac780, {t.person, 0}); break;
                case 23: t.invoke(0x103acc10, {t.person}); break;
                case 24: {   // a news item naming the managed club (its index in the club array)
                    const auto news = t.news_item(6000, {0, (t.managed - c.u32(c.toc(-0x7be8))) / 0x248});
                    t.invoke(0x10075380, {t.instance, news}); break;
                }
                case 25: {   // a job offer from another club, named by its index in the club array
                    const auto news = t.news_item(6000, {(other - c.u32(c.toc(-0x7be8))) / 0x248});
                    t.invoke(0x101c9350, {t.instance, news}); break;
                }
                case 26: {   // the managed club's player asks for a transfer: club index, an unused word and the player's ID
                    const auto news = t.news_item(6000, {(t.managed - c.u32(c.toc(-0x7be8))) / 0x248, 0, c.u32(t.person)});
                    t.invoke(0x10093ae0, {t.instance, news}); break;
                }
                case 27: case 28: {   // an invalid squad notice of an international side: the club's index (word 0, or word 4 of the second kind)
                    const auto index = (other - c.u32(c.toc(-0x7be8))) / 0x248;
                    const auto news = scenario == 27 ? t.news_item(6000, {index, 0, 0, 0, 0, 0}) : t.news_item(6000, {0, 0, 0, 0, index, 0});
                    t.invoke(scenario == 27 ? 0x102bcec0 : 0x102bd7f0, {t.instance, news}); break;
                }
                case 29: {   // an invitation by another club to a tournament slot
                    const auto news = t.news_item(6000, {0, (other - c.u32(c.toc(-0x7be8))) / 0x248, 0});
                    t.invoke(0x100493c0, {t.instance, news}); break;
                }
                case 30: t.invoke(0x10340d40, {}); break;
                case 31: t.invoke(0x103410b0, {}); break;
                case 32: {   // entering the English cup in a later round: the news item and the competition the news handler left in its global
                    const auto news = t.news_item(6000, {0});
                    t.invoke(0x104c6510, {t.instance, news}); break;
                }
                case 33: t.invoke(0x1033bd50, {other}); break;
                case 34: t.invoke(0x1033c900, {t.instance}); break;
                case 35: {   // a player's appeal against his punishment: an item with every payload word zero (the first player)
                    const auto news = t.news_item(6000, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
                    t.invoke(0x106fc7f0, {t.instance, news}); break;
                }
                case 36: {   // the ultimatum to the board
                    const auto news = t.news_item(6000, {0, 0, 0, 0});
                    t.invoke(0x10702a40, {t.instance, news}); break;
                }
                case 37: case 38: case 39: case 40: {   // the national team squad: the manager has the international side (instance +0x20); a player of it, the squad (0 main, 1 reserve, 2 Under-21) and whether it is the limit notice
                    c.put32(t.instance + 0x20, t.managed);
                    const unsigned squad[4]{0, 1, 0, 2}, limit[4]{0, 0, 1, 1};
                    t.invoke(0x102bc340, {t.person, squad[scenario - 37], limit[scenario - 37]}); break;
                }
                case 41: case 42: t.invoke(0x10702470, {t.person, scenario == 41 ? 1u : 0u}); break;   // the player's action menu: a special holiday (1) or a spiritual counsellor (0) for him
                case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50: {
                    // The generic box, as the menu bar opens it: the heading, "Please Confirm", the body, Yes/No (1) or Ok (0), the colours and the parent value.
                    const auto text = [&](const std::string& value) {
                        const auto at = allocate(t.s, unsigned(value.size()) + 1, true);
                        for (std::size_t k = 0; k < value.size(); ++k) c.put8(at + unsigned(k), std::uint8_t(value[k]));
                        return at;
                    };
                    const std::string paragraph = "The directors have asked for a meeting about the squad, the budget and the plans for the coming weeks, and want the answer before the transfer window closes. ";
                    const char* bodies[8]{"Exit the game ?", "Restart the game ?", "Exit the game ?", "The game cannot be saved while a player is on a response screen.", "The saved game could not be saved.", nullptr, nullptr, nullptr};
                    std::string body = bodies[scenario - 43] ? bodies[scenario - 43] : "";
                    if (scenario == 48) { for (int k = 0; k < 2; ++k) body += paragraph; body += "Do you agree ?"; }
                    if (scenario == 49) { for (int k = 0; k < 4; ++k) body += paragraph; }
                    if (scenario == 50) { for (int k = 0; k < 5; ++k) body += paragraph + "\n"; body += "Do you agree ?"; }
                    if (scenario == 45 || scenario == 47) c.put32(t.manager + 4, 0);
                    const unsigned parent[8]{0xb, 0x3d, 0xb, 0, 0, 0x3d, 0, 0x3d};
                    const bool question = scenario != 46 && scenario != 47 && scenario != 49;
                    const auto heading = t.invoke(0x10355cf0, {});
                    const auto words = text("Please Confirm"), message = text(body);
                    const auto ink = t.invoke(0x10355d30, {1}), paper = t.invoke(0x10355d30, {0});
                    t.invoke(0x100f05a0, {heading, words, message, question ? 1u : 0u, ink, paper, parent[scenario - 43]});
                    break;
                }
                case 51: {   // the confirmation before the match: the human manager's slot holds the match record MATCH_DAY leaves there (the home club in +0x20, the away club in +0x24, home in +0x44, the competition in +0x18, the substitutes in +0x52)
                    const auto slot = t.invoke(0x102aa3d0, {0});
                    require(slot, "Line-up fixture needs the human manager's slot");
                    const auto record = allocate(t.s, 0x200, true), competition = allocate(t.s, 0x100, true);
                    c.put32(record + 0x20, t.managed); c.put32(record + 0x24, other); c.put8(record + 0x44, 1); c.put32(record + 0x18, competition); c.put8(record + 0x52, 0x50);
                    c.put32(slot + 0xec, record); c.put32(slot + 0xf0, t.managed);
                    t.invoke(0x1029ab60, {});
                    break;
                }   // the confirmation before the match, as the manager's next match opens it
                default: t.invoke(0x1042f670, {t.person}); break;
                }
                require(t.builder() == builders[scenario], "Message screen did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records({{offer, 0x54}, {link, 4}}); };
                struct Shape { int message = -1, footer = -1; std::vector<unsigned> footer_controls; };
                const auto shape = [&] {
                    Shape r;
                    for (unsigned i = 0; i < t.grids(); ++i) {
                        const auto box = t.rect(t.grid(i)); const auto columns = c.u8(t.grid(i) + 0xbb0);
                        if ((box == ScreenRect{110, 140, 780, 545} || box == ScreenRect{110, 145, 780, 535}) && (columns == 1 || columns == 7)) r.message = int(i);
                        if (box == ScreenRect{100, 555, 790, 590} && c.u8(t.grid(i) + 0xbb1) == 1) r.footer = int(i);
                    }
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if ((r.footer >= 0 && owner == r.footer) || (owner < 0 && (t.rect(t.control(i) + 0x10) == ScreenRect{100, 555, 790, 590} || t.rect(t.control(i) + 0x10) == ScreenRect{110, 555, 780, 590})))
                            r.footer_controls.push_back(i);
                    }
                    return r;
                };
                const auto pick = [&] {
                    const auto r = shape();
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        const bool ours = owner == r.message || std::find(r.footer_controls.begin(), r.footer_controls.end(), i) != r.footer_controls.end();
                        if (ours && c.u32(t.control(i) + 8) && t.enabled(i)) require(t.hit(i) == int(i), "Message picker missed an original control");
                    }
                };
                // Every property the layout promises, read from the guest after it ran.
                const auto verify = [&](GuiCanvas size, const Shape& before, const std::vector<std::uint8_t>& old) {
                    const auto panel = t.rect(t.grid(before.message));
                    const ScreenLayout shell{size};
                    const auto title = shell.title();
                    // Panel: a bounded dialog centered under the title bars, in the upper half of the screen.
                    require(panel != ScreenRect{110, 140, 780, 545} && panel != ScreenRect{110, 145, 780, 535}, "Message recipe did not run");
                    require(std::abs((panel.left + panel.right) - (title.left + title.right)) <= 2, "Message panel is not centered under the title bars");
                    require(panel.left >= 110 && panel.width() <= 800 && panel.right <= size.width - 10 && panel.top >= 140 && panel.bottom < size.height - 10,
                            "Message panel outside the content area");
                    // A message too long for the upper half starts where the free space's third puts it: at the top of the content.
                    const auto content = shell.content(140);
                    require(panel.top + panel.height() / 2 <= size.height / 2 ||
                            panel.top <= content.top + 5 + std::max(0, (content.height() - panel.height() - 45) / 3), "Message dialog not in the upper half");
                    // Height: the message's rows and a row of padding on each side, no more.
                    unsigned first = 99, last = 0;
                    int text_top = 100000, text_bottom = -100000;
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const auto at = t.control(i);
                        if (c.i16(at + 0x72) != before.message) continue;
                        const auto row = c.u32(at + 0x24); first = std::min(first, row); last = std::max(last, row);
                        const auto box = t.rect(at + 0x10);
                        require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom,
                                "Message text outside its panel");
                        text_top = std::min(text_top, box.top); text_bottom = std::max(text_bottom, box.bottom);
                        // The rows share the panel evenly: a line that lost its row is squeezed to nothing (a message as long as the
                        // grid's built rows needs its padding rows shown).
                        require(box.height() >= 14, "Message line squeezed out of its row");
                        // Text keeps its width and the center it had in the original panel's center column.
                        const unsigned at_old = 0xba864 + i * 0x17c;
                        const auto old_left = display_word(old, at_old + 0x28), old_right = display_word(old, at_old + 0x2c);
                        if (old_left && old_right && !c.u32(at + 8)) {
                            const int original_offset = (old_left + old_right) - (110 + 780);
                            require(std::abs((c.i32(at + 0x28) + c.i32(at + 0x2c)) - (panel.left + panel.right) - original_offset) <= 2,
                                    "Message text is not centered in the panel");
                            require(c.i32(at + 0x2c) - c.i32(at + 0x28) == old_right - old_left, "Message text changed width");
                        }
                    }
                    require(first <= last, "Message has no rows");
                    if (c.u8(t.grid(before.message) + 0xbb0) == 7) {
                        // The trial period's cells: in column order, apart from each other, only the widest grew.
                        struct Cell { unsigned column; ScreenRect box; int old_width; };
                        std::vector<Cell> cells;
                        for (unsigned i = 0; i < t.controls(); ++i) {
                            const auto at = t.control(i);
                            if (c.i16(at + 0x72) != before.message) continue;
                            const unsigned at_old = 0xba864 + i * 0x17c;
                            cells.push_back({c.u32(at + 0x20), t.rect(at + 0x10), display_word(old, at_old + 0x18) - display_word(old, at_old + 0x10)});
                        }
                        std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.column < b.column; });
                        require(cells.size() >= 3, "Message cells missing");
                        int widest = 0; for (const auto& cell : cells) widest = std::max(widest, cell.old_width);
                        for (std::size_t i = 0; i < cells.size(); ++i) {
                            if (i) require(cells[i - 1].box.right <= cells[i].box.left, "Message cells overlap or are out of order");
                            const int grown = cells[i].box.width() - cells[i].old_width;
                            require(cells[i].old_width == widest ? grown >= 0 : std::abs(grown) <= 1, "Message cell width changed besides the widest");
                        }
                    }

                    // Rows of 28 pixels, or down to 21 when a long message would push the buttons off the canvas.
                    const int rows_needed = int(std::max(last - first + 3, 5u));
                    require(panel.height() <= rows_needed * 28 + 16 && panel.height() >= rows_needed * 21 + 16 &&
                            (panel.height() == rows_needed * 28 + 16 || rows_needed * 28 + 16 + 45 > shell.content(145).height()),
                            "Message panel not bounded to its message");
                    require(std::abs((text_top + text_bottom) - (panel.top + panel.bottom)) <= 28, "Message not vertically centered in its panel");
                    // Buttons, Back included: one row, equal size, inside the panel's bounds, right-aligned to it.
                    if (!before.footer_controls.empty()) {
                        int right_edge = -1, left_edge = 100000;
                        const auto first_box = t.rect(t.control(before.footer_controls.front()) + 0x10);
                        for (const auto i : before.footer_controls) {
                            const auto box = t.rect(t.control(i) + 0x10);
                            require(box.top == first_box.top && box.bottom == first_box.bottom, "Message buttons are not on one row");
                            require(std::abs(box.width() - first_box.width()) <= 2 && box.width() >= 60, "Message buttons are unequal or too small");
                            require(box.left >= panel.left && box.right <= panel.right, "Message button outside the panel's horizontal bounds");
                            right_edge = std::max(right_edge, box.right); left_edge = std::min(left_edge, box.left);
                        }
                        require(right_edge == panel.right && left_edge >= panel.left, "Message buttons not right-aligned to the panel");
                        require(first_box.top >= panel.bottom && first_box.bottom <= size.height - 10, "Message buttons not under the panel");
                        for (const auto i : before.footer_controls) for (const auto j : before.footer_controls)
                            if (i < j) {
                                const auto a = t.rect(t.control(i) + 0x10), b = t.rect(t.control(j) + 0x10);
                                require(a.right <= b.left || b.right <= a.left, "Message buttons overlap");
                            }
                    }
                };
                Shape last_before;
                std::vector<std::uint8_t> last_old;
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Message gate disagrees with preference");
                    t.rebuild(size);
                    const auto before = shape();
                    require(before.message >= 0, "Original message panel absent");
                    const auto old = t.layout(records, "Message screen");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy message screen changed"); pick(); return; }
                    {   // A message whose rows cannot keep about 21 pixels on this canvas, or with more than 30 rows, stays as the builder made it.
                        unsigned first = 99, last = 0;
                        for (unsigned i = 0; i < t.controls(); ++i) {
                            const auto at = t.control(i);
                            if (c.i16(at + 0x72) == before.message) { first = std::min(first, c.u32(at + 0x24)); last = std::max(last, c.u32(at + 0x24)); }
                        }
                        const int count = int(std::max(last - first + 3, 5u));
                        const ScreenLayout shell{size};
                        const int top = display_word(old, 4 + unsigned(before.message) * 0xbf0 + 4);
                        const bool footer = !before.footer_controls.empty();
                        if (count > 30 || (shell.content(top).height() - 16 - (footer ? 45 : 0)) / count < 21) {
                            require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Message too long for the canvas was not left as built");
                            pick(); return;
                        }
                    }
                    verify(size, before, old);
                    last_before = before; last_old = old;
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1360, 786} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) for (const auto size : {GuiCanvas{800, 600}, {801, 601}, {800, 900}, {1440, 600}, {3200, 1800}, {1280, 900}, canvas}) build(size);
                // The original in-place refresh keeps the layout's bounds.
                if (adaptive) {
                    std::vector<std::uint8_t> boxes;
                    for (unsigned k = 0; k < t.controls(); ++k) { const auto part = t.s.memory.copy_out(t.control(k) + 0x10, 16); boxes.insert(boxes.end(), part.begin(), part.end()); }
                    for (unsigned k = 0; k < t.grids(); ++k) { t.invoke(0x10714630, {t.grid(k)}); t.invoke(0x10714110, {t.grid(k), 0}); }
                    std::vector<std::uint8_t> again;
                    for (unsigned k = 0; k < t.controls(); ++k) { const auto part = t.s.memory.copy_out(t.control(k) + 0x10, 16); again.insert(again.end(), part.begin(), part.end()); }
                    require(again == boxes, "Message redraw lost the layout's control bounds");
                    // Mutants: each geometric promise, broken in the guest, must be refused by the same checker.
                    const auto check = [&] { verify(canvas, last_before, last_old); };
                    const auto saved_display = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved_display); };
                    const auto refused = [&](const char* mutant) {
                        bool rejected = false;
                        try { check(); } catch (const std::runtime_error&) { rejected = true; }
                        require(rejected, (std::string("Message checker accepted mutant: ") + mutant).c_str());
                        restore();
                    };
                    check();
                    {   // the panel stays at its classic rectangle
                        const auto grid = t.grid(last_before.message);
                        const bool confirmation = (scenario >= 7 && scenario <= 9) || scenario >= 24;
                        c.put32(grid, 110); c.put32(grid + 4, confirmation ? 145 : 140); c.put32(grid + 8, 780); c.put32(grid + 12, confirmation ? 535 : 545);
                        refused("panel left at classic coordinates");
                    }
                    if (!last_before.footer_controls.empty()) {   // the last button drifts right
                        const auto at = t.control(last_before.footer_controls.back());
                        c.put32(at + 0x10, c.i32(at + 0x10) + 5); c.put32(at + 0x18, c.i32(at + 0x18) + 5);
                        refused("button drifted");
                    }
                    if (!last_before.footer_controls.empty()) {   // the first button climbs into the panel
                        const auto at = t.control(last_before.footer_controls.front());
                        const auto panel = t.rect(t.grid(last_before.message));
                        c.put32(at + 0x14, panel.bottom - 5);
                        refused("button inside the panel");
                    }
                    {   // the message text moves off the panel's center
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done && (scenario < 18 || scenario > 20); ++k) {
                            const auto at = t.control(k);
                            if (c.i16(at + 0x72) != last_before.message || c.u32(at + 8) || !c.i32(at + 0x28) || !c.i32(at + 0x2c)) continue;
                            c.put32(at + 0x28, c.i32(at + 0x28) + 30); c.put32(at + 0x2c, c.i32(at + 0x2c) + 30); done = true;
                        }
                        if (!done) {   // the trial period's cells: the plus button leaves the panel
                            for (unsigned k = 0; k < t.controls(); ++k) {
                                const auto at = t.control(k);
                                if (c.i16(at + 0x72) != last_before.message || c.u32(at + 0x20) != 4) continue;
                                c.put32(at + 0x10, c.i32(at + 0x10) + 500); c.put32(at + 0x18, c.i32(at + 0x18) + 500); done = true;
                            }
                        }
                        require(done, "Message fixture found no text to mutate");
                        refused("text off the panel's center");
                    }
                    check();
                }
                if (adaptive) { on = false; select_new_screens(false); build(canvas); }
                // Safe dismissals through the original handlers: records unchanged, same result in both runs.
                std::vector<std::uint8_t> outcome;
                const auto saved = records();
                const unsigned back = dismissals[scenario];
                if (back) {
                    const int i = t.find(back);
                    require(i >= 0 && t.hit(i) == i, "Message dismissal control missing");
                    const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(i)}));
                    // The answers to news-driven questions may answer the news (decline the job, say no to the request): their
                    // effect is part of the outcome compared between the original and the adaptive run instead.
                    require(scenario >= 24 || records() == saved, "Message dismissal changed records");
                    for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
                    // The news questions read the item and may mark it or change the person: those effects must be the same too.
                    if (scenario >= 24) {
                        const auto queue = t.news_queue(), after = records();
                        outcome.insert(outcome.end(), queue.begin(), queue.end()); outcome.insert(outcome.end(), after.begin(), after.end());
                    }
                }
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive message screen changed the original command outcome");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Message screen ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "message screen " << scenario << " (" << names[scenario] << "): dialog geometry, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

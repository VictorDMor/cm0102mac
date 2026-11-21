// The six message screens (offer decision from the news, transfer completion, transfer-list confirmation,
// an offer's restriction and feedback messages, staff release), opened through their original openers on
// real records of an existing capture. Built with New Screens off and on; the adaptive run must keep every
// control, action, record and RNG draw, and make a dialog of it: a panel centered under the title bars, the
// message centered in it, equal buttons on one row inside the panel's horizontal bounds, right-aligned to it.
// Needs TransferScene from adaptive_transfer_status_checks.hpp.
void message_screens_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 7;
    const char* names[scenarios]{"offer decision", "completion", "listing", "listing (loan)", "restriction", "feedback", "staff release"};
    const unsigned builders[scenarios]{0x10421fb0, 0x10423270, 0x1042a810, 0x1042a810, 0x10420b50, 0x10421070, 0x1009e8f0};
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
                default: t.invoke(0x1009e880, {t.person}); break;
                }
                require(t.builder() == builders[scenario], "Message screen did not open through its original opener");
                bool on = adaptive;
                const auto records = [&] { return t.records({{offer, 0x54}, {link, 4}}); };
                struct Shape { int message = -1, footer = -1; std::vector<unsigned> footer_controls; };
                const auto shape = [&] {
                    Shape r;
                    for (unsigned i = 0; i < t.grids(); ++i) {
                        const auto box = t.rect(t.grid(i)); const auto columns = c.u8(t.grid(i) + 0xbb0);
                        if (box == ScreenRect{110, 140, 780, 545} && columns == 1) r.message = int(i);
                        if (box == ScreenRect{100, 555, 790, 590} && c.u8(t.grid(i) + 0xbb1) == 1) r.footer = int(i);
                    }
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if ((r.footer >= 0 && owner == r.footer) || (owner < 0 && t.rect(t.control(i) + 0x10) == ScreenRect{100, 555, 790, 590}))
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
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Message gate disagrees with preference");
                    t.rebuild(size);
                    const auto before = shape();
                    require(before.message >= 0, "Original message panel absent");
                    const auto old = t.layout(records, "Message screen");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy message screen changed"); pick(); return; }
                    const auto panel = t.rect(t.grid(before.message));
                    const ScreenLayout shell{size};
                    const auto title = shell.title();
                    // Panel: a bounded dialog centered under the title bars, in the upper half of the screen.
                    require(panel != ScreenRect{110, 140, 780, 545}, "Message recipe did not run");
                    require(std::abs((panel.left + panel.right) - (title.left + title.right)) <= 2, "Message panel is not centered under the title bars");
                    require(panel.left >= 110 && panel.width() <= 800 && panel.right <= size.width - 10 && panel.top >= 140 && panel.bottom < size.height - 10,
                            "Message panel outside the content area");
                    require(panel.top + panel.height() / 2 <= size.height / 2, "Message dialog not in the upper half");
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
                    require(panel.height() == int(std::max(last - first + 3, 5u)) * 28 + 16, "Message panel not bounded to its message");
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
                }
                if (adaptive) { on = false; select_new_screens(false); build(canvas); }
                // Safe dismissals through the original handlers: records unchanged, same result in both runs.
                std::vector<std::uint8_t> outcome;
                const auto saved = records();
                const unsigned back = scenario == 0 || scenario == 4 ? 1 : scenario == 2 || scenario == 3 ? 1 : 0;
                if (back) {
                    const int i = t.find(back);
                    require(i >= 0 && t.hit(i) == i, "Message dismissal control missing");
                    const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(i)}));
                    require(records() == saved, "Message dismissal changed records");
                    for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift);
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

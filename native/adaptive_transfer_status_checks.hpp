// Player transfer status (0x10428ab0) and Previous offers (0x10422600), opened through their original
// openers in disposable copies of an existing capture. Each scenario is built twice, with New Screens off
// and on; the adaptive run must keep the original controls, commands, records and RNG, and only move them.
// Included by adaptive_gui_checks.cpp after its helpers.
#include <functional>
struct TransferScene {
    Clock clock;
    std::unique_ptr<State> state;
    State& s;
    guest::Cpu c;
    // A layout that compacts rows moves controls between rows; their column and everything else stays.
    bool rows_may_move = false;
    std::uint32_t manager = 0, display = 0, instance = 0, managed = 0, person = 0, contract = 0;
    TransferScene(const char* path, const std::filesystem::path& temp)
        : state(std::make_unique<State>()), s(*state), c(s) {
        clock.idle_input = true;
        load_match_snapshot(s, path); s.code_fragment_observer = &clock;
        c.set(2, c.u32(c.sp() + 0x14));
        manager = c.toc(-0x7bb0); c.put16(manager + 0x2c58, 0);
        s.platform = std::make_shared<NativePlatform>(temp / "game", temp / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
        require(s.platform->resize_gui_canvas(s, {801, 601}), "Transfer screen pixel initialization");
        instance = c.u32(manager + 4); managed = c.u32(instance + 0x24); display = c.u32(manager + 0x24);
        // A player of the manager's club with a current contract.
        for (unsigned j = 0; j < 50 && !person; ++j) {
            const auto candidate = c.u32(managed + 0x98 + j * 4);
            if (!candidate || !c.u32(candidate + 0x14)) continue;
            contract = invoke(0x10082f90, {c.u32(c.toc(-0x7b7c)), candidate, managed});
            if (contract) person = candidate;
        }
        require(person, "Transfer screen fixture needs a contracted player of the managed club");
    }
    unsigned invoke(unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000; c.call(entry, return_sentinel); return c.r(3);
    }
    std::uint32_t grid(unsigned i) const { return display + 4 + i * 0xbf0; }
    std::uint32_t control(unsigned i) const { return display + 0xba864 + i * 0x17c; }
    ScreenRect rect(unsigned at) const { return {c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)}; }
    std::uint32_t history() const { return c.u32(manager + 0x10); }
    std::uint32_t value(unsigned slot) const { return c.u32(history() + 0x14 + slot * 8); }
    void set_value(unsigned slot, unsigned v) { c.put32(history() + 0x14 + slot * 8, v); }
    std::uint32_t builder() const { return c.u32(c.u32(history())); }
    unsigned grids() const { return c.u16(display + 0x129da4); }
    unsigned controls() const { return c.u16(display + 0x129da6); }
    bool enabled(unsigned i) const { return !(c.u32(control(i) + 0x3c) & 0x20); }
    int find(unsigned action) const {
        for (unsigned i = 0; i < controls(); ++i) if (c.u32(control(i) + 8) == action && enabled(i)) return int(i);
        return -1;
    }
    // A news item of the given type and word payload for the manager, published by the original constructor and queue
    // insertion (the openers of the news-driven screens take the manager and this ID); returns the ID the queue gave it.
    unsigned news_item(unsigned type, std::initializer_list<unsigned> payload) {
        const auto queue = c.u32(c.toc(-0x7bf0)), item = allocate(s, 256, true), out = allocate(s, 256, true);
        invoke(0x102bdf90, {item, type, 0});
        unsigned slot = 0;
        for (const auto word : payload) invoke(0x102cb4b0, {item, slot++, word});
        invoke(0x102cbc10, {queue, item, instance, 0});
        const auto id = c.u32(queue + 16);
        require((invoke(0x102ccdd0, {queue, instance, id, out}) & 0xff) != 0, "News fixture: the published item cannot be fetched");
        return id;
    }
    // The news queue's items, for the screens that read (and may mark) them.
    std::vector<std::uint8_t> news_queue() {
        const auto queue = c.u32(c.toc(-0x7bf0));
        return s.memory.copy_out(c.u32(queue), c.u32(queue + 4) * 0xe4);
    }
    std::vector<std::uint8_t> meanings() const {
        std::vector<std::uint8_t> bytes;
        for (unsigned i = 0; i < controls(); ++i)
            for (const auto [offset, size] : {std::pair{8u, 8u}, {0x20u, rows_may_move ? 4u : 8u}, {0x38u, 20u}, {0x68u, 0x114u}}) {
                const auto part = s.memory.copy_out(control(i) + offset, size); bytes.insert(bytes.end(), part.begin(), part.end());
            }
        return bytes;
    }
    // The control under the middle of control `i`, as the game's own picker reports it.
    int hit(unsigned i) {
        const auto box = rect(control(i) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left + box.right) / 2); c.put32(c.toc(-0x5c14), (box.top + box.bottom) / 2);
        c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
        return std::int16_t(invoke(0x1071a280, {display}));
    }
    std::vector<std::uint8_t> records(std::initializer_list<std::pair<unsigned, unsigned>> extra = {}) {
        std::vector<std::uint8_t> bytes;
        const auto append = [&](unsigned at, unsigned count) {
            if (at) { const auto part = s.memory.copy_out(at, count); bytes.insert(bytes.end(), part.begin(), part.end()); }
        };
        append(person, 0x70); append(contract, 0x64); append(managed, 0x248); append(history(), 0x110);
        for (const auto& [at, count] : extra) append(at, count);
        const auto rng = random_state(s); bytes.insert(bytes.end(), rng.begin(), rng.end());
        return bytes;
    }
    // Gate checks shared by both screens: it follows the preference, and refuses a secondary or network screen.
    void check_gate(bool adaptive, const char* what) {
        require(adaptive_screen_available(s, builder()) == adaptive, what);
        if (!adaptive) return;
        c.put16(manager + 0x2c58, 1); require(!adaptive_screen_available(s, builder()), "Secondary transfer screen passed gate");
        c.put16(manager + 0x2c58, 0); c.put32(manager + 0x2c64, 1);
        require(!adaptive_screen_available(s, builder()), "Network transfer screen passed gate"); c.put32(manager + 0x2c64, 0);
    }
    // The original builder and its grid refresh, as the game runs them, at the given canvas.
    void rebuild(GuiCanvas size) {
        require(s.platform->resize_gui_canvas(s, size), "Transfer screen resize failed");
        const auto original = builder();
        invoke(0x10719560, {display, 1, 0}); invoke(original, {});
        for (unsigned i = 0; i < grids(); ++i) { invoke(0x10714630, {grid(i)}); invoke(0x10714110, {grid(i), 0}); }
    }
    // Runs the layout pass and checks what it must never change; returns the display image from before it.
    std::vector<std::uint8_t> layout(const std::function<std::vector<std::uint8_t>()>& records_of, const char* what) {
        const auto semantics = meanings(), saved = records_of(), old = s.memory.copy_out(display, 0x129da8);
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto stack_base = c.sp() - 0x8000; const auto stack = s.memory.copy_out(stack_base, 0x8100);
        layout_built_screen(s, display, builder());
        const std::string name = what;
        require(meanings() == semantics, (name + " changed control order, actions, payload, fonts, colors or text").c_str());
        require(records_of() == saved, (name + " layout changed history, records or RNG").c_str());
        require(s.registers == registers && s.unique == unique && s.pc == pc && s.memory.copy_out(stack_base, 0x8100) == stack,
                (name + " layout clobbered guest ABI/stack").c_str());
        return old;
    }
};
inline std::int32_t display_word(const std::vector<std::uint8_t>& image, unsigned offset) {
    return std::int32_t((unsigned(image[offset]) << 24) | (unsigned(image[offset + 1]) << 16) | (unsigned(image[offset + 2]) << 8) | image[offset + 3]);
}
inline const std::array<GuiCanvas, 6>& transfer_canvases() {
    static const std::array<GuiCanvas, 6> sizes{{{800, 600}, {801, 601}, {800, 900}, {1440, 600}, {3200, 1800}, {1280, 900}}};
    return sizes;
}

void transfer_status_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 8;
    const char* names[scenarios]{"price", "kind 1", "kind 3", "kind 6", "limited", "squad+automatic", "unavailable+listed", "no news"};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                // The contract the original opener reads: its kind picks the price row (kinds 1 and 3 show
                // it disabled, 6 omits it), the flags and squad status feed the choices below it.
                const auto kind = [&](int k) { c.put8(t.contract + 0x38, (c.u8(t.contract + 0x38) & 3) | ((k & 0x3f) << 2)); };
                kind(scenario == 1 ? 1 : scenario == 2 ? 3 : scenario == 3 ? 6 : 0);
                if (scenario == 5) { c.put8(t.contract + 0x60, (c.u8(t.contract + 0x60) & 0xf0) | 2); c.put8(t.contract + 0x5c, c.u8(t.contract + 0x5c) | 4); }
                if (scenario == 6) { c.put8(t.contract + 0x5c, c.u8(t.contract + 0x5c) | 8); c.put32(t.contract + 0x50, 100000); }
                t.invoke(0x10428750, {t.person, t.instance, 0xffffffffu, 0});
                require(t.builder() == 0x10428ab0 && c.u32(c.u32(t.history() + 8)) == 0x10429a10, "Original status opener/handler changed");
                if (scenario == 4) t.set_value(12, 1);
                if (scenario == 7) t.set_value(8, 0);
                bool on = adaptive;
                const auto find_grids = [&](int& price, int& choices, int& footer) {
                    price = choices = footer = -1;
                    for (unsigned i = 0; i < t.grids(); ++i) {
                        const auto box = t.rect(t.grid(i)); const auto columns = c.u8(t.grid(i) + 0xbb0);
                        if (box == ScreenRect{110, 140, 780, 545} && columns == 7) price = int(i);
                        else if (box == ScreenRect{110, 140, 780, 545} && columns == 2) choices = int(i);
                        else if (box == ScreenRect{100, 555, 790, 590} && c.u8(t.grid(i) + 0xbb1) == 1) footer = int(i);
                    }
                };
                // Every enabled control of the screen's own panels answers its own picker hit.
                const auto pick = [&] {
                    int price, choices, footer; find_grids(price, choices, footer);
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if ((owner == price || owner == choices || owner == footer) && c.u32(t.control(i) + 8) && t.enabled(i))
                            require(t.hit(i) == int(i), "Status picker missed an original control");
                    }
                };
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Status gate disagrees with preference");
                    t.rebuild(size);
                    int price, choices, footer; find_grids(price, choices, footer);
                    require(price >= 0 && choices >= 0 && footer >= 0, "Original status panels absent");
                    const auto old = t.layout([&] { return t.records(); }, "Status");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy status changed"); return; }
                    const auto body = t.rect(t.grid(price)), buttons = t.rect(t.grid(footer));
                    require(body.left >= 110 && body.width() <= 960 && body.right < size.width && body.bottom < buttons.top &&
                            buttons.right == body.right && buttons.bottom == size.height - 10 && buttons.width() <= 570,
                            "Status panel/footer bounds invalid");
                    require(body != ScreenRect{110, 140, 780, 545} && t.rect(t.grid(choices)) == body, "Status recipe did not run");
                    require(c.u8(t.grid(price) + 0xbb1) == c.u8(t.grid(choices) + 0xbb1), "Status grids lost their shared row count");
                    int footer_width = -1;
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const auto at = t.control(i); const int owner = c.i16(at + 0x72);
                        if (owner != price && owner != choices && owner != footer) continue;
                        const auto box = t.rect(at + 0x10), panel = owner == footer ? buttons : body;
                        if (box.top == 0 && box.bottom == 0) continue;
                        require(box.left >= panel.left && box.right <= panel.right && box.top >= panel.top && box.bottom <= panel.bottom,
                                "Status control outside its panel");
                        const unsigned old_at = 0xba864 + i * 0x17c;
                        const auto old_box = ScreenRect{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14),
                                                        display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
                        if (owner == footer) {
                            if (footer_width < 0) footer_width = box.width();
                            require(std::abs(box.width() - footer_width) <= 1 && box.width() >= 60, "Status footer buttons unequal or too small");
                        } else if (owner == price && c.u32(at + 0x20) > 1 && c.u32(at + 0x20) < 7 && display_word(old, old_at + 0x28) == 0) {
                            // Only the label column grows; the buttons and the amount keep their width.
                            require(box.width() == old_box.width(), "Status price control changed width");
                        } else if (display_word(old, old_at + 0x28) && display_word(old, old_at + 0x2c)) {
                            // Intro paragraph: each link keeps its offset from the paragraph's start.
                            for (unsigned j = 0; j < t.controls(); ++j) {
                                const auto other = t.control(j);
                                if (j == i || c.i16(other + 0x72) != owner || c.u32(other + 8) || c.u32(other + 0x24) != c.u32(at + 0x24)) continue;
                                const unsigned other_old = 0xba864 + j * 0x17c;
                                if (!display_word(old, other_old + 0x28)) continue;
                                require(c.i32(at + 0x28) - c.i32(other + 0x28) == display_word(old, old_at + 0x28) - display_word(old, other_old + 0x28),
                                        "Status link moved relative to its paragraph");
                            }
                        }
                    }
                    pick();
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    // A person outside the database is not a state the layout handles.
                    const auto id = t.value(0); t.set_value(0, 0x7fffffff);
                    require(!adaptive_screen_available(t.s, 0x10428ab0), "Status gate accepted an invalid person");
                    t.set_value(0, id);
                    for (const auto size : transfer_canvases()) build(size);
                }
                std::vector<std::uint8_t> outcome;
                const auto checkpoint = [&](int status) {
                    outcome.push_back(std::uint8_t(status)); outcome.push_back(std::uint8_t(status >> 8));
                    for (unsigned slot = 0; slot < 16; ++slot) for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(t.value(slot) >> shift);
                    const auto part = t.s.memory.copy_out(t.contract, 0x64); outcome.insert(outcome.end(), part.begin(), part.end());
                };
                const auto action = [&](unsigned command) {
                    const int i = t.find(command);
                    if (i < 0) return false;
                    require(t.hit(i) == i, "Status command hit missed");
                    const int result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(i)}));
                    checkpoint(result);
                    require(result == -11 || result == 0, ("Status edit " + std::to_string(command) + " returned " + std::to_string(result)).c_str());
                    if (result == -11) build(canvas);
                    else {
                        // The price steps update their controls in place: the original redraw must keep
                        // the explicit bounds the layout gave them.
                        std::vector<std::uint8_t> boxes;
                        const auto sample = [&] {
                            boxes.clear();
                            for (unsigned k = 0; k < t.controls(); ++k) {
                                const auto part = t.s.memory.copy_out(t.control(k) + 0x10, 16); boxes.insert(boxes.end(), part.begin(), part.end());
                            }
                            return boxes;
                        };
                        const auto before = sample();
                        for (unsigned k = 0; k < t.grids(); ++k) { t.invoke(0x10714630, {t.grid(k)}); t.invoke(0x10714110, {t.grid(k), 0}); }
                        require(sample() == before, "Status redraw lost the layout's control bounds");
                        pick();
                    }
                    return true;
                };
                // Price steps, automatic pricing, squad status and the listing choices: every edit the
                // original offers on this screen short of applying it.
                unsigned edits = 0;
                for (unsigned command : {2u, 3u, 5u, 7u, 9u, 8u, 10u}) edits += action(command);
                if (scenario == 0) require(edits >= 4, "Status fixture exercised too few original edits");
                if (adaptive) for (const auto size : {GuiCanvas{801, 601}, GuiCanvas{3200, 1800}, canvas}) build(size);
                // The price menu opens at its moved trigger, inside the canvas, and its choices still work.
                bool menu_checked = false;
                for (unsigned i = 0; i < t.controls(); ++i) {
                    const auto at = t.control(i);
                    if (c.i16(at + 0x72) < 0 || c.i16(at + 0x72) >= int(t.grids()) || c.u8(t.grid(c.i16(at + 0x72)) + 0xbb0) != 7 ||
                        c.i16(at + 0x74) < 0 || !t.enabled(i) || c.u32(at + 8)) continue;
                    const int popup = c.i16(at + 0x74);
                    require(t.hit(i) == -1 && c.u8(t.display) == 1 && c.i16(t.display + 0x12a930) == popup, "Status price menu lost its anchor");
                    const auto menu = t.rect(t.grid(popup));
                    require(menu.left >= 0 && menu.right <= canvas.width && menu.top >= 0 && menu.bottom <= canvas.height, "Status price menu escaped the canvas");
                    const int choice = t.find(11);
                    require(choice >= 0, "Status price menu has no choice");
                    const int result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(choice)}));
                    checkpoint(result); require(result == -11, "Status menu choice navigated away");
                    build(canvas); menu_checked = true; break;
                }
                // Only kinds with an editable, unlimited price own the menu.
                require(menu_checked == (scenario == 0 || scenario == 5 || scenario == 6 || scenario == 7), "Status price menu coverage changed");
                if (adaptive) {
                    // Turning New Screens off restores the classic layout on the next build.
                    on = false; select_new_screens(false); build(canvas);
                }
                const int set = t.find(6);
                if (set >= 0) require(t.hit(set) == set, "Status Set hit missed");
                const auto saved = t.records();
                const int back = t.find(4);
                require(back >= 0 && t.hit(back) == back, "Status Back missing");
                const auto result = std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(back)}));
                require(result == -9 && t.records() == saved, "Status Back changed records");
                checkpoint(result);
                if (!adaptive) baseline = outcome;
                else require(outcome == baseline, "Adaptive status changed original command outcomes");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Status scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "status state " << scenario << " (" << names[scenario] << "): original commands, input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

void previous_offers_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    // Which message the original builds: the offer changed since the revision shown, or its status.
    constexpr unsigned scenarios = 6;
    const char* names[scenarios]{"changed", "status 2", "status 5", "status 6", "status 3", "status 0"};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                const auto offer = allocate(t.s, 0x54, true);
                t.invoke(0x10411ea0, {offer, t.person, t.managed, 0, 0xffffffffu, 1});
                c.put32(offer + 0x28, 0xffffffffu); c.put32(offer + 0x2c, 7);
                static constexpr unsigned statuses[]{0, 2, 5, 6, 3, 0};
                c.put8(offer + 0x34, statuses[scenario]);
                t.invoke(0x10422570, {offer, scenario ? 7u : 3u});
                require(t.builder() == 0x10422600 && c.u32(c.u32(t.history() + 8)) == 0x10422a00, "Original previous-offer opener/handler changed");
                bool on = adaptive;
                const auto records = [&] { return t.records({{offer, 0x54}}); };
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Previous offers gate disagrees with preference");
                    t.rebuild(size);
                    int subtitle = -1, message = -1;
                    for (unsigned i = 0; i < t.grids(); ++i) {
                        const auto box = t.rect(t.grid(i));
                        if (box == ScreenRect{100, 80, 790, 125}) subtitle = int(i);
                        if (box == ScreenRect{110, 140, 780, 545}) message = int(i);
                    }
                    int footer = -1;
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        if (c.i16(t.control(i) + 0x72) >= 0) continue;
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box == ScreenRect{100, 555, 790, 590}) footer = int(i);
                    }
                    require(subtitle >= 0 && message >= 0 && footer >= 0, "Original previous-offer panels absent");
                    const auto old = t.layout(records, "Previous offers");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy previous offers changed"); }
                    else {
                        const auto body = t.rect(t.grid(message)), button = t.rect(t.control(footer) + 0x10);
                        require(body.left >= 110 && body.width() <= 960 && body.right < size.width && body.bottom < button.top &&
                                button.right == body.right && button.bottom == size.height - 10 && button.width() <= 190 && button.width() >= 100,
                                "Previous offers panel/footer bounds invalid");
                        require(body != ScreenRect{110, 140, 780, 545} && button != ScreenRect{100, 555, 790, 590},
                                "Previous offers recipe did not run");
                        // Centered texts keep their widths and offsets: the whole group moves by one shift.
                        for (const int owner : {subtitle, message}) {
                            int shift = 0; bool first = true;
                            for (unsigned i = 0; i < t.controls(); ++i) {
                                const auto at = t.control(i);
                                const unsigned old_at = 0xba864 + i * 0x17c;
                                if (c.i16(at + 0x72) != owner || !display_word(old, old_at + 0x28)) continue;
                                const int moved = c.i32(at + 0x28) - display_word(old, old_at + 0x28);
                                require(c.i32(at + 0x2c) - c.i32(at + 0x28) == display_word(old, old_at + 0x2c) - display_word(old, old_at + 0x28),
                                        "Previous offers text changed width");
                                if (first) shift = moved;
                                require(first || moved == shift, "Previous offers link moved relative to its text");
                                first = false;
                                const auto grid = t.rect(t.grid(owner)), box = t.rect(at + 0x10);
                                require(box.left >= grid.left && box.right <= grid.right, "Previous offers text outside its panel");
                            }
                        }
                    }
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        if ((owner == subtitle || owner == message || int(i) == footer) && c.u32(t.control(i) + 8) && t.enabled(i))
                            require(t.hit(i) == int(i), "Previous offers picker missed an original control");
                    }
                };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                if (adaptive) {
                    // No offer record, or one naming a person outside the database, is not handled.
                    const auto record = t.value(0); t.set_value(0, 0);
                    require(!adaptive_screen_available(t.s, 0x10422600), "Previous offers gate accepted a missing offer");
                    t.set_value(0, record); c.put32(offer + 12, 0x7fffffff);
                    require(!adaptive_screen_available(t.s, 0x10422600), "Previous offers gate accepted an invalid person");
                    c.put32(offer + 12, t.person ? c.u32(t.person) : 0);
                    for (const auto size : transfer_canvases()) build(size);
                }
                if (adaptive) { on = false; select_new_screens(false); build(canvas); }
                // The footer button dismisses the screen through the original handler, records untouched.
                const auto saved = records();
                const int back = t.find(3);
                require(back >= 0 && t.hit(back) == back, "Previous offers footer missing");
                require(std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {unsigned(back)})) == -10 && records() == saved,
                        "Previous offers footer changed records");
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Previous offers scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "previous offers state " << scenario << " (" << names[scenario] << "): original input, records/RNG and guest ABI preserved" << std::endl;
    }
    initialize_new_screens();
}

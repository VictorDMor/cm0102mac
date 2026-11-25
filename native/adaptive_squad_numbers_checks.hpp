// Squad Numbers and the European Squad (builder 0x1004a4a0, handler 0x10052740), opened through their original openers on a
// disposable copy of an existing capture: 0x10049f80 with a news item of type 24 (the numbers of the forthcoming season) and
// of another type (changes during the season), and 0x1004a200 (the European squad). The original handler is driven through
// the controls of the screen (a player, a number, Auto, Clear, Back). Built with New Screens off and on at seven canvases; the
// adaptive run keeps every control, command, record and RNG draw and only moves them: a centered panel with the menus at the
// left and the Auto/Clear buttons at the right of one header row, the fifty numbers as two stacked rows under it, the heading and
// the list of the squad with equal rows (as many as the canvas holds, the whole squad when it fits, the scrollbar only when
// it does not), the counter of the European squad under its list, and a compact Back/Submit footer at the panel's right edge.
// Mutants of each geometric promise must be refused by the same checker, and a screen without its heading must keep the classic
// geometry. Needs TransferScene (adaptive_transfer_status_checks.hpp) and require_rejected (adaptive_paged_checks.hpp).
inline unsigned squad_scrolling_layouts = 0, squad_whole_layouts = 0;
struct SquadShape { int menus = -1, buttons = -1, low = -1, high = -1, list = -1, footer = -1, heading = -1, counter = -1; bool european = false; };

inline SquadShape squad_shape(TransferScene& t) {
    SquadShape r;
    for (unsigned i = 0; i < t.grids(); ++i) {
        const auto box = t.rect(t.grid(i)); const auto columns = t.c.u8(t.grid(i) + 0xbb0), rows = t.c.u8(t.grid(i) + 0xbb1);
        if (box == ScreenRect{110, 145, 360, 165} && columns == 2) r.menus = int(i);
        else if (box == ScreenRect{530, 145, 780, 165} && columns == 2) r.buttons = int(i);
        else if (box == ScreenRect{110, 210, 780, 230} && columns == 25) r.low = int(i);
        else if (box == ScreenRect{110, 515, 780, 535} && columns == 25) r.high = int(i);
        else if ((box == ScreenRect{110, 235, 780, 510} || box == ScreenRect{110, 210, 780, 495}) && columns == 8 && rows == 12) { r.list = int(i); r.european = box.top == 210; }
        else if (box == ScreenRect{100, 555, 790, 590} && columns == 2) r.footer = int(i);
    }
    for (unsigned i = 0; i < t.controls(); ++i) {
        if (t.c.i16(t.control(i) + 0x72) >= 0) continue;
        const auto box = t.rect(t.control(i) + 0x10);
        if (box == ScreenRect{110, 170, 780, 205}) r.heading = int(i);
        else if (box == ScreenRect{110, 500, 780, 535}) r.counter = int(i);
    }
    return r;
}

inline void verify_squad_numbers(TransferScene& t, GuiCanvas size, const SquadShape& shape, const std::vector<std::uint8_t>& old, const char* what) {
    auto& c = t.c;
    const std::string name = what;
    const auto fail = [&](const char* message) { throw std::runtime_error(name + ": " + message); };
    const ScreenLayout shell{size};
    const auto title = shell.title();
    const auto grid_rect = [&](int grid) { return t.rect(t.grid(unsigned(grid))); };
    const auto list = grid_rect(shape.list), menus = grid_rect(shape.menus), buttons = grid_rect(shape.buttons), footer = grid_rect(shape.footer);
    const auto list_grid = t.grid(unsigned(shape.list));
    const int width = std::min(960, shell.content(145).width());
    // At a width of 800 the bounded panel is the classic one; the footer, which moves to the bottom, tells that the layout ran.
    if (footer == ScreenRect{100, 555, 790, 590} && size.height == 600) fail("layout did not run, the footer stays at classic coordinates");
    if (list.width() != width || std::abs(list.left + list.right - title.left - title.right) > 2 || list.left < 110 || list.right > size.width - 10)
        fail("panel is not bounded and centered under the title bars");
    // One header row: the menus at the panel's left edge, Auto and Clear at its right edge.
    if (menus != ScreenRect{list.left, 145, list.left + 250, 165}) fail("menus are not at the left of the header row");
    if (buttons != ScreenRect{list.right - 250, 145, list.right, 165}) fail("Auto/Clear are not at the right of the header row");
    // The numbers: two rows of 25 equal buttons, one under the other, under the header row.
    std::vector<std::vector<ScreenRect>> strips;
    if (!shape.european) {
        if (grid_rect(shape.low) != ScreenRect{list.left, 171, list.right, 195} || grid_rect(shape.high) != ScreenRect{list.left, 197, list.right, 221})
            fail("the numbers are not one palette of two rows under the header");
        for (const int strip : {shape.low, shape.high}) {
            std::vector<ScreenRect> cells(25);
            unsigned seen = 0;
            for (unsigned i = 0; i < t.controls(); ++i) {
                const auto at = t.control(i);
                if (c.i16(at + 0x72) != strip) continue;
                const auto column = c.u32(at + 0x20); const auto box = t.rect(at + 0x10);
                if (column >= 25) fail("a number button has no column");
                cells[column] = box; ++seen;
                const auto frame = grid_rect(strip);
                if (box.top != frame.top || box.bottom != frame.bottom || box.left < frame.left || box.right > frame.right) fail("a number button is outside its row");
            }
            if (seen != 25) fail("the palette lost a number button");
            for (unsigned k = 0; k < 25; ++k) {
                if (cells[k].width() < 24 || std::abs(cells[k].width() - cells[0].width()) > 1) fail("number buttons are not equal");
                if (k && cells[k - 1].right > cells[k].left) fail("number buttons overlap");
            }
            if (cells[24].right < grid_rect(strip).right - 2) fail("the palette does not fill its row");
            strips.push_back(cells);
        }
        if (strips[0][0].left != strips[1][0].left || strips[0][24].right != strips[1][24].right) fail("the two rows of numbers are not aligned");
    }
    // The heading sits on the list (with the palette above it, or alone under the header in the European squad).
    const auto heading_at = t.control(unsigned(shape.heading));
    const auto heading = t.rect(heading_at + 0x10);
    if (heading.left != list.left || heading.right != list.right || heading.height() != 35 || c.i32(heading_at + 0x28) != list.left || c.i32(heading_at + 0x2c) != list.right)
        fail("the heading does not span the panel");
    if (shape.european ? (heading.top != 170 || list.top != 210) : heading.bottom != list.top) fail("the heading is not directly above the list");
    if (shape.european) {
        const auto counter_at = t.control(unsigned(shape.counter));
        if (t.rect(counter_at + 0x10) != ScreenRect{list.left, list.bottom + 5, list.right, list.bottom + 40} || c.i32(counter_at + 0x28) != list.left || c.i32(counter_at + 0x2c) != list.right)
            fail("the counter does not follow the list");
    }
    // The footer: Back and Submit, equal, flush, right-aligned to the panel on the bottom row.
    if (footer.right != list.right || footer.bottom != size.height - 10 || footer.height() != 35 || footer.width() != std::min(list.width(), 340)) fail("footer is not compact under the panel");
    std::vector<ScreenRect> buttons_row;
    for (unsigned i = 0; i < t.controls(); ++i)
        if (c.i16(t.control(i) + 0x72) == shape.footer) buttons_row.push_back(t.rect(t.control(i) + 0x10));
    if (buttons_row.size() != 2 || std::abs(buttons_row[0].width() - buttons_row[1].width()) > 1 || buttons_row[0].left != footer.left ||
        buttons_row[1].right != footer.right || buttons_row[0].right > buttons_row[1].left) fail("footer buttons are not equal and flush");
    // The list: equal rows, as many as the room holds, columns in the halves' boxes, the scrollbar only when needed.
    const int rows = c.u8(list_grid + 0xbb1), total = std::max(12, c.i32(list_grid + 0x14) + 1);
    const int limit = size.height - 65 - (shape.european ? 40 : 0);
    if (rows < 12 || rows > total) fail("the list shows a wrong number of rows");
    if (rows < total && list.bottom + 24 <= limit) fail("the list leaves room for another row unused");
    if (list.bottom > limit || list.top < 170) fail("the list is outside its room");
    const bool scrollbar = c.u8(list_grid + 0xbef) != 0;
    if (scrollbar != (c.i32(list_grid + 0x14) + 1 > rows)) fail("the scrollbar does not follow the rows that fit");
    const int pitch = (list.height() - 16) / rows;
    if (pitch < 21 || pitch > 24 || list.height() != rows * pitch + 16) fail("rows are not equal in height");
    std::array<std::pair<int, int>, 8> column{};
    std::array<bool, 8> known{};
    unsigned cells = 0;
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i);
        if (c.i16(at + 0x72) != shape.list) continue;
        const auto row = c.u32(at + 0x24), col = c.u32(at + 0x20);
        if (col >= 8) fail("a list cell has no column");
        const auto box = t.rect(at + 0x10);
        if (int(row) >= rows) continue;   // waiting behind the scrollbar
        const int top = list.top + 8 + int(row) * pitch;
        if (std::abs(box.top - top) > 1 || box.height() < 18 || box.height() > pitch || box.bottom > list.bottom - 6) fail("a list cell is not in its row");
        if (box.left < list.left || box.right > list.right - (scrollbar ? 24 : 2)) fail("a list cell is outside the list");
        if (c.i32(at + 0x28) != box.left || c.i32(at + 0x2c) != box.right) fail("a list cell's text bounds differ from its rectangle");
        if (!known[col]) { column[col] = {box.left, box.right}; known[col] = true; }
        else if (column[col] != std::pair{box.left, box.right}) fail("a column is not aligned across the rows");
        ++cells;
    }
    if (cells < 8 * 4) fail("the list lost cells");
    for (int side = 0; side < 2; ++side) {
        const auto number = column[side * 4], marker = column[side * 4 + 1], player = column[side * 4 + 2], positions = column[side * 4 + 3];
        if (!known[side * 4] || !known[side * 4 + 2] || !known[side * 4 + 3]) fail("a half of the list has no cells");
        if (number.second - number.first != 32 || positions.second - positions.first != 89 || player.second - player.first < 100) fail("a column has the wrong width");
        if (known[side * 4 + 1] && (marker.second - marker.first != 27 || marker.first < number.second || marker.second > player.first)) fail("the marker column is misplaced");
        if (number.second > player.first || player.second > positions.first) fail("columns overlap");
    }
    if (column[2].second - column[2].first != column[6].second - column[6].first || column[3].second > column[4].first) fail("the two halves are not equal");
    if (column[7].second > list.right - (scrollbar ? 24 : 2)) fail("the right half runs under the scrollbar");
    // Nothing the screen shows overlaps anything else, so every control is reachable where it is drawn (the number buttons are
    // drop targets of the original's drag, which the picker cannot find, so their rectangles are the proof).
    {
        std::vector<std::pair<unsigned, ScreenRect>> shown;
        for (unsigned i = 0; i < t.controls(); ++i) {
            const auto at = t.control(i);
            const int owner = c.i16(at + 0x72);
            const bool ours = owner == shape.menus || owner == shape.buttons || owner == shape.low || owner == shape.high || owner == shape.footer ||
                              owner == shape.list || int(i) == shape.heading || int(i) == shape.counter;
            const auto box = t.rect(at + 0x10);
            if (!ours || box.width() <= 0 || box.height() <= 0) continue;
            if (owner == shape.list && int(c.u32(at + 0x24)) >= rows) continue;
            if (box.left < 0 || box.top < 0 || box.right > size.width || box.bottom > size.height) fail("a control is outside the canvas");
            shown.emplace_back(i, box);
        }
        for (std::size_t a = 0; a < shown.size(); ++a)
            for (std::size_t b = a + 1; b < shown.size(); ++b) {
                const auto& [i, p] = shown[a]; const auto& [j, q] = shown[b];
                (void)i; (void)j;
                if (p.left < q.right && q.left < p.right && p.top < q.bottom && q.top < p.bottom) fail("two controls overlap");
            }
        if (shown.size() < (shape.european ? 0u : 50u) + 8 * 4 + 4) fail("fewer controls shown than the screen has");
    }
    // Everything else the builder placed stays: standalone controls apart from the heading and the counter, and the pop-up cells.
    for (unsigned i = 0; i < t.controls(); ++i) {
        const auto at = t.control(i); const unsigned old_at = 0xba864 + i * 0x17c;
        if (c.i16(at + 0x72) >= 0 || int(i) == shape.heading || int(i) == shape.counter) continue;
        const ScreenRect before{display_word(old, old_at + 0x10), display_word(old, old_at + 0x14), display_word(old, old_at + 0x18), display_word(old, old_at + 0x1c)};
        if (t.rect(at + 0x10) != before) fail("an unrelated standalone control moved");
    }
    ++(scrollbar ? squad_scrolling_layouts : squad_whole_layouts);
}

struct SquadCase { const char* name; unsigned opener; unsigned news; std::initializer_list<unsigned> payload; };

void squad_numbers_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    constexpr unsigned scenarios = 6;
    const char* names[scenarios]{"forthcoming season", "current season", "european squad", "another sort", "auto then clear", "european, another view"};
    const std::array<SquadCase, scenarios> cases{{
        {names[0], 0x10049f80, 24, {}}, {names[1], 0x10049f80, 25, {}}, {names[2], 0x1004a200, 26, {0, 0, 2, 0, 1, 1}},
        {names[3], 0x10049f80, 24, {}}, {names[4], 0x10049f80, 24, {}}, {names[5], 0x1004a200, 26, {0, 0, 2, 0, 1, 1}}}};
    for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
        std::vector<std::uint8_t> baseline;
        for (bool adaptive : {false, true}) {
            try {
                select_new_screens(adaptive);
                TransferScene t(path, temp.path);
                auto& c = t.c;
                const auto& item = cases[scenario];
                t.invoke(item.opener, {t.instance, t.news_item(item.news, item.payload)});
                require(t.builder() == 0x1004a4a0, "Squad numbers did not open through their original opener");
                bool on = adaptive;
                // The numbers the club gives its players (0x104add10), as the screen itself reads them.
                const auto numbers = [&] {
                    std::vector<std::uint8_t> bytes;
                    for (unsigned j = 0; j < 50; ++j) {
                        const auto player = c.u32(t.managed + 0x98 + j * 4);
                        bytes.push_back(player ? std::uint8_t(t.invoke(0x104add10, {c.u32(c.toc(-0x7a98)), player, t.managed})) : 0xff);
                    }
                    return bytes;
                };
                const auto records = [&] { auto bytes = t.records(); const auto n = numbers(); bytes.insert(bytes.end(), n.begin(), n.end()); return bytes; };
                SquadShape shape;
                std::vector<std::uint8_t> old;
                // The European squad's other views are other tables (thirteen columns): the layout must leave them as built.
                bool other_table = false;

                // Every number button, every visible list row and the buttons are reachable through the game's own picker.
                const auto pick = [&] {
                    for (unsigned i = 0; i < t.controls(); ++i) {
                        const int owner = c.i16(t.control(i) + 0x72);
                        const bool ours = owner == shape.low || owner == shape.high || owner == shape.list || owner == shape.buttons || owner == shape.footer;
                        // Action 5 is the drop of the original's drag (a number button, and a player's number once he has one): its
                        // click picker never answers for them, so they are checked as rectangles (no overlap) rather than by a click.
                        if (c.u32(t.control(i) + 8) == 5) continue;
                        if (!ours || !c.u32(t.control(i) + 8) || !t.enabled(i)) continue;
                        const auto box = t.rect(t.control(i) + 0x10);
                        if (box.top == 0 && box.bottom == 0) continue;
                        if (owner == shape.list && int(c.u32(t.control(i) + 0x24)) >= int(c.u8(t.grid(unsigned(shape.list)) + 0xbb1))) continue;   // behind the scrollbar
                        const auto got = t.hit(i);
                        if (got != int(i)) throw std::runtime_error("Squad numbers picker missed an original control: " + std::to_string(i) + " (owner " + std::to_string(owner) + ", action " + std::to_string(c.u32(t.control(i) + 8)) + ", row " + std::to_string(c.u32(t.control(i) + 0x24)) + ") hit " + std::to_string(got));
                    }
                };
                const auto build = [&](GuiCanvas size) {
                    t.check_gate(on, "Squad numbers gate disagrees with preference");
                    t.rebuild(size);
                    shape = squad_shape(t);
                    if (other_table) {
                        require(shape.list < 0, "The European squad's other view still has the squad list");
                        old = t.layout(records, "Squad numbers other view");
                        require(t.s.memory.copy_out(t.display, 0x129da8) == old, "The layout moved a table it does not know");
                        return;
                    }
                    require(shape.menus >= 0 && shape.buttons >= 0 && shape.list >= 0 && shape.footer >= 0 && shape.heading >= 0 &&
                            shape.european == (item.opener == 0x1004a200) && (shape.low >= 0) == !shape.european && (shape.counter >= 0) == shape.european,
                            ("Original squad numbers grids absent: " + std::to_string(shape.menus) + " " + std::to_string(shape.buttons) + " " + std::to_string(shape.low) + " " + std::to_string(shape.high) + " " + std::to_string(shape.list) + " " + std::to_string(shape.footer) + " " + std::to_string(shape.heading) + " " + std::to_string(shape.counter)).c_str());
                    old = t.layout(records, "Squad numbers");
                    if (!on || size.original()) { require(t.s.memory.copy_out(t.display, 0x129da8) == old, "Legacy squad numbers changed"); pick(); return; }
                    verify_squad_numbers(t, size, shape, old, "Squad numbers");
                    pick();
                };
                const auto handler = [&](unsigned control) { return std::int32_t(t.invoke(c.u32(c.u32(t.history() + 8)), {control})); };
                const GuiCanvas canvas = adaptive ? GuiCanvas{1280, 900} : GuiCanvas{800, 600};
                build(canvas);
                const auto opened = t.meanings();
                std::vector<std::uint8_t> outcome;
                const auto note = [&](std::int32_t result) { for (unsigned shift : {24u, 16u, 8u, 0u}) outcome.push_back(std::uint32_t(result) >> shift); };
                // The pop-up menus' items are controls without a rectangle until their menu opens: the host delivers them by index
                // (`popup N`), and so does this fixture. Buttons are clicked where the original picker finds them.
                const auto menu_item = [&](unsigned action, unsigned nth) {
                    unsigned seen = 0;
                    for (unsigned i = 0; i < t.controls(); ++i)
                        if (c.u32(t.control(i) + 8) == action && t.enabled(i) && c.i16(t.control(i) + 0x72) >= 0 && c.i16(t.control(i) + 0x72) != shape.list && seen++ == nth) return int(i);
                    return -1;
                };
                const auto choose = [&](int control, const char* what) {
                    require(control >= 0, what);
                    note(handler(unsigned(control)));
                    build(canvas);
                };
                const auto click = [&](int control, const char* what) {
                    require(control >= 0 && t.hit(unsigned(control)) == control, what);
                    choose(control, what);
                };
                if (scenario == 3) {
                    choose(menu_item(6, 3), "Squad numbers have no such sort");
                    require(t.meanings() != opened, "Sorting changed nothing the fixture can see");
                }
                if (scenario == 4) {
                    click(t.find(3), "Squad numbers have no Auto button");
                    require(t.meanings() != opened, "Auto changed nothing the fixture can see");
                    click(t.find(4), "Squad numbers have no Clear button");
                }
                if (scenario == 5) {
                    other_table = true;
                    choose(menu_item(1, 1), "European squad has no such view");
                    require(t.meanings() != opened, "Changing the view changed nothing the fixture can see");
                }
                if (adaptive) {
                    // Only a career is a state the layout handles.
                    c.put32(t.manager + 4, 0);
                    require(!adaptive_screen_available(t.s, 0x1004a4a0), "Squad numbers gate accepted a missing instance");
                    c.put32(t.manager + 4, t.instance);
                    for (const auto size : transfer_canvases()) build(size);
                    build({1800, 1035});
                    build(canvas);
                    // The original in-place refresh keeps the layout's bounds.
                    std::vector<std::uint8_t> boxes, again;
                    const auto sample = [&](std::vector<std::uint8_t>& out) {
                        out.clear();
                        for (unsigned k = 0; k < t.controls(); ++k) { const auto part = t.s.memory.copy_out(t.control(k) + 0x10, 16); out.insert(out.end(), part.begin(), part.end()); }
                    };
                    sample(boxes);
                    for (unsigned k = 0; k < t.grids(); ++k) { t.invoke(0x10714630, {t.grid(k)}); t.invoke(0x10714110, {t.grid(k), 0}); }
                    sample(again);
                    require(again == boxes, "Squad numbers redraw lost the layout's control bounds");
                    if (!other_table) {
                    // A screen that lost its heading is not this recipe's: the classic geometry stays.
                    {
                        t.rebuild(canvas);
                        const auto heading_at = t.control(unsigned(shape.heading));
                        t.c.put32(heading_at + 0x14, t.c.i32(heading_at + 0x14) + 1);
                        const auto classic = t.s.memory.copy_out(t.display, 0x129da8);
                        layout_built_screen(t.s, t.display, 0x1004a4a0);
                        require(t.s.memory.copy_out(t.display, 0x129da8) == classic, "Squad numbers laid out a screen without its heading");
                    }
                    build(canvas);
                    // Mutants: each geometric promise, broken in the guest, must be refused by the checker.
                    const auto verify = [&] { verify_squad_numbers(t, canvas, shape, old, "Squad numbers"); };
                    const auto saved = t.s.memory.copy_out(t.display, 0x129da8);
                    const auto restore = [&] { t.s.memory.copy_in(t.display, saved); };
                    verify();
                    const auto put_rect = [&](std::uint32_t at, ScreenRect box) { t.c.put32(at, box.left); t.c.put32(at + 4, box.top); t.c.put32(at + 8, box.right); t.c.put32(at + 12, box.bottom); };
                    {   // the panel stays at its classic rectangle
                        put_rect(t.grid(unsigned(shape.list)), shape.european ? ScreenRect{110, 210, 780, 495} : ScreenRect{110, 235, 780, 510});
                        require_rejected(verify, "list left at classic coordinates"); restore();
                    }
                    {   // the Auto/Clear grid stays at its classic place
                        put_rect(t.grid(unsigned(shape.buttons)), {530, 145, 780, 165});
                        require_rejected(verify, "Auto/Clear left at classic coordinates"); restore();
                    }
                    {   // the second button of the footer drifts right
                        for (unsigned k = 0; k < t.controls(); ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.footer && c.u32(t.control(k) + 0x20) == 1) { c.put32(t.control(k) + 0x10, c.i32(t.control(k) + 0x10) + 5); c.put32(t.control(k) + 0x18, c.i32(t.control(k) + 0x18) + 5); }
                        require_rejected(verify, "footer button drifted"); restore();
                    }
                    {   // the heading stays where the classic screen has it
                        const auto at = t.control(unsigned(shape.heading));
                        put_rect(at + 0x10, {110, 170, 780, 205}); c.put32(at + 0x28, 110); c.put32(at + 0x2c, 780);
                        require_rejected(verify, "heading left at its classic place"); restore();
                    }
                    {   // a list cell keeps text bounds of the classic list
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.list && c.u32(t.control(k) + 0x20) == 2 && c.u32(t.control(k) + 0x24) == 3) { c.put32(t.control(k) + 0x28, 175); done = true; }
                        require(done, "Squad numbers fixture found no cell to mutate");
                        require_rejected(verify, "list cell left at classic bounds"); restore();
                    }
                    if (!shape.european) {   // a number button leaves its row
                        const int number = t.find(5);
                        require(number >= 0, "Squad numbers fixture found no number button to mutate");
                        c.put32(t.control(unsigned(number)) + 0x14, c.i32(t.control(unsigned(number)) + 0x14) + 30); c.put32(t.control(unsigned(number)) + 0x1c, c.i32(t.control(unsigned(number)) + 0x1c) + 30);
                        require_rejected(verify, "a number button left its row"); restore();
                    } else {   // the counter stays at its classic place
                        const auto at = t.control(unsigned(shape.counter));
                        put_rect(at + 0x10, {110, 500, 780, 535}); c.put32(at + 0x28, 110); c.put32(at + 0x2c, 780);
                        require_rejected(verify, "counter left at its classic place"); restore();
                    }
                    {   // the list's row pitch is uneven
                        bool done = false;
                        for (unsigned k = 0; k < t.controls() && !done; ++k)
                            if (c.i16(t.control(k) + 0x72) == shape.list && c.u32(t.control(k) + 0x24) == 5) { c.put32(t.control(k) + 0x14, c.i32(t.control(k) + 0x14) + 4); c.put32(t.control(k) + 0x1c, c.i32(t.control(k) + 0x1c) + 4); done = true; }
                        require(done, "Squad numbers fixture found no row to mutate");
                        require_rejected(verify, "a row is off its pitch"); restore();
                    }
                    verify();
                    }
                    on = false; select_new_screens(false); build(canvas);
                }
                // Dismissing through the original Back: the same result, records, numbers and texts with and without the layout.
                const auto saved = records();
                const auto texts = t.meanings();
                const int back = t.find(11);
                require(back >= 0 && t.hit(unsigned(back)) == back, "Squad numbers Back button missing");
                note(handler(unsigned(back)));
                require(records() == saved, "Squad numbers Back changed records");
                outcome.insert(outcome.end(), saved.begin(), saved.end()); outcome.insert(outcome.end(), texts.begin(), texts.end());
                if (!adaptive) baseline = outcome;
                else {
                    std::size_t at = 0; while (at < outcome.size() && at < baseline.size() && outcome[at] == baseline[at]) ++at;
                    require(outcome == baseline, ("Adaptive squad numbers changed the original command outcome at byte " + std::to_string(at) + " of " + std::to_string(outcome.size())).c_str());
                }
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Squad numbers scenario ") + names[scenario] + (adaptive ? " adaptive: " : " original: ") + e.what());
            }
        }
        std::cout << "squad numbers state " << scenario << " (" << names[scenario] << "): palette, list rows and columns, footer, original input, records/RNG and guest ABI preserved" << std::endl;
    }
    require(squad_scrolling_layouts && squad_whole_layouts, "The fixture did not reach both a scrolling list and a list that shows the whole squad");
    std::cout << "squad numbers lists: " << squad_scrolling_layouts << " layouts with a scrollbar, " << squad_whole_layouts << " showing the whole squad" << std::endl;
    initialize_new_screens();
}

#include "paged_screen_layout.hpp"

namespace cm::aot {
// Squad Numbers and the European Squad (CLUB_SCREENS.CPP, builder 0x1004a4a0, handler 0x10052740, opened by 0x10049f80 and
// 0x1004a200 from the news, and by 0x10049c10 before a match). One builder draws both: history 2 is 1 for the squad numbers of the
// forthcoming (or the current) season and 2 for the European squad. The classic screen is, top to bottom: a row of two pop-up
// menus (View, Sort by) at 110,145-360,165 and the Auto / Clear buttons at 530,145-780,165; a heading with the current sort at
// 110,170-780,205; a strip of the numbers 1-25 at 110,210-780,230; the squad as eight columns (number, status marker, name,
// positions, twice) of twelve visible rows with a scrollbar at 110,235-780,510 (the builder made cells for every player, twenty
// rows for forty, and the rest wait hidden behind the scrollbar); a strip of 26-50 at 110,515-780,535; and the Back / Submit footer. The
// European squad has no strips: its list is at 110,210-780,495 with a counter line (how many are selected) at 110,500-780,535.
//
// This pass keeps every control, text, command and record of the builder and moves their rectangles. The panel is centered
// and at most 960 wide; the menus keep the left edge and the buttons the right edge of one header row; the fifty numbers are
// one palette of two rows (1-25 over 26-50) directly under that row; the heading and the list follow, and the list shows as
// many of its rows as the canvas can hold, up to the whole squad (the scrollbar goes when everything fits); the footer is the
// shared compact Back/Submit pair at the panel's right edge. Rows are 21 to 24 px apart, equal across the list. Each half of
// a row (number box, marker, name, positions) keeps the classic widths of its boxes, a name takes the width the panel adds, and
// a gap grows between the two halves. The European squad is the same without the palette, and its counter follows the list.
// The pass declines (classic geometry) when a grid or the heading is missing or duplicated, when a list cell already has
// explicit bounds, when the panel is too narrow to keep a name 100 px wide, or when the canvas is too short for twelve rows.
void layout_squad_numbers(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect menus_box{110, 145, 360, 165}, buttons_box{530, 145, 780, 165}, heading_box{110, 170, 780, 205},
                         low_strip_box{110, 210, 780, 230}, high_strip_box{110, 515, 780, 535},
                         numbers_list_box{110, 235, 780, 510}, european_list_box{110, 210, 780, 495}, counter_box{110, 500, 780, 535};
    const ScreenLayout shell{canvas};
    int menus = -1, buttons = -1, low_strip = -1, high_strip = -1, list = -1, footer = -1, heading = -1, counter = -1;
    bool ambiguous = false, european = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        const unsigned columns = screen.columns(grid), rows = screen.rows(grid);
        if (box == menus_box && columns == 2 && rows == 1) claim(menus, i);
        else if (box == buttons_box && columns == 2 && rows == 1) claim(buttons, i);
        else if (box == low_strip_box && columns == 25 && rows == 1) claim(low_strip, i);
        else if (box == high_strip_box && columns == 25 && rows == 1) claim(high_strip, i);
        else if ((box == numbers_list_box || box == european_list_box) && columns == 8 && rows == 12) {
            claim(list, i); european = box == european_list_box;
        } else if (is_footer(screen, grid, shell)) claim(footer, i);
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) >= 0) continue;
        const auto box = screen.rect(at + 0x10);
        if (box == heading_box && c.i32(at + 0x28) == heading_box.left && c.i32(at + 0x2c) == heading_box.right) claim(heading, i);
        else if (box == counter_box && c.i32(at + 0x28) == counter_box.left && c.i32(at + 0x2c) == counter_box.right) claim(counter, i);
    }
    if (ambiguous || menus < 0 || buttons < 0 || list < 0 || footer < 0 || heading < 0) return;
    if (european ? (low_strip >= 0 || high_strip >= 0 || counter < 0) : (low_strip < 0 || high_strip < 0 || counter >= 0)) return;
    const auto list_grid = screen.grid(list);
    const int original_rows = screen.rows(list_grid);
    // The builder made cells for rows+1 up to its last scrolled row; those are the rows the canvas may show.
    const int total_rows = std::clamp(std::max<int>(original_rows, c.i32(list_grid + 0x14) + 1), original_rows, 30);

    // The panel: centered, at most 960 wide, from the classic content top down to the classic gap above the footer.
    constexpr int gap = 6, strip_height = 24, strip_gap = 2, heading_height = 35, header_height = 20, counter_height = 35, frame = 16;
    constexpr int minimum_pitch = 21, preferred_pitch = 24;
    auto available = shell.content(menus_box.top);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const int bottom = shell.content(menus_box.top).bottom - 10;
    int y = menus_box.top + header_height + gap;
    const int low_strip_top = y, high_strip_top = y + strip_height + strip_gap;
    if (!european) y += 2 * strip_height + strip_gap + gap;
    const int heading_top = european ? heading_box.top : y;
    const int list_top = european ? european_list_box.top : y + heading_height;
    const int list_room = bottom - list_top - (european ? 5 + counter_height : 0);
    if (list_room < original_rows * minimum_pitch + frame) return;
    const int rows = std::clamp((list_room - frame) / preferred_pitch, original_rows, std::max(original_rows, total_rows));
    const int pitch = std::clamp((list_room - frame) / rows, minimum_pitch, preferred_pitch);
    const ScreenRect panel{available.left, list_top, available.right, list_top + rows * pitch + frame};
    const int fits = c.i32(list_grid + 0x14) + 1 <= rows;
    // Inside the grid AutoGrid keeps a 2px frame and, with a scrollbar, a 22px gutter.
    constexpr int number_width = 32, marker_width = 27, positions_width = 89, cell_gap = 2;
    const int inner = panel.width() - 4 - (fits ? 0 : 22);
    const int halves_gap = cell_gap + std::clamp((inner - 2 * 321 - cell_gap) / 4, 0, 22);
    const int half = (inner - halves_gap) / 2;
    const int name_width = half - number_width - marker_width - positions_width - 3 * cell_gap;
    if (name_width < 100) return;
    // Each column's [left, right) as offsets from the grid's left edge.
    std::array<std::pair<int, int>, 8> column{};
    for (int side = 0, left = 2; side < 2; ++side, left += half + halves_gap) {
        int x = left;
        for (const auto [index, size] : {std::pair{0, number_width}, {1, marker_width}, {2, name_width}, {3, positions_width}}) {
            column[side * 4 + index] = {x, x + size};
            x += size + cell_gap;
        }
    }
    // Cells of the list start without bounds of their own; a screen that gave them some is not this recipe's.
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) == list && (c.i32(at + 0x28) || c.i32(at + 0x2c) || c.u32(at + 0x20) >= 8)) return;
    }

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    const auto place = [&](int grid, ScreenRect box) {
        screen.write(screen.grid(unsigned(grid)), box);
        screen.measure(screen.grid(unsigned(grid)));
        screen.publish(screen.grid(unsigned(grid)));
    };
    place(menus, {panel.left, menus_box.top, panel.left + menus_box.width(), menus_box.bottom});
    place(buttons, {panel.right - buttons_box.width(), buttons_box.top, panel.right, buttons_box.bottom});
    if (!european) {
        place(low_strip, {panel.left, low_strip_top, panel.right, low_strip_top + strip_height});
        place(high_strip, {panel.left, high_strip_top, panel.right, high_strip_top + strip_height});
    }
    const auto retitle = [&](int control, int top, int height) {
        const auto at = screen.control(unsigned(control));
        screen.write(at + 0x10, {panel.left, top, panel.right, top + height});
        c.put32(at + 0x28, panel.left); c.put32(at + 0x2c, panel.right);
    };
    retitle(heading, heading_top, heading_height);
    if (european) retitle(counter, panel.bottom + 5, counter_height);

    screen.write(list_grid, panel);
    screen.show_rows(list_grid, original_rows, rows);
    screen.set_rows(list_grid, rows);
    screen.hide_scrollbar_if_fits(list_grid, rows);
    screen.resize_rows(list_grid);
    screen.measure(list_grid);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != list) continue;
        const auto [first, last] = column[c.u32(at + 0x20)];
        c.put32(at + 0x28, panel.left + first); c.put32(at + 0x2c, panel.left + last);
    }
    screen.publish(list_grid);
    place_footer(screen, c, screen.grid(unsigned(footer)), footer_box(shell, panel));
}
}

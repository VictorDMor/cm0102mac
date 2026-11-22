#include "paged_screen_layout.hpp"

namespace cm::aot {
// Disciplinary action on a player (builder 0x1010ff90, opener 0x1010fef0, opened from the player's action menu) and the
// press question (builder 0x1067bea0, opener 0x1067b920, opened from a news item: defend, criticize or say nothing, with a
// Cancel/Ok footer), which has the same shape: two
// grids over the same rectangle 110,140-780,545, one of seven columns (the intro line with the player's name as a link,
// the action row "label - level +" and the reason heading) and one of two columns (the five reasons in the wide
// column), and a Cancel/Confirm footer of two columns at 100,555-790,590. The original builder keeps every text,
// level, reason and command; this pass makes the same kind of dialog as the other transfer screens: both grids as one
// panel centered under the title bars and at most 800 wide, trailing empty rows dropped so it is only as tall as its
// content, the wide column of each grid taking the extra width, the full intro line following both edges, and the
// buttons as equal 190px cells right-aligned under the panel. It declines (classic geometry) any other arrangement.
static void layout_two_panels(State& s, std::uint32_t display, GuiCanvas canvas, unsigned first_columns, unsigned second_columns) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110, 140, 780, 545}, footer_classic{100, 555, 790, 590};
    int wide = -1, narrow = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const ScreenLayout shell{canvas};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        // The two panels are told apart by their order (the screens' first grid holds the intro, heading or action row).
        if (box == panel_box) claim(wide < 0 ? wide : narrow, i);
        else if ((box == footer_classic || box == shell.footer()) && screen.rows(grid) == 1 && screen.columns(grid) == 2) claim(footer, i);
    }
    if (ambiguous || wide < 0 || narrow < 0 || footer < 0) return;
    if (screen.columns(screen.grid(unsigned(wide))) != first_columns || screen.columns(screen.grid(unsigned(narrow))) != second_columns) return;
    const auto wide_grid = screen.grid(wide), narrow_grid = screen.grid(narrow), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(wide_grid);
    // The two grids are one panel: same rows, no scrollbar (it narrows the columns and is not ours to move).
    if (!rows || rows > 30 || screen.rows(narrow_grid) != rows || c.u8(wide_grid + 0xbef) || c.u8(narrow_grid + 0xbef)) return;
    unsigned last = 0;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        if (owner != wide && owner != narrow) continue;
        const unsigned row = c.u32(at + 0x24);
        if (row >= rows) return;
        last = std::max(last, row);
    }
    // The rows after the last reason stay out of the panel, with one empty row of padding.
    const unsigned count = std::min(rows, last + 2);

    constexpr int row_height = 28, gap = 10, button_height = 35;
    auto available = shell.content(panel_box.top);
    const int width = std::min(800, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const int panel_height = int(count) * row_height + 16;
    // The original row height is about 21 pixels; a panel that cannot keep that stays as built.
    if (int(count) * 21 + 16 + gap + button_height > available.height()) return;
    const ScreenRect body{available.left, available.top, available.right, available.top + panel_height};
    const ScreenRect buttons_box{body.right - std::min(body.width(), 2 * 190), body.bottom + gap, body.right, body.bottom + gap + button_height};
    const int lead = body.left - panel_box.left, trail = body.right - panel_box.right;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(wide_grid);
    screen.measure(narrow_grid);
    ColumnMap wide_map, narrow_map;
    // A grid whose controls all carry their own bounds (the press question's three lines) needs no columns: its
    // seven may even include an empty one the map refuses.
    const bool wide_columns = wide_map.read(c, screen, wide_grid, panel_box.width(), body), narrow_columns = narrow_map.read(c, screen, narrow_grid, panel_box.width(), body);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        if ((owner == wide && !wide_columns) || (owner == narrow && !narrow_columns))
            if (!c.i32(at + 0x28) || !c.i32(at + 0x2c)) return;
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        if (owner != wide && owner != narrow) continue;
        const int left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        const auto& map = owner == wide ? wide_map : narrow_map;
        if (left && right) {
            // The intro line follows the panel's right edge; the player's name, a link measured to its words, keeps its width.
            c.put32(at + 0x28, left + lead);
            c.put32(at + 0x2c, right >= panel_box.right - 10 ? right + trail : right + lead);
        } else if (c.u32(at + 0x20) < map.count()) {
            const auto [first, last_x] = map.cell(c.u32(at + 0x20));
            c.put32(at + 0x28, first); c.put32(at + 0x2c, last_x);
        }
    }
    // Rows beyond the compacted panel do not exist any more; the used-row range and scroll position stay.
    for (const auto grid : {wide_grid, narrow_grid}) {
        const auto first_used = c.u32(grid + 0x10), last_used = c.u32(grid + 0x14), scroll = c.u32(grid + 0x1fc);
        screen.set_rows(grid, count);
        screen.resize_rows(grid);
        c.put32(grid + 0x10, first_used); c.put32(grid + 0x14, last_used); c.put32(grid + 0x1fc, scroll);
        screen.write(grid, body);
        screen.measure(grid);
        screen.publish(grid);
    }
    place_footer(screen, c, footer_grid, buttons_box);
}

// The disciplinary action on a player: an intro/action row grid of seven columns over the reasons' grid of two.
void layout_disciplinary(State& s, std::uint32_t display, GuiCanvas canvas) { layout_two_panels(s, display, canvas, 7, 2); }
// Choosing a player's role in the club (builder 0x100952b0, opener 0x10094ae0): a heading grid and the options' grid, both
// of two columns over the same rectangle, and the Back/Choose footer, which this pass places like the dialogs' buttons.
void layout_role_choice(State& s, std::uint32_t display, GuiCanvas canvas) { layout_two_panels(s, display, canvas, 2, 2); }
}

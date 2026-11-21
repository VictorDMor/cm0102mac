#include "paged_screen_layout.hpp"
#include "gui_guest_scope.hpp"

namespace cm::aot {
// Manager Statistics (builder 0x101c4460): a table of sixteen rows (rank, manager, club, status) for the
// nation chosen in the menu under the subtitle, a pager above its right edge and the Back/Next footer.
// Nations without statistics build a one-column table holding one centered message. The original builder
// keeps every text, link, command and menu; this pass moves the table, the chosen-nation control, the
// pager and the footer, and lets only the manager column take the extra width.
void layout_manager_statistics(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110, 170, 780, 535};
    const ScreenLayout shell{canvas};
    int table = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        if (screen.rect(grid) == panel_box && (screen.columns(grid) == 4 || screen.columns(grid) == 1)) claim(table, i);
        else if (is_footer(screen, grid, shell)) claim(footer, i);
    }
    if (ambiguous || table < 0 || footer < 0) return;
    const auto table_grid = screen.grid(table), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(table_grid);
    // A scrollbar would change the column widths measured below; these pages never have one.
    if (!rows || rows > 30 || c.u8(table_grid + 0xbef)) return;
    const auto panel = paged_screen::body(shell, panel_box, rows);

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    screen.measure(table_grid);
    ColumnMap columns;
    if (!columns.read(c, screen, table_grid, panel_box.width(), panel)) return;
    const int center = (panel.left + panel.right - panel_box.left - panel_box.right) / 2;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != table) continue;
        const int left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (left && right) {
            // The empty-table message is centered text with measured bounds.
            c.put32(at + 0x28, left + center); c.put32(at + 0x2c, right + center);
            continue;
        }
        const unsigned column = c.u32(at + 0x20);
        if (column >= columns.count()) continue;
        const auto [first, last] = columns.cell(column);
        c.put32(at + 0x28, first); c.put32(at + 0x2c, last);
    }
    move_chrome(screen, c, controls, shell, panel_box, panel);
    screen.write(table_grid, panel);
    screen.measure(table_grid);
    screen.publish(table_grid);
    place_footer(screen, c, footer_grid, footer_box(shell, panel));
}
}

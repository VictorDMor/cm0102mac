#include "paged_screen_layout.hpp"

namespace cm::aot {
// "Last meetings" between two clubs (builder 0x10041550, opener 0x100413b0(club, club), the club screen's head-to-head view):
// a toolbar row (a view menu on the left and a filter menu on the right, standalone controls), a small summary table of eleven
// columns (position, name and the played/won/drawn/lost/for/against/points counts) of three rows at 110,170-780,249, the table of
// meetings of seven columns and fifteen rows at 110,254-780,535 and the Back/Next footer. The summary has one column AutoGrid
// measured as empty. The original builder keeps every text, link, row and command; this pass makes both tables one panel
// centered under the title bars and at most 960 wide, the summary at its own height and the meetings taking the extra height
// (their rows no taller than 28 pixels), the widest column of each table taking the extra width, the toolbar menus following
// the panel's edges and the Back/Next buttons right-aligned to it. It declines (classic geometry) any other arrangement.
void layout_head_to_head(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect summary_box{110, 170, 780, 249}, meetings_box{110, 254, 780, 535};
    const ScreenLayout shell{canvas};
    int summary = -1, meetings = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        if (screen.rect(grid) == summary_box && screen.columns(grid) == 11) claim(summary, i);
        else if (screen.rect(grid) == meetings_box && screen.columns(grid) == 7) claim(meetings, i);
        else if (is_footer(screen, grid, shell)) claim(footer, i);
    }
    if (ambiguous || summary < 0 || meetings < 0 || footer < 0) return;
    const auto summary_grid = screen.grid(unsigned(summary)), meetings_grid = screen.grid(unsigned(meetings)), footer_grid = screen.grid(unsigned(footer));
    const unsigned rows = screen.rows(meetings_grid);
    if (!rows || rows > 30 || screen.rows(summary_grid) > 8) return;

    auto available = shell.anchor(meetings_box, LayoutAxis::stretch, LayoutAxis::stretch);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const std::array<FormTrack, 1> section{{{meetings_box.height(), 1, int(rows) * 28 + 16}}};
    const auto meetings_panel = form_tracks(available, section, false).front();
    const ScreenRect summary_panel{available.left, summary_box.top, available.right, summary_box.bottom};

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(summary_grid);
    screen.measure(meetings_grid);
    ColumnMap summary_map, meetings_map;
    if (!summary_map.read(c, screen, summary_grid, summary_box.width(), summary_panel, 0, true) ||
        !meetings_map.read(c, screen, meetings_grid, meetings_box.width(), meetings_panel)) return;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        if (owner != summary && owner != meetings) continue;
        const auto& map = owner == summary ? summary_map : meetings_map;
        const auto& panel = owner == summary ? summary_panel : meetings_panel;
        const auto& box = owner == summary ? summary_box : meetings_box;
        if (c.i32(at + 0x28) || c.i32(at + 0x2c)) {
            // Text with measured bounds stays centered in its table.
            const int center = (panel.left + panel.right - box.left - box.right) / 2;
            c.put32(at + 0x28, c.i32(at + 0x28) + center); c.put32(at + 0x2c, c.i32(at + 0x2c) + center);
            continue;
        }
        const unsigned column = c.u32(at + 0x20);
        if (column >= map.count()) continue;
        const auto [first, last] = map.cell(column);
        c.put32(at + 0x28, first); c.put32(at + 0x2c, last);
    }
    // The menus above the tables keep their side of the panel (the subtitle follows the shell).
    move_chrome(screen, c, controls, shell, summary_box, summary_panel);
    screen.write(summary_grid, summary_panel);
    screen.measure(summary_grid);
    screen.publish(summary_grid);
    screen.write(meetings_grid, meetings_panel);
    screen.measure(meetings_grid);
    screen.publish(meetings_grid);
    place_footer(screen, c, footer_grid, footer_box(shell, meetings_panel));
}
}

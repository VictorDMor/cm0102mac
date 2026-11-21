#include "paged_screen_layout.hpp"
#include "gui_guest_scope.hpp"

namespace cm::aot {
// Board Confidence (builder 0x101c8fb0): a one-column panel of sixteen rows holding the board's message
// as lines the builder already wrapped, two request links above it and the Back/Next footer. The original
// builder keeps every text, link and command; this pass moves the panel, the links and the footer, and
// carries each line with the panel's left edge (its right bound follows the panel's right edge).
void layout_board_confidence(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110, 155, 780, 540};
    const ScreenLayout shell{canvas};
    int message = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        if (screen.rect(grid) == panel_box && screen.columns(grid) == 1) claim(message, i);
        else if (is_footer(screen, grid, shell)) claim(footer, i);
    }
    if (ambiguous || message < 0 || footer < 0) return;
    const auto message_grid = screen.grid(message), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(message_grid);
    if (!rows || rows > 30 || c.u8(message_grid + 0xbef)) return;
    const auto panel = paged_screen::body(shell, panel_box, rows);
    const int lead = panel.left - panel_box.left, trail = panel.right - panel_box.right;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != message) continue;
        const int left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (!left || !right) continue;
        // A full line spans the panel's text area; anything shorter is a fragment that keeps its width.
        c.put32(at + 0x28, left + lead);
        c.put32(at + 0x2c, right >= panel_box.right - 10 ? right + trail : right + lead);
    }
    move_chrome(screen, c, controls, shell, panel_box, panel);
    screen.write(message_grid, panel);
    screen.measure(message_grid);
    screen.publish(message_grid);
    place_footer(screen, c, footer_grid, footer_box(shell, panel));
}
}
